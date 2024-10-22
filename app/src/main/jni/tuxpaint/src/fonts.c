/*
  fonts.c

  Copyright (c) 2009-2023
  https://tuxpaint.org/

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  (See COPYING.txt)

  Last modified: June 6, 2024
*/

#include <stdio.h>
#ifndef __USE_GNU
#define __USE_GNU               /* for strcasestr() */
#endif
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#include <errno.h>

#include <libintl.h>
#ifndef gettext_noop
#define gettext_noop(String) String
#endif

/*
	The following section renames global variables defined in SDL2_Pango.h to avoid errors during linking.
	It is okay to rename these variables because they are constants.
	SDL2_Pango.h is included by fonts.h.  
*/
#define _MATRIX_WHITE_BACK _MATRIX_WHITE_BACK2
#define MATRIX_WHITE_BACK MATRIX_WHITE_BACK2
#define _MATRIX_BLACK_BACK _MATRIX_BLACK_BACK2
#define MATRIX_BLACK_BACK MATRIX_BLACK_BACK2
#define _MATRIX_TRANSPARENT_BACK_BLACK_LETTER _MATRIX_TRANSPARENT_BACK_BLACK_LETTER2
#define MATRIX_TRANSPARENT_BACK_BLACK_LETTER MATRIX_TRANSPARENT_BACK_BLACK_LETTER2
#define _MATRIX_TRANSPARENT_BACK_WHITE_LETTER _MATRIX_TRANSPARENT_BACK_WHITE_LETTER2
#define MATRIX_TRANSPARENT_BACK_WHITE_LETTER MATRIX_TRANSPARENT_BACK_WHITE_LETTER2
#define _MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER _MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER2
#define MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER2
/*
	The renaming ends here.
*/

#include "platform.h"
#include "fonts.h"
#include "i18n.h"
#include "progressbar.h"
#include "dirwalk.h"
#include "get_fname.h"
#include "debug.h"

#ifdef WIN32
#include "win32_print.h"
extern char *strcasestr(const char *haystack, const char *needle);
#endif

#if defined(__MACOS__)
#include "macos.h"
#elif defined(__IOS__)
#include "ios.h"
#endif

#ifdef __HAIKU__
#include <FindDirectory.h>
#include <fs_info.h>
#define _POSIX_PRIORITY_SCHEDULING
#endif

/* system fonts that cause TTF_OpenFont to crash */
static const char *problemFonts[] = {
  "/Library/Fonts//AppleMyungjo.ttf",
  NULL
};

/* font types that cause TTF_OpenFont to crash */
static const char *problemFontExtensions[] = {
  ".pfb",                       /* Ubuntu 14.04 (libsdl-ttf2.0-0 2.0.11-3, libfreetype6 2.5.2-1ubuntu2) -bjk 2014.04.19 */
  NULL
};

#ifdef FORKED_FONTS

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/wait.h>

#if defined(_POSIX_PRIORITY_SCHEDULING) && !defined(__OS2__)
#include <sched.h>
#else
#define sched_yield()
#endif

#ifdef __linux__
#include <sys/prctl.h>
#else
#define prctl(o,a1)
#define PR_SET_PDEATHSIG 0
#endif

#endif

#ifndef FORKED_FONTS
SDL_Thread *font_thread;
#endif

#include "pango/pango.h"
#include "pango/pangoft2.h"
#if !defined(__PANGO_H__)
#error "---------------------------------------------------"
#error "If you installed pango from a package, be sure"
#error "to get the development package, as well!"
#error "(e.g., 'libpango1.0-dev.rpm')"
#error "---------------------------------------------------"
#endif

#include "SDL2_Pango.h"
#if !defined(SDL_PANGO_H)
#error "---------------------------------------------------"
#error "If you installed SDL2_Pango from a package, be sure"
#error "to get the development package, as well!"
#error "(e.g., 'SDL2_Pango-2.1.5-dev.rpm')"
#error "---------------------------------------------------"
#endif


#ifdef FORKED_FONTS
int no_system_fonts;
#else
int no_system_fonts = 1;
#endif
int all_locale_fonts;
volatile long font_thread_done;
volatile long font_thread_aborted;
volatile long waiting_for_fonts;

#ifdef FORKED_FONTS
static int font_scanner_pid;
#endif
int font_socket_fd;

TuxPaint_Font *medium_font, *small_font, *large_font, *locale_font;

family_info **user_font_families;
int num_font_families;
static int num_font_families_max;

style_info **user_font_styles;
int num_font_styles;
int num_font_styles_max;

int text_state;
unsigned text_size = 4;         // initial text size

int button_label_y_nudge;

#ifdef FORKED_FONTS
static void reliable_read(int fd, void *buf, size_t count);
#endif

const char *PANGO_DEFAULT_FONT = "DejaVu Sans";
const char *PANGO_DEFAULT_FONT_FALLBACK = NULL;

/* Names of the fonts we include in `fonts/locale/`
   (LANG_* codes are from `src/i18n.h`) */
// (Try `otfinfo --info fonts/locale/*.ttf | grep "Family:"`)
/* Note: When there's a preferred family name, and a fall-back,
   this is generally when Tux Paint ships a 'subset' font based
   on the strings used in the app, and we offer a separate
   download of the full (much larger) font TTF file.
   (https://tuxpaint.org/download/fonts/) */
default_locale_font_t default_local_fonts[] = {
  {LANG_AR, "ae_Nice", NULL},
  {LANG_BO, "Tsampa Keyboard", NULL},   // NOTE: Our current translation is Wylie transliterated, not Unicode! */
  {LANG_EL, "Thryomanes", NULL},
  {LANG_GU, "Lohit Gujarati", NULL},
  {LANG_HE, "Nachlieli Light", NULL},
  {LANG_HI, "Raghindi", NULL},
  {LANG_JA, "Noto Sans CJK JP", "GJGothicPNSubset"},
  {LANG_KA, "TuxPaint Georgian", NULL}, /* FIXME: Upon what was this font based? Never knew -bjk 2023.06.12 */
  {LANG_KO, "Baekmuk Gulim", NULL},
  {LANG_TA, "TSCu_Comic", NULL},
  {LANG_TE, "Vemana2000", NULL},
  {LANG_TH, "Garuda", NULL},
  {LANG_ZH_CN, "AR PL SungtiL GB", NULL},
  {LANG_ZH_TW, "HanWangKaiMediumChuIn", "SubsetForTuxPaint"},
  {-1, NULL, NULL},
};

