/*
  dirwalk.c

  Copyright (c) 2009-2014
  http://www.tuxpaint.org/

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

  $Id: dirwalk.c,v 1.20 2014/03/30 07:23:20 wkendrick Exp $
*/

#include <stdio.h>
#include <stdlib.h>
#ifndef __USE_GNU
#define __USE_GNU		/* for strcasestr() */
#endif
#include <string.h>
#include <locale.h>
#include <libintl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "SDL_ttf.h"

#include "dirwalk.h"
#include "progressbar.h"
#include "android_assets.h"

/*
	The following section renames global variables defined in SDL_Pango.h to avoid errors during linking.
	It is okay to rename these variables because they are constants.
	SDL_Pang.h is included by fonts.h.  
*/
#define _MATRIX_WHITE_BACK _MATRIX_WHITE_BACK1
#define MATRIX_WHITE_BACK MATRIX_WHITE_BACK1
#define _MATRIX_BLACK_BACK _MATRIX_BLACK_BACK1
#define MATRIX_BLACK_BACK MATRIX_BLACK_BACK1
#define _MATRIX_TRANSPARENT_BACK_BLACK_LETTER _MATRIX_TRANSPARENT_BACK_BLACK_LETTER1
#define MATRIX_TRANSPARENT_BACK_BLACK_LETTER MATRIX_TRANSPARENT_BACK_BLACK_LETTER1
#define _MATRIX_TRANSPARENT_BACK_WHITE_LETTER _MATRIX_TRANSPARENT_BACK_WHITE_LETTER1
#define MATRIX_TRANSPARENT_BACK_WHITE_LETTER MATRIX_TRANSPARENT_BACK_WHITE_LETTER1
#define _MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER _MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER1
#define MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER1
/*
	The renaming ends here.
*/

#include "fonts.h"

#include "debug.h"



///////////////// directory walking callers and callbacks //////////////////

