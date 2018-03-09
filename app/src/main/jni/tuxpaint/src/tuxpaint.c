/*
  tuxpaint.c

  Tux Paint - A simple drawing program for children.

  Copyright (c) 2002-2017 by Bill Kendrick and others; see AUTHORS.txt
  bill@newbreedsoftware.com
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

  June 14, 2002 - October 15, 2017
*/


/* (Note: VER_VERSION and VER_DATE are now handled by Makefile) */


/* FIXME: */

/* Use this in places where we can only (or only want to, for whatever reason)
   use 'unlink()' to delete files, rather than trying to put them in the
   desktop enivronment's "trash" -bjk 2011.04.18 */
/* #define UNLINK_ONLY */

/* Color depth for Tux Paint to run in, and store canvases in: */

#if defined(NOKIA_770)
#define VIDEO_BPP 16
#endif

#if defined(OLPC_XO)
#define VIDEO_BPP 15
#endif

#ifndef VIDEO_BPP
                                                                               /*# define VIDEO_BPP 15 *//* saves memory */
                                                                               /*# define VIDEO_BPP 16 *//* causes discoloration */
                                                                               /*# define VIDEO_BPP 24 *//* compromise */
#define VIDEO_BPP 32            /* might be fastest, if conversion funcs removed */
#endif


                                                                                     /* #define CORNER_SHAPES *//* need major work! */


/* Method for printing images: */

#define PRINTMETHOD_PS          /* Direct to PostScript */
                                                                                                  /*#define PRINTMETHOD_PNM_PS *//* Output PNM, assuming it gets printed */
                                                                                                          /*#define PRINTMETHOD_PNG_PNM_PS *//* Output PNG, assuming it gets printed */


#define MAX_PATH 256

/* Compile-time options: */

#include "debug.h"

#ifdef NOKIA_770
#define LOW_QUALITY_THUMBNAILS
#define LOW_QUALITY_STAMP_OUTLINE
#define NO_PROMPT_SHADOWS
#define USE_HWSURFACE
#else
/* #define DEBUG_MALLOC */
/* #define LOW_QUALITY_THUMBNAILS */
/* #define LOW_QUALITY_COLOR_SELECTOR */
/* #define LOW_QUALITY_STAMP_OUTLINE */
/* #define NO_PROMPT_SHADOWS */
/* #define USE_HWSURFACE */

/* FIXME: Deal with this option properly -bjk 2010.02.25 */
#define GAMMA_CORRECTED_THUMBNAILS
#endif

#define ALLOW_STAMP_OVERSCAN


/* Disable fancy cursors in fullscreen mode, to avoid SDL bug: */
/* (This bug is still around, as of SDL 1.2.9, October 2005) */
/* (Is it still in SDL 1.2.11 in May 2007, though!? -bjk) */
/* #define LARGE_CURSOR_FULLSCREEN_BUG */

/* control the color selector */
#define COLORSEL_DISABLE 0      /* disable and draw the (greyed out) colors */
#define COLORSEL_ENABLE  1      /* enable and draw the colors */
#define COLORSEL_CLOBBER 2      /* colors get scribbled over */
#define COLORSEL_REFRESH 4      /* redraw the colors, either on or off */
#define COLORSEL_CLOBBER_WIPE 8 /* draw the (greyed out) colors, but don't disable */
#define COLORSEL_FORCE_REDRAW 16        /* enable, and force redraw (to make color picker work) */

/* FIXME: Why are we checking this BEFORE the #include "SDL.h"!? Does this even work? -bjk 2010.04.24 */
/* Setting the amask value based on endianness*/
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define TPAINT_AMASK 0xff000000
#else
#define TPAINT_AMASK 0x000000ff
#endif

static unsigned draw_colors(unsigned action);


/* hide all scale-related values here */
typedef struct scaleparams
{
  unsigned numer, denom;
} scaleparams;

static scaleparams scaletable[] = {
  {1, 256},                     /*  0.00390625 */
  {3, 512},                     /*  0.005859375 */
  {1, 128},                     /*  0.0078125 */
  {3, 256},                     /*  0.01171875 */
  {1, 64},                      /*  0.015625 */
  {3, 128},                     /*  0.0234375 */
  {1, 32},                      /*  0.03125 */
  {3, 64},                      /*  0.046875 */
  {1, 16},                      /*  0.0625 */
  {3, 32},                      /*  0.09375 */
  {1, 8},                       /*  0.125 */
  {3, 16},                      /*  0.1875 */
  {1, 4},                       /*  0.25 */
  {3, 8},                       /*  0.375 */
  {1, 2},                       /*  0.5 */
  {3, 4},                       /*  0.75 */
  {1, 1},                       /*  1 */
  {3, 2},                       /*  1.5 */
  {2, 1},                       /*  2 */
  {3, 1},                       /*  3 */
  {4, 1},                       /*  4 */
  {6, 1},                       /*  6 */
  {8, 1},                       /*  8 */
  {12, 1},                      /* 12 */
  {16, 1},                      /* 16 */
  {24, 1},                      /* 24 */
  {32, 1},                      /* 32 */
  {48, 1},                      /* 48 */
};


/* Macros: */

#define HARD_MIN_STAMP_SIZE 0   /* bottom of scaletable */
#define HARD_MAX_STAMP_SIZE (sizeof scaletable / sizeof scaletable[0] - 1)

#define MIN_STAMP_SIZE (stamp_data[stamp_group][cur_stamp[stamp_group]]->min)
#define MAX_STAMP_SIZE (stamp_data[stamp_group][cur_stamp[stamp_group]]->max)

/* to scale some offset, in pixels, like the current stamp is scaled */
#define SCALE_LIKE_STAMP(x) ( ((x) * scaletable[stamp_data[stamp_group][cur_stamp[stamp_group]]->size].numer + scaletable[stamp_data[stamp_group][cur_stamp[stamp_group]]->size].denom-1) / scaletable[stamp_data[stamp_group][cur_stamp[stamp_group]]->size].denom )

/* pixel dimensions of the current stamp, as scaled */
#define CUR_STAMP_W SCALE_LIKE_STAMP(active_stamp->w)
#define CUR_STAMP_H SCALE_LIKE_STAMP(active_stamp->h)


#define REPEAT_SPEED 300        /* Initial repeat speed for scrollbars */
#define CURSOR_BLINK_SPEED 500  /* Initial repeat speed for cursor */


#ifndef _GNU_SOURCE
#define _GNU_SOURCE             /* for strcasestr() */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <libgen.h>             /* EP added this include for basename() */

/* On Linux, we can use 'wordexp()' to expand env. vars. in settings
   pulled from config. files */
#ifdef __linux__

/* However, Android has __linux__ macro but does not support 'wordexp()'*/
#ifndef __ANDROID__
#include <wordexp.h>
#endif

#endif


/* Check if features.h did its 'magic', in which case strcasestr() is
   likely available; if not using GNU, you can set HAVE_STRCASESTR to
   avoid trying to redefine it -bjk 2006.06.02 */

#if !defined(__USE_GNU) && !defined(HAVE_STRCASESTR)
#warning "Attempting to define strcasestr(); if errors, build with -DHAVE_STRCASESTR"

char *strcasestr(const char *haystack, const char *needle)
{
  char *uphaystack, *upneedle, *result;
  unsigned int i;

  uphaystack = strdup(haystack);
  upneedle = strdup(needle);

  if (uphaystack == NULL || upneedle == NULL)
    return (NULL);

  for (i = 0; i < strlen(uphaystack); i++)
    uphaystack[i] = toupper(uphaystack[i]);

  for (i = 0; i < strlen(upneedle); i++)
    upneedle[i] = toupper(upneedle[i]);

  result = strstr(uphaystack, upneedle);

  if (result != NULL)
    return (result - uphaystack + (char *)haystack);
  else
    return NULL;
}

#endif


/* math.h makes y1 an obscure function! */
#define y1 evil_y1
#include <math.h>
#undef y1

#include <locale.h>

#ifdef __HAIKU__
#include <zlib.h>
#include <zconf.h>
#include <FindDirectory.h>
#include <fs_info.h>
#endif
#if defined __BEOS__ || defined __HAIKU__ || defined __APPLE__ || defined __ANDROID__
#include <wchar.h>
#include <stdbool.h>
#ifndef __HAIKU__
#define FALSE false
#define TRUE true
#endif
#else
#include <wchar.h>
#include <wctype.h>
#endif

#include <libintl.h>
#ifndef gettext_noop
#define gettext_noop(String) String
#endif

#ifdef DEBUG
#undef gettext                  /* EP to avoid warning on following line */
#define gettext(String) debug_gettext(String)
#endif


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32

/* Not Windows: */

#include <unistd.h>
#include <dirent.h>
#include <signal.h>

#if defined __BEOS__

/* BeOS */

#include "BeOS_print.h"

/* workaround dirent handling bug in TuxPaint code */
typedef struct safer_dirent
{
  dev_t d_dev;
  dev_t d_pdev;
  ino_t d_ino;
  ino_t d_pino;
  unsigned short d_reclen;
  char d_name[FILENAME_MAX];
} safer_dirent;

#define dirent safer_dirent

#else /* __BEOS__ */

#ifdef __ANDROID__

#define AUTOSAVE_GOING_BACKGROUND
#include "android_print.h"
#include "android_assets.h"

#else

/* Not Windows, not BeOS, not Android */

#include "postscript_print.h"

#endif /* __ANDROID__ */

#endif /* __BEOS__ */

#else /* WIN32 */

/* Windows */

#include <unistd.h>
#include <dirent.h>
#include <malloc.h>
#include "win32_print.h"
#include <io.h>
#include <direct.h>
#include <iconv.h>

#define mkdir(path,access)    _mkdir(path)

static void mtw(wchar_t * wtok, char *tok)
{
  /* workaround using iconv to get a functionallity somewhat approximate as mbstowcs() */
  Uint16 *ui16;

  ui16 = malloc(255);
  char *wrptr = (char *)ui16;
  size_t n, in, out;
  iconv_t trans;
  wchar_t *wch;

  n = 255;
  in = 250;
  out = 250;
  wch = malloc(255);

  trans = iconv_open("WCHAR_T", "UTF-8");
  iconv(trans, (const char **)&tok, &in, &wrptr, &out);
  *((wchar_t *) wrptr) = L'\0';
  swprintf(wtok, L"%ls", ui16);
  free(ui16);
  iconv_close(trans);
}

#endif /* WIN32 */

#ifdef __APPLE__
#include "macos.h"
#endif

#include <errno.h>
#include <sys/stat.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_thread.h"

#if !defined(_SDL_H)
#error "---------------------------------------------------"
#error "If you installed SDL from a package, be sure to get"
#error "the development package, as well!"
#error "(e.g., 'libsdl1.2-devel.rpm')"
#error "---------------------------------------------------"
#endif

#include "SDL2/SDL_image.h"

#if !defined(_SDL_IMAGE_H) && !defined(_IMG_h)
#error "---------------------------------------------------"
#error "If you installed SDL_image from a package, be sure"
#error "to get the development package, as well!"
#error "(e.g., 'libsdl-image1.2-devel.rpm')"
#error "---------------------------------------------------"
#endif

#include "SDL2/SDL_ttf.h"

#if !defined(_SDL_TTF_H) && !defined(_SDLttf_h)
#error "---------------------------------------------------"
#error "If you installed SDL_ttf from a package, be sure"
#error "to get the development package, as well!"
#error "(e.g., 'libsdl-ttf1.2-devel.rpm')"
#error "---------------------------------------------------"
#endif


#ifndef NO_SDLPANGO

/*
 The following section renames global variables defined in SDL_Pango.h to avoid errors during linking.
 It is okay to rename these variables because they are constants.
 SDL_Pango.h is included by tuxpaint.c.
 */
#define _MATRIX_WHITE_BACK _MATRIX_WHITE_BACK0
#define MATRIX_WHITE_BACK MATRIX_WHITE_BACK0
#define _MATRIX_BLACK_BACK _MATRIX_BLACK_BACK0
#define MATRIX_BLACK_BACK MATRIX_BLACK_BACK0
#define _MATRIX_TRANSPARENT_BACK_BLACK_LETTER _MATRIX_TRANSPARENT_BACK_BLACK_LETTER0
#define MATRIX_TRANSPARENT_BACK_BLACK_LETTER MATRIX_TRANSPARENT_BACK_BLACK_LETTER0
#define _MATRIX_TRANSPARENT_BACK_WHITE_LETTER _MATRIX_TRANSPARENT_BACK_WHITE_LETTER0
#define MATRIX_TRANSPARENT_BACK_WHITE_LETTER MATRIX_TRANSPARENT_BACK_WHITE_LETTER0
#define _MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER _MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER0
#define MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER0
/*
 The renaming ends here.
 */

#include "SDL2_Pango.h"
#if !defined(SDL_PANGO_H)
#error "---------------------------------------------------"
#error "If you installed SDL_Pango from a package, be sure"
#error "to get the development package, as well!"
#error "(e.g., 'libsdl-pango1-dev.rpm')"
#error "---------------------------------------------------"
#endif

#endif


#ifndef NOSOUND

#include "SDL2/SDL_mixer.h"

#if !defined(_SDL_MIXER_H) && !defined(_MIXER_H_)
#error "---------------------------------------------------"
#error "If you installed SDL_mixer from a package, be sure"
#error "to get the development package, as well!"
#error "(e.g., 'libsdl-mixer1.2-devel.rpm')"
#error "---------------------------------------------------"
#endif

#endif

#ifndef NOSVG

#ifdef OLD_SVG
#include "cairo.h"
#include "svg.h"
#include "svg-cairo.h"
#if !defined(CAIRO_H) || !defined(SVG_H) || !defined(SVG_CAIRO_H)
#error "---------------------------------------------------"
#error "If you installed Cairo, libSVG or svg-cairo from packages, be sure"
#error "to get the development package, as well!"
#error "(e.g., 'libcairo-dev.rpm')"
#error "---------------------------------------------------"
#endif

#else

#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
/* #include "rsvg.h" */
/* #include "rsvg-cairo.h" */
#if !defined(RSVG_H) || !defined(RSVG_CAIRO_H)
#error "---------------------------------------------------"
#error "If you installed libRSVG from packages, be sure"
#error "to get the development package, as well!"
#error "(e.g., 'librsvg2-dev.rpm')"
#error "---------------------------------------------------"
#endif

#endif

#endif

#include <zlib.h>               /* EP added for PNG upgrade from 1.2 to 1.5 */
#define PNG_INTERNAL
#include <png.h>
#define FNAME_EXTENSION ".png"
#ifndef PNG_H
#error "---------------------------------------------------"
#error "If you installed the PNG libraries from a package,"
#error "be sure to get the development package, as well!"
#error "(e.g., 'libpng2-devel.rpm')"
#error "---------------------------------------------------"
#endif

#define AUTOSAVED_NAME "AUTOSAVED"
//#include "SDL_getenv.h"

#include "i18n.h"
#include "cursor.h"
#include "pixels.h"
#include "rgblinear.h"
#include "playsound.h"
#include "progressbar.h"
#include "fonts.h"
#include "dirwalk.h"
#include "get_fname.h"
#include "onscreen_keyboard.h"

#include "tools.h"
#include "titles.h"
#include "colors.h"
#include "shapes.h"
#include "sounds.h"
#include "tip_tux.h"
#include "great.h"

#include "im.h"


#ifdef DEBUG_MALLOC
#include "malloc.c"
#endif

#include "parse.h"

#include "compiler.h"


/* EP added #ifndef __APPLE__ because macros are buggy (shifted by 1 byte), plus the function exists in SDL */
#ifndef __APPLE__
#if VIDEO_BPP==32
#ifdef __GNUC__
#define SDL_GetRGBA(p,f,rp,gp,bp,ap) ({ \
  unsigned u_p = p;                     \
  *(ap) = (u_p >> 24) & 0xff;           \
  *(rp) = (u_p >> 16) & 0xff;           \
  *(gp) = (u_p >>  8) & 0xff;           \
  *(bp) = (u_p >>  0) & 0xff;           \
})
#define SDL_GetRGB(p,f,rp,gp,bp) ({ \
  unsigned u_p = p;                     \
  *(rp) = (u_p >> 16) & 0xff;           \
  *(gp) = (u_p >>  8) & 0xff;           \
  *(bp) = (u_p >>  0) & 0xff;           \
})
#endif
#define SDL_MapRGBA(f,r,g,b,a) ( \
  (((a) & 0xffu) << 24)          \
  |                              \
  (((r) & 0xffu) << 16)          \
  |                              \
  (((g) & 0xffu) <<  8)          \
  |                              \
  (((b) & 0xffu) <<  0)          \
)
#define SDL_MapRGB(f,r,g,b) (   \
  (((r) & 0xffu) << 16)          \
  |                              \
  (((g) & 0xffu) <<  8)          \
  |                              \
  (((b) & 0xffu) <<  0)          \
)
#endif
#endif


int TP_EventFilter(void *data, const SDL_Event * event);


                                                                     /* #define fmemopen_alternative *//* Uncomment this to test the fmemopen alternative in systems were fmemopen exists */

#if defined (WIN32) || defined (__APPLE__) || defined(__NetBSD__) || defined(__sun) || defined(__ANDROID__)     /* MINGW/MSYS, NetBSD, and MacOSX need it, at least for now */
#define fmemopen_alternative
#endif

#ifdef fmemopen_alternative
#undef fmemopen

FILE *my_fmemopen(unsigned char *data, size_t size, const char *mode);

FILE *my_fmemopen(unsigned char *data, size_t size, const char *mode)
{
  unsigned int i;
  char *fname;
  FILE *fi;

#ifndef WIN32
  fname = get_fname("tmpfile", DIR_SAVE);
#else
  fname = get_temp_fname("tmpfile");
#endif


  fi = fopen(fname, "wb");
  if (fi == NULL)
    {
      free(fname);
      return (NULL);
    }

  for (i = 0; i < size; i++)
    {
      fwrite(data, 1, 1, fi);
      data++;
    }

  fclose(fi);
  fi = fopen(fname, mode);
  free(fname);
  return (fi);
}

#define fmemopen my_fmemopen

#endif


enum
{
  SAVE_OVER_UNSET = -1,
  SAVE_OVER_PROMPT,
  SAVE_OVER_ALWAYS,
  SAVE_OVER_NO
};

enum
{
  ALTPRINT_MOD,
  ALTPRINT_ALWAYS,
  ALTPRINT_NEVER
};


enum
{
  STARTER_OUTLINE,
  STARTER_SCENE
};

enum
{
  LABEL_OFF,
  LABEL_LABEL,
  LABEL_SELECT
    /* , LABEL_ROTATE */
};



/* Color globals (copied from colors.h, if no colors specified by user) */

static int NUM_COLORS;
static Uint8 **color_hexes;
static char **color_names;


/* Show debugging stuff: */

static void debug(const char *const str)
{
#ifndef DEBUG
  (void)str;
#else
  fprintf(stderr, "DEBUG: %s\n", str);
  fflush(stderr);
#endif
}

/* sizing */

/* The old Tux Paint:
   640x480 screen
   448x376 canvas
    40x96  titles near the top
    48x48  button tiles
    ??x56  tux area
    room for 2x7 button tile grids */

typedef struct
{
  Uint8 rows, cols;
} grid_dims;

                                                                                              /* static SDL_Rect r_screen; *//* was 640x480 @ 0,0  -- but this isn't so useful */
static SDL_Rect r_canvas;       /* was 448x376 @ 96,0 */
static SDL_Rect r_tools;        /* was 96x336 @ 0,40 */
static SDL_Rect r_sfx;
static SDL_Rect r_toolopt;      /* was 96x336 @ 544,40 */
static SDL_Rect r_colors;       /* was 544x48 @ 96,376 */
static SDL_Rect r_ttools;       /* was 96x40 @ 0,0  (title for tools, "Tools") */
static SDL_Rect r_tcolors;      /* was 96x48 @ 0,376 (title for colors, "Colors") */
static SDL_Rect r_ttoolopt;     /* was 96x40 @ 544,0 (title for tool options) */
static SDL_Rect r_tuxarea;      /* was 640x56 */
static SDL_Rect r_label;
static SDL_Rect old_dest;
static SDL_Rect r_tir;          /* Text input rectangle */
static float render_scale;      /* Scale factor for the render */

static int button_w;            /* was 48 */
static int button_h;            /* was 48 */

static int color_button_w;      /* was 32 */
static int color_button_h;      /* was 48 */

/* Define button grid dimensions. (in button units) */
/* These are the maximum slots -- some may be unused. */
static grid_dims gd_tools;      /* was 2x7 */
static grid_dims gd_sfx;
static grid_dims gd_toolopt;    /* was 2x7 */

                                                                                                /* static grid_dims gd_open; *//* was 4x4 */
static grid_dims gd_colors;     /* was 17x1 */

#define HEIGHTOFFSET (((WINDOW_HEIGHT - 480) / 48) * 48)
#define TOOLOFFSET (HEIGHTOFFSET / 48 * 2)
#define PROMPTOFFSETX (WINDOW_WIDTH - 640) / 2
#define PROMPTOFFSETY (HEIGHTOFFSET / 2)

#define THUMB_W ((WINDOW_WIDTH - 96 - 96) / 4)
#define THUMB_H (((48 * 7 + 40 + HEIGHTOFFSET) - 72) / 4)

#ifdef NOKIA_770
static int WINDOW_WIDTH = 800;
static int WINDOW_HEIGHT = 480;
#elif defined(OLPC_XO)
/* ideally we'd support rotation and 2x scaling */
static int WINDOW_WIDTH = 1200;
static int WINDOW_HEIGHT = 900;
#else
static int WINDOW_WIDTH = 800;
static int WINDOW_HEIGHT = 600;
#endif

static void magic_putpixel(SDL_Surface * surface, int x, int y, Uint32 pixel);
static Uint32 magic_getpixel(SDL_Surface * surface, int x, int y);


static void setup_normal_screen_layout(void)
{
  int buttons_tall;

  button_w = 48;
  button_h = 48;

  gd_toolopt.cols = 2;
  gd_tools.cols = 2;

  r_ttools.x = 0;
  r_ttools.y = 0;
  r_ttools.w = gd_tools.cols * button_w;
  r_ttools.h = 40;

  r_ttoolopt.w = gd_toolopt.cols * button_w;
  r_ttoolopt.h = 40;
  r_ttoolopt.x = WINDOW_WIDTH - r_ttoolopt.w;
  r_ttoolopt.y = 0;

  gd_colors.rows = 1;
  gd_colors.cols = (NUM_COLORS + gd_colors.rows - 1) / gd_colors.rows;

  r_colors.h = 48;
  color_button_h = r_colors.h / gd_colors.rows;
  r_tcolors.h = r_colors.h;

  r_tcolors.x = 0;
  r_tcolors.w = gd_tools.cols * button_w;
  r_colors.x = r_tcolors.w;
  r_colors.w = WINDOW_WIDTH - r_tcolors.w;

  color_button_w = r_colors.w / gd_colors.cols;

  /* This would make it contain _just_ the color spots,
     without any leftover bit on the end. Hmmm... */
  /* r_colors.w = color_button_w * gd_colors.cols; */

  r_canvas.x = gd_tools.cols * button_w;
  r_canvas.y = 0;
  r_canvas.w = WINDOW_WIDTH - (gd_tools.cols + gd_toolopt.cols) * button_w;

  r_tuxarea.x = 0;
  r_tuxarea.w = WINDOW_WIDTH;

  /* need 56 minimum for the Tux area */
  buttons_tall = (WINDOW_HEIGHT - r_ttoolopt.h - 56 - r_colors.h) / button_h;
  gd_tools.rows = buttons_tall;
  gd_toolopt.rows = buttons_tall;

  r_canvas.h = r_ttoolopt.h + buttons_tall * button_h;

  r_label = r_canvas;

  r_colors.y = r_canvas.h + r_canvas.y;
  r_tcolors.y = r_canvas.h + r_canvas.y;

  r_tuxarea.y = r_colors.y + r_colors.h;
  r_tuxarea.h = WINDOW_HEIGHT - r_tuxarea.y;

  r_sfx.x = r_tuxarea.x;
  r_sfx.y = r_tuxarea.y;
  r_sfx.w = button_w;           /* Two half-sized buttons across */
  r_sfx.h = button_h >> 1;      /* One half-sized button down */

  gd_sfx.rows = 1;
  gd_sfx.cols = 2;

  r_tools.x = 0;
  r_tools.y = r_ttools.h + r_ttools.y;
  r_tools.w = gd_tools.cols * button_w;
  r_tools.h = gd_tools.rows * button_h;

  r_toolopt.w = gd_toolopt.cols * button_w;
  r_toolopt.h = gd_toolopt.rows * button_h;
  r_toolopt.x = WINDOW_WIDTH - r_ttoolopt.w;
  r_toolopt.y = r_ttoolopt.h + r_ttoolopt.y;

  /* TODO: dialog boxes */

}

#ifdef DEBUG
static void debug_rect(SDL_Rect * r, char *name)
{
  printf("%-12s %dx%d @ %d,%d\n", name, r->w, r->h, r->x, r->y);
}

#define DR(x) debug_rect(&x, #x)

static void debug_dims(grid_dims * g, char *name)
{
  printf("%-12s %dx%d\n", name, g->cols, g->rows);
}

#define DD(x) debug_dims(&x, #x)

static void print_layout(void)
{
  printf("\n--- layout ---\n");
  DR(r_canvas);
  DR(r_tools);
  DR(r_toolopt);
  DR(r_colors);
  DR(r_ttools);
  DR(r_tcolors);
  DR(r_ttoolopt);
  DR(r_tuxarea);
  DD(gd_tools);
  DD(gd_toolopt);
  DD(gd_colors);
  printf("buttons are %dx%d\n", button_w, button_h);
  printf("color buttons are %dx%d\n", color_button_w, color_button_h);
}

#undef DD
#undef DR
#endif

static void setup_screen_layout(void)
{
  /* can do right-to-left, colors at the top, extra tool option columns, etc. */
  setup_normal_screen_layout();
#ifdef DEBUG
  print_layout();
#endif
}

static SDL_Window *window_screen;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static SDL_Surface *screen = NULL;
static SDL_Surface *canvas = NULL;
static SDL_Surface *label = NULL;
static SDL_Surface *save_canvas = NULL;
static SDL_Surface *canvas_back = NULL;
static SDL_Surface *img_starter = NULL, *img_starter_bkgd = NULL;

/* This SDL 1.2 <-> 2.0 backward compatibility functions are focused to the main window */

static Uint32 window_format;

#define SDLKey SDL_Keycode
#define SDLMod SDL_Keymod

static void SDL_WarpMouse(Uint16 x, Uint16 y)
{
  SDL_WarpMouseInWindow(window_screen, x, y);
}

static SDL_Surface *SDL_DisplayFormat(SDL_Surface * surface)
{
  SDL_Surface *tmp;

  tmp = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB888, 0);
  return (tmp);
}

static SDL_Surface *SDL_DisplayFormatAlpha(SDL_Surface * surface)
{
  SDL_Surface *tmp;

  tmp = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
  return (tmp);
}

static void SDL_Flip(SDL_Surface * screen)
{
  //SDL_UpdateWindowSurface(window_screen);
  SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

static void SDL_UpdateRect(SDL_Surface * screen, Sint32 x, Sint32 y, Sint32 w, Sint32 h)
{
  SDL_Rect r;

  r.x = x;
  r.y = y;
  r.w = w;
  r.h = h;

  SDL_UpdateTexture(texture, &r, screen->pixels + (y * screen->pitch + x * 4), screen->pitch);

  //  Docs says one must clear the renderer, even if this means a refresh of the whole thing.
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);

  SDL_RenderPresent(renderer);
}

static void show_progress_bar(SDL_Surface * screen)
{
  show_progress_bar_(screen, texture, renderer);
}


/* Update a rect. based on two x/y coords (not necessarly in order): */
static void update_screen(int x1, int y1, int x2, int y2)
{
  int tmp;

  if (x1 > x2)
    {
      tmp = x1;
      x1 = x2;
      x2 = tmp;
    }

  if (y1 > y2)
    {
      tmp = y1;
      y1 = y2;
      y2 = tmp;
    }

  x1 = x1 - 1;
  x2 = x2 + 1;
  y1 = y1 - 1;
  y2 = y2 + 1;


  if (x1 < 0)
    x1 = 0;
  if (x2 < 0)
    x2 = 0;
  if (y1 < 0)
    y1 = 0;
  if (y2 < 0)
    y2 = 0;

  if (x1 >= WINDOW_WIDTH)
    x1 = WINDOW_WIDTH - 1;
  if (x2 >= WINDOW_WIDTH)
    x2 = WINDOW_WIDTH - 1;
  if (y1 >= WINDOW_HEIGHT)
    y1 = WINDOW_HEIGHT - 1;
  if (y2 >= WINDOW_HEIGHT)
    y2 = WINDOW_HEIGHT - 1;

  SDL_UpdateRect(screen, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}


static void update_screen_rect(SDL_Rect * r)
{
  SDL_UpdateRect(screen, r->x, r->y, r->w, r->h);
}

static int hit_test(const SDL_Rect * const r, unsigned x, unsigned y)
{
  /* note the use of unsigned math: no need to check for negative */
  return x - r->x < r->w && y - r->y < r->h;
}

#define HIT(r) hit_test(&(r), event.button.x, event.button.y)


/* "#if"ing out, since unused; bjk 2005.01.09 */

#if 0

/* x,y are pixel-wise screen-relative (mouse location), not grid-wise
   w,h are the size of a grid item
   Return the grid box.
   NOTE: grid items must fill full SDL_Rect width exactly */
static int grid_hit_wh(const SDL_Rect * const r, unsigned x, unsigned y, unsigned w, unsigned h)
{
  return (x - r->x) / w + (y - r->y) / h * (r->w / w);
}

/* test an SDL_Rect r containing an array of WxH items for a grid location */
#define GRIDHIT_WH(r,W,H) grid_hit_wh(&(r), event.button.x, event.button.y, W,H)

#endif

/* test an SDL_Rect r containing an array of SDL_Surface surf for a grid location */
#define GRIDHIT_SURF(r,surf) grid_hit_wh(&(r), event.button.x, event.button.y, (surf)->w, (surf)->h)

/* x,y are pixel-wise screen-relative (mouse location), not grid-wise
   Return the grid box.
   NOTE: returns -1 if hit is below or to the right of the grid */
static int grid_hit_gd(const SDL_Rect * const r, unsigned x, unsigned y, grid_dims * gd)
{
  unsigned item_w = r->w / gd->cols;
  unsigned item_h = r->h / gd->rows;
  unsigned col = (x - r->x) / item_w;
  unsigned row = (y - r->y) / item_h;

#ifdef DEBUG
  printf("%d,%d resolves to %d,%d in a %dx%d grid, index is %d\n", x, y, col,
         row, gd->cols, gd->rows, col + row * gd->cols);
#endif
  if (col >= gd->cols || row >= gd->rows)
    return -1;
  return col + row * gd->cols;
}

/* test an SDL_Rect r for a grid location, based on a grid_dims gd */
#define GRIDHIT_GD(r,gd) grid_hit_gd(&(r), event.button.x, event.button.y, &(gd))

/* One global variable defined here so that update_canvas() need not be moved below */

#if VIDEO_BPP != 32
static int disable_label = 1;
#else
static int disable_label;
#endif

/* Update the contents of a region */
static void update_canvas_ex_r(int x1, int y1, int x2, int y2, int screen_too)
{
  SDL_Rect src, dest;

  src.x = x1;
  src.y = y1;
  src.w = x2 - x1 + 1;
  src.h = y2 - y1 + 1;

  dest.x = x1;
  dest.y = y1;
  dest.w = src.w;
  dest.h = src.h;

  if (img_starter != NULL)
    {
      /* If there was a starter, cover this part of the drawing with
         the corresponding part of the starter's foreground! */

      SDL_BlitSurface(img_starter, &dest, canvas, &dest);
    }

  dest.x = x1 + 96;

  SDL_BlitSurface(canvas, &src, screen, &dest);

  /* If label is not disabled, cover canvas with label layer */

  if (!disable_label)
    SDL_BlitSurface(label, &src, screen, &dest);

  if (screen_too)
    update_screen(x1 + 96, y1, x2 + 96, y2);
}

static void update_canvas_ex(int x1, int y1, int x2, int y2, int screen_too)
{
  SDL_Rect src, dest;

  if (img_starter != NULL)
    {
      /* If there was a starter, cover this part of the drawing with
         the corresponding part of the starter's foreground! */

      src.x = x1;
      src.y = y1;
      src.w = x2 - x1 + 1;
      src.h = y2 - y1 + 1;

      dest.x = x1;
      dest.y = y1;
      dest.w = src.w;
      dest.h = src.h;

      SDL_BlitSurface(img_starter, &dest, canvas, &dest);
    }

  SDL_BlitSurface(canvas, NULL, screen, &r_canvas);

  /* If label is not disabled, cover canvas with label layer */

  if (!disable_label)
    SDL_BlitSurface(label, NULL, screen, &r_label);

  if (screen_too)
    update_screen(x1 + 96, y1, x2 + 96, y2);
}

/* Update the screen with the new canvas: */
static void update_canvas(int x1, int y1, int x2, int y2)
{
  update_canvas_ex(x1, y1, x2, y2, 1);
}


/* Globals: */
static int emulate_button_pressed = 0;
static int mouseaccessibility = 0;
static int onscreen_keyboard = 0;
static char *onscreen_keyboard_layout = NULL;
static on_screen_keyboard *kbd = NULL;
static int onscreen_keyboard_disable_change = 0;
static int joystick_low_threshold = 3200;
static int joystick_slowness = 15;
static int joystick_maxsteps = 7;
static int joystick_hat_slowness = 15;
static Uint32 joystick_hat_timeout = 1000;
static int joystick_button_escape = 255;
static int joystick_button_selectbrushtool = 255;
static int joystick_button_selectstamptool = 255;
static int joystick_button_selectlinestool = 255;
static int joystick_button_selectshapestool = 255;
static int joystick_button_selecttexttool = 255;
static int joystick_button_selectlabeltool = 255;
static int joystick_button_selectmagictool = 255;
static int joystick_button_undo = 255;
static int joystick_button_redo = 255;
static int joystick_button_selecterasertool = 255;
static int joystick_button_new = 255;
static int joystick_button_open = 255;
static int joystick_button_save = 255;
static int joystick_button_pagesetup = 255;
static int joystick_button_print = 255;
static int joystick_buttons_ignore_len = 0;
static int joystick_buttons_ignore[256];
static Uint32 old_hat_ticks = 0;
static int oldpos_x;
static int oldpos_y;
static int disable_screensaver;

#ifdef NOKIA_770
static int fullscreen = 1;
#else
static int fullscreen;
#endif
static int native_screensize;
static int grab_input;
static int rotate_orientation;
static int joystick_dev = 0;

static int disable_print;
static int print_delay;
static int use_print_config = 1;
static int alt_print_command_default = ALTPRINT_MOD;
static int want_alt_printcommand;

static int wheely = 1;
static int keymouse = 0;
static int no_button_distinction;
static int button_down;
static int scrolling;

static int promptless_save = SAVE_OVER_UNSET;
static int _promptless_save_over, _promptless_save_over_ask, _promptless_save_over_new;
static int disable_quit;

static int noshortcuts;
static int disable_save;
static int ok_to_use_lockfile = 1;
static int start_blank;
static int autosave_on_quit;

static int dont_do_xor;
static int dont_load_stamps;
static int mirrorstamps;
static int disable_stamp_controls;
static int stamp_size_override = -1;

#ifdef NOKIA_770
static int simple_shapes = 1;
#else
static int simple_shapes;
#endif
static int only_uppercase;

static int disable_magic_controls;

static int starter_mirrored;
static int starter_flipped;
static int starter_personal;
static int template_personal;
static int starter_modified;

static Uint8 canvas_color_r, canvas_color_g, canvas_color_b;
static Uint8 *touched;
static int last_print_time = 0;

static int shape_radius;

/* Text label tool struct to hold information about text on the label layer */
typedef struct label_node
{
  unsigned int save_texttool_len;
  wchar_t save_texttool_str[256];
  SDL_Color save_color;
  int save_width;
  int save_height;
  Uint16 save_x;
  Uint16 save_y;
  int save_cur_font;
  char *save_font_type;
  int save_text_state;
  unsigned save_text_size;
  int save_undoid;
  int is_enabled;
  struct label_node *disables;
  struct label_node *next_to_up_label_node;
  struct label_node *next_to_down_label_node;
  SDL_Surface *label_node_surface;
} label_node;


static struct label_node *start_label_node;
static struct label_node *current_label_node;
static struct label_node *first_label_node_in_redo_stack;
static struct label_node *label_node_to_edit;
static struct label_node *highlighted_label_node;

static unsigned int select_texttool_len;
static wchar_t select_texttool_str[256];
static unsigned select_color;
static int select_width;
static int select_height;
static Uint16 select_x;
static Uint16 select_y;
static int select_cur_font;
static int select_text_state;
static unsigned select_text_size;
static int coming_from_undo_or_redo = FALSE;


static void add_label_node(int, int, Uint16, Uint16, SDL_Surface * label_node_surface);
static void load_info_about_label_surface(FILE * lfi);

static struct label_node *search_label_list(struct label_node **, Uint16, Uint16, int hover);
static void highlight_label_nodes(void);
static void cycle_highlighted_label_node(void);
static int are_labels(void);

static void do_undo_label_node(void);
static void do_redo_label_node(void);
static void rec_undo_label(void);

static void render_all_nodes_starting_at(struct label_node **);
static void simply_render_node(struct label_node *);

static void derender_node(struct label_node **);

static void delete_label_list(struct label_node **);

static void myblit(SDL_Surface * src_surf, SDL_Rect * src_rect, SDL_Surface * dest_surf, SDL_Rect * dest_rect);

static void set_label_fonts(void);

static void tmp_apply_uncommited_text(void);
static void undo_tmp_applied_text(void);

static void handle_joyaxismotion(SDL_Event event, int *motioner, int *val_x, int *val_y);
static void handle_joyhatmotion(SDL_Event event, int oldpos_x, int oldpos_y, int *valhat_x, int *valhat_y,
                                int *hat_motioner, Uint32 * old_hat_ticks);
static void handle_joyballmotion(SDL_Event event, int oldpos_x, int oldpos_y);
static void handle_joybuttonupdown(SDL_Event event, int oldpos_x, int oldpos_y);
static void handle_motioners(int oldpos_x, int oldpos_y, int motioner, int hatmotioner, int old_hat_ticks, int val_x,
                             int val_y, int valhat_x, int valhat_y);

static void handle_joybuttonupdownscl(SDL_Event event, int oldpos_x, int oldpos_y, SDL_Rect real_r_tools);

#ifdef __ANDROID__
static void start_motion_convert(SDL_Event event);
static void convert_motion_to_wheel(SDL_Event event);
static void stop_motion_convert(SDL_Event event);
#endif

/* Magic tools API and tool handles: */

#include "tp_magic_api.h"

static void update_progress_bar(void);
static void special_notify(int flags);

typedef struct magic_funcs_s
{
  int (*get_tool_count) (magic_api *);
  char *(*get_name) (magic_api *, int);
  SDL_Surface *(*get_icon) (magic_api *, int);
  char *(*get_description) (magic_api *, int, int);
  int (*requires_colors) (magic_api *, int);
  int (*modes) (magic_api *, int);
  void (*set_color) (magic_api *, Uint8, Uint8, Uint8);
  int (*init) (magic_api *);
   Uint32(*api_version) (void);
  void (*shutdown) (magic_api *);
  void (*click) (magic_api *, int, int, SDL_Surface *, SDL_Surface *, int, int, SDL_Rect *);
  void (*drag) (magic_api *, int, SDL_Surface *, SDL_Surface *, int, int, int, int, SDL_Rect *);
  void (*release) (magic_api *, int, SDL_Surface *, SDL_Surface *, int, int, SDL_Rect *);
  void (*switchin) (magic_api *, int, int, SDL_Surface *, SDL_Surface *);
  void (*switchout) (magic_api *, int, int, SDL_Surface *, SDL_Surface *);
} magic_funcs_t;


typedef struct magic_s
{
  int place;
  int handle_idx;               /* Index to magic funcs for each magic tool (shared objs may report more than 1 tool) */
  int idx;                      /* Index to magic tools within shared objects (shared objs may report more than 1 tool) */
  int mode;                     /* Current mode (paint or fullscreen) */
  int avail_modes;              /* Available modes (paint &/or fullscreen) */
  int colors;                   /* Whether magic tool accepts colors */
  char *name;                   /* Name of magic tool */
  char *tip[MAX_MODES];         /* Description of magic tool, in each possible mode */
  SDL_Surface *img_icon;
  SDL_Surface *img_name;
} magic_t;


/* FIXME: Drop the 512 constants :^P */

static int num_plugin_files;    /* How many shared object files we went through */
static void *magic_handle[512]; /* Handle to shared object (to be unloaded later) *//* FIXME: Unload them! */
static magic_funcs_t magic_funcs[512];  /* Pointer to shared objects' functions */

static magic_t magics[512];
static int num_magics;          /* How many magic tools were loaded (note: shared objs may report more than 1 tool) */

enum
{
  MAGIC_PLACE_GLOBAL,
  MAGIC_PLACE_LOCAL,
#ifdef __APPLE__
  MAGIC_PLACE_ALLUSERS,
#endif
  NUM_MAGIC_PLACES
};

static magic_api *magic_api_struct;     /* Pointer to our internal functions; passed to shared object's functions when we call them */


#if !defined(WIN32) && !defined(__BEOS__) && !defined(__HAIKU__) && !defined(__ANDROID__)
#include <paper.h>
#if !defined(PAPER_H)
#error "---------------------------------------------------"
#error "If you installed libpaper from a package, be sure"
#error "to get the development package, as well!"
#error "(eg., 'libpaper-dev_1.1.21.deb')"
#error "---------------------------------------------------"
#endif
static const char *printcommand = PRINTCOMMAND;
static const char *altprintcommand = ALTPRINTCOMMAND;
static const char *papersize;
#endif


#if 1                           /* FIXME: ifdef for platforms that lack fribidi? */
#include <fribidi/fribidi.h>
#if !defined(_FRIBIDI_H) && !defined(FRIBIDI_H)
#error "---------------------------------------------------"
#error "If you installed libfribidi from a package, be sure"
#error "to get the development package, as well!"
#error "(eg., 'libfribidi-dev')"
#error "---------------------------------------------------"
#endif
#else
/* FIXME: define a noop function */
#endif

enum
{
  UNDO_STARTER_NONE,
  UNDO_STARTER_MIRRORED,
  UNDO_STARTER_FLIPPED
};

#define NUM_UNDO_BUFS 20
static SDL_Surface *undo_bufs[NUM_UNDO_BUFS];
static int undo_starters[NUM_UNDO_BUFS];
static int cur_undo, oldest_undo, newest_undo;
static int text_undo[NUM_UNDO_BUFS];
static int have_to_rec_label_node;
static int have_to_rec_label_node_back;
static SDL_Surface *img_title, *img_title_credits, *img_title_tuxpaint;
static SDL_Surface *img_btn_up, *img_btn_down, *img_btn_off;
static SDL_Surface *img_btnsm_up, *img_btnsm_off, *img_btnsm_down, *img_btnsm_hold;
static SDL_Surface *img_btn_nav, *img_btnsm_nav;
static SDL_Surface *img_prev, *img_next;
static SDL_Surface *img_mirror, *img_flip;
static SDL_Surface *img_dead40x40;
static SDL_Surface *img_black, *img_grey;
static SDL_Surface *img_yes, *img_no;
static SDL_Surface *img_sfx, *img_speak;
static SDL_Surface *img_open, *img_erase, *img_back, *img_trash;
static SDL_Surface *img_slideshow, *img_play, *img_select_digits;
static SDL_Surface *img_printer, *img_printer_wait;
static SDL_Surface *img_save_over, *img_popup_arrow;
static SDL_Surface *img_cursor_up, *img_cursor_down;
static SDL_Surface *img_cursor_starter_up, *img_cursor_starter_down;
static SDL_Surface *img_scroll_up, *img_scroll_down;
static SDL_Surface *img_scroll_up_off, *img_scroll_down_off;
static SDL_Surface *img_grow, *img_shrink;
static SDL_Surface *img_magic_paint, *img_magic_fullscreen;
static SDL_Surface *img_bold, *img_italic;
static SDL_Surface *img_label, *img_label_select;
static SDL_Surface *img_color_picker, *img_color_picker_thumb, *img_paintwell, *img_color_sel;
static int color_picker_x, color_picker_y;

static SDL_Surface *img_title_on, *img_title_off, *img_title_large_on, *img_title_large_off;
static SDL_Surface *img_title_names[NUM_TITLES];
static SDL_Surface *img_tools[NUM_TOOLS], *img_tool_names[NUM_TOOLS];

static SDL_Surface *img_oskdel, *img_osktab, *img_oskenter, *img_oskcapslock, *img_oskshift;
static SDL_Surface *thumbnail(SDL_Surface * src, int max_x, int max_y, int keep_aspect);
static SDL_Surface *thumbnail2(SDL_Surface * src, int max_x, int max_y, int keep_aspect, int keep_alpha);

#ifndef NO_BILINEAR
static SDL_Surface *zoom(SDL_Surface * src, int new_x, int new_y);
#endif



static SDL_Surface *render_text(TuxPaint_Font * restrict font, const char *restrict str, SDL_Color color)
{
  SDL_Surface *ret = NULL;
  int height;

#ifndef NO_SDLPANGO
  SDLPango_Matrix pango_color;
#endif

  if (font == NULL)
    {
      printf("render_text() received a NULL font!\n");
      fflush(stdout);
      return NULL;
    }

#ifndef NO_SDLPANGO
  if (font->typ == FONT_TYPE_PANGO)
    {
      sdl_color_to_pango_color(color, &pango_color);

#ifdef DEBUG
      printf("Calling SDLPango_SetText(\"%s\")\n", str);
      fflush(stdout);
#endif

#ifdef __ANDROID__
      /* FIXME This extrange workaround helps in getting the translations working
         on 4.3 4.4 */
      SDLPango_SetLanguage(font->pango_context, "ca");
#endif

      SDLPango_SetDefaultColor(font->pango_context, &pango_color);
      SDLPango_SetText(font->pango_context, str, -1);
      ret = SDLPango_CreateSurfaceDraw(font->pango_context);
    }
#endif

  if (font->typ == FONT_TYPE_TTF)
    {
#ifdef DEBUG
      printf("Calling TTF_RenderUTF8_Blended(\"%s\")\n", str);
      fflush(stdout);
#endif

      ret = TTF_RenderUTF8_Blended(font->ttf_font, str, color);
    }

  if (ret)
    return ret;

  /* Sometimes a font will be missing a character we need. Sometimes the library
     will substitute a rectangle without telling us. Sometimes it returns NULL.
     Probably we should use FreeType directly. For now though... */

  height = 2;

  return thumbnail(img_title_large_off, height * strlen(str) / 2, height, 0);
}


/* This conversion is required on platforms where Uint16 doesn't match wchar_t.
   On Windows, wchar_t is 16-bit, elsewhere it is 32-bit.
   Mismatch caused by the use of Uint16 for unicode characters by SDL, SDL_ttf.
   I guess wchar_t is really only suitable for internal use ... */
static Uint16 *wcstou16(const wchar_t * str)
{
  unsigned int i, len = wcslen(str);
  Uint16 *res = malloc((len + 1) * sizeof(Uint16));

  for (i = 0; i < len + 1; ++i)
    {
      /* This is a bodge, but it seems unlikely that a case-conversion
         will cause a change from one utf16 character into two....
         (though at least UTF-8 suffers from this problem) */

      /* FIXME: mangles non-BMP characters rather than using UTF-16 surrogates! */
      res[i] = (Uint16) str[i];
    }

  return res;
}


static SDL_Surface *render_text_w(TuxPaint_Font * restrict font, const wchar_t * restrict str, SDL_Color color)
{
  SDL_Surface *ret = NULL;
  int height;
  Uint16 *ustr;

#ifndef NO_SDLPANGO
  unsigned int i, j;
  int utfstr_max;
  char *utfstr;
  SDLPango_Matrix pango_color;
#endif

#ifndef NO_SDLPANGO
  if (font->typ == FONT_TYPE_PANGO)
    {
      sdl_color_to_pango_color(color, &pango_color);

      SDLPango_SetDefaultColor(font->pango_context, &pango_color);

      /* Convert from 16-bit UNICODE to UTF-8 encoded for SDL_Pango: */

      utfstr_max = (sizeof(char) * 4 * (wcslen(str) + 1));
      utfstr = (char *)malloc(utfstr_max);
      memset(utfstr, 0, utfstr_max);

      j = 0;
      for (i = 0; i < wcslen(str); i++)
        {
          if (str[i] <= 0x0000007F)
            {
              /* 0x00000000 - 0x0000007F:
                 0xxxxxxx */

              utfstr[j++] = (str[i] & 0x7F);
            }
          else if (str[i] <= 0x000007FF)
            {
              /* 0x00000080 - 0x000007FF:

                 00000abc defghijk
                 110abcde 10fghijk */

              utfstr[j++] = (((str[i] & 0x0700) >> 6) | /* -----abc -------- to ---abc-- */
                             ((str[i] & 0x00C0) >> 6) | /* -------- de------ to ------de */
                             (0xC0));   /*                  add 110----- */

              utfstr[j++] = (((str[i] & 0x003F)) |      /* -------- --fghijk to --fghijk */
                             (0x80));   /*                  add 10------ */
            }
          else if (str[i] <= 0x0000FFFF)
            {
              /* 0x00000800 - 0x0000FFFF:

                 abcdefgh ijklmnop
                 1110abcd 10efghij 10klmnop */

              utfstr[j++] = (((str[i] & 0xF000) >> 12) |        /* abcd---- -------- to ----abcd */
                             (0xE0));   /*                  add 1110---- */
              utfstr[j++] = (((str[i] & 0x0FC0) >> 6) | /* ----efgh ij------ to --efghij */
                             (0x80));   /*                  add 10------ */
              utfstr[j++] = (((str[i] & 0x003F)) |      /* -------- --klmnop to --klmnop */
                             (0x80));   /*                  add 10------ */
            }
          else
            {
              /* 0x00010000 - 0x001FFFFF:
                 11110abc 10defghi 10jklmno 10pqrstu */

              utfstr[j++] = (((str[i] & 0x1C0000) >> 18) |      /* ---abc-- -------- --------  to -----abc */
                             (0xF0));   /*                            add 11110000 */
              utfstr[j++] = (((str[i] & 0x030000) >> 12) |      /* ------de -------- --------  to --de---- */
                             ((str[i] & 0x00F000) >> 12) |      /* -------- fghi---- --------  to ----fghi */
                             (0x80));   /*                            add 10------ */
              utfstr[j++] = (((str[i] & 0x000F00) >> 6) |       /* -------- ----jklm --------  to --jklm-- */
                             ((str[i] & 0x0000C0) >> 6) |       /* -------- -------- no------  to ------no */
                             (0x80));   /*                            add 10------ */
              utfstr[j++] = ((str[i] & 0x00003F) |      /* -------- -------- --pqrstu  to --prqstu */
                             (0x80));   /*                            add 10------ */
            }
        }
      utfstr[j] = '\0';


      SDLPango_SetText(font->pango_context, utfstr, -1);
      ret = SDLPango_CreateSurfaceDraw(font->pango_context);
    }
#endif

  if (font->typ == FONT_TYPE_TTF)
    {
      ustr = wcstou16(str);
      ret = TTF_RenderUNICODE_Blended(font->ttf_font, ustr, color);
      free(ustr);
    }

  if (ret)
    return ret;

  /* Sometimes a font will be missing a character we need. Sometimes the library
     will substitute a rectangle without telling us. Sometimes it returns NULL.
     Probably we should use FreeType directly. For now though... */

  height = 2;
  return thumbnail(img_title_large_off, height * wcslen(str) / 2, height, 0);
}


typedef struct stamp_type
{
  char *stampname;
  char *stxt;
  Uint8 locale_text;
#ifndef NOSOUND
  Mix_Chunk *ssnd;
  Mix_Chunk *sdesc;
#endif

  SDL_Surface *thumbnail;
  unsigned thumb_mirrored:1;
  unsigned thumb_flipped:1;
  unsigned thumb_mirrored_flipped:1;
  unsigned no_premirror:1;
  unsigned no_preflip:1;
  unsigned no_premirrorflip:1;

  unsigned processed:1;         /* got *.dat, computed size limits, etc. */

  unsigned no_sound:1;
  unsigned no_descsound:1;
  unsigned no_txt:1;
  /*  unsigned no_local_sound : 1;  *//* to remember, if code written to discard sound */

  unsigned tinter:3;
  unsigned colorable:1;
  unsigned tintable:1;
  unsigned mirrorable:1;
  unsigned flipable:1;

  unsigned mirrored:1;
  unsigned flipped:1;
  unsigned min:5;
  unsigned size:5;
  unsigned max:5;

  unsigned is_svg:1;
} stamp_type;

#define MAX_STAMP_GROUPS 256

static unsigned int stamp_group_dir_depth = 1;  /* How deep (how many slashes in a subdirectory path) we think a new stamp group should be */

static int stamp_group = 0;

static const char *load_stamp_basedir;
static int num_stamp_groups = 0;
static int num_stamps[MAX_STAMP_GROUPS];
static int max_stamps[MAX_STAMP_GROUPS];
static stamp_type **stamp_data[MAX_STAMP_GROUPS];

static SDL_Surface *active_stamp;

/* Returns whether a particular stamp can be colored: */
static int stamp_colorable(int stamp)
{
  return stamp_data[stamp_group][stamp]->colorable;
}

/* Returns whether a particular stamp can be tinted: */
static int stamp_tintable(int stamp)
{
  return stamp_data[stamp_group][stamp]->tintable;
}



#define SHAPE_BRUSH_NAME "aa_round_03.png"
static int num_brushes, num_brushes_max, shape_brush = 0;
static SDL_Surface **img_brushes;
static int *brushes_frames = NULL;
static int *brushes_spacing = NULL;
static short *brushes_directional = NULL;

static SDL_Surface *img_shapes[NUM_SHAPES], *img_shape_names[NUM_SHAPES];
static SDL_Surface *img_openlabels_open, *img_openlabels_erase,
  *img_openlabels_slideshow, *img_openlabels_back, *img_openlabels_play, *img_openlabels_next;

static SDL_Surface *img_tux[NUM_TIP_TUX];

static SDL_Surface *img_mouse, *img_mouse_click;

#ifdef LOW_QUALITY_COLOR_SELECTOR
static SDL_Surface *img_paintcan;
#else
static SDL_Surface **img_color_btns;
static SDL_Surface *img_color_btn_off;
#endif

static int colors_are_selectable;

enum
{
  BRUSH_DIRECTION_RIGHT,
  BRUSH_DIRECTION_DOWN_RIGHT,
  BRUSH_DIRECTION_DOWN,
  BRUSH_DIRECTION_DOWN_LEFT,
  BRUSH_DIRECTION_LEFT,
  BRUSH_DIRECTION_UP_LEFT,
  BRUSH_DIRECTION_UP,
  BRUSH_DIRECTION_UP_RIGHT,
  BRUSH_DIRECTION_NONE
};

static SDL_Surface *img_cur_brush;
static int img_cur_brush_frame_w, img_cur_brush_w, img_cur_brush_h,
  img_cur_brush_frames, img_cur_brush_directional, img_cur_brush_spacing;
static int brush_counter, brush_frame;

#define NUM_ERASERS 12          /* How many sizes of erasers
                                   (from ERASER_MIN to _MAX as squares, then again
                                   from ERASER_MIN to _MAX as circles) */
#define ERASER_MIN 13
#define ERASER_MAX 128



static unsigned cur_color;
static int cur_tool, cur_brush, old_tool;
static int cur_stamp[MAX_STAMP_GROUPS];
static int cur_shape, cur_magic;
static int cur_font, cur_eraser;
static int cursor_left, cursor_x, cursor_y, cursor_textwidth;   /* canvas-relative */
static int old_cursor_x, old_cursor_y;
static int cur_label, cur_select;
static int been_saved;
static char file_id[FILENAME_MAX];
static char starter_id[FILENAME_MAX];
static char template_id[FILENAME_MAX];
static int brush_scroll;
static int stamp_scroll[MAX_STAMP_GROUPS];
static int font_scroll, magic_scroll, tool_scroll;
static int eraser_scroll, shape_scroll; /* dummy variables for now */

static int eraser_sound;

static IM_DATA im_data;
static wchar_t texttool_str[256];
static unsigned int texttool_len;

static int tool_avail[NUM_TOOLS], tool_avail_bak[NUM_TOOLS];

static Uint32 cur_toggle_count;

typedef struct edge_type
{
  int y_upper;
  float x_intersect, dx_per_scan;
  struct edge_type *next;
} edge;


typedef struct point_type
{
  int x, y;
} point_type;

typedef struct fpoint_type
{
  float x, y;
} fpoint_type;

typedef enum
{ Left, Right, Bottom, Top } an_edge;

#define NUM_EDGES 4

static SDL_Event scrolltimer_event;

int non_left_click_count = 0;


typedef struct dirent2
{
  struct dirent f;
  int place;
} dirent2;

SDL_Joystick *joystick;

/* Local function prototypes: */

static void mainloop(void);
static void brush_draw(int x1, int y1, int x2, int y2, int update);
static void blit_brush(int x, int y, int direction);
static void stamp_draw(int x, int y);
static void rec_undo_buffer(void);

void show_version(int details);
void show_usage(int exitcode);
static char *progname;

static SDL_Cursor *get_cursor(unsigned char *bits, unsigned char *mask_bits,
                              unsigned int w, unsigned int h, unsigned int x, unsigned int y);
static void seticon(void);
static SDL_Surface *loadimage(const char *const fname);
static SDL_Surface *do_loadimage(const char *const fname, int abort_on_error);
static void draw_toolbar(void);
static void draw_magic(void);
static void draw_brushes(void);
static void draw_stamps(void);
static void draw_shapes(void);
static void draw_erasers(void);
static void draw_fonts(void);
static void draw_none(void);

static void do_undo(void);
static void do_redo(void);
static void render_brush(void);
static void line_xor(int x1, int y1, int x2, int y2);
static void rect_xor(int x1, int y1, int x2, int y2);
static void draw_blinking_cursor(void);
static void hide_blinking_cursor(void);

static void reset_brush_counter(void);

#ifdef LOW_QUALITY_STAMP_OUTLINE
#define stamp_xor(x,y) rect_xor( \
  (x) - (CUR_STAMP_W+1)/2, \
  (y) - (CUR_STAMP_H+1)/2, \
  (x) + (CUR_STAMP_W+1)/2, \
  (y) + (CUR_STAMP_H+1)/2 \
)
#define update_stamp_xor()
#else
static void stamp_xor(int x1, int y1);
static void update_stamp_xor(void);
#endif

static void set_active_stamp(void);

static void do_eraser(int x, int y);
static void disable_avail_tools(void);
static void enable_avail_tools(void);
static void reset_avail_tools(void);
static int compare_dirent2s(struct dirent2 *f1, struct dirent2 *f2);
static void redraw_tux_text(void);
static void draw_tux_text(int which_tux, const char *const str, int want_right_to_left);
static void draw_tux_text_ex(int which_tux, const char *const str, int want_right_to_left, Uint8 locale_text);
static void wordwrap_text(const char *const str, SDL_Color color, int left, int top, int right, int want_right_to_left);
static void wordwrap_text_ex(const char *const str, SDL_Color color,
                             int left, int top, int right, int want_right_to_left, Uint8 locale_text);
static char *loaddesc(const char *const fname, Uint8 * locale_text);
static double loadinfo(const char *const fname, stamp_type * inf);

#ifndef NOSOUND
static Mix_Chunk *loadsound(const char *const fname);
static Mix_Chunk *loaddescsound(const char *const fname);
static void playstampdesc(int chan);
#endif
static void do_wait(int counter);
static void load_current(void);
static void save_current(void);
static int do_prompt_image_flash(const char *const text,
                                 const char *const btn_yes,
                                 const char *const btn_no, SDL_Surface * img1,
                                 SDL_Surface * img2, SDL_Surface * img3, int animate, int ox, int oy);
static int do_prompt_image_flash_snd(const char *const text,
                                     const char *const btn_yes,
                                     const char *const btn_no,
                                     SDL_Surface * img1, SDL_Surface * img2,
                                     SDL_Surface * img3, int animate, int snd, int ox, int oy);
static int do_prompt_image(const char *const text, const char *const btn_yes,
                           const char *const btn_no, SDL_Surface * img1,
                           SDL_Surface * img2, SDL_Surface * img3, int ox, int oy);
static int do_prompt_image_snd(const char *const text,
                               const char *const btn_yes,
                               const char *const btn_no, SDL_Surface * img1,
                               SDL_Surface * img2, SDL_Surface * img3, int snd, int ox, int oy);
static int do_prompt(const char *const text, const char *const btn_yes, const char *const btn_no, int ox, int oy);
static int do_prompt_snd(const char *const text, const char *const btn_yes,
                         const char *const btn_no, int snd, int ox, int oy);
static void cleanup(void);
static void free_surface(SDL_Surface ** surface_array);
static void free_surface_array(SDL_Surface * surface_array[], int count);

/*static void update_shape(int cx, int ox1, int ox2, int cy, int oy1, int oy2,
                          int fixed); */
static void do_shape(int cx, int cy, int ox, int oy, int rotn, int use_brush);
static int shape_rotation(int ctr_x, int ctr_y, int ox, int oy);
static int brush_rotation(int ctr_x, int ctr_y, int ox, int oy);
static int do_save(int tool, int dont_show_success_results, int autosave);
static int do_png_save(FILE * fi, const char *const fname, SDL_Surface * surf, int embed);
static void load_embedded_data(char *fname, SDL_Surface * org_surf);
static int chunk_is_valid(const char *chunk_name, png_unknown_chunk unknown);
Bytef *get_chunk_data(FILE * fp, char *fname, png_structp png_ptr,
                      png_infop info_ptr, const char *chunk_name, png_unknown_chunk unknown, int *unc_size);
static void get_new_file_id(void);
static int do_quit(int tool);
static int do_open(void);
static int do_new_dialog(void);
static int do_color_picker(void);
static int do_color_sel(void);
static int do_slideshow(void);
static void play_slideshow(int *selected, int num_selected, char *dirname, char **d_names, char **d_exts, int speed);
static void draw_selection_digits(int right, int bottom, int n);
static void wait_for_sfx(void);
static void rgbtohsv(Uint8 r8, Uint8 g8, Uint8 b8, float *h, float *s, float *v);
static void hsvtorgb(float h, float s, float v, Uint8 * r8, Uint8 * g8, Uint8 * b8);

static SDL_Surface *flip_surface(SDL_Surface * s);
static SDL_Surface *mirror_surface(SDL_Surface * s);

static void print_image(void);
static void do_print(void);
static void strip_trailing_whitespace(char *buf);
static void do_render_cur_text(int do_blit);
static char *uppercase(const char *restrict const str);
static wchar_t *uppercase_w(const wchar_t * restrict const str);
static char *textdir(const char *const str);
static SDL_Surface *do_render_button_label(const char *const label);
static void create_button_labels(void);
static Uint32 scrolltimer_callback(Uint32 interval, void *param);
static Uint32 drawtext_callback(Uint32 interval, void *param);
static void control_drawtext_timer(Uint32 interval, const char *const text, Uint8 locale_text);
static const char *great_str(void);
static void draw_image_title(int t, SDL_Rect dest);
static void handle_keymouse(SDLKey key, Uint32 updown, int steps, SDL_Rect * area1, SDL_Rect * area2);
static void handle_keymouse_buttons(SDLKey key, int *whicht, int *whichc, SDL_Rect real_r_tools);
static void handle_active(SDL_Event * event);

/*static char *replace_tilde(const char* const path);*/
#ifdef NO_SDLPANGO
static void anti_carriage_return(int left, int right, int cur_top, int new_top, int cur_bot, int line_width);
#endif
static void load_starter_id(char *saved_id, FILE * fil);
static void load_starter(char *img_id);
static void load_template(char *img_id);
static SDL_Surface *duplicate_surface(SDL_Surface * orig);
static void mirror_starter(void);
static void flip_starter(void);
static int valid_click(Uint8 button);
static int in_circle_rad(int x, int y, int rad);
static int paintsound(int size);
static void load_magic_plugins(void);
static int magic_sort(const void *a, const void *b);

Mix_Chunk *magic_current_snd_ptr;
static void magic_playsound(Mix_Chunk * snd, int left_right, int up_down);
static void magic_stopsound(void);
static void magic_line_func(void *mapi,
                            int which, SDL_Surface * canvas, SDL_Surface * last,
                            int x1, int y1, int x2, int y2, int step,
                            void (*cb) (void *, int, SDL_Surface *, SDL_Surface *, int, int));

static Uint8 magic_linear_to_sRGB(float lin);
static float magic_sRGB_to_linear(Uint8 srgb);
static int magic_button_down(void);
static SDL_Surface *magic_scale(SDL_Surface * surf, int w, int h, int aspect);
static void reset_touched(void);
static Uint8 magic_touched(int x, int y);

static void magic_switchin(SDL_Surface * last);
static void magic_switchout(SDL_Surface * last);
static int magic_modeint(int mode);

#ifdef DEBUG
static char *debug_gettext(const char *str);
static int charsize(Uint16 c);
#endif

static SDL_Surface *load_kpx(char *file);

#ifndef NOSVG
static SDL_Surface *load_svg(char *file);
static float pick_best_scape(unsigned int orig_w, unsigned int orig_h, unsigned int max_w, unsigned int max_h);
#endif
static SDL_Surface *myIMG_Load_RWops(char *file);
static SDL_Surface *myIMG_Load(char *file);
static int trash(char *path);
int file_exists(char *path);


#define MAX_UTF8_CHAR_LENGTH 6

#define USEREVENT_TEXT_UPDATE 1
#define USEREVENT_PLAYDESCSOUND 2

static int bypass_splash_wait;

/* Wait for a keypress or mouse click.
   counter is in 1/10 second units */
static void do_wait(int counter)
{
  SDL_Event event;
  int done;

  if (bypass_splash_wait)
    return;

  done = 0;

  do
    {
      while (SDL_PollEvent(&event))
        {
          if (event.type == SDL_QUIT)
            {
              done = 1;

              /* FIXME: Handle SDL_Quit better */
            }
          else if (event.type == SDL_WINDOWEVENT)
            {
              handle_active(&event);
            }
          else if (event.type == SDL_KEYDOWN)
            {
              done = 1;
            }
          else if (event.type == SDL_MOUSEBUTTONDOWN && valid_click(event.button.button))
            {
              done = 1;
            }
        }

      counter--;
      SDL_Delay(100);
    }
  while (!done && counter > 0);
}


/* This lets us exit quickly; perhaps the system is swapping to death
   or the user started Tux Paint by accident. It also lets the user
   more easily bypass the splash screen wait. */

/* Was used in progressbar.c, but is currently commented out!
   -bjk 2006.06.02 */

#if 0
static void eat_sdl_events(void)
{
  SDL_Event event;

  while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
        {
          SDL_Quit();
          exit(0);              /* can't safely use do_quit during start-up */
        }
      else if (event.type == SDL_WINDOWEVENT)
        handle_active(&event);
      else if (event.type == SDL_KEYDOWN)
        {
          SDLKey key = event.key.keysym.sym;
          SDLMod ctrl = event.key.keysym.mod & KMOD_CTRL;
          SDLMod alt = event.key.keysym.mod & KMOD_ALT;

          if ((key == SDLK_c && ctrl) || (key == SDLK_F4 && alt))
            {
              SDL_Quit();
              exit(0);
            }
          else if (key == SDLK_ESCAPE && waiting_for_fonts)
            {
              /* abort font loading! */

              printf("Aborting font load!\n");

              font_thread_aborted = 1;
              /* waiting_for_fonts = 0; */
            }
          else
            bypass_splash_wait = 1;
        }
      else if (event.type == SDL_MOUSEBUTTONDOWN)
        bypass_splash_wait = 1;
    }
}
#endif


/* Prompt to confirm user wishes to quit */
#define PROMPT_QUIT_TXT gettext_noop("Do you really want to quit?")

/* Quit prompt positive response (quit) */
#define PROMPT_QUIT_YES gettext_noop("Yes, I’m done!")

/* Quit prompt negative response (don't quit) */
#define PROMPT_QUIT_NO  gettext_noop("No, take me back!")


/* Current picture is not saved; user is quitting */
#define PROMPT_QUIT_SAVE_TXT gettext_noop("If you quit, you’ll lose your picture! Save it?")
#define PROMPT_QUIT_SAVE_YES gettext_noop("Yes, save it!")
#define PROMPT_QUIT_SAVE_NO gettext_noop("No, don’t bother saving!")

/* Current picture is not saved; user is opening another picture */
#define PROMPT_OPEN_SAVE_TXT gettext_noop("Save your picture first?")
#define PROMPT_OPEN_SAVE_YES gettext_noop("Yes, save it!")
#define PROMPT_OPEN_SAVE_NO gettext_noop("No, don’t bother saving!")

/* Error opening picture */
#define PROMPT_OPEN_UNOPENABLE_TXT gettext_noop("Can’t open that picture!")

/* Generic dialog dismissal */
#define PROMPT_OPEN_UNOPENABLE_YES gettext_noop("OK")


/* Notification that 'Open' dialog has nothing to show */
#define PROMPT_OPEN_NOFILES_TXT gettext_noop("There are no saved files!")
#define PROMPT_OPEN_NOFILES_YES gettext_noop("OK")

/* Verification of print action */
#define PROMPT_PRINT_NOW_TXT gettext_noop("Print your picture now?")
#define PROMPT_PRINT_NOW_YES gettext_noop("Yes, print it!")
#define PROMPT_PRINT_NOW_NO gettext_noop("No, take me back!")

/* Confirmation of successful (we hope) printing */
#define PROMPT_PRINT_TXT gettext_noop("Your picture has been printed!")
#define PROMPT_PRINT_YES gettext_noop("OK")

/* We got an error printing */
#define PROMPT_PRINT_FAILED_TXT gettext_noop("Sorry! Your picture could not be printed!")

/* Notification that it's too soon to print again (--printdelay option is in effect) */
#define PROMPT_PRINT_TOO_SOON_TXT gettext_noop("You can’t print yet!")
#define PROMPT_PRINT_TOO_SOON_YES gettext_noop("OK")

/* Prompt to confirm erasing a picture in the Open dialog */
#define PROMPT_ERASE_TXT gettext_noop("Erase this picture?")
#define PROMPT_ERASE_YES gettext_noop("Yes, erase it!")
#define PROMPT_ERASE_NO gettext_noop("No, don’t erase it!")

/* Reminder that Mouse Button 1 is the button to use in Tux Paint */
#define PROMPT_TIP_LEFTCLICK_TXT gettext_noop("Remember to use the left mouse button!")
#define PROMPT_TIP_LEFTCLICK_YES gettext_noop("OK")


enum
{
  SHAPE_TOOL_MODE_STRETCH,
  SHAPE_TOOL_MODE_ROTATE,
  SHAPE_TOOL_MODE_DONE
};


int brushflag, xnew, ynew, eraflag, lineflag, magicflag, keybd_flag, keybd_position, keyglobal, initial_y, gen_key_flag,
  ide, activeflag, old_x, old_y;
int cur_thing;

/* --- MAIN LOOP! --- */

static void mainloop(void)
{
  int done, val_x, val_y, valhat_x, valhat_y, new_x, new_y,
    shape_tool_mode, shape_ctr_x, shape_ctr_y, shape_outer_x, shape_outer_y, old_stamp_group, which;
  int num_things;
  int *thing_scroll;
  int do_draw, max;
  int ignoring_motion;
  int motioner = 0;
  int hatmotioner = 0;
  int whichc = 0;
  int whicht = 0;
  int line_start_x = 0;
  int line_start_y = 0;
  int j = 0;
  int stamp_size_selector_clicked = 0;
  int stamp_xored = 0;

  unsigned int i = 0;

#ifdef AUTOSAVE_GOING_BACKGROUND
  char *fname;
  char tmp[1024];
  FILE *fi;
#endif

  Uint32 TP_SDL_MOUSEBUTTONSCROLL = SDL_RegisterEvents(1);
  SDL_TimerID scrolltimer = NULL;
  SDL_Event event;
  SDLKey key;
  SDLMod mod;
  Uint32 last_cursor_blink, cur_cursor_blink, pre_event_time, current_event_time;
  SDL_Rect update_rect;
  SDL_Rect real_r_tools = r_tools;

#ifdef DEBUG
  Uint16 key_unicode;
  SDLKey key_down;
#endif
  on_screen_keyboard *new_kbd;
  SDL_Rect kbd_rect;

  num_things = num_brushes;
  thing_scroll = &brush_scroll;
  cur_thing = 0;
  do_draw = 0;
  old_x = 0;
  old_y = 0;
  which = 0;
  shape_ctr_x = 0;
  shape_ctr_y = 0;
  shape_outer_x = 0;
  shape_outer_y = 0;
  shape_tool_mode = SHAPE_TOOL_MODE_DONE;
  button_down = 0;
  last_cursor_blink = cur_toggle_count = 0;
  texttool_len = 0;
  scrolling = 0;
  scrolltimer = 0;
  val_x = 0;
  val_y = 0;
  valhat_x = 0;
  valhat_y = 0;
  done = 0;
  keyglobal = 0;
  r_tir.x = 0;
  r_tir.y = 0;
  r_tir.w = 0;
  r_tir.h = 0;

  if (NUM_TOOLS > 14 + TOOLOFFSET)
    {
      real_r_tools.h = r_tools.h - button_h;
      real_r_tools.y = r_tools.y + button_h / 2;
    }

  do
    {
      ignoring_motion = 0;

      pre_event_time = SDL_GetTicks();


      while (SDL_PollEvent(&event))
        {
          current_event_time = SDL_GetTicks();

          /* To avoid getting stuck in a 'catching up with mouse motion' interface lock-up */
          /* FIXME: Another thing we could do here is peek into events, and 'skip' to the last motion...? Or something... -bjk 2011.04.26 */
          if (current_event_time > pre_event_time + 500 && event.type == SDL_MOUSEMOTION)
            ignoring_motion = (ignoring_motion + 1) % 3;        /* Ignore every couple of motion events, to keep things moving quickly (but avoid, e.g., attempts to draw "O" from looking like "D") */


          if (event.type == SDL_QUIT)
            {
              magic_switchout(canvas);
              done = do_quit(cur_tool);
              if (!done)
                {
                  magic_switchin(canvas);

                  if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                    {
                      if (onscreen_keyboard && kbd)
                        {
                          SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                          update_screen_rect(&kbd_rect);
                        }

                      if (onscreen_keyboard && !kbd)
                        {
                          SDL_StartTextInput();
                        }
                    }
                }
            }
#ifdef AUTOSAVE_GOING_BACKGROUND
          else if (event.type == SDL_APP_DIDENTERBACKGROUND)
            {
              /* Triggers a save to a temporary file in case the app is killed later.
                 That file should be deleted if tuxpaint continues without being killed but it contains unsaved data
                 so it must be used as the next start drawing if tuxpaint is killed.
                 Also a reference to the actual file being drawn should be kept and restored. */
              if (!been_saved)
                {
                  do_save(cur_tool, 0, 1);
                  save_current();
                }
            }
          else if (event.type == SDL_APP_DIDENTERFOREGROUND)
            {
              /* Discard the temp file saved before as the user takes again control */
              snprintf(tmp, sizeof(tmp), "saved/%s%s", AUTOSAVED_NAME, FNAME_EXTENSION);
              fname = get_fname(tmp, DIR_SAVE);
              fi = fopen(fname, "wb");
              if (fi != NULL)
                {
                  fclose(fi);
                  unlink(fname);
                  free(fname);
                }
            }
#endif
          else if (event.type == SDL_WINDOWEVENT)
            {
              /* Reset Shapes tool and clean the canvas if we lose focus */
              if (mouseaccessibility && emulate_button_pressed &&
                  ((cur_tool == TOOL_SHAPES && shape_tool_mode != SHAPE_TOOL_MODE_DONE) || cur_tool == TOOL_LINES) &&
                  event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
                /* event.active.state & (SDL_APPINPUTFOCUS|SDL_APPACTIVE) &&
                   event.active.gain == 0) */
                {
                  do_undo();
                  tool_avail[TOOL_REDO] = 0;    /* Don't let them 'redo' to get preview back */
                  draw_toolbar();
                  update_screen_rect(&r_tools);
                  shape_tool_mode = SHAPE_TOOL_MODE_DONE;
                }
              handle_active(&event);

            }
          else if (event.type == SDL_KEYUP)
            {
              key = event.key.keysym.sym;
              handle_keymouse(key, SDL_KEYUP, 16, NULL, NULL);
            }
          else if (event.type == SDL_KEYDOWN || event.type == SDL_TEXTINPUT)
            {
              key = event.key.keysym.sym;
              mod = event.key.keysym.mod;

#ifdef DEBUG
/* FIXME: debug junk */
              fprintf(stderr,
                      "key 0x%04x mod 0x%04x character 0x%04x %d <%c> is %sprintable, key_down 0x%x\n",
                      (unsigned)key,
                      (unsigned)mod,
                      (unsigned)event.text.text,
                      (int)event.text.text,
                      (key_unicode > ' ' && key_unicode < 127) ? (char)event.text.text : ' ',
                      iswprint(key_unicode) ? "" : "not ", (unsigned)key_down);
#endif
              if (cur_tool == TOOL_STAMP)
                {
                  SDL_Rect r_stamps_sizesel;

                  r_stamps_sizesel.x = r_canvas.x + r_canvas.w;
                  r_stamps_sizesel.y = r_canvas.h - img_btn_up->h;
                  r_stamps_sizesel.w = img_btn_up->w * 2;
                  r_stamps_sizesel.h = img_btn_up->h;
                  handle_keymouse(key, SDL_KEYDOWN, 16, &r_canvas, &r_stamps_sizesel);
                }
              else
                handle_keymouse(key, SDL_KEYDOWN, 16, &r_canvas, NULL);

              /* handle_keymouse_buttons will move one button at a time */
              handle_keymouse_buttons(key, &whicht, &whichc, real_r_tools);


              if ((key == SDLK_ESCAPE || key == SDLK_AC_BACK) && !disable_quit)
                {
                  magic_switchout(canvas);
                  done = do_quit(cur_tool);
                  if (!done)
                    {
                      magic_switchin(canvas);

                      if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                        {
                          if (onscreen_keyboard && kbd)
                            {
                              SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                              update_screen_rect(&kbd_rect);
                            }

                          if (onscreen_keyboard && !kbd)
                            {
                              SDL_StartTextInput();
                            }
                        }
                    }
                }
              else if (key == SDLK_s && (mod & KMOD_ALT))
                {
#ifndef NOSOUND
                  if (use_sound)
                    {
                      printf("modstate at mainloop %d, mod %d\n", SDL_GetModState(), mod);

                      mute = !mute;
                      Mix_HaltChannel(-1);

                      if (mute)
                        {
                          /* Sound has been muted (silenced) via keyboard shortcut */
                          draw_tux_text(TUX_BORED, gettext("Sound muted."), 0);
                        }
                      else
                        {
                          /* Sound has been unmuted (unsilenced) via keyboard shortcut */
                          draw_tux_text(TUX_BORED, gettext("Sound unmuted."), 0);
                        }
                    }
#endif
                }
              else if ((key == SDLK_ESCAPE || key == SDLK_AC_BACK) && (mod & KMOD_SHIFT) && (mod & KMOD_CTRL))
                {
                  magic_switchout(canvas);
                  done = do_quit(cur_tool);
                  if (!done)
                    magic_switchin(canvas);
                }
#ifdef WIN32
              else if (key == SDLK_F4 && (mod & KMOD_ALT))
                {
                  magic_switchout(canvas);
                  done = do_quit(cur_tool);
                  if (!done)
                    magic_switchin(canvas);
                }
#endif
              else if (key == SDLK_z && (mod & KMOD_CTRL) && !noshortcuts)
                {
                  /* Ctrl-Z - Undo */

                  magic_switchout(canvas);

                  if (tool_avail[TOOL_UNDO])
                    {
                      if (cursor_x != -1 && cursor_y != -1)
                        {
                          hide_blinking_cursor();
                          if (texttool_len > 0)
                            {
                              rec_undo_buffer();
                              do_render_cur_text(1);
                              texttool_len = 0;
                              cursor_textwidth = 0;
                              label_node_to_edit = NULL;
                            }
                          else if (cur_tool == TOOL_LABEL && label_node_to_edit)
                            {
                              rec_undo_buffer();
                              have_to_rec_label_node = TRUE;
                              add_label_node(0, 0, 0, 0, NULL);
                              derender_node(&label_node_to_edit);
                              label_node_to_edit = NULL;
                            }
                        }

                      if (cur_undo == newest_undo)
                        {
                          rec_undo_buffer();
                          do_undo();
                        }
                      do_undo();
                      update_screen_rect(&r_tools);
                      shape_tool_mode = SHAPE_TOOL_MODE_DONE;
                    }

                  magic_switchin(canvas);
                }
              else if (key == SDLK_r && (mod & KMOD_CTRL) && !noshortcuts)
                {
                  /* Ctrl-R - Redo */

                  magic_switchout(canvas);

                  if (tool_avail[TOOL_REDO])
                    {
                      hide_blinking_cursor();
                      do_redo();
                      update_screen_rect(&r_tools);
                      shape_tool_mode = SHAPE_TOOL_MODE_DONE;
                    }

                  magic_switchin(canvas);
                }
              else if (key == SDLK_o && (mod & KMOD_CTRL) && !noshortcuts)
                {
                  /* Ctrl-O - Open */

                  magic_switchout(canvas);

                  disable_avail_tools();
                  draw_toolbar();
                  draw_colors(COLORSEL_CLOBBER_WIPE);
                  draw_none();

                  if (do_open() == 0)
                    {
                      if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                        do_render_cur_text(0);
                    }

                  enable_avail_tools();

                  draw_toolbar();
                  update_screen_rect(&r_tools);
                  draw_colors(COLORSEL_REFRESH);

                  if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_LINES)
                    draw_brushes();
                  else if (cur_tool == TOOL_MAGIC)
                    draw_magic();
                  else if (cur_tool == TOOL_STAMP)
                    draw_stamps();
                  else if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                    {
                      draw_fonts();
                      if (onscreen_keyboard && kbd)
                        {
                          SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                          update_screen_rect(&kbd_rect);
                        }

                      if (onscreen_keyboard && !kbd)
                        {
                          SDL_StartTextInput();
                        }
                    }
                  else if (cur_tool == TOOL_SHAPES)
                    draw_shapes();
                  else if (cur_tool == TOOL_ERASER)
                    draw_erasers();

                  draw_tux_text(TUX_GREAT, tool_tips[cur_tool], 1);

                  /* FIXME: Make delay configurable: */
                  control_drawtext_timer(1000, tool_tips[cur_tool], 0);

                  magic_switchin(canvas);
                }
              else if ((key == SDLK_n && (mod & KMOD_CTRL)) && !noshortcuts)
                {
                  /* Ctrl-N - New */

                  magic_switchout(canvas);

                  hide_blinking_cursor();
                  shape_tool_mode = SHAPE_TOOL_MODE_DONE;

                  disable_avail_tools();
                  draw_toolbar();
                  draw_colors(COLORSEL_CLOBBER_WIPE);
                  draw_none();

                  if (do_new_dialog() == 0)
                    {
                      draw_tux_text(tool_tux[TUX_DEFAULT], TIP_NEW_ABORT, 1);

                      if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                        do_render_cur_text(0);
                    }

                  enable_avail_tools();

                  draw_toolbar();
                  update_screen_rect(&r_tools);
                  draw_colors(COLORSEL_REFRESH);

                  if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_LINES)
                    draw_brushes();
                  else if (cur_tool == TOOL_MAGIC)
                    draw_magic();
                  else if (cur_tool == TOOL_STAMP)
                    draw_stamps();
                  else if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                    {
                      draw_fonts();
                      if (onscreen_keyboard && kbd)
                        {
                          SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                          update_screen_rect(&kbd_rect);
                        }

                      if (onscreen_keyboard && !kbd)
                        {
                          SDL_StartTextInput();
                        }
                    }
                  else if (cur_tool == TOOL_SHAPES)
                    draw_shapes();
                  else if (cur_tool == TOOL_ERASER)
                    draw_erasers();

                  update_screen_rect(&r_toolopt);
                  update_screen_rect(&r_ttoolopt);
                  magic_switchin(canvas);
                }
              else if (key == SDLK_s && (mod & KMOD_CTRL) && !noshortcuts)
                {
                  /* Ctrl-S - Save */

                  magic_switchout(canvas);
                  hide_blinking_cursor();

                  if (do_save(cur_tool, 0, 0))
                    {
                      /* Only think it's been saved if it HAS been saved :^) */

                      been_saved = 1;
                      tool_avail[TOOL_SAVE] = 0;
                    }

                  draw_toolbar();
                  update_screen_rect(&r_tools);
                  if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                    {
                      if (onscreen_keyboard && kbd)
                        {
                          SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                          update_screen_rect(&kbd_rect);
                        }

                      if (onscreen_keyboard && !kbd)
                        {
                          SDL_StartTextInput();
                        }
                    }

                  magic_switchin(canvas);
                }
              else if (key == SDLK_p && (mod & KMOD_CTRL) && !noshortcuts)
                {
                  /* Ctrl-P - Print */

                  if (!disable_print)
                    {
                      magic_switchout(canvas);

                      /* If they haven't hit [Enter], but clicked 'Print', add their text now -bjk 2007.10.25 */

                      tmp_apply_uncommited_text();
                      print_image();
                      undo_tmp_applied_text();
                      magic_switchin(canvas);

                      if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                        {
                          if (onscreen_keyboard && kbd)
                            {
                              SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                              update_screen_rect(&kbd_rect);
                            }

                          if (onscreen_keyboard && !kbd)
                            {
                              SDL_StartTextInput();
                            }
                        }

                      draw_toolbar();
                      draw_tux_text(TUX_BORED, "", 0);
                      update_screen_rect(&r_tools);
                    }
                }
              else if (event.type == SDL_TEXTINPUT ||
                       (event.type == SDL_KEYDOWN &&
                        (event.key.keysym.sym == SDLK_BACKSPACE ||
                         event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_TAB)))
                {
                  /* Handle key in text tool: */

                  if (((cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL) && cursor_x != -1 && cursor_y != -1) ||
                      (cur_tool == TOOL_LABEL && cur_label == LABEL_SELECT))
                    {
                      static int redraw = 0;
                      wchar_t *im_cp = im_data.s;

#ifdef DEBUG
//          key_down = key;
//          key_unicode = event.key.keysym.unicode;
//          printf(
//            "character 0x%04x %d <%c> is %d pixels, %sprintable, key_down 0x%x\n",
//            (unsigned)event.key.keysym.unicode,
//            (int)event.key.keysym.unicode,
//            (key_unicode>' ' && key_unicode<127)?(char)event.key.keysym.unicode:' ',
//              (int)charsize(event.key.keysym.unicode),
//              iswprint(key_unicode)?"":"not ",
//              (unsigned)key_down
//            );
#if 0
                      /* this doesn't work for some reason */
                      wprintf(L"character 0x%04x %d <%lc> is %d pixels, %lsprintable, key_down 0x%x\n",
                              event.key.keysym.unicode,
                              event.key.keysym.unicode,
                              (key_unicode > L' ') ? event.key.keysym.unicode : L' ',
                              charsize(event.key.keysym.unicode), iswprint(key_unicode) ? L"" : L"not ", key_down);
#endif
#endif
                      /* Set the text input rectangle for system onscreen keyboards */
                      if (onscreen_keyboard && !kbd)
                        {
                          r_tir.y = (float)cursor_y / render_scale;
                          r_tir.x = (float)cursor_x / render_scale;
                          SDL_SetTextInputRect(&r_tir);
                          SDL_StartTextInput();
                        }


                      /* Discard previous # of redraw characters */
                      if ((int)texttool_len <= redraw)
                        texttool_len = 0;
                      else
                        texttool_len -= redraw;
                      texttool_str[texttool_len] = L'\0';

                      /* Read IM, remember how many to redraw next iteration */
                      redraw = im_read(&im_data, event);

                      /* Korean Hangul needs this to refresh when buffered chars gets emptied */
                      if (!*im_cp)
                        do_render_cur_text(0);

                      /* Queue each character to be displayed */
                      while (*im_cp)
                        {
                          if (*im_cp == L'\b')
                            {
                              hide_blinking_cursor();
                              if (texttool_len > 0)
                                {
                                  texttool_len--;
                                  texttool_str[texttool_len] = L'\0';
                                  playsound(screen, 0, SND_KEYCLICK, 0, SNDPOS_CENTER, SNDDIST_NEAR);

                                  do_render_cur_text(0);

                                  if (been_saved)
                                    {
                                      been_saved = 0;

                                      if (!disable_save)
                                        tool_avail[TOOL_SAVE] = 1;

                                      draw_toolbar();
                                      update_screen_rect(&r_tools);
                                    }

                                }
                            }
                          else if (*im_cp == L'\r')
                            {
                              int font_height;

                              font_height = TuxPaint_Font_FontHeight(getfonthandle(cur_font));

                              hide_blinking_cursor();
                              if (texttool_len > 0)
                                {
                                  rec_undo_buffer();
                                  do_render_cur_text(1);
                                  label_node_to_edit = NULL;
                                  texttool_len = 0;
                                  cursor_textwidth = 0;
                                  if (cur_tool == TOOL_LABEL)
                                    {
                                      draw_fonts();
                                      update_screen_rect(&r_toolopt);
                                    }

                                  if (been_saved)
                                    {
                                      been_saved = 0;

                                      if (!disable_save)
                                        tool_avail[TOOL_SAVE] = 1;

                                      draw_toolbar();
                                      update_screen_rect(&r_tools);
                                    }


                                  cursor_x = cursor_left;
                                  cursor_y = min(cursor_y + font_height, canvas->h - font_height);

                                  playsound(screen, 0, SND_RETURN, 1, SNDPOS_RIGHT, SNDDIST_NEAR);

                                }
                              else if (cur_tool == TOOL_LABEL && label_node_to_edit)
                                {
                                  rec_undo_buffer();
                                  have_to_rec_label_node = TRUE;
                                  add_label_node(0, 0, 0, 0, NULL);
                                  derender_node(&label_node_to_edit);
                                  label_node_to_edit = NULL;
                                  /* playsound(screen, 0, SND_DELETE_LABEL, 0, SNDPOS_CENTER); *//* FIXME lack of specific sound */

                                  if (been_saved)
                                    {
                                      been_saved = 0;

                                      if (!disable_save)
                                        tool_avail[TOOL_SAVE] = 1;

                                      draw_toolbar();
                                      update_screen_rect(&r_tools);
                                    }
                                }

                              /* Select a node to edit */
                              else if (cur_tool == TOOL_LABEL && cur_label == LABEL_SELECT)
                                {
                                  label_node_to_edit =
                                    search_label_list(&highlighted_label_node, highlighted_label_node->save_x + 3,
                                                      highlighted_label_node->save_y + 3, 0);
                                  if (label_node_to_edit)
                                    {
                                      cur_label = LABEL_LABEL;
                                      cur_thing = label_node_to_edit->save_cur_font;
                                      do_setcursor(cursor_insertion);
                                      i = 0;
                                      label_node_to_edit->is_enabled = FALSE;
                                      derender_node(&label_node_to_edit);

                                      texttool_len = select_texttool_len;
                                      while (i < texttool_len)
                                        {
                                          texttool_str[i] = select_texttool_str[i];
                                          i = i + 1;
                                        }
                                      texttool_str[i] = L'\0';
                                      cur_color = select_color;
                                      old_x = select_x;
                                      old_y = select_y;
                                      cur_font = select_cur_font;
                                      text_state = select_text_state;
                                      text_size = select_text_size;
                                      for (j = 0; j < num_font_families; j++)
                                        {
                                          if (user_font_families[j] && user_font_families[j]->handle)
                                            {
                                              TuxPaint_Font_CloseFont(user_font_families[j]->handle);
                                              user_font_families[j]->handle = NULL;
                                            }
                                        }
                                      draw_fonts();
                                      update_screen_rect(&r_toolopt);

                                      cursor_x = old_x;
                                      cursor_y = old_y;
                                      cursor_left = old_x;

                                      draw_colors(COLORSEL_REFRESH);
                                      draw_fonts();
                                    }


                                  do_render_cur_text(0);

                                }
                              else
                                {
                                  cursor_x = cursor_left;
                                  cursor_y = min(cursor_y + font_height, canvas->h - font_height);
                                }

#ifdef SPEECH
#ifdef __APPLE__
                              if (use_sound)
                                speak_string(texttool_str);
#endif
#endif
                              im_softreset(&im_data);
                            }
                          else if (*im_cp == L'\t')
                            {

                              if (texttool_len > 0)
                                {
                                  rec_undo_buffer();
                                  do_render_cur_text(1);
                                  label_node_to_edit = NULL;
                                  cursor_x = min(cursor_x + cursor_textwidth, canvas->w);
                                  texttool_len = 0;
                                  cursor_textwidth = 0;
                                  if (cur_tool == TOOL_LABEL)
                                    {
                                      draw_fonts();
                                      update_screen_rect(&r_toolopt);
                                    }

                                  if (been_saved)
                                    {
                                      been_saved = 0;

                                      if (!disable_save)
                                        tool_avail[TOOL_SAVE] = 1;

                                      draw_toolbar();
                                      update_screen_rect(&r_tools);
                                    }
                                }
                              else if (cur_tool == TOOL_LABEL && label_node_to_edit)
                                {
                                  rec_undo_buffer();
                                  have_to_rec_label_node = TRUE;
                                  add_label_node(0, 0, 0, 0, NULL);
                                  derender_node(&label_node_to_edit);
                                  label_node_to_edit = NULL;
                                  /* playsound(screen, 0, SND_DELETE_LABEL, 0, SNDPOS_CENTER); *//* FIXME lack of specific sound */

                                  if (been_saved)
                                    {
                                      been_saved = 0;

                                      if (!disable_save)
                                        tool_avail[TOOL_SAVE] = 1;

                                      draw_toolbar();
                                      update_screen_rect(&r_tools);
                                    }
                                }
                              /* Cycle accross the nodes */
                              else if (cur_tool == TOOL_LABEL && cur_label == LABEL_SELECT)
                                {
                                  cycle_highlighted_label_node();
                                  highlight_label_nodes();
                                }



#ifdef SPEECH
#ifdef __APPLE__
                              if (use_sound)
                                speak_string(texttool_str);
#endif
#endif
                              im_softreset(&im_data);
                            }
                          else if (cur_tool == TOOL_TEXT || cur_label == LABEL_LABEL)
                            {
                              // iswprintf seems not supported well in Android
#ifndef  __ANDROID__
                              if (!iswprint(*im_cp))
                                break;
#endif
                              if (texttool_len < (sizeof(texttool_str) / sizeof(wchar_t)) - 1)
                                {
                                  int old_cursor_textwidth = cursor_textwidth;

#ifdef DEBUG
//                wprintf(L"    key = <%c>\nunicode = <%lc> 0x%04x %d\n\n",
//                      key_down, key_unicode, key_unicode, key_unicode);
#endif

                                  texttool_str[texttool_len++] = *im_cp;
                                  texttool_str[texttool_len] = 0;

                                  do_render_cur_text(0);

                                  if (been_saved)
                                    {
                                      been_saved = 0;

                                      if (!disable_save)
                                        tool_avail[TOOL_SAVE] = 1;

                                      draw_toolbar();
                                      update_screen_rect(&r_tools);
                                    }


                                  if (cursor_x + old_cursor_textwidth <= canvas->w - 50 &&
                                      cursor_x + cursor_textwidth > canvas->w - 50)
                                    {
                                      playsound(screen, 0, SND_KEYCLICKRING, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
                                    }
                                  else
                                    {
                                      /* FIXME: Might be fun to position the
                                         sound based on keyboard layout...? */

                                      playsound(screen, 0, SND_KEYCLICK, 0, SNDPOS_CENTER, SNDDIST_NEAR);
                                    }
                                }
                            }

                          im_cp++;
                        }       /* while(*im_cp) */

                      /* Show IM tip text */
                      if (im_data.tip_text)
                        {
                          draw_tux_text(TUX_DEFAULT, im_data.tip_text, 1);
                        }

                    }
                }
            }

          else if (event.type == SDL_JOYAXISMOTION)
            handle_joyaxismotion(event, &motioner, &val_x, &val_y);

          else if (event.type == SDL_JOYHATMOTION)
            handle_joyhatmotion(event, oldpos_x, oldpos_y, &valhat_x, &valhat_y, &hatmotioner, &old_hat_ticks);

          else if (event.type == SDL_JOYBALLMOTION)
            handle_joyballmotion(event, oldpos_x, oldpos_y);

          else if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP)
            handle_joybuttonupdownscl(event, oldpos_x, oldpos_y, real_r_tools);

          else if (event.type == SDL_MOUSEBUTTONDOWN &&
                   event.button.button >= 2 &&
                   event.button.button <= 3 &&
                   (no_button_distinction == 0 && !(HIT(r_tools) && GRIDHIT_GD(r_tools, gd_tools) == TOOL_PRINT)))
            {
              /* They're using the middle or right mouse buttons! */

              non_left_click_count++;


              if (non_left_click_count == 10 || non_left_click_count == 20 || (non_left_click_count % 50) == 0)
                {
                  /* Pop up an informative animation: */

                  hide_blinking_cursor();
                  do_prompt_image_flash(PROMPT_TIP_LEFTCLICK_TXT,
                                        PROMPT_TIP_LEFTCLICK_YES,
                                        "", img_mouse, img_mouse_click, NULL, 1, event.button.x, event.button.y);
                  if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                    {
                      if (onscreen_keyboard && !kbd)
                        {
                          r_tir.y = (float)event.button.y / render_scale;
                          SDL_SetTextInputRect(&r_tir);
                          SDL_StartTextInput();
                        }
                      do_render_cur_text(0);
                    }
                  draw_tux_text(TUX_BORED, "", 0);
                }
            }
          else if ((event.type == SDL_MOUSEBUTTONDOWN ||
                    event.type == TP_SDL_MOUSEBUTTONSCROLL) && event.button.button <= 3)
            {


              if (HIT(r_tools))
                {


                  if (HIT(real_r_tools))
                    {
                      /* A tool on the left has been pressed! */
                      brushflag = 0;
                      magicflag = 0;
                      magic_switchout(canvas);
                      whicht = tool_scroll + GRIDHIT_GD(real_r_tools, gd_tools);

                      if (whicht < NUM_TOOLS && tool_avail[whicht] &&
                          (valid_click(event.button.button) || whicht == TOOL_PRINT))
                        {
                          /* Allow middle/right-click on "Print", since [Alt]+click
                             on Mac OS X changes it from left click to middle! */

                          /* Render any current text, if switching to a different
                             drawing tool: */

                          if ((cur_tool == TOOL_TEXT && whicht != TOOL_TEXT &&
                               whicht != TOOL_NEW && whicht != TOOL_OPEN &&
                               whicht != TOOL_SAVE && whicht != TOOL_PRINT &&
                               whicht != TOOL_QUIT) ||
                              (cur_tool == TOOL_LABEL && whicht != TOOL_LABEL &&
                               whicht != TOOL_NEW && whicht != TOOL_OPEN &&
                               whicht != TOOL_SAVE && whicht != TOOL_PRINT && whicht != TOOL_QUIT))
                            {
                              if (cursor_x != -1 && cursor_y != -1)
                                {
                                  hide_blinking_cursor();
                                  if (texttool_len > 0)
                                    {
                                      rec_undo_buffer();
                                      do_render_cur_text(1);
                                      texttool_len = 0;
                                      cursor_textwidth = 0;
                                      label_node_to_edit = NULL;
                                    }
                                  else if (cur_tool == TOOL_LABEL && label_node_to_edit)
                                    {
                                      rec_undo_buffer();
                                      have_to_rec_label_node = TRUE;
                                      add_label_node(0, 0, 0, 0, NULL);
                                      derender_node(&label_node_to_edit);
                                      label_node_to_edit = NULL;
                                    }
                                }
                            }
                          update_canvas(0, 0, WINDOW_WIDTH - 96, (48 * 7) + 40 + HEIGHTOFFSET);
                          old_tool = cur_tool;
                          cur_tool = whicht;
                          draw_toolbar();
                          update_screen_rect(&r_tools);
                          printf("screenrectr_tools %d, %d, %d, %d\n", r_tools.x, r_tools.y, r_tools.w, r_tools.h);
                          playsound(screen, 1, SND_CLICK, 0, SNDPOS_LEFT, SNDDIST_NEAR);

                          /* FIXME: this "if" is just plain gross */
                          if (cur_tool != TOOL_TEXT)
                            draw_tux_text(tool_tux[cur_tool], tool_tips[cur_tool], 1);

                          /* Draw items for this tool: */

                          if (cur_tool == TOOL_BRUSH)
                            {
                              keybd_flag = 0;
                              cur_thing = cur_brush;
                              num_things = num_brushes;
                              thing_scroll = &brush_scroll;
                              draw_brushes();
                              draw_colors(COLORSEL_ENABLE);
                            }
                          else if (cur_tool == TOOL_STAMP)
                            {
                              keybd_flag = 0;
                              cur_thing = cur_stamp[stamp_group];
                              num_things = num_stamps[stamp_group];
                              thing_scroll = &(stamp_scroll[stamp_group]);
                              draw_stamps();
                              draw_colors(stamp_colorable(cur_stamp[stamp_group]) ||
                                          stamp_tintable(cur_stamp[stamp_group]));
                              set_active_stamp();
                              update_stamp_xor();
                            }
                          else if (cur_tool == TOOL_LINES)
                            {
                              keybd_flag = 0;
                              cur_thing = cur_brush;
                              num_things = num_brushes;
                              thing_scroll = &brush_scroll;
                              draw_brushes();
                              draw_colors(COLORSEL_ENABLE);
                            }
                          else if (cur_tool == TOOL_SHAPES)
                            {
                              keybd_flag = 0;
                              cur_thing = cur_shape;
                              num_things = NUM_SHAPES;
                              thing_scroll = &shape_scroll;
                              draw_shapes();
                              draw_colors(COLORSEL_ENABLE);
                              shape_tool_mode = SHAPE_TOOL_MODE_DONE;
                            }
                          else if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                            {
                              if (onscreen_keyboard && kbd)
                                {
                                  kbd_rect.x = button_w * 2 + (canvas->w - kbd->surface->w) / 2;
                                  if (old_y > canvas->h / 2)
                                    kbd_rect.y = 0;
                                  else
                                    kbd_rect.y = canvas->h - kbd->surface->h;
                                  kbd_rect.w = kbd->surface->w;
                                  kbd_rect.h = kbd->surface->h;
                                  SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                                  update_screen_rect(&kbd_rect);
                                }

                              if (onscreen_keyboard && !kbd)
                                {
                                  r_tir.y = (float)event.button.y / render_scale;
                                  SDL_SetTextInputRect(&r_tir);
                                  SDL_StartTextInput();
                                }
                              if (!font_thread_done)
                                {
                                  draw_colors(COLORSEL_DISABLE);
                                  draw_none();
                                  update_screen_rect(&r_toolopt);
                                  update_screen_rect(&r_ttoolopt);
                                  do_setcursor(cursor_watch);

                                  /* Wait while Text tool finishes loading fonts */
                                  draw_tux_text(TUX_WAIT, gettext("Please wait…"), 1);

                                  waiting_for_fonts = 1;
#ifdef FORKED_FONTS
                                  receive_some_font_info(screen, texture, renderer);
#else
                                  while (!font_thread_done && !font_thread_aborted)
                                    {
                                      /* FIXME: should have a read-depends memory barrier around here */
                                      show_progress_bar(screen);
                                      SDL_Delay(20);
                                    }
                                  /* FIXME: should kill this in any case */
                                  SDL_WaitThread(font_thread, NULL);
#endif
                                  set_label_fonts();
                                  do_setcursor(cursor_arrow);
                                }
                              draw_tux_text(tool_tux[cur_tool], tool_tips[cur_tool], 1);

                              if (num_font_families > 0)
                                {
                                  cur_thing = cur_font;
                                  num_things = num_font_families;
                                  thing_scroll = &font_scroll;
                                  cur_label = LABEL_LABEL;

                                  draw_fonts();
                                  draw_colors(COLORSEL_ENABLE);
                                }
                              else
                                {
                                  /* Problem using fonts! */

                                  cur_tool = old_tool;
                                  draw_toolbar();
                                  update_screen_rect(&r_tools);
                                }
                            }
                          else if (cur_tool == TOOL_MAGIC)
                            {
                              keybd_flag = 0;
                              cur_thing = cur_magic;
                              num_things = num_magics;
                              thing_scroll = &magic_scroll;
                              magic_current_snd_ptr = NULL;
                              draw_magic();
                              draw_colors(magics[cur_magic].colors);

                              if (magics[cur_magic].colors)
                                magic_funcs[magics[cur_magic].handle_idx].set_color(magic_api_struct,
                                                                                    color_hexes[cur_color][0],
                                                                                    color_hexes[cur_color][1],
                                                                                    color_hexes[cur_color][2]);
                            }
                          else if (cur_tool == TOOL_ERASER)
                            {
                              keybd_flag = 0;
                              cur_thing = cur_eraser;
                              num_things = NUM_ERASERS;
                              thing_scroll = &eraser_scroll;
                              draw_erasers();
                              draw_colors(COLORSEL_DISABLE);
                            }
                          else if (cur_tool == TOOL_UNDO)
                            {
                              if (cur_undo == newest_undo)
                                {
                                  rec_undo_buffer();
                                  do_undo();
                                }
                              do_undo();

                              been_saved = 0;

                              if (!disable_save)
                                tool_avail[TOOL_SAVE] = 1;

                              cur_tool = old_tool;
                              draw_toolbar();
                              update_screen_rect(&r_tools);
                              shape_tool_mode = SHAPE_TOOL_MODE_DONE;
                            }
                          else if (cur_tool == TOOL_REDO)
                            {
                              do_redo();

                              been_saved = 0;

                              if (!disable_save)
                                tool_avail[TOOL_SAVE] = 1;

                              cur_tool = old_tool;
                              draw_toolbar();
                              update_screen_rect(&r_tools);
                              shape_tool_mode = SHAPE_TOOL_MODE_DONE;
                            }
                          else if (cur_tool == TOOL_OPEN)
                            {
                              disable_avail_tools();
                              draw_toolbar();
                              draw_colors(COLORSEL_CLOBBER_WIPE);
                              draw_none();

                              if (do_open() == 0)
                                {
                                  if (old_tool == TOOL_TEXT || old_tool == TOOL_LABEL)
                                    do_render_cur_text(0);
                                }

                              enable_avail_tools();

                              cur_tool = old_tool;
                              draw_toolbar();
                              update_screen_rect(&r_tools);

                              draw_tux_text(TUX_GREAT, tool_tips[cur_tool], 1);

                              draw_colors(COLORSEL_REFRESH);

                              if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_LINES)
                                draw_brushes();
                              else if (cur_tool == TOOL_MAGIC)
                                draw_magic();
                              else if (cur_tool == TOOL_STAMP)
                                draw_stamps();
                              else if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                                {
                                  draw_fonts();
                                  if (onscreen_keyboard && kbd)
                                    {
                                      SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                                      update_screen_rect(&kbd_rect);
                                    }

                                  if (onscreen_keyboard && !kbd)
                                    {
                                      SDL_StartTextInput();
                                    }
                                }
                              else if (cur_tool == TOOL_SHAPES)
                                draw_shapes();
                              else if (cur_tool == TOOL_ERASER)
                                draw_erasers();
                            }
                          else if (cur_tool == TOOL_SAVE)
                            {
                              if (do_save(old_tool, 0, 0))
                                {
                                  been_saved = 1;
                                  tool_avail[TOOL_SAVE] = 0;
                                }

                              if (old_tool == TOOL_TEXT || old_tool == TOOL_LABEL)
                                {
                                  if (onscreen_keyboard && kbd)
                                    {
                                      SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                                      update_screen_rect(&kbd_rect);
                                    }

                                  if (onscreen_keyboard && !kbd)
                                    {
                                      SDL_StartTextInput();
                                    }
                                }

                              cur_tool = old_tool;
                              draw_toolbar();
                              update_screen_rect(&r_tools);
                            }
                          else if (cur_tool == TOOL_NEW)
                            {
                              shape_tool_mode = SHAPE_TOOL_MODE_DONE;

                              disable_avail_tools();
                              draw_toolbar();
                              draw_colors(COLORSEL_CLOBBER_WIPE);
                              draw_none();

                              if (do_new_dialog() == 0)
                                {
                                  cur_tool = old_tool;

                                  draw_tux_text(tool_tux[TUX_DEFAULT], TIP_NEW_ABORT, 1);

                                  if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                                    do_render_cur_text(0);
                                }

                              cur_tool = old_tool;

                              enable_avail_tools();

                              draw_toolbar();
                              update_screen_rect(&r_tools);
                              draw_colors(COLORSEL_REFRESH);

                              if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_LINES)
                                draw_brushes();
                              else if (cur_tool == TOOL_MAGIC)
                                draw_magic();
                              else if (cur_tool == TOOL_STAMP)
                                draw_stamps();
                              else if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                                {
                                  draw_fonts();
                                  if (onscreen_keyboard && kbd)
                                    {
                                      SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                                      update_screen_rect(&kbd_rect);
                                    }

                                  if (onscreen_keyboard && !kbd)
                                    {
                                      SDL_StartTextInput();

                                    }
                                }
                              else if (cur_tool == TOOL_SHAPES)
                                draw_shapes();
                              else if (cur_tool == TOOL_ERASER)
                                draw_erasers();
                            }
                          else if (cur_tool == TOOL_PRINT)
                            {
                              /* If they haven't hit [Enter], but clicked 'Print', add their text now -bjk 2007.10.25 */
                              tmp_apply_uncommited_text();
                              /* original print code was here */
                              print_image();
                              undo_tmp_applied_text();

                              if (old_tool == TOOL_TEXT || old_tool == TOOL_LABEL)
                                {
                                  if (onscreen_keyboard && kbd)
                                    {
                                      SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                                      update_screen_rect(&kbd_rect);
                                    }

                                  if (onscreen_keyboard && !kbd)
                                    {
                                      SDL_StartTextInput();
                                    }
                                }

                              cur_tool = old_tool;
                              draw_toolbar();
                              draw_tux_text(TUX_BORED, "", 0);
                              update_screen_rect(&r_tools);
                            }
                          else if (cur_tool == TOOL_QUIT)
                            {
                              done = do_quit(old_tool);

                              if (old_tool == TOOL_TEXT || old_tool == TOOL_LABEL)
                                {
                                  if (onscreen_keyboard && kbd)
                                    {
                                      SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                                      update_screen_rect(&kbd_rect);
                                    }

                                  if (onscreen_keyboard && !kbd)
                                    {
                                      SDL_StartTextInput();

                                    }
                                }

                              cur_tool = old_tool;
                              draw_toolbar();
                              update_screen_rect(&r_tools);
                            }
                          update_screen_rect(&r_toolopt);
                          update_screen_rect(&r_ttoolopt);
                        }

                      if (!done)
                        magic_switchin(canvas);
                    }
                  else if ((event.button.y < r_tools.y + button_h / 2) && tool_scroll > 0)
                    {
                      tool_scroll -= gd_tools.cols;
                      playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                      draw_toolbar();
                      update_screen_rect(&r_tools);

                    }
                  else if ((event.button.y > real_r_tools.y + real_r_tools.h)
                           && (tool_scroll < NUM_TOOLS - 12 - TOOLOFFSET))
                    {
                      tool_scroll += gd_tools.cols;
                      draw_toolbar();
                      playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                      update_screen_rect(&r_tools);

                    }
                }

              else if (HIT(r_toolopt) && valid_click(event.button.button))
                {
                  /* Options on the right
                     WARNING: this must be kept in sync with the mouse-move
                     code (for cursor changes) and mouse-scroll code. */

                  if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_STAMP ||
                      cur_tool == TOOL_SHAPES || cur_tool == TOOL_LINES ||
                      cur_tool == TOOL_MAGIC || cur_tool == TOOL_TEXT ||
                      cur_tool == TOOL_ERASER || cur_tool == TOOL_LABEL)
                    {
                      int num_rows_needed;
                      SDL_Rect r_controls;
                      SDL_Rect r_notcontrols;
                      SDL_Rect r_items; /* = r_notcontrols; */
                      int toolopt_changed;
                      int select_changed = 0;
                      grid_dims gd_controls;    /* might become 2-by-2 */
                      grid_dims gd_items;       /* generally becoming 2-by-whatever */

                      gd_controls.rows = 0;
                      gd_controls.cols = 0;
                      gd_items.rows = 2;
                      gd_items.cols = 2;

                      /* Note set of things we're dealing with */
                      /* (stamps, brushes, etc.) */

                      if (cur_tool == TOOL_STAMP)
                        {
                          if (!disable_stamp_controls)
                            {
                              /* was 2,2 before adding left/right stamp group buttons -bjk 2007.05.15 */
                              gd_controls.rows = 3;
                              gd_controls.cols = 2;
                            }
                          else
                            {
                              /* was left 0,0 before adding left/right stamp group buttons -bjk 2007.05.03 */
                              gd_controls.rows = 1;
                              gd_controls.cols = 2;
                            }
                        }
                      else if (cur_tool == TOOL_TEXT)
                        {
                          if (!disable_stamp_controls)
                            {
                              gd_controls.rows = 2;
                              gd_controls.cols = 2;
                            }
                        }
                      else if (cur_tool == TOOL_LABEL)
                        {
                          if (!disable_stamp_controls)
                            {
                              gd_controls.rows = 3;
                              gd_controls.cols = 2;
                            }
                          else
                            {
                              gd_controls.rows = 1;
                              gd_controls.cols = 2;
                            }
                        }

                      else if (cur_tool == TOOL_MAGIC)
                        {
                          if (!disable_magic_controls)
                            {
                              gd_controls.rows = 1;
                              gd_controls.cols = 2;
                            }
                        }

                      /* number of whole or partial rows that will be needed
                         (can make this per-tool if variable columns needed) */
                      num_rows_needed = (num_things + gd_items.cols - 1) / gd_items.cols;

                      do_draw = 0;

                      r_controls.w = r_toolopt.w;
                      r_controls.h = gd_controls.rows * button_h;
                      r_controls.x = r_toolopt.x;
                      r_controls.y = r_toolopt.y + r_toolopt.h - r_controls.h;

                      r_notcontrols.w = r_toolopt.w;
                      r_notcontrols.h = r_toolopt.h - r_controls.h;
                      r_notcontrols.x = r_toolopt.x;
                      r_notcontrols.y = r_toolopt.y;

                      r_items.x = r_notcontrols.x;
                      r_items.y = r_notcontrols.y;
                      r_items.w = r_notcontrols.w;
                      r_items.h = r_notcontrols.h;

                      if (num_rows_needed * button_h > r_items.h)
                        {
                          /* too many; we'll need scroll buttons */
                          r_items.h -= button_h;
                          r_items.y += button_h / 2;
                        }
                      gd_items.rows = r_items.h / button_h;

                      toolopt_changed = 0;

                      if (HIT(r_items))
                        {
                          which = GRIDHIT_GD(r_items, gd_items) + *thing_scroll;

                          if (which < num_things)
                            {
                              toolopt_changed = 1;
#ifndef NOSOUND
                              if (cur_tool != TOOL_STAMP || stamp_data[stamp_group][which]->ssnd == NULL)
                                {
                                  playsound(screen, 1, SND_BLEEP, 0, SNDPOS_RIGHT, SNDDIST_NEAR);
                                }
#endif
                              cur_thing = which;
                              do_draw = 1;
                            }
                        }
                      else if (HIT(r_controls))
                        {
                          which = GRIDHIT_GD(r_controls, gd_controls);
                          if (cur_tool == TOOL_STAMP)
                            {
                              /* Stamp controls! */
                              int control_sound = -1;

                              if (which == 4 || which == 5)
                                {
                                  /* Grow/Shrink Controls: */
#ifdef OLD_STAMP_GROW_SHRINK
                                  if (which == 5)
                                    {
                                      /* Bottom right button: Grow: */
                                      if (stamp_data[stamp_group][cur_stamp[stamp_group]]->size < MAX_STAMP_SIZE)
                                        {
                                          stamp_data[stamp_group][cur_stamp[stamp_group]]->size++;
                                          control_sound = SND_GROW;
                                        }
                                    }
                                  else
                                    {
                                      /* Bottom left button: Shrink: */
                                      if (stamp_data[stamp_group][cur_stamp[stamp_group]]->size > MIN_STAMP_SIZE)
                                        {
                                          stamp_data[stamp_group][cur_stamp[stamp_group]]->size--;
                                          control_sound = SND_SHRINK;
                                        }
                                    }
#else
                                  int old_size;

#ifdef DEBUG
                                  float choice;
#endif

                                  old_size = stamp_data[stamp_group][cur_stamp[stamp_group]]->size;

                                  stamp_data[stamp_group][cur_stamp[stamp_group]]->size =
                                    (((MAX_STAMP_SIZE - MIN_STAMP_SIZE + 1
                                       /* +1 to address lack of ability to get back to max default stamp size (SF Bug #1668235 -bjk 2011.01.08) */
                                      ) * (event.button.x - (WINDOW_WIDTH - 96))) / 96) + MIN_STAMP_SIZE;

#ifdef DEBUG
                                  printf("Old size = %d, Chose %0.4f, New size =%d\n", old_size, choice,
                                         stamp_data[stamp_group][cur_stamp[stamp_group]]->size);
#endif

                                  if (stamp_data[stamp_group][cur_stamp[stamp_group]]->size < old_size)
                                    control_sound = SND_SHRINK;
                                  else if (stamp_data[stamp_group][cur_stamp[stamp_group]]->size > old_size)
                                    control_sound = SND_GROW;
#endif
                                }
                              else if (which == 2 || which == 3)
                                {
                                  /* Mirror/Flip Controls: */
                                  if (which == 3)
                                    {
                                      /* Top right button: Flip: */
                                      if (stamp_data[stamp_group][cur_stamp[stamp_group]]->flipable)
                                        {
                                          stamp_data[stamp_group][cur_stamp[stamp_group]]->flipped =
                                            !stamp_data[stamp_group][cur_stamp[stamp_group]]->flipped;
                                          control_sound = SND_FLIP;
                                        }
                                    }
                                  else
                                    {
                                      /* Top left button: Mirror: */
                                      if (stamp_data[stamp_group][cur_stamp[stamp_group]]->mirrorable)
                                        {
                                          stamp_data[stamp_group][cur_stamp[stamp_group]]->mirrored =
                                            !stamp_data[stamp_group][cur_stamp[stamp_group]]->mirrored;
                                          control_sound = SND_MIRROR;
                                        }
                                    }
                                }
                              else
                                {
                                  /* Prev/Next Controls: */

                                  old_stamp_group = stamp_group;

                                  if (which == 1)
                                    {
                                      /* Next group */
                                      stamp_group++;
                                      if (stamp_group >= num_stamp_groups)
                                        stamp_group = 0;
                                      control_sound = SND_CLICK;
                                    }
                                  else
                                    {
                                      /* Prev group */
                                      stamp_group--;
                                      if (stamp_group < 0)
                                        stamp_group = num_stamp_groups - 1;
                                      control_sound = SND_CLICK;
                                    }

                                  if (stamp_group == old_stamp_group)
                                    control_sound = -1;
                                  else
                                    {
                                      cur_thing = cur_stamp[stamp_group];
                                      num_things = num_stamps[stamp_group];
                                      thing_scroll = &(stamp_scroll[stamp_group]);
                                    }
                                }

                              if (control_sound != -1)
                                {
                                  playsound(screen, 0, control_sound, 0, SNDPOS_CENTER, SNDDIST_NEAR);
                                  draw_stamps();
                                  update_screen_rect(&r_toolopt);
                                  set_active_stamp();
                                  update_stamp_xor();
                                }
                            }
                          else if (cur_tool == TOOL_MAGIC)
                            {
                              /* Magic controls! */
                              if (which == 1 && magics[cur_magic].avail_modes & MODE_FULLSCREEN)
                                {
                                  magic_switchout(canvas);
                                  magics[cur_magic].mode = MODE_FULLSCREEN;
                                  magic_switchin(canvas);
                                  draw_magic();
                                  update_screen_rect(&r_toolopt);
                                }
                              else if (which == 0 && magics[cur_magic].avail_modes & MODE_PAINT)
                                {
                                  magic_switchout(canvas);
                                  magics[cur_magic].mode = MODE_PAINT;
                                  magic_switchin(canvas);
                                  draw_magic();
                                  update_screen_rect(&r_toolopt);
                                }
                              else if (which == 0 && magics[cur_magic].avail_modes & MODE_PAINT_WITH_PREVIEW)
                                {
                                  magic_switchout(canvas);
                                  magics[cur_magic].mode = MODE_PAINT_WITH_PREVIEW;
                                  magic_switchin(canvas);
                                  draw_magic();
                                  update_screen_rect(&r_toolopt);
                                }
                              else if (which == 0 && magics[cur_magic].avail_modes & MODE_ONECLICK)
                                {
                                  magic_switchout(canvas);
                                  magics[cur_magic].mode = MODE_ONECLICK;
                                  magic_switchin(canvas);
                                  draw_magic();
                                  update_screen_rect(&r_toolopt);
                                }
                              /* FIXME: Sfx */
                            }
                          else if (cur_tool == TOOL_TEXT)
                            {
                              /* Text controls! */
                              int control_sound = -1;

                              if (which & 2)
                                {
                                  /* One of the bottom buttons: */
                                  if (which & 1)
                                    {
                                      /* Bottom right button: Grow: */
                                      if (text_size < MAX_TEXT_SIZE)
                                        {
                                          text_size++;
                                          control_sound = SND_GROW;
                                          toolopt_changed = 1;
                                        }
                                    }
                                  else
                                    {
                                      /* Bottom left button: Shrink: */
                                      if (text_size > MIN_TEXT_SIZE)
                                        {
                                          text_size--;
                                          control_sound = SND_SHRINK;
                                          toolopt_changed = 1;
                                        }
                                    }
                                }
                              else
                                {
                                  /* One of the top buttons: */
                                  if (which & 1)
                                    {
                                      /* Top right button: Italic: */
                                      if (text_state & TTF_STYLE_ITALIC)
                                        {
                                          text_state &= ~TTF_STYLE_ITALIC;
                                          control_sound = SND_ITALIC_ON;
                                        }
                                      else
                                        {
                                          text_state |= TTF_STYLE_ITALIC;
                                          control_sound = SND_ITALIC_OFF;
                                        }
                                    }
                                  else
                                    {
                                      /* Top left button: Bold: */
                                      if (text_state & TTF_STYLE_BOLD)
                                        {
                                          text_state &= ~TTF_STYLE_BOLD;
                                          control_sound = SND_THIN;
                                        }
                                      else
                                        {
                                          text_state |= TTF_STYLE_BOLD;
                                          control_sound = SND_THICK;
                                        }
                                    }
                                  toolopt_changed = 1;
                                }
                              if (control_sound != -1)
                                {
                                  playsound(screen, 0, control_sound, 0, SNDPOS_CENTER, SNDDIST_NEAR);


                                  if (cur_tool == TOOL_TEXT)    /* Huh? It had better be! */
                                    {
                                      /* need to invalidate all the cached user fonts, causing reload on demand */

                                      int i;

                                      for (i = 0; i < num_font_families; i++)
                                        {
                                          if (user_font_families[i] && user_font_families[i]->handle)
                                            {
                                              TuxPaint_Font_CloseFont(user_font_families[i]->handle);
                                              user_font_families[i]->handle = NULL;
                                            }
                                        }
                                      draw_fonts();
                                      update_screen_rect(&r_toolopt);
                                    }
                                }
                            }

                          /* Label controls! */
                          else if (cur_tool == TOOL_LABEL)
                            {
                              int control_sound = -1;

                              if (which & 4)
                                {
                                  /* One of the bottom buttons: */
                                  if (which & 1)
                                    {
                                      /* Bottom right button: Grow: */
                                      if (text_size < MAX_TEXT_SIZE)
                                        {
                                          text_size++;
                                          control_sound = SND_GROW;
                                          toolopt_changed = 1;
                                        }
                                    }
                                  else
                                    {
                                      /* Bottom left button: Shrink: */
                                      if (text_size > MIN_TEXT_SIZE)
                                        {
                                          text_size--;
                                          control_sound = SND_SHRINK;
                                          toolopt_changed = 1;
                                        }
                                    }
                                }
                              else
                                {
                                  if (which & 2)
                                    {
                                      /* One of the middle buttons: */
                                      if (which & 1)
                                        {
                                          /*  right button: Italic: */
                                          if (text_state & TTF_STYLE_ITALIC)
                                            {
                                              text_state &= ~TTF_STYLE_ITALIC;
                                              control_sound = SND_ITALIC_ON;
                                            }
                                          else
                                            {
                                              text_state |= TTF_STYLE_ITALIC;
                                              control_sound = SND_ITALIC_OFF;
                                            }
                                        }
                                      else
                                        {
                                          /* middle left button: Bold: */
                                          if (text_state & TTF_STYLE_BOLD)
                                            {
                                              text_state &= ~TTF_STYLE_BOLD;
                                              control_sound = SND_THIN;
                                            }
                                          else
                                            {
                                              text_state |= TTF_STYLE_BOLD;
                                              control_sound = SND_THICK;
                                            }
                                        }
                                      toolopt_changed = 1;
                                    }

                                  else
                                    {
                                      /* One of the top buttons: */
                                      if (which & 1)
                                        {
                                          /* Select button: */
                                          if (cur_label == LABEL_SELECT)
                                            {
                                              cur_label = LABEL_LABEL;
                                              update_canvas(0, 0, WINDOW_WIDTH - 96, (48 * 7) + 40 + HEIGHTOFFSET);
                                              if (onscreen_keyboard)
                                                {
                                                  SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                                                  update_screen_rect(&kbd_rect);
                                                }

                                              if (onscreen_keyboard && !kbd)
                                                {
                                                  SDL_StartTextInput();

                                                }
                                            }
                                          else
                                            {
                                              if (are_labels())
                                                {
                                                  update_canvas_ex_r(kbd_rect.x - 96, kbd_rect.y,
                                                                     kbd_rect.x + kbd_rect.w, kbd_rect.y + kbd_rect.h,
                                                                     1);
                                                  if (texttool_len > 0)
                                                    {
                                                      rec_undo_buffer();
                                                      do_render_cur_text(1);
                                                      texttool_len = 0;
                                                      cursor_textwidth = 0;
                                                      label_node_to_edit = NULL;
                                                    }
                                                  else if (label_node_to_edit)
                                                    {
                                                      rec_undo_buffer();
                                                      have_to_rec_label_node = TRUE;
                                                      add_label_node(0, 0, 0, 0, NULL);
                                                      label_node_to_edit = NULL;

                                                    }

                                                  cur_label = LABEL_SELECT;
                                                  highlight_label_nodes();
                                                }
                                            }
                                          toolopt_changed = 1;
                                        }
                                    }
                                }

                              if (control_sound != -1)
                                {
                                  playsound(screen, 0, control_sound, 0, SNDPOS_CENTER, SNDDIST_NEAR);


                                  if (cur_tool == TOOL_LABEL)   /* Huh? It had better be! */
                                    {
                                      /* need to invalidate all the cached user fonts, causing reload on demand */

                                      int i;

                                      for (i = 0; i < num_font_families; i++)
                                        {
                                          if (user_font_families[i] && user_font_families[i]->handle)
                                            {
                                              TuxPaint_Font_CloseFont(user_font_families[i]->handle);
                                              user_font_families[i]->handle = NULL;
                                            }
                                        }
                                      draw_fonts();
                                      update_screen_rect(&r_toolopt);
                                    }
                                }
                              draw_fonts();
                              update_screen_rect(&r_toolopt);

                            }
                        }
                      else
                        {
                          /* scroll button */
                          int is_upper = event.button.y < r_toolopt.y + button_h / 2;

                          if ((is_upper && *thing_scroll > 0)   /* upper arrow */
                              || (!is_upper && *thing_scroll / gd_items.cols < num_rows_needed - gd_items.rows) /* lower arrow */
                            )
                            {
                              *thing_scroll += is_upper ? -gd_items.cols : gd_items.cols;
                              do_draw = 1;
                              playsound(screen, 1, SND_SCROLL, 1, SNDPOS_RIGHT, SNDDIST_NEAR);

                              if (scrolltimer != NULL)
                                {
                                  SDL_RemoveTimer(scrolltimer);
                                  scrolltimer = NULL;
                                }

                              if (!scrolling && event.type == SDL_MOUSEBUTTONDOWN)
                                {
                                  /* printf("Starting scrolling\n"); */
                                  memcpy(&scrolltimer_event, &event, sizeof(SDL_Event));
                                  scrolltimer_event.type = TP_SDL_MOUSEBUTTONSCROLL;

                                  scrolling = 1;

                                  scrolltimer =
                                    SDL_AddTimer(REPEAT_SPEED, scrolltimer_callback, (void *)&scrolltimer_event);
                                }
                              else
                                {
                                  /* printf("Continuing scrolling\n"); */
                                  scrolltimer =
                                    SDL_AddTimer(REPEAT_SPEED / 3, scrolltimer_callback, (void *)&scrolltimer_event);
                                }

                              if (*thing_scroll == 0)
                                {
                                  do_setcursor(cursor_arrow);
                                  if (scrolling)
                                    {
                                      if (scrolltimer != NULL)
                                        {
                                          SDL_RemoveTimer(scrolltimer);
                                          scrolltimer = NULL;
                                        }
                                      scrolling = 0;
                                    }
                                }
                            }
                        }


                      /* Assign the change(s), if any / redraw, if needed: */

                      if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_LINES)
                        {
                          cur_brush = cur_thing;
                          render_brush();

                          if (do_draw)
                            draw_brushes();
                        }
                      else if (cur_tool == TOOL_ERASER)
                        {
                          cur_eraser = cur_thing;

                          if (do_draw)
                            draw_erasers();
                        }
                      else if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                        {
                          /* FIXME */
                          /* char font_tux_text[512]; */

                          cur_font = cur_thing;

                          /* FIXME */
                          /*
                             snprintf(font_tux_text, sizeof font_tux_text, "%s (%s).",
                             TTF_FontFaceFamilyName(getfonthandle(cur_font)),
                             TTF_FontFaceStyleName(getfonthandle(cur_font)));
                             draw_tux_text(TUX_GREAT, font_tux_text, 1);
                           */

                          if (do_draw)
                            draw_fonts();


                          /* Only rerender when picking a different font */
                          if (toolopt_changed)
                            {
                              draw_fonts();
                              if (select_changed)
                                {
                                  rec_undo_buffer();
                                  do_render_cur_text(1);
                                  texttool_len = 0;
                                }
                              else
                                {
                                  do_render_cur_text(0);
                                }
                            }
                        }
                      else if (cur_tool == TOOL_STAMP)
                        {
#ifndef NOSOUND
                          /* Only play when picking a different stamp */
                          if (toolopt_changed && !mute)
                            {
                              /* If there's an SFX, play it! */

                              if (stamp_data[stamp_group][cur_thing]->ssnd != NULL)
                                {
                                  Mix_ChannelFinished(NULL);    /* Prevents multiple clicks from toggling between SFX and desc sound, rather than always playing SFX first, then desc sound... */

                                  Mix_PlayChannel(2, stamp_data[stamp_group][cur_thing]->ssnd, 0);

                                  /* If there's a description sound, play it after the SFX! */

                                  if (stamp_data[stamp_group][cur_thing]->sdesc != NULL)
                                    {
                                      Mix_ChannelFinished(playstampdesc);
                                    }
                                }
                              else
                                {
                                  /* No SFX?  If there's a description sound, play it now! */

                                  if (stamp_data[stamp_group][cur_thing]->sdesc != NULL)
                                    {
                                      Mix_PlayChannel(2, stamp_data[stamp_group][cur_thing]->sdesc, 0);
                                    }
                                }
                            }
#endif

                          if (cur_thing != cur_stamp[stamp_group])
                            {
                              cur_stamp[stamp_group] = cur_thing;
                              set_active_stamp();
                              update_stamp_xor();
                            }

                          if (do_draw)
                            draw_stamps();

                          if (stamp_data[stamp_group][cur_stamp[stamp_group]]->stxt != NULL)
                            {
#ifdef DEBUG
                              printf("stamp_data[stamp_group][cur_stamp[stamp_group]]->stxt = %s\n",
                                     stamp_data[stamp_group][cur_stamp[stamp_group]]->stxt);
#endif

                              draw_tux_text_ex(TUX_GREAT, stamp_data[stamp_group][cur_stamp[stamp_group]]->stxt, 1,
                                               stamp_data[stamp_group][cur_stamp[stamp_group]]->locale_text);
                            }
                          else
                            draw_tux_text(TUX_GREAT, "", 0);

                          /* Enable or disable color selector: */
                          draw_colors(stamp_colorable(cur_stamp[stamp_group])
                                      || stamp_tintable(cur_stamp[stamp_group]));
                          if (!scrolling)
                            {
                              stamp_xor(canvas->w / 2, canvas->h / 2);
                              stamp_xored = 1;
                              stamp_size_selector_clicked = 1;
                              update_screen(canvas->w / 2 - (CUR_STAMP_W + 1) / 2 + r_canvas.x,
                                            canvas->h / 2 - (CUR_STAMP_H + 1) / 2 + r_canvas.y,
                                            canvas->w / 2 + (CUR_STAMP_W + 1) / 2 + r_canvas.x,
                                            canvas->h / 2 + (CUR_STAMP_H + 1) / 2 + r_canvas.y);
                            }
                        }
                      else if (cur_tool == TOOL_SHAPES)
                        {
                          cur_shape = cur_thing;

                          /* Remove ghost previews an reset the tool */
                          if (shape_tool_mode != SHAPE_TOOL_MODE_DONE)
                            {
                              shape_tool_mode = SHAPE_TOOL_MODE_DONE;
                              do_undo();
                              tool_avail[TOOL_REDO] = 0;        /* Don't let them 'redo' to get preview back */
                              draw_toolbar();
                              update_screen_rect(&r_tools);
                              update_canvas(0, 0, canvas->w, canvas->h);
                            }


                          draw_tux_text(TUX_GREAT, shape_tips[cur_shape], 1);

                          if (do_draw)
                            draw_shapes();
                        }
                      else if (cur_tool == TOOL_MAGIC)
                        {
                          if (cur_thing != cur_magic)
                            {
                              magic_switchout(canvas);

                              cur_magic = cur_thing;
                              draw_colors(magics[cur_magic].colors);

                              if (magics[cur_magic].colors)
                                magic_funcs[magics[cur_magic].handle_idx].set_color(magic_api_struct,
                                                                                    color_hexes[cur_color][0],
                                                                                    color_hexes[cur_color][1],
                                                                                    color_hexes[cur_color][2]);

                              magic_switchin(canvas);
                            }

                          draw_tux_text(TUX_GREAT, magics[cur_magic].tip[magic_modeint(magics[cur_magic].mode)], 1);

                          if (do_draw)
                            draw_magic();
                        }

                      /* Update the screen: */
                      if (do_draw)
                        update_screen_rect(&r_toolopt);
                    }
                }
              else if (HIT(r_colors) && colors_are_selectable)
                {
                  /* Color! */
                  whichc = GRIDHIT_GD(r_colors, gd_colors);

                  if (valid_click(event.button.button))
                    {
                      // magic_switchout(canvas);

                      if (whichc >= 0 && whichc < NUM_COLORS)
                        {
                          cur_color = whichc;
                          draw_tux_text(TUX_KISS, color_names[cur_color], 1);

                          if (cur_color == (unsigned)(NUM_COLORS - 1) || cur_color == (unsigned)(NUM_COLORS - 2))
                            {
                              disable_avail_tools();
                              draw_toolbar();
                              draw_colors(COLORSEL_CLOBBER_WIPE);
                              draw_none();

                              if (cur_color == (unsigned)(NUM_COLORS - 1))
                                do_color_picker();
                              else
                                do_color_sel();

                              if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                                {
                                  if (onscreen_keyboard && kbd)
                                    {
                                      SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                                      update_screen_rect(&kbd_rect);
                                    }

                                  if (onscreen_keyboard && !kbd)
                                    {
                                      SDL_StartTextInput();
                                    }
                                }

                              enable_avail_tools();
                              draw_toolbar();
                              update_screen_rect(&r_tools);

                              draw_tux_text(TUX_GREAT, tool_tips[cur_tool], 1);

                              draw_colors(COLORSEL_FORCE_REDRAW);

                              if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_LINES)
                                draw_brushes();
                              else if (cur_tool == TOOL_MAGIC)
                                draw_magic();
                              else if (cur_tool == TOOL_STAMP)
                                draw_stamps();
                              else if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                                draw_fonts();
                              else if (cur_tool == TOOL_SHAPES)
                                draw_shapes();
                              else if (cur_tool == TOOL_ERASER)
                                draw_erasers();

                              playsound(screen, 1, SND_BUBBLE, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                              SDL_Flip(screen);
                            }
                          else
                            {
                              draw_colors(COLORSEL_REFRESH);

                              playsound(screen, 1, SND_BUBBLE, 1, event.button.x, SNDDIST_NEAR);
                            }

                          render_brush();


                          if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                            do_render_cur_text(0);
                          else if (cur_tool == TOOL_MAGIC)
                            magic_funcs[magics[cur_magic].handle_idx].set_color(magic_api_struct,
                                                                                color_hexes[cur_color][0],
                                                                                color_hexes[cur_color][1],
                                                                                color_hexes[cur_color][2]);
                        }
                    }
                }
              else if (HIT(r_canvas) && valid_click(event.button.button) && keyglobal == 0)
                {
                  /* Draw something! */
                  old_x = event.button.x - r_canvas.x;
                  old_y = event.button.y - r_canvas.y;
                  /* if (old_y < r_canvas.h/2) */
                  /* { */
                  /*            keybd_position = 0; */
                  /* } */
                  /* else */
                  /* { */
                  /*            keybd_position = 1; */
                  /* } */

                  if (been_saved)
                    {
                      been_saved = 0;

                      if (!disable_save)
                        tool_avail[TOOL_SAVE] = 1;

                      draw_toolbar();
                      update_screen_rect(&r_tools);
                    }

                  if (cur_tool == TOOL_BRUSH)
                    {
                      /* Start painting! */
                      if (!emulate_button_pressed)
                        rec_undo_buffer();

                      /* (Arbitrarily large, so we draw once now) */
                      reset_brush_counter();

                      /* brush_draw(old_x, old_y, old_x, old_y, 1); fixes SF #1934883? */
                      playsound(screen, 0, paintsound(img_cur_brush_w), 1, event.button.x, SNDDIST_NEAR);

                      if (mouseaccessibility)
                        emulate_button_pressed = !emulate_button_pressed;
                    }
                  else if (cur_tool == TOOL_LINES)
                    {
                      /* Start a line! */

                      if (!emulate_button_pressed)
                        {
                          rec_undo_buffer();

                          line_start_x = old_x;
                          line_start_y = old_y;

                          /* (Arbitrarily large, so we draw once now) */
                          reset_brush_counter();

                          /* brush_draw(old_x, old_y, old_x, old_y, 1); fixes sf #1934883? */

                          playsound(screen, 1, SND_LINE_START, 1, event.button.x, SNDDIST_NEAR);
                          draw_tux_text(TUX_BORED, TIP_LINE_START, 1);
                        }
                      if (mouseaccessibility)
                        emulate_button_pressed = !emulate_button_pressed;
                    }
                  else if (cur_tool == TOOL_SHAPES)
                    {
                      if (shape_tool_mode == SHAPE_TOOL_MODE_DONE)
                        {
                          /* Start drawing a shape! */

                          rec_undo_buffer();

                          shape_ctr_x = old_x;
                          shape_ctr_y = old_y;

                          shape_tool_mode = SHAPE_TOOL_MODE_STRETCH;

                          playsound(screen, 1, SND_LINE_START, 1, event.button.x, SNDDIST_NEAR);
                          draw_tux_text(TUX_BORED, TIP_SHAPE_START, 1);
                          if (mouseaccessibility)
                            emulate_button_pressed = 1;
                        }
                      else if (shape_tool_mode == SHAPE_TOOL_MODE_ROTATE)
                        {
                          /* Draw the shape with the brush! */

                          /* Only draw here in mouse accessibility mode as there IS a mouse */
                          /* See #300881 for the reasons that this is deplaced to draw in mouse release */
                          if (mouseaccessibility)
                            {
                              /* (Arbitrarily large...) */
                              reset_brush_counter();

                              playsound(screen, 1, SND_LINE_END, 1, event.button.x, SNDDIST_NEAR);
                              do_shape(shape_ctr_x, shape_ctr_y, shape_outer_x, shape_outer_y,
                                       shape_rotation(shape_ctr_x, shape_ctr_y,
                                                      event.button.x - r_canvas.x, event.button.y - r_canvas.y), 1);

                              shape_tool_mode = SHAPE_TOOL_MODE_DONE;
                              draw_tux_text(TUX_GREAT, tool_tips[TOOL_SHAPES], 1);
                            }
                        }
                      else if (shape_tool_mode == SHAPE_TOOL_MODE_STRETCH)
                        /* Only reached in accessibility mode */
                        emulate_button_pressed = 0;
                    }
                  else if (cur_tool == TOOL_MAGIC)
                    {
                      if (!emulate_button_pressed)
                        {
                          int undo_ctr;
                          SDL_Surface *last;

                          /* Start doing magic! */

                          /* These switchout/in are here for Magic tools that abuse the canvas
                             by drawing widgets on them; you don't want the widgets recorded as part
                             of the canvas in the undo buffer!
                             HOWEVER, as Pere noted in 2010.March, this causes many 'normal' Magic
                             tools to not work right, because they lose their record of the 'original'
                             canvas, before the user started using the tool (e.g., Rails, Perspective, Zoom).
                             FIXME: Some in-between solution is needed (a 'clean up the canvas'/'dirty the canvas'
                             pair of functions for the widgety abusers?) -bjk 2010.03.22 */

                          /* magic_switchout(canvas); *//* <-- FIXME: I dislike this -bjk 2009.10.13 */
                          rec_undo_buffer();
                          /* magic_switchin(canvas); *//* <-- FIXME: I dislike this -bjk 2009.10.13 */

                          if (cur_undo > 0)
                            undo_ctr = cur_undo - 1;
                          else
                            undo_ctr = NUM_UNDO_BUFS - 1;

                          last = undo_bufs[undo_ctr];

                          update_rect.x = 0;
                          update_rect.y = 0;
                          update_rect.w = 0;
                          update_rect.h = 0;

                          reset_touched();

                          magic_funcs[magics[cur_magic].handle_idx].click(magic_api_struct,
                                                                          magics[cur_magic].idx,
                                                                          magics[cur_magic].mode,
                                                                          canvas, last, old_x, old_y, &update_rect);

                          draw_tux_text(TUX_GREAT, magics[cur_magic].tip[magic_modeint(magics[cur_magic].mode)], 1);

                          update_canvas(update_rect.x, update_rect.y,
                                        update_rect.x + update_rect.w, update_rect.y + update_rect.h);
                        }

                      if (mouseaccessibility)
                        {
                          if (magics[cur_magic].mode != MODE_FULLSCREEN && magics[cur_magic].mode != MODE_ONECLICK)     /* Note: some non-fullscreen tools are also click-only (not click-and-drag) -bjk 2011.04.26 */
                            emulate_button_pressed = !emulate_button_pressed;
                        }
                    }
                  else if (cur_tool == TOOL_ERASER)
                    {
                      /* Erase! */
                      if (!emulate_button_pressed)
                        rec_undo_buffer();

                      do_eraser(old_x, old_y);

                      if (mouseaccessibility)
                        emulate_button_pressed = !emulate_button_pressed;
                    }
                  else if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                    {
                      if (onscreen_keyboard && !kbd)
                        {
                          r_tir.y = (float)old_y / render_scale;
                          SDL_SetTextInputRect(&r_tir);
                          SDL_StartTextInput();
                        }

                      /* Text and Label Tools! */
                      if (cur_tool == TOOL_LABEL && cur_label == LABEL_SELECT)
                        {
                          label_node_to_edit = search_label_list(&highlighted_label_node, old_x, old_y, 0);
                          if (label_node_to_edit)
                            {
                              cur_label = LABEL_LABEL;
                              cur_thing = label_node_to_edit->save_cur_font;
                              do_setcursor(cursor_insertion);
                              i = 0;
                              label_node_to_edit->is_enabled = FALSE;
                              derender_node(&label_node_to_edit);

                              texttool_len = select_texttool_len;
                              while (i < texttool_len)
                                {
                                  texttool_str[i] = select_texttool_str[i];
                                  i = i + 1;
                                }
                              texttool_str[i] = L'\0';
                              cur_color = select_color;
                              old_x = select_x;
                              old_y = select_y;
                              cur_font = select_cur_font;
                              text_state = select_text_state;
                              text_size = select_text_size;
                              // int j;
                              for (j = 0; j < num_font_families; j++)
                                {
                                  if (user_font_families[j] && user_font_families[j]->handle)
                                    {
                                      TuxPaint_Font_CloseFont(user_font_families[j]->handle);
                                      user_font_families[j]->handle = NULL;
                                    }
                                }
                              draw_fonts();
                              update_screen_rect(&r_toolopt);
                              if (onscreen_keyboard && kbd)
                                {
                                  if (old_y < r_canvas.h / 2)
                                    kbd_rect.y = r_canvas.h - kbd->surface->h;
                                  else
                                    kbd_rect.y = 0;

                                  SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                                  update_screen_rect(&kbd_rect);
                                }

                              if (onscreen_keyboard && !kbd)
                                {
                                  r_tir.y = (float)old_y / render_scale;
                                  SDL_SetTextInputRect(&r_tir);
                                  SDL_StartTextInput();
                                }
                              do_render_cur_text(0);
                              draw_colors(COLORSEL_REFRESH);
                              draw_fonts();
                            }

                        }
                      else
                        hide_blinking_cursor();



                      if (cursor_x != -1 && cursor_y != -1)
                        {
                          /*
                             if (texttool_len > 0)
                             {
                             rec_undo_buffer();
                             do_render_cur_text(1);
                             texttool_len = 0;
                             }
                           */
                        }
                      if (onscreen_keyboard && kbd && HIT(kbd_rect)
                          && !(cur_tool == TOOL_LABEL && cur_label == LABEL_SELECT))
                        {
                          new_kbd = osk_clicked(kbd, old_x - kbd_rect.x + r_canvas.x, old_y - kbd_rect.y + r_canvas.y);
                          /* keyboard has changed, erase the old, note that the old kbd has yet been freed. */
                          if (new_kbd != kbd)
                            {
                              kbd = new_kbd;
                              update_canvas_ex(kbd_rect.x, kbd_rect.y, kbd_rect.x + kbd_rect.w, kbd_rect.y + kbd_rect.h,
                                               0);
                              /* set kbd_rect dimensions according to the new keyboard */
                              kbd_rect.x = button_w * 2 + (canvas->w - kbd->surface->w) / 2;
                              if (kbd_rect.y != 0)
                                kbd_rect.y = canvas->h - kbd->surface->h;
                              kbd_rect.w = kbd->surface->w;
                              kbd_rect.h = kbd->surface->h;
                            }
                          SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                          update_screen_rect(&kbd_rect);
                        }
                      else
                        {
                          cursor_x = old_x;
                          cursor_y = old_y;
                          cursor_left = old_x;

                          if (onscreen_keyboard && kbd && !(cur_tool == TOOL_LABEL && cur_label == LABEL_SELECT))
                            {
                              if (old_y < r_canvas.h / 2)
                                {
                                  if (kbd_rect.y != r_canvas.h - kbd->surface->h)
                                    {
                                      update_canvas_ex(kbd_rect.x, kbd_rect.y, kbd_rect.x + kbd_rect.w,
                                                       kbd_rect.y + kbd_rect.h, 0);
                                      update_screen_rect(&kbd_rect);
                                      kbd_rect.y = r_canvas.h - kbd->surface->h;
                                      SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                                      update_screen_rect(&kbd_rect);
                                    }
                                }
                              else
                                {
                                  if (kbd_rect.y != 0)
                                    {
                                      update_canvas_ex(kbd_rect.x, kbd_rect.y, kbd_rect.x + kbd_rect.w,
                                                       kbd_rect.y + kbd_rect.h, 0);
                                      update_screen_rect(&kbd_rect);
                                      kbd_rect.y = 0;
                                      SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                                      update_screen_rect(&kbd_rect);
                                    }
                                }
                            }

                          if (onscreen_keyboard && !kbd)
                            {
                              r_tir.y = (float)cursor_y / render_scale;
                              SDL_SetTextInputRect(&r_tir);
                              SDL_StartTextInput();
                            }
                        }

                      do_render_cur_text(0);

                    }

                  button_down = 1;
                }
              else if (HIT(r_sfx) && valid_click(event.button.button))
                {
                  /* A sound player button on the lower left has been pressed! */
#ifndef NOSOUND
                  if (cur_tool == TOOL_STAMP && use_sound && !mute)
                    {
                      which = GRIDHIT_GD(r_sfx, gd_sfx);

                      if (which == 0 && !stamp_data[stamp_group][cur_stamp[stamp_group]]->no_sound)
                        {
                          /* Re-play sound effect: */

                          Mix_ChannelFinished(NULL);
                          Mix_PlayChannel(2, stamp_data[stamp_group][cur_thing]->ssnd, 0);
                        }
                      else if (which == 1 && !stamp_data[stamp_group][cur_stamp[stamp_group]]->no_descsound)
                        {
                          Mix_ChannelFinished(NULL);
                          Mix_PlayChannel(2, stamp_data[stamp_group][cur_thing]->sdesc, 0);
                        }

                      magic_switchout(canvas);
                    }
#endif
                }

#ifdef __ANDROID__
              start_motion_convert(event);
#endif
            }

          else if (event.type == SDL_MOUSEWHEEL && wheely)
            {
              int most = 14;
              int num_rows_needed;
              int xpos, ypos;
              SDL_Rect r_controls;
              SDL_Rect r_notcontrols;
              SDL_Rect r_items; /* = r_notcontrols; */

              SDL_GetMouseState(&xpos, &ypos);

              /* Scroll wheel code.
                 WARNING: this must be kept in sync with the mouse-move
                 code (for cursor changes) and mouse-click code. */

              if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_STAMP ||
                  cur_tool == TOOL_SHAPES || cur_tool == TOOL_LINES ||
                  cur_tool == TOOL_MAGIC || cur_tool == TOOL_TEXT || cur_tool == TOOL_ERASER || cur_tool == TOOL_LABEL)
                {

                  /* Left tools scroll */
                  if (hit_test(&r_tools, xpos, ypos) && NUM_TOOLS > most + TOOLOFFSET)
                    {
                      int is_upper = (event.wheel.y > 0);

                      if (is_upper && tool_scroll > 0)
                        {
                          tool_scroll -= gd_tools.cols;
                          playsound(screen, 1, SND_SCROLL, 1, event.button.x, SNDDIST_NEAR);
                          draw_toolbar();
                        }
                      else if (!is_upper && tool_scroll < NUM_TOOLS - 12 - TOOLOFFSET)
                        {
                          tool_scroll += gd_tools.cols;
                          playsound(screen, 1, SND_SCROLL, 1, event.button.x, SNDDIST_NEAR);
                          draw_toolbar();
                        }

                      if (event.button.y < r_tools.y + button_h / 2)    // cursor on the upper button
                        {
                          if (tool_scroll == 0)
                            do_setcursor(cursor_arrow);
                          else
                            do_setcursor(cursor_up);
                        }

                      else if (event.button.y > r_tools.y + r_tools.h - button_h / 2)   // cursor on the lower button
                        {
                          if (tool_scroll < NUM_TOOLS - 12 - TOOLOFFSET)
                            do_setcursor(cursor_down);
                          else
                            do_setcursor(cursor_arrow);
                        }

                      else if (tool_avail[((event.button.x - r_tools.x) / button_w) +
                                          ((event.button.y -
                                            r_tools.y - button_h / 2) / button_h) * gd_tools.cols + tool_scroll])
                        {
                          do_setcursor(cursor_hand);
                        }
                      else
                        {
                          do_setcursor(cursor_arrow);
                        }
                      update_screen_rect(&r_tools);
                    }

                  /* Right tool options scroll */
                  else
                    {
                      grid_dims gd_controls;    /* might become 2-by-2 */
                      grid_dims gd_items;       /* generally becoming 2-by-whatever */

                      gd_controls.rows = 0;
                      gd_controls.cols = 0;
                      gd_items.rows = 2;
                      gd_items.cols = 2;

                      /* Note set of things we're dealing with */
                      /* (stamps, brushes, etc.) */

                      if (cur_tool == TOOL_STAMP)
                        {
                          if (!disable_stamp_controls)
                            {
                              /* was 2,2 before adding left/right stamp group buttons -bjk 2007.05.15 */
                              gd_controls.rows = 3;
                              gd_controls.cols = 2;
                            }
                          else
                            {
                              /* was left 0,0 before adding left/right stamp group buttons -bjk 2007.05.03 */
                              gd_controls.rows = 1;
                              gd_controls.cols = 2;
                            }
                        }
                      else if (cur_tool == TOOL_TEXT)
                        {
                          if (!disable_stamp_controls)
                            {
                              gd_controls.rows = 2;
                              gd_controls.cols = 2;
                            }
                        }
                      else if (cur_tool == TOOL_LABEL)
                        {
                          if (!disable_stamp_controls)
                            {
                              gd_controls.rows = 3;
                              gd_controls.cols = 2;
                            }
                          else
                            {
                              gd_controls.rows = 1;
                              gd_controls.cols = 2;
                            }
                        }
                      else if (cur_tool == TOOL_MAGIC)
                        {
                          if (!disable_magic_controls)
                            {
                              gd_controls.rows = 1;
                              gd_controls.cols = 2;
                            }
                        }

                      /* number of whole or partial rows that will be needed
                         (can make this per-tool if variable columns needed) */
                      num_rows_needed = (num_things + gd_items.cols - 1) / gd_items.cols;

                      do_draw = 0;

                      r_controls.w = r_toolopt.w;
                      r_controls.h = gd_controls.rows * button_h;
                      r_controls.x = r_toolopt.x;
                      r_controls.y = r_toolopt.y + r_toolopt.h - r_controls.h;

                      r_notcontrols.w = r_toolopt.w;
                      r_notcontrols.h = r_toolopt.h - r_controls.h;
                      r_notcontrols.x = r_toolopt.x;
                      r_notcontrols.y = r_toolopt.y;

                      r_items.x = r_notcontrols.x;
                      r_items.y = r_notcontrols.y;
                      r_items.w = r_notcontrols.w;
                      r_items.h = r_notcontrols.h;

                      if (num_rows_needed * button_h > r_items.h)
                        {
                          /* too many; we'll need scroll buttons */
                          r_items.h -= button_h;
                          r_items.y += button_h / 2;
                        }
                      gd_items.rows = r_items.h / button_h;

                      if (0)
                        {
                        }
                      else
                        {
                          /* scroll button */
                          int is_upper = (event.wheel.y > (Sint32) 0);

                          if ((is_upper && *thing_scroll > 0)   /* upper arrow */
                              || (!is_upper && *thing_scroll / gd_items.cols < num_rows_needed - gd_items.rows) /* lower arrow */
                            )
                            {
                              *thing_scroll += is_upper ? -gd_items.cols : gd_items.cols;
                              do_draw = 1;
                              playsound(screen, 1, SND_SCROLL, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
                              if (*thing_scroll == 0)
                                {
                                  do_setcursor(cursor_arrow);
                                }
                            }
                        }


                      /* Assign the change(s), if any / redraw, if needed: */

                      if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_LINES)
                        {
                          if (do_draw)
                            draw_brushes();
                        }
                      else if (cur_tool == TOOL_ERASER)
                        {
                          if (do_draw)
                            draw_erasers();
                        }
                      else if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                        {
                          if (do_draw)
                            draw_fonts();
                        }
                      else if (cur_tool == TOOL_STAMP)
                        {
                          if (do_draw)
                            draw_stamps();
                        }
                      else if (cur_tool == TOOL_SHAPES)
                        {
                          if (do_draw)
                            draw_shapes();
                        }
                      else if (cur_tool == TOOL_MAGIC)
                        {
                          if (do_draw)
                            draw_magic();
                        }

                      /* Update the screen: */
                      if (do_draw)
                        update_screen_rect(&r_toolopt);

                    }
                }
            }
          else if (event.type == SDL_USEREVENT)
            {
              if (event.user.code == USEREVENT_TEXT_UPDATE)
                {
                  /* Time to replace "Great!" with old tip text: */

                  if (event.user.data1 != NULL)
                    {
                      if (((unsigned char *)event.user.data1)[0] == '=')
                        {
                          draw_tux_text_ex(TUX_GREAT, (char *)event.user.data1 + 1, 1, (int)(intptr_t) event.user.data2);       //EP added (intptr_t) to avoid warning on x64
                        }
                      else
                        {
                          draw_tux_text_ex(TUX_GREAT, (char *)event.user.data1, 0, (int)(intptr_t) event.user.data2);   //EP added (intptr_t) to avoid warning on x64
                        }
                    }
                  else
                    draw_tux_text(TUX_GREAT, "", 1);
                }
              else if (event.user.code == USEREVENT_PLAYDESCSOUND)
                {
                  /* Play a stamp's spoken description (because the sound effect just finished) */
                  /* (This event is pushed into the queue by playstampdesc(), which
                     is a callback from Mix_ChannelFinished() when playing a stamp SFX) */

                  debug("Playing description sound...");

#ifndef NOSOUND
                  Mix_ChannelFinished(NULL);    /* Kill the callback, so we don't get stuck in a loop! */

                  if (event.user.data1 != NULL)
                    {
                      if ((int)(intptr_t) event.user.data1 == cur_stamp[stamp_group])   /* Don't play old stamp's sound... *///EP added (intptr_t) to avoid warning on x64
                        {
                          if (!mute && stamp_data[stamp_group][(int)(intptr_t) event.user.data1]->sdesc != NULL)        //EP added (intptr_t) to avoid warning on x64
                            Mix_PlayChannel(2, stamp_data[stamp_group][(int)(intptr_t) event.user.data1]->sdesc,        //EP added (intptr_t) to avoid warning on x64
                                            0);
                        }
                    }
#endif
                }
            }
          else if (event.type == SDL_MOUSEBUTTONUP)
            {
              if (scrolling)
                {
                  if (scrolltimer != NULL)
                    {
                      SDL_RemoveTimer(scrolltimer);
                      scrolltimer = NULL;
                    }
                  scrolling = 0;

                  /* printf("Killing scrolling\n"); */
                }
              /* Erase the xor drawed at click */
              else if (cur_tool == TOOL_STAMP && stamp_xored && event.button.button < 4)
                {
                  stamp_xor(canvas->w / 2, canvas->h / 2);
                  stamp_xored = 0;
                  stamp_size_selector_clicked = 0;
                  update_screen(canvas->w / 2 - (CUR_STAMP_W + 1) / 2 + r_canvas.x,
                                canvas->h / 2 - (CUR_STAMP_H + 1) / 2 + r_canvas.y,
                                canvas->w / 2 + (CUR_STAMP_W + 1) / 2 + r_canvas.x,
                                canvas->h / 2 + (CUR_STAMP_H + 1) / 2 + r_canvas.y);
                }


              if (button_down || emulate_button_pressed)
                {
                  if (cur_tool == TOOL_BRUSH)
                    {
                      /* (Drawing on mouse release to fix single click issue) */
                      brush_draw(old_x, old_y, old_x, old_y, 1);
                    }
                  else if (cur_tool == TOOL_STAMP)
                    {
                      if (old_x >= 0 && old_y >= 0 && old_x <= r_canvas.w && old_y <= r_canvas.h)
                        {
                          /* Draw a stamp! */

                          rec_undo_buffer();

                          stamp_draw(old_x, old_y);
                          stamp_xor(old_x, old_y);
                          playsound(screen, 1, SND_STAMP, 1, event.button.x, SNDDIST_NEAR);

                          draw_tux_text(TUX_GREAT, great_str(), 1);

                          /* FIXME: Make delay configurable: */

                          control_drawtext_timer(1000, stamp_data[stamp_group][cur_stamp[stamp_group]]->stxt,
                                                 stamp_data[stamp_group][cur_stamp[stamp_group]]->locale_text);
                        }
                    }

                  else if (cur_tool == TOOL_LINES)
                    {
                      if (!mouseaccessibility || (mouseaccessibility && !emulate_button_pressed))
                        {
                          /* (Arbitrarily large, so we draw once now) */
                          reset_brush_counter();

                          brush_draw(line_start_x, line_start_y,
                                     event.button.x - r_canvas.x, event.button.y - r_canvas.y, 1);
                          brush_draw(event.button.x - r_canvas.x,
                                     event.button.y - r_canvas.y,
                                     event.button.x - r_canvas.x, event.button.y - r_canvas.y, 1);

                          playsound(screen, 1, SND_LINE_END, 1, event.button.x, SNDDIST_NEAR);
                          draw_tux_text(TUX_GREAT, tool_tips[TOOL_LINES], 1);
                        }
                    }

                  else if (cur_tool == TOOL_SHAPES)
                    {
                      if (!mouseaccessibility || (mouseaccessibility && !emulate_button_pressed))
                        {
                          if (shape_tool_mode == SHAPE_TOOL_MODE_STRETCH)
                            {
                              /* Now we can rotate the shape... */

                              shape_outer_x = event.button.x - r_canvas.x;
                              shape_outer_y = event.button.y - r_canvas.y;

                              if (!simple_shapes && !shape_no_rotate[cur_shape])
                                {
                                  shape_tool_mode = SHAPE_TOOL_MODE_ROTATE;

                                  shape_radius =
                                    sqrt((shape_ctr_x - shape_outer_x) * (shape_ctr_x - shape_outer_x) +
                                         (shape_ctr_y - shape_outer_y) * (shape_ctr_y - shape_outer_y));

                                  SDL_WarpMouse(shape_outer_x + 96, shape_ctr_y);
                                  do_setcursor(cursor_rotate);


                                  /* Erase stretchy XOR: */

                                  if (abs(shape_ctr_x - shape_outer_x) > 15 || abs(shape_ctr_y - shape_outer_y) > 15)
                                    do_shape(shape_ctr_x, shape_ctr_y, old_x, old_y, 0, 0);

                                  /* Make an initial rotation XOR to be erased: */

                                  do_shape(shape_ctr_x, shape_ctr_y,
                                           shape_outer_x, shape_outer_y,
                                           shape_rotation(shape_ctr_x, shape_ctr_y, shape_outer_x, shape_outer_y), 0);

                                  playsound(screen, 1, SND_LINE_START, 1, event.button.x, SNDDIST_NEAR);
                                  draw_tux_text(TUX_BORED, TIP_SHAPE_NEXT, 1);


                                  /* FIXME: Do something less intensive! */

                                  SDL_Flip(screen);
                                }
                              else
                                {
                                  reset_brush_counter();


                                  playsound(screen, 1, SND_LINE_END, 1, event.button.x, SNDDIST_NEAR);
                                  do_shape(shape_ctr_x, shape_ctr_y, shape_outer_x, shape_outer_y, 0, 1);

                                  SDL_Flip(screen);

                                  shape_tool_mode = SHAPE_TOOL_MODE_DONE;
                                  draw_tux_text(TUX_GREAT, tool_tips[TOOL_SHAPES], 1);
                                }
                            }
                          else if (shape_tool_mode == SHAPE_TOOL_MODE_ROTATE)
                            {
                              reset_brush_counter();

                              playsound(screen, 1, SND_LINE_END, 1, event.button.x, SNDDIST_NEAR);
                              do_shape(shape_ctr_x, shape_ctr_y, shape_outer_x, shape_outer_y,
                                       shape_rotation(shape_ctr_x, shape_ctr_y,
                                                      event.button.x - r_canvas.x, event.button.y - r_canvas.y), 1);

                              shape_tool_mode = SHAPE_TOOL_MODE_DONE;
                              draw_tux_text(TUX_GREAT, tool_tips[TOOL_SHAPES], 1);
                            }
                        }
                    }
                  else if (cur_tool == TOOL_MAGIC
                           && (magics[cur_magic].mode == MODE_PAINT || magics[cur_magic].mode == MODE_ONECLICK
                               || magics[cur_magic].mode == MODE_PAINT_WITH_PREVIEW))
                    {
                      if (!mouseaccessibility || (mouseaccessibility && !emulate_button_pressed))
                        {
                          int undo_ctr;
                          SDL_Surface *last;

                          /* Releasing button: Finish the magic: */

                          if (cur_undo > 0)
                            undo_ctr = cur_undo - 1;
                          else
                            undo_ctr = NUM_UNDO_BUFS - 1;

                          last = undo_bufs[undo_ctr];

                          update_rect.x = 0;
                          update_rect.y = 0;
                          update_rect.w = 0;
                          update_rect.h = 0;

                          magic_funcs[magics[cur_magic].handle_idx].release(magic_api_struct,
                                                                            magics[cur_magic].idx,
                                                                            canvas, last, old_x, old_y, &update_rect);

                          draw_tux_text(TUX_GREAT, magics[cur_magic].tip[magic_modeint(magics[cur_magic].mode)], 1);

                          update_canvas(update_rect.x, update_rect.y,
                                        update_rect.x + update_rect.w, update_rect.y + update_rect.h);
                        }
                    }
                  else if (cur_tool == TOOL_TEXT || (cur_tool == TOOL_LABEL && cur_label != LABEL_SELECT))
                    {
                      if (onscreen_keyboard && kbd)
                        {
                          osk_released(kbd);
                          SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                          update_screen_rect(&kbd_rect);
                          //      SDL_Flip(screen);
                        }

                      if (onscreen_keyboard && !kbd)
                        {
                          SDL_StartTextInput();
                        }
                    }
                }
              button_down = 0;

#ifdef __ANDROID__
              stop_motion_convert(event);
#endif
            }
          else if (event.type == SDL_MOUSEMOTION && !ignoring_motion)
            {
              new_x = event.button.x - r_canvas.x;
              new_y = event.button.y - r_canvas.y;

#ifdef __ANDROID__
              convert_motion_to_wheel(event);
#endif

              oldpos_x = event.motion.x;
              oldpos_y = event.motion.y;


              /* FIXME: Is doing this every event too intensive? */
              /* Should I check current cursor first? */

              if (HIT(r_tools))
                {
                  int most = 14;

                  /* Tools: */

                  if (NUM_TOOLS > most + TOOLOFFSET)
                    {
                      if (event.button.y < r_tools.y + button_h / 2)
                        {
                          if (tool_scroll > 0)
                            do_setcursor(cursor_up);
                          else
                            do_setcursor(cursor_arrow);
                        }
                      else if (event.button.y > r_tools.y + r_tools.h - button_h / 2)
                        {
                          if (tool_scroll < NUM_TOOLS - 12 - TOOLOFFSET)
                            do_setcursor(cursor_down);
                          else
                            do_setcursor(cursor_arrow);
                        }

                      else if (tool_avail[((event.button.x - r_tools.x) / button_w) +
                                          ((event.button.y -
                                            r_tools.y - button_h / 2) / button_h) * gd_tools.cols + tool_scroll])
                        {
                          do_setcursor(cursor_hand);
                        }
                      else
                        {
                          do_setcursor(cursor_arrow);
                        }

                    }

                  else
                    {
                      if (tool_avail[((event.button.x - r_tools.x) / button_w) +
                                     ((event.button.y - r_tools.y) / button_h) * gd_tools.cols])
                        {
                          do_setcursor(cursor_hand);
                        }
                      else
                        {
                          do_setcursor(cursor_arrow);
                        }
                    }
                }
              else if (HIT(r_sfx))
                {
                  /* Sound player buttons: */

                  if (cur_tool == TOOL_STAMP && use_sound && !mute &&
                      ((GRIDHIT_GD(r_sfx, gd_sfx) == 0 &&
                        !stamp_data[stamp_group][cur_stamp[stamp_group]]->no_sound) ||
                       (GRIDHIT_GD(r_sfx, gd_sfx) == 1 &&
                        !stamp_data[stamp_group][cur_stamp[stamp_group]]->no_descsound)))
                    {
                      do_setcursor(cursor_hand);
                    }
                  else
                    {
                      do_setcursor(cursor_arrow);
                    }
                }
              else if (HIT(r_colors))
                {
                  /* Color picker: */
                  if (colors_are_selectable)
                    do_setcursor(cursor_hand);
                  else
                    do_setcursor(cursor_arrow);
                }
              else if (HIT(r_toolopt))
                {
                  /* mouse cursor code
                     WARNING: this must be kept in sync with the mouse-click
                     and mouse-click code. (it isn't, currently!) */

                  /* Note set of things we're dealing with */
                  /* (stamps, brushes, etc.) */
                  if (cur_tool == TOOL_STAMP)
                    {
                    }
                  else if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                    {
                    }

                  max = 14;
                  if (cur_tool == TOOL_STAMP && !disable_stamp_controls)
                    max = 8;    /* was 10 before left/right group buttons -bjk 2007.05.03 */
                  if (cur_tool == TOOL_LABEL)
                    {
                      max = 12;
                      if (!disable_stamp_controls)
                        max = 8;
                    }

                  if (cur_tool == TOOL_TEXT && !disable_stamp_controls)
                    max = 10;
                  if (cur_tool == TOOL_MAGIC && !disable_magic_controls)
                    max = 12;


                  if (num_things > max + TOOLOFFSET)
                    {
                      /* Are there scroll buttons? */
                      if (event.button.y < 40 + 24)
                        {
                          /* Up button; is it available? */

                          if (*thing_scroll > 0)
                            do_setcursor(cursor_up);
                          else
                            do_setcursor(cursor_arrow);
                        }
                      else if (event.button.y >
                               (48 * ((max - 2) / 2 + TOOLOFFSET / 2)) + 40 + 24
                               && event.button.y <= (48 * ((max - 2) / 2 + TOOLOFFSET / 2)) + 40 + 24 + 24)
                        {
                          /* Down button; is it available? */

                          if (*thing_scroll < num_things - (max - 2))
                            do_setcursor(cursor_down);
                          else
                            do_setcursor(cursor_arrow);
                        }
                      else
                        {
                          /* One of the selectors: */

                          which = ((event.button.y - 40 - 24) / 48) * 2 + (event.button.x - (WINDOW_WIDTH - 96)) / 48;

                          if (which < num_things)
                            do_setcursor(cursor_hand);
                          else
                            do_setcursor(cursor_arrow);
                        }
                    }
                  else
                    {
                      /* No scroll buttons - must be a selector: */

                      which = ((event.button.y - 40) / 48) * 2 + (event.button.x - (WINDOW_WIDTH - 96)) / 48;

                      if (which < num_things)
                        do_setcursor(cursor_hand);
                      else
                        do_setcursor(cursor_arrow);
                    }
                }
              else if (HIT(r_canvas) && keyglobal == 0)
                {
                  /* Canvas: */

                  if (cur_tool == TOOL_BRUSH)
                    do_setcursor(cursor_brush);
                  else if (cur_tool == TOOL_STAMP)
                    do_setcursor(cursor_tiny);
                  else if (cur_tool == TOOL_LINES)
                    do_setcursor(cursor_crosshair);
                  else if (cur_tool == TOOL_SHAPES)
                    {
                      if (shape_tool_mode != SHAPE_TOOL_MODE_ROTATE)
                        do_setcursor(cursor_crosshair);
                      else
                        do_setcursor(cursor_rotate);
                    }
                  else if (cur_tool == TOOL_TEXT)
                    {
                      if (onscreen_keyboard && HIT(kbd_rect))
                        do_setcursor(cursor_hand);
                      else
                        do_setcursor(cursor_insertion);
                    }
                  else if (cur_tool == TOOL_LABEL)
                    {
                      if (cur_label == LABEL_LABEL)
                        if (onscreen_keyboard && HIT(kbd_rect))
                          do_setcursor(cursor_hand);
                        else
                          do_setcursor(cursor_insertion);
                      else if (cur_label == LABEL_SELECT)
                        {
                          if (search_label_list(&current_label_node, event.button.x - 96, event.button.y, 1))
                            do_setcursor(cursor_hand);
                          else
                            do_setcursor(cursor_arrow);
                        }
                    }

                  else if (cur_tool == TOOL_MAGIC)
                    do_setcursor(cursor_wand);
                  else if (cur_tool == TOOL_ERASER)
                    do_setcursor(cursor_tiny);

                }
              else
                {
                  do_setcursor(cursor_arrow);
                }


              if (button_down || emulate_button_pressed)
                {
                  if (cur_tool == TOOL_BRUSH)
                    {
                      /* Pushing button and moving: Draw with the brush: */

                      brush_draw(old_x, old_y, new_x, new_y, 1);

                      playsound(screen, 0, paintsound(img_cur_brush_w), 0, event.button.x, SNDDIST_NEAR);
                    }
                  else if (cur_tool == TOOL_LINES)
                    {
                      /* Still pushing button, while moving:
                         Draw XOR where line will go: */

                      line_xor(line_start_x, line_start_y, old_x, old_y);

                      line_xor(line_start_x, line_start_y, new_x, new_y);

#ifndef __ANDROID__
                      update_screen(line_start_x + r_canvas.x,
                                    line_start_y + r_canvas.y, old_x + r_canvas.x, old_y + r_canvas.y);
                      update_screen(line_start_x + r_canvas.x,
                                    line_start_y + r_canvas.y, new_x + r_canvas.x, new_y + r_canvas.y);
#else
                      /* Anyway SDL_UpdateRect() backward compatibility function refreshes all the screen on Android */
                      SDL_UpdateRect(screen, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
#endif
                    }
                  else if (cur_tool == TOOL_SHAPES)
                    {
                      /* Still pushing button, while moving:
                         Draw XOR where shape will go: */

                      if (shape_tool_mode == SHAPE_TOOL_MODE_STRETCH)
                        {
                          do_shape(shape_ctr_x, shape_ctr_y, old_x, old_y, 0, 0);

                          do_shape(shape_ctr_x, shape_ctr_y, new_x, new_y, 0, 0);


                          /* FIXME: Fix update shape function! */

                          /* update_shape(shape_ctr_x, old_x, new_x,
                             shape_ctr_y, old_y, new_y,
                             shape_locked[cur_shape]); */

                          SDL_Flip(screen);
                        }
                    }
                  else if (cur_tool == TOOL_MAGIC
                           && (magics[cur_magic].mode == MODE_PAINT || magics[cur_magic].mode == MODE_ONECLICK
                               || magics[cur_magic].mode == MODE_PAINT_WITH_PREVIEW))
                    {
                      int undo_ctr;
                      SDL_Surface *last;

                      /* Pushing button and moving: Continue doing the magic: */

                      if (cur_undo > 0)
                        undo_ctr = cur_undo - 1;
                      else
                        undo_ctr = NUM_UNDO_BUFS - 1;

                      last = undo_bufs[undo_ctr];

                      update_rect.x = 0;
                      update_rect.y = 0;
                      update_rect.w = 0;
                      update_rect.h = 0;

                      magic_funcs[magics[cur_magic].handle_idx].drag(magic_api_struct,
                                                                     magics[cur_magic].idx,
                                                                     canvas, last,
                                                                     old_x, old_y, new_x, new_y, &update_rect);

                      update_canvas(update_rect.x, update_rect.y,
                                    update_rect.x + update_rect.w, update_rect.y + update_rect.h);
                    }
                  else if (cur_tool == TOOL_ERASER)
                    {
                      /* Still pushing, and moving - Erase! */

                      do_eraser(new_x, new_y);
                    }
                }


              if (cur_tool == TOOL_STAMP ||
                  ((cur_tool == TOOL_ERASER && !button_down) &&
                   (!mouseaccessibility || (mouseaccessibility && !emulate_button_pressed))))
                {
                  int w = 0;
                  int h = 0;

                  /* Moving: Draw XOR where stamp/eraser will apply: */


                  if (cur_tool == TOOL_STAMP)
                    {
                      w = active_stamp->w;
                      h = active_stamp->h;
                    }
                  else
                    {
                      if (cur_eraser < NUM_ERASERS / 2)
                        {
                          w = (ERASER_MIN +
                               (((NUM_ERASERS / 2) - cur_eraser - 1) *
                                ((ERASER_MAX - ERASER_MIN) / ((NUM_ERASERS / 2) - 1))));
                        }
                      else
                        {
                          w = (ERASER_MIN +
                               (((NUM_ERASERS / 2) - (cur_eraser - NUM_ERASERS / 2) - 1) *
                                ((ERASER_MAX - ERASER_MIN) / ((NUM_ERASERS / 2) - 1))));
                        }

                      h = w;
                    }

                  if (old_x >= 0 && old_x < r_canvas.w && old_y >= 0 && old_y < r_canvas.h)
                    {
                      if (cur_tool == TOOL_STAMP)
                        {
                          stamp_xor(old_x, old_y);

                          update_screen(old_x - (CUR_STAMP_W + 1) / 2 + r_canvas.x,
                                        old_y - (CUR_STAMP_H + 1) / 2 + r_canvas.y,
                                        old_x + (CUR_STAMP_W + 1) / 2 + r_canvas.x,
                                        old_y + (CUR_STAMP_H + 1) / 2 + r_canvas.y);
                        }

                      else
                        {
                          rect_xor(old_x - w / 2, old_y - h / 2, old_x + w / 2, old_y + h / 2);

                          update_screen(old_x - w / 2 + r_canvas.x,
                                        old_y - h / 2 + r_canvas.y,
                                        old_x + w / 2 + r_canvas.x, old_y + h / 2 + r_canvas.y);
                        }
                    }

                  if (new_x >= 0 && new_x < r_canvas.w && new_y >= 0 && new_y < r_canvas.h)
                    {
                      if (cur_tool == TOOL_STAMP)
                        {
                          stamp_xor(new_x, new_y);

                          update_screen(old_x - (CUR_STAMP_W + 1) / 2 + r_canvas.x,
                                        old_y - (CUR_STAMP_H + 1) / 2 + r_canvas.y,
                                        old_x + (CUR_STAMP_W + 1) / 2 + r_canvas.x,
                                        old_y + (CUR_STAMP_H + 1) / 2 + r_canvas.y);
                        }
                      else
                        {
                          rect_xor(new_x - w / 2, new_y - h / 2, new_x + w / 2, new_y + h / 2);

                          update_screen(new_x - w / 2 + r_canvas.x,
                                        new_y - h / 2 + r_canvas.y,
                                        new_x + w / 2 + r_canvas.x, new_y + h / 2 + r_canvas.y);
                        }
                    }
                  if (cur_tool == TOOL_STAMP && HIT(r_toolopt) && event.motion.y > r_toolopt.h
                      && event.motion.state == SDL_PRESSED && stamp_size_selector_clicked)
                    {
                      int control_sound = -1;
                      int w, h;
                      int old_size;

#ifdef DEBUG
                      float choice;
#endif

                      old_size = stamp_data[stamp_group][cur_stamp[stamp_group]]->size;
                      w = CUR_STAMP_W;
                      h = CUR_STAMP_H;

                      stamp_data[stamp_group][cur_stamp[stamp_group]]->size = (((MAX_STAMP_SIZE - MIN_STAMP_SIZE + 1
                                                                                 /* +1 to address lack of ability to get back to max default stamp size (SF Bug #1668235 -bjk 2011.01.08) */
                                                                                ) * (event.button.x -
                                                                                     (WINDOW_WIDTH - 96))) / 96) +
                        MIN_STAMP_SIZE;

#ifdef DEBUG
                      printf("Old size = %d, Chose %0.4f, New size =%d\n", old_size, choice,
                             stamp_data[stamp_group][cur_stamp[stamp_group]]->size);
#endif

                      if (stamp_data[stamp_group][cur_stamp[stamp_group]]->size != old_size)
                        {
                          if (stamp_xored)
                            {
                              stamp_xor(canvas->w / 2, canvas->h / 2);
                              stamp_xored = 0;

                              update_screen(canvas->w / 2 - (w + 1) / 2 + r_canvas.x,
                                            canvas->h / 2 - (h + 1) / 2 + r_canvas.y,
                                            canvas->w / 2 + (w + 1) / 2 + r_canvas.x,
                                            canvas->h / 2 + (h + 1) / 2 + r_canvas.y);
                            }

                          update_stamp_xor();
                          stamp_xor(canvas->w / 2, canvas->h / 2);
                          stamp_xored = 1;
                          update_screen(canvas->w / 2 - (CUR_STAMP_W + 1) / 2 + r_canvas.x,
                                        canvas->h / 2 - (CUR_STAMP_H + 1) / 2 + r_canvas.y,
                                        canvas->w / 2 + (CUR_STAMP_W + 1) / 2 + r_canvas.x,
                                        canvas->h / 2 + (CUR_STAMP_H + 1) / 2 + r_canvas.y);
                        }

                      if (stamp_data[stamp_group][cur_stamp[stamp_group]]->size < old_size)
                        control_sound = SND_SHRINK;
                      else if (stamp_data[stamp_group][cur_stamp[stamp_group]]->size > old_size)
                        control_sound = SND_GROW;

                      if (control_sound)
                        {
                          playsound(screen, 0, control_sound, 0, SNDPOS_CENTER, SNDDIST_NEAR);
                          draw_stamps();
                          update_screen_rect(&r_toolopt);
                        }
                    }
                }
              else if (cur_tool == TOOL_SHAPES && shape_tool_mode == SHAPE_TOOL_MODE_ROTATE)
                {
                  do_shape(shape_ctr_x, shape_ctr_y,
                           shape_outer_x, shape_outer_y, shape_rotation(shape_ctr_x, shape_ctr_y, old_x, old_y), 0);


                  do_shape(shape_ctr_x, shape_ctr_y,
                           shape_outer_x, shape_outer_y, shape_rotation(shape_ctr_x, shape_ctr_y, new_x, new_y), 0);


                  /* FIXME: Do something less intensive! */
                  SDL_Flip(screen);
                }

              old_x = new_x;
              old_y = new_y;
              oldpos_x = event.button.x;
              oldpos_y = event.button.y;
            }
        }

      if (cur_tool == TOOL_TEXT || (cur_tool == TOOL_LABEL && cur_label != LABEL_SELECT))
        {
          /* if (onscreen_keyboard) */
          /*   osk_clicked(kbd, old_x, old_y); */
          /* on_screen_keyboardd(); */
          cur_cursor_blink = SDL_GetTicks();

          if (cursor_x != -1 && cursor_y != -1 && cur_cursor_blink > last_cursor_blink + CURSOR_BLINK_SPEED)
            {
              last_cursor_blink = SDL_GetTicks();
              draw_blinking_cursor();
            }
        }

      if (cur_tool != TOOL_TEXT && cur_tool != TOOL_LABEL)
        {
          if (onscreen_keyboard && !kbd)
            {
              SDL_StopTextInput();
            }
        }

      if (motioner | hatmotioner)
        handle_motioners(oldpos_x, oldpos_y, motioner, hatmotioner, old_hat_ticks, val_x, val_y, valhat_x, valhat_y);


      SDL_Delay(1);
    }
  while (!done);
}

/* Draw using the text entry cursor/caret: */
static void hide_blinking_cursor(void)
{
  if (cur_toggle_count & 1)
    {
      draw_blinking_cursor();
    }
}

static void draw_blinking_cursor(void)
{
  cur_toggle_count++;

  line_xor(cursor_x + cursor_textwidth, cursor_y,
           cursor_x + cursor_textwidth, cursor_y + TuxPaint_Font_FontHeight(getfonthandle(cur_font)));

  update_screen(cursor_x + r_canvas.x + cursor_textwidth,
                cursor_y + r_canvas.y,
                cursor_x + r_canvas.x + cursor_textwidth,
                cursor_y + r_canvas.y + TuxPaint_Font_FontHeight(getfonthandle(cur_font)));
}

/* Draw using the current brush: */

static void brush_draw(int x1, int y1, int x2, int y2, int update)
{
  int dx, dy, y, frame_w, w, h;
  int orig_x1, orig_y1, orig_x2, orig_y2, tmp;
  int direction, r;
  float m, b;

  orig_x1 = x1;
  orig_y1 = y1;

  orig_x2 = x2;
  orig_y2 = y2;


  frame_w = img_brushes[cur_brush]->w / abs(brushes_frames[cur_brush]);
  w = frame_w / (brushes_directional[cur_brush] ? 3 : 1);
  h = img_brushes[cur_brush]->h / (brushes_directional[cur_brush] ? 3 : 1);

  x1 = x1 - (w >> 1);
  y1 = y1 - (h >> 1);

  x2 = x2 - (w >> 1);
  y2 = y2 - (h >> 1);


  direction = BRUSH_DIRECTION_NONE;
  if (brushes_directional[cur_brush])
    {
      r = brush_rotation(x1, y1, x2, y2) + 22;
      if (r < 0)
        r = r + 360;

      if (x1 != x2 || y1 != y2)
        direction = (r / 45);
    }


  dx = x2 - x1;
  dy = y2 - y1;

  if (dx != 0)
    {
      m = ((float)dy) / ((float)dx);
      b = y1 - m * x1;

      if (x2 >= x1)
        dx = 1;
      else
        dx = -1;


      while (x1 != x2)
        {
          y1 = m * x1 + b;
          y2 = m * (x1 + dx) + b;

          if (y1 > y2)
            {
              for (y = y1; y >= y2; y--)
                blit_brush(x1, y, direction);
            }
          else
            {
              for (y = y1; y <= y2; y++)
                blit_brush(x1, y, direction);
            }

          x1 = x1 + dx;
        }
    }
  else
    {
      if (y1 > y2)
        {
          y = y1;
          y1 = y2;
          y2 = y;
        }

      for (y = y1; y <= y2; y++)
        blit_brush(x1, y, direction);
    }

  if (orig_x1 > orig_x2)
    {
      tmp = orig_x1;
      orig_x1 = orig_x2;
      orig_x2 = tmp;
    }

  if (orig_y1 > orig_y2)
    {
      tmp = orig_y1;
      orig_y1 = orig_y2;
      orig_y2 = tmp;
    }


  if (update)
    {
      update_canvas(orig_x1 - (w >> 1), orig_y1 - (h >> 1), orig_x2 + (w >> 1), orig_y2 + (h >> 1));
    }
}

void reset_brush_counter(void)
{
  brush_counter = 999;
}


/* Draw the current brush in the current color: */

static void blit_brush(int x, int y, int direction)
{
  SDL_Rect src, dest;

  brush_counter++;

  if (brush_counter >= img_cur_brush_spacing)
    {
      brush_counter = 0;

      if (img_cur_brush_frames >= 0)
        {
          brush_frame++;
          if (brush_frame >= img_cur_brush_frames)
            brush_frame = 0;
        }
      else
        {
          int old_brush_frame = brush_frame;

          do
            {
              brush_frame = rand() % abs(img_cur_brush_frames);
            }
          while (brush_frame == old_brush_frame);
        }

      dest.x = x;
      dest.y = y;

      if (img_cur_brush_directional)
        {
          if (direction == BRUSH_DIRECTION_UP_LEFT ||
              direction == BRUSH_DIRECTION_UP || direction == BRUSH_DIRECTION_UP_RIGHT)
            {
              src.y = 0;
            }
          else if (direction == BRUSH_DIRECTION_LEFT ||
                   direction == BRUSH_DIRECTION_NONE || direction == BRUSH_DIRECTION_RIGHT)
            {
              src.y = img_cur_brush_h;
            }
          else if (direction == BRUSH_DIRECTION_DOWN_LEFT ||
                   direction == BRUSH_DIRECTION_DOWN || direction == BRUSH_DIRECTION_DOWN_RIGHT)
            {
              src.y = img_cur_brush_h << 1;
            }

          if (direction == BRUSH_DIRECTION_UP_LEFT ||
              direction == BRUSH_DIRECTION_LEFT || direction == BRUSH_DIRECTION_DOWN_LEFT)
            {
              src.x = brush_frame * img_cur_brush_frame_w;
            }
          else if (direction == BRUSH_DIRECTION_UP ||
                   direction == BRUSH_DIRECTION_NONE || direction == BRUSH_DIRECTION_DOWN)
            {
              src.x = brush_frame * img_cur_brush_frame_w + img_cur_brush_w;
            }
          else if (direction == BRUSH_DIRECTION_UP_RIGHT ||
                   direction == BRUSH_DIRECTION_RIGHT || direction == BRUSH_DIRECTION_DOWN_RIGHT)
            {
              src.x = brush_frame * img_cur_brush_frame_w + (img_cur_brush_w << 1);
            }
        }
      else
        {
          src.x = brush_frame * img_cur_brush_w;
          src.y = 0;
        }

      src.w = img_cur_brush_w;
      src.h = img_cur_brush_h;

      SDL_BlitSurface(img_cur_brush, &src, canvas, &dest);
    }
}


/* stamp tinter */

#define TINTER_ANYHUE 0         /* like normal, but remaps all hues in the stamp */
#define TINTER_NARROW 1         /* like normal, but narrow hue angle */
#define TINTER_NORMAL 2         /* normal */
#define TINTER_VECTOR 3         /* map black->white to black->destination */


typedef struct multichan
{
  double L, hue, sat;           /* L,a,b would be better -- 2-way formula unknown */
  unsigned char or, og, ob, alpha;      /* old 8-bit values */
} multichan;

#define X0 ((double)0.9505)
#define Y0 ((double)1.0000)
#define Z0 ((double)1.0890)
#define u0_prime ( (4.0 * X0) / (X0 + 15.0*Y0 + 3.0*Z0) )
#define v0_prime ( (9.0 * Y0) / (X0 + 15.0*Y0 + 3.0*Z0) )


static void fill_multichan(multichan * mc, double *up, double *vp)
{
  double X, Y, Z, u, v;
  double u_prime, v_prime;      /* temp, part of official formula */
  double Y_norm, fract;         /* severely temp */

  double r = sRGB_to_linear_table[mc->or];
  double g = sRGB_to_linear_table[mc->og];
  double b = sRGB_to_linear_table[mc->ob];

  /* coordinate change, RGB --> XYZ */
  X = 0.4124 * r + 0.3576 * g + 0.1805 * b;
  Y = 0.2126 * r + 0.7152 * g + 0.0722 * b;
  Z = 0.0193 * r + 0.1192 * g + 0.9505 * b;

  /* XYZ --> Luv */
  Y_norm = Y / Y0;
  fract = 1.0 / (X + 15.0 * Y + 3.0 * Z);
  u_prime = 4.0 * X * fract;
  v_prime = 9.0 * Y * fract;
  mc->L = (Y_norm > 0.008856) ? 116.0 * pow(Y_norm, 1.0 / 3.0) - 16.0 : 903.3 * Y_norm;
  u = 13.0 * mc->L * (u_prime - u0_prime);
  v = 13.0 * mc->L * (v_prime - v0_prime);

  mc->sat = sqrt(u * u + v * v);
  mc->hue = atan2(u, v);
  if (up)
    *up = u;
  if (vp)
    *vp = v;
}


static double tint_part_1(multichan * work, SDL_Surface * in)
{
  int xx, yy;
  double u_total = 0;
  double v_total = 0;
  double u, v;

  Uint32(*getpixel) (SDL_Surface *, int, int) = getpixels[in->format->BytesPerPixel];


  SDL_LockSurface(in);
  for (yy = 0; yy < in->h; yy++)
    {
      for (xx = 0; xx < in->w; xx++)
        {
          multichan *mc = work + yy * in->w + xx;

          /* put pixels into a more tolerable form */
          SDL_GetRGBA(getpixel(in, xx, yy), in->format, &mc->or, &mc->og, &mc->ob, &mc->alpha);

          fill_multichan(mc, &u, &v);

          /* average out u and v, giving more weight to opaque
             high-saturation pixels
             (this is to take an initial guess at the primary hue) */

          u_total += mc->alpha * u * mc->sat;
          v_total += mc->alpha * v * mc->sat;

        }
    }
  SDL_UnlockSurface(in);

#ifdef DEBUG
  fprintf(stderr, "u_total=%f\nv_total=%f\natan2()=%f\n", u_total, v_total, atan2(u_total, v_total));
#endif

  return atan2(u_total, v_total);
}


static void change_colors(SDL_Surface * out, multichan * work, double hue_range, multichan * key_color_ptr)
{
  double lower_hue_1, upper_hue_1, lower_hue_2, upper_hue_2;
  int xx, yy;
  multichan dst;
  double satratio;
  double slope;
  void (*putpixel) (SDL_Surface *, int, int, Uint32);
  double old_sat;
  double newsat;
  double L;
  double X, Y, Z;
  double u_prime, v_prime;      /* temp, part of official formula */
  unsigned tries;
  double u;
  double v;
  double r, g, b;


  /* prepare source and destination color info
     should reset hue_range or not? won't bother for now */
  multichan key_color = *key_color_ptr; /* want to work from a copy, for safety */

  lower_hue_1 = key_color.hue - hue_range;
  upper_hue_1 = key_color.hue + hue_range;
  if (lower_hue_1 < -M_PI)
    {
      lower_hue_2 = lower_hue_1 + 2 * M_PI;
      upper_hue_2 = upper_hue_1 + 2 * M_PI;
    }
  else
    {
      lower_hue_2 = lower_hue_1 - 2 * M_PI;
      upper_hue_2 = upper_hue_1 - 2 * M_PI;
    }

  /* get the destination color set up */
  dst.or = color_hexes[cur_color][0];
  dst.og = color_hexes[cur_color][1];
  dst.ob = color_hexes[cur_color][2];
  fill_multichan(&dst, NULL, NULL);

  satratio = dst.sat / key_color.sat;
  slope = (dst.L - key_color.L) / dst.sat;
  putpixel = putpixels[out->format->BytesPerPixel];

  SDL_LockSurface(out);
  for (yy = 0; yy < out->h; yy++)
    {
      for (xx = 0; xx < out->w; xx++)
        {
          multichan *mc = work + yy * out->w + xx;
          double oldhue = mc->hue;

          /* if not in the first range, and not in the second range, skip this one
             (really should alpha-blend as a function of hue angle difference) */
          if ((oldhue < lower_hue_1 || oldhue > upper_hue_1) && (oldhue < lower_hue_2 || oldhue > upper_hue_2))
            {
              putpixel(out, xx, yy, SDL_MapRGBA(out->format, mc->or, mc->og, mc->ob, mc->alpha));
              continue;
            }

          /* Modify the pixel */
          old_sat = mc->sat;
          newsat = old_sat * satratio;
          L = mc->L;
          if (dst.sat > 0)
            L += newsat * slope;        /* not greyscale destination */
          else
            L += old_sat * (dst.L - key_color.L) / key_color.sat;

          /* convert from L,u,v all the way back to sRGB with 8-bit channels */
          tries = 3;
        trysat:;
          u = newsat * sin(dst.hue);
          v = newsat * cos(dst.hue);

          /* Luv to XYZ */
          u_prime = u / (13.0 * L) + u0_prime;
          v_prime = v / (13.0 * L) + v0_prime;
          Y = (L > 7.99959199307) ? Y0 * pow((L + 16.0) / 116.0, 3.0) : Y0 * L / 903.3;
          X = 2.25 * Y * u_prime / v_prime;
          Z = (3.0 * Y - 0.75 * Y * u_prime) / v_prime - 5.0 * Y;

          /* coordinate change: XYZ to RGB */
          r = 3.2410 * X + -1.5374 * Y + -0.4986 * Z;
          g = -0.9692 * X + 1.8760 * Y + 0.0416 * Z;
          b = 0.0556 * X + -0.2040 * Y + 1.0570 * Z;

          /* If it is out of gamut, try to de-saturate it a few times before truncating.
             (the linear_to_sRGB function will truncate) */
          if ((r <= -0.5 || g <= -0.5 || b <= -0.5 || r >= 255.5 || g >= 255.5 || b >= 255.5) && tries--)
            {
              newsat *= 0.8;
              goto trysat;
            }

          putpixel(out, xx, yy,
                   SDL_MapRGBA(out->format, linear_to_sRGB(r), linear_to_sRGB(g), linear_to_sRGB(b), mc->alpha));
        }
    }
  SDL_UnlockSurface(out);
}


static multichan *find_most_saturated(double initial_hue, multichan * work, unsigned num, double *hue_range_ptr)
{
  /* find the most saturated pixel near the initial hue guess */
  multichan *key_color_ptr = NULL;
  double hue_range;
  unsigned i;
  double max_sat;
  double lower_hue_1;
  double upper_hue_1;
  double lower_hue_2;
  double upper_hue_2;
  multichan *mc;

  switch (stamp_data[stamp_group][cur_stamp[stamp_group]]->tinter)
    {
    default:
    case TINTER_NORMAL:
      hue_range = 18 * M_PI / 180.0;    /* plus or minus 18 degrees search, 27 replace */
      break;
    case TINTER_NARROW:
      hue_range = 6 * M_PI / 180.0;     /* plus or minus 6 degrees search, 9 replace */
      break;
    case TINTER_ANYHUE:
      hue_range = M_PI;         /* plus or minus 180 degrees */
      break;
    }

hue_range_retry:;

  max_sat = 0;
  lower_hue_1 = initial_hue - hue_range;
  upper_hue_1 = initial_hue + hue_range;

  if (lower_hue_1 < -M_PI)
    {
      lower_hue_2 = lower_hue_1 + 2 * M_PI;
      upper_hue_2 = upper_hue_1 + 2 * M_PI;
    }
  else
    {
      lower_hue_2 = lower_hue_1 - 2 * M_PI;
      upper_hue_2 = upper_hue_1 - 2 * M_PI;
    }

  i = num;
  while (i--)
    {
      mc = work + i;

      /* if not in the first range, and not in the second range, skip this one */
      if ((mc->hue < lower_hue_1 || mc->hue > upper_hue_1) && (mc->hue < lower_hue_2 || mc->hue > upper_hue_2))
        continue;

      if (mc->sat > max_sat)
        {
          max_sat = mc->sat;
          key_color_ptr = mc;
        }
    }

  if (!key_color_ptr)
    {
      hue_range *= 1.5;

      if (hue_range < M_PI)
        goto hue_range_retry;
    }

  *hue_range_ptr = hue_range;

  return key_color_ptr;
}


static void vector_tint_surface(SDL_Surface * out, SDL_Surface * in)
{
  int xx, yy;

  Uint32(*getpixel) (SDL_Surface *, int, int) = getpixels[in->format->BytesPerPixel];
  void (*putpixel) (SDL_Surface *, int, int, Uint32) = putpixels[out->format->BytesPerPixel];

  double r = sRGB_to_linear_table[color_hexes[cur_color][0]];
  double g = sRGB_to_linear_table[color_hexes[cur_color][1]];
  double b = sRGB_to_linear_table[color_hexes[cur_color][2]];

  SDL_LockSurface(in);
  for (yy = 0; yy < in->h; yy++)
    {
      for (xx = 0; xx < in->w; xx++)
        {
          unsigned char r8, g8, b8, a8;
          double old;

          SDL_GetRGBA(getpixel(in, xx, yy), in->format, &r8, &g8, &b8, &a8);
          /* get the linear greyscale value */
          old =
            sRGB_to_linear_table[r8] * 0.2126 + sRGB_to_linear_table[g8] * 0.7152 + sRGB_to_linear_table[b8] * 0.0722;

          putpixel(out, xx, yy,
                   SDL_MapRGBA(out->format, linear_to_sRGB(r * old),
                               linear_to_sRGB(g * old), linear_to_sRGB(b * old), a8));
        }
    }
  SDL_UnlockSurface(in);
}


static void tint_surface(SDL_Surface * tmp_surf, SDL_Surface * surf_ptr)
{
  unsigned width = surf_ptr->w;
  unsigned height = surf_ptr->h;
  multichan *work;
  multichan *key_color_ptr;
  double initial_hue;
  double hue_range;


  work = malloc(sizeof(multichan) * width * height);

  if (work)
    {
      initial_hue = tint_part_1(work, surf_ptr);

#ifdef DEBUG
      printf("initial_hue = %f\n", initial_hue);
#endif

      key_color_ptr = find_most_saturated(initial_hue, work, width * height, &hue_range);

#ifdef DEBUG
      printf("key_color_ptr = %d\n", (int)(intptr_t) key_color_ptr);    //EP added (intptr_t) to avoid warning on x64
#endif

      if (key_color_ptr)
        {
          /* wider for processing than for searching */
          hue_range *= 1.5;

          change_colors(tmp_surf, work, hue_range, key_color_ptr);

          free(work);
          return;
        }
      else
        {
          fprintf(stderr, "find_most_saturated() failed\n");
        }

      free(work);
    }

  /* Failed!  Fall back: */

  fprintf(stderr, "Falling back to tinter=vector, " "this should be in the *.dat file\n");

  vector_tint_surface(tmp_surf, surf_ptr);
}



/* Draw using the current stamp: */

static void stamp_draw(int x, int y)
{
  SDL_Rect dest;
  SDL_Surface *tmp_surf, *surf_ptr, *final_surf;
  Uint32 amask;
  Uint8 r, g, b, a;
  int xx, yy, dont_free_tmp_surf, base_x, base_y;

  Uint32(*getpixel) (SDL_Surface *, int, int);
  void (*putpixel) (SDL_Surface *, int, int, Uint32);

  surf_ptr = active_stamp;

  getpixel = getpixels[surf_ptr->format->BytesPerPixel];


  /* Create a temp surface to play with: */

  if (stamp_colorable(cur_stamp[stamp_group]) || stamp_tintable(cur_stamp[stamp_group]))
    {
      amask = ~(surf_ptr->format->Rmask | surf_ptr->format->Gmask | surf_ptr->format->Bmask);

      tmp_surf =
        SDL_CreateRGBSurface(SDL_SWSURFACE,
                             surf_ptr->w,
                             surf_ptr->h,
                             surf_ptr->format->BitsPerPixel,
                             surf_ptr->format->Rmask, surf_ptr->format->Gmask, surf_ptr->format->Bmask, amask);

      if (tmp_surf == NULL)
        {
          fprintf(stderr, "\nError: Can't render the colored stamp!\n"
                  "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

          cleanup();
          exit(1);
        }

      dont_free_tmp_surf = 0;
    }
  else
    {
      /* Not altering color; no need to create temp surf if we don't use it! */

      tmp_surf = NULL;
      dont_free_tmp_surf = 1;
    }

  if (tmp_surf != NULL)
    putpixel = putpixels[tmp_surf->format->BytesPerPixel];
  else
    putpixel = NULL;


  /* Alter the stamp's color, if needed: */

  if (stamp_colorable(cur_stamp[stamp_group]) && tmp_surf != NULL)
    {
      /* Render the stamp in the chosen color: */

      /* FIXME: It sucks to render this EVERY TIME.  Why not just when
         they pick the color, or pick the stamp, like with brushes? */

      /* Render the stamp: */

      SDL_LockSurface(surf_ptr);
      SDL_LockSurface(tmp_surf);

      for (yy = 0; yy < surf_ptr->h; yy++)
        {
          for (xx = 0; xx < surf_ptr->w; xx++)
            {
              SDL_GetRGBA(getpixel(surf_ptr, xx, yy), surf_ptr->format, &r, &g, &b, &a);

              putpixel(tmp_surf, xx, yy,
                       SDL_MapRGBA(tmp_surf->format,
                                   color_hexes[cur_color][0], color_hexes[cur_color][1], color_hexes[cur_color][2], a));
            }
        }

      SDL_UnlockSurface(tmp_surf);
      SDL_UnlockSurface(surf_ptr);
    }
  else if (stamp_tintable(cur_stamp[stamp_group]))
    {
      if (stamp_data[stamp_group][cur_stamp[stamp_group]]->tinter == TINTER_VECTOR)
        vector_tint_surface(tmp_surf, surf_ptr);
      else
        tint_surface(tmp_surf, surf_ptr);
    }
  else
    {
      /* No color change, just use it! */
      tmp_surf = surf_ptr;
    }

  /* Shrink or grow it! */
  final_surf = thumbnail(tmp_surf, CUR_STAMP_W, CUR_STAMP_H, 0);

  /* Where it will go? */
  base_x = x - (CUR_STAMP_W + 1) / 2;
  base_y = y - (CUR_STAMP_H + 1) / 2;

  /* And blit it! */
  dest.x = base_x;
  dest.y = base_y;
  SDL_BlitSurface(final_surf, NULL, canvas, &dest);     /* FIXME: Conditional jump or move depends on uninitialised value(s) */

  update_canvas(x - (CUR_STAMP_W + 1) / 2,
                y - (CUR_STAMP_H + 1) / 2, x + (CUR_STAMP_W + 1) / 2, y + (CUR_STAMP_H + 1) / 2);

  /* Free the temporary surfaces */

  if (!dont_free_tmp_surf)
    SDL_FreeSurface(tmp_surf);

  SDL_FreeSurface(final_surf);
}


/* Store canvas or label into undo buffer: */

static void rec_undo_buffer(void)
{
  int wanna_update_toolbar;

  wanna_update_toolbar = 0;

  rec_undo_label();

  SDL_BlitSurface(canvas, NULL, undo_bufs[cur_undo], NULL);
  undo_starters[cur_undo] = UNDO_STARTER_NONE;

  cur_undo = (cur_undo + 1) % NUM_UNDO_BUFS;

  if (cur_undo == oldest_undo)
    oldest_undo = (oldest_undo + 1) % NUM_UNDO_BUFS;

  newest_undo = cur_undo;

#ifdef DEBUG
  printf("DRAW: Current=%d  Oldest=%d  Newest=%d\n", cur_undo, oldest_undo, newest_undo);
#endif


  /* Update toolbar buttons, if needed: */

  if (tool_avail[TOOL_UNDO] == 0)
    {
      tool_avail[TOOL_UNDO] = 1;
      wanna_update_toolbar = 1;
    }

  if (tool_avail[TOOL_REDO])
    {
      tool_avail[TOOL_REDO] = 0;
      wanna_update_toolbar = 1;
    }

  if (wanna_update_toolbar)
    {
      draw_toolbar();
      update_screen_rect(&r_tools);
    }
}


/* Show program version: */
void show_version(int details)
{
  printf("\nTux Paint\n");
  printf("  Version " VER_VERSION " (" VER_DATE ")\n");


  if (details == 0)
    return;


  printf("\nBuilt with these options:\n");


  /* Quality reductions: */

#ifdef LOW_QUALITY_THUMBNAILS
  printf("  Low Quality Thumbnails enabled  (LOW_QUALITY_THUMBNAILS)\n");
#endif

#ifdef LOW_QUALITY_COLOR_SELECTOR
  printf("  Low Quality Color Selector enabled  (LOW_QUALITY_COLOR_SELECTOR)\n");
#endif

#ifdef LOW_QUALITY_STAMP_OUTLINE
  printf("  Low Quality Stamp Outline enabled  (LOW_QUALITY_STAMP_OUTLINE)\n");
#endif

#ifdef NO_PROMPT_SHADOWS
  printf("  Prompt Shadows disabled  (NO_PROMPT_SHADOWS)\n");
#endif

#ifdef SMALL_CURSOR_SHAPES
  printf("  Small cursor shapes enabled  (SMALL_CURSOR_SHAPES)\n");
#endif

#ifdef NO_BILINEAR
  printf("  Bilinear scaling disabled  (NO_BILINEAR)\n");
#endif

#ifdef NOSVG
  printf("  SVG support disabled  (NOSVG)\n");
#endif

  /* Sound: */

#ifdef NOSOUND
  printf("  Sound disabled  (NOSOUND)\n");
#endif


  /* Platform */

#ifdef __APPLE__
  printf("  Built for Mac OS X  (__APPLE__)\n");
#elif WIN32
  printf("  Built for Windows  (WIN32)\n");
#elif __BEOS__
  printf("  Built for BeOS  (__BEOS__)\n");
#elif __HAIKU__
  printf("  Built for Haiku (__HAIKU__)\n");
#elif NOKIA_770
  printf("  Built for Maemo  (NOKIA_770)\n");
#elif OLPC_XO
  printf("  Built for XO  (OLPC_XO)\n");
#elif __ANDROID__
  printf("  Built for Android  (__ANDROID__)\n");
#else
  printf("  Built for POSIX\n");
#endif


  /* Video options */

#ifdef USE_HWSURFACE
  printf("  Using hardware surface  (USE_HWSURFACE)\n");
#else
  printf("  Using software surface  (no USE_HWSURFACE)\n");
#endif
  printf("  Using %dbpp video  (VIDEO_BPP=%d)\n", VIDEO_BPP, VIDEO_BPP);


  /* Print method */

#ifdef PRINTMETHOD_PNG_PNM_PS
  printf("  Prints as PNGs  (PRINTMETHOD_PNG_PNM_PS)\n");
#endif

#ifdef PRINTMETHOD_PS
  printf("  Prints as PostScript  (PRINTMETHOD_PS)\n");
#endif


  /* Threading */

#ifdef FORKED_FONTS
  printf("  Threaded font loader enabled  (FORKED_FONTS)\n");
#else
  printf("  Threaded font loader disabled  (no FORKED_FONTS)\n");
#endif


  /* Old code used */

#ifdef OLD_STAMP_GROW_SHRINK
  printf("  Old-style stamp size UI  (OLD_STAMP_GROW_SHRINK)\n");
#endif

  printf("  Data directory (DATA_PREFIX) = %s\n", DATA_PREFIX);
  printf("  Plugin directory (MAGIC_PREFIX) = %s\n", MAGIC_PREFIX);
  printf("  Doc directory (DOC_PREFIX) = %s\n", DOC_PREFIX);
  printf("  Locale directory (LOCALEDIR) = %s\n", LOCALEDIR);
  printf("  Input Method directory (IMDIR) = %s\n", IMDIR);
  printf("  System config directory (CONFDIR) = %s\n", CONFDIR);


  /* Debugging */

#ifdef DEBUG
  printf("  Verbose debugging enabled  (DEBUG)\n");
#endif

#ifdef DEBUG_MALLOC
  printf("  Memory allocation debugging enabled  (DEBUG_MALLOC)\n");
#endif


  printf("\n");
}


/* Show usage display: */

void show_usage(int exitcode)
{
  FILE *f = exitcode ? stderr : stdout;
  char *blank;
  unsigned i;

  blank = strdup(progname);

  for (i = 0; i < strlen(blank); i++)
    blank[i] = ' ';

  fprintf(f,
          "\n"
          "Usage: %s {--usage | --help | --version | --verbose-version | --copying}\n"
          "\n"
          "  %s [--windowed | --fullscreen]\n"
          "  %s [--WIDTHxHEIGHT | --native]\n"
          "  %s [--disablescreensaver | --allowscreensaver ]\n"
          "  %s [--orient=landscape | --orient=portrait]\n"
          "  %s [--startblank | --startlast]\n"
          "  %s [--sound | --nosound]\n"
          "  %s [--quit | --noquit]\n"
          "  %s [--print | --noprint]\n"
          "  %s [--complexshapes | --simpleshapes]\n"
          "  %s [--mixedcase | --uppercase]\n"
          "  %s [--fancycursors | --nofancycursors]\n"
          "  %s [--hidecursor | --showcursor]\n"
          "  %s [--mouse | --keyboard]\n"
          "  %s [--dontgrab | --grab]\n"
          "  %s [--noshortcuts | --shortcuts]\n"
          "  %s [--wheelmouse | --nowheelmouse]\n"
          "  %s [--nobuttondistinction | --buttondistinction]\n"
          "  %s [--outlines | --nooutlines]\n"
          "  %s [--stamps | --nostamps]\n"
          "  %s [--sysfonts | --nosysfonts]\n"
          "  %s [--nostampcontrols | --stampcontrols]\n"
          "  %s [--nomagiccontrols | --magiccontrols]\n"
          "  %s [--nolabel | --label]\n"
          "  %s [--mirrorstamps | --dontmirrorstamps]\n"
          "  %s [--stampsize=[0-10] | --stampsize=default]\n"
          "  %s [--saveoverask | --saveover | --saveovernew]\n"
          "  %s [--nosave | --save]\n"
          "  %s [--autosave | --noautosave]\n" "  %s [--savedir DIRECTORY]\n" "  %s [--datadir DIRECTORY]\n"
#if defined(WIN32) || defined(__APPLE__)
          "  %s [--printcfg | --noprintcfg]\n"
#endif
          "  %s [--printdelay=SECONDS]\n" "  %s [--altprintmod | --altprintalways | --altprintnever]\n"
#if !defined(WIN32) && !defined(__APPLE__) && !defined(__BEOS__) && !defined(__HAIKU__) && !defined(__ANDROID__)
          "  %s [--papersize PAPERSIZE | --papersize help]\n"
#endif
          "  %s [--lang LANGUAGE | --locale LOCALE | --lang help]\n"
          "  %s [--nosysconfig]\n"
          "  %s [--nolockfile]\n"
          "  %s [--colorfile FILE]\n"
          "  %s [--mouse-accessibility]\n"
          "  %s [--onscreen-keyboard]\n"
          "  %s [--joystick-dev N] (default=0)\n"
          "  %s [--joystick-slowness N] (0-500; default value is 15)\n"
          "  %s [--joystick-threshold N] (0-32766; default value is 3200)\n"
          "  %s [--joystick-maxsteps N] (1-7; default value is 7)\n"
          "\n",
          progname, progname,
          blank, blank, blank, blank,
          blank, blank, blank, blank,
          blank, blank, blank, blank,
          blank, blank, blank, blank,
          blank, blank, blank, blank, blank, blank, blank, blank, blank, blank, blank, blank, blank,
#ifdef WIN32
          blank,
#endif
          blank, blank,
#if !defined(WIN32) && !defined(__APPLE__) && !defined(__BEOS__) && !defined(__HAIKU__) && !defined(__ANDROID__)
          blank,
#endif
          blank, blank, blank, blank, blank, blank, blank, blank, blank, blank);

  free(blank);
}


/* The original Tux Paint canvas was 448x376. The canvas can be
   other sizes now, but many old stamps are sized for the small
   canvas. So, with larger canvases, we must choose a good scale
   factor to compensate. As the canvas size grows, the user will
   want a balance of "more stamps on the screen" and "stamps not
   getting tiny". This will calculate the needed scale factor. */
static unsigned compute_default_scale_factor(double ratio)
{
  double old_diag = sqrt(448 * 448 + 376 * 376);
  double new_diag = sqrt(canvas->w * canvas->w + canvas->h * canvas->h);
  double good_def = ratio * sqrt(new_diag / old_diag);
  double good_log = log(good_def);
  unsigned defsize = HARD_MAX_STAMP_SIZE;

  while (defsize > 0)
    {
      double this_err = good_log - log(scaletable[defsize].numer / (double)scaletable[defsize].denom);
      double next_err = good_log - log(scaletable[defsize - 1].numer / (double)scaletable[defsize - 1].denom);

      if (fabs(next_err) > fabs(this_err))
        break;
      defsize--;
    }
  return defsize;
}


/* directory walking... */

static void loadbrush_callback(SDL_Surface * screen,
                               SDL_Texture * texture,
                               SDL_Renderer * renderer,
                               const char *restrict const dir,
                               unsigned dirlen, tp_ftw_str * files, unsigned i, const char *restrict const locale)
{
  FILE *fi;
  char buf[64];
  int want_rand;

  (void)dirlen;
  (void)locale;


  qsort(files, i, sizeof *files, compare_ftw_str);
  while (i--)
    {
      show_progress_bar(screen);
      if (strcasestr(files[i].str, ".png"))
        {
          char fname[512];

          if (strcasecmp(files[i].str, SHAPE_BRUSH_NAME) == 0)
            shape_brush = num_brushes;

          snprintf(fname, sizeof fname, "%s/%s", dir, files[i].str);
          if (num_brushes == num_brushes_max)
            {
              num_brushes_max = num_brushes_max * 5 / 4 + 4;
              img_brushes = realloc(img_brushes, num_brushes_max * sizeof *img_brushes);
              brushes_frames = realloc(brushes_frames, num_brushes_max * sizeof(int));
              brushes_directional = realloc(brushes_directional, num_brushes_max * sizeof(short));
              brushes_spacing = realloc(brushes_spacing, num_brushes_max * sizeof(int));
            }
          img_brushes[num_brushes] = loadimage(fname);


          /* Load brush metadata, if any: */

          brushes_frames[num_brushes] = 1;
          brushes_directional[num_brushes] = 0;
          brushes_spacing[num_brushes] = img_brushes[num_brushes]->h / 4;

          strcpy(strcasestr(fname, ".png"), ".dat");
          fi = fopen(fname, "r");

          want_rand = 0;

          if (fi != NULL)
            {
              do
                {
                  if (fgets(buf, sizeof(buf), fi))
                    {
                      if (strstr(buf, "frames=") != NULL)
                        {
                          brushes_frames[num_brushes] = atoi(strstr(buf, "frames=") + 7);
                        }
                      else if (strstr(buf, "spacing=") != NULL)
                        {
                          brushes_spacing[num_brushes] = atoi(strstr(buf, "spacing=") + 8);
                        }
                      else if (strstr(buf, "directional") != NULL)
                        {
                          brushes_directional[num_brushes] = 1;
                        }
                      else if (strstr(buf, "random") != NULL)
                        {
                          want_rand = 1;
                        }
                    }
                }
              while (!feof(fi));
              fclose(fi);

              if (want_rand)
                brushes_frames[num_brushes] *= -1;
            }

          num_brushes++;
        }
      free(files[i].str);
    }
  free(files);
}



static void load_brush_dir(SDL_Surface * screen, const char *restrict const dir)
{
  char buf[TP_FTW_PATHSIZE];
  unsigned dirlen = strlen(dir);

  memcpy(buf, dir, dirlen);
  tp_ftw(screen, texture, renderer, buf, dirlen, 0, loadbrush_callback, NULL);
}

SDL_Surface *mirror_surface(SDL_Surface * s)
{
  SDL_Surface *new_surf;
  int x;
  SDL_Rect src, dest;


  /* Mirror surface: */

  new_surf = duplicate_surface(s);
  SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_NONE);

  if (new_surf != NULL)
    {
      for (x = 0; x < s->w; x++)
        {
          src.x = x;
          src.y = 0;
          src.w = 1;
          src.h = s->h;

          dest.x = s->w - x - 1;
          dest.y = 0;

          SDL_BlitSurface(s, &src, new_surf, &dest);
        }

      SDL_FreeSurface(s);

      return (new_surf);
    }
  else
    {
      return (s);
    }
}

SDL_Surface *flip_surface(SDL_Surface * s)
{
  SDL_Surface *new_surf;
  int y;
  SDL_Rect src, dest;


  /* Flip surface: */

  new_surf = duplicate_surface(s);
  SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_NONE);

  if (new_surf != NULL)
    {
      for (y = 0; y < s->h; y++)
        {
          src.x = 0;
          src.y = y;
          src.w = s->w;
          src.h = 1;

          dest.x = 0;
          dest.y = s->h - y - 1;

          SDL_BlitSurface(s, &src, new_surf, &dest);
        }

      SDL_FreeSurface(s);

      return (new_surf);
    }
  else
    {
      return (s);
    }
}

static unsigned default_stamp_size;

static void loadstamp_finisher(stamp_type * sd, unsigned w, unsigned h, double ratio)
{
  unsigned int upper = HARD_MAX_STAMP_SIZE;
  unsigned int underscanned_upper = HARD_MAX_STAMP_SIZE;
  unsigned int lower = 0;
  unsigned mid;

#ifdef DEBUG
  printf("Finishing %s for %dx%d (ratio=%0.4f)\n", sd->stampname, w, h, ratio);
#endif

  /* If Tux Paint is in mirror-image-by-default mode, mirror, if we can: */
  if (mirrorstamps && sd->mirrorable)
    sd->mirrored = 1;

  do
    {
      scaleparams *s = &scaletable[upper];
      int pw, ph;               /* proposed width and height */

      pw = (w * s->numer + s->denom - 1) / s->denom;
      ph = (h * s->numer + s->denom - 1) / s->denom;

#ifdef ALLOW_STAMP_OVERSCAN
      /* OK to let a stamp stick off the sides in one direction, not two */
      /* By default, Tux Paint allowed stamps to be, at max, 2x as wide OR 2x as tall as canvas; scaled that back to 1.5 -bjk 2011.01.08 */
      if (pw < canvas->w * 1.5 && ph < canvas->h * 1)
        {
#ifdef DEBUG
          printf("Upper at %d with proposed size %dx%d (wide)\n", upper, pw, ph);
#endif
          if (pw > canvas->w)
            {
              underscanned_upper = upper - 1;
            }
          else
            {
              underscanned_upper = upper;
            }
          break;
        }
      if (pw < canvas->w * 1 && ph < canvas->h * 1.5)
        {
#ifdef DEBUG
          printf("Upper at %d with proposed size %dx%d (tall)\n", upper, pw, ph);
#endif
          if (ph > canvas->h)
            {
              underscanned_upper = upper - 1;
            }
          else
            {
              underscanned_upper = upper;
            }
          break;
        }
#else
      if (pw <= canvas->w * 1 && ph <= canvas->h * 1)
        {
#ifdef DEBUG
          printf("Upper at %d with proposed size %dx%d\n", upper, pw, ph);
#endif
          underscanned_upper = upper;
          break;
        }
#endif
    }
  while (--upper);


  do
    {
      scaleparams *s = &scaletable[lower];
      int pw, ph;               /* proposed width and height */

      pw = (w * s->numer + s->denom - 1) / s->denom;
      ph = (h * s->numer + s->denom - 1) / s->denom;

      if (pw * ph > 20)
        {
#ifdef DEBUG
          printf("Lower at %d with proposed size %dx%d\n", lower, pw, ph);
#endif
          break;
        }
    }
  while (++lower < HARD_MAX_STAMP_SIZE);


  if (upper < lower)
    {
      /* this, if it ever happens, is very bad */
      upper = (upper + lower) / 2;
      lower = upper;
    }

  mid = default_stamp_size;
  if (ratio != 1.0)
    mid = compute_default_scale_factor(ratio);

  /* Ratio override for SVGs! */
  if (ratio == 1.0 && sd->is_svg)
    {
      mid = compute_default_scale_factor(0.2);
    }

  if (mid > upper)
    mid = upper;

  if (mid > underscanned_upper)
    mid = underscanned_upper;

  if (mid < lower)
    mid = lower;

  sd->min = lower;
  sd->size = mid;
  sd->max = upper;

#ifdef DEBUG
  printf("Final min=%d, size=%d, max=%d\n", lower, mid, upper);
#endif

  if (stamp_size_override != -1)
    {
      sd->size = (((upper - lower) * stamp_size_override) / 10) + lower;
#ifdef DEBUG
      printf("...but adjusting size to %d\n", sd->size);
#endif
    }
#ifdef DEBUG
  printf("\n");
#endif
}


/* Note: must have read the *.dat file before calling this */
static void set_active_stamp(void)
{
  stamp_type *sd = stamp_data[stamp_group][cur_stamp[stamp_group]];
  unsigned len = strlen(sd->stampname);
  char *buf = alloca(len + strlen("_mirror_flip.EXT") + 1);
  int needs_mirror, needs_flip;

  if (active_stamp)
    SDL_FreeSurface(active_stamp);
  active_stamp = NULL;

  memcpy(buf, sd->stampname, len);

#ifdef DEBUG
  printf("\nset_active_stamp()\n");
#endif

  /* Look for pre-mirrored and pre-flipped version: */

  needs_mirror = sd->mirrored;
  needs_flip = sd->flipped;

  if (sd->mirrored && sd->flipped)
    {
      /* Want mirrored and flipped, both */

#ifdef DEBUG
      printf("want both mirrored & flipped\n");
#endif

      if (!sd->no_premirrorflip)
        {
#ifndef NOSVG
          memcpy(buf + len, "_mirror_flip.svg", 17);
          active_stamp = do_loadimage(buf, 0);
#endif

          if (active_stamp == NULL)
            {
              memcpy(buf + len, "_mirror_flip.png", 17);
              active_stamp = do_loadimage(buf, 0);
            }
        }


      if (active_stamp != NULL)
        {
#ifdef DEBUG
          printf("found a _mirror_flip!\n");
#endif

          needs_mirror = 0;
          needs_flip = 0;
        }
      else
        {
          /* Couldn't get one that was both, look for _mirror then _flip and
             flip or mirror it: */

#ifdef DEBUG
          printf("didn't find a _mirror_flip\n");
#endif

          if (!sd->no_premirror)
            {
#ifndef NOSVG
              memcpy(buf + len, "_mirror.svg", 12);
              active_stamp = do_loadimage(buf, 0);
#endif

              if (active_stamp == NULL)
                {
                  memcpy(buf + len, "_mirror.png", 12);
                  active_stamp = do_loadimage(buf, 0);
                }
            }

          if (active_stamp != NULL)
            {
#ifdef DEBUG
              printf("found a _mirror!\n");
#endif
              needs_mirror = 0;
            }
          else
            {
              /* Couldn't get one that was just pre-mirrored, look for a
                 pre-flipped */

#ifdef DEBUG
              printf("didn't find a _mirror, either\n");
#endif

              if (!sd->no_preflip)
                {
#ifndef NOSVG
                  memcpy(buf + len, "_flip.svg", 10);
                  active_stamp = do_loadimage(buf, 0);
#endif

                  if (active_stamp == NULL)
                    {
                      memcpy(buf + len, "_flip.png", 10);
                      active_stamp = do_loadimage(buf, 0);
                    }
                }

              if (active_stamp != NULL)
                {
#ifdef DEBUG
                  printf("found a _flip!\n");
#endif
                  needs_flip = 0;
                }
              else
                {
#ifdef DEBUG
                  printf("didn't find a _flip, either\n");
#endif
                }
            }
        }
    }
  else if (sd->flipped && !sd->no_preflip)
    {
      /* Want flipped only */

#ifdef DEBUG
      printf("want flipped only\n");
#endif

#ifndef NOSVG
      memcpy(buf + len, "_flip.svg", 10);
      active_stamp = do_loadimage(buf, 0);
#endif

      if (active_stamp == NULL)
        {
          memcpy(buf + len, "_flip.png", 10);
          active_stamp = do_loadimage(buf, 0);
        }

      if (active_stamp != NULL)
        {
#ifdef DEBUG
          printf("found a _flip!\n");
#endif
          needs_flip = 0;
        }
      else
        {
#ifdef DEBUG
          printf("didn't find a _flip\n");
#endif
        }
    }
  else if (sd->mirrored && !sd->no_premirror)
    {
      /* Want mirrored only */

#ifdef DEBUG
      printf("want mirrored only\n");
#endif

#ifndef NOSVG
      memcpy(buf + len, "_mirror.svg", 12);
      active_stamp = do_loadimage(buf, 0);
#endif

      if (active_stamp == NULL)
        {
          memcpy(buf + len, "_mirror.png", 12);
          active_stamp = do_loadimage(buf, 0);
        }

      if (active_stamp != NULL)
        {
#ifdef DEBUG
          printf("found a _mirror!\n");
#endif
          needs_mirror = 0;
        }
      else
        {
#ifdef DEBUG
          printf("didn't find a _mirror\n");
#endif
        }
    }


  /* Didn't want mirrored, or flipped, or couldn't load anything
     that was pre-rendered: */

  if (!active_stamp)
    {
#ifdef DEBUG
      printf("loading normal\n");
#endif

#ifndef NOSVG
      memcpy(buf + len, ".svg", 5);
      active_stamp = do_loadimage(buf, 0);
#endif

      if (active_stamp == NULL)
        {
          memcpy(buf + len, ".png", 5);
          active_stamp = do_loadimage(buf, 0);
        }

    }

  /* Never allow a NULL image! */

  if (!active_stamp)
    active_stamp = thumbnail(img_dead40x40, 40, 40, 1); /* copy it */


  /* If we wanted mirrored or flipped, and didn't get something pre-rendered,
     do it to the image we did load: */

  if (needs_mirror)
    {
#ifdef DEBUG
      printf("mirroring\n");
#endif
      active_stamp = mirror_surface(active_stamp);
    }

  if (needs_flip)
    {
#ifdef DEBUG
      printf("flipping\n");
#endif
      active_stamp = flip_surface(active_stamp);
    }

#ifdef DEBUG
  printf("\n\n");
#endif
}

static void get_stamp_thumb(stamp_type * sd)
{
  SDL_Surface *bigimg = NULL;
  unsigned len = strlen(sd->stampname);
  char *buf = alloca(len + strlen("_mirror_flip.EXT") + 1);
  int need_mirror, need_flip;
  double ratio;
  unsigned w;
  unsigned h;

#ifdef DEBUG
  printf("\nget_stamp_thumb()\n");
#endif

  memcpy(buf, sd->stampname, len);

  if (!sd->processed)
    {
      memcpy(buf + len, ".dat", 5);
      ratio = loadinfo(buf, sd);
    }
  else
    {
      /* So here, unless an SVG stamp has a .dat file with a 'scale',
         the Stamp ends up defaulting to 100% (ratio=1.0).
         Since we render the SVG as large as possible, for quality reasons,
         we almost never want the _default_ size to be 100%.

         So we need to either (a) keep track of the SVG's own pixel size
         and try to set the default size to something close to that,
         or (b) pick a universal initial size that we can apply to _all_ SVGs
         where the initial size is left unspecified (which means knowing when
         they're SVGs).

         So far, I'm doing (b), in loadstamp_finisher...

         -bjk 2009.09.29 */

      ratio = 1.0;
    }

  if (!sd->no_txt && !sd->stxt)
    {
      /* damn thing wants a .png extension; give it one */
      memcpy(buf + len, ".png", 5);
      sd->stxt = loaddesc(buf, &(sd->locale_text));
      sd->no_txt = !sd->stxt;
    }

#ifndef NOSOUND
  /* good time to load the sound */
  if (!sd->no_sound && !sd->ssnd && use_sound)
    {
      /* damn thing wants a .png extension; give it one */
      memcpy(buf + len, ".png", 5);
      sd->ssnd = loadsound(buf);
      sd->no_sound = !sd->ssnd;
    }

  /* ...and the description */
  if (!sd->no_descsound && !sd->sdesc && use_sound)
    {
      /* damn thing wants a .png extension; give it one */
      memcpy(buf + len, ".png", 5);
      sd->sdesc = loaddescsound(buf);
      sd->no_descsound = !sd->sdesc;
    }
#endif


  /* first see if we can re-use an existing thumbnail */
  if (sd->thumbnail)
    {
#ifdef DEBUG
      printf("have an sd->thumbnail\n");
#endif

      if (sd->thumb_mirrored_flipped == sd->flipped &&
          sd->thumb_mirrored_flipped == sd->mirrored &&
          sd->mirrored == sd->thumb_mirrored && sd->flipped == sd->thumb_flipped)
        {
          /* It's already the way we want */

#ifdef DEBUG
          printf("mirrored == flipped == thumb_mirrored_flipped [bye]\n");
#endif

          return;
        }
    }


  /* nope, see if there's a pre-rendered one we can use */

  need_mirror = sd->mirrored;
  need_flip = sd->flipped;
  bigimg = NULL;

  if (sd->mirrored && sd->flipped)
    {
#ifdef DEBUG
      printf("want mirrored & flipped\n");
#endif

      if (!sd->no_premirrorflip)
        {
          memcpy(buf + len, "_mirror_flip.png", 17);
          bigimg = do_loadimage(buf, 0);

#ifndef NOSVG
          if (bigimg == NULL)
            {
              memcpy(buf + len, "_mirror_flip.svg", 17);
              bigimg = do_loadimage(buf, 0);
            }
#endif
        }

      if (bigimg)
        {
#ifdef DEBUG
          printf("found a _mirror_flip!\n");
#endif

          need_mirror = 0;
          need_flip = 0;
        }
      else
        {
#ifdef DEBUG
          printf("didn't find a mirror_flip\n");
#endif
          sd->no_premirrorflip = 1;

          if (!sd->no_premirror)
            {
              memcpy(buf + len, "_mirror.png", 12);
              bigimg = do_loadimage(buf, 0);

#ifndef NOSVG
              if (bigimg == NULL)
                {
                  memcpy(buf + len, "_mirror.svg", 12);
                  bigimg = do_loadimage(buf, 0);
                }
#endif
            }

          if (bigimg)
            {
#ifdef DEBUG
              printf("found a _mirror\n");
#endif

              need_mirror = 0;
            }
          else
            {
#ifdef DEBUG
              printf("didn't find a mirror\n");
#endif

              if (!sd->no_preflip)
                {
                  memcpy(buf + len, "_flip.png", 10);
                  bigimg = do_loadimage(buf, 0);

#ifndef NOSVG
                  if (bigimg == NULL)
                    {
                      memcpy(buf + len, "_flip.svg", 10);
                      bigimg = do_loadimage(buf, 0);
                    }
#endif
                }

              if (bigimg)
                {
#ifdef DEBUG
                  printf("found a _flip\n");
#endif

                  need_flip = 0;
                }
            }
        }
    }
  else if (sd->mirrored && !sd->no_premirror)
    {
#ifdef DEBUG
      printf("want mirrored only\n");
#endif

      memcpy(buf + len, "_mirror.png", 12);
      bigimg = do_loadimage(buf, 0);

#ifndef NOSVG
      if (bigimg == NULL)
        {
          memcpy(buf + len, "_mirror.svg", 12);
          bigimg = do_loadimage(buf, 0);
        }
#endif

      if (bigimg)
        {
#ifdef DEBUG
          printf("found a _mirror!\n");
#endif
          need_mirror = 0;
        }
      else
        {
#ifdef DEBUG
          printf("didn't find a mirror\n");
#endif
          sd->no_premirror = 1;
        }
    }
  else if (sd->flipped && !sd->no_preflip)
    {
#ifdef DEBUG
      printf("want flipped only\n");
#endif

      memcpy(buf + len, "_flip.png", 10);
      bigimg = do_loadimage(buf, 0);

#ifndef NOSVG
      if (bigimg == NULL)
        {
          memcpy(buf + len, "_flip.svg", 10);
          bigimg = do_loadimage(buf, 0);
        }
#endif

      if (bigimg)
        {
#ifdef DEBUG
          printf("found a _flip!\n");
#endif
          need_flip = 0;
        }
      else
        {
#ifdef DEBUG
          printf("didn't find a flip\n");
#endif
          sd->no_preflip = 1;
        }
    }


  /* If we didn't load a pre-rendered, load the normal one: */

  if (!bigimg)
    {
#ifdef DEBUG
      printf("loading normal...\n");
#endif

      memcpy(buf + len, ".png", 5);
      bigimg = do_loadimage(buf, 0);

#ifndef NOSVG
      if (bigimg == NULL)
        {
          memcpy(buf + len, ".svg", 5);
          bigimg = do_loadimage(buf, 0);
        }
#endif
    }


  /* Scale the stamp down to its thumbnail size: */

  w = 40;
  h = 40;
  if (bigimg)
    {
      w = bigimg->w;
      h = bigimg->h;
    }

  if (!bigimg)
    sd->thumbnail = thumbnail(img_dead40x40, 40, 40, 1);        /* copy */
  else if (bigimg->w > 40 || bigimg->h > 40)
    {
      sd->thumbnail = thumbnail(bigimg, 40, 40, 1);
      SDL_FreeSurface(bigimg);
    }
  else
    sd->thumbnail = bigimg;


  /* Mirror and/or flip the thumbnail, if we still need to do so: */

  if (need_mirror)
    {
#ifdef DEBUG
      printf("mirroring\n");
#endif
      sd->thumbnail = mirror_surface(sd->thumbnail);
    }

  if (need_flip)
    {
#ifdef DEBUG
      printf("flipping\n");
#endif
      sd->thumbnail = flip_surface(sd->thumbnail);
    }


  /* Note the fact that the thumbnail's mirror/flip is the same as the main
     stamp: */

  if (sd->mirrored && sd->flipped)
    sd->thumb_mirrored_flipped = 1;
  else
    sd->thumb_mirrored_flipped = 0;

  sd->thumb_mirrored = sd->mirrored;
  sd->thumb_flipped = sd->flipped;

#ifdef DEBUG
  printf("\n\n");
#endif


  /* Finish up, if we need to: */

  if (sd->processed)
    return;

  sd->processed = 1;            /* not really, but on the next line... */
  loadstamp_finisher(sd, w, h, ratio);
}


static void loadstamp_callback(SDL_Surface * screen,
                               SDL_Texture * texture,
                               SDL_Renderer * renderer,
                               const char *restrict const dir,
                               unsigned dirlen, tp_ftw_str * files, unsigned i, const char *restrict const locale)
{
  (void)locale;
#ifdef DEBUG
  printf("loadstamp_callback: %s\n", dir);
#endif

  if (num_stamps[stamp_group] > 0)
    {
      /* If previous group had any stamps... */

      unsigned int i, slashcount;


      /* See if the current directory is shallow enough to be
         important for making a new stamp group: */

      slashcount = 0;

      for (i = strlen(load_stamp_basedir) + 1; i < strlen(dir); i++)
        {
          if (dir[i] == '/' || dir[i] == '\\')
            slashcount++;
        }

      if (slashcount <= stamp_group_dir_depth)
        {
          stamp_group++;
#ifdef DEBUG
          printf("\n...counts as a new group! now: %d\n", stamp_group);
#endif
        }
      else
        {
#ifdef DEBUG
          printf("...is still part of group %d\n", stamp_group);
#endif
        }
    }


  /* Sort and iterate the file list: */

  qsort(files, i, sizeof *files, compare_ftw_str);
  while (i--)
    {
      char fname[512];
      const char *dotext, *ext, *mirror_ext, *flip_ext, *mirrorflip_ext;

      ext = ".png";
      mirror_ext = "_mirror.png";
      flip_ext = "_flip.png";
      mirrorflip_ext = "_mirror_flip.png";
      dotext = (char *)strcasestr(files[i].str, ext);

#ifndef NOSVG
      if (dotext == NULL)
        {
          ext = ".svg";
          mirror_ext = "_mirror.svg";
          flip_ext = "_flip.svg";
          mirrorflip_ext = "_mirror_flip.svg";
          dotext = (char *)strcasestr(files[i].str, ext);
        }
      else
        {
          /* Found PNG, but we support SVG; let's see if there's an SVG
           *       version too, before loading the PNG */

          char svgname[512];
          FILE *fi;

          snprintf(svgname, sizeof(svgname), "%s/%s", dir, files[i].str);
          strcpy(strcasestr(svgname, ".png"), ".svg");

          fi = fopen(svgname, "r");
          if (fi != NULL)
            {
              debug("Found SVG version of ");
              debug(files[i].str);
              debug("\n");

              fclose(fi);
              continue;         /* ugh, i hate continues */
            }
        }
#endif

      show_progress_bar(screen);

      if (dotext > files[i].str && !strcasecmp(dotext, ext)
          && (dotext - files[i].str + 1 + dirlen < (int)(sizeof fname))
          && !strcasestr(files[i].str, mirror_ext)
          && !strcasestr(files[i].str, flip_ext) && !strcasestr(files[i].str, mirrorflip_ext))
        {
          snprintf(fname, sizeof fname, "%s/%s", dir, files[i].str);
          if (num_stamps[stamp_group] == max_stamps[stamp_group])
            {
              max_stamps[stamp_group] = max_stamps[stamp_group] * 5 / 4 + 15;
              stamp_data[stamp_group] = realloc(stamp_data[stamp_group],
                                                max_stamps[stamp_group] * sizeof(*stamp_data[stamp_group]));
            }
          stamp_data[stamp_group][num_stamps[stamp_group]] =
            calloc(1, sizeof *stamp_data[stamp_group][num_stamps[stamp_group]]);
          stamp_data[stamp_group][num_stamps[stamp_group]]->stampname = malloc(dotext - files[i].str + 1 + dirlen + 1);
          memcpy(stamp_data[stamp_group][num_stamps[stamp_group]]->stampname, fname,
                 dotext - files[i].str + 1 + dirlen);
          stamp_data[stamp_group][num_stamps[stamp_group]]->stampname[dotext - files[i].str + 1 + dirlen] = '\0';

          if (strcmp(ext, ".svg") == 0)
            {
              stamp_data[stamp_group][num_stamps[stamp_group]]->is_svg = 1;
            }
          else
            {
              stamp_data[stamp_group][num_stamps[stamp_group]]->is_svg = 0;
            }

          num_stamps[stamp_group]++;
        }
      free(files[i].str);
    }
  free(files);
}



static void load_stamp_dir(SDL_Surface * screen, const char *const dir)
{
  char buf[TP_FTW_PATHSIZE];
  unsigned dirlen = strlen(dir);

  memcpy(buf, dir, dirlen);
  load_stamp_basedir = dir;
  tp_ftw(screen, texture, renderer, buf, dirlen, 0, loadstamp_callback, NULL);
}


static void load_stamps(SDL_Surface * screen)
{
  char *homedirdir = get_fname("stamps", DIR_DATA);

  default_stamp_size = compute_default_scale_factor(1.0);

  load_stamp_dir(screen, homedirdir);
#ifndef __ANDROID__
  load_stamp_dir(screen, DATA_PREFIX "stamps");
#else
  load_stamp_dir(screen, ASSETS_STAMPS_DIR);
#endif
#ifdef __APPLE__
  load_stamp_dir(screen, "Resources/stamps");
  load_stamp_dir(screen, "/Library/Application Support/TuxPaint/stamps");
#endif
#ifdef WIN32
  free(homedirdir);
  homedirdir = get_fname("data/stamps", DIR_DATA);
  load_stamp_dir(screen, homedirdir);
#endif

  if (num_stamps[0] == 0)
    {
      fprintf(stderr, "\nWarning: No stamps found in " DATA_PREFIX "stamps/\n" "or %s\n\n", homedirdir);
    }

  num_stamp_groups = stamp_group + 1;

  free(homedirdir);
}

#ifndef FORKED_FONTS
static int load_user_fonts_stub(void *vp)
{
  return load_user_fonts(screen, texture, renderer, vp, NULL);
}
#endif

#ifndef NO_SDLPANGO
volatile long fontconfig_thread_done = 0;

int generate_fontconfig_cache_spinner(SDL_Surface * screen)
{
  SDL_Event event;

  while (fontconfig_thread_done == 0)
    {
      show_progress_bar(screen);
      SDL_Flip(screen);
      SDL_Delay(20);

      while (SDL_PollEvent(&event) > 0)
        {
          if (event.type == SDL_QUIT ||
              (event.type == SDL_KEYDOWN
               && (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_AC_BACK)))
            {
              printf("Aborting!\n");
              fflush(stdout);
              return (1);
            }
        }
    }
  return (0);
}

static int generate_fontconfig_cache_real(void)
{
  TuxPaint_Font *tmp_font;
  SDL_Surface *tmp_surf;
  SDL_Color black = { 0, 0, 0, 0 };

#ifdef DEBUG
  printf("-- Hello from generate_fontconfig_cache() (thread # %d)\n", SDL_ThreadID());
  fflush(stdout);
#endif

  tmp_font = TuxPaint_Font_OpenFont(PANGO_DEFAULT_FONT, NULL, 12);

  if (tmp_font != NULL)
    {
#ifdef DEBUG
      printf("-- Generated a font.\n");
      fflush(stdout);
#endif
      tmp_surf = render_text(tmp_font, "Test", black);
      if (tmp_surf != NULL)
        {
#ifdef DEBUG
          printf("-- Generated a surface\n");
          fflush(stdout);
#endif
          SDL_FreeSurface(tmp_surf);
        }
      else
        {
#ifdef DEBUG
          printf("-- Failed to make a surface!\n");
          fflush(stdout);
#endif
        }
      TuxPaint_Font_CloseFont(tmp_font);
    }
  else
    {
#ifdef DEBUG
      printf("-- Failed to generate a font!\n");
      fflush(stdout);
#endif
    }

  fontconfig_thread_done = 1;

#ifdef DEBUG
  printf("-- generate_fontconfig_cache() is done\n");
  fflush(stdout);
#endif
  return (0);
}

static int generate_fontconfig_cache(void *vp)
{
  return generate_fontconfig_cache_real();
}
#endif

#define hex2dec(c) (((c) >= '0' && (c) <= '9') ? ((c) - '0') : \
  ((c) >= 'A' && (c) <= 'F') ? ((c) - 'A' + 10) : \
  ((c) >= 'a' && (c) <= 'f') ? ((c) - 'a' + 10) : 0)

#ifndef WIN32
static void signal_handler(int sig)
{
  (void)sig;
  // It is not legal to call printf or most other functions here!
}
#endif

/* Render a button label using the appropriate string/font: */
static SDL_Surface *do_render_button_label(const char *const label)
{
  SDL_Surface *tmp_surf, *surf;
  SDL_Color black = { 0, 0, 0, 0 };
  TuxPaint_Font *myfont = small_font;
  char *td_str = textdir(gettext(label));
  char *upstr = uppercase(td_str);

  free(td_str);

  if (need_own_font && strcmp(gettext(label), label))
    myfont = locale_font;
  tmp_surf = render_text(myfont, upstr, black);
  free(upstr);
  surf = thumbnail(tmp_surf, min(48, tmp_surf->w), min(18 + button_label_y_nudge, tmp_surf->h), 0);
  SDL_FreeSurface(tmp_surf);

  return surf;
}

static void create_button_labels(void)
{
  int i;

  for (i = 0; i < NUM_TOOLS; i++)
    img_tool_names[i] = do_render_button_label(tool_names[i]);

  for (i = 0; i < num_magics; i++)
    magics[i].img_name = do_render_button_label(magics[i].name);

  for (i = 0; i < NUM_SHAPES; i++)
    img_shape_names[i] = do_render_button_label(shape_names[i]);

  /* buttons for the file open dialog */

  /* Open dialog: 'Open' button, to load the selected picture */
  img_openlabels_open = do_render_button_label(gettext_noop("Open"));

  /* Open dialog: 'Erase' button, to erase/deleted the selected picture */
  img_openlabels_erase = do_render_button_label(gettext_noop("Erase"));

  /* Open dialog: 'Slides' button, to switch to slide show mode */
  img_openlabels_slideshow = do_render_button_label(gettext_noop("Slides"));

  /* Open dialog: 'Back' button, to dismiss Open dialog without opening a picture */
  img_openlabels_back = do_render_button_label(gettext_noop("Back"));

  /* Slideshow: 'Next' button, to load next slide (image) */
  img_openlabels_next = do_render_button_label(gettext_noop("Next"));

  /* Slideshow: 'Play' button, to begin a slideshow sequence */
  img_openlabels_play = do_render_button_label(gettext_noop("Play"));
}


static void seticon(void)
{
#ifndef WIN32
  int masklen;
  Uint8 *mask;
#endif
  SDL_Surface *icon;

  /* Load icon into a surface: */

#ifndef WIN32
  icon = IMG_Load(DATA_PREFIX "images/icon.png");
#else
  icon = IMG_Load(DATA_PREFIX "images/icon32x32.png");
#endif

  if (icon == NULL)
    {
      fprintf(stderr,
              "\nWarning: I could not load the icon image: %s\n"
              "The Simple DirectMedia error that occurred was:\n"
              "%s\n\n", DATA_PREFIX "images/icon.png", SDL_GetError());
      return;
    }


#ifndef WIN32
  /* Create mask: */
  masklen = (((icon->w) + 7) / 8) * (icon->h);
  mask = malloc(masklen * sizeof(Uint8));
  memset(mask, 0xFF, masklen);

  /* Set icon: */
  //  SDL_WM_SetIcon(icon, mask);
  SDL_SetWindowIcon(window_screen, icon);
  /* Free icon surface & mask: */
  free(mask);
#else
  /* Set icon: */
  SDL_WM_SetIcon(icon, NULL);
#endif
  SDL_FreeSurface(icon);


  /* Grab keyboard and mouse, if requested: */

  if (grab_input)
    {
      debug("Grabbing input!");
      SDL_SetWindowGrab(window_screen, SDL_TRUE);
    }
}


/* Load a mouse pointer (cursor) shape: */

static SDL_Cursor *get_cursor(unsigned char *bits, unsigned char *mask_bits,
                              unsigned int width, unsigned int height, unsigned int x, unsigned int y)
{
  Uint8 b;
  Uint8 temp_bitmap[128], temp_bitmask[128];
  unsigned int i;


  if (((width + 7) / 8) * height > 128)
    {
      fprintf(stderr, "Error: Cursor is too large!\n");
      cleanup();
      exit(1);
    }

  for (i = 0; i < ((width + 7) / 8) * height; i++)
    {
      b = bits[i];

      temp_bitmap[i] = (((b & 0x01) << 7) |
                        ((b & 0x02) << 5) |
                        ((b & 0x04) << 3) |
                        ((b & 0x08) << 1) |
                        ((b & 0x10) >> 1) | ((b & 0x20) >> 3) | ((b & 0x40) >> 5) | ((b & 0x80) >> 7));

      b = mask_bits[i];

      temp_bitmask[i] = (((b & 0x01) << 7) |
                         ((b & 0x02) << 5) |
                         ((b & 0x04) << 3) |
                         ((b & 0x08) << 1) |
                         ((b & 0x10) >> 1) | ((b & 0x20) >> 3) | ((b & 0x40) >> 5) | ((b & 0x80) >> 7));
    }

  return (SDL_CreateCursor(temp_bitmap, temp_bitmask, width, height, x, y));
}


/* Load an image (with errors): */

static SDL_Surface *loadimage(const char *const fname)
{
  return (do_loadimage(fname, 1));
}


/* Load an image: */

static SDL_Surface *do_loadimage(const char *const fname, int abort_on_error)
{
  SDL_Surface *s, *disp_fmt_s;


  /* Load the image file: */

  s = myIMG_Load((char *)fname);
  if (s == NULL)
    {
      if (abort_on_error)
        {
          fprintf(stderr,
                  "\nError: I couldn't load a graphics file:\n"
                  "%s\n" "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", fname, SDL_GetError());

          cleanup();
          exit(1);
        }
      else
        {
          return (NULL);
        }
    }


  /* Convert to the display format: */

  disp_fmt_s = SDL_DisplayFormatAlpha(s);
  if (disp_fmt_s == NULL)
    {
      if (abort_on_error)
        {
          fprintf(stderr,
                  "\nError: I couldn't convert a graphics file:\n"
                  "%s\n" "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", fname, SDL_GetError());

          SDL_FreeSurface(s);
          cleanup();
          exit(1);
        }
      else
        {
          SDL_FreeSurface(s);
          return (NULL);
        }
    }


  /* Free the temp. surface & return the converted one: */

  SDL_FreeSurface(s);

  return (disp_fmt_s);
}


/* Draw the toolbar: */

static void draw_toolbar(void)
{
  int i, off_y, max, most, tool;
  SDL_Rect dest;

  most = 14;
  off_y = 0;
  /* FIXME: Only allow print if we have something to print! */


  draw_image_title(TITLE_TOOLS, r_ttools);



  /* Do we need scrollbars? */
  if (NUM_TOOLS > most + TOOLOFFSET)
    {
      off_y = 24;
      max = most - 2 + TOOLOFFSET;
      gd_tools.rows = max / 2;

      dest.x = 0;
      dest.y = 40;

      if (tool_scroll > 0)
        {
          SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);
        }

      dest.x = 0;
      dest.y = 40 + 24 + ((6 + TOOLOFFSET / 2) * 48);



      if (tool_scroll < NUM_TOOLS - (most - 2) - TOOLOFFSET)
        {
          SDL_BlitSurface(img_scroll_down, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_scroll_down_off, NULL, screen, &dest);
        }
    }
  else
    {
      off_y = 0;
      max = 14 + TOOLOFFSET;
    }




  for (tool = tool_scroll; tool < tool_scroll + max; tool++)
    {
      i = tool - tool_scroll;
      dest.x = ((i % 2) * 48);
      dest.y = ((i / 2) * 48) + 40 + off_y;


      if (tool < NUM_TOOLS)
        {
          SDL_Surface *button_color;
          SDL_Surface *button_body;

          if (tool_scroll + i == cur_tool)
            {
              button_body = img_btn_down;
              button_color = img_black;
            }
          else if (tool_avail[tool])
            {
              button_body = img_btn_up;
              button_color = img_black;
            }
          else
            {
              button_body = img_btn_off;
              button_color = img_grey;
            }
          SDL_BlitSurface(button_body, NULL, screen, &dest);
          SDL_BlitSurface(button_color, NULL, img_tools[tool], NULL);
          SDL_BlitSurface(button_color, NULL, img_tool_names[tool], NULL);

          dest.x = ((i % 2) * 48) + 4;
          dest.y = ((i / 2) * 48) + 40 + 2 + off_y;

          SDL_BlitSurface(img_tools[tool], NULL, screen, &dest);


          dest.x = ((i % 2) * 48) + 4 + (40 - img_tool_names[tool]->w) / 2;
          dest.y = ((i / 2) * 48) + 40 + 2 + (44 + button_label_y_nudge - img_tool_names[tool]->h) + off_y;

          SDL_BlitSurface(img_tool_names[tool], NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
        }
    }
}


/* Draw magic controls: */

static void draw_magic(void)
{
  int magic, i, max, off_y;
  SDL_Rect dest;
  int most;


  draw_image_title(TITLE_MAGIC, r_ttoolopt);

  /* How many can we show? */

  most = 12;
  if (disable_magic_controls)
    most = 14;

  if (num_magics > most + TOOLOFFSET)
    {
      off_y = 24;
      max = (most - 2) + TOOLOFFSET;

      dest.x = WINDOW_WIDTH - 96;
      dest.y = 40;

      if (magic_scroll > 0)
        {
          SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);
        }

      dest.x = WINDOW_WIDTH - 96;
      dest.y = 40 + 24 + ((((most - 2) / 2) + TOOLOFFSET / 2) * 48);

      if (magic_scroll < num_magics - (most - 2) - TOOLOFFSET)
        {
          SDL_BlitSurface(img_scroll_down, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_scroll_down_off, NULL, screen, &dest);
        }
    }
  else
    {
      off_y = 0;
      max = most + TOOLOFFSET;
    }


  for (magic = magic_scroll; magic < magic_scroll + max; magic++)
    {
      i = magic - magic_scroll;

      dest.x = ((i % 2) * 48) + (WINDOW_WIDTH - 96);
      dest.y = ((i / 2) * 48) + 40 + off_y;

      if (magic < num_magics)
        {
          if (magic == cur_magic)
            {
              SDL_BlitSurface(img_btn_down, NULL, screen, &dest);
            }
          else
            {
              SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
            }

          dest.x = WINDOW_WIDTH - 96 + ((i % 2) * 48) + 4;
          dest.y = ((i / 2) * 48) + 40 + 4 + off_y;

          SDL_BlitSurface(magics[magic].img_icon, NULL, screen, &dest);


          dest.x = WINDOW_WIDTH - 96 + ((i % 2) * 48) + 4 + (40 - magics[magic].img_name->w) / 2;
          dest.y = (((i / 2) * 48) + 40 + 4 + (44 - magics[magic].img_name->h) + off_y);

          SDL_BlitSurface(magics[magic].img_name, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
        }
    }


  /* Draw text controls: */

  if (!disable_magic_controls)
    {
      SDL_Surface *button_color;

      /* Show paint button: */

      if (magics[cur_magic].mode == MODE_PAINT || magics[cur_magic].mode == MODE_ONECLICK
          || magics[cur_magic].mode == MODE_PAINT_WITH_PREVIEW)
        button_color = img_btn_down;    /* Active */
      else if (magics[cur_magic].avail_modes & MODE_PAINT || magics[cur_magic].avail_modes & MODE_ONECLICK
               || magics[cur_magic].avail_modes & MODE_PAINT_WITH_PREVIEW)
        button_color = img_btn_up;      /* Available, but not active */
      else
        button_color = img_btn_off;     /* Unavailable */

      dest.x = WINDOW_WIDTH - 96;
      dest.y = 40 + ((6 + TOOLOFFSET / 2) * 48);

      SDL_BlitSurface(button_color, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - 96 + (48 - img_magic_paint->w) / 2;
      dest.y = (40 + ((6 + TOOLOFFSET / 2) * 48) + (48 - img_magic_paint->h) / 2);

      SDL_BlitSurface(img_magic_paint, NULL, screen, &dest);


      /* Show fullscreen button: */

      if (magics[cur_magic].mode == MODE_FULLSCREEN)
        button_color = img_btn_down;    /* Active */
      else if (magics[cur_magic].avail_modes & MODE_FULLSCREEN)
        button_color = img_btn_up;      /* Available, but not active */
      else
        button_color = img_btn_off;     /* Unavailable */

      dest.x = WINDOW_WIDTH - 48;
      dest.y = 40 + ((6 + TOOLOFFSET / 2) * 48);

      SDL_BlitSurface(button_color, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - 48 + (48 - img_magic_fullscreen->w) / 2;
      dest.y = (40 + ((6 + TOOLOFFSET / 2) * 48) + (48 - img_magic_fullscreen->h) / 2);

      SDL_BlitSurface(img_magic_fullscreen, NULL, screen, &dest);
    }
}


/* Draw color selector: */

static unsigned colors_state = COLORSEL_ENABLE | COLORSEL_CLOBBER;

static unsigned draw_colors(unsigned action)
{
  unsigned i;
  SDL_Rect dest;
  static unsigned old_color;
  unsigned old_colors_state;

  old_colors_state = colors_state;

  if (action == COLORSEL_CLOBBER || action == COLORSEL_CLOBBER_WIPE)
    colors_state |= COLORSEL_CLOBBER;
  else if (action == COLORSEL_REFRESH)
    colors_state &= ~COLORSEL_CLOBBER;
  else if (action == COLORSEL_DISABLE)
    colors_state = COLORSEL_DISABLE;
  else if (action == COLORSEL_ENABLE || action == COLORSEL_FORCE_REDRAW)
    colors_state = COLORSEL_ENABLE;

  colors_are_selectable = (colors_state == COLORSEL_ENABLE);

  if (colors_state & COLORSEL_CLOBBER && action != COLORSEL_CLOBBER_WIPE)
    return old_colors_state;

  if (cur_color == old_color && colors_state == old_colors_state &&
      action != COLORSEL_CLOBBER_WIPE && action != COLORSEL_FORCE_REDRAW)
    return old_colors_state;

  old_color = cur_color;

  for (i = 0; i < (unsigned int)NUM_COLORS; i++)
    {
      dest.x = r_colors.x + i % gd_colors.cols * color_button_w;
      dest.y = r_colors.y + i / gd_colors.cols * color_button_h;
#ifndef LOW_QUALITY_COLOR_SELECTOR
      SDL_BlitSurface((colors_state == COLORSEL_ENABLE)
                      ? img_color_btns[i + (i == cur_color) * NUM_COLORS] : img_color_btn_off, NULL, screen, &dest);
#else
      dest.w = color_button_w;
      dest.h = color_button_h;
      if (colors_state == COLORSEL_ENABLE)
        SDL_FillRect(screen, &dest,
                     SDL_MapRGB(screen->format, color_hexes[i][0], color_hexes[i][1], color_hexes[i][2]));
      else
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 240, 240, 240));

      if (i == cur_color && colors_state == COLORSEL_ENABLE)
        {
          dest.y += 4;
          SDL_BlitSurface(img_paintcan, NULL, screen, &dest);
        }
#endif

    }
  update_screen_rect(&r_colors);

  /* if only the color changed, no need to draw the title */
  if (colors_state == old_colors_state)
    return old_colors_state;

  if (colors_state == COLORSEL_ENABLE)
    {
      SDL_BlitSurface(img_title_large_on, NULL, screen, &r_tcolors);

      dest.x = r_tcolors.x + (r_tcolors.w - img_title_names[TITLE_COLORS]->w) / 2;
      dest.y = r_tcolors.y + (r_tcolors.h - img_title_names[TITLE_COLORS]->h) / 2;
      SDL_BlitSurface(img_title_names[TITLE_COLORS], NULL, screen, &dest);
    }
  else
    {
      SDL_BlitSurface(img_title_large_off, NULL, screen, &r_tcolors);
    }

  update_screen_rect(&r_tcolors);

  return old_colors_state;
}


/* Draw brushes: */

static void draw_brushes(void)
{
  int i, off_y, max, brush;
  SDL_Rect src, dest;


  /* Draw the title: */
  draw_image_title(TITLE_BRUSHES, r_ttoolopt);


  /* Do we need scrollbars? */

  if (num_brushes > 14 + TOOLOFFSET)
    {
      off_y = 24;
      max = 12 + TOOLOFFSET;

      dest.x = WINDOW_WIDTH - 96;
      dest.y = 40;

      if (brush_scroll > 0)
        {
          SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);
        }

      dest.x = WINDOW_WIDTH - 96;
      dest.y = 40 + 24 + ((6 + TOOLOFFSET / 2) * 48);

      if (brush_scroll < num_brushes - 12 - TOOLOFFSET)
        {
          SDL_BlitSurface(img_scroll_down, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_scroll_down_off, NULL, screen, &dest);
        }
    }
  else
    {
      off_y = 0;
      max = 14 + TOOLOFFSET;
    }


  /* Draw each of the shown brushes: */

  for (brush = brush_scroll; brush < brush_scroll + max; brush++)
    {
      i = brush - brush_scroll;


      dest.x = ((i % 2) * 48) + (WINDOW_WIDTH - 96);
      dest.y = ((i / 2) * 48) + 40 + off_y;

      if (brush == cur_brush)
        {
          SDL_BlitSurface(img_btn_down, NULL, screen, &dest);
        }
      else if (brush < num_brushes)
        {
          SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
        }

      if (brush < num_brushes)
        {
          if (brushes_directional[brush])
            src.x = (img_brushes[brush]->w / abs(brushes_frames[brush])) / 3;
          else
            src.x = 0;

          src.y = brushes_directional[brush] ? (img_brushes[brush]->h / 3) : 0;

          src.w = (img_brushes[brush]->w / abs(brushes_frames[brush])) / (brushes_directional[brush] ? 3 : 1);
          src.h = (img_brushes[brush]->h / (brushes_directional[brush] ? 3 : 1));

          dest.x = ((i % 2) * 48) + (WINDOW_WIDTH - 96) + ((48 - src.w) >> 1);
          dest.y = ((i / 2) * 48) + 40 + ((48 - src.h) >> 1) + off_y;

          SDL_BlitSurface(img_brushes[brush], &src, screen, &dest);
        }
    }
}


/* Draw fonts: */
static void draw_fonts(void)
{
  int i, off_y, max, font, most;
  SDL_Rect dest, src;
  SDL_Surface *tmp_surf;
  SDL_Color black = { 0, 0, 0, 0 };

  /* Draw the title: */
  draw_image_title(TITLE_LETTERS, r_ttoolopt);

  /* How many can we show? */

  if (cur_tool == TOOL_LABEL)
    {
      most = 8;
      if (disable_stamp_controls)
        most = 12;
    }
  else
    {
      most = 10;
      if (disable_stamp_controls)
        most = 14;
    }

#ifdef DEBUG
  printf("there are %d font families\n", num_font_families);
#endif


  /* Do we need scrollbars? */

  if (num_font_families > most + TOOLOFFSET)
    {
      off_y = 24;
      max = most - 2 + TOOLOFFSET;

      dest.x = WINDOW_WIDTH - 96;
      dest.y = 40;

      if (font_scroll > 0)
        {
          SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);
        }

      dest.x = WINDOW_WIDTH - 96;
      if (cur_tool == TOOL_LABEL)
        dest.y = 40 + 24 + ((5 + TOOLOFFSET / 2) * 48);
      else
        dest.y = 40 + 24 + ((6 + TOOLOFFSET / 2) * 48);

      if (!disable_stamp_controls)
        dest.y = dest.y - (48 * 2);

      if (font_scroll < num_font_families - (most - 2) - TOOLOFFSET)
        {
          SDL_BlitSurface(img_scroll_down, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_scroll_down_off, NULL, screen, &dest);
        }
    }
  else
    {
      off_y = 0;
      max = most + TOOLOFFSET;
    }

  /* Draw each of the shown fonts: */
  if (!num_font_families % 2)
    font_scroll = min(font_scroll, max(0, num_font_families - max));    /*     FIXAM COMENTARI */
  else
    font_scroll = min(font_scroll, max(0, num_font_families + 1 - max));        /*     FIXAM COMENTARI */

  for (font = font_scroll; font < font_scroll + max; font++)
    {
      i = font - font_scroll;


      dest.x = ((i % 2) * 48) + (WINDOW_WIDTH - 96);
      dest.y = ((i / 2) * 48) + 40 + off_y;

      if (font == cur_font)
        {
          SDL_BlitSurface(img_btn_down, NULL, screen, &dest);
        }
      else if (font < num_font_families)
        {
          SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
        }

      if (font < num_font_families)
        {
          SDL_Surface *tmp_surf_1;

          /* Label for 'Letters' buttons (font selector, down the right when the Text tool is being used); used to show the difference between font faces */
          tmp_surf_1 = render_text(getfonthandle(font), gettext("Aa"), black);

          if (tmp_surf_1 == NULL)
            {
              printf("render_text() returned NULL!\n");
              return;
            }

          if (tmp_surf_1->w > 48 || tmp_surf_1->h > 48)
            {
              tmp_surf = thumbnail(tmp_surf_1, 48, 48, 1);
              SDL_FreeSurface(tmp_surf_1);
            }
          else
            tmp_surf = tmp_surf_1;

          src.x = (tmp_surf->w - 48) / 2;
          src.y = (tmp_surf->h - 48) / 2;
          src.w = 48;
          src.h = 48;

          if (src.x < 0)
            src.x = 0;
          if (src.y < 0)
            src.y = 0;

          dest.x = ((i % 2) * 48) + (WINDOW_WIDTH - 96);
          if (src.w > tmp_surf->w)
            {
              src.w = tmp_surf->w;
              dest.x = dest.x + ((48 - (tmp_surf->w)) / 2);
            }


          dest.y = ((i / 2) * 48) + 40 + off_y;
          if (src.h > tmp_surf->h)
            {
              src.h = tmp_surf->h;
              dest.y = dest.y + ((48 - (tmp_surf->h)) / 2);
            }

          SDL_BlitSurface(tmp_surf, &src, screen, &dest);

          SDL_FreeSurface(tmp_surf);
        }
    }


  /* Draw text controls: */

  if (!disable_stamp_controls)
    {
      SDL_Surface *button_color;
      SDL_Surface *button_body;

      if (cur_tool == TOOL_LABEL)
        {

          /* disabling rotation as I am not sure how this should be implemented */
          dest.x = WINDOW_WIDTH - 96;
          dest.y = 40 + ((4 + TOOLOFFSET / 2) * 48);
          SDL_BlitSurface(img_btn_off, NULL, screen, &dest);

          /* if(cur_label == LABEL_ROTATE) */
          /*   SDL_BlitSurface(img_btn_down, NULL, screen, &dest); */
          /* else */
          /*   SDL_BlitSurface(img_btn_up, NULL, screen, &dest); */

          /* dest.x = WINDOW_WIDTH - 96 + (48 - img_label->w) / 2; */
          /* dest.y = (40 + ((4 + TOOLOFFSET / 2) * 48) + (48 - img_label->h) / 2); */

          /* SDL_BlitSurface(img_label, NULL, screen, &dest); */

          dest.x = WINDOW_WIDTH - 48;
          dest.y = 40 + ((4 + TOOLOFFSET / 2) * 48);

          if (cur_label == LABEL_SELECT)
            SDL_BlitSurface(img_btn_down, NULL, screen, &dest);

          else
            {
              if (are_labels())
                SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
              else
                SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
            }


          dest.x = WINDOW_WIDTH - 48 + (48 - img_label_select->w) / 2;
          dest.y = (40 + ((4 + TOOLOFFSET / 2) * 48) + (48 - img_label_select->h) / 2);

          SDL_BlitSurface(img_label_select, NULL, screen, &dest);
        }

      /* Show bold button: */

      dest.x = WINDOW_WIDTH - 96;
      dest.y = 40 + ((5 + TOOLOFFSET / 2) * 48);

      if (text_state & TTF_STYLE_BOLD)
        SDL_BlitSurface(img_btn_down, NULL, screen, &dest);
      else
        SDL_BlitSurface(img_btn_up, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - 96 + (48 - img_bold->w) / 2;
      dest.y = (40 + ((5 + TOOLOFFSET / 2) * 48) + (48 - img_bold->h) / 2);

      SDL_BlitSurface(img_bold, NULL, screen, &dest);


      /* Show italic button: */

      dest.x = WINDOW_WIDTH - 48;
      dest.y = 40 + ((5 + TOOLOFFSET / 2) * 48);

      if (text_state & TTF_STYLE_ITALIC)
        SDL_BlitSurface(img_btn_down, NULL, screen, &dest);
      else
        SDL_BlitSurface(img_btn_up, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - 48 + (48 - img_italic->w) / 2;
      dest.y = (40 + ((5 + TOOLOFFSET / 2) * 48) + (48 - img_italic->h) / 2);

      SDL_BlitSurface(img_italic, NULL, screen, &dest);


      /* Show shrink button: */

      dest.x = WINDOW_WIDTH - 96;
      dest.y = 40 + ((6 + TOOLOFFSET / 2) * 48);

      if (text_size > MIN_TEXT_SIZE)
        {
          button_color = img_black;
          button_body = img_btn_up;
        }
      else
        {
          button_color = img_grey;
          button_body = img_btn_off;
        }
      SDL_BlitSurface(button_body, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - 96 + (48 - img_shrink->w) / 2;
      dest.y = (40 + ((6 + TOOLOFFSET / 2) * 48) + (48 - img_shrink->h) / 2);

      SDL_BlitSurface(button_color, NULL, img_shrink, NULL);
      SDL_BlitSurface(img_shrink, NULL, screen, &dest);


      /* Show grow button: */

      dest.x = WINDOW_WIDTH - 48;
      dest.y = 40 + ((6 + TOOLOFFSET / 2) * 48);

      if (text_size < MAX_TEXT_SIZE)
        {
          button_color = img_black;
          button_body = img_btn_up;
        }
      else
        {
          button_color = img_grey;
          button_body = img_btn_off;
        }
      SDL_BlitSurface(button_body, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - 48 + (48 - img_grow->w) / 2;
      dest.y = (40 + ((6 + TOOLOFFSET / 2) * 48) + (48 - img_grow->h) / 2);

      SDL_BlitSurface(button_color, NULL, img_grow, NULL);
      SDL_BlitSurface(img_grow, NULL, screen, &dest);
    }
  else
    {
      if (cur_tool == TOOL_LABEL)
        {
          dest.x = WINDOW_WIDTH - 96;
          dest.y = 40 + ((6 + TOOLOFFSET / 2) * 48);

          SDL_BlitSurface(img_btn_up, NULL, screen, &dest);

          dest.x = WINDOW_WIDTH - 96 + (48 - img_label->w) / 2;
          dest.y = (40 + ((6 + TOOLOFFSET / 2) * 48) + (48 - img_label->h) / 2);

          SDL_BlitSurface(img_label, NULL, screen, &dest);

          dest.x = WINDOW_WIDTH - 48;
          dest.y = 40 + ((6 + TOOLOFFSET / 2) * 48);

          SDL_BlitSurface(img_btn_up, NULL, screen, &dest);

          dest.x = WINDOW_WIDTH - 48 + (48 - img_label_select->w) / 2;
          dest.y = (40 + ((6 + TOOLOFFSET / 2) * 48) + (48 - img_label_select->h) / 2);

          SDL_BlitSurface(img_label_select, NULL, screen, &dest);
        }
    }
}


/* Draw stamps: */

static void draw_stamps(void)
{
  int i, off_y, max, stamp, most;
  int base_x, base_y;
  SDL_Rect dest;
  SDL_Surface *img;
  int sizes, size_at;
  float x_per, y_per;
  int xx, yy;
  SDL_Surface *btn, *blnk;
  SDL_Surface *button_color;
  SDL_Surface *button_body;


  /* Draw the title: */
  draw_image_title(TITLE_STAMPS, r_ttoolopt);


  /* How many can we show? */

  most = 8;                     /* was 10 and 14, before left/right controls -bjk 2007.05.03 */
  if (disable_stamp_controls)
    most = 12;


  /* Do we need scrollbars? */

  if (num_stamps[stamp_group] > most + TOOLOFFSET)
    {
      off_y = 24;
      max = (most - 2) + TOOLOFFSET;

      dest.x = WINDOW_WIDTH - 96;
      dest.y = 40;

      if (stamp_scroll[stamp_group] > 0)
        {
          SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);
        }


      dest.x = WINDOW_WIDTH - 96;
      dest.y = 40 + 24 + ((5 + TOOLOFFSET / 2) * 48);   /* was 6, before left/right controls -bjk 2007.05.03 */

      if (!disable_stamp_controls)
        dest.y = dest.y - (48 * 2);

      if (stamp_scroll[stamp_group] < num_stamps[stamp_group] - (most - 2) - TOOLOFFSET)
        {
          SDL_BlitSurface(img_scroll_down, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_scroll_down_off, NULL, screen, &dest);
        }
    }
  else
    {
      off_y = 0;
      max = most + TOOLOFFSET;
    }


  /* Draw each of the shown stamps: */

  for (stamp = stamp_scroll[stamp_group]; stamp < stamp_scroll[stamp_group] + max; stamp++)
    {
      i = stamp - stamp_scroll[stamp_group];


      dest.x = ((i % 2) * 48) + (WINDOW_WIDTH - 96);
      dest.y = ((i / 2) * 48) + 40 + off_y;

      if (stamp == cur_stamp[stamp_group])
        {
          SDL_BlitSurface(img_btn_down, NULL, screen, &dest);
        }
      else if (stamp < num_stamps[stamp_group])
        {
          SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
        }

      if (stamp < num_stamps[stamp_group])
        {
          get_stamp_thumb(stamp_data[stamp_group][stamp]);
          img = stamp_data[stamp_group][stamp]->thumbnail;

          base_x = ((i % 2) * 48) + (WINDOW_WIDTH - 96) + ((48 - (img->w)) / 2);

          base_y = ((i / 2) * 48) + 40 + ((48 - (img->h)) / 2) + off_y;

          dest.x = base_x;
          dest.y = base_y;

          SDL_BlitSurface(img, NULL, screen, &dest);
        }
    }


  /* Draw stamp group buttons (prev/next): */


  /* Show prev button: */

  button_color = img_black;
  button_body = img_btn_nav;

  dest.x = WINDOW_WIDTH - 96;
  dest.y = 40 + (((most + TOOLOFFSET) / 2) * 48);

  SDL_BlitSurface(button_body, NULL, screen, &dest);

  dest.x = WINDOW_WIDTH - 96 + (48 - img_prev->w) / 2;
  dest.y = (40 + (((most + TOOLOFFSET) / 2) * 48) + (48 - img_prev->h) / 2);

  SDL_BlitSurface(button_color, NULL, img_prev, NULL);
  SDL_BlitSurface(img_prev, NULL, screen, &dest);

  /* Show next button: */

  button_color = img_black;
  button_body = img_btn_nav;

  dest.x = WINDOW_WIDTH - 48;
  dest.y = 40 + (((most + TOOLOFFSET) / 2) * 48);

  SDL_BlitSurface(button_body, NULL, screen, &dest);

  dest.x = WINDOW_WIDTH - 48 + (48 - img_next->w) / 2;
  dest.y = (40 + (((most + TOOLOFFSET) / 2) * 48) + (48 - img_next->h) / 2);

  SDL_BlitSurface(button_color, NULL, img_next, NULL);
  SDL_BlitSurface(img_next, NULL, screen, &dest);


  /* Draw stamp controls: */

  if (!disable_stamp_controls)
    {
      /* Show mirror button: */

      dest.x = WINDOW_WIDTH - 96;
      dest.y = 40 + ((5 + TOOLOFFSET / 2) * 48);

      if (stamp_data[stamp_group][cur_stamp[stamp_group]]->mirrorable)
        {
          if (stamp_data[stamp_group][cur_stamp[stamp_group]]->mirrored)
            {
              button_color = img_black;
              button_body = img_btn_down;
            }
          else
            {
              button_color = img_black;
              button_body = img_btn_up;
            }
        }
      else
        {
          button_color = img_grey;
          button_body = img_btn_off;
        }
      SDL_BlitSurface(button_body, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - 96 + (48 - img_mirror->w) / 2;
      dest.y = (40 + ((5 + TOOLOFFSET / 2) * 48) + (48 - img_mirror->h) / 2);

      SDL_BlitSurface(button_color, NULL, img_mirror, NULL);
      SDL_BlitSurface(img_mirror, NULL, screen, &dest);

      /* Show flip button: */

      dest.x = WINDOW_WIDTH - 48;
      dest.y = 40 + ((5 + TOOLOFFSET / 2) * 48);

      if (stamp_data[stamp_group][cur_stamp[stamp_group]]->flipable)
        {
          if (stamp_data[stamp_group][cur_stamp[stamp_group]]->flipped)
            {
              button_color = img_black;
              button_body = img_btn_down;
            }
          else
            {
              button_color = img_black;
              button_body = img_btn_up;
            }
        }
      else
        {
          button_color = img_grey;
          button_body = img_btn_off;
        }
      SDL_BlitSurface(button_body, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - 48 + (48 - img_flip->w) / 2;
      dest.y = (40 + ((5 + TOOLOFFSET / 2) * 48) + (48 - img_flip->h) / 2);

      SDL_BlitSurface(button_color, NULL, img_flip, NULL);
      SDL_BlitSurface(img_flip, NULL, screen, &dest);


#ifdef OLD_STAMP_GROW_SHRINK
      /* Show shrink button: */

      dest.x = WINDOW_WIDTH - 96;
      dest.y = 40 + ((6 + TOOLOFFSET / 2) * 48);

      if (stamp_data[stamp_group][cur_stamp[stamp_group]]->size > MIN_STAMP_SIZE)
        {
          button_color = img_black;
          button_body = img_btn_up;
        }
      else
        {
          button_color = img_grey;
          button_body = img_btn_off;
        }
      SDL_BlitSurface(button_body, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - 96 + (48 - img_shrink->w) / 2;
      dest.y = (40 + ((6 + TOOLOFFSET / 2) * 48) + (48 - img_shrink->h) / 2);

      SDL_BlitSurface(button_color, NULL, img_shrink, NULL);
      SDL_BlitSurface(img_shrink, NULL, screen, &dest);


      /* Show grow button: */

      dest.x = WINDOW_WIDTH - 48;
      dest.y = 40 + ((6 + TOOLOFFSET / 2) * 48);

      if (stamp_data[stamp_group][cur_stamp[stamp_group]]->size < MAX_STAMP_SIZE)
        {
          button_color = img_black;
          button_body = img_btn_up;
        }
      else
        {
          button_color = img_grey;
          button_body = img_btn_off;
        }
      SDL_BlitSurface(button_body, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - 48 + (48 - img_grow->w) / 2;
      dest.y = (40 + ((6 + TOOLOFFSET / 2) * 48) + (48 - img_grow->h) / 2);

      SDL_BlitSurface(button_color, NULL, img_grow, NULL);
      SDL_BlitSurface(img_grow, NULL, screen, &dest);

#else
      sizes = MAX_STAMP_SIZE - MIN_STAMP_SIZE + 1;      /* +1 for SF Bug #1668235 -bjk 2011.01.08 */
      size_at = (stamp_data[stamp_group][cur_stamp[stamp_group]]->size - MIN_STAMP_SIZE);
      x_per = 96.0 / sizes;
      y_per = 48.0 / sizes;

      for (i = 0; i < sizes; i++)
        {
          xx = ceil(x_per);
          yy = ceil(y_per * i);

          if (i <= size_at)
            btn = thumbnail(img_btn_down, xx, yy, 0);
          else
            btn = thumbnail(img_btn_up, xx, yy, 0);

          blnk = thumbnail(img_btn_off, xx, 48 - yy, 0);

          /* FIXME: Check for NULL! */

          dest.x = (WINDOW_WIDTH - 96) + (i * x_per);
          dest.y = (((7 + TOOLOFFSET / 2) * 48)) - 8;
          SDL_BlitSurface(blnk, NULL, screen, &dest);

          dest.x = (WINDOW_WIDTH - 96) + (i * x_per);
          dest.y = (((8 + TOOLOFFSET / 2) * 48)) - 8 - (y_per * i);
          SDL_BlitSurface(btn, NULL, screen, &dest);

          SDL_FreeSurface(btn);
          SDL_FreeSurface(blnk);
        }
#endif
    }
}


/* Draw the shape selector: */

static void draw_shapes(void)
{
  int i, shape, max, off_y;
  SDL_Rect dest;


  draw_image_title(TITLE_SHAPES, r_ttoolopt);


  if (NUM_SHAPES > 14 + TOOLOFFSET)
    {
      off_y = 24;
      max = 12 + TOOLOFFSET;

      dest.x = WINDOW_WIDTH - 96;
      dest.y = 40;

      if (shape_scroll > 0)
        {
          SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);
        }

      dest.x = WINDOW_WIDTH - 96;
      dest.y = 40 + 24 + ((6 + TOOLOFFSET / 2) * 48);

      if (shape_scroll < NUM_SHAPES - 12 - TOOLOFFSET)
        {
          SDL_BlitSurface(img_scroll_down, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_scroll_down_off, NULL, screen, &dest);
        }
    }
  else
    {
      off_y = 0;
      max = 14 + TOOLOFFSET;
    }

  for (shape = shape_scroll; shape < shape_scroll + max; shape++)
    {
      i = shape - shape_scroll;

      dest.x = ((i % 2) * 48) + WINDOW_WIDTH - 96;
      dest.y = ((i / 2) * 48) + 40 + off_y;

      if (shape == cur_shape)
        {
          SDL_BlitSurface(img_btn_down, NULL, screen, &dest);
        }
      else if (shape < NUM_SHAPES)
        {
          SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
        }


      if (shape < NUM_SHAPES)
        {
          dest.x = ((i % 2) * 48) + 4 + WINDOW_WIDTH - 96;
          dest.y = ((i / 2) * 48) + 40 + 4 + off_y;

          SDL_BlitSurface(img_shapes[shape], NULL, screen, &dest);

          dest.x = ((i % 2) * 48) + 4 + WINDOW_WIDTH - 96 + (40 - img_shape_names[shape]->w) / 2;
          dest.y = ((i / 2) * 48) + 40 + 4 + (44 - img_shape_names[shape]->h) + off_y;

          SDL_BlitSurface(img_shape_names[shape], NULL, screen, &dest);
        }
    }
}


/* Draw the eraser selector: */

static void draw_erasers(void)
{
  int i, x, y, sz;
  int xx, yy, n;
  void (*putpixel) (SDL_Surface *, int, int, Uint32);
  SDL_Rect dest;

  putpixel = putpixels[screen->format->BytesPerPixel];

  draw_image_title(TITLE_ERASERS, r_ttoolopt);

  for (i = 0; i < 14 + TOOLOFFSET; i++)
    {
      dest.x = ((i % 2) * 48) + WINDOW_WIDTH - 96;
      dest.y = ((i / 2) * 48) + 40;


      if (i == cur_eraser)
        {
          SDL_BlitSurface(img_btn_down, NULL, screen, &dest);
        }
      else if (i < NUM_ERASERS)
        {
          SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
        }
      else
        {
          SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
        }


      if (i < NUM_ERASERS)
        {
          if (i < NUM_ERASERS / 2)
            {
              /* Square */

              sz = (2 + (((NUM_ERASERS / 2) - 1 - i) * (38 / ((NUM_ERASERS / 2) - 1))));

              x = ((i % 2) * 48) + WINDOW_WIDTH - 96 + 24 - sz / 2;
              y = ((i / 2) * 48) + 40 + 24 - sz / 2;

              dest.x = x;
              dest.y = y;
              dest.w = sz;
              dest.h = 2;

              SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));

              dest.x = x;
              dest.y = y + sz - 2;
              dest.w = sz;
              dest.h = 2;

              SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));

              dest.x = x;
              dest.y = y;
              dest.w = 2;
              dest.h = sz;

              SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));

              dest.x = x + sz - 2;
              dest.y = y;
              dest.w = 2;
              dest.h = sz;

              SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));
            }
          else
            {
              /* Circle */

              sz = (2 + (((NUM_ERASERS / 2) - 1 - (i - NUM_ERASERS / 2)) * (38 / ((NUM_ERASERS / 2) - 1))));

              x = ((i % 2) * 48) + WINDOW_WIDTH - 96 + 24 - sz / 2;
              y = ((i / 2) * 48) + 40 + 24 - sz / 2;

              for (yy = 0; yy <= sz; yy++)
                {
                  for (xx = 0; xx <= sz; xx++)
                    {
                      n = (xx * xx) + (yy * yy) - ((sz / 2) * (sz / 2));

                      if (n >= -sz && n <= sz)
                        {
                          putpixel(screen, (x + sz / 2) + xx, (y + sz / 2) + yy, SDL_MapRGB(screen->format, 0, 0, 0));

                          putpixel(screen, (x + sz / 2) - xx, (y + sz / 2) + yy, SDL_MapRGB(screen->format, 0, 0, 0));

                          putpixel(screen, (x + sz / 2) + xx, (y + sz / 2) - yy, SDL_MapRGB(screen->format, 0, 0, 0));

                          putpixel(screen, (x + sz / 2) - xx, (y + sz / 2) - yy, SDL_MapRGB(screen->format, 0, 0, 0));

                        }
                    }
                }
            }
        }
    }
}


/* Draw no selectables: */

static void draw_none(void)
{
  int i;
  SDL_Rect dest;

  dest.x = WINDOW_WIDTH - 96;
  dest.y = 0;
  SDL_BlitSurface(img_title_off, NULL, screen, &dest);

  for (i = 0; i < 14 + TOOLOFFSET; i++)
    {
      dest.x = ((i % 2) * 48) + WINDOW_WIDTH - 96;
      dest.y = ((i / 2) * 48) + 40;

      SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
    }
}



/* Create a thumbnail: */

static SDL_Surface *thumbnail(SDL_Surface * src, int max_x, int max_y, int keep_aspect)
{
  return (thumbnail2(src, max_x, max_y, keep_aspect, 1));
}

static SDL_Surface *thumbnail2(SDL_Surface * src, int max_x, int max_y, int keep_aspect, int keep_alpha)
{
  int x, y;
  float src_x, src_y, off_x, off_y;
  SDL_Surface *s;

#ifdef GAMMA_CORRECTED_THUMBNAILS
  float tr, tg, tb, ta;
#else
  Uint32 tr, tg, tb, ta;
#endif
  Uint8 r, g, b, a;
  float xscale, yscale;
  int tmp;
  void (*putpixel) (SDL_Surface *, int, int, Uint32);

  Uint32(*getpixel) (SDL_Surface *, int, int) = getpixels[src->format->BytesPerPixel];

  /* Determine scale and centering offsets: */
  if (!keep_aspect)
    {
      yscale = (float)((float)src->h / (float)max_y);
      xscale = (float)((float)src->w / (float)max_x);

      off_x = 0;
      off_y = 0;
    }
  else
    {
      if (src->h > src->w)
        {
          yscale = (float)((float)src->h / (float)max_y);
          xscale = yscale;

          off_x = ((src->h - src->w) / xscale) / 2;
          off_y = 0;
        }
      else
        {
          xscale = (float)((float)src->w / (float)max_x);
          yscale = xscale;

          off_x = 0;
          off_y = ((src->w - src->h) / xscale) / 2;
        }
    }


#ifndef NO_BILINEAR
  if (max_x > src->w && max_y > src->h)
    return (zoom(src, max_x, max_y));
#endif


  /* Create surface for thumbnail: */

  s = SDL_CreateRGBSurface(src->flags,  /* SDL_SWSURFACE, */
                           max_x, max_y, src->format->BitsPerPixel, src->format->Rmask, src->format->Gmask,
                           src->format->Bmask, src->format->Amask);


  if (s == NULL)
    {
      fprintf(stderr, "\nError: Can't build stamp thumbnails\n"
              "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

      cleanup();
      exit(1);
    }

  putpixel = putpixels[s->format->BytesPerPixel];

  /* Create thumbnail itself: */

  SDL_LockSurface(src);
  SDL_LockSurface(s);

  for (y = 0; y < max_y; y++)
    {
      for (x = 0; x < max_x; x++)
        {
#ifndef LOW_QUALITY_THUMBNAILS

#ifdef GAMMA_CORRECTED_THUMBNAILS
          /* per: http://www.4p8.com/eric.brasseur/gamma.html */
          float gamma = 2.2;
          float gamma_invert = 1.0 / gamma;
#endif

          tr = 0;
          tg = 0;
          tb = 0;
          ta = 0;

          tmp = 0;

          for (src_y = y * yscale; src_y < y * yscale + yscale && src_y < src->h; src_y++)
            {
              for (src_x = x * xscale; src_x < x * xscale + xscale && src_x < src->w; src_x++)
                {
                  SDL_GetRGBA(getpixel(src, src_x, src_y), src->format, &r, &g, &b, &a);

#ifdef GAMMA_CORRECTED_THUMBNAILS
//        tr = tr + pow((float)r, gamma);
//        tb = tb + pow((float)b, gamma);
//        tg = tg + pow((float)g, gamma);
                  tr = tr + sRGB_to_linear_table[r];
                  tg = tg + sRGB_to_linear_table[g];
                  tb = tb + sRGB_to_linear_table[b];
#else
                  tr = tr + r;
                  tb = tb + b;
                  tg = tg + g;
#endif
                  ta = ta + a;

                  tmp++;
                }
            }

          if (tmp != 0)
            {
              tr = tr / tmp;
              tb = tb / tmp;
              tg = tg / tmp;
              ta = ta / tmp;

#ifdef GAMMA_CORRECTED_THUMBNAILS
//      tr = ceil(pow(tr, gamma_invert));
//      tg = ceil(pow(tg, gamma_invert));
//      tb = ceil(pow(tb, gamma_invert));
              tr = linear_to_sRGB(tr);
              tg = linear_to_sRGB(tg);
              tb = linear_to_sRGB(tb);
#endif

              if (keep_alpha == 0 && s->format->Amask != 0)
                {
                  tr = ((ta * tr) / 255) + (255 - ta);
                  tg = ((ta * tg) / 255) + (255 - ta);
                  tb = ((ta * tb) / 255) + (255 - ta);

                  putpixel(s, x + off_x, y + off_y, SDL_MapRGBA(s->format, (Uint8) tr, (Uint8) tg, (Uint8) tb, 0xff));
                }
              else
                {
                  putpixel(s, x + off_x, y + off_y, SDL_MapRGBA(s->format,
                                                                (Uint8) tr, (Uint8) tg, (Uint8) tb, (Uint8) ta));
                }
            }
#else
          src_x = x * xscale;
          src_y = y * yscale;

          putpixel(s, x + off_x, y + off_y, getpixel(src, src_x, src_y));
#endif
        }
    }

  SDL_UnlockSurface(s);
  SDL_UnlockSurface(src);

  return s;
}


#ifndef NO_BILINEAR

/* Based on code from: http://www.codeproject.com/cs/media/imageprocessing4.asp
   copyright 2002 Christian Graus */

static SDL_Surface *zoom(SDL_Surface * src, int new_w, int new_h)
{
  SDL_Surface *s;
  void (*putpixel) (SDL_Surface *, int, int, Uint32);

  Uint32(*getpixel) (SDL_Surface *, int, int) = getpixels[src->format->BytesPerPixel];
  float xscale, yscale;
  int x, y;
  float floor_x, ceil_x, floor_y, ceil_y, fraction_x, fraction_y, one_minus_x, one_minus_y;
  float n1, n2;
  float r1, g1, b1, a1;
  float r2, g2, b2, a2;
  float r3, g3, b3, a3;
  float r4, g4, b4, a4;
  Uint8 r, g, b, a;


  /* Create surface for zoom: */

  s = SDL_CreateRGBSurface(src->flags,  /* SDL_SWSURFACE, */
                           new_w, new_h, src->format->BitsPerPixel,
                           src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);


  if (s == NULL)
    {
      fprintf(stderr, "\nError: Can't build zoom surface\n"
              "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

      cleanup();
      exit(1);
    }

  putpixel = putpixels[s->format->BytesPerPixel];


  SDL_LockSurface(src);
  SDL_LockSurface(s);

  xscale = (float)src->w / (float)new_w;
  yscale = (float)src->h / (float)new_h;

  for (x = 0; x < new_w; x++)
    {
      for (y = 0; y < new_h; y++)
        {
          floor_x = floor((float)x * xscale);
          ceil_x = floor_x + 1;
          if (ceil_x >= src->w)
            ceil_x = floor_x;

          floor_y = floor((float)y * yscale);
          ceil_y = floor_y + 1;
          if (ceil_y >= src->h)
            ceil_y = floor_y;

          fraction_x = x * xscale - floor_x;
          fraction_y = y * yscale - floor_y;

          one_minus_x = 1.0 - fraction_x;
          one_minus_y = 1.0 - fraction_y;

#if VIDEO_BPP==32
          {                     //EP added local block to avoid warning "Passing arg 3 from incompatible pointer type" of section below block
            Uint8 r, g, b, a;

            SDL_GetRGBA(getpixel(src, floor_x, floor_y), src->format, &r, &g, &b, &a);
            r1 = (float)r;
            g1 = (float)g;
            b1 = (float)b;
            a1 = (float)a;
            SDL_GetRGBA(getpixel(src, ceil_x, floor_y), src->format, &r, &g, &b, &a);
            r2 = (float)r;
            g2 = (float)g;
            b2 = (float)b;
            a2 = (float)a;
            SDL_GetRGBA(getpixel(src, floor_x, ceil_y), src->format, &r, &g, &b, &a);
            r3 = (float)r;
            g3 = (float)g;
            b3 = (float)b;
            a3 = (float)a;
            SDL_GetRGBA(getpixel(src, ceil_x, ceil_y), src->format, &r, &g, &b, &a);
            r4 = (float)r;
            g4 = (float)g;
            b4 = (float)b;
            a4 = (float)a;
          }
          /*
             SDL_GetRGBA(getpixel(src, floor_x, floor_y), src->format,
             &r1, &g1, &b1, &a1);
             SDL_GetRGBA(getpixel(src, ceil_x,  floor_y), src->format,
             &r2, &g2, &b2, &a2);
             SDL_GetRGBA(getpixel(src, floor_x, ceil_y),  src->format,
             &r3, &g3, &b3, &a3);
             SDL_GetRGBA(getpixel(src, ceil_x,  ceil_y),  src->format,
             &r4, &g4, &b4, &a4);
           */
#else
          {
            Uint8 r, g, b, a;

            r = g = b = a = 0;  /* Unused, bah! */

            SDL_GetRGBA(getpixel(src, floor_x, floor_y), src->format, &r, &g, &b, &a);
            r1 = (float)r;
            g1 = (float)g;
            b1 = (float)b;
            a1 = (float)a;

            SDL_GetRGBA(getpixel(src, ceil_x, floor_y), src->format, &r, &g, &b, &a);
            r2 = (float)r;
            g2 = (float)g;
            b2 = (float)b;
            a2 = (float)a;

            SDL_GetRGBA(getpixel(src, floor_x, ceil_y), src->format, &r, &g, &b, &a);
            r3 = (float)r;
            g3 = (float)g;
            b3 = (float)b;
            a3 = (float)a;

            SDL_GetRGBA(getpixel(src, ceil_x, ceil_y), src->format, &r, &g, &b, &a);
            r4 = (float)r;
            g4 = (float)g;
            b4 = (float)b;
            a4 = (float)a;
          }
#endif

          n1 = (one_minus_x * r1 + fraction_x * r2);
          n2 = (one_minus_x * r3 + fraction_x * r4);
          r = (one_minus_y * n1 + fraction_y * n2);

          n1 = (one_minus_x * g1 + fraction_x * g2);
          n2 = (one_minus_x * g3 + fraction_x * g4);
          g = (one_minus_y * n1 + fraction_y * n2);

          n1 = (one_minus_x * b1 + fraction_x * b2);
          n2 = (one_minus_x * b3 + fraction_x * b4);
          b = (one_minus_y * n1 + fraction_y * n2);

          n1 = (one_minus_x * a1 + fraction_x * a2);
          n2 = (one_minus_x * a3 + fraction_x * a4);
          a = (one_minus_y * n1 + fraction_y * n2);

          putpixel(s, x, y, SDL_MapRGBA(s->format, r, g, b, a));
        }
    }

  SDL_UnlockSurface(s);
  SDL_UnlockSurface(src);

  return s;

}
#endif


/* XOR must show up on black, white, 0x7f grey, and 0x80 grey.
  XOR must be exactly 100% perfectly reversable. */
static void xorpixel(int x, int y)
{
  Uint8 *p;
  int BytesPerPixel;

  /* if outside the canvas, return */
  if ((unsigned)x >= (unsigned)canvas->w || (unsigned)y >= (unsigned)canvas->h)
    return;
  /* now switch to screen coordinates */
  x += r_canvas.x;
  y += r_canvas.y;

  /* Always 4, except 3 when loading a saved image. */
  BytesPerPixel = screen->format->BytesPerPixel;

  /* Set a pointer to the exact location in memory of the pixel */
  p = (Uint8 *) (((Uint8 *) screen->pixels) +   /* Start: beginning of RAM */
                 (y * screen->pitch) +  /* Go down Y lines */
                 (x * BytesPerPixel));  /* Go in X pixels */

  /* XOR the (correctly-sized) piece of data in the screen's RAM */
  if (likely(BytesPerPixel == 4))
    *(Uint32 *) p ^= 0x80808080u;       /* 32-bit display */
  else if (BytesPerPixel == 1)
    *p ^= 0x80;
  else if (BytesPerPixel == 2)
    *(Uint16 *) p ^= 0xd6d6;
  else if (BytesPerPixel == 3)
    {
      p[0] ^= 0x80;
      p[1] ^= 0x80;
      p[2] ^= 0x80;
    }
}


/* Undo! */

static void do_undo(void)
{
  int wanna_update_toolbar;

  wanna_update_toolbar = 0;

  if (cur_undo != oldest_undo)
    {
      cur_undo--;

      do_undo_label_node();

      if (cur_undo < 0)
        cur_undo = NUM_UNDO_BUFS - 1;

#ifdef DEBUG
      printf("BLITTING: %d\n", cur_undo);
#endif
      SDL_BlitSurface(undo_bufs[cur_undo], NULL, canvas, NULL);


      if (img_starter != NULL)
        {
          if (undo_starters[cur_undo] == UNDO_STARTER_MIRRORED)
            {
              starter_mirrored = !starter_mirrored;
              mirror_starter();
            }
          else if (undo_starters[cur_undo] == UNDO_STARTER_FLIPPED)
            {
              starter_flipped = !starter_flipped;
              flip_starter();
            }
        }

      update_canvas(0, 0, (WINDOW_WIDTH - 96), (48 * 7) + 40 + HEIGHTOFFSET);


      if (cur_undo == oldest_undo)
        {
          tool_avail[TOOL_UNDO] = 0;
          wanna_update_toolbar = 1;
        }

      if (tool_avail[TOOL_REDO] == 0)
        {
          tool_avail[TOOL_REDO] = 1;
          wanna_update_toolbar = 1;
        }

      if (wanna_update_toolbar)
        {
          draw_toolbar();
          update_screen_rect(&r_tools);
        }

      been_saved = 0;
    }

#ifdef DEBUG
  printf("UNDO: Current=%d  Oldest=%d  Newest=%d\n", cur_undo, oldest_undo, newest_undo);
#endif
}


/* Redo! */

static void do_redo(void)
{
  if (cur_undo != newest_undo)
    {
      if (img_starter != NULL)
        {
          if (undo_starters[cur_undo] == UNDO_STARTER_MIRRORED)
            {
              starter_mirrored = !starter_mirrored;
              mirror_starter();
            }
          else if (undo_starters[cur_undo] == UNDO_STARTER_FLIPPED)
            {
              starter_flipped = !starter_flipped;
              flip_starter();
            }
        }

      cur_undo = (cur_undo + 1) % NUM_UNDO_BUFS;

#ifdef DEBUG
      printf("BLITTING: %d\n", cur_undo);
#endif
      do_redo_label_node();
      SDL_BlitSurface(undo_bufs[cur_undo], NULL, canvas, NULL);

      update_canvas(0, 0, (WINDOW_WIDTH - 96), (48 * 7) + 40 + HEIGHTOFFSET);

      been_saved = 0;
    }

#ifdef DEBUG
  printf("REDO: Current=%d  Oldest=%d  Newest=%d\n", cur_undo, oldest_undo, newest_undo);
#endif


  if (((cur_undo + 1) % NUM_UNDO_BUFS) == newest_undo)
    {
      tool_avail[TOOL_REDO] = 0;
    }

  tool_avail[TOOL_UNDO] = 1;

  draw_toolbar();
  update_screen_rect(&r_tools);
}


/* Create the current brush in the current color: */

static void render_brush(void)
{
  Uint32 amask;
  int x, y;
  Uint8 r, g, b, a;

  Uint32(*getpixel_brush) (SDL_Surface *, int, int) = getpixels[img_brushes[cur_brush]->format->BytesPerPixel];
  void (*putpixel_brush) (SDL_Surface *, int, int, Uint32) = putpixels[img_brushes[cur_brush]->format->BytesPerPixel];


  /* Kludge; not sure why cur_brush would become greater! */

  if (cur_brush >= num_brushes)
    cur_brush = 0;


  /* Free the old rendered brush (if any): */

  if (img_cur_brush != NULL)
    {
      SDL_FreeSurface(img_cur_brush);
    }


  /* Create a surface to render into: */

  amask = ~(img_brushes[cur_brush]->format->Rmask |
            img_brushes[cur_brush]->format->Gmask | img_brushes[cur_brush]->format->Bmask);

  img_cur_brush =
    SDL_CreateRGBSurface(SDL_SWSURFACE,
                         img_brushes[cur_brush]->w,
                         img_brushes[cur_brush]->h,
                         img_brushes[cur_brush]->format->BitsPerPixel,
                         img_brushes[cur_brush]->format->Rmask,
                         img_brushes[cur_brush]->format->Gmask, img_brushes[cur_brush]->format->Bmask, amask);

  if (img_cur_brush == NULL)
    {
      fprintf(stderr, "\nError: Can't render a brush!\n"
              "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

      cleanup();
      exit(1);
    }


  /* Render the new brush: */

  SDL_LockSurface(img_brushes[cur_brush]);
  SDL_LockSurface(img_cur_brush);

  for (y = 0; y < img_brushes[cur_brush]->h; y++)
    {
      for (x = 0; x < img_brushes[cur_brush]->w; x++)
        {
          SDL_GetRGBA(getpixel_brush(img_brushes[cur_brush], x, y), img_brushes[cur_brush]->format, &r, &g, &b, &a);

          if (r == g && g == b)
            {
              putpixel_brush(img_cur_brush, x, y,
                             SDL_MapRGBA(img_cur_brush->format,
                                         color_hexes[cur_color][0],
                                         color_hexes[cur_color][1], color_hexes[cur_color][2], a));
            }
          else
            {
              putpixel_brush(img_cur_brush, x, y,
                             SDL_MapRGBA(img_cur_brush->format,
                                         (r + color_hexes[cur_color][0]) >> 1,
                                         (g + color_hexes[cur_color][1]) >> 1,
                                         (b + color_hexes[cur_color][2]) >> 1, a));
            }
        }
    }

  SDL_UnlockSurface(img_cur_brush);
  SDL_UnlockSurface(img_brushes[cur_brush]);

  img_cur_brush_frame_w = img_cur_brush->w / abs(brushes_frames[cur_brush]);
  img_cur_brush_w = img_cur_brush_frame_w / (brushes_directional[cur_brush] ? 3 : 1);
  img_cur_brush_h = img_cur_brush->h / (brushes_directional[cur_brush] ? 3 : 1);
  img_cur_brush_frames = brushes_frames[cur_brush];
  img_cur_brush_directional = brushes_directional[cur_brush];
  img_cur_brush_spacing = brushes_spacing[cur_brush];

  brush_counter = 0;
}


/* Draw a XOR line: */

static void line_xor(int x1, int y1, int x2, int y2)
{
  int dx, dy, y, num_drawn;
  float m, b;


  /* Kludgey, but it works: */

  /* SDL_LockSurface(screen); */

  dx = x2 - x1;
  dy = y2 - y1;

  num_drawn = 0;

  if (dx != 0)
    {
      m = ((float)dy) / ((float)dx);
      b = y1 - m * x1;

      if (x2 >= x1)
        dx = 1;
      else
        dx = -1;


      while (x1 != x2)
        {
          y1 = m * x1 + b;
          y2 = m * (x1 + dx) + b;

          if (y1 > y2)
            {
              y = y1;
              y1 = y2;
              y2 = y;
            }

          for (y = y1; y <= y2; y++)
            {
              num_drawn++;
              if (num_drawn < 10 || dont_do_xor == 0)
                xorpixel(x1, y);
            }

          x1 = x1 + dx;
        }
    }
  else
    {
      if (y1 > y2)
        {
          for (y = y1; y >= y2; y--)
            {
              num_drawn++;

              if (num_drawn < 10 || dont_do_xor == 0)
                xorpixel(x1, y);
            }
        }
      else
        {
          for (y = y1; y <= y2; y++)
            {
              num_drawn++;

              if (num_drawn < 10 || dont_do_xor == 0)
                xorpixel(x1, y);
            }
        }
    }

  /* SDL_UnlockSurface(screen); */
}


/* Draw a XOR rectangle: */

static void rect_xor(int x1, int y1, int x2, int y2)
{
  if (x1 < 0)
    x1 = 0;

  if (x2 < 0)
    x2 = 0;

  if (y1 < 0)
    y1 = 0;

  if (y2 < 0)
    y2 = 0;

  if (x1 >= (WINDOW_WIDTH - 96 - 96))
    x1 = (WINDOW_WIDTH - 96 - 96) - 1;

  if (x2 >= (WINDOW_WIDTH - 96 - 96))
    x2 = (WINDOW_WIDTH - 96 - 96) - 1;

  if (y1 >= (48 * 7) + 40 + HEIGHTOFFSET)
    y1 = (48 * 7) + 40 + HEIGHTOFFSET - 1;

  if (y2 >= (48 * 7) + 40 + HEIGHTOFFSET)
    y2 = (48 * 7) + 40 + HEIGHTOFFSET - 1;

  line_xor(x1, y1, x2, y1);
  line_xor(x2, y1, x2, y2);
  line_xor(x2, y2, x1, y2);
  line_xor(x1, y2, x1, y1);
}


/* Erase at the cursor! */

static void do_eraser(int x, int y)
{
  SDL_Rect dest;
  int sz;
  int xx, yy, n, hit;

  if (cur_eraser < NUM_ERASERS / 2)
    {
      /* Square eraser: */

      sz = (ERASER_MIN +
            (((NUM_ERASERS / 2) - 1 - cur_eraser) * ((ERASER_MAX - ERASER_MIN) / ((NUM_ERASERS / 2) - 1))));

      dest.x = x - (sz / 2);
      dest.y = y - (sz / 2);
      dest.w = sz;
      dest.h = sz;

      if (img_starter_bkgd == NULL)
        {
          SDL_FillRect(canvas, &dest, SDL_MapRGB(canvas->format, canvas_color_r, canvas_color_g, canvas_color_b));
        }
      else
        {
          SDL_BlitSurface(img_starter_bkgd, &dest, canvas, &dest);
        }
    }
  else
    {
      /* Round eraser: */

      sz = (ERASER_MIN +
            (((NUM_ERASERS / 2) - 1 - (cur_eraser - (NUM_ERASERS / 2))) *
             ((ERASER_MAX - ERASER_MIN) / ((NUM_ERASERS / 2) - 1))));

      for (yy = 0; yy < sz; yy++)
        {
          hit = 0;
          for (xx = 0; xx <= sz && hit == 0; xx++)
            {
              n = (xx * xx) + (yy * yy) - ((sz / 2) * (sz / 2));

              if (n >= -sz && n <= sz)
                hit = 1;

              if (hit)
                {
                  dest.x = x - xx;
                  dest.y = y - yy;
                  dest.w = xx * 2;
                  dest.h = 1;

                  if (img_starter_bkgd == NULL)
                    {
                      SDL_FillRect(canvas, &dest, SDL_MapRGB(canvas->format,
                                                             canvas_color_r, canvas_color_g, canvas_color_b));
                    }
                  else
                    {
                      SDL_BlitSurface(img_starter_bkgd, &dest, canvas, &dest);
                    }


                  dest.x = x - xx;
                  dest.y = y + yy;
                  dest.w = xx * 2;
                  dest.h = 1;

                  if (img_starter_bkgd == NULL)
                    {
                      SDL_FillRect(canvas, &dest, SDL_MapRGB(canvas->format,
                                                             canvas_color_r, canvas_color_g, canvas_color_b));
                    }
                  else
                    {
                      SDL_BlitSurface(img_starter_bkgd, &dest, canvas, &dest);
                    }
                }
            }
        }
    }


#ifndef NOSOUND
  if (!mute && use_sound)
    {
      if (!Mix_Playing(0))
        {
          eraser_sound = (eraser_sound + 1) % 2;

          playsound(screen, 0, SND_ERASER1 + eraser_sound, 0, x, SNDDIST_NEAR);
        }
    }
#endif

  update_canvas(x - sz / 2, y - sz / 2, x + sz / 2, y + sz / 2);

  rect_xor(x - sz / 2, y - sz / 2, x + sz / 2, y + sz / 2);

#ifdef __APPLE__
  /* Prevent ghosted eraser outlines from remaining on the screen in windowed mode */
  update_screen(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
#endif
}


/* Reset available tools (for new image / starting out): */

static void reset_avail_tools(void)
{
  int i;
  int disallow_print = disable_print;   /* set to 1 later if printer unavail */

  for (i = 0; i < NUM_TOOLS; i++)
    {
      tool_avail[i] = 1;
    }


  /* Unavailable at the beginning of a new canvas: */

  tool_avail[TOOL_UNDO] = 0;
  tool_avail[TOOL_REDO] = 0;

  if (been_saved)
    tool_avail[TOOL_SAVE] = 0;


  /* Unavailable in rare circumstances: */

  if (num_stamps[0] == 0)
    tool_avail[TOOL_STAMP] = 0;

  if (num_magics == 0)
    tool_avail[TOOL_MAGIC] = 0;


  /* Disable quit? */

  if (disable_quit)
    tool_avail[TOOL_QUIT] = 0;


  /* Disable Label? */

  if (disable_label)
    tool_avail[TOOL_LABEL] = 0;


  /* TBD... */

  tool_avail[TOOL_NA] = 0;


  /* Disable save? */

  if (disable_save)
    tool_avail[TOOL_SAVE] = 0;


#ifdef WIN32
  if (!IsPrinterAvailable())
    disallow_print = 1;
#endif

#if defined __BEOS__
  if (!IsPrinterAvailable())
    disallow_print = disable_print = 1;
#endif

#if defined __ANDROID__
  if (!IsPrinterAvailable())
    disallow_print = disable_print = 1;
#endif

  /* Disable print? */

  if (disallow_print)
    tool_avail[TOOL_PRINT] = 0;

#ifdef NOKIA_770
  /* There is no way for the user to enter text, so just disable this. */
  /* FIXME: Some Maemo devices have built-in keyboards now, don't they!? -bjk 2009.10.09 */
  tool_avail[TOOL_TEXT] = 0;
  tool_avail[TOOL_LABEL] = 0;
#endif
}


/* Save and disable available tools (for Open-Dialog) */

static void disable_avail_tools(void)
{
  int i;

  hide_blinking_cursor();
  for (i = 0; i < NUM_TOOLS; i++)
    {
      tool_avail_bak[i] = tool_avail[i];
      tool_avail[i] = 0;
    }
}

/* Restore and enable available tools (for End-Of-Open-Dialog) */

static void enable_avail_tools(void)
{
  int i;

  for (i = 0; i < NUM_TOOLS; i++)
    {
      tool_avail[i] = tool_avail_bak[i];
    }
}


/* For qsort() call in do_open()... */

static int compare_dirent2s(struct dirent2 *f1, struct dirent2 *f2)
{
#ifdef DEBUG
  printf("compare_dirents: %s\t%s\n", f1->f.d_name, f2->f.d_name);
#endif

  if (f1->place == f2->place)
    return (strcmp(f1->f.d_name, f2->f.d_name));
  else
    return (f1->place - f2->place);
}


/* Draw tux's text on the screen: */

static void draw_tux_text(int which_tux, const char *const str, int want_right_to_left)
{
  draw_tux_text_ex(which_tux, str, want_right_to_left, 0);
}

static int latest_tux;
static const char *latest_tux_text;
static int latest_r2l;
static Uint8 latest_locale_text;

static void redraw_tux_text(void)
{
  draw_tux_text_ex(latest_tux, latest_tux_text, latest_r2l, latest_locale_text);
}

static void draw_tux_text_ex(int which_tux, const char *const str, int want_right_to_left, Uint8 locale_text)
{
  SDL_Rect dest;
  SDL_Color black = { 0, 0, 0, 0 };
  int w;
  SDL_Surface *btn;

  latest_tux = which_tux;
  latest_tux_text = str;
  latest_r2l = want_right_to_left;
  latest_locale_text = locale_text;


  /* Remove any text-changing timer if one is running: */
  control_drawtext_timer(0, "", 0);

  /* Clear first: */
  SDL_FillRect(screen, &r_tuxarea, SDL_MapRGB(screen->format, 255, 255, 255));

  /* Draw tux: */
  dest.x = r_tuxarea.x;
  dest.y = r_tuxarea.y + r_tuxarea.h - img_tux[which_tux]->h;

  /* if he's too tall to fit, go off the bottom (not top) edge */
  if (dest.y < r_tuxarea.y)
    dest.y = r_tuxarea.y;

  /* Don't let sfx and speak buttons cover the top of Tux, either: */
  if (cur_tool == TOOL_STAMP && use_sound && !mute)
    {
      if (dest.y < r_sfx.y + r_sfx.h)
        dest.y = r_sfx.y + r_sfx.h;
    }

  SDL_BlitSurface(img_tux[which_tux], NULL, screen, &dest);

  /* Wide enough for Tux, or two stamp sound buttons (whichever's wider) */
  w = max(img_tux[which_tux]->w, img_btnsm_up->w) + 5;

  wordwrap_text_ex(str, black, w, r_tuxarea.y, r_tuxarea.w, want_right_to_left, locale_text);


  /* Draw 'sound effect' and 'speak' buttons, if we're in the Stamp tool */

  if (cur_tool == TOOL_STAMP && use_sound && !mute)
    {
      /* Sound effect: */

      if (stamp_data[stamp_group][cur_stamp[stamp_group]]->no_sound)
        btn = img_btnsm_off;
      else
        btn = img_btnsm_up;

      dest.x = 0;
      dest.y = r_tuxarea.y;

      SDL_BlitSurface(btn, NULL, screen, &dest);

      dest.x = (img_btnsm_up->w - img_sfx->w) / 2;
      dest.y = r_tuxarea.y + ((img_btnsm_up->h - img_sfx->h) / 2);

      SDL_BlitSurface(img_sfx, NULL, screen, &dest);


      /* Speak: */

      if (stamp_data[stamp_group][cur_stamp[stamp_group]]->no_descsound)
        btn = img_btnsm_off;
      else
        btn = img_btnsm_up;

      dest.x = img_btnsm_up->w;
      dest.y = r_tuxarea.y;

      SDL_BlitSurface(btn, NULL, screen, &dest);

      dest.x = img_btnsm_up->w + ((img_btnsm_up->w - img_speak->w) / 2);
      dest.y = r_tuxarea.y + ((img_btnsm_up->h - img_speak->h) / 2);

      SDL_BlitSurface(img_speak, NULL, screen, &dest);
    }

  update_screen_rect(&r_tuxarea);
}


static void wordwrap_text(const char *const str, SDL_Color color, int left, int top, int right, int want_right_to_left)
{
  wordwrap_text_ex(str, color, left, top, right, want_right_to_left, 0);
}

static void wordwrap_text_ex(const char *const str, SDL_Color color,
                             int left, int top, int right, int want_right_to_left, Uint8 locale_text)
{
  SDL_Surface *text;
  TuxPaint_Font *myfont = medium_font;
  SDL_Rect dest;

#ifdef NO_SDLPANGO
  int len;
  int x, y, j;
  unsigned int i;
  char substr[512];
  unsigned char *locale_str;
  char *tstr;
  unsigned char utf8_char[5];
  SDL_Rect src;
  int utf8_str_len, last_text_height;
  unsigned char utf8_str[512];
#else
  SDLPango_Matrix pango_color;
#endif


  if (str == NULL || str[0] == '\0')
    return;                     /* No-op! */

  if (need_own_font && (strcmp(gettext(str), str) || locale_text))
    myfont = locale_font;

  if (strcmp(str, gettext(str)) == 0)
    {
      /* String isn't translated!  Don't write right-to-left, even if our locale is an RTOL language: */
      want_right_to_left = 0;
    }


#ifndef NO_SDLPANGO
  /* Letting SDL_Pango do all this stuff! */

  sdl_color_to_pango_color(color, &pango_color);

  SDLPango_SetDefaultColor(myfont->pango_context, &pango_color);
  SDLPango_SetMinimumSize(myfont->pango_context, right - left, canvas->h - top);
  if (want_right_to_left && need_right_to_left)
    {
      SDLPango_SetBaseDirection(locale_font->pango_context, SDLPANGO_DIRECTION_RTL);
      if (only_uppercase)
        {
          char *upper_str = uppercase(gettext(str));

          SDLPango_SetText_GivenAlignment(myfont->pango_context, upper_str, -1, SDLPANGO_ALIGN_RIGHT);
          free(upper_str);
        }
      else
        SDLPango_SetText_GivenAlignment(myfont->pango_context, gettext(str), -1, SDLPANGO_ALIGN_RIGHT);
    }
  else
    {
      SDLPango_SetBaseDirection(locale_font->pango_context, SDLPANGO_DIRECTION_LTR);
      if (only_uppercase)
        {
          char *upper_str = uppercase(gettext(str));

          SDLPango_SetText_GivenAlignment(myfont->pango_context, upper_str, -1, SDLPANGO_ALIGN_LEFT);
          free(upper_str);
        }
      else
        SDLPango_SetText_GivenAlignment(myfont->pango_context, gettext(str), -1, SDLPANGO_ALIGN_LEFT);
    }

  text = SDLPango_CreateSurfaceDraw(myfont->pango_context);

  dest.x = left;
  dest.y = top;
  if (text != NULL)
    {
      SDL_BlitSurface(text, NULL, screen, &dest);
      SDL_FreeSurface(text);
    }
#else

  /* Cursor starting position: */

  x = left;
  y = top;

  last_text_height = 0;

  debug(str);
  debug(gettext(str));
  debug("...");

  if (strcmp(str, "") != 0)
    {
      if (want_right_to_left == 0)
        locale_str = (unsigned char *)strdup(gettext(str));
      else
        locale_str = (unsigned char *)textdir(gettext(str));


      /* For each UTF8 character: */

      utf8_str_len = 0;
      utf8_str[0] = '\0';

      for (i = 0; i <= strlen((char *)locale_str); i++)
        {
          if (locale_str[i] < 128)
            {
              utf8_str[utf8_str_len++] = locale_str[i];
              utf8_str[utf8_str_len] = '\0';


              /* Space?  Blit the word! (Word-wrap if necessary) */

              if (locale_str[i] == ' ' || locale_str[i] == '\0')
                {
                  if (only_uppercase)
                    {
                      char *upper_utf8_str = uppercase((char *)utf8_str);

                      text = render_text(myfont, (char *)upper_utf8_str, color);
                      free(upper_utf8_str);
                    }
                  else
                    text = render_text(myfont, (char *)utf8_str, color);

                  if (!text)
                    continue;   /* Didn't render anything... */

                  /* ----------- */
                  if (text->w > right - left)
                    {
                      /* Move left and down (if not already at left!) */

                      if (x > left)
                        {
                          if (need_right_to_left && want_right_to_left)
                            anti_carriage_return(left, right, top, top + text->h, y + text->h, x - left);

                          x = left;
                          y = y + text->h;
                        }


                      /* Junk the blitted word; it's too long! */

                      last_text_height = text->h;
                      SDL_FreeSurface(text);


                      /* For each UTF8 character: */

                      for (j = 0; j < utf8_str_len; j++)
                        {
                          /* How many bytes does this character need? */

                          if (utf8_str[j] < 128)        /* 0xxx xxxx - 1 byte */
                            {
                              utf8_char[0] = utf8_str[j];
                              utf8_char[1] = '\0';
                            }
                          else if ((utf8_str[j] & 0xE0) == 0xC0)        /* 110x xxxx - 2 bytes */
                            {
                              utf8_char[0] = utf8_str[j];
                              utf8_char[1] = utf8_str[j + 1];
                              utf8_char[2] = '\0';
                              j = j + 1;
                            }
                          else if ((utf8_str[j] & 0xF0) == 0xE0)        /* 1110 xxxx - 3 bytes */
                            {
                              utf8_char[0] = utf8_str[j];
                              utf8_char[1] = utf8_str[j + 1];
                              utf8_char[2] = utf8_str[j + 2];
                              utf8_char[3] = '\0';
                              j = j + 2;
                            }
                          else  /* 1111 0xxx - 4 bytes */
                            {
                              utf8_char[0] = utf8_str[j];
                              utf8_char[1] = utf8_str[j + 1];
                              utf8_char[2] = utf8_str[j + 2];
                              utf8_char[3] = utf8_str[j + 3];
                              utf8_char[4] = '\0';
                              j = j + 3;
                            }


                          if (utf8_char[0] != '\0')
                            {
                              text = render_text(myfont, (char *)utf8_char, color);
                              if (text != NULL)
                                {
                                  if (x + text->w > right)
                                    {
                                      if (need_right_to_left && want_right_to_left)
                                        anti_carriage_return(left, right, top, top + text->h, y + text->h, x - left);

                                      x = left;
                                      y = y + text->h;
                                    }

                                  dest.x = x;

                                  if (need_right_to_left && want_right_to_left)
                                    dest.y = top;
                                  else
                                    dest.y = y;

                                  SDL_BlitSurface(text, NULL, screen, &dest);

                                  last_text_height = text->h;

                                  x = x + text->w;

                                  SDL_FreeSurface(text);
                                }
                            }
                        }
                    }
                  else
                    {
                      /* Not too wide for one line... */

                      if (x + text->w > right)
                        {
                          /* This word needs to move down? */

                          if (need_right_to_left && want_right_to_left)
                            anti_carriage_return(left, right, top, top + text->h, y + text->h, x - left);

                          x = left;
                          y = y + text->h;
                        }

                      dest.x = x;

                      if (need_right_to_left && want_right_to_left)
                        dest.y = top;
                      else
                        dest.y = y;

                      SDL_BlitSurface(text, NULL, screen, &dest);

                      last_text_height = text->h;
                      x = x + text->w;

                      SDL_FreeSurface(text);
                    }


                  utf8_str_len = 0;
                  utf8_str[0] = '\0';
                }
            }
          else if ((locale_str[i] & 0xE0) == 0xC0)
            {
              utf8_str[utf8_str_len++] = locale_str[i];
              utf8_str[utf8_str_len++] = locale_str[i + 1];
              utf8_str[utf8_str_len] = '\0';
              i++;
            }
          else if ((locale_str[i] & 0xF0) == 0xE0)
            {
              utf8_str[utf8_str_len++] = locale_str[i];
              utf8_str[utf8_str_len++] = locale_str[i + 1];
              utf8_str[utf8_str_len++] = locale_str[i + 2];
              utf8_str[utf8_str_len] = '\0';
              i = i + 2;
            }
          else
            {
              utf8_str[utf8_str_len++] = locale_str[i];
              utf8_str[utf8_str_len++] = locale_str[i + 1];
              utf8_str[utf8_str_len++] = locale_str[i + 2];
              utf8_str[utf8_str_len++] = locale_str[i + 3];
              utf8_str[utf8_str_len] = '\0';
              i = i + 3;
            }
        }

      free(locale_str);
    }
  else if (strlen(str) != 0)
    {
      /* Truncate if too big! (sorry!) */

      {
        char *s1 = gettext(str);

        if (want_right_to_left)
          {
            char *freeme = s1;

            s1 = textdir(s1);
            free(freeme);
          }
        tstr = uppercase(s1);
        free(s1);
      }

      if (strlen(tstr) > sizeof(substr) - 1)
        tstr[sizeof(substr) - 1] = '\0';


      /* For each word... */

      for (i = 0; i < strlen(tstr); i++)
        {
          /* Figure out the word... */

          len = 0;

          for (j = i; tstr[j] != ' ' && tstr[j] != '\0'; j++)
            {
              substr[len++] = tstr[j];
            }

          substr[len++] = ' ';
          substr[len] = '\0';


          /* Render the word for display... */


          if (only_uppercase)
            {
              char *uppercase_substr = uppercase(substr);

              text = render_text(myfont, uppercase_substr, color);
              free(uppercase_substr);
            }
          else
            text = render_text(myfont, substr, color);


          /* If it won't fit on this line, move to the next! */

          if (x + text->w > right)      /* Correct? */
            {
              if (need_right_to_left && want_right_to_left)
                anti_carriage_return(left, right, top, top + text->h, y + text->h, x - left);

              x = left;
              y = y + text->h;
            }


          /* Draw the word: */

          dest.x = x;

          if (need_right_to_left && want_right_to_left)
            dest.y = top;
          else
            dest.y = y;

          SDL_BlitSurface(text, NULL, screen, &dest);


          /* Move the cursor one word's worth: */

          x = x + text->w;


          /* Free the temp. surface: */

          last_text_height = text->h;
          SDL_FreeSurface(text);


          /* Now on to the next word... */

          i = j;
        }

      free(tstr);
    }


  /* Right-justify the final line of text, in right-to-left mode: */

  if (need_right_to_left && want_right_to_left && last_text_height > 0)
    {
      src.x = left;
      src.y = top;
      src.w = x - left;
      src.h = last_text_height;

      dest.x = right - src.w;
      dest.y = top;

      SDL_BlitSurface(screen, &src, screen, &dest);

      dest.x = left;
      dest.y = top;
      dest.w = (right - left - src.w);
      dest.h = last_text_height;

      SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));
    }
#endif
}


#ifndef NOSOUND

static void playstampdesc(int chan)
{
  static SDL_Event playsound_event;

  if (chan == 2)                /* Only do this when the channel playing the stamp sfx has ended! */
    {
      debug("Stamp SFX ended. Pushing event to play description sound...");

      playsound_event.type = SDL_USEREVENT;
      playsound_event.user.code = USEREVENT_PLAYDESCSOUND;
      playsound_event.user.data1 = (void *)(intptr_t) cur_stamp[stamp_group];   //EP added (intptr_t) to avoid warning on x64

      SDL_PushEvent(&playsound_event);
    }
}

#endif


/* Load a file's sound: */

#ifndef NOSOUND

static Mix_Chunk *loadsound_extra(const char *const fname, const char *extra)
{
  char *snd_fname;
  char tmp_str[MAX_PATH], ext[5];
  Mix_Chunk *tmp_snd;


  if (strcasestr(fname, ".png") != NULL)
    {
      strcpy(ext, ".png");
    }
  else
    {
      /* Sorry, we only handle images */

      return (NULL);
    }

  /* First, check for localized version of sound: */

  snd_fname = malloc(strlen(fname) + strlen(lang_prefix) + 16);

  strcpy(snd_fname, fname);
  snprintf(tmp_str, sizeof(tmp_str), "%s_%s.ogg", extra, lang_prefix);
  strcpy((char *)strcasestr(snd_fname, ext), tmp_str);
  debug(snd_fname);
  tmp_snd = Mix_LoadWAV(snd_fname);

  if (tmp_snd == NULL)
    {
      debug("...No local version of sound (OGG)!");

      strcpy(snd_fname, fname);
      snprintf(tmp_str, sizeof(tmp_str), "%s_%s.wav", extra, lang_prefix);
      strcpy((char *)strcasestr(snd_fname, ext), tmp_str);
      debug(snd_fname);
      tmp_snd = Mix_LoadWAV(snd_fname);

      if (tmp_snd == NULL)
        {
          debug("...No local version of sound (WAV)!");

          /* Check for non-country-code locale */

          strcpy(snd_fname, fname);
          snprintf(tmp_str, sizeof(tmp_str), "%s_%s.ogg", extra, short_lang_prefix);
          strcpy((char *)strcasestr(snd_fname, ext), tmp_str);
          debug(snd_fname);
          tmp_snd = Mix_LoadWAV(snd_fname);

          if (tmp_snd == NULL)
            {
              debug("...No short local version of sound (OGG)!");

              strcpy(snd_fname, fname);
              snprintf(tmp_str, sizeof(tmp_str), "%s_%s.wav", extra, short_lang_prefix);
              strcpy((char *)strcasestr(snd_fname, ext), tmp_str);
              debug(snd_fname);
              tmp_snd = Mix_LoadWAV(snd_fname);

              if (tmp_snd == NULL)
                {
                  /* Now, check for default sound: */

                  debug("...No short local version of sound (WAV)!");

                  strcpy(snd_fname, fname);
                  snprintf(tmp_str, sizeof(tmp_str), "%s.ogg", extra);
                  strcpy((char *)strcasestr(snd_fname, ext), tmp_str);
                  debug(snd_fname);
                  tmp_snd = Mix_LoadWAV(snd_fname);

                  if (tmp_snd == NULL)
                    {
                      debug("...No default version of sound (OGG)!");

                      strcpy(snd_fname, fname);
                      snprintf(tmp_str, sizeof(tmp_str), "%s.wav", extra);
                      strcpy((char *)strcasestr(snd_fname, ext), tmp_str);
                      debug(snd_fname);
                      tmp_snd = Mix_LoadWAV(snd_fname);

                      if (tmp_snd == NULL)
                        debug("...No default version of sound (WAV)!");
                    }
                }
            }
        }
    }

  free(snd_fname);
  return (tmp_snd);
}


static Mix_Chunk *loadsound(const char *const fname)
{
  return (loadsound_extra(fname, ""));
}

static Mix_Chunk *loaddescsound(const char *const fname)
{
  return (loadsound_extra(fname, "_desc"));
}

#endif


/* Strip any trailing spaces: */

static void strip_trailing_whitespace(char *buf)
{
  unsigned i = strlen(buf);

  while (i--)
    {
      if (!isspace(buf[i]))
        break;
      buf[i] = '\0';
    }
}


/* Load a file's description: */

static char *loaddesc(const char *const fname, Uint8 * locale_text)
{
  char *txt_fname, *extptr;
  char buf[512], def_buf[512];  /* doubled to 512 per TOYAMA Shin-Ichi's requested; -bjk 2007.05.10 */
  int found, got_first;
  FILE *fi;
  int i;

  txt_fname = strdup(fname);
  *locale_text = 0;

  extptr = strcasestr(txt_fname, ".png");

#ifndef NOSVG
  if (extptr == NULL)
    extptr = strcasestr(txt_fname, ".svg");
#endif

  if (extptr != NULL)
    {
      found = 0;

      /* Set the first available language */
      for (i = 0; i < num_wished_langs && !found; i++)
        {
          strcpy((char *)extptr, ".txt");

          fi = fopen(txt_fname, "r");

          if (!fi)
            return NULL;


          got_first = 0;
          //    found = 0;

          strcpy(def_buf, "");

          do
            {
              if (fgets(buf, sizeof(buf), fi))
                {
                  if (!feof(fi))
                    {
                      strip_trailing_whitespace(buf);

                      if (!got_first)
                        {
                          /* First one is the default: */

                          strcpy(def_buf, buf);
                          got_first = 1;
                        }


                      debug(buf);


                      //      lang_prefix = lang_prefixes[langint];
                      /* See if it's the one for this locale... */

                      if ((char *)strcasestr(buf, wished_langs[i].lang_prefix) == buf)
                        {

                          debug(buf + strlen(wished_langs[i].lang_prefix));
                          if ((char *)strcasestr(buf + strlen(wished_langs[i].lang_prefix), ".utf8=") ==
                              buf + strlen(wished_langs[i].lang_prefix))
                            {
                              lang_prefix = wished_langs[i].lang_prefix;
                              short_lang_prefix = strdup(lang_prefix);
                              /* When in doubt, cut off country code */
                              if (strchr(short_lang_prefix, '_'))
                                *strchr(short_lang_prefix, '_') = '\0';

                              need_own_font = wished_langs[i].need_own_font;
                              need_right_to_left = wished_langs[i].need_right_to_left;
                              need_right_to_left_word = wished_langs[i].need_right_to_left_word;

                              found = 1;

                              debug("...FOUND!");
                            }
                        }
                    }
                }
            }
          while (!feof(fi) && !found);

          fclose(fi);

        }
      free(txt_fname);


      /* Return the string: */

      if (found)
        {
          *locale_text = 1;
          return (strdup(buf + (strlen(lang_prefix)) + 6));
        }
      else
        {
          /* No locale-specific translation; use the default (English) */

          return (strdup(def_buf));
        }
    }
  else
    {
      return NULL;
    }
}


/* Load a *.dat file */
static double loadinfo(const char *const fname, stamp_type * inf)
{
  char buf[256];
  FILE *fi;
  double ratio = 1.0;

  inf->colorable = 0;
  inf->tintable = 0;
  inf->mirrorable = 1;
  inf->flipable = 1;
  inf->tinter = TINTER_NORMAL;

  fi = fopen(fname, "r");
  if (!fi)
    return ratio;

  do
    {
      if (fgets(buf, sizeof buf, fi))
        {
          if (!feof(fi))
            {
              strip_trailing_whitespace(buf);

              if (strcmp(buf, "colorable") == 0)
                inf->colorable = 1;
              else if (strcmp(buf, "tintable") == 0)
                inf->tintable = 1;
              else if (!memcmp(buf, "scale", 5) && (isspace(buf[5]) || buf[5] == '='))
                {
                  double tmp, tmp2;
                  char *cp = buf + 6;

                  while (isspace(*cp) || *cp == '=')
                    cp++;
                  if (strchr(cp, '%'))
                    {
                      tmp = strtod(cp, NULL) / 100.0;
                      if (tmp > 0.0001 && tmp < 10000.0)
                        ratio = tmp;
                    }
                  else if (strchr(cp, '/'))
                    {
                      tmp = strtod(cp, &cp);
                      while (*cp && !isdigit(*cp))
                        cp++;
                      tmp2 = strtod(cp, NULL);
                      if (tmp > 0.0001 && tmp < 10000.0 && tmp2 > 0.0001 && tmp2 < 10000.0
                          && tmp / tmp2 > 0.0001 && tmp / tmp2 < 10000.0)
                        ratio = tmp / tmp2;
                    }
                  else if (strchr(cp, ':'))
                    {
                      tmp = strtod(cp, &cp);
                      while (*cp && !isdigit(*cp))
                        cp++;
                      tmp2 = strtod(cp, NULL);
                      if (tmp > 0.0001 && tmp < 10000.0 &&
                          tmp2 > 0.0001 && tmp2 < 10000.0 && tmp2 / tmp > 0.0001 && tmp2 / tmp < 10000.0)
                        ratio = tmp2 / tmp;
                    }
                  else
                    {
                      tmp = strtod(cp, NULL);
                      if (tmp > 0.0001 && tmp < 10000.0)
                        ratio = 1.0 / tmp;
                    }
                }
              else if (!memcmp(buf, "tinter", 6) && (isspace(buf[6]) || buf[6] == '='))
                {
                  char *cp = buf + 7;

                  while (isspace(*cp) || *cp == '=')
                    cp++;
                  if (!strcmp(cp, "anyhue"))
                    {
                      inf->tinter = TINTER_ANYHUE;
                    }
                  else if (!strcmp(cp, "narrow"))
                    {
                      inf->tinter = TINTER_NARROW;
                    }
                  else if (!strcmp(cp, "normal"))
                    {
                      inf->tinter = TINTER_NORMAL;
                    }
                  else if (!strcmp(cp, "vector"))
                    {
                      inf->tinter = TINTER_VECTOR;
                    }
                  else
                    {
                      debug(cp);
                    }

                  /* printf("tinter=%d\n", inf->tinter); */
                }
              else if (strcmp(buf, "nomirror") == 0)
                inf->mirrorable = 0;
              else if (strcmp(buf, "noflip") == 0)
                inf->flipable = 0;
            }
        }
    }
  while (!feof(fi));

  fclose(fi);
  return ratio;
}


static int SDLCALL NondefectiveBlit(SDL_Surface * src, SDL_Rect * srcrect, SDL_Surface * dst, SDL_Rect * dstrect)
{
  int dstx = 0;
  int dsty = 0;
  int srcx = 0;
  int srcy = 0;
  int srcw = src->w;
  int srch = src->h;

  Uint32(*getpixel) (SDL_Surface *, int, int) = getpixels[src->format->BytesPerPixel];
  void (*putpixel) (SDL_Surface *, int, int, Uint32) = putpixels[dst->format->BytesPerPixel];


  if (srcrect)
    {
      srcx = srcrect->x;
      srcy = srcrect->y;
      srcw = srcrect->w;
      srch = srcrect->h;
    }
  if (dstrect)
    {
      dstx = dstrect->x;
      dsty = dstrect->y;
    }
  if (dsty < 0)
    {
      srcy += -dsty;
      srch -= -dsty;
      dsty = 0;
    }
  if (dstx < 0)
    {
      srcx += -dstx;
      srcw -= -dstx;
      dstx = 0;
    }
  if (dstx + srcw > dst->w - 1)
    {
      srcw -= (dstx + srcw) - (dst->w - 1);
    }
  if (dsty + srch > dst->h - 1)
    {
      srch -= (dsty + srch) - (dst->h - 1);
    }
  if (srcw < 1 || srch < 1)
    return -1;                  /* no idea what to return if nothing done */
  srch++;                       /* "++" is a tweak to get to edges -bjk 2009.10.11 */
  while (srch--)
    {
      int i = srcw + 1;         /* "+ 1" is a tweak to get to edges -bjk 2009.10.11 */

      while (i--)
        {
          putpixel(dst, i + dstx, srch + dsty, getpixel(src, i + srcx, srch + srcy));
        }
    }

  return (0);
}


/* For the 3rd arg, pass either NondefectiveBlit or SDL_BlitSurface. */
static void autoscale_copy_smear_free(SDL_Surface * src, SDL_Surface * dst,
                                      int SDLCALL(*blit) (SDL_Surface * src,
                                                          SDL_Rect * srcrect, SDL_Surface * dst, SDL_Rect * dstrect))
{
  SDL_Surface *src1;
  SDL_Rect dest;

  /* What to do when in 640x480 mode, and loading an
     800x600 (or larger) image!? Scale it. Starters must
     be scaled too. Keep the pixels square though, filling
     in the gaps via a smear. */
  if (src->w != dst->w || src->h != dst->h)
    {
      if (src->w / (float)dst->w > src->h / (float)dst->h)
        src1 = thumbnail(src, dst->w, src->h * dst->w / src->w, 0);
      else
        src1 = thumbnail(src, src->w * dst->h / src->h, dst->h, 0);
      SDL_FreeSurface(src);
      src = src1;
    }

  dest.x = (dst->w - src->w) / 2;
  dest.y = (dst->h - src->h) / 2;
  blit(src, NULL, dst, &dest);

  if (src->w != dst->w)
    {
      /* we know that the heights match, and src is narrower */
      SDL_Rect sour;
      int i = (dst->w - src->w) / 2;

      sour.w = 1;
      sour.x = 0;
      sour.h = src->h;
      sour.y = 0;
      while (i-- > 0)
        {
          dest.x = i;
          blit(src, &sour, dst, &dest);
        }
      sour.x = src->w - 1;
      i = (dst->w - src->w) / 2 + src->w - 1;
      while (++i < dst->w)
        {
          dest.x = i;
          blit(src, &sour, dst, &dest);
        }
    }

  if (src->h != dst->h)
    {
      /* we know that the widths match, and src is shorter */
      SDL_Rect sour;
      int i = (dst->h - src->h) / 2;

      sour.w = src->w;
      sour.x = 0;
      sour.h = 1;
      sour.y = 0;
      while (i-- > 0)
        {
          dest.y = i;
          blit(src, &sour, dst, &dest);
        }
      sour.y = src->h - 1;
      i = (dst->h - src->h) / 2 + src->h - 1;
      while (++i < dst->h)
        {
          dest.y = i;
          blit(src, &sour, dst, &dest);
        }
    }

  SDL_FreeSurface(src);
}


static void load_starter_id(char *saved_id, FILE * fil)
{
  char *rname;
  char fname[FILENAME_MAX];
  FILE *fi;
  char color_tag;
  int r, g, b, tmp;

  rname = NULL;

  if (saved_id != NULL)
    {
      snprintf(fname, sizeof(fname), "saved/%s.dat", saved_id);
      rname = get_fname(fname, DIR_SAVE);

      fi = fopen(rname, "r");
    }
  else
    fi = fil;

  starter_id[0] = '\0';
  template_id[0] = '\0';

  if (fi != NULL)
    {
      if (fgets(starter_id, sizeof(starter_id), fi))
        {
          starter_id[strlen(starter_id) - 1] = '\0';

          tmp = fscanf(fi, "%d", &starter_mirrored);
          tmp = fscanf(fi, "%d", &starter_flipped);
          tmp = fscanf(fi, "%d", &starter_personal);

          do
            {
              color_tag = fgetc(fi);
            }
          while ((color_tag == '\n' || color_tag == '\r') && !feof(fi));

          if (!feof(fi) && color_tag == 'c')
            {
              tmp = fscanf(fi, "%d", &r);
              tmp = fscanf(fi, "%d", &g);
              tmp = fscanf(fi, "%d", &b);

              canvas_color_r = (Uint8) r;
              canvas_color_g = (Uint8) g;
              canvas_color_b = (Uint8) b;
            }
          else
            {
              canvas_color_r = 255;
              canvas_color_g = 255;
              canvas_color_b = 255;
            }

          do
            {
              color_tag = fgetc(fi);
            }
          while ((color_tag == '\n' || color_tag == '\r') && !feof(fi));

          if (!feof(fi) && color_tag == 'T')
            {
              tmp = fgets(template_id, sizeof(template_id), fi);
              template_id[strlen(template_id) - 1] = '\0';
              tmp = fscanf(fi, "%d", &template_personal);
              /* FIXME: Debug only? */
              printf("template = %s\n (Personal=%d)", template_id, template_personal);
            }
          if (!feof(fi) && color_tag == 'M')
            {
              starter_modified = fgetc(fi);
            }
        }
      fclose(fi);
    }
  else
    {
      canvas_color_r = 255;
      canvas_color_g = 255;
      canvas_color_b = 255;
    }

  if (saved_id != NULL)
    free(rname);
}


static SDL_Surface *load_starter_helper(char *path_and_basename, char *extension, SDL_Surface * (*load_func) (char *))
{
  char *ext;
  char fname[256];
  SDL_Surface *surf;
  int i;

  ext = strdup(extension);
  snprintf(fname, sizeof(fname), "%s.%s", path_and_basename, ext);
  surf = (*load_func) (fname);

  if (surf == NULL)
    {
      for (i = 0; i < strlen(ext); i++)
        {
          ext[i] = toupper(ext[i]);
        }
      snprintf(fname, sizeof(fname), "%s.%s", path_and_basename, ext);
      surf = (*load_func) (fname);
    }

  free(ext);

  return (surf);
}


static void load_starter(char *img_id)
{
  char *dirname;
  char fname[256];
  SDL_Surface *tmp_surf;

  /* Determine path to starter files: */

  if (starter_personal == 0)
    dirname = strdup(DATA_PREFIX "starters");
  else
    dirname = get_fname("starters", DIR_DATA);

  /* Clear them to NULL first: */
  img_starter = NULL;
  img_starter_bkgd = NULL;

  /* Load the core image: */
  tmp_surf = NULL;

#ifndef NOSVG
  if (tmp_surf == NULL)
    {
      /* Try loading an SVG */

      snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
      tmp_surf = load_starter_helper(fname, "svg", &load_svg);
    }
#endif

  if (tmp_surf == NULL)
    {
      snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
      tmp_surf = load_starter_helper(fname, "png", &IMG_Load);
    }

  if (tmp_surf == NULL)
    {
      /* Try loading a KPX */
      snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
      tmp_surf = load_starter_helper(fname, "kpx", &myIMG_Load);
    }


  if (tmp_surf != NULL)
    {
      img_starter = SDL_DisplayFormatAlpha(tmp_surf);
      //SDL_SetSurfaceAlphaMod(img_starter, SDL_ALPHA_OPAQUE);
      //SDL_SetSurfaceBlendMode(img_starter, SDL_BLENDMODE_BLEND);
      printf("QQQQ\n");
      SDL_FreeSurface(tmp_surf);
    }

  /* Try to load the a background image: */

  tmp_surf = NULL;

#ifndef NOSVG
  /* (Try SVG) */
  if (tmp_surf == NULL)
    {
      snprintf(fname, sizeof(fname), "%s/%s-back", dirname, img_id);
      tmp_surf = load_starter_helper(fname, "svg", &load_svg);
    }
#endif

  /* (JPEG) */
  if (tmp_surf == NULL)
    {
      snprintf(fname, sizeof(fname), "%s/%s-back", dirname, img_id);
      tmp_surf = load_starter_helper(fname, "jpeg", &IMG_Load);
    }

  if (tmp_surf == NULL)
    {
      /* (Then just JPG) */
      snprintf(fname, sizeof(fname), "%s/%s-back", dirname, img_id);
      tmp_surf = load_starter_helper(fname, "jpg", &IMG_Load);
    }

  /* (Failed? Try PNG next) */
  if (tmp_surf == NULL)
    {
      snprintf(fname, sizeof(fname), "%s/%s-back", dirname, img_id);
      tmp_surf = load_starter_helper(fname, "png", &IMG_Load);
    }

  if (tmp_surf != NULL)
    {
      img_starter_bkgd = SDL_DisplayFormat(tmp_surf);
      SDL_FreeSurface(tmp_surf);
    }


  /* If no background, let's try to remove all white
     (so we don't have to _REQUIRE_ users create Starters with
     transparency, if they're simple black-and-white outlines */

  if (img_starter != NULL && img_starter_bkgd == NULL)
    {
      int x, y;

      Uint32(*getpixel) (SDL_Surface *, int, int) = getpixels[img_starter->format->BytesPerPixel];
      void (*putpixel) (SDL_Surface *, int, int, Uint32) = putpixels[img_starter->format->BytesPerPixel];
      Uint32 p;
      Uint8 r, g, b, a;
      int any_transparency;

      any_transparency = 0;

      for (y = 0; y < img_starter->h && !any_transparency; y++)
        {
          for (x = 0; x < img_starter->w && !any_transparency; x++)
            {
              p = getpixel(img_starter, x, y);
              SDL_GetRGBA(p, img_starter->format, &r, &g, &b, &a);

              if (a < 255)
                any_transparency = 1;
            }
        }

      if (!any_transparency)
        {
          /* No transparency found!  We MUST check for white pixels to save
             the day! */

          for (y = 0; y < img_starter->h; y++)
            {
              for (x = 0; x < img_starter->w; x++)
                {
                  p = getpixel(img_starter, x, y);
                  SDL_GetRGBA(p, img_starter->format, &r, &g, &b, &a);

                  if (abs(r - g) < 16 && abs(r - b) < 16 && abs(b - g) < 16)
                    {
                      a = 255 - ((r + g + b) / 3);
                    }

                  p = SDL_MapRGBA(img_starter->format, r, g, b, a);
                  putpixel(img_starter, x, y, p);
                }
            }
        }
    }


  /* Scale if needed... */

  if (img_starter != NULL && (img_starter->w != canvas->w || img_starter->h != canvas->h))
    {
      tmp_surf = img_starter;

      img_starter = SDL_CreateRGBSurface(canvas->flags,
                                         canvas->w, canvas->h,
                                         tmp_surf->format->BitsPerPixel,
                                         tmp_surf->format->Rmask,
                                         tmp_surf->format->Gmask, tmp_surf->format->Bmask, tmp_surf->format->Amask);

      /* 3rd arg ignored for RGBA surfaces */
      //    SDL_SetAlpha(tmp_surf, SDL_RLEACCEL, SDL_ALPHA_OPAQUE);
      //SDL_SetSurfaceBlendMode(tmp_surf, SDL_BLENDMODE_BLEND);

      autoscale_copy_smear_free(tmp_surf, img_starter, NondefectiveBlit);
      //    SDL_SetAlpha(img_starter, SDL_RLEACCEL, SDL_ALPHA_OPAQUE);
      SDL_SetSurfaceBlendMode(img_starter, SDL_BLENDMODE_BLEND);

    }


  if (img_starter_bkgd != NULL && (img_starter_bkgd->w != canvas->w || img_starter_bkgd->h != canvas->h))
    {
      tmp_surf = img_starter_bkgd;

      img_starter_bkgd = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                              canvas->w, canvas->h,
                                              canvas->format->BitsPerPixel,
                                              canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, 0);

      autoscale_copy_smear_free(tmp_surf, img_starter_bkgd, SDL_BlitSurface);
    }

  free(dirname);
}


static void load_template(char *img_id)
{
  char *dirname;
  char fname[256];
  SDL_Surface *tmp_surf;

  /* Determine path to starter files: */

  if (template_personal == 0)
    dirname = strdup(DATA_PREFIX "templates");
  else
    dirname = get_fname("templates", DIR_SAVE);

  /* Clear them to NULL first: */
  img_starter = NULL;
  img_starter_bkgd = NULL;

  /* (Try loading a KPX) */
  snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
  tmp_surf = load_starter_helper(fname, "kpx", &myIMG_Load);

#ifndef NOSVG
  /* (Failed? Try SVG next) */
  if (tmp_surf == NULL)
    {
      snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
      tmp_surf = load_starter_helper(fname, "svg", &load_svg);
    }
#endif

  /* (JPEG) */
  if (tmp_surf == NULL)
    {
      snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
      tmp_surf = load_starter_helper(fname, "jpeg", &IMG_Load);
    }
  if (tmp_surf == NULL)
    {
      /* (Then just JPG) */
      snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
      tmp_surf = load_starter_helper(fname, "jpg", &IMG_Load);
    }

  /* (Failed? Try PNG next) */
  if (tmp_surf == NULL)
    {
      snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
      tmp_surf = load_starter_helper(fname, "png", &IMG_Load);
    }

  if (tmp_surf != NULL)
    {
      img_starter_bkgd = SDL_DisplayFormat(tmp_surf);
      SDL_FreeSurface(tmp_surf);
    }


  /* Scale if needed... */

  if (img_starter_bkgd != NULL && (img_starter_bkgd->w != canvas->w || img_starter_bkgd->h != canvas->h))
    {
      tmp_surf = img_starter_bkgd;

      img_starter_bkgd = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                              canvas->w, canvas->h,
                                              canvas->format->BitsPerPixel,
                                              canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, 0);

      autoscale_copy_smear_free(tmp_surf, img_starter_bkgd, SDL_BlitSurface);
    }

  free(dirname);
}



  /* Determine the current picture's ID: */
static void determine_id(void)
{
  char *fname;
  FILE *fi;

  fname = get_fname("current_id.txt", DIR_SAVE);

  fi = fopen(fname, "r");
  if (fi == NULL)
    {
      fprintf(stderr,
              "\nWarning: Couldn't determine the current image's ID\n"
              "%s\n" "The system error that occurred was:\n" "%s\n\n", fname, strerror(errno));
      file_id[0] = '\0';
      starter_id[0] = '\0';
      template_id[0] = '\0';
    }
  else
    {
      if (fgets(file_id, sizeof(file_id), fi))
        {
          if (strlen(file_id) > 0)
            {
              file_id[strlen(file_id) - 1] = '\0';
            }
        }
      fclose(fi);
    }

  free(fname);
}


/* Load current (if any) image: */

static void load_current(void)
{
  SDL_Surface *tmp, *org_surf;
  char *fname;
  char ftmp[1024];
  FILE *fi;

  int found_autosaved = 0;

#ifdef AUTOSAVE_GOING_BACKGROUND
  /* Look for an automatically saved file */

  snprintf(ftmp, sizeof(ftmp), "saved/%s%s", AUTOSAVED_NAME, FNAME_EXTENSION);
  fname = get_fname(ftmp, DIR_SAVE);

  fi = fopen(fname, "r");
  if (fi != NULL)
    {
      snprintf(file_id, sizeof(file_id), "%s", AUTOSAVED_NAME);
      file_id[strlen(file_id)] = '\0';
      found_autosaved = 1;
      fclose(fi);
      free(fname);
    }
#endif

  if (!found_autosaved)
    determine_id();

  /* Load that image: */

  if (file_id[0] != '\0')
    {

      start_label_node = NULL;
      current_label_node = NULL;
      first_label_node_in_redo_stack = NULL;
      highlighted_label_node = NULL;
      label_node_to_edit = NULL;
      have_to_rec_label_node = FALSE;

      snprintf(ftmp, sizeof(ftmp), "saved/%s%s", file_id, FNAME_EXTENSION);
      fname = get_fname(ftmp, DIR_SAVE);

      tmp = myIMG_Load_RWops(fname);

      if (tmp == NULL)
        {
          fprintf(stderr,
                  "\nWarning: Couldn't load any current image.\n"
                  "%s\n" "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", fname, SDL_GetError());

          file_id[0] = '\0';
          starter_id[0] = '\0';
          template_id[0] = '\0';
        }
      else
        {
          org_surf = SDL_DisplayFormat(tmp);
          autoscale_copy_smear_free(tmp, canvas, SDL_BlitSurface);

          /* First we run this for compatibility, then we will chek if
             there are data embedded in the png file */
          load_starter_id(file_id, NULL);
          if (starter_id[0] != '\0')
            {
              load_starter(starter_id);

              if (starter_mirrored)
                mirror_starter();

              if (starter_flipped)
                flip_starter();
            }
          else if (template_id[0] != '\0')
            {
              load_template(template_id);
            }

          load_embedded_data(fname, org_surf);
        }

      free(fname);
    }

  if (!found_autosaved)
    {
      been_saved = 1;
      tool_avail[TOOL_SAVE] = 0;
    }
  else
    {
      /* Set file_id to the draw that were edited when the autosave triggered */
      determine_id();
      snprintf(ftmp, sizeof(ftmp), "saved/%s%s", AUTOSAVED_NAME, FNAME_EXTENSION);
      fname = get_fname(ftmp, DIR_SAVE);
      unlink(fname);
      free(fname);

      /* The autosaved file comes from work that was not explicitely saved */
      been_saved = 0;
      tool_avail[TOOL_SAVE] = 1;
    }
}


/* Make sure we have a 'path' directory */

static int make_directory(const char *path, const char *errmsg)
{
  char *fname;
  int res;

  fname = get_fname(path, DIR_SAVE);
  res = mkdir(fname, 0755);
  if (res != 0 && errno != EEXIST)
    {
      fprintf(stderr,
              "\nError: %s:\n" "%s\n" "The error that occurred was:\n" "%s\n\n", errmsg, fname, strerror(errno));
      free(fname);
      return 0;
    }
  free(fname);
  return 1;
}

/* Save the current image to disk: */

static void save_current(void)
{
  char *fname;
  FILE *fi;

  if (!make_directory("", "Can't create user data directory"))
    {
      draw_tux_text(TUX_OOPS, strerror(errno), 0);
      return;
    }

  fname = get_fname("current_id.txt", DIR_SAVE);

  fi = fopen(fname, "w");
  if (fi == NULL)
    {
      fprintf(stderr,
              "\nError: Can't keep track of current image.\n"
              "%s\n" "The error that occurred was:\n" "%s\n\n", fname, strerror(errno));

      draw_tux_text(TUX_OOPS, strerror(errno), 0);
    }
  else
    {
      fprintf(fi, "%s\n", file_id);
      fclose(fi);
    }

  free(fname);
}


/* Prompt the user with a yes/no question: */

static int do_prompt(const char *const text, const char *const btn_yes, const char *const btn_no, int ox, int oy)
{
  return (do_prompt_image(text, btn_yes, btn_no, NULL, NULL, NULL, ox, oy));
}


static int do_prompt_snd(const char *const text, const char *const btn_yes,
                         const char *const btn_no, int snd, int ox, int oy)
{
  return (do_prompt_image_flash_snd(text, btn_yes, btn_no, NULL, NULL, NULL, 0, snd, ox, oy));
}

static int do_prompt_image(const char *const text, const char *const btn_yes,
                           const char *const btn_no, SDL_Surface * img1,
                           SDL_Surface * img2, SDL_Surface * img3, int ox, int oy)
{
  return (do_prompt_image_snd(text, btn_yes, btn_no, img1, img2, img3, SND_NONE, ox, oy));
}

static int do_prompt_image_snd(const char *const text,
                               const char *const btn_yes,
                               const char *const btn_no, SDL_Surface * img1,
                               SDL_Surface * img2, SDL_Surface * img3, int snd, int ox, int oy)
{
  return (do_prompt_image_flash_snd(text, btn_yes, btn_no, img1, img2, img3, 0, snd, ox, oy));
}

static int do_prompt_image_flash(const char *const text,
                                 const char *const btn_yes,
                                 const char *const btn_no, SDL_Surface * img1,
                                 SDL_Surface * img2, SDL_Surface * img3, int animate, int ox, int oy)
{
  return (do_prompt_image_flash_snd(text, btn_yes, btn_no, img1, img2, img3, animate, SND_NONE, ox, oy));
}

#define PROMPT_LEFT 96
#define PROMPT_W 440

static int do_prompt_image_flash_snd(const char *const text,
                                     const char *const btn_yes,
                                     const char *const btn_no,
                                     SDL_Surface * img1, SDL_Surface * img2,
                                     SDL_Surface * img3, int animate, int snd, int ox, int oy)
{
  int oox, ooy, nx, ny;
  SDL_Event event;
  SDL_Rect dest;
  int done, ans, w, counter;
  SDL_Color black = { 0, 0, 0, 0 };
  SDLKey key;
  SDLKey key_y, key_n;
  char *keystr;
  SDL_Surface *backup;

#ifndef NO_PROMPT_SHADOWS
  int i;
  SDL_Surface *alpha_surf;
#endif
  int img1_w, img2_w, img3_w, max_img_w, img_x, img_y, offset;
  SDL_Surface *img1b;
  int free_img1b;
  int txt_left, txt_right, img_left, btn_left, txt_btn_left, txt_btn_right;
  int val_x, val_y, motioner;
  int valhat_x, valhat_y, hatmotioner;

  val_x = val_y = motioner = 0;
  valhat_x = valhat_y = hatmotioner = 0;
  emulate_button_pressed = 0;

  hide_blinking_cursor();

  /* Admittedly stupid way of determining which keys can be used for
     positive and negative responses in dialogs (e.g., [Y] (for 'yes') in English) */
  keystr = textdir(gettext("Yes"));
  key_y = tolower(keystr[0]);
  free(keystr);

  keystr = textdir(gettext("No"));
  key_n = tolower(keystr[0]);
  free(keystr);


  do_setcursor(cursor_arrow);


  /* Draw button box: */

  playsound(screen, 0, SND_PROMPT, 1, SNDPOS_CENTER, SNDDIST_NEAR);

  backup = SDL_CreateRGBSurface(screen->flags, screen->w, screen->h,
                                screen->format->BitsPerPixel,
                                screen->format->Rmask,
                                screen->format->Gmask, screen->format->Bmask, screen->format->Amask);

  SDL_BlitSurface(screen, NULL, backup, NULL);

  for (w = 0; w <= 96; w = w + 2)
    {
      oox = ox - w;
      ooy = oy - w;

      nx = PROMPT_LEFT + 96 - w + PROMPTOFFSETX;
      ny = 94 + 96 - w + PROMPTOFFSETY;

      dest.x = ((nx * w) + (oox * (96 - w))) / 96;
      dest.y = ((ny * w) + (ooy * (96 - w))) / 96;
      dest.w = (PROMPT_W - 96 * 2) + w * 2;
      dest.h = w * 2;
      SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 224 - w, 224 - w, 244 - w));

      SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);

      if ((w % 8) == 0)
        SDL_Delay(1);

      if (w == 94)
        SDL_BlitSurface(backup, NULL, screen, NULL);
    }

  SDL_FreeSurface(backup);


  playsound(screen, 1, snd, 1, SNDPOS_LEFT, SNDDIST_NEAR);

#ifndef NO_PROMPT_SHADOWS
  alpha_surf = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                    (PROMPT_W - 96 * 2) + (w - 4) * 2,
                                    (w - 4) * 2,
                                    screen->format->BitsPerPixel,
                                    screen->format->Rmask,
                                    screen->format->Gmask, screen->format->Bmask, screen->format->Amask);

  if (alpha_surf != NULL)
    {

      SDL_FillRect(alpha_surf, NULL, SDL_MapRGB(alpha_surf->format, 0, 0, 0));
      SDL_SetSurfaceAlphaMod(alpha_surf, 64);


      for (i = 8; i > 0; i = i - 2)
        {
          dest.x = PROMPT_LEFT + 96 - (w - 4) + i + PROMPTOFFSETX;
          dest.y = 94 + 96 - (w - 4) + i + PROMPTOFFSETY;
          dest.w = (PROMPT_W - 96 * 2) + (w - 4) * 2;
          dest.h = (w - 4) * 2;

          SDL_BlitSurface(alpha_surf, NULL, screen, &dest);
        }

      SDL_FreeSurface(alpha_surf);
    }
#endif


  w = w - 6;

  dest.x = PROMPT_LEFT + 96 - w + PROMPTOFFSETX;
  dest.y = 94 + 96 - w + PROMPTOFFSETY;
  dest.w = (PROMPT_W - 96 * 2) + w * 2;
  dest.h = w * 2;
  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));


  /* Make sure image on the right isn't too tall!
     (Thumbnails in Open dialog are larger on larger displays, and can
     cause arrow and trashcan icons to be pushed out of the dialog window!) */

  free_img1b = 0;
  img1b = NULL;

  if (img1 != NULL)
    {
      if (img1->h > 64 && img2 != NULL /* Only scale if it matters */ )
        {
          img1b = thumbnail(img1, 80, 64, 1);
          free_img1b = 1;
        }
      else
        {
          img1b = img1;
        }
    }


  /* If we're showing any images on the right, determine the widest width
     for them: */

  offset = img1_w = img2_w = img3_w = 0;

  if (img1b != NULL)
    img1_w = img1b->w;
  if (img2 != NULL)
    img2_w = img2->w;
  if (img3 != NULL)
    img3_w = img3->w;

  max_img_w = max(img1_w, max(img2_w, img3_w));

  if (max_img_w > 0)
    offset = max_img_w + 8;


  /* Draw the question: */

  if (need_right_to_left == 0)
    {
      txt_left = (PROMPT_LEFT + 6) + PROMPTOFFSETX;
      txt_right = (PROMPT_LEFT + PROMPT_W - 5) + PROMPTOFFSETX - offset;
      img_left = (PROMPT_LEFT + PROMPT_W - 5) + PROMPTOFFSETX - max_img_w - 4;
      btn_left = (PROMPT_LEFT + 6) + PROMPTOFFSETX;
      txt_btn_left = txt_left + img_yes->w + 4;
      txt_btn_right = txt_right;
    }
  else
    {
      txt_left = (PROMPT_LEFT + 6) + PROMPTOFFSETX + offset;
      txt_right = (PROMPT_LEFT + PROMPT_W - 5) + PROMPTOFFSETX;
      img_left = (PROMPT_LEFT + 6) + PROMPTOFFSETX + 4;
      btn_left = (PROMPT_LEFT + PROMPT_W - 5) + PROMPTOFFSETX - img_yes->w - 4;
      txt_btn_left = txt_left;
      txt_btn_right = btn_left;
    }

  wordwrap_text(text, black, txt_left, 100 + PROMPTOFFSETY, txt_right, 1);


  /* Draw the images (if any, and if not animated): */

  img_x = img_left;
  img_y = 100 + PROMPTOFFSETY + 4;

  if (img1b != NULL)
    {
      dest.x = img_left + (max_img_w - img1b->w) / 2;
      dest.y = img_y;

      SDL_BlitSurface(img1b, NULL, screen, &dest);

      if (!animate)
        img_y = img_y + img1b->h + 4;
    }

  if (!animate)
    {
      if (img2 != NULL)
        {
          dest.x = img_left + (max_img_w - img2->w) / 2;
          dest.y = img_y;

          SDL_BlitSurface(img2, NULL, screen, &dest);

          img_y = img_y + img2->h + 4;
        }

      if (img3 != NULL)
        {
          dest.x = img_left + (max_img_w - img3->w) / 2;
          dest.y = img_y;

          SDL_BlitSurface(img3, NULL, screen, &dest);

          img_y = img_y + img3->h + 4;  /* unnecessary */
        }
    }


  /* Draw yes button: */

  dest.x = btn_left;
  dest.y = 178 + PROMPTOFFSETY;
  SDL_BlitSurface(img_yes, NULL, screen, &dest);


  /* (Bound to UTF8 domain, so always ask for UTF8 rendering!) */

  wordwrap_text(btn_yes, black, txt_btn_left, 183 + PROMPTOFFSETY, txt_btn_right, 1);


  /* Draw no button: */

  if (strlen(btn_no) != 0)
    {
      dest.x = btn_left;
      dest.y = 230 + PROMPTOFFSETY;
      SDL_BlitSurface(img_no, NULL, screen, &dest);

      wordwrap_text(btn_no, black, txt_btn_left, 235 + PROMPTOFFSETY, txt_btn_right, 1);
    }


  /* Draw Tux, waiting... */

  draw_tux_text(TUX_BORED, "", 0);

  SDL_Flip(screen);

  done = 0;
  counter = 0;
  ans = 0;

  do
    {
      while (SDL_PollEvent(&event))
        {
          if (event.type == SDL_QUIT)
            {
              ans = 0;
              done = 1;
            }
          else if (event.type == SDL_WINDOWEVENT)
            {
              handle_active(&event);
            }
          else if (event.type == SDL_KEYUP)
            {
              key = event.key.keysym.sym;

              handle_keymouse(key, SDL_KEYUP, 24, NULL, NULL);
            }
          else if (event.type == SDL_KEYDOWN)
            {
              key = event.key.keysym.sym;

              handle_keymouse(key, SDL_KEYDOWN, 24, NULL, NULL);


              /* FIXME: Should use SDLK_{c} instead of '{c}'?  How? */

              if (key == key_y || key == SDLK_RETURN)
                {
                  /* Y or ENTER - Yes! */

                  ans = 1;
                  done = 1;
                }
              else if (key == key_n || key == SDLK_ESCAPE || key == SDLK_AC_BACK)
                {
                  /* N or ESCAPE - No! */

                  if (strlen(btn_no) != 0)
                    {
                      ans = 0;
                      done = 1;
                    }
                  else
                    {
                      if (key == SDLK_ESCAPE || key == SDLK_AC_BACK)
                        {
                          /* ESCAPE also simply dismisses if there's no Yes/No
                             choice: */

                          ans = 1;
                          done = 1;
                        }
                    }
                }
            }
          else if (event.type == SDL_MOUSEBUTTONDOWN && valid_click(event.button.button))
            {
              if (event.button.x >= btn_left && event.button.x < btn_left + img_yes->w)
                {
                  if (event.button.y >= 178 + PROMPTOFFSETY && event.button.y < 178 + PROMPTOFFSETY + img_yes->h)
                    {
                      ans = 1;
                      done = 1;
                    }
                  else if (strlen(btn_no) != 0 &&
                           event.button.y >= 230 + PROMPTOFFSETY && event.button.y < 230 + PROMPTOFFSETY + img_no->h)
                    {
                      ans = 0;
                      done = 1;
                    }
                }
            }
          else if (event.type == SDL_MOUSEMOTION)
            {
              if (event.button.x >= btn_left &&
                  event.button.x < btn_left + img_yes->w &&
                  ((event.button.y >= 178 + PROMPTOFFSETY &&
                    event.button.y < 178 + img_yes->h + PROMPTOFFSETY) ||
                   (strlen(btn_no) != 0 &&
                    event.button.y >= 230 + PROMPTOFFSETY && event.button.y < 230 + img_yes->h + PROMPTOFFSETY)))
                {
                  do_setcursor(cursor_hand);
                }
              else
                {
                  do_setcursor(cursor_arrow);
                }
              oldpos_x = event.button.x;
              oldpos_y = event.button.y;
            }

          else if (event.type == SDL_JOYAXISMOTION)
            handle_joyaxismotion(event, &motioner, &val_x, &val_y);

          else if (event.type == SDL_JOYHATMOTION)
            handle_joyhatmotion(event, oldpos_x, oldpos_y, &valhat_x, &valhat_y, &hatmotioner, &old_hat_ticks);


          else if (event.type == SDL_JOYBALLMOTION)
            handle_joyballmotion(event, oldpos_x, oldpos_y);

          else if (event.type == SDL_JOYBUTTONDOWN)
            handle_joybuttonupdown(event, oldpos_x, oldpos_y);
        }

      if (motioner | hatmotioner)
        handle_motioners(oldpos_x, oldpos_y, motioner, hatmotioner, old_hat_ticks, val_x, val_y, valhat_x, valhat_y);


      SDL_Delay(10);

      if (animate)
        {
          counter++;

          if (counter == 5)
            {
              dest.x = img_left + (max_img_w - img2->w) / 2;
              dest.y = img_y;

              SDL_BlitSurface(img2, NULL, screen, &dest);
              SDL_Flip(screen);
            }
          else if (counter == 10)
            {
              if (img3 != NULL)
                {
                  dest.x = img_left + (max_img_w - img3->w) / 2;
                  dest.y = img_y;

                  SDL_BlitSurface(img3, NULL, screen, &dest);
                  SDL_Flip(screen);
                }
              else
                counter = 15;
            }

          if (counter == 15)
            {
              dest.x = img_left + (max_img_w - img1b->w) / 2;
              dest.y = img_y;

              SDL_BlitSurface(img1b, NULL, screen, &dest);
              SDL_Flip(screen);

              counter = 0;
            }
        }
    }
  while (!done);


  /* FIXME: Sound effect! */
  /* ... */


  /* Erase question prompt: */

  update_canvas(0, 0, canvas->w, canvas->h);

  if (free_img1b)
    SDL_FreeSurface(img1b);

  return ans;
}


/* Free memory and prepare to quit: */

static void cleanup(void)
{
  int i, j;

  for (j = 0; j < num_stamp_groups; j++)
    {
      for (i = 0; i < num_stamps[j]; i++)
        {
#ifndef NOSOUND
          if (stamp_data[j][i]->ssnd)
            {
              Mix_FreeChunk(stamp_data[j][i]->ssnd);
              stamp_data[j][i]->ssnd = NULL;
            }
          if (stamp_data[j][i]->sdesc)
            {
              Mix_FreeChunk(stamp_data[j][i]->sdesc);
              stamp_data[j][i]->sdesc = NULL;
            }
#endif
          if (stamp_data[j][i]->stxt)
            {
              free(stamp_data[j][i]->stxt);
              stamp_data[j][i]->stxt = NULL;
            }
          free_surface(&stamp_data[j][i]->thumbnail);

          free(stamp_data[j][i]->stampname);
          free(stamp_data[j][i]);
          stamp_data[j][i] = NULL;
        }
      free(stamp_data[j]);
    }
  free_surface(&active_stamp);

  free_surface_array(img_brushes, num_brushes);
  free(brushes_frames);
  free(brushes_directional);
  free(brushes_spacing);
  free_surface_array(img_tools, NUM_TOOLS);
  free_surface_array(img_tool_names, NUM_TOOLS);
  free_surface_array(img_title_names, NUM_TITLES);
  for (i = 0; i < num_magics; i++)
    {
      free_surface(&(magics[i].img_icon));
      free_surface(&(magics[i].img_name));
    }
  free_surface_array(img_shapes, NUM_SHAPES);
  free_surface_array(img_shape_names, NUM_SHAPES);
  free_surface_array(img_tux, NUM_TIP_TUX);

  free_surface(&img_openlabels_open);
  free_surface(&img_openlabels_slideshow);
  free_surface(&img_openlabels_erase);
  free_surface(&img_openlabels_back);
  free_surface(&img_openlabels_next);
  free_surface(&img_openlabels_play);

  free_surface(&img_progress);

  free_surface(&img_yes);
  free_surface(&img_no);

  free_surface(&img_prev);
  free_surface(&img_next);

  free_surface(&img_mirror);
  free_surface(&img_flip);

  free_surface(&img_title_on);
  free_surface(&img_title_off);
  free_surface(&img_title_large_on);
  free_surface(&img_title_large_off);

  free_surface(&img_open);
  free_surface(&img_erase);
  free_surface(&img_back);
  free_surface(&img_trash);

  free_surface(&img_slideshow);
  free_surface(&img_play);
  free_surface(&img_select_digits);

  free_surface(&img_dead40x40);

  free_surface(&img_printer);
  free_surface(&img_printer_wait);
  free_surface(&img_save_over);

  free_surface(&img_btn_up);
  free_surface(&img_btn_down);
  free_surface(&img_btn_off);

  free_surface(&img_btnsm_up);
  free_surface(&img_btnsm_off);
  free_surface(&img_btnsm_down);
  free_surface(&img_btnsm_hold);

  free_surface(&img_btn_nav);
  free_surface(&img_btnsm_nav);

  free_surface(&img_sfx);
  free_surface(&img_speak);

  free_surface(&img_cursor_up);
  free_surface(&img_cursor_down);

  free_surface(&img_cursor_starter_up);
  free_surface(&img_cursor_starter_down);

  free_surface(&img_scroll_up);
  free_surface(&img_scroll_down);
  free_surface(&img_scroll_up_off);
  free_surface(&img_scroll_down_off);

  free_surface(&img_grow);
  free_surface(&img_shrink);

  free_surface(&img_magic_paint);
  free_surface(&img_magic_fullscreen);

  free_surface(&img_bold);
  free_surface(&img_italic);

  free_surface_array(undo_bufs, NUM_UNDO_BUFS);

#ifdef LOW_QUALITY_COLOR_SELECTOR
  free_surface(&img_paintcan);
#else
  free_surface_array(img_color_btns, NUM_COLORS * 2);
  free(img_color_btns);
#endif

  if (onscreen_keyboard)
    {
      free_surface(&img_oskdel);
      free_surface(&img_osktab);
      free_surface(&img_oskenter);
      free_surface(&img_oskcapslock);
      free_surface(&img_oskshift);

      if (kbd)
        osk_free(kbd);
      else
        SDL_StopTextInput();
    }

  free_surface(&screen);
  free_surface(&img_starter);
  free_surface(&img_starter_bkgd);
  free_surface(&canvas);
  free_surface(&save_canvas);
  free_surface(&img_cur_brush);

  if (touched != NULL)
    {
      free(touched);
      touched = NULL;
    }

  if (medium_font != NULL)
    {
#ifdef DEBUG
      printf("cleanup: medium font\n"); //EP
#endif
      TuxPaint_Font_CloseFont(medium_font);
      medium_font = NULL;
    }

  if (small_font != NULL)
    {
#ifdef DEBUG
      printf("cleanup: small font\n");  //EP
#endif
      TuxPaint_Font_CloseFont(small_font);
      small_font = NULL;
    }

  if (large_font != NULL)
    {
#ifdef DEBUG
      printf("cleanup: large font\n");  //EP
#endif
      TuxPaint_Font_CloseFont(large_font);
      large_font = NULL;
    }

#ifdef FORKED_FONTS
  free(user_font_families);     /* we'll leak the bodies... oh well */
#else
  for (i = 0; i < num_font_families; i++)
    {
      if (user_font_families[i])
        {
          char **cpp = user_font_families[i]->filename - 1;

          if (*++cpp)
            free(*cpp);
          if (*++cpp)
            free(*cpp);
          if (*++cpp)
            free(*cpp);
          if (*++cpp)
            free(*cpp);
          if (user_font_families[i]->handle)
            TuxPaint_Font_CloseFont(user_font_families[i]->handle);
          free(user_font_families[i]->directory);
          free(user_font_families[i]->family);
          free(user_font_families[i]);
          user_font_families[i] = NULL;
        }
    }
#endif

#ifndef NOSOUND
  if (use_sound)
    {
      for (i = 0; i < NUM_SOUNDS; i++)
        {
          if (sounds[i])
            {
              Mix_FreeChunk(sounds[i]);
              sounds[i] = NULL;
            }
        }

      Mix_CloseAudio();
    }
#endif

  for (i = 0; i < num_plugin_files; i++)
    magic_funcs[i].shutdown(magic_api_struct);

  free_cursor(&cursor_hand);
  free_cursor(&cursor_arrow);
  free_cursor(&cursor_watch);
  free_cursor(&cursor_up);
  free_cursor(&cursor_down);
  free_cursor(&cursor_tiny);
  free_cursor(&cursor_crosshair);
  free_cursor(&cursor_brush);
  free_cursor(&cursor_wand);
  free_cursor(&cursor_insertion);
  free_cursor(&cursor_rotate);

  for (i = 0; i < NUM_COLORS; i++)
    {
      free(color_hexes[i]);
      free(color_names[i]);
    }
  free(color_hexes);
  free(color_names);


  /* (Just in case...) */

  //  SDL_WM_GrabInput(SDL_GRAB_OFF);
  SDL_SetWindowGrab(window_screen, SDL_FALSE);


  /* If we're using a lockfile, we can 'clear' it when we quit
     (so Tux Paint can be launched again soon, if the user wants to!) */

  if (ok_to_use_lockfile)
    {
      char *lock_fname;
      time_t zero_time;
      FILE *fi;

#ifndef WIN32
      lock_fname = get_fname("lockfile.dat", DIR_SAVE);
#else
      lock_fname = get_temp_fname("lockfile.dat");
#endif

      zero_time = (time_t) 0;

      fi = fopen(lock_fname, "w");
      if (fi != NULL)
        {
          /* If we can write to it, do so! */

          fwrite(&zero_time, sizeof(time_t), 1, fi);
          fclose(fi);
        }
      else
        {
          fprintf(stderr,
                  "\nWarning: I couldn't create the lockfile (%s)\n"
                  "The error that occurred was:\n" "%s\n\n", lock_fname, strerror(errno));
        }

      free(lock_fname);
    }

#if !defined(WIN32) && !defined(__APPLE__) && !defined(__BEOS__) && !defined(__ANDROID__)
//  if (papersize != NULL)
//    free(papersize);
#endif


  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window_screen);

  /* Close up! */

  /* FIXME: Pango contexts lying around? -bjk 2007.07.24 */

  TTF_Quit();
  SDL_Quit();

  /* Call this once only, at exit */
//EP now deprecated
/*
#if !defined(NOSVG) && !defined(OLD_SVG)
#ifdef DEBUG
  printf("rsvg_term()\n"); fflush(stdout);
#endif
  rsvg_term();
#endif
*/
}


static void free_surface(SDL_Surface ** surface_array)
{
  if (surface_array)            //EP added this line to avoid app crash
    if (*surface_array)
      {
        SDL_FreeSurface(*surface_array);
        *surface_array = NULL;
      }
}


static void free_surface_array(SDL_Surface * surface_array[], int count)
{
  int i;

  if (surface_array)            //EP added this line to avoid app crash
    for (i = 0; i < count; ++i)
      {
        free_surface(&surface_array[i]);
      }
}


/* Draw a shape! */

static void do_shape(int cx, int cy, int ox, int oy, int rotn, int use_brush)
{
  int side, angle_skip, init_ang, rx, ry, rmax, x1, y1, x2, y2, xp, yp, xv, yv, old_brush, step;
  float a1, a2, rotn_rad;
  int xx, yy;


  /* Determine radius/shape of the shape to draw: */

  old_brush = 0;

#ifdef CORNER_SHAPES
  int tmp = 0;

  if (cx > ox)
    {
      tmp = cx;
      cx = ox;
      ox = tmp;
    }

  if (cy > oy)
    {
      tmp = cy;
      cy = oy;
      oy = tmp;
    }

  x1 = cx;
  x2 = ox;
  y1 = cy;
  y2 = oy;

  cx += ((x2 - x1) / 2);
  cy += ((y2 - y1) / 2);
#endif

  rx = abs(ox - cx);
  ry = abs(oy - cy);

  /* If the shape has a 1:1 ("locked") aspect ratio, use the larger radius: */

  if (shape_locked[cur_shape])
    {
      if (rx > ry)
        ry = rx;
      else
        rx = ry;
    }


  /* Is the shape tiny?  Make it SOME size, first! */

  if (rx < 15 && ry < 15)
    {
      rx = 15;
      ry = 15;
    }


  /* Render a default brush: */

  if (use_brush)
    {
      old_brush = cur_brush;
      cur_brush = shape_brush;  /* Now only semi-ludgy! */
      render_brush();
    }


  /* Draw the shape: */

  angle_skip = 360 / shape_sides[cur_shape];

  init_ang = shape_init_ang[cur_shape];


  step = 1;

  if (dont_do_xor && !use_brush)
    {
      /* If we're in light outline mode & not drawing the shape with the brush,
         if it has lots of sides (like a circle), reduce the number of sides: */

      if (shape_sides[cur_shape] > 5)
        step = (shape_sides[cur_shape] / 8);
    }


  for (side = 0; side < shape_sides[cur_shape]; side = side + step)
    {
      a1 = (angle_skip * side + init_ang) * M_PI / 180;
      a2 = (angle_skip * (side + 1) + init_ang) * M_PI / 180;

      x1 = (int)(cos(a1) * rx);
      y1 = (int)(-sin(a1) * ry);

      x2 = (int)(cos(a2) * rx);
      y2 = (int)(-sin(a2) * ry);

      xv = (int)(cos((a1 + a2) / 2) * rx * shape_valley[cur_shape] / 100);
      yv = (int)(-sin((a1 + a2) / 2) * ry * shape_valley[cur_shape] / 100);




      /* Rotate the line: */

      if (rotn != 0)
        {
          rotn_rad = rotn * M_PI / 180;

          xp = x1 * cos(rotn_rad) - y1 * sin(rotn_rad);
          yp = x1 * sin(rotn_rad) + y1 * cos(rotn_rad);

          x1 = xp;
          y1 = yp;

          xp = x2 * cos(rotn_rad) - y2 * sin(rotn_rad);
          yp = x2 * sin(rotn_rad) + y2 * cos(rotn_rad);

          x2 = xp;
          y2 = yp;

          xp = xv * cos(rotn_rad) - yv * sin(rotn_rad);
          yp = xv * sin(rotn_rad) + yv * cos(rotn_rad);

          xv = xp;
          yv = yp;
        }


      /* Center the line around the center of the shape: */

      x1 = x1 + cx;
      y1 = y1 + cy;
      x2 = x2 + cx;
      y2 = y2 + cy;
      xv = xv + cx;
      yv = yv + cy;


      /* Draw: */

      if (!use_brush)
        {
          /* (XOR) */
          if (shape_valley[cur_shape] == 100)
            line_xor(x1, y1, x2, y2);
          else
            {
              line_xor(x1, y1, xv, yv);
              line_xor(xv, yv, x2, y2);
            }
        }
      else
        {
          if (shape_valley[cur_shape] == 100)
            /* Brush */

            brush_draw(x1, y1, x2, y2, 0);
          else
            /* Stars */
            {
              brush_draw(x1, y1, xv, yv, 0);
              brush_draw(xv, yv, x2, y2, 0);
            }
        }
    }


  if (use_brush && shape_filled[cur_shape] && rx > 0 && ry > 0)
    {
      /* FIXME: In the meantime, we'll do this lame radius-based fill: */

      for (xx = max(abs(rx), abs(ry)); xx > 0; xx--)
        {
          yy = min(xx * rx / ry, xx * ry / rx);

          for (side = 0; side < shape_sides[cur_shape]; side++)
            {
              a1 = (angle_skip * side + init_ang) * M_PI / 180;
              a2 = (angle_skip * (side + 1) + init_ang) * M_PI / 180;

              if (yy == xx * ry / rx)
                {
                  x1 = (int)(cos(a1) * xx);
                  y1 = (int)(-sin(a1) * yy);

                  x2 = (int)(cos(a2) * xx);
                  y2 = (int)(-sin(a2) * yy);

                  xv = (int)(cos((a1 + a2) / 2) * xx * shape_valley[cur_shape] / 100);
                  yv = (int)(-sin((a1 + a2) / 2) * yy * shape_valley[cur_shape] / 100);
                }
              else
                {
                  x1 = (int)(cos(a1) * yy);
                  y1 = (int)(-sin(a1) * xx);

                  x2 = (int)(cos(a2) * yy);
                  y2 = (int)(-sin(a2) * xx);

                  xv = (int)(cos((a1 + a2) / 2) * yy * shape_valley[cur_shape] / 100);
                  yv = (int)(-sin((a1 + a2) / 2) * xx * shape_valley[cur_shape] / 100);
                }

              /* Rotate the line: */

              if (rotn != 0)
                {
                  rotn_rad = rotn * M_PI / 180;

                  xp = x1 * cos(rotn_rad) - y1 * sin(rotn_rad);
                  yp = x1 * sin(rotn_rad) + y1 * cos(rotn_rad);

                  x1 = xp;
                  y1 = yp;

                  xp = x2 * cos(rotn_rad) - y2 * sin(rotn_rad);
                  yp = x2 * sin(rotn_rad) + y2 * cos(rotn_rad);

                  x2 = xp;
                  y2 = yp;

                  xp = xv * cos(rotn_rad) - yv * sin(rotn_rad);
                  yp = xv * sin(rotn_rad) + yv * cos(rotn_rad);

                  xv = xp;
                  yv = yp;
                }


              /* Center the line around the center of the shape: */

              x1 = x1 + cx;
              y1 = y1 + cy;
              x2 = x2 + cx;
              y2 = y2 + cy;
              xv = xv + cx;
              yv = yv + cy;


              /* Draw: */
              if (shape_valley[cur_shape] == 100)
                brush_draw(x1, y1, x2, y2, 0);
              else
                /* Stars */
                {
                  brush_draw(x1, y1, xv, yv, 0);
                  brush_draw(xv, yv, x2, y2, 0);
                }
            }

          if (xx % 10 == 0)
            update_canvas(0, 0, WINDOW_WIDTH - 96, (48 * 7) + 40 + HEIGHTOFFSET);
        }
    }


  /* Update it! */

  if (use_brush)
    {
      if (abs(rx) > abs(ry))
        rmax = abs(rx) + 20;
      else
        rmax = abs(ry) + 20;

      update_canvas(cx - rmax, cy - rmax, cx + rmax, cy + rmax);
    }


  /* Return to normal brush (for paint brush and line tools): */

  if (use_brush)
    {
      cur_brush = old_brush;
      render_brush();
    }
}


/* What angle is the mouse away from the center of a shape? */

static int shape_rotation(int ctr_x, int ctr_y, int ox, int oy)
{
  int deg;


  deg = (atan2(oy - ctr_y, ox - ctr_x) * 180 / M_PI);

  if (shape_radius < 50)
    deg = ((deg - 15) / 30) * 30;
  else if (shape_radius < 100)
    deg = ((deg - 7) / 15) * 15;

  if (shape_locked[cur_shape])
    {
      int angle_skip;

      angle_skip = 360 / shape_sides[cur_shape];
      deg = deg % angle_skip;
    }

  return (deg);
}


/* What angle is the mouse away from a brush drag or line draw? */

static int brush_rotation(int ctr_x, int ctr_y, int ox, int oy)
{
  int deg;

  deg = (atan2(oy - ctr_y, ox - ctr_x) * 180 / M_PI);

  return (deg);
}


/* Prompt to ask whether user wishes to save over old version of their file */
#define PROMPT_SAVE_OVER_TXT gettext_noop("Replace the picture with your changes?")

/* Positive response to saving over old version
   (like a 'File:Save' action in other applications) */
#define PROMPT_SAVE_OVER_YES gettext_noop("Yes, replace the old one!")

/* Negative response to saving over old version (saves a new image)
   (like a 'File:Save As...' action in other applications) */
#define PROMPT_SAVE_OVER_NO  gettext_noop("No, save a new file!")


/* Save the current image: */

static int do_save(int tool, int dont_show_success_results, int autosave)
{
  char *fname;
  char tmp[1024];
  SDL_Surface *thm;
  FILE *fi;

  /* Was saving completely disabled? */
  if (disable_save && !autosave)
    return 0;

  tmp_apply_uncommited_text();

  SDL_BlitSurface(canvas, NULL, save_canvas, NULL);
  SDL_BlitSurface(label, NULL, save_canvas, NULL);

  if (autosave)
    {
      /* No prompts, no progressbar, always save to autosave.png */
    }
  else if (promptless_save == SAVE_OVER_NO)
    {
      /* Never save over - _always_ save a new file! */

      get_new_file_id();
    }
  else if (promptless_save == SAVE_OVER_PROMPT)
    {
      /* Saving the same picture? */

      if (file_id[0] != '\0')
        {
          /* We sure we want to do that? */

          if (do_prompt_image_snd(PROMPT_SAVE_OVER_TXT,
                                  PROMPT_SAVE_OVER_YES,
                                  PROMPT_SAVE_OVER_NO,
                                  img_save_over, NULL, NULL, SND_AREYOUSURE,
                                  (TOOL_SAVE % 2) * 48 + 24, (TOOL_SAVE / 2) * 48 + 40 + 24) == 0)
            {
              /* No - Let's save a new picture! */

              get_new_file_id();
            }
          if (tool == TOOL_TEXT || tool == TOOL_LABEL)
            do_render_cur_text(0);
        }
      else
        {
          /* Saving a new picture: */

          get_new_file_id();
        }
    }
  else if (promptless_save == SAVE_OVER_ALWAYS)
    {
      if (file_id[0] == '\0')
        get_new_file_id();
    }


  /* Make sure we have a ~/.tuxpaint directory: */

  if (!autosave)
    {
      show_progress_bar(screen);
      do_setcursor(cursor_watch);
    }

  if (!make_directory("", "Can't create user data directory"))
    {
      fprintf(stderr, "Cannot save the any pictures! SORRY!\n\n");
      draw_tux_text(TUX_OOPS, strerror(errno), 0);
      return 0;
    }

  if (!autosave)
    {
      show_progress_bar(screen);
    }


  /* Make sure we have a ~/.tuxpaint/saved directory: */

  if (!make_directory("saved", "Can't create user data directory"))
    {
      fprintf(stderr, "Cannot save any pictures! SORRY!\n\n");
      draw_tux_text(TUX_OOPS, strerror(errno), 0);
      return 0;
    }

  if (!autosave)
    {
      show_progress_bar(screen);
    }


  /* Make sure we have a ~/.tuxpaint/saved/.thumbs/ directory: */

  if (!make_directory("saved/.thumbs", "Can't create user data thumbnail directory"))
    {
      fprintf(stderr, "Cannot save any pictures! SORRY!\n\n");
      draw_tux_text(TUX_OOPS, strerror(errno), 0);
      return 0;
    }

  if (!autosave)
    {
      show_progress_bar(screen);
    }


  /* Save the file: */

  if (autosave)
    snprintf(tmp, sizeof(tmp), "saved/AUTOSAVED%s", FNAME_EXTENSION);
  else
    snprintf(tmp, sizeof(tmp), "saved/%s%s", file_id, FNAME_EXTENSION);
  fname = get_fname(tmp, DIR_SAVE);
  debug(fname);

  fi = fopen(fname, "wb");
  if (fi == NULL)
    {
      fprintf(stderr,
              "\nError: Couldn't save the current image!\n"
              "%s\n" "The system error that occurred was:\n" "%s\n\n", fname, strerror(errno));

      draw_tux_text(TUX_OOPS, strerror(errno), 0);
    }
  else
    {
      if (!do_png_save(fi, fname, save_canvas, 1))
        {
          free(fname);
          return 0;
        }
    }

  free(fname);


  if (autosave)
    /* No more process needed */
    return 1;

  show_progress_bar(screen);


  /* Save thumbnail, too: */

  /* (Was thumbnail in old directory, rather than under .thumbs?) */

  snprintf(tmp, sizeof(tmp), "saved/%s-t%s", file_id, FNAME_EXTENSION);
  fname = get_fname(tmp, DIR_SAVE);
  fi = fopen(fname, "r");
  if (fi != NULL)
    {
      fclose(fi);
    }
  else
    {
      /* No old thumbnail!  Save this image's thumbnail in the new place,
         under ".thumbs" */

      snprintf(tmp, sizeof(tmp), "saved/.thumbs/%s-t%s", file_id, FNAME_EXTENSION);
      fname = get_fname(tmp, DIR_SAVE);
    }

  debug(fname);

  thm = thumbnail(save_canvas, THUMB_W - 20, THUMB_H - 20, 0);

  fi = fopen(fname, "wb");
  if (fi == NULL)
    {
      fprintf(stderr, "\nError: Couldn't save thumbnail of image!\n"
              "%s\n" "The system error that occurred was:\n" "%s\n\n", fname, strerror(errno));
    }
  else
    {
      do_png_save(fi, fname, thm, 0);
    }
  SDL_FreeSurface(thm);

  free(fname);

#if 0                           /* No more writing the .dat file */
  /* Write 'starter' and/or canvas color info, if it's useful to: */

  if (starter_id[0] != '\0' ||
      template_id[0] != '\0' || canvas_color_r != 255 || canvas_color_g != 255 || canvas_color_b != 255)
    {
      snprintf(tmp, sizeof(tmp), "saved/%s.dat", file_id);
      fname = get_fname(tmp, DIR_SAVE);
      fi = fopen(fname, "w");
      if (fi != NULL)
        {
          fprintf(fi, "%s\n", starter_id);
          fprintf(fi, "%d %d %d\n", starter_mirrored, starter_flipped, starter_personal);
          fprintf(fi, "c%d %d %d\n", canvas_color_r, canvas_color_g, canvas_color_b);
          fprintf(fi, "T%s\n", template_id);
          fprintf(fi, "%d\n", template_personal);
          fclose(fi);
        }

      free(fname);
    }
#endif

  /* All happy! */

  playsound(screen, 0, SND_SAVE, 1, SNDPOS_CENTER, SNDDIST_NEAR);

  if (!dont_show_success_results)
    {
      draw_tux_text(TUX_DEFAULT, tool_tips[TOOL_SAVE], 1);
      do_setcursor(cursor_arrow);
    }

  undo_tmp_applied_text();

  return 1;
}

static void set_chunk_data(unsigned char **chunk_data, size_t * chunk_data_len, size_t uncompressed_size, Bytef * data,
                           size_t dataLen)
{
  int headersLen;
  unsigned int i;
  char *line, *headers, *cdata;

  headersLen = 0;
  headers = calloc(256, 1);
  line = calloc(256, 1);

  strcat(headers, "Tuxpaint\n");
  strcat(headers, "Tuxpaint_" VER_VERSION "\n");
  sprintf(line, "%d%s", uncompressed_size, "\n");
  strcat(headers, line);
  sprintf(line, "%d%s", dataLen, "\n");
  strcat(headers, line);

  headersLen = strlen(headers);
  *chunk_data_len = headersLen + dataLen;

  cdata = calloc(*chunk_data_len, sizeof(unsigned char *));
  strcat(cdata, headers);

  for (i = 0; i < dataLen; i++)
    cdata[headersLen + i] = data[i];
  *chunk_data = (unsigned char *)cdata;

  free(line);
  free(headers);
}

static void do_png_embed_data(png_structp png_ptr)
{

  /* Embedding data and labels in the png file */

  /*
     Tuxpaint chunks:

     bKGD background color

     Custom chunks:

     tpDT -> 0 the traditional .dat file
     tpFG -> 1 the starter foreground surface with the transparent pixels cleaned up
     tpBG -> 2 the starter background surface cleared from what is covered by the foreground to compress better
     tpLD -> 3 the label diff
     tpLL -> 4 the label data

     Except in tpDT, the data of all other custom chunks will be compressed

     Chunk data must have a header to avoid conflicts with other software that may use similar names
     Headers are composed by four fields delimited with "\n" :
     The string "Tuxpaint" to easy identify them as Tuxpaint chunks.
     A string identifying the sofware who created it, in our case "Tuxpaint_"VER_VERSION No spaces allowed
     The size of the uncompressed data.
     The sizeof the compressed data following. These two are only relevant for compressed chunks
     After the fourth "\n" comes the data itself
   */
  int x, y;
  Uint8 r, g, b, a;

  png_unknown_chunk tuxpaint_chunks[5];
  size_t size_of_uncompressed_label_data, chunk_data_len;
  unsigned char *sbk_pixs;
  uLongf compressedLen;
  unsigned char *chunk_data;
  Bytef *compressed_data;

#ifdef fmemopen_alternative
  char *fname;
#endif
  char *ldata;
  FILE *lfi;
  int list_ctr = 0;
  Uint32 pix;
  int alpha_size;
  Uint32 i;
  struct label_node *current_node;
  char *char_stream, *line;
  size_t dat_size;


  /* Starter foreground */
  if (img_starter)
    {
      printf("Saving starter... %d\n", (int)(intptr_t) img_starter);    //EP added (intptr_t) to avoid warning on x64
      sbk_pixs = malloc(img_starter->h * img_starter->w * 4);
      compressedLen = compressBound(img_starter->h * img_starter->w * 4);

      compressed_data = malloc(compressedLen * sizeof(Bytef *));

      if (SDL_MUSTLOCK(img_starter))
        SDL_LockSurface(img_starter);

      for (y = 0; y < img_starter->h; y++)
        for (x = 0; x < img_starter->w; x++)
          {
            SDL_GetRGBA(getpixels[img_starter->format->BytesPerPixel] (img_starter, x, y), img_starter->format, &r, &g,
                        &b, &a);

/* clear the transparent pixels assigning the same r g and b values */
            if (a == SDL_ALPHA_TRANSPARENT)
              {
                sbk_pixs[4 * (y * img_starter->w + x)] = SDL_ALPHA_TRANSPARENT;
                sbk_pixs[4 * (y * img_starter->w + x) + 1] = SDL_ALPHA_TRANSPARENT;
                sbk_pixs[4 * (y * img_starter->w + x) + 2] = SDL_ALPHA_TRANSPARENT;
                sbk_pixs[4 * (y * img_starter->w + x) + 3] = SDL_ALPHA_TRANSPARENT;
              }
            else
              {
                sbk_pixs[4 * (y * img_starter->w + x)] = r;
                sbk_pixs[4 * (y * img_starter->w + x) + 1] = g;
                sbk_pixs[4 * (y * img_starter->w + x) + 2] = b;
                sbk_pixs[4 * (y * img_starter->w + x) + 3] = a;
              }
          }

      if (SDL_MUSTLOCK(img_starter))
        SDL_UnlockSurface(img_starter);

      compress(compressed_data, &compressedLen, sbk_pixs, img_starter->h * img_starter->w * 4);
      set_chunk_data(&chunk_data, &chunk_data_len, img_starter->w * img_starter->h * 4, compressed_data, compressedLen);

      tuxpaint_chunks[1].data = (png_byte *) chunk_data;
      tuxpaint_chunks[1].size = chunk_data_len;
      tuxpaint_chunks[1].location = PNG_HAVE_IHDR;
      tuxpaint_chunks[1].name[0] = 't';
      tuxpaint_chunks[1].name[1] = 'p';
      tuxpaint_chunks[1].name[2] = 'F';
      tuxpaint_chunks[1].name[3] = 'G';
      tuxpaint_chunks[1].name[4] = '\0';
      png_write_chunk(png_ptr, tuxpaint_chunks[1].name, tuxpaint_chunks[1].data, tuxpaint_chunks[1].size);

      free(compressed_data);
      free(chunk_data);
      free(sbk_pixs);
    }

  /* Starter background */
  if (img_starter_bkgd)
    {
      sbk_pixs = malloc(img_starter_bkgd->w * img_starter_bkgd->h * 3);
      compressedLen = compressBound(img_starter_bkgd->h * img_starter_bkgd->w * 3);

      compressed_data = malloc(compressedLen * sizeof(Bytef *));

      if (SDL_MUSTLOCK(img_starter_bkgd))
        SDL_LockSurface(img_starter_bkgd);

      for (y = 0; y < img_starter_bkgd->h; y++)
        for (x = 0; x < img_starter_bkgd->w; x++)
          {
            SDL_GetRGB(getpixels[img_starter_bkgd->format->BytesPerPixel] (img_starter_bkgd, x, y),
                       img_starter_bkgd->format, &r, &g, &b);

            sbk_pixs[3 * (y * img_starter_bkgd->w + x)] = r;
            sbk_pixs[3 * (y * img_starter_bkgd->w + x) + 1] = g;
            sbk_pixs[3 * (y * img_starter_bkgd->w + x) + 2] = b;
          }

      /* Clear the parts covered by the foreground */
      if (img_starter)
        {
          if (SDL_MUSTLOCK(img_starter))
            SDL_LockSurface(img_starter);
          for (y = 0; y < img_starter_bkgd->h; y++)
            for (x = 0; x < img_starter_bkgd->w; x++)
              {
                SDL_GetRGBA(getpixels[img_starter->format->BytesPerPixel] (img_starter, x, y), img_starter->format, &r,
                            &g, &b, &a);

                if (a == SDL_ALPHA_OPAQUE)
                  {
                    sbk_pixs[3 * (y * img_starter_bkgd->w + x)] = SDL_ALPHA_TRANSPARENT;
                    sbk_pixs[3 * (y * img_starter_bkgd->w + x) + 1] = SDL_ALPHA_TRANSPARENT;
                    sbk_pixs[3 * (y * img_starter_bkgd->w + x) + 2] = SDL_ALPHA_TRANSPARENT;
                  }
              }
          if (SDL_MUSTLOCK(img_starter))
            SDL_UnlockSurface(img_starter);
        }

      if (SDL_MUSTLOCK(img_starter_bkgd))
        SDL_UnlockSurface(img_starter_bkgd);

      printf("%d \n", (int)compressedLen);

      compress(compressed_data, &compressedLen, sbk_pixs, img_starter_bkgd->h * img_starter_bkgd->w * 3);

      set_chunk_data(&chunk_data, &chunk_data_len, img_starter_bkgd->w * img_starter_bkgd->h * 3, compressed_data,
                     compressedLen);
      printf("%d \n", (int)compressedLen);


      tuxpaint_chunks[2].data = (png_byte *) chunk_data;
      tuxpaint_chunks[2].size = chunk_data_len;
      tuxpaint_chunks[2].location = PNG_HAVE_IHDR;
      tuxpaint_chunks[2].name[0] = 't';
      tuxpaint_chunks[2].name[1] = 'p';
      tuxpaint_chunks[2].name[2] = 'B';
      tuxpaint_chunks[2].name[3] = 'G';
      tuxpaint_chunks[2].name[4] = '\0';
      png_write_chunk(png_ptr, tuxpaint_chunks[2].name, tuxpaint_chunks[2].data, tuxpaint_chunks[2].size);

      free(compressed_data);
      free(chunk_data);
      free(sbk_pixs);
    }

  /* Label:  diff from label surface to canvas surface */
  if (label && are_labels())
    {
      sbk_pixs = malloc(label->h * label->w * 4);
      compressedLen = (uLongf) compressBound(label->h * label->w * 4);
      compressed_data = malloc(compressedLen * sizeof(Bytef *));

      if (SDL_MUSTLOCK(label))
        SDL_LockSurface(label);
      if (SDL_MUSTLOCK(canvas))
        SDL_LockSurface(canvas);

      for (y = 0; y < label->h; y++)
        {
          for (x = 0; x < label->w; x++)
            {
              SDL_GetRGBA(getpixels[label->format->BytesPerPixel] (label, x, y), label->format, &r, &g, &b, &a);
              if (a != SDL_ALPHA_TRANSPARENT)
                {
                  SDL_GetRGB(getpixels[canvas->format->BytesPerPixel] (canvas, x, y), canvas->format, &r, &g, &b);

                  sbk_pixs[4 * (y * label->w + x)] = r;
                  sbk_pixs[4 * (y * label->w + x) + 1] = g;
                  sbk_pixs[4 * (y * label->w + x) + 2] = b;
                  sbk_pixs[4 * (y * label->w + x) + 3] = SDL_ALPHA_OPAQUE;
                }
              else
                {
                  sbk_pixs[4 * (y * label->w + x)] = SDL_ALPHA_TRANSPARENT;
                  sbk_pixs[4 * (y * label->w + x) + 1] = SDL_ALPHA_TRANSPARENT;
                  sbk_pixs[4 * (y * label->w + x) + 2] = SDL_ALPHA_TRANSPARENT;
                  sbk_pixs[4 * (y * label->w + x) + 3] = SDL_ALPHA_TRANSPARENT;
                }
            }
        }

      if (SDL_MUSTLOCK(label))
        SDL_UnlockSurface(label);
      if (SDL_MUSTLOCK(canvas))
        SDL_UnlockSurface(canvas);

      compress(compressed_data, &compressedLen, sbk_pixs, canvas->h * canvas->w * 4);
      set_chunk_data(&chunk_data, &chunk_data_len, canvas->w * canvas->h * 4, compressed_data, compressedLen);

      tuxpaint_chunks[3].data = chunk_data;
      tuxpaint_chunks[3].size = chunk_data_len;
      tuxpaint_chunks[3].location = PNG_HAVE_IHDR;
      tuxpaint_chunks[3].name[0] = 't';
      tuxpaint_chunks[3].name[1] = 'p';
      tuxpaint_chunks[3].name[2] = 'L';
      tuxpaint_chunks[3].name[3] = 'D';
      tuxpaint_chunks[3].name[4] = '\0';

      png_write_chunk(png_ptr, tuxpaint_chunks[3].name, tuxpaint_chunks[3].data, tuxpaint_chunks[3].size);
      free(compressed_data);
      free(chunk_data);
      free(sbk_pixs);

      /* Label data */

#ifndef fmemopen_alternative

      lfi = open_memstream(&ldata, &size_of_uncompressed_label_data);

#else
#ifndef WIN32
      fname = get_fname("tmpfile", DIR_SAVE);
#else
      fname = get_temp_fname("tmpfile");
#endif

      lfi = fopen(fname, "wb+");

#endif

      current_node = current_label_node;
      while (current_node != NULL)
        {
          if (current_node->is_enabled && current_node->save_texttool_len > 0)
            list_ctr = list_ctr + 1;
          current_node = current_node->next_to_down_label_node;
        }

      fprintf(lfi, "%d\n", list_ctr);
      fprintf(lfi, "%d\n", r_canvas.w);
      fprintf(lfi, "%d\n\n", r_canvas.h);

      current_node = start_label_node;
      while (current_node && current_node != first_label_node_in_redo_stack)
        {
          if (current_node->is_enabled == TRUE && current_node->save_texttool_len > 0)
            {

#ifdef WIN32
              iconv_t trans;
              wchar_t *wch;
              char *conv, *conv2;
              size_t in, out;

              in = out = 1;
              conv = malloc(255);
              trans = iconv_open("UTF-8", "WCHAR_T");

              fprintf(lfi, "%u\n", current_node->save_texttool_len);
              for (i = 0; i < current_node->save_texttool_len; i++)
                {
                  conv2 = conv;
                  in = 2;
                  out = 10;
                  wch = &current_node->save_texttool_str[i];
                  iconv(trans, (char **)&wch, &in, &conv, &out);
                  conv[0] = '\0';
                  fprintf(lfi, "%s", conv2);
                }
#elif defined(__ANDROID__)
              fprintf(lfi, "%u\n", current_node->save_texttool_len);

              for (i = 0; i < current_node->save_texttool_len; i++)
                {
                  fprintf(lfi, "%d ", (int)current_node->save_texttool_str[i]);
                }
#else
              fprintf(lfi, "%u\n", current_node->save_texttool_len);

              for (i = 0; i < current_node->save_texttool_len; i++)
                {
                  fprintf(lfi, "%lc", (wint_t) current_node->save_texttool_str[i]);
                }
#endif

              fprintf(lfi, "\n");


              fprintf(lfi, "%u\n", current_node->save_color.r);
              fprintf(lfi, "%u\n", current_node->save_color.g);
              fprintf(lfi, "%u\n", current_node->save_color.b);
              fprintf(lfi, "%d\n", current_node->save_width);
              fprintf(lfi, "%d\n", current_node->save_height);
              fprintf(lfi, "%u\n", current_node->save_x);
              fprintf(lfi, "%u\n", current_node->save_y);

              if (current_node->save_font_type == NULL) /* Fonts yet setted */
                {
                  fprintf(lfi, "%d\n", current_node->save_cur_font);
                  fprintf(lfi, "%s\n", TTF_FontFaceFamilyName(getfonthandle(current_node->save_cur_font)->ttf_font));
                }
              else
                {
                  fprintf(lfi, "%d\n", 0);
                  fprintf(lfi, "%s\n", current_node->save_font_type);
                }

              fprintf(lfi, "%d\n", current_node->save_text_state);
              fprintf(lfi, "%u\n", current_node->save_text_size);

              SDL_LockSurface(current_node->label_node_surface);
              alpha_size = sizeof(Uint8);
              for (x = 0; x < current_node->save_width; x++)
                for (y = 0; y < current_node->save_height; y++)
                  {
                    pix =
                      getpixels[current_node->label_node_surface->format->BytesPerPixel] (current_node->
                                                                                          label_node_surface, x, y);
                    SDL_GetRGBA(pix, current_label_node->label_node_surface->format, &r, &g, &b, &a);
                    fwrite(&a, alpha_size, 1, lfi);

                  }
              SDL_UnlockSurface(current_node->label_node_surface);
              fprintf(lfi, "\n\n");
            }
          current_node = current_node->next_to_up_label_node;
          printf("cur %p, red %p\n", current_node, first_label_node_in_redo_stack);
        }

#ifdef fmemopen_alternative
      size_of_uncompressed_label_data = ftell(lfi);
      rewind(lfi);
      ldata = malloc(size_of_uncompressed_label_data);
      for (i = 0; i < size_of_uncompressed_label_data; i++)
        fread(&ldata[i], 1, 1, lfi);
#endif

      fclose(lfi);

      compressedLen = compressBound(size_of_uncompressed_label_data);
      compressed_data = malloc(compressedLen * sizeof(Bytef *));
      compress((Bytef *) compressed_data, &compressedLen, (unsigned char *)ldata, size_of_uncompressed_label_data);
      set_chunk_data(&chunk_data, &chunk_data_len, size_of_uncompressed_label_data, compressed_data, compressedLen);

      tuxpaint_chunks[4].data = chunk_data;
      tuxpaint_chunks[4].size = chunk_data_len;
      tuxpaint_chunks[4].location = PNG_HAVE_IHDR;
      tuxpaint_chunks[4].name[0] = 't';
      tuxpaint_chunks[4].name[1] = 'p';
      tuxpaint_chunks[4].name[2] = 'L';
      tuxpaint_chunks[4].name[3] = 'L';
      tuxpaint_chunks[4].name[4] = '\0';

      png_write_chunk(png_ptr, tuxpaint_chunks[4].name, tuxpaint_chunks[4].data, tuxpaint_chunks[4].size);

      free(compressed_data);
      free(chunk_data);
    }


  /* Write 'starter' and/or canvas color info, if it's useful to: */

  if (starter_id[0] != '\0' ||
      template_id[0] != '\0' || canvas_color_r != 255 || canvas_color_g != 255 || canvas_color_b != 255)
    {
      /* Usually the .dat data are less than 100 bytes, hope this keeps line and char_stream in the safe side */
      line = calloc(256, 1);
      char_stream = calloc(256 + sizeof(starter_id) + sizeof(template_id), 1);

      sprintf(char_stream, "%s\n", starter_id);

      sprintf(line, "%d %d %d\n", starter_mirrored, starter_flipped, starter_personal);
      strcat(char_stream, line);

      sprintf(line, "c%d %d %d\n", canvas_color_r, canvas_color_g, canvas_color_b);
      strcat(char_stream, line);

      sprintf(line, "T%s\n", template_id);
      strcat(char_stream, line);

      sprintf(line, "%d\n", template_personal);
      strcat(char_stream, line);

      sprintf(line, "M%d\n", starter_modified);
      strcat(char_stream, line);

      dat_size = strlen(char_stream);

      set_chunk_data(&chunk_data, &chunk_data_len, dat_size, (Bytef *) char_stream, dat_size);

      tuxpaint_chunks[4].data = chunk_data;
      tuxpaint_chunks[4].size = chunk_data_len;
      tuxpaint_chunks[4].location = PNG_HAVE_IHDR;
      tuxpaint_chunks[4].name[0] = 't';
      tuxpaint_chunks[4].name[1] = 'p';
      tuxpaint_chunks[4].name[2] = 'D';
      tuxpaint_chunks[4].name[3] = 'T';
      tuxpaint_chunks[4].name[4] = '\0';

      png_write_chunk(png_ptr, tuxpaint_chunks[4].name, tuxpaint_chunks[4].data, tuxpaint_chunks[4].size);

      free(char_stream);
      free(line);
      free(chunk_data);
    }
}

/* Actually save the PNG data to the file stream: */
static int do_png_save(FILE * fi, const char *const fname, SDL_Surface * surf, int embed)
{
  png_structp png_ptr;
  png_infop info_ptr;
  png_text text_ptr[4];
  unsigned char **png_rows;
  Uint8 r, g, b;
  int x, y, count;

  Uint32(*getpixel) (SDL_Surface *, int, int) = getpixels[surf->format->BytesPerPixel];


  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL)
    {
      fclose(fi);
      png_destroy_write_struct(&png_ptr, (png_infopp) NULL);

      fprintf(stderr, "\nError: Couldn't save the image!\n%s\n\n", fname);
      draw_tux_text(TUX_OOPS, strerror(errno), 0);
    }
  else
    {
      info_ptr = png_create_info_struct(png_ptr);
      if (info_ptr == NULL)
        {
          fclose(fi);
          png_destroy_write_struct(&png_ptr, (png_infopp) NULL);

          fprintf(stderr, "\nError: Couldn't save the image!\n%s\n\n", fname);
          draw_tux_text(TUX_OOPS, strerror(errno), 0);
        }
      else
        {
          if (setjmp(png_jmpbuf(png_ptr)))
            {
              fclose(fi);
              png_destroy_write_struct(&png_ptr, (png_infopp) NULL);

              fprintf(stderr, "\nError: Couldn't save the image!\n%s\n\n", fname);
              draw_tux_text(TUX_OOPS, strerror(errno), 0);

              return 0;
            }
          else
            {
              png_init_io(png_ptr, fi);

              png_set_IHDR(png_ptr, info_ptr, surf->w, surf->h, 8, PNG_COLOR_TYPE_RGB, 1, PNG_COMPRESSION_TYPE_BASE,
                           PNG_FILTER_TYPE_BASE);

              png_set_sRGB_gAMA_and_cHRM(png_ptr, info_ptr, PNG_sRGB_INTENT_PERCEPTUAL);

              /* Set headers */

              count = 0;

              /*
                 if (title != NULL && strlen(title) > 0)
                 {
                 text_ptr[count].key = "Title";
                 text_ptr[count].text = title;
                 text_ptr[count].compression = PNG_TEXT_COMPRESSION_NONE;
                 count++;
                 }
               */

              text_ptr[count].key = (png_charp) "Software";
              text_ptr[count].text = (png_charp) "Tux Paint " VER_VERSION " (" VER_DATE ")";
              text_ptr[count].compression = PNG_TEXT_COMPRESSION_NONE;
              count++;


              png_set_text(png_ptr, info_ptr, text_ptr, count);

              png_write_info(png_ptr, info_ptr);

              if (embed)
                do_png_embed_data(png_ptr);

              /* Save the picture: */

              png_rows = malloc(sizeof(char *) * surf->h);

              for (y = 0; y < surf->h; y++)
                {
                  png_rows[y] = malloc(sizeof(char) * 3 * surf->w);

                  for (x = 0; x < surf->w; x++)
                    {
                      SDL_GetRGB(getpixel(surf, x, y), surf->format, &r, &g, &b);

                      png_rows[y][x * 3 + 0] = r;
                      png_rows[y][x * 3 + 1] = g;
                      png_rows[y][x * 3 + 2] = b;
                    }
                }

              png_write_image(png_ptr, png_rows);

              for (y = 0; y < surf->h; y++)
                free(png_rows[y]);

              free(png_rows);


              png_write_end(png_ptr, NULL);
              png_destroy_write_struct(&png_ptr, &info_ptr);
              fclose(fi);

              return 1;
            }
        }
    }

  return 0;
}

/* Pick a new file ID: */
static void get_new_file_id(void)
{
  time_t t;

  t = time(NULL);

  strftime(file_id, sizeof(file_id), "%Y%m%d%H%M%S", localtime(&t));
  debug(file_id);


  /* FIXME: Show thumbnail and prompt for title: */
}


/* Handle quitting (and prompting to save, if necessary!) */

static int do_quit(int tool)
{
  int done, tmp_tool;

  done = do_prompt_snd(PROMPT_QUIT_TXT,
                       PROMPT_QUIT_YES, PROMPT_QUIT_NO, SND_AREYOUSURE,
                       (TOOL_QUIT % 2) * 48 + 24, (TOOL_QUIT / 2) * 48 + 40 + 24);

  if (done && !been_saved && !disable_save)
    {
      if (autosave_on_quit ||
          do_prompt(PROMPT_QUIT_SAVE_TXT, PROMPT_QUIT_SAVE_YES, PROMPT_QUIT_SAVE_NO, screen->w / 2, screen->h / 2))
        {
          if (do_save(tool, 1, 0))
            {
              /* Don't bug user about successful save when quitting -bjk 2007.05.15 */
              /* do_prompt(tool_tips[TOOL_SAVE], "OK", ""); */
            }
          else
            {
              /* Couldn't save!  Abort quit! */

              done = 0;
            }
        }
    }
  else
    {
      if (tool == TOOL_TEXT || tool == TOOL_LABEL)
        do_render_cur_text(0);

      /* Bring back stamp sound effects and speak buttons, if we were in
         Stamps tool: */

      tmp_tool = cur_tool;
      cur_tool = tool;
      draw_tux_text(TUX_BORED, "", 0);
      cur_tool = tmp_tool;
    }
  if (done)
    SDL_JoystickClose(joystick);
  return (done);
}



/* Open a saved image: */

#define PLACE_COLOR_PALETTE (-1)
#define PLACE_SAVED_DIR 0
#define PLACE_PERSONAL_STARTERS_DIR 1
#define PLACE_STARTERS_DIR 2
#define PLACE_PERSONAL_TEMPLATES_DIR 3
#define PLACE_TEMPLATES_DIR 4
#define NUM_PLACES_TO_LOOK 5


/* FIXME: This, do_slideshow() and do_new_dialog() should be combined
   and modularized! */

static int do_open(void)
{
  SDL_Surface *img, *img1, *img2, *org_surf;
  int things_alloced;
  SDL_Surface **thumbs = NULL;
  DIR *d;
  struct dirent *f;
  struct dirent2 *fs;
  int place;
  char *dirname[NUM_PLACES_TO_LOOK];
  char *rfname;
  char **d_names = NULL, **d_exts = NULL;
  int *d_places;
  FILE *fi;
  char fname[1024];
  int num_files, i, done, slideshow, update_list, want_erase, cur, which, num_files_in_dirs, j, any_saved_files;
  SDL_Rect dest;
  SDL_Event event;
  SDLKey key;
  Uint32 last_click_time;
  int last_click_which, last_click_button;
  int places_to_look;
  int opened_something;
  int val_x, val_y, motioner;
  int valhat_x, valhat_y, hatmotioner;

  val_x = val_y = motioner = 0;
  valhat_x = valhat_y = hatmotioner = 0;
  opened_something = 0;

  do
    {
      do_setcursor(cursor_watch);

      /* Allocate some space: */

      things_alloced = 32;

      fs = (struct dirent2 *)malloc(sizeof(struct dirent2) * things_alloced);

      num_files = 0;
      cur = 0;
      which = 0;
      slideshow = 0;
      num_files_in_dirs = 0;
      any_saved_files = 0;


      /* Open directories of images: */

      for (places_to_look = 0; places_to_look < NUM_PLACES_TO_LOOK; places_to_look++)
        {
          if (places_to_look == PLACE_SAVED_DIR)
            {
              /* Saved-images: */

              dirname[places_to_look] = get_fname("saved", DIR_SAVE);
            }
          else if (places_to_look == PLACE_PERSONAL_STARTERS_DIR)
            {
              /* Starters handled by New dialog... */
              dirname[places_to_look] = NULL;
              continue;
            }
          else if (places_to_look == PLACE_STARTERS_DIR)
            {
              /* Starters handled by New dialog... */
              dirname[places_to_look] = NULL;
              continue;
            }
          else if (places_to_look == PLACE_PERSONAL_TEMPLATES_DIR)
            {
              /* Templates handled by New dialog... */
              dirname[places_to_look] = NULL;
              continue;
            }
          else if (places_to_look == PLACE_TEMPLATES_DIR)
            {
              /* Templates handled by New dialog... */
              dirname[places_to_look] = NULL;
              continue;
            }


          /* Read directory of images and build thumbnails: */

          d = opendir(dirname[places_to_look]);

          if (d != NULL)
            {
              /* Gather list of files (for sorting): */

              do
                {
                  f = readdir(d);

                  if (f != NULL)
                    {
                      memcpy(&(fs[num_files_in_dirs].f), f, sizeof(struct dirent));
                      fs[num_files_in_dirs].place = places_to_look;

                      num_files_in_dirs++;

                      if (places_to_look == PLACE_SAVED_DIR)
                        any_saved_files = 1;

                      if (num_files_in_dirs >= things_alloced)
                        {
                          things_alloced = things_alloced + 32;

                          /* FIXME: Valgrind says this is leaked -bjk 2007.07.19 */
                          fs = (struct dirent2 *)realloc(fs, sizeof(struct dirent2) * things_alloced);
                        }
                    }
                }
              while (f != NULL);

              closedir(d);
            }
        }


      /* (Re)allocate space for the information about these files: */

      thumbs = (SDL_Surface * *)malloc(sizeof(SDL_Surface *) * num_files_in_dirs);
      d_places = (int *)malloc(sizeof(int) * num_files_in_dirs);
      d_names = (char **)malloc(sizeof(char *) * num_files_in_dirs);
      d_exts = (char **)malloc(sizeof(char *) * num_files_in_dirs);


      /* Sort: */

      qsort(fs, num_files_in_dirs, sizeof(struct dirent2), (int (*)(const void *, const void *))compare_dirent2s);


      /* Read directory of images and build thumbnails: */

      for (j = 0; j < num_files_in_dirs; j++)
        {
          f = &(fs[j].f);
          place = fs[j].place;

          show_progress_bar(screen);

          if (f != NULL)
            {
              debug(f->d_name);

              if (strcasestr(f->d_name, "-t.") == NULL && strcasestr(f->d_name, "-back.") == NULL)
                {
                  if (strcasestr(f->d_name, FNAME_EXTENSION) != NULL
                      /* Support legacy BMP files for load: */
                      || strcasestr(f->d_name, ".bmp") != NULL)
                    {
                      strcpy(fname, f->d_name);
                      if (strcasestr(fname, FNAME_EXTENSION) != NULL)
                        {
                          d_exts[num_files] = strdup(strcasestr(fname, FNAME_EXTENSION));
                          strcpy((char *)strcasestr(fname, FNAME_EXTENSION), "");
                        }

                      if (strcasestr(fname, ".bmp") != NULL)
                        {
                          d_exts[num_files] = strdup(strcasestr(fname, ".bmp"));
                          strcpy((char *)strcasestr(fname, ".bmp"), "");
                        }

                      d_names[num_files] = strdup(fname);
                      d_places[num_files] = place;


                      /* Is it the 'current' file we just loaded?
                         We'll make it the current selection! */

                      if (strcmp(d_names[num_files], file_id) == 0)
                        {
                          which = num_files;
                          cur = (which / 4) * 4;

                          /* Center the cursor (useful for when the last item is
                             selected first!) */

                          if (cur - 8 >= 0)
                            cur = cur - 8;
                          else if (cur - 4 >= 0)
                            cur = cur - 4;
                        }


                      /* Try to load thumbnail first: */

                      snprintf(fname, sizeof(fname), "%s/.thumbs/%s-t.png",
                               dirname[d_places[num_files]], d_names[num_files]);
                      debug(fname);
                      img = IMG_Load(fname);

                      if (img == NULL)
                        {
                          /* No thumbnail in the new location ("saved/.thumbs"),
                             try the old locatin ("saved/"): */

                          snprintf(fname, sizeof(fname), "%s/%s-t.png",
                                   dirname[d_places[num_files]], d_names[num_files]);
                          debug(fname);

                          img = IMG_Load(fname);
                        }

                      if (img != NULL)
                        {
                          /* Loaded the thumbnail from one or the other location */
                          show_progress_bar(screen);

                          img1 = SDL_DisplayFormat(img);
                          SDL_FreeSurface(img);

                          /* if too big, or too small in both dimensions, rescale it
                             (for now: using old thumbnail as source for high speed, low quality) */
                          if (img1->w > THUMB_W - 20 || img1->h > THUMB_H - 20
                              || (img1->w < THUMB_W - 20 && img1->h < THUMB_H - 20))
                            {
                              img2 = thumbnail(img1, THUMB_W - 20, THUMB_H - 20, 0);
                              SDL_FreeSurface(img1);
                              img1 = img2;
                            }

                          thumbs[num_files] = img1;

                          if (thumbs[num_files] == NULL)
                            {
                              fprintf(stderr,
                                      "\nError: Couldn't create a thumbnail of " "saved image!\n" "%s\n", fname);
                            }

                          num_files++;
                        }
                      else
                        {
                          /* No thumbnail - load original: */

                          /* Make sure we have a ~/.tuxpaint/saved directory: */
                          if (make_directory("saved", "Can't create user data directory"))
                            {
                              /* (Make sure we have a .../saved/.thumbs/ directory:) */
                              make_directory("saved/.thumbs", "Can't create user data thumbnail directory");
                            }


                          if (img == NULL)
                            {
                              snprintf(fname, sizeof(fname), "%s/%s", dirname[d_places[num_files]], f->d_name);
                              debug(fname);
                              img = myIMG_Load(fname);
                            }


                          show_progress_bar(screen);

                          if (img == NULL)
                            {
                              fprintf(stderr,
                                      "\nWarning: I can't open one of the saved files!\n"
                                      "%s\n"
                                      "The Simple DirectMedia Layer error that "
                                      "occurred was:\n" "%s\n\n", fname, SDL_GetError());

                              free(d_names[num_files]);
                              free(d_exts[num_files]);
                            }
                          else
                            {
                              /* Turn it into a thumbnail: */

                              img1 = SDL_DisplayFormatAlpha(img);
                              img2 = thumbnail2(img1, THUMB_W - 20, THUMB_H - 20, 0, 0);
                              SDL_FreeSurface(img1);

                              show_progress_bar(screen);

                              thumbs[num_files] = SDL_DisplayFormat(img2);
                              SDL_FreeSurface(img2);
                              if (thumbs[num_files] == NULL)
                                {
                                  fprintf(stderr,
                                          "\nError: Couldn't create a thumbnail of " "saved image!\n" "%s\n", fname);
                                }

                              SDL_FreeSurface(img);

                              show_progress_bar(screen);


                              /* Let's save this thumbnail, so we don't have to
                                 create it again next time 'Open' is called: */

                              if (d_places[num_files] == PLACE_SAVED_DIR)
                                {
                                  debug("Saving thumbnail for this one!");

                                  snprintf(fname, sizeof(fname), "%s/.thumbs/%s-t.png",
                                           dirname[d_places[num_files]], d_names[num_files]);

                                  fi = fopen(fname, "wb");
                                  if (fi == NULL)
                                    {
                                      fprintf(stderr,
                                              "\nError: Couldn't save thumbnail of "
                                              "saved image!\n"
                                              "%s\n" "The error that occurred was:\n" "%s\n\n", fname, strerror(errno));
                                    }
                                  else
                                    {
                                      do_png_save(fi, fname, thumbs[num_files], 0);
                                    }

                                  show_progress_bar(screen);
                                }


                              num_files++;
                            }
                        }
                    }
                }
              else
                {
                  /* It was a thumbnail file ("...-t.png") */
                }
            }
        }



#ifdef DEBUG
      printf("%d saved files were found!\n", num_files);
#endif



      if (num_files == 0)
        {
          do_prompt_snd(PROMPT_OPEN_NOFILES_TXT, PROMPT_OPEN_NOFILES_YES, "",
                        SND_YOUCANNOT, (TOOL_OPEN % 2) * 48 + 24, (TOOL_OPEN / 2) * 48 + 40 + 24);
        }
      else
        {
          /* Let user choose an image: */

          /* Instructions for 'Open' file dialog */
          char *freeme = textdir(gettext_noop("Choose the picture you want, " "then click “Open”."));

          draw_tux_text(TUX_BORED, freeme, 1);
          free(freeme);

          /* NOTE: cur is now set above; if file_id'th file is found, it's
             set to that file's index; otherwise, we default to '0' */

          update_list = 1;
          want_erase = 0;

          done = 0;
          slideshow = 0;

          last_click_which = -1;
          last_click_time = 0;
          last_click_button = -1;


          do_setcursor(cursor_arrow);


          do
            {
              /* Update screen: */

              if (update_list)
                {
                  /* Erase screen: */

                  dest.x = 96;
                  dest.y = 0;
                  dest.w = WINDOW_WIDTH - 96 - 96;
                  dest.h = 48 * 7 + 40 + HEIGHTOFFSET;

                  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));


                  /* Draw icons: */

                  for (i = cur; i < cur + 16 && i < num_files; i++)
                    {
                      /* Draw cursor: */

                      dest.x = THUMB_W * ((i - cur) % 4) + 96;
                      dest.y = THUMB_H * ((i - cur) / 4) + 24;

                      if (i == which)
                        {
                          SDL_BlitSurface(img_cursor_down, NULL, screen, &dest);
                          debug(d_names[i]);
                        }
                      else
                        SDL_BlitSurface(img_cursor_up, NULL, screen, &dest);



                      dest.x = THUMB_W * ((i - cur) % 4) + 96 + 10 + (THUMB_W - 20 - thumbs[i]->w) / 2;
                      dest.y = THUMB_H * ((i - cur) / 4) + 24 + 10 + (THUMB_H - 20 - thumbs[i]->h) / 2;

                      if (thumbs[i] != NULL)
                        SDL_BlitSurface(thumbs[i], NULL, screen, &dest);
                    }


                  /* Draw arrows: */

                  dest.x = (WINDOW_WIDTH - img_scroll_up->w) / 2;
                  dest.y = 0;

                  if (cur > 0)
                    SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
                  else
                    SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);

                  dest.x = (WINDOW_WIDTH - img_scroll_up->w) / 2;
                  dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48;

                  if (cur < num_files - 16)
                    SDL_BlitSurface(img_scroll_down, NULL, screen, &dest);
                  else
                    SDL_BlitSurface(img_scroll_down_off, NULL, screen, &dest);


                  /* "Open" button: */

                  dest.x = 96;
                  dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48;
                  SDL_BlitSurface(img_open, NULL, screen, &dest);

                  dest.x = 96 + (48 - img_openlabels_open->w) / 2;
                  dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - img_openlabels_open->h;
                  SDL_BlitSurface(img_openlabels_open, NULL, screen, &dest);


                  /* "Slideshow" button: */

                  dest.x = 96 + 48;
                  dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48;
                  if (any_saved_files)
                    SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
                  else
                    SDL_BlitSurface(img_btn_off, NULL, screen, &dest);

                  dest.x = 96 + 48;
                  dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48;
                  SDL_BlitSurface(img_slideshow, NULL, screen, &dest);

                  dest.x = 96 + 48 + (48 - img_openlabels_slideshow->w) / 2;
                  dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - img_openlabels_slideshow->h;
                  SDL_BlitSurface(img_openlabels_slideshow, NULL, screen, &dest);


                  /* "Back" button: */

                  dest.x = WINDOW_WIDTH - 96 - 48;
                  dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48;
                  SDL_BlitSurface(img_back, NULL, screen, &dest);

                  dest.x = WINDOW_WIDTH - 96 - 48 + (48 - img_openlabels_back->w) / 2;
                  dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - img_openlabels_back->h;
                  SDL_BlitSurface(img_openlabels_back, NULL, screen, &dest);


                  /* "Erase" button: */

                  dest.x = WINDOW_WIDTH - 96 - 48 - 48;
                  dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48;

                  if (d_places[which] != PLACE_STARTERS_DIR && d_places[which] != PLACE_PERSONAL_STARTERS_DIR)
                    SDL_BlitSurface(img_erase, NULL, screen, &dest);
                  else
                    SDL_BlitSurface(img_btn_off, NULL, screen, &dest);

                  dest.x = WINDOW_WIDTH - 96 - 48 - 48 + (48 - img_openlabels_erase->w) / 2;
                  dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - img_openlabels_erase->h;
                  SDL_BlitSurface(img_openlabels_erase, NULL, screen, &dest);


                  SDL_Flip(screen);

                  update_list = 0;
                }

              while (SDL_PollEvent(&event))
                {
                  if (event.type == SDL_QUIT)
                    {
                      done = 1;

                      /* FIXME: Handle SDL_Quit better */
                    }
                  else if (event.type == SDL_WINDOWEVENT)
                    {
                      handle_active(&event);
                    }
                  else if (event.type == SDL_KEYUP)
                    {
                      key = event.key.keysym.sym;

                      handle_keymouse(key, SDL_KEYUP, 24, NULL, NULL);
                    }
                  else if (event.type == SDL_KEYDOWN)
                    {
                      key = event.key.keysym.sym;

                      handle_keymouse(key, SDL_KEYDOWN, 24, NULL, NULL);

                      /* This was interfering with handle_keymouse above,
                         remapping from LEFT RIGHT UP DOWN to F11 F12 F8 F7 */
                      if (key == SDLK_F11)
                        {
                          if (which > 0)
                            {
                              which--;

                              if (which < cur)
                                cur = cur - 4;

                              update_list = 1;
                            }
                        }
                      else if (key == SDLK_F12)
                        {
                          if (which < num_files - 1)
                            {
                              which++;

                              if (which >= cur + 16)
                                cur = cur + 4;

                              update_list = 1;
                            }
                        }
                      else if (key == SDLK_F8)
                        {
                          if (which >= 0)
                            {
                              which = which - 4;

                              if (which < 0)
                                which = 0;

                              if (which < cur)
                                cur = cur - 4;

                              update_list = 1;
                            }
                        }
                      else if (key == SDLK_F7)
                        {
                          if (which < num_files)
                            {
                              which = which + 4;

                              if (which >= num_files)
                                which = num_files - 1;

                              if (which >= cur + 16)
                                cur = cur + 4;

                              update_list = 1;
                            }
                        }
                      else if (key == SDLK_RETURN)      /* space also conflicts with handle_keymouse || key == SDLK_SPACE) */
                        {
                          /* Open */

                          done = 1;
                          playsound(screen, 1, SND_CLICK, 1, SNDPOS_LEFT, SNDDIST_NEAR);
                        }
                      else if (key == SDLK_ESCAPE || key == SDLK_AC_BACK)
                        {
                          /* Go back: */

                          which = -1;
                          done = 1;
                          playsound(screen, 1, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
                        }
                      else if (key == SDLK_d &&
                               (event.key.keysym.mod & KMOD_CTRL) &&
                               d_places[which] != PLACE_STARTERS_DIR &&
                               d_places[which] != PLACE_PERSONAL_STARTERS_DIR && !noshortcuts)
                        {
                          /* Delete! */

                          want_erase = 1;
                        }
                    }
                  else if (event.type == SDL_MOUSEBUTTONDOWN && valid_click(event.button.button))
                    {
                      if (event.button.x >= 96 && event.button.x < WINDOW_WIDTH - 96 &&
                          event.button.y >= 24 && event.button.y < (48 * 7 + 40 + HEIGHTOFFSET - 48))
                        {
                          /* Picked an icon! */

                          int old_which = which;

                          which = ((event.button.x - 96) / (THUMB_W) + (((event.button.y - 24) / THUMB_H) * 4)) + cur;

                          if (which < num_files)
                            {
                              playsound(screen, 1, SND_BLEEP, 1, event.button.x, SNDDIST_NEAR);
                              update_list = 1;


                              if (which == last_click_which &&
                                  SDL_GetTicks() < last_click_time + 1000 && event.button.button == last_click_button)
                                {
                                  /* Double-click! */

                                  done = 1;
                                }

                              last_click_which = which;
                              last_click_time = SDL_GetTicks();
                              last_click_button = event.button.button;
                            }
                          else
                            which = old_which;
                        }
                      else if (event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2 &&
                               event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2)
                        {
                          if (event.button.y < 24)
                            {
                              /* Up scroll button: */

                              if (cur > 0)
                                {
                                  cur = cur - 4;
                                  update_list = 1;
                                  playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                                  if (cur == 0)
                                    do_setcursor(cursor_arrow);
                                }

                              if (which >= cur + 16)
                                which = which - 4;
                            }
                          else if (event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET - 48) &&
                                   event.button.y < (48 * 7 + 40 + HEIGHTOFFSET - 24))
                            {
                              /* Down scroll button: */

                              if (cur < num_files - 16)
                                {
                                  cur = cur + 4;
                                  update_list = 1;
                                  playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                                  if (cur >= num_files - 16)
                                    do_setcursor(cursor_arrow);
                                }

                              if (which < cur)
                                which = which + 4;
                            }
                        }
                      else if (event.button.x >= 96 && event.button.x < 96 + 48 &&
                               event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET) - 48 &&
                               event.button.y < (48 * 7 + 40 + HEIGHTOFFSET))
                        {
                          /* Open */

                          done = 1;
                          playsound(screen, 1, SND_CLICK, 1, SNDPOS_LEFT, SNDDIST_NEAR);
                        }
                      else if (event.button.x >= 96 + 48 && event.button.x < 96 + 48 + 48 &&
                               event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET) - 48 &&
                               event.button.y < (48 * 7 + 40 + HEIGHTOFFSET) && any_saved_files == 1)
                        {
                          /* Slideshow */

                          done = 1;
                          slideshow = 1;
                          playsound(screen, 1, SND_CLICK, 1, SNDPOS_LEFT, SNDDIST_NEAR);
                        }
                      else if (event.button.x >= (WINDOW_WIDTH - 96 - 48) &&
                               event.button.x < (WINDOW_WIDTH - 96) &&
                               event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET) - 48 &&
                               event.button.y < (48 * 7 + 40 + HEIGHTOFFSET))
                        {
                          /* Back */

                          which = -1;
                          done = 1;
                          playsound(screen, 1, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
                        }
                      else if (event.button.x >= (WINDOW_WIDTH - 96 - 48 - 48) &&
                               event.button.x < (WINDOW_WIDTH - 48 - 96) &&
                               event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET) - 48 &&
                               event.button.y < (48 * 7 + 40 + HEIGHTOFFSET) &&
                               d_places[which] != PLACE_STARTERS_DIR && d_places[which] != PLACE_PERSONAL_STARTERS_DIR)
                        {
                          /* Erase */

                          want_erase = 1;
                        }
#ifdef __ANDROID__
                      start_motion_convert(event);
#endif
                    }
                  else if (event.type == SDL_MOUSEBUTTONUP)
                    {
#ifdef __ANDROID__
                      stop_motion_convert(event);
#endif
                    }
                  else if (event.type == SDL_MOUSEWHEEL && wheely)
                    {
                      /* Scroll wheel! */

                      if (event.wheel.y > 0 && cur > 0)
                        {
                          cur = cur - 4;
                          update_list = 1;
                          playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                          if (cur == 0)
                            do_setcursor(cursor_arrow);

                          if (which >= cur + 16)
                            which = which - 4;
                        }
                      else if (event.wheel.y < 0 && cur < num_files - 16)
                        {
                          cur = cur + 4;
                          update_list = 1;
                          playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                          if (cur >= num_files - 16)
                            do_setcursor(cursor_arrow);

                          if (which < cur)
                            which = which + 4;
                        }
                    }
                  else if (event.type == SDL_MOUSEMOTION)
                    {
                      /* Deal with mouse pointer shape! */

                      if (event.button.y < 24 &&
                          event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2 &&
                          event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2 && cur > 0)
                        {
                          /* Scroll up button: */

                          do_setcursor(cursor_up);
                        }
                      else if (event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET - 48) &&
                               event.button.y < (48 * 7 + 40 + HEIGHTOFFSET - 24) &&
                               event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2 &&
                               event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2 && cur < num_files - 16)
                        {
                          /* Scroll down button: */

                          do_setcursor(cursor_down);
                        }
                      else if (((event.button.x >= 96 && event.button.x < 96 + 48 + 48) ||
                                (event.button.x >= (WINDOW_WIDTH - 96 - 48) &&
                                 event.button.x < (WINDOW_WIDTH - 96)) ||
                                (event.button.x >= (WINDOW_WIDTH - 96 - 48 - 48) &&
                                 event.button.x < (WINDOW_WIDTH - 48 - 96) &&
                                 d_places[which] != PLACE_STARTERS_DIR &&
                                 d_places[which] != PLACE_PERSONAL_STARTERS_DIR)) &&
                               event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET) - 48 &&
                               event.button.y < (48 * 7 + 40 + HEIGHTOFFSET))
                        {
                          /* One of the command buttons: */

                          do_setcursor(cursor_hand);
                        }
                      else if (event.button.x >= 96 && event.button.x < WINDOW_WIDTH - 96 &&
                               event.button.y > 24 &&
                               event.button.y < (48 * 7 + 40 + HEIGHTOFFSET) - 48 &&
                               ((((event.button.x - 96) / (THUMB_W) +
                                  (((event.button.y - 24) / THUMB_H) * 4)) + cur) < num_files))
                        {
                          /* One of the thumbnails: */

                          do_setcursor(cursor_hand);
                        }
                      else
                        {
                          /* Unclickable... */

                          do_setcursor(cursor_arrow);
                        }

#ifdef __ANDROID__
                      convert_motion_to_wheel(event);
#endif

                      oldpos_x = event.button.x;
                      oldpos_y = event.button.y;
                    }

                  else if (event.type == SDL_JOYAXISMOTION)
                    handle_joyaxismotion(event, &motioner, &val_x, &val_y);

                  else if (event.type == SDL_JOYHATMOTION)
                    handle_joyhatmotion(event, oldpos_x, oldpos_y, &valhat_x, &valhat_y, &hatmotioner, &old_hat_ticks);

                  else if (event.type == SDL_JOYBALLMOTION)
                    handle_joyballmotion(event, oldpos_x, oldpos_y);

                  else if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP)
                    handle_joybuttonupdown(event, oldpos_x, oldpos_y);
                }

              if (motioner | hatmotioner)
                handle_motioners(oldpos_x, oldpos_y, motioner, hatmotioner, old_hat_ticks, val_x, val_y, valhat_x,
                                 valhat_y);


              SDL_Delay(10);

              if (want_erase)
                {
                  want_erase = 0;

                  if (do_prompt_image_snd(PROMPT_ERASE_TXT,
                                          PROMPT_ERASE_YES, PROMPT_ERASE_NO,
                                          thumbs[which],
                                          img_popup_arrow, img_trash, SND_AREYOUSURE,
                                          WINDOW_WIDTH - 96 - 48 - 48 + 24, 48 * 7 + 40 + HEIGHTOFFSET - 48 + 24))
                    {
                      snprintf(fname, sizeof(fname), "saved/%s%s", d_names[which], d_exts[which]);

                      rfname = get_fname(fname, DIR_SAVE);

                      if (trash(rfname) == 0)
                        {
                          update_list = 1;


                          /* Delete the thumbnail, too: */

                          snprintf(fname, sizeof(fname), "saved/.thumbs/%s-t.png", d_names[which]);

                          free(rfname);
                          rfname = get_fname(fname, DIR_SAVE);

                          unlink(rfname);


                          /* Try deleting old-style thumbnail, too: */

                          snprintf(fname, sizeof(fname), "saved/%s-t.png", d_names[which]);

                          free(rfname);
                          rfname = get_fname(fname, DIR_SAVE);

                          unlink(rfname);


                          /* Delete .dat file, if any: */

                          snprintf(fname, sizeof(fname), "saved/%s.dat", d_names[which]);

                          free(rfname);
                          rfname = get_fname(fname, DIR_SAVE);

                          trash(rfname);



                          /* Move all other files up a notch: */

                          free(d_names[which]);
                          free(d_exts[which]);
                          free_surface(&thumbs[which]);

                          thumbs[which] = NULL;

                          for (i = which; i < num_files - 1; i++)
                            {
                              d_names[i] = d_names[i + 1];
                              d_exts[i] = d_exts[i + 1];
                              thumbs[i] = thumbs[i + 1];
                              d_places[i] = d_places[i + 1];
                            }

                          num_files--;


                          /* Make sure the cursor doesn't go off the end! */

                          if (which >= num_files)
                            which = num_files - 1;


                          /* Scroll up if the cursor goes off top of screen! */

                          if (which < cur && cur >= 4)
                            {
                              cur = cur - 4;
                              update_list = 1;
                            }


                          /* No files to open now? */

                          if (which < 0)
                            {
                              do_prompt_snd(PROMPT_OPEN_NOFILES_TXT,
                                            PROMPT_OPEN_NOFILES_YES, "", SND_YOUCANNOT, screen->w / 2, screen->h / 2);
                              done = 1;
                            }
                        }
                      else
                        {
                          perror(rfname);

                          do_prompt_snd("CAN'T", "OK", "", SND_YOUCANNOT, 0, 0);
                          update_list = 1;
                        }

                      free(rfname);
                    }
                  else
                    {
                      update_list = 1;
                    }
                }
            }
          while (!done);


          if (!slideshow)
            {
              /* Load the chosen picture: */

              if (which != -1)
                {
                  /* Save old one first? */

                  if (!been_saved && !disable_save)
                    {
                      if (do_prompt_image_snd(PROMPT_OPEN_SAVE_TXT,
                                              PROMPT_OPEN_SAVE_YES,
                                              PROMPT_OPEN_SAVE_NO,
                                              img_tools[TOOL_SAVE], NULL, NULL,
                                              SND_AREYOUSURE, screen->w / 2, screen->h / 2))
                        {
                          do_save(TOOL_OPEN, 1, 0);
                        }
                    }

                  /* Clean the label stuff */
                  delete_label_list(&start_label_node);
                  start_label_node = current_label_node = first_label_node_in_redo_stack = highlighted_label_node =
                    label_node_to_edit = NULL;
                  have_to_rec_label_node = FALSE;

                  SDL_FillRect(label, NULL, SDL_MapRGBA(label->format, 0, 0, 0, 0));

                  /* Figure out filename: */

                  snprintf(fname, sizeof(fname), "%s/%s%s", dirname[d_places[which]], d_names[which], d_exts[which]);
                  fi = fopen(fname, "r");
                  if (fi == NULL)
                    {
                      fprintf(stderr,
                              "\nWarning: Couldn't load the saved image! (1)\n"
                              "%s\n" "The file is missing.\n\n\n", fname);
                      do_prompt(PROMPT_OPEN_UNOPENABLE_TXT, PROMPT_OPEN_UNOPENABLE_YES, "", 0, 0);
                    }
                  fclose(fi);

                  img = myIMG_Load(fname);

                  if (img == NULL)
                    {
                      fprintf(stderr,
                              "\nWarning: Couldn't load the saved image! (2)\n"
                              "%s\n"
                              "The Simple DirectMedia Layer error that occurred "
                              "was:\n" "%s\n\n", fname, SDL_GetError());

                      do_prompt(PROMPT_OPEN_UNOPENABLE_TXT, PROMPT_OPEN_UNOPENABLE_YES, "", 0, 0);
                    }
                  else
                    {
                      free_surface(&img_starter);
                      free_surface(&img_starter_bkgd);
                      starter_mirrored = 0;
                      starter_flipped = 0;
                      starter_personal = 0;

                      org_surf = SDL_DisplayFormat(img);        /* Keep a copy of the original image
                                                                   unscaled to send to load_embedded_data */
                      autoscale_copy_smear_free(img, canvas, SDL_BlitSurface);

                      cur_undo = 0;
                      oldest_undo = 0;
                      newest_undo = 0;

                      /* Saved image: */

                      been_saved = 1;

                      strcpy(file_id, d_names[which]);
                      starter_id[0] = '\0';
                      template_id[0] = '\0';


                      /* Keep this for compatibility */
                      /* See if this saved image was based on a 'starter' */

                      load_starter_id(d_names[which], NULL);

                      if (starter_id[0] != '\0')
                        {
                          load_starter(starter_id);

                          if (starter_mirrored)
                            mirror_starter();

                          if (starter_flipped)
                            flip_starter();
                        }
                      else if (template_id[0] != '\0')
                        load_template(template_id);

                      load_embedded_data(fname, org_surf);

                      reset_avail_tools();

                      tool_avail_bak[TOOL_UNDO] = 0;
                      tool_avail_bak[TOOL_REDO] = 0;

                      opened_something = 1;
                    }
                }
            }


          update_canvas(0, 0, WINDOW_WIDTH - 96 - 96, 48 * 7 + 40 + HEIGHTOFFSET);
        }


      /* Clean up: */

      free_surface_array(thumbs, num_files);

      free(thumbs);

      for (i = 0; i < num_files; i++)
        {
          free(d_names[i]);
          free(d_exts[i]);
        }

      for (i = 0; i < NUM_PLACES_TO_LOOK; i++)
        if (dirname[i] != NULL)
          free(dirname[i]);

      free(d_names);
      free(d_exts);
      free(d_places);


      if (slideshow)
        {
          slideshow = do_slideshow();
        }
    }
  while (slideshow);

  return (opened_something);
}


/* FIXME: This, do_open() and do_new_dialog() should be combined and modularized! */

/* Slide Show Selection Screen: */

static int do_slideshow(void)
{
  SDL_Surface *img, *img1, *img2;
  int things_alloced;
  SDL_Surface **thumbs = NULL;
  DIR *d;
  struct dirent *f;
  struct dirent2 *fs;
  char *dirname;
  char **d_names = NULL, **d_exts = NULL;
  int *selected;
  int num_selected;
  FILE *fi;
  char fname[1024];
  int num_files, num_files_in_dir, i, done, update_list, cur, which, j, go_back, found, speed;
  SDL_Rect dest;
  SDL_Event event;
  SDLKey key;
  char *freeme;
  int speeds;
  float x_per, y_per;
  int xx, yy;
  SDL_Surface *btn, *blnk;
  int val_x, val_y, motioner;
  int valhat_x, valhat_y, hatmotioner;

  val_x = val_y = motioner = 0;
  valhat_x = valhat_y = hatmotioner = 0;
  do_setcursor(cursor_watch);

  /* Allocate some space: */

  things_alloced = 32;

  fs = (struct dirent2 *)malloc(sizeof(struct dirent2) * things_alloced);

  num_files_in_dir = 0;
  num_files = 0;
  cur = 0;
  which = 0;


  /* Load list of saved-images: */

  dirname = get_fname("saved", DIR_SAVE);


  /* Read directory of images and build thumbnails: */

  d = opendir(dirname);

  if (d != NULL)
    {
      /* Gather list of files (for sorting): */

      do
        {
          f = readdir(d);

          if (f != NULL)
            {
              memcpy(&(fs[num_files_in_dir].f), f, sizeof(struct dirent));
              fs[num_files_in_dir].place = PLACE_SAVED_DIR;

              num_files_in_dir++;

              if (num_files_in_dir >= things_alloced)
                {
                  things_alloced = things_alloced + 32;
                  fs = (struct dirent2 *)realloc(fs, sizeof(struct dirent2) * things_alloced);
                }
            }
        }
      while (f != NULL);

      closedir(d);
    }


  /* (Re)allocate space for the information about these files: */

  thumbs = (SDL_Surface * *)malloc(sizeof(SDL_Surface *) * num_files_in_dir);
  d_names = (char **)malloc(sizeof(char *) * num_files_in_dir);
  d_exts = (char **)malloc(sizeof(char *) * num_files_in_dir);
  selected = (int *)malloc(sizeof(int) * num_files_in_dir);


  /* Sort: */

  qsort(fs, num_files_in_dir, sizeof(struct dirent2), (int (*)(const void *, const void *))compare_dirent2s);


  /* Read directory of images and build thumbnails: */

  for (j = 0; j < num_files_in_dir; j++)
    {
      f = &(fs[j].f);

      show_progress_bar(screen);

      if (f != NULL)
        {
          debug(f->d_name);

          if (strcasestr(f->d_name, "-t.") == NULL && strcasestr(f->d_name, "-back.") == NULL)
            {
              if (strcasestr(f->d_name, FNAME_EXTENSION) != NULL
                  /* Support legacy BMP files for load: */
                  || strcasestr(f->d_name, ".bmp") != NULL)
                {
                  strcpy(fname, f->d_name);
                  if (strcasestr(fname, FNAME_EXTENSION) != NULL)
                    {
                      d_exts[num_files] = strdup(strcasestr(fname, FNAME_EXTENSION));
                      strcpy((char *)strcasestr(fname, FNAME_EXTENSION), "");
                    }

                  if (strcasestr(fname, ".bmp") != NULL)
                    {
                      d_exts[num_files] = strdup(strcasestr(fname, ".bmp"));
                      strcpy((char *)strcasestr(fname, ".bmp"), "");
                    }

                  d_names[num_files] = strdup(fname);


                  /* FIXME: Try to center list on whatever was selected
                     in do_open() when the slideshow button was clicked. */

                  /* Try to load thumbnail first: */

                  snprintf(fname, sizeof(fname), "%s/.thumbs/%s-t.png", dirname, d_names[num_files]);
                  debug("Loading thumbnail...");
                  debug(fname);
                  img = IMG_Load(fname);
                  if (img == NULL)
                    {
                      /* No thumbnail in the new location ("saved/.thumbs"),
                         try the old locatin ("saved/"): */

                      snprintf(fname, sizeof(fname), "%s/%s-t.png", dirname, d_names[num_files]);
                      debug(fname);

                      img = IMG_Load(fname);
                    }


                  if (img != NULL)
                    {
                      /* Loaded the thumbnail from one or the other location */

                      debug("Thumbnail loaded, scaling");
                      show_progress_bar(screen);

                      img1 = SDL_DisplayFormat(img);
                      SDL_FreeSurface(img);

                      if (img1 != NULL)
                        {
                          /* if too big, or too small in both dimensions, rescale it
                             (for now: using old thumbnail as source for high speed, low quality) */
                          if (img1->w > THUMB_W - 20 || img1->h > THUMB_H - 20
                              || (img1->w < THUMB_W - 20 && img1->h < THUMB_H - 20))
                            {
                              img2 = thumbnail(img1, THUMB_W - 20, THUMB_H - 20, 0);
                              SDL_FreeSurface(img1);
                              img1 = img2;
                            }

                          thumbs[num_files] = img1;

                          if (thumbs[num_files] == NULL)
                            {
                              fprintf(stderr, "\nError: Couldn't create a thumbnail of saved image!\n" "%s\n", fname);
                            }
                          else
                            num_files++;
                        }
                    }
                  else
                    {
                      /* No thumbnail - load original: */

                      /* Make sure we have a ~/.tuxpaint/saved directory: */
                      if (make_directory("saved", "Can't create user data directory"))
                        {
                          /* (Make sure we have a .../saved/.thumbs/ directory:) */
                          make_directory("saved/.thumbs", "Can't create user data thumbnail directory");
                        }

                      snprintf(fname, sizeof(fname), "%s/%s", dirname, f->d_name);

                      debug("Loading original, to make thumbnail");
                      debug(fname);
                      img = myIMG_Load(fname);


                      show_progress_bar(screen);


                      if (img == NULL)
                        {
                          fprintf(stderr,
                                  "\nWarning: I can't open one of the saved files!\n"
                                  "%s\n"
                                  "The Simple DirectMedia Layer error that "
                                  "occurred was:\n" "%s\n\n", fname, SDL_GetError());
                        }
                      else
                        {
                          /* Turn it into a thumbnail: */

                          img1 = SDL_DisplayFormatAlpha(img);
                          img2 = thumbnail2(img1, THUMB_W - 20, THUMB_H - 20, 0, 0);
                          SDL_FreeSurface(img1);

                          show_progress_bar(screen);

                          thumbs[num_files] = SDL_DisplayFormat(img2);
                          SDL_FreeSurface(img2);

                          SDL_FreeSurface(img);

                          if (thumbs[num_files] == NULL)
                            {
                              fprintf(stderr, "\nError: Couldn't create a thumbnail of saved image!\n" "%s\n", fname);
                            }
                          else
                            {
                              show_progress_bar(screen);

                              /* Let's save this thumbnail, so we don't have to
                                 create it again next time 'Open' is called: */

                              debug("Saving thumbnail for this one!");

                              snprintf(fname, sizeof(fname), "%s/.thumbs/%s-t.png", dirname, d_names[num_files]);

                              fi = fopen(fname, "wb");
                              if (fi == NULL)
                                {
                                  fprintf(stderr,
                                          "\nError: Couldn't save thumbnail of saved image!\n"
                                          "%s\n" "The error that occurred was:\n" "%s\n\n", fname, strerror(errno));
                                }
                              else
                                {
                                  do_png_save(fi, fname, thumbs[num_files], 0);
                                }

                              show_progress_bar(screen);

                              num_files++;
                            }
                        }
                    }
                }
            }
        }
    }

#ifdef DEBUG
  printf("%d saved files were found!\n", num_files);
#endif
  /* Let user choose images: */

  /* Instructions for Slideshow file dialog (FIXME: Make a #define) */
  freeme = textdir(gettext_noop("Choose the pictures you want, " "then click “Play”."));
  draw_tux_text(TUX_BORED, freeme, 1);
  free(freeme);

  /* NOTE: cur is now set above; if file_id'th file is found, it's
     set to that file's index; otherwise, we default to '0' */

  update_list = 1;

  go_back = 0;
  done = 0;

  /* FIXME: Make these global, so it sticks between views? */
  num_selected = 0;
  speed = 5;

  do_setcursor(cursor_arrow);


  do
    {
      /* Update screen: */

      if (update_list)
        {
          /* Erase screen: */

          dest.x = 96;
          dest.y = 0;
          dest.w = WINDOW_WIDTH - 96 - 96;
          dest.h = 48 * 7 + 40 + HEIGHTOFFSET;

          SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));


          /* Draw icons: */

          for (i = cur; i < cur + 16 && i < num_files; i++)
            {
              /* Draw cursor: */

              dest.x = THUMB_W * ((i - cur) % 4) + 96;
              dest.y = THUMB_H * ((i - cur) / 4) + 24;

              if (i == which)
                {
                  SDL_BlitSurface(img_cursor_down, NULL, screen, &dest);
                  debug(d_names[i]);
                }
              else
                SDL_BlitSurface(img_cursor_up, NULL, screen, &dest);

              if (thumbs[i] != NULL)
                {
                  dest.x = THUMB_W * ((i - cur) % 4) + 96 + 10 + (THUMB_W - 20 - thumbs[i]->w) / 2;
                  dest.y = THUMB_H * ((i - cur) / 4) + 24 + 10 + (THUMB_H - 20 - thumbs[i]->h) / 2;

                  SDL_BlitSurface(thumbs[i], NULL, screen, &dest);
                }

              found = -1;

              for (j = 0; j < num_selected && found == -1; j++)
                {
                  if (selected[j] == i)
                    found = j;
                }

              if (found != -1)
                {
                  dest.x = (THUMB_W * ((i - cur) % 4) + 96 + 10 + (THUMB_W - 20 - thumbs[i]->w) / 2) + thumbs[i]->w;
                  dest.y = (THUMB_H * ((i - cur) / 4) + 24 + 10 + (THUMB_H - 20 - thumbs[i]->h) / 2) + thumbs[i]->h;

                  draw_selection_digits(dest.x, dest.y, found + 1);
                }
            }


          /* Draw arrows: */

          dest.x = (WINDOW_WIDTH - img_scroll_up->w) / 2;
          dest.y = 0;

          if (cur > 0)
            SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
          else
            SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);

          dest.x = (WINDOW_WIDTH - img_scroll_up->w) / 2;
          dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48;

          if (cur < num_files - 16)
            SDL_BlitSurface(img_scroll_down, NULL, screen, &dest);
          else
            SDL_BlitSurface(img_scroll_down_off, NULL, screen, &dest);


          /* "Play" button: */

          dest.x = 96;
          dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48;
          SDL_BlitSurface(img_play, NULL, screen, &dest);

          dest.x = 96 + (48 - img_openlabels_play->w) / 2;
          dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - img_openlabels_play->h;
          SDL_BlitSurface(img_openlabels_play, NULL, screen, &dest);


          /* "Back" button: */

          dest.x = WINDOW_WIDTH - 96 - 48;
          dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48;
          SDL_BlitSurface(img_back, NULL, screen, &dest);

          dest.x = WINDOW_WIDTH - 96 - 48 + (48 - img_openlabels_back->w) / 2;
          dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - img_openlabels_back->h;
          SDL_BlitSurface(img_openlabels_back, NULL, screen, &dest);


          /* Speed control: */

          speeds = 10;
          x_per = 96.0 / speeds;
          y_per = 48.0 / speeds;

          for (i = 0; i < speeds; i++)
            {
              xx = ceil(x_per);
              yy = ceil(y_per * i);

              if (i <= speed)
                btn = thumbnail(img_btn_down, xx, yy, 0);
              else
                btn = thumbnail(img_btn_up, xx, yy, 0);

              blnk = thumbnail(img_btn_off, xx, 48 - yy, 0);

              /* FIXME: Check for NULL! */

              dest.x = 96 + 48 + (i * x_per);
              dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48;
              SDL_BlitSurface(blnk, NULL, screen, &dest);

              dest.x = 96 + 48 + (i * x_per);
              dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - (y_per * i);
              SDL_BlitSurface(btn, NULL, screen, &dest);

              SDL_FreeSurface(btn);
              SDL_FreeSurface(blnk);
            }

          SDL_Flip(screen);

          update_list = 0;
        }

      /* Was a call to SDL_WaitEvent(&event); before,
         changed to this while loop in order to get joystick working */
      while (SDL_PollEvent(&event))
        {
          if (event.type == SDL_QUIT)
            {
              done = 1;

              /* FIXME: Handle SDL_Quit better */
            }
          else if (event.type == SDL_WINDOWEVENT)
            {
              handle_active(&event);
            }
          else if (event.type == SDL_KEYUP)
            {
              key = event.key.keysym.sym;

              handle_keymouse(key, SDL_KEYUP, 24, NULL, NULL);
            }
          else if (event.type == SDL_KEYDOWN)
            {
              key = event.key.keysym.sym;


              dest.x = button_w * 3;
              dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48;
              dest.w = button_w * 2;
              dest.h = button_h;

              handle_keymouse(key, SDL_KEYDOWN, 24, &dest, NULL);

              if (key == SDLK_RETURN)
                {
                  /* Play */

                  //      done = 1;
                  //      playsound(screen, 1, SND_CLICK, 1, SNDPOS_LEFT, SNDDIST_NEAR);
                  event.type = SDL_MOUSEBUTTONDOWN;
                  event.button.x = button_w * 2 + 5;
                  event.button.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48 + 5;
                  event.button.button = 1;
                  SDL_PushEvent(&event);


                }
              else if (key == SDLK_ESCAPE || key == SDLK_AC_BACK)
                {
                  /* Go back: */

                  go_back = 1;
                  done = 1;
                  playsound(screen, 1, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
                }
            }
          else if (event.type == SDL_MOUSEBUTTONDOWN && valid_click(event.button.button))
            {
              if (event.button.x >= 96 && event.button.x < WINDOW_WIDTH - 96 &&
                  event.button.y >= 24 && event.button.y < (48 * 7 + 40 + HEIGHTOFFSET - 48))
                {
                  /* Picked an icon! */

                  which = ((event.button.x - 96) / (THUMB_W) + (((event.button.y - 24) / THUMB_H) * 4)) + cur;

                  if (which < num_files)
                    {
                      playsound(screen, 1, SND_BLEEP, 1, event.button.x, SNDDIST_NEAR);

                      /* Is it selected already? */

                      found = -1;
                      for (i = 0; i < num_selected && found == -1; i++)
                        {
                          if (selected[i] == which)
                            found = i;
                        }

                      if (found == -1)
                        {
                          /* No!  Select it! */

                          selected[num_selected++] = which;
                        }
                      else
                        {
                          /* Yes!  Unselect it! */

                          for (i = found; i < num_selected - 1; i++)
                            selected[i] = selected[i + 1];

                          num_selected--;
                        }

                      update_list = 1;
                    }
                }
              else if (event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2 &&
                       event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2)
                {
                  if (event.button.y < 24)
                    {
                      /* Up scroll button: */

                      if (cur > 0)
                        {
                          cur = cur - 4;
                          update_list = 1;
                          playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                          if (cur == 0)
                            do_setcursor(cursor_arrow);
                        }

                      if (which >= cur + 16)
                        which = which - 4;
                    }
                  else if (event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET - 48) &&
                           event.button.y < (48 * 7 + 40 + HEIGHTOFFSET - 24))
                    {
                      /* Down scroll button: */

                      if (cur < num_files - 16)
                        {
                          cur = cur + 4;
                          update_list = 1;
                          playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                          if (cur >= num_files - 16)
                            do_setcursor(cursor_arrow);
                        }

                      if (which < cur)
                        which = which + 4;
                    }
                }
              else if (event.button.x >= 96 && event.button.x < 96 + 48 &&
                       event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET) - 48 &&
                       event.button.y < (48 * 7 + 40 + HEIGHTOFFSET))
                {
                  /* Play */

                  playsound(screen, 1, SND_CLICK, 1, SNDPOS_LEFT, SNDDIST_NEAR);


                  /* If none selected, select all, in order! */

                  if (num_selected == 0)
                    {
                      for (i = 0; i < num_files; i++)
                        selected[i] = i;
                      num_selected = num_files;
                    }

                  play_slideshow(selected, num_selected, dirname, d_names, d_exts, speed);


                  /* Redraw entire screen, after playback: */

                  SDL_FillRect(screen, NULL, SDL_MapRGB(canvas->format, 255, 255, 255));
                  draw_toolbar();
                  draw_colors(COLORSEL_CLOBBER_WIPE);
                  draw_none();

                  /* Instructions for Slideshow file dialog (FIXME: Make a #define) */
                  freeme = textdir(gettext_noop("Choose the pictures you want, " "then click “Play”."));
                  draw_tux_text(TUX_BORED, freeme, 1);
                  free(freeme);

                  SDL_Flip(screen);

                  update_list = 1;
                }
              else if (event.button.x >= 96 + 48 && event.button.x < 96 + 48 + 96 &&
                       event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET) - 48 &&
                       event.button.y < (48 * 7 + 40 + HEIGHTOFFSET))
                {
                  /* Speed slider */

                  int old_speed, control_sound, click_x;

                  old_speed = speed;

                  click_x = event.button.x - 96 - 48;
                  speed = ((10 * click_x) / 96);

                  control_sound = -1;

                  if (speed < old_speed)
                    control_sound = SND_SHRINK;
                  else if (speed > old_speed)
                    control_sound = SND_GROW;

                  if (control_sound != -1)
                    {
                      playsound(screen, 0, control_sound, 0, SNDPOS_CENTER, SNDDIST_NEAR);

                      update_list = 1;
                    }
                }
              else if (event.button.x >= (WINDOW_WIDTH - 96 - 48) &&
                       event.button.x < (WINDOW_WIDTH - 96) &&
                       event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET) - 48 &&
                       event.button.y < (48 * 7 + 40 + HEIGHTOFFSET))
                {
                  /* Back */

                  go_back = 1;
                  done = 1;
                  playsound(screen, 1, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
                }
#ifdef __ANDROID__
              start_motion_convert(event);
#endif
            }
          else if (event.type == SDL_MOUSEBUTTONUP)
            {
#ifdef __ANDROID__
              stop_motion_convert(event);
#endif
            }
          else if (event.type == SDL_MOUSEWHEEL && wheely)
            {
              /* Scroll wheel! */

              if (event.wheel.y > 0 && cur > 0)
                {
                  cur = cur - 4;
                  update_list = 1;
                  playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                  if (cur == 0)
                    do_setcursor(cursor_arrow);

                  if (which >= cur + 16)
                    which = which - 4;
                }
              else if (event.wheel.y < 0 && cur < num_files - 16)
                {
                  cur = cur + 4;
                  update_list = 1;
                  playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                  if (cur >= num_files - 16)
                    do_setcursor(cursor_arrow);

                  if (which < cur)
                    which = which + 4;
                }
            }
          else if (event.type == SDL_MOUSEMOTION)
            {
              /* Deal with mouse pointer shape! */

              if (event.button.y < 24 &&
                  event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2 &&
                  event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2 && cur > 0)
                {
                  /* Scroll up button: */

                  do_setcursor(cursor_up);
                }
              else if (event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET - 48) &&
                       event.button.y < (48 * 7 + 40 + HEIGHTOFFSET - 24) &&
                       event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2 &&
                       event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2 && cur < num_files - 16)
                {
                  /* Scroll down button: */

                  do_setcursor(cursor_down);
                }
              else if (((event.button.x >= 96 && event.button.x < 96 + 48 + 96) ||
                        (event.button.x >= (WINDOW_WIDTH - 96 - 48) &&
                         event.button.x < (WINDOW_WIDTH - 96))) &&
                       event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET) - 48 &&
                       event.button.y < (48 * 7 + 40 + HEIGHTOFFSET))
                {
                  /* One of the command buttons: */

                  do_setcursor(cursor_hand);
                }
              else if (event.button.x >= 96 && event.button.x < WINDOW_WIDTH - 96
                       && event.button.y > 24
                       && event.button.y < (48 * 7 + 40 + HEIGHTOFFSET) - 48
                       &&
                       ((((event.button.x - 96) / (THUMB_W) +
                          (((event.button.y - 24) / THUMB_H) * 4)) + cur) < num_files))
                {
                  /* One of the thumbnails: */

                  do_setcursor(cursor_hand);
                }
              else
                {
                  /* Unclickable... */

                  do_setcursor(cursor_arrow);
                }

#ifdef __ANDROID__
              convert_motion_to_wheel(event);
#endif

              oldpos_x = event.button.x;
              oldpos_y = event.button.y;
            }
          else if (event.type == SDL_JOYAXISMOTION)
            handle_joyaxismotion(event, &motioner, &val_x, &val_y);

          else if (event.type == SDL_JOYHATMOTION)
            handle_joyhatmotion(event, oldpos_x, oldpos_y, &valhat_x, &valhat_y, &hatmotioner, &old_hat_ticks);

          else if (event.type == SDL_JOYBALLMOTION)
            handle_joyballmotion(event, oldpos_x, oldpos_y);

          else if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP)
            handle_joybuttonupdown(event, oldpos_x, oldpos_y);
        }

      if (motioner | hatmotioner)
        handle_motioners(oldpos_x, oldpos_y, motioner, hatmotioner, old_hat_ticks, val_x, val_y, valhat_x, valhat_y);

      SDL_Delay(10);
    }
  while (!done);


  /* Clean up: */

  free_surface_array(thumbs, num_files);

  free(thumbs);

  for (i = 0; i < num_files; i++)
    {
      free(d_names[i]);
      free(d_exts[i]);
    }

  free(dirname);

  free(d_names);
  free(d_exts);
  free(selected);


  return go_back;
}


static void play_slideshow(int *selected, int num_selected, char *dirname, char **d_names, char **d_exts, int speed)
{
  int i, which, next, done;
  int val_x, val_y, motioner;
  int valhat_x, valhat_y, hatmotioner;

  SDL_Surface *img;
  char *tmp_starter_id, *tmp_template_id, *tmp_file_id;
  int tmp_starter_mirrored, tmp_starter_flipped, tmp_starter_personal;
  char fname[1024];
  SDL_Event event;
  SDLKey key;
  SDL_Rect dest;
  Uint32 last_ticks;

  val_x = val_y = motioner = 0;
  valhat_x = valhat_y = hatmotioner = 0;
  /* Back up the current image's IDs, because they will get
     clobbered below! */

  tmp_starter_id = strdup(starter_id);
  tmp_template_id = strdup(template_id);
  tmp_file_id = strdup(file_id);
  tmp_starter_mirrored = starter_mirrored;
  tmp_starter_flipped = starter_flipped;
  tmp_starter_personal = starter_personal;

  do_setcursor(cursor_tiny);

  done = 0;

  do
    {
      for (i = 0; i < num_selected && !done; i++)
        {
          which = selected[i];
          show_progress_bar(screen);


          /* Figure out filename: */

          snprintf(fname, sizeof(fname), "%s/%s%s", dirname, d_names[which], d_exts[which]);


          img = myIMG_Load(fname);

          if (img != NULL)
            {
              autoscale_copy_smear_free(img, screen, SDL_BlitSurface);

              strcpy(file_id, d_names[which]);


/* FIXME: is the starter even used??? -bjk 2009.10.16 */

              /* See if this saved image was based on a 'starter' */

              load_starter_id(d_names[which], NULL);

              if (starter_id[0] != '\0')
                {
                  load_starter(starter_id);

                  if (starter_mirrored)
                    mirror_starter();

                  if (starter_flipped)
                    flip_starter();
                }
              else
                load_template(template_id);
            }

          /* "Back" button: */

          dest.x = screen->w - 48;
          dest.y = screen->h - 48;
          SDL_BlitSurface(img_back, NULL, screen, &dest);

          dest.x = screen->w - 48 + (48 - img_openlabels_back->w) / 2;
          dest.y = screen->h - img_openlabels_back->h;
          SDL_BlitSurface(img_openlabels_back, NULL, screen, &dest);

          /* "Next" button: */

          dest.x = 0;
          dest.y = screen->h - 48;
          SDL_BlitSurface(img_play, NULL, screen, &dest);

          dest.x = (48 - img_openlabels_next->w) / 2;
          dest.y = screen->h - img_openlabels_next->h;
          SDL_BlitSurface(img_openlabels_next, NULL, screen, &dest);


          SDL_Flip(screen);


          /* Handle any events, and otherwise wait for time to count down: */

          next = 0;
          last_ticks = SDL_GetTicks();

          do
            {
              while (SDL_PollEvent(&event))
                {
                  if (event.type == SDL_QUIT)
                    {
                      /* FIXME: Handle SDL_QUIT better! */

                      next = 1;
                      done = 1;
                    }
                  else if (event.type == SDL_WINDOWEVENT)
                    {
                      handle_active(&event);
                    }
                  else if (event.type == SDL_KEYDOWN)
                    {
                      key = event.key.keysym.sym;

                      handle_keymouse(key, SDL_KEYDOWN, 24, NULL, NULL);

                      if (key == SDLK_RETURN || key == SDLK_SPACE || key == SDLK_PAGEDOWN)
                        {
                          /* RETURN, SPACE or PAGEDOWN: Skip to next right away! */

                          next = 1;
                          playsound(screen, 1, SND_CLICK, 1, SNDPOS_LEFT, SNDDIST_NEAR);
                        }
                      else if (key == SDLK_PAGEUP)
                        {
                          /* LEFT: Go back one! */

                          i = i - 2;

                          if (i < -1)
                            i = num_selected - 2;

                          next = 1;
                          playsound(screen, 1, SND_CLICK, 1, SNDPOS_LEFT, SNDDIST_NEAR);
                        }
                      else if (key == SDLK_ESCAPE || key == SDLK_AC_BACK)
                        {
                          /* Go back: */

                          next = 1;
                          done = 1;
                          playsound(screen, 1, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
                        }
                    }
                  else if (event.type == SDL_MOUSEBUTTONDOWN)
                    {
                      /* Mouse click! */

                      if (event.button.x >= screen->w - 48 && event.button.y >= screen->h - 48)
                        {
                          /* Back button */

                          next = 1;
                          done = 1;
                          playsound(screen, 1, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
                        }
                      else
                        {
                          /* Otherwise, skip to next image right away! */

                          next = 1;
                          playsound(screen, 1, SND_CLICK, 1, SNDPOS_LEFT, SNDDIST_NEAR);
                        }
                    }
                  else if (event.type == SDL_MOUSEMOTION)
                    {
                      /* Deal with mouse pointer shape! */

                      if ((event.button.x >= screen->w - 48 || event.button.x < 48) && event.button.y >= screen->h - 48)
                        {
                          /* Back or Next buttons */

                          do_setcursor(cursor_hand);
                        }
                      else
                        {
                          /* Otherwise, minimal cursor... */

                          do_setcursor(cursor_tiny);
                        }
                      oldpos_x = event.button.x;
                      oldpos_y = event.button.y;
                    }

                  else if (event.type == SDL_JOYAXISMOTION)
                    handle_joyaxismotion(event, &motioner, &val_x, &val_y);

                  else if (event.type == SDL_JOYHATMOTION)
                    handle_joyhatmotion(event, oldpos_x, oldpos_y, &valhat_x, &valhat_y, &hatmotioner, &old_hat_ticks);

                  else if (event.type == SDL_JOYBALLMOTION)
                    handle_joyballmotion(event, oldpos_x, oldpos_y);

                  else if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP)
                    handle_joybuttonupdown(event, oldpos_x, oldpos_y);

                }

              if (motioner | hatmotioner)
                handle_motioners(oldpos_x, oldpos_y, motioner, hatmotioner, old_hat_ticks, val_x, val_y, valhat_x,
                                 valhat_y);

              SDL_Delay(10);


              /* Automatically skip to the next one after time expires: */

              if (speed != 0)
                {
                  if (SDL_GetTicks() >= last_ticks + (10 - speed) * 500)
                    next = 1;
                }
            }
          while (!next);
        }
    }
  while (!done);

  strcpy(starter_id, tmp_starter_id);
  free(tmp_starter_id);

  strcpy(template_id, tmp_template_id);
  free(tmp_template_id);

  strcpy(file_id, tmp_file_id);
  free(tmp_file_id);

  starter_mirrored = tmp_starter_mirrored;
  starter_flipped = tmp_starter_flipped;
  starter_personal = tmp_starter_personal;
}



/* Draws large, bitmap digits over thumbnails in slideshow selection screen: */

static void draw_selection_digits(int right, int bottom, int n)
{
  SDL_Rect src, dest;
  int i, v, len, place;
  int digit_w, digit_h, x, y;

  digit_w = img_select_digits->w / 10;
  digit_h = img_select_digits->h;

  if (n > 99)
    {
      len = 3;
      place = 100;
    }
  else if (n > 9)
    {
      len = 2;
      place = 10;
    }
  else
    {
      len = 1;
      place = 1;
    }

  x = right - digit_w * len;
  y = bottom - digit_h;

  for (i = 0; i < len; i++)
    {
      v = (n / place) % (place * 10);

      src.x = digit_w * v;
      src.y = 0;
      src.w = digit_w;
      src.h = digit_h;

      dest.x = x;
      dest.y = y;

      SDL_BlitSurface(img_select_digits, &src, screen, &dest);

      x = x + digit_w;
      place = place / 10;
    }
}


/* Let sound effects (e.g., "Save" sfx) play out before quitting... */

static void wait_for_sfx(void)
{
#ifndef NOSOUND
  if (use_sound)
    {
      while (Mix_Playing(-1))
        SDL_Delay(10);
    }
#endif
}


/* stamp outline */
#ifndef LOW_QUALITY_STAMP_OUTLINE
/* XOR-based outline of rubber stamp shapes
   (unused if LOW_QUALITY_STAMP_OUTLINE is #defined) */

#if 1
#define STIPLE_W 5
#define STIPLE_H 5
static char stiple[] = "84210" "10842" "42108" "08421" "21084";
#endif

#if 0
#define STIPLE_W 4
#define STIPLE_H 4
static char stiple[] = "8000" "0800" "0008" "0080";
#endif

#if 0
#define STIPLE_W 12
#define STIPLE_H 12
static char stiple[] =
  "808808000000"
  "800000080880"
  "008088080000"
  "808000000808"
  "000080880800"
  "088080000008" "000000808808" "080880800000" "080000008088" "000808808000" "880800000080" "000008088080";
#endif

static unsigned char *stamp_outline_data;
static int stamp_outline_w, stamp_outline_h;

static void update_stamp_xor(void)
{
  int xx, yy, rx, ry;
  Uint8 dummy;
  SDL_Surface *src;

  Uint32(*getpixel) (SDL_Surface *, int, int);
  unsigned char *alphabits;
  int new_w;
  int new_h;
  unsigned char *outline;
  unsigned char *old_outline_data;

  src = active_stamp;

  /* start by scaling */
  src = thumbnail(src, CUR_STAMP_W, CUR_STAMP_H, 0);

  getpixel = getpixels[src->format->BytesPerPixel];
  alphabits = calloc(src->w + 4, src->h + 4);

  SDL_LockSurface(src);
  for (yy = 0; yy < src->h; yy++)
    {
      ry = yy;
      for (xx = 0; xx < src->w; xx++)
        {
          rx = xx;
          SDL_GetRGBA(getpixel(src, rx, ry),
                      src->format, &dummy, &dummy, &dummy, alphabits + xx + 2 + (yy + 2) * (src->w + 4));
        }
    }
  SDL_UnlockSurface(src);

  new_w = src->w + 4;
  new_h = src->h + 4;
  SDL_FreeSurface(src);
  outline = calloc(new_w, new_h);

  for (yy = 1; yy < new_h - 1; yy++)
    {
      for (xx = 1; xx < new_w - 1; xx++)
        {
          unsigned char above = 0;
          unsigned char below = 0xff;
          unsigned char tmp;

          tmp = alphabits[(xx - 1) + (yy - 1) * new_w];
          above |= tmp;
          below &= tmp;
          tmp = alphabits[(xx + 1) + (yy - 1) * new_w];
          above |= tmp;
          below &= tmp;

          tmp = alphabits[(xx + 0) + (yy - 1) * new_w];
          above |= tmp;
          below &= tmp;
          tmp = alphabits[(xx + 0) + (yy + 0) * new_w];
          above |= tmp;
          below &= tmp;
          tmp = alphabits[(xx + 1) + (yy + 0) * new_w];
          above |= tmp;
          below &= tmp;
          tmp = alphabits[(xx - 1) + (yy + 0) * new_w];
          above |= tmp;
          below &= tmp;
          tmp = alphabits[(xx + 0) + (yy + 1) * new_w];
          above |= tmp;
          below &= tmp;

          tmp = alphabits[(xx - 1) + (yy + 1) * new_w];
          above |= tmp;
          below &= tmp;
          tmp = alphabits[(xx + 1) + (yy + 1) * new_w];
          above |= tmp;
          below &= tmp;

          outline[xx + yy * new_w] = (above ^ below) >> 7;
        }
    }

  old_outline_data = stamp_outline_data;
  stamp_outline_data = outline;
  stamp_outline_w = new_w;
  stamp_outline_h = new_h;
  if (old_outline_data)
    free(old_outline_data);
  free(alphabits);
}

static void stamp_xor(int x, int y)
{
  int xx, yy, sx, sy;

  SDL_LockSurface(screen);
  for (yy = 0; yy < stamp_outline_h; yy++)
    {
      for (xx = 0; xx < stamp_outline_w; xx++)
        {
          if (!stamp_outline_data[xx + yy * stamp_outline_w])   /* FIXME: Conditional jump or move depends on uninitialised value(s) */
            continue;
          sx = x + xx - stamp_outline_w / 2;
          sy = y + yy - stamp_outline_h / 2;
          if (stiple[sx % STIPLE_W + sy % STIPLE_H * STIPLE_W] != '8')
            continue;
          xorpixel(sx, sy);
        }
    }
  SDL_UnlockSurface(screen);
}

#endif

static void rgbtohsv(Uint8 r8, Uint8 g8, Uint8 b8, float *h, float *s, float *v)
{
  float rgb_min, rgb_max, delta, r, g, b;

  r = (r8 / 255.0);
  g = (g8 / 255.0);
  b = (b8 / 255.0);

  rgb_min = min(r, min(g, b));
  rgb_max = max(r, max(g, b));
  *v = rgb_max;

  delta = rgb_max - rgb_min;

  if (rgb_max == 0)
    {
      /* Black */

      *s = 0;
      *h = -1;
    }
  else
    {
      *s = delta / rgb_max;

      if (r == rgb_max)
        *h = (g - b) / delta;
      else if (g == rgb_max)
        *h = 2 + (b - r) / delta;       /* between cyan & yellow */
      else
        *h = 4 + (r - g) / delta;       /* between magenta & cyan */

      *h = (*h * 60);           /* degrees */

      if (*h < 0)
        *h = (*h + 360);
    }
}


static void hsvtorgb(float h, float s, float v, Uint8 * r8, Uint8 * g8, Uint8 * b8)
{
  int i;
  float f, p, q, t, r, g, b;

  if (s == 0)
    {
      /* Achromatic (grey) */

      r = v;
      g = v;
      b = v;
    }
  else
    {
      h = h / 60;
      i = floor(h);
      f = h - i;
      p = v * (1 - s);
      q = v * (1 - s * f);
      t = v * (1 - s * (1 - f));

      if (i == 0)
        {
          r = v;
          g = t;
          b = p;
        }
      else if (i == 1)
        {
          r = q;
          g = v;
          b = p;
        }
      else if (i == 2)
        {
          r = p;
          g = v;
          b = t;
        }
      else if (i == 3)
        {
          r = p;
          g = q;
          b = v;
        }
      else if (i == 4)
        {
          r = t;
          g = p;
          b = v;
        }
      else
        {
          r = v;
          g = p;
          b = q;
        }
    }


  *r8 = (Uint8) (r * 255);
  *g8 = (Uint8) (g * 255);
  *b8 = (Uint8) (b * 255);
}

static void print_image(void)
{
  int cur_time;

  cur_time = SDL_GetTicks() / 1000;

#ifdef DEBUG
  printf("Current time = %d\n", cur_time);
#endif

  if (cur_time >= last_print_time + print_delay)
    {
      if (alt_print_command_default == ALTPRINT_ALWAYS)
        want_alt_printcommand = 1;
      else if (alt_print_command_default == ALTPRINT_NEVER)
        want_alt_printcommand = 0;
      else                      /* ALTPRINT_MOD */
        want_alt_printcommand = (SDL_GetModState() & KMOD_ALT);

      if (do_prompt_image_snd(PROMPT_PRINT_NOW_TXT,
                              PROMPT_PRINT_NOW_YES,
                              PROMPT_PRINT_NOW_NO,
                              img_printer, NULL, NULL, SND_AREYOUSURE,
                              (TOOL_PRINT % 2) * 48 + 24, (TOOL_PRINT / 2) * 48 + 40 + 24))
        {
          do_print();

          last_print_time = cur_time;
        }
    }
  else
    {
      do_prompt_image_snd(PROMPT_PRINT_TOO_SOON_TXT,
                          PROMPT_PRINT_TOO_SOON_YES, "", img_printer_wait, NULL, NULL, SND_YOUCANNOT, 0, screen->h);
    }
}

void do_print(void)
{
  /* Assemble drawing plus any labels: */
  SDL_BlitSurface(canvas, NULL, save_canvas, NULL);
  SDL_BlitSurface(label, NULL, save_canvas, NULL);

#if !defined(WIN32) && !defined(__BEOS__) && !defined(__HAIKU__) && !defined(__ANDROID__)
  const char *pcmd;
  FILE *pi;

  /* Linux, Unix, etc. */

  if (want_alt_printcommand && !fullscreen)
    pcmd = altprintcommand;
  else
    pcmd = printcommand;

  pi = popen(pcmd, "w");

  if (pi == NULL)
    {
      perror(pcmd);
    }
  else
    {
#ifdef PRINTMETHOD_PNG_PNM_PS
      if (do_png_save(pi, pcmd, save_canvas, 0))
        do_prompt_snd(PROMPT_PRINT_TXT, PROMPT_PRINT_YES, "", SND_TUXOK, screen->w / 2, screen->h / 2);
#elif defined(PRINTMETHOD_PNM_PS)
      /* nothing here */
#elif defined(PRINTMETHOD_PS)
      if (do_ps_save(pi, pcmd, save_canvas, papersize, 1))
        do_prompt_snd(PROMPT_PRINT_TXT, PROMPT_PRINT_YES, "", SND_TUXOK, screen->w / 2, screen->h / 2);
      else
        do_prompt_snd(PROMPT_PRINT_FAILED_TXT, PROMPT_PRINT_YES, "", SND_YOUCANNOT, screen->w / 2, screen->h / 2);
#else
#error No print method defined!
#endif
    }
#else
#ifdef WIN32
  /* Win32 */

  char f[512];
  int show = want_alt_printcommand;

  snprintf(f, sizeof(f), "%s/%s", savedir, "print.cfg");        /* FIXME */

  {
    const char *error = SurfacePrint(save_canvas, use_print_config ? f : NULL, show);

    if (error)
      fprintf(stderr, "%s\n", error);
  }
#elif defined(__BEOS__)
  /* BeOS */

  SurfacePrint(save_canvas);

#elif defined(__ANDROID__)

  int x, y;
  Uint8 src_r, src_g, src_b, src_a;
  SDL_Surface *save_canvas_and = SDL_CreateRGBSurface(0,
                                                      WINDOW_WIDTH - (96 * 2),
                                                      (48 * 7) + 40 + HEIGHTOFFSET,
                                                      screen->format->BitsPerPixel,
                                                      screen->format->Rmask,
                                                      screen->format->Gmask,
                                                      screen->format->Bmask, 0);


  for (x = 0; x < save_canvas->w; x++)
    for (y = 0; y < save_canvas->h; y++)
      {
        SDL_GetRGBA(getpixels[save_canvas->format->BytesPerPixel] (save_canvas, x, y),
                    save_canvas->format, &src_r, &src_g, &src_b, &src_a);

        putpixels[save_canvas_and->format->BytesPerPixel] (save_canvas_and, x, y,
                                                           SDL_MapRGBA(save_canvas_and->format, src_r, src_g, src_b,
                                                                       SDL_ALPHA_OPAQUE));
      }

  const char *error = SurfacePrint(save_canvas_and);

  if (error)
    {
      fprintf(stderr, "Cannot print: %s\n", error);
      do_prompt_snd(error, PROMPT_PRINT_YES, "", SND_TUXOK, 0, 0);
    }
  SDL_FreeSurface(save_canvas_and);

#endif

#endif
}

static void do_render_cur_text(int do_blit)
{
  int w, h;

  SDL_Color color = { color_hexes[cur_color][0],
    color_hexes[cur_color][1],
    color_hexes[cur_color][2],
    0
  };

  SDL_Surface *tmp_surf;
  SDL_Rect dest, src;
  wchar_t *str;

  /* I THINK this is unnecessary to call here; trying to prevent flicker when typing -bjk 2010.02.10 */
  /* hide_blinking_cursor(); */


  /* Keep cursor on the screen! */

  if (cursor_y > ((48 * 7 + 40 + HEIGHTOFFSET) - TuxPaint_Font_FontHeight(getfonthandle(cur_font))))
    {
      cursor_y = ((48 * 7 + 40 + HEIGHTOFFSET) - TuxPaint_Font_FontHeight(getfonthandle(cur_font)));
    }


  /* Render the text: */

  if (texttool_len > 0)
    {
#if defined(_FRIBIDI_H) || defined(FRIBIDI_H)
      //FriBidiCharType baseDir = FRIBIDI_TYPE_LTR;
      //FriBidiCharType baseDir = FRIBIDI_TYPE_WL; /* Per: Shai Ayal <shaiay@gmail.com>, 2009-01-14 */
      FriBidiParType baseDir = FRIBIDI_TYPE_WL; //EP to avoid warning on types in now commented line above
      FriBidiChar *unicodeIn, *unicodeOut;
      unsigned int i;

      unicodeIn = (FriBidiChar *) malloc(sizeof(FriBidiChar) * (texttool_len + 1));
      unicodeOut = (FriBidiChar *) malloc(sizeof(FriBidiChar) * (texttool_len + 1));

      str = (wchar_t *) malloc(sizeof(wchar_t) * (texttool_len + 1));

      for (i = 0; i < texttool_len; i++)
        unicodeIn[i] = (FriBidiChar) texttool_str[i];

      fribidi_log2vis(unicodeIn, texttool_len, &baseDir, unicodeOut, 0, 0, 0);

      /* FIXME: If we determine that some new text was RtoL, we should
         reposition the text */

      for (i = 0; i < texttool_len; i++)
        str[i] = (long)unicodeOut[i];

      str[texttool_len] = L'\0';

      free(unicodeIn);
      free(unicodeOut);
#else
      str = uppercase_w(texttool_str);
#endif

      tmp_surf = render_text_w(getfonthandle(cur_font), str, color);

      w = tmp_surf->w;
      h = tmp_surf->h;
      r_tir.h = (float)tmp_surf->h / render_scale;
      r_tir.w = (float)tmp_surf->w / render_scale;

      cursor_textwidth = w;
    }
  else                          /* Erase the stalle letter . */
    {
      if (cur_label != LABEL_SELECT)
        {
          update_canvas_ex_r(old_dest.x - 96, old_dest.y, old_dest.x + old_dest.w, old_dest.y + old_dest.h, 0);
          old_dest.x = old_dest.y = old_dest.w = old_dest.h = 0;


          update_canvas_ex_r(old_cursor_x - 1,
                             old_cursor_y - 1,
                             old_cursor_x + 1, old_cursor_y + 1 + TuxPaint_Font_FontHeight(getfonthandle(cur_font)), 0);

          /* FIXME: Do less flickery updating here (use update_canvas_ex() above, then SDL_Flip() or SDL_UpdateRect() here -bjk 2010.02.10 */

          old_cursor_x = cursor_x;
          old_cursor_y = cursor_y;
          cursor_textwidth = 0;
        }
      /* FIXME: Is this SDL_Flip() still needed? Pere 2011.06.28 */
      SDL_Flip(screen);
      return;

    }


  if (!do_blit)
    {
      update_canvas_ex_r(old_dest.x - 96, old_dest.y, old_dest.x + old_dest.w, old_dest.y + old_dest.h, 0);

      /* update_canvas_ex_r(cursor_x - 1, */
      /*            cursor_y - 1, */
      /*            cursor_x + 1 +  TuxPaint_Font_FontHeight(getfonthandle(cur_font)) * 3, */
      /*            cursor_y + 1 + TuxPaint_Font_FontHeight(getfonthandle(cur_font)), 0); */


      /* Draw outline around text: */

      dest.x = cursor_x - 2 + 96;
      dest.y = cursor_y - 2;
      dest.w = w + 4;
      dest.h = h + 4;

      if (dest.x + dest.w > WINDOW_WIDTH - 96)
        dest.w = WINDOW_WIDTH - 96 - dest.x;
      if (dest.y + dest.h > (48 * 7 + 40 + HEIGHTOFFSET))
        dest.h = (48 * 7 + 40 + HEIGHTOFFSET) - dest.y;

      SDL_FillRect(screen, &dest, SDL_MapRGB(canvas->format, 0, 0, 0));

      old_dest.x = dest.x;
      old_dest.y = dest.y;
      old_dest.w = dest.w;
      old_dest.h = dest.h;

      /* FIXME: This would be nice if it were alpha-blended: */

      dest.x = cursor_x + 96;
      dest.y = cursor_y;
      dest.w = w;
      dest.h = h;

      if (dest.x + dest.w > WINDOW_WIDTH - 96)
        dest.w = WINDOW_WIDTH - 96 - dest.x;
      if (dest.y + dest.h > (48 * 7 + 40 + HEIGHTOFFSET))
        dest.h = (48 * 7 + 40 + HEIGHTOFFSET) - dest.y;

      if ((color_hexes[cur_color][0] + color_hexes[cur_color][1] + color_hexes[cur_color][2]) >= 384)
        {
          /* Grey background if blit is white!... */

          SDL_FillRect(screen, &dest, SDL_MapRGB(canvas->format, 64, 64, 64));
        }
      else
        {
          /* White background, normally... */

          SDL_FillRect(screen, &dest, SDL_MapRGB(canvas->format, 255, 255, 255));
        }
    }


  /* Draw the text itself! */

  if (tmp_surf != NULL)
    {
      dest.x = cursor_x;
      dest.y = cursor_y;

      src.x = 0;
      src.y = 0;
      src.w = tmp_surf->w;
      src.h = tmp_surf->h;

      if (dest.x + src.w > WINDOW_WIDTH - 96 - 96)
        src.w = WINDOW_WIDTH - 96 - 96 - dest.x;
      if (dest.y + src.h > (48 * 7 + 40 + HEIGHTOFFSET))
        src.h = (48 * 7 + 40 + HEIGHTOFFSET) - dest.y;

      if (do_blit)
        {
          if ((cur_tool == TOOL_LABEL && label_node_to_edit) ||
              ((old_tool == TOOL_LABEL && label_node_to_edit) &&
               (cur_tool == TOOL_PRINT ||
                cur_tool == TOOL_SAVE || cur_tool == TOOL_OPEN || cur_tool == TOOL_NEW || cur_tool == TOOL_QUIT)))
            {
              have_to_rec_label_node = TRUE;
              add_label_node(src.w, src.h, dest.x, dest.y, tmp_surf);
              simply_render_node(current_label_node);
            }
          else if (cur_tool == TOOL_LABEL ||
                   (old_tool == TOOL_LABEL &&
                    (cur_tool == TOOL_PRINT ||
                     cur_tool == TOOL_SAVE || cur_tool == TOOL_OPEN || cur_tool == TOOL_NEW || cur_tool == TOOL_QUIT)))
            {
              myblit(tmp_surf, &src, label, &dest);

              have_to_rec_label_node = TRUE;
              add_label_node(src.w, src.h, dest.x, dest.y, tmp_surf);

            }
          else
            {
              SDL_BlitSurface(tmp_surf, &src, canvas, &dest);
            }
          update_canvas_ex_r(dest.x - 2, dest.y - 2, dest.x + tmp_surf->w + 4, dest.y + tmp_surf->h + 4, 0);
        }
      else
        {
          dest.x = dest.x + 96;
          SDL_BlitSurface(tmp_surf, &src, screen, &dest);
        }
    }


  /* FIXME: Only update what's changed! */
  SDL_Flip(screen);

  //update_screen_rect(&dest);
  free(str);

  if (tmp_surf != NULL)
    SDL_FreeSurface(tmp_surf);
/*   if (tmp_label != NULL) */
  /*    SDL_FreeSurface(tmp_label); */
  // SDL_Delay(5000);

}


/* Return string as uppercase if that option is set: */

static char *uppercase(const char *restrict const str)
{
  unsigned int i, n;
  wchar_t *dest;
  char *ustr;

  if (!only_uppercase)
    return strdup(str);

  /* watch out: uppercase chars may need extra bytes!
     http://en.wikipedia.org/wiki/Turkish_dotted_and_dotless_I */
  n = strlen(str) + 1;
  dest = alloca(sizeof(wchar_t) * n);
  ustr = malloc(n * 2);         /* use *2 in case 'i' becomes U+0131 */

  mbstowcs(dest, str, sizeof(wchar_t) * n);     /* at most n wchar_t written */
  i = 0;
  do
    {
      dest[i] = towupper(dest[i]);
    }
  while (dest[i++]);
  wcstombs(ustr, dest, n);      /* at most n bytes written */

#ifdef DEBUG
  printf(" ORIGINAL: %s\n" "UPPERCASE: %s\n\n", str, ustr);
#endif
  return ustr;
}

static wchar_t *uppercase_w(const wchar_t * restrict const str)
{
  unsigned n = wcslen(str) + 1;
  wchar_t *ustr = malloc(sizeof(wchar_t) * n);

  memcpy(ustr, str, sizeof(wchar_t) * n);

  if (only_uppercase)
    {
      unsigned i = 0;

      do
        {
          ustr[i] = towupper(ustr[i]);
        }
      while (ustr[i++]);
    }

  return ustr;
}


/* Return string in right-to-left mode, if necessary: */

static char *textdir(const char *const str)
{
  unsigned char *dstr;
  unsigned i, j;
  unsigned char c1, c2, c3, c4;

#ifdef DEBUG
  printf("ORIG_DIR: %s\n", str);
#endif

  dstr = malloc(strlen(str) + 5);

  if (need_right_to_left_word)
    {
      dstr[strlen(str)] = '\0';

      for (i = 0; i < strlen(str); i++)
        {
          j = (strlen(str) - i - 1);

          c1 = (unsigned char)str[i + 0];
          c2 = (unsigned char)str[i + 1];
          c3 = (unsigned char)str[i + 2];
          c4 = (unsigned char)str[i + 3];

          if (c1 < 128)         /* 0xxx xxxx - 1 byte */
            {
              dstr[j] = str[i];
            }
          else if ((c1 & 0xE0) == 0xC0) /* 110x xxxx - 2 bytes */
            {
              dstr[j - 1] = c1;
              dstr[j - 0] = c2;
              i = i + 1;
            }
          else if ((c1 & 0xF0) == 0xE0) /* 1110 xxxx - 3 bytes */
            {
              dstr[j - 2] = c1;
              dstr[j - 1] = c2;
              dstr[j - 0] = c3;
              i = i + 2;
            }
          else                  /* 1111 0xxx - 4 bytes */
            {
              dstr[j - 3] = c1;
              dstr[j - 2] = c2;
              dstr[j - 1] = c3;
              dstr[j - 0] = c4;
              i = i + 3;
            }
        }
    }
  else
    {
      strcpy((char *)dstr, str);
    }

#ifdef DEBUG
  printf("L2R_DIR: %s\n", dstr);
#endif

  return ((char *)dstr);
}


/* Scroll Timer */

static Uint32 scrolltimer_callback(Uint32 interval, void *param)
{
  /* printf("scrolltimer_callback(%d) -- ", interval); */
  if (scrolling)
    {
      /* printf("(Still scrolling)\n"); */
      SDL_PushEvent((SDL_Event *) param);
      return interval;
    }
  else
    {
      /* printf("(all done)\n"); */
      return 0;
    }
}


/* Controls the Text-Timer - interval == 0 removes the timer */

static void control_drawtext_timer(Uint32 interval, const char *const text, Uint8 locale_text)
{
  static int activated = 0;
  static SDL_TimerID TimerID = 0;
  static SDL_Event drawtext_event;


  /* Remove old timer if any is running */

  if (activated)
    {
      SDL_RemoveTimer(TimerID);
      activated = 0;
      TimerID = 0;
    }

  if (interval == 0)
    return;

  drawtext_event.type = SDL_USEREVENT;
  drawtext_event.user.code = USEREVENT_TEXT_UPDATE;
  drawtext_event.user.data1 = (void *)text;
  drawtext_event.user.data2 = (void *)(intptr_t) ((int)locale_text);    //EP added (intptr_t) to avoid warning on x64


  /* Add new timer */

  TimerID = SDL_AddTimer(interval, drawtext_callback, (void *)&drawtext_event);
  activated = 1;
}


/* Drawtext Timer */

static Uint32 drawtext_callback(Uint32 interval, void *param)
{
  (void)interval;
  SDL_PushEvent((SDL_Event *) param);

  return 0;                     /* Remove timer */
}




#ifdef DEBUG
static char *debug_gettext(const char *str)
{
  if (strcmp(str, dgettext(NULL, str)) == 0)
    {
      printf("NOTRANS: %s\n", str);
      printf("..TRANS: %s\n", dgettext(NULL, str));
      fflush(stdout);
    }

  return (dgettext(NULL, str));
}
#endif


static const char *great_str(void)
{
  return (great_strs[rand() % (sizeof(great_strs) / sizeof(char *))]);
}


#ifdef DEBUG
static int charsize(Uint16 c)
{
  Uint16 str[2];
  int w, h;

  str[0] = c;
  str[1] = '\0';

  TTF_SizeUNICODE(getfonthandle(cur_font)->ttf_font, str, &w, &h);

  return w;
}
#endif

static void draw_image_title(int t, SDL_Rect dest)
{
  SDL_BlitSurface(img_title_on, NULL, screen, &dest);

  dest.x += (dest.w - img_title_names[t]->w) / 2;
  dest.y += (dest.h - img_title_names[t]->h) / 2;
  SDL_BlitSurface(img_title_names[t], NULL, screen, &dest);
}



/* Handle keyboard events to control the mouse: */
/* Move as many pixels as bigsteps outside the areas,
   in the areas and 5 pixels around, move 1 pixel at a time */
static void handle_keymouse(SDLKey key, Uint32 updown, int steps, SDL_Rect * area1, SDL_Rect * area2)
{
  int left, right, up, bottom;
  SDL_Event event;
  SDL_Rect r1, r2;

  if (keymouse)
    {
      /* make the compiler happy */
      r1.x = r1.y = r1.w = r1.h = 0;
      r2.x = r2.y = r2.w = r2.h = 0;

      if (area1)
        {
          r1.x = max(0, area1->x - 5);
          r1.y = max(0, area1->y - 5);
          r1.w = area1->x - r1.x + area1->w + 5;
          r1.h = area1->y - r1.y + area1->h + 5;
        }

      if (area2)
        {
          r2.x = max(0, area2->x - 5);
          r2.y = max(0, area2->y - 5);
          r2.w = area2->x - r2.x + area2->w + 5;
          r2.h = area2->y - r2.y + area2->h + 5;
        }

      /* The defaults */
      left = max(0, oldpos_x - steps);
      right = min(screen->w, oldpos_x + steps);
      up = max(0, oldpos_y - steps);
      bottom = min(screen->h, oldpos_y + steps);

      /* If Shift is pressed, go with the defaults */
      if (!(SDL_GetModState() & KMOD_SHIFT))
        {
          /* 1 pixel if in one of the areas */
          if ((area1 && oldpos_x > r1.x && oldpos_x - r1.x < r1.w && oldpos_y > r1.y && oldpos_y - r1.y < r1.h) ||
              (area2 && oldpos_x > r2.x && oldpos_x - r2.x < r2.w && oldpos_y > r2.y && oldpos_y - r2.y < r2.h))
            {
              left = max(0, oldpos_x - 1);
              right = min(screen->w, oldpos_x + 1);
              up = max(0, oldpos_y - 1);
              bottom = min(screen->h, oldpos_y + 1);
            }

          /* Not enter too deep in the areas at once */
          else if (area1 || area2)
            {
              if (area1)
                if (oldpos_y - r1.y < r1.h && oldpos_y > r1.y)
                  {
                    if (oldpos_x - r1.x < steps)
                      right = min(oldpos_x + steps, r1.x + 1);
                    else if (oldpos_x - r1.x - r1.w < steps)
                      left = max(r1.x + r1.w - 1, oldpos_x - steps);
                  }

              if (oldpos_x - r1.x < r1.w && oldpos_x > r1.x)
                {
                  if (oldpos_y - r1.y < steps)
                    bottom = min(r1.y + 1, oldpos_y + steps);
                  else if (oldpos_y - r1.y - r1.h < steps)
                    up = max(r1.y + r1.h - 1, oldpos_y - steps);
                }

              if (area2)
                if (oldpos_y - r2.y < r2.h && oldpos_y > r2.y)
                  {
                    if (oldpos_x - r2.x < steps)
                      right = min(oldpos_x + steps, r2.x + 1);
                    else if (oldpos_x - r2.x - r2.w < steps)
                      left = max(r2.x + r2.w - 1, oldpos_x - steps);
                  }

              if (oldpos_x - r2.x < r2.w && oldpos_x > r2.x)
                {
                  if (oldpos_y - r2.y < steps)
                    bottom = min(r2.y + 1, oldpos_y + steps);
                  else if (oldpos_y - r2.y - r2.h < steps)
                    up = max(r2.y + r2.h - 1, oldpos_y - steps);
                }
            }
        }

      if (updown == SDL_KEYUP)
        {
          if (key == SDLK_INSERT || key == SDLK_F5 ||
              ((cur_tool != TOOL_TEXT && cur_tool != TOOL_LABEL) &&
               (key == SDLK_SPACE || key == SDLK_5 || key == SDLK_KP_5)))
            {
              event.type = SDL_MOUSEBUTTONUP;
              event.button.x = oldpos_x;
              event.button.y = oldpos_y;
              event.button.button = 1;
              SDL_PushEvent(&event);
            }
        }

      else
        {
          if (key == SDLK_LEFT)
            SDL_WarpMouse(left, oldpos_y);

          else if (key == SDLK_RIGHT)
            SDL_WarpMouse(right, oldpos_y);

          else if (key == SDLK_UP)
            SDL_WarpMouse(oldpos_x, up);

          else if (key == SDLK_DOWN)
            SDL_WarpMouse(oldpos_x, bottom);

          else if ((key == SDLK_INSERT || key == SDLK_F5) && !button_down)
            {
              event.type = SDL_MOUSEBUTTONDOWN;
              event.button.x = oldpos_x;
              event.button.y = oldpos_y;
              event.button.button = 1;
              SDL_PushEvent(&event);
            }

          else if (cur_tool != TOOL_TEXT && cur_tool != TOOL_LABEL)
            {
              if (!button_down && (key == SDLK_SPACE || key == SDLK_5 || key == SDLK_KP_5))
                {
                  event.type = SDL_MOUSEBUTTONDOWN;
                  event.button.x = oldpos_x;
                  event.button.y = oldpos_y;
                  event.button.button = 1;
                  SDL_PushEvent(&event);
                }

              else if (key == SDLK_1 || key == SDLK_KP_1)
                SDL_WarpMouse(left, bottom);

              else if (key == SDLK_3 || key == SDLK_KP_3)
                SDL_WarpMouse(right, bottom);

              else if (key == SDLK_7 || key == SDLK_KP_7)
                SDL_WarpMouse(left, up);

              else if (key == SDLK_9 || key == SDLK_KP_9)
                SDL_WarpMouse(right, up);

              else if (key == SDLK_2 || key == SDLK_KP_2)
                SDL_WarpMouse(oldpos_x, bottom);

              else if (key == SDLK_8 || key == SDLK_KP_8)
                SDL_WarpMouse(oldpos_x, up);

              else if (key == SDLK_6 || key == SDLK_KP_6)
                SDL_WarpMouse(right, oldpos_y);

              else if (key == SDLK_4 || key == SDLK_KP_4)
                SDL_WarpMouse(left, oldpos_y);

              /* FIXME: This is qwerty centric and interferes with gettexted reponses for yes/no,
                 so disabling until either is removed or is configurable. */
#if 0
              else if (key == SDLK_s)
                SDL_WarpMouse(oldpos_x, bottom);

              else if (key == SDLK_w)
                SDL_WarpMouse(oldpos_x, up);

              else if (key == SDLK_d)
                SDL_WarpMouse(right, oldpos_y);

              else if (key == SDLK_a)
                SDL_WarpMouse(left, oldpos_y);
#endif

            }
        }
    }
}

/* A subset of keys that will move one button at a time and jump between r_canvas<->r_tools<->r_colors */
static void handle_keymouse_buttons(SDLKey key, int *whicht, int *whichc, SDL_Rect real_r_tools)
{
  if (hit_test(&real_r_tools, oldpos_x, oldpos_y) &&
      (key == SDLK_F7 || key == SDLK_F8 || key == SDLK_F11 || key == SDLK_F12))
    {
      *whicht = tool_scroll + grid_hit_gd(&real_r_tools, oldpos_x, oldpos_y, &gd_tools);

      if (key == SDLK_F7 && hit_test(&real_r_tools, oldpos_x, oldpos_y))
        {
          *whicht += 2;
          *whicht = *whicht % NUM_TOOLS;
          while (!tool_avail[*whicht])
            {
              *whicht += 2;
              *whicht = *whicht % NUM_TOOLS;
            }
        }

      else if (key == SDLK_F8 && hit_test(&real_r_tools, oldpos_x, oldpos_y))
        {
          *whicht -= 2;
          if (*whicht < 0)
            *whicht += NUM_TOOLS;
          while (!tool_avail[*whicht])
            {
              *whicht -= 2;
              if (*whicht < 0)
                *whicht += NUM_TOOLS;
            }
        }

      else if (key == SDLK_F12 && hit_test(&real_r_tools, oldpos_x, oldpos_y))
        {
          *whicht = *whicht + 1;
          *whicht = *whicht % NUM_TOOLS;
          while (!tool_avail[*whicht])
            {
              *whicht += 1;
              *whicht = *whicht % NUM_TOOLS;
            }
        }

      else if (key == SDLK_F11 && hit_test(&real_r_tools, oldpos_x, oldpos_y))
        {
          *whicht = tool_scroll + grid_hit_gd(&real_r_tools, oldpos_x, oldpos_y, &gd_tools);
          *whicht = *whicht - 1;
          if (*whicht < 0)
            *whicht += NUM_TOOLS;
          while (!tool_avail[*whicht])
            {
              *whicht -= 1;
              if (*whicht < 0)
                *whicht += NUM_TOOLS;
            }
        }

      while (*whicht >= tool_scroll + 2 * real_r_tools.h / button_h)
        {
          tool_scroll += 2;
          draw_toolbar();
          update_screen_rect(&r_tools);
        }
      while (*whicht < tool_scroll)
        {
          tool_scroll -= 2;
          draw_toolbar();
          update_screen_rect(&r_tools);
        }

      SDL_WarpMouse(button_w / 2 + *whicht % 2 * button_w,
                    real_r_tools.y - *whicht % 2 * button_w / 2 + *whicht * button_h / 2 + 10 -
                    tool_scroll * button_h / 2);
    }

  else if (key == SDLK_F11 && hit_test(&r_colors, oldpos_x, oldpos_y))
    {
      *whichc = grid_hit_gd(&r_colors, oldpos_x, oldpos_y, &gd_colors);
      *whichc = *whichc - 1;
      if (*whichc < 0)
        *whichc += NUM_COLORS;
      SDL_WarpMouse(button_w * 2 + *whichc * color_button_w + 12, r_canvas.h + (r_colors.h / 2));
    }

  else if (key == SDLK_F12 && hit_test(&r_colors, oldpos_x, oldpos_y))
    {
      *whichc = grid_hit_gd(&r_colors, oldpos_x, oldpos_y, &gd_colors);
      *whichc = *whichc + 1;
      *whichc = *whichc % NUM_COLORS;
      SDL_WarpMouse(button_w * 2 + *whichc * color_button_w + 12, r_canvas.h + (r_colors.h / 2));
    }

  else if (key == SDLK_F4)
    {
      if (hit_test(&r_tools, oldpos_x, oldpos_y))
        SDL_WarpMouse(button_w * 2 + *whichc * color_button_w + 12, r_canvas.h + (r_colors.h / 2));
      else if (hit_test(&r_colors, oldpos_x, oldpos_y))
        SDL_WarpMouse(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
      else
        SDL_WarpMouse(button_w / 2 + *whicht % 2 * button_w,
                      real_r_tools.y - *whicht % 2 * button_w / 2 + *whicht * button_h / 2 + 10 -
                      tool_scroll * button_h / 2);

      /* Play a sound here as there is a big jump */
      playsound(screen, 1, SND_CLICK, 0, SNDPOS_LEFT, SNDDIST_NEAR);
    }
}

/* Unblank screen in fullscreen mode, if needed: */

static void handle_active(SDL_Event * event)
{
  if (event->window.event == SDL_WINDOWEVENT_EXPOSED || SDL_WINDOWEVENT_RESTORED)
    {
      //      if (fullscreen)
      SDL_Flip(screen);
    }
  if (event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
    {
      if (mouseaccessibility)
        {
          magic_switchin(canvas);
        }
      else if (mouseaccessibility && emulate_button_pressed)
        {
          magic_switchout(canvas);
        }

#ifdef _WIN32
      SetActivationState(event->active.gain);
#endif
    }
}


/* For right-to-left languages, when word-wrapping, we need to
   make sure the text doesn't end up going from bottom-to-top, too! */

#ifdef NO_SDLPANGO
static void anti_carriage_return(int left, int right, int cur_top, int new_top, int cur_bot, int line_width)
{
  SDL_Rect src, dest;


  /* Move current set of text down one line (and right-justify it!): */

  src.x = left;
  src.y = cur_top;
  src.w = line_width;
  src.h = cur_bot - cur_top;

  dest.x = right - line_width;
  dest.y = new_top;

  SDL_BlitSurface(screen, &src, screen, &dest);


  /* Clear the top line for new text: */

  dest.x = left;
  dest.y = cur_top;
  dest.w = right - left;
  dest.h = new_top - cur_top;

  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));
}
#endif


static SDL_Surface *duplicate_surface(SDL_Surface * orig)
{
  /*
     Uint32 amask;

     amask = ~(orig->format->Rmask |
     orig->format->Gmask |
     orig->format->Bmask);

     return(SDL_CreateRGBSurface(SDL_SWSURFACE,
     orig->w, orig->h,
     orig->format->BitsPerPixel,
     orig->format->Rmask,
     orig->format->Gmask,
     orig->format->Bmask,
     amask));
   */

  return (SDL_DisplayFormatAlpha(orig));
}

static void mirror_starter(void)
{
  SDL_Surface *orig;
  int x;
  SDL_Rect src, dest;


  /* Mirror overlay: */

  orig = img_starter;
  img_starter = duplicate_surface(orig);

  if (img_starter != NULL)
    {
      for (x = 0; x < orig->w; x++)
        {
          src.x = x;
          src.y = 0;
          src.w = 1;
          src.h = orig->h;

          dest.x = orig->w - x - 1;
          dest.y = 0;

          SDL_BlitSurface(orig, &src, img_starter, &dest);
        }

      SDL_FreeSurface(orig);
    }
  else
    {
      img_starter = orig;
    }


  /* Mirror background: */

  if (img_starter_bkgd != NULL)
    {
      orig = img_starter_bkgd;
      img_starter_bkgd = duplicate_surface(orig);

      if (img_starter_bkgd != NULL)
        {
          for (x = 0; x < orig->w; x++)
            {
              src.x = x;
              src.y = 0;
              src.w = 1;
              src.h = orig->h;

              dest.x = orig->w - x - 1;
              dest.y = 0;

              SDL_BlitSurface(orig, &src, img_starter_bkgd, &dest);
            }

          SDL_FreeSurface(orig);
        }
      else
        {
          img_starter_bkgd = orig;
        }
    }
}


static void flip_starter(void)
{
  SDL_Surface *orig;
  int y;
  SDL_Rect src, dest;


  /* Flip overlay: */

  orig = img_starter;
  img_starter = duplicate_surface(orig);

  if (img_starter != NULL)
    {
      for (y = 0; y < orig->h; y++)
        {
          src.x = 0;
          src.y = y;
          src.w = orig->w;
          src.h = 1;

          dest.x = 0;
          dest.y = orig->h - y - 1;

          SDL_BlitSurface(orig, &src, img_starter, &dest);
        }

      SDL_FreeSurface(orig);
    }
  else
    {
      img_starter = orig;
    }


  /* Flip background: */

  if (img_starter_bkgd != NULL)
    {
      orig = img_starter_bkgd;
      img_starter_bkgd = duplicate_surface(orig);

      if (img_starter_bkgd != NULL)
        {
          for (y = 0; y < orig->h; y++)
            {
              src.x = 0;
              src.y = y;
              src.w = orig->w;
              src.h = 1;

              dest.x = 0;
              dest.y = orig->h - y - 1;

              SDL_BlitSurface(orig, &src, img_starter_bkgd, &dest);
            }

          SDL_FreeSurface(orig);
        }
      else
        {
          img_starter_bkgd = orig;
        }
    }
}


static int valid_click(Uint8 button)
{
  if (button == 1 || ((button == 2 || button == 3) && no_button_distinction))
    return (1);
  else
    return (0);
}


static int in_circle_rad(int x, int y, int rad)
{
  if ((x * x) + (y * y) - (rad * rad) < 0)
    return (1);
  else
    return (0);
}


static int paintsound(int size)
{
  if (SND_PAINT1 + (size / 12) >= SND_PAINT4)
    return (SND_PAINT4);
  else
    return (SND_PAINT1 + (size / 12));
}


#ifndef NOSVG

#ifdef OLD_SVG

/* Old libcairo1, svg and svg-cairo based code
   Based on cairo-demo/sdl/main.c from Cairo (GPL'd, (c) 2004 Eric Windisch):
*/

static SDL_Surface *load_svg(char *file)
{
  svg_cairo_t *scr;
  int bpp, btpp, stride;
  unsigned int width, height;
  unsigned int rwidth, rheight;
  float scale;
  unsigned char *image;
  cairo_surface_t *cairo_surface;
  cairo_t *cr;
  SDL_Surface *sdl_surface, *sdl_surface_tmp;
  Uint32 rmask, gmask, bmask, amask;
  svg_cairo_status_t res;


#ifdef DEBUG
  printf("Attempting to load \"%s\" as an SVG\n", file);
#endif

  /* Create the SVG cairo stuff: */
  if (svg_cairo_create(&scr) != SVG_CAIRO_STATUS_SUCCESS)
    {
#ifdef DEBUG
      printf("svg_cairo_create() failed\n");
#endif
      return (NULL);
    }

  /* Parse the SVG file: */
  if (svg_cairo_parse(scr, file) != SVG_CAIRO_STATUS_SUCCESS)
    {
      svg_cairo_destroy(scr);
#ifdef DEBUG
      printf("svg_cairo_parse(%s) failed\n", file);
#endif
      return (NULL);
    }

  /* Get the natural size of the SVG */
  svg_cairo_get_size(scr, &rwidth, &rheight);
#ifdef DEBUG
  printf("svg_get_size(): %d x %d\n", rwidth, rheight);
#endif

  if (rwidth == 0 || rheight == 0)
    {
      svg_cairo_destroy(scr);
#ifdef DEBUG
      printf("SVG %s had 0 width or height!\n", file);
#endif
      return (NULL);
    }

  /* We will create a CAIRO_FORMAT_ARGB32 surface. We don't need to match
     the screen SDL format, but we are interested in the alpha bit... */
  bpp = 32;
  btpp = 4;

  /* We want to render at full Tux Paint canvas size, so that the stamp
     at its largest scale remains highest quality (no pixelization):
     (but not messing up the aspect ratio) */

  scale = pick_best_scape(rwidth, rheight, r_canvas.w, r_canvas.h);

  width = ((float)rwidth * scale);
  height = ((float)rheight * scale);

#ifdef DEBUG
  printf("scaling to %d x %d (%f scale)\n", width, height, scale);
#endif

  /* scanline width */
  stride = width * btpp;

  /* Allocate space for an image: */
  image = calloc(stride * height, 1);

#ifdef DEBUG
  printf("calling cairo_image_surface_create_for_data(..., CAIRO_FORMAT_ARGB32, %d(w), %d(h), %d(stride))\n", width,
         height, stride);
#endif

  /* Create the cairo surface with the adjusted width and height */

  cairo_surface = cairo_image_surface_create_for_data(image, CAIRO_FORMAT_ARGB32, width, height, stride);
  cr = cairo_create(cairo_surface);
  if (cr == NULL)
    {
      svg_cairo_destroy(scr);
#ifdef DEBUG
      printf("cairo_create() failed\n");
#endif
      return (NULL);
    }

  /* Scale it (proportionally) */
  cairo_scale(cr, scale, scale);        /* no return value :( */

  /* Render SVG to our surface: */
  res = svg_cairo_render(scr, cr);

  /* Clean up: */
  cairo_surface_destroy(cairo_surface);
  cairo_destroy(cr);
  svg_cairo_destroy(scr);

  if (res != SVG_CAIRO_STATUS_SUCCESS)
    {
#ifdef DEBUG
      printf("svg_cairo_render() failed\n");
#endif
      return (NULL);
    }


  /* Adjust the SDL surface to match the cairo surface created
     (surface mask of ARGB)  NOTE: Is this endian-agnostic? -bjk 2006.10.25 */
  rmask = 0x00ff0000;
  gmask = 0x0000ff00;
  bmask = 0x000000ff;
  amask = 0xff000000;

  /* Create the SDL surface using the pixel data stored: */
  sdl_surface_tmp = SDL_CreateRGBSurfaceFrom((void *)image, width, height, bpp, stride, rmask, gmask, bmask, amask);

  if (sdl_surface_tmp == NULL)
    {
#ifdef DEBUG
      printf("SDL_CreateRGBSurfaceFrom() failed\n");
#endif
      return (NULL);
    }


  /* Convert the SDL surface to the display format, for faster blitting: */
  sdl_surface = SDL_DisplayFormatAlpha(sdl_surface_tmp);
  SDL_FreeSurface(sdl_surface_tmp);

  if (sdl_surface == NULL)
    {
#ifdef DEBUG
      printf("SDL_DisplayFormatAlpha() failed\n");
#endif
      return (NULL);
    }

#ifdef DEBUG
  printf("SDL surface from %d x %d SVG is %d x %d\n", rwidth, rheight, sdl_surface->w, sdl_surface->h);
#endif

  return (sdl_surface);
}

#else

/* New libcairo2, rsvg and rsvg-cairo based code */
static SDL_Surface *load_svg(char *file)
{
  cairo_surface_t *cairo_surf;
  cairo_t *cr;
  RsvgHandle *rsvg_handle;
  GError *gerr;
  unsigned char *image;
  int rwidth, rheight;
  int width, height, stride;
  float scale;
  int bpp = 32, btpp = 4;
  RsvgDimensionData dimensions;
  SDL_Surface *sdl_surface, *sdl_surface_tmp;
  Uint32 rmask, gmask, bmask, amask;

#ifdef DEBUG
  printf("load_svg(%s)\n", file);
#endif

  /* Create an RSVG Handle from the SVG file: */

  gerr = NULL;

  rsvg_handle = rsvg_handle_new_from_file(file, &gerr);
  if (rsvg_handle == NULL)
    {
#ifdef DEBUG
      fprintf(stderr, "rsvg_handle_new_from_file() failed\n");
#endif
      return (NULL);
    }

  rsvg_handle_get_dimensions(rsvg_handle, &dimensions);
  rwidth = dimensions.width;
  rheight = dimensions.height;

#ifdef DEBUG
  printf("SVG is %d x %d\n", rwidth, rheight);
#endif


  /* Pick best scale to render to (for the canvas in this instance of Tux Paint) */

  scale = pick_best_scape(rwidth, rheight, r_canvas.w, r_canvas.h);

#ifdef DEBUG
  printf("best scale is %.4f\n", scale);
#endif

  width = ((float)rwidth * scale);
  height = ((float)rheight * scale);

#ifdef DEBUG
  printf("scaling to %d x %d (%f scale)\n", width, height, scale);
#endif

  /* scanline width */
  stride = width * btpp;

  /* Allocate space for an image: */
  image = calloc(stride * height, 1);
  if (image == NULL)
    {
#ifdef DEBUG
      fprintf(stderr, "Unable to allocate image buffer\n");
#endif
      rsvg_handle_close(rsvg_handle, &gerr);
      return (NULL);
    }


  /* Create a surface for Cairo to draw into: */

  cairo_surf = cairo_image_surface_create_for_data(image, CAIRO_FORMAT_ARGB32, width, height, stride);

  if (cairo_surface_status(cairo_surf) != CAIRO_STATUS_SUCCESS)
    {
#ifdef DEBUG
      fprintf(stderr, "cairo_image_surface_create() failed\n");
#endif
      rsvg_handle_close(rsvg_handle, &gerr);
      free(image);
      return (NULL);
    }


  /* Create a new Cairo object: */

  cr = cairo_create(cairo_surf);
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
    {
#ifdef DEBUG
      fprintf(stderr, "cairo_create() failed\n");
#endif
      rsvg_handle_close(rsvg_handle, &gerr);
      cairo_surface_destroy(cairo_surf);
      free(image);
      return (NULL);
    }


  /* Ask RSVG to render the SVG into the Cairo object: */

  cairo_scale(cr, scale, scale);

  /* FIXME: We can use cairo_rotate() here to rotate stamps! -bjk 2007.06.21 */

  rsvg_handle_render_cairo(rsvg_handle, cr);


  cairo_surface_finish(cairo_surf);


  /* Adjust the SDL surface to match the cairo surface created
     (surface mask of ARGB)  NOTE: Is this endian-agnostic? -bjk 2006.10.25 */
  rmask = 0x00ff0000;
  gmask = 0x0000ff00;
  bmask = 0x000000ff;
  amask = 0xff000000;

  /* Create the SDL surface using the pixel data stored: */
  sdl_surface_tmp = SDL_CreateRGBSurfaceFrom((void *)image, width, height, bpp, stride, rmask, gmask, bmask, amask);

  if (sdl_surface_tmp == NULL)
    {
#ifdef DEBUG
      fprintf(stderr, "SDL_CreateRGBSurfaceFrom() failed\n");
#endif
      rsvg_handle_close(rsvg_handle, &gerr);
      cairo_surface_destroy(cairo_surf);
      free(image);
      cairo_destroy(cr);
      return (NULL);
    }

  /* Convert the SDL surface to the display format, for faster blitting: */
  sdl_surface = SDL_DisplayFormatAlpha(sdl_surface_tmp);
  SDL_FreeSurface(sdl_surface_tmp);

  if (sdl_surface == NULL)
    {
#ifdef DEBUG
      fprintf(stderr, "SDL_DisplayFormatAlpha() failed\n");
#endif
      rsvg_handle_close(rsvg_handle, &gerr);
      cairo_surface_destroy(cairo_surf);
      free(image);
      cairo_destroy(cr);
      return (NULL);
    }


#ifdef DEBUG
  printf("SDL surface from %d x %d SVG is %d x %d\n", rwidth, rheight, sdl_surface->w, sdl_surface->h);
#endif


  /* Clean up: */

  rsvg_handle_close(rsvg_handle, &gerr);
  cairo_surface_destroy(cairo_surf);
  free(image);
  cairo_destroy(cr);

  return (sdl_surface);
}

#endif


static float pick_best_scape(unsigned int orig_w, unsigned int orig_h, unsigned int max_w, unsigned int max_h)
{
  float aspect, scale, wscale, hscale;

  aspect = (float)orig_w / (float)orig_h;

#ifdef DEBUG
  printf("trying to fit %d x %d (aspect: %.4f) into %d x %d\n", orig_w, orig_h, aspect, max_w, max_h);
#endif

  wscale = (float)max_w / (float)orig_w;
  hscale = (float)max_h / (float)orig_h;

#ifdef DEBUG
  printf("max_w / orig_w = wscale: %.4f\n", wscale);
  printf("max_h / orig_h = hscale: %.4f\n", hscale);
  printf("\n");
#endif

  if (aspect >= 1)
    {
      /* Image is wider-than-tall (or square) */

      scale = wscale;

#ifdef DEBUG
      printf("Wider-than-tall.  Using wscale.\n");
      printf("new size would be: %d x %d\n", (int)((float)orig_w * scale), (int)((float)orig_h * scale));
#endif

      if ((float)orig_h * scale > (float)max_h)
        {
          scale = hscale;

#ifdef DEBUG
          printf("Too tall!  Using hscale!\n");
          printf("new size would be: %d x %d\n", (int)((float)orig_w * scale), (int)((float)orig_h * scale));
#endif
        }
    }
  else
    {
      /* Taller-than-wide */

      scale = hscale;

#ifdef DEBUG
      printf("Taller-than-wide.  Using hscale.\n");
      printf("new size would be: %d x %d\n", (int)((float)orig_w * scale), (int)((float)orig_h * scale));
#endif

      if ((float)orig_w * scale > (float)max_w)
        {
          scale = wscale;

#ifdef DEBUG
          printf("Too wide!  Using wscale!\n");
          printf("new size would be: %d x %d\n", (int)((float)orig_w * scale), (int)((float)orig_h * scale));
#endif
        }
    }


#ifdef DEBUG
  printf("\n");
  printf("Final scale: %.4f\n", scale);
#endif

  return (scale);
}

#endif

/* FIXME: we can remove this after SDL folks fix their bug at http://bugzilla.libsdl.org/show_bug.cgi?id=1485 */
/* Try to load an image with IMG_Load(), if it fails, then try with RWops() */
static SDL_Surface *myIMG_Load_RWops(char *file)
{
  SDL_Surface *surf;
  FILE *fi;
  SDL_RWops *data;

  surf = IMG_Load(file);
  if (surf != NULL)
    return (surf);

  /* From load_kpx() */
  fi = fopen(file, "rb");
  if (fi == NULL)
    return NULL;

  data = SDL_RWFromFP(fi, 1);   /* 1 = Close when we're done */

  if (data == NULL)
    return (NULL);

  surf = IMG_Load_RW(data, 1);  /* 1 = Free when we're done */
  if (surf == NULL)
    return (NULL);

  return (surf);
}

/* Load an image; call load_svg() (above, to call Cairo and SVG-Cairo funcs)
   if we notice it's an SVG file (if available!);
   call load_kpx() if we notice it's a KPX file (JPEG with wrapper);
   otherwise call SDL_Image lib's IMG_Load() (for PNGs, JPEGs, BMPs, etc.) */
static SDL_Surface *myIMG_Load(char *file)
{
  if (strlen(file) > 4 && strcasecmp(file + strlen(file) - 4, ".kpx") == 0)
    {
      return (load_kpx(file));
#ifndef NOSVG
    }
  else if (strlen(file) > 4 && strcasecmp(file + strlen(file) - 4, ".svg") == 0)
    {
      return (load_svg(file));
#endif
    }
  else
    {
      return (myIMG_Load_RWops(file));
    }
}

static SDL_Surface *load_kpx(char *file)
{
  SDL_RWops *data;
  FILE *fi;
  SDL_Surface *surf;
  int i;

  fi = fopen(file, "rb");
  if (fi == NULL)
    return NULL;

  /* Skip header */
  for (i = 0; i < 60; i++)
    fgetc(fi);

  data = SDL_RWFromFP(fi, 1);   /* 1 = Close when we're done */

  if (data == NULL)
    return (NULL);

  surf = IMG_Load_RW(data, 1);  /* 1 = Free when we're done */
  if (surf == NULL)
    return (NULL);

  return (surf);
}


static void load_magic_plugins(void)
{
  int res, n, i, plc;
  char *place;
  int err;
  DIR *d;
  struct dirent *f;
  char fname[512];
  char objname[512];
  char funcname[512];

  num_plugin_files = 0;
  num_magics = 0;

  for (plc = 0; plc < NUM_MAGIC_PLACES; plc++)
    {
      if (plc == MAGIC_PLACE_GLOBAL)
        place = strdup(MAGIC_PREFIX);
      else if (plc == MAGIC_PLACE_LOCAL)
        place = get_fname("plugins/", DIR_DATA);
#ifdef __APPLE__
      else if (plc == MAGIC_PLACE_ALLUSERS)
        place = strdup("/Library/Application Support/TuxPaint/plugins/");
#endif
      else
        continue;               /* Huh? */

#ifdef DEBUG
      printf("\n");
      printf("Loading magic plug-ins from %s\n", place);
      fflush(stdout);
#endif

      /* Gather list of files (for sorting): */

      d = opendir(place);

      if (d != NULL)
        {
          /* Set magic API hooks: */

          magic_api_struct = (magic_api *) malloc(sizeof(magic_api));
          magic_api_struct->tp_version = strdup(VER_VERSION);

          if (plc == MAGIC_PLACE_GLOBAL)
            magic_api_struct->data_directory = strdup(DATA_PREFIX);
          else if (plc == MAGIC_PLACE_LOCAL)
            magic_api_struct->data_directory = get_fname("plugins/data/", DIR_DATA);
#ifdef __APPLE__
          else if (plc == MAGIC_PLACE_ALLUSERS)
            magic_api_struct->data_directory = strdup("/Library/Application Support/TuxPaint/plugins/data");
#endif
          else
            magic_api_struct->data_directory = strdup("./");

          magic_api_struct->update_progress_bar = update_progress_bar;
          magic_api_struct->sRGB_to_linear = magic_sRGB_to_linear;
          magic_api_struct->linear_to_sRGB = magic_linear_to_sRGB;
          magic_api_struct->in_circle = in_circle_rad;
          magic_api_struct->getpixel = magic_getpixel;
          magic_api_struct->putpixel = magic_putpixel;
          magic_api_struct->line = magic_line_func;
          magic_api_struct->playsound = magic_playsound;
          magic_api_struct->stopsound = magic_stopsound;
          magic_api_struct->special_notify = special_notify;
          magic_api_struct->button_down = magic_button_down;
          magic_api_struct->rgbtohsv = rgbtohsv;
          magic_api_struct->hsvtorgb = hsvtorgb;
          magic_api_struct->canvas_w = canvas->w;
          magic_api_struct->canvas_h = canvas->h;
          magic_api_struct->scale = magic_scale;
          magic_api_struct->touched = magic_touched;


          do
            {
              f = readdir(d);

              if (f != NULL)
                {
                  struct stat sbuf;

                  snprintf(fname, sizeof(fname), "%s%s", place, f->d_name);
                  if (!stat(fname, &sbuf) && S_ISREG(sbuf.st_mode))
                    {
                      /* Get just the name of the object (e.g., "negative"), w/o filename
                         extension: */

                      strcpy(objname, f->d_name);
                      strcpy(strchr(objname, '.'), "");

#if defined(__ANDROID__)
                      // since Android compiles magic tools with name like "libxxx.so", here we shall exclude the prefix "lib".
                      strcpy(objname, objname + 3);
#endif

                      magic_handle[num_plugin_files] = SDL_LoadObject(fname);

                      if (magic_handle[num_plugin_files] != NULL)
                        {
#ifdef DEBUG
                          printf("loading: %s\n", fname);
                          fflush(stdout);
#endif

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "get_tool_count");
                          magic_funcs[num_plugin_files].get_tool_count =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "get_name");
                          magic_funcs[num_plugin_files].get_name =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "get_icon");
                          magic_funcs[num_plugin_files].get_icon =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "get_description");
                          magic_funcs[num_plugin_files].get_description =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "requires_colors");
                          magic_funcs[num_plugin_files].requires_colors =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "modes");
                          magic_funcs[num_plugin_files].modes =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "set_color");
                          magic_funcs[num_plugin_files].set_color =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "init");
                          magic_funcs[num_plugin_files].init =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "api_version");
                          magic_funcs[num_plugin_files].api_version =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "shutdown");
                          magic_funcs[num_plugin_files].shutdown =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "click");
                          magic_funcs[num_plugin_files].click =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "drag");
                          magic_funcs[num_plugin_files].drag =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "release");
                          magic_funcs[num_plugin_files].release =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "switchin");
                          magic_funcs[num_plugin_files].switchin =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

                          snprintf(funcname, sizeof(funcname), "%s_%s", objname, "switchout");
                          magic_funcs[num_plugin_files].switchout =
                            SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

#ifdef DEBUG
                          //EP added (intptr_t) to avoid warning on x64 on all lines below
                          printf("get_tool_count = 0x%x\n",
                                 (int)(intptr_t) magic_funcs[num_plugin_files].get_tool_count);
                          printf("get_name = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].get_name);
                          printf("get_icon = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].get_icon);
                          printf("get_description = 0x%x\n",
                                 (int)(intptr_t) magic_funcs[num_plugin_files].get_description);
                          printf("requires_colors = 0x%x\n",
                                 (int)(intptr_t) magic_funcs[num_plugin_files].requires_colors);
                          printf("modes = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].modes);
                          printf("set_color = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].set_color);
                          printf("init = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].init);
                          printf("api_version = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].api_version);
                          printf("shutdown = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].shutdown);
                          printf("click = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].click);
                          printf("drag = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].drag);
                          printf("release = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].release);
                          printf("switchin = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].switchin);
                          printf("switchout = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].switchout);
#endif

                          err = 0;

                          if (magic_funcs[num_plugin_files].get_tool_count == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing get_tool_count\n", fname);
                              err = 1;
                            }
                          if (magic_funcs[num_plugin_files].get_name == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing get_name\n", fname);
                              err = 1;
                            }
                          if (magic_funcs[num_plugin_files].get_icon == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing get_icon\n", fname);
                              err = 1;
                            }
                          if (magic_funcs[num_plugin_files].get_description == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing get_description\n", fname);
                              err = 1;
                            }
                          if (magic_funcs[num_plugin_files].requires_colors == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing requires_colors\n", fname);
                              err = 1;
                            }
                          if (magic_funcs[num_plugin_files].modes == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing modes\n", fname);
                              err = 1;
                            }
                          if (magic_funcs[num_plugin_files].set_color == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing set_color\n", fname);
                              err = 1;
                            }
                          if (magic_funcs[num_plugin_files].init == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing init\n", fname);
                              err = 1;
                            }
                          if (magic_funcs[num_plugin_files].shutdown == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing shutdown\n", fname);
                              err = 1;
                            }
                          if (magic_funcs[num_plugin_files].click == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing click\n", fname);
                              err = 1;
                            }
                          if (magic_funcs[num_plugin_files].release == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing release\n", fname);
                              err = 1;
                            }
                          if (magic_funcs[num_plugin_files].switchin == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing switchin\n", fname);
                              err = 1;
                            }
                          if (magic_funcs[num_plugin_files].switchout == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing switchout\n", fname);
                              err = 1;
                            }
                          if (magic_funcs[num_plugin_files].drag == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing drag\n", fname);
                              err = 1;
                            }

                          if (magic_funcs[num_plugin_files].api_version == NULL)
                            {
                              fprintf(stderr, "Error: plugin %s is missing api_version\n", fname);
                              err = 1;
                            }
                          else if (magic_funcs[num_plugin_files].api_version() != TP_MAGIC_API_VERSION)
                            {
                              fprintf(stderr,
                                      "Warning: plugin %s uses Tux Paint 'Magic' tool API version %x,\nbut Tux Paint needs version %x.\n",
                                      fname, magic_funcs[num_plugin_files].api_version(), TP_MAGIC_API_VERSION);
                              err = 1;
                            }

                          if (err)
                            {
                              SDL_UnloadObject(magic_handle[num_plugin_files]);
                            }
                          else
                            {
                              res = magic_funcs[num_plugin_files].init(magic_api_struct);

                              if (res != 0)
                                n = magic_funcs[num_plugin_files].get_tool_count(magic_api_struct);
                              else
                                {
                                  magic_funcs[num_plugin_files].shutdown(magic_api_struct);
                                  n = 0;
                                }


                              if (n == 0)
                                {
                                  fprintf(stderr, "Error: plugin %s failed to startup or reported 0 magic tools\n",
                                          fname);
                                  fflush(stderr);
                                  SDL_UnloadObject(magic_handle[num_plugin_files]);
                                }
                              else
                                {
                                  int j;

                                  for (i = 0; i < n; i++)
                                    {
                                      magics[num_magics].idx = i;
                                      magics[num_magics].place = plc;
                                      magics[num_magics].handle_idx = num_plugin_files;
                                      magics[num_magics].name =
                                        magic_funcs[num_plugin_files].get_name(magic_api_struct, i);

                                      magics[num_magics].avail_modes =
                                        magic_funcs[num_plugin_files].modes(magic_api_struct, i);

                                      for (j = 0; j < MAX_MODES; j++)
                                        {
                                          magics[num_magics].tip[j] = NULL;
                                          if (j)
                                            {
                                              if (magics[num_magics].avail_modes & MODE_FULLSCREEN)
                                                magics[num_magics].tip[j] =
                                                  magic_funcs[num_plugin_files].get_description(magic_api_struct, i,
                                                                                                MODE_FULLSCREEN);
                                            }
                                          else
                                            {
                                              if (magics[num_magics].avail_modes & MODE_PAINT)
                                                magics[num_magics].tip[j] =
                                                  magic_funcs[num_plugin_files].get_description(magic_api_struct, i,
                                                                                                MODE_PAINT);
                                              else if (magics[num_magics].avail_modes & MODE_ONECLICK)
                                                magics[num_magics].tip[j] =
                                                  magic_funcs[num_plugin_files].get_description(magic_api_struct, i,
                                                                                                MODE_ONECLICK);
                                              else if (magics[num_magics].avail_modes & MODE_PAINT_WITH_PREVIEW)
                                                magics[num_magics].tip[j] =
                                                  magic_funcs[num_plugin_files].get_description(magic_api_struct, i,
                                                                                                MODE_PAINT_WITH_PREVIEW);
                                            }
                                        }

                                      magics[num_magics].colors =
                                        magic_funcs[num_plugin_files].requires_colors(magic_api_struct, i);
                                      if (magics[num_magics].avail_modes & MODE_PAINT)
                                        magics[num_magics].mode = MODE_PAINT;
                                      else if (magics[num_magics].avail_modes & MODE_ONECLICK)
                                        magics[num_magics].mode = MODE_ONECLICK;
                                      else if (magics[num_magics].avail_modes & MODE_PAINT_WITH_PREVIEW)
                                        magics[num_magics].mode = MODE_PAINT_WITH_PREVIEW;
                                      else
                                        magics[num_magics].mode = MODE_FULLSCREEN;

                                      magics[num_magics].img_icon =
                                        magic_funcs[num_plugin_files].get_icon(magic_api_struct, i);

#ifdef DEBUG
                                      printf("-- %s\n", magics[num_magics].name);
                                      printf("avail_modes = %d\n", magics[num_magics].avail_modes);
#endif

                                      num_magics++;
                                    }

                                  num_plugin_files++;
                                }
                            }
                        }
                      else
                        {
                          fprintf(stderr, "Warning: Failed to load object %s: %s\n", fname, SDL_GetError());
                          fflush(stderr);
                        }
                    }
                }
            }
          while (f != NULL);

          closedir(d);
        }
    }


  qsort(magics, num_magics, sizeof(magic_t), magic_sort);

#ifdef DEBUG
  printf("Loaded %d magic tools from %d plug-in files\n", num_magics, num_plugin_files);
  printf("\n");
  fflush(stdout);
#endif
}



static int magic_sort(const void *a, const void *b)
{
  magic_t *am = (magic_t *) a;
  magic_t *bm = (magic_t *) b;

  return (strcoll(gettext(am->name), gettext(bm->name)));
}


static void update_progress_bar(void)
{
  show_progress_bar(screen);
}

static void magic_line_func(void *mapi,
                            int which, SDL_Surface * canvas, SDL_Surface * last,
                            int x1, int y1, int x2, int y2, int step,
                            void (*cb) (void *, int, SDL_Surface *, SDL_Surface *, int, int))
{
  int dx, dy, y;
  float m, b;
  int cnt;

  dx = x2 - x1;
  dy = y2 - y1;

  cnt = step - 1;

  if (dx != 0)
    {
      m = ((float)dy) / ((float)dx);
      b = y1 - m * x1;

      if (x2 >= x1)
        dx = 1;
      else
        dx = -1;


      while (x1 != x2)
        {
          y1 = m * x1 + b;
          y2 = m * (x1 + dx) + b;

          if (y1 > y2)
            {
              for (y = y1; y >= y2; y--)
                {
                  cnt = (cnt + 1) % step;
                  if (cnt == 0)
                    cb((void *)mapi, which, canvas, last, x1, y);
                }
            }
          else
            {
              for (y = y1; y <= y2; y++)
                {
                  cnt = (cnt + 1) % step;
                  if (cnt == 0)
                    cb((void *)mapi, which, canvas, last, x1, y);
                }
            }

          x1 = x1 + dx;
        }
    }
  else
    {
      if (y1 > y2)
        {
          for (y = y1; y >= y2; y--)
            {
              cnt = (cnt + 1) % step;
              if (cnt == 0)
                cb((void *)mapi, which, canvas, last, x1, y);
            }
        }
      else
        {
          for (y = y1; y <= y2; y++)
            {
              cnt = (cnt + 1) % step;
              if (cnt == 0)
                cb((void *)mapi, which, canvas, last, x1, y);
            }
        }
    }

  /* FIXME: Set and return an update rect? */
}


/* Handle special things that some magic tools do that
   need to affect more than just the current canvas: */

static void special_notify(int flags)
{
  int tmp_int;

  tmp_int = cur_undo - 1;
  if (tmp_int < 0)
    tmp_int = NUM_UNDO_BUFS - 1;

  if (flags & SPECIAL_MIRROR)
    {
      /* Mirror starter, too! */

      starter_mirrored = !starter_mirrored;

      if (img_starter != NULL)
        mirror_starter();

      undo_starters[tmp_int] = UNDO_STARTER_MIRRORED;
    }

  if (flags & SPECIAL_FLIP)
    {
      /* Flip starter, too! */

      starter_flipped = !starter_flipped;

      if (img_starter != NULL)
        flip_starter();

      undo_starters[tmp_int] = UNDO_STARTER_FLIPPED;
    }
}

static void magic_stopsound(void)
{
#ifndef NOSOUND
  if (mute || !use_sound)
    return;

  Mix_HaltChannel(0);
#endif
}

static void magic_playsound(Mix_Chunk * snd, int left_right, int up_down)
{
#ifndef NOSOUND

  int left, dist;


  /* Don't play if sound is disabled (nosound), or sound is temporarily
     muted (Alt+S), or sound ptr is NULL */

  if (mute || !use_sound || snd == NULL)
    return;


  /* Don't override the same sound, if it's already playing */

  if (!Mix_Playing(0) || magic_current_snd_ptr != snd)
    Mix_PlayChannel(0, snd, 0);

  magic_current_snd_ptr = snd;


  /* Adjust panning */

  if (up_down < 0)
    up_down = 0;
  else if (up_down > 255)
    up_down = 255;

  dist = 255 - up_down;

  if (left_right < 0)
    left_right = 0;
  else if (left_right > 255)
    left_right = 255;

  left = ((255 - dist) * (255 - left_right)) / 255;

  Mix_SetPanning(0, left, (255 - dist) - left);
#endif
}

static Uint8 magic_linear_to_sRGB(float lin)
{
  return (linear_to_sRGB(lin));
}

static float magic_sRGB_to_linear(Uint8 srgb)
{
  return (sRGB_to_linear_table[srgb]);
}

static int magic_button_down(void)
{
  return (button_down || emulate_button_pressed);
}

static SDL_Surface *magic_scale(SDL_Surface * surf, int w, int h, int aspect)
{
  return (thumbnail2(surf, w, h, aspect, 1));
}

/* FIXME: This, do_open() and do_slideshow() should be combined and modularized! */

static int do_new_dialog(void)
{
  SDL_Surface *img, *img1, *img2;
  int things_alloced;
  SDL_Surface **thumbs = NULL;
  DIR *d;
  struct dirent *f;
  struct dirent2 *fs;
  int place, oldplace;
  char *dirname[NUM_PLACES_TO_LOOK];
  char **d_names = NULL, **d_exts = NULL;
  int *d_places;
  FILE *fi;
  char fname[1024];
  int num_files, i, done, update_list, cur, which, num_files_in_dirs, j;
  SDL_Rect dest;
  SDL_Event event;
  SDLKey key;
  Uint32 last_click_time;
  int last_click_which, last_click_button;
  int places_to_look;
  int tot;
  int first_starter, first_template;
  int added;
  Uint8 r, g, b;
  int white_in_palette;
  int val_x, val_y, motioner;
  int valhat_x, valhat_y, hatmotioner;
  int skip, k;


  val_x = val_y = motioner = 0;
  valhat_x = valhat_y = hatmotioner = 0;
  do_setcursor(cursor_watch);

  /* Allocate some space: */

  things_alloced = 32;

  fs = (struct dirent2 *)malloc(sizeof(struct dirent2) * things_alloced);

  num_files = 0;
  cur = 0;
  which = 0;
  num_files_in_dirs = 0;


  /* Open directories of images: */

  for (places_to_look = 0; places_to_look < NUM_PLACES_TO_LOOK; places_to_look++)
    {
      if (places_to_look == PLACE_SAVED_DIR)
        {
          /* Skip saved images; only want starters! */
          dirname[places_to_look] = NULL;
          continue;             /* ugh */
        }
      else if (places_to_look == PLACE_PERSONAL_STARTERS_DIR)
        {
          /* Check for coloring-book style 'starter' images in our folder: */

          dirname[places_to_look] = get_fname("starters", DIR_DATA);
        }
      else if (places_to_look == PLACE_STARTERS_DIR)
        {
          /* Finally, check for system-wide coloring-book style
             'starter' images: */

          dirname[places_to_look] = strdup(DATA_PREFIX "starters");
        }
      else if (places_to_look == PLACE_PERSONAL_TEMPLATES_DIR)
        {
          /* Check for 'template' images in our folder: */

          dirname[places_to_look] = get_fname("templates", DIR_DATA);
        }
      else if (places_to_look == PLACE_TEMPLATES_DIR)
        {
          /* Finally, check for system-wide 'template' images: */

          dirname[places_to_look] = strdup(DATA_PREFIX "templates");
        }


      /* Read directory of images and build thumbnails: */

      d = opendir(dirname[places_to_look]);

      if (d != NULL)
        {
          /* Gather list of files (for sorting): */

          do
            {
              f = readdir(d);

              if (f != NULL)
                {
                  memcpy(&(fs[num_files_in_dirs].f), f, sizeof(struct dirent));
                  fs[num_files_in_dirs].place = places_to_look;

                  num_files_in_dirs++;

                  if (num_files_in_dirs >= things_alloced)
                    {
                      things_alloced = things_alloced + 32;

                      fs = (struct dirent2 *)realloc(fs, sizeof(struct dirent2) * things_alloced);
                    }
                }
            }
          while (f != NULL);

          closedir(d);
        }
#ifdef __ANDROID__
      else if (dirname[places_to_look][0] != "/")
        {
          /* Try inside android assets only if it is a relative path */

          AAssetDir *ad = open_asset_dir(dirname[places_to_look]);
          const char *afilename = (const char *)NULL;

          while ((afilename = AAssetDir_getNextFileName(ad)) != NULL)
            {
              f = malloc(sizeof(struct dirent));
              strncpy(f->d_name, afilename, sizeof(f->d_name));

              memcpy(&(fs[num_files_in_dirs].f), f, sizeof(struct dirent));
              fs[num_files_in_dirs].place = places_to_look;
              free(f);
              num_files_in_dirs++;

              if (num_files_in_dirs >= things_alloced)
                {
                  things_alloced = things_alloced + 32;

                  fs = (struct dirent2 *)realloc(fs, sizeof(struct dirent2) * things_alloced);
                }
            }
        }
#endif
    }


  /* (Re)allocate space for the information about these files: */

  tot = num_files_in_dirs + NUM_COLORS;

  thumbs = (SDL_Surface * *)malloc(sizeof(SDL_Surface *) * tot);
  d_places = (int *)malloc(sizeof(int) * tot);
  d_names = (char **)malloc(sizeof(char *) * tot);
  d_exts = (char **)malloc(sizeof(char *) * tot);


  /* Sort: */

  qsort(fs, num_files_in_dirs, sizeof(struct dirent2), (int (*)(const void *, const void *))compare_dirent2s);


  /* Throw the color palette at the beginning: */

  white_in_palette = -1;

  for (j = -1; j < NUM_COLORS; j++)
    {
      added = 0;

      if (j < NUM_COLORS - 1)
        {
          if (j == -1 ||        /* (short circuit) */
              color_hexes[j][0] != 255 ||       /* Ignore white, we'll have already added it */
              color_hexes[j][1] != 255 || color_hexes[j][2] != 255)
            {
              /* Palette colors: */

              thumbs[num_files] = SDL_CreateRGBSurface(screen->flags,
                                                       THUMB_W - 20, THUMB_H - 20,
                                                       screen->format->BitsPerPixel,
                                                       screen->format->Rmask,
                                                       screen->format->Gmask, screen->format->Bmask, 0);

              if (thumbs[num_files] != NULL)
                {
                  if (j == -1)
                    {
                      r = g = b = 255;  /* White */
                    }
                  else
                    {
                      r = color_hexes[j][0];
                      g = color_hexes[j][1];
                      b = color_hexes[j][2];
                    }
                  SDL_FillRect(thumbs[num_files], NULL, SDL_MapRGB(thumbs[num_files]->format, r, g, b));
                  added = 1;
                }
            }
          else
            {
              white_in_palette = j;
            }
        }
      else
        {
          /* Color picker: */

          thumbs[num_files] = thumbnail(img_color_picker, THUMB_W - 20, THUMB_H - 20, 0);
          added = 1;
        }

      if (added)
        {
          d_places[num_files] = PLACE_COLOR_PALETTE;
          d_names[num_files] = NULL;
          d_exts[num_files] = NULL;

          num_files++;
        }
    }

  first_starter = num_files;
  first_template = -1;          /* In case there are none... */


  /* Read directory of images and build thumbnails: */

  oldplace = -1;

  for (j = 0; j < num_files_in_dirs; j++)
    {
      f = &(fs[j].f);
      place = fs[j].place;

      if ((place == PLACE_PERSONAL_TEMPLATES_DIR || place == PLACE_TEMPLATES_DIR) && oldplace != place)
        first_template = num_files;

      oldplace = place;


      show_progress_bar(screen);

      if (f != NULL)
        {
          debug(f->d_name);

          if (strcasestr(f->d_name, "-t.") == NULL && strcasestr(f->d_name, "-back.") == NULL)
            {
              if (strcasestr(f->d_name, FNAME_EXTENSION) != NULL
                  /* Support legacy BMP files for load: */
                  || strcasestr(f->d_name, ".bmp") != NULL
                  /* Support for KPX (Kid Pix templates; just a JPEG with resource fork header): */
                  || strcasestr(f->d_name, ".kpx") != NULL || strcasestr(f->d_name, ".jpg") != NULL
#ifndef NOSVG
                  || strcasestr(f->d_name, ".svg") != NULL
#endif
                )
                {
                  strcpy(fname, f->d_name);
                  skip = 0;

                  if (strcasestr(fname, FNAME_EXTENSION) != NULL)
                    {
                      d_exts[num_files] = strdup(strcasestr(fname, FNAME_EXTENSION));
                      strcpy((char *)strcasestr(fname, FNAME_EXTENSION), "");
                    }

                  if (strcasestr(fname, ".bmp") != NULL)
                    {
                      d_exts[num_files] = strdup(strcasestr(fname, ".bmp"));
                      strcpy((char *)strcasestr(fname, ".bmp"), "");
                    }

#ifndef NOSVG
                  if (strcasestr(fname, ".svg") != NULL)
                    {
                      d_exts[num_files] = strdup(strcasestr(fname, ".svg"));
                      strcpy((char *)strcasestr(fname, ".svg"), "");
                    }
#endif

                  if (strcasestr(fname, ".kpx") != NULL)
                    {
                      d_exts[num_files] = strdup(strcasestr(fname, ".kpx"));
                      strcpy((char *)strcasestr(fname, ".kpx"), "");
                    }

                  if (strcasestr(fname, ".jpg") != NULL)
                    {
                      d_exts[num_files] = strdup(strcasestr(fname, ".jpg"));
                      strcpy((char *)strcasestr(fname, ".jpg"), "");
                    }

#ifndef NOSVG
                  /* If identically-named SVG exists, skip this version */
                  for (k = 0; k < num_files_in_dirs; k++)
                    {
                      if (k != j)
                        {
                          struct dirent *f2;
                          char fname2[1024];

                          f2 = &(fs[k].f);
                          strcpy(fname2, f2->d_name);

                          if (strstr(fname2, fname) == fname2 && strcasestr(fname2, ".svg") != NULL)
                            {
                              /* SVG of this bitmap exists; we'll skip it */
                              skip = 1;
                            }
                        }
                    }
#endif
                  if (skip)
                    {
                      free(d_exts[num_files]);
                    }
                  else
                    {
                      d_names[num_files] = strdup(fname);
                      d_places[num_files] = place;


                      /* Try to load thumbnail first: */

                      snprintf(fname, sizeof(fname), "%s/.thumbs/%s-t.png",
                               dirname[d_places[num_files]], d_names[num_files]);
                      debug(fname);
                      img = IMG_Load(fname);

                      if (img == NULL)
                        {
                          /* No thumbnail in the new location ("saved/.thumbs"),
                             try the old location ("saved/"): */

                          snprintf(fname, sizeof(fname), "%s/%s-t.png",
                                   dirname[d_places[num_files]], d_names[num_files]);
                          debug(fname);

                          img = IMG_Load(fname);
                        }

                      if (img != NULL)
                        {
                          /* Loaded the thumbnail from one or the other location */
                          show_progress_bar(screen);

                          img1 = SDL_DisplayFormat(img);
                          SDL_FreeSurface(img);

                          /* if too big, or too small in both dimensions, rescale it
                             (for now: using old thumbnail as source for high speed,
                             low quality) */
                          if (img1->w > THUMB_W - 20 || img1->h > THUMB_H - 20
                              || (img1->w < THUMB_W - 20 && img1->h < THUMB_H - 20))
                            {
                              img2 = thumbnail(img1, THUMB_W - 20, THUMB_H - 20, 0);
                              SDL_FreeSurface(img1);
                              img1 = img2;
                            }

                          thumbs[num_files] = img1;

                          if (thumbs[num_files] == NULL)
                            {
                              fprintf(stderr,
                                      "\nError: Couldn't create a thumbnail of " "saved image!\n" "%s\n", fname);
                            }

                          num_files++;
                        }
                      else
                        {
                          /* No thumbnail - load original: */

                          /* Make sure we have a ~/.tuxpaint/saved directory: */
                          if (make_directory("saved", "Can't create user data directory"))
                            {
                              /* (Make sure we have a .../saved/.thumbs/ directory:) */
                              make_directory("saved/.thumbs", "Can't create user data thumbnail directory");
                            }

                          img = NULL;

                          if (d_places[num_files] == PLACE_STARTERS_DIR ||
                              d_places[num_files] == PLACE_PERSONAL_STARTERS_DIR)
                            {
                              /* Try to load a starter's background image, first!
                                 If it exists, it should give a better idea of what the
                                 starter looks like, compared to the overlay image... */

                              /* FIXME: Add .jpg support -bjk 2007.03.22 */

                              /* (Try JPEG first) */
                              snprintf(fname, sizeof(fname), "%s/%s-back",
                                       dirname[d_places[num_files]], d_names[num_files]);
                              img = load_starter_helper(fname, "jpeg", &IMG_Load);
                              if (img == NULL)
                                {
                                  snprintf(fname, sizeof(fname), "%s/%s-back",
                                           dirname[d_places[num_files]], d_names[num_files]);
                                  img = load_starter_helper(fname, "jpg", &IMG_Load);
                                }

#ifndef NOSVG
                              if (img == NULL)
                                {
                                  /* (Try SVG next) */
                                  snprintf(fname, sizeof(fname), "%s/%s-back",
                                           dirname[d_places[num_files]], d_names[num_files]);
                                  img = load_starter_helper(fname, "svg", &load_svg);
                                }
#endif

                              if (img == NULL)
                                {
                                  /* (Try PNG next) */
                                  snprintf(fname, sizeof(fname), "%s/%s-back",
                                           dirname[d_places[num_files]], d_names[num_files]);
                                  img = load_starter_helper(fname, "png", &IMG_Load);
                                }
                            }

                          if (img == NULL)
                            {
                              /* Didn't load a starter background (or didn't try!),
                                 try loading the actual image... */

                              snprintf(fname, sizeof(fname), "%s/%s", dirname[d_places[num_files]], f->d_name);
                              debug(fname);
                              img = myIMG_Load(fname);
                            }


                          show_progress_bar(screen);

                          if (img == NULL)
                            {
                              fprintf(stderr,
                                      "\nWarning: I can't open one of the saved files!\n"
                                      "%s\n"
                                      "The Simple DirectMedia Layer error that "
                                      "occurred was:\n" "%s\n\n", fname, SDL_GetError());

                              free(d_names[num_files]);
                              free(d_exts[num_files]);
                            }
                          else
                            {
                              /* Turn it into a thumbnail: */

                              img1 = SDL_DisplayFormatAlpha(img);
                              img2 = thumbnail2(img1, THUMB_W - 20, THUMB_H - 20, 0, 0);
                              SDL_FreeSurface(img1);

                              show_progress_bar(screen);

                              thumbs[num_files] = SDL_DisplayFormat(img2);
                              SDL_FreeSurface(img2);
                              if (thumbs[num_files] == NULL)
                                {
                                  fprintf(stderr,
                                          "\nError: Couldn't create a thumbnail of " "saved image!\n" "%s\n", fname);
                                }

                              SDL_FreeSurface(img);

                              show_progress_bar(screen);


                              /* Let's save this thumbnail, so we don't have to
                                 create it again next time 'Open' is called: */
                              /* if (d_places[num_files] == PLACE_SAVED_DIR) *//* <-- FIXME: This test should probably go...? -bjk 2009.10.15 */

                              if (d_places[num_files] == PLACE_PERSONAL_STARTERS_DIR || /* We must check to not try to write to system wide dirs  Pere 2010.3.25 */
                                  d_places[num_files] == PLACE_PERSONAL_TEMPLATES_DIR)
                                {
                                  debug("Saving thumbnail for this one!");

                                  snprintf(fname, sizeof(fname), "%s/.thumbs/%s-t.png",
                                           dirname[d_places[num_files]], d_names[num_files]);

                                  if (!make_directory("starters", "Can't create user data directory") ||
                                      !make_directory("templates", "Can't create user data directory") ||
                                      !make_directory("starters/.thumbs", "Can't create user data directory") ||
                                      !make_directory("templates/.thumbs", "Can't create user data directory"))
                                    fprintf(stderr, "Cannot save any pictures! SORRY!\n\n");
                                  else
                                    {
                                      fi = fopen(fname, "wb");
                                      if (fi == NULL)
                                        {
                                          fprintf(stderr,
                                                  "\nError: Couldn't save thumbnail of "
                                                  "saved image!\n"
                                                  "%s\n"
                                                  "The error that occurred was:\n" "%s\n\n", fname, strerror(errno));
                                        }
                                      else
                                        {
                                          do_png_save(fi, fname, thumbs[num_files], 0);
                                        }
                                    }

                                  show_progress_bar(screen);
                                }


                              num_files++;
                            }
                        }
                    }
                }
            }
          else
            {
              /* It was a thumbnail file ("...-t.png") or immutable scene starter's
                 overlay layer ("...-front.png") */
            }
        }
    }



#ifdef DEBUG
  printf("%d files were found!\n", num_files);
#endif


  /* Let user choose a color or image: */

  /* Instructions for 'New' file/color dialog */
  draw_tux_text(TUX_BORED, tool_tips[TOOL_NEW], 1);

  /* NOTE: cur is now set above; if file_id'th file is found, it's
     set to that file's index; otherwise, we default to '0' */

  update_list = 1;

  done = 0;

  last_click_which = -1;
  last_click_time = 0;
  last_click_button = -1;


  do_setcursor(cursor_arrow);


  do
    {
      /* Update screen: */

      if (update_list)
        {
          /* Erase screen: */

          dest.x = 96;
          dest.y = 0;
          dest.w = WINDOW_WIDTH - 96 - 96;
          dest.h = 48 * 7 + 40 + HEIGHTOFFSET;

          SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));


          /* Draw icons: */

          for (i = cur; i < cur + 16 && i < num_files; i++)
            {
              /* Draw cursor: */

              dest.x = THUMB_W * ((i - cur) % 4) + 96;
              dest.y = THUMB_H * ((i - cur) / 4) + 24;

              if (d_places[i] == PLACE_SAVED_DIR)
                {
                  if (i == which)
                    {
                      SDL_BlitSurface(img_cursor_down, NULL, screen, &dest);
                      debug(d_names[i]);
                    }
                  else
                    SDL_BlitSurface(img_cursor_up, NULL, screen, &dest);
                }
              else
                {
                  if (i == which)
                    {
                      SDL_BlitSurface(img_cursor_starter_down, NULL, screen, &dest);
                      debug(d_names[i]);
                    }
                  else
                    SDL_BlitSurface(img_cursor_starter_up, NULL, screen, &dest);
                }



              dest.x = THUMB_W * ((i - cur) % 4) + 96 + 10 + (THUMB_W - 20 - thumbs[i]->w) / 2;
              dest.y = THUMB_H * ((i - cur) / 4) + 24 + 10 + (THUMB_H - 20 - thumbs[i]->h) / 2;

              if (thumbs[i] != NULL)
                SDL_BlitSurface(thumbs[i], NULL, screen, &dest);
            }


          /* Draw arrows: */

          dest.x = (WINDOW_WIDTH - img_scroll_up->w) / 2;
          dest.y = 0;

          if (cur > 0)
            SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
          else
            SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);

          dest.x = (WINDOW_WIDTH - img_scroll_up->w) / 2;
          dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48;

          if (cur < num_files - 16)
            SDL_BlitSurface(img_scroll_down, NULL, screen, &dest);
          else
            SDL_BlitSurface(img_scroll_down_off, NULL, screen, &dest);


          /* "Open" button: */

          dest.x = 96;
          dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48;
          SDL_BlitSurface(img_open, NULL, screen, &dest);

          dest.x = 96 + (48 - img_openlabels_open->w) / 2;
          dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - img_openlabels_open->h;
          SDL_BlitSurface(img_openlabels_open, NULL, screen, &dest);


          /* "Back" button: */

          dest.x = WINDOW_WIDTH - 96 - 48;
          dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - 48;
          SDL_BlitSurface(img_back, NULL, screen, &dest);

          dest.x = WINDOW_WIDTH - 96 - 48 + (48 - img_openlabels_back->w) / 2;
          dest.y = (48 * 7 + 40 + HEIGHTOFFSET) - img_openlabels_back->h;
          SDL_BlitSurface(img_openlabels_back, NULL, screen, &dest);


          SDL_Flip(screen);

          update_list = 0;
        }

      /* Was a call to SDL_WaitEvent(&event); before,
         changed to this while loop in order to get joystick working */
      while (SDL_PollEvent(&event))
        {
          if (event.type == SDL_QUIT)
            {
              done = 1;

              /* FIXME: Handle SDL_Quit better */
            }
          else if (event.type == SDL_WINDOWEVENT)
            {
              handle_active(&event);
            }
          else if (event.type == SDL_KEYUP)
            {
              key = event.key.keysym.sym;

              handle_keymouse(key, SDL_KEYUP, 24, NULL, NULL);
            }
          else if (event.type == SDL_KEYDOWN)
            {
              key = event.key.keysym.sym;

              handle_keymouse(key, SDL_KEYDOWN, 24, NULL, NULL);

              /* Moved from LEFT RIGHT UP DOWN to F11 F12 F8 F7 */

              if (key == SDLK_F11)
                {
                  if (which > 0)
                    {
                      which--;

                      if (which < cur)
                        cur = cur - 4;

                      update_list = 1;
                    }
                }
              else if (key == SDLK_F12)
                {
                  if (which < num_files - 1)
                    {
                      which++;

                      if (which >= cur + 16)
                        cur = cur + 4;

                      update_list = 1;
                    }
                }
              else if (key == SDLK_F8)
                {
                  if (which >= 0)
                    {
                      which = which - 4;

                      if (which < 0)
                        which = 0;

                      if (which < cur)
                        cur = cur - 4;

                      update_list = 1;
                    }
                }
              else if (key == SDLK_F7)
                {
                  if (which < num_files)
                    {
                      which = which + 4;

                      if (which >= num_files)
                        which = num_files - 1;

                      if (which >= cur + 16)
                        cur = cur + 4;

                      update_list = 1;
                    }
                }
              else if (key == SDLK_RETURN)
                {
                  /* Open */

                  done = 1;
                  playsound(screen, 1, SND_CLICK, 1, SNDPOS_LEFT, SNDDIST_NEAR);
                }
              else if (key == SDLK_ESCAPE || key == SDLK_AC_BACK)
                {
                  /* Go back: */

                  which = -1;
                  done = 1;
                  playsound(screen, 1, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
                }
            }
          else if (event.type == SDL_MOUSEBUTTONDOWN && valid_click(event.button.button))
            {
              if (event.button.x >= 96 && event.button.x < WINDOW_WIDTH - 96 &&
                  event.button.y >= 24 && event.button.y < (48 * 7 + 40 + HEIGHTOFFSET - 48))
                {
                  /* Picked an icon! */

                  which = ((event.button.x - 96) / (THUMB_W) + (((event.button.y - 24) / THUMB_H) * 4)) + cur;

                  if (which < num_files)
                    {
                      playsound(screen, 1, SND_BLEEP, 1, event.button.x, SNDDIST_NEAR);
                      update_list = 1;


                      if (which == last_click_which &&
                          SDL_GetTicks() < last_click_time + 1000 && event.button.button == last_click_button)
                        {
                          /* Double-click! */

                          done = 1;
                        }

                      last_click_which = which;
                      last_click_time = SDL_GetTicks();
                      last_click_button = event.button.button;
                    }
                }
              else if (event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2 &&
                       event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2)
                {
                  if (event.button.y < 24)
                    {
                      /* Up scroll button: */

                      if (cur > 0)
                        {
                          cur = cur - 4;
                          update_list = 1;
                          playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                          if (cur == 0)
                            do_setcursor(cursor_arrow);
                        }

                      if (which >= cur + 16)
                        which = which - 4;
                    }
                  else if (event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET - 48) &&
                           event.button.y < (48 * 7 + 40 + HEIGHTOFFSET - 24))
                    {
                      /* Down scroll button: */

                      if (cur < num_files - 16)
                        {
                          cur = cur + 4;
                          update_list = 1;
                          playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                          if (cur >= num_files - 16)
                            do_setcursor(cursor_arrow);
                        }

                      if (which < cur)
                        which = which + 4;
                    }
                }
              else if (event.button.x >= 96 && event.button.x < 96 + 48 &&
                       event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET) - 48 &&
                       event.button.y < (48 * 7 + 40 + HEIGHTOFFSET))
                {
                  /* Open */

                  done = 1;
                  playsound(screen, 1, SND_CLICK, 1, SNDPOS_LEFT, SNDDIST_NEAR);
                }
              else if (event.button.x >= (WINDOW_WIDTH - 96 - 48) &&
                       event.button.x < (WINDOW_WIDTH - 96) &&
                       event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET) - 48 &&
                       event.button.y < (48 * 7 + 40 + HEIGHTOFFSET))
                {
                  /* Back */

                  which = -1;
                  done = 1;
                  playsound(screen, 1, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
                }
#ifdef __ANDROID__
              start_motion_convert(event);
#endif
            }
          else if (event.type == SDL_MOUSEBUTTONUP)
            {
#ifdef __ANDROID__
              stop_motion_convert(event);
#endif
            }
          else if (event.type == SDL_MOUSEWHEEL && wheely)
            {
              /* Scroll wheel! */

              if (event.wheel.y > 0 && cur > 0)
                {
                  cur = cur - 4;
                  update_list = 1;
                  playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                  if (cur == 0)
                    do_setcursor(cursor_arrow);

                  if (which >= cur + 16)
                    which = which - 4;
                }
              else if (event.wheel.y < 0 && cur < num_files - 16)
                {
                  cur = cur + 4;
                  update_list = 1;
                  playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                  if (cur >= num_files - 16)
                    do_setcursor(cursor_arrow);

                  if (which < cur)
                    which = which + 4;
                }
            }
          else if (event.type == SDL_MOUSEMOTION)
            {
              /* Deal with mouse pointer shape! */

              if (event.button.y < 24 &&
                  event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2 &&
                  event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2 && cur > 0)
                {
                  /* Scroll up button: */

                  do_setcursor(cursor_up);
                }
              else if (event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET - 48) &&
                       event.button.y < (48 * 7 + 40 + HEIGHTOFFSET - 24) &&
                       event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2 &&
                       event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2 && cur < num_files - 16)
                {
                  /* Scroll down button: */

                  do_setcursor(cursor_down);
                }
              else if (((event.button.x >= 96 && event.button.x < 96 + 48 + 48) ||
                        (event.button.x >= (WINDOW_WIDTH - 96 - 48) &&
                         event.button.x < (WINDOW_WIDTH - 96)) ||
                        (event.button.x >= (WINDOW_WIDTH - 96 - 48 - 48) &&
                         event.button.x < (WINDOW_WIDTH - 48 - 96) &&
                         d_places[which] != PLACE_STARTERS_DIR &&
                         d_places[which] != PLACE_PERSONAL_STARTERS_DIR)) &&
                       event.button.y >= (48 * 7 + 40 + HEIGHTOFFSET) - 48 &&
                       event.button.y < (48 * 7 + 40 + HEIGHTOFFSET))
                {
                  /* One of the command buttons: */

                  do_setcursor(cursor_hand);
                }
              else if (event.button.x >= 96 && event.button.x < WINDOW_WIDTH - 96 &&
                       event.button.y > 24 &&
                       event.button.y < (48 * 7 + 40 + HEIGHTOFFSET) - 48 &&
                       ((((event.button.x - 96) / (THUMB_W) +
                          (((event.button.y - 24) / THUMB_H) * 4)) + cur) < num_files))
                {
                  /* One of the thumbnails: */

                  do_setcursor(cursor_hand);
                }
              else
                {
                  /* Unclickable... */

                  do_setcursor(cursor_arrow);
                }

#ifdef __ANDROID__
              convert_motion_to_wheel(event);
#endif

              oldpos_x = event.button.x;
              oldpos_y = event.button.y;
            }

          else if (event.type == SDL_JOYAXISMOTION)
            handle_joyaxismotion(event, &motioner, &val_x, &val_y);

          else if (event.type == SDL_JOYHATMOTION)
            handle_joyhatmotion(event, oldpos_x, oldpos_y, &valhat_x, &valhat_y, &hatmotioner, &old_hat_ticks);

          else if (event.type == SDL_JOYBALLMOTION)
            handle_joyballmotion(event, oldpos_x, oldpos_y);

          else if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP)
            handle_joybuttonupdown(event, oldpos_x, oldpos_y);
        }

      if (motioner | hatmotioner)
        handle_motioners(oldpos_x, oldpos_y, motioner, hatmotioner, old_hat_ticks, val_x, val_y, valhat_x, valhat_y);

      SDL_Delay(10);
    }
  while (!done);


  /* Load the chosen starter, or start with a blank solid color: */

  if (which != -1)
    {
      /* Save old one first? */

      if (!been_saved && !disable_save)
        {
          if (do_prompt_image_snd(PROMPT_OPEN_SAVE_TXT,
                                  PROMPT_OPEN_SAVE_YES,
                                  PROMPT_OPEN_SAVE_NO,
                                  img_tools[TOOL_SAVE], NULL, NULL, SND_AREYOUSURE, screen->w / 2, screen->h / 2))
            {
              do_save(TOOL_NEW, 1, 0);
            }
        }

      /* Clear label surface */

      SDL_FillRect(label, NULL, SDL_MapRGBA(label->format, 0, 0, 0, 0));

      /* Clear all info related to label surface */

      delete_label_list(&start_label_node);
      start_label_node = current_label_node = first_label_node_in_redo_stack = highlighted_label_node =
        label_node_to_edit = NULL;
      have_to_rec_label_node = FALSE;

      if (which >= first_starter && (first_template == -1 || which < first_template))
        {
          /* Load a starter: */

          /* Figure out filename: */

          snprintf(fname, sizeof(fname), "%s/%s%s", dirname[d_places[which]], d_names[which], d_exts[which]);

          img = myIMG_Load(fname);

          if (img == NULL)
            {
              fprintf(stderr,
                      "\nWarning: Couldn't load the saved image! (3)\n"
                      "%s\n"
                      "The Simple DirectMedia Layer error that occurred " "was:\n" "%s\n\n", fname, SDL_GetError());

              do_prompt(PROMPT_OPEN_UNOPENABLE_TXT, PROMPT_OPEN_UNOPENABLE_YES, "", 0, 0);
            }
          else
            {
              free_surface(&img_starter);
              free_surface(&img_starter_bkgd);
              starter_mirrored = 0;
              starter_flipped = 0;
              starter_personal = 0;
              starter_modified = 0;

              autoscale_copy_smear_free(img, canvas, SDL_BlitSurface);

              cur_undo = 0;
              oldest_undo = 0;
              newest_undo = 0;

              /* Immutable 'starter' image;
                 we'll need to save a new image when saving...: */

              been_saved = 1;

              file_id[0] = '\0';
              strcpy(starter_id, d_names[which]);
              template_id[0] = '\0';

              if (d_places[which] == PLACE_PERSONAL_STARTERS_DIR)
                starter_personal = 1;
              else
                starter_personal = 0;

              load_starter(starter_id);

              canvas_color_r = 255;
              canvas_color_g = 255;
              canvas_color_b = 255;

              SDL_FillRect(canvas, NULL, SDL_MapRGB(canvas->format, 255, 255, 255));
              SDL_BlitSurface(img_starter_bkgd, NULL, canvas, NULL);
              SDL_BlitSurface(img_starter, NULL, canvas, NULL);
            }
        }
      else if (first_template != -1 && which >= first_template)
        {
          /* Load a template: */

          /* Figure out filename: */

          snprintf(fname, sizeof(fname), "%s/%s%s", dirname[d_places[which]], d_names[which], d_exts[which]);
          img = myIMG_Load(fname);

          if (img == NULL)
            {
              fprintf(stderr,
                      "\nWarning: Couldn't load the saved image! (4)\n"
                      "%s\n"
                      "The Simple DirectMedia Layer error that occurred " "was:\n" "%s\n\n", fname, SDL_GetError());

              do_prompt(PROMPT_OPEN_UNOPENABLE_TXT, PROMPT_OPEN_UNOPENABLE_YES, "", 0, 0);
            }
          else
            {
              free_surface(&img_starter);
              free_surface(&img_starter_bkgd);
              template_personal = 0;

              autoscale_copy_smear_free(img, canvas, SDL_BlitSurface);

              cur_undo = 0;
              oldest_undo = 0;
              newest_undo = 0;

              /* Immutable 'template' image;
                 we'll need to save a new image when saving...: */

              been_saved = 1;

              file_id[0] = '\0';
              strcpy(template_id, d_names[which]);
              starter_id[0] = '\0';

              if (d_places[which] == PLACE_PERSONAL_TEMPLATES_DIR)
                template_personal = 1;
              else
                template_personal = 0;

              load_template(template_id);

              canvas_color_r = 255;
              canvas_color_g = 255;
              canvas_color_b = 255;

              SDL_FillRect(canvas, NULL, SDL_MapRGB(canvas->format, 255, 255, 255));
              SDL_BlitSurface(img_starter_bkgd, NULL, canvas, NULL);
            }
        }
      else
        {
          /* A color! */

          free_surface(&img_starter);
          free_surface(&img_starter_bkgd);
          starter_mirrored = 0;
          starter_flipped = 0;
          starter_personal = 0;
          starter_modified = 0;

          /* Launch color picker if they chose that: */

          if (which == NUM_COLORS - 1)
            {
              if (do_color_picker() == 0)
                return (0);
            }

          /* FIXME: Don't do anything and go back to Open dialog if they
             hit BACK in color picker! */

          if (which == 0)       /* White */
            {
              canvas_color_r = canvas_color_g = canvas_color_b = 255;
            }
          else if (which <= white_in_palette)   /* One of the colors before white in the pallete */
            {
              canvas_color_r = color_hexes[which - 1][0];
              canvas_color_g = color_hexes[which - 1][1];
              canvas_color_b = color_hexes[which - 1][2];
            }
          else
            {
              canvas_color_r = color_hexes[which][0];
              canvas_color_g = color_hexes[which][1];
              canvas_color_b = color_hexes[which][2];
            }

          SDL_FillRect(canvas, NULL, SDL_MapRGB(canvas->format, canvas_color_r, canvas_color_g, canvas_color_b));

          cur_undo = 0;
          oldest_undo = 0;
          newest_undo = 0;

          been_saved = 1;
          reset_avail_tools();

          tool_avail_bak[TOOL_UNDO] = 0;
          tool_avail_bak[TOOL_REDO] = 0;

          file_id[0] = '\0';
          starter_id[0] = '\0';

          playsound(screen, 1, SND_HARP, 1, SNDPOS_CENTER, SNDDIST_NEAR);
        }
    }

  update_canvas(0, 0, WINDOW_WIDTH - 96 - 96, 48 * 7 + 40 + HEIGHTOFFSET);


  /* Clean up: */

  free_surface_array(thumbs, num_files);

  free(thumbs);

  for (i = 0; i < num_files; i++)
    {
      if (d_names[i] != NULL)
        free(d_names[i]);
      if (d_exts[i] != NULL)
        free(d_exts[i]);
    }

  for (i = 0; i < NUM_PLACES_TO_LOOK; i++)
    if (dirname[i] != NULL)
      free(dirname[i]);

  free(d_names);
  free(d_exts);
  free(d_places);

  return (which != -1);
}

/* FIXME: Use a bitmask! */

static void reset_touched(void)
{
  int x, y;

  for (y = 0; y < canvas->h; y++)
    {
      for (x = 0; x < canvas->w; x++)
        {
          touched[(y * canvas->w) + x] = 0;
        }
    }
}

static Uint8 magic_touched(int x, int y)
{
  Uint8 res;

  if (x < 0 || x >= canvas->w || y < 0 || y >= canvas->h)
    return (1);

  res = touched[(y * canvas->w) + x];
  touched[(y * canvas->w) + x] = 1;

  return (res);
}

static int do_color_sel(void)
{
#ifndef NO_PROMPT_SHADOWS
  SDL_Surface *alpha_surf;
#endif
  SDL_Rect dest;
  int x, y, w;
  int ox, oy;
  int val_x, val_y, motioner;
  int valhat_x, valhat_y, hatmotioner;
  int stepx, stepy;
  int number_of_steps = 20;
  int i, dx, dy;
  int done, chose;
  int back_left, back_top;
  int color_sel_x, color_sel_y;
  SDL_Surface *tmp_btn_up, *tmp_btn_down;

  Uint32(*getpixel_tmp_btn_up) (SDL_Surface *, int, int);
  Uint32(*getpixel_tmp_btn_down) (SDL_Surface *, int, int);
  Uint32(*getpixel_img_paintwell) (SDL_Surface *, int, int);
  Uint32(*getpixel_img_color_picker) (SDL_Surface *, int, int);
  Uint8 r, g, b;
  double rh, gh, bh;
  SDL_Event event;
  SDLKey key;
  SDL_Rect r_color_sel;
  SDL_Rect color_example_dest;
  SDL_Surface *backup;
  SDL_Rect r_color_picker;

  val_x = val_y = motioner = 0;
  valhat_x = valhat_y = hatmotioner = 0;

  /* FIXME this is the first step to make animated popups optional,
     to be removed from here when implemented in a more general way */
  int want_animated_popups = 1;

  hide_blinking_cursor();

  do_setcursor(cursor_hand);


  /* Draw button box: */

  playsound(screen, 0, SND_PROMPT, 1, SNDPOS_RIGHT, 128);

  backup = SDL_CreateRGBSurface(screen->flags, screen->w, screen->h,
                                screen->format->BitsPerPixel,
                                screen->format->Rmask,
                                screen->format->Gmask, screen->format->Bmask, screen->format->Amask);

  SDL_BlitSurface(screen, NULL, backup, NULL);

  r_color_sel.x = r_canvas.x;
  r_color_sel.y = r_canvas.h;
  r_color_sel.w = r_canvas.w;
  r_color_sel.h = button_w;


  if (want_animated_popups)
    {
      /* center of button */
      ox = WINDOW_WIDTH - color_button_w - color_button_w / 2;
      oy = r_colors.y + r_colors.h / 2;
      dx = 0;
      dy = 0;
      w = 0;
      stepx = (r_color_sel.x - ox) / number_of_steps;
      stepy = (r_color_sel.y - oy) / number_of_steps;

      for (i = 0; i < number_of_steps; i++)
        {
          w = w + (128 + 6 + 4) / number_of_steps;
          dx = i * stepx;
          dy = i * stepy;


          dest.x = ox + dx;
          dest.y = oy + dy;

          dest.w = i * r_color_sel.w / number_of_steps;
          dest.h = i * r_color_sel.h / number_of_steps;

          SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255 - w, 255 - w, 255 - w));
          SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);
          SDL_Delay(2);
        }

      SDL_BlitSurface(backup, NULL, screen, NULL);
    }


#ifndef NO_PROMPT_SHADOWS
  alpha_surf = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                    r_color_sel.w,
                                    r_color_sel.h,
                                    screen->format->BitsPerPixel,
                                    screen->format->Rmask,
                                    screen->format->Gmask, screen->format->Bmask, screen->format->Amask);

  if (alpha_surf != NULL)
    {
      SDL_FillRect(alpha_surf, NULL, SDL_MapRGB(alpha_surf->format, 0, 0, 0));
      SDL_SetSurfaceAlphaMod(alpha_surf, 64);

      for (i = 8; i > 0; i = i - 2)
        {
          dest.x = r_color_sel.x + i;
          dest.y = r_color_sel.y + i;
          dest.w = r_color_sel.w;
          dest.h = r_color_sel.h;

          SDL_BlitSurface(alpha_surf, NULL, screen, &dest);
        }

      SDL_FreeSurface(alpha_surf);
    }
#endif


  /* Draw prompt box: */

  SDL_FillRect(screen, &r_color_sel, SDL_MapRGB(screen->format, 255, 255, 255));



  /* Determine spot for example color: */


  color_example_dest.x = r_color_sel.x + 2;
  color_example_dest.y = r_color_sel.y + 2;
  color_example_dest.w = r_color_sel.w - button_w - 8;
  color_example_dest.h = r_color_sel.h - 4;


  SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, 0, 0, 0));

  color_example_dest.x += 2;
  color_example_dest.y += 2;
  color_example_dest.w -= 4;
  color_example_dest.h -= 4;

  SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, 255, 255, 255));

  color_example_dest.x += 2;
  color_example_dest.y += 2;
  color_example_dest.w -= 4;
  color_example_dest.h -= 4;



  /* Draw current color picker color: */

  SDL_FillRect(screen, &color_example_dest,
               SDL_MapRGB(screen->format,
                          color_hexes[NUM_COLORS - 2][0],
                          color_hexes[NUM_COLORS - 2][1], color_hexes[NUM_COLORS - 2][2]));



  /* Show "Back" button */

  back_left = r_color_sel.x + r_color_sel.w - button_w - 4;
  back_top = r_color_sel.y;

  dest.x = back_left;
  dest.y = back_top;

  SDL_BlitSurface(img_back, NULL, screen, &dest);

  dest.x = back_left + (img_back->w - img_openlabels_back->w) / 2;
  dest.y = back_top + img_back->h - img_openlabels_back->h;
  SDL_BlitSurface(img_openlabels_back, NULL, screen, &dest);



  /* Blit canvas on screen */
  SDL_BlitSurface(canvas, NULL, screen, &r_canvas);

  SDL_Flip(screen);


  /* Let the user pick a color, or go back: */

  done = 0;
  chose = 0;
  x = y = 0;
#ifndef __ANDROID__
  /* FIXME: Strangely, this SDL_WarpMouse makes further event.button.x/y to be 0 on Android, thus making the selector unresponsive.
     Needs testing on other operating sistems with touchscreen.  */
  SDL_WarpMouse(r_color_sel.x + r_color_sel.w / 2, r_color_sel.y + r_color_sel.h / 2);
#endif
  do
    {
      while (SDL_PollEvent(&event))
        {
          if (event.type == SDL_QUIT)
            {
              chose = 0;
              done = 1;
            }
          else if (event.type == SDL_WINDOWEVENT)
            {
              handle_active(&event);
            }
          else if (event.type == SDL_KEYUP)
            {
              key = event.key.keysym.sym;

              handle_keymouse(key, SDL_KEYUP, 24, NULL, NULL);
            }
          else if (event.type == SDL_KEYDOWN)
            {
              key = event.key.keysym.sym;

              handle_keymouse(key, SDL_KEYDOWN, 24, &r_color_picker, NULL);

              if (key == SDLK_ESCAPE)
                {
                  chose = 0;
                  done = 1;
                }
            }
          else if (event.type == SDL_MOUSEBUTTONUP && valid_click(event.button.button))
            {
              if (event.button.x >= r_canvas.x &&
                  event.button.x < r_canvas.x + r_canvas.w &&
                  event.button.y >= r_canvas.y && event.button.y < r_canvas.y + r_canvas.h)
                {
                  /* Picked a color! */

                  chose = 1;
                  done = 1;

                  x = event.button.x - r_canvas.x;
                  y = event.button.y - r_canvas.y;

                  color_sel_x = x;
                  color_sel_y = y;
                }
              else if (event.button.x >= back_left &&
                       event.button.x < back_left + img_back->w &&
                       event.button.y >= back_top && event.button.y < back_top + img_back->h)
                {
                  /* Decided to go Back */

                  chose = 0;
                  done = 1;
                }
            }
          else if (event.type == SDL_MOUSEMOTION)
            {
              if (event.button.x >= r_canvas.x &&
                  event.button.x < r_canvas.x + r_canvas.w &&
                  event.button.y >= r_canvas.y && event.button.y < r_canvas.y + r_canvas.h)
                {
                  /* Hovering over the colors! */

                  do_setcursor(cursor_hand);


                  /* Show a big solid example of the color: */

                  x = event.button.x - r_canvas.x;
                  y = event.button.y - r_canvas.y;

                  getpixel_img_color_picker = getpixels[canvas->format->BytesPerPixel];
                  SDL_GetRGB(getpixel_img_color_picker(canvas, x, y), canvas->format, &r, &g, &b);

                  SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, r, g, b));

                  SDL_UpdateRect(screen,
                                 color_example_dest.x,
                                 color_example_dest.y, color_example_dest.w, color_example_dest.h);
                }
              else
                {
                  /* Revert to current color picker color, so we know what it was,
                     and what we'll get if we go Back: */

                  SDL_FillRect(screen, &color_example_dest,
                               SDL_MapRGB(screen->format,
                                          color_hexes[NUM_COLORS - 2][0],
                                          color_hexes[NUM_COLORS - 2][1], color_hexes[NUM_COLORS - 2][2]));

                  SDL_UpdateRect(screen,
                                 color_example_dest.x,
                                 color_example_dest.y, color_example_dest.w, color_example_dest.h);


                  /* Change cursor to arrow (or hand, if over Back): */

                  if (event.button.x >= back_left &&
                      event.button.x < back_left + img_back->w &&
                      event.button.y >= back_top && event.button.y < back_top + img_back->h)
                    do_setcursor(cursor_hand);
                  else
                    do_setcursor(cursor_arrow);
                }

              oldpos_x = event.motion.x;
              oldpos_y = event.motion.y;
            }
          else if (event.type == SDL_JOYAXISMOTION)
            handle_joyaxismotion(event, &motioner, &val_x, &val_y);

          else if (event.type == SDL_JOYHATMOTION)
            handle_joyhatmotion(event, oldpos_x, oldpos_y, &valhat_x, &valhat_y, &hatmotioner, &old_hat_ticks);

          else if (event.type == SDL_JOYBALLMOTION)
            handle_joyballmotion(event, oldpos_x, oldpos_y);

          else if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP)
            handle_joybuttonupdown(event, oldpos_x, oldpos_y);
        }

      if (motioner | hatmotioner)
        handle_motioners(oldpos_x, oldpos_y, motioner, hatmotioner, old_hat_ticks, val_x, val_y, valhat_x, valhat_y);

      SDL_Delay(10);
    }
  while (!done);


  /* Set the new color: */

  if (chose)
    {
      getpixel_img_color_picker = getpixels[canvas->format->BytesPerPixel];
      SDL_GetRGB(getpixel_img_color_picker(canvas, color_sel_x, color_sel_y), canvas->format, &r, &g, &b);

      color_hexes[NUM_COLORS - 2][0] = r;
      color_hexes[NUM_COLORS - 2][1] = g;
      color_hexes[NUM_COLORS - 2][2] = b;


      /* Re-render color picker to show the current color it contains: */

      tmp_btn_up = thumbnail(img_btn_up, color_button_w, color_button_h, 0);
      tmp_btn_down = thumbnail(img_btn_down, color_button_w, color_button_h, 0);
      img_color_btn_off = thumbnail(img_btn_off, color_button_w, color_button_h, 0);

      getpixel_tmp_btn_up = getpixels[tmp_btn_up->format->BytesPerPixel];
      getpixel_tmp_btn_down = getpixels[tmp_btn_down->format->BytesPerPixel];
      getpixel_img_paintwell = getpixels[img_paintwell->format->BytesPerPixel];

      rh = sRGB_to_linear_table[color_hexes[NUM_COLORS - 2][0]];
      gh = sRGB_to_linear_table[color_hexes[NUM_COLORS - 2][1]];
      bh = sRGB_to_linear_table[color_hexes[NUM_COLORS - 2][2]];



      SDL_LockSurface(img_color_btns[NUM_COLORS - 2]);
      SDL_LockSurface(img_color_btns[NUM_COLORS - 2 + NUM_COLORS]);

      for (y = 0; y < tmp_btn_up->h /* 48 */ ; y++)
        {
          for (x = 0; x < tmp_btn_up->w; x++)
            {
              double ru, gu, bu, rd, gd, bd, aa;
              Uint8 a;

              SDL_GetRGB(getpixel_tmp_btn_up(tmp_btn_up, x, y), tmp_btn_up->format, &r, &g, &b);

              ru = sRGB_to_linear_table[r];
              gu = sRGB_to_linear_table[g];
              bu = sRGB_to_linear_table[b];
              SDL_GetRGB(getpixel_tmp_btn_down(tmp_btn_down, x, y), tmp_btn_down->format, &r, &g, &b);

              rd = sRGB_to_linear_table[r];
              gd = sRGB_to_linear_table[g];
              bd = sRGB_to_linear_table[b];
              SDL_GetRGBA(getpixel_img_paintwell(img_paintwell, x, y), img_paintwell->format, &r, &g, &b, &a);

              aa = a / 255.0;

              if (a == 255)
                {
                  putpixels[img_color_btns[NUM_COLORS - 2]->format->BytesPerPixel]
                    (img_color_btns[NUM_COLORS - 2], x, y,
                     SDL_MapRGB(img_color_btns[i]->format,
                                linear_to_sRGB(rh * aa + ru * (1.0 - aa)),
                                linear_to_sRGB(gh * aa + gu * (1.0 - aa)), linear_to_sRGB(bh * aa + bu * (1.0 - aa))));

                  putpixels[img_color_btns[NUM_COLORS - 2 + NUM_COLORS]->format->BytesPerPixel]
                    (img_color_btns[NUM_COLORS - 2 + NUM_COLORS], x, y,
                     SDL_MapRGB(img_color_btns[i + NUM_COLORS]->format,
                                linear_to_sRGB(rh * aa + rd * (1.0 - aa)),
                                linear_to_sRGB(gh * aa + gd * (1.0 - aa)), linear_to_sRGB(bh * aa + bd * (1.0 - aa))));
                }
            }
        }

      SDL_UnlockSurface(img_color_btns[NUM_COLORS - 2]);
      SDL_UnlockSurface(img_color_btns[NUM_COLORS - 2 + NUM_COLORS]);

      dest.x = (img_color_btns[NUM_COLORS - 2]->w - img_color_sel->w) / 2;
      dest.y = (img_color_btns[NUM_COLORS - 2]->h - img_color_sel->h) / 2;
      dest.w = img_color_sel->w;
      dest.h = img_color_sel->h;
      SDL_BlitSurface(img_color_sel, NULL, img_color_btns[NUM_COLORS - 2], &dest);

      dest.x = (img_color_btns[NUM_COLORS - 2 + NUM_COLORS]->w - img_color_sel->w) / 2;
      dest.y = (img_color_btns[NUM_COLORS - 2 + NUM_COLORS]->h - img_color_sel->h) / 2;
      SDL_BlitSurface(img_color_sel, NULL, img_color_btns[NUM_COLORS - 2 + NUM_COLORS], &dest);
    }

  return (chose);
}

static int do_color_picker(void)
{
#ifndef NO_PROMPT_SHADOWS
  int i;
  SDL_Surface *alpha_surf;
#endif
  SDL_Rect dest;
  int x, y, w;
  int ox, oy, oox, ooy, nx, ny;
  int val_x, val_y, motioner;
  int valhat_x, valhat_y, hatmotioner;
  SDL_Surface *tmp_btn_up, *tmp_btn_down;

  Uint32(*getpixel_tmp_btn_up) (SDL_Surface *, int, int);
  Uint32(*getpixel_tmp_btn_down) (SDL_Surface *, int, int);
  Uint32(*getpixel_img_paintwell) (SDL_Surface *, int, int);
  Uint32(*getpixel_img_color_picker) (SDL_Surface *, int, int);
  Uint8 r, g, b;
  double rh, gh, bh;
  int done, chose;
  SDL_Event event;
  SDLKey key;
  int color_picker_left, color_picker_top;
  int back_left, back_top;
  SDL_Rect color_example_dest;
  SDL_Surface *backup;
  SDL_Rect r_color_picker;

  val_x = val_y = motioner = 0;
  valhat_x = valhat_y = hatmotioner = 0;
  hide_blinking_cursor();

  do_setcursor(cursor_hand);


  /* Draw button box: */

  playsound(screen, 0, SND_PROMPT, 1, SNDPOS_RIGHT, 128);

  backup = SDL_CreateRGBSurface(screen->flags, screen->w, screen->h,
                                screen->format->BitsPerPixel,
                                screen->format->Rmask,
                                screen->format->Gmask, screen->format->Bmask, screen->format->Amask);

  SDL_BlitSurface(screen, NULL, backup, NULL);

  ox = screen->w;
  oy = r_colors.y + r_colors.h / 2;

  for (w = 0; w <= 128 + 6 + 4; w = w + 4)
    {
      oox = ox - w;
      ooy = oy - w;

      nx = PROMPT_LEFT + 96 - w + PROMPTOFFSETX;
      ny = 94 + 96 - w + PROMPTOFFSETY;

      dest.x = ((nx * w) + (oox * (128 - w))) / 128;
      dest.y = ((ny * w) + (ooy * (128 - w))) / 128;

      dest.w = (PROMPT_W - 96 * 2) + w * 2;
      dest.h = w * 2;
      SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255 - w, 255 - w, 255 - w));


      SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);
      if (w % 16 == 0)
        SDL_Delay(1);
    }

  SDL_BlitSurface(backup, NULL, screen, NULL);

#ifndef NO_PROMPT_SHADOWS
  alpha_surf = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                    (PROMPT_W - 96 * 2) + (w - 4) * 2,
                                    (w - 4) * 2,
                                    screen->format->BitsPerPixel,
                                    screen->format->Rmask,
                                    screen->format->Gmask, screen->format->Bmask, screen->format->Amask);

  if (alpha_surf != NULL)
    {
      SDL_FillRect(alpha_surf, NULL, SDL_MapRGB(alpha_surf->format, 0, 0, 0));
      SDL_SetSurfaceAlphaMod(alpha_surf, 64);


      for (i = 8; i > 0; i = i - 2)
        {
          dest.x = PROMPT_LEFT + 96 - (w - 4) + i + PROMPTOFFSETX;
          dest.y = 94 + 96 - (w - 4) + i + PROMPTOFFSETY;
          dest.w = (PROMPT_W - 96 * 2) + (w - 4) * 2;
          dest.h = (w - 4) * 2;

          SDL_BlitSurface(alpha_surf, NULL, screen, &dest);
        }

      SDL_FreeSurface(alpha_surf);
    }
#endif


  /* Draw prompt box: */

  w = w - 6;

  dest.x = PROMPT_LEFT + 96 - w + PROMPTOFFSETX;
  dest.y = 94 + 96 - w + PROMPTOFFSETY;
  dest.w = (PROMPT_W - 96 * 2) + w * 2;
  dest.h = w * 2;
  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));


  /* Draw color palette: */

  color_picker_left = PROMPT_LEFT + 96 - w + PROMPTOFFSETX + 2;
  color_picker_top = 94 + 96 - w + PROMPTOFFSETY + 2;

  dest.x = color_picker_left;
  dest.y = color_picker_top;

  SDL_BlitSurface(img_color_picker, NULL, screen, &dest);

  r_color_picker.x = dest.x;
  r_color_picker.y = dest.y;
  r_color_picker.w = dest.w;
  r_color_picker.h = dest.h;


  /* Draw last color position: */

  dest.x = color_picker_x + color_picker_left - 3;
  dest.y = color_picker_y + color_picker_top - 1;
  dest.w = 7;
  dest.h = 3;

  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));

  dest.x = color_picker_x + color_picker_left - 1;
  dest.y = color_picker_y + color_picker_top - 3;
  dest.w = 3;
  dest.h = 7;

  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));

  dest.x = color_picker_x + color_picker_left - 2;
  dest.y = color_picker_y + color_picker_top;
  dest.w = 5;
  dest.h = 1;

  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));

  dest.x = color_picker_x + color_picker_left;
  dest.y = color_picker_y + color_picker_top - 2;
  dest.w = 1;
  dest.h = 5;

  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));


  /* Determine spot for example color: */

  color_example_dest.x = color_picker_left + img_color_picker->w + 2;
  color_example_dest.y = color_picker_top + 2;
  color_example_dest.w = (PROMPT_W - 96 * 2) + w * 2 - img_color_picker->w - 6;
  color_example_dest.h = 124;


  SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, 0, 0, 0));

  color_example_dest.x += 2;
  color_example_dest.y += 2;
  color_example_dest.w -= 4;
  color_example_dest.h -= 4;

  SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, 255, 255, 255));

  color_example_dest.x += 2;
  color_example_dest.y += 2;
  color_example_dest.w -= 4;
  color_example_dest.h -= 4;


  /* Draw current color picker color: */

  SDL_FillRect(screen, &color_example_dest,
               SDL_MapRGB(screen->format,
                          color_hexes[NUM_COLORS - 1][0],
                          color_hexes[NUM_COLORS - 1][1], color_hexes[NUM_COLORS - 1][2]));



  /* Show "Back" button */

  back_left =
    (((PROMPT_W - 96 * 2) + w * 2 - img_color_picker->w) - img_back->w) / 2 + color_picker_left + img_color_picker->w;
  back_top = color_picker_top + img_color_picker->h - img_back->h - 2;

  dest.x = back_left;
  dest.y = back_top;

  SDL_BlitSurface(img_back, NULL, screen, &dest);

  dest.x = back_left + (img_back->w - img_openlabels_back->w) / 2;
  dest.y = back_top + img_back->h - img_openlabels_back->h;
  SDL_BlitSurface(img_openlabels_back, NULL, screen, &dest);


  SDL_Flip(screen);


  /* Let the user pick a color, or go back: */

  done = 0;
  chose = 0;
  x = y = 0;
  SDL_WarpMouse(back_left + button_w / 2, back_top - button_w / 2);

  do
    {
      while (SDL_PollEvent(&event))
        {
          if (event.type == SDL_QUIT)
            {
              chose = 0;
              done = 1;
            }
          else if (event.type == SDL_WINDOWEVENT)
            {
              handle_active(&event);
            }
          else if (event.type == SDL_KEYUP)
            {
              key = event.key.keysym.sym;

              handle_keymouse(key, SDL_KEYUP, 24, NULL, NULL);
            }
          else if (event.type == SDL_KEYDOWN)
            {
              key = event.key.keysym.sym;

              handle_keymouse(key, SDL_KEYDOWN, 24, &r_color_picker, NULL);

              if (key == SDLK_ESCAPE || key == SDLK_AC_BACK)
                {
                  chose = 0;
                  done = 1;
                }
            }
          else if (event.type == SDL_MOUSEBUTTONUP && valid_click(event.button.button))
            {
              if (event.button.x >= color_picker_left &&
                  event.button.x < color_picker_left + img_color_picker->w &&
                  event.button.y >= color_picker_top && event.button.y < color_picker_top + img_color_picker->h)
                {
                  /* Picked a color! */

                  chose = 1;
                  done = 1;

                  x = event.button.x - color_picker_left;
                  y = event.button.y - color_picker_top;

                  color_picker_x = x;
                  color_picker_y = y;
                }
              else if (event.button.x >= back_left &&
                       event.button.x < back_left + img_back->w &&
                       event.button.y >= back_top && event.button.y < back_top + img_back->h)
                {
                  /* Decided to go Back */

                  chose = 0;
                  done = 1;
                }
            }
          else if (event.type == SDL_MOUSEMOTION)
            {
              if (event.button.x >= color_picker_left &&
                  event.button.x < color_picker_left + img_color_picker->w &&
                  event.button.y >= color_picker_top && event.button.y < color_picker_top + img_color_picker->h)
                {
                  /* Hovering over the colors! */

                  do_setcursor(cursor_hand);


                  /* Show a big solid example of the color: */

                  x = event.button.x - color_picker_left;
                  y = event.button.y - color_picker_top;

                  getpixel_img_color_picker = getpixels[img_color_picker->format->BytesPerPixel];
                  SDL_GetRGB(getpixel_img_color_picker(img_color_picker, x, y), img_color_picker->format, &r, &g, &b);

                  SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, r, g, b));

                  SDL_UpdateRect(screen,
                                 color_example_dest.x,
                                 color_example_dest.y, color_example_dest.w, color_example_dest.h);
                }
              else
                {
                  /* Revert to current color picker color, so we know what it was,
                     and what we'll get if we go Back: */

                  SDL_FillRect(screen, &color_example_dest,
                               SDL_MapRGB(screen->format,
                                          color_hexes[NUM_COLORS - 1][0],
                                          color_hexes[NUM_COLORS - 1][1], color_hexes[NUM_COLORS - 1][2]));

                  SDL_UpdateRect(screen,
                                 color_example_dest.x,
                                 color_example_dest.y, color_example_dest.w, color_example_dest.h);


                  /* Change cursor to arrow (or hand, if over Back): */

                  if (event.button.x >= back_left &&
                      event.button.x < back_left + img_back->w &&
                      event.button.y >= back_top && event.button.y < back_top + img_back->h)
                    do_setcursor(cursor_hand);
                  else
                    do_setcursor(cursor_arrow);
                }

              oldpos_x = event.motion.x;
              oldpos_y = event.motion.y;
            }
          else if (event.type == SDL_JOYAXISMOTION)
            handle_joyaxismotion(event, &motioner, &val_x, &val_y);

          else if (event.type == SDL_JOYHATMOTION)
            handle_joyhatmotion(event, oldpos_x, oldpos_y, &valhat_x, &valhat_y, &hatmotioner, &old_hat_ticks);

          else if (event.type == SDL_JOYBALLMOTION)
            handle_joyballmotion(event, oldpos_x, oldpos_y);

          else if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP)
            handle_joybuttonupdown(event, oldpos_x, oldpos_y);
        }

      if (motioner | hatmotioner)
        handle_motioners(oldpos_x, oldpos_y, motioner, hatmotioner, old_hat_ticks, val_x, val_y, valhat_x, valhat_y);

      SDL_Delay(10);
    }
  while (!done);


  /* Set the new color: */

  if (chose)
    {
      getpixel_img_color_picker = getpixels[img_color_picker->format->BytesPerPixel];
      SDL_GetRGB(getpixel_img_color_picker(img_color_picker, x, y), img_color_picker->format, &r, &g, &b);

      color_hexes[NUM_COLORS - 1][0] = r;
      color_hexes[NUM_COLORS - 1][1] = g;
      color_hexes[NUM_COLORS - 1][2] = b;


      /* Re-render color picker to show the current color it contains: */

      tmp_btn_up = thumbnail(img_btn_up, color_button_w, color_button_h, 0);
      tmp_btn_down = thumbnail(img_btn_down, color_button_w, color_button_h, 0);
      img_color_btn_off = thumbnail(img_btn_off, color_button_w, color_button_h, 0);

      getpixel_tmp_btn_up = getpixels[tmp_btn_up->format->BytesPerPixel];
      getpixel_tmp_btn_down = getpixels[tmp_btn_down->format->BytesPerPixel];
      getpixel_img_paintwell = getpixels[img_paintwell->format->BytesPerPixel];

      rh = sRGB_to_linear_table[color_hexes[NUM_COLORS - 1][0]];
      gh = sRGB_to_linear_table[color_hexes[NUM_COLORS - 1][1]];
      bh = sRGB_to_linear_table[color_hexes[NUM_COLORS - 1][2]];
      SDL_BlitSurface(tmp_btn_down, NULL, img_color_btns[NUM_COLORS - 1], NULL);
      SDL_BlitSurface(tmp_btn_up, NULL, img_color_btns[NUM_COLORS - 1 + NUM_COLORS], NULL);

      SDL_LockSurface(img_color_btns[NUM_COLORS - 1]);
      SDL_LockSurface(img_color_btns[NUM_COLORS - 1 + NUM_COLORS]);

      for (y = 0; y < tmp_btn_up->h /* 48 */ ; y++)
        {
          for (x = 0; x < tmp_btn_up->w; x++)
            {
              double ru, gu, bu, rd, gd, bd, aa;
              Uint8 a;

              SDL_GetRGB(getpixel_tmp_btn_up(tmp_btn_up, x, y), tmp_btn_up->format, &r, &g, &b);

              ru = sRGB_to_linear_table[r];
              gu = sRGB_to_linear_table[g];
              bu = sRGB_to_linear_table[b];
              SDL_GetRGB(getpixel_tmp_btn_down(tmp_btn_down, x, y), tmp_btn_down->format, &r, &g, &b);

              rd = sRGB_to_linear_table[r];
              gd = sRGB_to_linear_table[g];
              bd = sRGB_to_linear_table[b];
              SDL_GetRGBA(getpixel_img_paintwell(img_paintwell, x, y), img_paintwell->format, &r, &g, &b, &a);

              aa = a / 255.0;

              putpixels[img_color_btns[NUM_COLORS - 1]->format->BytesPerPixel]
                (img_color_btns[NUM_COLORS - 1], x, y,
                 getpixels[img_color_picker_thumb->format->BytesPerPixel] (img_color_picker_thumb, x, y));
              putpixels[img_color_btns[NUM_COLORS - 1 + NUM_COLORS]->format->BytesPerPixel]
                (img_color_btns[NUM_COLORS - 1 + NUM_COLORS], x, y,
                 getpixels[img_color_picker_thumb->format->BytesPerPixel] (img_color_picker_thumb, x, y));

              if (a == 255)
                {
                  putpixels[img_color_btns[NUM_COLORS - 1]->format->BytesPerPixel]
                    (img_color_btns[NUM_COLORS - 1], x, y,
                     SDL_MapRGB(img_color_btns[i]->format,
                                linear_to_sRGB(rh * aa + ru * (1.0 - aa)),
                                linear_to_sRGB(gh * aa + gu * (1.0 - aa)), linear_to_sRGB(bh * aa + bu * (1.0 - aa))));

                  putpixels[img_color_btns[NUM_COLORS - 1 + NUM_COLORS]->format->BytesPerPixel]
                    (img_color_btns[NUM_COLORS - 1 + NUM_COLORS], x, y,
                     SDL_MapRGB(img_color_btns[i + NUM_COLORS]->format,
                                linear_to_sRGB(rh * aa + rd * (1.0 - aa)),
                                linear_to_sRGB(gh * aa + gd * (1.0 - aa)), linear_to_sRGB(bh * aa + bd * (1.0 - aa))));
                }
            }
        }

      SDL_UnlockSurface(img_color_btns[NUM_COLORS - 1]);
      SDL_UnlockSurface(img_color_btns[NUM_COLORS - 1 + NUM_COLORS]);
    }


  /* Remove the prompt: */

  update_canvas(0, 0, canvas->w, canvas->h);


  return (chose);
}


static void magic_putpixel(SDL_Surface * surface, int x, int y, Uint32 pixel)
{
  putpixels[surface->format->BytesPerPixel] (surface, x, y, pixel);
}

static Uint32 magic_getpixel(SDL_Surface * surface, int x, int y)
{
  return (getpixels[surface->format->BytesPerPixel] (surface, x, y));
}


static void magic_switchout(SDL_Surface * last)
{
  int was_clicking = 0;

  if (mouseaccessibility && emulate_button_pressed)
    {
      /* We were 'clicking' in mouse accessibility mode; stop clicking now */
      /* (EVEN if we weren't in magic tool) */
      emulate_button_pressed = 0;
      was_clicking = 1;
    }

  if (cur_tool == TOOL_MAGIC)
    {
      magic_funcs[magics[cur_magic].handle_idx].switchout(magic_api_struct,
                                                          magics[cur_magic].idx, magics[cur_magic].mode, canvas, last);
      update_canvas(0, 0, canvas->w, canvas->h);

      if (was_clicking && magics[cur_magic].mode == MODE_PAINT_WITH_PREVIEW)
        {
          /* Clean up preview! */
          do_undo();
          tool_avail[TOOL_REDO] = 0;    /* Don't let them 'redo' to get preview back */
          draw_toolbar();
          update_screen_rect(&r_tools);
        }
    }
}

static void magic_switchin(SDL_Surface * last)
{
  if (cur_tool == TOOL_MAGIC)
    {
      magic_funcs[magics[cur_magic].handle_idx].switchin(magic_api_struct,
                                                         magics[cur_magic].idx, magics[cur_magic].mode, canvas, last);

      /* In case the Magic tool's switchin() called update_progress_bar(),
         let's put the old Tux text back: */

      redraw_tux_text();

      update_canvas(0, 0, canvas->w, canvas->h);
    }
}

static int magic_modeint(int mode)
{
  if (mode == MODE_PAINT || mode == MODE_ONECLICK || mode == MODE_PAINT_WITH_PREVIEW)
    return 0;
  else if (mode == MODE_FULLSCREEN)
    return 1;
  else
    return 0;
}

static void add_label_node(int w, int h, Uint16 x, Uint16 y, SDL_Surface * label_node_surface)
{
  struct label_node *new_node = malloc(sizeof(struct label_node));
  struct label_node *aux_node;

  unsigned int i = 0;

  new_node->save_texttool_len = texttool_len;
  while (i < texttool_len)
    {
      new_node->save_texttool_str[i] = texttool_str[i];
      i = i + 1;
    }
  new_node->save_color.r = color_hexes[cur_color][0];
  new_node->save_color.g = color_hexes[cur_color][1];
  new_node->save_color.b = color_hexes[cur_color][2];
  new_node->save_width = w;
  new_node->save_height = h;
  new_node->save_x = x;
  new_node->save_y = y;
  new_node->save_cur_font = cur_font;
  new_node->save_text_state = text_state;
  new_node->save_text_size = text_size;
  new_node->save_undoid = 255;

  if (texttool_len > 0)
    {
      new_node->is_enabled = TRUE;
    }
  else
    {
      new_node->is_enabled = FALSE;
    }

  new_node->save_font_type = NULL;

  if (label_node_to_edit)
    {
      new_node->disables = label_node_to_edit;
    }
  else
    new_node->disables = NULL;

  if (label_node_surface != NULL)
    {
      new_node->label_node_surface = label_node_surface;
      new_node->label_node_surface->refcount++;
    }
  else
    new_node->label_node_surface = NULL;


  new_node->next_to_up_label_node = 0;

  new_node->next_to_down_label_node = current_label_node;
  if (current_label_node)
    {
      aux_node = current_label_node;
      aux_node->next_to_up_label_node = new_node;
    }

  current_label_node = new_node;

  if (start_label_node == NULL)
    start_label_node = current_label_node;

  highlighted_label_node = new_node;
  if (highlighted_label_node->is_enabled == FALSE)
    cycle_highlighted_label_node();
}


static struct label_node *search_label_list(struct label_node **ref_head, Uint16 x, Uint16 y, int hover)
{
  struct label_node *current_node;
  struct label_node *tmp_node = NULL;
  unsigned u;
  int done = FALSE;

  Uint8 r, g, b, a;
  int i, j, k;

  if (*ref_head == NULL)
    return (NULL);

  current_node = *ref_head;

  while (done != TRUE)
    {
      if (x >= current_node->save_x)
        {
          if (y >= current_node->save_y)
            {
              if (x <= (current_node->save_x) + (current_node->save_width))
                {
                  if (y <= (current_node->save_y) + (current_node->save_height))
                    {
                      if (current_node->is_enabled == TRUE)
                        {
                          if (hover == 1)
                            return (current_node);
                          tmp_node = current_node;
                          done = TRUE;
                        }
                    }
                }
            }
        }
      current_node = current_node->next_to_down_label_node;
      if (current_node == NULL)
        current_node = current_label_node;
      if (current_node == *ref_head)
        done = TRUE;
    }

  if (tmp_node != NULL)
    {
      select_texttool_len = tmp_node->save_texttool_len;

      u = 0;
      while (u < select_texttool_len)
        {
          select_texttool_str[u] = tmp_node->save_texttool_str[u];
          u = u + 1;
        }

      for (k = 0; k < NUM_COLORS; k++)
        {
          if ((color_hexes[k][0] == tmp_node->save_color.r) &&
              (color_hexes[k][1] == tmp_node->save_color.g) &&
              (color_hexes[k][2] == tmp_node->save_color.b) && (k < NUM_COLORS - 1))
            {
              select_color = k;
              cur_color = k;
              break;
            }

          if (k == NUM_COLORS - 1)
            {
              cur_color = NUM_COLORS - 1;
              select_color = NUM_COLORS - 1;
              color_hexes[select_color][0] = tmp_node->save_color.r;
              color_hexes[select_color][1] = tmp_node->save_color.g;
              color_hexes[select_color][2] = tmp_node->save_color.b;
              SDL_LockSurface(img_color_btns[NUM_COLORS - 1]);
              SDL_LockSurface(img_color_btns[NUM_COLORS - 1 + NUM_COLORS]);

              for (j = 0; j < 48 /* 48 */ ; j++)
                {
                  for (i = 0; i < 48; i++)
                    {
                      SDL_GetRGBA(getpixels[img_paintwell->format->BytesPerPixel] (img_paintwell, i, j),
                                  img_paintwell->format, &r, &g, &b, &a);
                      if (a == 255)
                        {
                          putpixels[img_color_btns[NUM_COLORS - 1]->format->BytesPerPixel]
                            (img_color_btns[NUM_COLORS - 1], i, j,
                             SDL_MapRGB(img_color_btns[NUM_COLORS - 1]->format,
                                        tmp_node->save_color.r, tmp_node->save_color.g, tmp_node->save_color.b));
                          putpixels[img_color_btns[NUM_COLORS - 1 + NUM_COLORS]->format->BytesPerPixel]
                            (img_color_btns[NUM_COLORS - 1 + NUM_COLORS], i, j,
                             SDL_MapRGB(img_color_btns[NUM_COLORS - 1 + NUM_COLORS]->format,
                                        tmp_node->save_color.r, tmp_node->save_color.g, tmp_node->save_color.b));
                        }
                    }
                }
              SDL_UnlockSurface(img_color_btns[NUM_COLORS - 1]);
              SDL_UnlockSurface(img_color_btns[NUM_COLORS - 1 + NUM_COLORS]);

              draw_colors(COLORSEL_CLOBBER);
              render_brush();   /* FIXME: render_brush should be called at the start of Brush and Line tools? */
            }
        }

      select_width = tmp_node->save_width;
      select_height = tmp_node->save_height;
      select_x = tmp_node->save_x;
      select_y = tmp_node->save_y;
      select_cur_font = tmp_node->save_cur_font;
      select_text_state = tmp_node->save_text_state;
      select_text_size = tmp_node->save_text_size;

      return tmp_node;
    }

  return NULL;
}

static void rec_undo_label(void)
{
  if (first_label_node_in_redo_stack != NULL)
    {
      delete_label_list(&first_label_node_in_redo_stack);
      first_label_node_in_redo_stack = NULL;
    }

  if (coming_from_undo_or_redo) // yet recorded, avoiding to write text_undo
    {
      coming_from_undo_or_redo = FALSE;
      return;
    }

  // FIXME:
  // It's all wrong to have a separate undo stack anyway. We need a way
  // for arbitrary code to supply callback functions and parameters when
  // creating an undo entry. One obvious function is a destructor for the
  // private data, for when it drops off the far end of the stack or gets
  // wiped out by an undo,draw combo. Others might be for when the level
  // stops being current or for when the level becomes current again.

  if (have_to_rec_label_node)
    {
      current_label_node->save_undoid = cur_undo;
      text_undo[cur_undo] = 1;
      have_to_rec_label_node = FALSE;
    }
  else
    {
      text_undo[cur_undo] = 0;

      /* Have we cycled around NUM_UNDO_BUFS? */
      if (current_label_node != NULL && current_label_node->save_undoid == (cur_undo + 1) % NUM_UNDO_BUFS)
        current_label_node->save_undoid = 255;
    }
}

static void do_undo_label_node()
{
  if (text_undo[(cur_undo + 1) % NUM_UNDO_BUFS] == 1)
    if (current_label_node != NULL)
      {
        if (current_label_node->save_undoid == (cur_undo + 1) % NUM_UNDO_BUFS)
          {
            if (current_label_node->disables != NULL)   /* If current node is an editing of an older one, reenable it. */
              current_label_node->disables->is_enabled = TRUE;

            first_label_node_in_redo_stack = current_label_node;
            current_label_node = current_label_node->next_to_down_label_node;

            if (current_label_node == NULL)
              start_label_node = current_label_node;

            highlighted_label_node = current_label_node;
            derender_node(&first_label_node_in_redo_stack);
            coming_from_undo_or_redo = TRUE;
          }
      }
  highlighted_label_node = current_label_node;
}

static void do_redo_label_node()
{
  if ((text_undo[cur_undo] == 1) && (first_label_node_in_redo_stack != NULL))
    {
      if (first_label_node_in_redo_stack->save_undoid == cur_undo)
        {
          current_label_node = first_label_node_in_redo_stack;
          first_label_node_in_redo_stack = current_label_node->next_to_up_label_node;

          if (start_label_node == NULL)
            start_label_node = current_label_node;

          highlighted_label_node = current_label_node;
          if (current_label_node->disables != NULL)     /* If this is a redo of an editing, redisable the old node. */
            {
              current_label_node->disables->is_enabled = FALSE;
              derender_node(&current_label_node->disables);
            }
          else
            simply_render_node(current_label_node);

          coming_from_undo_or_redo = TRUE;
        }
    }
}


static void simply_render_node(struct label_node *node)
{

  SDL_Surface *tmp_surf;
  SDL_Rect dest, src;
  wchar_t *str;
  wchar_t tmp_str[256];
  int j, w;
  unsigned i;

  if (node->label_node_surface == NULL)
    {
      /* Render the text: */

      SDL_Color color = node->save_color;

      text_state = node->save_text_state;
      text_size = node->save_text_size;

      i = 0;
      while (i < node->save_texttool_len)
        {
          tmp_str[i] = node->save_texttool_str[i];
          i = i + 1;
        }
      tmp_str[i] = L'\0';

      str = uppercase_w(tmp_str);

      text_state = node->save_text_state;
      text_size = node->save_text_size;

      for (j = 0; j < num_font_families; j++)
        {
          if (user_font_families[j] && user_font_families[j]->handle)
            {
              TuxPaint_Font_CloseFont(user_font_families[j]->handle);
              user_font_families[j]->handle = NULL;
            }
        }

      tmp_surf = render_text_w(getfonthandle(node->save_cur_font), str, color);
      if (tmp_surf != NULL)

        node->label_node_surface = tmp_surf;
    }

  if (node->label_node_surface != NULL)
    {
      w = node->label_node_surface->w;

      cursor_textwidth = w;
      /* Draw the text itself! */

      dest.x = node->save_x;
      dest.y = node->save_y;

      src.x = 0;
      src.y = 0;
      src.w = node->label_node_surface->w;
      src.h = node->label_node_surface->h;

      if (dest.x + src.w > WINDOW_WIDTH - 96 - 96)
        src.w = WINDOW_WIDTH - 96 - 96 - dest.x;
      if (dest.y + src.h > (48 * 7 + 40 + HEIGHTOFFSET))
        src.h = (48 * 7 + 40 + HEIGHTOFFSET) - dest.y;

      myblit(node->label_node_surface, &src, label, &dest);

      update_canvas(dest.x, dest.y, dest.x + node->label_node_surface->w, dest.y + node->label_node_surface->h);

      /* Setting the sizes correctly */
      node->save_width = node->label_node_surface->w;
      node->save_height = node->label_node_surface->h;
    }
}

static void render_all_nodes_starting_at(struct label_node **node)
{
  struct label_node *current_node;

  if (*node != NULL)
    {
      current_node = *node;
      while (current_node != first_label_node_in_redo_stack)
        {
          if (current_node->is_enabled == TRUE)
            {
              simply_render_node(current_node);
            }
          if (current_node->next_to_up_label_node == NULL)
            return;
          current_node = current_node->next_to_up_label_node;
        }
    }
}

/* FIXME: This should search for the top-down of the overlaping labels and only re-render from it */
static void derender_node(struct label_node **ref_head)
{
  SDL_Rect r_tmp_derender;

  r_tmp_derender.w = label->w;
  r_tmp_derender.h = label->h;
  r_tmp_derender.x = 0;
  r_tmp_derender.y = 0;

  SDL_FillRect(label, &r_tmp_derender, SDL_MapRGBA(label->format, 0, 0, 0, 0));

  render_all_nodes_starting_at(&start_label_node);
}

static void delete_label_list(struct label_node **ref_head)
{
  struct label_node *current = *ref_head;
  struct label_node *next;

  while (current != NULL)
    {
      fflush(stdout);

      next = current->next_to_up_label_node;
      if (current->label_node_surface)
        SDL_FreeSurface(current->label_node_surface);
      free(current);
      current = next;
    }

  *ref_head = NULL;
}

/* A custom bliter that allows to put two transparent layers toghether without having to deal with colorkeys or SDL_SRCALPHA
   I am always reinventing the wheel. Hope this one is not squared. Pere */
static void myblit(SDL_Surface * src_surf, SDL_Rect * src_rect, SDL_Surface * dest_surf, SDL_Rect * dest_rect)
{
  int x, y;
  Uint8 src_r, src_g, src_b, src_a;
  Uint8 dest_r, dest_g, dest_b, dest_a;

  for (x = src_rect->x; x < src_rect->w + src_rect->x; x++)
    for (y = src_rect->y; y < src_rect->h + src_rect->y; y++)
      {
        SDL_GetRGBA(getpixels[src_surf->format->BytesPerPixel] (src_surf, x - src_rect->x, y - src_rect->y),
                    src_surf->format, &src_r, &src_g, &src_b, &src_a);
        if (src_a != SDL_ALPHA_TRANSPARENT)
          {
            if (src_a == SDL_ALPHA_OPAQUE)
              putpixels[dest_surf->format->BytesPerPixel] (dest_surf, x + dest_rect->x, y + dest_rect->y,
                                                           SDL_MapRGBA(dest_surf->format, src_r, src_g, src_b, src_a));
            else
              {
                SDL_GetRGBA(getpixels[dest_surf->format->BytesPerPixel] (dest_surf, x + dest_rect->x, y + dest_rect->y),
                            src_surf->format, &dest_r, &dest_g, &dest_b, &dest_a);
                if (dest_a == SDL_ALPHA_TRANSPARENT)
                  putpixels[dest_surf->format->BytesPerPixel] (dest_surf, x + dest_rect->x, y + dest_rect->y,
                                                               SDL_MapRGBA(dest_surf->format, src_r, src_g, src_b,
                                                                           src_a));
                else
                  {
                    dest_r = src_r * src_a / 255 + dest_r * dest_a * (255 - src_a) / 255 / 255;
                    dest_g = src_g * src_a / 255 + dest_g * dest_a * (255 - src_a) / 255 / 255;
                    dest_b = src_b * src_a / 255 + dest_b * dest_a * (255 - src_a) / 255 / 255;
                    dest_a = src_a + dest_a * (255 - src_a) / 255;
                    putpixels[dest_surf->format->BytesPerPixel] (dest_surf, x + dest_rect->x, y + dest_rect->y,
                                                                 SDL_MapRGBA(dest_surf->format, dest_r, dest_g, dest_b,
                                                                             dest_a));
                  }
              }
          }
      }
}

static void load_info_about_label_surface(FILE * lfi)
{
  struct label_node *new_node;
  int list_ctr;
  int tmp_scale_w;
  int tmp_scale_h;
  SDL_Surface *label_node_surface, *label_node_surface_aux;
  float new_text_size;

  int k;
  unsigned l;
  unsigned tmp_pos;
  wchar_t tmp_char;
  int old_width;
  int old_height;
  int new_width;
  int new_height;
  float new_ratio;
  float old_ratio;
  float new_to_old_ratio;
  int old_pos;
  int new_pos;
  int x, y;
  int tmp_fscanf_return;
  char *tmp_fgets_return;
  Uint8 a;

  /* Clear label surface */

  SDL_FillRect(label, NULL, SDL_MapRGBA(label->format, 0, 0, 0, 0));

  /* Clear all info related to label surface */

  delete_label_list(&start_label_node);
  start_label_node = current_label_node = first_label_node_in_redo_stack = highlighted_label_node = label_node_to_edit =
    NULL;
  have_to_rec_label_node = FALSE;


  if (lfi == NULL)
    return;
  tmp_fscanf_return = fscanf(lfi, "%d\n", &list_ctr);
  tmp_fscanf_return = fscanf(lfi, "%d\n", &tmp_scale_w);
  tmp_fscanf_return = fscanf(lfi, "%d\n\n", &tmp_scale_h);
  (void)tmp_fscanf_return;

  old_width = tmp_scale_w;
  old_height = tmp_scale_h;
  new_width = r_canvas.w;
  new_height = r_canvas.h;
  new_ratio = (float)new_width / new_height;
  old_ratio = (float)old_width / old_height;
  if (new_ratio < old_ratio)
    new_to_old_ratio = (float)new_width / old_width;
  else
    new_to_old_ratio = (float)new_height / old_height;

  for (k = 0; k < list_ctr; k++)
    {
      new_node = malloc(sizeof(struct label_node));

      tmp_fscanf_return = fscanf(lfi, "%u\n", &new_node->save_texttool_len);
#ifdef WIN32
      char *tmpstr;
      wchar_t *wtmpstr;

      tmpstr = malloc(1024);
      wtmpstr = malloc(1024);
      fgets(tmpstr, 1024, lfi);
      mtw(wtmpstr, tmpstr);
      for (l = 0; l < new_node->save_texttool_len; l++)
        {
          new_node->save_texttool_str[l] = wtmpstr[l];
        }

#elif defined(__ANDROID__)
      for (l = 0; l < new_node->save_texttool_len; l++)
        {
          fscanf(lfi, "%d ", &tmp_char);
          new_node->save_texttool_str[l] = tmp_char;
        }
      fscanf(lfi, "\n");
#else
      for (l = 0; l < new_node->save_texttool_len; l++)
        {
          tmp_fscanf_return = fscanf(lfi, "%lc", &tmp_char);
          new_node->save_texttool_str[l] = tmp_char;
        }
      tmp_fscanf_return = fscanf(lfi, "\n");
#endif
      tmp_fscanf_return = fscanf(lfi, "%u\n", &l);
      new_node->save_color.r = (Uint8) l;
      tmp_fscanf_return = fscanf(lfi, "%u\n", &l);
      new_node->save_color.g = (Uint8) l;
      tmp_fscanf_return = fscanf(lfi, "%u\n", &l);
      new_node->save_color.b = (Uint8) l;
      tmp_fscanf_return = fscanf(lfi, "%d\n", &new_node->save_width);
      tmp_fscanf_return = fscanf(lfi, "%d\n", &new_node->save_height);
      tmp_fscanf_return = fscanf(lfi, "%d\n", &tmp_pos);
      old_pos = (int)tmp_pos;

      if (new_ratio < old_ratio)
        {
          new_pos = (old_pos * new_to_old_ratio);
          tmp_pos = new_pos;
          new_node->save_x = tmp_pos;
          tmp_fscanf_return = fscanf(lfi, "%d\n", &tmp_pos);
          old_pos = (int)tmp_pos;
          new_pos = old_pos * new_to_old_ratio + (new_height - old_height * new_to_old_ratio) / 2;
          tmp_pos = new_pos;
          new_node->save_y = tmp_pos;
        }
      else
        {
          new_pos = (old_pos * new_to_old_ratio) + (new_width - old_width * new_to_old_ratio) / 2;
          tmp_pos = new_pos;
          new_node->save_x = tmp_pos;
          tmp_fscanf_return = fscanf(lfi, "%d\n", &tmp_pos);
          old_pos = (int)tmp_pos;
          new_pos = (old_pos * new_to_old_ratio);
          tmp_pos = new_pos;
          new_node->save_y = tmp_pos;
        }

      printf("Original label size %dx%d\n", new_node->save_width, new_node->save_height);

      tmp_fscanf_return = fscanf(lfi, "%d\n", &new_node->save_cur_font);
      new_node->save_cur_font = 0;

      new_node->save_font_type = malloc(64);
      tmp_fgets_return = fgets(new_node->save_font_type, 64, lfi);
      (void)tmp_fgets_return;

      tmp_fscanf_return = fscanf(lfi, "%d\n", &new_node->save_text_state);
      tmp_fscanf_return = fscanf(lfi, "%u\n", &new_node->save_text_size);

      label_node_surface = SDL_CreateRGBSurface(screen->flags,
                                                new_node->save_width,
                                                new_node->save_height,
                                                screen->format->BitsPerPixel,
                                                screen->format->Rmask,
                                                screen->format->Gmask, screen->format->Bmask, TPAINT_AMASK);

      SDL_LockSurface(label_node_surface);
      for (x = 0; x < new_node->save_width; x++)
        for (y = 0; y < new_node->save_height; y++)
          {
            a = fgetc(lfi);
            putpixels[label_node_surface->format->BytesPerPixel] (label_node_surface, x, y,
                                                                  SDL_MapRGBA(label_node_surface->format,
                                                                              new_node->save_color.r,
                                                                              new_node->save_color.g,
                                                                              new_node->save_color.b, a));
          }
      SDL_UnlockSurface(label_node_surface);

      new_text_size = (float)new_node->save_text_size * new_to_old_ratio;
      label_node_surface_aux =
        zoom(label_node_surface, label_node_surface->w * new_to_old_ratio, label_node_surface->h * new_to_old_ratio);
      SDL_FreeSurface(label_node_surface);
      new_node->label_node_surface = label_node_surface_aux;
      new_node->label_node_surface->refcount++;
      SDL_FreeSurface(label_node_surface_aux);

      if ((unsigned)new_text_size > MAX_TEXT_SIZE)      /* Here we reach the limits when scaling the font size */
        new_node->save_text_size = MAX_TEXT_SIZE;
      else if ((unsigned)new_text_size > MIN_TEXT_SIZE)
        new_node->save_text_size = floor(new_text_size + 0.5);
      else
        new_node->save_text_size = MIN_TEXT_SIZE;


      new_node->save_undoid = 255;      /* A value that cur_undo will likely never reach */
      new_node->is_enabled = TRUE;
      new_node->disables = NULL;
      new_node->next_to_down_label_node = NULL;
      new_node->next_to_up_label_node = NULL;
      tmp_fscanf_return = fscanf(lfi, "\n");

      if (current_label_node == NULL)
        {
          current_label_node = new_node;
          start_label_node = current_label_node;
        }
      else
        {
          new_node->next_to_down_label_node = current_label_node;
          current_label_node->next_to_up_label_node = new_node;
          current_label_node = new_node;
        }

      highlighted_label_node = current_label_node;
      simply_render_node(current_label_node);

    }
  first_label_node_in_redo_stack = NULL;
  fclose(lfi);

  if (font_thread_done)
    set_label_fonts();
}

static void set_label_fonts()
{
  struct label_node *node;
  int i;
  char *ttffont;

  node = current_label_node;
  while (node != NULL)

    {
      for (i = 0; i < num_font_families; i++)
        {
          Uint32 c;

          /* FIXME: 2009/09/13 TTF_FontFaceFamilyName() appends random "\n" at the end
             of the returned string.  Should investigate why, and when corrected,
             remove the code that deals whith the ending "\n"s in ttffont */
          ttffont = TTF_FontFaceFamilyName(getfonthandle(i)->ttf_font);
          for (c = 0; c < strlen(ttffont); c++)
            if (ttffont[c] == '\n')
              ttffont[c] = '\0';
          for (c = 0; c < strlen(node->save_font_type); c++)
            if (node->save_font_type[c] == '\n')
              node->save_font_type[c] = '\0';

#ifdef DEBUG
          printf("ttffont A%sA\n", ttffont);
          printf("font_type B%sB\n", node->save_font_type);
#endif

          if (strcmp(node->save_font_type, ttffont) == 0)

            {
#ifdef DEBUG
              printf("Font matched %s !!!\n", ttffont);
#endif
              node->save_cur_font = i;
              break;
            }
          else if (strstr(ttffont, node->save_font_type) || strstr(node->save_font_type, ttffont))
            {
#ifdef DEBUG
              printf("setting %s as replacement", TTF_FontFaceFamilyName(getfonthandle(i)->ttf_font));
#endif
              node->save_cur_font = i;
            }
        }

      if (node->save_cur_font > num_font_families)      /* This should never happens, setting default font. */
        node->save_cur_font = 0;

      free(node->save_font_type);       /* Not needed anymore */
      node->save_font_type = NULL;
      node = node->next_to_down_label_node;

    }
}


static void tmp_apply_uncommited_text()
{
  have_to_rec_label_node_back = have_to_rec_label_node;

  if (texttool_len > 0)
    {
      if (cur_tool == TOOL_TEXT ||
          (old_tool == TOOL_TEXT &&
           (cur_tool == TOOL_PRINT ||
            cur_tool == TOOL_SAVE || cur_tool == TOOL_OPEN || cur_tool == TOOL_NEW || cur_tool == TOOL_QUIT)))
        {
          canvas_back = SDL_CreateRGBSurface(canvas->flags,
                                             canvas->w,
                                             canvas->h,
                                             canvas->format->BitsPerPixel,
                                             canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, 0);
          SDL_BlitSurface(canvas, NULL, canvas_back, NULL);
          do_render_cur_text(1);
        }

      else if (cur_tool == TOOL_LABEL ||
               (old_tool == TOOL_LABEL &&
                (cur_tool == TOOL_PRINT ||
                 cur_tool == TOOL_SAVE || cur_tool == TOOL_OPEN || cur_tool == TOOL_NEW || cur_tool == TOOL_QUIT)))
        {
          do_render_cur_text(1);
          current_label_node->save_undoid = 253;
        }
    }
  else if ((cur_tool == TOOL_LABEL && label_node_to_edit) ||
           ((old_tool == TOOL_LABEL && label_node_to_edit) &&
            (cur_tool == TOOL_PRINT ||
             cur_tool == TOOL_SAVE || cur_tool == TOOL_OPEN || cur_tool == TOOL_NEW || cur_tool == TOOL_QUIT)))
    {
      add_label_node(0, 0, 0, 0, NULL);
      current_label_node->is_enabled = FALSE;
      current_label_node->save_undoid = 253;

      derender_node(&label_node_to_edit);
    }
}

static void undo_tmp_applied_text()
{
  struct label_node *aux_label_node;

  if (texttool_len > 0)
    {
      if (cur_tool == TOOL_TEXT ||
          (cur_tool == TOOL_PRINT && old_tool == TOOL_TEXT) ||
          (cur_tool == TOOL_SAVE && old_tool == TOOL_TEXT) ||
          (cur_tool == TOOL_OPEN && old_tool == TOOL_TEXT) ||
          (cur_tool == TOOL_NEW && old_tool == TOOL_TEXT) || (cur_tool == TOOL_QUIT && old_tool == TOOL_TEXT))
        {
          SDL_BlitSurface(canvas_back, NULL, canvas, NULL);
          SDL_FreeSurface(canvas_back);
          do_render_cur_text(0);
        }
    }
  if (current_label_node != NULL && current_label_node->save_undoid == 253)
    {
      aux_label_node = current_label_node;
      current_label_node = current_label_node->next_to_down_label_node;

      if (current_label_node == NULL)
        start_label_node = NULL;
      else
        current_label_node->next_to_up_label_node = first_label_node_in_redo_stack;

      derender_node(&aux_label_node);
      delete_label_list(&aux_label_node);
      have_to_rec_label_node = have_to_rec_label_node_back;
      do_render_cur_text(0);
    }
}


/* Painting on the screen surface to avoid unnecessary complexity */
static void highlight_label_nodes()
{
  int j;
  SDL_Rect rect, rect1;
  struct label_node *aux_node;

  if (highlighted_label_node != NULL)
    {
      aux_node = highlighted_label_node->next_to_up_label_node;
      if (aux_node == first_label_node_in_redo_stack)
        aux_node = start_label_node;


      while (aux_node != highlighted_label_node)
        {
          if (aux_node->is_enabled)
            {
              rect.x = aux_node->save_x + button_w * 2;
              rect.y = aux_node->save_y;
              rect.w = aux_node->save_width;
              rect.h = aux_node->save_height;

              SDL_FillRect(screen, &rect, SDL_MapRGBA(screen->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT));

              for (j = 2; j < aux_node->save_height / 4; j++)
                {
                  rect1.x = rect.x + j;
                  rect1.y = rect.y + j;
                  rect1.w = rect.w - 2 * j;
                  if (rect1.w < 2)
                    break;
                  rect1.h = rect.h - 2 * j;
                  SDL_FillRect(screen,
                               &rect1,
                               SDL_MapRGBA(screen->format,
                                           4 * j * 200 / aux_node->save_height,
                                           4 * j * 200 / aux_node->save_height,
                                           4 * j * 200 / aux_node->save_height, SDL_ALPHA_OPAQUE));

                  SDL_BlitSurface(aux_node->label_node_surface, NULL, screen, &rect);
                }

            }

          aux_node = aux_node->next_to_up_label_node;
          if (aux_node == first_label_node_in_redo_stack)
            aux_node = start_label_node;
        }

      aux_node = highlighted_label_node;
      rect.x = aux_node->save_x + button_w * 2;
      rect.y = aux_node->save_y;
      rect.w = aux_node->save_width;
      rect.h = aux_node->save_height;
      SDL_FillRect(screen, &rect, SDL_MapRGBA(screen->format, 255, 0, 0, SDL_ALPHA_OPAQUE));

      for (j = 2; j < aux_node->save_height / 4; j++)
        {
          rect1.x = rect.x + j;
          rect1.y = rect.y + j;
          rect1.w = rect.w - 2 * j;
          if (rect1.w < 2)
            break;
          rect1.h = rect.h - 2 * j;
          SDL_FillRect(screen,
                       &rect1,
                       SDL_MapRGBA(screen->format, 255, 4 * j * 225 / aux_node->save_height, 0, SDL_ALPHA_OPAQUE));

          SDL_BlitSurface(aux_node->label_node_surface, NULL, screen, &rect);
        }

      SDL_Flip(screen);
    }
}

static void cycle_highlighted_label_node()
{
  struct label_node *aux_node;

  if (highlighted_label_node)
    {
      aux_node = highlighted_label_node->next_to_down_label_node;
      if (aux_node == NULL)
        aux_node = current_label_node;
      if (aux_node->is_enabled)
        highlighted_label_node = aux_node;
      else
        while (aux_node->is_enabled == FALSE && aux_node != highlighted_label_node)
          {
            aux_node = aux_node->next_to_down_label_node;
            if (aux_node == NULL)
              aux_node = current_label_node;
            if (aux_node->is_enabled)
              highlighted_label_node = aux_node;
          }
    }

}

static int are_labels()
{
  struct label_node *aux_node;

  if (current_label_node)
    {
      aux_node = current_label_node;
      while (aux_node)
        {
          if (aux_node->is_enabled)
            return (TRUE);
          aux_node = aux_node->next_to_down_label_node;
        }
    }
  return (FALSE);
}

int chunk_is_valid(const char *chunk_name, png_unknown_chunk unknown)
{
  unsigned int count, fields;
  int new_field;
  char *control, *softwr;
  int unc_size, comp;

  if (chunk_name[0] == unknown.name[0] &&
      chunk_name[1] == unknown.name[1] &&
      chunk_name[2] == unknown.name[2] &&
      chunk_name[3] == unknown.name[3] &&
      50 < unknown.size &&
      'T' == unknown.data[0] &&
      'u' == unknown.data[1] &&
      'x' == unknown.data[2] &&
      'p' == unknown.data[3] &&
      'a' == unknown.data[4] &&
      'i' == unknown.data[5] && 'n' == unknown.data[6] && 't' == unknown.data[7] && '\n' == unknown.data[8])
    {
      /* Passed the first test, now checking if there are  at least
         4 fields in the first 50 bytes of the chunk data */
      count = 9;
      fields = 1;
      new_field = 1;
      while (count < 50)
        {
          if (unknown.data[count] == '\n')
            {
              if (new_field == 1)
                return (FALSE); /* Avoid empty fields */
              fields++;
              if (fields == 4)
                {               /* Last check, see if the sizes match */
                  control = malloc(50);
                  softwr = malloc(50);
                  sscanf((char *)unknown.data, "%s\n%s\n%d\n%d\n", control, softwr, &unc_size, &comp);
                  free(control);
                  free(softwr);
                  if (count + comp + 1 == unknown.size)
                    return (TRUE);
                  else
                    return (FALSE);
                }
              new_field = 1;
            }
          else
            {
              /* Check if there are decimal values here */
              if ((fields < 4 && fields > 1) &&
                  !((unknown.data[count] == '0') ||
                    (unknown.data[count] == '1') ||
                    (unknown.data[count] == '2') ||
                    (unknown.data[count] == '3') ||
                    (unknown.data[count] == '4') ||
                    (unknown.data[count] == '5') ||
                    (unknown.data[count] == '6') ||
                    (unknown.data[count] == '7') || (unknown.data[count] == '8') || (unknown.data[count] == '9')))
                return (FALSE);

              new_field = 0;
            }
          count++;
        }
    }

  return (FALSE);
}

Bytef *get_chunk_data(FILE * fp, char *fname, png_structp png_ptr,
                      png_infop info_ptr, const char *chunk_name, png_unknown_chunk unknown, int *unc_size)
{
  unsigned int i;

  int f, count, comp, unc_err;
  char *control, *softwr;
  Bytef *comp_buff, *unc_buff;

  z_streamp zstp;

  control = malloc(50);
  softwr = malloc(50);
  sscanf((char *)unknown.data, "%s\n%s\n%d\n%d\n", control, softwr, unc_size, &comp);
  free(control);
  free(softwr);
  comp_buff = malloc(comp * sizeof(Bytef));

  if (comp_buff == NULL)
    {
      fclose(fp);

      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);

      fprintf(stderr,
              "\nError: Couldn't recover the embedded data in %s\n\nUnable to allocate memory for the compressed buffer for %s\n\n",
              fname, chunk_name);
      draw_tux_text(TUX_OOPS, strerror(errno), 0);
      return (NULL);
    }
  f = 0;
  count = 0;

  for (i = 0; i < unknown.size; i++)
    {
      if (f > 3)
        {
          comp_buff[i - count] = unknown.data[i];
          //            printf("%d, %d, %d    ",i-count, comp_buff[i - count], unknown.data[i]);
        }

      if (unknown.data[i] == '\n' && f < 4)
        {
          f++;
          count = i + 1;
        }
    }

  unc_buff = malloc(*unc_size * sizeof(Bytef));

  if (unc_buff == NULL)
    {
      fclose(fp);

      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);

      fprintf(stderr,
              "\nError: Couldn't recover the embedded data in %s\n\nUnable to allocate memory for the compressed buffer for %s\n\n",
              fname, chunk_name);
      draw_tux_text(TUX_OOPS, strerror(errno), 0);
      return (NULL);
    }

  /* Seems that uncompress() has problems in 64bits systems, so using inflate() Pere 2012/03/28 */
  /*  unc_err = uncompress(unc_buff, (uLongf *) unc_size, comp_buff, comp); */
  zstp = malloc(sizeof(z_stream));
  zstp->next_in = comp_buff;
  zstp->avail_in = comp;
  zstp->total_in = comp;

  zstp->next_out = unc_buff;
  zstp->avail_out = *unc_size;
  zstp->total_out = 0;

  zstp->zalloc = Z_NULL;
  zstp->zfree = Z_NULL;
  zstp->opaque = Z_NULL;

  inflateInit(zstp);
  unc_err = inflate(zstp, Z_FINISH);
  inflateEnd(zstp);

  if (unc_err != Z_STREAM_END)
    {
      printf("\n error %d, unc %d, comp %d\n", unc_err, *unc_size, comp);
      fclose(fp);
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
      free(comp_buff);
      free(unc_buff);

      printf("Can't recover the embedded data in %s, error in uncompressing data from %s\n\n", fname, chunk_name);
      draw_tux_text(TUX_OOPS, strerror(errno), 0);
      return (NULL);
    }

  free(comp_buff);
  return (unc_buff);

}

void load_embedded_data(char *fname, SDL_Surface * org_surf)
{
  FILE *fi, *fp;
  char *control;
  char *CHAR_PTR_TMP;
  Bytef *unc_buff;

  int unc_size;
  int u;
  int have_background, have_foreground, have_label_delta, have_label_data;
  int ldelta, ldata, fgnd, bgnd;
  int num_unknowns = 0;
  SDL_Surface *aux_surf;

  png_structp png_ptr;
  png_infop info_ptr;
  png_unknown_chunkp unknowns;

  png_uint_32 ww, hh;
  png_uint_32 i, j;

  printf("Loading embedded data...\n");
  printf("%s\n", fname);

  fp = fopen(fname, "rb");
  if (!fp)
    {
      SDL_FreeSurface(org_surf);
      return;
    }

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL)
    {
      fclose(fp);
      png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);

      fprintf(stderr, "\nError: Couldn't open the image!\n%s\n\n", fname);
      draw_tux_text(TUX_OOPS, strerror(errno), 0);
      SDL_FreeSurface(org_surf);
      return;
    }
  else
    {
      printf("%s\n", fname);

      info_ptr = png_create_info_struct(png_ptr);
      if (info_ptr == NULL)
        {
          fclose(fp);
          png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);

          fprintf(stderr, "\nError: Couldn't open the image!\n%s\n\n", fname);
          draw_tux_text(TUX_OOPS, strerror(errno), 0);
          SDL_FreeSurface(org_surf);
          return;
        }

      png_init_io(png_ptr, fp);

      png_set_keep_unknown_chunks(png_ptr, 3, NULL, 0);

      png_read_info(png_ptr, info_ptr);

      ww = png_get_image_width(png_ptr, info_ptr);
      hh = png_get_image_height(png_ptr, info_ptr);

      num_unknowns = (int)png_get_unknown_chunks(png_ptr, info_ptr, &unknowns);

      printf("num_unknowns %i\n", num_unknowns);
      if (num_unknowns)
        {
          have_label_delta = have_label_data = have_background = have_foreground = FALSE;
          ldelta = ldata = fgnd = bgnd = FALSE;

          /* Need to get things in order, as we can't enforce any order in custom chunks,
             we need to go around them 3 times */

          /* First we search for the things that usually were in the .dat file, so if a starter or a
             template is found and if it is not modified, we can load it clean (i.e. not rebluring a
             blured when scaled one) */
          for (u = 0; u < num_unknowns; u++)
            {
              printf("%s, %d\n", unknowns[u].name, (int)unknowns[u].size);

              if (chunk_is_valid("tpDT", unknowns[u]))
                {
                  printf("Valid tpDT\n");
                  fi = fmemopen(unknowns[u].data, unknowns[u].size, "r");
                  if (fi == NULL)
                    {
                      fclose(fp);
                      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);

                      fprintf(stderr, "\nError: Couldn't load the data embedded in %s\n\n", fname);
                      draw_tux_text(TUX_OOPS, strerror(errno), 0);
                      SDL_FreeSurface(org_surf);
                      return;   /* Refusing to go further with the other chunks */
                    }

                  /* Put fi position at the right place after the chunk headers */
                  control = malloc(50);
                  CHAR_PTR_TMP = fgets(control, 49, fi);
                  CHAR_PTR_TMP = fgets(control, 49, fi);
                  CHAR_PTR_TMP = fgets(control, 49, fi);
                  CHAR_PTR_TMP = fgets(control, 49, fi);
                  (void)CHAR_PTR_TMP;
                  free(control);

                  /* fi will be closed in load_starter_id() */
                  load_starter_id(NULL, fi);
                  if (!starter_modified)
                    {
                      /* Code adapted from load_current() */
                      if (starter_id[0] != '\0')
                        {
                          load_starter(starter_id);

                          if (starter_mirrored && img_starter)
                            mirror_starter();

                          if (starter_flipped && img_starter)
                            flip_starter();
                        }
                      else if (template_id[0] != '\0')
                        {
                          load_template(template_id);
                        }
                    }
                }
              /* Also check what we have there */
              if (chunk_is_valid("tpBK", unknowns[u]))
                have_background = TRUE;
              if (chunk_is_valid("tpFG", unknowns[u]))
                have_foreground = TRUE;
              if (chunk_is_valid("tpLD", unknowns[u]))
                have_label_delta = TRUE;
              if (chunk_is_valid("tpLL", unknowns[u]))
                have_label_data = TRUE;
            }

          /* Recover the labels and apply the diff from label to canvas. */
          if (!disable_label && have_label_delta && have_label_data)
            {
              for (u = 0; u < num_unknowns; u++)
                {
                  if (chunk_is_valid("tpLD", unknowns[u]))
                    {
                      printf("Valid tpLD\n");

                      unc_buff = get_chunk_data(fp, fname, png_ptr, info_ptr, "tpLD", unknowns[u], &unc_size);
                      if (unc_buff == NULL)
                        {
                          if (are_labels())
                            {
                              delete_label_list(&start_label_node);
                              start_label_node = current_label_node = NULL;
                            }

                          SDL_FreeSurface(org_surf);
                          return;
                        }
                      else
                        {
                          SDL_LockSurface(org_surf);
                          for (j = 0; j < hh; j++)
                            for (i = 0; i < ww; i++)
                              {
                                if ((Uint8) unc_buff[4 * j * ww + 4 * i + 3] == SDL_ALPHA_OPAQUE)
                                  putpixels[org_surf->format->BytesPerPixel] (org_surf, i, j,
                                                                              SDL_MapRGB(org_surf->format,
                                                                                         unc_buff[4 * (j * ww + i)],
                                                                                         unc_buff[4 * (j * ww + i) + 1],
                                                                                         unc_buff[4 * (j * ww + i) +
                                                                                                  2]));
                              }
                        }

                      SDL_UnlockSurface(org_surf);

                      free(unc_buff);
                      ldelta = TRUE;
                    }

                  /* Label Data */
                  if (!disable_label && chunk_is_valid("tpLL", unknowns[u]))
                    {
                      printf("Valid tpLL\n");

                      unc_buff = get_chunk_data(fp, fname, png_ptr, info_ptr, "tpLL", unknowns[u], &unc_size);
                      if (unc_buff == NULL)
                        {
                          SDL_FreeSurface(org_surf);
                          return;
                        }
                      else
                        {
                          fi = fmemopen(unc_buff, unc_size, "rb");
                          if (fi == NULL)
                            {
                              printf("Can't recover the label data embedded in %s, error in create file stream\n\n",
                                     fname);
                              fclose(fp);
                              png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
                              free(unc_buff);
                              SDL_FreeSurface(org_surf);

                              draw_tux_text(TUX_OOPS, strerror(errno), 0);
                              return;
                            }
                          else
                            load_info_about_label_surface(fi);
                        }

                      free(unc_buff);
                      ldata = TRUE;
                      printf("Out of label data\n");
                    }
                }
            }
          /* Apply the original canvas */
          if (ldelta && ldata)
            autoscale_copy_smear_free(org_surf, canvas, SDL_BlitSurface);
          else
            SDL_FreeSurface(org_surf);

          /* Third run, back and foreground */
          if (have_background || have_foreground)
            {
              for (u = 0; u < num_unknowns; u++)
                {
                  if ((starter_modified || !img_starter_bkgd) && chunk_is_valid("tpBG", unknowns[u]))
                    {
                      unc_buff = get_chunk_data(fp, fname, png_ptr, info_ptr, "tpBG", unknowns[u], &unc_size);
                      if (unc_buff == NULL)
                        return;
                      aux_surf =
                        SDL_CreateRGBSurface(0, ww, hh, canvas->format->BitsPerPixel,
                                             canvas->format->Rmask, canvas->format->Gmask, canvas->format->Gmask, 0);
                      if (aux_surf == NULL)
                        {
                          printf("Can't recover the background data embedded in %s, error in create aux image\n\n",
                                 fname);
                          fclose(fp);
                          png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
                          free(unc_buff);

                          draw_tux_text(TUX_OOPS, strerror(errno), 0);

                          free(unc_buff);
                          return;
                        }
                      SDL_LockSurface(aux_surf);

                      printf("Bkgd!!!\n");
                      for (j = 0; j < hh; j++)
                        for (i = 0; i < ww; i++)
                          putpixels[aux_surf->format->BytesPerPixel] (aux_surf, i, j,
                                                                      SDL_MapRGB
                                                                      (aux_surf->format,
                                                                       unc_buff[3 * j * ww + 3 * i],
                                                                       unc_buff[3 * j * ww + 3 * i + 1],
                                                                       unc_buff[3 * j * ww + 3 * i + 2]));
                      SDL_UnlockSurface(aux_surf);

                      if (img_starter_bkgd)
                        SDL_FreeSurface(img_starter_bkgd);

                      if (aux_surf->w != canvas->w || aux_surf->h != canvas->h)
                        {
                          img_starter_bkgd = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                                                  canvas->w,
                                                                  canvas->h,
                                                                  canvas->format->BitsPerPixel,
                                                                  canvas->format->Rmask,
                                                                  canvas->format->Gmask, canvas->format->Bmask, 0);

                          autoscale_copy_smear_free(aux_surf, img_starter_bkgd, SDL_BlitSurface);
                        }
                      free(unc_buff);
                    }

                  if ((starter_modified || !img_starter) && chunk_is_valid("tpFG", unknowns[u]))
                    {
                      printf("Frgd!!!\n");

                      unc_buff = get_chunk_data(fp, fname, png_ptr, info_ptr, "tpFG", unknowns[u], &unc_size);
                      if (unc_buff == NULL)
                        return;

                      aux_surf = SDL_CreateRGBSurface(canvas->flags, ww, hh,
                                                      canvas->format->BitsPerPixel,
                                                      canvas->format->Rmask,
                                                      canvas->format->Gmask, canvas->format->Gmask, TPAINT_AMASK);
                      if (aux_surf == NULL)
                        {
                          printf("Can't recover the foreground data embedded in %s, error in create aux image\n\n",
                                 fname);
                          fclose(fp);
                          png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
                          free(unc_buff);

                          draw_tux_text(TUX_OOPS, strerror(errno), 0);

                          free(unc_buff);
                          return;
                        }

                      SDL_LockSurface(aux_surf);
                      for (j = 0; j < hh; j++)
                        for (i = 0; i < ww; i++)
                          {
                            putpixels[aux_surf->format->BytesPerPixel] (aux_surf, i, j,
                                                                        SDL_MapRGBA
                                                                        (aux_surf->format,
                                                                         unc_buff[4 * j * ww + 4 * i],
                                                                         unc_buff[4 * j * ww + 4 * i + 1],
                                                                         unc_buff[4 * j * ww + 4 * i + 2],
                                                                         unc_buff[4 * j * ww + 4 * i + 3]));
                          }
                      SDL_UnlockSurface(aux_surf);

                      if (img_starter)
                        SDL_FreeSurface(img_starter);


                      /* Code adapted from load_starter */
                      img_starter = SDL_CreateRGBSurface(canvas->flags,
                                                         canvas->w, canvas->h,
                                                         canvas->format->BitsPerPixel,
                                                         canvas->format->Rmask,
                                                         canvas->format->Gmask, canvas->format->Bmask, TPAINT_AMASK);

                      /* 3rd arg ignored for RGBA surfaces */
                      //      SDL_SetAlpha(aux_surf, SDL_RLEACCEL, SDL_ALPHA_OPAQUE);
                      SDL_SetSurfaceBlendMode(aux_surf, SDL_BLENDMODE_NONE);
                      autoscale_copy_smear_free(aux_surf, img_starter, NondefectiveBlit);
                      //      SDL_SetAlpha(img_starter, SDL_ALPHA_OPAQUE);
                      SDL_SetSurfaceBlendMode(img_starter, SDL_BLENDMODE_NONE);

                      free(unc_buff);
                    }
                }
            }
        }
    }

  png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
  fclose(fp);
}


/* ================================================================================== */

#if !defined(WIN32) && !defined(__BEOS__) && !defined(__HAIKU__) && !defined(__ANDROID__)
static void show_available_papersizes(int exitcode)
{
  FILE *fi = exitcode ? stderr : stdout;
  const struct paper *ppr;
  int cnt;

  fprintf(fi, "Usage: %s [--papersize PAPERSIZE]\n", progname);
  fprintf(fi, "\n");
  fprintf(fi, "PAPERSIZE may be one of:\n");

  ppr = paperfirst();
  cnt = 0;

  while (ppr != NULL)
    {
      fprintf(fi, "\t%s", papername(ppr));
      cnt++;
      if (cnt == 5)
        {
          cnt = 0;
          fprintf(fi, "\n");
        }

      ppr = papernext(ppr);
    }

  fprintf(fi, "\n");
  if (cnt != 0)
    fprintf(fi, "\n");
}
#endif

/* ================================================================================== */

static void parse_file_options(struct cfginfo *restrict tmpcfg, const char *filename)
{
  char str[256];
  char *arg;
  FILE *fi = fopen(filename, "r");

  if (!fi)
    return;

  while (fgets(str, sizeof(str), fi))
    {
      if (!isalnum(*str))
        continue;
      strip_trailing_whitespace(str);
      arg = strchr(str, '=');
      if (arg)
        *arg++ = '\0';

#ifdef __linux__
#ifndef __ANDROID__
      /* Perform shell expansion */
      wordexp_t result;

      wordexp(arg, &result, 0);
      arg = strdup(result.we_wordv[0]);
      wordfree(&result);
#endif
#endif

      /* FIXME: leaking mem here, but the trouble is that these
         strings get mixed in with ones from .data and .rodata
         and free() isn't smart about the situation -- also some
         of the strings end up being kept around */
      parse_one_option(tmpcfg, str, strdup(arg), filename);
#ifdef __linux__
#ifndef __ANDROID__
      free(arg);
#endif
#endif
    }
  fclose(fi);

  /* These interact in horrid ways. */
  if (tmpcfg->parsertmp_lang && tmpcfg->parsertmp_locale)
    fprintf(stderr,
            "Warning: option 'lang=%s' overrides option 'locale=%s' in '%s'\n",
            tmpcfg->parsertmp_lang, tmpcfg->parsertmp_locale, filename);
  if (tmpcfg->parsertmp_lang)
    tmpcfg->parsertmp_locale = PARSE_CLOBBER;
  else if (tmpcfg->parsertmp_locale)
    tmpcfg->parsertmp_lang = PARSE_CLOBBER;
}

static void parse_argv_options(struct cfginfo *restrict tmpcfg, char *argv[])
{
  char *str, *arg;

  const char *short_synonyms[][2] = {
    {"-c", "copying"},
    {"-h", "help"},
    {"-u", "usage"},
    {"-v", "version"},
    {"-vv", "verbose-version"},
    {"-l", "lang"},
    {"-L", "locale"},
    {"-b", "startblank"},
    {"-f", "fullscreen"},
    {"-m", "mixedcase"},
    {"-p", "noprint"},
    {"-q", "nosound"},
    {"-s", "simpleshapes"},
    {"-w", "windowed"},
    {"-x", "noquit"},
    {NULL, NULL}
  };

  while ((str = *++argv))
    {
      if (str[0] == '-' && str[1] != '-' && str[1] != '\0')
        {
          int i, found = 0;

          for (i = 0; short_synonyms[i][0] != NULL && !found; i++)
            {
              if (strcmp(short_synonyms[i][0], str) == 0)
                {
                  if (argv[1] && argv[1][0] != '-')
                    arg = *++argv;
                  else
                    arg = NULL;
                  parse_one_option(tmpcfg, short_synonyms[i][1], arg, NULL);
                  found = 1;
                }
            }
          if (found)
            continue;
        }
      else if (str[0] == '-' && str[1] == '-' && str[2])
        {
          str += 2;
          arg = strchr(str, '=');
          if (arg)
            *arg++ = '\0';
          else if (argv[1] && argv[1][0] != '-')
            arg = *++argv;
          parse_one_option(tmpcfg, str, arg, NULL);
          continue;
        }
      fprintf(stderr, "%s is not understood\n", *argv);
      show_usage(63);
      exit(1);
    }

  /* These interact in horrid ways. */
  if (tmpcfg->parsertmp_lang && tmpcfg->parsertmp_locale)
    {
      fprintf(stderr,
              "Error: command line option '--lang=%s' overrides option '--locale=%s'\n",
              tmpcfg->parsertmp_lang, tmpcfg->parsertmp_locale);
      exit(92);
    }
  if (tmpcfg->parsertmp_lang)
    tmpcfg->parsertmp_locale = PARSE_CLOBBER;
  else if (tmpcfg->parsertmp_locale)
    tmpcfg->parsertmp_lang = PARSE_CLOBBER;
}

/* merge two configs, with the winner taking priority */
static void tmpcfg_merge(struct cfginfo *loser, const struct cfginfo *winner)
{
  int i = CFGINFO_MAXOFFSET / sizeof(char *);

  while (i--)
    {
      const char *cfgitem;

      memcpy(&cfgitem, i * sizeof(const char *) + (const char *)winner, sizeof cfgitem);
      if (!cfgitem)
        continue;
      memcpy(i * sizeof(const char *) + (char *)loser, &cfgitem, sizeof cfgitem);
    }
}

static void setup_config(char *argv[])
{
  char str[128];

#if !defined(_WIN32) && !defined(__ANDROID__)
  const char *home = getenv("HOME");
#endif

  struct cfginfo tmpcfg_usr;
  struct cfginfo tmpcfg_cmd;
  struct cfginfo tmpcfg;

  memset(&tmpcfg_usr, '\0', sizeof tmpcfg_usr);
  memset(&tmpcfg_cmd, '\0', sizeof tmpcfg_cmd);
  memset(&tmpcfg, '\0', sizeof tmpcfg);

  parse_argv_options(&tmpcfg_cmd, argv);

#if defined(__APPLE__)
  /* EP added this conditional section for Mac to allow for a config in
     the current directory, that supersedes sys and user configs */
  /* Mac OS X: Use a "tuxpaint.cfg" file in the current folder */
  struct cfginfo tmpcfg_curdir;

  memset(&tmpcfg_curdir, '\0', sizeof tmpcfg_curdir);
  parse_file_options(&tmpcfg_curdir, "./tuxpaint.cfg");
  tmpcfg_merge(&tmpcfg_curdir, &tmpcfg_cmd);
#endif

  /* Set default options: */

#if !defined(_WIN32) && !defined(__ANDROID__)
  if (!home)
    {
      /* Woah, don't know where $HOME is? */
      fprintf(stderr, "Error: You have no $HOME environment variable!\n");
      exit(1);
    }
#endif

  if (tmpcfg_cmd.savedir)
    savedir = strdup(tmpcfg_cmd.savedir);
  else
    {
#ifdef _WIN32
      savedir = GetDefaultSaveDir("TuxPaint");
#elif __BEOS__
      savedir = strdup("./tuxpaint");
#elif __HAIKU__
      /* Haiku: Make use of find_directory() */
      dev_t volume = dev_for_path("/boot");
      char buffer[B_PATH_NAME_LENGTH + B_FILE_NAME_LENGTH];
      status_t result;

      result = find_directory(B_USER_DIRECTORY, volume, false, buffer, sizeof(buffer));
      asprintf((char **)&savedir, "%s/%s", buffer, "TuxPaint");
#elif __APPLE__
      savedir = strdup(macos.preferencesPath());
#elif __ANDROID__
      savedir = SDL_AndroidGetExternalStoragePath();
#else
      int tmp;

      tmp = asprintf((char **)&savedir, "%s/%s", home, ".tuxpaint");
      if (tmp < 0)
        {
          fprintf(stderr, "Can't set savedir\n");
          exit(91);
        }
#endif
    }

  /* Load options from user's own configuration (".rc" / ".cfg") file: */

#if defined(_WIN32)
  /* Default local config file in users savedir directory on Windows */
  snprintf(str, sizeof(str), "%s/tuxpaint.cfg", savedir);       /* FIXME */
#elif defined(__BEOS__) || defined(__HAIKU__)
  /* BeOS: Use a "tuxpaint.cfg" file: */
  strcpy(str, "tuxpaint.cfg");
#elif defined(__APPLE__)
  /* Mac OS X: Use a "tuxpaint.cfg" file in the Tux Paint application support folder */
  snprintf(str, sizeof(str), "%s/tuxpaint.cfg", macos.preferencesPath());
#elif defined(__ANDROID__)
  /* Try to find the user's config file */
  /* This file is writed by the tuxpaint config activity when the user runs it */
  snprintf(str, sizeof(str), "%s/tuxpaint.cfg", SDL_AndroidGetExternalStoragePath());
#else
  /* Linux and other Unixes:  Use 'rc' style (~/.tuxpaintrc) */
  /* it should it be "~/.tuxpaint/tuxpaintrc" instead, but too late now */
  snprintf(str, sizeof(str), "%s/.tuxpaintrc", home);
#endif
  parse_file_options(&tmpcfg_usr, str);



#if defined(__APPLE__)
  /* EP added this conditional section for Mac */
  tmpcfg_merge(&tmpcfg_usr, &tmpcfg_curdir);
#else
  tmpcfg_merge(&tmpcfg_usr, &tmpcfg_cmd);
#endif

  if (tmpcfg_usr.parsertmp_sysconfig != PARSE_NO)
    {
      struct cfginfo tmpcfg_sys;

      memset(&tmpcfg_sys, '\0', sizeof tmpcfg_sys);
#ifdef _WIN32
      /* global config file in the application directory */
      parse_file_options(&tmpcfg_sys, "tuxpaint.cfg");
#elif defined(__APPLE__)
      /* EP added this conditional section for Mac to fix
         folder & extension inconsistency with Tux Paint Config application) */
      /* Mac OS X: Use a "tuxpaint.cfg" file in the *global* Tux Paint
         application support folder */
      snprintf(str, sizeof(str), "%s/tuxpaint.cfg", macos_globalPreferencesPath());
      parse_file_options(&tmpcfg_sys, str);
#elif defined(__ANDROID__)
      /* Load the config file we provide in assets/etc/tuxpaint.cfg */
      snprintf(str, sizeof(str), "etc/tuxpaint.cfg");

      parse_file_options(&tmpcfg_sys, str);
#else
      /* normally /etc/tuxpaint/tuxpaint.conf */
      parse_file_options(&tmpcfg_sys, CONFDIR "tuxpaint.conf");
#endif
      tmpcfg_merge(&tmpcfg, &tmpcfg_sys);
    }
  tmpcfg_merge(&tmpcfg, &tmpcfg_usr);

  if (tmpcfg.savedir)
    {
      free((char *)savedir);
      savedir = tmpcfg.savedir;
    }

  datadir = tmpcfg.datadir ? tmpcfg.datadir : savedir;

  if (tmpcfg.parsertmp_lang == PARSE_CLOBBER)
    tmpcfg.parsertmp_lang = NULL;
  if (tmpcfg.parsertmp_locale == PARSE_CLOBBER)
    tmpcfg.parsertmp_locale = NULL;
  button_label_y_nudge = setup_i18n(tmpcfg.parsertmp_lang, tmpcfg.parsertmp_locale);


  /* FIXME: most of this is not required before starting the font scanner */

#ifdef PAPER_H
  if (tmpcfg_cmd.papersize && !strcmp(tmpcfg_cmd.papersize, "help"))
    show_available_papersizes(0);
#endif

#define SETBOOL(x) do{ if(tmpcfg.x) x = (tmpcfg.x==PARSE_YES); }while(0)
  SETBOOL(all_locale_fonts);
  SETBOOL(autosave_on_quit);
  SETBOOL(disable_label);
  SETBOOL(disable_magic_controls);
  SETBOOL(disable_print);
  SETBOOL(disable_quit);
  SETBOOL(disable_save);
  SETBOOL(disable_screensaver);
  SETBOOL(disable_stamp_controls);
  SETBOOL(dont_do_xor);
  SETBOOL(dont_load_stamps);
  SETBOOL(fullscreen);
  SETBOOL(grab_input);
  SETBOOL(hide_cursor);
  SETBOOL(keymouse);
  SETBOOL(mirrorstamps);
  SETBOOL(native_screensize);
  SETBOOL(no_button_distinction);
  SETBOOL(no_fancy_cursors);
  SETBOOL(no_system_fonts);
  SETBOOL(noshortcuts);
  SETBOOL(ok_to_use_lockfile);
  SETBOOL(only_uppercase);
  SETBOOL(simple_shapes);
  SETBOOL(start_blank);
  SETBOOL(use_print_config);
  SETBOOL(use_sound);
  SETBOOL(wheely);
  SETBOOL(mouseaccessibility);
  SETBOOL(onscreen_keyboard);
  SETBOOL(onscreen_keyboard_disable_change);
  SETBOOL(_promptless_save_over);
  SETBOOL(_promptless_save_over_new);
  SETBOOL(_promptless_save_over_ask);
#undef SETBOOL

  if (tmpcfg.parsertmp_windowsize)
    {
      char *endp1;
      char *endp2;
      int w = strtoul(tmpcfg.parsertmp_windowsize, &endp1, 10);
      int h = strtoul(endp1 + 1, &endp2, 10);

      if (tmpcfg.parsertmp_windowsize == endp1 || endp1 + 1 == endp2 || *endp1 != 'x' || *endp2)
        {
          fprintf(stderr, "Window size '%s' is not understood.\n", tmpcfg.parsertmp_windowsize);
          exit(97);
        }
      if (w < 500 || w > 32000 || h < 480 || h > 32000 || h > w * 3 || w > h * 4)
        {
          fprintf(stderr, "Window size '%s' is not reasonable.\n", tmpcfg.parsertmp_windowsize);
          exit(93);
        }
      WINDOW_WIDTH = w;
      WINDOW_HEIGHT = h;
    }
  if (tmpcfg.parsertmp_fullscreen_native)
    {
      /* should conflict with other fullscreen/native_screensize setting? */
      if (!strcmp(tmpcfg.parsertmp_fullscreen_native, "native"))
        native_screensize = 1;
      fullscreen = strcmp(tmpcfg.parsertmp_fullscreen_native, "no");
    }
  if (tmpcfg.stamp_size_override)
    {
      if (!strcmp(tmpcfg.stamp_size_override, "default"))
        stamp_size_override = -1;
      else
        {
          /* FIXME: needs to be a scaling factor */
          stamp_size_override = atoi(tmpcfg.stamp_size_override);
          if (stamp_size_override > 10)
            stamp_size_override = 10;
        }
    }
  /* FIXME: make this dynamic (accelerometer or OLPC XO-1 rotation button) */
  if (tmpcfg.rotate_orientation)
    rotate_orientation = !strcmp(tmpcfg.rotate_orientation, "portrait");        /* alternative is "landscape" */
  if (tmpcfg.colorfile)
    strcpy(colorfile, tmpcfg.colorfile);        /* FIXME can overflow */
  if (tmpcfg.print_delay)
    {
      print_delay = atoi(tmpcfg.print_delay);
      last_print_time = -print_delay;
    }
#ifdef PAPER_H
  if (tmpcfg.printcommand)
    printcommand = tmpcfg.printcommand;
  if (tmpcfg.altprintcommand)
    altprintcommand = tmpcfg.altprintcommand;
#endif
  if (tmpcfg.alt_print_command_default)
    {
      /* FIXME: probably need extra variables */
      if (!strcmp(tmpcfg.alt_print_command_default, "always"))
        alt_print_command_default = ALTPRINT_ALWAYS;
      else if (!strcmp(tmpcfg.alt_print_command_default, "never"))
        alt_print_command_default = ALTPRINT_NEVER;
      else
        alt_print_command_default = ALTPRINT_MOD;       /* default ("mod") */
    }
#ifdef PAPER_H
  if (tmpcfg.papersize)
    papersize = tmpcfg.papersize;
#endif
  if (tmpcfg.joystick_dev)
    {
      if (strcmp(tmpcfg.joystick_dev, "list") == 0)
        {
          joystick_dev = -1;
        }
      else
        {
          if (strtof(tmpcfg.joystick_dev, NULL) < 0 || strtof(tmpcfg.joystick_dev, NULL) > 100)
            {
              printf("Joystick dev (now %s) must be between 0 and 100.\n", tmpcfg.joystick_dev);
              exit(1);
            }
          joystick_dev = strtof(tmpcfg.joystick_dev, NULL);
        }
    }
  if (tmpcfg.joystick_slowness)
    {
      if (strtof(tmpcfg.joystick_slowness, NULL) < 0 || strtof(tmpcfg.joystick_slowness, NULL) > 500)
        {
          printf("Joystick slowness (now %s) must be between 0 and 500.\n", tmpcfg.joystick_slowness);
          exit(1);
        }
      joystick_slowness = strtof(tmpcfg.joystick_slowness, NULL);
    }
  if (tmpcfg.joystick_lowthreshold)
    {
      if (strtof(tmpcfg.joystick_lowthreshold, NULL) < 0 || strtof(tmpcfg.joystick_lowthreshold, NULL) > 32766)
        {
          /* FIXME: Find better exit code */
          printf("Joystick lower threshold (now %s)  must be between 0 and 32766", tmpcfg.joystick_lowthreshold);
          exit(1);
        }
      joystick_low_threshold = strtof(tmpcfg.joystick_lowthreshold, NULL);
    }
  if (tmpcfg.joystick_maxsteps)
    {
      if (strtof(tmpcfg.joystick_maxsteps, NULL) < 1 || strtof(tmpcfg.joystick_maxsteps, NULL) > 7)
        {
          /* FIXME: Find better exit code */
          printf("Joystick max steps (now %s)  must be between 1 and 7", tmpcfg.joystick_maxsteps);
          exit(1);
        }
      joystick_maxsteps = strtof(tmpcfg.joystick_maxsteps, NULL);
    }
  if (tmpcfg.joystick_hat_slowness)
    {
      if (strtof(tmpcfg.joystick_hat_slowness, NULL) < 0 || strtof(tmpcfg.joystick_hat_slowness, NULL) > 500)
        {
          printf("Joystick hat slowness (now %s) must be between 0 and 500.\n", tmpcfg.joystick_hat_slowness);
          exit(1);
        }
      joystick_hat_slowness = strtof(tmpcfg.joystick_hat_slowness, NULL);
    }
  if (tmpcfg.joystick_hat_timeout)
    {
      if (strtof(tmpcfg.joystick_hat_timeout, NULL) < 0 || strtof(tmpcfg.joystick_hat_timeout, NULL) > 3000)
        {
          /* FIXME: Find better exit code */
          printf("Joystick hat timeout (now %s)  must be between 0 and 3000", tmpcfg.joystick_hat_timeout);
          exit(1);
        }
      joystick_hat_timeout = strtof(tmpcfg.joystick_hat_timeout, NULL);
    }
  if (tmpcfg.joystick_button_escape)
    {
      if (strtof(tmpcfg.joystick_button_escape, NULL) < 0 || strtof(tmpcfg.joystick_button_escape, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button escape shortcurt (now %s)  must be between 0 and 254", tmpcfg.joystick_button_escape);
          exit(1);
        }
      joystick_button_escape = strtof(tmpcfg.joystick_button_escape, NULL);
    }
  if (tmpcfg.joystick_button_selectbrushtool)
    {
      if (strtof(tmpcfg.joystick_button_selectbrushtool, NULL) < 0
          || strtof(tmpcfg.joystick_button_selectbrushtool, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button brush tool shortcurt (now %s)  must be between 0 and 254",
                 tmpcfg.joystick_button_selectbrushtool);
          exit(1);
        }
      joystick_button_selectbrushtool = strtof(tmpcfg.joystick_button_selectbrushtool, NULL);
    }
  if (tmpcfg.joystick_button_selectstamptool)
    {
      if (strtof(tmpcfg.joystick_button_selectstamptool, NULL) < 0
          || strtof(tmpcfg.joystick_button_selectstamptool, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button stamp tool shortcurt (now %s)  must be between 0 and 254",
                 tmpcfg.joystick_button_selectstamptool);
          exit(1);
        }
      joystick_button_selectstamptool = strtof(tmpcfg.joystick_button_selectstamptool, NULL);
    }
  if (tmpcfg.joystick_button_selectlinestool)
    {
      if (strtof(tmpcfg.joystick_button_selectlinestool, NULL) < 0
          || strtof(tmpcfg.joystick_button_selectlinestool, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button lines tool shortcurt (now %s)  must be between 0 and 254",
                 tmpcfg.joystick_button_selectlinestool);
          exit(1);
        }
      joystick_button_selectlinestool = strtof(tmpcfg.joystick_button_selectlinestool, NULL);
    }
  if (tmpcfg.joystick_button_selectshapestool)
    {
      if (strtof(tmpcfg.joystick_button_selectshapestool, NULL) < 0
          || strtof(tmpcfg.joystick_button_selectshapestool, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button shapes tool shortcurt (now %s)  must be between 0 and 254",
                 tmpcfg.joystick_button_selectshapestool);
          exit(1);
        }
      joystick_button_selectshapestool = strtof(tmpcfg.joystick_button_selectshapestool, NULL);
    }
  if (tmpcfg.joystick_button_selecttexttool)
    {
      if (strtof(tmpcfg.joystick_button_selecttexttool, NULL) < 0
          || strtof(tmpcfg.joystick_button_selecttexttool, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button text tool shortcurt (now %s)  must be between 0 and 254",
                 tmpcfg.joystick_button_selecttexttool);
          exit(1);
        }
      joystick_button_selecttexttool = strtof(tmpcfg.joystick_button_selecttexttool, NULL);
    }
  if (tmpcfg.joystick_button_selectlabeltool)
    {
      if (strtof(tmpcfg.joystick_button_selectlabeltool, NULL) < 0
          || strtof(tmpcfg.joystick_button_selectlabeltool, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button label tool shortcurt (now %s)  must be between 0 and 254",
                 tmpcfg.joystick_button_selectlabeltool);
          exit(1);
        }
      joystick_button_selectlabeltool = strtof(tmpcfg.joystick_button_selectlabeltool, NULL);
    }
  if (tmpcfg.joystick_button_selectmagictool)
    {
      if (strtof(tmpcfg.joystick_button_selectmagictool, NULL) < 0
          || strtof(tmpcfg.joystick_button_selectmagictool, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button magic tool shortcurt (now %s)  must be between 0 and 254",
                 tmpcfg.joystick_button_selectmagictool);
          exit(1);
        }
      joystick_button_selectmagictool = strtof(tmpcfg.joystick_button_selectmagictool, NULL);
    }
  if (tmpcfg.joystick_button_undo)
    {
      if (strtof(tmpcfg.joystick_button_undo, NULL) < 0 || strtof(tmpcfg.joystick_button_undo, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button undo shortcurt (now %s)  must be between 0 and 254", tmpcfg.joystick_button_undo);
          exit(1);
        }
      joystick_button_undo = strtof(tmpcfg.joystick_button_undo, NULL);
    }
  if (tmpcfg.joystick_button_redo)
    {
      if (strtof(tmpcfg.joystick_button_redo, NULL) < 0 || strtof(tmpcfg.joystick_button_redo, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button redo shortcurt (now %s)  must be between 0 and 254", tmpcfg.joystick_button_redo);
          exit(1);
        }
      joystick_button_redo = strtof(tmpcfg.joystick_button_redo, NULL);
    }
  if (tmpcfg.joystick_button_selecterasertool)
    {
      if (strtof(tmpcfg.joystick_button_selecterasertool, NULL) < 0
          || strtof(tmpcfg.joystick_button_selecterasertool, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button eraser tool shortcurt (now %s)  must be between 0 and 254",
                 tmpcfg.joystick_button_selecterasertool);
          exit(1);
        }
      joystick_button_selecterasertool = strtof(tmpcfg.joystick_button_selecterasertool, NULL);
    }
  if (tmpcfg.joystick_button_new)
    {
      if (strtof(tmpcfg.joystick_button_new, NULL) < 0 || strtof(tmpcfg.joystick_button_new, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button new shortcurt (now %s)  must be between 0 and 254", tmpcfg.joystick_button_new);
          exit(1);
        }
      joystick_button_new = strtof(tmpcfg.joystick_button_new, NULL);
    }
  if (tmpcfg.joystick_button_open)
    {
      if (strtof(tmpcfg.joystick_button_open, NULL) < 0 || strtof(tmpcfg.joystick_button_open, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button open shortcurt (now %s)  must be between 0 and 254", tmpcfg.joystick_button_open);
          exit(1);
        }
      joystick_button_open = strtof(tmpcfg.joystick_button_open, NULL);
    }
  if (tmpcfg.joystick_button_save)
    {
      if (strtof(tmpcfg.joystick_button_save, NULL) < 0 || strtof(tmpcfg.joystick_button_save, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button save shortcurt (now %s)  must be between 0 and 254", tmpcfg.joystick_button_save);
          exit(1);
        }
      joystick_button_save = strtof(tmpcfg.joystick_button_save, NULL);
    }
  if (tmpcfg.joystick_button_pagesetup)
    {
      if (strtof(tmpcfg.joystick_button_pagesetup, NULL) < 0 || strtof(tmpcfg.joystick_button_pagesetup, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button page setup shortcurt (now %s)  must be between 0 and 254",
                 tmpcfg.joystick_button_pagesetup);
          exit(1);
        }
      joystick_button_pagesetup = strtof(tmpcfg.joystick_button_pagesetup, NULL);
    }
  if (tmpcfg.joystick_button_print)
    {
      if (strtof(tmpcfg.joystick_button_print, NULL) < 0 || strtof(tmpcfg.joystick_button_print, NULL) > 254)
        {
          /* FIXME: Find better exit code */
          printf("Joystick button print shortcurt (now %s)  must be between 0 and 254", tmpcfg.joystick_button_print);
          exit(1);
        }
      joystick_button_print = strtof(tmpcfg.joystick_button_print, NULL);
    }
  if (tmpcfg.joystick_buttons_ignore)
    {
      char *token;

      token = strtok(tmpcfg.joystick_buttons_ignore, ",");
      while (token != NULL)
        {
          if (strtof(token, NULL) < 0 || strtof(token, NULL) > 254)
            {
              /* FIXME: Find better exit code */
              printf("Joystick buttons must be between 0 and 254 (don't like %s)", tmpcfg.joystick_buttons_ignore);
              exit(1);
            }
          joystick_buttons_ignore[joystick_buttons_ignore_len++] = strtof(token, NULL);
          token = strtok(NULL, ",");
        }
    }


  /* having any of theese implies having onscreen keyboard setted */
  if (tmpcfg.onscreen_keyboard_layout)
    {
      onscreen_keyboard_layout = strdup(tmpcfg.onscreen_keyboard_layout);
      onscreen_keyboard = TRUE;
    }

  if (tmpcfg.onscreen_keyboard_disable_change)
    {
      onscreen_keyboard = TRUE;
    }

#ifdef DEBUG
  printf("\n\nPromptless save:\nask: %d\nnew: %d\nover: %d\n\n", _promptless_save_over_ask, _promptless_save_over_new,
         _promptless_save_over);
#endif

  if (_promptless_save_over_ask)
    {
      promptless_save = SAVE_OVER_PROMPT;
    }
  else if (_promptless_save_over_new)
    {
      promptless_save = SAVE_OVER_NO;
    }
  else if (_promptless_save_over)
    {
      promptless_save = SAVE_OVER_ALWAYS;
    }


}


static void chdir_to_binary(char *argv0)
{
  /*
     char curdir[256];
   */
  /* EP added this block to print out of current directory */

  /*
     getcwd(curdir, sizeof(curdir));
     #ifdef DEBUG
     printf("Binary Path: %s\nCurrent directory at launchtime: %s\n", argv0, curdir);
     #endif
   */

#if defined(__BEOS__) || defined(WIN32) || defined(__APPLE__)
  /* if run from gui, like OpenTracker in BeOS or Explorer in Windows,
     find path from which binary was run and change dir to it
     so all files will be local :) */
  /* UPDATE (2004.10.06): Since SDL 1.2.7 SDL sets that path correctly,
     so this code wouldn't be needed if SDL was init before anything else,
     (just basic init, window shouldn't be needed). */
  /* UPDATE (2005 July 19): Enable and make work on Windows. Makes testing
     with MINGW/MSYS easier */

  if (argv0)
    {
      char *app_path = strdup(argv0);
      char *slash = strrchr(app_path, '/');

#if defined(__APPLE__)
      // On macOS, execution is deep inside the app bundle.
      // E.g., "/Applications/TuxPaint.app/Contents/MacOS/tuxpaint"
      // But we want to point somewhere higher up, say to "Contents", so we can access
      // the resources in Resources folder. So move up one level.
      int levels = 1;           /* we need to back up 1 level */

      while ((levels-- > 0) && (slash))
        {
          *slash = '\0';        /* this overwrites the \0 at end of string */
          slash = strrchr(app_path, '/');       /* so we can carry on our back-pedaling... */
        }
#endif

      if (!slash)
        {
          slash = strrchr(app_path, '\\');
        }
      if (slash)
        {
          *(slash + 1) = '\0';
          chdir(app_path);
        }
      free(app_path);
      /*
         getcwd(curdir, sizeof(curdir));
         printf("New current directory for runtime: %s\n", curdir);
       */
    }
#else
  (void)argv0;
#endif
}

/* ================================================================================== */

static void setup_colors(void)
{
  FILE *fi;
  int i, j;

  /* Load colors, or use default ones: */

  if (colorfile[0] != '\0')
    {
      fi = fopen(colorfile, "r");
      if (fi == NULL)
        {
          fprintf(stderr, "\nWarning, could not open color file. Using defaults.\n");
          perror(colorfile);
          colorfile[0] = '\0';
        }
      else
        {
          int max = 0, per = 5;
          char str[80], tmp_str[80];
          int count;

          NUM_COLORS = 0;

          do
            {
              if (fgets(str, sizeof(str), fi))
                {
                  if (!feof(fi))
                    {
                      if (NUM_COLORS + 1 > max)
                        {
                          color_hexes = realloc(color_hexes, sizeof(Uint8 *) * (max + per));
                          color_names = realloc(color_names, sizeof(char *) * (max + per));

                          for (i = max; i < max + per; i++)
                            color_hexes[i] = malloc(sizeof(Uint8) * 3);

                          max = max + per;
                        }

                      while (str[strlen(str) - 1] == '\n' || str[strlen(str) - 1] == '\r')
                        str[strlen(str) - 1] = '\0';

                      if (str[0] == '#')
                        {
                          /* Hex form */

                          sscanf(str + 1, "%s %n", tmp_str, &count);

                          if (strlen(tmp_str) == 6)
                            {
                              /* Byte (#rrggbb) form */

                              color_hexes[NUM_COLORS][0] = (hex2dec(tmp_str[0]) << 4) + hex2dec(tmp_str[1]);
                              color_hexes[NUM_COLORS][1] = (hex2dec(tmp_str[2]) << 4) + hex2dec(tmp_str[3]);
                              color_hexes[NUM_COLORS][2] = (hex2dec(tmp_str[4]) << 4) + hex2dec(tmp_str[5]);

                              color_names[NUM_COLORS] = strdup(str + count);
                              NUM_COLORS++;
                            }
                          else if (strlen(tmp_str) == 3)
                            {
                              /* Nybble (#rgb) form */

                              color_hexes[NUM_COLORS][0] = (hex2dec(tmp_str[0]) << 4) + hex2dec(tmp_str[0]);
                              color_hexes[NUM_COLORS][1] = (hex2dec(tmp_str[1]) << 4) + hex2dec(tmp_str[1]);
                              color_hexes[NUM_COLORS][2] = (hex2dec(tmp_str[2]) << 4) + hex2dec(tmp_str[2]);

                              color_names[NUM_COLORS] = strdup(str + count);
                              NUM_COLORS++;
                            }
                        }
                      else
                        {
                          /* Assume int form */

                          if (sscanf(str, "%hu %hu %hu %n",
                                     (short unsigned int *)&(color_hexes[NUM_COLORS][0]),
                                     (short unsigned int *)&(color_hexes[NUM_COLORS][1]),
                                     (short unsigned int *)&(color_hexes[NUM_COLORS][2]), &count) >= 3)
                            {
                              color_names[NUM_COLORS] = strdup(str + count);
                              NUM_COLORS++;
                            }
                        }
                    }
                }
            }
          while (!feof(fi));

          if (NUM_COLORS < 2)
            {
              fprintf(stderr, "\nWarning, not enough colors in color file. Using defaults.\n");
              fprintf(stderr, "%s\n", colorfile);
              colorfile[0] = '\0';

              for (i = 0; i < NUM_COLORS; i++)
                {
                  free(color_names[i]);
                  free(color_hexes[i]);
                }

              free(color_names);
              free(color_hexes);
            }
        }
    }

  /* Use default, if no file specified (or trouble opening it) */

  if (colorfile[0] == '\0')
    {
      NUM_COLORS = NUM_DEFAULT_COLORS;

      color_hexes = malloc(sizeof(Uint8 *) * NUM_COLORS);
      color_names = malloc(sizeof(char *) * NUM_COLORS);

      for (i = 0; i < NUM_COLORS; i++)
        {
          color_hexes[i] = malloc(sizeof(Uint8 *) * 3);

          for (j = 0; j < 3; j++)
            color_hexes[i][j] = default_color_hexes[i][j];

          color_names[i] = strdup(default_color_names[i]);
        }
    }


  /* Add "Color Select" color: */

  color_hexes = (Uint8 **) realloc(color_hexes, sizeof(Uint8 *) * (NUM_COLORS + 1));

  color_names = (char **)realloc(color_names, sizeof(char *) * (NUM_COLORS + 1));
  color_names[NUM_COLORS] = strdup(gettext("Select a color from your drawing."));
  color_hexes[NUM_COLORS] = (Uint8 *) malloc(sizeof(Uint8) * 3);
  color_hexes[NUM_COLORS][0] = 0;
  color_hexes[NUM_COLORS][1] = 0;
  color_hexes[NUM_COLORS][2] = 0;
  NUM_COLORS++;

  /* Add "Color Picker" color: */

  color_hexes = (Uint8 **) realloc(color_hexes, sizeof(Uint8 *) * (NUM_COLORS + 1));

  color_names = (char **)realloc(color_names, sizeof(char *) * (NUM_COLORS + 1));
  color_names[NUM_COLORS] = strdup(gettext("Pick a color."));
  color_hexes[NUM_COLORS] = (Uint8 *) malloc(sizeof(Uint8) * 3);
  color_hexes[NUM_COLORS][0] = 0;
  color_hexes[NUM_COLORS][1] = 0;
  color_hexes[NUM_COLORS][2] = 0;
  color_picker_x = 0;
  color_picker_y = 0;
  NUM_COLORS++;

}

/* ================================================================================== */

static void do_lock_file(void)
{
  FILE *fi;
  char *lock_fname;
  time_t time_lock, time_now;
  char *homedirdir;

  /* Test for lockfile, if we're using one: */

  if (!ok_to_use_lockfile)
    return;

  /* Get the current time: */

  time_now = time(NULL);

  /* Look for the lockfile... */

#ifndef WIN32
  lock_fname = get_fname("lockfile.dat", DIR_SAVE);
#else
  lock_fname = get_temp_fname("lockfile.dat");
#endif

  fi = fopen(lock_fname, "r");
  if (fi != NULL)
    {
      /* If it exists, read its contents: */

      if (fread(&time_lock, sizeof(time_t), 1, fi) > 0)
        {
          /* Has it not been 30 seconds yet? */

          if (time_now < time_lock + 30)
            {
              /* FIXME: Wrap in gettext() */
              printf
                ("You have already started tuxpaint less than 30 seconds ago.\n"
                 "To prevent multiple executions by mistake, TuxPaint will not run\n"
                 "before 30 seconds have elapsed since it was last started.\n"
                 "\n" "You can also use the --nolockfile argument, see tuxpaint(1).\n\n");

              free(lock_fname);

              fclose(fi);
              exit(0);
            }
        }

      fclose(fi);
    }


  /* Okay to run; create/update the lockfile */

  /* (Make sure the directory exists, first!) */
  homedirdir = get_fname("", DIR_SAVE);
  mkdir(homedirdir, 0755);
  free(homedirdir);


  fi = fopen(lock_fname, "w");
  if (fi != NULL)
    {
      /* If we can write to it, do so! */

      fwrite(&time_now, sizeof(time_t), 1, fi);
      fclose(fi);
    }
  else
    {
      fprintf(stderr,
              "\nWarning: I couldn't create the lockfile (%s)\n"
              "The error that occurred was:\n" "%s\n\n", lock_fname, strerror(errno));
    }

  free(lock_fname);
}

int TP_EventFilter(void *data, const SDL_Event * event)
{
  if (event->type == SDL_QUIT ||
      event->type == SDL_WINDOWEVENT ||
      event->type == SDL_JOYAXISMOTION ||
      event->type == SDL_JOYBALLMOTION ||
      event->type == SDL_JOYHATMOTION ||
      event->type == SDL_JOYBUTTONDOWN ||
      event->type == SDL_JOYBUTTONUP ||
      event->type == SDL_KEYDOWN ||
      event->type == SDL_KEYUP ||
      event->type == SDL_MOUSEBUTTONDOWN ||
      event->type == SDL_MOUSEBUTTONUP ||
      event->type == SDL_MOUSEMOTION ||
      event->type == SDL_QUIT ||
      event->type == SDL_USEREVENT ||
      event->type == SDL_MOUSEWHEEL ||
      event->type == SDL_TEXTINPUT ||
      event->type == SDL_APP_WILLENTERBACKGROUND ||
      event->type == SDL_APP_WILLENTERFOREGROUND ||
      event->type == SDL_APP_DIDENTERBACKGROUND || event->type == SDL_APP_DIDENTERFOREGROUND)
    return 1;

  return 0;
}

/* ================================================================================== */

static void setup(void)
{
  int i;
  int ww, hh;
  char *upstr;
  SDL_Color black = { 0, 0, 0, 0 };
  char *homedirdir;
  SDL_Surface *tmp_surf;
  SDL_Rect dest;
  int scale;

#ifndef LOW_QUALITY_COLOR_SELECTOR
  int x, y;
  SDL_Surface *tmp_btn_up;
  SDL_Surface *tmp_btn_down;
  Uint8 r, g, b;
#endif
  SDL_Surface *tmp_imgcurup, *tmp_imgcurdown;
  Uint32 init_flags;
  char tmp_str[128];
  SDL_Surface *img1;

  Uint32(*getpixel_tmp_btn_up) (SDL_Surface *, int, int);
  Uint32(*getpixel_tmp_btn_down) (SDL_Surface *, int, int);
  Uint32(*getpixel_img_paintwell) (SDL_Surface *, int, int);
  int big_title;

#ifndef NO_SDLPANGO
  SDL_Thread *fontconfig_thread;
#endif

  render_scale = 1.0;


#ifdef _WIN32
  if (fullscreen)
    {
      InstallKeyboardHook();
      SetActivationState(1);
    }
#endif

  im_init(&im_data, get_current_language());

#ifndef NO_SDLPANGO
  SDLPango_Init();
#endif

#ifndef WIN32
  putenv((char *)"SDL_VIDEO_X11_WMCLASS=TuxPaint.TuxPaint");
#endif

  if (disable_screensaver == 0)
    {
      putenv((char *)"SDL_VIDEO_ALLOW_SCREENSAVER=1");
      if (SDL_MAJOR_VERSION < 2 || (SDL_MAJOR_VERSION == 2 && SDL_MINOR_VERSION == 0 && SDL_PATCHLEVEL < 2))
        {
          fprintf(stderr, "Note: 'allowscreensaver' requires SDL 1.2.12 or higher\n");
        }
    }

  if (joystick_dev != -1)
    do_lock_file();

  init_flags = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK;
  if (use_sound)
    init_flags |= SDL_INIT_AUDIO;
  if (!fullscreen)
    init_flags |= SDL_INIT_NOPARACHUTE; /* allow debugger to catch crash */

  /* Init SDL */
  if (SDL_Init(init_flags) < 0)
    {
#ifndef NOSOUND
      char *olderr = strdup(SDL_GetError());

      use_sound = 0;
      init_flags &= ~SDL_INIT_AUDIO;
      if (SDL_Init(init_flags) >= 0)
        {
          /* worked, w/o sound */
          fprintf(stderr,
                  "\nWarning: I could not initialize audio!\n"
                  "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", olderr);
          free(olderr);
        }
      else
#endif
        {
          fprintf(stderr,
                  "\nError: I could not initialize video and/or the timer!\n"
                  "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());
          exit(1);
        }
    }

  /* Set up event filter */

  SDL_SetEventFilter(TP_EventFilter, NULL);


  /* Set up joystick */

  if (joystick_dev == -1)
    {
      printf("%i joystick(s) were found:\n", SDL_NumJoysticks());

      for (i = 0; i < SDL_NumJoysticks(); i++)
        {
          printf(" %d: %s\n", i, SDL_JoystickName(i));
        }

      SDL_Quit();
      exit(0);
    }

  joystick = SDL_JoystickOpen(joystick_dev);
  if (joystick == NULL)
    {
      fprintf(stderr, "Could not open joystick device %d: %s\n", joystick_dev, SDL_GetError());
    }
  else
    {
      SDL_JoystickEventState(SDL_ENABLE);
#ifdef DEBUG
      printf("Number of Axes: %d\n", SDL_JoystickNumAxes(joystick));
      printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(joystick));
      printf("Number of Balls: %d\n", SDL_JoystickNumBalls(joystick));
      printf("Number of Hats: %d\n", SDL_JoystickNumHats(joystick));
#endif
    }


#ifndef NOSOUND
#ifndef WIN32
  if (use_sound && Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 1024) < 0)
#else
  if (use_sound && Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 2048) < 0)
#endif
    {
      fprintf(stderr,
              "\nWarning: I could not set up audio for 44100 Hz "
              "16-bit stereo.\n" "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());
      use_sound = 0;
    }

  i = NUM_SOUNDS;
  while (use_sound && i--)
    {
      sounds[i] = Mix_LoadWAV(sound_fnames[i]);

      if (sounds[i] == NULL)
        {
          fprintf(stderr,
                  "\nWarning: I couldn't open a sound file:\n%s\n"
                  "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", sound_fnames[i], SDL_GetError());
          use_sound = 0;
        }
    }
#endif


  /* Set-up Key-Repeat: */
  //  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  /* Init TTF stuff: */
  if (TTF_Init() < 0)
    {
      fprintf(stderr,
              "\nError: I could not initialize the font (TTF) library!\n"
              "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

      SDL_Quit();
      exit(1);
    }


  setup_colors();

  /* Set window icon and caption: */

#ifndef __APPLE__
  seticon();
#endif
  if (hide_cursor)
    SDL_ShowCursor(SDL_DISABLE);


  /* Deal with orientation rotation option */

  if (rotate_orientation)
    {
      if (native_screensize && fullscreen)
        {
          fprintf(stderr, "Warning: Asking for native screen size overrides request to rotate orientation.\n");
        }
      else
        {
          int tmp;

          tmp = WINDOW_WIDTH;
          WINDOW_WIDTH = WINDOW_HEIGHT;
          WINDOW_HEIGHT = tmp;
        }
    }

  /* Deal with 'native' screen size option */

  if (native_screensize)
    {
      if (!fullscreen)
        {
          fprintf(stderr, "Warning: Asking for native screensize in a window. Ignoring.\n");
        }
      else
        {
          WINDOW_WIDTH = 0;
          WINDOW_HEIGHT = 0;
        }
    }


  /* Open Window: */

  if (fullscreen)
    {
#ifdef USE_HWSURFACE
      /*    screen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT,
         VIDEO_BPP, SDL_FULLSCREEN | SDL_HWSURFACE); */
      window_screen = SDL_CreateWindow("Tux Paint", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                       //0,0,
                                       WINDOW_WIDTH, WINDOW_HEIGHT,
                                       SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_HWSURFACE);
      printf("1\n");
      if (window_screen == NULL)
        printf("window_screen = NULL 1\n");

#else
/*    screen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT,
      VIDEO_BPP, SDL_FULLSCREEN | SDL_SWSURFACE);*/
      window_screen = SDL_CreateWindow(NULL,
                                       //"Tux Paint",
                                       SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                       //0, 0,
                                       WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_FULLSCREEN_DESKTOP);
      printf("2\n");
      if (window_screen == NULL)
        printf("window_screen = NULL 2\n");
#endif

      renderer = SDL_CreateRenderer(window_screen, -1, 0);

      if (native_screensize)
        {
          SDL_GL_GetDrawableSize(window_screen, &ww, &hh);

          /* Tuxpaint goes wrong under 500x480.
             Scale it using SDL2 features */
          if (ww < 500 || hh < 480)
            {
              float window_scale_w = 1.;
              float window_scale_h = 1.;

              window_scale_w = 501.f / ww;
              window_scale_h = 481.f / hh;

	      SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
              if (window_scale_w > window_scale_h)
                {
                  /* Keep things squared */
                  ww = window_scale_w * ww;
                  hh = window_scale_w * hh;
                  render_scale = window_scale_w;

                  SDL_RenderSetScale(renderer, window_scale_w, window_scale_w);
                }
              else
                {
                  ww = window_scale_h * ww;
                  hh = window_scale_h * hh;
                  render_scale = window_scale_h;

                  SDL_RenderSetScale(renderer, window_scale_h, window_scale_h);
                }
            }
        }
      else
        {
          ww = WINDOW_WIDTH;
          hh = WINDOW_HEIGHT;
        }

      texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STATIC, ww, hh);


      screen = SDL_CreateRGBSurface(0, ww, hh, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
      if (screen == NULL)
        {
          fprintf(stderr,
                  "\nWarning: I could not open the display in fullscreen mode.\n"
                  "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

          fullscreen = 0;
        }
      else
        {
          /* Get resolution if we asked for native: */

          if (native_screensize)
            {
              WINDOW_WIDTH = screen->w;
              WINDOW_HEIGHT = screen->h;
            }
        }
    }


  if (!fullscreen)
    {
      int set_window_pos = 0;

      if (getenv((char *)"SDL_VIDEO_WINDOW_POS") == NULL)
        {
          set_window_pos = 1;
          putenv((char *)"SDL_VIDEO_WINDOW_POS=center");
        }

#ifdef USE_HWSURFACE
      /*    screen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT,
         VIDEO_BPP, SDL_HWSURFACE); */
      window_screen = SDL_CreateWindow("Tux Paint",
                                       SDL_WINDOWPOS_UNDEFINED,
                                       SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HWSURFACE);
      printf("3\n");
      if (window_screen == NULL)
        printf("window_screen = NULL 3\n");
#else
      window_screen = SDL_CreateWindow("Tux Paint",
                                       SDL_WINDOWPOS_UNDEFINED,
                                       SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, NULL);
      /*    screen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT,
         s
         VIDEO_BPP, SDL_SWSURFACE); */
      printf("4\n");
      if (window_screen == NULL)
        printf("window_screen = NULL 4\n");

#endif

      if (set_window_pos)
        putenv((char *)"SDL_VIDEO_WINDOW_POS=nopref");

      /* Note: Seems that this depends on the compliance by the window manager
         currently this doesn't works under Fvwm */
      SDL_SetWindowMinimumSize(window_screen, WINDOW_WIDTH, WINDOW_HEIGHT);
      SDL_SetWindowMaximumSize(window_screen, WINDOW_WIDTH, WINDOW_HEIGHT);


      renderer = SDL_CreateRenderer(window_screen, -1, 0);
      SDL_GL_GetDrawableSize(window_screen, &ww, &hh);
      texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STATIC, ww, hh);

      screen = SDL_CreateRGBSurface(0, ww, hh, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);


      //screen = SDL_GetWindowSurface(window_screen);




      if (screen == NULL)
        {
          fprintf(stderr,
                  "\nError: 1 I could not open the display.\n"
                  "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

          cleanup();
          exit(1);
        }
    }

  SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

  window_format = SDL_GetWindowPixelFormat(window_screen);

  /* (Need to do this after native screen resolution is handled) */

  setup_screen_layout();


  /* quickly: title image, version, progress bar, and watch cursor */

  img_title = loadimage(DATA_PREFIX "images/title.png");
  img_title_credits = loadimage(DATA_PREFIX "images/title-credits.png");
  img_progress = loadimage(DATA_PREFIX "images/ui/progress.png");

  if (screen->w - img_title->w >= 410 && screen->h - img_progress->h - img_title_credits->h - 40)       /* FIXME: Font */
    big_title = 1;
  else
    big_title = 0;


  if (big_title)
    img_title_tuxpaint = loadimage(DATA_PREFIX "images/title-tuxpaint-2x.png");
  else
    img_title_tuxpaint = loadimage(DATA_PREFIX "images/title-tuxpaint.png");

  SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 255, 255, 255));

  dest.x = ((WINDOW_WIDTH - img_title->w - (img_title_tuxpaint->w / 2)) / 2) + (img_title_tuxpaint->w / 2) + 20;
  dest.y = (WINDOW_HEIGHT - img_title->h);

  SDL_BlitSurface(img_title, NULL, screen, &dest);

  dest.x = 10;
  if (big_title)
    dest.y = WINDOW_HEIGHT - img_title_tuxpaint->h - img_progress->h - 40;
  else
    dest.y = (WINDOW_HEIGHT - img_title->h) + img_title_tuxpaint->h * 0.8 + 7;

  SDL_BlitSurface(img_title_tuxpaint, NULL, screen, &dest);

  dest.x = 10;
  dest.y = 5;

  SDL_BlitSurface(img_title_credits, NULL, screen, &dest);

  prog_bar_ctr = 0;
  show_progress_bar(screen);

  SDL_Flip(screen);


#if defined(WIN32) && defined(LARGE_CURSOR_FULLSCREEN_BUG)
  if (fullscreen && no_fancy_cursors == 0)
    {
      fprintf(stderr, "Warning: An SDL bug causes the fancy cursors to leave\n"
              "trails in fullscreen mode.  Disabling fancy cursors.\n"
              "(You can do this yourself with 'nofancycursors' option,\n" "to avoid this warning in the future.)\n");
      no_fancy_cursors = 1;
    }
#endif


  /* Create cursors: */

  scale = 1;

#ifdef SMALL_CURSOR_SHAPES
  scale = 2;
#endif

#ifdef __APPLE__
  cursor_arrow = SDL_GetCursor();       /* use standard system cursor */
#endif

  /* this one first, because we need it yesterday */
  cursor_watch = get_cursor(watch_bits, watch_mask_bits, watch_width, watch_height, 14 / scale, 14 / scale);

  do_setcursor(cursor_watch);
  show_progress_bar(screen);




#ifndef NO_SDLPANGO
  /* Let Pango & fontcache do their work without locking up */

  fontconfig_thread_done = 0;

#ifdef DEBUG
  printf("Spawning Pango thread\n");
  fflush(stdout);
#endif

  fontconfig_thread = SDL_CreateThread(generate_fontconfig_cache, "fontconfig_thread", NULL);
  if (fontconfig_thread == NULL)
    {
      fprintf(stderr, "Failed to create Pango setup thread: %s\n", SDL_GetError());
    }
  else
    {
#ifdef DEBUG
      printf("Thread spawned\n");
      fflush(stdout);
#endif
      if (generate_fontconfig_cache_spinner(screen))    /* returns 1 if aborted */
        {
          printf("Pango thread aborted!\n");
          fflush(stdout);
          // FIXME SDL2
          //      SDL_KillThread(fontconfig_thread);
          SDL_Quit();
          exit(0);
          /* FIXME: Let's be more graceful about exiting (e.g., clean up lockfile!) -bjk 2010.04.27 */
        }
#ifdef DEBUG
      printf("Done generating cache\n");
      fflush(stdout);
#endif
    }


#ifdef FORKED_FONTS
  /* NOW we can fork our own font scanner stuff, and let it run in the bgkd -bjk 2010.04.27 */
#ifdef DEBUG
  printf("Now running font scanner\n");
  fflush(stdout);
#endif
  run_font_scanner(screen, texture, renderer, lang_prefixes[get_current_language()]);
#endif

#endif

  medium_font = TuxPaint_Font_OpenFont(PANGO_DEFAULT_FONT,
                                       DATA_PREFIX "fonts/default_font.ttf", 18 - (only_uppercase * 3));

  if (medium_font == NULL)
    {
      fprintf(stderr,
              "\nError: Can't load font file: "
              DATA_PREFIX "fonts/default_font.ttf\n"
              "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

      cleanup();
      exit(1);
    }

  snprintf(tmp_str, sizeof(tmp_str), "Version: %s – %s", VER_VERSION, VER_DATE);

  tmp_surf = render_text(medium_font, tmp_str, black);
  dest.x = 10;
  dest.y = WINDOW_HEIGHT - img_progress->h - tmp_surf->h;
  SDL_BlitSurface(tmp_surf, NULL, screen, &dest);
  SDL_FreeSurface(tmp_surf);

#ifdef DEBUG
  printf("%s\n", tmp_str);
#endif

  snprintf(tmp_str, sizeof(tmp_str), "© 2002–2014 Bill Kendrick et al.");
  tmp_surf = render_text(medium_font, tmp_str, black);
  dest.x = 10;
  dest.y = WINDOW_HEIGHT - img_progress->h - (tmp_surf->h * 2);
  SDL_BlitSurface(tmp_surf, NULL, screen, &dest);
  SDL_FreeSurface(tmp_surf);

  SDL_Flip(screen);


#ifdef FORKED_FONTS
  reliable_write(font_socket_fd, &no_system_fonts, sizeof no_system_fonts);
#else
  font_thread = SDL_CreateThread(load_user_fonts_stub, NULL);
#endif

  /* continuing on with the rest of the cursors... */


#ifndef __APPLE__
  cursor_arrow = get_cursor(arrow_bits, arrow_mask_bits, arrow_width, arrow_height, 0, 0);
#endif

  cursor_hand = get_cursor(hand_bits, hand_mask_bits, hand_width, hand_height, 12 / scale, 1 / scale);

  cursor_wand = get_cursor(wand_bits, wand_mask_bits, wand_width, wand_height, 4 / scale, 4 / scale);

  cursor_insertion = get_cursor(insertion_bits, insertion_mask_bits,
                                insertion_width, insertion_height, 7 / scale, 4 / scale);

  cursor_brush = get_cursor(brush_bits, brush_mask_bits, brush_width, brush_height, 4 / scale, 28 / scale);

  cursor_crosshair = get_cursor(crosshair_bits, crosshair_mask_bits,
                                crosshair_width, crosshair_height, 15 / scale, 15 / scale);

  cursor_rotate = get_cursor(rotate_bits, rotate_mask_bits, rotate_width, rotate_height, 15 / scale, 15 / scale);

  cursor_up = get_cursor(up_bits, up_mask_bits, up_width, up_height, 15 / scale, 1 / scale);

  cursor_down = get_cursor(down_bits, down_mask_bits, down_width, down_height, 15 / scale, 30 / scale);

  cursor_tiny = get_cursor(tiny_bits, tiny_mask_bits, tiny_width, tiny_height, 3, 3);   /* Exactly the same in SMALL (16x16) size! */



  /* Create drawing canvas: */

  canvas = SDL_CreateRGBSurface(screen->flags,
                                WINDOW_WIDTH - (96 * 2),
                                (48 * 7) + 40 + HEIGHTOFFSET,
                                screen->format->BitsPerPixel,
                                screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, 0);

  save_canvas = SDL_CreateRGBSurface(screen->flags,
                                     WINDOW_WIDTH - (96 * 2),
                                     (48 * 7) + 40 + HEIGHTOFFSET,
                                     screen->format->BitsPerPixel,
                                     screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, 0);


  img_starter = NULL;
  img_starter_bkgd = NULL;
  starter_mirrored = 0;
  starter_flipped = 0;
  starter_personal = 0;
  starter_modified = 0;

  if (canvas == NULL)
    {
      fprintf(stderr, "\nError: Can't build drawing canvas!\n"
              "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

      cleanup();
      exit(1);
    }

  touched = (Uint8 *) malloc(sizeof(Uint8) * (canvas->w * canvas->h));
  if (touched == NULL)
    {
      fprintf(stderr, "\nError: Can't build drawing touch mask!\n");

      cleanup();
      exit(1);
    }

  canvas_color_r = 255;
  canvas_color_g = 255;
  canvas_color_b = 255;

  SDL_FillRect(canvas, NULL, SDL_MapRGB(canvas->format, 255, 255, 255));

  /* Creating the label surface: */

  label = SDL_CreateRGBSurface(screen->flags,
                               WINDOW_WIDTH - (96 * 2),
                               (48 * 7) + 40 + HEIGHTOFFSET,
                               screen->format->BitsPerPixel,
                               screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, TPAINT_AMASK);

  /* making the label layer transparent */
  SDL_FillRect(label, NULL, SDL_MapRGBA(label->format, 0, 0, 0, 0));

  /* Create undo buffer space: */

  for (i = 0; i < NUM_UNDO_BUFS; i++)
    {
      undo_bufs[i] = SDL_CreateRGBSurface(screen->flags,
                                          WINDOW_WIDTH - (96 * 2),
                                          (48 * 7) + 40 + HEIGHTOFFSET,
                                          screen->format->BitsPerPixel,
                                          screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, 0);


      if (undo_bufs[i] == NULL)
        {
          fprintf(stderr, "\nError: Can't build undo buffer! (%d of %d)\n"
                  "The Simple DirectMedia Layer error that occurred was:\n"
                  "%s\n\n", i + 1, NUM_UNDO_BUFS, SDL_GetError());

          cleanup();
          exit(1);
        }

      undo_starters[i] = UNDO_STARTER_NONE;
    }



  /* Load other images: */

  for (i = 0; i < NUM_TOOLS; i++)
    img_tools[i] = loadimage(tool_img_fnames[i]);

  img_title_on = loadimage(DATA_PREFIX "images/ui/title.png");
  img_title_large_on = loadimage(DATA_PREFIX "images/ui/title_large.png");
  img_title_off = loadimage(DATA_PREFIX "images/ui/no_title.png");
  img_title_large_off = loadimage(DATA_PREFIX "images/ui/no_title_large.png");

  img_btn_up = loadimage(DATA_PREFIX "images/ui/btn_up.png");
  img_btn_down = loadimage(DATA_PREFIX "images/ui/btn_down.png");
  img_btn_off = loadimage(DATA_PREFIX "images/ui/btn_off.png");

  img_btnsm_up = loadimage(DATA_PREFIX "images/ui/btnsm_up.png");
  img_btnsm_off = loadimage(DATA_PREFIX "images/ui/btnsm_off.png");
  img_btnsm_down = loadimage(DATA_PREFIX "images/ui/btnsm_down.png");
  img_btnsm_hold = loadimage(DATA_PREFIX "images/ui/btnsm_hold.png");

  img_btn_nav = loadimage(DATA_PREFIX "images/ui/btn_nav.png");
  img_btnsm_nav = loadimage(DATA_PREFIX "images/ui/btnsm_nav.png");

  img_sfx = loadimage(DATA_PREFIX "images/tools/sfx.png");
  img_speak = loadimage(DATA_PREFIX "images/tools/speak.png");

  img_black = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                   img_btn_off->w, img_btn_off->h,
                                   img_btn_off->format->BitsPerPixel,
                                   img_btn_off->format->Rmask,
                                   img_btn_off->format->Gmask, img_btn_off->format->Bmask, img_btn_off->format->Amask);
  SDL_SetSurfaceAlphaMod(img_black, SDL_ALPHA_TRANSPARENT);

  SDL_FillRect(img_black, NULL, SDL_MapRGBA(screen->format, 0, 0, 0, 255));

  img_grey = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                  img_btn_off->w, img_btn_off->h,
                                  img_btn_off->format->BitsPerPixel,
                                  img_btn_off->format->Rmask,
                                  img_btn_off->format->Gmask, img_btn_off->format->Bmask, img_btn_off->format->Amask);
  SDL_SetSurfaceAlphaMod(img_grey, SDL_ALPHA_TRANSPARENT);

  SDL_FillRect(img_grey, NULL, SDL_MapRGBA(screen->format, 0x88, 0x88, 0x88, 255));

  show_progress_bar(screen);

  img_yes = loadimage(DATA_PREFIX "images/ui/yes.png");
  img_no = loadimage(DATA_PREFIX "images/ui/no.png");

  img_prev = loadimage(DATA_PREFIX "images/ui/prev.png");
  img_next = loadimage(DATA_PREFIX "images/ui/next.png");

  img_mirror = loadimage(DATA_PREFIX "images/ui/mirror.png");
  img_flip = loadimage(DATA_PREFIX "images/ui/flip.png");

  img_open = loadimage(DATA_PREFIX "images/ui/open.png");
  img_erase = loadimage(DATA_PREFIX "images/ui/erase.png");
  img_back = loadimage(DATA_PREFIX "images/ui/back.png");
  img_trash = loadimage(DATA_PREFIX "images/ui/trash.png");

  img_slideshow = loadimage(DATA_PREFIX "images/ui/slideshow.png");
  img_play = loadimage(DATA_PREFIX "images/ui/play.png");
  img_select_digits = loadimage(DATA_PREFIX "images/ui/select_digits.png");

  img_popup_arrow = loadimage(DATA_PREFIX "images/ui/popup_arrow.png");

  img_dead40x40 = loadimage(DATA_PREFIX "images/ui/dead40x40.png");

  img_printer = loadimage(DATA_PREFIX "images/ui/printer.png");
  img_printer_wait = loadimage(DATA_PREFIX "images/ui/printer_wait.png");

  img_save_over = loadimage(DATA_PREFIX "images/ui/save_over.png");

  img_grow = loadimage(DATA_PREFIX "images/ui/grow.png");
  img_shrink = loadimage(DATA_PREFIX "images/ui/shrink.png");

  img_magic_paint = loadimage(DATA_PREFIX "images/ui/magic_paint.png");
  img_magic_fullscreen = loadimage(DATA_PREFIX "images/ui/magic_fullscreen.png");

  img_bold = loadimage(DATA_PREFIX "images/ui/bold.png");
  img_italic = loadimage(DATA_PREFIX "images/ui/italic.png");

  img_label = loadimage(DATA_PREFIX "images/tools/label.png");
  img_label_select = loadimage(DATA_PREFIX "images/tools/label_select.png");

  show_progress_bar(screen);

  tmp_imgcurup = loadimage(DATA_PREFIX "images/ui/cursor_up_large.png");
  tmp_imgcurdown = loadimage(DATA_PREFIX "images/ui/cursor_down_large.png");
  img_cursor_up = thumbnail(tmp_imgcurup, THUMB_W, THUMB_H, 0);
  img_cursor_down = thumbnail(tmp_imgcurdown, THUMB_W, THUMB_H, 0);

  tmp_imgcurup = loadimage(DATA_PREFIX "images/ui/cursor_starter_up.png");
  tmp_imgcurdown = loadimage(DATA_PREFIX "images/ui/cursor_starter_down.png");
  img_cursor_starter_up = thumbnail(tmp_imgcurup, THUMB_W, THUMB_H, 0);
  img_cursor_starter_down = thumbnail(tmp_imgcurdown, THUMB_W, THUMB_H, 0);
  SDL_FreeSurface(tmp_imgcurup);
  SDL_FreeSurface(tmp_imgcurdown);

  show_progress_bar(screen);

  img_scroll_up = loadimage(DATA_PREFIX "images/ui/scroll_up.png");
  img_scroll_down = loadimage(DATA_PREFIX "images/ui/scroll_down.png");

  img_scroll_up_off = loadimage(DATA_PREFIX "images/ui/scroll_up_off.png");
  img_scroll_down_off = loadimage(DATA_PREFIX "images/ui/scroll_down_off.png");
  img_color_sel = loadimage(DATA_PREFIX "images/ui/csel.png");

#ifdef LOW_QUALITY_COLOR_SELECTOR
  img_paintcan = loadimage(DATA_PREFIX "images/ui/paintcan.png");
#endif

  if (onscreen_keyboard)
    {
      img_oskdel = loadimage(DATA_PREFIX "images/ui/osk_delete.png");
      img_osktab = loadimage(DATA_PREFIX "images/ui/osk_tab.png");
      img_oskenter = loadimage(DATA_PREFIX "images/ui/osk_enter.png");
      img_oskcapslock = loadimage(DATA_PREFIX "images/ui/osk_capslock.png");
      img_oskshift = loadimage(DATA_PREFIX "images/ui/osk_shift.png");

      if (onscreen_keyboard_layout)
        {
          // use platform system onscreen keybord or tuxpaint onscreen keybord
          if (strcmp(onscreen_keyboard_layout, "SYSTEM") == 0)
            kbd = NULL;
          else
            kbd =
              osk_create(onscreen_keyboard_layout, screen, img_btnsm_up, img_btnsm_down, img_btnsm_off, img_btnsm_nav,
                         img_btnsm_hold, img_oskdel, img_osktab, img_oskenter, img_oskcapslock, img_oskshift,
                         onscreen_keyboard_disable_change);
        }
      else
        {
          kbd =
            osk_create(strdup("default.layout"), screen, img_btnsm_up, img_btnsm_down, img_btnsm_off, img_btnsm_nav,
                       img_btnsm_hold, img_oskdel, img_osktab, img_oskenter, img_oskcapslock, img_oskshift,
                       onscreen_keyboard_disable_change);
        }
    }

  show_progress_bar(screen);


  /* Load brushes: */
  load_brush_dir(screen, DATA_PREFIX "brushes");
  homedirdir = get_fname("brushes", DIR_DATA);
  load_brush_dir(screen, homedirdir);
#ifdef WIN32
  free(homedirdir);
  homedirdir = get_fname("data/brushes", DIR_DATA);
  load_brush_dir(screen, homedirdir);
#endif

  if (num_brushes == 0)
    {
      fprintf(stderr, "\nError: No brushes found in " DATA_PREFIX "brushes/\n" "or %s\n\n", homedirdir);
      cleanup();
      exit(1);
    }

  free(homedirdir);


  /* Load system fonts: */

  large_font = TuxPaint_Font_OpenFont(PANGO_DEFAULT_FONT,
                                      DATA_PREFIX "fonts/default_font.ttf", 30 - (only_uppercase * 3));

  if (large_font == NULL)
    {
      fprintf(stderr,
              "\nError: Can't load font file: "
              DATA_PREFIX "fonts/default_font.ttf\n"
              "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

      cleanup();
      exit(1);
    }


  small_font = TuxPaint_Font_OpenFont(PANGO_DEFAULT_FONT, DATA_PREFIX "fonts/default_font.ttf",
#ifdef __APPLE__
                                      12 - (only_uppercase * 2));
#else
                                      13 - (only_uppercase * 2));
#endif

  if (small_font == NULL)
    {
      fprintf(stderr,
              "\nError: Can't load font file: "
              DATA_PREFIX "fonts/default_font.ttf\n"
              "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

      cleanup();
      exit(1);
    }


#ifdef NO_SDLPANGO
  locale_font = load_locale_font(medium_font, 18);
#else
  locale_font = medium_font;
#endif


#if 0
  /* put elsewhere for THREADED_FONTS */
  /* Load user fonts, for the text tool */
  load_user_fonts();
#endif

  if (!dont_load_stamps)
    load_stamps(screen);


  /* Load magic tool plugins: */

  load_magic_plugins();

  show_progress_bar(screen);

  /* Load shape icons: */
  for (i = 0; i < NUM_SHAPES; i++)
    img_shapes[i] = loadimage(shape_img_fnames[i]);

  show_progress_bar(screen);

  /* Load tip tux images: */
  for (i = 0; i < NUM_TIP_TUX; i++)
    img_tux[i] = loadimage(tux_img_fnames[i]);

  show_progress_bar(screen);

  img_mouse = loadimage(DATA_PREFIX "images/ui/mouse.png");
  img_mouse_click = loadimage(DATA_PREFIX "images/ui/mouse_click.png");

  show_progress_bar(screen);

  img_color_picker = loadimage(DATA_PREFIX "images/ui/color_picker.png");

  /* Create toolbox and selector labels: */

  for (i = 0; i < NUM_TITLES; i++)
    {
      if (strlen(title_names[i]) > 0)
        {
          TuxPaint_Font *myfont = large_font;
          char *td_str = textdir(gettext(title_names[i]));

          if (need_own_font && strcmp(gettext(title_names[i]), title_names[i]))
            myfont = locale_font;
          upstr = uppercase(td_str);
          free(td_str);
          tmp_surf = render_text(myfont, upstr, black);
          free(upstr);
          img_title_names[i] = thumbnail(tmp_surf, min(84, tmp_surf->w), tmp_surf->h, 0);
          SDL_FreeSurface(tmp_surf);
        }
      else
        {
          img_title_names[i] = NULL;
        }
    }



  /* Generate color selection buttons: */

#ifndef LOW_QUALITY_COLOR_SELECTOR

  /* Create appropriately-shaped buttons: */
  img1 = loadimage(DATA_PREFIX "images/ui/paintwell.png");
  img_paintwell = thumbnail(img1, color_button_w, color_button_h, 0);
  tmp_btn_up = thumbnail(img_btn_up, color_button_w, color_button_h, 0);
  tmp_btn_down = thumbnail(img_btn_down, color_button_w, color_button_h, 0);
  img_color_btn_off = thumbnail(img_btn_off, color_button_w, color_button_h, 0);
  SDL_FreeSurface(img1);

  img_color_picker_thumb = thumbnail(img_color_picker, color_button_w, color_button_h, 0);

  /* Create surfaces to draw them into: */

  img_color_btns = malloc(sizeof(SDL_Surface *) * NUM_COLORS * 2);

  for (i = 0; i < NUM_COLORS * 2; i++)
    {
      img_color_btns[i] = SDL_CreateRGBSurface(screen->flags,
                                               /* (WINDOW_WIDTH - 96) / NUM_COLORS, 48, */
                                               tmp_btn_up->w, tmp_btn_up->h,
                                               screen->format->BitsPerPixel,
                                               screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, 0);

      if (img_color_btns[i] == NULL)
        {
          fprintf(stderr, "\nError: Can't build color button!\n"
                  "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

          cleanup();
          exit(1);
        }

      SDL_LockSurface(img_color_btns[i]);
    }


  /* Generate the buttons based on the thumbnails: */

  SDL_LockSurface(tmp_btn_down);
  SDL_LockSurface(tmp_btn_up);

  getpixel_tmp_btn_up = getpixels[tmp_btn_up->format->BytesPerPixel];
  getpixel_tmp_btn_down = getpixels[tmp_btn_down->format->BytesPerPixel];
  getpixel_img_paintwell = getpixels[img_paintwell->format->BytesPerPixel];


  for (y = 0; y < tmp_btn_up->h /* 48 */ ; y++)
    {
      for (x = 0; x < tmp_btn_up->w /* (WINDOW_WIDTH - 96) / NUM_COLORS */ ;
           x++)
        {
          double ru, gu, bu, rd, gd, bd, aa;
          Uint8 a;

          SDL_GetRGB(getpixel_tmp_btn_up(tmp_btn_up, x, y), tmp_btn_up->format, &r, &g, &b);

          ru = sRGB_to_linear_table[r];
          gu = sRGB_to_linear_table[g];
          bu = sRGB_to_linear_table[b];
          SDL_GetRGB(getpixel_tmp_btn_down(tmp_btn_down, x, y), tmp_btn_down->format, &r, &g, &b);
          rd = sRGB_to_linear_table[r];
          gd = sRGB_to_linear_table[g];
          bd = sRGB_to_linear_table[b];
          SDL_GetRGBA(getpixel_img_paintwell(img_paintwell, x, y), img_paintwell->format, &r, &g, &b, &a);
          aa = a / 255.0;

          for (i = 0; i < NUM_COLORS; i++)
            {
              double rh = sRGB_to_linear_table[color_hexes[i][0]];
              double gh = sRGB_to_linear_table[color_hexes[i][1]];
              double bh = sRGB_to_linear_table[color_hexes[i][2]];

              if (i == NUM_COLORS - 1)
                {
                  putpixels[img_color_btns[i]->format->BytesPerPixel]
                    (img_color_btns[i], x, y,
                     getpixels[img_color_picker_thumb->format->BytesPerPixel] (img_color_picker_thumb, x, y));
                  putpixels[img_color_btns[i + NUM_COLORS]->format->BytesPerPixel]
                    (img_color_btns[i + NUM_COLORS], x, y,
                     getpixels[img_color_picker_thumb->format->BytesPerPixel] (img_color_picker_thumb, x, y));
                }

              if (i < NUM_COLORS - 1 || a == 255)
                {
                  putpixels[img_color_btns[i]->format->BytesPerPixel]
                    (img_color_btns[i], x, y,
                     SDL_MapRGB(img_color_btns[i]->format,
                                linear_to_sRGB(rh * aa + ru * (1.0 - aa)),
                                linear_to_sRGB(gh * aa + gu * (1.0 - aa)), linear_to_sRGB(bh * aa + bu * (1.0 - aa))));
                  putpixels[img_color_btns[i + NUM_COLORS]->format->BytesPerPixel]
                    (img_color_btns[i + NUM_COLORS], x, y,
                     SDL_MapRGB(img_color_btns[i + NUM_COLORS]->format,
                                linear_to_sRGB(rh * aa + rd * (1.0 - aa)),
                                linear_to_sRGB(gh * aa + gd * (1.0 - aa)), linear_to_sRGB(bh * aa + bd * (1.0 - aa))));
                }
            }
        }
    }

  for (i = 0; i < NUM_COLORS * 2; i++)
    {
      SDL_UnlockSurface(img_color_btns[i]);
      if (i == NUM_COLORS - 2 || i == 2 * NUM_COLORS - 2)
        {
          dest.x = (img_color_btns[i]->w - img_color_sel->w) / 2;
          dest.y = (img_color_btns[i]->h - img_color_sel->h) / 2;
          dest.w = img_color_sel->w;
          dest.h = img_color_sel->h;
          SDL_BlitSurface(img_color_sel, NULL, img_color_btns[i], &dest);
        }
    }

  SDL_UnlockSurface(tmp_btn_up);
  SDL_UnlockSurface(tmp_btn_down);
  SDL_FreeSurface(tmp_btn_up);
  SDL_FreeSurface(tmp_btn_down);

#endif

  create_button_labels();


  /* Seed random-number generator: */

  srand(SDL_GetTicks());


  /* Enable Unicode support in SDL: */

//  SDL_EnableUNICODE(1);

#ifndef _WIN32
  /* Set up signal handler for SIGPIPE (in case printer command dies;
     e.g., altprintcommand=kprinter, but 'Cancel' is clicked,
     instead of 'Ok') */

  signal(SIGPIPE, signal_handler);
#endif
}

/* ================================================================================== */

static void claim_to_be_ready(void)
{
  SDL_Rect dest;
  SDL_Rect src;
  int i;

  /* Let the user know we're (nearly) ready now */

  dest.x = 0;
  dest.y = WINDOW_HEIGHT - img_progress->h;
  dest.h = img_progress->h;
  dest.w = WINDOW_WIDTH;
  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));
  src.h = img_progress->h;
  src.w = img_title->w;
  src.x = 0;
  src.y = img_title->h - img_progress->h;
  dest.x = ((WINDOW_WIDTH - img_title->w - (img_title_tuxpaint->w / 2)) / 2) + (img_title_tuxpaint->w / 2) + 20;
  SDL_BlitSurface(img_title, &src, screen, &dest);

  SDL_FreeSurface(img_title);
  SDL_FreeSurface(img_title_credits);
  SDL_FreeSurface(img_title_tuxpaint);

  dest.x = 0;
  dest.w = WINDOW_WIDTH;        /* SDL mangles this! So, do repairs. */
  update_screen_rect(&dest);

  do_setcursor(cursor_arrow);
  playsound(screen, 0, SND_HARP, 1, SNDPOS_CENTER, SNDDIST_NEAR);
#if !defined (__ANDROID__)
  do_wait(50);                  /* about 5 seconds */
#endif

  /* Set defaults! */

  cur_undo = 0;
  oldest_undo = 0;
  newest_undo = 0;

  cur_tool = TOOL_BRUSH;
  cur_color = COLOR_BLACK;
  colors_are_selectable = 1;
  cur_brush = 0;
  for (i = 0; i < MAX_STAMP_GROUPS; i++)
    cur_stamp[i] = 0;
  cur_shape = SHAPE_SQUARE;
  cur_magic = 0;
  cur_font = 0;
  cur_eraser = 0;
  cur_label = LABEL_LABEL;
  cur_select = 0;
  cursor_left = -1;
  cursor_x = -1;
  cursor_y = -1;
  cursor_textwidth = 0;

  oldpos_x = WINDOW_WIDTH / 2;
  oldpos_y = WINDOW_HEIGHT / 2;

  SDL_WarpMouse(oldpos_x, oldpos_y);

  eraser_sound = 0;

  img_cur_brush = NULL;
  render_brush();

  brush_scroll = 0;
  for (i = 0; i < MAX_STAMP_GROUPS; i++)
    stamp_scroll[i] = 0;
  stamp_group = 0;              /* reset! */
  font_scroll = 0;
  magic_scroll = 0;
  tool_scroll = 0;

  reset_avail_tools();


  /* Load current image (if any): */

  if (start_blank == 0)
    load_current();


  /* Draw the screen! */

  SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 255, 255, 255));

  draw_toolbar();
  draw_colors(COLORSEL_FORCE_REDRAW);
  draw_brushes();
  update_canvas(0, 0, WINDOW_WIDTH - 96, (48 * 7) + 40 + HEIGHTOFFSET);

  SDL_Flip(screen);

  draw_tux_text(tool_tux[cur_tool], tool_tips[cur_tool], 1);
}

/* ================================================================================== */

int main(int argc, char *argv[])
{
#ifdef DEBUG
  CLOCK_TYPE time1;
  CLOCK_TYPE time2;
#endif

  (void)argc;

#ifdef DEBUG
  CLOCK_ASM(time1);
#endif

  /* do not add code (slowness) here unless required for scanning fonts */
  progname = argv[0];

#if defined(DEBUG)
#if defined(__APPLE__)
  /* EP added block to log messages */
  freopen("/tmp/tuxpaint.log", "w", stdout);    /* redirect stdout to a file */
#elif defined (__ANDROID__)
  freopen("/mnt/sdcard/tuxpaint/tuxpaint.log", "w", stdout);    /* redirect stdout to a file */
#endif

  dup2(fileno(stdout), fileno(stderr)); /* redirect stderr to stdout */
  setvbuf(stdout, NULL, _IONBF, 0);     /* we don't want buffering to avoid de-sync'ing stdout and stderr */
  setvbuf(stderr, NULL, _IONBF, 0);     /* we don't want buffering to avoid de-sync'ing stdout and stderr */
  char logTime[100];
  time_t t = time(NULL);

  strftime(logTime, sizeof(logTime), "%A %d/%m/%Y %H:%M:%S", localtime(&t));
  printf("Tux Paint log - %s\n", logTime);
#endif

  chdir_to_binary(argv[0]);
  setup_config(argv);

#ifdef DEBUG
  CLOCK_ASM(time2);
#endif

#if defined(__APPLE__)
  /* Pango uses Fontconfig which requires /opt/local/etc/fonts/fonts.conf. This
   * file may not exist on the runtime system, however, so we copy the file
   * into our app bundle at compile time, and tell Fontconfig here to look for
   * the file within the app bundle. */
  putenv((char *)"FONTCONFIG_PATH=Resources/etc");
#endif

#ifdef FORKED_FONTS
  /* must start ASAP, but depends on locale which in turn needs the config */
#ifdef NO_SDLPANGO
  /* Only fork it now if we're not planning on creating a thread to handle fontconfig stuff -bjk 2010.04.27 */
#ifdef DEBUG
  printf("Running font scanner\n");
  fflush(stdout);
#endif
  run_font_scanner(screen, texture, renderer, lang_prefixes[get_current_language()]);
#else
#ifdef DEBUG
  printf("NOT running font scanner\n");
  fflush(stdout);
#endif
#endif
#endif

  /* Warnings to satisfy SF.net Bug #3327493 -bjk 2011.06.24 */
  if (disable_save && autosave_on_quit)
    {
      fprintf(stderr, "Warning: Autosave requested, but saving is disabled.\n");
    }
  if (disable_save && (promptless_save != SAVE_OVER_UNSET))
    {
      fprintf(stderr, "Warning: Save-over option specified, but saving is disabled.\n");
    }

  if (promptless_save == SAVE_OVER_UNSET)
    {
      promptless_save = SAVE_OVER_PROMPT;
    }

  /* Set up! */
  setup();

#ifdef DEBUG
  printf("Seconds in early start-up: %.3f\n", (double)(time2 - time1) / CLOCK_SPEED);
  printf("Seconds in late start-up:  %.3f\n", (double)(time2 - time1) / CLOCK_SPEED);
#endif



  claim_to_be_ready();

  mainloop();

  /* Close and quit! */
  save_current();
  wait_for_sfx();
  cleanup();
  return 0;
}




/* Moves a file to the trashcan (or deletes it) */

static int trash(char *path)
{
#ifdef UNLINK_ONLY
  return (unlink(path));
#else
  char fname[MAX_PATH], trashpath[MAX_PATH], dest[MAX_PATH], infoname[MAX_PATH], bname[MAX_PATH], ext[MAX_PATH];
  char deldate[32];
  struct tm tim;
  time_t now;
  int cnt, tmp;
  FILE *fi, *fo;
  unsigned char buf[1024];
  size_t len;

  debug(path);


  /* FIXME: This is Freedesktop.org-centric -bjk 2011.04.16 */

  if (basename(path) == NULL)
    {
      debug("Can't get basename! Deleting instead.");
      return (unlink(path));
    }

  printf("trash: basename=%s", basename(path)); /* EP */
  strcpy(fname, basename(path));

  if (!file_exists(path))
    {
      debug("Does't exist anyway, so skipping");
      return (1);
    }


  /* Move file into Trash folder */

  if (getenv("XDG_DATA_HOME") != NULL)
    {
      sprintf(trashpath, "%s/Trash", getenv("XDG_DATA_HOME"));
    }
  else if (getenv("HOME") != NULL)
    {
      sprintf(trashpath, "%s/.local/share/Trash", getenv("HOME"));
    }
  else
    {
      debug("Can't move to trash! Deleting instead.");
      return (unlink(path));
    }

  mkdir(trashpath, 0x777);
  sprintf(dest, "%s/files", trashpath);
  mkdir(dest, 0x777);
  sprintf(dest, "%s/info", trashpath);
  mkdir(dest, 0x777);

  sprintf(dest, "%s/files/%s", trashpath, fname);

  strcpy(bname, fname);
  if (strstr(bname, ".") != NULL)
    {
      strcpy(strstr(bname, "."), "\0");
      strcpy(ext, strstr(fname, ".") + 1);
    }
  else
    {
      debug("Filename format unfamiliar! Deleting instead.");
      return (unlink(path));
    }

  sprintf(infoname, "%s/info/%s.trashinfo", trashpath, fname);

  cnt = 1;
  while (file_exists(dest) && cnt < 100)
    {
      sprintf(fname, "%s_%d.%s", bname, cnt, ext);

      sprintf(dest, "%s/files/%s", trashpath, fname);
      sprintf(infoname, "%s/info/%s.trashinfo", trashpath, fname);
      cnt++;
    }

  if (cnt >= 100)
    {
      debug("Too many identically-named files! Deleting instead.");
      return (unlink(path));
    }

  debug(dest);

  if (rename(path, dest) == -1)
    {
      debug("Could not move to trash. Trying to copy, instead.");

      fi = fopen(path, "r");
      if (fi == NULL)
        {
          debug("Could not open source file for copy. Deleting instead.");
          return (unlink(path));
        }
      fo = fopen(dest, "w");
      if (fo == NULL)
        {
          debug("Could not open dest. file for copy. Deleting instead.");
          fclose(fi);
          return (unlink(path));
        }
      while (!feof(fi))
        {
          len = fread(buf, sizeof(buf), 1, fi);
          if (len > 0)
            {
              fwrite(buf, sizeof(buf), 1, fo);
            }
        }
      fclose(fi);
      fclose(fo);

      unlink(path);
    }

  /* Create info file */
  fo = fopen(infoname, "w");
  if (fo == NULL)
    {
      debug("Error: Couldn't create info file!");
      return (1);
    }

  now = time(NULL);
  tim = *(localtime(&now));
  strftime(deldate, sizeof(deldate), "%FT%T", &tim);

  fprintf(fo, "[Trash Info]\n");
  fprintf(fo, "Path=%s\n", path);
  fprintf(fo, "DeletionDate=%s\n", deldate);
  fclose(fo);


  /* Now we can alert the desktop GUI(s) running that something has been
     placed into the trash! */

  /* Tell KDE 4.x (e.g., Trash icon on your panel) that trash has been affected.
     Per dfaure (David Faure) and thiago (Thiago Macieria)
     on #kde-devel 2011.04.18
     -bjk 2011.04.18 */

  /* FIXME: Is this sufficient to find 'dbus-send' (rely on system to use $PATH?) -bjk 2011.04.18 */
  tmp = system("dbus-send / org.kde.KDirNotify.FilesAdded string:trash:/");
  (void)tmp;


  /* Note: GNOME figures out when things change because it asks the Kernel
     to tell it.
     Per cosimoc (Cosimo Cecchi) on #nautilus 2011.04.18
     -bjk 2011.04.18 */

  /* FIXME: xcfe and elsewhere: Anything to do? */

  /* FIXME: Windows */

  /* FIXME: Mac OS X */

  /* FIXME: Haiku */

  return (0);
#endif /* UNLINK_ONLY */
}

int file_exists(char *path)
{
  struct stat buf;
  int res;

  res = stat(path, &buf);
  return (res == 0);
}

/* Don't move the mouse here as this is only called when an event triggers it
   and the joystick can be holded withouth sending any event. */
static void handle_joyaxismotion(SDL_Event event, int *motioner, int *val_x, int *val_y)
{
  int i, j, step;

  if (event.jaxis.which != 0)
    return;

  i = SDL_JoystickGetAxis(joystick, 0);
  j = SDL_JoystickGetAxis(joystick, 1);
  step = 5000;
  if (abs(i) < joystick_low_threshold && abs(j) < joystick_low_threshold)
    {
      *motioner = FALSE;
      *val_x = 0;
      *val_y = 0;
    }
  else
    {
      if (i > joystick_low_threshold)
        *val_x = min((i - joystick_low_threshold) / step + 1, joystick_maxsteps);
      else if (i < -joystick_low_threshold)
        *val_x = max((i + joystick_low_threshold) / step - 1, -joystick_maxsteps);
      else
        *val_x = 0;

      if (j > joystick_low_threshold)
        *val_y = min((j - joystick_low_threshold) / step + 1, joystick_maxsteps);
      else if (j < -joystick_low_threshold)
        *val_y = max((j + joystick_low_threshold) / step - 1, -joystick_maxsteps);
      else
        *val_y = 0;

      if (*val_x || *val_y)
        {
          *motioner = TRUE;
        }
      else
        *motioner = FALSE;
    }
}

static void handle_joyhatmotion(SDL_Event event, int oldpos_x, int oldpos_y, int *valhat_x, int *valhat_y,
                                int *hatmotioner, Uint32 * old_hat_ticks)
{
  *hatmotioner = 1;

  switch (event.jhat.value)
    {
    case SDL_HAT_CENTERED:
      *valhat_x = 0;
      *valhat_y = 0;
      *hatmotioner = 0;
      break;
    case SDL_HAT_UP:
      *valhat_x = 0;
      *valhat_y = -1;
      break;
    case SDL_HAT_RIGHTUP:
      *valhat_x = 1;
      *valhat_y = -1;
      break;
    case SDL_HAT_RIGHT:
      *valhat_x = 1;
      *valhat_y = 0;
      break;
    case SDL_HAT_RIGHTDOWN:
      *valhat_x = 1;
      *valhat_y = 1;
      break;
    case SDL_HAT_DOWN:
      *valhat_x = 0;
      *valhat_y = 1;
      break;
    case SDL_HAT_LEFTDOWN:
      *valhat_x = -1;
      *valhat_y = 1;
      break;
    case SDL_HAT_LEFT:
      *valhat_x = -1;
      *valhat_y = 0;
      break;
    case SDL_HAT_LEFTUP:
      *valhat_x = -1;
      *valhat_y = -1;
      break;
    }
  if (*valhat_x || *valhat_y)
    SDL_WarpMouse(oldpos_x + *valhat_x, oldpos_y + *valhat_y);

  *old_hat_ticks = SDL_GetTicks();
}

static void handle_joyballmotion(SDL_Event event, int oldpos_x, int oldpos_y)
{
  int val_x, val_y;

  /* FIXME: NOT TESTED Should this act like handle_joyaxismotion?
     in the sense of setting the values for the moving but don't move the mouse here? */
  /* printf("\n ball movement \n"); */
  val_x = event.jball.xrel;
  val_y = event.jball.yrel;
  SDL_WarpMouse(oldpos_x + val_x, oldpos_y + val_y);
}


static void handle_motioners(int oldpos_x, int oldpos_y, int motioner, int hatmotioner, int old_hat_ticks, int val_x,
                             int val_y, int valhat_x, int valhat_y)
{
  int vx, vy;
  Uint32 ticks;

  ticks = SDL_GetTicks();
  vx = vy = 0;

  vx = oldpos_x + val_x;
  vy = oldpos_y + val_y;


  if (ticks - old_hat_ticks > joystick_hat_timeout)
    {
      vx += valhat_x;
      vy += valhat_y;
    }
  SDL_WarpMouse(vx, vy);

  if (motioner && joystick_slowness)
    SDL_Delay(joystick_slowness);

  if (hatmotioner && joystick_hat_slowness)
    SDL_Delay(joystick_hat_slowness);

}

static void handle_joybuttonupdown(SDL_Event event, int oldpos_x, int oldpos_y)
{
  handle_joybuttonupdownscl(event, oldpos_x, oldpos_y, r_tools);
}

static void handle_joybuttonupdownscl(SDL_Event event, int oldpos_x, int oldpos_y, SDL_Rect real_r_tools)
{
  int i, ignore = 0;
  int eby, ts;
  SDL_Event ev;

  ev.button.x = oldpos_x;
  ev.button.y = oldpos_y;
  ev.button.button = SDL_BUTTON_LEFT;
  ev.button.type = SDL_MOUSEBUTTONDOWN;
  ev.button.state = SDL_PRESSED;

  if (event.type == SDL_JOYBUTTONDOWN)
    {
      /* First the actions that can be reached via keyboard shortcurts. */
      /* Escape is usefull to dismiss dialogs */
      if (event.button.button == joystick_button_escape)
        {
          ev.type = SDL_KEYDOWN;
          ev.key.keysym.sym = SDLK_ESCAPE;
          ev.key.keysym.mod = KMOD_CTRL;
        }
      else if (event.button.button == joystick_button_pagesetup)
        {
          ev.type = SDL_KEYDOWN;
          ev.key.keysym.sym = SDLK_p;
          ev.key.keysym.mod = KMOD_CTRL | KMOD_SHIFT;
        }

      /* Those could be reached too via clicks on the buttons. */
      else if (event.button.button == joystick_button_undo)
        {
          ev.type = SDL_KEYDOWN;
          ev.key.keysym.sym = SDLK_z;
          ev.key.keysym.mod = KMOD_CTRL;
        }
      else if (event.button.button == joystick_button_redo)
        {
          ev.type = SDL_KEYDOWN;
          ev.key.keysym.sym = SDLK_r;
          ev.key.keysym.mod = KMOD_CTRL;
        }
      else if (event.button.button == joystick_button_open)
        {
          ev.type = SDL_KEYDOWN;
          ev.key.keysym.sym = SDLK_o;
          ev.key.keysym.mod = KMOD_CTRL;
        }
      else if (event.button.button == joystick_button_new)
        {
          ev.type = SDL_KEYDOWN;
          ev.key.keysym.sym = SDLK_n;
          ev.key.keysym.mod = KMOD_CTRL;
        }
      else if (event.button.button == joystick_button_save)
        {
          ev.type = SDL_KEYDOWN;
          ev.key.keysym.sym = SDLK_s;
          ev.key.keysym.mod = KMOD_CTRL;
        }
      else if (event.button.button == joystick_button_print)
        {
          ev.type = SDL_KEYDOWN;
          ev.key.keysym.sym = SDLK_p;
          ev.key.keysym.mod = KMOD_CTRL;
        }


      /* Now the clicks on the tool buttons. */
      /* Note that at small window sizes there are scroll buttons in the tools rectangle */
      /* and some tools are hiden. */
      /* As any click outside of real_r_tools will not select the desired tool, */
      /* the workaround I came up with is to click on the scroll buttons to reveal the button, */
      /* then click on it. */
      else if (event.button.button == joystick_button_selectbrushtool ||
               event.button.button == joystick_button_selectstamptool ||
               event.button.button == joystick_button_selectlinestool ||
               event.button.button == joystick_button_selectshapestool ||
               event.button.button == joystick_button_selecttexttool ||
               event.button.button == joystick_button_selectlabeltool ||
               event.button.button == joystick_button_selectmagictool ||
               event.button.button == joystick_button_selecterasertool)

        {
          if (event.button.button == joystick_button_selectbrushtool)
            {
              ev.button.x = (TOOL_BRUSH % 2) * button_w + button_w / 2;
              ev.button.y = real_r_tools.y + TOOL_BRUSH / 2 * button_h + button_h / 2;
            }

          else if (event.button.button == joystick_button_selectstamptool)
            {
              ev.button.x = (TOOL_STAMP % 2) * button_w + button_w / 2;
              ev.button.y = real_r_tools.y + TOOL_STAMP / 2 * button_h + button_h / 2;
            }

          else if (event.button.button == joystick_button_selectlinestool)
            {
              ev.button.x = (TOOL_LINES % 2) * button_w + button_w / 2;
              ev.button.y = real_r_tools.y + TOOL_LINES / 2 * button_h + button_h / 2;
            }

          else if (event.button.button == joystick_button_selectshapestool)
            {
              ev.button.x = (TOOL_SHAPES % 2) * button_w + button_w / 2;
              ev.button.y = real_r_tools.y + TOOL_SHAPES / 2 * button_h + button_h / 2;
            }

          else if (event.button.button == joystick_button_selecttexttool)
            {
              ev.button.x = (TOOL_TEXT % 2) * button_w + button_w / 2;
              ev.button.y = real_r_tools.y + TOOL_TEXT / 2 * button_h + button_h / 2;
            }

          else if (event.button.button == joystick_button_selectlabeltool)
            {
              ev.button.x = (TOOL_LABEL % 2) * button_w + button_w / 2;
              ev.button.y = real_r_tools.y + TOOL_LABEL / 2 * button_h + button_h / 2;
            }

          else if (event.button.button == joystick_button_selectmagictool)
            {
              ev.button.x = (TOOL_MAGIC % 2) * button_w + button_w / 2;
              ev.button.y = real_r_tools.y + TOOL_MAGIC / 2 * button_h + button_h / 2;
            }

          else if (event.button.button == joystick_button_selecterasertool)
            {
              ev.button.x = (TOOL_ERASER % 2) * button_w + button_w / 2;
              ev.button.y = real_r_tools.y + TOOL_ERASER / 2 * button_h + button_h / 2;
            }

          /* Deal with scroll to reveal the button that should be clicked */
          eby = ev.button.y;
          ts = tool_scroll;

          while (eby < real_r_tools.y + ts / 2 * button_h)
            {
              ev.button.y = real_r_tools.y - 1;
              SDL_PushEvent(&ev);
              ts -= 2;
            }

          /* We don't need this ATM, but better left it ready in case the number of tools grows enough */
          while (eby > real_r_tools.y + real_r_tools.h + ts / 2 * button_h)
            {
              ev.button.y = real_r_tools.y + real_r_tools.h + 1;
              SDL_PushEvent(&ev);
              ts += 2;
            }

          ev.button.y = eby - ts / 2 * button_h;
        }
    }
  else
    {
      ev.button.type = SDL_MOUSEBUTTONUP;
      ev.button.state = SDL_RELEASED;
    }

#ifdef DEBUG
  printf("result %d %d\n", ev.button.x, ev.button.y);
#endif

  /* See if it's a button we ignore */

  for (i = 0; i < joystick_buttons_ignore_len && !ignore; i++)
    {
      if (event.button.button == joystick_buttons_ignore[i])
        {
          ignore = 1;
        }
    }

  if (!ignore)
    SDL_PushEvent(&ev);
}

/*
 * When moving on some cases of supporting scroll wheel, including
 * the coming SDL_MOUSEMOTION events will be converted to SDL_MOUSEWHEEL
 *
 * Currently motion_dx is not used, since only scroll up and down is support.
 */
int motion_convert;
int motion_dx, motion_dy;

static void start_motion_convert(SDL_Event event)
{
  if (event.type != SDL_MOUSEBUTTONDOWN)
    return;

  int scroll = 0;

  if (HIT(r_tools))
    scroll = 1;
  else if (HIT(r_toolopt) && (cur_tool == TOOL_BRUSH || cur_tool == TOOL_STAMP ||
                              cur_tool == TOOL_LINES || cur_tool == TOOL_SHAPES || cur_tool == TOOL_TEXT ||
                              cur_tool == TOOL_LABEL || cur_tool == TOOL_MAGIC))
    scroll = 1;
  else if ((cur_tool == TOOL_OPEN) && HIT(r_canvas))
    scroll = 1;
  else if ((cur_tool == TOOL_NEW) && HIT(r_canvas))
    scroll = 1;

  if (scroll != 1)
    return;

  motion_convert = 1;
  motion_dx = motion_dy = 0;
}

#ifdef __ANDROID__
static void stop_motion_convert(SDL_Event event)
{
  if (event.type != SDL_MOUSEBUTTONUP)
    return;

  motion_convert = 0;
  motion_dx = motion_dy = 0;
}

static void convert_motion_to_wheel(SDL_Event event)
{
  if (event.type != SDL_MOUSEMOTION)
    return;

  if (motion_convert == 0)
    return;

  int scroll = 0;
  int high = 0;

  if (HIT(r_tools))
    {
      scroll = 1;
      high = 48;
    }
  else if (HIT(r_toolopt) && (cur_tool == TOOL_BRUSH || cur_tool == TOOL_STAMP ||
                              cur_tool == TOOL_LINES || cur_tool == TOOL_SHAPES || cur_tool == TOOL_TEXT ||
                              cur_tool == TOOL_LABEL || cur_tool == TOOL_MAGIC))
    {
      scroll = 1;
      high = 48;
    }
  else if ((cur_tool == TOOL_OPEN) && HIT(r_canvas))
    {
      scroll = 1;
      high = THUMB_H;
    }
  else if ((cur_tool == TOOL_NEW) && HIT(r_canvas))
    {
      scroll = 1;
      high = THUMB_H;
    }

  if (scroll != 1)
    return;

  motion_dx += event.button.x - oldpos_x;
  motion_dy += event.button.y - oldpos_y;

  if (motion_dy > 0)
    {
      while (motion_dy - high > 0)
        {
          SDL_SendMouseWheel(NULL, event.motion.which, 0, 1);
          motion_dy -= high;
        }
    }
  else
    {
      while (motion_dy + high < 0)
        {
          SDL_SendMouseWheel(NULL, event.motion.which, 0, -1);
          motion_dy += high;
        }
    }
}
#endif