void TuxPaint_Font_CloseFont(TuxPaint_Font *tpf)
{
#ifdef DEBUG
  printf("%s:%d - TuxPaint_Font_CloseFont step 1 (%p)\n", __FILE__, __LINE__, tpf);
#endif
  if (!tpf)
    return;

#ifdef DEBUG
  printf("%s:%d - TuxPaint_Font_CloseFont step 2 (%p, %d)\n", __FILE__, __LINE__, tpf->pango_context, tpf->typ);
#endif
  if (tpf->typ == FONT_TYPE_PANGO)
    if (tpf->pango_context)
    {
#ifndef __APPLE__               //EP added ifdef because SDLPango_FreeContext sometimes crashed with "pointer being freed was not allocated"
      SDLPango_FreeContext(tpf->pango_context);
#endif
      tpf->pango_context = NULL;
    }

#ifdef DEBUG
  printf("%s:%d - TuxPaint_Font_CloseFont step 3 (%p, %d)\n", __FILE__, __LINE__, tpf->ttf_font, tpf->typ);
  fflush(stdout);
#endif
  if (tpf->typ == FONT_TYPE_TTF)
    if (tpf->ttf_font)
    {
      TTF_CloseFont(tpf->ttf_font);
      tpf->ttf_font = NULL;
    }

  if (tpf->desc != NULL)
  {
    free(tpf->desc);
    tpf->desc = NULL;
  }

  free(tpf);
}

TuxPaint_Font *TuxPaint_Font_OpenFont(const char *pangodesc, const char *ttffilename, int size)
{
  TTF_Font *ttf_font;
  TuxPaint_Font *tpf = NULL;
  char *familyname;
  int i;
  char desc[1024];

#ifdef DEBUG
  printf("%s:%d - OpenFont(pango:\"%s\", ttf:\"%s\")\n", __FILE__, __LINE__, pangodesc, ttffilename);
#endif

  if (pangodesc != NULL && pangodesc[0] != '\0')
  {
    tpf = (TuxPaint_Font *) malloc(sizeof(TuxPaint_Font));
    tpf->typ = FONT_TYPE_PANGO;
    snprintf(desc, sizeof(desc), "%s %d", pangodesc, (size * 3) / 4);
    tpf->desc = strdup(desc);

#ifdef DEBUG
    printf("%s:%d - Creating Pango context: \"%s\"\n", __FILE__, __LINE__, desc);
#endif

#ifdef __APPLE__
    /*
     * SDLPango_CreateContext_GivenFontDesc() defaults to ASCII character set
     * (at least on the macOS) unless the CHARSET environment varaible is set.
     * May also want to set on non-macOS platforms, also.
     */

    mysetenv("CHARSET", "UTF-8");
#endif

    tpf->pango_context = SDLPango_CreateContext_GivenFontDesc(desc);
    if (tpf->pango_context == NULL)
    {
#ifdef DEBUG
      printf("%s:%d - Failed to load %s\n", __FILE__, __LINE__, desc);
#endif
      free(tpf);
      tpf = NULL;
    }
    else
      tpf->height = size;       /* FIXME: Is this accurate!? -bjk 2007.07.12 */

#ifdef DEBUG
    printf
      ("%s:%d - TuxPaint_Font_OpenFont(\"%s\", \"%s\", %d) done (SDL_Pango)\n\n",
       __FILE__, __LINE__, pangodesc, ttffilename, size);
    fflush(stdout);
#endif

    return (tpf);
  }


  /* -- Did not, at this point, load the font using SDL_Pango -- */

  if (ttffilename != NULL && ttffilename[0] != '\0')
  {
#ifdef DEBUG
    printf("%s:%d - Considering loading TTF \"%s\"\n", __FILE__, __LINE__, ttffilename);
    fflush(stdout);
#endif

    i = 0;
    while (problemFonts[i] != NULL)
    {
      if (!strcmp(ttffilename, problemFonts[i++]))
      {
#ifdef DEBUG
        fprintf(stderr, "Notice: Skipping problematic font: \"%s\"\n", ttffilename);
#endif
        return NULL;            /* bail on known problematic fonts that cause TTF_OpenFont to crash */
      }
    }

    i = 0;
    while (problemFontExtensions[i] != NULL)
    {
      if (strstr(ttffilename, problemFontExtensions[i++]))
      {
#ifdef DEBUG
        fprintf(stderr, "Notice: Skipping font with problematic extension: \"%s\"\n", ttffilename);
#endif
        return NULL;            /* bail on known problematic font types that cause TTF_OpenFont to crash */
      }
    }

    ttf_font = TTF_OpenFont(ttffilename, size);

    if (ttf_font == NULL)
    {
      fprintf(stderr, "Cannot open TTF font '%s' (size %d)\n", ttffilename, size);
      return NULL;
    }

    familyname = (char *)TTF_FontFaceFamilyName(ttf_font);      /* N.B.: I don't believe we're supposed to free() this... -bjk 2021.10.26 */
    (void)familyname;           // avoid compiler complaints if ALWAYS_LOAD_FONT_WITH_PANGO is not set, and DEBUG is not set

#ifdef DEBUG
    printf("%s:%d - Loaded %s (\"%s\")\n", __FILE__, __LINE__, ttffilename,
           (familyname != NULL ? familyname : "<NULL>"));
    fflush(stdout);
#endif


/* This is currently disabled, as it is under construction (and may be moot under SDL2) -bjk 2021.10.28 */
#ifdef ALWAYS_LOAD_FONT_WITH_PANGO
    /* -- Try loading the font with Pango, instead! */
    tpf = TuxPaint_Font_OpenFont(familyname, "", size);
    if (tpf != NULL)
    {
      /* Success! Clean up and return the TuxPaint_Font that we got back */
#ifdef DEBUG
      printf("%s:%d - Loaded via SDL_Pango!\n", __FILE__, __LINE__);
      printf
        ("%s:%d - TuxPaint_Font_OpenFont(\"%s\", \"%s\", %d) done (SDL_ttf -> SDL_Pango)\n\n",
         __FILE__, __LINE__, pangodesc, ttffilename, size);
      fflush(stdout);
#endif
      TTF_CloseFont(ttf_font);
      return (tpf);
    }
#endif


    /* -- Proceed with loading the TTF font file using SDL_ttf */

    tpf = (TuxPaint_Font *) malloc(sizeof(TuxPaint_Font));
    tpf->typ = FONT_TYPE_TTF;
    tpf->ttf_font = ttf_font;
    tpf->desc = strdup(ttffilename);

#ifdef DEBUG
    printf("%s:%d - Succeeded loading %s via SDL_ttf\n", __FILE__, __LINE__, ttffilename);
#endif
    tpf->height = TTF_FontHeight(tpf->ttf_font);

#ifdef DEBUG
    printf
      ("%s:%d - TuxPaint_Font_OpenFont(\"%s\", \"%s\", %d) done (SDL_ttf)\n\n",
       __FILE__, __LINE__, pangodesc, ttffilename, size);
    fflush(stdout);
#endif

    return (tpf);
  }

  fprintf(stderr, "TuxPaint_Font_OpenFont() called with no loadable font\n");
  return NULL;
}