void loadfont_callback(SDL_Surface * screen, SDL_Texture * texture, SDL_Renderer * renderer, const char *restrict const dir,
		       unsigned dirlen, tp_ftw_str * files, unsigned i, const char *restrict const locale)
{
  dirlen = dirlen;

  while (i--)
  {
    int loadable = 0;
    const char *restrict const cp = strchr(files[i].str, '.');
    show_progress_bar_(screen, texture, renderer);
    if (cp)
    {
      // need gcc 3.4 for the restrict in this location
      const char * /*restrict */ const suffixes[] =
	{ "ttc", "dfont", "pfa", "pfb", "otf", "ttf", };
      int j = sizeof suffixes / sizeof suffixes[0];
      while (j--)
      {
	// only check part, because of potential .gz or .bz2 suffix
	if (!strncasecmp(cp + 1, suffixes[j], strlen(suffixes[j])))
	{
	  loadable = 1;
	  break;
	}
      }
    }
    if (!loadable)
    {
      if (strcasestr(files[i].str, "/rsrc"))
	loadable = 1;
    }
    // Loadable: TrueType (.ttf), OpenType (.otf), Type1 (.pfa and .pfb),
    // and various useless bitmap fonts. Compressed files (with .gz or .bz2)
    // should also work. A *.dfont is pretty much a Mac resource fork in a normal
    // file, and may load with some library versions.
    if (loadable)
    {
      char fname[512];
      TuxPaint_Font *font;
      snprintf(fname, sizeof fname, "%s/%s", dir, files[i].str);
#ifdef DEBUG
      printf("Loading font: %s  (locale is: %s)\n", fname, (locale ? locale : "NULL")); //EP
#endif
      if (locale && strstr(fname, "locale") && !all_locale_fonts)
      {
        char fname_check[512];
        /* We're (probably) loading from our locale fonts folder; ONLY load our locale's font */
        snprintf(fname_check, sizeof fname_check, "%s/%s.ttf", dir, locale);
#ifdef DEBUG
        printf("checking \"%s\" vs \"%s\"\n", fname_check, fname); //EP
#endif
        if (strcmp(fname, fname_check) == 0)
          font = TuxPaint_Font_OpenFont("", fname, text_sizes[text_size]);
        else
          font = NULL;
      }
      else
      {
        font = TuxPaint_Font_OpenFont("", fname, text_sizes[text_size]);
      }
      if (font)
      {
	const char *restrict const family = TuxPaint_Font_FontFaceFamilyName(font);
	const char *restrict const style = TuxPaint_Font_FontFaceStyleName(font);


#ifdef DEBUG
	int numfaces = TTF_FontFaces(font->ttf_font);
	if (numfaces != 1)
	  printf("Found %d faces in %s, %s, %s\n", numfaces, files[i].str,
		 family, style);

        printf("success: tpf: 0x%x tpf->ttf_font: 0x%x\n",
			   (unsigned int)(intptr_t) font, (unsigned int)(intptr_t) font->ttf_font);	//EP added (intptr_t) to avoid warning on x64
#endif

	// These fonts crash Tux Paint via a library bug.
	int blacklisted = !strcmp("Zapfino", family) || !strcmp("Elvish Ring NFI", family);

	// First, the blacklist. We list font families that can crash Tux Paint
	// via bugs in the SDL_ttf library. We also test fonts to be sure that
	// they have both uppercase and lowercase letters. Note that we do not
	// test for "Aa", because it is OK if uppercase and lowercase are the
	// same (but not nice -- such fonts get a low score later).
	//
	// Most locales leave the blacklist strings alone: "QX" and "qx"
	// (it is less destructive to use the scoring strings instead)
	//
	// Locales that absolutely require all fonts to have some
	// extra characters should use "QX..." and "qx...", where "..."
	// are some characters you absolutely require in all fonts.
	//
	// Locales with absolutely NO use for ASCII may use "..." and "...",
	// where "..." are some characters you absolutely require in
	// all fonts. This would be the case for a locale in which it is
	// impossible for a user to type ASCII letters.
	//
	// Most translators should use scoring instead.
	if(!charset_works(font, gettext("qx")) || !charset_works(font, gettext("QX")))
		blacklisted = 1;

	if(!blacklisted){
	  if (num_font_styles == num_font_styles_max)
	  {
	    num_font_styles_max = num_font_styles_max * 5 / 4 + 30;
	    user_font_styles =
	      realloc(user_font_styles,
		      num_font_styles_max * sizeof *user_font_styles);
	  }
	  user_font_styles[num_font_styles] =
	    malloc(sizeof *user_font_styles[num_font_styles]);
	  user_font_styles[num_font_styles]->directory = strdup(dir);
	  user_font_styles[num_font_styles]->filename = files[i].str;	// steal it (mark NULL below)
	  user_font_styles[num_font_styles]->family = strdup(family);
	  user_font_styles[num_font_styles]->style = strdup(style);
          user_font_styles[num_font_styles]->score = 0;

	  // TODO: weight specification

	  // Now we score fonts to ensure that the best ones will be placed at
	  // the top of the list. The user will see them first. This sorting is
	  // especially important for users who have scroll buttons disabled.
	  // Translators should do whatever is needed to put crummy fonts last.
	  
	  // distinct uppercase and lowercase (e.g., 'o' vs. 'O')
	  user_font_styles[num_font_styles]->score += charset_works(font, gettext("oO"));

	  // common punctuation (e.g., '?', '!', '.', ',', etc.)
	  user_font_styles[num_font_styles]->score += charset_works(font, gettext(",.?!"));

	  // uncommon punctuation (e.g., '@', '#', '*', etc.)
	  user_font_styles[num_font_styles]->score += charset_works(font, gettext("`\%_@$~#{<(^&*"));

	  // digits (e.g., '0', '1' and '7')
	  user_font_styles[num_font_styles]->score += charset_works(font, gettext("017"));

	  // distinct circle-like characters (e.g., 'O' (capital oh) vs. '0' (zero))
	  user_font_styles[num_font_styles]->score += charset_works(font, gettext("O0"));

	  // distinct line-like characters (e.g., 'l' (lowercase elle) vs. '1' (one) vs. 'I' (capital aye))
	  user_font_styles[num_font_styles]->score += charset_works(font, gettext("1Il|"));

	  // translation spares -- design not finalized
#if 0
	  user_font_styles[num_font_styles]->score += charset_works(font, gettext("<1>spare-1a"));
	  user_font_styles[num_font_styles]->score += charset_works(font, gettext("<1>spare-1b"));
	  user_font_styles[num_font_styles]->score += charset_works(font, gettext("<9>spare-9a"))*9;
	  user_font_styles[num_font_styles]->score += charset_works(font, gettext("<9>spare-9b"))*9;
#endif

// this really should be dynamic, avoiding the need for a special build
#ifdef OLPC_XO
	  // Maybe German adds a "\xc2\xb7" (middle dot) and colon here? The key wouldn't change though.
	  user_font_styles[num_font_styles]->score += charset_works(font, "\xc3\x97\xc3\xb7");	// multiply and divide
#endif

	  // FIXME: add topology tests ('A' has one hole, 'B' has two holes, etc.)

	  num_font_styles++;
//printf("Accepted: %s, %s, %s, score(%d)\n", files[i].str, family, style, user_font_styles[num_font_styles]->score);
	  files[i].str = NULL;	// so free() won't crash -- we stole the memory
	}
	else
	{
#if 0
// THREADED_FONTS
	  printf("Font is too defective: %s, %s, %s\n", files[i].str, family,
		 style);
#endif
	}
	TuxPaint_Font_CloseFont(font);
      }
      else
      {
#if 0
// THREADED_FONTS
	printf("could not open %s\n", files[i].str);
#endif
      }
    }
    free(files[i].str);
  }
  free(files);
}