#ifdef FORKED_FONTS

void reliable_write(int fd, const void *buf, size_t count)
{
  struct pollfd p;

  do
  {
    ssize_t rc = write(fd, buf, count);

    if (rc == -1)
    {
      switch (errno)
      {
      default:
        return;
      case EAGAIN:
      case ENOSPC:
        ;                       // satisfy a C syntax abomination
        p = (struct pollfd)
        {
          fd, POLLOUT, 0
        };
        poll(&p, 1, -1);        // try not to burn CPU time
        // FALL THROUGH
      case EINTR:
        continue;
      }
    }
    buf += rc;
    count -= rc;
  }
  while (count);
}


static void reliable_read(int fd, void *buf, size_t count)
{
  struct pollfd p;

  do
  {
    ssize_t rc = read(fd, buf, count);

    if (rc == -1)
    {
      switch (errno)
      {
      default:
        return;
      case EAGAIN:
        ;                       // satisfy a C syntax abomination
        p = (struct pollfd)
        {
          fd, POLLIN, 0
        };
        poll(&p, 1, -1);        // try not to burn CPU time
        // FALL THROUGH
      case EINTR:
        continue;
      }
    }
    if (rc == 0)
      break;                    // EOF. Better not happen before the end!
    buf += rc;
    count -= rc;
  }
  while (count);
}

#endif

static void groupfonts_range(style_info **base, int count)
{
  int boldcounts[4] = { 0, 0, 0, 0 };
  int boldmap[4] = { -1, -1, -1, -1 };
  int i;
  int boldmax;
  int boldmin;
  int bolduse;
  int spot;
  family_info *fi;

#if 0
// THREADED_FONTS
  if (count < 1 || count > 4)
  {
    printf("\n::::::: %d styles in %s:\n", count, base[0]->family);
    i = count;
    while (i--)
    {
      printf("               %s\n", base[i]->style);
    }
  }
#endif

  i = count;
  while (i--)
    boldcounts[base[i]->boldness]++;

  boldmax = base[0]->boldness;
  boldmin = base[0]->boldness;
  bolduse = 0;

  i = 4;
  while (i--)
  {
    if (!boldcounts[i])
      continue;
    if (i > boldmax)
      boldmax = i;
    if (i < boldmin)
      boldmin = i;
    bolduse++;
  }
  if (likely(bolduse <= 2))
  {
    // in case they are same, we want non-bold,
    // so that setting goes second
    boldmap[boldmax] = 1;
    boldmap[boldmin] = 0;
  }
  else if (count == 3)
  {
    int boldmid;
    int zmin = 0, zmid = 0, zmax = 0;

    boldmap[boldmax] = 1;
    boldmap[boldmin] = 0;
    boldmid = boldcounts[boldmin + 1] ? boldmin + 1 : boldmin + 2;

    i = 3;
    while (i--)
    {
      if (base[i]->boldness == boldmin)
        zmin = base[i]->italic;
      if (base[i]->boldness == boldmid)
        zmid = base[i]->italic;
      if (base[i]->boldness == boldmax)
        zmax = base[i]->italic;
    }
    if (zmin != zmid)
      boldmap[boldmid] = 0;
    else if (zmid != zmax)
      boldmap[boldmid] = 1;
    else if (boldmin == 0 && boldmid == 1)
    {
      boldmap[0] = -1;
      boldmap[1] = 0;
    }
  }
  else
  {
    int claimed_bold = boldcounts[3];
    int claimed_norm = boldcounts[1];

    // 3 or 4 boldness levels, 4 or more styles!
    // This is going to be random hacks and hopes.

    // bold is bold
    boldmap[3] = 1;

    // norm is norm
    boldmap[1] = 0;

    // classify demi-bold or medium
    if (claimed_bold < 2)
    {
      boldmap[2] = 1;
      claimed_bold += boldcounts[2];
    }
    else if (claimed_norm < 2)
    {
      boldmap[2] = 0;
      claimed_norm += boldcounts[2];
    }

    // classify lightface
    if (claimed_norm < 2)
    {
      boldmap[0] = 0;
      //claimed_norm += boldcounts[0];
    }
  }

  if (num_font_families == num_font_families_max)
  {
    num_font_families_max = num_font_families_max * 5 / 4 + 30;
    user_font_families = realloc(user_font_families, num_font_families_max * sizeof *user_font_families);
  }

  fi = calloc(1, sizeof *fi);
  user_font_families[num_font_families++] = fi;
  fi->directory = strdup(base[0]->directory);
  fi->family = strdup(base[0]->family);
  fi->score = base[0]->truetype + base[0]->score;
  i = count;
  while (i--)
  {
    int b = boldmap[base[i]->boldness];

    if (b == -1)
    {
#if 0
// THREADED_FONTS
      printf("too many boldness levels, discarding: %s, %s\n", base[i]->family, base[i]->style);
#endif
      continue;
    }
    spot = b ? TTF_STYLE_BOLD : 0;
    spot += base[i]->italic ? TTF_STYLE_ITALIC : 0;
    if (fi->filename[spot])
    {
#if 0
// THREADED_FONTS
      printf("duplicates, discarding: %s, %s\n", base[i]->family, base[i]->style);
      printf("b %d, spot %d\n", b, spot);
      printf("occupancy %p %p %p %p\n", fi->filename[0], fi->filename[1], fi->filename[2], fi->filename[3]);
#endif
      continue;
    }
    fi->filename[spot] = strdup(base[i]->filename);
    fi->score += 2;
  }
  if (!fi->filename[0] && !fi->filename[1])
  {
    fi->filename[0] = fi->filename[2];
    fi->filename[2] = NULL;
    fi->filename[1] = fi->filename[3];
    fi->filename[3] = NULL;
  }
  if (!fi->filename[0] && !fi->filename[2])
  {
    fi->filename[0] = fi->filename[1];
    fi->filename[1] = NULL;
    fi->filename[2] = fi->filename[3];
    fi->filename[3] = NULL;
  }
  if (!fi->filename[0])
  {
    fi->filename[0] = strdup(fi->filename[TTF_STYLE_BOLD]);
  }
}


// void qsort(void *base, size_t nmemb, size_t size,
// int(*compar)(const void *, const void *));


// For qsort() and other use, to see if font files are groupable
static int compar_fontgroup(const void *v1, const void *v2)
{
  const style_info *s1 = *(style_info **) v1;
  const style_info *s2 = *(style_info **) v2;
  int rc;

  rc = strcmp(s1->directory, s2->directory);
  if (rc)
    return rc;

  rc = s1->truetype - s2->truetype;
  if (rc)
    return rc;

  return strcmp(s1->family, s2->family);
}


// For qsort() and other use, to see if font files are duplicates
static int compar_fontkiller(const void *v1, const void *v2)
{
  const family_info *f1 = *(family_info **) v1;
  const family_info *f2 = *(family_info **) v2;
  int rc;

  rc = strcmp(f1->family, f2->family);
  if (rc)
    return rc;

  return f1->score - f2->score;
}


// For qsort() and other use, to order the worst ones last
static int compar_fontscore(const void *v1, const void *v2)
{
  const family_info *f1 = *(family_info **) v1;
  const family_info *f2 = *(family_info **) v2;

  return f2->score - f1->score;
}


// Font style names are a mess that we must try to make
// sense of. For example...
//
// Cooper: Light, Medium, Light Bold, Black
// HoeflerText: (nil), Black
static void parse_font_style(style_info *si)
{
  int have_light = 0;
  int have_demi = 0;
  int have_bold = 0;
  int have_medium = 0;
  int have_black = 0;
  int have_heavy = 0;

  int stumped = 0;
  char *sp = si->style;

  si->italic = 0;


  while (*sp)
  {
    if (*sp == ' ')
    {
      sp++;
      continue;
    }
    if (!strncasecmp(sp, "Bold", strlen("Bold")))
    {
      sp += strlen("Bold");
      have_bold = 1;
      continue;
    }
    if (!strncasecmp(sp, "Regular", strlen("Regular")))
    {
      sp += strlen("Regular");
      continue;
    }
    if (!strncasecmp(sp, "Italic", strlen("Italic")))
    {
      sp += strlen("Italic");
      si->italic = 1;
      continue;
    }
    if (!strncasecmp(sp, "Oblique", strlen("Oblique")))
    {
      sp += strlen("Oblique");
      si->italic = 1;
      continue;
    }
    // move " Condensed" from style to family
    if (!strncasecmp(sp, "Condensed", strlen("Condensed")))
    {
      size_t len = strlen(si->family);
      char *name = malloc(len + strlen(" Condensed") + 1);

      sp += strlen("Condensed");
      memcpy(name, si->family, len);
      strcpy(name + len, " Condensed");
      free(si->family);
      si->family = name;
      continue;
    }
    if (!strncasecmp(sp, "Light", strlen("Light")))
    {
      sp += strlen("Light");
      have_light = 1;
      continue;
    }
    if (!strncasecmp(sp, "Medium", strlen("Medium")))
    {
      sp += strlen("Medium");
      have_medium = 1;
      continue;
    }
    if (!strncasecmp(sp, "Demi", strlen("Demi")))
    {
      sp += strlen("Demi");
      have_demi = 1;
      continue;
    }
    if (!strncasecmp(sp, "Heavy", strlen("Heavy")))
    {
      sp += strlen("Heavy");
      have_heavy = 1;
      continue;
    }
    if (!strncasecmp(sp, "Normal", strlen("Normal")))
    {
      sp += strlen("Normal");
      continue;
    }
    if (!strncasecmp(sp, "Black", strlen("Black")))
    {
      sp += strlen("Black");
      have_black = 1;
      continue;
    }
    if (!strncasecmp(sp, "Roman", strlen("Roman")))
    {
      sp += strlen("Roman");
      continue;
    }
    if (!strncasecmp(sp, "Book", strlen("Book")))
    {
      sp += strlen("Book");
      continue;
    }
    if (!strncasecmp(sp, "Chancery", strlen("Chancery")))
    {
      sp += strlen("Chancery");
      si->italic = 1;
      continue;
    }
    if (!strncasecmp(sp, "Thin", strlen("Thin")))
    {
      sp += strlen("Thin");
      have_light = 1;
      continue;
    }
    if (!strncmp(sp, "LR", strlen("LR")))
    {
      sp += strlen("LR");
      continue;
    }

    if (!stumped)
    {
      stumped = 1;
#if 0
// THREADED_FONTS
      printf("Font style parser stumped by \"%s\".\n", si->style);
#endif
    }
    sp++;                       // bad: an unknown character
  }


  if (have_demi || have_medium)
    si->boldness = 2;
  else if (have_bold || have_black || have_heavy)       // TODO: split these
    si->boldness = 3;
  else if (have_light)
    si->boldness = 0;
  else
    si->boldness = 1;

  // we'll count both TrueType and OpenType
  si->truetype = !!strcasestr(si->filename, ".ttf") || !!strcasestr(si->filename, ".otf");
}


static void dupe_markdown_range(family_info **base, int count)
{
  int bestscore = -999;
  int bestslot = 0;
  int i = count;

  while (i--)
  {
    int score = base[i]->score;

    if (score <= bestscore)
      continue;
    bestscore = score;
    bestslot = i;
  }
  i = count;
  while (i--)
  {
    if (i == bestslot)
      continue;
    base[i]->score = -999;
  }
}


static void groupfonts(void)
{
  char **cpp;
  int i = num_font_styles;
  int low = 0;

  while (i--)
    parse_font_style(user_font_styles[i]);

  qsort(user_font_styles, num_font_styles, sizeof user_font_styles[0], compar_fontgroup);
  //printf("groupfonts() qsort(user_font_styles...)\n");
  //fflush(stdout);

  for (;;)
  {
    int high = low;

    if (low >= num_font_styles)
      break;
    for (;;)
    {
      if (++high >= num_font_styles)
        break;
      if (compar_fontgroup(user_font_styles + low, user_font_styles + high))
        break;
    }
    groupfonts_range(user_font_styles + low, high - low);
    low = high;
  }

  i = num_font_styles;
  while (i--)
  {
    free(user_font_styles[i]->filename);
    free(user_font_styles[i]->directory);
    free(user_font_styles[i]->family);
    free(user_font_styles[i]->style);
    free(user_font_styles[i]);
  }
  free(user_font_styles);
  user_font_styles = NULL;      // just to catch bugs

  qsort(user_font_families, num_font_families, sizeof user_font_families[0], compar_fontkiller);
  low = 0;
  for (;;)
  {
    int high = low;

    if (low >= num_font_families)
      break;
    for (;;)
    {
      if (++high >= num_font_families)
        break;
      if (strcmp(user_font_families[low]->family, user_font_families[high]->family))
        break;
    }
    dupe_markdown_range(user_font_families + low, high - low);
    low = high;
  }
  qsort(user_font_families, num_font_families, sizeof user_font_families[0], compar_fontscore);
  //printf("groupfonts() qsort(user_font_families 2...)\n");
  //fflush(stdout);
  if (num_font_families > 0 && user_font_families[0]->score < 0)
    fprintf(stderr, "sorted the wrong way, or all fonts were unusable\n");
#if 0
// THREADED_FONTS
  printf("Trim starting with %d families\n", num_font_families);
#endif
  while (num_font_families > 1 && user_font_families[num_font_families - 1]->score < 0)
  {
    i = --num_font_families;
    free(user_font_families[i]->directory);
    free(user_font_families[i]->family);
    cpp = user_font_families[i]->filename;
    if (cpp[0])
      free(cpp[0]);
    if (cpp[1])
      free(cpp[1]);
    if (cpp[2])
      free(cpp[2]);
    if (cpp[3])
      free(cpp[3]);
    free(user_font_families[i]);
    user_font_families[i] = NULL;
  }
#if 0
// THREADED_FONTS
  printf("Trim ending with %d families\n", num_font_families);
#endif
}