// For qsort()
int compare_ftw_str(const void *v1, const void *v2)
{
  const char *restrict const s1 = ((tp_ftw_str *) v1)->str;
  const char *restrict const s2 = ((tp_ftw_str *) v2)->str;
  return -strcmp(s1, s2); /* FIXME: Should we try strcasecmp, to group things together despite uppercase/lowercase in filenames (e.g., Jigsaw* vs jigsaw* Starters)??? -bjk 2009.10.11 */
}

void tp_ftw(SDL_Surface * screen, SDL_Texture * texture, SDL_Renderer * renderer, char *restrict const dir, unsigned dirlen,
	    int rsrc, void (*fn) (SDL_Surface * screen,
				  SDL_Texture * texture,
				  SDL_Renderer * renderer,
				  const char *restrict const dir,
				  unsigned dirlen, tp_ftw_str * files,
				  unsigned count, const char *restrict const locale),
            const char *restrict const locale)
{
  DIR *d;
  unsigned num_file_names = 0;
  unsigned max_file_names = 0;
  tp_ftw_str *file_names = NULL;
  unsigned num_dir_names = 0;
  unsigned max_dir_names = 0;
  tp_ftw_str *dir_names = NULL;
  int d_namlen;
  int add_rsrc;
  char * di;
  unsigned dlen;

  dir[dirlen++] = '/';
  dir[dirlen] = '\0';
//printf("processing directory %s %d\n", dir, dirlen);

  dlen = dirlen;

  /* Open the directory: */
  d = opendir(dir);
#ifdef __ANDROID__
  AAssetDir * adir;
  if (!d) /* Fallback into assets */
  {
    /* Remove the trailing '/' */
    dlen = strlen(dir) - 1;
    di = strndup(dir, dlen);
    adir = open_asset_dir(di);
    if (!adir)
      return;
  }
#else
  if (!d)
    return;
#endif

  for (;;)
  {
    struct dirent *f;
    if (d)
      f = readdir(d);
    int filetype = TP_FTW_UNKNOWN;

#ifdef __ANDROID__
    char * afilename;

    if (!d)
    {
      afilename = AAssetDir_getNextFileName(adir);
      if (afilename)
      {
	f=malloc(sizeof(struct dirent));
	strncpy(f->d_name, afilename, sizeof(f->d_name));
	f->d_type=DT_REG;

	/* There is not _DIRENT_HAVE_D_NAMLEN currently on Android 4.3, but who knows in the future... */
#if defined(_DIRENT_HAVE_D_NAMLEN)
	f->d_namlen = strlen(f->d_name);
#endif

	/* AAssetDir_getNextFileName() only lists files, not (sub)dirs,
	   and we don't put any device or special file inside assets,
	   so it is a regular file */
	filetype = TP_FTW_NORMAL;
      }
      else
	break;
    }
#endif
    if (!f)
      break;
    if (f->d_name[0] == '.')
      continue;
// Linux and BSD can often provide file type info w/o the stat() call
#ifdef DT_UNKNOWN
    switch (f->d_type)
    {
    default:
      continue;
    case DT_REG:
      if (!rsrc)		// if maybe opening resource files, need st_size
	filetype = TP_FTW_NORMAL;
      break;
    case DT_DIR:
      filetype = TP_FTW_DIRECTORY;
      break;
    case DT_UNKNOWN:
    case DT_LNK:
      ;
    }
#else
#warning Failed to see DT_UNKNOWN
#endif

#if defined(_DIRENT_HAVE_D_NAMLEN) || defined(__APPLE__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__)
    d_namlen = f->d_namlen;
#else
    d_namlen = strlen(f->d_name);
#endif
    add_rsrc = 0;

    if (filetype == TP_FTW_UNKNOWN)
    {
      struct stat sbuf;
      memcpy(dir + dirlen, f->d_name, d_namlen + 1);
      if (stat(dir, &sbuf))
	continue;		// oh well... try the next one
      if (S_ISDIR(sbuf.st_mode))
	filetype = TP_FTW_DIRECTORY;
      else if (S_ISREG(sbuf.st_mode))
      {
	filetype = TP_FTW_NORMAL;
	if (rsrc && !sbuf.st_size)
	  add_rsrc = 5;		// 5 is length of "/rsrc"
      }
      else
	continue;		// was a device file or somesuch
    }
    if (filetype == TP_FTW_NORMAL)
    {
      char *cp;

      if (num_file_names == max_file_names)
      {
	max_file_names = max_file_names * 5 / 4 + 30;
	file_names = realloc(file_names, max_file_names * sizeof *file_names);
      }
      cp = malloc(d_namlen + add_rsrc + 1);
      memcpy(cp, f->d_name, d_namlen);
      if (add_rsrc)
	memcpy(cp + d_namlen, "/rsrc", 6);
      else
	cp[d_namlen] = '\0';
      file_names[num_file_names].str = cp;
      file_names[num_file_names].len = d_namlen;
      num_file_names++;
    }
    if (filetype == TP_FTW_DIRECTORY)
    {
      char *cp;
      if (num_dir_names == max_dir_names)
      {
	max_dir_names = max_dir_names * 5 / 4 + 3;
	dir_names = realloc(dir_names, max_dir_names * sizeof *dir_names);
      }
      cp = malloc(d_namlen + 1);
      memcpy(cp, f->d_name, d_namlen + 1);
      dir_names[num_dir_names].str = cp;
      dir_names[num_dir_names].len = d_namlen;
      num_dir_names++;
    }
  }

  closedir(d);
  show_progress_bar_(screen, texture, renderer);
  dir[dirlen] = '\0';		// repair it (clobbered for stat() call above)

  if (1 || file_names)  // Now ALWAYS calling callback function, so stamp loader can notice top-level directories (even if there are only subdirs, and no files, inside) -bjk 2007.05.16
  {
// let callee sort and keep the string
#if 0
    qsort(file_names, num_file_names, sizeof *file_names, compare_ftw_str);
    while (num_file_names--)
    {
      free(file_names[num_file_names].str);
    }
    free(file_names);
#else
    if (dlen != dirlen) /* First case only happens in Android files coming from assets */
      fn(screen, texture, renderer, di, dlen, file_names, num_file_names, locale);
    else
      fn(screen, texture, renderer, dir, dirlen, file_names, num_file_names, locale);
#endif
  }

  if (dir_names)
  {
    qsort(dir_names, num_dir_names, sizeof *dir_names, compare_ftw_str);
    while (num_dir_names--)
    {
      memcpy(dir + dirlen, dir_names[num_dir_names].str,
	     dir_names[num_dir_names].len + 1);
      tp_ftw(screen, texture, renderer, dir, dirlen + dir_names[num_dir_names].len, rsrc, fn, locale);
      free(dir_names[num_dir_names].str);
    }
    free(dir_names);
  }
}