static void loadfonts_locale_filter(SDL_Surface *screen,
                                    SDL_Texture *texture,
                                    SDL_Renderer *renderer, const char *const dir, const char *restrict const locale)
{
  char buf[TP_FTW_PATHSIZE];
  unsigned dirlen = strlen(dir);

  DEBUG_PRINTF("Loading fonts from [%s]\n", dir);

  memcpy(buf, dir, dirlen);
  tp_ftw(screen, texture, renderer, buf, dirlen, 1, loadfont_callback, locale);
}

static void loadfonts(SDL_Surface *screen, SDL_Texture *texture, SDL_Renderer *renderer, const char *const dir)
{
  loadfonts_locale_filter(screen, texture, renderer, dir, NULL);
}


/* static */ int load_user_fonts(SDL_Surface *screen, SDL_Texture *texture,
                                 SDL_Renderer *renderer, void *vp, const char *restrict const locale)
{
  char *homedirdir;

  (void)vp;                     // junk passed by threading library

  loadfonts_locale_filter(screen, texture, renderer, DATA_PREFIX "fonts", locale);

  if (!no_system_fonts)
  {
#ifdef WIN32
    homedirdir = GetSystemFontDir();
    loadfonts(screen, texture, renderer, homedirdir);
    free(homedirdir);
#elif defined(__BEOS__)
    loadfonts(screen, texture, renderer, "/boot/home/config/font/ttffonts");
    loadfonts(screen, texture, renderer, "/usr/share/fonts");
    loadfonts(screen, texture, renderer, "/usr/X11R6/lib/X11/fonts");
#elif defined(__HAIKU__)
    dev_t volume = dev_for_path("/boot");
    char buffer[B_PATH_NAME_LENGTH + B_FILE_NAME_LENGTH];
    status_t result;

    result = find_directory(B_SYSTEM_FONTS_DIRECTORY, volume, false, buffer, sizeof(buffer));
    loadfonts(screen, texture, renderer, buffer);
    result = find_directory(B_SYSTEM_NONPACKAGED_FONTS_DIRECTORY, volume, false, buffer, sizeof(buffer));
    loadfonts(screen, texture, renderer, buffer);
    result = find_directory(B_USER_FONTS_DIRECTORY, volume, false, buffer, sizeof(buffer));
    loadfonts(screen, texture, renderer, buffer);
    result = find_directory(B_USER_NONPACKAGED_FONTS_DIRECTORY, volume, false, buffer, sizeof(buffer));
    loadfonts(screen, texture, renderer, buffer);
#elif defined(__APPLE__)
    loadfonts(screen, texture, renderer, "/System/Library/Fonts");
    loadfonts(screen, texture, renderer, "/Library/Fonts");
    loadfonts(screen, texture, renderer, apple_fontsPath());
    loadfonts(screen, texture, renderer, "/usr/share/fonts");
    loadfonts(screen, texture, renderer, "/usr/X11R6/lib/X11/fonts");
#elif defined(__ANDROID__)
    loadfonts(screen, texture, renderer, "data/fonts");
    loadfonts(screen, texture, renderer, "/system/fonts");
#elif defined(__sun__)
    loadfonts(screen, texture, renderer, "/usr/openwin/lib/X11/fonts");
    loadfonts(screen, texture, renderer, "/usr/share/fonts");
    loadfonts(screen, texture, renderer, "/usr/X11R6/lib/X11/fonts");
#else
    loadfonts(screen, texture, renderer, "/usr/share/feh/fonts");
    loadfonts(screen, texture, renderer, "/usr/share/fonts");
    loadfonts(screen, texture, renderer, "/usr/X11R6/lib/X11/fonts");
    loadfonts(screen, texture, renderer, "/usr/share/texmf/fonts");
    loadfonts(screen, texture, renderer, "/usr/share/grace/fonts/type1");
    loadfonts(screen, texture, renderer, "/usr/share/hatman/fonts");
    loadfonts(screen, texture, renderer, "/usr/share/icewm/themes/jim-mac");
    loadfonts(screen, texture, renderer, "/usr/share/vlc/skins2/fonts");
    loadfonts(screen, texture, renderer, "/usr/share/xplanet/fonts");
#endif
  }

  homedirdir = get_fname("fonts", DIR_DATA);
  loadfonts(screen, texture, renderer, homedirdir);
  free(homedirdir);

#ifdef WIN32
  homedirdir = get_fname("data/fonts", DIR_DATA);
  loadfonts(screen, texture, renderer, homedirdir);
  free(homedirdir);
#endif

#ifdef __APPLE__
  homedirdir = malloc(snprintf(NULL, 0, "%s/fonts", apple_globalPreferencesPath()) + 1);
  if (homedirdir)
  {
    sprintf(homedirdir, "%s/fonts", apple_globalPreferencesPath());
    loadfonts(screen, texture, renderer, homedirdir);
    free(homedirdir);
  }
#endif

#ifdef DEBUG
  printf("%s:%d - Grouping %d fonts...\n", __FILE__, __LINE__, num_font_styles);
  fflush(stdout);
#endif

  groupfonts();

#ifdef DEBUG
  printf("%s:%d - Finished loading the fonts\n", __FILE__, __LINE__);
  fflush(stdout);
#endif

  font_thread_done = 1;
  waiting_for_fonts = 0;
  // FIXME: need a memory barrier here
  return 0;                     // useless, wanted by threading library
}


#ifdef FORKED_FONTS

void run_font_scanner(SDL_Surface *screen, SDL_Texture *texture,
                      SDL_Renderer *renderer, const char *restrict const locale)
{
  int sv[2];
  int size, i;
  char *buf, *walk;

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv))
    exit(42);
  font_scanner_pid = fork();
  if (font_scanner_pid)
  {
    // parent (or error -- but we're screwed in that case)
    font_socket_fd = sv[0];
    close(sv[1]);
    return;
  }
#ifndef __HAIKU__
  /* be nice, letting the main thread get the CPU */
  if (nice(42) == -1)
  {
    fprintf(stderr, "Font scanner thread can't nice() itself\n");
  }
#endif
  sched_yield();                // try to let the parent run right now
  prctl(PR_SET_PDEATHSIG, 9);   // get killed if parent exits
  if (getppid() == 1)
    _exit(99);                  // parent is already init, and won't be dying :-)
  font_socket_fd = sv[1];
  close(sv[0]);
  progress_bar_disabled = 1;
  reliable_read(font_socket_fd, &no_system_fonts, sizeof no_system_fonts);
  sched_yield();                // try to let the parent run right now
  SDL_Init(SDL_INIT_NOPARACHUTE);
  TTF_Init();
  load_user_fonts(screen, texture, renderer, NULL, locale);

  size = 0;
  i = num_font_families;
  while (i--)
  {
    char *s;

    s = user_font_families[i]->directory;
    if (s)
      size += strlen(s);
    s = user_font_families[i]->family;
    if (s)
      size += strlen(s);
    s = user_font_families[i]->filename[0];
    if (s)
      size += strlen(s);
    s = user_font_families[i]->filename[1];
    if (s)
      size += strlen(s);
    s = user_font_families[i]->filename[2];
    if (s)
      size += strlen(s);
    s = user_font_families[i]->filename[3];
    if (s)
      size += strlen(s);
    size += 6;                  // for '\0' on each of the above
  }
  size += 2;                    // for 2-byte font count
  buf = malloc(size);
  walk = buf;
#ifdef DEBUG
  printf("%s:%d - Sending %u bytes with %u families.\n", __FILE__, __LINE__, size, num_font_families);
#endif
  *walk++ = num_font_families & 0xffu;
  *walk++ = num_font_families >> 8u;
  i = num_font_families;
  while (i--)
  {
    int len;
    char *s;

    s = user_font_families[i]->directory;
    if (s)
    {
      len = strlen(s);
      memcpy(walk, s, len);
      walk += len;
    }
    *walk++ = '\0';

    s = user_font_families[i]->family;
    if (s)
    {
      len = strlen(s);
      memcpy(walk, s, len);
      walk += len;
    }
    *walk++ = '\0';

    s = user_font_families[i]->filename[0];
    if (s)
    {
      len = strlen(s);
      memcpy(walk, s, len);
      walk += len;
    }
    *walk++ = '\0';

    s = user_font_families[i]->filename[1];
    if (s)
    {
      len = strlen(s);
      memcpy(walk, s, len);
      walk += len;
    }
    *walk++ = '\0';

    s = user_font_families[i]->filename[2];
    if (s)
    {
      len = strlen(s);
      memcpy(walk, s, len);
      walk += len;
    }
    *walk++ = '\0';

    s = user_font_families[i]->filename[3];
    if (s)
    {
      len = strlen(s);
      memcpy(walk, s, len);
      walk += len;
    }
    *walk++ = '\0';
  }
  reliable_write(font_socket_fd, buf, size);
  _exit(0);
}


void receive_some_font_info(SDL_Surface *screen, SDL_Texture *texture, SDL_Renderer *renderer)
{
  char *buf = NULL;
  unsigned buf_size = 0;
  unsigned buf_fill = 0;
  ssize_t rc;
  struct pollfd p;
  int status;
  /* unsigned */ char *walk;
  unsigned i;
  family_info *fip;

  fcntl(font_socket_fd, F_SETFL, O_NONBLOCK);
  for (;;)
  {
    if (buf_size <= buf_fill * 9 / 8 + 128)
    {
      buf_size = buf_size * 5 / 4 + 256;

      // FIXME: Valgrind says this leaks -bjk 2007.07.19
      buf = realloc(buf, buf_size);
    }
    rc = read(font_socket_fd, buf + buf_fill, buf_size - buf_fill);
#ifdef DEBUG
    printf("%s:%d - read: fd=%d buf_fill=%u buf_size=%u rc=%ld\n", __FILE__,
           __LINE__, font_socket_fd, buf_fill, buf_size, (long int)rc);
#endif

    if (rc == -1)
    {
      switch (errno)
      {
      default:
        return;
      case EAGAIN:
        ;                       // satisfy a C syntax abomination
        p = (struct pollfd)
        {
          font_socket_fd, POLLIN, 0
        };
        show_progress_bar_(screen, texture, renderer);
        poll(&p, 1, 29);        // try not to burn CPU time
        continue;
      case EINTR:
        continue;
      }
    }
    buf_fill += rc;
    if (!rc || font_thread_aborted)
      break;
  }
  close(font_socket_fd);

  waitpid(font_scanner_pid, &status, 0);
  if (WIFSIGNALED(status) || font_thread_aborted)
  {
    fprintf(stderr, "child killed by signal %u\n", WTERMSIG(status));
    user_font_families = NULL;
    num_font_families = 0;
    font_thread_done = 1;

    return;
  }

  show_progress_bar_(screen, texture, renderer);
  walk = buf;
  num_font_families = *(unsigned char *)walk++;
  num_font_families += *(unsigned char *)walk++ << 8u;
#ifdef DEBUG
  printf("%s:%d - Got %u bytes with %u families.\n", __FILE__, __LINE__, buf_fill, num_font_families);
#endif
  user_font_families = malloc(num_font_families * sizeof *user_font_families);

  // FIXME: Valgrind says this malloc() is leaked -bjk 2007.07.19
  fip = malloc(num_font_families * sizeof **user_font_families);

  i = num_font_families;
  while (i--)
  {
    unsigned len;

    user_font_families[i] = fip + i;

    len = strlen(walk);
    user_font_families[i]->directory = len ? walk : NULL;
    walk += len + 1;

    len = strlen(walk);
    user_font_families[i]->family = len ? walk : NULL;
    walk += len + 1;

    len = strlen(walk);
    user_font_families[i]->filename[0] = len ? walk : NULL;
    walk += len + 1;

    len = strlen(walk);
    user_font_families[i]->filename[1] = len ? walk : NULL;
    walk += len + 1;

    len = strlen(walk);
    user_font_families[i]->filename[2] = len ? walk : NULL;
    walk += len + 1;

    len = strlen(walk);
    user_font_families[i]->filename[3] = len ? walk : NULL;
    walk += len + 1;

    user_font_families[i]->handle = NULL;

    // score left uninitialized
  }
  font_thread_done = 1;
}

#endif




TuxPaint_Font *getfonthandle(int desire)
{
  int missing = 0;
  family_info *fi = user_font_families[desire];
  char *name;
  char *pathname;
  char description[1024];

#ifdef DEBUG
  printf("%s:%d - \ngetfonthandle(%d)...\n", __FILE__, __LINE__, desire);
  fflush(stdout);
#endif

  if (fi == NULL)
  {
#ifdef DEBUG
    printf("%s:%d - getfonthandle(%d) points to a NULL family\n", __FILE__, __LINE__, desire);
    fflush(stdout);
#endif
    return NULL;
  }

  if (fi->filename[text_state] != NULL)
  {
#ifdef DEBUG
    printf("%s:%d - Setting 'name' to fi->filename[%d (0x%x)]\n", __FILE__, __LINE__, (int)text_state, (int)text_state);
    fflush(stdout);
#endif

    name = fi->filename[text_state];

#ifdef DEBUG
    printf("%s:%d - Which is: %s\n", __FILE__, __LINE__, name);
    fflush(stdout);
#endif
  }
  else
  {
#ifdef DEBUG
    printf("%s:%d - fi->filename[%d (0x%x) is NULL\n", __FILE__, __LINE__, (int)text_state, (int)text_state);
    fflush(stdout);
#endif

    name = NULL;
  }

  if (fi->handle)
  {
#ifdef DEBUG
    printf("%s:%d - fi->handle was set (0x%x)\n", __FILE__, __LINE__, (int)(intptr_t) fi->handle);

    fflush(stdout);
#endif
    return fi->handle;
  }

#ifdef DEBUG
  printf("%s:%d - fi->handle was not yet set\n", __FILE__, __LINE__);
  fflush(stdout);
#endif


  if (!name)
  {
    name = fi->filename[text_state ^ TTF_STYLE_ITALIC];
    missing = text_state & TTF_STYLE_ITALIC;
  }
  if (!name)
  {
    name = fi->filename[text_state ^ TTF_STYLE_BOLD];
    missing = text_state & TTF_STYLE_BOLD;
  }
  if (!name)
  {
    name = fi->filename[text_state ^ (TTF_STYLE_ITALIC | TTF_STYLE_BOLD)];
    missing = text_state & (TTF_STYLE_ITALIC | TTF_STYLE_BOLD);
  }
  if (!name)
  {
#ifdef DEBUG
    printf("%s:%d - name is still NULL\n", __FILE__, __LINE__);
    fflush(stdout);
#endif
    return (NULL);
  }

  pathname = alloca(strlen(fi->directory) + 1 + strlen(name) + 1);
  sprintf(pathname, "%s/%s", fi->directory, name);

  strcpy(description, "");

  fi->handle = TuxPaint_Font_OpenFont(description, pathname, text_sizes[text_size]);
  // if the font doesn't load, we die -- it did load OK before though

  if (fi->handle == NULL)
  {
#ifdef DEBUG
    printf("%s:%d - fi->handle is NULL!\n", __FILE__, __LINE__);
    fflush(stdout);
#endif
    return (NULL);
  }

  if (fi->handle->typ == FONT_TYPE_TTF)
  {
    if (fi->handle->ttf_font == NULL)
    {
#ifdef DEBUG
      printf("%s:%d - fi->handle->ttf_font is NULL!\n", __FILE__, __LINE__);
      fflush(stdout);
#endif
      return (NULL);
    }

#ifdef DEBUG
    printf("%s:%d - calling TTF_SetFontStyle(0x%x)\n", __FILE__, __LINE__, missing);
    fflush(stdout);
#endif

    TTF_SetFontStyle(fi->handle->ttf_font, missing);
  }

  if (fi->handle->typ == FONT_TYPE_PANGO)
    printf("%s:%d - It's a Pango context...\n", __FILE__, __LINE__);

  return fi->handle;
}


// backdoor into qsort operations, so we don't have to do work again
static int was_bad_font;

// see if two font surfaces are the same
static int do_surfcmp(const SDL_Surface *const *const v1, const SDL_Surface *const *const v2)
{
  const SDL_Surface *const s1 = *v1;
  const SDL_Surface *const s2 = *v2;
  int width;
  int cmp;
  int i;

  if (s1 == s2)
  {
    /* Should not be receiving the same SDL_Surfaces! */
    fprintf(stderr, "s1==s2?\n");
    return 0;
  }
  if (!s1 || !s2 || !s1->w || !s2->w || !s1->h || !s2->h || !s1->format || !s2->format)
  {
    /* One or the other or both SDL_Surfaces were invalid!? */
    was_bad_font = 1;
    return 0;
  }
  if (s1->format->BytesPerPixel != s2->format->BytesPerPixel)
  {
    /* The SDL_Surfaces are in different bit depths!? */
    was_bad_font = 1;
    return s1->format->BytesPerPixel - s2->format->BytesPerPixel;
  }

  /* If they are different shapes, sort by their shape
     (we can quickly tell that they are different) */
  if (s1->w != s2->w)
    return s1->w - s2->w;
  if (s1->h != s2->h)
    return s1->h - s2->h;

  /* Otherwise, compare the pixels in the surfaces
     (using `memcmp()`) */
  {
    const char *const c1 = (char *const)s1->pixels;
    const char *const c2 = (char *const)s2->pixels;

    width = s1->format->BytesPerPixel * s1->w;
    if (width == s1->pitch)
    {
      /* Same width, just compare scanline */
      return memcmp(c1, c2, width * s1->h);
    }

    /* Otherwise, compare pixels */
    cmp = 0;
    i = s1->h;
    while (i--)
    {
      cmp = memcmp(c1 + i * s1->pitch, c2 + i * s2->pitch, width);
      if (cmp)
        break;
    }
  }

  return cmp;
}

// see if two font surfaces are the same
// (if two come out identical, we'll set the `was_bad_font` flag)
static int surfcmp(const void *s1, const void *s2)
{
  int diff = do_surfcmp(s1, s2);

  if (!diff)
  {
#ifdef DEBUG
    printf("surfcmp found two identical renders!\n");
#endif
    was_bad_font = 1;
  }
  return diff;
}

// check if the characters will render distinctly
int charset_works(TuxPaint_Font *font, const char *s)
{
  SDL_Color black = { 0, 0, 0, 0 };
  SDLPango_Matrix pango_color;
  SDL_Surface **surfs = malloc(strlen(s) * sizeof surfs[0]);
  unsigned count = 0;
  int ret = 0;

#if defined (__ANDROID__)
  while (*s && !font_thread_aborted)
#else
  while (*s)
#endif
  {
    char c[8];
    unsigned offset = 0;
    SDL_Surface *tmp_surf = NULL;

    do
      c[offset++] = *s++;
    while ((*s & 0xc0u) == 0x80u);      // assume safe input
    c[offset++] = '\0';

    if (font->typ == FONT_TYPE_PANGO)
    {
      sdl_color_to_pango_color(black, &pango_color);
      SDLPango_SetDefaultColor(font->pango_context, &pango_color);
      SDLPango_SetText(font->pango_context, c, -1);
      tmp_surf = SDLPango_CreateSurfaceDraw(font->pango_context);
    }

    if (font->typ == FONT_TYPE_TTF)
    {
      tmp_surf = TTF_RenderUTF8_Blended(font->ttf_font, c, black);
    }

    if (!tmp_surf)
    {
      fprintf(stderr, "Could not render font %s\n", font->desc);
      goto out;
    }
    surfs[count++] = tmp_surf;
  }
  was_bad_font = 0;
  qsort(surfs, count, sizeof surfs[0], surfcmp);
  ret = !was_bad_font;
out:
  while (count--)
  {
    if (surfs[count] == NULL)
      fprintf(stderr, "TRYING TO RE-FREE!");
    else
    {
      SDL_FreeSurface(surfs[count]);
      surfs[count] = NULL;
    }
  }
  free(surfs);
#ifdef DEBUG
  printf("charset_works = %d for %s\n", ret, font->desc);
#endif
  return ret;
}

int TuxPaint_Font_FontHeight(TuxPaint_Font *tpf)
{
  if (tpf == NULL)
  {
#ifdef DEBUG
    printf("%s:%d - TuxPaint_Font_FontHeight() received NULL\n", __FILE__, __LINE__);
    fflush(stdout);
#endif
    return (1);
  }

  return (tpf->height);
}

const char *TuxPaint_Font_FontFaceFamilyName(TuxPaint_Font *tpf)
{
  if (tpf == NULL)
  {
#ifdef DEBUG
    printf("%s:%d - TuxPaint_Font_FontFaceFamilyName() received NULL\n", __FILE__, __LINE__);
    fflush(stdout);
#endif
    return ("");
  }

  if (tpf->typ == FONT_TYPE_PANGO)
  {
    (void)(tpf);
    /* FIXME */

    return ("");
  }

  if (tpf->typ == FONT_TYPE_TTF)
    return (TTF_FontFaceFamilyName(tpf->ttf_font));

#ifdef DEBUG
  printf("%s:%d - TuxPaint_Font_FontFaceFamilyName() is confused\n", __FILE__, __LINE__);
#endif

  return ("");
}

const char *TuxPaint_Font_FontFaceStyleName(TuxPaint_Font *tpf)
{
  if (tpf == NULL)
  {
#ifdef DEBUG
    printf("%s:%d - TuxPaint_Font_FontFaceStyleName() received NULL\n", __FILE__, __LINE__);
    fflush(stdout);
#endif
    return ("");
  }

  if (tpf->typ == FONT_TYPE_PANGO)
  {
    (void)(tpf);
    /* FIXME */

    return ("");
  }

  if (tpf->typ == FONT_TYPE_TTF)
    return (TTF_FontFaceStyleName(tpf->ttf_font));

#ifdef DEBUG
  printf("%s:%d - TuxPaint_Font_FontFaceStyleName() is confused\n", __FILE__, __LINE__);
#endif

  return ("");
}


void sdl_color_to_pango_color(SDL_Color sdl_color, SDLPango_Matrix *pango_color)
{
  Uint8 pc[4][4];

  pc[0][0] = 0;
  pc[1][0] = 0;
  pc[2][0] = 0;
  pc[3][0] = 0;

  pc[0][1] = sdl_color.r;
  pc[1][1] = sdl_color.g;
  pc[2][1] = sdl_color.b;
  pc[3][1] = 255;

  pc[0][2] = 0;
  pc[1][2] = 0;
  pc[2][2] = 0;
  pc[3][2] = 0;

  pc[0][3] = 0;
  pc[1][3] = 0;
  pc[2][3] = 0;
  pc[3][3] = 0;

  memcpy(pango_color, pc, 16);
}

/**
 * Given a font description, do the very bare minimum to
 * have Pango library try to load it, then retrieve the description
 * of the font that was _actually_ loaded.
 * (e.g., "tuxpaint --uifont bookman" finds and uses "URW Bookman"
 * on my system -bjk 2023.06.01)
 */
char *ask_pango_for_font(char *pangodesc)
{
  PangoFontMap *font_map;
  PangoContext *context;
  PangoFontDescription *font_desc;
  PangoFont *font;
  PangoFontDescription *pfd;
  char *descr;

  font_map = pango_ft2_font_map_new();
  if (font_map == NULL)
  {
    fprintf(stderr, "pango_ft2_font_map_new() failed\n");
    fflush(stderr);
    return NULL;
  }

  context = pango_font_map_create_context(font_map);
  if (context == NULL)
  {
    fprintf(stderr, "pango_font_map_create_context() failed\n");
    fflush(stderr);
    g_object_unref(font_map);
    return NULL;
  }

  font_desc = pango_font_description_from_string(pangodesc);
  if (font_desc == NULL)
  {
    fprintf(stderr, "pango_font_description_from_string() failed\n");
    fflush(stderr);
    g_object_unref(font_map);
    g_object_unref(context);
    return NULL;
  }

  font = pango_font_map_load_font(font_map, context, font_desc);
  if (font == NULL)
  {
    fprintf(stderr, "pango_font_map_load_font() failed\n");
    fflush(stderr);
    g_object_unref(font_map);
    g_object_unref(context);
    pango_font_description_free(font_desc);
    return NULL;
  }

  pfd = pango_font_describe(font);
  if (pfd == NULL)
  {
    fprintf(stderr, "pango_font_describe() failed\n");
    fflush(stderr);
    g_object_unref(font_map);
    g_object_unref(context);
    pango_font_description_free(font_desc);
    g_object_unref(font);
    return NULL;
  }

  descr = strdup(pango_font_description_get_family(pfd));

  g_object_unref(font_map);
  g_object_unref(context);
  pango_font_description_free(font_desc);
  g_object_unref(font);
  /* N.B. Not free'ing `pfd`: The data is owned by the instance */

  return descr;
}
