/*
  tuxpaint.c

  Tux Paint - A simple drawing program for children.

  Copyright (c) 2002-2025
  by various contributors; see AUTHORS.txt
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

  June 14, 2002 - April 19, 2025
*/

#include "platform.h"

/* (Note: VER_VERSION and VER_DATE are now handled by Makefile) */


/* FIXME: */

/* Use this in places where we can only (or only want to, for whatever reason)
   use 'unlink()' to delete files, rather than trying to put them in the
   desktop enivronment's "trash" -bjk 2011.04.18 */
/* #define UNLINK_ONLY */

/* Color depth for Tux Paint to run in, and store canvases in: */

/* *INDENT-OFF* */

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
    #define VIDEO_BPP 32      /* might be fastest, if conversion funcs removed */
#endif

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

/* *INDENT-ON* */

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

enum
{
  STARTER_TEMPLATE_SCALE_MODE_NONE,     /* smear or apply background color */
  STARTER_TEMPLATE_SCALE_MODE_HORIZ,    /* allow zooming in (cropping left/right) if image is wider than the canvas */
  STARTER_TEMPLATE_SCALE_MODE_VERT,     /* allow zooming in (cropping top/bottom) if image is taller than the canvas */
  STARTER_TEMPLATE_SCALE_MODE_BOTH      /* allow zooming in (cropping anything) if canvas is smaller in either/both dimensions */
};

enum
{
  STARTER_TEMPLATE_GRAVITY_HORIZ_CENTER,
  STARTER_TEMPLATE_GRAVITY_HORIZ_LEFT,
  STARTER_TEMPLATE_GRAVITY_HORIZ_RIGHT
};

enum
{
  STARTER_TEMPLATE_GRAVITY_VERT_CENTER,
  STARTER_TEMPLATE_GRAVITY_VERT_TOP,
  STARTER_TEMPLATE_GRAVITY_VERT_BOTTOM
};

typedef struct starter_template_options_s
{
  int scale_mode;
  int h_gravity;
  int v_gravity;
  int smear;
  int bkgd_color[3];
} starter_template_options_t;


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

char *strcasestr(const char *haystack, const char *needle);

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

#include <wchar.h>
#include <wctype.h>

#if defined __BEOS__ || defined __HAIKU__ || defined __APPLE__ || defined __ANDROID__
#include <stdbool.h>
#ifndef __HAIKU__
#define FALSE false
#define TRUE true
#endif
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

#if defined __BEOS__ || defined __HAIKU__

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

extern status_t haiku_trash(const char *f);

#define dirent safer_dirent

#else /* __BEOS__ */

/* Not BeOS */

#if defined(__MACOS__)

/* macOS */

#include "macos_print.h"

#elif defined(__IOS__)

/* iOS */

#include "ios_print.h"

#else /* __MACOS__, __IOS__ */

/* Not Windows, not BeOS, not macOS, not iOS */
#ifdef __ANDROID__

#define AUTOSAVE_GOING_BACKGROUND
#include "android_print.h"
#include "android_assets.h"
int entered_background = 0;

#else

/* Not Windows, not BeOS, not Apple, not Android*/

#include "postscript_print.h"

#endif /* __ANDROID__ */

#endif /* __MACOS__, __IOS__ */

#endif /* __BEOS__ */

#else /* WIN32 */

/* Windows */

#include <windows.h>
#include <unistd.h>
#include <dirent.h>
#include <malloc.h>
#include "win32_print.h"
#include <io.h>
#include <direct.h>

#undef min
#undef max
#define mkdir(path,access)    _mkdir(path)

#define mbstowcs(wtok, tok, size) MultiByteToWideChar(CP_UTF8,0,tok,-1,wtok,size)
#define wcstombs(tok, wtok, size) WideCharToMultiByte(CP_UTF8,0,wtok,-1,tok,size,NULL,NULL)

extern int win32_trash(const char *path);
extern void win32_print_version(void);

#undef iswprint
int iswprint(wchar_t wc)
{
  WORD t;

  GetStringTypeW(CT_CTYPE1, &wc, 1, &t);
  return (t & C1_DEFINED) && !(t & C1_CNTRL);
}

#endif /* WIN32 */

#include <errno.h>
#include <sys/stat.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_thread.h"

#if defined(__MACOS__)
#include "macos.h"
#elif defined(__IOS__)
#include "ios.h"
#endif

#if !defined(_SDL_H) && !defined(SDL_h_)
#error "---------------------------------------------------"
#error "If you installed SDL from a package, be sure to get"
#error "the development package, as well!"
#error "(e.g., 'libsdl1.2-devel.rpm')"
#error "---------------------------------------------------"
#endif

#include "SDL2/SDL_image.h"

#if !defined(_SDL_IMAGE_H) && !defined(_IMG_h) && !defined(SDL_IMAGE_H_)
#error "---------------------------------------------------"
#error "If you installed SDL_image from a package, be sure"
#error "to get the development package, as well!"
#error "(e.g., 'libsdl-image1.2-devel.rpm')"
#error "---------------------------------------------------"
#endif

#include "SDL2/SDL_ttf.h"

#if !defined(_SDL_TTF_H) && !defined(_SDLttf_h) && !defined(SDL_TTF_H_)
#error "---------------------------------------------------"
#error "If you installed SDL_ttf from a package, be sure"
#error "to get the development package, as well!"
#error "(e.g., 'libsdl-ttf1.2-devel.rpm')"
#error "---------------------------------------------------"
#endif

#include "SDL2_rotozoom.h"
#if !defined(_SDL2_rotozoom_h)
#error "---------------------------------------------------"
#error "If you installed SDL_gfx from a package, be sure"
#error "to get the development package, as well!"
#error "(e.g., 'libsdl-gfx1.2-devel.rpm')"
#error "---------------------------------------------------"
#endif


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

#include <pango/pango.h>
#include <pango/pangoft2.h>

#ifndef NOSOUND

#include "SDL2/SDL_mixer.h"

#if !defined(_SDL_MIXER_H) && !defined(_MIXER_H_) && !defined(SDL_MIXER_H_)
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
#if defined (__MINGW32__) && (__GNUC__ <= 4 )
#include <librsvg/rsvg-cairo.h>
#endif

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
#include "libimagequant.h"

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
#include "gifenc.h"

#include "tools.h"
#include "titles.h"
#include "colors.h"
#include "shapes.h"
#include "sounds.h"
#include "tip_tux.h"
#include "great.h"

#include "fill.h"
#include "fill_tools.h"

#include "im.h"


#ifdef DEBUG_MALLOC
#include "malloc.c"
#endif

#include "parse.h"

#include "compiler.h"

char *tp_ui_font = NULL;

/* Convert floats to fractions between (min/max) and ((max-min)/max)
   (anything with smaller resolution will round up or down) */
#define SLOPPY_FRAC_MIN (float) 1
#define SLOPPY_FRAC_MAX (float) 10
static void sloppy_frac(float f, int *numer, int *denom);


static void select_label_node(int *old_x, int *old_y);
static void apply_label_node(int old_x, int old_y);
static void reposition_onscreen_keyboard(int y);


int calc_magic_control_rows(void);
int calc_stamp_control_rows(void);
void maybe_redraw_eraser_xor(void);

static void reset_stamps(int *stamp_xored_rt, int *stamp_place_x, int *stamp_place_y, int *stamp_tool_mode);

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

#if (SDL_MAJOR_VERSION < 2)
#define EVENT_FILTER_EVENT_TYPE const SDL_Event
#else
#define EVENT_FILTER_EVENT_TYPE union SDL_Event
#endif

int TP_EventFilter(void *data, EVENT_FILTER_EVENT_TYPE * event);


/* *INDENT-OFF* */
/* #define fmemopen_alternative *//* Uncomment this to test the fmemopen alternative in systems were fmemopen exists */
/* *INDENT-ON* */

#if defined (WIN32) || defined (__APPLE__) || defined(__NetBSD__) || defined(__sun) || defined(__OS2__) || defined(__ANDROID__) /* MINGW/MSYS, NetBSD, and MacOSX need it, at least for now */
#define fmemopen_alternative
#endif

#ifdef fmemopen_alternative
#undef fmemopen

FILE *my_fmemopen(unsigned char *data, size_t size, const char *mode);

/**
 * Open memory as a stream.  Some platforms do not support
 * fmemopen(), so this simply dumps data to a temp file,
 * then opens it back up and returns the FILE pointer.
 *
 * @param data Data to perform I/O on.
 * @param size Size of the data
 * @param mode I/O mode (as in fopen(3))
 */
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

/* Modes of the "Label" tool */
enum
{
  LABEL_LABEL,                  /* Adding new label(s) */
  LABEL_SELECT,                 /* "Select" button clicked; user is selecting a label to edit */
  LABEL_APPLY                   /* "Apply" button clicked; user is selecting a label to apply permanently to the canvas */
};



/* Color globals (copied from colors.h, if no colors specified by user) */

static int NUM_COLORS;
static Uint8 **color_hexes;
static char **color_names;

/* Special color options (from left-to-right (very last entry)) */
#define COLOR_SELECTOR (NUM_COLORS - 3) /* Pick a color from the canvas */
#define COLOR_PICKER (NUM_COLORS - 2)   /* Pick a color from a palette */
#define COLOR_MIXER (NUM_COLORS - 1)    /* Mix colors together */

/* Show debugging stuff: */

/**
 * Echos debug info to STDERR, if debugging (DEBUG #define) is set.
 *
 * @param str text to echo
 */
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

/* *INDENT-OFF* */
/* static SDL_Rect r_screen; *//* was 640x480 @ 0,0  -- but this isn't so useful */
/* *INDENT-ON* */

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
static int button_size_auto = 0;        /* if the size of buttons should be autocalculated */
static float button_scale;      /* scale factor to be applied to the size of buttons */
static int color_button_w;      /* was 32 */
static int color_button_h;      /* was 48 */
static int colors_rows;

static int buttons_tall;        /* promoted to a global variable from setup_normal_screen_layout() */

/* Define button grid dimensions. (in button units) */
/* These are the maximum slots -- some may be unused. */
static grid_dims gd_tools;      /* was 2x7 */
static grid_dims gd_sfx;
static grid_dims gd_toolopt;    /* was 2x7 */

/* *INDENT-OFF* */
/* static grid_dims gd_open; *//* was 4x4 */
/* *INDENT-ON* */
static grid_dims gd_colors;     /* was 17x1 */

#define ORIGINAL_BUTTON_SIZE 48 /* Original Button Size */
#define HEIGHTOFFSET ((Sint16) (((WINDOW_HEIGHT - 480) / button_h) * button_h))
#define TOOLOFFSET ((Sint16) (HEIGHTOFFSET / button_h * 2))
#define PROMPTOFFSETX ((Sint16) (WINDOW_WIDTH - 640) / 2)
#define PROMPTOFFSETY ((Sint16) (HEIGHTOFFSET / 2))

#define THUMB_W ((WINDOW_WIDTH - r_ttools.w - r_ttoolopt.w) / 4)
#define THUMB_H (((button_h * buttons_tall + r_ttools.h) - button_h - button_h / 2) / 4)

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
static void magic_xorpixel(SDL_Surface * surface, int x, int y);

/**
 * Sets button_scale to the maximum Tux Paint can handle
 */
static void set_max_buttonscale(void)
{
  float max_w, max_h;

  /* WINDOW_WIDTH / original size of tools columns + 9 buttons + tooloption columns */
  max_w = (float)WINDOW_WIDTH / (gd_tools.cols * 48 + (9 * 48) + (gd_toolopt.cols * 48));

  /* WINDOW_HEIGHT / original size of r_ttools.h + 5 buttons + colors rows + tux area */
  max_h = (float)WINDOW_HEIGHT / (40 + (6 * 48) + (gd_colors.rows * 48) + 56);

  button_scale = min(max_w, max_h);
  fprintf(stderr, "Info: Will use a button size of %d (scale = %f)\n",
          (int)(button_scale * ORIGINAL_BUTTON_SIZE), button_scale);
}

/**
 * Sets a variety of screen layout globals, based on the
 * size of the window/screen Tux Paint is being displayed on
 * (WINDOW_WIDTH & WINDOW_HEIGHT).
 */
static void setup_normal_screen_layout(void)
{
  button_w = 48 * button_scale;
  button_h = 48 * button_scale;

  gd_toolopt.cols = 2;
  gd_tools.cols = 2;

  r_ttools.x = 0;
  r_ttools.y = 0;
  r_ttools.w = gd_tools.cols * button_w;
  r_ttools.h = 40 * button_scale;

  r_ttoolopt.w = gd_toolopt.cols * button_w;
  r_ttoolopt.h = 40 * button_scale;
  r_ttoolopt.x = WINDOW_WIDTH - r_ttoolopt.w;
  r_ttoolopt.y = 0;

  gd_colors.rows = colors_rows;
  gd_colors.cols = (NUM_COLORS + gd_colors.rows - 1) / gd_colors.rows;

  r_colors.h = 48 * button_scale * gd_colors.rows;
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
  buttons_tall = (WINDOW_HEIGHT - r_ttoolopt.h - 56 * button_scale - r_colors.h) / button_h;
  if (buttons_tall < 5)
  {
    fprintf(stderr,
            "Warning: Button size '%d' with window size '%dx%d' is not reasonable (not tall enough).\n",
            button_w, WINDOW_WIDTH, WINDOW_HEIGHT);
    set_max_buttonscale();
    setup_normal_screen_layout();
  }

  if (r_canvas.w < button_w * 9)
  {
    fprintf(stderr,
            "Warning: Button size '%d' with window size '%dx%d' is not reasonable (not wide enough).\n",
            button_w, WINDOW_WIDTH, WINDOW_HEIGHT);
    set_max_buttonscale();
    setup_normal_screen_layout();
  }

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
/**
 * Debug output that shows a layout rectangle's position & dimensions
 * (Used as a #define macro, by print_layout(), below)
 *
 * @param r The rectange
 * @param name The name of the rect object
 */
static void debug_rect(SDL_Rect *r, char *name)
{
  /* FIXME: Send to stderr, not stdout? */
  printf("%-12s %dx%d @ %d,%d\n", name, r->w, r->h, r->x, r->y);
}

#define DR(x) debug_rect(&x, #x)

/**
 * Debug output that shows a layout grid's dimensions
 * (Used as a #define macro, by print_layout(), below)
 *
 * @param g The grid
 * @param name The name of the grid object
 */
static void debug_dims(grid_dims *g, char *name)
{
  /* FIXME: Send to stderr, not stdout? */
  printf("%-12s %dx%d\n", name, g->cols, g->rows);
}

#define DD(x) debug_dims(&x, #x)

/**
 * Debug output that shows Tux Paint's layout
 */
static void print_layout(void)
{
  /* FIXME: Send to stderr, not stdout? */
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
  /* FIXME: Send to stderr, not stdout? */
  printf("buttons are %dx%d\n", button_w, button_h);
  printf("color buttons are %dx%d\n", color_button_w, color_button_h);
}

#undef DD
#undef DR
#endif

/**
 * Set up (and display, if debugging is enabled), the
 * position, size, and layout of Tux Paint's UI
 */
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

static SDL_Surface *SDL_DisplayFormat(SDL_Surface *surface)
{
  SDL_Surface *tmp;

  tmp = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB888, 0);
  return (tmp);
}

static SDL_Surface *SDL_DisplayFormatAlpha(SDL_Surface *surface)
{
  SDL_Surface *tmp;

  tmp = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
  return (tmp);
}

static void SDL_Flip(SDL_Surface *screen)
{
  //SDL_UpdateWindowSurface(window_screen);
  SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

static void SDL_UpdateRect(SDL_Surface *screen, Sint32 x, Sint32 y, Sint32 w, Sint32 h)
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

static void show_progress_bar(SDL_Surface *screen)
{
  show_progress_bar_(screen, texture, renderer);
}



/**
 * Update a rect. based on two x/y coords (not necessarly in order)
 * (calls SDL_UpdateRect())
 *
 * @param x1 X of first coordinate
 * @param y1 Y of first coordinate
 * @param x2 X of second coordinate
 * @param y2 Y of second coordinate
 */

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


/**
 * Update a rect. area of the screen
 * (calls SDL_UpdateRect())
 *
 * @param r The rect
 */
static void update_screen_rect(SDL_Rect *r)
{
  SDL_UpdateRect(screen, r->x, r->y, r->w, r->h);
}


/**
 * Test whether an x/y coordinate is within a given rect.
 *
 * @param r The rect
 * @param x X coordinate
 * @param y Y coordinate
 * @return true if a hit, else false
 */
static int hit_test(const SDL_Rect *const r, unsigned x, unsigned y)
{
  /* note the use of unsigned math: no need to check for negative */
  return (x - (unsigned)r->x < (unsigned)r->w) && (y - (unsigned)r->y < (unsigned)r->h);
}

#define HIT(r) hit_test(&(r), event.button.x, event.button.y)


/**
 * Returns which item in a grid was clicked, if any.
 *
 * @param r The rectangle containing the grid on the scren
 * @param x X coordinate (mouse location) of a click
 * @param y Y coordinate (mouse location) of a click
 * @param gd The grid of items
 * @returns The item clicked, or -1 if click was outside the grid.
 */
static int grid_hit_gd(const SDL_Rect *const r, unsigned x, unsigned y, grid_dims *gd)
{
  unsigned item_w = r->w / gd->cols;
  unsigned item_h = r->h / gd->rows;
  unsigned col = (x - r->x) / item_w;
  unsigned row = (y - r->y) / item_h;

  DEBUG_PRINTF("%d,%d resolves to %d,%d in a %dx%d grid, index is %d\n", x, y,
               col, row, gd->cols, gd->rows, col + row * gd->cols);
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

/**
 * Update the contents of a rectangular region of the drawing canvas.
 * If overlaying Starter Image exists, and/or Labels are in
 * the image, draw them over the picture.
 *
 * FIXME: Subtly different from update_canvas_ex(), below.
 *
 * @param x1 Left side of area to update
 * @param y1 Top side of area to update
 * @param x2 Right side of area to update
 * @param y2 Bottom side of area to update
 * @param screen_too If true, show updated canvas on the screen
 */
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

  dest.x = x1 + r_ttools.w;

  SDL_BlitSurface(canvas, &src, screen, &dest);

  /* If label is not disabled, cover canvas with label layer */

  if (!disable_label)
    SDL_BlitSurface(label, &src, screen, &dest);

  if (screen_too)
    update_screen(x1 + r_ttools.w, y1, x2 + r_ttools.w, y2);
}

/**
 * Update the contents of a rectangular region of the drawing canvas.
 * If overlaying Starter Image exists, and/or Labels are in
 * the image, draw them over the picture.
 *
 * FIXME: Subtly different from update_canvas_ex_r(), above.
 *
 * @param x1 Left side of area to update
 * @param y1 Top side of area to update
 * @param x2 Right side of area to update
 * @param y2 Bottom side of area to update
 * @param screen_too If true, show updated canvas on the screen
 */
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
    update_screen(x1 + r_ttools.w, y1, x2 + r_ttools.w, y2);
}

/**
 * Update the screen with the new canvas.
 * (Wrapper for update_canvas_ex(), above, with `screen_too` = true)
 *
 * @param x1 Left side of area to update
 * @param y1 Top side of area to update
 * @param x2 Right side of area to update
 * @param y2 Bottom side of area to update
 */
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
static int joystick_button_selectfilltool = 255;
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
static int motion_since_click = 0;
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
static int scrolling_selector, scrolling_tool, scrolling_dialog;

static int promptless_save = SAVE_OVER_UNSET;
static int _promptless_save_over, _promptless_save_over_ask, _promptless_save_over_new;
static int disable_quit;

static int noshortcuts;
static int disable_save;
static int disable_erase;
static int ok_to_use_lockfile = 1;
static int start_blank;
static int autosave_on_quit;
static int no_prompt_on_quit = 0;
static int reversesort = 0;

static int dont_do_xor;
static int dont_load_stamps;
static int mirrorstamps;
static int disable_stamp_controls;
static int stamp_size_override = -1;
static int no_stamp_rotation = 0;
static int new_colors_last;

static int disable_template_export;

#ifdef NOKIA_770
static int simple_shapes = 1;
#else
static int simple_shapes;
#endif
static int only_uppercase;

static int disable_magic_controls;
static int disable_magic_sizes;
static int disable_shape_controls;
static int no_magic_groups = 0;

static int shape_mode = SHAPEMODE_CENTER;
static int stamp_rotation_ctrl = 0;

static int starter_mirrored;
static int starter_flipped;
static int starter_personal;
static int template_personal;
static int starter_modified;

static int disable_brushspacing;

static Uint8 canvas_color_r, canvas_color_g, canvas_color_b;
static Uint8 *touched;
static Uint8 *sim_flood_touched;
int sim_flood_x1 = 0, sim_flood_y1 = 0, sim_flood_x2 = 0, sim_flood_y2 = 0;
int fill_x, fill_y;
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
static int coming_from_undo_or_redo = SDL_FALSE;


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
static void handle_joyhatmotion(SDL_Event event, int oldpos_x, int oldpos_y,
                                int *valhat_x, int *valhat_y, int *hat_motioner, Uint32 * old_hat_ticks);
static void handle_joyballmotion(SDL_Event event, int oldpos_x, int oldpos_y);
static void handle_joybuttonupdown(SDL_Event event, int oldpos_x, int oldpos_y);
static void handle_motioners(int oldpos_x, int oldpos_y, int motioner,
                             int hatmotioner, int old_hat_ticks, int val_x, int val_y, int valhat_x, int valhat_y);

static void handle_joybuttonupdownscl(SDL_Event event, int oldpos_x, int oldpos_y, SDL_Rect real_r_tools);

#ifdef __ANDROID__
static void start_motion_convert(SDL_Event event);
static void convert_motion_to_wheel(SDL_Event event);
static void stop_motion_convert(SDL_Event event);
#endif

char *get_xdg_user_dir(const char *dir_type, const char *fallback);

#ifdef WIN32
extern char *GetUserImageDir(void);
#endif

/* Magic tools API and tool handles: */

#include "tp_magic_api.h"

static Uint8 magic_disabled_features = 0x00000000;
static Uint8 magic_complexity_level = MAGIC_COMPLEXITY_DEFAULT;

static void update_progress_bar(void);
static void special_notify(int flags);

typedef struct magic_funcs_s
{
  int (*get_tool_count)(magic_api *);
  int (*get_group)(magic_api *, int);
  int (*get_order)(int);
  char *(*get_name)(magic_api *, int);
  SDL_Surface *(*get_icon)(magic_api *, int);
  char *(*get_description)(magic_api *, int, int);
  int (*requires_colors)(magic_api *, int);
   Uint8(*accepted_sizes) (magic_api *, int, int);
   Uint8(*default_size) (magic_api *, int, int);
  int (*modes)(magic_api *, int);
  void (*set_color)(magic_api *, int, SDL_Surface *, SDL_Surface *, Uint8, Uint8, Uint8, SDL_Rect *);
  void (*set_size)(magic_api *, int, int, SDL_Surface *, SDL_Surface *, Uint8, SDL_Rect *);
  int (*init)(magic_api *, Uint8, Uint8);
   Uint32(*api_version) (void);
  void (*shutdown)(magic_api *);
  void (*click)(magic_api *, int, int, SDL_Surface *, SDL_Surface *, int, int, SDL_Rect *);
  void (*drag)(magic_api *, int, SDL_Surface *, SDL_Surface *, int, int, int, int, SDL_Rect *);
  void (*release)(magic_api *, int, SDL_Surface *, SDL_Surface *, int, int, SDL_Rect *);
  void (*switchin)(magic_api *, int, int, SDL_Surface *, SDL_Surface *);
  void (*switchout)(magic_api *, int, int, SDL_Surface *, SDL_Surface *);
  void (*retract_undo)(void);
} magic_funcs_t;


typedef struct magic_s
{
  int place;                    /* System-wide or local to the user? */
  int handle_idx;               /* Index to magic funcs for each magic tool (shared objs may report more than 1 tool) */
  int idx;                      /* Index to magic tools within shared objects (shared objs may report more than 1 tool) */
  int mode;                     /* Current mode (paint or fullscreen) */
  int avail_modes;              /* Available modes (paint &/or fullscreen) */
  int colors;                   /* Whether magic tool accepts colors */
  int sizes[MAX_MODES];         /* Whether magic tool accepts sizes */
  int default_size[MAX_MODES];  /* Magic tool's default size setting */
  int size[MAX_MODES];          /* Magic tool's size setting */
  int group;                    /* Which group of magic tools this lives in */
  int order;                    /* Order within the group of magic tools (for sorting; falls back to name) */
  char *name;                   /* Name of magic tool */
  char *tip[MAX_MODES];         /* Description of magic tool, in each possible mode */
  SDL_Surface *img_icon;
  SDL_Surface *img_name;
} magic_t;


#define MAX_MAGIC_GROUPS 16
#define MAX_MAGICS_PER_GROUP 128

static int num_plugin_files;    /* How many shared object files we went through */
static void *magic_handle[MAX_MAGIC_GROUPS * MAX_MAGICS_PER_GROUP];     /* Handle to shared object (to be unloaded later) *//* FIXME: Unload them! */
static magic_funcs_t magic_funcs[MAX_MAGIC_GROUPS * MAX_MAGICS_PER_GROUP];      /* Pointer to shared objects' functions */

static magic_t magics[MAX_MAGIC_GROUPS][MAX_MAGICS_PER_GROUP];
static int num_magics[MAX_MAGIC_GROUPS];        /* How many magic tools were loaded (note: shared objs may report more than 1 tool) */
static int num_magics_total;

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


#if !defined(WIN32) && !defined(__APPLE__) && !defined(__BEOS__) && !defined(__HAIKU__) && !defined(__ANDROID__)
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
static SDL_Surface *img_btn_up, *img_btn_down, *img_btn_off, *img_btn_hold;
static SDL_Surface *img_btnsm_up, *img_btnsm_off, *img_btnsm_down, *img_btnsm_hold;
static SDL_Surface *img_btn_nav, *img_btnsm_nav;
static SDL_Surface *img_brush_anim, *img_brush_dir;
static SDL_Surface *img_prev, *img_next;
static SDL_Surface *img_mirror, *img_flip, *img_rotate;
static SDL_Surface *img_dead40x40;
static SDL_Surface *img_black, *img_grey;
static SDL_Surface *img_yes, *img_no;
static SDL_Surface *img_sfx, *img_speak;
static SDL_Surface *img_open, *img_erase, *img_back, *img_trash, *img_pict_export;
static SDL_Surface *img_slideshow, *img_template, *img_play, *img_gif_export, *img_select_digits;
static SDL_Surface *img_printer, *img_printer_wait;
static SDL_Surface *img_save_over, *img_popup_arrow;
static SDL_Surface *img_cursor_up, *img_cursor_down;
static SDL_Surface *img_cursor_starter_up, *img_cursor_starter_down;
static SDL_Surface *img_scroll_up, *img_scroll_down;
static SDL_Surface *img_scroll_up_off, *img_scroll_down_off;
static SDL_Surface *img_grow, *img_shrink;
static SDL_Surface *img_magic_paint, *img_magic_fullscreen;
static SDL_Surface *img_shapes_corner, *img_shapes_center;
static SDL_Surface *img_bold, *img_italic;
static SDL_Surface *img_label_select, *img_label_apply;
static SDL_Surface *img_color_picker, *img_color_picker_thumb, *img_color_picker_val;
static SDL_Surface *img_paintwell, *img_color_sel, *img_color_mix, *img_color_picker_icon;
static SDL_Surface *img_color_grab;
static int color_picker_x, color_picker_y, color_picker_v;
static int color_mixer_reset;

static SDL_Surface *img_title_on, *img_title_off, *img_title_large_on, *img_title_large_off;
static SDL_Surface *img_title_names[NUM_TITLES];
static SDL_Surface *img_tools[NUM_TOOLS], *img_tool_names[NUM_TOOLS];

static SDL_Surface *img_oskdel, *img_osktab, *img_oskenter, *img_oskcapslock, *img_oskshift, *img_oskpaste;
static SDL_Surface *thumbnail(SDL_Surface * src, int max_x, int max_y, int keep_aspect);
static SDL_Surface *thumbnail2(SDL_Surface * src, int max_x, int max_y, int keep_aspect, int keep_alpha);

#ifndef NO_BILINEAR
static SDL_Surface *zoom(SDL_Surface * src, int new_x, int new_y);
#endif


/**
 * Render some text (char's) as a bitmap
 *
 * @param font The font to use
 * @param str The string of text to render
 * @param color The color to draw it in
 * @return A new surface, containing the rendered text
 */
static SDL_Surface *render_text(TuxPaint_Font *restrict font, const char *restrict str, SDL_Color color)
{
  SDL_Surface *ret = NULL;
  int height;
  SDLPango_Matrix pango_color;

  if (font == NULL)
  {
    fprintf(stderr, "Warning: render_text() received a NULL font!\n");
    fflush(stdout);
    return NULL;
  }

  if (font->typ == FONT_TYPE_PANGO)
  {
    sdl_color_to_pango_color(color, &pango_color);

    DEBUG_PRINTF("Calling SDLPango_SetText(\"%s\")\n", str);

#ifdef __ANDROID__
    /* FIXME This extrange workaround helps in getting the translations working
       on 4.3 4.4 */
    SDLPango_SetLanguage(font->pango_context, "ca");
#endif

    SDLPango_SetDefaultColor(font->pango_context, &pango_color);
    // SDLPango_SetText(font->pango_context, str, -1);
    SDLPango_SetText_GivenAlignment(font->pango_context, str, -1, SDLPANGO_ALIGN_CENTER);
    ret = SDLPango_CreateSurfaceDraw(font->pango_context);
  }

  if (font->typ == FONT_TYPE_TTF)
  {
    DEBUG_PRINTF("Calling TTF_RenderUTF8_Blended(\"%s\")\n", str);

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


/**
 * Convert a wide-character string to string of Uint16's.
 *
 * This conversion is required on platforms where Uint16 doesn't match wchar_t.
 * On Windows, wchar_t is 16-bit, elsewhere it is 32-bit.
 * Mismatch caused by the use of Uint16 for unicode characters by SDL, SDL_ttf.
 * I guess wchar_t is really only suitable for internal use ...
 *
 * @param str The wide-character string
 * @return The string, as Uint16 characters.
 */
static Uint16 *wcstou16(const wchar_t *str)
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


/**
 * Render some text (wide-characters) as a bitmap
 *
 * @param font The font to use
 * @param str The string of text to render
 * @param color The color to draw it in
 * @return A new surface, containing the rendered text
 */
static SDL_Surface *render_text_w(TuxPaint_Font *restrict font, const wchar_t *restrict str, SDL_Color color)
{
  SDL_Surface *ret = NULL;
  int height;
  Uint16 *ustr;
  int utfstr_max;
  char *utfstr;
  SDLPango_Matrix pango_color;

  if (font->typ == FONT_TYPE_PANGO)
  {
    sdl_color_to_pango_color(color, &pango_color);

    SDLPango_SetDefaultColor(font->pango_context, &pango_color);

    /* Convert from 16-bit UNICODE to UTF-8 encoded for SDL_Pango: */

    utfstr_max = (sizeof(char) * 4 * (wcslen(str) + 1));
    utfstr = (char *)malloc(utfstr_max);

    wcstombs(utfstr, str, utfstr_max);

    SDLPango_SetText(font->pango_context, utfstr, -1);
    ret = SDLPango_CreateSurfaceDraw(font->pango_context);
  }

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
  unsigned sound_processed:1;
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

static void get_stamp_thumb(stamp_type * sd, int process_sound);

#define MAX_STAMP_GROUPS 256

static unsigned int stamp_group_dir_depth = 1;  /* How deep (how many slashes in a subdirectory path) we think a new stamp group should be */

static int stamp_group = 0;

static const char *load_stamp_basedir;
static int num_stamp_groups = 0;
static int num_stamps[MAX_STAMP_GROUPS];
static int max_stamps[MAX_STAMP_GROUPS];
static stamp_type **stamp_data[MAX_STAMP_GROUPS];

static SDL_Surface *active_stamp;
static SDL_Surface *current_stamp_cached = NULL;


/**
 * Returns whether a particular stamp can be colored.
 *
 * @param stamp Which stamp?
 * @return True/false
 */
static int stamp_colorable(int stamp)
{
  return stamp_data[stamp_group][stamp]->colorable;
}

/**
 * Returns whether a particular stamp can be tinted.
 *
 * @param stamp Which stamp?
 * @return True/false
 */
static int stamp_tintable(int stamp)
{
  return stamp_data[stamp_group][stamp]->tintable;
}


#define SHAPE_BRUSH_NAME "aa_round_03.png"
static int num_brushes, num_brushes_max, shape_brush = 0;
static SDL_Surface **img_brushes, **img_brushes_thumbs;
static int *brushes_frames = NULL;
static int *brushes_spacing = NULL;
static int *brushes_spacing_default = NULL;
static short *brushes_directional = NULL;
static short *brushes_rotate = NULL;
static short *brushes_chaotic = NULL;
static char **brushes_descr = NULL;
static Uint8 *brushes_descr_localized = NULL;

static SDL_Surface *img_shapes[NUM_SHAPES], *img_shape_names[NUM_SHAPES];
static SDL_Surface *img_fills[NUM_FILLS], *img_fill_names[NUM_FILLS];
static SDL_Surface *img_openlabels_open, *img_openlabels_erase,
  *img_openlabels_slideshow, *img_openlabels_back, *img_openlabels_play,
  *img_openlabels_template, *img_openlabels_gif_export,
  *img_openlabels_pict_export, *img_openlabels_next, *img_mixerlabel_clear;

static SDL_Surface *img_tux[NUM_TIP_TUX];

static SDL_Surface *img_mouse, *img_mouse_click;

static SDL_Surface **img_color_btns;
static SDL_Surface *img_color_btn_off;

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
  img_cur_brush_frames, img_cur_brush_directional, img_cur_brush_rotate, img_cur_brush_chaotic, img_cur_brush_spacing;
static int brush_counter, brush_frame;

enum
{
  ERASER_TYPE_SQUARE,
  ERASER_TYPE_CIRCLE,
  ERASER_TYPE_CIRCLE_FUZZY,
  ERASER_TYPE_CIRCLE_TRANSPARENT,
  NUM_ERASER_TYPES
};

#define NUM_ERASER_SIZES 8
#define NUM_ERASERS (NUM_ERASER_SIZES * NUM_ERASER_TYPES)

/* Min to max sizes of the erasers */
#define ERASER_MIN 5            /* Smaller than 5 will not render as a circle! */
#define ERASER_MAX 128

#define BRUSH_SPACING_SIZES 13  /* How many brush spacing options to provide
                                   (max will represent BRUSH_SPACING_MAX_MULTIPLIER times the
                                   max dimension of the brush; min will represent 1 pixel) */
#define BRUSH_SPACING_MAX_MULTIPLIER 5  /* How far apart (in terms of a multiplier of a
                                           brush's dimensions) the max brush spacing setting
                                           represents */

static unsigned cur_color;
static int cur_tool, cur_brush, old_tool;
static int magic_group = 0;
static int cur_stamp[MAX_STAMP_GROUPS];
static int cur_shape, cur_magic[MAX_MAGIC_GROUPS];
static int cur_font, cur_eraser, cur_fill, fill_drag_started;
static int cursor_left, cursor_x, cursor_y, cursor_textwidth;   /* canvas-relative */
static int old_cursor_x, old_cursor_y;
static int cur_label, cur_select;
static int been_saved;
static char file_id[FILENAME_MAX];
static char starter_id[FILENAME_MAX];
static char template_id[FILENAME_MAX];
static int brush_scroll;
static int stamp_scroll[MAX_STAMP_GROUPS];
static int magic_scroll[MAX_MAGIC_GROUPS];
static int font_scroll, tool_scroll;
static int eraser_scroll, shape_scroll, fill_scroll;

static int eraser_sound;

static IM_DATA im_data;
static wchar_t texttool_str[256];
static unsigned int texttool_len;

static int tool_avail[NUM_TOOLS], tool_avail_bak[NUM_TOOLS];

static Uint32 cur_toggle_count;

static int num_wished_langs;

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

static SDL_Event scrolltimer_selector_event, scrolltimer_tool_event, scrolltimer_dialog_event;

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
static void blit_brush(int x, int y, int direction, double rotation, int *w, int *h);
static void stamp_draw(int x, int y, int stamp_angle_rotation);
static void rec_undo_buffer(void);

void show_version(int details);
void show_usage(int exitcode);

int compare_font_family(const void *a, const void *b);
void show_fonts(void);

static char *progname;

static SDL_Cursor *get_cursor(unsigned char *bits, unsigned char *mask_bits,
                              unsigned int w, unsigned int h, unsigned int x, unsigned int y);
static void seticon(void);
static SDL_Surface *loadimage(const char *const fname);
static SDL_Surface *do_loadimage(const char *const fname, int abort_on_error);
static void draw_toolbar(void);
static void draw_magic(void);
static void draw_brushes(void);
static void draw_brushes_spacing(void);
static void draw_stamps(void);
static void draw_shapes(void);
static void draw_erasers(void);
static void draw_fonts(void);
static void draw_fills(void);
static void draw_none(void);

static void do_undo(void);
static void do_redo(void);
static void render_brush(void);
static void show_brush_tip(void);
static void _xorpixel(SDL_Surface * surf, int x, int y);
static void line_xor(int x1, int y1, int x2, int y2);
static void rect_xor(int x1, int y1, int x2, int y2);
static void circle_xor(int x, int y, int sz);
static void draw_blinking_cursor(void);
static void hide_blinking_cursor(void);
static int text_label_tool_enter(int font_height);

static void reset_brush_counter(int force);

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
static void update_stamp_xor(int stamp_angle_rotation);
#endif

static void set_active_stamp(void);
static void clear_cached_stamp(void);

static int calc_eraser_size(int which_eraser);
static void do_eraser(int x, int y, int update);
static void eraser_draw(int x1, int y1, int x2, int y2);
static void disable_avail_tools(void);
static void enable_avail_tools(void);
static void reset_avail_tools(void);
static int compare_dirent2s(struct dirent2 *f1, struct dirent2 *f2);
static int compare_dirent2s_invert(struct dirent2 *f1, struct dirent2 *f2);
static void redraw_tux_text(void);
static void draw_tux_text(int which_tux, const char *const str, int want_right_to_left);
static void draw_tux_text_ex(int which_tux, const char *const str, int want_right_to_left, Uint8 locale_text);
static void draw_cur_tool_tip(void);
static void wordwrap_text(const char *const str, SDL_Color color, int left, int top, int right, int want_right_to_left);
static void wordwrap_text_ex(const char *const str, SDL_Color color, int left,
                             int top, int right, int want_right_to_left, Uint8 locale_text);
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
static void do_shape(int sx, int sy, int nx, int ny, int rotn, int use_brush);
static int shape_rotation(int ctr_x, int ctr_y, int ox, int oy);
static int brush_rotation(int ctr_x, int ctr_y, int ox, int oy);
static int stamp_will_rotate(int ctr_x, int ctr_y, int ox, int oy);
static int stamp_rotation(int ctr_x, int ctr_y, int ox, int oy);
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
static int do_new_dialog_add_colors(SDL_Surface * *thumbs, int num_files,
                                    int *d_places, char * *d_names, char * *d_exts, int *white_in_palette);
static int do_color_picker(int prev_color);
static void draw_color_picker_crosshairs(int color_picker_left,
                                         int color_picker_top, int color_picker_val_left, int color_picker_val_top);
static void set_color_picker_crosshair_size(void);
static void draw_color_picker_values(int l, int t);
static void draw_color_grab_btn(SDL_Rect dest, int c);
static void draw_color_picker_palette_and_values(int color_picker_left,
                                                 int color_picker_top,
                                                 int color_picker_val_left, int color_picker_val_top);
static void render_color_picker_palette(void);
static int do_color_sel(int temp_mode);
static int do_color_mix(void);
static void draw_color_mixer_blank_example(void);
static void calc_color_mixer_average(float *out_h, float *out_s, float *out_v);
static void draw_color_mixer_tooltip(void);
static void draw_color_mix_undo_redo(void);
static void render_color_button(int the_color, SDL_Surface * icon);
static void handle_color_changed(void);
static void magic_set_color(void);
static void magic_set_size(void);

static void do_quick_eraser(void);

static int do_slideshow(void);
static void play_slideshow(int *selected, int num_selected, char *dirname, char **d_names, char **d_exts, int speed);
static void draw_selection_digits(int right, int bottom, int n);

static int export_gif(int *selected, int num_selected, char *dirname,
                      char **d_names, char **d_exts, int speed, char **dest_fname);
int export_gif_monitor_events(void);

/* Locations where export_pict() can save */
enum
{
  EXPORT_LOC_PICTURES,
  EXPORT_LOC_TEMPLATES
};

/* Return values of export_pict() */
enum
{
  EXPORT_SUCCESS,
  EXPORT_ERR_CANNOT_MKDIR,      /* Need to mkdir() but cannot */
  EXPORT_ERR_FILENAME_PROBLEM,  /* Problem creating output file's filename */
  EXPORT_ERR_CANNOT_OPEN_SOURCE,        /* Can't open input file for read */
  EXPORT_ERR_CANNOT_SAVE,       /* Can't open export file for write */
  EXPORT_ERR_ALREADY_EXPORTED   /* Exported template appears to already exist */
};

static int export_pict(char *fname, int where, char *orig_fname, char **dest_fname);
static char *get_export_filepath(const char *ext);
void get_img_dimensions(char *fpath, int *widht, int *height);
uLong get_img_crc(char *fpath);

static void wait_for_sfx(void);
static void rgbtohsv(Uint8 r8, Uint8 g8, Uint8 b8, float *h, float *s, float *v);
static void hsvtorgb(float h, float s, float v, Uint8 * r8, Uint8 * g8, Uint8 * b8);

static SDL_Surface *flip_surface(SDL_Surface * s);
static SDL_Surface *mirror_surface(SDL_Surface * s);

static void print_image(void);
static void do_print(void);
static void strip_trailing_whitespace(char *buf);
static void strip_quotes(char *buf);
static int do_render_cur_text(int do_blit);
static char *uppercase(const char *restrict const str);
static wchar_t *uppercase_w(const wchar_t *restrict const str);
static SDL_Surface *do_render_button_label(const char *const label);

#if 0
static SDL_Surface *crop_surface(SDL_Surface * surf);
#endif
static void create_button_labels(void);
static Uint32 scrolltimer_selector_callback(Uint32 interval, void *param);
static Uint32 scrolltimer_tool_callback(Uint32 interval, void *param);
static Uint32 scrolltimer_dialog_callback(Uint32 interval, void *param);
static Uint32 drawtext_callback(Uint32 interval, void *param);
static void control_drawtext_timer(Uint32 interval, const char *const text, Uint8 locale_text);
static const char *great_str(void);
static void draw_image_title(int t, SDL_Rect dest);
static void handle_keymouse(SDLKey key, Uint32 updown, int steps, SDL_Rect * area1, SDL_Rect * area2);
static void handle_keymouse_buttons(SDLKey key, int *whicht, int *whichc, SDL_Rect real_r_tools);
static void handle_active(SDL_Event * event);

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
static int magic_playingsound(void);
static void magic_pausesound(void);
static void magic_unpausesound(void);
static void magic_stopsound(void);
static void magic_line_func(void *mapi,
                            int which, SDL_Surface * canvas,
                            SDL_Surface * last, int x1, int y1, int x2,
                            int y2, int step, void (*cb)(void *, int, SDL_Surface *, SDL_Surface *, int, int));

static Uint8 magic_linear_to_sRGB(float lin);
static float magic_sRGB_to_linear(Uint8 srgb);
static int magic_button_down(void);
static SDL_Surface *magic_scale(SDL_Surface * surf, int w, int h, int aspect);
static SDL_Surface *magic_rotate_scale(SDL_Surface * surf, int r, int w);
static void reset_touched(void);
static Uint8 magic_touched(int x, int y);
static void magic_retract_undo(void);

static void magic_switchin(SDL_Surface * last);
static void magic_switchout(SDL_Surface * last);
static int magic_modeint(int mode);

#ifdef DEBUG
static char *debug_gettext(const char *str);
static int charsize(Uint16 c);
#endif

static SDL_Surface *load_kpx(const char *file);

#ifndef NOSVG
static SDL_Surface *load_svg(const char *file);
#endif
static float pick_best_scape(unsigned int orig_w, unsigned int orig_h, unsigned int max_w, unsigned int max_h);
static SDL_Surface *myIMG_Load_RWops(const char *file);
static SDL_Surface *myIMG_Load(const char *file);
static int trash(char *path);
int file_exists(char *path);

int generate_fontconfig_cache_spinner(SDL_Surface * screen);

char *safe_strncat(char *dest, const char *src, size_t n);
char *safe_strncpy(char *dest, const char *src, size_t n);
int safe_snprintf(char *str, size_t size, const char *format, ...);

static int generate_fontconfig_cache_real(void);

#define MAX_UTF8_CHAR_LENGTH 6

#define USEREVENT_TEXT_UPDATE 1
#define USEREVENT_PLAYDESCSOUND 2

static int bypass_splash_wait;


/* 2022 marked the 20th anniversary of Tux Paint, so the release
   that year included a special addition to the splash title screen */

/* #define ANNIVERSARY */

#ifdef ANNIVERSARY

#define NUM_CONFETTI 100

typedef struct
{
  float x, y, xm, ym, ymm;
  Uint32 color;
} confetti_t;

confetti_t confetti[NUM_CONFETTI];

#endif


/**
 * Wait for a keypress or mouse click.
 *
 * @param counter How long to wait (in 1/10th of seconds)
 */
static void do_wait(int counter)
{
  SDL_Event event;
  int done;

#ifdef ANNIVERSARY
  int i;
  SDL_Surface *back_surf;
  SDL_Rect r;
#endif

  if (bypass_splash_wait)
    return;

  done = 0;

#ifdef ANNIVERSARY
  for (i = 0; i < NUM_CONFETTI; i++)
  {
    confetti[i].x = rand() % screen->w;
    confetti[i].y = rand() % screen->h;
    confetti[i].xm = (rand() % 9) - 4;
    confetti[i].ym = (rand() % 4);
    confetti[i].ymm = ((rand() % 10) / 20) + 0.1;
    confetti[i].color = SDL_MapRGB(screen->format, (rand() % 128) + 96, (rand() % 128) + 96, (rand() % 128) + 96);
  }

  back_surf = SDL_DisplayFormat(screen);
#endif


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

#ifdef ANNIVERSARY
    for (i = 0; i < NUM_CONFETTI; i++)
    {
      r.x = confetti[i].x;
      r.y = confetti[i].y;
      r.w = 8;
      r.h = 8;

      SDL_BlitSurface(back_surf, &r, screen, &r);
    }

    for (i = 0; i < NUM_CONFETTI; i++)
    {
      confetti[i].x += confetti[i].xm;
      confetti[i].xm += (((rand() % 5) - 2) / 10);
      confetti[i].y += confetti[i].ym;
      confetti[i].ym += confetti[i].ymm;

      r.x = confetti[i].x;
      r.y = confetti[i].y;
      r.w = 6 + (rand() % 2);
      r.h = (r.y % 8) + 1;

      SDL_FillRect(screen, &r, confetti[i].color);
    }

    SDL_Flip(screen);
#endif


    counter--;
    SDL_Delay(100);
  }
  while (!done && counter > 0);

#ifdef ANNIVERSARY
  SDL_FreeSurface(back_surf);
#endif
}


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

/* Prompt to confirm erasing a picture in the Open dialog,
   or exported template from the New dialog */
#define PROMPT_ERASE_TXT gettext_noop("Erase this picture?")
#define PROMPT_ERASE_TEMPLATE_TXT gettext_noop("Erase this template?")
#define PROMPT_ERASE_YES gettext_noop("Yes, erase it!")
#define PROMPT_ERASE_NO gettext_noop("No, don’t erase it!")

/* Reminder that Mouse Button 1 is the button to use in Tux Paint */
#define PROMPT_TIP_LEFTCLICK_TXT gettext_noop("Remember to use the left mouse button!")
#define PROMPT_TIP_LEFTCLICK_YES gettext_noop("OK")

/* Confirmation of successful (we hope) image export */
#define PROMPT_PICT_EXPORT_TXT gettext_noop("Your picture has been exported to \"%s\"!")
#define PROMPT_GIF_EXPORT_TXT gettext_noop("Your slideshow GIF has been exported to \"%s\"!")
#define PROMPT_EXPORT_YES gettext_noop("OK")

/* We got an error exporting */
#define PROMPT_PICT_EXPORT_FAILED_TXT gettext_noop("Sorry! Your picture could not be exported!")
#define PROMPT_GIF_EXPORT_FAILED_TXT gettext_noop("Sorry! Your slideshow GIF could not be exported!")

/* Confirmation of successful (we hope) image-to-template conversion */
#define PROMPT_PICT_TEMPLATE_TXT gettext_noop("Your picture is now available as a template in the “New“ dialog!")
#define PROMPT_TEMPLATE_YES gettext_noop("OK")

/* We got an error doing image-to-template conversion */
#define PROMPT_PICT_TEMPLATE_EXISTS_TXT gettext_noop("You already turned this picture into a template. Look for it in the “New“ dialog!")
#define PROMPT_PICT_TEMPLATE_FAILED_TXT gettext_noop("Sorry! Your picture could not turned into a template!")

/* Slideshow instructions */
#define TUX_TIP_SLIDESHOW gettext("Choose the pictures you want, then click “Play”.")


enum
{
  SHAPE_TOOL_MODE_STRETCH,
  SHAPE_TOOL_MODE_ROTATE,
  SHAPE_TOOL_MODE_DONE
};

int shape_reverse;


enum
{
  STAMP_TOOL_MODE_PLACE,
  STAMP_TOOL_MODE_ROTATE
};

#define STAMP_XOR_LINE_UNSET INT_MIN
on_screen_keyboard *new_kbd;
SDL_Rect kbd_rect;

#if (SDL_MAJOR_VERSION < 2)
#define TIMERID_NONE NULL
#else
#define TIMERID_NONE 0
#endif

int brushflag, xnew, ynew, eraflag, lineflag, magicflag, keybd_flag,
  keybd_position, keyglobal, initial_y, gen_key_flag, ide, activeflag, old_x, old_y;
int cur_thing;
SDL_TimerID scrolltimer_dialog = TIMERID_NONE;  /* Used by Open, Open->Slideshow, and New dialogs */
Uint32 TP_SDL_MOUSEBUTTONSCROLL;
Uint32 TP_USEREVENT_PLAYDESCSOUND;

/**
 * --- MAIN LOOP! ---
 */
static void mainloop(void)
{
  int done, val_x, val_y, valhat_x, valhat_y, new_x, new_y,
    shape_tool_mode, shape_start_x, shape_start_y, shape_current_x,
    shape_current_y, shape_ctr_x, shape_ctr_y, old_stamp_group, which;
  int num_things;
  int *thing_scroll;
  int do_draw;
  int ignoring_motion;
  int motioner = 0;
  int hatmotioner = 0;
  int whichc = 0;
  int whicht = 0;
  int line_start_x = 0;
  int line_start_y = 0;
  int stamp_size_selector_clicked = 0;
  int stamp_xored = 0;
  int stamp_xored_rt = 0;
  int stamp_place_x = 0;
  int stamp_place_y = 0;
  int stamp_tool_mode = STAMP_TOOL_MODE_PLACE;

#ifdef EXPERIMENT_STAMP_ROTATION_LINE
  int stamp_xor_line_old_x = STAMP_XOR_LINE_UNSET;
  int stamp_xor_line_old_y = STAMP_XOR_LINE_UNSET;
#endif

#ifdef AUTOSAVE_GOING_BACKGROUND
  char *fname;
  char tmp[1024];
  FILE *fi;
#endif

  TP_SDL_MOUSEBUTTONSCROLL = SDL_RegisterEvents(1);
  TP_USEREVENT_PLAYDESCSOUND = SDL_RegisterEvents(1);
  SDL_TimerID scrolltimer_selector = TIMERID_NONE, scrolltimer_tool = TIMERID_NONE;
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

  float angle;
  char angle_tool_text[256];    // FIXME Consider malloc'ing
  char stretch_tool_text[256];  // FIXME Consider malloc'ing

  num_things = num_brushes;
  thing_scroll = &brush_scroll;
  cur_thing = 0;
  do_draw = 0;
  old_x = 0;
  old_y = 0;
  which = 0;
  shape_start_x = 0;
  shape_start_y = 0;
  shape_current_x = 0;
  shape_current_y = 0;
  shape_ctr_x = 0;
  shape_ctr_y = 0;
  shape_tool_mode = SHAPE_TOOL_MODE_DONE;
  stamp_tool_mode = STAMP_TOOL_MODE_PLACE;
  button_down = 0;
  last_cursor_blink = cur_toggle_count = 0;
  texttool_len = 0;
  scrolling_selector = 0;
  scrolltimer_selector = TIMERID_NONE;
  scrolling_tool = 0;
  scrolltimer_tool = TIMERID_NONE;
  scrolling_dialog = 0;
  scrolltimer_dialog = TIMERID_NONE;
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

  if (NUM_TOOLS > buttons_tall * gd_tools.cols)
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
      {
        if (cur_tool == TOOL_STAMP && stamp_tool_mode == STAMP_TOOL_MODE_ROTATE)
          /* Discarding old stamp XORs, don't need to keep any outdated mouse motion event */
        {
          int rest = SDL_PeepEvents(NULL, 1000, SDL_PEEKEVENT, SDL_MOUSEMOTION,
                                    SDL_MOUSEMOTION);
          int i;

          for (i = 0; i < rest; i++)
          {
            SDL_PollEvent(&event);
            if (event.type != SDL_MOUSEMOTION)  /* But keep any other events that could be there */
              break;
          }
        }
        else
          ignoring_motion = (ignoring_motion + 1) % 3;  /* Ignore every couple of motion events, to keep things moving quickly (but avoid, e.g., attempts to draw "O" from looking like "D") */
      }

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
          entered_background = 1;
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
        redraw_tux_text();
        entered_background = 0;
      }
#endif
      else if (event.type == SDL_WINDOWEVENT)
      {
        /* Reset Shapes tool and clean the canvas if we lose focus */
        if (mouseaccessibility && emulate_button_pressed &&
            ((cur_tool == TOOL_SHAPES
              && shape_tool_mode != SHAPE_TOOL_MODE_DONE)
             || cur_tool == TOOL_LINES) && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
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
                (key_unicode > ' '
                 && key_unicode < 127) ? (char)event.text.text : ' ',
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
            reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);

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
            DEBUG_PRINTF("modstate at mainloop %d, mod %d\n", SDL_GetModState(), mod);

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
        else if (key == SDLK_z && (mod & KMOD_CTRL) && !noshortcuts
                 && !button_down && !emulate_button_pressed
                 && stamp_tool_mode != STAMP_TOOL_MODE_ROTATE && shape_tool_mode != SHAPE_TOOL_MODE_ROTATE)
        {
          /* Ctrl-Z - Undo */
          /* (As long as we're not in the middle of something!!!) */

          if (tool_avail[TOOL_UNDO])
          {
            magic_switchout(canvas);

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
                have_to_rec_label_node = SDL_TRUE;
                add_label_node(0, 0, 0, 0, NULL);
                derender_node(&label_node_to_edit);
                label_node_to_edit = NULL;
              }
            }
            if (cur_tool == TOOL_STAMP)
            {
              reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);
            }

            if (cur_undo == newest_undo)
            {
              rec_undo_buffer();
              do_undo();
            }
            do_undo();
            update_screen_rect(&r_tools);
            shape_tool_mode = SHAPE_TOOL_MODE_DONE;
            maybe_redraw_eraser_xor();

            magic_switchin(canvas);
          }
        }
        else if (key == SDLK_r && (mod & KMOD_CTRL) && !noshortcuts)
        {
          /* Ctrl-R - Redo */

          if (tool_avail[TOOL_REDO])
          {
            magic_switchout(canvas);

            if (cur_tool == TOOL_STAMP)
              reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);
            hide_blinking_cursor();
            do_redo();
            update_screen_rect(&r_tools);
            shape_tool_mode = SHAPE_TOOL_MODE_DONE;
            maybe_redraw_eraser_xor();

            magic_switchin(canvas);
          }
        }
        else if (key == SDLK_o && (mod & KMOD_CTRL) && !noshortcuts)
        {
          /* Ctrl-O - Open */

          magic_switchout(canvas);
          if (cur_tool == TOOL_STAMP)
            reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);

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
          else if (cur_tool == TOOL_FILL)
            draw_fills();

          draw_cur_tool_tip();

          /* FIXME: Make delay configurable: */
          control_drawtext_timer(1000, tool_tips[cur_tool], 0);
          /* FIXME: May need to use draw_cur_tool_tip() here? -bjk 2021.09.06 */

          magic_switchin(canvas);
        }
        else if ((key == SDLK_n && (mod & KMOD_CTRL)) && !noshortcuts)
        {
          /* Ctrl-N - New */

          magic_switchout(canvas);
          if (cur_tool == TOOL_STAMP)
            reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);

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
          else if (cur_tool == TOOL_FILL)
            draw_fills();

          update_screen_rect(&r_toolopt);
          update_screen_rect(&r_ttoolopt);
          magic_switchin(canvas);
        }
        else if (key == SDLK_s && (mod & KMOD_CTRL) && !noshortcuts)
        {
          /* Ctrl-S - Save */

          magic_switchout(canvas);
          hide_blinking_cursor();

          /* Only reset stamp XORs if there will be prompt */
          if (cur_tool == TOOL_STAMP && promptless_save == SAVE_OVER_PROMPT && file_id[0] != '\0')
            reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);

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
#ifdef __APPLE__
        else if (key == SDLK_p && (mod & KMOD_CTRL) && (mod & KMOD_SHIFT) && !noshortcuts)
        {
          /* Ctrl-Shft-P - Page Setup */
          if (!disable_print)
            DisplayPageSetup(canvas);
        }
#endif
        else if (key == SDLK_p && (mod & KMOD_CTRL) && !noshortcuts)
        {
          /* Ctrl-P - Print */

          if (!disable_print)
          {
            magic_switchout(canvas);
            if (cur_tool == TOOL_STAMP)
              reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);

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
        else if (((key == SDLK_v && (mod & KMOD_CTRL)) && !noshortcuts) || key == SDLK_PASTE)
        {
          /* Ctrl-V - Paste */
          if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
          {
            char *pasted_txt;

            if (SDL_HasClipboardText())
            {
              pasted_txt = SDL_GetClipboardText();
              if (pasted_txt != NULL /* it shouldn't be */ )
              {
                if (pasted_txt[0] != '\0')
                {
                  int n;
                  wchar_t *tmp;

                  DEBUG_PRINTF("Pasting: %s\n", pasted_txt);

                  n = strlen(pasted_txt) + 1;
                  tmp = alloca(sizeof(wchar_t) * n);

                  if (tmp != NULL)
                  {
                    int exceeded;
                    int i;

                    mbstowcs(tmp, pasted_txt, n);       /* at most n wchar_t written */
                    exceeded = 0;
                    for (i = 0; tmp[i] != '\0' && !exceeded; i++)
                    {
                      if (tmp[i] == '\n')
                      {
                        exceeded = text_label_tool_enter(TuxPaint_Font_FontHeight(getfonthandle(cur_font)));
                      }
                      else
                      {
                        int txt_width;

                        texttool_str[texttool_len++] = tmp[i];
                        playsound(screen, 0, SND_KEYCLICK, 0, SNDPOS_CENTER, SNDDIST_NEAR);
                        txt_width = do_render_cur_text(0);
                        if (cursor_x + txt_width > canvas->w && texttool_len > 1)
                        {
                          int best, j, rewind;

                          best = -1;

                          for (j = texttool_len - 1; j >= 0 && best == -1; j--)
                          {
                            if (texttool_str[j] == ' ')
                            {
                              best = j + 1;     /* +1 to eat the space */
                            }
                            else if (texttool_str[j] == '-')
                            {   /* FIXME: Also en-dash, em-dash, others? -bjk 2024.12.25 */
                              best = j;
                            }
                          }

                          if (best == -1)
                          {
                            best = texttool_len - 1;
                          }

                          rewind = texttool_len - best;

                          texttool_str[best] = '\0';
                          txt_width = do_render_cur_text(0);
                          exceeded = text_label_tool_enter(TuxPaint_Font_FontHeight(getfonthandle(cur_font)));
                          i = i - rewind;
                        }
                        SDL_Delay(10);
                      }
                    }
                  }
                }
                SDL_free(pasted_txt);
              }
            }
          }
        }
        else if (event.type == SDL_TEXTINPUT ||
                 (event.type == SDL_KEYDOWN &&
                  (event.key.keysym.sym == SDLK_BACKSPACE ||
                   event.key.keysym.sym == SDLK_RETURN
                   || event.key.keysym.sym == SDLK_TAB
                   || event.key.keysym.sym == SDLK_LALT || event.key.keysym.sym == SDLK_RALT)))
        {
          /* Handle special keys key in text tool: */

          if (((cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
               && cursor_x != -1 && cursor_y != -1) || (cur_tool == TOOL_LABEL
                                                        && (cur_label == LABEL_SELECT || cur_label == LABEL_APPLY)))
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
            wprintf
              (L"character 0x%04x %d <%lc> is %d pixels, %lsprintable, key_down 0x%x\n",
               event.key.keysym.unicode, event.key.keysym.unicode,
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
#ifdef __APPLE__
              /* Apple uses DEL for BACKSPACE */
              if (*im_cp == SDLK_DELETE)
                *im_cp = L'\b';
#endif

              if (*im_cp == L'\b')
              {
                /* [Backspace] */
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
                /* [Enter]... */

                int font_height;

                font_height = TuxPaint_Font_FontHeight(getfonthandle(cur_font));

                hide_blinking_cursor();
                if (texttool_len > 0)
                {
                  /* [Enter] to finish entering text */

                  text_label_tool_enter(font_height);
                }
                else if (cur_tool == TOOL_LABEL && label_node_to_edit)
                {
                  /* [Enter] to finish erasing text from a pre-existing Label */

                  rec_undo_buffer();
                  have_to_rec_label_node = SDL_TRUE;
                  add_label_node(0, 0, 0, 0, NULL);
                  derender_node(&label_node_to_edit);
                  label_node_to_edit = NULL;

                  playsound(screen, 0, SND_LINE_END, 0, SNDPOS_CENTER, SNDDIST_NEAR);

                  if (been_saved)
                  {
                    been_saved = 0;

                    if (!disable_save)
                      tool_avail[TOOL_SAVE] = 1;

                    draw_toolbar();
                    update_screen_rect(&r_tools);
                  }
                }

                else if (cur_tool == TOOL_LABEL && cur_label == LABEL_SELECT)
                {
                  /* [Enter] to select a node to edit */

                  DEBUG_PRINTF("Searching for label @ (%d+3,%d+3)\n",
                               highlighted_label_node->save_x, highlighted_label_node->save_y);

                  label_node_to_edit =
                    search_label_list(&highlighted_label_node,
                                      highlighted_label_node->save_x + 3, highlighted_label_node->save_y + 3, 0);

                  if (label_node_to_edit)
                  {
                    select_label_node(&old_x, &old_y);
                    DEBUG_PRINTF("Got a label: \"%ls\" @ (%d,%d)\n",
                                 label_node_to_edit->save_texttool_str, old_x, old_y);
                    DEBUG_PRINTF("Cursor now @ (%d,%d); width = %d\n", cursor_x, cursor_y, cursor_textwidth);
                    cursor_x = label_node_to_edit->save_x;
                    cursor_y = label_node_to_edit->save_y;
                    cursor_left = cursor_x;
                    DEBUG_PRINTF("Cursor now @ (%d,%d)\n", cursor_x, cursor_y);
                  }

                  do_render_cur_text(0);
                }

                else if (cur_tool == TOOL_LABEL && cur_label == LABEL_APPLY)
                {
                  /* [Enter] to select a node to apply it to the canvas */

                  label_node_to_edit =
                    search_label_list(&highlighted_label_node,
                                      highlighted_label_node->save_x + 3, highlighted_label_node->save_y + 3, 0);

                  if (label_node_to_edit)
                  {
                    reposition_onscreen_keyboard(old_y);
                    apply_label_node(highlighted_label_node->save_x, highlighted_label_node->save_y);
                    do_render_cur_text(0);
                  }
                }
                else
                {
                  /* [Enter] with no text; just move insertion cursor down to the next 'line' */

                  cursor_x = cursor_left;
                  cursor_y = min(cursor_y + font_height, canvas->h - font_height);

                  /* Reposition the on-screen keyboard if we begin typing over it */
                  update_canvas_ex(kbd_rect.x, kbd_rect.y, kbd_rect.x + kbd_rect.w, kbd_rect.y + kbd_rect.h, 0);
                  update_screen_rect(&kbd_rect);
                  reposition_onscreen_keyboard(cursor_y);

                  playsound(screen, 0, SND_RETURN, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
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
                /* [Tab]... */

                if (texttool_len > 0)
                {
                  /* [Tab] to finish entering text */

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

                  playsound(screen, 0, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
                }
                else if (cur_tool == TOOL_LABEL && label_node_to_edit)
                {
                  /* [Tab] to finish erasing text from a pre-existing Label */

                  rec_undo_buffer();
                  have_to_rec_label_node = SDL_TRUE;
                  add_label_node(0, 0, 0, 0, NULL);
                  derender_node(&label_node_to_edit);
                  label_node_to_edit = NULL;

                  playsound(screen, 0, SND_LINE_END, 0, SNDPOS_CENTER, SNDDIST_NEAR);

                  if (been_saved)
                  {
                    been_saved = 0;

                    if (!disable_save)
                      tool_avail[TOOL_SAVE] = 1;

                    draw_toolbar();
                    update_screen_rect(&r_tools);
                  }
                }
                else if (cur_tool == TOOL_LABEL && (cur_label == LABEL_SELECT || cur_label == LABEL_APPLY))
                {
                  /* [Tab] to cycle between the Labels (nodes) */

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
                /* Printable characters... */

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


                  if (cursor_x + old_cursor_textwidth <= canvas->w - 50 && cursor_x + cursor_textwidth > canvas->w - 50)
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
            }                   /* while(*im_cp) */

            /* Show IM tip text */
            if (im_data.tip_text)
            {
              draw_tux_text(TUX_DEFAULT, im_data.tip_text, 1);
            }

          }
        }
      }
      else if (event.type == SDL_JOYAXISMOTION)
      {
        handle_joyaxismotion(event, &motioner, &val_x, &val_y);
      }
      else if (event.type == SDL_JOYHATMOTION)
      {
        handle_joyhatmotion(event, oldpos_x, oldpos_y, &valhat_x, &valhat_y, &hatmotioner, &old_hat_ticks);
      }
      else if (event.type == SDL_JOYBALLMOTION)
      {
        handle_joyballmotion(event, oldpos_x, oldpos_y);
      }
      else if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP)
      {
        handle_joybuttonupdownscl(event, oldpos_x, oldpos_y, real_r_tools);
      }
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

            if (whicht < NUM_TOOLS && tool_avail[whicht] && (valid_click(event.button.button) || whicht == TOOL_PRINT))
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
                    have_to_rec_label_node = SDL_TRUE;
                    add_label_node(0, 0, 0, 0, NULL);
                    derender_node(&label_node_to_edit);
                    label_node_to_edit = NULL;
                  }
                }
              }
              update_canvas(0, 0, WINDOW_WIDTH - r_ttoolopt.w, (button_h * buttons_tall) + r_ttools.h);

              old_tool = cur_tool;
              cur_tool = whicht;
              draw_toolbar();
              update_screen_rect(&r_tools);
              DEBUG_PRINTF("screenrectr_tools %d, %d, %d, %d\n", r_tools.x, r_tools.y, r_tools.w, r_tools.h);
              playsound(screen, 1, SND_CLICK, 0, SNDPOS_LEFT, SNDDIST_NEAR);

              /* FIXME: this "if" is just plain gross */
              if (cur_tool != TOOL_TEXT)
                draw_cur_tool_tip();

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
                draw_colors(stamp_colorable(cur_stamp[stamp_group]) || stamp_tintable(cur_stamp[stamp_group]));
                reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);
                set_active_stamp();
                update_stamp_xor(0);
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
              else if (cur_tool == TOOL_FILL)
              {
                keybd_flag = 0;
                cur_thing = cur_fill;
                num_things = NUM_FILLS;
                thing_scroll = &fill_scroll;
                draw_fills();
                draw_colors(fill_color[cur_fill]);
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
                  if (kbd == NULL)
                  {
                    if (onscreen_keyboard_layout)
                      kbd =
                        osk_create(onscreen_keyboard_layout, canvas,
                                   img_btn_up, img_btn_down, img_btn_off,
                                   img_btn_nav, img_btn_hold,
                                   img_oskdel, img_osktab, img_oskenter,
                                   img_oskcapslock, img_oskshift, img_oskpaste, onscreen_keyboard_disable_change);
                    else
                      kbd =
                        osk_create(strdup("default.layout"), canvas,
                                   img_btn_up, img_btn_down, img_btn_off,
                                   img_btn_nav, img_btn_hold,
                                   img_oskdel, img_osktab, img_oskenter,
                                   img_oskcapslock, img_oskshift, img_oskpaste, onscreen_keyboard_disable_change);
                  }

                  if (kbd == NULL)
                    fprintf(stderr, "Warning: kbd = NULL\n");
                  else
                    reposition_onscreen_keyboard(0);
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
                if (onscreen_keyboard && !kbd)
                {
                  r_tir.y = (float)event.button.y / render_scale;
                  SDL_SetTextInputRect(&r_tir);
                  SDL_StartTextInput();
                }
                draw_cur_tool_tip();

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
                cur_thing = cur_magic[magic_group];
                num_things = num_magics[magic_group];
                thing_scroll = &(magic_scroll[magic_group]);
                magic_current_snd_ptr = NULL;
                draw_magic();
                draw_colors(magics[magic_group][cur_thing].colors);

                if (magics[magic_group][cur_thing].colors)
                  magic_set_color();
                if (magics[magic_group][cur_thing].sizes[magic_modeint(magics[magic_group][cur_thing].mode)])
                  magic_set_size();
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
                reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);
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
                reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);
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

                draw_cur_tool_tip();

                draw_colors(COLORSEL_REFRESH);

                if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_LINES)
                  draw_brushes();
                else if (cur_tool == TOOL_MAGIC)
                  draw_magic();
                else if (cur_tool == TOOL_STAMP)
                {
                  draw_stamps();
                  reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);
                }
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
                else if (cur_tool == TOOL_FILL)
                  draw_fills();
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
                else if (old_tool == TOOL_STAMP)
                  reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);

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
                {
                  draw_stamps();
                  reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);
                }
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
                else if (cur_tool == TOOL_FILL)
                  draw_fills();
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
                else if (old_tool == TOOL_STAMP)
                  reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);

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
                else if (old_tool == TOOL_STAMP)
                  reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);

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
          else if (((event.button.y < r_tools.y + button_h / 2)
                    && tool_scroll > 0)
                   || ((event.button.y > real_r_tools.y + real_r_tools.h)
                       && (tool_scroll < NUM_TOOLS - buttons_tall * gd_tools.cols + gd_tools.cols)))
          {
            /* Tool up or down scroll buttons */

            if (event.button.y < r_tools.y + button_h / 2)
            {
              /* Tool up scroll button */
              tool_scroll -= gd_tools.cols;
              playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

              draw_toolbar();
              update_screen_rect(&r_tools);
            }
            else
            {
              /* Tool down scroll button */
              tool_scroll += gd_tools.cols;
              draw_toolbar();
              playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

              update_screen_rect(&r_tools);
            }

            if (!scrolling_tool && event.type == SDL_MOUSEBUTTONDOWN)
            {
              DEBUG_PRINTF("Starting scrolling\n");
              memcpy(&scrolltimer_tool_event, &event, sizeof(SDL_Event));
              scrolltimer_tool_event.type = TP_SDL_MOUSEBUTTONSCROLL;

              /*
               * We enable the timer subsystem only when needed (e.g., to use SDL_AddTimer() needed
               * for scrolling) then disable it immediately after (e.g., after the timer has fired or
               * after SDL_RemoveTimer()) because enabling the timer subsystem in SDL1 has a high
               * energy impact on the Mac.
               */

              scrolling_tool = 1;
              scrolltimer_tool = SDL_AddTimer(REPEAT_SPEED, scrolltimer_tool_callback, (void *)&scrolltimer_tool_event);
            }
            else
            {
              DEBUG_PRINTF("Continuing scrolling\n");
              scrolltimer_tool =
                SDL_AddTimer(REPEAT_SPEED / 3, scrolltimer_tool_callback, (void *)&scrolltimer_tool_event);
            }
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
              cur_tool == TOOL_ERASER || cur_tool == TOOL_LABEL || cur_tool == TOOL_FILL)
          {
            int num_rows_needed;
            SDL_Rect r_controls;
            SDL_Rect r_notcontrols;
            SDL_Rect r_items;   /* = r_notcontrols; */
            int toolopt_changed;
            int select_changed = 0;
            grid_dims gd_controls;      /* might become 2-by-2 */
            grid_dims gd_items; /* generally becoming 2-by-whatever */

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
                /* Account for stamp controls and group changing (left/right) buttons */
                if (!no_stamp_rotation)
                {
                  gd_controls.rows = 4;
                }
                else
                {
                  gd_controls.rows = 3;
                }
                gd_controls.cols = 2;
              }
              else
              {
                /* Stamp controls are disabled; account for group changing (left/right) buttons */
                gd_controls.rows = 1;
                gd_controls.cols = 2;
              }
            }
            else if (cur_tool == TOOL_TEXT)
            {
              if (!disable_stamp_controls)
              {
                /* Account for text controls */
                gd_controls.rows = 2;
                gd_controls.cols = 2;
              }
            }
            else if (cur_tool == TOOL_LABEL)
            {
              if (!disable_stamp_controls)
              {
                /* Account for text controls and label selection button */
                gd_controls.rows = 3;
                gd_controls.cols = 2;
              }
              else
              {
                /* Text controls disabled; only account for label selection button */
                gd_controls.rows = 1;
                gd_controls.cols = 2;
              }
            }
            else if (cur_tool == TOOL_MAGIC)
            {
              gd_controls.cols = 2;
              gd_controls.rows = calc_magic_control_rows();
            }
            else if (cur_tool == TOOL_SHAPES)
            {
              if (!disable_shape_controls)
              {
                /* Account for shape controls (corner- vs center-based expansion) */
                gd_controls.rows = 1;
                gd_controls.cols = 2;
              }
            }

            else if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_LINES)
            {
              if (!disable_brushspacing)
              {
                /* Account for brush spacing */
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
              /* A selection button was clicked... */
              which = GRIDHIT_GD(r_items, gd_items) + *thing_scroll;

              if (which < num_things)
              {
                /* ...and there was something there to click */
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
              /* Controls were clicked */

              which = GRIDHIT_GD(r_controls, gd_controls);

              if (cur_tool == TOOL_STAMP)
              {
                if (no_stamp_rotation && which > 1)
                {
                  /* No column for stamp rotation control, pretend the lower buttons are lower */
                  which += 2;
                }

                if (stamp_tool_mode == STAMP_TOOL_MODE_ROTATE)
                {
                  stamp_xor(stamp_place_x, stamp_place_y);
                  reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);
                }

                /* Stamp controls! */
                int control_sound = -1;

                if (which == 6 || which == 7)
                {
                  /* Grow/Shrink Controls: */
                  int old_size;

#ifdef DEBUG
                  float choice;
#endif

                  old_size = stamp_data[stamp_group][cur_stamp[stamp_group]]->size;

                  stamp_data[stamp_group][cur_stamp[stamp_group]]->size = (((MAX_STAMP_SIZE - MIN_STAMP_SIZE + 1
                                                                             /* +1 to address lack of ability to get back to max default stamp size (SF Bug #1668235 -bjk 2011.01.08) */
                                                                            ) * (event.button.x -
                                                                                 (WINDOW_WIDTH -
                                                                                  r_ttoolopt.w))) / r_ttoolopt.w) +
                    MIN_STAMP_SIZE;

                  DEBUG_PRINTF("Old size = %d, Chose %0.4f, New size =%d\n",
                               old_size, choice, stamp_data[stamp_group][cur_stamp[stamp_group]]->size);

                  if (stamp_data[stamp_group][cur_stamp[stamp_group]]->size < old_size)
                    control_sound = SND_SHRINK;
                  else if (stamp_data[stamp_group][cur_stamp[stamp_group]]->size > old_size)
                    control_sound = SND_GROW;
                }
                else if (which == 4 || which == 5)
                {
                  /* Mirror/Flip Controls: */
                  if (which == 5)
                  {
                    /* Top right button: Flip: */
                    if (stamp_data[stamp_group][cur_stamp[stamp_group]]->flipable)
                    {
                      stamp_data[stamp_group][cur_stamp[stamp_group]]->flipped
                        = !stamp_data[stamp_group][cur_stamp[stamp_group]]->flipped;
                      control_sound = SND_FLIP;
                    }
                  }
                  else
                  {
                    /* Top left button: Mirror: */
                    if (stamp_data[stamp_group][cur_stamp[stamp_group]]->mirrorable)
                    {
                      stamp_data[stamp_group][cur_stamp
                                              [stamp_group]]->mirrored =
                        !stamp_data[stamp_group][cur_stamp[stamp_group]]->mirrored;
                      control_sound = SND_MIRROR;
                    }
                  }
                }
                else if (which == 2 && !no_stamp_rotation)
                {
                  stamp_rotation_ctrl = !stamp_rotation_ctrl;
                  control_sound = SND_FLIP;
                }
                else if (which == 3)
                {
                  /* No-op */
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
                  update_stamp_xor(0);
                  stamp_tool_mode = STAMP_TOOL_MODE_PLACE;
                }
              }
              else if (cur_tool == TOOL_MAGIC)
              {
                /* Magic controls */

                int grp;
                int cur;

                grp = magic_group;
                cur = cur_magic[grp];

                if (no_magic_groups)
                  which += 2;

                if (which == 0 || which == 1)
                {
                  int tries = 0;

                  magic_switchout(canvas);

                  /* Magic pagination */
                  do
                  {
                    tries++;

                    if (which == 0)
                    {
                      magic_group--;
                      if (magic_group < 0)
                        magic_group = MAX_MAGIC_GROUPS - 1;
                    }
                    else if (which == 1)
                    {
                      magic_group++;
                      if (magic_group >= MAX_MAGIC_GROUPS)
                        magic_group = 0;
                    }
                  }
                  while (num_magics[magic_group] == 0 && tries < MAX_MAGIC_GROUPS);

                  keybd_flag = 0;
                  cur_thing = cur_magic[magic_group];
                  num_things = num_magics[magic_group];
                  thing_scroll = &(magic_scroll[magic_group]);
                  magic_current_snd_ptr = NULL;

                  draw_magic();
                  update_screen_rect(&r_toolopt);

                  draw_colors(magics[magic_group][cur_thing].colors);

                  if (magics[magic_group][cur_thing].colors)
                    magic_set_color();
                  if (magics[magic_group][cur_thing].sizes[magic_modeint(magics[magic_group][cur_thing].mode)])
                    magic_set_size();

                  magic_switchin(canvas);

                  playsound(screen, 0, SND_CLICK, 0, SNDPOS_CENTER, SNDDIST_NEAR);
                }
                else
                {
                  if (!disable_magic_controls && (which == 2 || which == 3))
                  {
                    /* Magic controls! */
                    if (which == 3 && magics[grp][cur].avail_modes & MODE_FULLSCREEN)
                    {
                      magic_switchout(canvas);
                      magics[grp][cur].mode = MODE_FULLSCREEN;
                      magic_switchin(canvas);
                      draw_magic();
                      update_screen_rect(&r_toolopt);
                    }
                    else if (which == 2 && magics[grp][cur].avail_modes & MODE_PAINT)
                    {
                      magic_switchout(canvas);
                      magics[grp][cur].mode = MODE_PAINT;
                      magic_switchin(canvas);
                      draw_magic();
                      update_screen_rect(&r_toolopt);
                    }
                    else if (which == 2 && magics[grp][cur].avail_modes & MODE_PAINT_WITH_PREVIEW)
                    {
                      magic_switchout(canvas);
                      magics[grp][cur].mode = MODE_PAINT_WITH_PREVIEW;
                      magic_switchin(canvas);
                      draw_magic();
                      update_screen_rect(&r_toolopt);
                    }
                    else if (which == 2 && magics[grp][cur].avail_modes & MODE_ONECLICK)
                    {
                      magic_switchout(canvas);
                      magics[grp][cur].mode = MODE_ONECLICK;
                      magic_switchin(canvas);
                      draw_magic();
                      update_screen_rect(&r_toolopt);
                    }
                    playsound(screen, 0, SND_CLICK, 0, SNDPOS_CENTER, SNDDIST_NEAR);

                    if (magics[grp][cur].sizes[magic_modeint(magics[grp][cur].mode)])
                    {
                      DEBUG_PRINTF
                        ("group %d thing %d in mode %04x (%d) has %d sizes; size is %d\n",
                         grp, cur, magics[grp][cur].mode,
                         magic_modeint(magics[grp][cur].mode),
                         magics[grp][cur].sizes[magic_modeint(magics[grp][cur].mode)],
                         magics[grp][cur].size[magic_modeint(magics[grp][cur].mode)]);
                      magic_set_size();
                    }
                  }
                  else if (!disable_magic_sizes)
                  {
                    int mode;

                    mode = magic_modeint(magics[grp][cur].mode);

                    if (magics[grp][cur].sizes[mode] > 1)
                    {
                      int old_size, new_size;

                      old_size = magics[grp][cur].size[mode];

                      new_size =
                        ((magics[grp][cur].sizes[mode] *
                          (event.button.x - (WINDOW_WIDTH - r_ttoolopt.w))) / r_ttoolopt.w) + 1;

                      if (new_size != old_size)
                      {
                        magics[grp][cur].size[mode] = new_size;
                        magic_set_size();

                        draw_magic();
                        update_screen_rect(&r_toolopt);

                        if (new_size < old_size)
                          playsound(screen, 0, SND_SHRINK, 0, SNDPOS_CENTER, SNDDIST_NEAR);
                        else
                          playsound(screen, 0, SND_GROW, 0, SNDPOS_CENTER, SNDDIST_NEAR);
                      }
                    }
                  }
                }
              }
              else if (cur_tool == TOOL_SHAPES)
              {
                /* Shape controls! */
                shape_mode = which;
                draw_shapes();
                update_screen_rect(&r_toolopt);
                draw_tux_text(TUX_GREAT, shapemode_tips[shape_mode], 1);
                playsound(screen, 0, SND_CLICK, 0, SNDPOS_RIGHT, SNDDIST_NEAR);
                update_screen_rect(&r_tuxarea);
                toolopt_changed = 0;
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
              else if (cur_tool == TOOL_LABEL)
              {
                /* Label controls! */
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
                      /* right button: Italic: */
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
                      /* Top right: Select button: */
                      if (cur_label == LABEL_SELECT)
                      {
                        /* Already in label select mode; turn it off */
                        cur_label = LABEL_LABEL;
                        update_canvas(0, 0, WINDOW_WIDTH - r_ttoolopt.w, (button_h * buttons_tall) + r_ttoolopt.h);
                        if (onscreen_keyboard)
                        {
                          SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                          update_screen_rect(&kbd_rect);
                        }

                        if (onscreen_keyboard && !kbd)
                        {
                          SDL_StartTextInput();

                        }
                        draw_tux_text(TUX_GREAT, tool_tips[TOOL_LABEL], 1);
                        playsound(screen, 1, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
                      }
                      else
                      {
                        /* Want to select a label */
                        if (are_labels())
                        {
                          update_canvas_ex_r(kbd_rect.x - r_ttools.w,
                                             kbd_rect.y, kbd_rect.x + kbd_rect.w, kbd_rect.y + kbd_rect.h, 1);
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
                            have_to_rec_label_node = SDL_TRUE;
                            add_label_node(0, 0, 0, 0, NULL);
                            label_node_to_edit = NULL;
                          }

                          cur_label = LABEL_SELECT;
                          highlight_label_nodes();

                          draw_tux_text(TUX_GREAT, TIP_LABEL_SELECTOR_ENABLED, 1);
                          playsound(screen, 1, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
                        }
                      }
                      toolopt_changed = 1;
                    }
                    else
                    {
                      /* Top left: "Apply" label */
                      if (cur_label == LABEL_APPLY)
                      {
                        /* Already in label apply mode; turn it off */
                        cur_label = LABEL_LABEL;
                        update_canvas(0, 0, WINDOW_WIDTH - r_ttoolopt.w, (button_h * buttons_tall) + r_ttoolopt.h);
                        if (onscreen_keyboard)
                        {
                          SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                          update_screen_rect(&kbd_rect);
                        }

                        draw_tux_text(TUX_GREAT, tool_tips[TOOL_LABEL], 1);
                        playsound(screen, 1, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
                      }
                      else
                      {
                        /* Want to apply a label */
                        if (are_labels())
                        {
                          update_canvas_ex_r(kbd_rect.x - r_ttools.w,
                                             kbd_rect.y, kbd_rect.x + kbd_rect.w, kbd_rect.y + kbd_rect.h, 1);
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
                            have_to_rec_label_node = SDL_TRUE;
                            add_label_node(0, 0, 0, 0, NULL);
                            label_node_to_edit = NULL;
                          }

                          cur_label = LABEL_APPLY;
                          highlight_label_nodes();

                          draw_tux_text(TUX_GREAT, TIP_LABEL_APPLIER_ENABLED, 1);
                          playsound(screen, 1, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
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
              else if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_LINES)
              {
                /* Brush spacing */

                int prev_size, chosen, new_size, frame_w, w, h, control_sound;
                int next_new_size, prev_new_size;
                int strike = event.button.x - r_ttoolopt.x;

                prev_size = brushes_spacing[cur_brush];
                chosen = ((BRUSH_SPACING_SIZES * strike) / r_ttoolopt.w);

                frame_w = img_brushes[cur_brush]->w / abs(brushes_frames[cur_brush]);
                w = frame_w / (brushes_directional[cur_brush] ? 3 : 1);
                h = img_brushes[cur_brush]->h / (brushes_directional[cur_brush] ? 3 : 1);

                /* Spacing ranges from 0px to "N x the max dimension of the brush"
                   (so a 48x48 brush would have a spacing of 48 if the center option is chosen) */
                if (chosen == 0)
                {
                  new_size = 0;
                }
                else
                {
                  new_size = (chosen * max(w, h) * BRUSH_SPACING_MAX_MULTIPLIER) / (BRUSH_SPACING_SIZES - 1);
                }

                if (new_size != brushes_spacing_default[cur_brush])
                {
                  prev_new_size = ((chosen - 1) * max(w, h) * BRUSH_SPACING_MAX_MULTIPLIER) / (BRUSH_SPACING_SIZES - 1);
                  next_new_size = ((chosen + 1) * max(w, h) * BRUSH_SPACING_MAX_MULTIPLIER) / (BRUSH_SPACING_SIZES - 1);

                  if (prev_new_size < brushes_spacing_default[cur_brush] &&
                      next_new_size > brushes_spacing_default[cur_brush])
                  {
                    DEBUG_PRINTF
                      ("Nudging %d brush spacing to my default: %d\n", new_size, brushes_spacing_default[cur_brush]);
                    new_size = brushes_spacing_default[cur_brush];
                  }
                }


                if (new_size != prev_size)
                {
                  char tmp_tip[256];
                  int numer, denom;

                  brushes_spacing[cur_brush] = new_size;
                  draw_brushes_spacing();
                  update_screen_rect(&r_toolopt);

                  if (new_size < prev_size)
                    control_sound = SND_SHRINK;
                  else
                    control_sound = SND_GROW;

                  /* Show a message about the brush spacing */
                  if (new_size == 0)
                  {
                    /* Smallest spacing (0px) */
                    draw_tux_text(TUX_GREAT, TIP_BRUSH_SPACING_ZERO, 1);
                  }
                  else if (new_size / max(w, h) == 1)
                  {
                    /* Spacing is the same size as the brush */
                    draw_tux_text(TUX_GREAT, TIP_BRUSH_SPACING_SAME, 1);
                  }
                  else if (new_size > max(w, h))
                  {
                    /* Spacing is larger than the brush */
                    double ratio, i, f;

                    ratio = (float)new_size / (float)max(w, h);
                    f = modf(ratio, &i);

                    if (f > (SLOPPY_FRAC_MAX - SLOPPY_FRAC_MIN) / SLOPPY_FRAC_MAX)
                    {
                      i++;
                      f = 0.0;
                    }
                    else if (f < SLOPPY_FRAC_MIN / SLOPPY_FRAC_MAX)
                    {
                      f = 0.0;
                    }

                    if (f == 0.0)
                    {
                      /* Spacing ratio has no fractional part (e.g., "...4 times as big...") */
                      snprintf(tmp_tip, sizeof(tmp_tip), gettext(TIP_BRUSH_SPACING_MORE), (int)i);
                    }
                    else
                    {
                      /* Spacing ratio has a fractional part (e.g., "... 2 1/2 times as big...") */
                      sloppy_frac(f, &numer, &denom);

                      snprintf(tmp_tip, sizeof(tmp_tip), gettext(TIP_BRUSH_SPACING_MORE_FRAC), (int)i, numer, denom);
                    }

                    draw_tux_text(TUX_GREAT, tmp_tip, 1);
                  }
                  else if (new_size < max(w, h))
                  {
                    /* Spacing is smaller than the brush (e.g., "... 1/3 as big...") */
                    sloppy_frac((float)new_size / (float)max(w, h), &numer, &denom);
                    snprintf(tmp_tip, sizeof(tmp_tip), gettext(TIP_BRUSH_SPACING_LESS), numer, denom);
                    draw_tux_text(TUX_GREAT, tmp_tip, 1);
                  }

                  playsound(screen, 0, control_sound, 0, SNDPOS_CENTER, SNDDIST_NEAR);
                }
              }
            }
            else
            {
              /* Scroll buttons */
              int is_upper = event.button.y < r_toolopt.y + button_h / 2;

              if ((is_upper && *thing_scroll > 0)       /* upper arrow */
                  || (!is_upper && *thing_scroll / gd_items.cols < num_rows_needed - gd_items.rows)     /* lower arrow */
                )
              {
                *thing_scroll += is_upper ? -gd_items.cols : gd_items.cols;
                do_draw = 1;
                playsound(screen, 1, SND_SCROLL, 1, SNDPOS_RIGHT, SNDDIST_NEAR);

                if (scrolltimer_selector != TIMERID_NONE)
                {
                  SDL_RemoveTimer(scrolltimer_selector);
                  scrolltimer_selector = TIMERID_NONE;
                }

                if (!scrolling_selector && event.type == SDL_MOUSEBUTTONDOWN)
                {
                  DEBUG_PRINTF("Starting scrolling\n");
                  memcpy(&scrolltimer_selector_event, &event, sizeof(SDL_Event));
                  scrolltimer_selector_event.type = TP_SDL_MOUSEBUTTONSCROLL;

                  /*
                   * We enable the timer subsystem only when needed (e.g., to use SDL_AddTimer() needed
                   * for scrolling) then disable it immediately after (e.g., after the timer has fired or
                   * after SDL_RemoveTimer()) because enabling the timer subsystem in SDL1 has a high
                   * energy impact on the Mac.
                   */

                  scrolling_selector = 1;
                  scrolltimer_selector =
                    SDL_AddTimer(REPEAT_SPEED, scrolltimer_selector_callback, (void *)&scrolltimer_selector_event);
                }
                else
                {
                  DEBUG_PRINTF("Continuing scrolling\n");
                  scrolltimer_selector =
                    SDL_AddTimer(REPEAT_SPEED / 3, scrolltimer_selector_callback, (void *)&scrolltimer_selector_event);
                }

                if (*thing_scroll == 0 || *thing_scroll / gd_items.cols == num_rows_needed - gd_items.rows)
                {
                  do_setcursor(cursor_arrow);
                  if (scrolling_selector)
                  {
                    if (scrolltimer_selector != TIMERID_NONE)
                    {
                      SDL_RemoveTimer(scrolltimer_selector);
                      scrolltimer_selector = TIMERID_NONE;
                    }
                    scrolling_selector = 0;
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
              {
                draw_brushes();
                show_brush_tip();
              }
            }
            else if (cur_tool == TOOL_ERASER)
            {
              cur_eraser = cur_thing;

              if (do_draw)
                draw_erasers();
            }
            else if (cur_tool == TOOL_FILL)
            {
              cur_fill = cur_thing;
              draw_tux_text(TUX_GREAT, fill_tips[cur_fill], 1);

              if (do_draw)
              {
                draw_fills();
                draw_colors(fill_color[cur_fill]);
              }
            }
            else if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
            {
              char font_tux_text[1024];
              const char *fmt_str;

              cur_font = cur_thing;

              /* Show a message about the chosen font */
              if (cur_tool == TOOL_TEXT)
                fmt_str = TIP_TEXT_FONTCHANGE;
              else
                fmt_str = TIP_LABEL_FONTCHANGE;

              safe_snprintf(font_tux_text, sizeof(font_tux_text),
                            gettext(fmt_str),
                            TTF_FontFaceFamilyName(getfonthandle(cur_font)->ttf_font),
                            TTF_FontFaceStyleName(getfonthandle(cur_font)->ttf_font), getfonthandle(cur_font)->height);
              draw_tux_text(TUX_GREAT, font_tux_text, 1);

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
                /* If the sound hasn't been loaded yet, do it now */

                if (!stamp_data[stamp_group][cur_thing]->sound_processed)
                {
                  get_stamp_thumb(stamp_data[stamp_group][cur_thing], 1);
                }

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
                if (stamp_tool_mode == STAMP_TOOL_MODE_ROTATE)
                {
                  stamp_xor(stamp_place_x, stamp_place_y);
                  reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);
                }
                cur_stamp[stamp_group] = cur_thing;
                set_active_stamp();
                update_stamp_xor(0);
                stamp_tool_mode = STAMP_TOOL_MODE_PLACE;
              }

              if (do_draw)
                draw_stamps();

              if (stamp_data[stamp_group][cur_stamp[stamp_group]]->stxt != NULL)
              {
                DEBUG_PRINTF
                  ("stamp_data[stamp_group][cur_stamp[stamp_group]]->stxt = %s\n",
                   stamp_data[stamp_group][cur_stamp[stamp_group]]->stxt);

                draw_tux_text_ex(TUX_GREAT,
                                 stamp_data[stamp_group][cur_stamp
                                                         [stamp_group]]->stxt,
                                 1, stamp_data[stamp_group][cur_stamp[stamp_group]]->locale_text);
              }
              else
                draw_tux_text(TUX_GREAT, "", 0);

              /* Enable or disable color selector: */
              draw_colors(stamp_colorable(cur_stamp[stamp_group]) || stamp_tintable(cur_stamp[stamp_group]));
              if (!scrolling_selector)
              {
                stamp_xor(canvas->w / 2, canvas->h / 2);
                stamp_xored = 1;
                stamp_size_selector_clicked = 1;
                update_screen(canvas->w / 2 - (CUR_STAMP_W + 1) / 2 +
                              r_canvas.x,
                              canvas->h / 2 - (CUR_STAMP_H + 1) / 2 +
                              r_canvas.y,
                              canvas->w / 2 + (CUR_STAMP_W + 1) / 2 +
                              r_canvas.x, canvas->h / 2 + (CUR_STAMP_H + 1) / 2 + r_canvas.y);
              }
            }
            else if (cur_tool == TOOL_SHAPES)
            {
              cur_shape = cur_thing;

              /* Remove ghost previews and reset the tool */
              if (shape_tool_mode != SHAPE_TOOL_MODE_DONE)
              {
                shape_tool_mode = SHAPE_TOOL_MODE_DONE;
                do_undo();
                tool_avail[TOOL_REDO] = 0;      /* Don't let them 'redo' to get preview back */
                draw_toolbar();
                update_screen_rect(&r_tools);
                update_canvas(0, 0, canvas->w, canvas->h);
              }

              if (toolopt_changed)
                draw_tux_text(TUX_GREAT, shape_tips[cur_shape], 1);

              if (do_draw)
                draw_shapes();
            }
            else if (cur_tool == TOOL_MAGIC)
            {
              int grp;
              int cur;

              grp = magic_group;
              cur = cur_magic[grp];

              if (cur_thing != cur)
              {
                cur = cur_thing;
                magic_switchout(canvas);

                cur_magic[grp] = cur_thing;
                draw_colors(magics[grp][cur].colors);

                if (magics[grp][cur].colors)
                  magic_set_color();
                if (magics[grp][cur].sizes[magic_modeint(magics[grp][cur].mode)])
                  magic_set_size();

                magic_switchin(canvas);
              }

              draw_tux_text(TUX_GREAT, magics[grp][cur].tip[magic_modeint(magics[grp][cur].mode)], 1);

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
              int old_color;

              old_color = cur_color;
              cur_color = whichc;
              draw_tux_text(TUX_KISS, color_names[cur_color], 1);

              if (cur_color == (unsigned)COLOR_PICKER
                  || cur_color == (unsigned)COLOR_SELECTOR || cur_color == (unsigned)COLOR_MIXER)
              {
                int chose_color;

                disable_avail_tools();
                draw_toolbar();
                draw_colors(COLORSEL_CLOBBER_WIPE);
                draw_none();

                chose_color = 0;
                if (cur_color == (unsigned)COLOR_PICKER)
                  chose_color = do_color_picker(old_color);
                else if (cur_color == (unsigned)COLOR_SELECTOR)
                  chose_color = do_color_sel(0);
                else if (cur_color == (unsigned)COLOR_MIXER)
                {
                  chose_color = do_color_mix();
                  if (!chose_color)
                    cur_color = old_color;
                }

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

                draw_cur_tool_tip();

                draw_colors(COLORSEL_FORCE_REDRAW);

                if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_LINES)
                  draw_brushes();
                else if (cur_tool == TOOL_MAGIC)
                  draw_magic();
                else if (cur_tool == TOOL_STAMP)
                {
                  if (stamp_tool_mode == STAMP_TOOL_MODE_ROTATE)
                  {
                    stamp_xor(stamp_place_x, stamp_place_y);
                  }
                  else if (stamp_xored)
                  {
                    stamp_xor(canvas->w / 2, canvas->h / 2);
                    stamp_xored = 0;
                    update_screen(canvas->w / 2 - (CUR_STAMP_W + 1) / 2 +
                                  r_canvas.x,
                                  canvas->h / 2 - (CUR_STAMP_H + 1) / 2 +
                                  r_canvas.y,
                                  canvas->w / 2 + (CUR_STAMP_W + 1) / 2 +
                                  r_canvas.x, canvas->h / 2 + (CUR_STAMP_H + 1) / 2 + r_canvas.y);
                  }
                  draw_stamps();
                }
                else if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
                  draw_fonts();
                else if (cur_tool == TOOL_SHAPES)
                  draw_shapes();
                else if (cur_tool == TOOL_ERASER)
                  draw_erasers();
                else if (cur_tool == TOOL_FILL)
                  draw_fills();

                if (chose_color)
                  playsound(screen, 1, SND_BUBBLE, 1, SNDPOS_CENTER, SNDDIST_NEAR);
                else
                  playsound(screen, 1, SND_CLICK, 1, SNDPOS_CENTER, SNDDIST_NEAR);

                SDL_Flip(screen);
              }
              else
              {
                draw_colors(COLORSEL_REFRESH);

                playsound(screen, 1, SND_BUBBLE, 1, event.button.x, SNDDIST_NEAR);
              }

              handle_color_changed();
            }
          }
        }
        else if (HIT(r_canvas) && valid_click(event.button.button) && keyglobal == 0)
        {
          const Uint8 *kbd_state;

          motion_since_click = 0;

          kbd_state = SDL_GetKeyboardState(NULL);

          if ((kbd_state[SDL_SCANCODE_LCTRL] || kbd_state[SDL_SCANCODE_RCTRL]) && colors_are_selectable)
          {
            int chose_color;

            /* Holding [Ctrl] while clicking; switch to temp-mode color selector! */
            chose_color = do_color_sel(1);

            draw_cur_tool_tip();

            if (chose_color)
            {
              playsound(screen, 1, SND_BUBBLE, 1, SNDPOS_CENTER, SNDDIST_NEAR);
              cur_color = COLOR_SELECTOR;
              handle_color_changed();
            }
            else
              playsound(screen, 1, SND_CLICK, 1, SNDPOS_CENTER, SNDDIST_NEAR);

            draw_colors(COLORSEL_FORCE_REDRAW);

            SDL_Flip(screen);
          }
          else if (kbd_state[SDL_SCANCODE_X])
          {
            /* Holding [X] while clicking; switch to temp-mode eraser!
               (as long as we're not involved in anything else within
               this main loop!) */

            if ((cur_tool != TOOL_SHAPES
                 || shape_mode == SHAPE_TOOL_MODE_DONE)
                && (cur_tool != TOOL_STAMP
                    || stamp_tool_mode == STAMP_TOOL_MODE_PLACE) && cur_tool != TOOL_TEXT && cur_tool != TOOL_LABEL)
            {
              /* Jump into quick eraser loop */
              do_quick_eraser();

              /* Avoid XOR outlines from getting drawn
                 at our initial "click + [X]" position */
              if (cur_tool == TOOL_STAMP)
              {
                reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);
              }
              else if (cur_tool == TOOL_ERASER)
              {
                int mx, my;

                SDL_GetMouseState(&mx, &my);
                old_x = mx - r_canvas.x;
                old_y = my - r_canvas.y;
                maybe_redraw_eraser_xor();
              }
            }
          }
          else
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
              reset_brush_counter(FALSE);

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
                reset_brush_counter(FALSE);

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

                shape_start_x = old_x;
                shape_start_y = old_y;
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
                  reset_brush_counter(FALSE);

                  playsound(screen, 1, SND_LINE_END, 1, event.button.x, SNDDIST_NEAR);
                  do_shape(shape_start_x, shape_start_y, shape_current_x,
                           shape_current_y, shape_rotation(shape_ctr_x,
                                                           shape_ctr_y,
                                                           event.button.x -
                                                           r_canvas.x, event.button.y - r_canvas.y), 1);

                  shape_tool_mode = SHAPE_TOOL_MODE_DONE;
                  draw_tux_text(TUX_GREAT,
                                shape_tool_tips[simple_shapes ? SHAPE_COMPLEXITY_SIMPLE : SHAPE_COMPLEXITY_NORMAL], 1);
                }
              }
              else if (shape_tool_mode == SHAPE_TOOL_MODE_STRETCH)
                /* Only reached in accessibility mode */
                emulate_button_pressed = 0;
            }
            else if (cur_tool == TOOL_MAGIC)
            {
              int grp;
              int cur;

              grp = magic_group;
              cur = cur_magic[grp];

              if (!emulate_button_pressed)
              {
                int undo_ctr;
                SDL_Surface *last;

                /* Start doing magic! */

                rec_undo_buffer();

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

                magic_funcs[magics[grp][cur].handle_idx].click(magic_api_struct, magics[grp][cur].idx,
                                                               magics[grp][cur].mode, canvas, last, old_x, old_y,
                                                               &update_rect);

                draw_tux_text(TUX_GREAT, magics[grp][cur].tip[magic_modeint(magics[grp][cur].mode)], 1);

                update_canvas(update_rect.x, update_rect.y,
                              update_rect.x + update_rect.w, update_rect.y + update_rect.h);
              }

              if (mouseaccessibility)
              {
                if (magics[grp][cur].mode != MODE_FULLSCREEN && magics[grp][cur].mode != MODE_ONECLICK) /* Note: some non-fullscreen tools are also click-only (not click-and-drag) -bjk 2011.04.26 */
                  emulate_button_pressed = !emulate_button_pressed;
              }
            }
            else if (cur_tool == TOOL_ERASER)
            {
              /* Erase! */
              if (!emulate_button_pressed)
                rec_undo_buffer();

              do_eraser(old_x, old_y, 1);

              if (mouseaccessibility)
                emulate_button_pressed = !emulate_button_pressed;
            }
            else if (cur_tool == TOOL_FILL)
            {
              Uint32 draw_color, canv_color;
              int would_fill = 0;

              /* Fill */

              fill_x = old_x;
              fill_y = old_y;

              canv_color = getpixels[canvas->format->BytesPerPixel] (canvas, old_x, old_y);

              if (cur_fill == FILL_ERASER)
              {
                if (img_starter_bkgd != NULL)
                  draw_color = getpixels[img_starter_bkgd->format->BytesPerPixel] (img_starter_bkgd, old_x, old_y);
                else
                  draw_color = SDL_MapRGB(canvas->format, canvas_color_r, canvas_color_g, canvas_color_b);
              }
              else
              {
                draw_color = SDL_MapRGB(canvas->format,
                                        color_hexes[cur_color][0],
                                        color_hexes[cur_color][1], color_hexes[cur_color][2]);
              }

              would_fill = would_flood_fill(canvas, draw_color, canv_color);

              if (would_fill)
              {
                int x1, y1, x2, y2;
                SDL_Surface *last;
                int undo_ctr;

                /* We only bother recording an undo buffer
                   (which may kill our redos) if we're about
                   to actually change the picture */
                rec_undo_buffer();
                x1 = x2 = old_x;
                y1 = y2 = old_y;

                if (cur_undo > 0)
                  undo_ctr = cur_undo - 1;
                else
                  undo_ctr = NUM_UNDO_BUFS - 1;

                last = undo_bufs[undo_ctr];


                for (y1 = 0; y1 < canvas->h; y1++)
                {
                  for (x1 = 0; x1 < canvas->w; x1++)
                  {
                    sim_flood_touched[(y1 * canvas->w) + x1] = 0;
                  }
                }

                if (cur_fill == FILL_FLOOD || (cur_fill == FILL_ERASER && img_starter_bkgd == NULL))
                {
                  /* Flood fill a solid color */

                  /* (both standard flood fill, and "erase fill", when the
                     current drawing's background is a solid color) */
                  do_flood_fill(screen, texture, renderer, last, canvas,
                                old_x, old_y, draw_color, canv_color, &x1, &y1, &x2, &y2, sim_flood_touched);

                  update_canvas(x1, y1, x2, y2);
                }
                else
                {
                  SDL_Surface *tmp_canvas;

                  tmp_canvas = SDL_CreateRGBSurface(canvas->flags,
                                                    canvas->w, canvas->h,
                                                    canvas->format->BitsPerPixel,
                                                    canvas->format->Rmask,
                                                    canvas->format->Gmask,
                                                    canvas->format->Bmask, canvas->format->Amask);
                  SDL_BlitSurface(canvas, NULL, tmp_canvas, NULL);

                  simulate_flood_fill(screen, texture, renderer, last,
                                      tmp_canvas, old_x, old_y, draw_color,
                                      canv_color, &x1, &y1, &x2, &y2, sim_flood_touched);
                  SDL_FreeSurface(tmp_canvas);

                  sim_flood_x1 = x1;
                  sim_flood_y1 = y1;
                  sim_flood_x2 = x2;
                  sim_flood_y2 = y2;

                  if (cur_fill == FILL_GRADIENT_RADIAL)
                  {
                    /* Radial gradient */
                    draw_radial_gradient(canvas, sim_flood_x1, sim_flood_y1,
                                         sim_flood_x2, sim_flood_y2, old_x, old_y, draw_color, sim_flood_touched);
                  }
                  else if (cur_fill == FILL_GRADIENT_SHAPED)
                  {
                    /* Shaped gradient */
                    draw_shaped_gradient(canvas, draw_color, sim_flood_touched);
                  }
                  else if (cur_fill == FILL_GRADIENT_LINEAR)
                  {
                    /* Start a linear gradient */
                    draw_linear_gradient(canvas, canvas, sim_flood_x1,
                                         sim_flood_y1, sim_flood_x2,
                                         sim_flood_y2, fill_x, fill_y, old_x, old_y + 1, draw_color, sim_flood_touched);
                    fill_drag_started = 1;
                  }
                  else if (cur_fill == FILL_BRUSH)
                  {
                    /* Start painting within the fill area */
                    draw_brush_fill(canvas, sim_flood_x1, sim_flood_y1,
                                    sim_flood_x2, sim_flood_y2, fill_x,
                                    fill_y, old_x, old_y, draw_color, sim_flood_touched, &x1, &y1, &x2, &y2);
                  }
                  else if (cur_fill == FILL_ERASER)
                  {
                    void (*putpixel)(SDL_Surface *, int, int, Uint32) = putpixels[canvas->format->BytesPerPixel];

                    Uint32(*getpixel) (SDL_Surface *, int, int) = getpixels[img_starter_bkgd->format->BytesPerPixel];

                    int x, y;

                    /* Replace the flooded area with the background image */
                    /* (solid color background is handled above, in FILL_FLOOD mode) */

//printf("checking (%d,%d)->(%d,%d) for matches\n", sim_flood_x1, sim_flood_xy, sim_flood_x2, sim_flood_y2);

                    for (y = sim_flood_y1; y <= sim_flood_y2; y++)
                    {
                      for (x = sim_flood_x1; x <= sim_flood_x2; x++)
                      {
                        if (sim_flood_touched[y * canvas->w + x])
                        {
                          putpixel(canvas, x, y, getpixel(img_starter_bkgd, x, y));
                        }
                      }
                    }
                  }

                  update_canvas(x1, y1, x2, y2);
                }

                draw_tux_text(TUX_GREAT, fill_tips[cur_fill], 1);
              }
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
                /* Click to select a node to edit */

                DEBUG_PRINTF("Searching for label @ (%d,%d)\n", old_x, old_y);

                label_node_to_edit = search_label_list(&highlighted_label_node, old_x, old_y, 0);

                if (label_node_to_edit)
                {
                  DEBUG_PRINTF("Got a label: \"%ls\" @ (%d,%d)\n", label_node_to_edit->save_texttool_str, old_x, old_y);
                  DEBUG_PRINTF("Cursor now @ (%d,%d); width = %d\n", cursor_x, cursor_y, cursor_textwidth);
                  select_label_node(&old_x, &old_y);
                }
              }
              else if (cur_tool == TOOL_LABEL && cur_label == LABEL_APPLY)
              {
                /* Click to select a node to apply it to the canvas */

                label_node_to_edit = search_label_list(&highlighted_label_node, old_x, old_y, 0);
                if (label_node_to_edit)
                  apply_label_node(old_x, old_y);
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
                  && !(cur_tool == TOOL_LABEL && (cur_label == LABEL_SELECT || cur_label == LABEL_APPLY)))
              {
                new_kbd = osk_clicked(kbd, old_x - kbd_rect.x + r_canvas.x, old_y - kbd_rect.y + r_canvas.y);
                /* keyboard has changed, erase the old, note that the old kbd has yet been freed. */
                if (new_kbd != kbd)
                {
                  kbd = new_kbd;
                  update_canvas_ex(kbd_rect.x, kbd_rect.y, kbd_rect.x + kbd_rect.w, kbd_rect.y + kbd_rect.h, 0);
                  /* set kbd_rect dimensions according to the new keyboard */
                  reposition_onscreen_keyboard(-1);
                }
                SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
                update_screen_rect(&kbd_rect);
              }
              else
              {
                cursor_x = old_x;
                cursor_y = old_y;
                cursor_left = old_x;

                if (onscreen_keyboard
                    && !(cur_tool == TOOL_LABEL && (cur_label == LABEL_SELECT || cur_label == LABEL_APPLY)))
                {
                  update_canvas_ex(kbd_rect.x, kbd_rect.y, kbd_rect.x + kbd_rect.w, kbd_rect.y + kbd_rect.h, 0);
                  update_screen_rect(&kbd_rect);
                  reposition_onscreen_keyboard(old_y);
                }
              }

              if (onscreen_keyboard && !kbd)
              {
                r_tir.y = (float)cursor_y / render_scale;
                SDL_SetTextInputRect(&r_tir);
                SDL_StartTextInput();
              }


              do_render_cur_text(0);
            }

            button_down = 1;
          }
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
        SDL_Rect r_items;       /* = r_notcontrols; */

        SDL_GetMouseState(&xpos, &ypos);

        /* Scroll wheel code.
           WARNING: this must be kept in sync with the mouse-move
           code (for cursor changes) and mouse-click code. */

        if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_STAMP ||
            cur_tool == TOOL_SHAPES || cur_tool == TOOL_LINES ||
            cur_tool == TOOL_MAGIC || cur_tool == TOOL_TEXT ||
            cur_tool == TOOL_ERASER || cur_tool == TOOL_LABEL || cur_tool == TOOL_FILL)
        {

          /* Left tools scroll (via scroll wheel) */
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

            if (event.button.y < r_tools.y + button_h / 2)      // cursor on the upper button
            {
              if (tool_scroll == 0)
                do_setcursor(cursor_arrow);
              else
                do_setcursor(cursor_up);
            }

            else if (event.button.y > r_tools.y + r_tools.h - button_h / 2)     // cursor on the lower button
            {
              if (tool_scroll < NUM_TOOLS - 12 - TOOLOFFSET)
                do_setcursor(cursor_down);
              else
                do_setcursor(cursor_arrow);
            }

            else if (tool_avail[((event.button.x - r_tools.x) / button_w) +
                                ((event.button.y - r_tools.y - button_h / 2) / button_h) * gd_tools.cols + tool_scroll])
            {
              do_setcursor(cursor_hand);
            }
            else
            {
              do_setcursor(cursor_arrow);
            }
            update_screen_rect(&r_tools);
          }

          /* Right tool options scroll (via scroll wheel) */
          else
          {
            grid_dims gd_controls;      /* might become 2-by-2 */
            grid_dims gd_items; /* generally becoming 2-by-whatever */

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
                if (!no_stamp_rotation)
                  gd_controls.rows = 4;
                else
                  gd_controls.rows = 3;
                gd_controls.cols = 2;
              }
              else
              {
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
              gd_controls.cols = 2;
              gd_controls.rows = calc_magic_control_rows();
            }
            else if (cur_tool == TOOL_SHAPES)
            {
              if (!disable_shape_controls)
              {
                gd_controls.rows = 1;
                gd_controls.cols = 2;
              }
            }
            else if (cur_tool == TOOL_BRUSH || cur_tool == TOOL_LINES)
            {
              if (!disable_brushspacing)
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

              if ((is_upper && *thing_scroll > 0)       /* upper arrow */
                  || (!is_upper && *thing_scroll / gd_items.cols < num_rows_needed - gd_items.rows)     /* lower arrow */
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
            else if (cur_tool == TOOL_FILL)
            {
              if (do_draw)
                draw_fills();
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
              draw_tux_text_ex(TUX_GREAT, (char *)event.user.data1 + 1, 1, (int)(intptr_t) event.user.data2);   //EP added (intptr_t) to avoid warning on x64
            }
            else
            {
              draw_tux_text_ex(TUX_GREAT, (char *)event.user.data1, 0, (int)(intptr_t) event.user.data2);       //EP added (intptr_t) to avoid warning on x64
            }
          }
          else
            draw_tux_text(TUX_GREAT, "", 1);
        }
      }
      else if (event.type == TP_USEREVENT_PLAYDESCSOUND)
      {
        /* Play a stamp's spoken description (because the sound effect just finished) */
        /* (This event is pushed into the queue by playstampdesc(), which
           is a callback from Mix_ChannelFinished() when playing a stamp SFX) */

        debug("Playing description sound...");

#ifndef NOSOUND
        Mix_ChannelFinished(NULL);      /* Kill the callback, so we don't get stuck in a loop! */

        if (event.user.data1 != NULL)
        {
          if ((int)(intptr_t) event.user.data1 == cur_stamp[stamp_group])       /* Don't play old stamp's sound... *///EP added (intptr_t) to avoid warning on x64
          {
            if (!mute && stamp_data[stamp_group][(int)(intptr_t) event.user.data1]->sdesc != NULL)      //EP added (intptr_t) to avoid warning on x64
              Mix_PlayChannel(2, stamp_data[stamp_group][(int)(intptr_t) event.user.data1]->sdesc,      //EP added (intptr_t) to avoid warning on x64
                              0);
          }
        }
#endif
      }
      else if (event.type == SDL_MOUSEBUTTONUP)
      {
        if (scrolling_selector)
        {
          if (scrolltimer_selector != TIMERID_NONE)
          {
            SDL_RemoveTimer(scrolltimer_selector);
            scrolltimer_selector = TIMERID_NONE;
          }
          scrolling_selector = 0;

          DEBUG_PRINTF("Killing selector scrolling\n");
        }

        else if (scrolling_tool)
        {
          if (scrolltimer_tool != TIMERID_NONE)
          {
            SDL_RemoveTimer(scrolltimer_tool);
            scrolltimer_tool = TIMERID_NONE;
          }
          scrolling_tool = 0;

          DEBUG_PRINTF("Killing tool scrolling\n");
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

            if (motion_since_click == 0)
            {
              /* Click and release with no drag?
                 Insist on blitting the brush, even if
                 the spacing is large */
              reset_brush_counter(TRUE);
            }

            brush_draw(old_x, old_y, old_x, old_y, 1);
          }
          else if (cur_tool == TOOL_STAMP)
          {
            if (stamp_tool_mode == STAMP_TOOL_MODE_PLACE)
            {
              /* Updating the screen to draw the outlines in touchscreens where there could be touch without previous drag. */
              update_screen(old_x - (CUR_STAMP_W + 1) / 2 + r_canvas.x,
                            old_y - (CUR_STAMP_H + 1) / 2 + r_canvas.y,
                            old_x + (CUR_STAMP_W + 1) / 2 + r_canvas.x, old_y + (CUR_STAMP_H + 1) / 2 + r_canvas.y);

              if (old_x >= 0 && old_y >= 0 && old_x <= r_canvas.w && old_y <= r_canvas.h)
              {
                if (!no_stamp_rotation && stamp_rotation_ctrl)
                {
                  /* Going through stamp rotation step, first */
#if 0
                  int mouse_warp_x;

                  /* Warp mouse to the far right of the stamp,
                     where we'll start at 0-degrees of rotation
                     (keep it within the canvas, though!) */
                  mouse_warp_x = r_tools.w + old_x + (CUR_STAMP_W / 2);
                  if (mouse_warp_x >= WINDOW_WIDTH - r_ttoolopt.w)
                    mouse_warp_x = WINDOW_WIDTH - r_ttoolopt.w - 1;

                  SDL_WarpMouse(mouse_warp_x, old_y);

                  do_setcursor(cursor_rotate);
#endif
                  do_setcursor(cursor_hand);

                  stamp_tool_mode = STAMP_TOOL_MODE_ROTATE;
                  stamp_place_x = old_x;
                  stamp_place_y = old_y;
#ifdef EXPERIMENT_STAMP_ROTATION_LINE
                  stamp_xor_line_old_x = STAMP_XOR_LINE_UNSET;
                  stamp_xor_line_old_y = STAMP_XOR_LINE_UNSET;
#endif

                  snprintf(angle_tool_text, sizeof(angle_tool_text), gettext(TIP_STAMPS_ROTATING), 0);
                  draw_tux_text(TUX_GREAT, angle_tool_text, 1);
                }
                else
                {
                  /* Draw a stamp! */
                  rec_undo_buffer();
                  playsound(screen, 1, SND_STAMP, 1, event.button.x, SNDDIST_NEAR);
                  stamp_draw(old_x, old_y, 0);
                  reset_stamps(&stamp_xored_rt, &old_x, &old_y, &stamp_tool_mode);

                  draw_tux_text(TUX_GREAT, great_str(), 1);

                  /* FIXME: Make delay configurable: */

                  control_drawtext_timer(1000,
                                         stamp_data[stamp_group][cur_stamp
                                                                 [stamp_group]]->stxt,
                                         stamp_data[stamp_group][cur_stamp[stamp_group]]->locale_text);
                }
              }
            }
            else if (stamp_tool_mode == STAMP_TOOL_MODE_ROTATE)
            {
              /* Draw a stamp (finishing rotation step)! */
              rec_undo_buffer();
              playsound(screen, 1, SND_STAMP, 1, stamp_place_x, SNDDIST_NEAR);
              int stamp_angle_rotation = 360 - stamp_rotation(stamp_place_x, stamp_place_y, old_x,
                                                              old_y);

              stamp_draw(stamp_place_x, stamp_place_y, stamp_angle_rotation);
              draw_tux_text(TUX_GREAT, great_str(), 1);
              reset_stamps(&stamp_xored_rt, &stamp_place_x, &stamp_place_y, &stamp_tool_mode);

              /* FIXME: Make delay configurable: */

              control_drawtext_timer(1000,
                                     stamp_data[stamp_group][cur_stamp
                                                             [stamp_group]]->stxt,
                                     stamp_data[stamp_group][cur_stamp[stamp_group]]->locale_text);

            }
          }

          else if (cur_tool == TOOL_LINES)
          {
            if (!mouseaccessibility || (mouseaccessibility && !emulate_button_pressed))
            {
              /* (Arbitrarily large, so we draw once now) */
              reset_brush_counter(FALSE);

              brush_draw(line_start_x, line_start_y, event.button.x - r_canvas.x, event.button.y - r_canvas.y, 1);
              brush_draw(event.button.x - r_canvas.x,
                         event.button.y - r_canvas.y, event.button.x - r_canvas.x, event.button.y - r_canvas.y, 1);

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

                shape_current_x = event.button.x - r_canvas.x;
                shape_current_y = event.button.y - r_canvas.y;

                if (shape_mode == SHAPEMODE_CENTER)
                {
                  shape_ctr_x = shape_start_x;
                  shape_ctr_y = shape_start_y;
                }
                else
                {
                  shape_ctr_x = shape_start_x + (shape_current_x - shape_start_x) / 2;
                  shape_ctr_y = shape_start_y + (shape_current_y - shape_start_y) / 2;
                }

                if (!simple_shapes && !shape_no_rotate[cur_shape])
                {
                  shape_tool_mode = SHAPE_TOOL_MODE_ROTATE;

                  shape_radius =
                    sqrt((shape_start_x - shape_current_x) * (shape_start_x -
                                                              shape_current_x)
                         + (shape_start_y - shape_current_y) * (shape_start_y - shape_current_y));

                  SDL_WarpMouse(shape_ctr_x +
                                (shape_current_x - shape_ctr_x) * 1.05 + r_canvas.x, shape_ctr_y + r_canvas.y);
                  do_setcursor(cursor_rotate);


                  /* Erase stretchy XOR: */

                  if (abs(shape_start_x - shape_current_x) > 15 || abs(shape_start_y - shape_current_y) > 15)
                    do_shape(shape_start_x, shape_start_y, old_x, old_y, 0, 0);

                  /* Make an initial rotation XOR to be erased: */

                  do_shape(shape_start_x, shape_start_y,
                           shape_current_x, shape_current_y,
                           shape_rotation(shape_ctr_x, shape_ctr_y, shape_current_x, shape_current_y), 0);

                  playsound(screen, 1, SND_LINE_START, 1, event.button.x, SNDDIST_NEAR);
                  draw_tux_text(TUX_BORED, TIP_SHAPE_NEXT, 1);


                  /* FIXME: Do something less intensive! */

                  SDL_Flip(screen);
                }
                else
                {
                  reset_brush_counter(FALSE);

                  playsound(screen, 1, SND_LINE_END, 1, event.button.x, SNDDIST_NEAR);
                  do_shape(shape_start_x, shape_start_y, shape_current_x, shape_current_y, 0, 1);

                  SDL_Flip(screen);

                  shape_tool_mode = SHAPE_TOOL_MODE_DONE;
                  draw_tux_text(TUX_GREAT,
                                shape_tool_tips[simple_shapes ? SHAPE_COMPLEXITY_SIMPLE : SHAPE_COMPLEXITY_NORMAL], 1);
                }
              }
              else if (shape_tool_mode == SHAPE_TOOL_MODE_ROTATE)
              {
                reset_brush_counter(FALSE);

                playsound(screen, 1, SND_LINE_END, 1, event.button.x, SNDDIST_NEAR);
                do_shape(shape_start_x, shape_start_y, shape_current_x,
                         shape_current_y, shape_rotation(shape_ctr_x,
                                                         shape_ctr_y,
                                                         event.button.x - r_canvas.x, event.button.y - r_canvas.y), 1);

                shape_tool_mode = SHAPE_TOOL_MODE_DONE;
                draw_tux_text(TUX_GREAT,
                              shape_tool_tips[simple_shapes ? SHAPE_COMPLEXITY_SIMPLE : SHAPE_COMPLEXITY_NORMAL], 1);

                /* FIXME: Do something less intensive! */

                SDL_Flip(screen);
              }
            }
          }
          else if (cur_tool == TOOL_MAGIC
                   && (magics[magic_group][cur_magic[magic_group]].mode ==
                       MODE_PAINT
                       || magics[magic_group][cur_magic[magic_group]].mode ==
                       MODE_ONECLICK || magics[magic_group][cur_magic[magic_group]].mode == MODE_PAINT_WITH_PREVIEW))
          {
            int grp;
            int cur;

            grp = magic_group;
            cur = cur_magic[grp];

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

              magic_funcs[magics[grp][cur].handle_idx].release(magic_api_struct, magics[grp][cur].idx, canvas, last,
                                                               old_x, old_y, &update_rect);

              draw_tux_text(TUX_GREAT, magics[grp][cur].tip[magic_modeint(magics[grp][cur].mode)], 1);

              update_canvas(update_rect.x, update_rect.y, update_rect.x + update_rect.w, update_rect.y + update_rect.h);
            }
          }
          else if (onscreen_keyboard &&
                   (cur_tool == TOOL_TEXT
                    || (cur_tool == TOOL_LABEL && cur_label != LABEL_SELECT && cur_label != LABEL_APPLY)))
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
          else if (cur_tool == TOOL_FILL)
          {
            fill_drag_started = 0;
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

        motion_since_click = 1;


        /* FIXME: Is doing this every event too intensive? */
        /* Should I check current cursor first? */

        if (HIT(r_tools))
        {
          int most = buttons_tall * gd_tools.cols;

          /* Tools: */

          if (NUM_TOOLS > most)
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
              if (tool_scroll < NUM_TOOLS - buttons_tall * gd_tools.cols + gd_tools.cols)
                do_setcursor(cursor_down);
              else
                do_setcursor(cursor_arrow);
            }

            else if (tool_avail[((event.button.x - r_tools.x) / button_w) +
                                ((event.button.y - r_tools.y - button_h / 2) / button_h) * gd_tools.cols + tool_scroll])
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
               (GRIDHIT_GD(r_sfx, gd_sfx) == 1 && !stamp_data[stamp_group][cur_stamp[stamp_group]]->no_descsound)))
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
          int control_rows = 0, num_places;

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

          if (cur_tool == TOOL_LABEL)
          {
            control_rows = 1;
            if (!disable_stamp_controls)
              control_rows = 3;
          }

          if (cur_tool == TOOL_TEXT && !disable_stamp_controls)
            control_rows = 2;
          if (cur_tool == TOOL_MAGIC)
            control_rows = calc_magic_control_rows();
          if (cur_tool == TOOL_STAMP)
            control_rows = calc_stamp_control_rows();
          if (cur_tool == TOOL_SHAPES && !disable_shape_controls)
            control_rows = 1;
          if ((cur_tool == TOOL_BRUSH || cur_tool == TOOL_LINES) && !disable_brushspacing)
            control_rows = 1;

          num_places = buttons_tall * gd_toolopt.cols - control_rows * gd_toolopt.cols;

          if (num_things > num_places)
          {
            /* Are there scroll buttons? */
            num_places = num_places - gd_toolopt.cols;  /* Two scroll buttons occupy one row */
            if (event.button.y < r_ttoolopt.h + img_scroll_up->h)
            {
              /* Up button; is it available? */

              if (*thing_scroll > 0)
                do_setcursor(cursor_up);
              else
                do_setcursor(cursor_arrow);
            }
            else if (event.button.y >
                     (button_h * (num_places / gd_toolopt.cols) +
                      r_ttoolopt.h + img_scroll_up->h)
                     && event.button.y <=
                     (button_h * (num_places / gd_toolopt.cols) + r_ttoolopt.h + img_scroll_up->h + img_scroll_up->h))
            {
              /* Down button; is it available? */

              if (*thing_scroll < num_things - num_places)
                do_setcursor(cursor_down);
              else
                do_setcursor(cursor_arrow);
            }
            else
            {
              /* One of the selectors or controls: */

              which =
                ((event.button.y - r_ttoolopt.h -
                  img_scroll_up->h) / button_h) * 2 + (event.button.x - (WINDOW_WIDTH - r_ttoolopt.w)) / button_w;

              if (which + *thing_scroll < num_things)
              {
                /* A selectable item */
                do_setcursor(cursor_hand);
              }
              else if (which >= (buttons_tall - control_rows) * 2 - 2 /* account for scroll button */ )
              {
                /* Controls at the bottom (below scroll-down button, if any) */
                do_setcursor(cursor_hand);
              }
              else
              {
                /* Within the visible items, but nothing selectable */
                do_setcursor(cursor_arrow);
              }
            }
          }
          else
          {
            /* No scroll buttons - must be a selector or control: */

            which =
              ((event.button.y - r_ttoolopt.h) / button_h) * 2 +
              (event.button.x - (WINDOW_WIDTH - r_ttoolopt.w)) / button_w;

            if (which + *thing_scroll < num_things)
            {
              /* A selectable item */
              do_setcursor(cursor_hand);
            }
            else if (which >= (buttons_tall - control_rows) * 2)
            {
              /* Controls at the bottom (below scroll-down button, if any) */
              do_setcursor(cursor_hand);
            }
            else
            {
              /* Within the visible items, but nothing selectable */
              do_setcursor(cursor_arrow);
            }
          }
        }
        else if (HIT(r_canvas) && keyglobal == 0)
        {
          /* Canvas: */

          if (cur_tool == TOOL_BRUSH)
            do_setcursor(cursor_brush);
          else if (cur_tool == TOOL_STAMP)
          {
            if (stamp_tool_mode != STAMP_TOOL_MODE_ROTATE)
            {
              do_setcursor(cursor_tiny);
            }
            else
            {
              if (stamp_will_rotate(new_x, new_y, stamp_place_x, stamp_place_y))
              {
                do_setcursor(cursor_rotate);
              }
              else
              {
                do_setcursor(cursor_hand);
              }
            }
          }
          else if (cur_tool == TOOL_LINES || cur_tool == TOOL_FILL)
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
            {
              if (onscreen_keyboard && HIT(kbd_rect))
                do_setcursor(cursor_hand);
              else
                do_setcursor(cursor_insertion);
            }
            else if (cur_label == LABEL_SELECT || cur_label == LABEL_APPLY)
            {
              if (search_label_list(&current_label_node, event.button.x - r_ttools.w, event.button.y, 1))
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

            if (new_y != line_start_y)
              angle = (atan2f((new_x - line_start_x), (new_y - line_start_y)) * 180 / M_PI) - 90.0;     // we want straight line to the right to be 0 degrees
            else if (new_x >= line_start_x)
              angle = 0.0;
            else
              angle = 180.0;

            if (angle < 0.0)
              angle += 360.0;

#ifndef __ANDROID__
            update_screen(line_start_x + r_canvas.x, line_start_y + r_canvas.y, old_x + r_canvas.x, old_y + r_canvas.y);
            update_screen(line_start_x + r_canvas.x, line_start_y + r_canvas.y, new_x + r_canvas.x, new_y + r_canvas.y);
            update_screen(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
#else
            /* Anyway SDL_UpdateRect() backward compatibility function refreshes all the screen on Android */
            SDL_UpdateRect(screen, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
#endif

            snprintf(angle_tool_text, sizeof(angle_tool_text), gettext(TIP_LINE_MOVING), floor(angle));
            draw_tux_text(TUX_BORED, angle_tool_text, 1);
          }
          else if (cur_tool == TOOL_SHAPES)
          {
            /* Still pushing button, while moving:
               Draw XOR where shape will go: */

            if (shape_tool_mode == SHAPE_TOOL_MODE_STRETCH)
            {
              do_shape(shape_start_x, shape_start_y, old_x, old_y, 0, 0);

              do_shape(shape_start_x, shape_start_y, new_x, new_y, 0, 0);

              shape_reverse = (new_x < shape_start_x);


              /* FIXME: Fix update shape function! */

              /* update_shape(shape_start_x, old_x, new_x,
                 shape_start_y, old_y, new_y,
                 shape_locked[cur_shape]); */

              SDL_Flip(screen);
            }
          }
          else if (cur_tool == TOOL_MAGIC
                   && (magics[magic_group][cur_magic[magic_group]].mode ==
                       MODE_PAINT
                       || magics[magic_group][cur_magic[magic_group]].mode ==
                       MODE_ONECLICK || magics[magic_group][cur_magic[magic_group]].mode == MODE_PAINT_WITH_PREVIEW))
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

            magic_funcs[magics[magic_group]
                        [cur_magic[magic_group]].handle_idx].drag(magic_api_struct,
                                                                  magics[magic_group][cur_magic[magic_group]].idx,
                                                                  canvas, last, old_x, old_y, new_x, new_y,
                                                                  &update_rect);

            update_canvas(update_rect.x, update_rect.y, update_rect.x + update_rect.w, update_rect.y + update_rect.h);
          }
          else if (cur_tool == TOOL_ERASER)
          {
            int sz, eraser_type;

            /* Still pushing, and moving - Erase! */

            eraser_draw(old_x, old_y, new_x, new_y);

            sz = calc_eraser_size(cur_eraser);
            eraser_type = cur_eraser / NUM_ERASER_SIZES;

            if (eraser_type == ERASER_TYPE_SQUARE)
            {
              /* Square eraser */
              rect_xor(new_x - sz / 2, new_y - sz / 2, new_x + sz / 2, new_y + sz / 2);
            }
            else
            {
              /* Circle eraser */
              circle_xor(new_x, new_y, sz / 2);
            }
          }
          else if (cur_tool == TOOL_FILL && cur_fill == FILL_GRADIENT_LINEAR && fill_drag_started)
          {
            Uint32 draw_color;
            int undo_ctr;
            SDL_Surface *last;

            if (cur_undo > 0)
              undo_ctr = cur_undo - 1;
            else
              undo_ctr = NUM_UNDO_BUFS - 1;

            last = undo_bufs[undo_ctr];

            /* Pushing button and moving: Update the gradient: */

            draw_color = SDL_MapRGB(canvas->format,
                                    color_hexes[cur_color][0], color_hexes[cur_color][1], color_hexes[cur_color][2]);
            draw_linear_gradient(canvas, last, sim_flood_x1, sim_flood_y1,
                                 sim_flood_x2, sim_flood_y2, fill_x, fill_y,
                                 new_x, new_y, draw_color, sim_flood_touched);

            update_canvas(sim_flood_x1, sim_flood_y1, sim_flood_x2, sim_flood_y2);

            if (new_y != fill_y)
              angle = (atan2f((new_x - fill_x), (new_y - fill_y)) * 180 / M_PI) - 90.0;
            else if (new_x >= fill_x)
              angle = 0.0;
            else
              angle = 180.0;

            if (angle < 0.0)
              angle += 360.0;

            snprintf(angle_tool_text, sizeof(angle_tool_text), gettext(TIP_FILL_LINEAR_MOVING), floor(angle));
            draw_tux_text(TUX_GREAT, angle_tool_text, 1);
          }
          else if (cur_tool == TOOL_FILL && cur_fill == FILL_BRUSH)
          {
            Uint32 draw_color;
            int x1, y1, x2, y2;

            /* Pushing button and moving: Paint more within the fill area: */

            draw_color = SDL_MapRGB(canvas->format,
                                    color_hexes[cur_color][0], color_hexes[cur_color][1], color_hexes[cur_color][2]);

            draw_brush_fill(canvas, sim_flood_x1, sim_flood_y1, sim_flood_x2,
                            sim_flood_y2, old_x, old_y, new_x, new_y,
                            draw_color, sim_flood_touched, &x1, &y1, &x2, &y2);

            update_canvas(x1, y1, x2, y2);
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
            w = calc_eraser_size(cur_eraser);
            h = w;
          }

          if (old_x >= 0 && old_x < r_canvas.w && old_y >= 0 && old_y < r_canvas.h)
          {
            if (cur_tool == TOOL_STAMP)
            {
              if (stamp_tool_mode == STAMP_TOOL_MODE_ROTATE)
              {
                int deg;

                stamp_xor(stamp_place_x, stamp_place_y);
#ifdef EXPERIMENT_STAMP_ROTATION_LINE
                /* Erase old stamp rotation angle XOR'd line */
                /* FIXME: Needs also be erased in other situations! */
                if (stamp_xor_line_old_x != STAMP_XOR_LINE_UNSET && stamp_xor_line_old_y != STAMP_XOR_LINE_UNSET)
                {
                  if (stamp_will_rotate(stamp_place_x, stamp_place_y, stamp_xor_line_old_x, stamp_xor_line_old_y))
                  {
                    line_xor(stamp_place_x, stamp_place_y, stamp_xor_line_old_x, stamp_xor_line_old_y);
                  }
                }
#endif

                deg = (360 - stamp_rotation(stamp_place_x, stamp_place_y, new_x, new_y)) % 360;
                update_stamp_xor(deg);

                stamp_xor(stamp_place_x, stamp_place_y);
#ifdef EXPERIMENT_STAMP_ROTATION_LINE
                /* Erase old stamp rotation angle XOR'd line */
                if (stamp_will_rotate(stamp_place_x, stamp_place_y, new_x, new_y))
                {
                  line_xor(stamp_place_x, stamp_place_y, new_x, new_y);
                  stamp_xor_line_old_x = new_x;
                  stamp_xor_line_old_y = new_y;
                }
                else
                {
                  stamp_xor_line_old_x = STAMP_XOR_LINE_UNSET;
                  stamp_xor_line_old_y = STAMP_XOR_LINE_UNSET;
                }
#endif

#ifndef EXPERIMENT_STAMP_ROTATION_LINE
                /* The half of maximum size the stamp could have when rotating. */
                int half_bigbox =
                  sqrt((CUR_STAMP_W + 1) * (CUR_STAMP_W + 1) + (CUR_STAMP_H + 1) * (CUR_STAMP_H + 1)) / 2;
                update_screen(min
                              (min(new_x, old_x),
                               stamp_place_x - half_bigbox) + r_canvas.x,
                              min(min(new_y, old_y),
                                  stamp_place_y - half_bigbox) + r_canvas.y,
                              max(max(new_x, old_x),
                                  stamp_place_x + half_bigbox) + r_canvas.x,
                              max(max(new_y, old_y), stamp_place_y + half_bigbox) + r_canvas.y);
#else
                /* FIXME: Be smarter about this */
                SDL_UpdateRect(screen, 0, 0, screen->w, screen->h);
#endif

                snprintf(angle_tool_text, sizeof(angle_tool_text), gettext(TIP_STAMPS_ROTATING), deg);
                draw_tux_text(TUX_GREAT, angle_tool_text, 1);
              }
              else if (stamp_xored_rt)
              {
                /* Stamp */
                stamp_xor(old_x, old_y);
                stamp_xored_rt = !stamp_xored_rt;

                update_screen(old_x - (CUR_STAMP_W + 1) / 2 + r_canvas.x,
                              old_y - (CUR_STAMP_H + 1) / 2 + r_canvas.y,
                              old_x + (CUR_STAMP_W + 1) / 2 + r_canvas.x, old_y + (CUR_STAMP_H + 1) / 2 + r_canvas.y);
              }
            }
            else
            {
              int eraser_type;

              eraser_type = cur_eraser / NUM_ERASER_SIZES;

              if (eraser_type == ERASER_TYPE_SQUARE)
              {
                /* Square eraser */
                rect_xor(old_x - w / 2, old_y - h / 2, old_x + w / 2, old_y + h / 2);
              }
              else
              {
                /* Circle eraser */
                circle_xor(old_x, old_y, calc_eraser_size(cur_eraser) / 2);
              }

              update_screen(old_x - w / 2 + r_canvas.x,
                            old_y - h / 2 + r_canvas.y, old_x + w / 2 + r_canvas.x, old_y + h / 2 + r_canvas.y);
            }
          }
          if (new_x >= 0 && new_x < r_canvas.w && new_y >= 0 && new_y < r_canvas.h)
          {
            if (cur_tool == TOOL_STAMP)
            {
              /* Stamp */
              if (stamp_tool_mode != STAMP_TOOL_MODE_ROTATE)
              {
                stamp_xor(new_x, new_y);
                stamp_xored_rt = 1;
              }
              update_screen(old_x - (CUR_STAMP_W + 1) / 2 + r_canvas.x,
                            old_y - (CUR_STAMP_H + 1) / 2 + r_canvas.y,
                            old_x + (CUR_STAMP_W + 1) / 2 + r_canvas.x, old_y + (CUR_STAMP_H + 1) / 2 + r_canvas.y);
            }
            else if (cur_tool == TOOL_ERASER)
            {
              int eraser_type;

              eraser_type = cur_eraser / NUM_ERASER_SIZES;

              if (eraser_type == ERASER_TYPE_SQUARE)
              {
                /* Square eraser */
                rect_xor(new_x - w / 2, new_y - h / 2, new_x + w / 2, new_y + h / 2);
              }
              else
              {
                /* Circle eraser */
                circle_xor(new_x, new_y, calc_eraser_size(cur_eraser) / 2);
              }

              update_screen(new_x - w / 2 + r_canvas.x,
                            new_y - h / 2 + r_canvas.y, new_x + w / 2 + r_canvas.x, new_y + h / 2 + r_canvas.y);
            }
          }

          if (cur_tool == TOOL_STAMP && HIT(r_toolopt)
              && event.motion.y > r_toolopt.h && event.motion.state == SDL_PRESSED && stamp_size_selector_clicked)
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
                                                                           (WINDOW_WIDTH -
                                                                            r_ttoolopt.w))) / r_toolopt.w) +
              MIN_STAMP_SIZE;

            DEBUG_PRINTF("Old size = %d, Chose %0.4f, New size =%d\n",
                         old_size, choice, stamp_data[stamp_group][cur_stamp[stamp_group]]->size);

            if (stamp_data[stamp_group][cur_stamp[stamp_group]]->size != old_size)
            {
              if (stamp_xored)
              {
                stamp_xor(canvas->w / 2, canvas->h / 2);
                stamp_xored = 0;

                update_screen(canvas->w / 2 - (w + 1) / 2 + r_canvas.x,
                              canvas->h / 2 - (h + 1) / 2 + r_canvas.y,
                              canvas->w / 2 + (w + 1) / 2 + r_canvas.x, canvas->h / 2 + (h + 1) / 2 + r_canvas.y);
              }

              update_stamp_xor(0);
              stamp_xor(canvas->w / 2, canvas->h / 2);
              stamp_xored = 1;
              update_screen(canvas->w / 2 - (CUR_STAMP_W + 1) / 2 +
                            r_canvas.x,
                            canvas->h / 2 - (CUR_STAMP_H + 1) / 2 +
                            r_canvas.y,
                            canvas->w / 2 + (CUR_STAMP_W + 1) / 2 +
                            r_canvas.x, canvas->h / 2 + (CUR_STAMP_H + 1) / 2 + r_canvas.y);
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
        else if (cur_tool == TOOL_SHAPES)
        {
          if (shape_tool_mode == SHAPE_TOOL_MODE_STRETCH && !shape_locked[cur_shape])
          {
            float aspect;
            int w, h;

            w = abs(shape_start_x - new_x);
            h = abs(shape_start_y - new_y);


            if (w < 2 || h < 2)
              aspect = 0;
            else if (w > h)
              aspect = (float)w / (float)h;
            else
              aspect = (float)h / (float)w;

            if (aspect == 0 || aspect >= 100)
            {
              draw_tux_text(TUX_BORED, TIP_SHAPE_START, 1);
            }
            else
            {
              snprintf(stretch_tool_text, sizeof(stretch_tool_text), gettext(TIP_SHAPE_STRETCHING_UNLOCKED), aspect);
              draw_tux_text(TUX_BORED, stretch_tool_text, 1);
            }
          }
          else if (shape_tool_mode == SHAPE_TOOL_MODE_ROTATE)
          {
            int deg;

            deg = shape_rotation(shape_ctr_x, shape_ctr_y, old_x, old_y);
            do_shape(shape_start_x, shape_start_y, shape_current_x, shape_current_y, deg, 0);

            deg = shape_rotation(shape_ctr_x, shape_ctr_y, new_x, new_y);
            do_shape(shape_start_x, shape_start_y, shape_current_x, shape_current_y, deg, 0);

            deg = -deg;
            if (deg < 0)
              deg += 360;

            snprintf(angle_tool_text, sizeof(angle_tool_text), gettext(TIP_SHAPE_ROTATING), deg);
            draw_tux_text(TUX_BORED, angle_tool_text, 1);

            /* FIXME: Do something less intensive! */
            SDL_Flip(screen);
          }
        }

        old_x = new_x;
        old_y = new_y;
        oldpos_x = event.button.x;
        oldpos_y = event.button.y;
      }
    }

    if (cur_tool == TOOL_TEXT || (cur_tool == TOOL_LABEL && cur_label != LABEL_SELECT && cur_label != LABEL_APPLY))
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


    SDL_Delay(10);
  }
  while (!done);

#if defined (__ANDROID__)
  /* Closing Tux Paint before the end of font scanning resulted in crashes in the Android port */
  /* This is an abuse of font_thread_aborted, maybe it is better to use a new, more descriptive marker? */
  if (!font_thread_done)
    font_thread_aborted = 1;
#endif
}

/**
 * Hide the blinking text entry cursor (caret),
 * if it was visible.
 */
static void hide_blinking_cursor(void)
{
  if (cur_toggle_count & 1)
  {
    draw_blinking_cursor();
  }
}

/**
 * Draw & hide the blinking text entry cursor (caret),
 * via XOR.  (Also keeps track of whether the cursor
 * is currently visible; used by hide_blinking_cursor(), above.)
 */
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

/**
 * Draw a line on the canvas using the current paint brush.
 *
 * @param x1 Starting X coordinate
 * @param y1 Starting Y coordinate
 * @param x2 Ending X coordinate
 * @param y2 Ending Y coordinate
 * @param update Update the screen afterwards?
 */
static void brush_draw(int x1, int y1, int x2, int y2, int update)
{
  int dx, dy, y, frame_w, w, h, sz;
  int orig_x1, orig_y1, orig_x2, orig_y2, tmp;
  int direction;
  float m, b;
  double r;

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
  r = -1.0;
  if (brushes_directional[cur_brush] || brushes_rotate[cur_brush])
  {
    r = brush_rotation(x1, y1, x2, y2);

    if (brushes_directional[cur_brush])
    {
      r = r + 22.0;
      if (r < 0.0)
        r = r + 360.0;
      if (x1 != x2 || y1 != y2)
        direction = (r / 45.0);
    }
    else
    {
      if (x1 != x2 || y1 != y2)
      {
        r = 270.0 - r;
      }
      else
      {
        /* Point "up" if there was no motion
           (brush will appear as it does in the selector;
           it's "natural" direction) */
        r = 360.0;
      }
    }
  }

  if (brushes_chaotic[cur_brush])
  {
    r = (float)(rand() % 36) * 10.0;
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
          blit_brush(x1, y, direction, r, &w, &h);
      }
      else
      {
        for (y = y1; y <= y2; y++)
          blit_brush(x1, y, direction, r, &w, &h);
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
      blit_brush(x1, y, direction, r, &w, &h);
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
    sz = max(w, h);
    update_canvas(orig_x1 - sz, orig_y1 - sz, orig_x2 + sz, orig_y2 + sz);
  }
}


/**
 * Reset the brush counter, such that the next attempt to draw something
 * is either (a) guaranteed to do so, regardless of the brush's spacing
 * (for non-rotational brushes), or (b) requires the user to have moved
 * far enough to get a good idea of the angle they're drawing
 * (for rotational brushes) -- unless "force" is true (which will happen
 * if the user clicks and releases with no motion whatsoever).
 *
 * @param force if true, resets counter even if a rotating brush
 */
void reset_brush_counter(int force)
{
  if (img_cur_brush_rotate && !force)
    brush_counter = 0;
  else
    brush_counter = 999;
}


/**
 * Draw into the canvas using the current paint brush.
 * Adheres to:
 *  - brush spacing (used by some brushes to avoid smearing)
 *  - brush animation
 *  - drawing direction
 *
 * @param x X coordinate
 * @param y Y coordinate
 * @param direction BRUSH_DIRECTION_... being drawn (for compass direction brushes)
 * @param rotation angle being drawn (for brushes which may rotate at any angle (0-360 degrees))
 */
static void blit_brush(int x, int y, int direction, double rotation, int *w, int *h)
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

    if ((img_cur_brush_rotate || img_cur_brush_chaotic) && rotation != -1.0 /* only if we're moving */ )
    {
      SDL_Surface *rotated_brush;

      /* TODO: Cache these; discard them when the user changes the brush or alters its color */

      rotated_brush = NULL;

      if (img_cur_brush_frames != 1)
      {
        SDL_Surface *brush_frame_surf;

        brush_frame_surf = SDL_CreateRGBSurface(0, src.w, src.h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

        if (brush_frame_surf != NULL)
        {
          /* Ensure any semi-transparent areas or edges match the same color as we're painting
             (and not cause a black halo; see https://sourceforge.net/p/tuxpaint/bugs/259/ -bjk 2024.10.11) */
          SDL_FillRect(brush_frame_surf, NULL,
                       SDL_MapRGBA(brush_frame_surf->format,
                                   color_hexes[cur_color][0], color_hexes[cur_color][1], color_hexes[cur_color][2], 0));

          /* 2021/09/28 SDL(2)_gfxBlitRGBA() is not available in the SDL2_gfx library, using plain SDL_BlitSurface() instead. Pere
             SDL_gfxBlitRGBA(img_cur_brush, &src, brush_frame_surf, NULL); */
          SDL_BlitSurface(img_cur_brush, &src, brush_frame_surf, NULL);
          rotated_brush = rotozoomSurface(brush_frame_surf, rotation, 1.0, SMOOTHING_ON);
          SDL_FreeSurface(brush_frame_surf);
        }
      }
      else
      {
        rotated_brush = rotozoomSurface(img_cur_brush, rotation, 1.0, SMOOTHING_ON);
      }

      if (rotated_brush != NULL)
      {
        src.x = 0;
        src.y = 0;
        src.w = rotated_brush->w;
        src.h = rotated_brush->h;

        /* Incoming x,y is based on center of an _unrotated_ brush,
           so undo that (back to mouse position), then offset so
           the rotated version is centered around the mouse. */
        dest.x = dest.x + (img_cur_brush_w >> 1) - (rotated_brush->w >> 1);
        dest.y = dest.y + (img_cur_brush_h >> 1) - (rotated_brush->h >> 1);
        dest.w = rotated_brush->w;
        dest.h = rotated_brush->h;

        SDL_BlitSurface(rotated_brush, &src, canvas, &dest);

        SDL_FreeSurface(rotated_brush);
      }
    }
    else
    {
      SDL_BlitSurface(img_cur_brush, &src, canvas, &dest);
    }
  }

  *w = src.w;
  *h = src.h;
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


/**
 * FIXME
 */
static void fill_multichan(multichan *mc, double *up, double *vp)
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


/**
 * FIXME
 */
static double tint_part_1(multichan *work, SDL_Surface *in)
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


/**
 * FIXME
 */
static void change_colors(SDL_Surface *out, multichan *work, double hue_range, multichan *key_color_ptr)
{
  double lower_hue_1, upper_hue_1, lower_hue_2, upper_hue_2;
  int xx, yy;
  multichan dst;
  double satratio;
  double slope;
  void (*putpixel)(SDL_Surface *, int, int, Uint32);
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
        L += newsat * slope;    /* not greyscale destination */
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


/**
 * FIXME
 */
static multichan *find_most_saturated(double initial_hue, multichan *work, unsigned num, double *hue_range_ptr)
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
    hue_range = 18 * M_PI / 180.0;      /* plus or minus 18 degrees search, 27 replace */
    break;
  case TINTER_NARROW:
    hue_range = 6 * M_PI / 180.0;       /* plus or minus 6 degrees search, 9 replace */
    break;
  case TINTER_ANYHUE:
    hue_range = M_PI;           /* plus or minus 180 degrees */
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


/**
 * FIXME
 */
static void vector_tint_surface(SDL_Surface *out, SDL_Surface *in)
{
  int xx, yy;

  Uint32(*getpixel) (SDL_Surface *, int, int) = getpixels[in->format->BytesPerPixel];
  void (*putpixel)(SDL_Surface *, int, int, Uint32) = putpixels[out->format->BytesPerPixel];

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
      old = sRGB_to_linear_table[r8] * 0.2126 + sRGB_to_linear_table[g8] * 0.7152 + sRGB_to_linear_table[b8] * 0.0722;

      putpixel(out, xx, yy,
               SDL_MapRGBA(out->format, linear_to_sRGB(r * old), linear_to_sRGB(g * old), linear_to_sRGB(b * old), a8));
    }
  }
  SDL_UnlockSurface(in);
}


/**
 * Tint a surface (e.g., a stamp) using the currently-selected color.
 *
 * @param tmp_surf Destination surface
 * @param surf_ptr Source surface
 */
static void tint_surface(SDL_Surface *tmp_surf, SDL_Surface *surf_ptr)
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

    DEBUG_PRINTF("initial_hue = %f\n", initial_hue);

    key_color_ptr = find_most_saturated(initial_hue, work, width * height, &hue_range);

    DEBUG_PRINTF("key_color_ptr = %d\n", (int)(intptr_t) key_color_ptr);        //EP added (intptr_t) to avoid warning on x64

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

  fprintf(stderr, "Falling back to tinter=vector, this should be in the *.dat file\n");

  vector_tint_surface(tmp_surf, surf_ptr);
}

/**
 * Draw the current stamp onto the canvas.
 *
 * @param x X coordinate
 * @param y Y coordinate
 */
static void stamp_draw(int x, int y, int stamp_angle_rotation)
{
  SDL_Rect dest;
  SDL_Surface *scaled_surf, *tmp_surf, *surf_ptr;
  Uint32 amask;
  Uint8 r, g, b, a;
  int xx, yy, base_x, base_y;
  int dont_free_tmp_surf, dont_free_scaled_surf;

  if (current_stamp_cached == NULL)
  {
    Uint32(*getpixel) (SDL_Surface *, int, int);
    void (*putpixel)(SDL_Surface *, int, int, Uint32);

    /* Shrink or grow it! */
    scaled_surf = thumbnail(active_stamp, CUR_STAMP_W, CUR_STAMP_H, 0);
    dont_free_scaled_surf = 0;


    surf_ptr = scaled_surf;

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
      /* Tintable */
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

    current_stamp_cached = SDL_DisplayFormatAlpha(tmp_surf);
  }
  else
  {
    tmp_surf = current_stamp_cached;
    dont_free_tmp_surf = 1;
    dont_free_scaled_surf = 1;
  }

  /* Rotate the stamp (if no_stamp_rotation is not configured) */
  if (stamp_angle_rotation)
    tmp_surf = rotozoomSurface(tmp_surf, stamp_angle_rotation, 1.0, SMOOTHING_ON);

  /* Where it will go? */
  base_x = x - (tmp_surf->w + 1) / 2;
  base_y = y - (tmp_surf->h + 1) / 2;


  /* And blit it! */
  dest.x = base_x;
  dest.y = base_y;
  SDL_BlitSurface(tmp_surf, NULL, canvas, &dest);       /* FIXME: Conditional jump or move depends on uninitialised value(s) */


  update_canvas(x - (tmp_surf->w + 1) / 2,
                y - (tmp_surf->h + 1) / 2, x + (tmp_surf->w + 1) / 2, y + (tmp_surf->h + 1) / 2);

  /* Free the temporary surfaces */

  if (!dont_free_tmp_surf)
    SDL_FreeSurface(tmp_surf);

  if (!dont_free_scaled_surf)
    SDL_FreeSurface(scaled_surf);
}


/**
 * Store canvas or label into undo buffer
 */
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

  DEBUG_PRINTF("DRAW: Current=%d  Oldest=%d  Newest=%d\n", cur_undo, oldest_undo, newest_undo);


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


/**
 * Show program version, build date, and (optionally) details,
 * to stdout
 *
 * @param details Show details?
 */
void show_version(int details)
{
  printf("\nTux Paint\n");
  printf("  Version " VER_VERSION " (" VER_DATE ")\n");


  if (details == 0)
    return;


  printf("\nBuilt with these options:\n");

  printf("  SDL version %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);

  /* Quality reductions: */

#ifdef LOW_QUALITY_THUMBNAILS
  printf("  Low Quality Thumbnails enabled  (LOW_QUALITY_THUMBNAILS)\n");
#else
  printf("  High Quality Thumbnails enabled  (not LOW_QUALITY_THUMBNAILS)\n");
#endif

#ifdef LOW_QUALITY_STAMP_OUTLINE
  printf("  Low Quality Stamp Outline enabled  (LOW_QUALITY_STAMP_OUTLINE)\n");
#else
  printf("  High Quality Stamp Outline enabled  (not LOW_QUALITY_STAMP_OUTLINE)\n");
#endif

#ifdef NO_PROMPT_SHADOWS
  printf("  Prompt Shadows disabled  (NO_PROMPT_SHADOWS)\n");
#else
  printf("  Prompt Shadows enabled  (not NO_PROMPT_SHADOWS)\n");
#endif

#ifdef SMALL_CURSOR_SHAPES
  printf("  Small cursor shapes enabled  (SMALL_CURSOR_SHAPES)\n");
#else
  printf("  Large cursor shapes enabled  (not SMALL_CURSOR_SHAPES)\n");
#endif

#ifdef NO_BILINEAR
  printf("  Bilinear scaling disabled  (NO_BILINEAR)\n");
#else
  printf("  Bilinear scaling enabled  (BILINEAR)\n");
#endif

#ifdef NOSVG
  printf("  SVG support disabled  (NOSVG)\n");
#else
#ifdef OLD_SVG
  printf("  SVG support enabled -- old: libCairo  (not NOSVG, OLD_SVG)\n");
#else
  printf("  SVG support enabled -- new: libRSVG  (not NOSVG, not OLD_SVG)\n");
#endif
#endif

  /* Sound: */

#ifdef NOSOUND
  printf("  Sound disabled  (NOSOUND)\n");
#else
  printf("  Sound enabled  (not NOSOUND)\n");
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
#elif OS2
  printf("  Built for OS2  (OS2)\n");
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


/**
 * Show usage display and exit.
 *
 * @param exitcode What exit() code to give;
 *   also determines stdout (0) vs stderr (non-zero) for output
 */
void show_usage(int exitcode)
{
  FILE *f = exitcode ? stderr : stdout;

  /* *INDENT-OFF* */
  fprintf(f,
          "\n"
          "Usage: %s {--usage | --help | --version | --verbose-version | --copying | --listfonts}\n"
          "\n"
          " Config:\n"
          "  [--nosysconfig]\n"
          "\n"
          " Video/Sound:\n"
          "  [--fullscreen=yes | --fullscreen=native | --fullscreen=no]\n"
          "  [--windowed]\n"
          "  [--WIDTHxHEIGHT | --native]\n"
          "  [--orient=landscape | --orient=portrait]\n"
          "  [--disablescreensaver | --allowscreensaver ]\n"
          "  [--sound | --nosound]\n"
          "  [--stereo | --nostereo]\n"
          "  [--buttonsize=N (24-192; default=48) | --buttonsize=auto]\n"
          "  [--colorsrows=N] (1-3; default=1)\n"
          "  [--colorfile FILE]\n"
          "\n"
          " Mouse/Keyboard:\n"
          "  [--fancycursors | --nofancycursors]\n"
          "  [--hidecursor | --showcursor]\n"
          "  [--noshortcuts | --shortcuts]\n"
          "  [--dontgrab | --grab]\n"
          "  [--wheelmouse | --nowheelmouse]\n"
          "  [--nobuttondistinction | --buttondistinction]\n"
          "\n"
          " Simplification:\n"
          "  [--complexshapes | --simpleshapes]\n"
          "  [--outlines | --nooutlines]\n"
          "  [--mixedcase | --uppercase]\n"
          "  [--stampsize=[0-10] | --stampsize=default]\n"
          "  [--quit | --noquit]\n"
          "  [--stamps | --nostamps]\n"
          "  [--nostampcontrols | --stampcontrols]\n"
          "  [--nomagiccontrols | --magiccontrols]\n"
          "  [--nomagicsizes | --magicsizes]\n"
          "  [--ungroupmagictools | --groupmagictools]\n"
          "  [--noshapecontrols | --shapecontrols]\n"
          "  [--nolabel | --label]\n"
          "  [--nobrushspacing | --brushspacing]\n"
          "  [--notemplateexport | --templateexport]\n"
          "  [--complexity=advanced | --complexity=beginner | --complexity=novice]\n"
          "  [--noerase | --erase]\n"
          "\n"
          " Languages:\n"
          "  [--lang LANGUAGE | --locale LOCALE | --lang help]\n"
          "  [--mirrorstamps | --dontmirrorstamps]\n"
          "  [--uifont \"FONT NAME\" | --uifont default]\n"
          "  [--sysfonts | --nosysfonts]\n"
          "  [--currentlocalefont | --alllocalefonts]\n"
          "\n"
          " Printing:\n"
          "  [--print | --noprint]\n"
          "  [--printdelay=SECONDS]\n"
          "  [--altprintmod | --altprintalways | --altprintnever]\n"
#if defined(WIN32) || defined(__APPLE__)
          "  [--printcfg | --noprintcfg]\n"
#endif

#if !defined(WIN32) && !defined(__APPLE__) && !defined(__BEOS__) && !defined(__HAIKU__) && !defined(__ANDROID__)
          "  [--printcommand=COMMAND]\n"
          "  [--altprintcommand=COMMAND]\n"
          "  [--papersize PAPERSIZE | --papersize help]\n"
#endif
          "\n"
          " Saving:\n"
          "  [--saveoverask | --saveover | --saveovernew]\n"
          "  [--startblank | --startlast]\n"
          "  [--newcolorsfirst | --newcolorslast]\n"
          "  [--savedir DIRECTORY]\n"
          "  [--nosave | --save]\n"
          "  [--autosave | --noautosave]\n"
          "  [--reversesort | --noreversesort]\n"
          "\n"
          " Data:\n"
          "  [--nolockfile]\n"
          "  [--datadir DIRECTORY]\n"
          "\n"
          " Exporting:\n"
          "  [--exportdir DIRECTORY]\n"
          "\n"
          " Accessibility:\n"
          "  [--mouse-accessibility]\n"
          "  [--mouse | --keyboard]\n"
          "  [--onscreen-keyboard]\n"
          "  [--onscreen-keyboard-layout=LAYOUT]\n"
          "  [--onscreen-keyboard-disable-change]\n"
          "\n"
          " Joystick:\n"
          "  [--joystick-dev N] (default=0)\n"
          "  [--joystick-slowness N] (0-500; default value is 15)\n"
          "  [--joystick-threshold N] (0-32766; default value is 3200)\n"
          "  [--joystick-maxsteps N] (1-7; default value is 7)\n"
          "  [--joystick-hat-slowness N] (0-500; default value is 15)\n"
          "  [--joystick-hat-timeout N] (0-3000; default value is 1000)\n"
          "  [--joystick-buttons-ignore=BUTTON1,BUTTON2,...]\n"
          "  [--joystick-btn-COMMAND=BUTTON]\n"
          /* Find these in "src/parse.gperf" & "src/tuxpaint-completion.bash" */
          "    (commands: escape, brush, stamp, lines, shapes, text, label, fill, magic,\n"
          "    undo, redo, eraser, new, open, save, pgsetup, print)\n"
          "\n",
          progname);
  /* *INDENT-ON* */
}

/**
 * Show a list of fonts that Pango finds (and hence
 * should be available to "--uifont" argument) and exit.
 */
void show_fonts(void)
{
  PangoFontMap *fontmap;
  PangoFontFamily **families;
  int i, n_families;
  char **family_names;
  char locale_fontdir[MAX_PATH];
  FcBool fontAddStatus;

  snprintf(locale_fontdir, sizeof(locale_fontdir), "%s/fonts", DATA_PREFIX);

  fontAddStatus = FcConfigAppFontAddDir(FcConfigGetCurrent(), (const FcChar8 *)locale_fontdir);
  if (fontAddStatus == FcFalse)
  {
    fprintf(stderr, "Unable to add font dir %s\n", locale_fontdir);
  }

  FcDirCacheRead((const FcChar8 *)locale_fontdir, FcTrue /* force */ ,
                 FcConfigGetCurrent());
  FcDirCacheRescan((const FcChar8 *)locale_fontdir, FcConfigGetCurrent());

  generate_fontconfig_cache_real();

  fontmap = pango_ft2_font_map_new();
  pango_font_map_list_families(fontmap, &families, &n_families);

  family_names = (char * *)malloc(sizeof(char *) * n_families);
  for (i = 0; i < n_families; i++)
  {
    family_names[i] = strdup(pango_font_family_get_name(families[i]));
  }

  qsort(family_names, n_families, sizeof(char *), compare_font_family);

  for (i = 0; i < n_families; i++)
  {
    printf("%s\n", family_names[i]);
    free(family_names[i]);
  }
  free(family_names);

  exit(0);
}

int compare_font_family(const void *a, const void *b)
{
  return strcasecmp(*(char *const *)a, *(char *const *)b);
}

/**
 * Compute default scale factor for stamps.
 *
 * The original Tux Paint canvas was 448x376. The canvas can be
 * other sizes now, but many old stamps are sized for the small
 * canvas. So, with larger canvases, we must choose a good scale
 * factor to compensate. As the canvas size grows, the user will
 * want a balance of "more stamps on the screen" and "stamps not
 * getting tiny". This will calculate the needed scale factor.
 *
 * @param ratio FIXME
 * @return FIXME
 */
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


/**
 * Callback for directory walking while loading brushes
 *
 * @param screen Screen/window surface, for drawing progress bar animation
 * @param dir Directory path
 * @param dirlen Length of directory path string (ignored)
 * @param files List of files (being collected)
 * @param i Counter
 * @param locale UI's locale, for loading localized text (ignored)
 */
static void loadbrush_callback(SDL_Surface *screen,
                               __attribute__((unused)) SDL_Texture *texture,
                               __attribute__((unused)) SDL_Renderer *renderer,
                               const char *restrict const dir,
                               unsigned dirlen, tp_ftw_str *files, unsigned i, const char *restrict const locale)
{
  FILE *fi;
  char buf[64];
  int want_rand, spacing;
  int brush_w, brush_h;
  float scale;

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

      safe_snprintf(fname, sizeof fname, "%s/%s", dir, files[i].str);
      if (num_brushes == num_brushes_max)
      {
        num_brushes_max = num_brushes_max * 5 / 4 + 4;
        img_brushes = realloc(img_brushes, num_brushes_max * sizeof *img_brushes);
        img_brushes_thumbs = realloc(img_brushes_thumbs, num_brushes_max * sizeof *img_brushes_thumbs);
        brushes_frames = realloc(brushes_frames, num_brushes_max * sizeof(int));
        brushes_directional = realloc(brushes_directional, num_brushes_max * sizeof(short));
        brushes_rotate = realloc(brushes_rotate, num_brushes_max * sizeof(short));
        brushes_chaotic = realloc(brushes_chaotic, num_brushes_max * sizeof(short));
        brushes_spacing = realloc(brushes_spacing, num_brushes_max * sizeof(int));
        brushes_spacing_default = realloc(brushes_spacing_default, num_brushes_max * sizeof(int));
        brushes_descr = realloc(brushes_descr, num_brushes_max * sizeof *brushes_descr);
        brushes_descr_localized = realloc(brushes_descr_localized, num_brushes_max * sizeof(Uint8));
      }
      img_brushes[num_brushes] = loadimage(fname);

      /* Load brush description, if any: */
      brushes_descr[num_brushes] = loaddesc(fname, &(brushes_descr_localized[num_brushes]));
      DEBUG_PRINTF("%s: %s (%d)\n", fname,
                   (brushes_descr[num_brushes] !=
                    NULL ? brushes_descr[num_brushes] : "NULL"), brushes_descr_localized[num_brushes]);

      /* Load brush metadata, if any: */

      /* (Brush setting defaults) */
      brushes_frames[num_brushes] = 1;
      brushes_directional[num_brushes] = 0;
      brushes_rotate[num_brushes] = 0;
      brushes_chaotic[num_brushes] = 0;
      spacing = img_brushes[num_brushes]->h / 4;

      strcpy(strcasestr(fname, ".png"), ".dat");        /* FIXME: Use strncpy (ugh, complicated) */
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
              spacing = atoi(strstr(buf, "spacing=") + 8);
            }
            else if (strstr(buf, "directional") != NULL)
            {
              brushes_directional[num_brushes] = 1;
            }
            else if (strstr(buf, "rotate") != NULL)
            {
              brushes_rotate[num_brushes] = 1;
            }
            else if (strstr(buf, "chaotic") != NULL)
            {
              brushes_chaotic[num_brushes] = 1;
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

      /* Generate thumbnail */
      brush_w =
        ((img_brushes[num_brushes]->w / abs(brushes_frames[num_brushes])) / (brushes_directional[num_brushes] ? 3 : 1));
      brush_h = (img_brushes[num_brushes]->h / (brushes_directional[num_brushes] ? 3 : 1));

      if (brush_w <= button_w && brush_h <= button_h)
      {
        img_brushes_thumbs[num_brushes] = duplicate_surface(img_brushes[num_brushes]);
      }
      else
      {
        if (brush_w > brush_h)
        {
          scale = (float)((float)button_w / (float)brush_w);
        }
        else
        {
          scale = (float)((float)button_h / (float)brush_h);
        }

        img_brushes_thumbs[num_brushes] = thumbnail2(img_brushes[num_brushes], img_brushes[num_brushes]->w * scale, img_brushes[num_brushes]->h * scale, 0,     /* no need to ask to keep aspect; already kept */
                                                     1  /* keep alpha */
          );
      }

      brushes_spacing[num_brushes] = spacing;
      brushes_spacing_default[num_brushes] = spacing;

      num_brushes++;
    }
    free(files[i].str);
  }
  free(files);
}


/**
 * FIXME
 */
static void load_brush_dir(SDL_Surface *screen, const char *restrict const dir)
{
  char buf[TP_FTW_PATHSIZE];
  unsigned dirlen = strlen(dir);

  memcpy(buf, dir, dirlen);
  tp_ftw(screen, texture, renderer, buf, dirlen, 0, loadbrush_callback, NULL);
}

/**
 * FIXME
 */
SDL_Surface *mirror_surface(SDL_Surface *s)
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

/**
 * FIXME
 */
SDL_Surface *flip_surface(SDL_Surface *s)
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

/**
 * FIXME
 */
static void loadstamp_finisher(stamp_type *sd, unsigned w, unsigned h, double ratio)
{
  unsigned int upper = HARD_MAX_STAMP_SIZE;
  unsigned int underscanned_upper = HARD_MAX_STAMP_SIZE;
  unsigned int lower = 0;
  unsigned mid;

  DEBUG_PRINTF("Finishing %s for %dx%d (ratio=%0.4f)\n", sd->stampname, w, h, ratio);

  /* If Tux Paint is in mirror-image-by-default mode, mirror, if we can: */
  if (mirrorstamps && sd->mirrorable)
    sd->mirrored = 1;

  do
  {
    scaleparams *s = &scaletable[upper];
    int pw, ph;                 /* proposed width and height */

    pw = (w * s->numer + s->denom - 1) / s->denom;
    ph = (h * s->numer + s->denom - 1) / s->denom;

#ifdef ALLOW_STAMP_OVERSCAN
    /* OK to let a stamp stick off the sides in one direction, not two */
    /* By default, Tux Paint allowed stamps to be, at max, 2x as wide OR 2x as tall as canvas; scaled that back to 1.5 -bjk 2011.01.08 */
    if (pw < canvas->w * 1.5 && ph < canvas->h * 1)
    {
      DEBUG_PRINTF("Upper at %d with proposed size %dx%d (wide)\n", upper, pw, ph);

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
      DEBUG_PRINTF("Upper at %d with proposed size %dx%d (tall)\n", upper, pw, ph);

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
      DEBUG_PRINTF("Upper at %d with proposed size %dx%d\n", upper, pw, ph);

      underscanned_upper = upper;
      break;
    }
#endif
  }
  while (--upper);


  do
  {
    scaleparams *s = &scaletable[lower];
    int pw, ph;                 /* proposed width and height */

    pw = (w * s->numer + s->denom - 1) / s->denom;
    ph = (h * s->numer + s->denom - 1) / s->denom;

    if (pw * ph > 20)
    {
      DEBUG_PRINTF("Lower at %d with proposed size %dx%d\n", lower, pw, ph);
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

  DEBUG_PRINTF("Final min=%d, size=%d, max=%d\n", lower, mid, upper);

  if (stamp_size_override != -1)
  {
    sd->size = (((upper - lower) * stamp_size_override) / 10) + lower;
    DEBUG_PRINTF("...but adjusting size to %d\n", sd->size);
  }
  DEBUG_PRINTF("\n");
}


/**
 * Clear the currently-cached stamp image
 * (what we blit to the canvas, so the stamp in its
 * current orientation, scale, and color), since we'll
 * need to generate something new based on a change
 * (a new size, color, orientation, or a completely
 * different stamp)
 */
static void clear_cached_stamp(void)
{
  if (current_stamp_cached != NULL)
  {
    SDL_FreeSurface(current_stamp_cached);
    current_stamp_cached = NULL;
  }
}

/**
 * FIXME
 */
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

  DEBUG_PRINTF("\nset_active_stamp()\n");

  /* Look for pre-mirrored and pre-flipped version: */

  needs_mirror = sd->mirrored;
  needs_flip = sd->flipped;

  if (sd->mirrored && sd->flipped)
  {
    /* Want mirrored and flipped, both */

    DEBUG_PRINTF("want both mirrored & flipped\n");

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
      DEBUG_PRINTF("found a _mirror_flip!\n");

      needs_mirror = 0;
      needs_flip = 0;
    }
    else
    {
      /* Couldn't get one that was both, look for _mirror then _flip and
         flip or mirror it: */

      DEBUG_PRINTF("didn't find a _mirror_flip\n");

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
        DEBUG_PRINTF("found a _mirror!\n");
        needs_mirror = 0;
      }
      else
      {
        /* Couldn't get one that was just pre-mirrored, look for a
           pre-flipped */

        DEBUG_PRINTF("didn't find a _mirror, either\n");

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
          DEBUG_PRINTF("found a _flip!\n");
          needs_flip = 0;
        }
        else
        {
          DEBUG_PRINTF("didn't find a _flip, either\n");
        }
      }
    }
  }
  else if (sd->flipped && !sd->no_preflip)
  {
    /* Want flipped only */

    DEBUG_PRINTF("want flipped only\n");

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
      DEBUG_PRINTF("found a _flip!\n");
      needs_flip = 0;
    }
    else
    {
      DEBUG_PRINTF("didn't find a _flip\n");
    }
  }
  else if (sd->mirrored && !sd->no_premirror)
  {
    /* Want mirrored only */

    DEBUG_PRINTF("want mirrored only\n");

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
      DEBUG_PRINTF("found a _mirror!\n");
      needs_mirror = 0;
    }
    else
    {
      DEBUG_PRINTF("didn't find a _mirror\n");
    }
  }


  /* Didn't want mirrored, or flipped, or couldn't load anything
     that was pre-rendered: */

  if (!active_stamp)
  {
    DEBUG_PRINTF("loading normal\n");

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
    DEBUG_PRINTF("mirroring\n");
    active_stamp = mirror_surface(active_stamp);
  }

  if (needs_flip)
  {
    DEBUG_PRINTF("flipping\n");
    active_stamp = flip_surface(active_stamp);
  }

  DEBUG_PRINTF("\n\n");

  clear_cached_stamp();
}

/**
 * FIXME
 */
static void get_stamp_thumb(stamp_type *sd, int process_sound)
{
  SDL_Surface *bigimg = NULL;
  unsigned len = strlen(sd->stampname);
  char *buf = alloca(len + strlen("_mirror_flip.EXT") + 1);
  int need_mirror, need_flip;
  double ratio;
  unsigned w;
  unsigned h;

  DEBUG_PRINTF("\nget_stamp_thumb()\n");

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
  if (!sd->sound_processed && process_sound)
  {
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

    sd->sound_processed = 1;
  }
#endif


  /* first see if we can re-use an existing thumbnail */
  if (sd->thumbnail)
  {
    DEBUG_PRINTF("have an sd->thumbnail\n");

    if (sd->thumb_mirrored_flipped == sd->flipped &&
        sd->thumb_mirrored_flipped == sd->mirrored &&
        sd->mirrored == sd->thumb_mirrored && sd->flipped == sd->thumb_flipped)
    {
      /* It's already the way we want */

      DEBUG_PRINTF("mirrored == flipped == thumb_mirrored_flipped [bye]\n");

      return;
    }
  }


  /* nope, see if there's a pre-rendered one we can use */

  show_progress_bar(screen);

  need_mirror = sd->mirrored;
  need_flip = sd->flipped;
  bigimg = NULL;

  if (sd->mirrored && sd->flipped)
  {
    DEBUG_PRINTF("want mirrored & flipped\n");

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
      DEBUG_PRINTF("found a _mirror_flip!\n");

      need_mirror = 0;
      need_flip = 0;
    }
    else
    {
      DEBUG_PRINTF("didn't find a mirror_flip\n");

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
        DEBUG_PRINTF("found a _mirror\n");

        need_mirror = 0;
      }
      else
      {
        DEBUG_PRINTF("didn't find a mirror\n");

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
          DEBUG_PRINTF("found a _flip\n");

          need_flip = 0;
        }
      }
    }
  }
  else if (sd->mirrored && !sd->no_premirror)
  {
    DEBUG_PRINTF("want mirrored only\n");

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
      DEBUG_PRINTF("found a _mirror!\n");
      need_mirror = 0;
    }
    else
    {
      DEBUG_PRINTF("didn't find a mirror\n");
      sd->no_premirror = 1;
    }
  }
  else if (sd->flipped && !sd->no_preflip)
  {
    DEBUG_PRINTF("want flipped only\n");

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
      DEBUG_PRINTF("found a _flip!\n");
      need_flip = 0;
    }
    else
    {
      DEBUG_PRINTF("didn't find a flip\n");
      sd->no_preflip = 1;
    }
  }


  /* If we didn't load a pre-rendered, load the normal one: */

  if (!bigimg)
  {
    DEBUG_PRINTF("loading normal...\n");

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
  int ww = (40 * button_w) / ORIGINAL_BUTTON_SIZE;
  int hh = (40 * button_h) / ORIGINAL_BUTTON_SIZE;

  if (bigimg)
  {
    w = bigimg->w;
    h = bigimg->h;
  }

  if (!bigimg)
    sd->thumbnail = thumbnail(img_dead40x40, ww, hh, 1);        /* copy */
  else if (bigimg->w > 40 || bigimg->h > 40)
  {
    sd->thumbnail = thumbnail(bigimg, ww, hh, 1);
    SDL_FreeSurface(bigimg);
  }
  else
    sd->thumbnail = bigimg;


  /* Mirror and/or flip the thumbnail, if we still need to do so: */

  if (need_mirror)
  {
    DEBUG_PRINTF("mirroring\n");
    sd->thumbnail = mirror_surface(sd->thumbnail);
  }

  if (need_flip)
  {
    DEBUG_PRINTF("flipping\n");
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

  DEBUG_PRINTF("\n\n");


  /* Finish up, if we need to: */

  if (sd->processed)
    return;

  sd->processed = 1;            /* not really, but on the next line... */
  loadstamp_finisher(sd, w, h, ratio);
}


/**
 * Callback for directory walking while loading stamps
 *
 * @param screen Screen/window surface, for drawing progress bar animation
 * @param dir Directory path
 * @param dirlen Length of directory path string
 * @param files List of files (being collected)
 * @param i Counter
 * @param locale UI's locale, for loading localized text (ignored)
 */
static void loadstamp_callback(SDL_Surface *screen,
                               __attribute__((unused)) SDL_Texture *texture,
                               __attribute__((unused)) SDL_Renderer *renderer,
                               const char *restrict const dir,
                               unsigned dirlen, tp_ftw_str *files, unsigned i, const char *restrict const locale)
{
  (void)locale;

  DEBUG_PRINTF("loadstamp_callback (%d): %s\n", i, dir);

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
      DEBUG_PRINTF("\n...counts as a new group! now: %d\n", stamp_group);
    }
    else
    {
      DEBUG_PRINTF("...is still part of group %d\n", stamp_group);
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

      safe_snprintf(svgname, sizeof(svgname), "%s/%s", dir, files[i].str);
      strcpy(strcasestr(svgname, ".png"), ".svg");      /* FIXME: Use strncpy (ugh, complicated) */

      fi = fopen(svgname, "r");
      if (fi != NULL)
      {
        debug("Found SVG version of ");
        debug(files[i].str);
        debug("\n");

        fclose(fi);
        continue;               /* ugh, i hate continues */
      }
    }
#endif

    /*
     * Showing the progress bar across the screen can be CPU-intensive, so
     * update infrequently.
     */
    if ((i % 32) == 0)
      show_progress_bar(screen);

    if (dotext > files[i].str && !strcasecmp(dotext, ext)
        && (dotext - files[i].str + 1 + dirlen < (int)(sizeof fname))
        && !strcasestr(files[i].str, mirror_ext)
        && !strcasestr(files[i].str, flip_ext) && !strcasestr(files[i].str, mirrorflip_ext))
    {
      safe_snprintf(fname, sizeof fname, "%s/%s", dir, files[i].str);
      if (num_stamps[stamp_group] == max_stamps[stamp_group])
      {
        max_stamps[stamp_group] = max_stamps[stamp_group] * 5 / 4 + 15;
        stamp_data[stamp_group] = realloc(stamp_data[stamp_group],
                                          max_stamps[stamp_group] * sizeof(*stamp_data[stamp_group]));
      }
      stamp_data[stamp_group][num_stamps[stamp_group]] =
        calloc(1, sizeof *stamp_data[stamp_group][num_stamps[stamp_group]]);
      stamp_data[stamp_group][num_stamps[stamp_group]]->stampname = malloc(dotext - files[i].str + 1 + dirlen + 1);
      memcpy(stamp_data[stamp_group][num_stamps[stamp_group]]->stampname, fname, dotext - files[i].str + 1 + dirlen);
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

/**
 * FIXME
 */
static void load_stamp_dir(SDL_Surface *screen, const char *const dir)
{
  char buf[TP_FTW_PATHSIZE];
  unsigned dirlen = strlen(dir);

  memcpy(buf, dir, dirlen);
  load_stamp_basedir = dir;
  tp_ftw(screen, texture, renderer, buf, dirlen, 0, loadstamp_callback, NULL);
}

/**
 * FIXME
 */
static void load_stamps(SDL_Surface *screen)
{
  char *homedirdir = get_fname("stamps", DIR_DATA);

  default_stamp_size = compute_default_scale_factor(1.0);

  load_stamp_dir(screen, homedirdir);
#ifndef __ANDROID__
  load_stamp_dir(screen, DATA_PREFIX "stamps");
#else
  load_stamp_dir(screen, "stamps/animals");
  load_stamp_dir(screen, "stamps/cartoon/tux");
  load_stamp_dir(screen, "stamps/clothes");
  load_stamp_dir(screen, "stamps/food");
  load_stamp_dir(screen, "stamps/hobbies");
  load_stamp_dir(screen, "stamps/household");
  load_stamp_dir(screen, "stamps/medical");
  load_stamp_dir(screen, "stamps/military");
  load_stamp_dir(screen, "stamps/naturalforces");
  load_stamp_dir(screen, "stamps/people");
  load_stamp_dir(screen, "stamps/plants");
  load_stamp_dir(screen, "stamps/seasonal");
  load_stamp_dir(screen, "stamps/space");
  load_stamp_dir(screen, "stamps/sports");
  load_stamp_dir(screen, "stamps/symbols");
  load_stamp_dir(screen, "stamps/town");
  load_stamp_dir(screen, "stamps/vehicles");
#endif
#ifdef __MACOS__
  load_stamp_dir(screen, "Resources/stamps");
  load_stamp_dir(screen, "/Library/Application Support/TuxPaint/stamps");
#endif
#ifdef __IOS__
  load_stamp_dir(screen, "stamps");
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
/**
 * FIXME
 */
static int load_user_fonts_stub(void *vp)
{
  return load_user_fonts(screen, texture, renderer, vp, NULL);
}
#endif


volatile long fontconfig_thread_done = 0;

/**
 * FIXME
 */
int generate_fontconfig_cache_spinner(SDL_Surface *screen)
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
          (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_AC_BACK)))
      {
        fprintf(stderr, "Aborting!\n");
        fflush(stdout);
        return (1);
      }
    }
  }
  return (0);
}

/**
 * FIXME
 */
static int generate_fontconfig_cache_real(void)
{
  TuxPaint_Font *tmp_font;
  SDL_Surface *tmp_surf;
  SDL_Color black = { 0, 0, 0, 0 };

  DEBUG_PRINTF("-- Hello from generate_fontconfig_cache() (thread # %d)\n", SDL_ThreadID());


  tmp_font = TuxPaint_Font_OpenFont(PANGO_DEFAULT_FONT, NULL, 12);      /* always just using the default font for the purpose of getting FontConfig to generate its cache */

  if (tmp_font != NULL)
  {
    DEBUG_PRINTF("-- Generated a font.\n");
    tmp_surf = render_text(tmp_font, "Test", black);
    if (tmp_surf != NULL)
    {
      DEBUG_PRINTF("-- Generated a surface\n");
      SDL_FreeSurface(tmp_surf);
    }
    else
    {
      DEBUG_PRINTF("-- Failed to make a surface!\n");
    }
    TuxPaint_Font_CloseFont(tmp_font);
  }
  else
  {
    DEBUG_PRINTF("-- Failed to generate a font!\n");
  }

  fontconfig_thread_done = 1;

  DEBUG_PRINTF("-- generate_fontconfig_cache() is done\n");

  return (0);
}

/**
 * FIXME
 */
static int generate_fontconfig_cache( __attribute__((unused))
                                     void *vp)
{
  return generate_fontconfig_cache_real();
}


#define hex2dec(c) (((c) >= '0' && (c) <= '9') ? ((c) - '0') : \
  ((c) >= 'A' && (c) <= 'F') ? ((c) - 'A' + 10) : \
  ((c) >= 'a' && (c) <= 'f') ? ((c) - 'a' + 10) : 0)

#ifndef WIN32
static void signal_handler(int sig)
{
  // It is not legal to call printf or most other functions here!
  if (sig == SIGUSR1 || sig == SIGUSR2)
  {
    autosave_on_quit = 1;
    no_prompt_on_quit = 1;
    if (sig == SIGUSR1)
    {
      promptless_save = SAVE_OVER_NO;
    }
    else
    {
      promptless_save = SAVE_OVER_ALWAYS;
    }
    raise(SIGTERM);
  }
}
#endif


/* If the label text is greater than or equal to (>=) the button's
   width times this multiplier, we'll attempt to word-wrap the text */
#define BUTTON_LABEL_WRAP_THRESHOLD_MULT 1.3

/* If the label text got word-wrapped (ends up on more than
   1 line, via introduction of '\n'), we'll give it slightly
   more height */
#define BUTTON_LABEL_MULTILINE_HEIGHT_MULT 1.2

/**
 * FIXME
 */
/* Render a button label using the appropriate string/font: */
static SDL_Surface *do_render_button_label(const char *const label)
{
  SDL_Surface *tmp_surf1, *tmp_surf, *surf;
  SDL_Color black = { 0, 0, 0, 0 };
  TuxPaint_Font *myfont;
  int want_h;
  float height_mult;
  char *upstr;

  upstr = uppercase(gettext(label));

  DEBUG_PRINTF("do_render_button_label(\"%s\")\n", label);
  if (button_w <= ORIGINAL_BUTTON_SIZE)
  {
    DEBUG_PRINTF("Small font\n");
    myfont = small_font;
  }
  else if (button_w <= ORIGINAL_BUTTON_SIZE * 3)
  {
    DEBUG_PRINTF("Medium font\n");
    myfont = medium_font;
  }
  else
  {
    DEBUG_PRINTF("Large font\n");
    myfont = large_font;
  }

  if (need_own_font && strcmp(gettext(label), label))
  {
    myfont = locale_font;
    DEBUG_PRINTF("Need local font\n");
  }

  tmp_surf1 = render_text(myfont, upstr, black);
  if (tmp_surf1 == NULL)
  {
    fprintf(stderr, "Failed to render button '%s'!\n", upstr);
    exit(1);
  }

  height_mult = 1.0;

  /* If very wide, try to wrap on a space (near the end) */
  if (tmp_surf1->w >= button_w * BUTTON_LABEL_WRAP_THRESHOLD_MULT)
  {
    int i, found = -1, wrapped = 0;

    DEBUG_PRINTF("'%s' is very wide (%d) compared to button size (%d)\n", upstr, tmp_surf1->w, button_w);

    if (strstr(upstr, " ") != NULL)
    {
      for (i = (strlen(upstr) * 3 / 4); i >= 0 && found == -1; i--)
      {
        if (upstr[i] == ' ')
        {
          found = i;
        }
      }

      if (found != -1)
      {
        upstr[found] = '\n';
        wrapped = 1;
      }

      if (wrapped)
      {
        SDL_FreeSurface(tmp_surf1);
        tmp_surf1 = render_text(myfont, upstr, black);

        height_mult = BUTTON_LABEL_MULTILINE_HEIGHT_MULT;
      }
    }
  }

  /* If STILL very wide, try to wrap on visible hyphen/dash */
  if (tmp_surf1->w >= button_w * BUTTON_LABEL_WRAP_THRESHOLD_MULT)
  {
    int i, found = -1, wrapped = 0;
    char *broken_str;

    DEBUG_PRINTF("'%s' is STILL very wide (%d) compared to button size (%d)\n", upstr, tmp_surf1->w, button_w);

    /* Try to wrap on a visible hyphen/dash */
    if (strstr(upstr, "-") != NULL)
    {
      for (i = (strlen(upstr) - 1); i >= 0 && found == -1; i--)
      {
        if (upstr[i] == '-')
        {
          found = i;
        }
      }

      if (found != -1)
      {
        broken_str = alloca(sizeof(wchar_t) * strlen(upstr) + 2);
        if (broken_str != NULL)
        {
          for (i = 0; i <= found; i++)
          {
            broken_str[i] = upstr[i];
          }
          broken_str[i] = '\n';
          for (i = found + 1; i < (int)strlen(upstr); i++)
          {
            broken_str[i + 1] = upstr[i];
          }
          broken_str[i + 1] = '\0';

          wrapped = 1;
          free(upstr);
          upstr = strdup(broken_str);
        }
      }

      if (wrapped)
      {
        SDL_FreeSurface(tmp_surf1);
        tmp_surf1 = render_text(myfont, upstr, black);

        height_mult = BUTTON_LABEL_MULTILINE_HEIGHT_MULT;
      }
    }
  }

  /* If STILL very wide, try to wrap on invisible soft hyphen */
  if (tmp_surf1->w >= button_w * BUTTON_LABEL_WRAP_THRESHOLD_MULT)
  {
    int i, found = -1, wrapped = 0;
    char *broken_str;

    DEBUG_PRINTF("'%s' is STILL very wide (%d) compared to button size (%d)\n", upstr, tmp_surf1->w, button_w);

    /* Try to wrap on an invisible soft hyphen */
    if (strstr(upstr, "\302\255") != NULL)
    {
      /* (This also _introduces_ a visible hyphen;
         basically we replace the two-byte UTF-8 sequence
         with ASCII '-' and '\n') */
      found = (int)(strstr(upstr, "\302\255") - upstr);

      DEBUG_PRINTF("\"%s\" has a soft hypen at %d\n", upstr, found);

      broken_str = alloca(sizeof(wchar_t) * strlen(upstr) + 3);
      if (broken_str != NULL)
      {
        for (i = 0; i < found; i++)
        {
          broken_str[i] = upstr[i];
        }
        broken_str[found] = '-';
        broken_str[found + 1] = '\n';
        for (i = found + 2; i < (int)strlen(upstr); i++)
        {
          broken_str[i] = upstr[i];
        }
        broken_str[i] = '\0';

        wrapped = 1;
        free(upstr);
        upstr = strdup(broken_str);
      }

      if (wrapped)
      {
        SDL_FreeSurface(tmp_surf1);
        tmp_surf1 = render_text(myfont, upstr, black);

        height_mult = BUTTON_LABEL_MULTILINE_HEIGHT_MULT;
      }
    }
  }

  free(upstr);

  tmp_surf = tmp_surf1;

  // FIXME: CROP LABELS
#if 0
  tmp_surf = crop_surface(tmp_surf1);
  if (tmp_surf == NULL)
    return NULL;

  SDL_FreeSurface(tmp_surf1);
#endif

  DEBUG_PRINTF("Rendered as: %d x %d\n", tmp_surf->w, tmp_surf->h);

  want_h = (int)(18 * button_scale) * height_mult;

  DEBUG_PRINTF("  button_w = %d -- min w = %d\n", button_w, min(button_w, tmp_surf->w));
  DEBUG_PRINTF("  want_h   = %d -- min h = %d\n", want_h, min(want_h, tmp_surf->h));

  surf = thumbnail(tmp_surf, min(button_w, tmp_surf->w), min(want_h, tmp_surf->h), 1 /* keep aspect! */ );
  SDL_FreeSurface(tmp_surf);

  DEBUG_PRINTF("Resized to:  %d x %d\n", surf->w, surf->h);
  DEBUG_PRINTF("\n");

  return surf;
}

#if 0
static SDL_Surface *crop_surface(SDL_Surface *surf)
{
  int top, bottom, left, right, x, y, w, h;
  Uint8 r, g, b, a, r1, g1, b1, a1;

  Uint32(*getpixel) (SDL_Surface *, int, int) = getpixels[surf->format->BytesPerPixel];
  SDL_Surface *new_surf;
  SDL_Rect src_rect;

  SDL_GetRGBA(getpixel(surf, 0, 0), surf->format, &r1, &g1, &b1, &a1);

  top = surf->h - 1;
  bottom = 0;
  left = surf->w - 1;
  right = 0;

  for (y = 0; y < surf->h; y++)
  {
    for (x = 0; x < surf->w; x++)
    {
      SDL_GetRGBA(getpixel(surf, x, y), surf->format, &r, &g, &b, &a);
      if (r != r1 || g != g1 || b != b1 || a != a1)
      {
        if (y < top)
          top = y;
        if (x < left)
          left = x;
        if (y > bottom)
          bottom = y;
        if (x > right)
          right = x;
      }
    }
  }

  w = right - left + 1;
  h = bottom - top + 1;

  DEBUG_PRINTF("Cropping %d x %d to %d x %d, from (%d,%d)\n", surf->w, surf->h, w, h, left, top);

  if ((top == 0 && bottom == surf->h - 1 && left == 0 && right == surf->w - 1) || w <= 0 || h <= 0)
  {
    /* Not cropped; return the whole thing */
    return SDL_DisplayFormatAlpha(surf);
  }

  new_surf = SDL_CreateRGBSurface(surf->flags,
                                  w, h, surf->format->BitsPerPixel,
                                  surf->format->Rmask, surf->format->Gmask, surf->format->Bmask, surf->format->Amask);
  if (new_surf == NULL)
  {
    fprintf(stderr, "crop_surface() cannot create new surface!\n");
    return NULL;
  }

  src_rect.x = left;
  src_rect.y = top;
  src_rect.w = w;
  src_rect.h = h;
  SDL_BlitSurface(surf, &src_rect, new_surf, NULL);

  return new_surf;
}
#endif

/**
 * FIXME
 */
static void create_button_labels(void)
{
  int i, j;

  /* Main tools */
  for (i = 0; i < NUM_TOOLS; i++)
    img_tool_names[i] = do_render_button_label(tool_names[i]);

  /* Magic Tools */
  for (i = 0; i < MAX_MAGIC_GROUPS; i++)
  {
    for (j = 0; j < num_magics[i]; j++)
    {
      magics[i][j].img_name = do_render_button_label(magics[i][j].name);
    }
  }

  /* Shapes for Shape Tool */
  for (i = 0; i < NUM_SHAPES; i++)
    img_shape_names[i] = do_render_button_label(shape_names[i]);

  /* Fill methods for Fill Tool */
  for (i = 0; i < NUM_FILLS; i++)
    img_fill_names[i] = do_render_button_label(fill_names[i]);

  /* Buttons for the file open dialog */

  /* Open dialog: 'Open' button, to load the selected picture */
  img_openlabels_open = do_render_button_label(gettext_noop("Open"));

  /* Open dialog: 'Erase' button, to erase/deleted the selected picture */
  img_openlabels_erase = do_render_button_label(gettext_noop("Erase"));

  /* Open dialog: 'Slides' button, to switch to slide show mode */
  img_openlabels_slideshow = do_render_button_label(gettext_noop("Slides"));

  /* Open dialog: 'Template' button, to make a template out of a drawing */
  img_openlabels_template = do_render_button_label(gettext_noop("Template"));

  /* Open dialog: 'Export' button, to copy an image to an easily-accessible location */
  img_openlabels_pict_export = do_render_button_label(gettext_noop("Export"));

  /* Open dialog: 'Back' button, to dismiss Open dialog without opening a picture */
  img_openlabels_back = do_render_button_label(gettext_noop("Back"));

  /* Slideshow: 'Play' button, to begin a slideshow sequence */
  img_openlabels_play = do_render_button_label(gettext_noop("Play"));

  /* Slideshow: 'GIF Export' button, to create an animated GIF */
  img_openlabels_gif_export = do_render_button_label(gettext_noop("GIF Export"));

  /* Slideshow: 'Next' button, to load next slide (image) */
  img_openlabels_next = do_render_button_label(gettext_noop("Next"));

  /* Color mixer dialog: 'Clear' button, to reset the mixed color */
  img_mixerlabel_clear = do_render_button_label(gettext_noop("Clear"));
}


/**
 * FIXME
 */
static void seticon(void)
{
  int masklen;
  Uint8 *mask;
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


  /* Create mask: */
  masklen = (((icon->w) + 7) / 8) * (icon->h);
  mask = malloc(masklen * sizeof(Uint8));
  memset(mask, 0xFF, masklen);

  /* Set icon: */
  //  SDL_WM_SetIcon(icon, mask);
  SDL_SetWindowIcon(window_screen, icon);
  /* Free icon surface & mask: */
  free(mask);
  SDL_FreeSurface(icon);


  /* Grab keyboard and mouse, if requested: */

  if (grab_input)
  {
    debug("Grabbing input!");
    SDL_SetWindowGrab(window_screen, SDL_TRUE);
  }
}


/**
 * FIXME
 */
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

/**
 * Load an image and Resize it according to the difference between the size of the Buttons original and current ones:
 */
static SDL_Surface *loadimagerb(const char *const fname)
{
  /* For the vast majority of users return as soon as possible and touch the image as little as we can. */
  if (button_h == ORIGINAL_BUTTON_SIZE)
    return (loadimage(fname));

  /* Going to resize the button */
  int w, h;
  SDL_Surface *aux_surf;
  SDL_Surface *aux2_surf;

  aux_surf = loadimage(fname);
  if (aux_surf)
  {
    w = (aux_surf->w * button_w) / ORIGINAL_BUTTON_SIZE;
    h = (aux_surf->h * button_h) / ORIGINAL_BUTTON_SIZE;
    aux2_surf = thumbnail(aux_surf, w, h, 0);
  }
  else
    return (NULL);

  SDL_FreeSurface(aux_surf);
  return (aux2_surf);
}

/**
 * FIXME
 */
/* Load an image (with errors): */
static SDL_Surface *loadimage(const char *const fname)
{
  return (do_loadimage(fname, 1));
}


/**
 * FIXME
 */
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


/**
 * FIXME
 */
/* Draw the toolbar: */
static void draw_toolbar(void)
{
  int i, off_y, max, most, tool;
  SDL_Rect dest;

  most = (buttons_tall * gd_toolopt.cols) - TOOLOFFSET;
  off_y = 0;
  /* FIXME: Only allow print if we have something to print! */


  draw_image_title(TITLE_TOOLS, r_ttools);



  /* Do we need scrollbars? */
  if (NUM_TOOLS > most + TOOLOFFSET)
  {
    off_y = img_scroll_up->h;
    max = most - gd_tools.cols + TOOLOFFSET;
    gd_tools.rows = max / gd_tools.cols;

    dest.x = 0;
    dest.y = r_ttools.h;

    if (tool_scroll > 0)
    {
      SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
    }
    else
    {
      SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);
    }

    dest.x = 0;
    dest.y = r_ttools.h + off_y + ((most - gd_tools.cols + TOOLOFFSET) / gd_tools.cols * button_h);



    if (tool_scroll < NUM_TOOLS - (most - gd_tools.cols) - TOOLOFFSET)
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




  for (tool = tool_scroll; tool < tool_scroll + max; tool++)
  {
    i = tool - tool_scroll;
    dest.x = ((i % 2) * button_w);
    dest.y = ((i / 2) * button_h) + r_ttools.h + off_y;


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

      dest.x = ((i % 2) * button_w) + 4;
      dest.y = ((i / 2) * button_h) + r_ttools.h + 2 + off_y;

      SDL_BlitSurface(img_tools[tool], NULL, screen, &dest);

      dest.x =
        ((i % 2) * button_w) + (4 * button_w) / ORIGINAL_BUTTON_SIZE +
        ((40 * button_w) / ORIGINAL_BUTTON_SIZE - img_tool_names[tool]->w) / 2;
      dest.y =
        ((i / 2) * button_h) + r_ttools.h +
        (2 * button_w) / ORIGINAL_BUTTON_SIZE +
        ((46 * button_w) / ORIGINAL_BUTTON_SIZE - img_tool_names[tool]->h) + off_y;

      SDL_BlitSurface(img_tool_names[tool], NULL, screen, &dest);
    }
    else
    {
      SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
    }
  }
}


/**
 * FIXME
 */
/* Draw magic controls: */
static void draw_magic(void)
{
  int magic, i, max, off_y;
  SDL_Rect dest;
  int most;
  SDL_Surface *button_color;
  SDL_Surface *button_body;

  draw_image_title(TITLE_MAGIC, r_ttoolopt);

  /* How many can we show? */

  most = (buttons_tall * gd_toolopt.cols) - (gd_toolopt.cols * 2) - TOOLOFFSET - 2;
  if (no_magic_groups)
    most = most + gd_toolopt.cols;
  if (disable_magic_controls)
    most = most + gd_toolopt.cols;
  if (disable_magic_sizes)
    most = most + gd_toolopt.cols;

  /* Draw scroll bars, if we need them */

  if (num_magics[magic_group] > most + TOOLOFFSET)
  {
    off_y = img_scroll_down->h;
    max = (most - 2) + TOOLOFFSET;

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h;

    if (magic_scroll[magic_group] > 0)
    {
      SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
    }
    else
    {
      SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);
    }

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h + img_scroll_down->h + ((((most - 2) / 2) + TOOLOFFSET / 2) * button_h);

    if (magic_scroll[magic_group] < num_magics[magic_group] - (most - 2) - TOOLOFFSET)
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


  /* Draw the magic tool buttons */

  for (magic = magic_scroll[magic_group]; magic < magic_scroll[magic_group] + max; magic++)
  {
    i = magic - magic_scroll[magic_group];

    dest.x = ((i % 2) * button_w) + (WINDOW_WIDTH - r_ttoolopt.w);
    dest.y = ((i / 2) * button_h) + r_ttoolopt.h + off_y;

    if (magic < num_magics[magic_group])
    {
      if (magic == cur_magic[magic_group])
      {
        SDL_BlitSurface(img_btn_down, NULL, screen, &dest);
      }
      else
      {
        SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
      }

      dest.x = WINDOW_WIDTH - r_ttoolopt.w + ((i % 2) * button_w) + 4;
      dest.y = ((i / 2) * button_h) + r_ttoolopt.h + 4 + off_y;

      SDL_BlitSurface(magics[magic_group][magic].img_icon, NULL, screen, &dest);


      dest.x =
        WINDOW_WIDTH - r_ttoolopt.w + ((i % 2) * button_w) +
        (4 * button_w) / ORIGINAL_BUTTON_SIZE +
        ((40 * button_w) / ORIGINAL_BUTTON_SIZE - magics[magic_group][magic].img_name->w) / 2;
      dest.y = (((i / 2) * button_h) + r_ttoolopt.h + (4 * button_h) / ORIGINAL_BUTTON_SIZE + ((44 * button_h) / ORIGINAL_BUTTON_SIZE - magics[magic_group][magic].img_name->h) + off_y);       // FIXME: CROP LABELS

      SDL_BlitSurface(magics[magic_group][magic].img_name, NULL, screen, &dest);
    }
    else
    {
      SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
    }
  }


  /* Draw group pagination buttons: */

  if (!no_magic_groups)
  {
    /* Show prev button: */

    button_color = img_black;
    button_body = img_btn_nav;

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h + (((most + TOOLOFFSET) / 2) * button_h);

    SDL_BlitSurface(button_body, NULL, screen, &dest);

    dest.x = WINDOW_WIDTH - r_ttoolopt.w + (button_w - img_prev->w) / 2;
    dest.y = (r_ttoolopt.h + (((most + TOOLOFFSET) / 2) * button_h) + (button_h - img_prev->h) / 2);

    SDL_BlitSurface(button_color, NULL, img_prev, NULL);
    SDL_BlitSurface(img_prev, NULL, screen, &dest);

    /* Show next button: */

    button_color = img_black;
    button_body = img_btn_nav;

    dest.x = WINDOW_WIDTH - button_w;
    dest.y = r_ttoolopt.h + (((most + TOOLOFFSET) / gd_toolopt.cols) * button_h);

    SDL_BlitSurface(button_body, NULL, screen, &dest);

    dest.x = WINDOW_WIDTH - button_w + (button_w - img_next->w) / 2;
    dest.y = (r_ttoolopt.h + (((most + TOOLOFFSET) / gd_toolopt.cols) * button_h) + (button_h - img_next->h) / 2);

    SDL_BlitSurface(button_color, NULL, img_next, NULL);
    SDL_BlitSurface(img_next, NULL, screen, &dest);
  }

  /* Draw magic controls: */

  if (!disable_magic_controls)
  {
    SDL_Surface *button_color;
    int grp, cur;

    grp = magic_group;
    cur = cur_magic[magic_group];


    /* Show paint button: */

    if (magics[grp][cur].mode == MODE_PAINT
        || magics[grp][cur].mode == MODE_ONECLICK || magics[grp][cur].mode == MODE_PAINT_WITH_PREVIEW)
      button_color = img_btn_down;      /* Active */
    else if (magics[grp][cur].avail_modes & MODE_PAINT
             || magics[grp][cur].avail_modes & MODE_ONECLICK || magics[grp][cur].avail_modes & MODE_PAINT_WITH_PREVIEW)
      button_color = img_btn_up;        /* Available, but not active */
    else
      button_color = img_btn_off;       /* Unavailable */

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    // dest.y = r_ttoolopt.h + ((most / gd_toolopt.cols + (TOOLOFFSET + 2) / gd_toolopt.cols) * button_h);
    dest.y = (button_h * buttons_tall + r_ttools.h) - button_h * (disable_magic_sizes ? 1 : 2);

    SDL_BlitSurface(button_color, NULL, screen, &dest);

    dest.x = WINDOW_WIDTH - r_ttoolopt.w + (button_w - img_magic_paint->w) / 2;
    //dest.y =
    //  (r_ttoolopt.h +
    //   ((most / gd_toolopt.cols +
    //     (TOOLOFFSET + 2) / gd_toolopt.cols) * button_h) + (button_h - img_magic_paint->h) / 2);
    dest.y =
      (button_h * buttons_tall + r_ttools.h) -
      button_h * (disable_magic_sizes ? 1 : 2) + ((button_h - img_magic_paint->h) / 2);

    SDL_BlitSurface(img_magic_paint, NULL, screen, &dest);


    /* Show fullscreen button: */

    if (magics[grp][cur].mode == MODE_FULLSCREEN)
      button_color = img_btn_down;      /* Active */
    else if (magics[grp][cur].avail_modes & MODE_FULLSCREEN)
      button_color = img_btn_up;        /* Available, but not active */
    else
      button_color = img_btn_off;       /* Unavailable */

    dest.x = WINDOW_WIDTH - button_w;
    // dest.y = r_ttoolopt.h + ((most / gd_toolopt.cols + (TOOLOFFSET + 2) / gd_toolopt.cols) * button_h);
    dest.y = (button_h * buttons_tall + r_ttools.h) - button_h * (disable_magic_sizes ? 1 : 2);

    SDL_BlitSurface(button_color, NULL, screen, &dest);

    dest.x = WINDOW_WIDTH - button_w + (button_w - img_magic_fullscreen->w) / 2;
    //dest.y =
    //  (r_ttoolopt.h +
    //   ((most / gd_toolopt.cols +
    //     (TOOLOFFSET + 2) / gd_toolopt.cols) * button_h) + (button_h - img_magic_fullscreen->h) / 2);
    dest.y =
      (button_h * buttons_tall + r_ttools.h) -
      button_h * (disable_magic_sizes ? 1 : 2) + ((button_h - img_magic_fullscreen->h) / 2);

    SDL_BlitSurface(img_magic_fullscreen, NULL, screen, &dest);
  }


  /* Draw magic size controls: */

  if (!disable_magic_sizes)
  {
    int grp, cur, mode;

    grp = magic_group;
    cur = cur_magic[magic_group];
    mode = magic_modeint(magics[grp][cur].mode);

    if (magics[grp][cur].sizes[mode] > 1)
    {
      int i, xx, yy, sizes;
      float x_per, y_per;
      SDL_Surface *blnk, *btn;

      sizes = magics[grp][cur].sizes[mode];
      x_per = (float)r_ttoolopt.w / (float)sizes;
      y_per = (float)button_h / (float)(sizes + 1);

      for (i = 1; i < sizes + 1; i++)
      {
        xx = ceil(x_per);
        yy = ceil(y_per * (float)i);

        if (i <= magics[grp][cur].size[mode])
          btn = thumbnail(img_btn_down, xx, yy, 0);
        else
          btn = thumbnail(img_btn_up, xx, yy, 0);

        blnk = thumbnail(img_btn_off, xx, button_h - yy, 0);

        /* FIXME: Check for NULL! */

        dest.x = (WINDOW_WIDTH - r_ttoolopt.w) + ((i - 1) * x_per);
        dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
        SDL_BlitSurface(blnk, NULL, screen, &dest);

        dest.x = (WINDOW_WIDTH - r_ttoolopt.w) + ((i - 1) * x_per);
        dest.y = (button_h * buttons_tall + r_ttools.h) - (y_per * i);
        SDL_BlitSurface(btn, NULL, screen, &dest);

        SDL_FreeSurface(btn);
        SDL_FreeSurface(blnk);
      }
    }
    else
    {
      SDL_Surface *wide_button_off;

      /* Sizing not supported, just draw a big blank */

      wide_button_off = thumbnail(img_btn_off, r_ttoolopt.w, button_h, 0);

      /* FIXME: Check for NULL! */

      dest.x = WINDOW_WIDTH - r_ttoolopt.w;
      dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
      SDL_BlitSurface(wide_button_off, NULL, screen, &dest);

      SDL_FreeSurface(wide_button_off);
    }
  }
}


static unsigned colors_state = COLORSEL_ENABLE | COLORSEL_CLOBBER;

/**
 * FIXME
 */
/* Draw color selector: */
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
    SDL_BlitSurface((colors_state == COLORSEL_ENABLE)
                    ? img_color_btns[i + (i == cur_color) * NUM_COLORS] : img_color_btn_off, NULL, screen, &dest);
  }
  update_screen_rect(&r_colors);

  /* if only the color changed, no need to draw the title */
  if (colors_state == old_colors_state)
    return old_colors_state;

  /* If more than one colors rows, fill the parts of the r_tcolors not covered by the title. */
  if (gd_colors.rows > 1)
    SDL_FillRect(screen, &r_tcolors, SDL_MapRGBA(screen->format, 255, 255, 255, 255));

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


/**
 * FIXME
 */
/* Draw brushes: */
static void draw_brushes(void)
{
  int i, off_y, max, brush;
  SDL_Rect src, dest;
  int most;

  /* Draw the title: */
  draw_image_title(TITLE_BRUSHES, r_ttoolopt);

  /* Space for buttons, was 14 */
  most = (buttons_tall * gd_toolopt.cols) - TOOLOFFSET;
  if (!disable_brushspacing)
  {
    /* Make room for spacing controls */
    most -= 2;
  }

  /* Do we need scrollbars? */

  if (num_brushes > most + TOOLOFFSET)
  {
    most = most - gd_toolopt.cols;      /* was 12 */
    off_y = img_scroll_up->h;
    max = most + TOOLOFFSET;

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h;

    if (brush_scroll > 0)
    {
      SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
    }
    else
    {
      SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);
    }

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h + img_scroll_up->h + ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h);

    if (brush_scroll < num_brushes - most - TOOLOFFSET)
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


  /* Draw each of the shown brushes: */

  for (brush = brush_scroll; brush < brush_scroll + max; brush++)
  {
    i = brush - brush_scroll;


    dest.x = ((i % 2) * button_w) + (WINDOW_WIDTH - r_ttoolopt.w);
    dest.y = ((i / 2) * button_h) + r_ttoolopt.h + off_y;

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
      int ui_btn_x, ui_btn_y;

      if (brushes_directional[brush])
        src.x = (img_brushes_thumbs[brush]->w / abs(brushes_frames[brush])) / 3;
      else
        src.x = 0;

      src.y = brushes_directional[brush] ? (img_brushes_thumbs[brush]->h / 3) : 0;

      src.w = (img_brushes_thumbs[brush]->w / abs(brushes_frames[brush])) / (brushes_directional[brush] ? 3 : 1);
      src.h = (img_brushes_thumbs[brush]->h / (brushes_directional[brush] ? 3 : 1));

      ui_btn_x = ((i % 2) * button_w) + (WINDOW_WIDTH - r_ttoolopt.w);
      ui_btn_y = ((i / 2) * button_h) + r_ttoolopt.h + off_y;

      dest.x = ui_btn_x + ((button_w - src.w) >> 1);
      dest.y = ui_btn_y + ((button_h - src.h) >> 1);

      SDL_BlitSurface(img_brushes_thumbs[brush], &src, screen, &dest);

      if (brushes_directional[brush] || brushes_rotate[brush] || brushes_chaotic[brush])
      {
        dest.x = ui_btn_x + button_w - img_brush_dir->w;
        dest.y = ui_btn_y + button_h - img_brush_dir->h;
        SDL_BlitSurface(img_brush_dir, NULL, screen, &dest);
      }
      if (brushes_frames[brush] != 1)
      {
        dest.x = ui_btn_x;
        dest.y = ui_btn_y + button_h - img_brush_anim->h;
        SDL_BlitSurface(img_brush_anim, NULL, screen, &dest);
      }
    }
  }

  if (!disable_brushspacing)
    draw_brushes_spacing();
}

static void draw_brushes_spacing(void)
{
  int i, frame_w, w, h, size_at;
  float x_per, y_per;
  int xx, yy;
  SDL_Surface *btn, *blnk;
  SDL_Rect dest;

  frame_w = img_brushes[cur_brush]->w / abs(brushes_frames[cur_brush]);
  w = frame_w / (brushes_directional[cur_brush] ? 3 : 1);
  h = img_brushes[cur_brush]->h / (brushes_directional[cur_brush] ? 3 : 1);

  /* Spacing ranges from 0px to "N x the max dimension of the brush"
     (so a 48x48 brush would have a spacing of 48 if the center option is chosen) */
  size_at = ((BRUSH_SPACING_SIZES - 1) * brushes_spacing[cur_brush]) / (max(w, h) * BRUSH_SPACING_MAX_MULTIPLIER);

  x_per = (float)r_ttoolopt.w / BRUSH_SPACING_SIZES;
  y_per = (float)button_h / (BRUSH_SPACING_SIZES + 1);

  for (i = 1; i < BRUSH_SPACING_SIZES + 1; i++)
  {
    xx = ceil(x_per);
    yy = ceil(y_per * i);

    if (i <= size_at + 1)
      btn = thumbnail(img_btn_down, xx, yy, 0);
    else
      btn = thumbnail(img_btn_up, xx, yy, 0);

    blnk = thumbnail(img_btn_off, xx, button_h - yy, 0);

    /* FIXME: Check for NULL! */


    dest.x = (WINDOW_WIDTH - r_ttoolopt.w) + ((i - 1) * x_per);
    dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
    SDL_BlitSurface(blnk, NULL, screen, &dest);

    dest.x = (WINDOW_WIDTH - r_ttoolopt.w) + ((i - 1) * x_per);
    dest.y = (button_h * buttons_tall + r_ttools.h) - (y_per * i);
    SDL_BlitSurface(btn, NULL, screen, &dest);

    SDL_FreeSurface(btn);
    SDL_FreeSurface(blnk);
  }
}

/**
 * FIXME
 */
/* Draw fonts: */
static void draw_fonts(void)
{
  int i, off_y, max, font, most;
  SDL_Rect dest, src;
  SDL_Surface *tmp_surf;
  SDL_Color black = { 0, 0, 0, 0 };

  /* Draw the title: */
  draw_image_title(TITLE_LETTERS, r_ttoolopt);

  /* Space for buttons, was 14 */
  most = (buttons_tall * gd_toolopt.cols) - TOOLOFFSET;

  /* How many can we show? */

  if (cur_tool == TOOL_LABEL)
  {
    most = most - gd_toolopt.cols - gd_toolopt.cols - gd_toolopt.cols;
    if (disable_stamp_controls)
      most = most + gd_toolopt.cols + gd_toolopt.cols;
  }
  else
  {
    most = most - gd_toolopt.cols - gd_toolopt.cols;
    if (disable_stamp_controls)
      most = most + gd_toolopt.cols + gd_toolopt.cols /* Ugly! */ ;
  }

  DEBUG_PRINTF("there are %d font families\n", num_font_families);


  /* Do we need scrollbars? */

  if (num_font_families > most + TOOLOFFSET)
  {
    off_y = img_scroll_up->h;
    max = most - gd_toolopt.cols + TOOLOFFSET;

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h;

    if (font_scroll > 0)
    {
      SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
    }
    else
    {
      SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);
    }

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y =
      r_ttoolopt.h + off_y + (((most - gd_toolopt.cols) / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h);

    if (font_scroll < num_font_families - (most - gd_toolopt.cols) - TOOLOFFSET)
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


    dest.x = ((i % 2) * button_w) + (WINDOW_WIDTH - r_ttoolopt.w);
    dest.y = ((i / 2) * button_h) + r_ttoolopt.h + off_y;

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
      TuxPaint_Font *fonthandle;

      /* Label for 'Letters' buttons (font selector, down the right when
         the Text or Label tool are being used); used to show the difference
         between font faces.  (We'll decide whether a font is suitable to
         use the localized string here, or fallback to "Aa", depending on
         whether the two glyphs render the same. i.e., if they both come
         back as identical rectangles, we know the font doesn't adequately
         support this font.) */
      fonthandle = getfonthandle(font);
      if (charset_works(fonthandle, gettext("Aa")))
      {
        /* Use the localized label string (e.g., "あぁ" in Japanese) */
        DEBUG_PRINTF("Font label '%s' for %s\n", gettext("Aa"), fonthandle->desc);
        tmp_surf_1 = render_text(fonthandle, gettext("Aa"), black);
      }
      else
      {
        /* Fallback; use the latin "Aa" string */
        DEBUG_PRINTF("Fallback font label 'Aa' for %s\n", fonthandle->desc);
        tmp_surf_1 = render_text(fonthandle, "Aa", black);
      }

      if (tmp_surf_1 == NULL)
      {
        fprintf(stderr, "render_text() returned NULL!\n");
        return;
      }

      if (tmp_surf_1->w > button_w || tmp_surf_1->h > button_h)
      {
        tmp_surf = thumbnail(tmp_surf_1, button_w, button_h, 1);
        SDL_FreeSurface(tmp_surf_1);
      }
      else
        tmp_surf = tmp_surf_1;

      src.x = (tmp_surf->w - button_w) / 2;
      src.y = (tmp_surf->h - button_h) / 2;
      src.w = button_w;
      src.h = button_h;

      if (src.x < 0)
        src.x = 0;
      if (src.y < 0)
        src.y = 0;

      dest.x = ((i % 2) * button_w) + (WINDOW_WIDTH - r_ttoolopt.w);
      if (src.w > tmp_surf->w)
      {
        src.w = tmp_surf->w;
        dest.x = dest.x + ((button_w - (tmp_surf->w)) / 2);
      }


      dest.y = ((i / 2) * button_h) + r_ttoolopt.h + off_y;
      if (src.h > tmp_surf->h)
      {
        src.h = tmp_surf->h;
        dest.y = dest.y + ((button_h - (tmp_surf->h)) / 2);
      }

      SDL_BlitSurface(tmp_surf, &src, screen, &dest);

      SDL_FreeSurface(tmp_surf);
    }
  }


  /* Draw text controls: */

  /* Label controls, if in label tool (always!) */

  if (cur_tool == TOOL_LABEL)
  {
    /* "Apply Label" button */
    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h + ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h);
    if (cur_label == LABEL_APPLY)
      SDL_BlitSurface(img_btn_down, NULL, screen, &dest);
    else
    {
      if (are_labels())
        SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
      else
        SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
    }

    dest.x = WINDOW_WIDTH - r_ttoolopt.w + (button_w - img_label_apply->w) / 2;
    dest.y =
      (r_ttoolopt.h +
       ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h) + (button_h - img_label_apply->h) / 2);

    SDL_BlitSurface(img_label_apply, NULL, screen, &dest);


    /* "Select Label" button */
    dest.x = WINDOW_WIDTH - button_w;
    dest.y = r_ttoolopt.h + ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h);

    if (cur_label == LABEL_SELECT)
      SDL_BlitSurface(img_btn_down, NULL, screen, &dest);
    else
    {
      if (are_labels())
        SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
      else
        SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
    }


    dest.x = WINDOW_WIDTH - button_w + (button_w - img_label_select->w) / 2;
    dest.y =
      (r_ttoolopt.h +
       ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h) + (button_h - img_label_select->h) / 2);

    SDL_BlitSurface(img_label_select, NULL, screen, &dest);

    most = most + gd_toolopt.cols;
  }


  /* Size, italic, and bold, only appear when not UI is not being simplified */
  if (!disable_stamp_controls)
  {
    SDL_Surface *button_color;
    SDL_Surface *button_body;

    // label_ctrl_y = r_ttoolopt.h + ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h);

    /* Show bold button: */

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h + ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h);

    if (text_state & TTF_STYLE_BOLD)
      SDL_BlitSurface(img_btn_down, NULL, screen, &dest);
    else
      SDL_BlitSurface(img_btn_up, NULL, screen, &dest);

    dest.x = WINDOW_WIDTH - r_ttoolopt.w + (button_w - img_bold->w) / 2;
    dest.y =
      (r_ttoolopt.h +
       ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h) + (button_h - img_bold->h) / 2);

    SDL_BlitSurface(img_bold, NULL, screen, &dest);


    /* Show italic button: */

    dest.x = WINDOW_WIDTH - button_w;
    dest.y = r_ttoolopt.h + ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h);

    if (text_state & TTF_STYLE_ITALIC)
      SDL_BlitSurface(img_btn_down, NULL, screen, &dest);
    else
      SDL_BlitSurface(img_btn_up, NULL, screen, &dest);

    dest.x = WINDOW_WIDTH - button_w + (button_w - img_italic->w) / 2;
    dest.y =
      (r_ttoolopt.h +
       ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h) + (button_h - img_italic->h) / 2);

    SDL_BlitSurface(img_italic, NULL, screen, &dest);

    most = most + gd_toolopt.cols;


    /* Show shrink button: */

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h + ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h);

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

    dest.x = WINDOW_WIDTH - r_ttoolopt.w + (button_w - img_shrink->w) / 2;
    dest.y =
      (r_ttoolopt.h +
       ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h) + (button_h - img_shrink->h) / 2);

    SDL_BlitSurface(button_color, NULL, img_shrink, NULL);
    SDL_BlitSurface(img_shrink, NULL, screen, &dest);


    /* Show grow button: */

    dest.x = WINDOW_WIDTH - button_w;
    dest.y = r_ttoolopt.h + ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h);

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

    dest.x = WINDOW_WIDTH - button_w + (button_w - img_grow->w) / 2;
    dest.y =
      (r_ttoolopt.h +
       ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h) + (button_h - img_grow->h) / 2);

    SDL_BlitSurface(button_color, NULL, img_grow, NULL);
    SDL_BlitSurface(img_grow, NULL, screen, &dest);
  }
}


/**
 * FIXME
 */
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

  if (!disable_stamp_controls)
  {
    if (!no_stamp_rotation)
    {
      most = ((buttons_tall - 4) * gd_toolopt.cols) - TOOLOFFSET;
    }
    else
    {
      most = ((buttons_tall - 3) * gd_toolopt.cols) - TOOLOFFSET;
    }
  }
  else
  {
    most = ((buttons_tall - 1) * gd_toolopt.cols) - TOOLOFFSET;
  }


  /* Do we need scrollbars? */

  if (num_stamps[stamp_group] > most + TOOLOFFSET)
  {
    /* Yes, need scrollbars */

    off_y = img_scroll_up->h;
    max = (most - gd_toolopt.cols) + TOOLOFFSET;

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h;

    if (stamp_scroll[stamp_group] > 0)
    {
      SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
    }
    else
    {
      SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);
    }


    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h + off_y + ((((most + TOOLOFFSET) / gd_toolopt.cols) - 1) * button_h);

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
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
    /* No, do not need scrollbars */

    off_y = 0;
    max = most + TOOLOFFSET;
  }


  /* Draw each of the shown stamps: */

  for (stamp = stamp_scroll[stamp_group]; stamp < stamp_scroll[stamp_group] + max; stamp++)
  {
    i = stamp - stamp_scroll[stamp_group];

    dest.x = ((i % 2) * button_w) + (WINDOW_WIDTH - r_ttoolopt.w);
    dest.y = ((i / 2) * button_h) + r_ttoolopt.h + off_y;

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
      /* Loads the thumbnail and sounds, the sounds just if this is the current stamp, increasing responsivity for low powered devices */
      get_stamp_thumb(stamp_data[stamp_group][stamp], stamp == cur_stamp[stamp_group] ? 1 : 0);
      img = stamp_data[stamp_group][stamp]->thumbnail;

      base_x = ((i % 2) * button_w) + (WINDOW_WIDTH - r_ttoolopt.w) + ((button_w - (img->w)) / 2);

      base_y = ((i / 2) * button_h) + r_ttoolopt.h + ((button_h - (img->h)) / 2) + off_y;

      dest.x = base_x;
      dest.y = base_y;

      SDL_BlitSurface(img, NULL, screen, &dest);
    }
  }


  /* Draw stamp group buttons (prev/next): */

  /* Show prev button: */

  button_color = img_black;
  button_body = img_btn_nav;

  dest.x = WINDOW_WIDTH - r_ttoolopt.w;
  dest.y = r_ttoolopt.h + (((most + TOOLOFFSET) / 2) * button_h);

  SDL_BlitSurface(button_body, NULL, screen, &dest);

  dest.x = WINDOW_WIDTH - r_ttoolopt.w + (button_w - img_prev->w) / 2;
  dest.y = (r_ttoolopt.h + (((most + TOOLOFFSET) / 2) * button_h) + (button_h - img_prev->h) / 2);

  SDL_BlitSurface(button_color, NULL, img_prev, NULL);
  SDL_BlitSurface(img_prev, NULL, screen, &dest);

  /* Show next button: */

  button_color = img_black;
  button_body = img_btn_nav;

  dest.x = WINDOW_WIDTH - button_w;
  dest.y = r_ttoolopt.h + (((most + TOOLOFFSET) / gd_toolopt.cols) * button_h);

  SDL_BlitSurface(button_body, NULL, screen, &dest);

  dest.x = WINDOW_WIDTH - button_w + (button_w - img_next->w) / 2;
  dest.y = (r_ttoolopt.h + (((most + TOOLOFFSET) / gd_toolopt.cols) * button_h) + (button_h - img_next->h) / 2);

  SDL_BlitSurface(button_color, NULL, img_next, NULL);
  SDL_BlitSurface(img_next, NULL, screen, &dest);


  /* Draw stamp controls: */

  if (!disable_stamp_controls)
  {
    if (!no_stamp_rotation)
    {
      /* Show rotation button */

      dest.x = WINDOW_WIDTH - r_ttoolopt.w;
      dest.y = r_ttoolopt.h + ((most + gd_toolopt.cols + TOOLOFFSET) / gd_toolopt.cols * button_h);

      if (stamp_rotation_ctrl)
        button_body = img_btn_down;
      else
        button_body = img_btn_up;

      SDL_BlitSurface(button_body, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - (button_w * 2) + (button_w - img_rotate->w) / 2;
      dest.y =
        (r_ttoolopt.h +
         ((most + gd_toolopt.cols + TOOLOFFSET) / gd_toolopt.cols * button_h) + (button_h - img_rotate->h) / 2);

      SDL_BlitSurface(img_black, NULL, img_rotate, NULL);
      SDL_BlitSurface(img_rotate, NULL, screen, &dest);

      /* No-op button */

      dest.x = WINDOW_WIDTH - r_ttoolopt.w + button_w;
      dest.y = r_ttoolopt.h + ((most + gd_toolopt.cols + TOOLOFFSET) / gd_toolopt.cols * button_h);

      SDL_BlitSurface(img_btn_off, NULL, screen, &dest);


      /* Push other buttons down */
      off_y = button_h;
    }
    else
    {
      off_y = 0;
    }                           /* !no_stamp_rotation */


    /* Show mirror button: */

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h + off_y + ((most + gd_toolopt.cols + TOOLOFFSET) / gd_toolopt.cols * button_h);

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

    dest.x = WINDOW_WIDTH - r_ttoolopt.w + (button_w - img_mirror->w) / 2;
    dest.y =
      (r_ttoolopt.h + off_y +
       ((most + gd_toolopt.cols + TOOLOFFSET) / gd_toolopt.cols * button_h) + (button_h - img_mirror->h) / 2);

    SDL_BlitSurface(button_color, NULL, img_mirror, NULL);
    SDL_BlitSurface(img_mirror, NULL, screen, &dest);

    /* Show flip button: */

    dest.x = WINDOW_WIDTH - button_w;
    dest.y = r_ttoolopt.h + off_y + ((most + gd_toolopt.cols + TOOLOFFSET) / gd_toolopt.cols * button_h);

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

    dest.x = WINDOW_WIDTH - button_w + (button_w - img_flip->w) / 2;
    dest.y =
      (r_ttoolopt.h + off_y +
       ((most + gd_toolopt.cols + TOOLOFFSET) / gd_toolopt.cols * button_h) + (button_h - img_flip->h) / 2);

    SDL_BlitSurface(button_color, NULL, img_flip, NULL);
    SDL_BlitSurface(img_flip, NULL, screen, &dest);

    /* Stamp size control: */
    sizes = MAX_STAMP_SIZE - MIN_STAMP_SIZE + 1;        /* +1 for SF Bug #1668235 -bjk 2011.01.08 */
    size_at = (stamp_data[stamp_group][cur_stamp[stamp_group]]->size - MIN_STAMP_SIZE);

    x_per = (float)r_ttoolopt.w / sizes;
    y_per = (float)button_h / sizes;

    for (i = 0; i < sizes; i++)
    {
      xx = ceil(x_per);
      yy = ceil(y_per * i);

      if (i <= size_at)
        btn = thumbnail(img_btn_down, xx, yy, 0);
      else
        btn = thumbnail(img_btn_up, xx, yy, 0);

      blnk = thumbnail(img_btn_off, xx, button_h - yy, 0);

      /* FIXME: Check for NULL! */

      dest.x = (WINDOW_WIDTH - r_ttoolopt.w) + (i * x_per);
      dest.y =
        (((most + gd_toolopt.cols + gd_toolopt.cols + gd_toolopt.cols +
           TOOLOFFSET) / gd_toolopt.cols * button_h)) - 8 * button_scale + off_y;
      SDL_BlitSurface(blnk, NULL, screen, &dest);

      dest.x = (WINDOW_WIDTH - r_ttoolopt.w) + (i * x_per);
      dest.y =
        (((most + gd_toolopt.cols + gd_toolopt.cols + gd_toolopt.cols +
           gd_toolopt.cols + TOOLOFFSET) / gd_toolopt.cols * button_h)) - 8 * button_scale - (y_per * i) + off_y;
      SDL_BlitSurface(btn, NULL, screen, &dest);

      SDL_FreeSurface(btn);
      SDL_FreeSurface(blnk);
    }
  }                             /* !disable_stamp_controls */

  redraw_tux_text();
}


/**
 * FIXME
 */
/* Draw the shape selector: */
static void draw_shapes(void)
{
  int i, shape, max, off_y, most;
  SDL_Rect dest;


  draw_image_title(TITLE_SHAPES, r_ttoolopt);

  if (disable_shape_controls)
    most = (buttons_tall * gd_toolopt.cols) - TOOLOFFSET;
  else
    most = (buttons_tall * gd_toolopt.cols) - gd_toolopt.cols - TOOLOFFSET;

  if (NUM_SHAPES > most + TOOLOFFSET)
  {
    off_y = img_scroll_up->h;
    max = (most - 2) + TOOLOFFSET;

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h;

    if (shape_scroll > 0)
    {
      SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
    }
    else
    {
      SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);
    }

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h + img_scroll_up->h + ((((most - 2) / 2) + TOOLOFFSET / 2) * button_h);

    if (shape_scroll < NUM_SHAPES - (most - 2) - TOOLOFFSET)
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

  for (shape = shape_scroll; shape < shape_scroll + max; shape++)
  {
    i = shape - shape_scroll;

    dest.x = ((i % 2) * button_w) + WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = ((i / 2) * button_h) + r_ttoolopt.h + off_y;

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
      dest.x = ((i % 2) * button_w) + (4 * button_w) / ORIGINAL_BUTTON_SIZE + WINDOW_WIDTH - r_ttoolopt.w;
      dest.y = ((i / 2) * button_h) + r_ttoolopt.h + (4 * button_h) / ORIGINAL_BUTTON_SIZE + off_y;

      SDL_BlitSurface(img_shapes[shape], NULL, screen, &dest);

      dest.x =
        ((i % 2) * button_w) + (4 * button_w) / ORIGINAL_BUTTON_SIZE +
        WINDOW_WIDTH - r_ttoolopt.w + ((40 * button_w) / ORIGINAL_BUTTON_SIZE - img_shape_names[shape]->w) / 2;
      dest.y = ((i / 2) * button_h) + r_ttoolopt.h + (4 * button_h) / ORIGINAL_BUTTON_SIZE + ((44 * button_h) / ORIGINAL_BUTTON_SIZE - img_shape_names[shape]->h) +     // FIXME: CROP LABELS
        off_y;

      SDL_BlitSurface(img_shape_names[shape], NULL, screen, &dest);
    }
  }

  /* Draw shape tool controls: */

  if (!disable_shape_controls)
  {
    SDL_Surface *button_color;

    /* Show shape-from-center button: */

    if (shape_mode == SHAPEMODE_CENTER)
      button_color = img_btn_down;
    else
      button_color = img_btn_up;

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h + ((most + TOOLOFFSET) / gd_toolopt.cols * button_h);

    SDL_BlitSurface(button_color, NULL, screen, &dest);

    dest.x = WINDOW_WIDTH - r_ttoolopt.w + (button_w - img_shapes_center->w) / 2;
    dest.y =
      (r_ttoolopt.h + ((most + TOOLOFFSET) / gd_toolopt.cols * button_h) + (button_h - img_shapes_center->h) / 2);

    SDL_BlitSurface(img_shapes_center, NULL, screen, &dest);


    /* Show shape-from-corner button: */

    if (shape_mode == SHAPEMODE_CORNER)
      button_color = img_btn_down;
    else
      button_color = img_btn_up;

    dest.x = WINDOW_WIDTH - button_w;
    dest.y = r_ttoolopt.h + ((most + TOOLOFFSET) / gd_toolopt.cols * button_h);

    SDL_BlitSurface(button_color, NULL, screen, &dest);

    dest.x = WINDOW_WIDTH - button_w + (button_w - img_shapes_corner->w) / 2;
    dest.y =
      (r_ttoolopt.h + ((most + TOOLOFFSET) / gd_toolopt.cols * button_h) + (button_h - img_shapes_corner->h) / 2);

    SDL_BlitSurface(img_shapes_corner, NULL, screen, &dest);
  }
}


/**
 * FIXME
 */
/* Draw the eraser selector: */
static void draw_erasers(void)
{
  int i, j, x, y, sz;
  void (*putpixel)(SDL_Surface *, int, int, Uint32);
  SDL_Rect dest;
  int most, off_y;
  int eraser_type, fuzzy, trans;

  putpixel = putpixels[screen->format->BytesPerPixel];

  draw_image_title(TITLE_ERASERS, r_ttoolopt);

  /* Space for buttons */
  most = (buttons_tall * gd_toolopt.cols) - TOOLOFFSET;

  /* Do we need scrollbars? */
  if (NUM_ERASERS > most + TOOLOFFSET)
  {
    most = most - gd_toolopt.cols;
    off_y = img_scroll_up->h;

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h;

    if (eraser_scroll > 0)
    {
      SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
    }
    else
    {
      SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);
    }

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h + img_scroll_up->h + ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h);

    if (eraser_scroll < NUM_ERASERS - most - TOOLOFFSET)
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
  }

  for (j = 0; j < most + TOOLOFFSET; j++)
  {
    i = j;
    dest.x = ((i % 2) * button_w) + WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = ((i / 2) * button_h) + r_ttoolopt.h + off_y;

    i = j + eraser_scroll;

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
      eraser_type = i / NUM_ERASER_SIZES;

      sz = (2 + ((NUM_ERASER_SIZES - 1 - (i % NUM_ERASER_SIZES)) * (38 / (NUM_ERASER_SIZES - 1)))) * button_scale;
      if (sz < 4)
        sz = 4;

      x = ((i % 2) * button_w) + WINDOW_WIDTH - r_ttoolopt.w + 24 * button_scale - sz / 2;
      y = ((j / 2) * button_h) + r_ttoolopt.h + 24 * button_scale - sz / 2 + off_y;

      fuzzy = (eraser_type == ERASER_TYPE_CIRCLE_FUZZY);
      trans = (eraser_type == ERASER_TYPE_CIRCLE_TRANSPARENT);

      if (eraser_type == ERASER_TYPE_SQUARE)
      {
        /* Square */

        dest.x = x;
        dest.y = y;
        dest.w = sz;
        dest.h = sz;

        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));
      }
      else
      {
        /* Circle */

        int rad, rad_sqr;
        int yy, w, sx, sy;

        if (fuzzy || trans)
        {
          /* Fuzzy or transparent; draw dithered circle */
          rad = sz / 2;
          rad_sqr = (rad * rad);

          for (yy = -rad; yy <= rad; yy++)
          {
            w = sqrt(rad_sqr - (yy * yy));
            sx = x + rad - w;
            sy = y + rad + yy;

            if (fuzzy || trans)
            {
              int xxx;

              for (xxx = 0; xxx < w * 2; xxx++)
              {
                if ((sx + xxx) % 2 == sy % 2)
                {
                  putpixel(screen, sx + xxx, sy, SDL_MapRGB(screen->format, 0, 0, 0));
                }
              }
            }
          }
        }

        if (fuzzy || !trans)
        {
          /* Solid or fuzzy, draw solid circle */

          if (fuzzy)
          {
            /* Fuzzy's solid circle is within the dithered circle drawn above */
            sz -= 2;
            x++;
            y++;
          }

          if (sz > 0)
          {
            rad = sz / 2;
            rad_sqr = (rad * rad);

            for (yy = -rad; yy <= rad; yy++)
            {
              w = sqrt(rad_sqr - (yy * yy));
              sx = x + rad - w;
              sy = y + rad + yy;

              dest.x = sx;
              dest.y = sy;
              dest.w = w * 2;
              dest.h = 1;

              SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));
            }
          }
        }
      }
    }
  }
}


/**
 * FIXME
 */
/* Draw no selectables: */
static void draw_none(void)
{
  int i;
  SDL_Rect dest;

  dest.x = WINDOW_WIDTH - r_ttoolopt.w;
  dest.y = 0;
  SDL_BlitSurface(img_title_off, NULL, screen, &dest);

  for (i = 0; i < buttons_tall * gd_toolopt.cols; i++)
  {
    dest.x = ((i % 2) * button_w) + WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = ((i / 2) * button_h) + r_ttoolopt.h;

    SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
  }
}

/* Draw Fill sub-tools */
static void draw_fills(void)
{
  int i, j;
  SDL_Rect dest;
  int most, off_y;

  draw_image_title(TITLE_FILLS, r_ttoolopt);

  /* Space for buttons, was 14 */
  most = (buttons_tall * gd_toolopt.cols) - TOOLOFFSET;


  /* Do we need scrollbars? */

  if (NUM_FILLS > most + TOOLOFFSET)
  {
    most = most - gd_toolopt.cols;      /* was 12 */
    off_y = img_scroll_up->h;

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h;

    if (fill_scroll > 0)
    {
      SDL_BlitSurface(img_scroll_up, NULL, screen, &dest);
    }
    else
    {
      SDL_BlitSurface(img_scroll_up_off, NULL, screen, &dest);
    }

    dest.x = WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = r_ttoolopt.h + img_scroll_up->h + ((most / gd_toolopt.cols + TOOLOFFSET / gd_toolopt.cols) * button_h);

    if (fill_scroll < NUM_FILLS - most - TOOLOFFSET)
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
  }

  for (j = 0; j < most + TOOLOFFSET; j++)
  {
    i = j;
    dest.x = ((i % 2) * button_w) + WINDOW_WIDTH - r_ttoolopt.w;
    dest.y = ((i / 2) * button_h) + r_ttoolopt.h + off_y;

    i = j + fill_scroll;

    if (i == cur_fill)
    {
      SDL_BlitSurface(img_btn_down, NULL, screen, &dest);
    }
    else if (i < NUM_FILLS)
    {
      SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
    }
    else
    {
      SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
    }

    if (i < NUM_FILLS)
    {
      dest.x = ((i % 2) * button_w) + (4 * button_w) / ORIGINAL_BUTTON_SIZE + WINDOW_WIDTH - r_ttoolopt.w;
      dest.y = ((i / 2) * button_h) + r_ttoolopt.h + (4 * button_h) / ORIGINAL_BUTTON_SIZE + off_y;

      SDL_BlitSurface(img_fills[i], NULL, screen, &dest);

      dest.x =
        ((i % 2) * button_w) + (4 * button_w) / ORIGINAL_BUTTON_SIZE +
        WINDOW_WIDTH - r_ttoolopt.w + ((40 * button_w) / ORIGINAL_BUTTON_SIZE - img_fill_names[i]->w) / 2;
      dest.y = ((i / 2) * button_h) + r_ttoolopt.h + (4 * button_h) / ORIGINAL_BUTTON_SIZE + ((44 * button_h) / ORIGINAL_BUTTON_SIZE - img_fill_names[i]->h) +  // FIXME: CROP LABELS
        off_y;

      SDL_BlitSurface(img_fill_names[i], NULL, screen, &dest);
    }
  }
}

/**
 * FIXME
 */
/* Create a thumbnail: */
static SDL_Surface *thumbnail(SDL_Surface *src, int max_x, int max_y, int keep_aspect)
{
  return (thumbnail2(src, max_x, max_y, keep_aspect, 1));
}

/**
 * FIXME
 */
static SDL_Surface *thumbnail2(SDL_Surface *src, int max_x, int max_y, int keep_aspect, int keep_alpha)
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
  int new_x, new_y;
  float xscale, yscale;
  int tmp;
  void (*putpixel)(SDL_Surface *, int, int, Uint32);

  Uint32(*getpixel) (SDL_Surface *, int, int) = getpixels[src->format->BytesPerPixel];

  /* Determine scale and centering offsets: */
  if (!keep_aspect)
  {
    DEBUG_PRINTF("thumbnail2() asked for %d x %d => %d x %d, DON'T keep aspect\n", src->w, src->h, max_x, max_y);

    yscale = (float)((float)src->h / (float)max_y);
    xscale = (float)((float)src->w / (float)max_x);
  }
  else
  {
    float scale_factor;
    int sx, sy;

    scale_factor = pick_best_scape(src->w, src->h, max_x, max_y);

    sx = ((float)src->w * scale_factor);
    sy = ((float)src->h * scale_factor);

    yscale = (float)((float)src->h / (float)sy);
    xscale = (float)((float)src->w / (float)sx);
  }

  new_x = ceil((float)src->w / xscale);
  new_y = ceil((float)src->h / yscale);

  if (new_x > max_x)
    new_x = max_x;
  if (new_y > max_y)
    new_y = max_y;

  if (!keep_aspect)
  {
    off_x = 0;
    off_y = 0;
  }
  else
  {
    off_x = ((float)max_x - (float)new_x) / 2.0;
    off_y = ((float)max_y - (float)new_y) / 2.0;
    DEBUG_PRINTF("  off_x = (%d - %d) / 2 = %.2f\n", max_x, new_x, off_x);
    DEBUG_PRINTF("  off_y = (%d - %d) / 2 = %.2f\n", max_y, new_y, off_y);
  }

#ifndef NO_BILINEAR
  if (max_x > src->w && max_y > src->h)
  {
    DEBUG_PRINTF("Calling zoom(%d,%d)\n", new_x, new_y);
    return (zoom(src, new_x, new_y));
  }
#endif


  /* Create surface for thumbnail: */

  s = SDL_CreateRGBSurface(src->flags,  /* SDL_SWSURFACE, */
                           max_x, max_y, src->format->BitsPerPixel,
                           src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);


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
          /* per: http://www.ericbrasseur.org/gamma.html?i=1 */

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
          putpixel(s, x + off_x, y + off_y, SDL_MapRGBA(s->format, (Uint8) tr, (Uint8) tg, (Uint8) tb, (Uint8) ta));
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

/**
 * FIXME
 */
/* Based on code from: http://www.codeproject.com/cs/media/imageprocessing4.asp
   copyright 2002 Christian Graus */
static SDL_Surface *zoom(SDL_Surface *src, int new_w, int new_h)
{
  SDL_Surface *s;
  void (*putpixel)(SDL_Surface *, int, int, Uint32);

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
      {                         //EP added local block to avoid warning "Passing arg 3 from incompatible pointer type" of section below block
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

        r = g = b = a = 0;      /* Unused, bah! */

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
static void _xorpixel(SDL_Surface *surf, int x, int y)
{
  Uint8 *p;
  int BytesPerPixel;

  /* if outside the canvas, return */
  if (x < 0 || x >= surf->w || y < 0 || y >= surf->h)
    return;

  /* Always 4, except 3 when loading a saved image. */
  BytesPerPixel = surf->format->BytesPerPixel;

  /* Set a pointer to the exact location in memory of the pixel */
  p = (Uint8 *) (((Uint8 *) surf->pixels) +     /* Start: beginning of RAM */
                 (y * surf->pitch) +    /* Go down Y lines */
                 (x * BytesPerPixel));  /* Go in X pixels */

  /* XOR the (correctly-sized) piece of data in the surface's RAM */
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

/**
 * FIXME
 */
static void xorpixel(int x, int y)
{
  /* if outside the canvas, return */
  if ((unsigned)x >= (unsigned)canvas->w || (unsigned)y >= (unsigned)canvas->h)
    return;

  /* now switch to screen coordinates */
  x += r_canvas.x;
  y += r_canvas.y;

  _xorpixel(screen, x, y);
}


/**
 * FIXME
 */
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

    DEBUG_PRINTF("BLITTING: %d\n", cur_undo);
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

    update_canvas(0, 0, (WINDOW_WIDTH - r_ttoolopt.w), (button_h * 7) + 40 + HEIGHTOFFSET);


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

  DEBUG_PRINTF("UNDO: Current=%d  Oldest=%d  Newest=%d\n", cur_undo, oldest_undo, newest_undo);
}


/**
 * FIXME
 */
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

    DEBUG_PRINTF("BLITTING: %d\n", cur_undo);

    do_redo_label_node();
    SDL_BlitSurface(undo_bufs[cur_undo], NULL, canvas, NULL);

    update_canvas(0, 0, (WINDOW_WIDTH - r_ttoolopt.w), (button_h * 7) + 40 + HEIGHTOFFSET);

    been_saved = 0;
  }

  DEBUG_PRINTF("REDO: Current=%d  Oldest=%d  Newest=%d\n", cur_undo, oldest_undo, newest_undo);


  if (((cur_undo + 1) % NUM_UNDO_BUFS) == newest_undo)
  {
    tool_avail[TOOL_REDO] = 0;
  }

  tool_avail[TOOL_UNDO] = 1;

  draw_toolbar();
  update_screen_rect(&r_tools);
}


/**
 * FIXME
 */
/* Create the current brush in the current color: */
static void render_brush(void)
{
  Uint32 amask;
  int x, y;
  Uint8 r, g, b, a;

  Uint32(*getpixel_brush) (SDL_Surface *, int, int) = getpixels[img_brushes[cur_brush]->format->BytesPerPixel];
  void (*putpixel_brush)(SDL_Surface *, int, int, Uint32) = putpixels[img_brushes[cur_brush]->format->BytesPerPixel];


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
        /* FIXME: This doesn't seem necessary, since B&W brushes work fine
           using the `else` codepath below.  Brushes like `impasto.png`
           do not work properly without having a slight non-greyscale tint
           applied. -bjk 2024.10.20 */
        putpixel_brush(img_cur_brush, x, y,
                       SDL_MapRGBA(img_cur_brush->format,
                                   color_hexes[cur_color][0], color_hexes[cur_color][1], color_hexes[cur_color][2], a));
      }
      else
      {
        putpixel_brush(img_cur_brush, x, y,
                       SDL_MapRGBA(img_cur_brush->format,
                                   (r + color_hexes[cur_color][0]) >> 1,
                                   (g + color_hexes[cur_color][1]) >> 1, (b + color_hexes[cur_color][2]) >> 1, a));
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
  img_cur_brush_rotate = brushes_rotate[cur_brush];
  img_cur_brush_chaotic = brushes_chaotic[cur_brush];
  img_cur_brush_spacing = brushes_spacing[cur_brush];

  brush_counter = 0;
}


/**
 * Show a tool tip, for when the brush choice has changed
 */
static void show_brush_tip(void)
{
  if (brushes_descr[cur_brush] != NULL)
  {
    draw_tux_text_ex(TUX_GREAT, brushes_descr[cur_brush], 1, brushes_descr_localized[cur_brush]);

  }
  else
  {
    if (img_cur_brush_rotate || img_cur_brush_directional)
    {
      if (abs(img_cur_brush_frames) > 1)
      {
        draw_tux_text(TUX_GREAT, TIP_BRUSH_CHOICE_ANM_DIR, 1);
      }
      else
      {
        draw_tux_text(TUX_GREAT, TIP_BRUSH_CHOICE_DIR, 1);
      }
    }
    else if (abs(img_cur_brush_frames) > 1)
    {
      draw_tux_text(TUX_GREAT, TIP_BRUSH_CHOICE_ANM, 1);
    }
    else
    {
      draw_tux_text(TUX_GREAT, tool_tips[cur_tool], 1);
    }
  }
}


/**
 * FIXME
 */
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


/**
 * Draw a XOR rectangle on the canvas.
 * @param x1 left edge
 * @param y1 top edge
 * @param x2 right edge
 * @param y2 bottom edge
 */
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

  if (x1 >= (WINDOW_WIDTH - r_ttoolopt.w - r_ttools.w))
    x1 = (WINDOW_WIDTH - r_ttoolopt.w - r_ttools.w) - 1;

  if (x2 >= (WINDOW_WIDTH - r_ttoolopt.w - r_ttools.w))
    x2 = (WINDOW_WIDTH - r_ttoolopt.w - r_ttools.w) - 1;

  if (y1 >= (button_h * 7) + 40 + HEIGHTOFFSET)
    y1 = (button_h * 7) + 40 + HEIGHTOFFSET - 1;

  if (y2 >= (button_h * 7) + 40 + HEIGHTOFFSET)
    y2 = (button_h * 7) + 40 + HEIGHTOFFSET - 1;

  line_xor(x1, y1, x2, y1);
  line_xor(x2, y1, x2, y2);
  line_xor(x2, y2, x1, y2);
  line_xor(x1, y2, x1, y1);
}


/**
 * Draw a XOR circle on the canvas.
 * @param x center x position
 * @param y center y position
 * @param sz size (radius)
 */
static void circle_xor(int x, int y, int sz)
{
  int xx, yy, sz2;

  /* h/t http://groups.csail.mit.edu/graphics/classes/6.837/F98/Lecture6/circle.html */

  sz2 = sz * sz;

  xorpixel(x, y + sz);
  xorpixel(x, y - sz);

  for (xx = 1; xx < sz; xx++)
  {
    yy = sqrt(sz2 - (xx * xx) + 0.5);
    xorpixel(x + xx, y + yy);
    xorpixel(x + xx, y - yy);
    xorpixel(x - xx, y + yy);
    xorpixel(x - xx, y - yy);
  }
}


static int calc_eraser_size(int which_eraser)
{
  while (which_eraser >= NUM_ERASER_SIZES)
    which_eraser -= NUM_ERASER_SIZES;

  return (((NUM_ERASER_SIZES - 1 - which_eraser) * ((ERASER_MAX - ERASER_MIN) / (NUM_ERASER_SIZES - 1))) + ERASER_MIN);
}

/**
 * FIXME
 */
/* Erase at the cursor! */
static void do_eraser(int x, int y, int update)
{
  SDL_Rect dest;
  int sz;
  int xx, yy, n, hit;
  int eraser_type;
  int undo_ctr;
  SDL_Surface *last;

  if (cur_undo > 0)
    undo_ctr = cur_undo - 1;
  else
    undo_ctr = NUM_UNDO_BUFS - 1;

  last = undo_bufs[undo_ctr];


  sz = calc_eraser_size(cur_eraser);
  eraser_type = cur_eraser / NUM_ERASER_SIZES;

  if (eraser_type == ERASER_TYPE_SQUARE)
  {
    /* Square eraser: */

    dest.x = x - (sz / 2);
    dest.y = y - (sz / 2);
    dest.w = sz;
    dest.h = sz;

    if (img_starter_bkgd == NULL)
      SDL_FillRect(canvas, &dest, SDL_MapRGB(canvas->format, canvas_color_r, canvas_color_g, canvas_color_b));
    else
      SDL_BlitSurface(img_starter_bkgd, &dest, canvas, &dest);
  }
  else if (eraser_type == ERASER_TYPE_CIRCLE)
  {
    /* Round sharp eraser: */

    for (yy = 0; yy <= sz; yy++)
    {
      hit = 0;
      for (xx = 0; xx <= sz && hit == 0; xx++)
      {
        n = (xx * xx) + (yy * yy) - ((sz * sz) / 4);

        if (n >= -sz && n <= sz)
          hit = 1;

        if (hit)
        {
          dest.x = x - xx;
          dest.y = y - yy;
          dest.w = xx * 2;
          dest.h = 1;

          if (img_starter_bkgd == NULL)
            SDL_FillRect(canvas, &dest, SDL_MapRGB(canvas->format, canvas_color_r, canvas_color_g, canvas_color_b));
          else
            SDL_BlitSurface(img_starter_bkgd, &dest, canvas, &dest);


          dest.x = x - xx;
          dest.y = y + yy;
          dest.w = xx * 2;
          dest.h = 1;

          if (img_starter_bkgd == NULL)
            SDL_FillRect(canvas, &dest, SDL_MapRGB(canvas->format, canvas_color_r, canvas_color_g, canvas_color_b));
          else
            SDL_BlitSurface(img_starter_bkgd, &dest, canvas, &dest);
        }
      }
    }
  }
  else if (eraser_type == ERASER_TYPE_CIRCLE_FUZZY || eraser_type == ERASER_TYPE_CIRCLE_TRANSPARENT)
  {
    Uint8 r_erase, g_erase, b_erase;
    Uint8 r_canvas, g_canvas, b_canvas;

    Uint32(*getpixel_bkgd) (SDL_Surface *, int, int) = NULL;
    Uint32(*getpixel_canvas) (SDL_Surface *, int, int) = getpixels[canvas->format->BytesPerPixel];
    void (*putpixel)(SDL_Surface *, int, int, Uint32) = putpixels[canvas->format->BytesPerPixel];
    float sq, erase_pct, canvas_pct, r, g, b;

    /* Round fuzzy eraser & round transparent erasers: */

    r_erase = canvas_color_r;
    g_erase = canvas_color_g;
    b_erase = canvas_color_b;

    if (img_starter_bkgd != NULL)
      getpixel_bkgd = getpixels[img_starter_bkgd->format->BytesPerPixel];

    for (yy = -sz / 2; yy <= sz / 2; yy++)
    {
      for (xx = -sz / 2; xx <= sz / 2; xx++)
      {
        sq = sqrt((xx * xx) + (yy * yy));

        if (sq <= sz / 2)
        {
          if (img_starter_bkgd != NULL)
            SDL_GetRGB(getpixel_bkgd(img_starter_bkgd, x + xx, y + yy),
                       img_starter_bkgd->format, &r_erase, &g_erase, &b_erase);

          if (eraser_type == ERASER_TYPE_CIRCLE_FUZZY)
          {
            /* Fuzzy */
            SDL_GetRGB(getpixel_canvas(canvas, x + xx, y + yy), canvas->format, &r_canvas, &g_canvas, &b_canvas);

            canvas_pct = (float)sq / (sz / 2);
            erase_pct = 1.0 - canvas_pct;
          }
          else
          {
            /* Transparent */
            SDL_GetRGB(getpixels[last->format->BytesPerPixel]
                       (last, x + xx, y + yy), last->format, &r_canvas, &g_canvas, &b_canvas);

            canvas_pct = 0.75;
            erase_pct = 0.25;
          }

          r = (((float)r_erase * erase_pct) + ((float)r_canvas) * canvas_pct);
          g = (((float)g_erase * erase_pct) + ((float)g_canvas) * canvas_pct);
          b = (((float)b_erase * erase_pct) + ((float)b_canvas) * canvas_pct);

          putpixel(canvas, x + xx, y + yy, SDL_MapRGB(canvas->format, (Uint8) r, (Uint8) g, (Uint8) b));
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
      // FIXME: Would be fun to play half-volume when using transparent erasers
    }
  }
#endif

  if (update)
  {
    update_canvas(x - sz / 2, y - sz / 2, x + sz / 2, y + sz / 2);

    if (cur_eraser >= NUM_ERASER_SIZES)
    {
      /* Circle eraser (sharp & fuzzy) */
      circle_xor(x, y, sz / 2);
    }
    else
    {
      /* Square eraser */
      rect_xor(x - sz / 2, y - sz / 2, x + sz / 2, y + sz / 2);
    }

#ifdef __APPLE__
    /* Prevent ghosted eraser outlines from remaining on the screen in windowed mode */
    update_screen(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
#endif
  }
}

/**
 * Draw a line on the canvas using the eraser.
 *
 * @param x1 Starting X coordinate
 * @param y1 Starting Y coordinate
 * @param x2 Ending X coordinate
 * @param y2 Ending Y coordinate
 */
static void eraser_draw(int x1, int y1, int x2, int y2)
{
  int dx, dy, y;
  int orig_x1, orig_y1, orig_x2, orig_y2, tmp, length;
  float m, b;

  orig_x1 = x1;
  orig_y1 = y1;

  orig_x2 = x2;
  orig_y2 = y2;


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
          do_eraser(x1, y, 0);
      }
      else
      {
        for (y = y1; y <= y2; y++)
          do_eraser(x1, y, 0);
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
      do_eraser(x1, y, 0);
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

  length = (calc_eraser_size(cur_eraser) >> 1) + 1;
  update_canvas(orig_x1 - length, orig_y1 - length, orig_x2 + length, orig_y2 + length);
}

/**
 * FIXME
 */
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

  if (num_magics_total == 0)
    tool_avail[TOOL_MAGIC] = 0;


  /* Disable quit? */

  if (disable_quit)
    tool_avail[TOOL_QUIT] = 0;


  /* Disable Label? */

  if (disable_label)
    tool_avail[TOOL_LABEL] = 0;


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


/**
 * FIXME
 */
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

/**
 * FIXME
 */
/* Restore and enable available tools (for End-Of-Open-Dialog) */
static void enable_avail_tools(void)
{
  int i;

  for (i = 0; i < NUM_TOOLS; i++)
  {
    tool_avail[i] = tool_avail_bak[i];
  }
}


/**
 * FIXME
 */
/* For qsort() call in do_open()... */
static int compare_dirent2s(struct dirent2 *f1, struct dirent2 *f2)
{
  DEBUG_PRINTF("compare_dirents: %s\t%s\n", f1->f.d_name, f2->f.d_name);

  if (f1->place == f2->place)
    return (strcmp(f1->f.d_name, f2->f.d_name));
  else
    return (f1->place - f2->place);
}


/**
 * FIXME
 */
/* For qsort() call in do_open()... */
static int compare_dirent2s_invert(struct dirent2 *f1, struct dirent2 *f2)
{
  return compare_dirent2s(f2, f1);
}


/**
 * FIXME
 */
/* Draw tux's text on the screen: */
static void draw_tux_text(int which_tux, const char *const str, int want_right_to_left)
{
  draw_tux_text_ex(which_tux, str, want_right_to_left, 0);
}

static int latest_tux;
static const char *latest_tux_text;
static int latest_r2l;
static Uint8 latest_locale_text;

/**
 * FIXME
 */
static void redraw_tux_text(void)
{
  draw_tux_text_ex(latest_tux, latest_tux_text, latest_r2l, latest_locale_text);
}

/**
 * FIXME
 */
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
  w = max(img_tux[which_tux]->w, img_btnsm_up->w * 2) + 5;

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


/**
 * Draw the current tool's tool tip.
 * Mostly this comes from `tool_tips[]`, based on the current
 * tool (`cur_tool`).  However, some tools have various
 * context-specific strings to show:
 *  - Fill: Describe the currently-selected fill mode (`fill_tips[]` from `fill_tools.h`)
 *  - Shapes: Depends on "simple" vs "complex" shapes option (`shape_tool_tips[]` from `shapes.h`)
 */
static void draw_cur_tool_tip(void)
{
  if (cur_tool == TOOL_FILL)
  {
    draw_tux_text(tool_tux[cur_tool], fill_tips[cur_fill], 1);
  }
  else if (cur_tool == TOOL_SHAPES)
  {
    draw_tux_text(tool_tux[cur_tool],
                  shape_tool_tips[simple_shapes ? SHAPE_COMPLEXITY_SIMPLE : SHAPE_COMPLEXITY_NORMAL], 1);
  }
  else
  {
    draw_tux_text(tool_tux[cur_tool], tool_tips[cur_tool], 1);
  }
}

/**
 * FIXME
 */
static void wordwrap_text(const char *const str, SDL_Color color, int left, int top, int right, int want_right_to_left)
{
  wordwrap_text_ex(str, color, left, top, right, want_right_to_left, 0);
}

/**
 * FIXME
 */
static void wordwrap_text_ex(const char *const str, SDL_Color color,
                             int left, int top, int right, int want_right_to_left, Uint8 locale_text)
{
  SDL_Surface *text;
  TuxPaint_Font *myfont = medium_font;
  SDL_Rect dest;
  SDLPango_Matrix pango_color;


  if (str == NULL || str[0] == '\0')
    return;                     /* No-op! */

  if (need_own_font && (strcmp(gettext(str), str) || locale_text))
    myfont = locale_font;

  if (strcmp(str, gettext(str)) == 0)
  {
    /* String isn't translated!  Don't write right-to-left, even if our locale is an RTOL language: */
    want_right_to_left = 0;
  }


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
}


#ifndef NOSOUND

/**
 * FIXME
 */
static void playstampdesc(int chan)
{
  static SDL_Event playsound_event;

  if (chan == 2)                /* Only do this when the channel playing the stamp sfx has ended! */
  {
    debug("Stamp SFX ended. Pushing event to play description sound...");

    playsound_event.type = TP_USEREVENT_PLAYDESCSOUND;
    playsound_event.user.code = USEREVENT_PLAYDESCSOUND;
    playsound_event.user.data1 = (void *)(intptr_t) cur_stamp[stamp_group];     //EP added (intptr_t) to avoid warning on x64

    SDL_PushEvent(&playsound_event);
  }
}

#endif


/* Load a file's sound: */

#ifndef NOSOUND

/**
 * FIXME
 */
static Mix_Chunk *loadsound_extra(const char *const fname, const char *extra)
{
  char *snd_fname;
  char tmp_str[MAX_PATH], ext[5];
  Mix_Chunk *tmp_snd;


  if (strcasestr(fname, ".png") != NULL)
  {
    strcpy(ext, ".png");        /* char[5] size is sufficient */
  }
  else
  {
    /* Sorry, we only handle images */

    return (NULL);
  }

  /* First, check for localized version of sound: */

  snd_fname = malloc(strlen(fname) + strlen(lang_prefix) + 16);

  strcpy(snd_fname, fname);     /* malloc'd size should be sufficient */
  safe_snprintf(tmp_str, sizeof(tmp_str), "%s_%s.ogg", extra, lang_prefix);
  strcpy((char *)strcasestr(snd_fname, ext), tmp_str);  /* FIXME: Use strncpy() (ugh, complicated) */
  debug(snd_fname);
  tmp_snd = Mix_LoadWAV(snd_fname);

  if (tmp_snd == NULL)
  {
    debug("...No local version of sound (OGG)!");

    strcpy(snd_fname, fname);   /* malloc'd size should be sufficient */
    safe_snprintf(tmp_str, sizeof(tmp_str), "%s_%s.wav", extra, lang_prefix);
    strcpy((char *)strcasestr(snd_fname, ext), tmp_str);        /* FIXME: Use strncpy() (ugh, complicated) */
    debug(snd_fname);
    tmp_snd = Mix_LoadWAV(snd_fname);

    if (tmp_snd == NULL)
    {
      debug("...No local version of sound (WAV)!");

      /* Check for non-country-code locale */

      strcpy(snd_fname, fname); /* malloc'd size should be sufficient */
      safe_snprintf(tmp_str, sizeof(tmp_str), "%s_%s.ogg", extra, short_lang_prefix);
      strcpy((char *)strcasestr(snd_fname, ext), tmp_str);      /* FIXME: Use strncpy() (ugh, complicated) */
      debug(snd_fname);
      tmp_snd = Mix_LoadWAV(snd_fname);

      if (tmp_snd == NULL)
      {
        debug("...No short local version of sound (OGG)!");

        strcpy(snd_fname, fname);       /* malloc'd size should be sufficient */
        safe_snprintf(tmp_str, sizeof(tmp_str), "%s_%s.wav", extra, short_lang_prefix);
        strcpy((char *)strcasestr(snd_fname, ext), tmp_str);    /* FIXME: Use strncpy() (ugh, complicated) */
        debug(snd_fname);
        tmp_snd = Mix_LoadWAV(snd_fname);

        if (tmp_snd == NULL)
        {
          /* Now, check for default sound: */

          debug("...No short local version of sound (WAV)!");

          if (strcmp(extra, "_desc") != 0 || strcmp(short_lang_prefix, "en") == 0)
          {
            /* (Not loading a descriptive sound, or we're in English locale, go ahead and fall back;
               i.e., if loading a descriptive sound in a non-English locale, let's not load the
               English version; see https://sourceforge.net/p/tuxpaint/bugs/261/) */
            strcpy(snd_fname, fname);   /* malloc'd size should be sufficient */
            safe_snprintf(tmp_str, sizeof(tmp_str), "%s.ogg", extra);
            strcpy((char *)strcasestr(snd_fname, ext), tmp_str);        /* FIXME: Use strncpy() (ugh, complicated) */
            debug(snd_fname);
            tmp_snd = Mix_LoadWAV(snd_fname);

            if (tmp_snd == NULL)
            {
              debug("...No default version of sound (OGG)!");

              strcpy(snd_fname, fname); /* malloc'd size should be sufficient */
              safe_snprintf(tmp_str, sizeof(tmp_str), "%s.wav", extra);
              strcpy((char *)strcasestr(snd_fname, ext), tmp_str);      /* FIXME: Use strncpy() (ugh, complicated) */
              debug(snd_fname);
              tmp_snd = Mix_LoadWAV(snd_fname);

              if (tmp_snd == NULL)
                debug("...No default version of sound (WAV)!");
            }
          }
        }
      }
    }
  }

  free(snd_fname);
  return (tmp_snd);
}


/**
 * FIXME
 */
static Mix_Chunk *loadsound(const char *const fname)
{
  return (loadsound_extra(fname, ""));
}

/**
 * FIXME
 */
static Mix_Chunk *loaddescsound(const char *const fname)
{
  return (loadsound_extra(fname, "_desc"));
}

#endif


/* Strip any trailing spaces: */

/**
 * FIXME
 */
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

/**
 * Strip double quotes (") at the beggining and end of the string:
 *
 * @param char * buf -- String to strip quotes from; contents will be modified in that case
*/
static void strip_quotes(char *buf)
{
  unsigned i = strlen(buf);
  int k;

  if (i > 2)
  {
    if (buf[0] == '"')
    {
      for (k = 0; k < (int)i - 2; k++)
      {
        buf[k] = buf[k + 1];
      }
      buf[i - 2] = '\0';
    }
  }
}

/* Load a file's description: */

/**
 * FIXME
 */
static char *loaddesc(const char *const fname, Uint8 *locale_text)
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
    strcpy(def_buf, "");        /* In case of total inability to find anything */

    /* Set the first available language */
    for (i = 0; i < num_wished_langs && !found; i++)
    {
      strcpy((char *)extptr, ".txt");   /* safe; pointing into a safe spot within an existing string (txt_fname) */
      fi = fopen(txt_fname, "r");
      if (!fi)
        return NULL;

      got_first = 0;
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

              strcpy(def_buf, buf);     /* safe; both the same size */
              got_first = 1;
            }

            debug(buf);


            //      lang_prefix = lang_prefixes[langint];
            /* See if it's the one for this locale... */

            if ((char *)strcasestr(buf, wished_langs[i].lang_prefix) == buf)
            {

              debug(buf + strlen(wished_langs[i].lang_prefix));
              if ((char *)strcasestr(buf + strlen(wished_langs[i].lang_prefix),
                                     ".utf8=") == buf + strlen(wished_langs[i].lang_prefix))
              {
                lang_prefix = wished_langs[i].lang_prefix;
                short_lang_prefix = strdup(lang_prefix);
                /* When in doubt, cut off country code */
                if (strchr(short_lang_prefix, '_'))
                  *strchr(short_lang_prefix, '_') = '\0';

                need_own_font = wished_langs[i].need_own_font;
                need_right_to_left = wished_langs[i].need_right_to_left;

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
    fprintf(stderr, "Somehow, '%s' doesn't have a filename extension!?\n", fname);
    return NULL;
  }
}


/**
 * FIXME
 */
/* Load a *.dat file */
static double loadinfo(const char *const fname, stamp_type *inf)
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
            if (tmp > 0.0001 && tmp < 10000.0 && tmp2 > 0.0001
                && tmp2 < 10000.0 && tmp / tmp2 > 0.0001 && tmp / tmp2 < 10000.0)
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


/**
 * FIXME
 */
static int SDLCALL NondefectiveBlit(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
  int dstx = 0;
  int dsty = 0;
  int srcx = 0;
  int srcy = 0;
  int srcw = src->w;
  int srch = src->h;

  Uint32(*getpixel) (SDL_Surface *, int, int) = getpixels[src->format->BytesPerPixel];
  void (*putpixel)(SDL_Surface *, int, int, Uint32) = putpixels[dst->format->BytesPerPixel];


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
    int i = srcw + 1;           /* "+ 1" is a tweak to get to edges -bjk 2009.10.11 */

    while (i--)
    {
      putpixel(dst, i + dstx, srch + dsty, getpixel(src, i + srcx, srch + srcy));
    }
  }

  return (0);
}


/**
 * Copy an image, scaling and smearing, as needed, into a new surface.
 * Free the original surface.
 *
 * @param SDL_Surface * src -- source surface (will be freed by this function!)
 * @param SDL_Surface * dst -- destination surface
 * @param int SDCALL(*blit) -- function for blitting; "NondefectiveBlit" or "SDL_BlitSurface"
 */
static void autoscale_copy_smear_free(SDL_Surface *src, SDL_Surface *dst,
                                      int SDLCALL(*blit) (SDL_Surface *src,
                                                          const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect))
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


/**
 * Copy an image, scaling and smearing, as needed, into a new surface.
 * Free the original surface.
 *
 * @param SDL_Surface * src -- source surface (will be freed by this function!)
 * @param SDL_Surface * dst -- destination surface
 * @param int SDCALL(*blit) -- function for blitting; "NondefectiveBlit" or "SDL_BlitSurface"
 * @param starter_template_options_t opts -- options (loaded from ".dat" file) describing strategies to take
 */
static void autoscale_copy_scale_or_smear_free(SDL_Surface *src,
                                               SDL_Surface *dst,
                                               int SDLCALL(*blit) (SDL_Surface
                                                                   *src,
                                                                   const
                                                                   SDL_Rect
                                                                   *srcrect,
                                                                   SDL_Surface
                                                                   *dst,
                                                                   SDL_Rect *dstrect), starter_template_options_t opts)
{
  int new_w, new_h;
  float src_aspect, dst_aspect;

  new_w = src->w;
  new_h = src->h;

  src_aspect = (float)src->w / (float)src->h;
  dst_aspect = (float)dst->w / (float)dst->h;

  if (src_aspect > dst_aspect)
  {
    DEBUG_PRINTF
      ("Image (%d x %d) is of a wider aspect (%0.5f) than canvas (%d x %d) (%0.5f)\n",
       src->w, src->h, src_aspect, dst->w, dst->h, dst_aspect);
    if (opts.scale_mode == STARTER_TEMPLATE_SCALE_MODE_HORIZ || opts.scale_mode == STARTER_TEMPLATE_SCALE_MODE_BOTH)
    {
      new_h = dst->h;
      new_w = dst->h * src_aspect;
      DEBUG_PRINTF("Okay to crop left/right. Keeping aspect; scaling to %d x %d\n", new_w, new_h);
    }
  }
  else if (src_aspect < dst_aspect)
  {
    DEBUG_PRINTF
      ("Image (%d x %d) is of a taller aspect (%0.5f) than canvas (%d x %d) (%0.5f)\n",
       src->w, src->h, src_aspect, dst->w, dst->h, dst_aspect);
    if (opts.scale_mode == STARTER_TEMPLATE_SCALE_MODE_VERT || opts.scale_mode == STARTER_TEMPLATE_SCALE_MODE_BOTH)
    {
      new_w = dst->w;
      new_h = dst->w / src_aspect;
      DEBUG_PRINTF("Okay to crop top/bottom. Keeping aspect; scaling to %d x %d\n", new_w, new_h);
    }
  }
  else
  {
    DEBUG_PRINTF
      ("Image (%d x %d) is the same aspect as canvas (%d x %d) (%0.05f)\n", src->w, src->h, dst->w, dst->h, src_aspect);
  }


  /* Scale and crop based on any aspect-ratio-keeping adjustments */
  if (new_w != src->w || new_h != src->h)
  {
    SDL_Surface *scaled, *src1;
    SDL_Rect src_rect;

    /* Scale, keeping aspect, which will cause extra content that needs cropping */

    DEBUG_PRINTF("Scaling from %d x %d to %d x %d\n", src->w, src->h, new_w, new_h);

    scaled = thumbnail2(src, new_w, new_h, 0 /* keep aspect */ ,
                        1 /* keep alpha */ );
    if (scaled == NULL)
    {
      fprintf(stderr, "Failed to scale an image!\n");
      return;
    }

    /* Create a new surface to blit (crop) into */
    src1 = SDL_CreateRGBSurface(src->flags,     /* SDL_SWSURFACE, */
                                dst->w, dst->h, src->format->BitsPerPixel,
                                src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);
    if (src1 == NULL)
    {
      fprintf(stderr, "Failed to create a surface!\n");
      return;
    }

    SDL_FreeSurface(src);
    src = src1;

    /* Place the new image onto the dest */
    if (opts.h_gravity == STARTER_TEMPLATE_GRAVITY_HORIZ_LEFT)
    {
      src_rect.x = 0;
    }
    else if (opts.h_gravity == STARTER_TEMPLATE_GRAVITY_HORIZ_RIGHT)
    {
      src_rect.x = scaled->w - dst->w;
    }
    else                        /* opts.h_gravity == STARTER_TEMPLATE_GRAVITY_HORIZ_CENTER */
    {
      src_rect.x = (scaled->w - dst->w) / 2;
    }

    if (opts.v_gravity == STARTER_TEMPLATE_GRAVITY_VERT_TOP)
    {
      src_rect.y = 0;
    }
    else if (opts.v_gravity == STARTER_TEMPLATE_GRAVITY_VERT_BOTTOM)
    {
      src_rect.y = scaled->h - dst->h;
    }
    else                        /* opts.v_gravity == STARTER_TEMPLATE_GRAVITY_VERT_CENTER */
    {
      src_rect.y = (scaled->h - dst->h) / 2;
    }

    src_rect.w = scaled->w;
    src_rect.h = scaled->h;

    DEBUG_PRINTF
      ("Blitting scaled image (%d x %d) into new 'src' image (%d x %d) at (%d,%d) %d x %d\n",
       scaled->w, scaled->h, src->w, src->h, src_rect.x, src_rect.y, src_rect.w, src_rect.h);

    SDL_BlitSurface(scaled, &src_rect, src, NULL);
  }


  if (src->w != dst->w || src->h != dst->h)
  {
    DEBUG_PRINTF("Fitting %d x %d onto %d x %d canvas\n", src->w, src->h, dst->w, dst->h);

    if (opts.smear)
    {
      autoscale_copy_smear_free(src, dst, blit);
      /* Note: autoscale_copy_smear_free() calls SDL_FreeSurface(src)! */
    }
    else
    {
      SDL_Surface *scaled;
      SDL_Rect dst_rect;

      if (src->w != dst->w || src->h != dst->h)
      {
        if (src->w / (float)dst->w > src->h / (float)dst->h)
        {
          DEBUG_PRINTF("Scaling from %d x %d to %d x %d\n", src->w, src->h, dst->w, src->h * dst->w / src->w);
          scaled = thumbnail(src, dst->w, src->h * dst->w / src->w, 0);
        }
        else
        {
          DEBUG_PRINTF("Scaling from %d x %d to %d x %d\n", src->w, src->h, src->w * dst->h / src->h, dst->h);
          scaled = thumbnail(src, src->w * dst->h / src->h, dst->h, 0);
        }
      }

      SDL_FreeSurface(src);

      if (scaled == NULL)
      {
        fprintf(stderr, "Failed to scale an image!\n");
        return;
      }

      DEBUG_PRINTF("Centering on a background color\n");

      SDL_FillRect(dst, NULL, SDL_MapRGB(dst->format, opts.bkgd_color[0], opts.bkgd_color[1], opts.bkgd_color[2]));

      dst_rect.x = (dst->w - scaled->w) / 2;
      dst_rect.y = (dst->h - scaled->h) / 2;
      dst_rect.w = scaled->w;
      dst_rect.h = scaled->h;

      SDL_BlitSurface(scaled, NULL, dst, &dst_rect);

      SDL_FreeSurface(scaled);
    }
  }
  else
  {
    DEBUG_PRINTF("No smearing or background needed\n");

    autoscale_copy_smear_free(src, dst, blit);
    // SDL_FreeSurface(src);
  }
}


/**
 * Attempt to open a starter/template image's options (".dat")
 * file and read the settings into an options structure.
 *
 * @param char * dirname -- directory source of image
 * @param char * img_id -- basename of image
 * @param starter_template_options_t * opts -- pointer to options struct to fill
 */
static void get_starter_template_options(char *dirname, char *img_id, starter_template_options_t *opts)
{
  char fname[256], buf[256];
  char *arg;
  FILE *fi;

  /* Set defaults for all options (in case file missing, or file doesn't specify certain options) */
  opts->scale_mode = STARTER_TEMPLATE_SCALE_MODE_NONE;
  opts->h_gravity = STARTER_TEMPLATE_GRAVITY_HORIZ_CENTER;
  opts->v_gravity = STARTER_TEMPLATE_GRAVITY_VERT_CENTER;
  opts->smear = 1;
  opts->bkgd_color[0] = 255;
  opts->bkgd_color[1] = 255;
  opts->bkgd_color[2] = 255;

  /* Attempt to open the file */
  safe_snprintf(fname, sizeof(fname), "%s/%s.dat", dirname, img_id);
  fi = fopen(fname, "r");
  if (fi == NULL)
  {
    return;
  }

  while (!feof(fi))
  {
    if (fgets(buf, sizeof(buf), fi))
    {
      if (!feof(fi))
      {
        strip_trailing_whitespace(buf);

        if (buf[0] == '\0' || buf[0] == '#')
        {
          continue;
        }

        arg = strchr(buf, '=');
        if (arg)
        {
          *arg++ = '\0';
        }
        else
        {
          fprintf(stderr, "Don't understand line in '%s': '%s'\n", fname, buf);
          continue;
        }

        if (strcmp(buf, "allowscale") == 0)
        {
          if (strcmp(arg, "horizontal") == 0)
          {
            opts->scale_mode = STARTER_TEMPLATE_SCALE_MODE_HORIZ;
          }
          else if (strcmp(arg, "vertical") == 0)
          {
            opts->scale_mode = STARTER_TEMPLATE_SCALE_MODE_VERT;
          }
          else if (strcmp(arg, "both") == 0)
          {
            opts->scale_mode = STARTER_TEMPLATE_SCALE_MODE_BOTH;
          }
          else if (strcmp(arg, "none") == 0)
          {
            opts->scale_mode = STARTER_TEMPLATE_SCALE_MODE_NONE;
          }
          else
          {
            fprintf(stderr, "Unknown 'autoscale' option in '%s': '%s'\n", fname, arg);
          }
        }
        else if (strcmp(buf, "gravity") == 0)
        {
          if (strcmp(arg, "top") == 0)
          {
            opts->h_gravity = STARTER_TEMPLATE_GRAVITY_HORIZ_CENTER;
            opts->v_gravity = STARTER_TEMPLATE_GRAVITY_VERT_TOP;
          }
          else if (strcmp(arg, "bottom") == 0)
          {
            opts->h_gravity = STARTER_TEMPLATE_GRAVITY_HORIZ_CENTER;
            opts->v_gravity = STARTER_TEMPLATE_GRAVITY_VERT_BOTTOM;
          }
          else if (strcmp(arg, "left") == 0)
          {
            opts->h_gravity = STARTER_TEMPLATE_GRAVITY_HORIZ_LEFT;
            opts->v_gravity = STARTER_TEMPLATE_GRAVITY_VERT_CENTER;
          }
          else if (strcmp(arg, "right") == 0)
          {
            opts->h_gravity = STARTER_TEMPLATE_GRAVITY_HORIZ_RIGHT;
            opts->v_gravity = STARTER_TEMPLATE_GRAVITY_VERT_CENTER;
          }
          else if (strcmp(arg, "top-left") == 0)
          {
            opts->h_gravity = STARTER_TEMPLATE_GRAVITY_HORIZ_LEFT;
            opts->v_gravity = STARTER_TEMPLATE_GRAVITY_VERT_TOP;
          }
          else if (strcmp(arg, "bottom-left") == 0)
          {
            opts->h_gravity = STARTER_TEMPLATE_GRAVITY_HORIZ_LEFT;
            opts->v_gravity = STARTER_TEMPLATE_GRAVITY_VERT_BOTTOM;
          }
          else if (strcmp(arg, "top-right") == 0)
          {
            opts->h_gravity = STARTER_TEMPLATE_GRAVITY_HORIZ_RIGHT;
            opts->v_gravity = STARTER_TEMPLATE_GRAVITY_VERT_TOP;
          }
          else if (strcmp(arg, "bottom-right") == 0)
          {
            opts->h_gravity = STARTER_TEMPLATE_GRAVITY_HORIZ_RIGHT;
            opts->v_gravity = STARTER_TEMPLATE_GRAVITY_VERT_BOTTOM;
          }
          else if (strcmp(arg, "center") == 0)
          {
            opts->h_gravity = STARTER_TEMPLATE_GRAVITY_HORIZ_CENTER;
            opts->v_gravity = STARTER_TEMPLATE_GRAVITY_VERT_CENTER;
          }
          else
          {
            fprintf(stderr, "Unknown 'autoscale' option in '%s': '%s'\n", fname, arg);
          }
        }
        else if (strcmp(buf, "background") == 0)
        {
          if (strcmp(arg, "smear") == 0)
          {
            opts->smear = 1;
          }
          else
          {
            int count;
            char tmp_str[256];

            opts->smear = 0;

            /* FIXME: This and setup_colors() needs to be modularized -bjk 2023.02.10 */

            if (arg[0] == '#')
            {
              /* Hex form */

              sscanf(arg + 1, "%s %n", tmp_str, &count);

              if (strlen(tmp_str) == 6)
              {
                DEBUG_PRINTF("6 digit hex ('%s')\n", arg);

                /* Byte (#rrggbb) form */

                opts->bkgd_color[0] = (hex2dec(tmp_str[0]) << 4) + hex2dec(tmp_str[1]);
                opts->bkgd_color[1] = (hex2dec(tmp_str[2]) << 4) + hex2dec(tmp_str[3]);
                opts->bkgd_color[2] = (hex2dec(tmp_str[4]) << 4) + hex2dec(tmp_str[5]);
              }
              else if (strlen(tmp_str) == 3)
              {
                DEBUG_PRINTF("3 digit hex ('%s')\n", arg);

                /* Nybble (#rgb) form */

                opts->bkgd_color[0] = (hex2dec(tmp_str[0]) << 4) + hex2dec(tmp_str[0]);
                opts->bkgd_color[1] = (hex2dec(tmp_str[1]) << 4) + hex2dec(tmp_str[1]);
                opts->bkgd_color[2] = (hex2dec(tmp_str[2]) << 4) + hex2dec(tmp_str[2]);
              }
              else
              {
                fprintf(stderr, "Don't understand color hex '%s'\n", arg);
              }
            }
            else
            {
              short unsigned int r, g, b;

              DEBUG_PRINTF("Integers ('%s')\n", arg);

              /* Assume int form */

              sscanf(arg, "%hu %hu %hu %n", &r, &g, &b, &count);
              opts->bkgd_color[0] = r;
              opts->bkgd_color[1] = g;
              opts->bkgd_color[2] = b;
            }

            DEBUG_PRINTF("Background color: %d,%d,%d\n", opts->bkgd_color[0], opts->bkgd_color[1], opts->bkgd_color[2]);
          }
        }
        else
        {
          fprintf(stderr, "Unrecognized option in '%s': '%s' (set to '%s')\n", fname, buf, arg);
          continue;
        }
      }
    }
  }
  fclose(fi);

  /* FIXME: It might be worth reporting warnings for these situations: */
  /* N.B. If 'allowscale=both', then background options are meaningless;
     we could report that here (e.g., to stderr) -bjk 2023.02.10 */
  /* N.B. Certain 'gravity' options don't make sense based on the
     'allowscale' option (e.g., if "vertical", then gravities that
     include 'left' or 'right' are meaningless) -bjk 2023.02.12 */
}


/**
 * FIXME
 */
static void load_starter_id(char *saved_id, FILE *fil)
{
  char *rname;
  char fname[FILENAME_MAX];
  FILE *fi;
  char color_tag;
  int r, g, b, __attribute__((unused)) tmp;
  char * __attribute__((unused)) tmp_ptr;

  rname = NULL;

  if (saved_id != NULL)
  {
    safe_snprintf(fname, sizeof(fname), "saved/%s.dat", saved_id);
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
        tmp_ptr = fgets(template_id, sizeof(template_id), fi);
        template_id[strlen(template_id) - 1] = '\0';
        tmp = fscanf(fi, "%d", &template_personal);
        DEBUG_PRINTF("template = %s\n (Personal=%d)", template_id, template_personal);
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


/**
 * FIXME
 */
static SDL_Surface *load_starter_helper(char *path_and_basename,
                                        const char *extension, SDL_Surface *(*load_func) (const char *))
{
  char *ext;
  char fname[256];
  SDL_Surface *surf;
  unsigned int i;

#ifndef __ANDROID__
  struct stat stat_buf;
#endif

  ext = strdup(extension);
  safe_snprintf(fname, sizeof(fname), "%s.%s", path_and_basename, ext);

#ifndef __ANDROID__
  if (stat(fname, &stat_buf) != 0)
  {
    /* File by that name doesn't exist; give up now */
    return NULL;
  }
#endif

  surf = (*load_func) (fname);

  if (surf == NULL)
  {
    for (i = 0; i < strlen(ext); i++)
    {
      ext[i] = toupper(ext[i]);
    }
    safe_snprintf(fname, sizeof(fname), "%s.%s", path_and_basename, ext);
    surf = (*load_func) (fname);
  }

  free(ext);

  return (surf);
}


/**
 * FIXME
 */
static void load_starter(char *img_id)
{
  char *dirname;
  char fname[256];
  SDL_Surface *tmp_surf;
  starter_template_options_t template_options;


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

    safe_snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
    tmp_surf = load_starter_helper(fname, "svg", &load_svg);
  }
#endif

  if (tmp_surf == NULL)
  {
    safe_snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
    tmp_surf = load_starter_helper(fname, "png", &IMG_Load);
  }

  if (tmp_surf == NULL)
  {
    /* Try loading a KPX */
    safe_snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
    tmp_surf = load_starter_helper(fname, "kpx", &myIMG_Load);
  }


  if (tmp_surf != NULL)
  {
    img_starter = SDL_DisplayFormatAlpha(tmp_surf);
    SDL_FreeSurface(tmp_surf);
  }

  /* Try to load the a background image: */

  tmp_surf = NULL;

#ifndef NOSVG
  /* (Try SVG) */
  if (tmp_surf == NULL)
  {
    safe_snprintf(fname, sizeof(fname), "%s/%s-back", dirname, img_id);
    tmp_surf = load_starter_helper(fname, "svg", &load_svg);
  }
#endif

  /* (JPEG) */
  if (tmp_surf == NULL)
  {
    safe_snprintf(fname, sizeof(fname), "%s/%s-back", dirname, img_id);
    tmp_surf = load_starter_helper(fname, "jpeg", &IMG_Load);
  }

  if (tmp_surf == NULL)
  {
    /* (Then just JPG) */
    safe_snprintf(fname, sizeof(fname), "%s/%s-back", dirname, img_id);
    tmp_surf = load_starter_helper(fname, "jpg", &IMG_Load);
  }

  /* (Failed? Try PNG next) */
  if (tmp_surf == NULL)
  {
    safe_snprintf(fname, sizeof(fname), "%s/%s-back", dirname, img_id);
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
    void (*putpixel)(SDL_Surface *, int, int, Uint32) = putpixels[img_starter->format->BytesPerPixel];
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

    get_starter_template_options(dirname, img_id, &template_options);

    autoscale_copy_scale_or_smear_free(tmp_surf, img_starter, NondefectiveBlit, template_options);

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

    autoscale_copy_scale_or_smear_free(tmp_surf, img_starter_bkgd, SDL_BlitSurface, template_options);
  }

  free(dirname);
}


/**
 * FIXME
 */
static void load_template(char *img_id)
{
  char *dirname;
  char fname[256];
  SDL_Surface *tmp_surf;
  starter_template_options_t template_options;

  /* Determine path to starter files: */

  if (template_personal == 0)
    dirname = strdup(DATA_PREFIX "templates");
  else
    dirname = get_fname("templates", DIR_DATA);

  /* Clear them to NULL first: */
  img_starter = NULL;
  img_starter_bkgd = NULL;

  /* (Try loading a KPX) */
  safe_snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
  tmp_surf = load_starter_helper(fname, "kpx", &myIMG_Load);

#ifndef NOSVG
  /* (Failed? Try SVG next) */
  if (tmp_surf == NULL)
  {
    safe_snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
    tmp_surf = load_starter_helper(fname, "svg", &load_svg);
  }
#endif

  /* (JPEG) */
  if (tmp_surf == NULL)
  {
    safe_snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
    tmp_surf = load_starter_helper(fname, "jpeg", &IMG_Load);
  }
  if (tmp_surf == NULL)
  {
    /* (Then just JPG) */
    safe_snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
    tmp_surf = load_starter_helper(fname, "jpg", &IMG_Load);
  }

  /* (Failed? Try PNG next) */
  if (tmp_surf == NULL)
  {
    safe_snprintf(fname, sizeof(fname), "%s/%s", dirname, img_id);
    tmp_surf = load_starter_helper(fname, "png", &IMG_Load);
  }

  if (tmp_surf != NULL)
  {
    img_starter_bkgd = SDL_DisplayFormat(tmp_surf);
    SDL_FreeSurface(tmp_surf);
  }

  /* Get template's options */
  get_starter_template_options(dirname, img_id, &template_options);


  /* Scale if needed... */

  if (img_starter_bkgd != NULL && (img_starter_bkgd->w != canvas->w || img_starter_bkgd->h != canvas->h))
  {
    tmp_surf = img_starter_bkgd;

    img_starter_bkgd = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                            canvas->w, canvas->h,
                                            canvas->format->BitsPerPixel,
                                            canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, 0);

    autoscale_copy_scale_or_smear_free(tmp_surf, img_starter_bkgd, SDL_BlitSurface, template_options);
  }

  free(dirname);
}

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

/**
 * FIXME
 */
/* Load current (if any) image: */

static void load_current(void)
{
  SDL_Surface *tmp, *org_surf;
  char *fname;
  char ftmp[1024];

#ifdef AUTOSAVE_GOING_BACKGROUND
  FILE *fi;
#endif

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
    have_to_rec_label_node = SDL_FALSE;

    safe_snprintf(ftmp, sizeof(ftmp), "saved/%s%s", file_id, FNAME_EXTENSION);
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
      DEBUG_PRINTF("Smearing canvas @ 4\n");
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

      /* FIXME: Consider using the new
         autoscale_copy_smear_or_scale_free() based on the starter/template
         file's options? (Will need to do here, rather than first thing,
         above) -bjk 2023.02.09 */
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


/**
 * FIXME
 */
/* Make sure we have a 'path' directory */
static int make_directory(int dir_type, const char *path, const char *errmsg)
{
  char *fname;
  int res;

  fname = get_fname(path, dir_type);
  res = mkdir(fname, 0755);
  if (res != 0 && errno != EEXIST)
  {
    fprintf(stderr, "\nError: %s:\n" "%s\n" "The error that occurred was:\n" "%s\n\n", errmsg, fname, strerror(errno));
    free(fname);
    return 0;
  }
  free(fname);
  return 1;
}

/**
 * FIXME
 */
/* Save the current image to disk: */
static void save_current(void)
{
  char *fname;
  FILE *fi;

  if (!make_directory(DIR_SAVE, "", "Can't create user data directory (E001)"))
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


/**
 * FIXME
 */
/* Prompt the user with a yes/no question: */
static int do_prompt(const char *const text, const char *const btn_yes, const char *const btn_no, int ox, int oy)
{
  return (do_prompt_image(text, btn_yes, btn_no, NULL, NULL, NULL, ox, oy));
}

/**
 * FIXME
 */
static int do_prompt_snd(const char *const text, const char *const btn_yes,
                         const char *const btn_no, int snd, int ox, int oy)
{
  return (do_prompt_image_flash_snd(text, btn_yes, btn_no, NULL, NULL, NULL, 0, snd, ox, oy));
}

/**
 * FIXME
 */
static int do_prompt_image(const char *const text, const char *const btn_yes,
                           const char *const btn_no, SDL_Surface *img1,
                           SDL_Surface *img2, SDL_Surface *img3, int ox, int oy)
{
  return (do_prompt_image_snd(text, btn_yes, btn_no, img1, img2, img3, SND_NONE, ox, oy));
}

/**
 * FIXME
 */
static int do_prompt_image_snd(const char *const text,
                               const char *const btn_yes,
                               const char *const btn_no, SDL_Surface *img1,
                               SDL_Surface *img2, SDL_Surface *img3, int snd, int ox, int oy)
{
  return (do_prompt_image_flash_snd(text, btn_yes, btn_no, img1, img2, img3, 0, snd, ox, oy));
}

/**
 * FIXME
 */
static int do_prompt_image_flash(const char *const text,
                                 const char *const btn_yes,
                                 const char *const btn_no, SDL_Surface *img1,
                                 SDL_Surface *img2, SDL_Surface *img3, int animate, int ox, int oy)
{
  return (do_prompt_image_flash_snd(text, btn_yes, btn_no, img1, img2, img3, animate, SND_NONE, ox, oy));
}

#define PROMPT_W (min(canvas->w, ((int) (440 * button_scale))))
#define PROMPT_LEFT ((Sint16) (r_tools.w - PROMPTOFFSETX + canvas->w / 2 - PROMPT_W / 2 - 4))

/**
 * FIXME
 */
static int do_prompt_image_flash_snd(const char *const text,
                                     const char *const btn_yes,
                                     const char *const btn_no,
                                     SDL_Surface *img1, SDL_Surface *img2,
                                     SDL_Surface *img3, int animate, int snd, int ox, int oy)
{
  int oox, ooy, nx, ny;
  SDL_Event event;
  SDL_Rect dest, dest_back;
  int done, ans, w, counter;
  SDL_Color black = { 0, 0, 0, 0 };
  SDLKey key;
  SDLKey key_y, key_n;
  const char *keystr;
  SDL_Surface *backup;

#ifndef NO_PROMPT_SHADOWS
  int i;
  SDL_Surface *alpha_surf;
#endif
  int img1_w, img2_w, img3_w, max_img_w, img_y, offset;
  SDL_Surface *img1b;
  int free_img1b;
  int txt_left, txt_right, img_left, btn_left, txt_btn_left, txt_btn_right;
  int val_x, val_y, motioner;
  int valhat_x, valhat_y, hatmotioner;

#ifdef DEBUG
  if (snd >= 0)
  {
    printf("Prompt and play sound #%d: %s\n", snd, sound_fnames[snd]);
    fflush(stdout);
  }
  else
  {
    printf("Prompt without sound\n");
    fflush(stdout);
  }
#endif

#ifdef AUTOSAVE_GOING_BACKGROUND
  /* If we are already in background, and Android wants to recover resources, it will call for quitting.
     However, Android will not allow user interaction anymore.
     Since the drawing is already (auto)saved, it is safe to quit here */
  if (entered_background)
    cleanup();
#endif

  val_x = val_y = motioner = 0;
  valhat_x = valhat_y = hatmotioner = 0;
  emulate_button_pressed = 0;

  hide_blinking_cursor();

  /* Admittedly stupid way of determining which keys can be used for
     positive and negative responses in dialogs (e.g., [Y] (for 'yes') in English) */
  keystr = gettext("Yes");
  key_y = tolower(keystr[0]);

  keystr = gettext("No");
  key_n = tolower(keystr[0]);


  do_setcursor(cursor_arrow);


  /* Draw button box: */

  playsound(screen, 0, SND_PROMPT, 1, SNDPOS_CENTER, SNDDIST_NEAR);

  backup = SDL_CreateRGBSurface(screen->flags, screen->w, screen->h,
                                screen->format->BitsPerPixel,
                                screen->format->Rmask,
                                screen->format->Gmask, screen->format->Bmask, screen->format->Amask);

  SDL_FillRect(backup, NULL, SDL_MapRGBA(backup->format, 255, 255, 255, 0));
  SDL_BlitSurface(screen, NULL, backup, NULL);

  /*
   * This loop creates an animation effect of the dialog box popping up.  To
   * ensure the animation plays at the same speed regardless of the platform and
   * resource available at the time, capture the rate at which each frame is
   * being drawn and draw the next frame at the adaptive rate.
   */
  {
    Uint32 anim_ms = 120;
    Uint32 last_ms = SDL_GetTicks();

    w = 0;

    while (w <= r_ttools.w)
    {
      Uint32 next_ms = 0;
      Uint32 dw = 0;

      oox = ox - w;
      ooy = oy - w;

      nx = PROMPT_LEFT + r_ttools.w - w + PROMPTOFFSETX;
      ny = 2 + canvas->h / 2 - w;

      dest.x = ((nx * w) + (oox * (r_ttools.w - w))) / r_ttools.w;
      dest.y = ((ny * w) + (ooy * (r_ttools.w - w))) / r_ttools.w;
      dest.w = (PROMPT_W - r_ttools.w * 2) + w * 2;
      dest.h = w * 2;
      SDL_FillRect(screen, &dest,
                   SDL_MapRGB(screen->format, 224 - (int)(w / button_scale),
                              224 - (int)(w / button_scale), 244 - (int)(w / button_scale)));

      SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);

      /* Calculate the amount by which to move to the next animation frame */
      if (w < r_ttools.w - 2)
      {
        while (1)
        {
          next_ms = SDL_GetTicks();
          dw = ((next_ms - last_ms) * r_ttools.w + r_tools.w / 2) / anim_ms;
          if (dw)
            break;

          /* This platform is so fast that there is no new frame to draw.
           * Yield some time then recalculate the next frame. */
          SDL_Delay(1);
        }
        w += dw;
        w = min(w, r_ttools.w - 2);
        last_ms = next_ms;
      }
      else if (w == r_ttools.w - 2)
      {
        /* Draw the dialog box. The dialog box is drawn 1 frame before the last
         * frame because the last frame draws the top and left borders.  We
         * also skip a frame for artistic reasons. */
        SDL_BlitSurface(backup, NULL, screen, NULL);
        w += 2;
      }
      else
      {
        w += 2;
      }
    }
  }

  SDL_FreeSurface(backup);


  playsound(screen, 1, snd, 1, SNDPOS_LEFT, SNDDIST_NEAR);

#ifndef NO_PROMPT_SHADOWS
  alpha_surf = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                    (PROMPT_W - r_ttools.w * 2) + (w - 4) * 2,
                                    (w - 4) * 2,
                                    screen->format->BitsPerPixel,
                                    screen->format->Rmask,
                                    screen->format->Gmask, screen->format->Bmask, screen->format->Amask);

  if (alpha_surf != NULL)
  {
    SDL_FillRect(alpha_surf, NULL, SDL_MapRGBA(alpha_surf->format, 0, 0, 0, 64));

    for (i = 8; i > 0; i = i - 2)
    {
      dest.x = PROMPT_LEFT + r_ttools.w - (w - 4) + i + PROMPTOFFSETX;
      dest.y = 94 / button_scale + r_ttools.w / button_scale - (w - 4) + i + PROMPTOFFSETY;
      dest.w = (PROMPT_W - r_ttools.w * 2) + (w - 4) * 2;
      dest.h = (w - 4) * 2;
      dest.y = canvas->h / 2 - dest.h / 2 + i + 2;
      SDL_BlitSurface(alpha_surf, NULL, screen, &dest);
    }

    SDL_FreeSurface(alpha_surf);
  }
#endif


  w = w - 6;

  dest_back.x = dest.x = PROMPT_LEFT + r_ttools.w - w + PROMPTOFFSETX;
  dest_back.w = dest.w = (PROMPT_W - r_ttools.w * 2) + w * 2;
  dest_back.h = dest.h = w * 2;
  dest_back.y = dest.y = canvas->h / 2 - dest.h / 2 + 2;
  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));

  /* Make sure image on the right isn't too tall!
     (Thumbnails in Open dialog are larger on larger displays, and can
     cause arrow and trashcan icons to be pushed out of the dialog window!) */

  free_img1b = 0;
  img1b = NULL;

  if (img1 != NULL)
  {
    if (img1->h > 64 * button_scale && img2 != NULL /* Only scale if it matters */ )
    {
      img1b = thumbnail(img1, 80 * button_scale, 64 * button_scale, 1);
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

  wordwrap_text(text, black, txt_left, dest.y + 2, txt_right, 1);


  /* Draw the images (if any, and if not animated): */

  img_y = dest_back.y + 6;

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

      img_y = img_y + img3->h + 4;      /* unnecessary */
    }
  }


  /* Draw yes button: */

  dest.x = btn_left;
  dest.y = dest_back.y + dest_back.h - 4 - button_h - 4 - button_h;
  SDL_BlitSurface(img_yes, NULL, screen, &dest);


  /* (Bound to UTF8 domain, so always ask for UTF8 rendering!) */

  wordwrap_text(btn_yes, black, txt_btn_left, dest.y + 5, txt_btn_right, 1);


  /* Draw no button: */

  if (strlen(btn_no) != 0)
  {
    dest.x = btn_left;
    dest.y = dest.y + button_h + 4;
    SDL_BlitSurface(img_no, NULL, screen, &dest);

    wordwrap_text(btn_no, black, txt_btn_left, dest.y + 5, txt_btn_right, 1);
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
          if (event.button.y >=
              dest_back.y + dest_back.h - 4 - button_h - 4 - button_h
              && event.button.y < dest_back.y + dest_back.h - 4 - button_h - 4 - button_h + img_yes->h)
          {
            ans = 1;
            done = 1;
          }
          else if (strlen(btn_no) != 0 &&
                   event.button.y >= dest_back.y + dest_back.h - 4 - button_h
                   && event.button.y < dest_back.y + dest_back.h - 4 - button_h + img_no->h)
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
            ((event.button.y >=
              dest_back.y + dest_back.h - 4 - button_h - 4 - button_h
              && event.button.y <
              dest_back.y + dest_back.h - 4 - button_h - 4 - button_h +
              img_yes->h) || (strlen(btn_no) != 0
                              && event.button.y >=
                              dest_back.y + dest_back.h - 4 - button_h
                              && event.button.y < dest_back.y + dest_back.h - 4 - button_h + img_no->h)))
        {
          do_setcursor(cursor_hand);
        }
        else
        {
          do_setcursor(cursor_arrow);
        }
        oldpos_x = event.button.x;
        oldpos_y = event.button.y;

        motion_since_click = 1;
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


/**
 * FIXME
 */
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
  free_surface(&current_stamp_cached);

  free_surface_array(img_brushes, num_brushes);
  free_surface_array(img_brushes_thumbs, num_brushes);
  free(brushes_frames);
  free(brushes_directional);
  free(brushes_rotate);
  free(brushes_chaotic);
  free(brushes_spacing);
  free(brushes_spacing_default);
  for (i = 0; i < num_brushes; i++)
  {
    if (brushes_descr[i] != NULL)
    {
      free(brushes_descr[i]);
    }
  }
  free(brushes_descr);

  free_surface_array(img_tools, NUM_TOOLS);
  free_surface_array(img_tool_names, NUM_TOOLS);
  free_surface_array(img_title_names, NUM_TITLES);
  for (i = 0; i < MAX_MAGIC_GROUPS; i++)
  {
    for (j = 0; j < num_magics[i]; j++)
    {
      free_surface(&(magics[i][j].img_icon));
      free_surface(&(magics[i][j].img_name));
    }
  }
  free_surface_array(img_shapes, NUM_SHAPES);
  free_surface_array(img_shape_names, NUM_SHAPES);
  free_surface_array(img_fills, NUM_FILLS);
  free_surface_array(img_fill_names, NUM_FILLS);
  free_surface_array(img_tux, NUM_TIP_TUX);

  free_surface(&img_openlabels_open);
  free_surface(&img_openlabels_slideshow);
  free_surface(&img_openlabels_template);
  free_surface(&img_openlabels_erase);
  free_surface(&img_openlabels_pict_export);
  free_surface(&img_openlabels_back);
  free_surface(&img_openlabels_next);
  free_surface(&img_openlabels_play);
  free_surface(&img_openlabels_gif_export);

  free_surface(&img_mixerlabel_clear);

  free_surface(&img_progress);

  free_surface(&img_yes);
  free_surface(&img_no);

  free_surface(&img_prev);
  free_surface(&img_next);

  free_surface(&img_mirror);
  free_surface(&img_flip);
  free_surface(&img_rotate);

  free_surface(&img_title_on);
  free_surface(&img_title_off);
  free_surface(&img_title_large_on);
  free_surface(&img_title_large_off);

  free_surface(&img_open);
  free_surface(&img_erase);
  free_surface(&img_pict_export);
  free_surface(&img_back);
  free_surface(&img_trash);

  free_surface(&img_slideshow);
  free_surface(&img_template);
  free_surface(&img_play);
  free_surface(&img_gif_export);
  free_surface(&img_select_digits);

  free_surface(&img_dead40x40);

  free_surface(&img_printer);
  free_surface(&img_printer_wait);
  free_surface(&img_save_over);

  free_surface(&img_btn_up);
  free_surface(&img_btn_down);
  free_surface(&img_btn_off);
  free_surface(&img_btn_hold);

  free_surface(&img_btnsm_up);
  free_surface(&img_btnsm_off);
  free_surface(&img_btnsm_down);
  free_surface(&img_btnsm_hold);

  free_surface(&img_btn_nav);
  free_surface(&img_btnsm_nav);

  free_surface(&img_brush_anim);
  free_surface(&img_brush_dir);

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

  free_surface(&img_shapes_center);
  free_surface(&img_shapes_corner);

  free_surface(&img_bold);
  free_surface(&img_italic);

  free_surface(&img_label_select);
  free_surface(&img_label_apply);

  free_surface_array(undo_bufs, NUM_UNDO_BUFS);

  free_surface_array(img_color_btns, NUM_COLORS * 2);
  free(img_color_btns);

  if (onscreen_keyboard)
  {
    free_surface(&img_oskdel);
    free_surface(&img_osktab);
    free_surface(&img_oskenter);
    free_surface(&img_oskcapslock);
    free_surface(&img_oskshift);
    free_surface(&img_oskpaste);

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

  if (sim_flood_touched != NULL)
  {
    free(sim_flood_touched);
    sim_flood_touched = NULL;
  }

  if (medium_font != NULL)
  {
    DEBUG_PRINTF("cleanup: medium font\n");     //EP
    TuxPaint_Font_CloseFont(medium_font);
    medium_font = NULL;
  }

  if (small_font != NULL)
  {
    DEBUG_PRINTF("cleanup: small font\n");      //EP
    TuxPaint_Font_CloseFont(small_font);
    small_font = NULL;
  }

  if (large_font != NULL)
  {
    DEBUG_PRINTF("cleanup: large font\n");      //EP
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
  free_cursor(&cursor_pipette);

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


/**
 * FIXME
 */
static void free_surface(SDL_Surface **surface_array)
{
  if (surface_array)            //EP added this line to avoid app crash
    if (*surface_array)
    {
      SDL_FreeSurface(*surface_array);
      *surface_array = NULL;
    }
}


/**
 * FIXME
 */
static void free_surface_array(SDL_Surface *surface_array[], int count)
{
  int i;

  if (surface_array)            //EP added this line to avoid app crash
    for (i = 0; i < count; ++i)
    {
      free_surface(&surface_array[i]);
    }
}


/**
 * FIXME
 */
/* Draw a shape! */
static void do_shape(int sx, int sy, int nx, int ny, int rotn, int use_brush)
{
  int side, rx, ry, rmax, old_brush, step;
  float x1, y1, x2, y2, xp, yp, xv, yv;
  float a1, a2, rotn_rad, init_ang, angle_skip;
  int xx, yy, offx, offy, max_x, max_y;

  if (ny < sy)
  {
    rotn = (rotn + 180) % 360;
  }

  /* Determine radius/shape of the shape to draw: */

  old_brush = 0;

  rx = abs(nx - sx);
  ry = abs(ny - sy);
  if (shape_mode == SHAPEMODE_CORNER)
  {
    rx = sqrt(rx * rx) / 2;
    ry = sqrt(ry * ry) / 2;
  }

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

  if (rx < 2)
    rx = 2;
  if (ry < 2)
    ry = 2;


  /* Render a default brush: */

  if (use_brush)
  {
    old_brush = cur_brush;
    cur_brush = shape_brush;    /* Now only semi-kludgy! */
    render_brush();
  }


  /* Draw the shape: */

  angle_skip = 360.0 / (float)shape_sides[cur_shape];

  init_ang = shape_init_ang[cur_shape];


  if (shape_mode == SHAPEMODE_CORNER)
  {
    /* Get extent of shape based on it's vertices,
       and scale up if we need to
       (e.g., square's points are at 45, 135, 225 & 315 degrees,
       which do not extend to the full radius).

       This works well for square and rectangle; it mostly
       works for triangle and 5-pointed star, but it seems
       sufficient. -bjk 2020.08.15 */
    max_x = 0;
    max_y = 0;
    for (side = 0; side < shape_sides[cur_shape]; side++)
    {
      a1 = (angle_skip * side + init_ang) * M_PI / 180.0;
      a2 = (angle_skip * (side + 1) + init_ang) * M_PI / 180.0;
      x1 = (cos(a1) * (float)rx);
      y1 = (-sin(a1) * (float)ry);

      if (fabsf(x1) > max_x)
        max_x = fabsf(x1);
      if (fabsf(y1) > max_y)
        max_y = fabsf(y1);
    }

    if (max_x < rx)
      rx = (rx * rx) / (int)max_x;
    if (max_y < ry)
      ry = (ry * ry) / (int)max_y;
  }


  step = 1;

  if (dont_do_xor && !use_brush)
  {
    /* If we're in light outline mode & not drawing the shape with the brush,
       if it has lots of sides (like a circle), reduce the number of sides: */

    if (shape_sides[cur_shape] > 5)
      step = (shape_sides[cur_shape] / 8);
  }


  /* Where is the object? */
  if (shape_mode == SHAPEMODE_CENTER)
  {
    offx = 0;
    offy = 0;
  }
  else
  {
    offx = (nx - sx) / 2;
    offy = (ny - sy) / 2;

    if (shape_locked[cur_shape])
    {
      if (abs(offx) > abs(offy))
      {
        if (offy > 0)
          offy = abs(offx);
        else
          offy = -abs(offx);
      }
      else
      {
        if (offx > 0)
          offx = abs(offy);
        else
          offx = -abs(offy);
      }
    }
  }

  for (side = 0; side < shape_sides[cur_shape]; side = side + step)
  {
    a1 = (angle_skip * (float)side + init_ang) * M_PI / 180.0;

    x1 = (cos(a1) * (float)rx);
    y1 = (-sin(a1) * (float)ry);

    a2 = (angle_skip * ((float)side + 1.0) + init_ang) * M_PI / 180.0;

    x2 = (cos(a2) * (float)rx);
    y2 = (-sin(a2) * (float)ry);

    DEBUG_PRINTF("side=%d, a1=%f, a2=%f -- (%f,%f) -> (%f,%f)\n", side, a1, a2, x1, y1, x2, y2);

    xv = (cos((a1 + a2) / 2.0) * (float)rx * (float)shape_valley[cur_shape]) / 100.0;
    yv = (-sin((a1 + a2) / 2.0) * (float)ry * (float)shape_valley[cur_shape]) / 100.0;

    /* Rotate the line: */

    if (rotn != 0)
    {
      rotn_rad = rotn * M_PI / 180.0;

      if (shape_mode == SHAPEMODE_CENTER)
      {
        xp = (x1 + offx) * cos(rotn_rad) - (y1 + offy) * sin(rotn_rad);
        yp = (x1 + offx) * sin(rotn_rad) + (y1 + offy) * cos(rotn_rad);

        x1 = xp - offx;
        y1 = yp - offy;

        xp = (x2 + offx) * cos(rotn_rad) - (y2 + offy) * sin(rotn_rad);
        yp = (x2 + offx) * sin(rotn_rad) + (y2 + offy) * cos(rotn_rad);

        x2 = xp - offx;
        y2 = yp - offy;

        xp = (xv + offx) * cos(rotn_rad) - (yv + offy) * sin(rotn_rad);
        yp = (xv + offx) * sin(rotn_rad) + (yv + offy) * cos(rotn_rad);

        xv = xp - offx;
        yv = yp - offy;
      }
      else
      {
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
    }

    /* Center the line around the center of the shape: */

    x1 = x1 + sx + offx;
    y1 = y1 + sy + offy;
    x2 = x2 + sx + offx;
    y2 = y2 + sy + offy;
    xv = xv + sx + offx;
    yv = yv + sy + offy;


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
      /* Brush */
      if (shape_valley[cur_shape] == 100)
        brush_draw(x1, y1, x2, y2, 0);
      else
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
        a1 = (angle_skip * (float)side + init_ang) * M_PI / 180.0;
        a2 = (angle_skip * ((float)side + 1.0) + init_ang) * M_PI / 180.0;

        if (yy == xx * ry / rx)
        {
          x1 = (cos(a1) * xx);
          y1 = (-sin(a1) * yy);

          x2 = (cos(a2) * xx);
          y2 = (-sin(a2) * yy);

          xv = (cos((a1 + a2) / 2) * xx * shape_valley[cur_shape] / 100);
          yv = (-sin((a1 + a2) / 2) * yy * shape_valley[cur_shape] / 100);
        }
        else
        {
          x1 = (cos(a1) * yy);
          y1 = (-sin(a1) * xx);

          x2 = (cos(a2) * yy);
          y2 = (-sin(a2) * xx);

          xv = (cos((a1 + a2) / 2) * yy * shape_valley[cur_shape] / 100);
          yv = (-sin((a1 + a2) / 2) * xx * shape_valley[cur_shape] / 100);
        }

        /* Rotate the line: */

        if (rotn != 0)
        {
          rotn_rad = rotn * M_PI / 180;

          if (shape_mode == SHAPEMODE_CENTER)
          {
            xp = (x1 + offx) * cos(rotn_rad) - (y1 + offy) * sin(rotn_rad);
            yp = (x1 + offx) * sin(rotn_rad) + (y1 + offy) * cos(rotn_rad);

            x1 = xp - offx;
            y1 = yp - offy;

            xp = (x2 + offx) * cos(rotn_rad) - (y2 + offy) * sin(rotn_rad);
            yp = (x2 + offx) * sin(rotn_rad) + (y2 + offy) * cos(rotn_rad);

            x2 = xp - offx;
            y2 = yp - offy;

            xp = (xv + offx) * cos(rotn_rad) - (yv + offy) * sin(rotn_rad);
            yp = (xv + offx) * sin(rotn_rad) + (yv + offy) * cos(rotn_rad);

            xv = xp - offx;
            yv = yp - offy;
          }
          else
          {
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
        }


        /* Center the line around the center of the shape: */

        x1 = x1 + sx + offx;
        y1 = y1 + sy + offy;
        x2 = x2 + sx + offx;
        y2 = y2 + sy + offy;
        xv = xv + sx + offx;
        yv = yv + sy + offy;


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
        update_canvas(0, 0, WINDOW_WIDTH - r_ttoolopt.w, (button_h * 7) + 40 + HEIGHTOFFSET);
    }
  }


  /* Update it! */

  if (use_brush)
  {
    if (abs(rx) > abs(ry))
      rmax = abs(rx) + 20;
    else
      rmax = abs(ry) + 20;

    update_canvas(sx - rmax + offx, sy - rmax + offy, sx + rmax + offx, sy + rmax + offy);
  }


  /* Return to normal brush (for paint brush and line tools): */

  if (use_brush)
  {
    cur_brush = old_brush;
    render_brush();
    show_brush_tip();
  }
}


/**
 * FIXME
 */
/* What angle is the mouse away from the center of a shape? */
static int shape_rotation(int ctr_x, int ctr_y, int ox, int oy)
{
  int deg;

  deg = (atan2(oy - ctr_y, ox - ctr_x) * 180 / M_PI);

  if (shape_reverse)
  {
    deg = (deg + 180) % 360;
  }

  if (shape_radius < 50)
    deg = ((deg - 15) / 30) * 30;
  else if (shape_radius < 100)
    deg = ((deg - 7) / 15) * 15;

  /* Disabled b/c it adversely affected shape_locked shapes (square & octagon)
     with corner-dragged shapes; disabling doesn't seem to cause problems
     with any shape, in either drag mode... -bjk 2020-11-10 */
/*
  if (shape_locked[cur_shape])
    {
      int angle_skip;

      angle_skip = 360 / shape_sides[cur_shape];
      deg = deg % angle_skip;
    }
*/

  return (deg);
}


/**
 * FIXME
 */
/* What angle (degrees) is the mouse away from a brush drag, line draw, or stamp placement? */
static int brush_rotation(int ctr_x, int ctr_y, int ox, int oy)
{
  return (atan2(oy - ctr_y, ox - ctr_x) * 180 / M_PI);
}

static int stamp_will_rotate(int ctr_x, int ctr_y, int ox, int oy)
{
  return ((abs(ctr_x - ox) > button_w / 2) || (abs(ctr_y - oy) > button_h / 2));
}

/* What angle (degrees) is the mouse away from a stamp placement (keep at zero if within stamp's bounding box!) */
static int stamp_rotation(int ctr_x, int ctr_y, int ox, int oy)
{
  if (!stamp_will_rotate(ctr_x, ctr_y, ox, oy))
    return (0);

  return brush_rotation(ctr_x, ctr_y, ox, oy);
}


/* Prompt to ask whether user wishes to save over old version of their file */
#define PROMPT_SAVE_OVER_TXT gettext_noop("Replace the picture with your changes?")

/* Positive response to saving over old version
   (like a 'File:Save' action in other applications) */
#define PROMPT_SAVE_OVER_YES gettext_noop("Yes, replace the old one!")

/* Negative response to saving over old version (saves a new image)
   (like a 'File:Save As...' action in other applications) */
#define PROMPT_SAVE_OVER_NO  gettext_noop("No, save a new file!")


/**
 * FIXME
 */
/* Save the current image: */

static int do_save(int tool, int dont_show_success_results, int autosave)
{
  int scroll;
  char *fname;
  char tmp[FILENAME_MAX + 16];
  SDL_Surface *thm;
  FILE *fi;

  /* Was saving completely disabled? */
  if (disable_save && !autosave)
    return 0;

  scroll = (NUM_TOOLS > buttons_tall * gd_tools.cols) ? img_scroll_down->h : 0;
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
                              (TOOL_SAVE % 2) * button_w + button_w / 2,
                              (TOOL_SAVE / 2) * button_h + r_ttools.h +
                              button_h / 2 - tool_scroll * button_h / gd_tools.cols + scroll) == 0)
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

  if (!make_directory(DIR_SAVE, "", "Can't create user data directory (E002)"))
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

  if (!make_directory(DIR_SAVE, "saved", "Can't create user data directory (for saved drawings) (E003)"))
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

  if (!make_directory
      (DIR_SAVE, "saved/.thumbs", "Can't create user data thumbnail directory (for saved drawings' thumbnails) (E004)"))
  {
    fprintf(stderr, "Cannot save any pictures! SORRY!\n\n");
    draw_tux_text(TUX_OOPS, strerror(errno), 0);
    return 0;
  }



  if (!make_directory(DIR_SAVE, "saved/.label", "Can't create label information directory (E005)"))
  {
    fprintf(stderr, "Cannot save label information! SORRY!\n\n");
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

  safe_snprintf(tmp, sizeof(tmp), "saved/%s-t%s", file_id, FNAME_EXTENSION);
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

    safe_snprintf(tmp, sizeof(tmp), "saved/.thumbs/%s-t%s", file_id, FNAME_EXTENSION);
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
    safe_snprintf(tmp, sizeof(tmp), "saved/%s.dat", file_id);
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

/**
 * FIXME
 */
static void set_chunk_data(unsigned char **chunk_data, size_t *chunk_data_len,
                           size_t uncompressed_size, Bytef *data, size_t dataLen)
{
  int headersLen;
  unsigned int i;
  char *line, *headers, *cdata;
  size_t line_sz, headers_sz;

  headersLen = 0;
  headers_sz = 256;
  headers = calloc(headers_sz, 1);
  line_sz = 256;
  line = calloc(line_sz, 1);

  safe_strncat(headers, "Tuxpaint\n", headers_sz);
  safe_strncat(headers, "Tuxpaint_" VER_VERSION "\n", headers_sz);
  safe_snprintf(line, line_sz, "%lu%s", uncompressed_size, "\n");
  safe_strncat(headers, line, headers_sz);
  safe_snprintf(line, line_sz, "%lu%s", dataLen, "\n");
  safe_strncat(headers, line, headers_sz);

  headersLen = strlen(headers);
  *chunk_data_len = headersLen + dataLen;

  cdata = calloc(*chunk_data_len, sizeof(unsigned char *));
  strcat(cdata, headers);       /* FIXME: Use strncat() */

  for (i = 0; i < dataLen; i++)
    cdata[headersLen + i] = data[i];
  *chunk_data = (unsigned char *)cdata;

  free(line);
  free(headers);
}

/**
 * FIXME
 */
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
  size_t dat_size, char_stream_sz, line_sz;

#ifdef WIN32
  wchar_t wtmpchar;
  char tmpstr[16];
  size_t nbtmpstr;
#endif

  /* Starter foreground */
  if (img_starter)
  {
    DEBUG_PRINTF("Saving starter... %d\n", (int)(intptr_t) img_starter);        //EP added (intptr_t) to avoid warning on x64

    sbk_pixs = malloc(img_starter->h * img_starter->w * 4);
    compressedLen = compressBound(img_starter->h * img_starter->w * 4);

    compressed_data = malloc(compressedLen * sizeof(Bytef *));

    if (SDL_MUSTLOCK(img_starter))
      SDL_LockSurface(img_starter);

    for (y = 0; y < img_starter->h; y++)
      for (x = 0; x < img_starter->w; x++)
      {
        SDL_GetRGBA(getpixels[img_starter->format->BytesPerPixel]
                    (img_starter, x, y), img_starter->format, &r, &g, &b, &a);

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
        SDL_GetRGB(getpixels[img_starter_bkgd->format->BytesPerPixel]
                   (img_starter_bkgd, x, y), img_starter_bkgd->format, &r, &g, &b);

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
          SDL_GetRGBA(getpixels[img_starter->format->BytesPerPixel]
                      (img_starter, x, y), img_starter->format, &r, &g, &b, &a);

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

    DEBUG_PRINTF("%d \n", (int)compressedLen);

    compress(compressed_data, &compressedLen, sbk_pixs, img_starter_bkgd->h * img_starter_bkgd->w * 3);

    set_chunk_data(&chunk_data, &chunk_data_len,
                   img_starter_bkgd->w * img_starter_bkgd->h * 3, compressed_data, compressedLen);

    DEBUG_PRINTF("%d \n", (int)compressedLen);


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
      if (current_node->is_enabled == SDL_TRUE && current_node->save_texttool_len > 0)
      {
        fprintf(lfi, "%u\n", current_node->save_texttool_len);
        for (i = 0; i < current_node->save_texttool_len; i++)
        {
#ifdef WIN32
          wtmpchar = current_node->save_texttool_str[i];
          nbtmpstr = WideCharToMultiByte(CP_UTF8, 0, &wtmpchar, 1, tmpstr, 16, NULL, NULL);
          tmpstr[nbtmpstr] = '\0';
          fprintf(lfi, "%s", tmpstr);
#elif defined(__ANDROID__)
          fprintf(lfi, "%d ", (int)current_node->save_texttool_str[i]);
#else
          fprintf(lfi, "%lc", (wint_t) current_node->save_texttool_str[i]);
#endif
        }
        fprintf(lfi, "\n");

        fprintf(lfi, "%u\n", current_node->save_color.r);
        fprintf(lfi, "%u\n", current_node->save_color.g);
        fprintf(lfi, "%u\n", current_node->save_color.b);
        fprintf(lfi, "%d\n", current_node->save_width);
        fprintf(lfi, "%d\n", current_node->save_height);
        fprintf(lfi, "%u\n", current_node->save_x);
        fprintf(lfi, "%u\n", current_node->save_y);

        if (current_node->save_font_type == NULL)       /* Fonts yet setted */
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
              getpixels[current_node->label_node_surface->format->BytesPerPixel] (current_node->label_node_surface, x,
                                                                                  y);
            SDL_GetRGBA(pix, current_label_node->label_node_surface->format, &r, &g, &b, &a);
            fwrite(&a, alpha_size, 1, lfi);
          }
        SDL_UnlockSurface(current_node->label_node_surface);
        fprintf(lfi, "\n\n");
      }
      current_node = current_node->next_to_up_label_node;

      DEBUG_PRINTF("cur %p, red %p\n", current_node, first_label_node_in_redo_stack);
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
    line_sz = 256;
    line = calloc(line_sz, 1);
    char_stream_sz = 256 + sizeof(starter_id) + sizeof(template_id), char_stream = calloc(char_stream_sz, 1);

    safe_snprintf(char_stream, char_stream_sz, "%s\n", starter_id);

    safe_snprintf(line, line_sz, "%d %d %d\n", starter_mirrored, starter_flipped, starter_personal);
    safe_strncat(char_stream, line, char_stream_sz);

    safe_snprintf(line, line_sz, "c%d %d %d\n", canvas_color_r, canvas_color_g, canvas_color_b);
    safe_strncat(char_stream, line, char_stream_sz);

    safe_snprintf(line, line_sz, "T%s\n", template_id);
    safe_strncat(char_stream, line, char_stream_sz);

    safe_snprintf(line, line_sz, "%d\n", template_personal);
    safe_strncat(char_stream, line, char_stream_sz);

    safe_snprintf(line, line_sz, "M%d\n", starter_modified);
    safe_strncat(char_stream, line, char_stream_sz);

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

/**
 * FIXME
 */
/* Actually save the PNG data to the file stream: */
static int do_png_save(FILE *fi, const char *const fname, SDL_Surface *surf, int embed)
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

        png_set_IHDR(png_ptr, info_ptr, surf->w, surf->h, 8,
                     PNG_COLOR_TYPE_RGB, 1, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

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

/**
 * Generate a new file ID.  Used when saving a picture for
 * the first time (i.e., very first save, or saving as a new
 * file, rather than overwriting/replacing the existing version).
 *
 * Affects "file_id" string.
 */
static void get_new_file_id(void)
{
  time_t t;

  t = time(NULL);

  strftime(file_id, sizeof(file_id), "%Y%m%d%H%M%S", localtime(&t));
  debug(file_id);
}


/**
 * FIXME
 */
/* Handle quitting (and prompting to save, if necessary!) */
static int do_quit(int tool)
{
  int done, tmp_tool, scroll;

  if (!no_prompt_on_quit)
  {
    scroll = (NUM_TOOLS > buttons_tall * gd_tools.cols) ? img_scroll_down->h : 0;
    done =
      do_prompt_snd(PROMPT_QUIT_TXT, PROMPT_QUIT_YES, PROMPT_QUIT_NO,
                    SND_AREYOUSURE, (TOOL_QUIT % 2) * button_w + button_w / 2,
                    (TOOL_QUIT / 2) * button_h + r_ttools.h + button_h / 2 -
                    tool_scroll * button_h / gd_tools.cols + scroll);
  }
  else
  {
    done = 1;
  }

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
/**
 * FIXME
 */
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
  char fname[MAX_PATH];
  int num_files, i, done, slideshow, update_list, want_erase, want_export, want_template;
  int cur, which, num_files_in_dirs, j, any_saved_files;
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

    if (!reversesort)
      qsort(fs, num_files_in_dirs, sizeof(struct dirent2), (int (*)(const void *, const void *))compare_dirent2s);
    else
      qsort(fs, num_files_in_dirs, sizeof(struct dirent2),
            (int (*)(const void *, const void *))compare_dirent2s_invert);


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
            safe_strncpy(fname, f->d_name, sizeof(fname));
            if (strcasestr(fname, FNAME_EXTENSION) != NULL)
            {
              d_exts[num_files] = strdup(strcasestr(fname, FNAME_EXTENSION));
              strcpy((char *)strcasestr(fname, FNAME_EXTENSION), "");   /* Safe; truncating */
            }

            if (strcasestr(fname, ".bmp") != NULL)
            {
              d_exts[num_files] = strdup(strcasestr(fname, ".bmp"));
              strcpy((char *)strcasestr(fname, ".bmp"), "");    /* Safe; truncating */
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

            safe_snprintf(fname, sizeof(fname), "%s/.thumbs/%s-t.png",
                          dirname[d_places[num_files]], d_names[num_files]);
            debug(fname);
            img = IMG_Load(fname);

            if (img == NULL)
            {
              /* No thumbnail in the new location ("saved/.thumbs"),
                 try the old locatin ("saved/"): */

              safe_snprintf(fname, sizeof(fname), "%s/%s-t.png", dirname[d_places[num_files]], d_names[num_files]);
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
                fprintf(stderr, "\nError: Couldn't create a thumbnail of " "saved image!\n" "%s\n", fname);
              }

              num_files++;
            }
            else
            {
              /* No thumbnail - load original: */

              /* Make sure we have a ~/.tuxpaint/saved directory: */
              if (make_directory(DIR_SAVE, "saved", "Can't create user data directory (for saved drawings) (E006)"))
              {
                /* (Make sure we have a .../saved/.thumbs/ directory:) */
                make_directory(DIR_SAVE, "saved/.thumbs",
                               "Can't create user data thumbnail directory (for saved drawings' thumbnails) (E007)");
              }


              if (img == NULL)
              {
                safe_snprintf(fname, sizeof(fname), "%s/%s", dirname[d_places[num_files]], f->d_name);
                debug(fname);
                img = myIMG_Load(fname);
              }


              show_progress_bar(screen);

              if (img == NULL)
              {
                fprintf(stderr,
                        "\nWarning: I can't open one of the saved files!\n"
                        "%s\n"
                        "The Simple DirectMedia Layer error that " "occurred was:\n" "%s\n\n", fname, SDL_GetError());

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
                  fprintf(stderr, "\nError: Couldn't create a thumbnail of " "saved image!\n" "%s\n", fname);
                }

                SDL_FreeSurface(img);

                show_progress_bar(screen);


                /* Let's save this thumbnail, so we don't have to
                   create it again next time 'Open' is called: */

                if (d_places[num_files] == PLACE_SAVED_DIR)
                {
                  debug("Saving thumbnail for this one!");

                  safe_snprintf(fname, sizeof(fname), "%s/.thumbs/%s-t.png",
                                dirname[d_places[num_files]], d_names[num_files]);

                  fi = fopen(fname, "wb");
                  if (fi == NULL)
                  {
                    fprintf(stderr,
                            "\nError: Couldn't save thumbnail of "
                            "saved image!\n" "%s\n" "The error that occurred was:\n" "%s\n\n", fname, strerror(errno));
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


    DEBUG_PRINTF("%d saved files were found!\n", num_files);


    if (num_files == 0)
    {
      do_prompt_snd(PROMPT_OPEN_NOFILES_TXT, PROMPT_OPEN_NOFILES_YES, "",
                    SND_YOUCANNOT,
                    (TOOL_OPEN % 2) * button_w + button_w / 2,
                    (TOOL_OPEN / 2) * button_h + r_ttools.h + button_h / 2 - tool_scroll * button_h / gd_tools.cols);
    }
    else
    {
      /* Let user choose an image: */

      /* Instructions for 'Open' file dialog */
      const char *instructions;
      int num_left_buttons;

      if (!disable_template_export)
      {
        instructions =
          gettext_noop
          ("Choose a picture and then click “Open”, “Export”, “Template“, or “Erase”. Click “Slides” to create a slideshow animation or “Back“ to return to your current picture.");
      }
      else
      {
        instructions =
          gettext_noop
          ("Choose a picture and then click “Open”, “Export”, or “Erase”. Click “Slides” to create a slideshow animation or “Back“ to return to your current picture.");
      }

      draw_tux_text(TUX_BORED, instructions, 1);

      /* NOTE: cur is now set above; if file_id'th file is found, it's
         set to that file's index; otherwise, we default to '0' */

      update_list = 1;
      want_erase = 0;
      want_export = 0;
      want_template = 0;

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

          dest.x = r_ttools.w;
          dest.y = 0;
          dest.w = WINDOW_WIDTH - r_ttoolopt.w - r_ttools.w;
          dest.h = button_h * buttons_tall + r_ttools.h;

          SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));


          /* Draw icons: */

          for (i = cur; i < cur + 16 && i < num_files; i++)
          {
            /* Draw cursor: */

            dest.x = THUMB_W * ((i - cur) % 4) + r_ttools.w;
            dest.y = THUMB_H * ((i - cur) / 4) + img_scroll_up->h;

            if (i == which)
            {
              SDL_BlitSurface(img_cursor_down, NULL, screen, &dest);
              debug(d_names[i]);
            }
            else
              SDL_BlitSurface(img_cursor_up, NULL, screen, &dest);



            dest.x = THUMB_W * ((i - cur) % 4) + r_ttools.w + 10 + (THUMB_W - 20 - thumbs[i]->w) / 2;
            dest.y = THUMB_H * ((i - cur) / 4) + img_scroll_up->h + 10 + (THUMB_H - 20 - thumbs[i]->h) / 2;

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
          dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;

          if (cur < num_files - 16)
            SDL_BlitSurface(img_scroll_down, NULL, screen, &dest);
          else
            SDL_BlitSurface(img_scroll_down_off, NULL, screen, &dest);


          /* "Open" button: */

          dest.x = r_ttools.w;
          dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
          SDL_BlitSurface(img_open, NULL, screen, &dest);

          dest.x = r_ttools.w + (button_w - img_openlabels_open->w) / 2;
          dest.y = (button_h * buttons_tall + r_ttools.h) - img_openlabels_open->h;
          SDL_BlitSurface(img_openlabels_open, NULL, screen, &dest);


          /* "Slides" (slideshow) button: */

          dest.x = r_ttools.w + button_w;
          dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
          if (any_saved_files)
            SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
          else
            SDL_BlitSurface(img_btn_off, NULL, screen, &dest);

          dest.x = r_ttools.w + button_w;
          dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
          SDL_BlitSurface(img_slideshow, NULL, screen, &dest);

          dest.x = r_ttools.w + button_w + (button_w - img_openlabels_slideshow->w) / 2;
          dest.y = (button_h * buttons_tall + r_ttools.h) - img_openlabels_slideshow->h;
          SDL_BlitSurface(img_openlabels_slideshow, NULL, screen, &dest);


          if (!disable_template_export)
          {
            /* "Template" (make template) button: */

            num_left_buttons = 3;

            dest.x = r_ttools.w + button_w * 2;
            dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
            if (any_saved_files)
              SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
            else
              SDL_BlitSurface(img_btn_off, NULL, screen, &dest);

            dest.x = r_ttools.w + button_w * 2;
            dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
            SDL_BlitSurface(img_template, NULL, screen, &dest);

            dest.x = r_ttools.w + button_w * 2 + (button_w - img_openlabels_template->w) / 2;
            dest.y = (button_h * buttons_tall + r_ttools.h) - img_openlabels_template->h;
            SDL_BlitSurface(img_openlabels_template, NULL, screen, &dest);
          }
          else
          {
            num_left_buttons = 2;
          }


          /* "Back" button: */

          dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w;
          dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
          SDL_BlitSurface(img_back, NULL, screen, &dest);

          dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w + (button_w - img_openlabels_back->w) / 2;
          dest.y = (button_h * buttons_tall + r_ttools.h) - img_openlabels_back->h;
          SDL_BlitSurface(img_openlabels_back, NULL, screen, &dest);


          /* "Export" button: */

          dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w - button_w - button_w;
          dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;

          if (d_places[which] != PLACE_STARTERS_DIR && d_places[which] != PLACE_PERSONAL_STARTERS_DIR)
            SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
          else
            SDL_BlitSurface(img_btn_off, NULL, screen, &dest);

          dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w - button_w - button_w + (button_w - img_pict_export->w) / 2;
          dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
          SDL_BlitSurface(img_pict_export, NULL, screen, &dest);

          dest.x =
            WINDOW_WIDTH - r_ttoolopt.w - button_w - button_w - button_w +
            (button_w - img_openlabels_pict_export->w) / 2;
          dest.y = (button_h * buttons_tall + r_ttools.h) - img_openlabels_pict_export->h;
          SDL_BlitSurface(img_openlabels_pict_export, NULL, screen, &dest);


          /* "Erase" button: */

          if (!disable_erase)
          {
            dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w - button_w;
            dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;

            if (d_places[which] != PLACE_STARTERS_DIR && d_places[which] != PLACE_PERSONAL_STARTERS_DIR)
              SDL_BlitSurface(img_erase, NULL, screen, &dest);
            else
              SDL_BlitSurface(img_btn_off, NULL, screen, &dest);

            dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w - button_w + (button_w - img_openlabels_erase->w) / 2;
            dest.y = (button_h * buttons_tall + r_ttools.h) - img_openlabels_erase->h;
            SDL_BlitSurface(img_openlabels_erase, NULL, screen, &dest);
          }

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
            else if (key == SDLK_RETURN)        /* space also conflicts with handle_keymouse || key == SDLK_SPACE) */
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
            else if (!disable_erase && key == SDLK_d && (event.key.keysym.mod & KMOD_CTRL) && d_places[which] != PLACE_STARTERS_DIR &&  // FIXME: Meaningless?
                     d_places[which] != PLACE_PERSONAL_STARTERS_DIR && !noshortcuts)    // FIXME: Meaningless?
            {
              /* Delete! */

              want_erase = 1;
            }
          }
          else
            if ((event.type == SDL_MOUSEBUTTONDOWN
                 && valid_click(event.button.button)) || event.type == TP_SDL_MOUSEBUTTONSCROLL)
          {
            if (event.button.x >= r_ttools.w
                && event.button.x < WINDOW_WIDTH - r_ttoolopt.w
                && event.button.y >= img_scroll_up->h
                && event.button.y < (button_h * buttons_tall + r_ttools.h) - button_h)
            {
              /* Picked an icon! */

              int old_which = which;

              which =
                ((event.button.x - r_ttools.w) / (THUMB_W) +
                 (((event.button.y - img_scroll_up->h) / THUMB_H) * 4)) + cur;

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
            else if (event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2
                     && event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2)
            {
              if (event.button.y < img_scroll_up->h ||
                  (event.button.y >=
                   (button_h * buttons_tall + r_ttools.h) - button_h
                   && event.button.y < (button_h * buttons_tall + r_ttools.h) - img_scroll_up->h))
              {
                /* Up or down scroll button in Open dialog: */

                if (event.button.y < img_scroll_up->h)
                {
                  /* Up scroll button in Open dialog: */

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
                else if (event.button.y >=
                         (button_h * buttons_tall + r_ttools.h) - button_h
                         && event.button.y < (button_h * buttons_tall + r_ttools.h) - img_scroll_up->h)
                {
                  /* Down scroll button in Open dialog: */

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

                if (scrolltimer_dialog != TIMERID_NONE)
                {
                  SDL_RemoveTimer(scrolltimer_dialog);
                  scrolltimer_dialog = TIMERID_NONE;
                }

                if (!scrolling_dialog && event.type == SDL_MOUSEBUTTONDOWN)
                {
                  DEBUG_PRINTF("Starting scrolling\n");
                  memcpy(&scrolltimer_dialog_event, &event, sizeof(SDL_Event));
                  scrolltimer_dialog_event.type = TP_SDL_MOUSEBUTTONSCROLL;

                  scrolling_dialog = 1;
                  scrolltimer_dialog =
                    SDL_AddTimer(REPEAT_SPEED, scrolltimer_dialog_callback, (void *)&scrolltimer_dialog_event);
                }
                else
                {
                  DEBUG_PRINTF("Continuing scrolling\n");
                  scrolltimer_dialog =
                    SDL_AddTimer(REPEAT_SPEED / 3, scrolltimer_dialog_callback, (void *)&scrolltimer_dialog_event);
                }
              }
            }
            else if (event.button.x >= r_ttools.w
                     && event.button.x < r_ttools.w + button_w
                     && event.button.y >=
                     (button_h * buttons_tall + r_ttools.h) - button_h
                     && event.button.y < (button_h * buttons_tall + r_ttools.h))
            {
              /* Open */

              done = 1;
              playsound(screen, 1, SND_CLICK, 1, SNDPOS_LEFT, SNDDIST_NEAR);
            }
            else if (!disable_template_export &&
                     event.button.x >= r_ttools.w + button_w * 2
                     && event.button.x < r_ttools.w + button_w * 3
                     && event.button.y >=
                     (button_h * buttons_tall + r_ttools.h) - button_h
                     && event.button.y < (button_h * buttons_tall + r_ttools.h) && any_saved_files == 1)
            {
              /* Make Template */

              want_template = 1;
            }
            else if (event.button.x >= r_ttools.w + button_w
                     && event.button.x < r_ttools.w + button_w + button_w
                     && event.button.y >=
                     (button_h * buttons_tall + r_ttools.h) - button_h
                     && event.button.y < (button_h * buttons_tall + r_ttools.h) && any_saved_files == 1)
            {
              /* Slideshow */

              done = 1;
              slideshow = 1;
              playsound(screen, 1, SND_CLICK, 1, SNDPOS_LEFT, SNDDIST_NEAR);
            }
            else if (event.button.x >=
                     (WINDOW_WIDTH - r_ttoolopt.w - button_w)
                     && event.button.x < (WINDOW_WIDTH - r_ttoolopt.w)
                     && event.button.y >=
                     (button_h * buttons_tall + r_ttools.h) - button_h
                     && event.button.y < (button_h * buttons_tall + r_ttools.h))
            {
              /* Back */

              which = -1;
              done = 1;
              playsound(screen, 1, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);
            }
            else if (!disable_erase &&
                     event.button.x >=
                     (WINDOW_WIDTH - r_ttoolopt.w - button_w - button_w)
                     && event.button.x <
                     (WINDOW_WIDTH - button_w - r_ttoolopt.w)
                     && event.button.y >=
                     (button_h * buttons_tall + r_ttools.h) - button_h
                     && event.button.y <
                     (button_h * buttons_tall + r_ttools.h)
                     && d_places[which] != PLACE_STARTERS_DIR && d_places[which] != PLACE_PERSONAL_STARTERS_DIR)
            {
              /* Erase */

              want_erase = 1;
            }
            else if (event.button.x >=
                     (WINDOW_WIDTH - r_ttoolopt.w - button_w - button_w -
                      button_w)
                     && event.button.x <
                     (WINDOW_WIDTH - button_w - button_w - r_ttoolopt.w)
                     && event.button.y >=
                     (button_h * buttons_tall + r_ttools.h) - button_h
                     && event.button.y <
                     (button_h * buttons_tall + r_ttools.h)
                     && d_places[which] != PLACE_STARTERS_DIR && d_places[which] != PLACE_PERSONAL_STARTERS_DIR)
            {
              /* Export */

              want_export = 1;
            }
#ifdef __ANDROID__
            start_motion_convert(event);
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

            if (event.button.y < img_scroll_up->h &&
                event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2 &&
                event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2 && cur > 0)
            {
              /* Scroll up button: */

              do_setcursor(cursor_up);
            }
            else if (event.button.y >=
                     (button_h * buttons_tall + r_ttools.h) - button_h
                     && event.button.y <
                     (button_h * buttons_tall + r_ttools.h - img_scroll_up->h)
                     && event.button.x >=
                     (WINDOW_WIDTH - img_scroll_up->w) / 2
                     && event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2 && cur < num_files - 16)
            {
              /* Scroll down button: */

              do_setcursor(cursor_down);
            }
            else if (event.button.y >=
                     (button_h * buttons_tall + r_ttools.h) - button_h
                     && event.button.y < (button_h * buttons_tall + r_ttools.h))
            {
              if (event.button.x >= r_ttools.w && event.button.x < r_ttools.w + (button_w * num_left_buttons))
              {
                /* One of the command buttons on the left: Open, Slides, Template [maybe] */
                do_setcursor(cursor_hand);
              }
              else if (event.button.x >=
                       (WINDOW_WIDTH - r_ttoolopt.w - button_w) && event.button.x < (WINDOW_WIDTH - r_ttoolopt.w))
              {
                /* Command button on the right: Back */
                do_setcursor(cursor_hand);
              }
              else if (event.button.x >=
                       (WINDOW_WIDTH - r_ttoolopt.w - button_w * 2)
                       && event.button.x < (WINDOW_WIDTH - r_ttoolopt.w - button_w) && !disable_erase)
              {
                /* Command button on the right: Erase [maybe] */
                do_setcursor(cursor_hand);
              }
              else if (event.button.x >=
                       (WINDOW_WIDTH - r_ttoolopt.w - button_w * 3)
                       && event.button.x < (WINDOW_WIDTH - r_ttoolopt.w - button_w * 2))
              {
                /* Command button on the right: Export */
                do_setcursor(cursor_hand);
              }
              else
              {
                do_setcursor(cursor_arrow);
              }
            }
            else if (event.button.x >= r_ttools.w
                     && event.button.x < WINDOW_WIDTH - r_ttoolopt.w
                     && event.button.y > img_scroll_up->h
                     && event.button.y <
                     (button_h * buttons_tall + r_ttools.h) - button_h
                     &&
                     ((((event.button.x - r_ttools.w) / (THUMB_W) +
                        (((event.button.y - img_scroll_up->h) / THUMB_H) * 4)) + cur) < num_files))
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

          else if (event.type == SDL_MOUSEBUTTONUP)
          {
#ifdef __ANDROID__
            stop_motion_convert(event);
#endif
            if (scrolling_dialog)
            {
              if (scrolltimer_dialog != TIMERID_NONE)
              {
                SDL_RemoveTimer(scrolltimer_dialog);
                scrolltimer_dialog = TIMERID_NONE;
              }
              scrolling_dialog = 0;
              DEBUG_PRINTF("Killing dialog scrolling\n");
            }
          }

          else if (event.type == SDL_JOYAXISMOTION)
            handle_joyaxismotion(event, &motioner, &val_x, &val_y);

          else if (event.type == SDL_JOYHATMOTION)
            handle_joyhatmotion(event, oldpos_x, oldpos_y, &valhat_x, &valhat_y, &hatmotioner, &old_hat_ticks);

          else if (event.type == SDL_JOYBALLMOTION)
            handle_joyballmotion(event, oldpos_x, oldpos_y);

          else if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP)
            handle_joybuttonupdown(event, oldpos_x, oldpos_y);
        }                       /* while (SDL_PollEvent(&event)) */

        if (motioner | hatmotioner)
          handle_motioners(oldpos_x, oldpos_y, motioner, hatmotioner, old_hat_ticks, val_x, val_y, valhat_x, valhat_y);


        SDL_Delay(10);

        if (want_erase)
        {
          want_erase = 0;

          if (do_prompt_image_snd(PROMPT_ERASE_TXT,
                                  PROMPT_ERASE_YES, PROMPT_ERASE_NO,
                                  thumbs[which],
                                  img_popup_arrow, img_trash, SND_AREYOUSURE,
                                  WINDOW_WIDTH - r_ttoolopt.w - button_w -
                                  button_w + 24, button_h * buttons_tall + r_ttools.h - button_h + img_scroll_up->h))
          {
            safe_snprintf(fname, sizeof(fname), "saved/%s%s", d_names[which], d_exts[which]);

            rfname = get_fname(fname, DIR_SAVE);

            if (trash(rfname) == 0)
            {
              update_list = 1;


              /* Delete the thumbnail, too: */

              safe_snprintf(fname, sizeof(fname), "saved/.thumbs/%s-t.png", d_names[which]);

              free(rfname);
              rfname = get_fname(fname, DIR_SAVE);

              unlink(rfname);


              /* Try deleting old-style thumbnail, too: */

              safe_snprintf(fname, sizeof(fname), "saved/%s-t.png", d_names[which]);

              free(rfname);
              rfname = get_fname(fname, DIR_SAVE);

              unlink(rfname);


              /* Delete .dat file, if any: */

              safe_snprintf(fname, sizeof(fname), "saved/%s.dat", d_names[which]);

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

        if (want_export)
        {
          int res;
          char *dest_fname;

          want_export = 0;

          safe_snprintf(fname, sizeof(fname), "saved/%s%s", d_names[which], d_exts[which]);
          rfname = get_fname(fname, DIR_SAVE);
          res = export_pict(rfname, EXPORT_LOC_PICTURES, NULL, &dest_fname);

          if (res == EXPORT_SUCCESS)
          {
            int n;
            char *msg;

            if (dest_fname != NULL)
            {
              n = asprintf(&msg, PROMPT_PICT_EXPORT_TXT, dest_fname);
              free(dest_fname);
            }
            else
            {
              n = asprintf(&msg, PROMPT_PICT_EXPORT_TXT, "???");
            }

            if (n != -1)
            {
              do_prompt_snd(msg, PROMPT_EXPORT_YES, "", SND_TUXOK, screen->w / 2, screen->h / 2);
              free(msg);
            }
            else
            {
              do_prompt_snd(PROMPT_PICT_EXPORT_TXT, PROMPT_EXPORT_YES, "", SND_TUXOK, screen->w / 2, screen->h / 2);
            }
          }
          else
          {
            do_prompt_snd(PROMPT_PICT_EXPORT_FAILED_TXT, PROMPT_EXPORT_YES,
                          "", SND_YOUCANNOT, screen->w / 2, screen->h / 2);
          }

          draw_tux_text(TUX_BORED, instructions, 1);
          update_list = 1;
        }

        if (want_template)
        {
          int res;

          want_template = 0;

          safe_snprintf(fname, sizeof(fname), "saved/%s%s", d_names[which], d_exts[which]);
          rfname = get_fname(fname, DIR_SAVE);
          res = export_pict(rfname, EXPORT_LOC_TEMPLATES, d_names[which], NULL);

          if (res == EXPORT_SUCCESS)
            do_prompt_snd(PROMPT_PICT_TEMPLATE_TXT, PROMPT_TEMPLATE_YES, "", SND_TUXOK, screen->w / 2, screen->h / 2);
          else if (res == EXPORT_ERR_ALREADY_EXPORTED)
            do_prompt_snd(PROMPT_PICT_TEMPLATE_EXISTS_TXT,
                          PROMPT_TEMPLATE_YES, "", SND_YOUCANNOT, screen->w / 2, screen->h / 2);
          else
            do_prompt_snd(PROMPT_PICT_TEMPLATE_FAILED_TXT,
                          PROMPT_TEMPLATE_YES, "", SND_YOUCANNOT, screen->w / 2, screen->h / 2);

          draw_tux_text(TUX_BORED, instructions, 1);
          update_list = 1;
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
                                    img_tools[TOOL_SAVE], NULL, NULL, SND_AREYOUSURE, screen->w / 2, screen->h / 2))
            {
              do_save(TOOL_OPEN, 1, 0);
            }
          }

          /* Clean the label stuff */
          delete_label_list(&start_label_node);
          start_label_node = current_label_node =
            first_label_node_in_redo_stack = highlighted_label_node = label_node_to_edit = NULL;
          have_to_rec_label_node = SDL_FALSE;

          /* Clean stale text */
          if (texttool_len > 0)
          {
            texttool_str[0] = L'\0';
            texttool_len = 0;
            cursor_textwidth = 0;
          }

          SDL_FillRect(label, NULL, SDL_MapRGBA(label->format, 0, 0, 0, 0));

          /* Figure out filename: */

          safe_snprintf(fname, sizeof(fname), "%s/%s%s", dirname[d_places[which]], d_names[which], d_exts[which]);
          fi = fopen(fname, "r");
          if (fi == NULL)
          {
            fprintf(stderr,
                    "\nWarning: Couldn't load the saved image! (1)\n" "%s\n" "The file is missing.\n\n\n", fname);
            do_prompt(PROMPT_OPEN_UNOPENABLE_TXT, PROMPT_OPEN_UNOPENABLE_YES, "", 0, 0);
          }
          fclose(fi);

          img = myIMG_Load(fname);

          if (img == NULL)
          {
            fprintf(stderr,
                    "\nWarning: Couldn't load the saved image! (2)\n"
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

            DEBUG_PRINTF("Smearing canvas @ 5\n");
            org_surf = SDL_DisplayFormat(img);  /* Keep a copy of the original image
                                                   unscaled to send to load_embedded_data */
            autoscale_copy_smear_free(img, canvas, SDL_BlitSurface);

            cur_undo = 0;
            oldest_undo = 0;
            newest_undo = 0;

            /* Saved image: */

            been_saved = 1;

            safe_strncpy(file_id, d_names[which], sizeof(file_id));
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

            /* FIXME: Consider using the new
               autoscale_copy_smear_or_scale_free() based on the starter/template
               file's options? (Will need to do here, rather than first thing,
               above) -bjk 2023.02.09 */

            reset_avail_tools();

            tool_avail_bak[TOOL_UNDO] = 0;
            tool_avail_bak[TOOL_REDO] = 0;

            opened_something = 1;
          }
        }
      }


      update_canvas(0, 0, WINDOW_WIDTH - r_ttoolopt.w - r_ttools.w, button_h * buttons_tall + r_ttools.h);
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

/**
 * FIXME
 */
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
  char *instructions;
  int speeds;
  float x_per, y_per;
  int xx, yy;
  SDL_Surface *btn, *blnk;
  int val_x, val_y, motioner;
  int valhat_x, valhat_y, hatmotioner;
  int export_successful;

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

  if (!reversesort)
    qsort(fs, num_files_in_dir, sizeof(struct dirent2), (int (*)(const void *, const void *))compare_dirent2s);
  else
    qsort(fs, num_files_in_dir, sizeof(struct dirent2), (int (*)(const void *, const void *))compare_dirent2s_invert);


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
          safe_strncpy(fname, f->d_name, sizeof(fname));
          if (strcasestr(fname, FNAME_EXTENSION) != NULL)
          {
            d_exts[num_files] = strdup(strcasestr(fname, FNAME_EXTENSION));
            strcpy((char *)strcasestr(fname, FNAME_EXTENSION), "");     /* FIXME: Use strncpy() (ugh, complicated) */
          }

          if (strcasestr(fname, ".bmp") != NULL)
          {
            d_exts[num_files] = strdup(strcasestr(fname, ".bmp"));
            strcpy((char *)strcasestr(fname, ".bmp"), "");      /* Safe; truncating */
          }

          d_names[num_files] = strdup(fname);


          /* FIXME: Try to center list on whatever was selected
             in do_open() when the slideshow button was clicked. */

          /* Try to load thumbnail first: */

          safe_snprintf(fname, sizeof(fname), "%s/.thumbs/%s-t.png", dirname, d_names[num_files]);
          debug("Loading thumbnail...");
          debug(fname);
          img = IMG_Load(fname);
          if (img == NULL)
          {
            /* No thumbnail in the new location ("saved/.thumbs"),
               try the old locatin ("saved/"): */

            safe_snprintf(fname, sizeof(fname), "%s/%s-t.png", dirname, d_names[num_files]);
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
            if (make_directory(DIR_SAVE, "saved", "Can't create user data directory (for saved drawings) (E008)"))
            {
              /* (Make sure we have a .../saved/.thumbs/ directory:) */
              make_directory(DIR_SAVE, "saved/.thumbs",
                             "Can't create user data thumbnail directory (for saved drawings' thumbnails) (E009)");
            }

            safe_snprintf(fname, sizeof(fname), "%s/%s", dirname, f->d_name);

            debug("Loading original, to make thumbnail");
            debug(fname);
            img = myIMG_Load(fname);


            show_progress_bar(screen);


            if (img == NULL)
            {
              fprintf(stderr,
                      "\nWarning: I can't open one of the saved files!\n"
                      "%s\n"
                      "The Simple DirectMedia Layer error that " "occurred was:\n" "%s\n\n", fname, SDL_GetError());
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

                safe_snprintf(fname, sizeof(fname), "%s/.thumbs/%s-t.png", dirname, d_names[num_files]);

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

  DEBUG_PRINTF("%d saved files were found!\n", num_files);

  /* Let user choose images: */

  /* Instructions for Slideshow file dialog */
  instructions = strdup(TUX_TIP_SLIDESHOW);
  draw_tux_text(TUX_BORED, instructions, 1);

  /* Focus us around the newest images, as it's highly likely the
     user wants to make a slideshow out of more recent images (vs. very
     old ones) */
  if (!reversesort)
  {
    /* Default sort puts newest at the top, oldest at the bottom,
       so start at the end */
    which = num_files - 1;
    cur = ((num_files - 16) / 4) * 4;
    if (cur < 0)
      cur = 0;
  }
  else
  {
    /* "reversesort" option puts oldest at the top, so start there */
    which = 0;
    cur = 0;
  }

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

      dest.x = r_ttools.w;
      dest.y = 0;
      dest.w = WINDOW_WIDTH - r_ttoolopt.w - r_ttools.w;
      dest.h = button_h * buttons_tall + r_ttools.h;

      SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));


      /* Draw icons: */

      for (i = cur; i < cur + 16 && i < num_files; i++)
      {
        /* Draw cursor: */

        dest.x = THUMB_W * ((i - cur) % 4) + r_ttools.w;
        dest.y = THUMB_H * ((i - cur) / 4) + img_scroll_up->h;

        if (i == which)
        {
          SDL_BlitSurface(img_cursor_down, NULL, screen, &dest);
          debug(d_names[i]);
        }
        else
          SDL_BlitSurface(img_cursor_up, NULL, screen, &dest);

        if (thumbs[i] != NULL)
        {
          dest.x = THUMB_W * ((i - cur) % 4) + r_ttools.w + 10 + (THUMB_W - 20 - thumbs[i]->w) / 2;
          dest.y = THUMB_H * ((i - cur) / 4) + img_scroll_up->h + 10 + (THUMB_H - 20 - thumbs[i]->h) / 2;

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
          dest.x = (THUMB_W * ((i - cur) % 4) + r_ttools.h + 10 + (THUMB_W - 20 - thumbs[i]->w) / 2) + thumbs[i]->w;
          dest.y =
            (THUMB_H * ((i - cur) / 4) + img_scroll_up->h + 10 + (THUMB_H - 20 - thumbs[i]->h) / 2) + thumbs[i]->h;

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
      dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;

      if (cur < num_files - 16)
        SDL_BlitSurface(img_scroll_down, NULL, screen, &dest);
      else
        SDL_BlitSurface(img_scroll_down_off, NULL, screen, &dest);


      /* "Play" button: */

      dest.x = r_ttools.w;
      dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
      SDL_BlitSurface(img_play, NULL, screen, &dest);

      dest.x = r_ttools.w + (button_w - img_openlabels_play->w) / 2;
      dest.y = (button_h * buttons_tall + r_ttools.h) - img_openlabels_play->h; // FIXME: CROP LABELS
      SDL_BlitSurface(img_openlabels_play, NULL, screen, &dest);


      /* "GIF Export" button: */

      dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w * 2;
      dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
      SDL_BlitSurface(img_btn_up, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w * 2 + (button_w - img_gif_export->w) / 2;
      dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
      SDL_BlitSurface(img_gif_export, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w * 2 + (button_w - img_openlabels_gif_export->w) / 2;
      dest.y = (button_h * buttons_tall + r_ttools.h) - img_openlabels_gif_export->h;   // FIXME: CROP LABELS
      SDL_BlitSurface(img_openlabels_gif_export, NULL, screen, &dest);


      /* "Back" button: */

      dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w;
      dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
      SDL_BlitSurface(img_back, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w + (button_w - img_openlabels_back->w) / 2;
      dest.y = (button_h * buttons_tall + r_ttools.h) - img_openlabels_back->h; // FIXME: CROP LABELS
      SDL_BlitSurface(img_openlabels_back, NULL, screen, &dest);


      /* Speed control: */

      speeds = 10;
      x_per = (float)r_ttools.w / speeds;
      y_per = (float)button_h / speeds;

      for (i = 0; i < speeds; i++)
      {
        xx = ceil(x_per);
        yy = ceil(y_per * i);

        if (i <= speed)
          btn = thumbnail(img_btn_down, xx, yy, 0);
        else
          btn = thumbnail(img_btn_up, xx, yy, 0);

        blnk = thumbnail(img_btn_off, xx, button_h - yy, 0);

        /* FIXME: Check for NULL! */

        dest.x = r_ttools.w + button_w + (i * x_per);
        dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
        SDL_BlitSurface(blnk, NULL, screen, &dest);

        dest.x = r_ttools.w + button_w + (i * x_per);
        dest.y = (button_h * buttons_tall + r_ttools.h) - (y_per * i);
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
        dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
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
          event.button.y = (button_h * buttons_tall + r_ttools.h) - button_h + 5;
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
      else
        if ((event.type == SDL_MOUSEBUTTONDOWN
             && valid_click(event.button.button)) || event.type == TP_SDL_MOUSEBUTTONSCROLL)
      {
        if (event.button.x >= r_ttools.w
            && event.button.x < WINDOW_WIDTH - r_ttoolopt.w
            && event.button.y >= img_scroll_up->h && event.button.y < (button_h * buttons_tall + r_ttools.h - button_h))
        {
          /* Picked an icon! */

          which =
            ((event.button.x - r_ttools.w) / (THUMB_W) + (((event.button.y - img_scroll_up->h) / THUMB_H) * 4)) + cur;

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
          if (event.button.y < img_scroll_up->h ||
              (event.button.y >=
               (button_h * buttons_tall + r_ttools.h - button_h)
               && event.button.y < (button_h * buttons_tall + r_ttools.h - img_scroll_down->h)))
          {
            /* Up or Down scroll button in Slideshow dialog: */

            if (event.button.y < img_scroll_up->h)
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
            else if (event.button.y >=
                     (button_h * buttons_tall + r_ttools.h - button_h)
                     && event.button.y < (button_h * buttons_tall + r_ttools.h - img_scroll_down->h))
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

          if (scrolltimer_dialog != TIMERID_NONE)
          {
            SDL_RemoveTimer(scrolltimer_dialog);
            scrolltimer_dialog = TIMERID_NONE;
          }

          if (!scrolling_dialog && event.type == SDL_MOUSEBUTTONDOWN)
          {
            DEBUG_PRINTF("Starting scrolling\n");
            memcpy(&scrolltimer_dialog_event, &event, sizeof(SDL_Event));
            scrolltimer_dialog_event.type = TP_SDL_MOUSEBUTTONSCROLL;

            scrolling_dialog = 1;
            scrolltimer_dialog =
              SDL_AddTimer(REPEAT_SPEED, scrolltimer_dialog_callback, (void *)&scrolltimer_dialog_event);
          }
          else
          {
            DEBUG_PRINTF("Continuing scrolling\n");
            scrolltimer_dialog =
              SDL_AddTimer(REPEAT_SPEED / 3, scrolltimer_dialog_callback, (void *)&scrolltimer_dialog_event);
          }
        }
        else if (event.button.x >= r_ttools.w
                 && event.button.x < r_ttools.w + button_w
                 && event.button.y >=
                 (button_h * buttons_tall + r_ttools.h) - button_h
                 && event.button.y < (button_h * buttons_tall + r_ttools.h))
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

          /* Instructions for Slideshow file dialog */
          draw_tux_text(TUX_BORED, TUX_TIP_SLIDESHOW, 1);

          SDL_Flip(screen);

          update_list = 1;
        }
        else if (event.button.x >= r_ttools.w + button_w
                 && event.button.x < r_ttools.w + button_w + r_ttools.w
                 && event.button.y >=
                 (button_h * buttons_tall + r_ttools.h) - button_h
                 && event.button.y < (button_h * buttons_tall + r_ttools.h))
        {
          /* Speed slider */

          int old_speed, control_sound, click_x;

          old_speed = speed;

          click_x = event.button.x - r_ttools.w - button_w;
          speed = ((10 * click_x) / r_ttools.w);

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
        else if (event.button.x >=
                 (WINDOW_WIDTH - r_ttoolopt.w - button_w - button_w)
                 && event.button.x < (WINDOW_WIDTH - r_ttoolopt.w - button_w)
                 && event.button.y >=
                 (button_h * buttons_tall + r_ttools.h) - button_h
                 && event.button.y < (button_h * buttons_tall + r_ttools.h))
        {
          /* GIF Export */

          playsound(screen, 1, SND_CLICK, 1, SNDPOS_RIGHT, SNDDIST_NEAR);

          if (num_selected < 2)
          {
            /* None selected? Too dangerous to automatically select all (like we do for slideshow playback).
               Only 1 selected?  No point in saving as GIF.
             */
            draw_tux_text(TUX_BORED, gettext_noop("Select 2 or more drawings to turn into an animated GIF."), 1);

            control_drawtext_timer(2000, instructions, 0);      /* N.B. It will draw instructions, regardless */
          }
          else
          {
            char *dest_fname;

            export_successful = export_gif(selected, num_selected, dirname, d_names, d_exts, speed, &dest_fname);

            /* Redraw entire screen, after export: */
            SDL_FillRect(screen, NULL, SDL_MapRGB(canvas->format, 255, 255, 255));
            draw_toolbar();
            draw_colors(COLORSEL_CLOBBER_WIPE);
            draw_none();

            /* Show a message depending on success */
            if (export_successful)
            {
              int n;
              char *msg;

              if (dest_fname != NULL)
              {
                n = asprintf(&msg, PROMPT_GIF_EXPORT_TXT, dest_fname);
                free(dest_fname);
              }
              else
              {
                n = asprintf(&msg, PROMPT_GIF_EXPORT_TXT, "???");
              }

              if (n != -1)
              {
                do_prompt_snd(msg, PROMPT_EXPORT_YES, "", SND_TUXOK, screen->w / 2, screen->h / 2);
                free(msg);
              }
              else
              {
                do_prompt_snd(PROMPT_GIF_EXPORT_TXT, PROMPT_EXPORT_YES, "", SND_TUXOK, screen->w / 2, screen->h / 2);
              }
            }
            else
              do_prompt_snd(PROMPT_GIF_EXPORT_FAILED_TXT, PROMPT_EXPORT_YES,
                            "", SND_YOUCANNOT, screen->w / 2, screen->h / 2);

            draw_tux_text(TUX_BORED, TUX_TIP_SLIDESHOW, 1);

            SDL_Flip(screen);

            update_list = 1;
          }
        }
        else if (event.button.x >= (WINDOW_WIDTH - r_ttoolopt.w - button_w) &&
                 event.button.x < (WINDOW_WIDTH - r_ttoolopt.w) &&
                 event.button.y >=
                 (button_h * buttons_tall + r_ttools.h) - button_h
                 && event.button.y < (button_h * buttons_tall + r_ttools.h))
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

        if (event.button.y < img_scroll_up->h &&
            event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2 &&
            event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2 && cur > 0)
        {
          /* Scroll up button: */

          do_setcursor(cursor_up);
        }
        else if (event.button.y >=
                 (button_h * buttons_tall + r_ttools.h - button_h)
                 && event.button.y <
                 (button_h * buttons_tall + r_ttools.h - img_scroll_up->h)
                 && event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2
                 && event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2 && cur < num_files - 16)
        {
          /* Scroll down button: */

          do_setcursor(cursor_down);
        }
        else
          if (((event.button.x >= r_ttools.w
                && event.button.x < r_ttools.w + button_w + r_ttools.w)
               || (event.button.x >=
                   (WINDOW_WIDTH - r_ttoolopt.w - button_w * 2)
                   && event.button.x < (WINDOW_WIDTH - r_ttoolopt.w)))
              && event.button.y >=
              (button_h * buttons_tall + r_ttools.h) - button_h
              && event.button.y < (button_h * buttons_tall + r_ttools.h))
        {
          /* One of the command buttons: */

          do_setcursor(cursor_hand);
        }
        else if (event.button.x >= r_ttools.w
                 && event.button.x < WINDOW_WIDTH - r_ttoolopt.w
                 && event.button.y > img_scroll_up->h
                 && event.button.y <
                 (button_h * buttons_tall + r_ttools.h) - button_h
                 &&
                 ((((event.button.x - r_ttools.w) / (THUMB_W) +
                    (((event.button.y - img_scroll_up->h) / THUMB_H) * 4)) + cur) < num_files))
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

      else if (event.type == SDL_MOUSEBUTTONUP)
      {
#ifdef __ANDROID__
        stop_motion_convert(event);
#endif

        if (scrolling_dialog)
        {
          if (scrolltimer_dialog != TIMERID_NONE)
          {
            SDL_RemoveTimer(scrolltimer_dialog);
            scrolltimer_dialog = TIMERID_NONE;
          }
          scrolling_dialog = 0;
          DEBUG_PRINTF("Killing dialog scrolling\n");
        }
      }

      else if (event.type == SDL_JOYAXISMOTION)
        handle_joyaxismotion(event, &motioner, &val_x, &val_y);

      else if (event.type == SDL_JOYHATMOTION)
        handle_joyhatmotion(event, oldpos_x, oldpos_y, &valhat_x, &valhat_y, &hatmotioner, &old_hat_ticks);

      else if (event.type == SDL_JOYBALLMOTION)
        handle_joyballmotion(event, oldpos_x, oldpos_y);

      else if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP)
        handle_joybuttonupdown(event, oldpos_x, oldpos_y);

      else if (event.type == SDL_USEREVENT)
      {
        if (event.user.code == USEREVENT_TEXT_UPDATE)
        {
          if (event.user.data1 != NULL)
          {
            draw_tux_text(TUX_BORED, instructions, 1);
          }
        }
      }
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

  control_drawtext_timer(0, "", 0);
  free(instructions);

  return go_back;
}


/**
 * Play an animated slideshow within Tux Paint.
 * Called by the Open->Slideshow dialog, once a set of images
 * have been selected/chosen, and "Play" is clicked.
 *
 * @param int * selected -- array of selected images, in the order they should be played
 * @param int num_selected -- count of how many images were selected
 * @char * dirname -- path to the directory of saved images to be played
 * @char ** d_names -- array of file basenames of the images to be played
 * @char ** d_ext -- array of file exentions of the images to be played
 * @int speed -- how fast to play the slideshow (0 = no automatic advance, 1 = slowest, 10 = as fast as possible)
 */
static void play_slideshow(int *selected, int num_selected, char *dirname, char **d_names, char **d_exts, int speed)
{
  int i, which, next, done;
  int val_x, val_y, motioner;
  int valhat_x, valhat_y, hatmotioner;

  SDL_Surface *img;
  char *tmp_starter_id, *tmp_template_id, *tmp_file_id;
  int tmp_starter_mirrored, tmp_starter_flipped, tmp_starter_personal;

  /* FIXME: Do we want to keep `template_personal` safe, too? */
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
  /* FIXME: Do we want to keep `template_personal` safe, too? */

  do_setcursor(cursor_tiny);

  done = 0;

  do
  {
    for (i = 0; i < num_selected && !done; i++)
    {
      which = selected[i];
      show_progress_bar(screen);


      /* Figure out filename: */

      safe_snprintf(fname, sizeof(fname), "%s/%s%s", dirname, d_names[which], d_exts[which]);


      img = myIMG_Load(fname);

      if (img != NULL)
      {
        DEBUG_PRINTF("Smearing starter @ 6 (slideshow)\n");
        autoscale_copy_smear_free(img, screen, SDL_BlitSurface);

        safe_strncpy(file_id, d_names[which], sizeof(file_id));


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
        else if (template_id[0] != '\0')
        {
          load_template(template_id);
        }
      }

      /* "Back" button: */

      dest.x = screen->w - button_w;
      dest.y = screen->h - button_h;
      SDL_BlitSurface(img_back, NULL, screen, &dest);

      dest.x = screen->w - button_w + (button_w - img_openlabels_back->w) / 2;
      dest.y = screen->h - img_openlabels_back->h;      // FIXME: CROP LABELS
      SDL_BlitSurface(img_openlabels_back, NULL, screen, &dest);

      /* "Next" button: */

      dest.x = 0;
      dest.y = screen->h - button_h;
      SDL_BlitSurface(img_play, NULL, screen, &dest);

      dest.x = (button_w - img_openlabels_next->w) / 2;
      dest.y = screen->h - img_openlabels_next->h;      // FIXME: CROP LABELS
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

            if (event.button.x >= screen->w - button_w && event.button.y >= screen->h - button_h)
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

            if ((event.button.x >= screen->w - button_w
                 || event.button.x < button_w) && event.button.y >= screen->h - button_h)
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
          handle_motioners(oldpos_x, oldpos_y, motioner, hatmotioner, old_hat_ticks, val_x, val_y, valhat_x, valhat_y);

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

  /* Restore everything about the currently-active image
     that got clobbered above */
  strcpy(starter_id, tmp_starter_id);   /* safe; originally strdup()'d from the dest. */
  free(tmp_starter_id);

  strcpy(template_id, tmp_template_id); /* safe; originally strdup()'d from the dest. */
  free(tmp_template_id);

  strcpy(file_id, tmp_file_id); /* safe; originally strdup()'d from the dest. */
  free(tmp_file_id);

  starter_mirrored = tmp_starter_mirrored;
  starter_flipped = tmp_starter_flipped;
  starter_personal = tmp_starter_personal;
}



/**
 * FIXME
 */
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


/**
 * FIXME
 */
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

#if 0
#define STIPLE_W 10
#define STIPLE_H 10
static char stiple[] =
  "8844221100"
  "8844221100" "1100884422" "1100884422" "4422110088" "4422110088" "0088442211" "0088442211" "2211008844" "2211008844";
#endif

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


static void reset_stamps(int *stamp_xored_rt, int *stamp_place_x, int *stamp_place_y, int *stamp_tool_mode)
{
  if (!active_stamp)
    return;

  *stamp_xored_rt = 0;
  *stamp_tool_mode = STAMP_TOOL_MODE_PLACE;
  int half_bigbox = sqrt((CUR_STAMP_W + 1) * (CUR_STAMP_W + 1) + (CUR_STAMP_H + 1) * (CUR_STAMP_H + 1)) / 2;

  update_screen(*stamp_place_x - half_bigbox + r_canvas.x,
                *stamp_place_y - half_bigbox + r_canvas.y,
                *stamp_place_x + half_bigbox + r_canvas.x, *stamp_place_y + half_bigbox + r_canvas.y);
  update_stamp_xor(0);
}

/**
 * FIXME
 */
static void update_stamp_xor(int stamp_angle_rotation)
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

  if (stamp_angle_rotation)
  {
    SDL_Surface *aux_surf = src;

    src = rotozoomSurface(aux_surf, stamp_angle_rotation, 1.0, SMOOTHING_ON);
    SDL_FreeSurface(aux_surf);
  }

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

/**
 * FIXME
 */
static void stamp_xor(int x, int y)
{
  int xx, yy, sx, sy;

  SDL_LockSurface(screen);
  for (yy = 0; yy < stamp_outline_h; yy++)
  {
    for (xx = 0; xx < stamp_outline_w; xx++)
    {
      if (!stamp_outline_data[xx + yy * stamp_outline_w])       /* FIXME: Conditional jump or move depends on uninitialised value(s) */
        continue;
      sx = x + xx - stamp_outline_w / 2;
      sy = y + yy - stamp_outline_h / 2;
      if (stiple[sx % STIPLE_W + sy % STIPLE_H * STIPLE_W] != '8')
        continue;

      xorpixel(sx, sy);

      if (xx < stamp_outline_w - 1)
      {
        if (stiple[(sx + 1) % STIPLE_W + sy % STIPLE_H * STIPLE_W] != '8')
        {
          xorpixel(sx + 1, sy);
        }
      }
      if (yy < stamp_outline_h - 1)
      {
        if (stiple[sx % STIPLE_W + (sy + 1) % STIPLE_H * STIPLE_W] != '8')
        {
          xorpixel(sx, sy + 1);
        }

        if (xx < stamp_outline_w - 1)
        {
          if (stiple[(sx + 1) % STIPLE_W + (sy + 1) % STIPLE_H * STIPLE_W] != '8')
          {
            xorpixel(sx + 1, sy + 1);
          }
        }
      }
    }
  }
  SDL_UnlockSurface(screen);
}

#endif

/**
 * FIXME
 */
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
      *h = 2 + (b - r) / delta; /* between cyan & yellow */
    else
      *h = 4 + (r - g) / delta; /* between magenta & cyan */

    *h = (*h * 60);             /* degrees */

    if (*h < 0)
      *h = (*h + 360);
  }
}


/**
 * FIXME
 */
static void hsvtorgb(float h, float s, float v, Uint8 *r8, Uint8 *g8, Uint8 *b8)
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

/**
 * FIXME
 */
static void print_image(void)
{
  int cur_time, scroll;

  cur_time = SDL_GetTicks() / 1000;
  scroll = (NUM_TOOLS > buttons_tall * gd_tools.cols) ? img_scroll_down->h : 0;

  DEBUG_PRINTF("Current time = %d\n", cur_time);

  if (cur_time >= last_print_time + print_delay)
  {
    if (alt_print_command_default == ALTPRINT_ALWAYS)
      want_alt_printcommand = 1;
    else if (alt_print_command_default == ALTPRINT_NEVER)
      want_alt_printcommand = 0;
    else                        /* ALTPRINT_MOD */
      want_alt_printcommand = (SDL_GetModState() & KMOD_ALT);

    if (do_prompt_image_snd(PROMPT_PRINT_NOW_TXT,
                            PROMPT_PRINT_NOW_YES,
                            PROMPT_PRINT_NOW_NO,
                            img_printer, NULL, NULL, SND_AREYOUSURE,
                            (TOOL_PRINT % 2) * button_w + button_w / 2,
                            (TOOL_PRINT / 2) * button_h + r_ttools.h +
                            button_h / 2 - tool_scroll * button_h / gd_tools.cols + scroll))
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

/**
 * FIXME
 */
void do_print(void)
{
  /* Assemble drawing plus any labels: */
  SDL_BlitSurface(canvas, NULL, save_canvas, NULL);
  SDL_BlitSurface(label, NULL, save_canvas, NULL);

#if !defined(WIN32) && !defined(__BEOS__) && !defined(__APPLE__) && !defined(__HAIKU__) && !defined(__ANDROID__)
  const char *pcmd;
  FILE *pi;

  /* Linux, Unix, etc. */

  if (want_alt_printcommand && !fullscreen)
    pcmd = altprintcommand;
  else
    pcmd = printcommand;

  DEBUG_PRINTF("printcmd: %s\n", printcommand);

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

  safe_snprintf(f, sizeof(f), "%s/%s", savedir, "print.cfg");   /* FIXME */

  {
    const char *error = SurfacePrint(window_screen, save_canvas, use_print_config ? f : NULL,
                                     show);

    if (error)
      fprintf(stderr, "%s\n", error);
  }
#elif defined(__BEOS__)
  /* BeOS */

  SurfacePrint(save_canvas);
#elif defined(__APPLE__)
  /* macOS, iOS */
  int show = (want_alt_printcommand && !fullscreen);

  const char *error = SurfacePrint(save_canvas, show);

  if (error)
  {
    fprintf(stderr, "Cannot print: %s\n", error);
    do_prompt_snd(error, PROMPT_PRINT_YES, "", SND_TUXOK, 0, 0);
  }

#elif defined(__ANDROID__)

  int x, y;
  Uint8 src_r, src_g, src_b, src_a;
  SDL_Surface *save_canvas_and = SDL_CreateRGBSurface(0,
                                                      WINDOW_WIDTH - (96 * 2),
                                                      (48 * 7) + 40 + HEIGHTOFFSET,
                                                      screen->format->BitsPerPixel,
                                                      screen->format->Rmask,
                                                      screen->format->Gmask,
                                                      screen->format->Bmask,
                                                      0);


  for (x = 0; x < save_canvas->w; x++)
    for (y = 0; y < save_canvas->h; y++)
    {
      SDL_GetRGBA(getpixels[save_canvas->format->BytesPerPixel]
                  (save_canvas, x, y), save_canvas->format, &src_r, &src_g, &src_b, &src_a);

      putpixels[save_canvas_and->format->BytesPerPixel] (save_canvas_and, x,
                                                         y,
                                                         SDL_MapRGBA
                                                         (save_canvas_and->format, src_r,
                                                          src_g, src_b, SDL_ALPHA_OPAQUE));
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

/**
 * FIXME
 */
static int do_render_cur_text(int do_blit)
{
  int w, h, txt_width;

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

  if (cursor_y > ((button_h * buttons_tall + r_ttools.h) - TuxPaint_Font_FontHeight(getfonthandle(cur_font))))
  {
    cursor_y = ((button_h * buttons_tall + r_ttools.h) - TuxPaint_Font_FontHeight(getfonthandle(cur_font)));
  }


  /* Render the text: */

  if (texttool_len > 0)
  {
#if defined(_FRIBIDI_H) || defined(FRIBIDI_H)
    //FriBidiCharType baseDir = FRIBIDI_TYPE_LTR;
#if defined (__MINGW32__) && (__GNUC__ <= 4 )
    FriBidiCharType baseDir = FRIBIDI_TYPE_WL;  /* Per: Shai Ayal <shaiay@gmail.com>, 2009-01-14 */
#else
    FriBidiParType baseDir = FRIBIDI_TYPE_WL;   //EP to avoid warning on types in now commented line above
#endif
    FriBidiChar *unicodeIn, *unicodeOut;
    unsigned int i;

    unicodeIn = (FriBidiChar *) malloc(sizeof(FriBidiChar) * (texttool_len + 1));
    unicodeOut = (FriBidiChar *) malloc(sizeof(FriBidiChar) * (texttool_len + 1));

    str = (wchar_t *)malloc(sizeof(wchar_t) * (texttool_len + 1));

    for (i = 0; i < texttool_len; i++)
      unicodeIn[i] = (FriBidiChar) texttool_str[i];

    int maxlevel = fribidi_log2vis(unicodeIn, texttool_len, &baseDir, unicodeOut, 0, 0, 0);

    maxlevel = maxlevel;        // FIXME: Avoiding "unused variable" warning.  Note: if we remove it, baseDir isn't used either! -bjk 2023.02.12

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
    if (cur_label != LABEL_SELECT && cur_label != LABEL_APPLY)
    {
      update_canvas_ex_r(old_dest.x - r_ttools.w, old_dest.y, old_dest.x + old_dest.w, old_dest.y + old_dest.h, 0);
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
    return 0;
  }


  if (!do_blit)
  {
    update_canvas_ex_r(old_dest.x - r_ttools.w, old_dest.y, old_dest.x + old_dest.w, old_dest.y + old_dest.h, 0);

    /* update_canvas_ex_r(cursor_x - 1, */
    /*            cursor_y - 1, */
    /*            cursor_x + 1 +  TuxPaint_Font_FontHeight(getfonthandle(cur_font)) * 3, */
    /*            cursor_y + 1 + TuxPaint_Font_FontHeight(getfonthandle(cur_font)), 0); */


    /* Draw outline around text: */

    dest.x = cursor_x - 2 + r_ttools.w;
    dest.y = cursor_y - 2;
    dest.w = w + 4;
    dest.h = h + 4;

    if (dest.x + dest.w > WINDOW_WIDTH - r_ttoolopt.w)
      dest.w = WINDOW_WIDTH - r_ttoolopt.w - dest.x;
    if (dest.y + dest.h > (button_h * buttons_tall + r_ttools.h))
      dest.h = (button_h * buttons_tall + r_ttools.h) - dest.y;

    SDL_FillRect(screen, &dest, SDL_MapRGB(canvas->format, 0, 0, 0));

    old_dest.x = dest.x;
    old_dest.y = dest.y;
    old_dest.w = dest.w;
    old_dest.h = dest.h;

    /* FIXME: This would be nice if it were alpha-blended: */

    dest.x = cursor_x + r_ttools.w;
    dest.y = cursor_y;
    dest.w = w;
    dest.h = h;

    if (dest.x + dest.w > WINDOW_WIDTH - r_ttoolopt.w)
      dest.w = WINDOW_WIDTH - r_ttoolopt.w - dest.x;
    if (dest.y + dest.h > (button_h * buttons_tall + r_ttools.h))
      dest.h = (button_h * buttons_tall + r_ttools.h) - dest.y;

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

    if (dest.x + src.w > WINDOW_WIDTH - r_ttoolopt.w - r_ttools.w)
      src.w = WINDOW_WIDTH - r_ttoolopt.w - r_ttools.w - dest.x;
    if (dest.y + src.h > (button_h * buttons_tall + r_ttools.h))
      src.h = (button_h * buttons_tall + r_ttools.h) - dest.y;

    if (do_blit)
    {
      if ((cur_tool == TOOL_LABEL && label_node_to_edit) ||
          ((old_tool == TOOL_LABEL && label_node_to_edit) &&
           (cur_tool == TOOL_PRINT ||
            cur_tool == TOOL_SAVE || cur_tool == TOOL_OPEN || cur_tool == TOOL_NEW || cur_tool == TOOL_QUIT)))
      {
        have_to_rec_label_node = SDL_TRUE;
        add_label_node(src.w, src.h, dest.x, dest.y, tmp_surf);
        simply_render_node(current_label_node);
      }
      else if (cur_tool == TOOL_LABEL ||
               (old_tool == TOOL_LABEL &&
                (cur_tool == TOOL_PRINT ||
                 cur_tool == TOOL_SAVE || cur_tool == TOOL_OPEN || cur_tool == TOOL_NEW || cur_tool == TOOL_QUIT)))
      {
        myblit(tmp_surf, &src, label, &dest);

        have_to_rec_label_node = SDL_TRUE;
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
      dest.x = dest.x + r_ttools.w;
      SDL_BlitSurface(tmp_surf, &src, screen, &dest);
    }
  }


  /* FIXME: Only update what's changed! */
  SDL_Flip(screen);

  //update_screen_rect(&dest);
  free(str);

  if (tmp_surf != NULL)
  {
    txt_width = tmp_surf->w;
    SDL_FreeSurface(tmp_surf);
  }
  else
    txt_width = 0;

  return txt_width;
}


/**
 * FIXME
 */
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
  wcstombs(ustr, dest, n * 2);  /* at most n * 2 bytes written */

  DEBUG_PRINTF(" ORIGINAL: %s\n" "UPPERCASE: %s\n\n", str, ustr);

  return ustr;
}

/**
 * FIXME
 */
static wchar_t *uppercase_w(const wchar_t *restrict const str)
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


/**
 * FIXME
 */
/* Scroll Timer for Tools */
static Uint32 scrolltimer_selector_callback(Uint32 interval, void *param)
{
  /* printf("scrolltimer_selector_callback(%d) -- ", interval); */
  if (scrolling_selector)
  {
    DEBUG_PRINTF("(Still scrolling selector)\n");
    SDL_PushEvent((SDL_Event *) param);
    return interval;
  }
  else
  {
    DEBUG_PRINTF("(all done scrolling selector)\n");
    return 0;
  }
}

/**
 * FIXME
 */
/* Scroll Timer for Selector */
static Uint32 scrolltimer_tool_callback(Uint32 interval, void *param)
{
  /* printf("scrolltimer_tool_callback(%d)\n", interval); */
  if (scrolling_tool)
  {
    DEBUG_PRINTF("(Still scrolling tool)\n");
    SDL_PushEvent((SDL_Event *) param);
    return interval;
  }
  else
  {
    DEBUG_PRINTF("(all done scrolling tool)\n");
    return 0;
  }
}


/**
 * FIXME
 */
/* Scroll Timer for Dialogs (Open & New) */
static Uint32 scrolltimer_dialog_callback(Uint32 interval, void *param)
{
  if (scrolling_dialog)
  {
    DEBUG_PRINTF("(Still scrolling dialog)\n");
    SDL_PushEvent((SDL_Event *) param);
    return interval;
  }
  else
  {
    DEBUG_PRINTF("(all done scrolling dialog)\n");
    return 0;
  }
}


/**
 * FIXME
 */
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


/**
 * FIXME
 */
/* Drawtext Timer */
static Uint32 drawtext_callback(Uint32 interval, void *param)
{
  (void)interval;
  SDL_PushEvent((SDL_Event *) param);

  return 0;                     /* Remove timer */
}


#ifdef DEBUG
/**
 * FIXME
 */
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


/**
 * FIXME
 */
static const char *great_str(void)
{
  return (great_strs[rand() % (sizeof(great_strs) / sizeof(char *))]);
}


#ifdef DEBUG
/**
 * FIXME
 */
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

/**
 * FIXME
 */
static void draw_image_title(int t, SDL_Rect dest)
{
  SDL_BlitSurface(img_title_on, NULL, screen, &dest);

  dest.x += (dest.w - img_title_names[t]->w) / 2;
  dest.y += (dest.h - img_title_names[t]->h) / 2;
  SDL_BlitSurface(img_title_names[t], NULL, screen, &dest);
}


/**
 * FIXME
 */
/* Handle keyboard events to control the mouse: */
/* Move as many pixels as bigsteps outside the areas,
   in the areas and 5 pixels around, move 1 pixel at a time */
static void handle_keymouse(SDLKey key, Uint32 updown, int steps, SDL_Rect *area1, SDL_Rect *area2)
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
      if ((area1 && oldpos_x > r1.x && oldpos_x - r1.x < r1.w
           && oldpos_y > r1.y && oldpos_y - r1.y < r1.h) || (area2
                                                             && oldpos_x >
                                                             r2.x
                                                             && oldpos_x -
                                                             r2.x < r2.w && oldpos_y > r2.y && oldpos_y - r2.y < r2.h))
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

/**
 * FIXME
 */
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
                  real_r_tools.y - *whicht % 2 * button_w / 2 +
                  *whicht * button_h / 2 + 10 - tool_scroll * button_h / 2);
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
                    real_r_tools.y - *whicht % 2 * button_w / 2 +
                    *whicht * button_h / 2 + 10 - tool_scroll * button_h / 2);

    /* Play a sound here as there is a big jump */
    playsound(screen, 1, SND_CLICK, 0, SNDPOS_LEFT, SNDDIST_NEAR);
  }
}

/**
 * FIXME
 */
/* Unblank screen in fullscreen mode, if needed: */
static void handle_active(SDL_Event *event)
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
    SetActivationState(1);
#endif
  }
}


/**
 * FIXME
 */
static SDL_Surface *duplicate_surface(SDL_Surface *orig)
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

/**
 * FIXME
 */
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


/**
 * FIXME
 */
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


/**
 * Returns whether a button click is valid, based on the
 * "no button distinction" setting.
 *
 * @param Uint8 button -- the button clicked (we check for 1, 2, or 3)
 * @return int -- 1/true if the button is recognized, else 0/false
 *   (2 & 3 are only valid if "no button distiction" is enabled)
 */
static int valid_click(Uint8 button)
{
  if (button == 1 || ((button == 2 || button == 3) && no_button_distinction))
    return (1);
  else
    return (0);
}


/**
 * Returns whether a point is within a circle.
 *
 * @param int x -- x position relative to center of circle
 * @param int y -- y position [ditto]
 * @param int rad -- radius of circle
 * @return int -- 1/true if (x,y) is within the circle, else 0/false
 */
static int in_circle_rad(int x, int y, int rad)
{
  if (abs(x) > rad || abs(y) > rad)     // short circuit to avoid unnecessary math
    return (0);
  if ((x * x) + (y * y) - (rad * rad) < 0)
    return (1);
  else
    return (0);
}


/**
 * FIXME
 */
static int paintsound(int size)
{
  if (SND_PAINT1 + (size / 12) >= SND_PAINT4)
    return (SND_PAINT4);
  else
    return (SND_PAINT1 + (size / 12));
}


#ifndef NOSVG

#ifdef OLD_SVG

/**
 * FIXME
 */
/* Old libcairo1, svg and svg-cairo based code
   Based on cairo-demo/sdl/main.c from Cairo (GPL'd, (c) 2004 Eric Windisch):
*/
static SDL_Surface *load_svg(const char *file)
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


  DEBUG_PRINTF("Attempting to load \"%s\" as an SVG\n", file);

  /* Create the SVG cairo stuff: */
  if (svg_cairo_create(&scr) != SVG_CAIRO_STATUS_SUCCESS)
  {
    fprintf(stderr, "svg_cairo_create(%s) failed on\n", file);
    return (NULL);
  }

  /* Parse the SVG file: */
  if (svg_cairo_parse(scr, file) != SVG_CAIRO_STATUS_SUCCESS)
  {
    svg_cairo_destroy(scr);
    fprintf(stderr, "svg_cairo_parse(%s) failed\n", file);
    return (NULL);
  }

  /* Get the natural size of the SVG */
  svg_cairo_get_size(scr, &rwidth, &rheight);

  DEBUG_PRINTF("svg_get_size(): %d x %d\n", rwidth, rheight);

  if (rwidth == 0 || rheight == 0)
  {
    svg_cairo_destroy(scr);
    fprintf(stderr, "SVG %s had 0 width or height!\n", file);
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

  DEBUG_PRINTF("scaling to %d x %d (%f scale)\n", width, height, scale);

  /* scanline width */
  stride = width * btpp;

  /* Allocate space for an image: */
  image = calloc(stride * height, 1);

  DEBUG_PRINTF
    ("calling cairo_image_surface_create_for_data(..., CAIRO_FORMAT_ARGB32, %d(w), %d(h), %d(stride))\n",
     width, height, stride);

  /* Create the cairo surface with the adjusted width and height */

  cairo_surface = cairo_image_surface_create_for_data(image, CAIRO_FORMAT_ARGB32, width, height, stride);
  cr = cairo_create(cairo_surface);
  if (cr == NULL)
  {
    svg_cairo_destroy(scr);
    fprintf(stderr, "cairo_create(%s) failed\n", file);
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
    fprintf(stderr, "svg_cairo_render(%s) failed\n");
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
    fprintf(stderr, "SDL_CreateRGBSurfaceFrom(%s) failed\n", file);
    return (NULL);
  }


  /* Convert the SDL surface to the display format, for faster blitting: */
  sdl_surface = SDL_DisplayFormatAlpha(sdl_surface_tmp);
  SDL_FreeSurface(sdl_surface_tmp);

  if (sdl_surface == NULL)
  {
    fprintf(stderr, "SDL_DisplayFormatAlpha(%s) failed\n", file);
    return (NULL);
  }

  DEBUG_PRINTF("SDL surface from %d x %d SVG is %d x %d\n", rwidth, rheight, sdl_surface->w, sdl_surface->h);

  return (sdl_surface);
}

#else /* #ifdef OLD_SVG */

/**
 * FIXME
 */
/* New libcairo2, rsvg and rsvg-cairo based code */
static SDL_Surface *_load_svg(const char *file)
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

#if !(LIBRSVG_MAJOR_VERSION < 2 || LIBRSVG_MINOR_VERSION < 46)
  RsvgRectangle viewport;
#endif
  SDL_Surface *sdl_surface, *sdl_surface_tmp;
  Uint32 rmask, gmask, bmask, amask;

  DEBUG_PRINTF("load_svg(%s)\n", file);

  /* Create an RSVG Handle from the SVG file: */

  gerr = NULL;

  rsvg_handle = rsvg_handle_new_from_file(file, &gerr);
  if (rsvg_handle == NULL)
  {
    fprintf(stderr, "rsvg_handle_new_from_file(%s) failed: %s\n", file, gerr->message);
    free(gerr);
    return (NULL);
  }
  free(gerr);

/* rsvg_handle_get_dimensions() is deprecated since since version 2.52,
   but we currently support some platforms where it's not yet available
   (e.g., Rocky Linux 9) */
#if LIBRSVG_MAJOR_VERSION < 2 || LIBRSVG_MINOR_VERSION < 52
  {
    RsvgDimensionData dim;

    rsvg_handle_get_dimensions(rsvg_handle, &dim);
    rwidth = dim.width;
    rheight = dim.height;

    DEBUG_PRINTF("SVG is %d x %d\n", rwidth, rheight);
  }
#else
  {
    gdouble d_rwidth, d_rheight;

    rsvg_handle_get_intrinsic_size_in_pixels(rsvg_handle, &d_rwidth, &d_rheight);
    rwidth = (int)d_rwidth;
    rheight = (int)d_rheight;

    DEBUG_PRINTF("SVG is %f x %f (%d x %d)\n", d_rwidth, d_rheight, rwidth, rheight);
  }
#endif



  /* Pick best scale to render to (for the canvas in this instance of Tux Paint) */

  scale = pick_best_scape(rwidth, rheight, r_canvas.w, r_canvas.h);

  DEBUG_PRINTF("best scale is %.4f\n", scale);

  width = ((float)rwidth * scale);
  height = ((float)rheight * scale);

  DEBUG_PRINTF("scaling to %d x %d (%f scale)\n", width, height, scale);

  /* scanline width */
  stride = width * btpp;

  /* Allocate space for an image: */
  image = calloc(stride * height, 1);
  if (image == NULL)
  {
    fprintf(stderr, "Unable to allocate image buffer for %s\n", file);
    // rsvg_handle_close(rsvg_handle, &gerr); Not needed
    return (NULL);
  }


  /* Create a surface for Cairo to draw into: */

  cairo_surf = cairo_image_surface_create_for_data(image, CAIRO_FORMAT_ARGB32, width, height, stride);

  if (cairo_surface_status(cairo_surf) != CAIRO_STATUS_SUCCESS)
  {
#ifdef DEBUG
    fprintf(stderr, "cairo_image_surface_create() failed\n");
#endif
    // rsvg_handle_close(rsvg_handle, &gerr); Not needed
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
    // rsvg_handle_close(rsvg_handle, &gerr); Not needed
    cairo_surface_destroy(cairo_surf);
    free(image);
    return (NULL);
  }


  /* Ask RSVG to render the SVG into the Cairo object: */

#if LIBRSVG_MAJOR_VERSION < 2 || LIBRSVG_MINOR_VERSION < 46
  cairo_scale(cr, scale, scale);

  rsvg_handle_render_cairo(rsvg_handle, cr);
#else
  // N.B. We do NOT call cairo_scale() in this case, since
  // we're setting a viewport to render into; else we'd end
  // up scaling twice, resulting in a too-large, and badly-cropped
  // stamp -bjk 2023.07.08

  viewport.x = 0;
  viewport.y = 0;
  viewport.width = width;
  viewport.height = height;

  /* FIXME: This returns a gboolean; not using (not 100% sure what to expect) -bjk 2023.06.18 */
  rsvg_handle_render_document(rsvg_handle, cr, &viewport, &gerr);
  /* FIXME: ignoring errors (gerr) for now -bjk 2023.06.18 */
#endif

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
    // rsvg_handle_close(rsvg_handle, &gerr); Not needed
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
    // rsvg_handle_close(rsvg_handle, &gerr); Not needed
    cairo_surface_destroy(cairo_surf);
    free(image);
    cairo_destroy(cr);
    return (NULL);
  }


  DEBUG_PRINTF("SDL surface from %d x %d SVG is %d x %d\n", rwidth, rheight, sdl_surface->w, sdl_surface->h);


  /* Clean up: */

  // rsvg_handle_close(rsvg_handle, &gerr); Not needed
  cairo_surface_destroy(cairo_surf);
  free(image);
  cairo_destroy(cr);

  return (sdl_surface);
}

/* Wraper to fallback to SDL2_Image's nanosvg in case rsvg fails for some reason
   like in Android builds trying to access starters provided as assets. */
static SDL_Surface *load_svg(const char *file)
{
  SDL_Surface *sdl_surface;

  sdl_surface = _load_svg(file);
  if (sdl_surface == NULL)
    sdl_surface = IMG_Load(file);
  return (sdl_surface);
}

#endif /* #ifdef OLD_SVG */

#endif /* #ifndef NOSVG */


/**
 * FIXME
 */
static float pick_best_scape(unsigned int orig_w, unsigned int orig_h, unsigned int max_w, unsigned int max_h)
{
  float aspect, scale, wscale, hscale;

  aspect = (float)orig_w / (float)orig_h;

  DEBUG_PRINTF("trying to fit %d x %d (aspect: %.4f) into %d x %d\n", orig_w, orig_h, aspect, max_w, max_h);

  wscale = (float)max_w / (float)orig_w;
  hscale = (float)max_h / (float)orig_h;

  DEBUG_PRINTF("max_w / orig_w = wscale: %.4f\n", wscale);
  DEBUG_PRINTF("max_h / orig_h = hscale: %.4f\n", hscale);
  DEBUG_PRINTF("\n");

  if (aspect >= 1)
  {
    /* Image is wider-than-tall (or square) */

    scale = wscale;

    DEBUG_PRINTF("Wider-than-tall.  Using wscale.\n");
    DEBUG_PRINTF("new size would be: %d x %d\n", (int)((float)orig_w * scale), (int)((float)orig_h * scale));

    if ((float)orig_h * scale > (float)max_h)
    {
      scale = hscale;

      DEBUG_PRINTF("Too tall!  Using hscale!\n");
      DEBUG_PRINTF("new size would be: %d x %d\n", (int)((float)orig_w * scale), (int)((float)orig_h * scale));
    }
  }
  else
  {
    /* Taller-than-wide */

    scale = hscale;

    DEBUG_PRINTF("Taller-than-wide.  Using hscale.\n");
    DEBUG_PRINTF("new size would be: %d x %d\n", (int)((float)orig_w * scale), (int)((float)orig_h * scale));

    if ((float)orig_w * scale > (float)max_w)
    {
      scale = wscale;

      DEBUG_PRINTF("Too wide!  Using wscale!\n");
      DEBUG_PRINTF("new size would be: %d x %d\n", (int)((float)orig_w * scale), (int)((float)orig_h * scale));
    }
  }

  DEBUG_PRINTF("\n");
  DEBUG_PRINTF("Final scale: %.4f\n", scale);

  return (scale);
}

/**
 * FIXME
 */
/* FIXME: we can remove this after SDL folks fix their bug at http://bugzilla.libsdl.org/show_bug.cgi?id=1485 */
/* Try to load an image with IMG_Load(), if it fails, then try with RWops() */
static SDL_Surface *myIMG_Load_RWops(const char *file)
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

/**
 * FIXME
 */
/* Load an image; call load_svg() (above, to call Cairo and SVG-Cairo funcs)
   if we notice it's an SVG file (if available!);
   call load_kpx() if we notice it's a KPX file (JPEG with wrapper);
   otherwise call SDL_Image lib's IMG_Load() (for PNGs, JPEGs, BMPs, etc.) */
static SDL_Surface *myIMG_Load(const char *file)
{
#ifndef __ANDROID__
  struct stat stat_buf;

  if (stat(file, &stat_buf) != 0)
  {
    /* File by that name doesn't exist; give up now */
    return NULL;
  }
#endif

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

/**
 * FIXME
 */
static SDL_Surface *load_kpx(const char *file)
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


/**
 * FIXME
 */
static void load_magic_plugins(void)
{
  int res, n, i, plc, tries;
  char *place;
  int err;
  DIR *d;
  struct dirent *f;
  char fname[512];
  char objname[512];
  char funcname[512];

  num_plugin_files = 0;
  for (i = 0; i < MAX_MAGIC_GROUPS; i++)
    num_magics[i] = 0;
  num_magics_total = 0;

  for (plc = 0; plc < NUM_MAGIC_PLACES; plc++)
  {
    if (plc == MAGIC_PLACE_GLOBAL)
    {
#if defined (__ANDROID__)
      /* Need this at runtime as Android installs on different locations depending on the user */
      place = strdup(get_nativelibdir());
#else
      place = strdup(MAGIC_PREFIX);
#endif
    }
    else if (plc == MAGIC_PLACE_LOCAL)
      place = get_fname("plugins/", DIR_DATA);
#ifdef __APPLE__
    else if (plc == MAGIC_PLACE_ALLUSERS)
      place = strdup("/Library/Application Support/TuxPaint/plugins/");
#endif
    else
      continue;                 /* Huh? */

    DEBUG_PRINTF("\n");
    DEBUG_PRINTF("Loading magic plug-ins from %s\n", place);

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
      magic_api_struct->xorpixel = magic_xorpixel;
      magic_api_struct->line = magic_line_func;
      magic_api_struct->playsound = magic_playsound;
      magic_api_struct->playingsound = magic_playingsound;
      magic_api_struct->pausesound = magic_pausesound;
      magic_api_struct->unpausesound = magic_unpausesound;
      magic_api_struct->stopsound = magic_stopsound;
      magic_api_struct->special_notify = special_notify;
      magic_api_struct->button_down = magic_button_down;
      magic_api_struct->rgbtohsv = rgbtohsv;
      magic_api_struct->hsvtorgb = hsvtorgb;
      magic_api_struct->canvas_w = canvas->w;
      magic_api_struct->canvas_h = canvas->h;
      magic_api_struct->scale = magic_scale;
      magic_api_struct->rotate_scale = magic_rotate_scale;
      magic_api_struct->touched = magic_touched;
      magic_api_struct->retract_undo = magic_retract_undo;

      do
      {
        f = readdir(d);

        if (f != NULL)
        {
          struct stat sbuf;

          safe_snprintf(fname, sizeof(fname), "%s%s", place, f->d_name);
          if (!stat(fname, &sbuf) && S_ISREG(sbuf.st_mode))
          {
            /* Get just the name of the object (e.g., "negative"), w/o filename
               extension: */

            safe_strncpy(objname, f->d_name, sizeof(objname));
            strcpy(strchr(objname, '.'), "");   /* safe; truncating */

#if defined(__ANDROID__)
            // since Android compiles magic tools with name like "libxxx.so", here we shall exclude the prefix "lib".
            strcpy(objname, objname + 3);
#endif

            magic_handle[num_plugin_files] = SDL_LoadObject(fname);

            if (magic_handle[num_plugin_files] != NULL)
            {
              DEBUG_PRINTF("loading: %s\n", fname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "get_tool_count");
              magic_funcs[num_plugin_files].get_tool_count = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "get_group");
              magic_funcs[num_plugin_files].get_group = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "get_order");
              magic_funcs[num_plugin_files].get_order = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "get_name");
              magic_funcs[num_plugin_files].get_name = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "get_icon");
              magic_funcs[num_plugin_files].get_icon = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "get_description");
              magic_funcs[num_plugin_files].get_description =
                SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "requires_colors");
              magic_funcs[num_plugin_files].requires_colors =
                SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "accepted_sizes");
              magic_funcs[num_plugin_files].accepted_sizes = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "default_size");
              magic_funcs[num_plugin_files].default_size = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "modes");
              magic_funcs[num_plugin_files].modes = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "set_color");
              magic_funcs[num_plugin_files].set_color = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "set_size");
              magic_funcs[num_plugin_files].set_size = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "init");
              magic_funcs[num_plugin_files].init = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "api_version");
              magic_funcs[num_plugin_files].api_version = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "shutdown");
              magic_funcs[num_plugin_files].shutdown = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "click");
              magic_funcs[num_plugin_files].click = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "drag");
              magic_funcs[num_plugin_files].drag = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "release");
              magic_funcs[num_plugin_files].release = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "switchin");
              magic_funcs[num_plugin_files].switchin = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              safe_snprintf(funcname, sizeof(funcname), "%s_%s", objname, "switchout");
              magic_funcs[num_plugin_files].switchout = SDL_LoadFunction(magic_handle[num_plugin_files], funcname);

              //EP added (intptr_t) to avoid warning on x64 on all lines below
              DEBUG_PRINTF("get_tool_count = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].get_tool_count);
              DEBUG_PRINTF("get_group = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].get_group);
              DEBUG_PRINTF("get_order = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].get_order);
              DEBUG_PRINTF("get_name = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].get_name);
              DEBUG_PRINTF("get_icon = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].get_icon);
              DEBUG_PRINTF("get_description = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].get_description);
              DEBUG_PRINTF("requires_colors = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].requires_colors);
              DEBUG_PRINTF("accepted_sizes = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].accepted_sizes);
              DEBUG_PRINTF("default_size = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].default_size);
              DEBUG_PRINTF("modes = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].modes);
              DEBUG_PRINTF("set_color = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].set_color);
              DEBUG_PRINTF("set_size = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].set_size);
              DEBUG_PRINTF("init = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].init);
              DEBUG_PRINTF("api_version = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].api_version);
              DEBUG_PRINTF("shutdown = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].shutdown);
              DEBUG_PRINTF("click = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].click);
              DEBUG_PRINTF("drag = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].drag);
              DEBUG_PRINTF("release = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].release);
              DEBUG_PRINTF("switchin = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].switchin);
              DEBUG_PRINTF("switchout = 0x%x\n", (int)(intptr_t) magic_funcs[num_plugin_files].switchout);

              err = 0;

              if (magic_funcs[num_plugin_files].get_tool_count == NULL)
              {
                fprintf(stderr, "Error: plugin %s is missing get_tool_count\n", fname);
                err = 1;
              }
              if (magic_funcs[num_plugin_files].get_group == NULL)
              {
                fprintf(stderr, "Error: plugin %s is missing get_group\n", fname);
                err = 1;
              }
              if (magic_funcs[num_plugin_files].get_order == NULL)
              {
                fprintf(stderr, "Error: plugin %s is missing get_order\n", fname);
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
              if (magic_funcs[num_plugin_files].accepted_sizes == NULL)
              {
                fprintf(stderr, "Error: plugin %s is missing accepted_sizes\n", fname);
                err = 1;
              }
              if (magic_funcs[num_plugin_files].default_size == NULL)
              {
                fprintf(stderr, "Error: plugin %s is missing default_size\n", fname);
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
              if (magic_funcs[num_plugin_files].set_size == NULL)
              {
                fprintf(stderr, "Error: plugin %s is missing set_size\n", fname);
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
                res =
                  magic_funcs[num_plugin_files].init(magic_api_struct, magic_disabled_features, magic_complexity_level);

                if (res != 0)
                  n = magic_funcs[num_plugin_files].get_tool_count(magic_api_struct);
                else
                {
                  magic_funcs[num_plugin_files].shutdown(magic_api_struct);
                  n = 0;
                }


                if (n == 0)
                {
                  fprintf(stderr,
                          "Notice: plugin %1$s failed to startup or reported 0 magic tools (Tux Paint is in complexity mode \"%2$s\")\n",
                          fname, MAGIC_COMPLEXITY_LEVEL_NAMES[magic_complexity_level]);
                  fflush(stderr);
                  SDL_UnloadObject(magic_handle[num_plugin_files]);
                }
                else
                {
                  int j, group, idx, want_group, want_order;
                  SDL_Surface *icon_tmp;

                  for (i = 0; i < n; i++)
                  {
                    want_group = magic_funcs[num_plugin_files].get_group(magic_api_struct, i);
                    if (!no_magic_groups)
                      group = want_group;
                    else
                      group = 0;

                    if (group < MAX_MAGIC_GROUPS)
                    {
                      idx = num_magics[group];

                      magics[group][idx].idx = i;
                      magics[group][idx].place = plc;
                      magics[group][idx].handle_idx = num_plugin_files;
                      magics[group][idx].group = group;
                      magics[group][idx].name = magic_funcs[num_plugin_files].get_name(magic_api_struct, i);
                      want_order = magic_funcs[num_plugin_files].get_order(i);
                      if (!no_magic_groups)
                        magics[group][idx].order = want_order;
                      else
                        magics[group][idx].order = (want_group * 1000000) + want_order;

                      for (j = 0; j < num_magics[group]; j++)
                      {
                        if (magics[group][j].order == magics[group][idx].order)
                        {
                          fprintf(stderr,
                                  "Warning: In group %d, tool %d (%s) has the same order (%d) as tool %d (%s)\n",
                                  group, idx, magics[group][idx].name,
                                  magics[group][j].order, j, magics[group][j].name);
                        }
                      }

                      magics[group][idx].avail_modes = magic_funcs[num_plugin_files].modes(magic_api_struct, i);

                      for (j = 0; j < MAX_MODES; j++)
                      {
                        magics[group][idx].tip[j] = NULL;
                        if (j)
                        {
                          if (magics[group][idx].avail_modes & MODE_FULLSCREEN)
                            magics[group][idx].tip[j] =
                              magic_funcs[num_plugin_files].get_description(magic_api_struct, i, MODE_FULLSCREEN);
                        }
                        else
                        {
                          if (magics[group][idx].avail_modes & MODE_PAINT)
                            magics[group][idx].tip[j] =
                              magic_funcs[num_plugin_files].get_description(magic_api_struct, i, MODE_PAINT);
                          else if (magics[group][idx].avail_modes & MODE_ONECLICK)
                            magics[group][idx].tip[j] =
                              magic_funcs[num_plugin_files].get_description(magic_api_struct, i, MODE_ONECLICK);
                          else if (magics[group][idx].avail_modes & MODE_PAINT_WITH_PREVIEW)
                            magics[group][idx].tip[j] =
                              magic_funcs[num_plugin_files].get_description(magic_api_struct, i,
                                                                            MODE_PAINT_WITH_PREVIEW);
                        }
                      }

                      magics[group][idx].colors = magic_funcs[num_plugin_files].requires_colors(magic_api_struct, i);

                      for (j = 0; j < MAX_MODES; j++)
                      {
                        int mode_bit;

                        mode_bit = 0;

                        if (j == 1 && magics[group][idx].avail_modes & MODE_FULLSCREEN)
                        {
                          mode_bit = MODE_FULLSCREEN;
                        }
                        else
                        {
                          if (magics[group][idx].avail_modes & MODE_PAINT)
                          {
                            mode_bit = MODE_PAINT;
                          }
                          else if (magics[group][idx].avail_modes & MODE_ONECLICK)
                          {
                            mode_bit = MODE_ONECLICK;
                          }
                          else if (magics[group][idx].avail_modes & MODE_PAINT_WITH_PREVIEW)
                          {
                            mode_bit = MODE_PAINT_WITH_PREVIEW;
                          }
                        }

                        if (mode_bit != 0)
                        {
                          magics[group][idx].sizes[j] =
                            magic_funcs[num_plugin_files].accepted_sizes(magic_api_struct, i, mode_bit);
                          if (magics[group][idx].sizes[j] > 1)
                          {
                            magics[group][idx].default_size[j] =
                              magic_funcs[num_plugin_files].default_size(magic_api_struct, i, mode_bit);
                            if (magics[group][idx].default_size[j] < 1
                                || magics[group][idx].default_size[j] > magics[group][idx].sizes[j])
                            {
                              fprintf(stderr,
                                      "Warning: plugin %s tool # %d for %d mode (%x) default size (%d) out of range (1-%d)\n",
                                      fname, i, j, mode_bit,
                                      magics[group][idx].default_size[j], magics[group][idx].sizes[j]);
                              magics[group][idx].default_size[j] = 1;
                            }
                            magics[group][idx].size[j] = magics[group][idx].default_size[j];
                          }
                        }
                      }

                      if (magics[group][idx].avail_modes & MODE_PAINT)
                        magics[group][idx].mode = MODE_PAINT;
                      else if (magics[group][idx].avail_modes & MODE_ONECLICK)
                        magics[group][idx].mode = MODE_ONECLICK;
                      else if (magics[group][idx].avail_modes & MODE_PAINT_WITH_PREVIEW)
                        magics[group][idx].mode = MODE_PAINT_WITH_PREVIEW;
                      else
                        magics[group][idx].mode = MODE_FULLSCREEN;

                      icon_tmp = magic_funcs[num_plugin_files].get_icon(magic_api_struct, i);
                      if (icon_tmp != NULL)
                      {
                        magics[group][idx].img_icon =
                          thumbnail(icon_tmp,
                                    40 * button_w / ORIGINAL_BUTTON_SIZE, 30 * button_h / ORIGINAL_BUTTON_SIZE, 1);
                        SDL_FreeSurface(icon_tmp);

                        DEBUG_PRINTF("-- %s\n", magics[group][idx].name);
                        DEBUG_PRINTF("avail_modes = %d\n", magics[group][idx].avail_modes);

                        num_magics[group]++;
                        num_magics_total++;

                        if (num_magics[group] >= MAX_MAGICS_PER_GROUP)
                        {
                          fprintf(stderr,
                                  "Error: exceeded maximum number of Magic tools (%d) in group %d!\n",
                                  MAX_MAGICS_PER_GROUP, group);
                          num_magics[group]--;  // FIXME: Do something better than just this! -bjk 2024.04.08
                        }
                      }
                      else
                      {
                        fprintf(stderr, "Error: plugin %s mode # %d failed to load an icon\n", fname, i);
                        fflush(stderr);
                      }
                    }
                    else
                    {
                      fprintf(stderr,
                              "Error: plugin %s mode # %d reported group %d (higher than %d)\n",
                              fname, i, group, MAX_MAGIC_GROUPS - 1);
                      fflush(stderr);
                    }
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


  for (i = 0; i < MAX_MAGIC_GROUPS; i++)
  {
    qsort(magics[i], num_magics[i], sizeof(magic_t), magic_sort);
  }

  DEBUG_PRINTF("Loaded %d magic tools from %d plug-in files\n", num_magics_total, num_plugin_files);
  DEBUG_PRINTF("\n");

  /* Start out with the first magic group that _has_ any tools */
  tries = 0;
  while (num_magics[magic_group] == 0 && tries < MAX_MAGIC_GROUPS)
  {
    magic_group++;
    if (magic_group >= MAX_MAGIC_GROUPS)
    {
      magic_group = 0;
    }
  }
}



/**
 * FIXME
 */
static int magic_sort(const void *a, const void *b)
{
  magic_t *am = (magic_t *) a;
  magic_t *bm = (magic_t *) b;

  if (am->order != bm->order)
  {
    /* Different 'order's, use them */
    return (am->order - bm->order);
  }
  else
  {
    /* Same 'order', use the (localized) name */
    return (strcoll(gettext(am->name), gettext(bm->name)));
  }
}


/**
 * FIXME
 */
static void update_progress_bar(void)
{
  show_progress_bar(screen);
}

/**
 * FIXME
 */
static void magic_line_func(void *mapi,
                            int which, SDL_Surface *canvas,
                            SDL_Surface *last, int x1, int y1, int x2,
                            int y2, int step, void (*cb)(void *, int, SDL_Surface *, SDL_Surface *, int, int))
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


/**
 * FIXME
 */
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

/**
 * FIXME
 */
static void magic_stopsound(void)
{
#ifndef NOSOUND
  if (mute || !use_sound)
    return;

  Mix_HaltChannel(0);
#endif
}

/**
 * FIXME
 */
static void magic_playsound(Mix_Chunk *snd, int left_right, int up_down)
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

  if (use_stereo)
  {
    left = ((255 - dist) * (255 - left_right)) / 255;
  }
  else
  {
    /* Stereo disabled; no panning (see playsound.c) */
    left = (255 - dist) / 2;
  }

  Mix_SetPanning(0, left, (255 - dist) - left);
#endif
}

static int magic_playingsound(void)
{
#ifndef NOSOUND
  int is_playing;

  is_playing = Mix_Playing(0);
  return is_playing;
#endif
}

static void magic_pausesound(void)
{
#ifndef NOSOUND
  return Mix_Pause(0);
#endif
}

static void magic_unpausesound(void)
{
#ifndef NOSOUND
  return Mix_Resume(0);
#endif
}

/**
 * FIXME
 */
static Uint8 magic_linear_to_sRGB(float lin)
{
  return (linear_to_sRGB(lin));
}

/**
 * FIXME
 */
static float magic_sRGB_to_linear(Uint8 srgb)
{
  return (sRGB_to_linear_table[srgb]);
}

/**
 * FIXME
 */
static int magic_button_down(void)
{
  return (button_down || emulate_button_pressed);
}

/**
 * FIXME
 */
static SDL_Surface *magic_scale(SDL_Surface *surf, int w, int h, int aspect)
{
  return (thumbnail2(surf, w, h, aspect, 1));
}

/**
 * FIXME
 */
static SDL_Surface *magic_rotate_scale(SDL_Surface *surf, int r, int w)
{
  return (rotozoomSurface(surf, r, (float)w / surf->w, SMOOTHING_ON));
}

/**
 * FIXME
 */
/* FIXME: This, do_open() and do_slideshow() should be combined and modularized! */
static int do_new_dialog(void)
{
  SDL_Surface *img, *img1, *img2;
  int things_alloced;
  SDL_Surface **thumbs = NULL;
  DIR *d;
  struct dirent *f;
  struct dirent2 *fs;
  struct stat sbuf;
  int place;
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
  int last_click_which, last_click_button, which_changed, erasable;
  int places_to_look;
  int tot;
  int first_color, first_starter, first_template;
  int white_in_palette;
  int val_x, val_y, motioner;
  int valhat_x, valhat_y, hatmotioner;
  int skip;

#ifndef NOSVG
  int k;
#endif


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
      continue;                 /* ugh */
    }
    else if (places_to_look == PLACE_PERSONAL_STARTERS_DIR)
    {
      /* Check for coloring-book style 'starter' images in our folder: */

      dirname[places_to_look] = get_fname("starters", DIR_DATA);
    }
    else if (places_to_look == PLACE_STARTERS_DIR)
    {
      /* Check for system-wide coloring-book style
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
          safe_snprintf(fname, sizeof(fname), "%s/%s", dirname[places_to_look], f->d_name);
          if (!stat(fname, &sbuf) && S_ISREG(sbuf.st_mode))
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
  tot = num_files_in_dirs;

  /* And colors... */
  tot += NUM_COLORS;

  thumbs = (SDL_Surface * *)malloc(sizeof(SDL_Surface *) * tot);
  d_places = (int *)malloc(sizeof(int) * tot);
  d_names = (char **)malloc(sizeof(char *) * tot);
  d_exts = (char **)malloc(sizeof(char *) * tot);


  /* Sort: */

  /* (N.B. "New" dialog not affected by 'reversesort' option) */
  qsort(fs, num_files_in_dirs, sizeof(struct dirent2), (int (*)(const void *, const void *))compare_dirent2s);


  /* Throw the color palette at the beginning (default): */

  white_in_palette = -1;

  if (!new_colors_last)
  {
    first_color = 0;
    num_files = do_new_dialog_add_colors(thumbs, num_files, d_places, d_names, d_exts, &white_in_palette);
  }

  first_starter = num_files;
  first_template = -1;          /* In case there are none... */


  /* Read directory of images and build thumbnails: */

  for (j = 0; j < num_files_in_dirs; j++)
  {
    f = &(fs[j].f);
    place = fs[j].place;

    if ((place == PLACE_PERSONAL_TEMPLATES_DIR || place == PLACE_TEMPLATES_DIR) && first_template == -1)
      first_template = num_files;

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
          safe_strncpy(fname, f->d_name, sizeof(fname));
          skip = 0;

          if (strcasestr(fname, FNAME_EXTENSION) != NULL)
          {
            d_exts[num_files] = strdup(strcasestr(fname, FNAME_EXTENSION));
            strcpy((char *)strcasestr(fname, FNAME_EXTENSION), "");     /* safe; truncating */
          }

          if (strcasestr(fname, ".bmp") != NULL)
          {
            d_exts[num_files] = strdup(strcasestr(fname, ".bmp"));
            strcpy((char *)strcasestr(fname, ".bmp"), "");      /* safe; truncating */
          }

#ifndef NOSVG
          if (strcasestr(fname, ".svg") != NULL)
          {
            d_exts[num_files] = strdup(strcasestr(fname, ".svg"));
            strcpy((char *)strcasestr(fname, ".svg"), "");      /* safe; truncating */
          }
#endif

          if (strcasestr(fname, ".kpx") != NULL)
          {
            d_exts[num_files] = strdup(strcasestr(fname, ".kpx"));
            strcpy((char *)strcasestr(fname, ".kpx"), "");      /* safe; truncating */
          }

          if (strcasestr(fname, ".jpg") != NULL)
          {
            d_exts[num_files] = strdup(strcasestr(fname, ".jpg"));
            strcpy((char *)strcasestr(fname, ".jpg"), "");      /* safe; truncating */
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
              safe_strncpy(fname2, f2->d_name, sizeof(fname2));

              if (strstr(fname2, fname) == fname2
                  && strlen(fname) == strlen(fname2) - strlen(".svg") && strcasestr(fname2, ".svg") != NULL)
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

            safe_snprintf(fname, sizeof(fname), "%s/.thumbs/%s-t.png",
                          dirname[d_places[num_files]], d_names[num_files]);
            debug(fname);
            img = IMG_Load(fname);

            if (img == NULL)
            {
              /* No thumbnail in the new location ("saved/.thumbs"),
                 try the old location ("saved/"): */

              safe_snprintf(fname, sizeof(fname), "%s/%s-t.png", dirname[d_places[num_files]], d_names[num_files]);
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
                fprintf(stderr, "\nError: Couldn't create a thumbnail of saved image!\n" "%s\n", fname);
              }

              num_files++;
            }
            else
            {
              /* No thumbnail - load original: */

              if (d_places[num_files] == PLACE_PERSONAL_TEMPLATES_DIR ||
                  d_places[num_files] == PLACE_PERSONAL_STARTERS_DIR)
              {
                /* Make sure we have a ~/.tuxpaint/[starters|templates] directory: */
                if (make_directory
                    (DIR_DATA, dirname[d_places[num_files]],
                     "Can't create user data directory (for starters/templates) (E010)"))
                {
                  /* (Make sure we have a .../[starters|templates]/.thumbs/ directory:) */
                  safe_snprintf(fname, sizeof(fname), "%s/.thumbs", dirname[d_places[num_files]]);
                  make_directory(DIR_DATA, fname,
                                 "Can't create user data thumbnail directory (for starters/templates) (E011)");
                }
              }

              img = NULL;

              if (d_places[num_files] == PLACE_STARTERS_DIR || d_places[num_files] == PLACE_PERSONAL_STARTERS_DIR)
              {
                /* Try to load a starter's background image, first!
                   If it exists, it should give a better idea of what the
                   starter looks like, compared to the overlay image... */

                /* (Try JPEG first) */
                safe_snprintf(fname, sizeof(fname), "%s/%s-back", dirname[d_places[num_files]], d_names[num_files]);
                img = load_starter_helper(fname, "jpeg", &IMG_Load);
                if (img == NULL)
                {
                  safe_snprintf(fname, sizeof(fname), "%s/%s-back", dirname[d_places[num_files]], d_names[num_files]);
                  img = load_starter_helper(fname, "jpg", &IMG_Load);
                }

#ifndef NOSVG
                if (img == NULL)
                {
                  /* (Try SVG next) */
                  safe_snprintf(fname, sizeof(fname), "%s/%s-back", dirname[d_places[num_files]], d_names[num_files]);
                  img = load_starter_helper(fname, "svg", &load_svg);
                }
#endif

                if (img == NULL)
                {
                  /* (Try PNG next) */
                  safe_snprintf(fname, sizeof(fname), "%s/%s-back", dirname[d_places[num_files]], d_names[num_files]);
                  img = load_starter_helper(fname, "png", &IMG_Load);
                }
              }

              if (img == NULL)
              {
                /* Didn't load a starter background (or didn't try!),
                   try loading the actual image... */

                safe_snprintf(fname, sizeof(fname), "%s/%s", dirname[d_places[num_files]], f->d_name);
                debug(fname);
                img = myIMG_Load(fname);
              }


              show_progress_bar(screen);

              if (img == NULL)
              {
                fprintf(stderr,
                        "\nWarning: I can't open one of the saved files!\n"
                        "%s\n"
                        "The Simple DirectMedia Layer error that " "occurred was:\n" "%s\n\n", fname, SDL_GetError());

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
                  fprintf(stderr, "\nError: Couldn't create a thumbnail of saved image!\n" "%s\n", fname);
                }

                SDL_FreeSurface(img);

                show_progress_bar(screen);


                /* Let's save this thumbnail, so we don't have to
                   create it again next time 'Open' is called: */
                /* if (d_places[num_files] == PLACE_SAVED_DIR) *//* <-- FIXME: This test should probably go...? -bjk 2009.10.15 */

                if (d_places[num_files] == PLACE_PERSONAL_STARTERS_DIR ||       /* We must check to not try to write to system wide dirs  Pere 2010.3.25 */
                    d_places[num_files] == PLACE_PERSONAL_TEMPLATES_DIR)
                {
                  debug("Saving thumbnail for this one!");

                  safe_snprintf(fname, sizeof(fname), "%s/.thumbs/%s-t.png",
                                dirname[d_places[num_files]], d_names[num_files]);

                  if (!make_directory
                      (DIR_DATA, "starters",
                       "Can't create user data directory (for starters) (E012)")
                      || !make_directory(DIR_DATA, "templates",
                                         "Can't create user data directory (for templates) (E013)")
                      || !make_directory(DIR_DATA, "starters/.thumbs",
                                         "Can't create user data directory (for starters) (E014)")
                      || !make_directory(DIR_DATA, "templates/.thumbs",
                                         "Can't create user data directory (for templates) (E015)"))
                    fprintf(stderr, "Cannot save any pictures! SORRY!\n\n");
                  else
                  {
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

  /* Throw the color palette at the end (alternative option): */

  if (new_colors_last)
  {
    first_color = num_files;
    num_files = do_new_dialog_add_colors(thumbs, num_files, d_places, d_names, d_exts, &white_in_palette);
  }


  DEBUG_PRINTF("%d files and colors were found!\n", num_files);
  DEBUG_PRINTF
    ("first_color = %d\nfirst_starter = %d\nfirst_template = %d\nnum_files = %d\n\n",
     first_color, first_starter, first_template, num_files);


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


  which_changed = 1;
  erasable = 0;

  do
  {
    /* If we're clicking an exported template, we can delete it */
    if (which_changed)
    {
      erasable = 0;

      if (!disable_erase &&
          d_places[which] == PLACE_PERSONAL_TEMPLATES_DIR &&
          strstr(d_names[which], EXPORTED_TEMPLATE_PREFIX) == d_names[which])
      {
        erasable = 1;
      }

      which_changed = 0;
    }

    /* Update screen: */

    if (update_list)
    {
      /* Erase screen: */

      dest.x = r_ttools.w;
      dest.y = 0;
      dest.w = WINDOW_WIDTH - r_ttoolopt.w - r_ttools.w;
      dest.h = button_h * buttons_tall + r_ttools.h;

      SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));


      /* Draw icons: */

      for (i = cur; i < cur + 16 && i < num_files; i++)
      {
        /* Draw cursor: */

        dest.x = THUMB_W * ((i - cur) % 4) + r_ttools.w;
        dest.y = THUMB_H * ((i - cur) / 4) + img_scroll_up->h;

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



        dest.x = THUMB_W * ((i - cur) % 4) + r_ttools.w + 10 + (THUMB_W - 20 - thumbs[i]->w) / 2;
        dest.y = THUMB_H * ((i - cur) / 4) + img_scroll_up->h + 10 + (THUMB_H - 20 - thumbs[i]->h) / 2;

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
      dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;

      if (cur < num_files - 16)
        SDL_BlitSurface(img_scroll_down, NULL, screen, &dest);
      else
        SDL_BlitSurface(img_scroll_down_off, NULL, screen, &dest);


      /* "Open" button: */

      dest.x = r_ttools.w;
      dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
      SDL_BlitSurface(img_open, NULL, screen, &dest);

      dest.x = r_ttools.w + (button_w - img_openlabels_open->w) / 2;
      dest.y = (button_h * buttons_tall + r_ttools.h) - img_openlabels_open->h;
      SDL_BlitSurface(img_openlabels_open, NULL, screen, &dest);

      /* "Erase" button: */
      if (erasable)
      {
        dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w * 2;
        dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
        SDL_BlitSurface(img_erase, NULL, screen, &dest);

        dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w * 2 + (button_w - img_openlabels_back->w) / 2;
        dest.y = (button_h * buttons_tall + r_ttools.h) - img_openlabels_back->h;
        SDL_BlitSurface(img_openlabels_erase, NULL, screen, &dest);
      }

      /* "Back" button: */

      dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w;
      dest.y = (button_h * buttons_tall + r_ttools.h) - button_h;
      SDL_BlitSurface(img_back, NULL, screen, &dest);

      dest.x = WINDOW_WIDTH - r_ttoolopt.w - button_w + (button_w - img_openlabels_back->w) / 2;
      dest.y = (button_h * buttons_tall + r_ttools.h) - img_openlabels_back->h;
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
      else
        if ((event.type == SDL_MOUSEBUTTONDOWN
             && valid_click(event.button.button)) || event.type == TP_SDL_MOUSEBUTTONSCROLL)
      {
        if (event.button.x >= r_ttools.w
            && event.button.x < WINDOW_WIDTH - r_ttoolopt.w
            && event.button.y >= img_scroll_up->h && event.button.y < (button_h * buttons_tall + r_ttools.h - button_h))
        {
          /* Picked an icon! */

          which =
            ((event.button.x - r_ttools.w) / (THUMB_W) + (((event.button.y - img_scroll_up->h) / THUMB_H) * 4)) + cur;

          which_changed = 1;

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
          if (event.button.y < img_scroll_up->h ||
              (event.button.y >=
               (button_h * buttons_tall + r_ttools.h - button_h)
               && event.button.y < (button_h * buttons_tall + r_ttools.h - img_scroll_up->h)))
          {
            /* Up or Down scroll button in New dialog: */

            if (event.button.y < img_scroll_up->h)
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
              {
                which = which - 4;
                which_changed = 1;
              }
            }
            else if (event.button.y >=
                     (button_h * buttons_tall + r_ttools.h - button_h)
                     && event.button.y < (button_h * buttons_tall + r_ttools.h - img_scroll_up->h))
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
              {
                which = which + 4;
                which_changed = 1;
              }
            }

            if (scrolltimer_dialog != TIMERID_NONE)
            {
              SDL_RemoveTimer(scrolltimer_dialog);
              scrolltimer_dialog = TIMERID_NONE;
            }

            if (!scrolling_dialog && event.type == SDL_MOUSEBUTTONDOWN)
            {
              DEBUG_PRINTF("Starting scrolling\n");
              memcpy(&scrolltimer_dialog_event, &event, sizeof(SDL_Event));
              scrolltimer_dialog_event.type = TP_SDL_MOUSEBUTTONSCROLL;

              /*
               * We enable the timer subsystem only when needed (e.g., to use SDL_AddTimer() needed
               * for scrolling) then disable it immediately after (e.g., after the timer has fired or
               * after SDL_RemoveTimer()) because enabling the timer subsystem in SDL1 has a high
               * energy impact on the Mac.
               */

              scrolling_dialog = 1;
              scrolltimer_dialog =
                SDL_AddTimer(REPEAT_SPEED, scrolltimer_dialog_callback, (void *)&scrolltimer_dialog_event);
            }
            else
            {
              DEBUG_PRINTF("Continuing scrolling\n");
              scrolltimer_dialog =
                SDL_AddTimer(REPEAT_SPEED / 3, scrolltimer_dialog_callback, (void *)&scrolltimer_dialog_event);
            }
          }
        }
        else if (event.button.x >= r_ttools.w
                 && event.button.x < r_ttools.w + button_w
                 && event.button.y >=
                 (button_h * buttons_tall + r_ttools.h) - button_h
                 && event.button.y < (button_h * buttons_tall + r_ttools.h))
        {
          /* Open */

          done = 1;
          playsound(screen, 1, SND_CLICK, 1, SNDPOS_LEFT, SNDDIST_NEAR);
        }
        else if (erasable
                 && event.button.x >=
                 (WINDOW_WIDTH - r_ttoolopt.w - button_w * 2)
                 && event.button.x < (WINDOW_WIDTH - r_ttoolopt.w - button_w)
                 && event.button.y >=
                 (button_h * buttons_tall + r_ttools.h) - button_h
                 && event.button.y < (button_h * buttons_tall + r_ttools.h))
        {
          /* "Erase" */

          if (do_prompt_image_snd(PROMPT_ERASE_TEMPLATE_TXT,
                                  PROMPT_ERASE_YES, PROMPT_ERASE_NO,
                                  thumbs[which],
                                  img_popup_arrow, img_trash, SND_AREYOUSURE,
                                  WINDOW_WIDTH - r_ttoolopt.w - button_w -
                                  button_w + 24, button_h * buttons_tall + r_ttools.h - button_h + img_scroll_up->h))
          {
            char *rfname;

            safe_snprintf(fname, sizeof(fname), "templates/%s%s", d_names[which], d_exts[which]);
            rfname = get_fname(fname, DIR_DATA);

            if (trash(rfname) == 0)
            {
              update_list = 1;


              /* Delete the thumbnail, too: */

              safe_snprintf(fname, sizeof(fname), "saved/.thumbs/%s-t.png", d_names[which]);

              free(rfname);
              rfname = get_fname(fname, DIR_SAVE);

              unlink(rfname);


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

              which_changed = 1;
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
        else if (event.button.x >= (WINDOW_WIDTH - r_ttoolopt.w - button_w) &&
                 event.button.x < (WINDOW_WIDTH - r_ttoolopt.w) &&
                 event.button.y >=
                 (button_h * buttons_tall + r_ttools.h) - button_h
                 && event.button.y < (button_h * buttons_tall + r_ttools.h))
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
          {
            which = which - 4;
            which_changed = 1;
          }
        }
        else if (event.wheel.y < 0 && cur < num_files - 16)
        {
          cur = cur + 4;
          update_list = 1;
          playsound(screen, 1, SND_SCROLL, 1, SNDPOS_CENTER, SNDDIST_NEAR);

          if (cur >= num_files - 16)
            do_setcursor(cursor_arrow);

          if (which < cur)
          {
            which = which + 4;
            which_changed = 1;
          }
        }
      }
      else if (event.type == SDL_MOUSEMOTION)
      {
        /* Deal with mouse pointer shape! */

        if (event.button.y < img_scroll_up->h &&
            event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2 &&
            event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2 && cur > 0)
        {
          /* Scroll up button: */

          do_setcursor(cursor_up);
        }
        else if (event.button.y >=
                 (button_h * buttons_tall + r_ttools.h - button_h)
                 && event.button.y <
                 (button_h * buttons_tall + r_ttools.h - img_scroll_up->h)
                 && event.button.x >= (WINDOW_WIDTH - img_scroll_up->w) / 2
                 && event.button.x <= (WINDOW_WIDTH + img_scroll_up->w) / 2 && cur < num_files - 16)
        {
          /* Scroll down button: */

          do_setcursor(cursor_down);
        }
        else
          if (((event.button.x >= r_ttools.w
                && event.button.x < r_ttools.w + button_w)
               || (event.button.x >=
                   (WINDOW_WIDTH - r_ttoolopt.w -
                    button_w * (erasable ? 2 : 1))
                   && event.button.x < (WINDOW_WIDTH - r_ttoolopt.w)
                   && d_places[which] != PLACE_STARTERS_DIR
                   && d_places[which] != PLACE_PERSONAL_STARTERS_DIR))
              && event.button.y >=
              (button_h * buttons_tall + r_ttools.h) - button_h
              && event.button.y < (button_h * buttons_tall + r_ttools.h))
        {
          /* One of the command buttons: */

          do_setcursor(cursor_hand);
        }
        else if (event.button.x >= r_ttools.w
                 && event.button.x < WINDOW_WIDTH - r_ttoolopt.w
                 && event.button.y > img_scroll_up->h
                 && event.button.y <
                 (button_h * buttons_tall + r_ttools.h) - button_h
                 &&
                 ((((event.button.x - r_ttools.w) / (THUMB_W) +
                    (((event.button.y - img_scroll_up->h) / THUMB_H) * 4)) + cur) < num_files))
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

      else if (event.type == SDL_MOUSEBUTTONUP)
      {
#ifdef __ANDROID__
        stop_motion_convert(event);
#endif

        if (scrolling_dialog)
        {
          if (scrolltimer_dialog != TIMERID_NONE)
          {
            SDL_RemoveTimer(scrolltimer_dialog);
            scrolltimer_dialog = TIMERID_NONE;
          }
          scrolling_dialog = 0;
          DEBUG_PRINTF("Killing dialog scrolling\n");
        }
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
    start_label_node = current_label_node = first_label_node_in_redo_stack =
      highlighted_label_node = label_node_to_edit = NULL;
    have_to_rec_label_node = SDL_FALSE;

    /* Clean stale text */
    if (texttool_len > 0)
    {
      texttool_str[0] = L'\0';
      texttool_len = 0;
      cursor_textwidth = 0;
    }

    if (which >= first_starter
        && (first_template == -1 || which < first_template) && (!new_colors_last || which < first_color))
    {
      /* Load a starter: */

      /* Figure out filename: */

      safe_snprintf(fname, sizeof(fname), "%s/%s%s", dirname[d_places[which]], d_names[which], d_exts[which]);

      img = myIMG_Load(fname);

      if (img == NULL)
      {
        fprintf(stderr,
                "\nWarning: Couldn't load the saved image! (3)\n"
                "%s\n" "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", fname, SDL_GetError());

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

        DEBUG_PRINTF("Smearing canvas @ 7\n");
        autoscale_copy_smear_free(img, canvas, SDL_BlitSurface);

        cur_undo = 0;
        oldest_undo = 0;
        newest_undo = 0;

        /* Immutable 'starter' image;
           we'll need to save a new image when saving...: */

        been_saved = 1;

        file_id[0] = '\0';
        safe_strncpy(starter_id, d_names[which], sizeof(starter_id));
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
    else if (first_template != -1 && which >= first_template && (!new_colors_last || which < first_color))
    {
      /* Load a template: */

      /* Figure out filename: */

      safe_snprintf(fname, sizeof(fname), "%s/%s%s", dirname[d_places[which]], d_names[which], d_exts[which]);
      img = myIMG_Load(fname);

      if (img == NULL)
      {
        fprintf(stderr,
                "\nWarning: Couldn't load the saved image! (4)\n"
                "%s\n" "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", fname, SDL_GetError());

        do_prompt(PROMPT_OPEN_UNOPENABLE_TXT, PROMPT_OPEN_UNOPENABLE_YES, "", 0, 0);
      }
      else
      {
        free_surface(&img_starter);
        free_surface(&img_starter_bkgd);
        template_personal = 0;

        DEBUG_PRINTF("Smearing template @ 8\n");
        autoscale_copy_smear_free(img, canvas, SDL_BlitSurface);

        cur_undo = 0;
        oldest_undo = 0;
        newest_undo = 0;

        /* Immutable 'template' image;
           we'll need to save a new image when saving...: */

        been_saved = 1;

        file_id[0] = '\0';
        safe_strncpy(template_id, d_names[which], sizeof(template_id));
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

      which = which - first_color;

      /* Launch color picker if they chose that: */

      if (which == COLOR_PICKER)
      {
        if (do_color_picker(-1) == 0)
          return (0);
      }
      else if (which == COLOR_MIXER)
      {
        if (do_color_mix() == 0)
          return (0);
      }

      /* FIXME: Don't do anything and go back to Open dialog if they
         hit BACK in color picker! */

      if (which == 0)           /* White */
      {
        canvas_color_r = canvas_color_g = canvas_color_b = 255;
      }
      else if (which <= white_in_palette)       /* One of the colors before white in the pallete */
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

  update_canvas(0, 0, WINDOW_WIDTH - r_ttoolopt.w - r_ttools.w, button_h * buttons_tall + r_ttools.h);


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

/* Add colors to the "New" dialog's list of choices;
   normally appears at the beginning (above Starts & Templates),
   but may be placed at the end with the "--newcolorslast" option.
*/
static int do_new_dialog_add_colors(SDL_Surface **thumbs, int num_files,
                                    int *d_places, char * *d_names, char * *d_exts, int *white_in_palette)
{
  int j;
  int added;
  Uint8 r, g, b;

  for (j = -1; j < NUM_COLORS; j++)
  {
    added = 0;

    if (j < COLOR_PICKER)
    {
      if (j == -1 ||            /* (short circuit) */
          color_hexes[j][0] != 255 ||   /* Ignore white, we'll have already added it */
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
            r = g = b = 255;    /* White */
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
        *white_in_palette = j;
      }
    }
    else if (j == COLOR_PICKER)
    {
      /* Color picker: */

      thumbs[num_files] = thumbnail(img_color_picker, THUMB_W - 20, THUMB_H - 20, 0);
      added = 1;
    }
    else if (j == COLOR_MIXER)
    {
      /* Color mixer: */

      thumbs[num_files] = thumbnail(img_color_mix, THUMB_W - 20, THUMB_H - 20, 0);
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

  return num_files;
}


/**
 * FIXME
 */
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

/**
 * FIXME
 */
static Uint8 magic_touched(int x, int y)
{
  Uint8 res;

  if (x < 0 || x >= canvas->w || y < 0 || y >= canvas->h)
    return (1);

  res = touched[(y * canvas->w) + x];
  touched[(y * canvas->w) + x] = 1;

  return (res);
}

/**
 * Removes the latest undo recorded
 */
void magic_retract_undo(void)
{
  if (cur_undo > 0)
    cur_undo--;
  else
    cur_undo = NUM_UNDO_BUFS - 1;

  newest_undo = cur_undo;
}


/**
 * Allow the user to select a color from one of the
 * pixels within their picture.
 *
 * A small dialog will appear over the color palette
 * at the bottom of the screen.
 *
 * @param boolean temp_mode - whether to only appear
 *   while a shortcut key is being held
 *   (if not, the mouse will be positioned at the bottom
 *   center, the UI will show a "Back" button, and wait
 *   for a color to be clicked in the canvas, or the "Back"
 *   button to be clicked)
 */
static int do_color_sel(int temp_mode)
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
  int done, chose, mouse_was_down;
  int back_left, back_top;
  int color_sel_x = 0, color_sel_y = 0;
  int want_animated_popups;
  Uint8 r, g, b;
  SDL_Event event;
  SDLKey key;
  SDL_Rect r_color_sel;
  SDL_Rect color_example_dest;
  SDL_Surface *backup;
  SDL_Rect r_color_picker;

  Uint32(*getpixel_img_color_picker) (SDL_Surface *, int, int);

  if (!temp_mode)
  {
    want_animated_popups = 1;
  }
  else
  {
    want_animated_popups = 0;
  }

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
    SDL_FillRect(alpha_surf, NULL, SDL_MapRGBA(alpha_surf->format, 0, 0, 0, 64));

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



  if (!temp_mode)
  {
    /* Draw current color picker color: */

    SDL_FillRect(screen, &color_example_dest,
                 SDL_MapRGB(screen->format,
                            color_hexes[COLOR_SELECTOR][0],
                            color_hexes[COLOR_SELECTOR][1], color_hexes[COLOR_SELECTOR][2]));

    /* Show "Back" button */
    back_left = r_color_sel.x + r_color_sel.w - button_w - 4;
    back_top = r_color_sel.y;

    dest.x = back_left;
    dest.y = back_top;

    SDL_BlitSurface(img_back, NULL, screen, &dest);

    dest.x = back_left + (img_back->w - img_openlabels_back->w) / 2;
    dest.y = back_top + img_back->h - img_openlabels_back->h;
    SDL_BlitSurface(img_openlabels_back, NULL, screen, &dest);
  }
  else
  {
    int mx, my;

    /* Temp mode: Grab color from canvas under the mouse immediately
       (let the motion-handling code in the loop below do it for us!) */
    SDL_GetMouseState(&mx, &my);
    SDL_WarpMouse(mx - 1, my);  /* Need to move to a different spot, or no events occur */
    SDL_WarpMouse(mx, my);

    /* These will be unused in this mode: */
    back_left = back_top = 0;
  }


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

  if (!temp_mode)
    SDL_WarpMouse(r_color_sel.x + r_color_sel.w / 2, r_color_sel.y + r_color_sel.h / 2);
#endif

  mouse_was_down = temp_mode;

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
          /* Hit [Escape]; abort (in either full UI or temporary mode) */
          chose = 0;
          done = 1;
        }
      }
      else if (event.type == SDL_MOUSEBUTTONDOWN)
      {
        mouse_was_down = 1;
      }
      else if (event.type == SDL_MOUSEBUTTONUP)
      {
        if (valid_click(event.button.button && mouse_was_down))
        {
          if (event.button.x >= r_canvas.x &&
              event.button.x < r_canvas.x + r_canvas.w &&
              event.button.y >= r_canvas.y && event.button.y < r_canvas.y + r_canvas.h)
          {
            /* Picked a color in the canvas, and released! */

            chose = 1;
            done = 1;

            x = event.button.x - r_canvas.x;
            y = event.button.y - r_canvas.y;

            color_sel_x = x;
            color_sel_y = y;
          }
          else
          {
            if (!temp_mode)
            {
              if (event.button.x >= back_left &&
                  event.button.x < back_left + img_back->w &&
                  event.button.y >= back_top && event.button.y < back_top + img_back->h)
              {
                /* Full UI mode: Decided to go Back; abort */

                chose = 0;
                done = 1;
              }
            }
            else
            {
              /* Temp mode: Released outside of canvas; abort */
              chose = 0;
              done = 1;
            }
          }
        }
        mouse_was_down = 0;
      }
      else if (event.type == SDL_MOUSEMOTION)
      {
        if (event.button.x >= r_canvas.x &&
            event.button.x < r_canvas.x + r_canvas.w &&
            event.button.y >= r_canvas.y && event.button.y < r_canvas.y + r_canvas.h)
        {
          /* Hovering over the canvas! */

          do_setcursor(cursor_pipette);


          /* Show a big solid example of the color: */

          x = event.button.x - r_canvas.x;
          y = event.button.y - r_canvas.y;

          getpixel_img_color_picker = getpixels[canvas->format->BytesPerPixel];
          SDL_GetRGB(getpixel_img_color_picker(canvas, x, y), canvas->format, &r, &g, &b);

          SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, r, g, b));

          SDL_UpdateRect(screen,
                         color_example_dest.x, color_example_dest.y, color_example_dest.w, color_example_dest.h);
        }
        else
        {
          /* Outside the canvas... */

          if (!temp_mode)
          {
            /* Revert to current color picker color, so we know what it was,
               and what we'll get if we go Back: */

            SDL_FillRect(screen, &color_example_dest,
                         SDL_MapRGB(screen->format,
                                    color_hexes[COLOR_SELECTOR][0],
                                    color_hexes[COLOR_SELECTOR][1], color_hexes[COLOR_SELECTOR][2]));

            SDL_UpdateRect(screen,
                           color_example_dest.x, color_example_dest.y, color_example_dest.w, color_example_dest.h);


            /* Change cursor to arrow (or hand, if over Back): */

            if (event.button.x >= back_left &&
                event.button.x < back_left + img_back->w &&
                event.button.y >= back_top && event.button.y < back_top + img_back->h)
              do_setcursor(cursor_hand);
            else
              do_setcursor(cursor_arrow);
          }
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

    color_hexes[COLOR_SELECTOR][0] = r;
    color_hexes[COLOR_SELECTOR][1] = g;
    color_hexes[COLOR_SELECTOR][2] = b;

    /* Re-render color selector to show the current color it contains: */
    render_color_button(COLOR_SELECTOR, img_color_sel);
  }

  return (chose);
}

/**
 * Quick eraser mode, invoked by holding [Del] while clicking.
 * (Eventually, we'll be able to detect tablet stylus erasers;
 * but waiting for https://github.com/libsdl-org/SDL/issues/2217)
 */
static void do_quick_eraser(void)
{
  SDL_Event event;
  SDLKey key;
  int val_x, val_y, motioner;
  int valhat_x, valhat_y, hatmotioner;
  int done, old_eraser;
  int mx, my;

  val_x = val_y = motioner = 0;
  valhat_x = valhat_y = hatmotioner = 0;

  /* Redraw canvas to zap any Stamps or Eraser XOR outlines */
  update_canvas(0, 0, canvas->w, canvas->h);

  /* Remember current eraser & switch to a suitable default */
  old_eraser = cur_eraser;
  cur_eraser = (NUM_ERASER_SIZES * 2) - 2;      /* 2nd-smallest circle */

  /* Snapshot the canvas, so we can undo */
  rec_undo_buffer();

  /* Do an initial erase at the click location */
  SDL_GetMouseState(&mx, &my);
  eraser_draw(mx - r_canvas.x, my - r_canvas.y, mx - r_canvas.x, my - r_canvas.y);

  done = 0;
  do
  {
    while (SDL_PollEvent(&event) && !done)
    {
      if (event.type == SDL_QUIT)
      {
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

        if (key == SDLK_ESCAPE)
        {
          /* Hit [Escape] */
          done = 1;
        }
      }
      else if (event.type == SDL_MOUSEBUTTONUP)
      {
        /* Released mouse button; all done */
        done = 1;
      }
      else if (event.type == SDL_MOUSEMOTION)
      {
        /* Dragging around the canvas */

        if (event.button.x >= r_canvas.x &&
            event.button.x < r_canvas.x + r_canvas.w &&
            event.button.y >= r_canvas.y && event.button.y < r_canvas.y + r_canvas.h)
        {
          do_setcursor(cursor_crosshair);
          eraser_draw(oldpos_x - r_canvas.x, oldpos_y - r_canvas.y,
                      event.button.x - r_canvas.x, event.button.y - r_canvas.y);
        }

        oldpos_x = event.motion.x;
        oldpos_y = event.motion.y;

        motion_since_click = 1;
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

  cur_eraser = old_eraser;
}

/**
 * Display a large prompt, allowing the user to pick a
 * color from a large palette.
 */
static int do_color_picker(int prev_color)
{
#ifndef NO_PROMPT_SHADOWS
  int i;
  SDL_Surface *alpha_surf;
#endif
  SDL_Rect dest;
  int x, y, w;
  int ox, oy;
  int val_x, val_y, motioner;
  int valhat_x, valhat_y, hatmotioner;
  int stop;

  Uint32(*getpixel_img_color_picker) (SDL_Surface *, int, int);
  Uint8 r, g, b;
  int done, chose;
  SDL_Event event;
  SDLKey key;
  int color_picker_left, color_picker_top;
  int color_picker_val_left, color_picker_val_top;
  int prev_color_left, prev_color_top;
  int picker_left, picker_top;
  int mixer_left, mixer_top;
  int back_left, back_top, done_left, done_top;
  SDL_Rect color_example_dest;
  SDL_Surface *backup;
  SDL_Rect r_color_picker;
  SDL_Rect r_final;

  val_x = val_y = motioner = 0;
  valhat_x = valhat_y = hatmotioner = 0;
  int old_cp_x, old_cp_y, old_cp_v;
  int last_motion_within_val_slider;

  /* Remember old choices, in case we hit [Back] */
  old_cp_x = color_picker_x;
  old_cp_y = color_picker_y;
  old_cp_v = color_picker_v;

  hide_blinking_cursor();


  do_setcursor(cursor_hand);
  getpixel_img_color_picker = getpixels[img_color_picker->format->BytesPerPixel];


  /* Draw button box: */

  playsound(screen, 0, SND_PROMPT, 1, SNDPOS_RIGHT, 128);

  backup = SDL_CreateRGBSurface(screen->flags, screen->w, screen->h,
                                screen->format->BitsPerPixel,
                                screen->format->Rmask,
                                screen->format->Gmask, screen->format->Bmask, screen->format->Amask);

  SDL_BlitSurface(screen, NULL, backup, NULL);

  ox = screen->w - color_button_w / 2;
  oy = r_colors.y + r_colors.h / 2;

  r_final.x = r_canvas.x + r_canvas.w / 2 - img_color_picker->w - 4;
  r_final.y = r_canvas.h / 2 - img_color_picker->h / 2 - 2;
  r_final.w = img_color_picker->w * 2;
  r_final.h = img_color_picker->h;

  stop = r_final.h / 2 + 6 + 4;

  for (w = 0; w <= stop; w = w + 4)
  {
    dest.x = ox - ((ox - r_final.x) * w) / stop;
    dest.y = oy - ((oy - r_final.y) * w) / stop;
    dest.w = w * 4;
    dest.h = w * 2;

    SDL_FillRect(screen, &dest,
                 SDL_MapRGB(screen->format, 255 - (int)(w / button_scale),
                            255 - (int)(w / button_scale), 255 - (int)(w / button_scale)));

    SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);
    if (w % 16 == 0)
      SDL_Delay(1);
  }

  SDL_BlitSurface(backup, NULL, screen, NULL);

#ifndef NO_PROMPT_SHADOWS
  alpha_surf = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                    r_final.w + 8,
                                    r_final.h + 16,
                                    screen->format->BitsPerPixel,
                                    screen->format->Rmask,
                                    screen->format->Gmask, screen->format->Bmask, screen->format->Amask);

  if (alpha_surf != NULL)
  {
    SDL_FillRect(alpha_surf, NULL, SDL_MapRGBA(alpha_surf->format, 0, 0, 0, 64));

    for (i = 8; i > 0; i = i - 2)
    {
      dest.x = r_final.x + i - 4;
      dest.y = r_final.y + i - 4;
      dest.w = r_final.w + 8;
      dest.h = r_final.h + 16;

      SDL_BlitSurface(alpha_surf, NULL, screen, &dest);
    }

    SDL_FreeSurface(alpha_surf);
  }
#endif


  /* Draw prompt box: */

  w = w - 6;
  dest.x = r_final.x - 2;
  dest.w = r_final.w + 4;
  dest.h = w * 2;
  dest.y = r_final.y - 2;
  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));


  /* Draw color palette: */

  render_color_picker_palette();

  color_picker_left = r_final.x;
  color_picker_top = r_final.y;

  dest.x = color_picker_left;
  dest.y = color_picker_top;

  SDL_BlitSurface(img_color_picker, NULL, screen, &dest);

  r_color_picker.x = dest.x;
  r_color_picker.y = dest.y;
  r_color_picker.w = dest.w;
  r_color_picker.h = dest.h;


  /* Draw values: */

  color_picker_val_left = color_picker_left + img_color_picker->w + 2;
  color_picker_val_top = color_picker_top;

  draw_color_picker_values(color_picker_val_left, color_picker_val_top);


  /* Determine spot for example color: */

  color_example_dest.x = color_picker_left + img_color_picker->w + 2 + img_back->w + 2;
  color_example_dest.y = color_picker_top + 2;
  color_example_dest.w = r_final.w / 2 - 2 - img_back->w - 2;
  color_example_dest.h = r_final.h / 2 - 4;


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
                          color_hexes[COLOR_PICKER][0], color_hexes[COLOR_PICKER][1], color_hexes[COLOR_PICKER][2]));


  /* Draw buttons to pull colors from other sources: */

  /* (Color buckets) */

  prev_color_left = r_final.x + r_final.w - (img_back->w + 2) * 3;
  prev_color_top = color_picker_top + img_color_picker->h - (img_back->h + 2) * 2;

  if (prev_color != -1 && prev_color < NUM_COLORS)
  {
    dest.x = prev_color_left;
    dest.y = prev_color_top;
    dest.w = img_back->w;
    dest.h = img_back->h;

    draw_color_grab_btn(dest, prev_color);
  }


  /* (Picker) */

  picker_left = r_final.x + r_final.w - (img_back->w + 2) * 2;
  picker_top = color_picker_top + img_color_picker->h - (img_back->h + 2) * 2;

  dest.x = picker_left;
  dest.y = picker_top;
  dest.w = img_back->w;
  dest.h = img_back->h;

  draw_color_grab_btn(dest, COLOR_PICKER);

  dest.x = picker_left + (img_back->w - img_color_sel->w) / 2;
  dest.y = picker_top + (img_back->h - img_color_sel->h) / 2;

  SDL_BlitSurface(img_color_sel, NULL, screen, &dest);


  /* (Mixer) */

  mixer_left = r_final.x + r_final.w - (img_back->w + 2);
  mixer_top = color_picker_top + img_color_picker->h - (img_back->h + 2) * 2;

  dest.x = mixer_left;
  dest.y = mixer_top;
  dest.w = img_back->w;
  dest.h = img_back->h;

  draw_color_grab_btn(dest, COLOR_MIXER);

  dest.x = mixer_left + (img_back->w - img_color_mix->w) / 2;
  dest.y = mixer_top + (img_back->h - img_color_mix->h) / 2;

  SDL_BlitSurface(img_color_mix, NULL, screen, &dest);


  /* Show "Back" button */

  back_left = r_final.x + r_final.w - img_back->w - 2;
  back_top = color_picker_top + img_color_picker->h - img_back->h - 2;

  dest.x = back_left;
  dest.y = back_top;
  dest.w = img_back->w;
  dest.h = img_back->h;

  SDL_BlitSurface(img_back, NULL, screen, &dest);

  dest.x = back_left + (img_back->w - img_openlabels_back->w) / 2;
  dest.y = back_top + img_back->h - img_openlabels_back->h;
  SDL_BlitSurface(img_openlabels_back, NULL, screen, &dest);


  /* Show "Done" button */
  done_left = back_left - img_yes->w - 2;
  done_top = back_top;

  dest.x = done_left;
  dest.y = done_top;

  SDL_BlitSurface(img_yes, NULL, screen, &dest);

  SDL_Flip(screen);


  /* Draw crosshairs */

  /* (N.B. - We do this the first time _after_ flipping the entire screen,
     so we avoid updating parts of the screen where the crosshairs
     extend past the rainbow rectangle or value slider, since this
     function does not (yet) do any clipping) */
  draw_color_picker_crosshairs(color_picker_left, color_picker_top, color_picker_val_left, color_picker_val_top);

  dest.x = color_picker_left;
  dest.y = color_picker_top;
  dest.w = img_color_picker->w;
  dest.h = img_color_picker->h;
  SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);

  dest.x = color_picker_val_left;
  dest.y = color_picker_val_top;
  dest.w = img_back->w;
  dest.h = img_color_picker_val->h;
  SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);


  /* Let the user pick a color, or go back: */

  done = 0;
  chose = 0;
  x = y = 0;
  last_motion_within_val_slider = 0;

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
      else
        if ((event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEBUTTONDOWN) && valid_click(event.button.button))
      {
        if (event.button.x >= color_picker_left &&
            event.button.x < color_picker_left + img_color_picker->w &&
            event.button.y >= color_picker_top && event.button.y < color_picker_top + img_color_picker->h)
        {
          /* Picked a color! */

          x = event.button.x - color_picker_left;
          y = event.button.y - color_picker_top;

          color_picker_x = x;
          color_picker_y = y;

          /* Update (entire) color box */
          SDL_GetRGB(getpixel_img_color_picker(img_color_picker, x, y), img_color_picker->format, &r, &g, &b);

          SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, r, g, b));

          SDL_UpdateRect(screen,
                         color_example_dest.x, color_example_dest.y, color_example_dest.w, color_example_dest.h);


          /* Reposition hue/sat crosshair */
          dest.x = color_picker_left;
          dest.y = color_picker_top;
          SDL_BlitSurface(img_color_picker, NULL, screen, &dest);
          draw_color_picker_crosshairs(color_picker_left, color_picker_top,
                                       color_picker_val_left, color_picker_val_top);
          SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);
        }
        else if (event.button.x >= color_picker_val_left &&
                 event.button.y >= color_picker_val_top &&
                 event.button.x <= color_picker_val_left + img_back->w &&
                 event.button.y <= color_picker_val_top + img_color_picker_val->h)
        {
          /* Picked a value from the slider */

          y = event.button.y - color_picker_val_top;
          color_picker_v = y;

          /* Re-render the palette with the new value */
          render_color_picker_palette();

          /* Update (entire) color box */
          SDL_GetRGB(getpixel_img_color_picker
                     (img_color_picker, color_picker_x, color_picker_y), img_color_picker->format, &r, &g, &b);

          SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, r, g, b));

          SDL_UpdateRect(screen,
                         color_example_dest.x, color_example_dest.y, color_example_dest.w, color_example_dest.h);


          /* Redraw hue/sat palette, and val slider, and redraw crosshairs */
          draw_color_picker_palette_and_values(color_picker_left,
                                               color_picker_top, color_picker_val_left, color_picker_val_top);
        }
        else if (event.button.x >= done_left &&
                 event.button.x < done_left + img_yes->w &&
                 event.button.y >= done_top && event.button.y < done_top + img_yes->h)
        {
          /* Accepting color */

          chose = 1;
          done = 1;
        }
        else if (event.button.x >= back_left &&
                 event.button.x < back_left + img_back->w &&
                 event.button.y >= back_top && event.button.y < back_top + img_back->h)
        {
          /* Decided to go Back */

          chose = 0;
          done = 1;
        }
        else if ((event.button.x >= prev_color_left &&
                  event.button.x < prev_color_left + img_back->w &&
                  event.button.y >= prev_color_top &&
                  event.button.y < prev_color_top + img_back->h &&
                  prev_color != -1 && prev_color < NUM_COLORS) ||
                 (event.button.x >= picker_left &&
                  event.button.x < picker_left + img_back->w &&
                  event.button.y >= picker_top &&
                  event.button.y < picker_top + img_back->h) ||
                 (event.button.x >= mixer_left &&
                  event.button.x < mixer_left + img_back->w &&
                  event.button.y >= mixer_top && event.button.y < mixer_top + img_back->h))
        {
          int c;
          float h, s, v;

          if (event.button.x >= prev_color_left &&
              event.button.x < prev_color_left + img_back->w &&
              event.button.y >= prev_color_top && event.button.y < prev_color_top + img_back->h)
          {
            /* Switch to the chosen bucket color */
            c = prev_color;
          }
          else if (event.button.x >= picker_left &&
                   event.button.x < picker_left + img_back->w &&
                   event.button.y >= picker_top && event.button.y < picker_top + img_back->h)
          {
            /* Picker */
            c = COLOR_PICKER;
          }
          else
          {
            /* Mixer */
            c = COLOR_MIXER;
          }

          /* Convert the chosen color to HSV & reposition crosshairs */
          rgbtohsv(color_hexes[c][0], color_hexes[c][1], color_hexes[c][2], &h, &s, &v);

          color_picker_v = (img_color_picker_val->h * (1.0 - v));
          color_picker_x = (img_color_picker->w * s);
          color_picker_y = (img_color_picker->h * (h / 360.0));

          /* Re-render the palette with the new value */
          render_color_picker_palette();

          /* Update (entire) color box */
          SDL_GetRGB(getpixel_img_color_picker
                     (img_color_picker, color_picker_x, color_picker_y), img_color_picker->format, &r, &g, &b);

          SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, r, g, b));

          SDL_UpdateRect(screen,
                         color_example_dest.x, color_example_dest.y, color_example_dest.w, color_example_dest.h);


          /* Redraw hue/sat palette, and val slider, and redraw crosshairs */
          draw_color_picker_palette_and_values(color_picker_left,
                                               color_picker_top, color_picker_val_left, color_picker_val_top);

          playsound(screen, 1, SND_BUBBLE, 1, SNDPOS_CENTER, SNDDIST_NEAR);
        }
      }
      else if (event.type == SDL_MOUSEMOTION)
      {
        if (event.button.x >= color_picker_val_left &&
            event.button.y >= color_picker_val_top &&
            event.button.x <= color_picker_val_left + img_back->w &&
            event.button.y <= color_picker_val_top + img_color_picker_val->h)
        {
          int tmp_color_picker_v;

          /* Hovering over a value from the slider */

          do_setcursor(cursor_hand);

          y = event.button.y - color_picker_val_top;
          tmp_color_picker_v = color_picker_v;
          color_picker_v = y;

          /* Re-render the palette with the new value */
          render_color_picker_palette();
          if (valid_click(event.button.button))
          {
            /* Click+dragging in value picker? */
            /* Redraw hue/sat palette, and val slider, and redraw crosshairs */
            draw_color_picker_palette_and_values(color_picker_left,
                                                 color_picker_top, color_picker_val_left, color_picker_val_top);
          }
          else
          {
            color_picker_v = tmp_color_picker_v;
          }

          /* Show a big solid example of the color: */

          x = event.button.x - color_picker_left;
          y = event.button.y - color_picker_top;

          SDL_GetRGB(getpixel_img_color_picker
                     (img_color_picker, color_picker_x, color_picker_y), img_color_picker->format, &r, &g, &b);

          if (valid_click(event.button.button))
          {
            /* Click+drag? Fill whole box */
            SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, r, g, b));
            SDL_UpdateRect(screen, color_example_dest.x, color_example_dest.y,
                           color_example_dest.w, color_example_dest.h);
          }
          else
          {
            /* Just hovering? Fill interior box */
            dest.x = color_example_dest.x + color_example_dest.w / 4;
            dest.y = color_example_dest.y + color_example_dest.h / 4;
            dest.w = color_example_dest.w / 2;
            dest.h = color_example_dest.h / 2;
            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, r, g, b));
            SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);
          }


          /* Redraw hue/sat palette, and val slider, and redraw crosshairs */
          draw_color_picker_palette_and_values(color_picker_left,
                                               color_picker_top, color_picker_val_left, color_picker_val_top);

          last_motion_within_val_slider = 1;
        }
        else
        {
          if (last_motion_within_val_slider)
          {
            render_color_picker_palette();
            draw_color_picker_palette_and_values(color_picker_left,
                                                 color_picker_top, color_picker_val_left, color_picker_val_top);
            last_motion_within_val_slider = 0;
          }

          if (event.button.x >= color_picker_left &&
              event.button.x < color_picker_left + img_color_picker->w &&
              event.button.y >= color_picker_top && event.button.y < color_picker_top + img_color_picker->h)
          {
            /* Hovering over the colors! */

            do_setcursor(cursor_pipette);


            /* Show a big solid example of the color: */

            x = event.button.x - color_picker_left;
            y = event.button.y - color_picker_top;

            SDL_GetRGB(getpixel_img_color_picker(img_color_picker, x, y), img_color_picker->format, &r, &g, &b);

            if (valid_click(event.button.button))
            {
              /* Click+drag? Fill whole box */
              SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, r, g, b));
              SDL_UpdateRect(screen, color_example_dest.x,
                             color_example_dest.y, color_example_dest.w, color_example_dest.h);
            }
            else
            {
              /* Just hovering? Fill interior box */
              dest.x = color_example_dest.x + color_example_dest.w / 4;
              dest.y = color_example_dest.y + color_example_dest.h / 4;
              dest.w = color_example_dest.w / 2;
              dest.h = color_example_dest.h / 2;
              SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, r, g, b));
              SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);
            }

            if (valid_click(event.button.button))
            {
              /* Click+dragging in the color picker? Pick the color and move the crosshair */
              x = event.button.x - color_picker_left;
              y = event.button.y - color_picker_top;

              color_picker_x = x;
              color_picker_y = y;

              dest.x = color_picker_left;
              dest.y = color_picker_top;
              SDL_BlitSurface(img_color_picker, NULL, screen, &dest);

              draw_color_picker_crosshairs(color_picker_left,
                                           color_picker_top, color_picker_val_left, color_picker_val_top);
              dest.x = color_picker_left;
              dest.y = color_picker_top;
              dest.w = img_color_picker->w;
              dest.h = img_color_picker->w;
              SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);
            }
          }
          else
          {
            /* Revert to current color picker color */

            SDL_GetRGB(getpixel_img_color_picker
                       (img_color_picker, color_picker_x, color_picker_y), img_color_picker->format, &r, &g, &b);

            SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, r, g, b));

            SDL_UpdateRect(screen,
                           color_example_dest.x, color_example_dest.y, color_example_dest.w, color_example_dest.h);

            /* Change cursor to arrow (or hand, if over Back or Done): */

            if (event.button.x >= back_left &&
                event.button.x < back_left + img_back->w &&
                event.button.y >= back_top && event.button.y < back_top + img_back->h)
              do_setcursor(cursor_hand);
            else if ((event.button.x >= prev_color_left &&
                      event.button.x < prev_color_left + img_back->w &&
                      event.button.y >= prev_color_top &&
                      event.button.y < prev_color_top + img_back->h &&
                      prev_color != -1 && prev_color < NUM_COLORS) ||
                     (event.button.x >= picker_left &&
                      event.button.x < picker_left + img_back->w &&
                      event.button.y >= picker_top &&
                      event.button.y < picker_top + img_back->h) ||
                     (event.button.x >= mixer_left &&
                      event.button.x < mixer_left + img_back->w &&
                      event.button.y >= mixer_top && event.button.y < mixer_top + img_back->h))
              do_setcursor(cursor_hand);
            else if (event.button.x >= done_left &&
                     event.button.x < done_left + img_yes->w &&
                     event.button.y >= done_top && event.button.y < done_top + img_yes->h)
              do_setcursor(cursor_hand);
            else
              do_setcursor(cursor_arrow);
          }
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


  if (chose)
  {
    /* Set the new color: */
    SDL_GetRGB(getpixel_img_color_picker
               (img_color_picker, color_picker_x, color_picker_y), img_color_picker->format, &r, &g, &b);

    color_hexes[COLOR_PICKER][0] = r;
    color_hexes[COLOR_PICKER][1] = g;
    color_hexes[COLOR_PICKER][2] = b;


    /* Re-render color picker to show the current color it contains: */
    render_color_button(COLOR_PICKER, img_color_picker_icon);
  }
  else
  {
    /* Set crosshairs to the existing color */
    color_picker_x = old_cp_x;
    color_picker_y = old_cp_y;
    color_picker_v = old_cp_v;
  }


  /* Remove the prompt: */

  update_canvas(0, 0, canvas->w, canvas->h);


  return (chose);
}


static void draw_color_picker_palette_and_values(int color_picker_left,
                                                 int color_picker_top,
                                                 int color_picker_val_left, int color_picker_val_top)
{
  SDL_Rect dest;

  draw_color_picker_values(color_picker_val_left, color_picker_val_top);

  dest.x = color_picker_left;
  dest.y = color_picker_top;
  SDL_BlitSurface(img_color_picker, NULL, screen, &dest);
  SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);

  draw_color_picker_crosshairs(color_picker_left, color_picker_top, color_picker_val_left, color_picker_val_top);

  dest.x = color_picker_val_left;
  dest.y = color_picker_val_top;
  dest.w = img_back->w;
  dest.h = img_color_picker_val->h;
  SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);

  dest.x = color_picker_left;
  dest.y = color_picker_top;
  dest.w = img_color_picker->w;
  dest.h = img_color_picker->h;
  SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);
}

static void render_color_picker_palette(void)
{
  int x, y;
  Uint8 r, g, b;
  void (*putpixel)(SDL_Surface *, int, int, Uint32);
  SDL_Event event;

  putpixel = putpixels[img_color_picker->format->BytesPerPixel];
  for (y = 0; y < img_color_picker->h; y++)
  {
    for (x = 0; x < img_color_picker->w; x++)
    {
      hsvtorgb((((float)y * 360.0) / ((float)img_color_picker->h)),
               ((float)x / ((float)img_color_picker->w)),
               1.0 - (((float)color_picker_v) / ((float)img_color_picker_val->h)), &r, &g, &b);
      putpixel(img_color_picker, x, y, SDL_MapRGBA(img_color_picker->format, r, g, b, 255));
    }
  }

  while (SDL_PollEvent(&event) && event.type == SDL_MOUSEMOTION)
  {
    /* Eat further motion events that may've been generated while
       we re-rendered the palette (so we don't spin tons of cycles
       rendering it more than we really need) */
  }

  SDL_PushEvent(&event);
}


/* Length & thickness should be odd numbers, so the
   center of the crosshair is positioned precisely */
int CROSSHAIR_LENGTH, CROSSHAIR_THICKNESS, CROSSHAIR_BORDER;

static void set_color_picker_crosshair_size(void)
{
  CROSSHAIR_LENGTH = (int)(11 * button_scale);
  CROSSHAIR_LENGTH /= 2;
  CROSSHAIR_LENGTH *= 2;
  CROSSHAIR_LENGTH++;

  if (CROSSHAIR_LENGTH < 3)
    CROSSHAIR_LENGTH = 3;


  CROSSHAIR_THICKNESS = (int)(button_scale);
  CROSSHAIR_THICKNESS /= 2;
  CROSSHAIR_THICKNESS *= 2;
  CROSSHAIR_THICKNESS++;

  if (CROSSHAIR_THICKNESS < 1)
    CROSSHAIR_THICKNESS = 1;

  CROSSHAIR_BORDER = CROSSHAIR_THICKNESS / 2;
  if (CROSSHAIR_BORDER < 1)
    CROSSHAIR_BORDER = 1;

  DEBUG_PRINTF
    ("Crosshair will be %d in size, with %d thickness, and a %d border\n",
     CROSSHAIR_LENGTH, CROSSHAIR_THICKNESS, CROSSHAIR_BORDER);
}

static void draw_color_picker_crosshairs(int color_picker_left,
                                         int color_picker_top, int color_picker_val_left, int color_picker_val_top)
{
  SDL_Rect dest;
  int ctr_x;

  /* Hue/Saturation (the big rectangle) */

  dest.x = color_picker_x + color_picker_left - (CROSSHAIR_LENGTH - 1) / 2 - CROSSHAIR_BORDER;
  dest.y = color_picker_y + color_picker_top - (CROSSHAIR_THICKNESS - 1) / 2 - CROSSHAIR_BORDER;
  dest.w = CROSSHAIR_LENGTH + CROSSHAIR_BORDER * 2;
  dest.h = CROSSHAIR_THICKNESS + CROSSHAIR_BORDER * 2;

  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));

  dest.x = color_picker_x + color_picker_left - (CROSSHAIR_THICKNESS - 1) / 2 - CROSSHAIR_BORDER;
  dest.y = color_picker_y + color_picker_top - (CROSSHAIR_LENGTH - 1) / 2 - CROSSHAIR_BORDER;
  dest.w = CROSSHAIR_THICKNESS + CROSSHAIR_BORDER * 2;
  dest.h = CROSSHAIR_LENGTH + CROSSHAIR_BORDER * 2;

  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));


  dest.x = color_picker_x + color_picker_left - (CROSSHAIR_LENGTH - 1) / 2;
  dest.y = color_picker_y + color_picker_top - (CROSSHAIR_THICKNESS - 1) / 2;
  dest.w = CROSSHAIR_LENGTH;
  dest.h = CROSSHAIR_THICKNESS;

  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));

  dest.x = color_picker_x + color_picker_left - (CROSSHAIR_THICKNESS - 1) / 2;
  dest.y = color_picker_y + color_picker_top - (CROSSHAIR_LENGTH - 1) / 2;
  dest.w = CROSSHAIR_THICKNESS;
  dest.h = CROSSHAIR_LENGTH;

  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));



  /* Value (the slider) */

  ctr_x = color_picker_val_left + img_back->w / 2;

  dest.x = ctr_x - (CROSSHAIR_LENGTH - 1) / 2 - CROSSHAIR_BORDER;
  dest.y = color_picker_v + color_picker_val_top - (CROSSHAIR_THICKNESS - 1) / 2 - CROSSHAIR_BORDER;
  dest.w = CROSSHAIR_LENGTH + CROSSHAIR_BORDER * 2;
  dest.h = CROSSHAIR_THICKNESS + CROSSHAIR_BORDER * 2;

  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));

  dest.x = ctr_x - (CROSSHAIR_THICKNESS - 1) / 2 - CROSSHAIR_BORDER;
  dest.y = color_picker_v + color_picker_val_top - (CROSSHAIR_LENGTH - 1) / 2 - CROSSHAIR_BORDER;
  dest.w = CROSSHAIR_THICKNESS + CROSSHAIR_BORDER * 2;
  dest.h = CROSSHAIR_LENGTH + CROSSHAIR_BORDER * 2;

  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 0, 0));


  dest.x = ctr_x - (CROSSHAIR_LENGTH - 1) / 2;
  dest.y = color_picker_v + color_picker_val_top - (CROSSHAIR_THICKNESS - 1) / 2;
  dest.w = CROSSHAIR_LENGTH;
  dest.h = CROSSHAIR_THICKNESS;

  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));

  dest.x = ctr_x - (CROSSHAIR_THICKNESS - 1) / 2;
  dest.y = color_picker_v + color_picker_val_top - (CROSSHAIR_LENGTH - 1) / 2;
  dest.w = CROSSHAIR_THICKNESS;
  dest.h = CROSSHAIR_LENGTH;

  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));
}


static void draw_color_picker_values(int l, int t)
{
  SDL_Rect dest;

  dest.x = l;
  dest.y = t;
  dest.w = img_color_picker_val->w;
  dest.h = img_color_picker_val->h;

  SDL_BlitSurface(img_color_picker_val, NULL, screen, &dest);
  SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);
}


static void draw_color_grab_btn(SDL_Rect dest, int c)
{
  int x, y;
  Uint8 cr, cg, cb, r, g, b, a, tmp;

  Uint32(*getpixel_btn) (SDL_Surface *, int, int);
  Uint32(*getpixel_scrn) (SDL_Surface *, int, int);
  void (*putpixel_scrn)(SDL_Surface *, int, int, Uint32);
  SDL_Rect outline_dest;

  for (y = -1; y <= 1; y++)
  {
    for (x = -1; x <= 1; x++)
    {
      outline_dest.x = dest.x + x;
      outline_dest.y = dest.y + y;
      outline_dest.w = dest.w;
      outline_dest.h = dest.h;

      SDL_BlitSurface(img_color_grab, NULL, screen, &outline_dest);
    }
  }

  cr = color_hexes[c][0];
  cg = color_hexes[c][1];
  cb = color_hexes[c][2];

  getpixel_btn = getpixels[img_color_grab->format->BytesPerPixel];
  getpixel_scrn = getpixels[img_color_grab->format->BytesPerPixel];
  putpixel_scrn = putpixels[screen->format->BytesPerPixel];

  SDL_LockSurface(screen);
  SDL_LockSurface(img_color_grab);
  for (y = 0; y < dest.h && y < img_color_grab->h; y++)
  {
    for (x = 0; x < dest.w && x < img_color_grab->w; x++)
    {
      SDL_GetRGBA(getpixel_btn(img_color_grab, x, y), img_color_grab->format, &tmp, &tmp, &tmp, &a);
      SDL_GetRGBA(getpixel_scrn(screen, dest.x + x, dest.y + y), screen->format, &r, &g, &b, &tmp);

      r = ((cr * a) + (r * (255 - a))) / 255;
      g = ((cg * a) + (g * (255 - a))) / 255;
      b = ((cb * a) + (b * (255 - a))) / 255;

      putpixel_scrn(screen, x + dest.x, y + dest.y, SDL_MapRGB(screen->format, r, g, b));
    }
  }
  SDL_UnlockSurface(screen);
  SDL_UnlockSurface(img_color_grab);
}

enum
{
  COLOR_MIXER_BTN_RED,
  COLOR_MIXER_BTN_YELLOW,
  COLOR_MIXER_BTN_BLUE,
  COLOR_MIXER_BTN_WHITE,
  COLOR_MIXER_BTN_GREY,
  COLOR_MIXER_BTN_BLACK,
#define NUM_MIXER_COLORS (COLOR_MIXER_BTN_BLACK + 1)
  COLOR_MIXER_BTN_UNDO,
  COLOR_MIXER_BTN_REDO,
  COLOR_MIXER_BTN_CLEAR,
  COLOR_MIXER_BTN_USE,
  COLOR_MIXER_BTN_BACK,
  NUM_COLOR_MIXER_BTNS
};

SDL_Rect color_example_dest;
int color_mix_btn_lefts[NUM_COLOR_MIXER_BTNS], color_mix_btn_tops[NUM_COLOR_MIXER_BTNS];

/* Hue (degrees 0-360, or -1 for N/A), Saturation (0.0-1.0), Value (0.0-1.0) */
float mixer_hsv[NUM_MIXER_COLORS][3] = {
  {330.0, 1.0, 0.9},            /* Red (Magenta-ish) */
  {60.0, 1.0, 0.9},             /* Yellow */
  {210.0, 1.0, 0.9},            /* Blue (Cyan-ish) */
  {-1, 0.0, 1.0},               /* White */
  {-1, 0.0, 0.5},               /* Grey */
  {-1, 0.0, 0.0}                /* Black */
};


const char *color_mixer_color_names[NUM_MIXER_COLORS] = {
  /* Descriptions (names) of the color mixer tool's primary colors and shades */
  gettext_noop("red"),
  gettext_noop("yellow"),
  gettext_noop("blue"),
  gettext_noop("white"),
  gettext_noop("grey"),
  gettext_noop("black")
};

const char *color_mixer_color_tips[] = {
  /* Tool tip text describing a mixed color (e.g., "1/3 red and 1/2 yellow", or "1/3 blue and 2/3 white", etc.) */
  gettext_noop("Your color is %1$s %2$s."),
  gettext_noop("Your color is %1$s %2$s and %3$s %4$s."),
  gettext_noop("Your color is %1$s %2$s, %3$s %4$s, and %5$s %6$s."),
  gettext_noop("Your color is %1$s %2$s, %3$s %4$s, %5$s %6$s, and %7$s %8$s."),
  gettext_noop("Your color is %1$s %2$s, %3$s %4$s, %5$s %6$s, %7$s %8$s, and %9$s %10$s."),
  gettext_noop("Your color is %1$s %2$s, %3$s %4$s, %5$s %6$s, %7$s %8$s, %9$s %10$s, and %11$s %12$s.")
};


int color_mixer_color_counts[NUM_MIXER_COLORS];

#define NUM_COLOR_MIX_UNDO_BUFS 20
int color_mix_cur_undo, color_mix_oldest_undo, color_mix_newest_undo;
int mixer_undo_buf[NUM_COLOR_MIX_UNDO_BUFS];

/**
 * Display a large prompt, allowing the user to mix
 * colors together from hues and black/grey/white.
 */
static int do_color_mix(void)
{
  int i, btn_clicked;

#ifndef NO_PROMPT_SHADOWS
  SDL_Surface *alpha_surf;
#endif
  int cell_w, cell_h;
  SDL_Rect dest;
  int w;
  int ox, oy;
  int val_x, val_y, motioner;
  int valhat_x, valhat_y, hatmotioner;
  int stop;
  Uint8 new_r, new_g, new_b;
  float h, s, v;
  Uint8 r, g, b;
  int done, chose;
  SDL_Event event;
  SDLKey key;
  SDL_Surface *backup;
  SDL_Rect r_final;
  int old_color_mixer_reset;

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

  ox = screen->w - color_button_w / 2;
  oy = r_colors.y + r_colors.h / 2;

  cell_w = img_back->w + 2;
  cell_h = img_back->h + 2;


  /* Area for the dialog window */
  r_final.x = r_canvas.x + (r_canvas.w - (cell_w * 6)) / 2 - 4;
  r_final.y = ((r_canvas.h - (cell_w * 4)) / 2) - 2;
  r_final.w = (cell_w * 6);
  r_final.h = (cell_h * 4);

  stop = r_final.h / 2 + 6 + 4;

  for (w = 0; w <= stop; w = w + 4)
  {
    dest.x = ox - ((ox - r_final.x) * w) / stop;
    dest.y = oy - ((oy - r_final.y) * w) / stop;
    dest.w = w * 4;
    dest.h = w * 2;

    SDL_FillRect(screen, &dest,
                 SDL_MapRGB(screen->format, 255 - (int)(w / button_scale),
                            255 - (int)(w / button_scale), 255 - (int)(w / button_scale)));

    SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);
    if (w % 16 == 0)
      SDL_Delay(1);
  }

  SDL_BlitSurface(backup, NULL, screen, NULL);

#ifndef NO_PROMPT_SHADOWS
  alpha_surf = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                    r_final.w + 8,
                                    r_final.h + 16,
                                    screen->format->BitsPerPixel,
                                    screen->format->Rmask,
                                    screen->format->Gmask, screen->format->Bmask, screen->format->Amask);

  if (alpha_surf != NULL)
  {
    SDL_FillRect(alpha_surf, NULL, SDL_MapRGBA(alpha_surf->format, 0, 0, 0, 64));

    for (i = 8; i > 0; i = i - 2)
    {
      dest.x = r_final.x + i - 4;
      dest.y = r_final.y + i - 4;
      dest.w = r_final.w + 8;
      dest.h = r_final.h + 16;

      SDL_BlitSurface(alpha_surf, NULL, screen, &dest);
    }


    SDL_FreeSurface(alpha_surf);
  }
#endif


  /* Draw prompt box: */

  w = w - 6;
  dest.x = r_final.x - 2;
  dest.w = r_final.w + 4;
  dest.h = w * 2;
  dest.y = r_final.y - 2;
  SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));


  /* Determine spot for example color: */

  color_example_dest.x = r_final.x + (cell_w) * 4;
  color_example_dest.y = r_final.y + 2;
  color_example_dest.w = cell_w * 2;
  color_example_dest.h = cell_h * 2;

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


  /* Draw current color mixer color: */
  if (color_mixer_reset)
  {
    draw_color_mixer_blank_example();
  }
  else
  {
    SDL_FillRect(screen, &color_example_dest,
                 SDL_MapRGB(screen->format,
                            color_hexes[COLOR_MIXER][0], color_hexes[COLOR_MIXER][1], color_hexes[COLOR_MIXER][2]));
  }

  rgbtohsv(color_hexes[COLOR_MIXER][0], color_hexes[COLOR_MIXER][1], color_hexes[COLOR_MIXER][2], &h, &s, &v);
  if (s == 0)
  {
    /* Current color is totally greyscale; set hue to "N/A" */
    h = -1;
  }

  /* Draw colors */
  for (i = 0; i < NUM_MIXER_COLORS; i++)
  {
    float tmp_v;

    color_mix_btn_lefts[i] = r_final.x + ((i % 3) * cell_w) + 2;
    color_mix_btn_tops[i] = r_final.y + ((i / 3) * cell_h) + 2;

    dest.x = color_mix_btn_lefts[i];
    dest.y = color_mix_btn_tops[i];
    dest.w = cell_w - 2;
    dest.h = cell_h - 2;

    tmp_v = mixer_hsv[i][2];
    if (tmp_v >= 0.05)
      tmp_v -= 0.05;

    hsvtorgb(mixer_hsv[i][0], mixer_hsv[i][1], tmp_v, &r, &g, &b);
    SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, r, g, b));
  }

  /* Draw "Undo" & "Redo" buttons */

  color_mix_btn_lefts[COLOR_MIXER_BTN_UNDO] = r_final.x + (cell_w * 0) + 2;
  color_mix_btn_tops[COLOR_MIXER_BTN_UNDO] = r_final.y + (cell_h * 3) + 2;

  color_mix_btn_lefts[COLOR_MIXER_BTN_REDO] = r_final.x + (cell_w * 1) + 2;
  color_mix_btn_tops[COLOR_MIXER_BTN_REDO] = r_final.y + (cell_h * 3) + 2;

  draw_color_mix_undo_redo();



  /* Show "Clear" button */

  color_mix_btn_lefts[COLOR_MIXER_BTN_CLEAR] = r_final.x + (cell_w * 2) + 2;
  color_mix_btn_tops[COLOR_MIXER_BTN_CLEAR] = r_final.y + (cell_h * 3) + 2;

  dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_CLEAR];
  dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_CLEAR];
  SDL_BlitSurface(img_erase, NULL, screen, &dest);

  dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_CLEAR] + (img_back->w - img_mixerlabel_clear->w) / 2;
  dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_CLEAR] + img_back->h - img_mixerlabel_clear->h;
  SDL_BlitSurface(img_mixerlabel_clear, NULL, screen, &dest);


  /* Show "Back" button */

  color_mix_btn_lefts[COLOR_MIXER_BTN_BACK] = r_final.x + (cell_w * 4) + 2;
  color_mix_btn_tops[COLOR_MIXER_BTN_BACK] = r_final.y + (cell_h * 3) + 2;

  dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_BACK];
  dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_BACK];
  SDL_BlitSurface(img_back, NULL, screen, &dest);

  dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_BACK] + (img_back->w - img_openlabels_back->w) / 2;
  dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_BACK] + img_back->h - img_openlabels_back->h;
  SDL_BlitSurface(img_openlabels_back, NULL, screen, &dest);

  /* Show "OK" button */

  color_mix_btn_lefts[COLOR_MIXER_BTN_USE] = r_final.x + (cell_w * 5) + 2;
  color_mix_btn_tops[COLOR_MIXER_BTN_USE] = r_final.y + (cell_h * 3) + 2;

  if (!color_mixer_reset)
  {
    /* Only draw "OK" button when we can accept! */
    dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_USE];
    dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_USE];
    SDL_BlitSurface(img_yes, NULL, screen, &dest);
  }

  SDL_Flip(screen);

  if (color_mixer_reset)
  {
    for (i = 0; i < NUM_MIXER_COLORS; i++)
      color_mixer_color_counts[i] = 0;
  }
  else
  {
    new_r = color_hexes[COLOR_MIXER][0];
    new_g = color_hexes[COLOR_MIXER][1];
    new_b = color_hexes[COLOR_MIXER][2];
  }



  done = 0;
  chose = 0;
  old_color_mixer_reset = color_mixer_reset;



  /* Let the user pick a color, or go back: */
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

        handle_keymouse(key, SDL_KEYDOWN, 24, NULL, NULL);

        if (key == SDLK_ESCAPE)
        {
          chose = 0;
          done = 1;
        }
      }
      else if (event.type == SDL_MOUSEMOTION)
      {
        /* Motion; change mouse pointer shape based on active UI buttons */

        btn_clicked = -1;
        for (i = 0; i < NUM_COLOR_MIXER_BTNS && btn_clicked == -1; i++)
        {
          if (event.button.x >= color_mix_btn_lefts[i] &&
              event.button.x < color_mix_btn_lefts[i] + img_back->w &&
              event.button.y >= color_mix_btn_tops[i] && event.button.y < color_mix_btn_tops[i] + img_back->h)
          {
            btn_clicked = i;
          }
        }

        if ((btn_clicked >= 0 && btn_clicked < NUM_MIXER_COLORS) ||
            btn_clicked == COLOR_MIXER_BTN_CLEAR ||
            (btn_clicked == COLOR_MIXER_BTN_USE && !color_mixer_reset) ||
            btn_clicked == COLOR_MIXER_BTN_BACK ||
            (btn_clicked == COLOR_MIXER_BTN_UNDO
             && color_mix_cur_undo != color_mix_oldest_undo)
            || (btn_clicked == COLOR_MIXER_BTN_REDO && color_mix_cur_undo != color_mix_newest_undo))
        {
          do_setcursor(cursor_hand);
        }
        else
        {
          do_setcursor(cursor_arrow);
        }
      }
      else if (event.type == SDL_MOUSEBUTTONUP && valid_click(event.button.button))
      {
        /* Released a click, determine what action to take! */

        /* Did they click any of the actual buttons? */
        btn_clicked = -1;
        for (i = 0; i < NUM_COLOR_MIXER_BTNS && btn_clicked == -1; i++)
        {
          if (event.button.x >= color_mix_btn_lefts[i] &&
              event.button.x < color_mix_btn_lefts[i] + img_back->w &&
              event.button.y >= color_mix_btn_tops[i] && event.button.y < color_mix_btn_tops[i] + img_back->h)
          {
            btn_clicked = i;
          }
        }

        if (btn_clicked >= 0 && btn_clicked < NUM_MIXER_COLORS)
        {
          /* Clicked a color! */

          if (color_mixer_reset)
          {
            /* Starting fresh; add the chosen paint 100% */
            h = mixer_hsv[btn_clicked][0];
            s = mixer_hsv[btn_clicked][1];
            v = mixer_hsv[btn_clicked][2];

            color_mixer_reset = 0;

            /* We can draw the "OK" button now! */
            dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_USE];
            dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_USE];
            dest.w = cell_w;
            dest.h = cell_h;
            SDL_BlitSurface(img_yes, NULL, screen, &dest);
            SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);

            color_mixer_color_counts[btn_clicked]++;
          }
          else
          {
            /* Blending in some color */

            color_mixer_color_counts[btn_clicked]++;

            calc_color_mixer_average(&h, &s, &v);
          }

          /* Record into undo buffer */

          mixer_undo_buf[color_mix_cur_undo] = btn_clicked;
          color_mix_cur_undo = (color_mix_cur_undo + 1) % NUM_COLOR_MIX_UNDO_BUFS;
          if (color_mix_cur_undo == color_mix_oldest_undo)
            color_mix_oldest_undo = (color_mix_oldest_undo + 1) % NUM_COLOR_MIX_UNDO_BUFS;
          color_mix_newest_undo = color_mix_cur_undo;

          draw_color_mix_undo_redo();


          /* Show the new color */

          hsvtorgb(h, s, v, &new_r, &new_g, &new_b);

          SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, new_r, new_g, new_b));
          SDL_UpdateRect(screen, color_example_dest.x, color_example_dest.y,
                         color_example_dest.w, color_example_dest.h);

          /* Draw the tooltip and play a sound */
          draw_color_mixer_tooltip();

          playsound(screen, 1, SND_BUBBLE, 1, SNDPOS_CENTER, SNDDIST_NEAR);
        }
        else if (btn_clicked == COLOR_MIXER_BTN_BACK)
        {
          /* Decided to go Back */

          chose = 0;
          done = 1;
        }
        else if (btn_clicked == COLOR_MIXER_BTN_USE && !color_mixer_reset)
        {
          /* Decided to use this color */
          chose = 1;
          done = 1;
        }
        else if (btn_clicked == COLOR_MIXER_BTN_CLEAR)
        {
          /* Decided to clear the color choice & start over */

          color_mixer_reset = 1;

          /* Wipe undo buffer */
          color_mix_cur_undo = color_mix_oldest_undo = color_mix_newest_undo = 0;
          draw_color_mix_undo_redo();

          /* Clear color usage counts */
          for (i = 0; i < NUM_MIXER_COLORS; i++)
            color_mixer_color_counts[i] = 0;

          draw_tux_text(TUX_BORED, color_names[COLOR_MIXER], 1);

          /* Erase the "OK" button! */
          dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_USE];
          dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_USE];
          dest.w = cell_w;
          dest.h = cell_h;

          SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));
          SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);

          draw_color_mixer_blank_example();

#ifndef NOSOUND
          if (!mute && use_sound)
          {
            if (!Mix_Playing(0))
            {
              eraser_sound = (eraser_sound + 1) % 2;

              playsound(screen, 0, SND_ERASER1 + eraser_sound, 0, SNDPOS_CENTER, SNDDIST_NEAR);
            }
          }
#endif
        }
        else if (btn_clicked == COLOR_MIXER_BTN_UNDO && color_mix_cur_undo != color_mix_oldest_undo)
        {
          int tot_count;

          /* Undo! */
          color_mix_cur_undo--;
          if (color_mix_cur_undo < 0)
            color_mix_cur_undo = NUM_COLOR_MIX_UNDO_BUFS - 1;

          color_mixer_color_counts[mixer_undo_buf[color_mix_cur_undo]]--;

          tot_count = 0;
          for (i = 0; i < NUM_MIXER_COLORS; i++)
            tot_count += color_mixer_color_counts[i];

          if (tot_count > 0)
          {
            /* Still have some paint on there */
            calc_color_mixer_average(&h, &s, &v);

            hsvtorgb(h, s, v, &new_r, &new_g, &new_b);

            SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, new_r, new_g, new_b));
            SDL_UpdateRect(screen, color_example_dest.x, color_example_dest.y,
                           color_example_dest.w, color_example_dest.h);

            draw_color_mixer_tooltip();
          }
          else
          {
            /* Back to the very beginning; show blank */
            color_mixer_reset = 1;
            draw_color_mixer_blank_example();

            /* Erase the "OK" button! */
            dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_USE];
            dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_USE];
            dest.w = cell_w;
            dest.h = cell_h;

            SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 255, 255, 255));
            SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);

            draw_tux_text(TUX_BORED, color_names[COLOR_MIXER], 1);
          }

          playsound(screen, 1, SND_CLICK, 1, SNDPOS_CENTER, SNDDIST_NEAR);
          draw_color_mix_undo_redo();
        }
        else if (btn_clicked == COLOR_MIXER_BTN_REDO && color_mix_cur_undo != color_mix_newest_undo)
        {
          /* Redo! */
          color_mixer_color_counts[mixer_undo_buf[color_mix_cur_undo]]++;

          calc_color_mixer_average(&h, &s, &v);

          hsvtorgb(h, s, v, &new_r, &new_g, &new_b);

          SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, new_r, new_g, new_b));
          SDL_UpdateRect(screen, color_example_dest.x, color_example_dest.y,
                         color_example_dest.w, color_example_dest.h);

          if (color_mixer_reset == 1)
          {
            /* Bringing back the first color */
            color_mixer_reset = 0;

            /* Draw "OK" */
            dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_USE];
            dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_USE];
            dest.w = cell_w;
            dest.h = cell_h;
            SDL_BlitSurface(img_yes, NULL, screen, &dest);
            SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);
          }

          color_mix_cur_undo = (color_mix_cur_undo + 1) % NUM_COLOR_MIX_UNDO_BUFS;

          playsound(screen, 1, SND_CLICK, 1, SNDPOS_CENTER, SNDDIST_NEAR);
          draw_color_mix_undo_redo();

          draw_color_mixer_tooltip();
        }
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
    color_hexes[COLOR_MIXER][0] = new_r;
    color_hexes[COLOR_MIXER][1] = new_g;
    color_hexes[COLOR_MIXER][2] = new_b;


    /* Re-render color mixer to show the current color it contains: */
    render_color_button(COLOR_MIXER, img_color_mix);
  }
  else
  {
    color_mixer_reset = old_color_mixer_reset;
  }

  /* Remove the prompt: */

  update_canvas(0, 0, canvas->w, canvas->h);


  return (chose);
}




/**
 * Draw a pattern over the color mixer sample,
 * for when no color has been chosen.
 */
static void draw_color_mixer_blank_example(void)
{
  int w;
  SDL_Rect dest;

  SDL_FillRect(screen, &color_example_dest, SDL_MapRGB(screen->format, 192, 192, 192));

  for (w = 0; w < color_example_dest.w; w += 4)
  {
    dest.x = color_example_dest.x + w;
    dest.y = color_example_dest.y;
    dest.w = 2;
    dest.h = color_example_dest.h;

    SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 128, 128, 128));
  }

  SDL_UpdateRect(screen, color_example_dest.x, color_example_dest.y, color_example_dest.w, color_example_dest.h);
}


/**
 * Take the colors chosen in the mixer, in their various
 * proportions, and calculate an HSV value for the
 * final color
 */
static void calc_color_mixer_average(float *out_h, float *out_s, float *out_v)
{
  float h, s, v;
  float circ_mean_avg_sin, circ_mean_avg_cos;
  int i, tot_count, tot_count_hue;
  float sat, val;

  tot_count = tot_count_hue = 0;
  circ_mean_avg_sin = circ_mean_avg_cos = 0.0;
  sat = val = 0.0;

  for (i = 0; i < NUM_MIXER_COLORS; i++)
  {
    tot_count += color_mixer_color_counts[i];

    if (mixer_hsv[i][0] != -1)
    {
      tot_count_hue += color_mixer_color_counts[i];

      circ_mean_avg_sin += (sin(mixer_hsv[i][0] * M_PI / 180.0) * color_mixer_color_counts[i]);
      circ_mean_avg_cos += (cos(mixer_hsv[i][0] * M_PI / 180.0) * color_mixer_color_counts[i]);
    }

    sat += mixer_hsv[i][1] * (color_mixer_color_counts[i]);
    val += mixer_hsv[i][2] * (color_mixer_color_counts[i]);
  }

  if (tot_count_hue == 0)
  {
    /* None of the colors we mixed has any hue! */
    h = -1;
  }
  else
  {
    /* Average all the hues we have */
    circ_mean_avg_sin /= tot_count_hue;
    circ_mean_avg_cos /= tot_count_hue;

    h = atan2(circ_mean_avg_sin, circ_mean_avg_cos) * 180.0 / M_PI;
    if (h < 0.0)
      h += 360.0;
    else if (h >= 360.0)
      h -= 360.0;
  }

  /* Average the saturation and values */

  s = sat / tot_count;
  v = val / tot_count;

  *out_h = h;
  *out_s = s;
  *out_v = v;
}

/**
 * Draw the undo & redo buttons of the color mixer,
 * making the buttons appear clickable ("up") or not ("off"),
 * depending on the state of the color mixer's undo buffer
 */
static void draw_color_mix_undo_redo(void)
{
  SDL_Rect dest;
  SDL_Surface *icon_label_color, *tmp_surf;

  /* Show "Undo" button */

  dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_UNDO];
  dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_UNDO];

  if (color_mix_cur_undo != color_mix_oldest_undo)
  {
    SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
    icon_label_color = img_black;
  }
  else
  {
    SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
    icon_label_color = img_grey;
  }

  dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_UNDO] + (img_back->w - img_tools[TOOL_UNDO]->w) / 2;
  dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_UNDO];

  tmp_surf = SDL_DisplayFormatAlpha(img_tools[TOOL_UNDO]);
  SDL_BlitSurface(icon_label_color, NULL, tmp_surf, NULL);
  SDL_BlitSurface(tmp_surf, NULL, screen, &dest);
  SDL_FreeSurface(tmp_surf);

  dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_UNDO] + (img_back->w - img_tool_names[TOOL_UNDO]->w) / 2;
  dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_UNDO] + img_back->h - img_tool_names[TOOL_UNDO]->h;

  tmp_surf = SDL_DisplayFormatAlpha(img_tool_names[TOOL_UNDO]);
  SDL_BlitSurface(icon_label_color, NULL, tmp_surf, NULL);
  SDL_BlitSurface(tmp_surf, NULL, screen, &dest);
  SDL_FreeSurface(tmp_surf);

  SDL_UpdateRect(screen, color_mix_btn_lefts[COLOR_MIXER_BTN_UNDO],
                 color_mix_btn_tops[COLOR_MIXER_BTN_UNDO], img_back->w, img_back->h);


  /* Show "Redo" button */

  dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_REDO];
  dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_REDO];

  if (color_mix_cur_undo != color_mix_newest_undo)
  {
    SDL_BlitSurface(img_btn_up, NULL, screen, &dest);
    icon_label_color = img_black;
  }
  else
  {
    SDL_BlitSurface(img_btn_off, NULL, screen, &dest);
    icon_label_color = img_grey;
  }

  dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_REDO] + (img_back->w - img_tools[TOOL_REDO]->w) / 2;
  dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_REDO];

  tmp_surf = SDL_DisplayFormatAlpha(img_tools[TOOL_REDO]);
  SDL_BlitSurface(icon_label_color, NULL, tmp_surf, NULL);
  SDL_BlitSurface(tmp_surf, NULL, screen, &dest);
  SDL_FreeSurface(tmp_surf);

  dest.x = color_mix_btn_lefts[COLOR_MIXER_BTN_REDO] + (img_back->w - img_tool_names[TOOL_REDO]->w) / 2;
  dest.y = color_mix_btn_tops[COLOR_MIXER_BTN_REDO] + img_back->h - img_tool_names[TOOL_REDO]->h;

  tmp_surf = SDL_DisplayFormatAlpha(img_tool_names[TOOL_REDO]);
  SDL_BlitSurface(icon_label_color, NULL, tmp_surf, NULL);
  SDL_BlitSurface(tmp_surf, NULL, screen, &dest);
  SDL_FreeSurface(tmp_surf);

  SDL_UpdateRect(screen, color_mix_btn_lefts[COLOR_MIXER_BTN_REDO],
                 color_mix_btn_tops[COLOR_MIXER_BTN_REDO], img_back->w, img_back->h);
}


/**
 * Show a tooltip describing the color the user has mixed
 */
static void draw_color_mixer_tooltip(void)
{
  int i, num_colors_used, tot_count;
  char tip_txt[1024];
  char tip_txt_proportions[NUM_MIXER_COLORS][64];
  int used_colors_color[NUM_MIXER_COLORS], used_colors_amount[NUM_MIXER_COLORS];

  num_colors_used = 0;
  tot_count = 0;
  for (i = 0; i < NUM_MIXER_COLORS; i++)
  {
    if (color_mixer_color_counts[i])
    {
      used_colors_color[num_colors_used] = i;
      used_colors_amount[num_colors_used] = color_mixer_color_counts[i];
      num_colors_used++;

      tot_count += color_mixer_color_counts[i];
    }
  }

  if (num_colors_used == 1)
  {
    if (used_colors_amount[0] == 1)
    {
      snprintf(tip_txt, sizeof(tip_txt), gettext(color_mixer_color_tips[0]),
               /* Color mixer; e.g., "Your color is entirely grey." */
               gettext("entirely"), gettext(color_mixer_color_names[used_colors_color[0]]));
    }
    else
    {
      snprintf(tip_txt_proportions[0], sizeof(tip_txt_proportions[0]),
               "%1$s (%2$d/%3$d)", gettext("entirely"), used_colors_amount[0], used_colors_amount[0]);

      snprintf(tip_txt, sizeof(tip_txt), gettext(color_mixer_color_tips[0]),
               tip_txt_proportions[0], gettext(color_mixer_color_names[used_colors_color[0]]));
    }
  }
  else
  {
    for (i = 0; i < num_colors_used; i++)
    {
      int factor, best_factor;

      best_factor = 0;
      for (factor = 2; factor <= used_colors_amount[i]; factor++)
      {
        if ((used_colors_amount[i] % factor) == 0 && (tot_count % factor) == 0)
          best_factor = factor;
      }

      if (best_factor)
      {
        snprintf(tip_txt_proportions[i], sizeof(tip_txt_proportions[i]),
                 "%d/%d (%d/%d)",
                 used_colors_amount[i], tot_count,
                 (int)(used_colors_amount[i] / best_factor), (int)(tot_count / best_factor));
      }
      else
      {
        snprintf(tip_txt_proportions[i], sizeof(tip_txt_proportions[i]), "%d/%d", used_colors_amount[i], tot_count);
      }
    }

    tip_txt[0] = '\0';          /* Just in case! */
    if (num_colors_used == 2)
    {
      snprintf(tip_txt, sizeof(tip_txt),
               gettext(color_mixer_color_tips[num_colors_used - 1]),
               tip_txt_proportions[0],
               gettext(color_mixer_color_names[used_colors_color[0]]),
               tip_txt_proportions[1], gettext(color_mixer_color_names[used_colors_color[1]]));
    }
    else if (num_colors_used == 3)
    {
      snprintf(tip_txt, sizeof(tip_txt),
               gettext(color_mixer_color_tips[num_colors_used - 1]),
               tip_txt_proportions[0],
               gettext(color_mixer_color_names[used_colors_color[0]]),
               tip_txt_proportions[1],
               gettext(color_mixer_color_names[used_colors_color[1]]),
               tip_txt_proportions[2], gettext(color_mixer_color_names[used_colors_color[2]]));
    }
    else if (num_colors_used == 4)
    {
      snprintf(tip_txt, sizeof(tip_txt),
               gettext(color_mixer_color_tips[num_colors_used - 1]),
               tip_txt_proportions[0],
               gettext(color_mixer_color_names[used_colors_color[0]]),
               tip_txt_proportions[1],
               gettext(color_mixer_color_names[used_colors_color[1]]),
               tip_txt_proportions[2],
               gettext(color_mixer_color_names[used_colors_color[2]]),
               tip_txt_proportions[3], gettext(color_mixer_color_names[used_colors_color[3]]));
    }
    else if (num_colors_used == 5)
    {
      snprintf(tip_txt, sizeof(tip_txt),
               gettext(color_mixer_color_tips[num_colors_used - 1]),
               tip_txt_proportions[0],
               gettext(color_mixer_color_names[used_colors_color[0]]),
               tip_txt_proportions[1],
               gettext(color_mixer_color_names[used_colors_color[1]]),
               tip_txt_proportions[2],
               gettext(color_mixer_color_names[used_colors_color[2]]),
               tip_txt_proportions[3],
               gettext(color_mixer_color_names[used_colors_color[3]]),
               tip_txt_proportions[4], gettext(color_mixer_color_names[used_colors_color[4]]));
    }
    else if (num_colors_used == 6)
    {
      snprintf(tip_txt, sizeof(tip_txt),
               gettext(color_mixer_color_tips[num_colors_used - 1]),
               tip_txt_proportions[0],
               gettext(color_mixer_color_names[used_colors_color[0]]),
               tip_txt_proportions[1],
               gettext(color_mixer_color_names[used_colors_color[1]]),
               tip_txt_proportions[2],
               gettext(color_mixer_color_names[used_colors_color[2]]),
               tip_txt_proportions[3],
               gettext(color_mixer_color_names[used_colors_color[3]]),
               tip_txt_proportions[4],
               gettext(color_mixer_color_names[used_colors_color[4]]),
               tip_txt_proportions[5], gettext(color_mixer_color_names[used_colors_color[5]]));
    }
  }

  draw_tux_text(TUX_GREAT, tip_txt, 1);
}

/**
 * Render an interactive color button (selector, picker, mixer)
 * with their current color.
 *
 * @param int the_color - the color within the palette (e.g., COLOR_PICKER) (its RGB values will be grabbed via global color_hexes[], and the new button will be rendered into the appropriate img_color_btns[])
 * @param SDL_Surface * icon - a bitmap to be applied to the button (or NULL if none) (e.g., the pipette icon that appears over the color selector)
 */
static void render_color_button(int the_color, SDL_Surface *icon)
{
  SDL_Surface *tmp_btn_up, *tmp_btn_down;
  SDL_Rect dest;
  double rh, gh, bh;
  int x, y;

  Uint32(*getpixel_tmp_btn_up) (SDL_Surface *, int, int);
  Uint32(*getpixel_tmp_btn_down) (SDL_Surface *, int, int);
  Uint32(*getpixel_img_paintwell) (SDL_Surface *, int, int);

  tmp_btn_up = thumbnail(img_btn_up, color_button_w, color_button_h, 0);
  tmp_btn_down = thumbnail(img_btn_down, color_button_w, color_button_h, 0);
  img_color_btn_off = thumbnail(img_btn_off, color_button_w, color_button_h, 0);        /* FIXME: Need to free? */

  getpixel_tmp_btn_up = getpixels[tmp_btn_up->format->BytesPerPixel];
  getpixel_tmp_btn_down = getpixels[tmp_btn_down->format->BytesPerPixel];
  getpixel_img_paintwell = getpixels[img_paintwell->format->BytesPerPixel];

  rh = sRGB_to_linear_table[color_hexes[the_color][0]];
  gh = sRGB_to_linear_table[color_hexes[the_color][1]];
  bh = sRGB_to_linear_table[color_hexes[the_color][2]];

  SDL_LockSurface(img_color_btns[the_color]);
  SDL_LockSurface(img_color_btns[the_color + NUM_COLORS]);

  for (y = 0; y < tmp_btn_up->h; y++)
  {
    for (x = 0; x < tmp_btn_up->w; x++)
    {
      double ru, gu, bu, rd, gd, bd, aa;
      Uint8 a, r, g, b;

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

      putpixels[img_color_btns[the_color]->format->BytesPerPixel]
        (img_color_btns[the_color], x, y,
         SDL_MapRGB(img_color_btns[the_color]->format,
                    linear_to_sRGB(rh * aa + ru * (1.0 - aa)),
                    linear_to_sRGB(gh * aa + gu * (1.0 - aa)), linear_to_sRGB(bh * aa + bu * (1.0 - aa))));

      putpixels[img_color_btns[the_color + NUM_COLORS]->format->BytesPerPixel] (img_color_btns[the_color + NUM_COLORS],
                                                                                x, y,
                                                                                SDL_MapRGB(img_color_btns
                                                                                           [the_color +
                                                                                            NUM_COLORS]->format,
                                                                                           linear_to_sRGB(rh * aa +
                                                                                                          rd * (1.0 -
                                                                                                                aa)),
                                                                                           linear_to_sRGB(gh * aa +
                                                                                                          gd * (1.0 -
                                                                                                                aa)),
                                                                                           linear_to_sRGB(bh * aa +
                                                                                                          bd * (1.0 -
                                                                                                                aa))));
    }
  }

  SDL_UnlockSurface(img_color_btns[the_color]);
  SDL_UnlockSurface(img_color_btns[the_color + NUM_COLORS]);

  SDL_FreeSurface(tmp_btn_up);
  SDL_FreeSurface(tmp_btn_down);

  if (icon != NULL)
  {
    /* Apply pipette to color selector entries */
    dest.x = (img_color_btns[the_color]->w - icon->w) / 2;
    dest.y = (img_color_btns[the_color]->h - icon->h) / 2;
    dest.w = icon->w;
    dest.h = icon->h;
    SDL_BlitSurface(icon, NULL, img_color_btns[the_color], &dest);

    dest.x = (img_color_btns[the_color + NUM_COLORS]->w - icon->w) / 2;
    dest.y = (img_color_btns[the_color + NUM_COLORS]->h - icon->h) / 2;
    SDL_BlitSurface(icon, NULL, img_color_btns[the_color + NUM_COLORS], &dest);
  }
}


/**
 * Things to do whenever a color is changed
 * (either by selecting a color, using the color selector in full-UI mode,
 * using the color picker (palette), or using the shortcut key to
 * use color selector in temp-mode.
 */
static void handle_color_changed(void)
{
  render_brush();

  if (cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL)
  {
    do_render_cur_text(0);
  }
  else if (cur_tool == TOOL_MAGIC)
  {
    magic_set_color();
  }
  else if (cur_tool == TOOL_STAMP)
  {
    clear_cached_stamp();
  }
}

static void magic_set_color(void)
{
  int undo_ctr;
  SDL_Surface *last;
  SDL_Rect update_rect;

  update_rect.x = 0;
  update_rect.y = 0;
  update_rect.w = 0;
  update_rect.h = 0;

  if (cur_undo > 0)
    undo_ctr = cur_undo - 1;
  else
    undo_ctr = NUM_UNDO_BUFS - 1;

  last = undo_bufs[undo_ctr];

  magic_funcs[magics[magic_group][cur_magic[magic_group]].handle_idx].set_color(magic_api_struct,
                                                                                magics[magic_group][cur_magic
                                                                                                    [magic_group]].idx,
                                                                                canvas, last, color_hexes[cur_color][0],
                                                                                color_hexes[cur_color][1],
                                                                                color_hexes[cur_color][2],
                                                                                &update_rect);

  if (update_rect.w > 0 && update_rect.h > 0)
  {
    update_canvas(update_rect.x, update_rect.y, update_rect.x + update_rect.w, update_rect.y + update_rect.h);
  }
}

/**
 * Send the [new] size choice for a given Magic tool to
 * the tool's plugin.
 *
 * We expect that `magics[magic_group][cur_magic[magic_group]].size`
 * has been set prior to calling this.
 */
static void magic_set_size()
{
  int undo_ctr;
  SDL_Surface *last;
  SDL_Rect update_rect;

  update_rect.x = 0;
  update_rect.y = 0;
  update_rect.w = 0;
  update_rect.h = 0;

  if (cur_undo > 0)
    undo_ctr = cur_undo - 1;
  else
    undo_ctr = NUM_UNDO_BUFS - 1;

  last = undo_bufs[undo_ctr];

  DEBUG_PRINTF("set_size for mode %04x (%d) being set to %d\n",
               magics[magic_group][cur_magic[magic_group]].mode,
               magic_modeint(magics[magic_group][cur_magic[magic_group]].mode),
               magics[magic_group][cur_magic[magic_group]].size[magic_modeint
                                                                (magics[magic_group][cur_magic[magic_group]].mode)]);

  magic_funcs[magics[magic_group][cur_magic[magic_group]].handle_idx].set_size(magic_api_struct,
                                                                               magics[magic_group][cur_magic
                                                                                                   [magic_group]].idx,
                                                                               magics[magic_group][cur_magic
                                                                                                   [magic_group]].mode,
                                                                               canvas, last,
                                                                               magics[magic_group][cur_magic
                                                                                                   [magic_group]].size
                                                                               [magic_modeint
                                                                                (magics[magic_group]
                                                                                 [cur_magic[magic_group]].mode)],
                                                                               &update_rect);

  if (update_rect.w > 0 && update_rect.h > 0)
  {
    update_canvas(update_rect.x, update_rect.y, update_rect.x + update_rect.w, update_rect.y + update_rect.h);
  }
}

/**
 * FIXME
 */
static void magic_putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
  putpixels[surface->format->BytesPerPixel] (surface, x, y, pixel);
}

/**
 * FIXME
 */
static Uint32 magic_getpixel(SDL_Surface *surface, int x, int y)
{
  return (getpixels[surface->format->BytesPerPixel] (surface, x, y));
}

/**
 * FIXME
 */
static void magic_xorpixel(SDL_Surface *surface, int x, int y)
{
  _xorpixel(surface, x, y);
}


/**
 * FIXME
 */
static void magic_switchout(SDL_Surface *last)
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
    int grp, cur;

    grp = magic_group;
    cur = cur_magic[magic_group];

    magic_funcs[magics[grp][cur].handle_idx].switchout(magic_api_struct,
                                                       magics[grp][cur].idx, magics[grp][cur].mode, canvas, last);
    update_canvas(0, 0, canvas->w, canvas->h);

    if (was_clicking && magics[grp][cur].mode == MODE_PAINT_WITH_PREVIEW)
    {
      /* Clean up preview! */
      do_undo();
      tool_avail[TOOL_REDO] = 0;        /* Don't let them 'redo' to get preview back */
      draw_toolbar();
      update_screen_rect(&r_tools);
    }

    update_screen_rect(&r_canvas);
  }
}

/**
 * FIXME
 */
static void magic_switchin(SDL_Surface *last)
{
  if (cur_tool == TOOL_MAGIC)
  {
    int grp, cur;

    grp = magic_group;
    cur = cur_magic[magic_group];

    magic_funcs[magics[grp][cur].handle_idx].switchin(magic_api_struct,
                                                      magics[grp][cur].idx, magics[grp][cur].mode, canvas, last);

    /* In case the Magic tool's switchin() called update_progress_bar(),
       let's put the old Tux text back: */

    redraw_tux_text();

    update_canvas(0, 0, canvas->w, canvas->h);
  }
}

/**
 * FIXME
 */
static int magic_modeint(int mode)
{
  if (mode == MODE_PAINT || mode == MODE_ONECLICK || mode == MODE_PAINT_WITH_PREVIEW)
    return 0;
  else if (mode == MODE_FULLSCREEN)
    return 1;
  else
    return 0;
}

/**
 * FIXME
 */
static void add_label_node(int w, int h, Uint16 x, Uint16 y, SDL_Surface *label_node_surface)
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
  new_node->save_texttool_str[i] = L'\0';
  // printf("New node's text is: \"%ls\"\n", new_node->save_texttool_str);

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
    new_node->is_enabled = SDL_TRUE;
  }
  else
  {
    new_node->is_enabled = SDL_FALSE;
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
  if (highlighted_label_node->is_enabled == SDL_FALSE)
    cycle_highlighted_label_node();
}


/**
 * FIXME
 */
static struct label_node *search_label_list(struct label_node **ref_head, Uint16 x, Uint16 y, int hover)
{
  struct label_node *current_node;
  struct label_node *tmp_node = NULL;
  unsigned u;
  int done = SDL_FALSE;
  int k;

  if (*ref_head == NULL)
    return (NULL);

  current_node = *ref_head;

  while (done != SDL_TRUE)
  {
    if (x >= current_node->save_x)
    {
      if (y >= current_node->save_y)
      {
        if (x <= (current_node->save_x) + (current_node->save_width))
        {
          if (y <= (current_node->save_y) + (current_node->save_height))
          {
            if (current_node->is_enabled == SDL_TRUE)
            {
              if (hover == 1)
                return (current_node);
              tmp_node = current_node;
              done = SDL_TRUE;
            }
          }
        }
      }
    }
    current_node = current_node->next_to_down_label_node;
    if (current_node == NULL)
      current_node = current_label_node;
    if (current_node == *ref_head)
      done = SDL_TRUE;
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
    select_texttool_str[u] = L'\0';

    for (k = 0; k < NUM_COLORS; k++)
    {
      if ((color_hexes[k][0] == tmp_node->save_color.r) &&
          (color_hexes[k][1] == tmp_node->save_color.g) &&
          (color_hexes[k][2] == tmp_node->save_color.b) && (k < COLOR_PICKER))
      {
        select_color = k;
        cur_color = k;
        break;
      }

      if (k == COLOR_PICKER)
      {
        /* If the label's color is not one of the built-ins,
           set the color picker to the label's color, and
           make the color picker be the currently-selected color */
        cur_color = COLOR_PICKER;
        select_color = COLOR_PICKER;
        color_hexes[select_color][0] = tmp_node->save_color.r;
        color_hexes[select_color][1] = tmp_node->save_color.g;
        color_hexes[select_color][2] = tmp_node->save_color.b;

        render_color_button(COLOR_PICKER, img_color_picker_icon);
        draw_colors(COLORSEL_CLOBBER);
        render_brush();         /* FIXME: render_brush should be called at the start of Brush and Line tools? */
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

/**
 * FIXME
 */
static void rec_undo_label(void)
{
  if (first_label_node_in_redo_stack != NULL)
  {
    delete_label_list(&first_label_node_in_redo_stack);
    first_label_node_in_redo_stack = NULL;
  }

  if (coming_from_undo_or_redo) // yet recorded, avoiding to write text_undo
  {
    coming_from_undo_or_redo = SDL_FALSE;
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
    have_to_rec_label_node = SDL_FALSE;
  }
  else
  {
    text_undo[cur_undo] = 0;

    /* Have we cycled around NUM_UNDO_BUFS? */
    if (current_label_node != NULL && current_label_node->save_undoid == (cur_undo + 1) % NUM_UNDO_BUFS)
      current_label_node->save_undoid = 255;
  }
}

/**
 * FIXME
 */
static void do_undo_label_node()
{
  if (text_undo[(cur_undo + 1) % NUM_UNDO_BUFS] == 1)
    if (current_label_node != NULL)
    {
      if (current_label_node->save_undoid == (cur_undo + 1) % NUM_UNDO_BUFS)
      {
        if (current_label_node->disables != NULL)       /* If current node is an editing of an older one, reenable it. */
          current_label_node->disables->is_enabled = SDL_TRUE;

        first_label_node_in_redo_stack = current_label_node;
        current_label_node = current_label_node->next_to_down_label_node;

        if (current_label_node == NULL)
          start_label_node = current_label_node;

        highlighted_label_node = current_label_node;
        derender_node(&first_label_node_in_redo_stack);
        coming_from_undo_or_redo = SDL_TRUE;
      }
    }
  highlighted_label_node = current_label_node;
}

/**
 * FIXME
 */
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
      if (current_label_node->disables != NULL) /* If this is a redo of an editing, redisable the old node. */
      {
        current_label_node->disables->is_enabled = SDL_FALSE;
        derender_node(&current_label_node->disables);
      }
      else
        simply_render_node(current_label_node);

      coming_from_undo_or_redo = SDL_TRUE;
    }
  }
}


/**
 * FIXME
 */
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

    if (dest.x + src.w > WINDOW_WIDTH - r_ttoolopt.w - r_ttools.w)
      src.w = WINDOW_WIDTH - r_ttoolopt.w - r_ttools.w - dest.x;
    if (dest.y + src.h > (button_h * buttons_tall + r_ttools.h))
      src.h = (button_h * buttons_tall + r_ttools.h) - dest.y;

    myblit(node->label_node_surface, &src, label, &dest);

    update_canvas(dest.x, dest.y, dest.x + node->label_node_surface->w, dest.y + node->label_node_surface->h);

    /* Setting the sizes correctly */
    node->save_width = node->label_node_surface->w;
    node->save_height = node->label_node_surface->h;
  }
}

/**
 * FIXME
 */
static void render_all_nodes_starting_at(struct label_node **node)
{
  struct label_node *current_node;

  if (*node != NULL)
  {
    current_node = *node;
    while (current_node != first_label_node_in_redo_stack)
    {
      if (current_node->is_enabled == SDL_TRUE)
      {
        simply_render_node(current_node);
      }
      if (current_node->next_to_up_label_node == NULL)
        return;
      current_node = current_node->next_to_up_label_node;
    }
  }
}

/**
 * FIXME
 */
/* FIXME: This should search for the top-down of the overlaping labels and only re-render from it */
static void derender_node( __attribute__((unused))
                          struct label_node **ref_head)
{
  SDL_Rect r_tmp_derender;

  r_tmp_derender.w = label->w;
  r_tmp_derender.h = label->h;
  r_tmp_derender.x = 0;
  r_tmp_derender.y = 0;

  SDL_FillRect(label, &r_tmp_derender, SDL_MapRGBA(label->format, 0, 0, 0, 0));

  render_all_nodes_starting_at(&start_label_node);
}

/**
 * FIXME
 */
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

/**
 * FIXME
 */
/* A custom bliter that allows to put two transparent layers toghether without having to deal with colorkeys or SDL_SRCALPHA
   I am always reinventing the wheel. Hope this one is not squared. Pere */
static void myblit(SDL_Surface *src_surf, SDL_Rect *src_rect, SDL_Surface *dest_surf, SDL_Rect *dest_rect)
{
  int x, y;
  Uint8 src_r, src_g, src_b, src_a;
  Uint8 dest_r, dest_g, dest_b, dest_a;

  for (x = src_rect->x; x < src_rect->w + src_rect->x; x++)
    for (y = src_rect->y; y < src_rect->h + src_rect->y; y++)
    {
      SDL_GetRGBA(getpixels[src_surf->format->BytesPerPixel]
                  (src_surf, x - src_rect->x, y - src_rect->y), src_surf->format, &src_r, &src_g, &src_b, &src_a);
      if (src_a != SDL_ALPHA_TRANSPARENT)
      {
        if (src_a == SDL_ALPHA_OPAQUE)
          putpixels[dest_surf->format->BytesPerPixel] (dest_surf,
                                                       x + dest_rect->x,
                                                       y + dest_rect->y,
                                                       SDL_MapRGBA(dest_surf->format, src_r, src_g, src_b, src_a));
        else
        {
          SDL_GetRGBA(getpixels[dest_surf->format->BytesPerPixel]
                      (dest_surf, x + dest_rect->x, y + dest_rect->y),
                      src_surf->format, &dest_r, &dest_g, &dest_b, &dest_a);
          if (dest_a == SDL_ALPHA_TRANSPARENT)
            putpixels[dest_surf->format->BytesPerPixel] (dest_surf,
                                                         x + dest_rect->x,
                                                         y + dest_rect->y,
                                                         SDL_MapRGBA(dest_surf->format, src_r, src_g, src_b, src_a));
          else
          {
            dest_r = src_r * src_a / 255 + dest_r * dest_a * (255 - src_a) / 255 / 255;
            dest_g = src_g * src_a / 255 + dest_g * dest_a * (255 - src_a) / 255 / 255;
            dest_b = src_b * src_a / 255 + dest_b * dest_a * (255 - src_a) / 255 / 255;
            dest_a = src_a + dest_a * (255 - src_a) / 255;
            putpixels[dest_surf->format->BytesPerPixel] (dest_surf,
                                                         x + dest_rect->x,
                                                         y + dest_rect->y,
                                                         SDL_MapRGBA
                                                         (dest_surf->format, dest_r, dest_g, dest_b, dest_a));
          }
        }
      }
    }
}

/**
 * FIXME
 */
static void load_info_about_label_surface(FILE *lfi)
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

#ifdef WIN32
  wchar_t *wtmpstr;
#endif
  char *tmpstr;

  /* Clear label surface */

  SDL_FillRect(label, NULL, SDL_MapRGBA(label->format, 0, 0, 0, 0));


  /* Clear all info related to label surface */

  delete_label_list(&start_label_node);
  start_label_node = current_label_node = first_label_node_in_redo_stack =
    highlighted_label_node = label_node_to_edit = NULL;
  have_to_rec_label_node = SDL_FALSE;


  if (lfi == NULL)
    return;


  /* Read count of label nodes: */
  tmp_fscanf_return = fscanf(lfi, "%d\n", &list_ctr);

  if (list_ctr <= 0)
  {
    fprintf(stderr, "Unexpected! Count of label notes is <= 0 (%d)!\n", list_ctr);
    fclose(lfi);
    return;
  }

  /* Read saved canvas width/height, so we can scale to the current canvas
     (in case it changed due to window size / fullscreen resolution changes,
     larger UI button size, etc. */
  tmp_fscanf_return = fscanf(lfi, "%d\n", &tmp_scale_w);
  tmp_fscanf_return = fscanf(lfi, "%d\n\n", &tmp_scale_h);
  (void)tmp_fscanf_return;

  if (tmp_scale_w <= 0 || tmp_scale_h <= 0)
  {
    fprintf(stderr, "Unexpected! Saved canvas dimensions %d x %d!\n", tmp_scale_w, tmp_scale_h);
    fclose(lfi);
    return;
  }

  /* Calculate canvas aspect ratios & such */
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

#ifdef WIN32
  wtmpstr = malloc(1024);
#endif
  tmpstr = malloc(1024);

  /* Read the labels' text: */

  for (k = 0; k < list_ctr; k++)
  {
    new_node = malloc(sizeof(struct label_node));

    new_node->save_texttool_len = atoi(fgets(tmpstr, 5, lfi));

    DEBUG_PRINTF("Reading %d wide chars\n", new_node->save_texttool_len);

    if (new_node->save_texttool_len >= 1024)
    {
      fprintf(stderr, "Unexpected! Saved text length is >= 1024 (%u!)\n", new_node->save_texttool_len);
      free(new_node);
#ifdef WIN32
      free(wtmpstr);
#endif
      free(tmpstr);
      fclose(lfi);
      return;
    }
    else
    {
#ifdef WIN32
      /* Using fancy "%[]" operator to scan until the end of a line */
      tmp_fscanf_return = fscanf(lfi, "%[^\n]\n", tmpstr);
      mbstowcs(wtmpstr, tmpstr, 1024);
      for (l = 0; l < new_node->save_texttool_len; l++)
        new_node->save_texttool_str[l] = wtmpstr[l];
      new_node->save_texttool_str[l] = L'\0';
#elif defined(__ANDROID__)
      wchar_t tmp_char;

      for (l = 0; l < new_node->save_texttool_len; l++)
      {
        fscanf(lfi, "%d ", &tmp_char);
        new_node->save_texttool_str[l] = tmp_char;
      }
      fscanf(lfi, "\n");
#else
      /* Using fancy "%[]" operator to scan until the end of a line */
      tmp_fscanf_return = fscanf(lfi, "%l[^\n]\n", new_node->save_texttool_str);
#endif

      DEBUG_PRINTF("Read: \"%ls\"\n", new_node->save_texttool_str);

      /* Read the label's color (RGB) */
      tmp_fscanf_return = fscanf(lfi, "%u\n", &l);
      new_node->save_color.r = (Uint8) l;
      tmp_fscanf_return = fscanf(lfi, "%u\n", &l);
      new_node->save_color.g = (Uint8) l;
      tmp_fscanf_return = fscanf(lfi, "%u\n", &l);
      new_node->save_color.b = (Uint8) l;

      /* Read the label's position */
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

      DEBUG_PRINTF("Original label size %dx%d\n", new_node->save_width, new_node->save_height);

      if (new_node->save_width > 8192 || new_node->save_height > 8192)
      {
        fprintf(stderr, "Unexpected! Save dimensions are (%u x %u!)\n", new_node->save_width, new_node->save_height);
        free(new_node);
        free(tmpstr);
#ifdef WIN32
        free(wtmpstr);
#endif
        fclose(lfi);
        return;
      }

      /* Read the label's font */
      tmp_fscanf_return = fscanf(lfi, "%d\n", &new_node->save_cur_font);
      /* FIXME: This seems wrong! -bjk 2022.04.02 */
      new_node->save_cur_font = 0;

      new_node->save_font_type = malloc(64);
      tmp_fgets_return = fgets(new_node->save_font_type, 64, lfi);
      (void)tmp_fgets_return;

      /* Read the label's state (italic &/or bold), and size */
      tmp_fscanf_return = fscanf(lfi, "%d\n", &new_node->save_text_state);
      tmp_fscanf_return = fscanf(lfi, "%u\n", &new_node->save_text_size);

      /* Read the bitmap data stored under the label */
      /* (The final PNG, when saved, includes the labels, as applied
         to the canvas. But we need to be able to edit/move/remove them,
         so we need to know what went _behind_ them) */
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

      /* Set the label's size, in proportion to any canvas size differences */
      new_text_size = (float)new_node->save_text_size * new_to_old_ratio;

      /* Scale the backbuffer, in proportion... */
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
      new_node->is_enabled = SDL_TRUE;
      new_node->disables = NULL;
      new_node->next_to_down_label_node = NULL;
      new_node->next_to_up_label_node = NULL;
      tmp_fscanf_return = fscanf(lfi, "\n");

      /* Link the labels together, for navigating between them */
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
  }

  first_label_node_in_redo_stack = NULL;
  fclose(lfi);

  free(tmpstr);
#ifdef WIN32
  free(wtmpstr);
#endif


  if (font_thread_done)
    set_label_fonts();
}

/**
 * FIXME
 */
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
      ttffont = (char *)TTF_FontFaceFamilyName(getfonthandle(i)->ttf_font);
      for (c = 0; c < strlen(ttffont); c++)
        if (ttffont[c] == '\n')
          ttffont[c] = '\0';
      for (c = 0; c < strlen(node->save_font_type); c++)
        if (node->save_font_type[c] == '\n')
          node->save_font_type[c] = '\0';

      DEBUG_PRINTF("ttffont A%sA\n", ttffont);
      DEBUG_PRINTF("font_type B%sB\n", node->save_font_type);

      if (strcmp(node->save_font_type, ttffont) == 0)

      {
        DEBUG_PRINTF("Font matched %s !!!\n", ttffont);
        node->save_cur_font = i;
        break;
      }
      else if (strstr(ttffont, node->save_font_type) || strstr(node->save_font_type, ttffont))
      {
        DEBUG_PRINTF("setting %s as replacement", TTF_FontFaceFamilyName(getfonthandle(i)->ttf_font));
        node->save_cur_font = i;
      }
    }

    if (node->save_cur_font > num_font_families)        /* This should never happens, setting default font. */
      node->save_cur_font = 0;

    free(node->save_font_type); /* Not needed anymore */
    node->save_font_type = NULL;
    node = node->next_to_down_label_node;
  }
}


/**
 * FIXME
 */
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
    current_label_node->is_enabled = SDL_FALSE;
    current_label_node->save_undoid = 253;

    derender_node(&label_node_to_edit);
  }
}

/**
 * FIXME
 */
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


/**
 * FIXME
 */
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
                   &rect1, SDL_MapRGBA(screen->format, 255, 4 * j * 225 / aux_node->save_height, 0, SDL_ALPHA_OPAQUE));

      SDL_BlitSurface(aux_node->label_node_surface, NULL, screen, &rect);
    }

    SDL_Flip(screen);
  }
}

/**
 * FIXME
 */
static void cycle_highlighted_label_node()
{
  struct label_node *aux_node;

  if (highlighted_label_node)
  {
    aux_node = highlighted_label_node->next_to_down_label_node;
    if (aux_node == NULL)
    {
      aux_node = current_label_node;
    }

    if (aux_node->is_enabled)
    {
      highlighted_label_node = aux_node;
    }
    else
    {
      while (aux_node->is_enabled == SDL_FALSE && aux_node != highlighted_label_node)
      {
        aux_node = aux_node->next_to_down_label_node;
        if (aux_node == NULL)
          aux_node = current_label_node;
        if (aux_node->is_enabled)
          highlighted_label_node = aux_node;
      }
    }
  }
}

/**
 * FIXME
 */
static int are_labels()
{
  struct label_node *aux_node;

  if (current_label_node)
  {
    aux_node = current_label_node;
    while (aux_node)
    {
      if (aux_node->is_enabled)
        return (SDL_TRUE);
      aux_node = aux_node->next_to_down_label_node;
    }
  }
  return (SDL_FALSE);
}

/**
 * FIXME
 */
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
          return (SDL_FALSE);   /* Avoid empty fields */
        fields++;
        if (fields == 4)
        {                       /* Last check, see if the sizes match */
          control = malloc(50);
          softwr = malloc(50);
          sscanf((char *)unknown.data, "%s\n%s\n%d\n%d\n", control, softwr, &unc_size, &comp);
          free(control);
          free(softwr);
          if (count + comp + 1 == unknown.size)
            return (SDL_TRUE);
          else
            return (SDL_FALSE);
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
          return (SDL_FALSE);

        new_field = 0;
      }
      count++;
    }
  }

  return (SDL_FALSE);
}

/**
 * FIXME
 */
Bytef *get_chunk_data(FILE *fp, char *fname, png_structp png_ptr,
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
      // printf("%d, %d, %d    ",i-count, comp_buff[i - count], unknown.data[i]);
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
    fprintf(stderr, "\n error %d, unc %d, comp %d\n", unc_err, *unc_size, comp);
    fclose(fp);
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
    free(comp_buff);
    free(unc_buff);

    fprintf(stderr,
            "Can't recover the embedded data in %s, error in uncompressing data from %s\n\n", fname, chunk_name);
    draw_tux_text(TUX_OOPS, strerror(errno), 0);
    return (NULL);
  }

  free(comp_buff);
  return (unc_buff);

}

/**
 * FIXME
 */
void load_embedded_data(char *fname, SDL_Surface *org_surf)
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

  DEBUG_PRINTF("Loading embedded data...\n");
  DEBUG_PRINTF("%s\n", fname);

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
    DEBUG_PRINTF("%s\n", fname);

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

    DEBUG_PRINTF("num_unknowns %i\n", num_unknowns);

    if (num_unknowns)
    {
      have_label_delta = have_label_data = have_background = have_foreground = SDL_FALSE;
      ldelta = ldata = fgnd = bgnd = SDL_FALSE;

      /* Need to get things in order, as we can't enforce any order in custom chunks,
         we need to go around them 3 times */

      /* First we search for the things that usually were in the .dat file, so if a starter or a
         template is found and if it is not modified, we can load it clean (i.e. not rebluring a
         blured when scaled one) */
      for (u = 0; u < num_unknowns; u++)
      {
        DEBUG_PRINTF("%s, %d\n", unknowns[u].name, (int)unknowns[u].size);

        if (chunk_is_valid("tpDT", unknowns[u]))
        {
          DEBUG_PRINTF("Valid tpDT\n");

          fi = fmemopen(unknowns[u].data, unknowns[u].size, "r");
          if (fi == NULL)
          {
            fclose(fp);
            png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);

            fprintf(stderr, "\nError: Couldn't load the data embedded in %s\n\n", fname);
            draw_tux_text(TUX_OOPS, strerror(errno), 0);
            SDL_FreeSurface(org_surf);
            return;             /* Refusing to go further with the other chunks */
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
          have_background = SDL_TRUE;
        if (chunk_is_valid("tpFG", unknowns[u]))
          have_foreground = SDL_TRUE;
        if (chunk_is_valid("tpLD", unknowns[u]))
          have_label_delta = SDL_TRUE;
        if (chunk_is_valid("tpLL", unknowns[u]))
          have_label_data = SDL_TRUE;
      }

      /* Recover the labels and apply the diff from label to canvas. */
      if (!disable_label && have_label_delta && have_label_data)
      {
        for (u = 0; u < num_unknowns; u++)
        {
          if (chunk_is_valid("tpLD", unknowns[u]))
          {
            DEBUG_PRINTF("Valid tpLD\n");

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
                    putpixels[org_surf->format->BytesPerPixel] (org_surf, i,
                                                                j,
                                                                SDL_MapRGB
                                                                (org_surf->format,
                                                                 unc_buff[4 *
                                                                          (j *
                                                                           ww
                                                                           +
                                                                           i)],
                                                                 unc_buff[4 *
                                                                          (j *
                                                                           ww
                                                                           + i) + 1], unc_buff[4 * (j * ww + i) + 2]));
                }
            }

            SDL_UnlockSurface(org_surf);

            free(unc_buff);
            ldelta = SDL_TRUE;
          }

          /* Label Data */
          if (!disable_label && chunk_is_valid("tpLL", unknowns[u]))
          {
            DEBUG_PRINTF("Valid tpLL\n");

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
                fprintf(stderr, "Can't recover the label data embedded in %s, error in create file stream\n\n", fname);
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
            ldata = SDL_TRUE;
            DEBUG_PRINTF("Out of label data\n");
          }
        }
      }

      /* Apply the original canvas */
      if (ldelta && ldata)
      {
        DEBUG_PRINTF("Smearing org_surf @ 9\n");
        autoscale_copy_smear_free(org_surf, canvas, SDL_BlitSurface);
      }
      else
      {
        SDL_FreeSurface(org_surf);
      }

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
#ifdef DEBUG
              fprintf(stderr, "Can't recover the background data embedded in %s, error in create aux image\n\n", fname);
#endif
              fclose(fp);
              png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
              free(unc_buff);

              draw_tux_text(TUX_OOPS, strerror(errno), 0);

              return;
            }
            SDL_LockSurface(aux_surf);

            DEBUG_PRINTF("Bkgd!!!\n");
            for (j = 0; j < hh; j++)
              for (i = 0; i < ww; i++)
                putpixels[aux_surf->format->BytesPerPixel] (aux_surf, i, j,
                                                            SDL_MapRGB
                                                            (aux_surf->format,
                                                             unc_buff[3 * j *
                                                                      ww +
                                                                      3 * i],
                                                             unc_buff[3 * j *
                                                                      ww +
                                                                      3 * i + 1], unc_buff[3 * j * ww + 3 * i + 2]));
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

              /* FIXME: How to handle starter/template scaling/smearing
                 options!? -bjk 2023.02.10 */

              DEBUG_PRINTF("Smearing embedded bkgd @ 10\n");
              autoscale_copy_smear_free(aux_surf, img_starter_bkgd, SDL_BlitSurface);
            }
            free(unc_buff);
          }

          if ((starter_modified || !img_starter) && chunk_is_valid("tpFG", unknowns[u]))
          {
            DEBUG_PRINTF("Frgd!!!\n");

            unc_buff = get_chunk_data(fp, fname, png_ptr, info_ptr, "tpFG", unknowns[u], &unc_size);
            if (unc_buff == NULL)
              return;

            aux_surf = SDL_CreateRGBSurface(canvas->flags, ww, hh,
                                            canvas->format->BitsPerPixel,
                                            canvas->format->Rmask,
                                            canvas->format->Gmask, canvas->format->Gmask, TPAINT_AMASK);
            if (aux_surf == NULL)
            {
              fprintf(stderr, "Can't recover the foreground data embedded in %s, error in create aux image\n\n", fname);
              fclose(fp);
              png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
              free(unc_buff);

              draw_tux_text(TUX_OOPS, strerror(errno), 0);

              return;
            }

            SDL_LockSurface(aux_surf);
            for (j = 0; j < hh; j++)
              for (i = 0; i < ww; i++)
              {
                putpixels[aux_surf->format->BytesPerPixel] (aux_surf, i, j,
                                                            SDL_MapRGBA
                                                            (aux_surf->format,
                                                             unc_buff[4 * j *
                                                                      ww +
                                                                      4 * i],
                                                             unc_buff[4 * j *
                                                                      ww +
                                                                      4 * i +
                                                                      1],
                                                             unc_buff[4 * j *
                                                                      ww +
                                                                      4 * i + 2], unc_buff[4 * j * ww + 4 * i + 3]));
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

            /* FIXME: How to handle starter/template scaling/smearing
               options!? -bjk 2023.02.10 */
            DEBUG_PRINTF("Smearing embedded foreground @ 11\n");
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

#if !defined(WIN32) && !defined(__APPLE__) && !defined(__BEOS__) && !defined(__HAIKU__) && !defined(__ANDROID__)
/**
 * FIXME
 */
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

/**
 * FIXME
 */
static void parse_file_options(struct cfginfo *restrict tmpcfg, const char *filename)
{
  char str[256];
  char *arg;
  FILE *fi = fopen(filename, "r");
  int line;

  if (!fi)
    return;

  line = 0;

  while (fgets(str, sizeof(str), fi))
  {
    line++;

    strip_trailing_whitespace(str);

    /* Comments and blank lines can be safely ignored */
    if (str[0] == '#' || str[0] == '\0')
      continue;

    /* Expecting alphanumeric at the beginning of a line; ignore (and complain about) the rest */
    if (!isalnum(*str))
    {
      fprintf(stderr, "Warning: do not understand '%s' on line %d of '%s'\n", str, line, filename);
      continue;
    }

    /* Expecting to be in the form of OPTION=ARG; ignore (and complain about) the rest */
    arg = strchr(str, '=');
    if (arg)
    {
      *arg++ = '\0';
    }
    else
    {
      fprintf(stderr, "Warning: do not understand '%s' on line %d of '%s'\n", str, line, filename);
      continue;
    }

#ifdef __linux__
#ifndef __ANDROID__
    wordexp_t result;

    wordexp(arg, &result, 0);
    if (result.we_wordv != NULL)
    {
      DEBUG_PRINTF("wordexp result.we_wordv of '%s' was '%s'\n", str, result.we_wordv[0]);
      arg = strdup(result.we_wordv[0]);
      wordfree(&result);
    }
    else
    {
      fprintf(stderr,
              "Shell expansion of '%s' on line %d of '%s' failed! (You probably need to wrap it in quotes (\")!)\n",
              str, line, filename);
      continue;
    }
#endif
#endif

    /* wordexp() on Linux (used above) will strip quotes, but on other
       platforms we'll want to do it ourselves */
    strip_quotes(arg);

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

/**
 * FIXME
 */
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

/**
 * FIXME
 */
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

/**
 * FIXME
 */
static void setup_config(char *argv[])
{
  char str[128];
  char *picturesdir;
  char *tp_ui_font_fallback;
  int i;

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

  /* == SAVEDIR == */
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

    result = find_directory(B_USER_SETTINGS_DIRECTORY, volume, false, buffer, sizeof(buffer));
    asprintf((char **)&savedir, "%s/%s", buffer, "TuxPaint");
#elif __APPLE__
    savedir = strdup(apple_preferencesPath());
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

  /* == EXPORTDIR == */
  if (tmpcfg_cmd.exportdir)
    exportdir = strdup(tmpcfg_cmd.exportdir);
  else
  {
    /* FIXME: Need assist for:
     * __BEOS__
     * __HAIKU__
     */
#ifdef WIN32
    picturesdir = GetUserImageDir();
#elif __APPLE__
    picturesdir = strdup(apple_picturesPath());
#elif __ANDROID__
    picturesdir = strdup(SDL_AndroidGetExternalStoragePath());
    char *substring = strstr(picturesdir, "/Android");

    if (substring != NULL)
    {
      strcpy(substring, "/Pictures");
    }
#else
    picturesdir = get_xdg_user_dir("PICTURES", "Pictures");
#endif
    safe_snprintf(str, sizeof(str), "%s/TuxPaint", picturesdir);
    free(picturesdir);
    exportdir = strdup(str);
  }

  /* Load options from user's own configuration (".rc" / ".cfg") file: */

#if defined(_WIN32)
  /* Default local config file in users savedir directory on Windows */
  safe_snprintf(str, sizeof(str), "%s/tuxpaint.cfg", savedir);  /* FIXME */
#elif defined(__BEOS__) || defined(__HAIKU__)
  /* BeOS: Use a "tuxpaint.cfg" file: */
  safe_snprintf(str, sizeof(str), "%s/config/settings/TuxPaint/tuxpaint.cfg", home);
#elif defined(__APPLE__)
  /* macOS, iOS: Use a "tuxpaint.cfg" file in the Tux Paint application support folder */
  safe_snprintf(str, sizeof(str), "%s/tuxpaint.cfg", apple_preferencesPath());
#elif defined(__ANDROID__)
  /* Try to find the user's config file */
  /* This file is writed by the tuxpaint config activity when the user runs it */
  safe_snprintf(str, sizeof(str), "%s/tuxpaint.cfg", SDL_AndroidGetExternalStoragePath());


#else
  /* Linux and other Unixes:  Use 'rc' style (~/.tuxpaintrc) */
  /* it should it be "~/.tuxpaint/tuxpaintrc" instead, but too late now */
  safe_snprintf(str, sizeof(str), "%s/.tuxpaintrc", home);
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
    /* macOS, iOS: Use a "tuxpaint.cfg" file in the *global* Tux Paint
       application support folder */
    safe_snprintf(str, sizeof(str), "%s/tuxpaint.cfg", apple_globalPreferencesPath());
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

  exportdir = tmpcfg.exportdir ? tmpcfg.exportdir : exportdir;

  if (tmpcfg.parsertmp_lang == PARSE_CLOBBER)
    tmpcfg.parsertmp_lang = NULL;
  if (tmpcfg.parsertmp_locale == PARSE_CLOBBER)
    tmpcfg.parsertmp_locale = NULL;

  setup_i18n(tmpcfg.parsertmp_lang, tmpcfg.parsertmp_locale, &num_wished_langs);

  /* Determine the referred font for the current locale */
  PANGO_DEFAULT_FONT_FALLBACK = NULL;
  for (i = 0; default_local_fonts[i].locale_id != -1; i++)
  {
    if (default_local_fonts[i].locale_id == get_current_language())
    {
      PANGO_DEFAULT_FONT = default_local_fonts[i].font_name;
      PANGO_DEFAULT_FONT_FALLBACK = default_local_fonts[i].font_name_fallback;
    }
  }

  tp_ui_font_fallback = NULL;

  char *tmp_str;
  FcBool fontAddStatus;
  char locale_fontdir[MAX_PATH];

  if (tmpcfg.tp_ui_font)
  {
    if (strcmp(tmpcfg.tp_ui_font, "default") == 0)
    {
      printf /*DEBUG_PRINTF */ ("Info: Requested default UI font, \"%s\"\n",
                                PANGO_DEFAULT_FONT);
      tp_ui_font = strdup(PANGO_DEFAULT_FONT);
      if (PANGO_DEFAULT_FONT_FALLBACK != NULL)
      {
        tp_ui_font_fallback = strdup(PANGO_DEFAULT_FONT_FALLBACK);
      }
    }
    else
    {
      tp_ui_font = strdup(tmpcfg.tp_ui_font);
      printf                    /*DEBUG_PRINTF */
        ("Info: Requested UI font described by \"%s\"\n", tp_ui_font);
    }
  }
  else
  {
    printf /*DEBUG_PRINTF */ ("Info: Requested default UI font, \"%s\"\n",
                              PANGO_DEFAULT_FONT);
    tp_ui_font = strdup(PANGO_DEFAULT_FONT);
    if (PANGO_DEFAULT_FONT_FALLBACK != NULL)
    {
      tp_ui_font_fallback = strdup(PANGO_DEFAULT_FONT_FALLBACK);
    }
  }

  /* Add Tux Paint's own set of fonts to FontConfig,
     so SDL2_Pango can find and use them */
  snprintf(locale_fontdir, sizeof(locale_fontdir), "%s/fonts", DATA_PREFIX);

  fontAddStatus = FcConfigAppFontAddDir(FcConfigGetCurrent(), (const FcChar8 *)locale_fontdir);
  if (fontAddStatus == FcFalse)
  {
    fprintf(stderr, "Unable to add font dir %s\n", locale_fontdir);
  }

  /* FIXME: Unclear whether this is necessary? -bjk 2023.06.12 */
  DEBUG_PRINTF("Rescanning fonts...");
  fflush(stdout);
  FcDirCacheRead((const FcChar8 *)locale_fontdir, FcTrue /* force */ ,
                 FcConfigGetCurrent());
  FcDirCacheRescan((const FcChar8 *)locale_fontdir, FcConfigGetCurrent());
  DEBUG_PRINTF("done\n");

#ifdef DEBUG
  {
    FcStrList *str_list;
    FcChar8 *path;

    str_list = FcConfigGetFontDirs(FcConfigGetCurrent());
    printf("FcConfigGetFontDirs():\n");
    while ((path = FcStrListNext(str_list)) != NULL)
    {
      printf(" * %s\n", (const char *)path);
    }
    printf("\n");
  }
#endif


  tmp_str = ask_pango_for_font(tp_ui_font);
  if (tmp_str != NULL)
  {
    if (strcmp(tp_ui_font, tmp_str) != 0 && tp_ui_font_fallback != NULL)
    {
      free(tp_ui_font);
      tp_ui_font = strdup(tp_ui_font_fallback);
      tp_ui_font_fallback = NULL;

      printf                    /*DEBUG_PRINTF */
        ("Info: Requested fallback default UI font, \"%s\"\n", tp_ui_font);
      tmp_str = ask_pango_for_font(tp_ui_font);
    }
  }

  if (tmp_str != NULL)
  {
    printf("Info: Actual UI font will be \"%s\"\n", tmp_str);
    free(tmp_str);
  }
  else
  {
    printf("Error: Problem asking pango for actual font!\n");
  }

  /* FIXME: most of this is not required before starting the font scanner */

#ifdef PAPER_H
  paperinit();

  if (tmpcfg_cmd.papersize && !strcmp(tmpcfg_cmd.papersize, "help"))
    show_available_papersizes(0);
#endif

#define SETBOOL(x) do{ if(tmpcfg.x) x = (tmpcfg.x==PARSE_YES); }while(0)
  SETBOOL(all_locale_fonts);
  SETBOOL(autosave_on_quit);
  SETBOOL(reversesort);
  SETBOOL(disable_label);
  SETBOOL(disable_brushspacing);
  SETBOOL(disable_magic_controls);
  SETBOOL(disable_magic_sizes);
  SETBOOL(no_magic_groups);
  SETBOOL(disable_shape_controls);
  SETBOOL(disable_print);
  SETBOOL(disable_quit);
  SETBOOL(disable_save);
  SETBOOL(disable_erase);
  SETBOOL(disable_screensaver);
  SETBOOL(disable_stamp_controls);
  SETBOOL(disable_template_export);
  SETBOOL(dont_do_xor);
  SETBOOL(dont_load_stamps);
  SETBOOL(fullscreen);
  SETBOOL(grab_input);
  SETBOOL(hide_cursor);
  SETBOOL(keymouse);
  SETBOOL(mirrorstamps);
  SETBOOL(no_stamp_rotation);
  SETBOOL(native_screensize);
  SETBOOL(new_colors_last);
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
  SETBOOL(use_stereo);
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
  if (tmpcfg.button_size)
  {
    if (strstr(tmpcfg.button_size, "auto"))
      button_size_auto = 1;
    else
    {
      if (strtof(tmpcfg.button_size, NULL) < 24 || strtof(tmpcfg.button_size, NULL) > 192)
      {
        fprintf(stderr, "Button size (now %s) must be between 24 and 192.\n", tmpcfg.button_size);
        exit(1);
      }
      button_scale = (float)strtof(tmpcfg.button_size, NULL) / (float)ORIGINAL_BUTTON_SIZE;
      DEBUG_PRINTF("Button size %s requested = %d (scale = %f)\n",
                   tmpcfg.button_size, (int)(button_scale * ORIGINAL_BUTTON_SIZE), button_scale);
    }
  }
  else
  {
    button_scale = 1;
  }
  if (tmpcfg.colors_rows)
  {
    if (strtof(tmpcfg.colors_rows, NULL) > 3)
    {
      fprintf(stderr, "Color rows (now %s) must be between 1 and 3.\n", tmpcfg.colors_rows);
      exit(1);
    }
    colors_rows = strtof(tmpcfg.colors_rows, NULL);
  }
  else
    colors_rows = 1;
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
  if (tmpcfg.complexity)
  {
    /* FIXME: Could maybe iterate the array of MAGIC_COMPLEXITY_LEVEL_NAMES[],
       but just hard-coding for now -bjk 2023.12.29 */
    if (!strcmp(tmpcfg.complexity, "novice"))
    {
      magic_complexity_level = MAGIC_COMPLEXITY_NOVICE;
    }
    else if (!strcmp(tmpcfg.complexity, "beginner"))
    {
      magic_complexity_level = MAGIC_COMPLEXITY_BEGINNER;
    }
    else if (!strcmp(tmpcfg.complexity, "advanced"))
    {
      magic_complexity_level = MAGIC_COMPLEXITY_ADVANCED;
    }
    else
    {
      fprintf(stderr, "Ignoring unknown 'complexity' value \"%s\"\n", tmpcfg.complexity);
    }
  }

  /* FIXME: make this dynamic (accelerometer or OLPC XO-1 rotation button) */
  if (tmpcfg.rotate_orientation)
    rotate_orientation = !strcmp(tmpcfg.rotate_orientation, "portrait");        /* alternative is "landscape" */
  if (tmpcfg.colorfile)
    safe_strncpy(colorfile, tmpcfg.colorfile, sizeof(colorfile));
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
      alt_print_command_default = ALTPRINT_MOD; /* default ("mod") */
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
        fprintf(stderr, "Joystick dev (now %s) must be between 0 and 100.\n", tmpcfg.joystick_dev);
        exit(1);
      }
      joystick_dev = strtof(tmpcfg.joystick_dev, NULL);
    }
  }
  if (tmpcfg.joystick_slowness)
  {
    if (strtof(tmpcfg.joystick_slowness, NULL) < 0 || strtof(tmpcfg.joystick_slowness, NULL) > 500)
    {
      fprintf(stderr, "Joystick slowness (now %s) must be between 0 and 500.\n", tmpcfg.joystick_slowness);
      exit(1);
    }
    joystick_slowness = strtof(tmpcfg.joystick_slowness, NULL);
  }
  if (tmpcfg.joystick_lowthreshold)
  {
    if (strtof(tmpcfg.joystick_lowthreshold, NULL) < 0 || strtof(tmpcfg.joystick_lowthreshold, NULL) > 32766)
    {
      /* FIXME: Find better exit code */
      fprintf(stderr, "Joystick lower threshold (now %s)  must be between 0 and 32766", tmpcfg.joystick_lowthreshold);
      exit(1);
    }
    joystick_low_threshold = strtof(tmpcfg.joystick_lowthreshold, NULL);
  }
  if (tmpcfg.joystick_maxsteps)
  {
    if (strtof(tmpcfg.joystick_maxsteps, NULL) < 1 || strtof(tmpcfg.joystick_maxsteps, NULL) > 7)
    {
      /* FIXME: Find better exit code */
      fprintf(stderr, "Joystick max steps (now %s)  must be between 1 and 7", tmpcfg.joystick_maxsteps);
      exit(1);
    }
    joystick_maxsteps = strtof(tmpcfg.joystick_maxsteps, NULL);
  }
  if (tmpcfg.joystick_hat_slowness)
  {
    if (strtof(tmpcfg.joystick_hat_slowness, NULL) < 0 || strtof(tmpcfg.joystick_hat_slowness, NULL) > 500)
    {
      fprintf(stderr, "Joystick hat slowness (now %s) must be between 0 and 500.\n", tmpcfg.joystick_hat_slowness);
      exit(1);
    }
    joystick_hat_slowness = strtof(tmpcfg.joystick_hat_slowness, NULL);
  }
  if (tmpcfg.joystick_hat_timeout)
  {
    if (strtof(tmpcfg.joystick_hat_timeout, NULL) < 0 || strtof(tmpcfg.joystick_hat_timeout, NULL) > 3000)
    {
      /* FIXME: Find better exit code */
      fprintf(stderr, "Joystick hat timeout (now %s)  must be between 0 and 3000", tmpcfg.joystick_hat_timeout);
      exit(1);
    }
    joystick_hat_timeout = strtof(tmpcfg.joystick_hat_timeout, NULL);
  }
  if (tmpcfg.joystick_button_escape)
  {
    if (strtof(tmpcfg.joystick_button_escape, NULL) < 0 || strtof(tmpcfg.joystick_button_escape, NULL) > 254)
    {
      /* FIXME: Find better exit code */
      fprintf(stderr,
              "Joystick button escape shortcurt (now %s)  must be between 0 and 254", tmpcfg.joystick_button_escape);
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
      fprintf(stderr,
              "Joystick button brush tool shortcurt (now %s)  must be between 0 and 254",
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
      fprintf(stderr,
              "Joystick button stamp tool shortcurt (now %s)  must be between 0 and 254",
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
      fprintf(stderr,
              "Joystick button lines tool shortcurt (now %s)  must be between 0 and 254",
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
      fprintf(stderr,
              "Joystick button shapes tool shortcurt (now %s)  must be between 0 and 254",
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
      fprintf(stderr,
              "Joystick button text tool shortcurt (now %s)  must be between 0 and 254",
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
      fprintf(stderr,
              "Joystick button label tool shortcurt (now %s)  must be between 0 and 254",
              tmpcfg.joystick_button_selectlabeltool);
      exit(1);
    }
    joystick_button_selectlabeltool = strtof(tmpcfg.joystick_button_selectlabeltool, NULL);
  }
  if (tmpcfg.joystick_button_selectfilltool)
  {
    if (strtof(tmpcfg.joystick_button_selectfilltool, NULL) < 0
        || strtof(tmpcfg.joystick_button_selectfilltool, NULL) > 254)
    {
      /* FIXME: Find better exit code */
      fprintf(stderr,
              "Joystick button fill tool shortcurt (now %s)  must be between 0 and 254",
              tmpcfg.joystick_button_selectfilltool);
      exit(1);
    }
    joystick_button_selectfilltool = strtof(tmpcfg.joystick_button_selectfilltool, NULL);
  }
  if (tmpcfg.joystick_button_selectmagictool)
  {
    if (strtof(tmpcfg.joystick_button_selectmagictool, NULL) < 0
        || strtof(tmpcfg.joystick_button_selectmagictool, NULL) > 254)
    {
      /* FIXME: Find better exit code */
      fprintf(stderr,
              "Joystick button magic tool shortcurt (now %s)  must be between 0 and 254",
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
      fprintf(stderr,
              "Joystick button undo shortcurt (now %s)  must be between 0 and 254", tmpcfg.joystick_button_undo);
      exit(1);
    }
    joystick_button_undo = strtof(tmpcfg.joystick_button_undo, NULL);
  }
  if (tmpcfg.joystick_button_redo)
  {
    if (strtof(tmpcfg.joystick_button_redo, NULL) < 0 || strtof(tmpcfg.joystick_button_redo, NULL) > 254)
    {
      /* FIXME: Find better exit code */
      fprintf(stderr,
              "Joystick button redo shortcurt (now %s)  must be between 0 and 254", tmpcfg.joystick_button_redo);
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
      fprintf(stderr,
              "Joystick button eraser tool shortcurt (now %s)  must be between 0 and 254",
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
      fprintf(stderr, "Joystick button new shortcurt (now %s)  must be between 0 and 254", tmpcfg.joystick_button_new);
      exit(1);
    }
    joystick_button_new = strtof(tmpcfg.joystick_button_new, NULL);
  }
  if (tmpcfg.joystick_button_open)
  {
    if (strtof(tmpcfg.joystick_button_open, NULL) < 0 || strtof(tmpcfg.joystick_button_open, NULL) > 254)
    {
      /* FIXME: Find better exit code */
      fprintf(stderr,
              "Joystick button open shortcurt (now %s)  must be between 0 and 254", tmpcfg.joystick_button_open);
      exit(1);
    }
    joystick_button_open = strtof(tmpcfg.joystick_button_open, NULL);
  }
  if (tmpcfg.joystick_button_save)
  {
    if (strtof(tmpcfg.joystick_button_save, NULL) < 0 || strtof(tmpcfg.joystick_button_save, NULL) > 254)
    {
      /* FIXME: Find better exit code */
      fprintf(stderr,
              "Joystick button save shortcurt (now %s)  must be between 0 and 254", tmpcfg.joystick_button_save);
      exit(1);
    }
    joystick_button_save = strtof(tmpcfg.joystick_button_save, NULL);
  }
  if (tmpcfg.joystick_button_pagesetup)
  {
    if (strtof(tmpcfg.joystick_button_pagesetup, NULL) < 0 || strtof(tmpcfg.joystick_button_pagesetup, NULL) > 254)
    {
      /* FIXME: Find better exit code */
      fprintf(stderr,
              "Joystick button page setup shortcurt (now %s)  must be between 0 and 254",
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
      fprintf(stderr,
              "Joystick button print shortcurt (now %s)  must be between 0 and 254", tmpcfg.joystick_button_print);
      exit(1);
    }
    joystick_button_print = strtof(tmpcfg.joystick_button_print, NULL);
  }
  if (tmpcfg.joystick_buttons_ignore)
  {
    char *token;

    token = strtok((char *)tmpcfg.joystick_buttons_ignore, ",");
    while (token != NULL)
    {
      if (strtof(token, NULL) < 0 || strtof(token, NULL) > 254)
      {
        /* FIXME: Find better exit code */
        fprintf(stderr, "Joystick buttons must be between 0 and 254 (don't like %s)", tmpcfg.joystick_buttons_ignore);
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
    onscreen_keyboard = SDL_TRUE;
  }

  if (tmpcfg.onscreen_keyboard_disable_change)
  {
    onscreen_keyboard = SDL_TRUE;
  }

  DEBUG_PRINTF("\n\nPromptless save:\nask: %d\nnew: %d\nover: %d\n\n",
               _promptless_save_over_ask, _promptless_save_over_new, _promptless_save_over);

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


/**
 * FIXME
 */
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

#if defined(__MACOS__)
    // On macOS, execution is deep inside the app bundle.
    // E.g., "/Applications/TuxPaint.app/Contents/MacOS/tuxpaint"
    // But we want to point somewhere higher up, say to "Contents", so we can access
    // the resources in Resources folder. So move up one level.
    int levels = 1;             /* we need to back up 1 level */

    while ((levels-- > 0) && (slash))
    {
      *slash = '\0';            /* this overwrites the \0 at end of string */
      slash = strrchr(app_path, '/');   /* so we can carry on our back-pedaling... */
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

/**
 * FIXME
 */
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

            /* FIXME: This and get_starter_template_options() needs to be modularized -bjk 2023.02.10 */

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


  /* Add room for dynamic color options at the end of the list: */

  color_hexes = (Uint8 **) realloc(color_hexes, sizeof(Uint8 *) * (NUM_COLORS + 3));
  color_names = (char **)realloc(color_names, sizeof(char *) * (NUM_COLORS + 3));

  /* Add "Color Select" color: */

  color_names[NUM_COLORS] = strdup(gettext("Select a color from your drawing."));
  color_hexes[NUM_COLORS] = (Uint8 *) malloc(sizeof(Uint8) * 3);
  color_hexes[NUM_COLORS][0] = 0;
  color_hexes[NUM_COLORS][1] = 0;
  color_hexes[NUM_COLORS][2] = 0;
  NUM_COLORS++;

  /* Add "Color Picker" color: */
  /* (This is an attempt to describe an HSV color picker in extremely basic terms!) */
  color_names[NUM_COLORS] =
    strdup(gettext
           ("Pick a color. Hues go top to bottom. Saturation/intensity goes left (pale) to right (pure). Value (lightness/darkness): grey bar."));
  color_hexes[NUM_COLORS] = (Uint8 *) malloc(sizeof(Uint8) * 3);
  color_hexes[NUM_COLORS][0] = 255;
  color_hexes[NUM_COLORS][1] = 255;
  color_hexes[NUM_COLORS][2] = 255;
  color_picker_x = 0;           /* Saturation */
  color_picker_y = 0;           /* Hue */
  color_picker_v = 0;           /* Value */
  NUM_COLORS++;

  /* Add "Color Mixer" color: */
  /* (The terms 'tint', 'tone', and 'shade' relate to combining white, grey, or black paint (respectively) to another color) */
  color_names[NUM_COLORS] =
    strdup(gettext
           ("Click the primary colors (red, yellow, and blue), white (to tint), grey (to tone), and black (to shade), to mix together a new color."));
  color_hexes[NUM_COLORS] = (Uint8 *) malloc(sizeof(Uint8) * 3);
  color_hexes[NUM_COLORS][0] = 255;
  color_hexes[NUM_COLORS][1] = 255;
  color_hexes[NUM_COLORS][2] = 255;
  color_mixer_reset = 1;
  NUM_COLORS++;
}

/* ================================================================================== */

/**
 * FIXME
 */
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

int TP_EventFilter( __attribute__((unused))
                   void *data, EVENT_FILTER_EVENT_TYPE *event)
/**
 * FIXME
 */
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
      event->type == SDL_APP_DIDENTERBACKGROUND ||
      event->type == SDL_APP_DIDENTERFOREGROUND || event->type == TP_USEREVENT_PLAYDESCSOUND)
    return 1;

  return 0;
}

/* ================================================================================== */

/**
 * FIXME
 */
static void setup(void)
{
  int i, j;
  int ww, hh;
  char *upstr;
  SDL_Color black = { 0, 0, 0, 0 };
  char *homedirdir;
  SDL_Surface *tmp_surf;
  SDL_Rect dest;
  int scale;
  int canvas_width, canvas_height;
  int win_x = SDL_WINDOWPOS_UNDEFINED, win_y = SDL_WINDOWPOS_UNDEFINED;
  int x, y;
  SDL_Surface *tmp_btn_up;
  SDL_Surface *tmp_btn_down;
  Uint8 r, g, b;
  SDL_Surface *tmp_imgcurup, *tmp_imgcurdown;
  Uint32 init_flags;
  char tmp_str[128];
  SDL_Surface *img1;

  Uint32(*getpixel_tmp_btn_up) (SDL_Surface *, int, int);
  Uint32(*getpixel_tmp_btn_down) (SDL_Surface *, int, int);
  Uint32(*getpixel_img_paintwell) (SDL_Surface *, int, int);
  int big_title;
  SDL_Thread *fontconfig_thread;

  render_scale = 1.0;


#ifdef _WIN32
  if (fullscreen)
  {
    InstallKeyboardHook();
    SetActivationState(1);
  }
#endif

  im_init(&im_data, get_current_language());

  SDLPango_Init();

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

  init_flags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER;
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

  /* Clicking into window while unfocused would not let
     the clicks through, which was unlike how things worked
     in SDL1.2, and I found that annoying -bjk 2022.09.15 */
  SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

  /* Set up event filter */

  SDL_SetEventFilter(TP_EventFilter, NULL);


  /* Set up joystick */

  if (joystick_dev == -1)
  {
    fprintf(stderr, "%i joystick(s) were found:\n", SDL_NumJoysticks());

    for (i = 0; i < SDL_NumJoysticks(); i++)
    {
      SDL_Joystick *joystick = SDL_JoystickOpen(i);

      fprintf(stderr, " %d: %s\n", i, SDL_JoystickName(joystick));
    }

    SDL_Quit();
    exit(0);
  }

  joystick = SDL_JoystickOpen(joystick_dev);
  if (joystick == NULL)
  {
    fprintf(stderr, "Info: Could not open joystick device %d: %s\n", joystick_dev, SDL_GetError());
  }
  else
  {
    SDL_JoystickEventState(SDL_ENABLE);
    DEBUG_PRINTF("Number of Axes: %d\n", SDL_JoystickNumAxes(joystick));
    DEBUG_PRINTF("Number of Buttons: %d\n", SDL_JoystickNumButtons(joystick));
    DEBUG_PRINTF("Number of Balls: %d\n", SDL_JoystickNumBalls(joystick));
    DEBUG_PRINTF("Number of Hats: %d\n", SDL_JoystickNumHats(joystick));
  }


#ifndef NOSOUND
  DEBUG_PRINTF("Initializing sound...\n");
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


  DEBUG_PRINTF("Enabling key repeat...\n");

  /* Set-up Key-Repeat: */
  //  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  DEBUG_PRINTF("Initializing TTF...\n");

  /* Init TTF stuff: */
  if (TTF_Init() < 0)
  {
    fprintf(stderr,
            "\nError: I could not initialize the font (TTF) library!\n"
            "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

    SDL_Quit();
    exit(1);
  }


  DEBUG_PRINTF("Setting up colors...\n");
  setup_colors();


  /* Deal with orientation rotation option */
#if defined __ANDROID__
  SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeRight");
#endif

  if (rotate_orientation)
  {
    if (native_screensize && fullscreen)
    {
#if defined __ANDROID__
      SDL_SetHint(SDL_HINT_ORIENTATIONS, "Portrait");
#else
      fprintf(stderr, "Warning: Asking for native screen size overrides request to rotate orientation.\n");
#endif
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


  /* SDL1.2 supported "SDL_VIDEO_WINDOW_POS", but SDL2 does not,
     so we implement it ourselves */
  if (getenv((char *)"SDL_VIDEO_WINDOW_POS") != NULL)
  {
    char *winpos;

    winpos = getenv((char *)"SDL_VIDEO_WINDOW_POS");
    if (strcmp(winpos, "nopref") != 0)
    {
      if (strcmp(winpos, "center") == 0)
      {
        win_x = SDL_WINDOWPOS_CENTERED;
        win_y = SDL_WINDOWPOS_CENTERED;
      }
      else
      {
        int success;

        success = sscanf(winpos, "%d,%d", &win_x, &win_y);
        if (success != 2)
        {
          fprintf(stderr, "Warning: Cannot parse SDL_VIDEO_WINDOW_POS value of \"%s\"; ignoring\n", winpos);
          win_x = SDL_WINDOWPOS_UNDEFINED;
          win_y = SDL_WINDOWPOS_UNDEFINED;
        }
      }
    }
  }


  /* Open Window: */

  if (fullscreen)
  {
#ifdef USE_HWSURFACE
    window_screen =
      SDL_CreateWindow("Tux Paint", win_x, win_y,
                       WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_HWSURFACE);
    if (window_screen == NULL)
      printf("Warning: Cannot open fullscreen with hardware surface\n");

#else
    window_screen = SDL_CreateWindow(NULL, win_x, win_y, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (window_screen == NULL)
      printf("Warning: Cannot open fullscreen with software surface\n");
#endif

    /* FIXME: Check window_screen for being NULL, and abort?! (Also see below) -bjk 2024.12.20 */

    renderer = SDL_CreateRenderer(window_screen, -1, 0);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

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
#if defined __ANDROID__
      /* Deal with rotate orientation in native screens */
      if (rotate_orientation)
      {
        if (ww > hh)
        {
          int tmp;

          tmp = ww;
          ww = hh;
          hh = tmp;
        }
      }
      else
      {
        if (hh > ww)
        {
          int tmp;

          tmp = ww;
          ww = hh;
          hh = tmp;
        }
      }
#endif
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
    int max_scrn_w, max_scrn_h;
    int num_displays, i, res;
    SDL_DisplayMode mode;

    /* Query all displays to ensure that, if we only have on display,
       that Tux Paint's window will not be larger than it */
    max_scrn_w = -1;
    max_scrn_h = -1;

    num_displays = SDL_GetNumVideoDisplays();
    if (num_displays == 0)
    {
      fprintf(stderr, "Warning: SDL_GetNumVideoDisplays() returned zero!");
    }
    else if (num_displays < 0)
    {
      fprintf(stderr, "Warning: SDL_GetNumVideoDisplays() failed: %s\n", SDL_GetError());
    }
    else
    {
      for (i = 0; i < num_displays; i++)
      {
        res = SDL_GetCurrentDisplayMode(i, &mode);
        if (res != 0)
        {
          fprintf(stderr, "Warning: SDL_GetCurrentDisplayMode() on display %d failed: %s\n", i, SDL_GetError());
        }
        else
        {
          if (mode.w >= WINDOW_WIDTH && mode.h >= WINDOW_HEIGHT)
          {
            /* Found a display capable of the chosen window size */
            max_scrn_w = mode.w;
            max_scrn_h = mode.h;
          }
          else
          {
            if (mode.w >= max_scrn_w)
              max_scrn_w = mode.w;
            if (mode.h >= max_scrn_h)
              max_scrn_h = mode.h;
          }
        }
      }
    }

    if (max_scrn_w == -1)
    {
      fprintf(stderr, "Warning: Could not query any display modes!?\n");
    }
    else if (num_displays == 1)
    {
      /* Only found one display, and window size is larger? Use that window size */
      if (WINDOW_WIDTH > max_scrn_w)
      {
        fprintf(stderr,
                "Warning: Asked for window width (%d) larger than max screen width (%d)\n", WINDOW_WIDTH, max_scrn_w);
        WINDOW_WIDTH = max_scrn_w;
      }

      if (WINDOW_HEIGHT > max_scrn_h)
      {
        fprintf(stderr,
                "Warning: Asked for window height (%d) larger than max screen height (%d)\n",
                WINDOW_HEIGHT, max_scrn_h);
        WINDOW_HEIGHT = max_scrn_h;
      }
    }


    /* Finally, ready to create a window! */
#ifdef USE_HWSURFACE
    window_screen = SDL_CreateWindow("Tux Paint", win_x, win_y, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HWSURFACE);
    if (window_screen == NULL)
      printf("Warning: Could not create a window with a hardware surface\n");
#else
    window_screen = SDL_CreateWindow("Tux Paint", win_x, win_y, WINDOW_WIDTH, WINDOW_HEIGHT, 0 /* no flags */ );
    if (window_screen == NULL)
      printf("Warning: Could not create a window with a software surface\n");
#endif

    /* FIXME: Check window_screen for being NULL, and abort?! (Also see above) -bjk 2024.12.20 */

    /* Note: Seems that this depends on the compliance by the window manager.
       Currently this doesn't work under Fvwm */
    SDL_SetWindowMinimumSize(window_screen, WINDOW_WIDTH, WINDOW_HEIGHT);
    SDL_SetWindowMaximumSize(window_screen, WINDOW_WIDTH, WINDOW_HEIGHT);


    renderer = SDL_CreateRenderer(window_screen, -1, 0);
    SDL_GL_GetDrawableSize(window_screen, &ww, &hh);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STATIC, ww, hh);

    screen = SDL_CreateRGBSurface(0, ww, hh, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

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

  if (button_size_auto)         /* Automatic size of buttons, see https://sourceforge.net/p/tuxpaint/feature-requests/218/ */
    button_scale = (float)min((48 * screen->w) / 800, (48 * screen->h) / 600) / ORIGINAL_BUTTON_SIZE;

  setup_screen_layout();
  set_color_picker_crosshair_size();

  /* Set window icon and caption: */

#ifndef __APPLE__
  seticon();
#endif
  if (hide_cursor)
    SDL_ShowCursor(SDL_DISABLE);


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


  /* Let Pango & fontcache do their work without locking up */

  fontconfig_thread_done = 0;

  DEBUG_PRINTF("Spawning Pango thread\n");

  fontconfig_thread = SDL_CreateThread(generate_fontconfig_cache, "fontconfig_thread", NULL);
  if (fontconfig_thread == NULL)
  {
    fprintf(stderr, "Failed to create Pango setup thread: %s\n", SDL_GetError());
  }
  else
  {
    DEBUG_PRINTF("Thread spawned\n");

    if (generate_fontconfig_cache_spinner(screen))      /* returns 1 if aborted */
    {
      fprintf(stderr, "Pango thread aborted!\n");
      fflush(stdout);
      // FIXME SDL2
      //      SDL_KillThread(fontconfig_thread);
      SDL_Quit();
      exit(0);
      /* FIXME: Let's be more graceful about exiting (e.g., clean up lockfile!) -bjk 2010.04.27 */
    }
    DEBUG_PRINTF("Done generating cache\n");
  }


#ifdef FORKED_FONTS
  /* NOW we can fork our own font scanner stuff, and let it run in the bgkd -bjk 2010.04.27 */
  DEBUG_PRINTF("Now running font scanner\n");
  run_font_scanner(screen, texture, renderer, lang_prefixes[get_current_language()]);
#endif


  medium_font = TuxPaint_Font_OpenFont(tp_ui_font, DATA_PREFIX "fonts/default_font.ttf",        /* FIXME: Does this matter any more? -bjk 2023.05.29 */
                                       (18 - (only_uppercase * 3)) * button_scale);

  if (medium_font == NULL)
  {
    fprintf(stderr,
            "\nError: Can't load font file: "
            DATA_PREFIX "fonts/default_font.ttf\n"
            "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

    cleanup();
    exit(1);
  }

  safe_snprintf(tmp_str, sizeof(tmp_str), "Version: %s – %s", VER_VERSION, VER_DATE);

  tmp_surf = render_text(medium_font, tmp_str, black);
  dest.x = 10;
  dest.y = WINDOW_HEIGHT - img_progress->h - tmp_surf->h;
  SDL_BlitSurface(tmp_surf, NULL, screen, &dest);
  SDL_FreeSurface(tmp_surf);

  DEBUG_PRINTF("%s\n", tmp_str);

  safe_snprintf(tmp_str, sizeof(tmp_str), "© 2002–2024 Bill Kendrick, et al.");
  tmp_surf = render_text(medium_font, tmp_str, black);
  dest.x = 10;
  dest.y = WINDOW_HEIGHT - img_progress->h - (tmp_surf->h * 2);
  SDL_BlitSurface(tmp_surf, NULL, screen, &dest);
  SDL_FreeSurface(tmp_surf);

  SDL_Flip(screen);


#ifdef FORKED_FONTS
  reliable_write(font_socket_fd, &no_system_fonts, sizeof no_system_fonts);
#else
  font_thread = SDL_CreateThread(load_user_fonts_stub, "font_thread", NULL);
#endif

  /* continuing on with the rest of the cursors... */


#ifndef __APPLE__
  cursor_arrow = get_cursor(arrow_bits, arrow_mask_bits, arrow_width, arrow_height, 0, 0);
#endif

  cursor_hand = get_cursor(hand_bits, hand_mask_bits, hand_width, hand_height, 12 / scale, 1 / scale);

  cursor_pipette = get_cursor(pipette_bits, pipette_mask_bits, pipette_width, pipette_height, 2 / scale, 20 / scale);

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

  canvas_width = WINDOW_WIDTH - r_ttools.w - r_ttoolopt.w;
  canvas_height = (button_h * buttons_tall) + r_ttools.h;

  DEBUG_PRINTF("Canvas size is %d x %d\n", canvas_width, canvas_height);

  /* Per https://wiki.libsdl.org/SDL_CreateRGBSurface,
   * the flags are unused and should be set to 0
   * Using zeros for the RGB masks sets a default value, based on the depth.
   */
  canvas = SDL_CreateRGBSurface(0, canvas_width, canvas_height, 24, 0, 0, 0, 0);

  save_canvas = SDL_CreateRGBSurface(0, canvas_width, canvas_height, 24, 0, 0, 0, 0);


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
    fprintf(stderr, "\nError: Can't build drawing touch mask for Magic!\n");

    cleanup();
    exit(1);
  }

  sim_flood_touched = (Uint8 *) malloc(sizeof(Uint8) * (canvas->w * canvas->h));
  if (sim_flood_touched == NULL)
  {
    fprintf(stderr, "\nError: Can't build drawing touch mask for Fill!\n");

    cleanup();
    exit(1);
  }

  canvas_color_r = 255;
  canvas_color_g = 255;
  canvas_color_b = 255;

  SDL_FillRect(canvas, NULL, SDL_MapRGB(canvas->format, 255, 255, 255));

  /* Creating the label surface: */

  label = SDL_CreateRGBSurface(screen->flags,
                               WINDOW_WIDTH - (r_ttools.w * 2),
                               (button_h * 7) + 40 + HEIGHTOFFSET,
                               screen->format->BitsPerPixel,
                               screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, TPAINT_AMASK);

  /* making the label layer transparent */
  SDL_FillRect(label, NULL, SDL_MapRGBA(label->format, 0, 0, 0, 0));

  /* Create undo buffer space: */

  for (i = 0; i < NUM_UNDO_BUFS; i++)
  {
    undo_bufs[i] =
      SDL_CreateRGBSurface(screen->flags, canvas_width, canvas_height,
                           screen->format->BitsPerPixel,
                           screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, 0);


    if (undo_bufs[i] == NULL)
    {
      fprintf(stderr, "\nError: Can't build undo buffer! (%d of %d)\n"
              "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", i + 1, NUM_UNDO_BUFS, SDL_GetError());

      cleanup();
      exit(1);
    }

    undo_starters[i] = UNDO_STARTER_NONE;
  }



  /* Load other images: */

  for (i = 0; i < NUM_TOOLS; i++)
    img_tools[i] = loadimagerb(tool_img_fnames[i]);

  img_title_on = loadimagerb(DATA_PREFIX "images/ui/title.png");
  img_title_large_on = loadimagerb(DATA_PREFIX "images/ui/title_large.png");
  img_title_off = loadimagerb(DATA_PREFIX "images/ui/no_title.png");
  img_title_large_off = loadimagerb(DATA_PREFIX "images/ui/no_title_large.png");

  img_btn_up = loadimagerb(DATA_PREFIX "images/ui/btn_up.png");
  img_btn_down = loadimagerb(DATA_PREFIX "images/ui/btn_down.png");
  img_btn_off = loadimagerb(DATA_PREFIX "images/ui/btn_off.png");
  img_btn_hold = loadimagerb(DATA_PREFIX "images/ui/btn_hold.png");

  img_btnsm_up = loadimagerb(DATA_PREFIX "images/ui/btnsm_up.png");
  img_btnsm_off = loadimagerb(DATA_PREFIX "images/ui/btnsm_off.png");
  img_btnsm_down = loadimagerb(DATA_PREFIX "images/ui/btnsm_down.png");
  img_btnsm_hold = loadimagerb(DATA_PREFIX "images/ui/btnsm_hold.png");

  img_btn_nav = loadimagerb(DATA_PREFIX "images/ui/btn_nav.png");
  img_btnsm_nav = loadimagerb(DATA_PREFIX "images/ui/btnsm_nav.png");

  img_brush_anim = loadimagerb(DATA_PREFIX "images/ui/brush_anim.png");
  img_brush_dir = loadimagerb(DATA_PREFIX "images/ui/brush_dir.png");

  img_sfx = loadimagerb(DATA_PREFIX "images/tools/sfx.png");
  img_speak = loadimagerb(DATA_PREFIX "images/tools/speak.png");

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

  img_yes = loadimagerb(DATA_PREFIX "images/ui/yes.png");
  img_no = loadimagerb(DATA_PREFIX "images/ui/no.png");

  img_prev = loadimagerb(DATA_PREFIX "images/ui/prev.png");
  img_next = loadimagerb(DATA_PREFIX "images/ui/next.png");

  img_mirror = loadimagerb(DATA_PREFIX "images/ui/mirror.png");
  img_flip = loadimagerb(DATA_PREFIX "images/ui/flip.png");
  img_rotate = loadimagerb(DATA_PREFIX "images/ui/rotate.png");

  img_open = loadimagerb(DATA_PREFIX "images/ui/open.png");
  img_erase = loadimagerb(DATA_PREFIX "images/ui/erase.png");
  img_pict_export = loadimagerb(DATA_PREFIX "images/ui/pict_export.png");
  img_back = loadimagerb(DATA_PREFIX "images/ui/back.png");
  img_trash = loadimagerb(DATA_PREFIX "images/ui/trash.png");

  img_slideshow = loadimagerb(DATA_PREFIX "images/ui/slideshow.png");
  img_template = loadimagerb(DATA_PREFIX "images/ui/template.png");
  img_play = loadimagerb(DATA_PREFIX "images/ui/play.png");
  img_gif_export = loadimagerb(DATA_PREFIX "images/ui/gif_export.png");
  img_select_digits = loadimagerb(DATA_PREFIX "images/ui/select_digits.png");

  img_popup_arrow = loadimagerb(DATA_PREFIX "images/ui/popup_arrow.png");

  img_dead40x40 = loadimagerb(DATA_PREFIX "images/ui/dead40x40.png");

  img_printer = loadimagerb(DATA_PREFIX "images/ui/printer.png");
  img_printer_wait = loadimagerb(DATA_PREFIX "images/ui/printer_wait.png");

  img_save_over = loadimagerb(DATA_PREFIX "images/ui/save_over.png");

  img_grow = loadimagerb(DATA_PREFIX "images/ui/grow.png");
  img_shrink = loadimagerb(DATA_PREFIX "images/ui/shrink.png");

  img_magic_paint = loadimagerb(DATA_PREFIX "images/ui/magic_paint.png");
  img_magic_fullscreen = loadimagerb(DATA_PREFIX "images/ui/magic_fullscreen.png");

  img_shapes_center = loadimagerb(DATA_PREFIX "images/ui/shapes_center.png");
  img_shapes_corner = loadimagerb(DATA_PREFIX "images/ui/shapes_corner.png");

  img_bold = loadimagerb(DATA_PREFIX "images/ui/bold.png");
  img_italic = loadimagerb(DATA_PREFIX "images/ui/italic.png");

  img_label_select = loadimagerb(DATA_PREFIX "images/tools/label_select.png");
  img_label_apply = loadimagerb(DATA_PREFIX "images/tools/label_apply.png");

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

  img_scroll_up = loadimagerb(DATA_PREFIX "images/ui/scroll_up.png");
  img_scroll_down = loadimagerb(DATA_PREFIX "images/ui/scroll_down.png");

  img_scroll_up_off = loadimagerb(DATA_PREFIX "images/ui/scroll_up_off.png");
  img_scroll_down_off = loadimagerb(DATA_PREFIX "images/ui/scroll_down_off.png");
  img_color_sel = loadimagerb(DATA_PREFIX "images/ui/csel.png");
  img_color_mix = loadimagerb(DATA_PREFIX "images/ui/cmix.png");
  img_color_picker_icon = loadimagerb(DATA_PREFIX "images/ui/color_picker_icon.png");
  img_color_grab = loadimagerb(DATA_PREFIX "images/ui/color_grab.png");

  if (onscreen_keyboard)
  {
    img_oskdel = loadimagerb(DATA_PREFIX "images/ui/osk_delete.png");
    img_osktab = loadimagerb(DATA_PREFIX "images/ui/osk_tab.png");
    img_oskenter = loadimagerb(DATA_PREFIX "images/ui/osk_enter.png");
    img_oskcapslock = loadimagerb(DATA_PREFIX "images/ui/osk_capslock.png");
    img_oskshift = loadimagerb(DATA_PREFIX "images/ui/osk_shift.png");
    img_oskpaste = loadimagerb(DATA_PREFIX "images/ui/osk_shift.png");  // FIXME

    if (onscreen_keyboard_layout)
    {
      // use platform system onscreen keybord or tuxpaint onscreen keybord
      if (strcmp(onscreen_keyboard_layout, "SYSTEM") == 0)
        kbd = NULL;
      else
        kbd =
          osk_create(onscreen_keyboard_layout, canvas,
                     img_btn_up, img_btn_down, img_btn_off,
                     img_btn_nav, img_btn_hold,
                     img_oskdel, img_osktab, img_oskenter,
                     img_oskcapslock, img_oskshift, img_oskpaste, onscreen_keyboard_disable_change);
    }
    else
    {
      kbd =
        osk_create(strdup("default.layout"), canvas,
                   img_btn_up, img_btn_down, img_btn_off,
                   img_btn_nav, img_btn_hold,
                   img_oskdel, img_osktab, img_oskenter,
                   img_oskcapslock, img_oskshift, img_oskpaste, onscreen_keyboard_disable_change);
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

  large_font = TuxPaint_Font_OpenFont(tp_ui_font, DATA_PREFIX "fonts/default_font.ttf", /* FIXME: Does this matter any more? -bjk 2023.05.29 */
                                      (30 - (only_uppercase * 3)) * button_scale);

  if (large_font == NULL)
  {
    fprintf(stderr,
            "\nError: Can't load font file: "
            DATA_PREFIX "fonts/default_font.ttf\n"
            "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

    cleanup();
    exit(1);
  }


  small_font = TuxPaint_Font_OpenFont(tp_ui_font, DATA_PREFIX "fonts/default_font.ttf", /* FIXME: Does this matter any more? -bjk 2023.05.29 */
#ifdef __APPLE__
                                      (12 - (only_uppercase * 2)) * button_scale
#else
                                      (13 - (only_uppercase * 2)) * button_scale
#endif
    );

  if (small_font == NULL)
  {
    fprintf(stderr,
            "\nError: Can't load font file: "
            DATA_PREFIX "fonts/default_font.ttf\n"
            "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());

    cleanup();
    exit(1);
  }

  locale_font = medium_font;


  if (!dont_load_stamps)
    load_stamps(screen);


  /* Load magic tool plugins: */

  magic_disabled_features = 0x00;       // 0b00000000
  if (disable_magic_sizes)
  {
    magic_disabled_features |= MAGIC_FEATURE_SIZE;
  }
  if (disable_magic_controls)
  {
    magic_disabled_features |= MAGIC_FEATURE_CONTROL;
  }

  load_magic_plugins();

  show_progress_bar(screen);

  /* Load shape icons: */
  for (i = 0; i < NUM_SHAPES; i++)
  {
    SDL_Surface *aux_surf = loadimage(shape_img_fnames[i]);

    img_shapes[i] =
      thumbnail2(aux_surf, (aux_surf->w * button_w) / ORIGINAL_BUTTON_SIZE,
                 (aux_surf->h * button_h) / ORIGINAL_BUTTON_SIZE, 0, 1);
    SDL_FreeSurface(aux_surf);
  }

  show_progress_bar(screen);

  /* Load fill sub-tool icons: */
  for (i = 0; i < NUM_FILLS; i++)
  {
    SDL_Surface *aux_surf = loadimage(fill_img_fnames[i]);

    img_fills[i] =
      thumbnail2(aux_surf, (aux_surf->w * button_w) / ORIGINAL_BUTTON_SIZE,
                 (aux_surf->h * button_h) / ORIGINAL_BUTTON_SIZE, 0, 1);
    SDL_FreeSurface(aux_surf);
  }

  show_progress_bar(screen);

  /* Load tip tux images: */
  for (i = 0; i < NUM_TIP_TUX; i++)
    img_tux[i] = loadimagerb(tux_img_fnames[i]);

  show_progress_bar(screen);

  img_mouse = loadimagerb(DATA_PREFIX "images/ui/mouse.png");
  img_mouse_click = loadimagerb(DATA_PREFIX "images/ui/mouse_click.png");

  show_progress_bar(screen);

  img_color_picker = loadimagerb(DATA_PREFIX "images/ui/color_picker.png");
  img_color_picker_val = loadimagerb(DATA_PREFIX "images/ui/color_picker_val.png");

  /* Create toolbox and selector labels: */

  for (i = 0; i < NUM_TITLES; i++)
  {
    if (strlen(title_names[i]) > 0)
    {
      TuxPaint_Font *myfont = large_font;
      char *loc_str = gettext(title_names[i]);

      if (need_own_font && strcmp(gettext(title_names[i]), title_names[i]))
        myfont = locale_font;
      upstr = uppercase(loc_str);
      tmp_surf = render_text(myfont, upstr, black);
      free(upstr);
      img_title_names[i] = thumbnail(tmp_surf, min((int)(84 * button_scale), tmp_surf->w), tmp_surf->h, 0);
      SDL_FreeSurface(tmp_surf);
    }
    else
    {
      img_title_names[i] = NULL;
    }
  }



  /* Generate color selection buttons: */

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
                                             /* (WINDOW_WIDTH - r_ttoolopt.w) / NUM_COLORS, 48, */
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


  for (y = 0; y < tmp_btn_up->h; y++)
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

      for (i = 0; i < NUM_COLORS; i++)
      {
        double rh = sRGB_to_linear_table[color_hexes[i][0]];
        double gh = sRGB_to_linear_table[color_hexes[i][1]];
        double bh = sRGB_to_linear_table[color_hexes[i][2]];

        putpixels[img_color_btns[i]->format->BytesPerPixel]
          (img_color_btns[i], x, y,
           SDL_MapRGB(img_color_btns[i]->format,
                      linear_to_sRGB(rh * aa + ru * (1.0 - aa)),
                      linear_to_sRGB(gh * aa + gu * (1.0 - aa)), linear_to_sRGB(bh * aa + bu * (1.0 - aa))));
        putpixels[img_color_btns[i + NUM_COLORS]->format->BytesPerPixel] (img_color_btns[i + NUM_COLORS], x, y,
                                                                          SDL_MapRGB(img_color_btns
                                                                                     [i + NUM_COLORS]->format,
                                                                                     linear_to_sRGB(rh * aa +
                                                                                                    rd * (1.0 - aa)),
                                                                                     linear_to_sRGB(gh * aa +
                                                                                                    gd * (1.0 - aa)),
                                                                                     linear_to_sRGB(bh * aa +
                                                                                                    bd * (1.0 - aa))));
      }
    }
  }

  /* Apply icons on top of the color buttons that have them */
  for (i = 0; i < NUM_COLORS * 2; i++)
  {
    SDL_UnlockSurface(img_color_btns[i]);
    if (i == COLOR_SELECTOR || i == COLOR_SELECTOR + NUM_COLORS)
    {
      /* Color selector; draw pipette */
      dest.x = (img_color_btns[i]->w - img_color_sel->w) / 2;
      dest.y = (img_color_btns[i]->h - img_color_sel->h) / 2;
      dest.w = img_color_sel->w;
      dest.h = img_color_sel->h;
      SDL_BlitSurface(img_color_sel, NULL, img_color_btns[i], &dest);
    }
    else if (i == COLOR_PICKER || i == COLOR_SELECTOR + NUM_COLORS)
    {
      /* Color selector; draw rainbow */
      dest.x = (img_color_btns[i]->w - img_color_picker_icon->w) / 2;
      dest.y = (img_color_btns[i]->h - img_color_picker_icon->h) / 2;
      dest.w = img_color_picker_icon->w;
      dest.h = img_color_picker_icon->h;
      SDL_BlitSurface(img_color_picker_icon, NULL, img_color_btns[i], &dest);
    }
    else if (i == COLOR_MIXER || i == COLOR_MIXER + NUM_COLORS)
    {
      /* Color mixer; draw palette */
      dest.x = (img_color_btns[i]->w - img_color_mix->w) / 2;
      dest.y = (img_color_btns[i]->h - img_color_mix->h) / 2;
      dest.w = img_color_mix->w;
      dest.h = img_color_mix->h;
      SDL_BlitSurface(img_color_mix, NULL, img_color_btns[i], &dest);
    }
  }

  SDL_UnlockSurface(tmp_btn_up);
  SDL_UnlockSurface(tmp_btn_down);
  SDL_FreeSurface(tmp_btn_up);
  SDL_FreeSurface(tmp_btn_down);

  create_button_labels();

  /* Resize any icons if the text we just rendered was too wide,
     and we word-wrapped it to be two lines tall */

  /* (Tools) */
  for (i = 0; i < NUM_TOOLS; i++)
  {
    if (img_tools[i]->h + img_tool_names[i]->h > button_h - 1)
    {
      tmp_surf = thumbnail(img_tools[i], img_tools[i]->w, (button_h - img_tool_names[i]->h - 1), 0);
      SDL_FreeSurface(img_tools[i]);
      img_tools[i] = tmp_surf;
    }
  }

  /* (Magic tools) */
  for (i = 0; i < MAX_MAGIC_GROUPS; i++)
  {
    for (j = 0; j < num_magics[i]; j++)
    {
      if (magics[i][j].img_icon->h + magics[i][j].img_name->h > button_h - 1)
      {
        tmp_surf =
          thumbnail(magics[i][j].img_icon, magics[i][j].img_icon->w, (button_h - magics[i][j].img_name->h - 1), 0);
        SDL_FreeSurface(magics[i][j].img_icon);
        magics[i][j].img_icon = tmp_surf;
      }
    }
  }

  /* (Shapes) */
  for (i = 0; i < NUM_SHAPES; i++)
  {
    if (img_shapes[i]->h + img_shape_names[i]->h > button_h - 1)
    {
      tmp_surf = thumbnail(img_shapes[i], img_shapes[i]->w, (button_h - img_shape_names[i]->h - 1), 0);
      SDL_FreeSurface(img_shapes[i]);
      img_shapes[i] = tmp_surf;
    }
  }

  /* (Fill methods) */
  for (i = 0; i < NUM_FILLS; i++)
  {
    if (img_fills[i]->h + img_fill_names[i]->h > button_h - 1)
    {
      tmp_surf = thumbnail(img_fills[i], img_fills[i]->w, (button_h - img_fill_names[i]->h - 1), 0);
      SDL_FreeSurface(img_fills[i]);
      img_fills[i] = tmp_surf;
    }
  }

  /* FIXME: Worth resizing img_openlabels_* or img_mixerlabel_clear? */


  /* Seed random-number generator: */

  srand(SDL_GetTicks());


  /* Enable Unicode support in SDL: */

//  SDL_EnableUNICODE(1);

#ifndef _WIN32
  /* Set up signal handler for SIGPIPE (in case printer command dies;
     e.g., altprintcommand=kprinter, but 'Cancel' is clicked,
     instead of 'Ok') */

  signal(SIGPIPE, signal_handler);

  /* Set up signal for no-questions-asked remote closing of app */
  signal(SIGUSR1, signal_handler);
  signal(SIGUSR2, signal_handler);
#endif

#ifdef __APPLE__
  apple_init();
#endif
}


/* ================================================================================== */

/**
 * FIXME
 */
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

  color_mix_cur_undo = color_mix_oldest_undo = color_mix_newest_undo = 0;

  cur_tool = TOOL_BRUSH;
  cur_color = COLOR_BLACK;
  colors_are_selectable = 1;
  cur_brush = 0;
  for (i = 0; i < MAX_STAMP_GROUPS; i++)
    cur_stamp[i] = 0;
  cur_shape = SHAPE_SQUARE;
  cur_font = 0;
  cur_eraser = 0;
  cur_fill = 0;
  fill_drag_started = 0;
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
  {
    stamp_scroll[i] = 0;
  }
  stamp_group = 0;              /* reset! */

  for (i = 0; i < MAX_MAGIC_GROUPS; i++)
  {
    magic_scroll[i] = 0;
    cur_magic[i] = 0;
  }
  font_scroll = 0;
  tool_scroll = 0;
  eraser_scroll = 0;
  fill_scroll = 0;

  reset_avail_tools();


  /* Load current image (if any): */

  if (start_blank == 0)
    load_current();


  /* Draw the screen! */

  SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 255, 255, 255));

  draw_toolbar();
  draw_colors(COLORSEL_FORCE_REDRAW);
  draw_brushes();
  update_canvas(0, 0, WINDOW_WIDTH - r_ttoolopt.w, (48 * 7) + 40 + HEIGHTOFFSET);

  SDL_Flip(screen);

  draw_cur_tool_tip();
}

/* ================================================================================== */

/**
 * FIXME
 */
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
  freopen("/storage/emulated/0/Android/data/org.tuxpaint/files/tuxpaint.log", "w", stdout);     /* redirect stdout to a file */
#endif

  dup2(fileno(stdout), fileno(stderr)); /* redirect stderr to stdout */
  setvbuf(stdout, NULL, _IONBF, 0);     /* we don't want buffering to avoid de-sync'ing stdout and stderr */
  setvbuf(stderr, NULL, _IONBF, 0);     /* we don't want buffering to avoid de-sync'ing stdout and stderr */
  char logTime[100];
  time_t t = time(NULL);

  strftime(logTime, sizeof(logTime), "%A %d/%m/%Y %H:%M:%S", localtime(&t));
  printf("Tux Paint log - %s\n", logTime);
#endif

#if defined(__MACOS__)
  /* Pango uses Fontconfig which requires fonts.conf.  By default, Fontconfig
   * searches for this file in a global path (e.g., /opt/local/etc/fonts if
   * using the MacPorts port of Fontconfig) which does not exist on a vanilla
   * macOS install.  As a workaround, the macOS port of Tux Paint provides its
   * own copy, and tells Fontconfig to look inside the Tux Paint app bundle for
   * this file by setting the FONTCONFIG_PATH environment variable here.
   */
  putenv((char *)"FONTCONFIG_PATH=Resources/etc");
#elif defined(__IOS__)
  /* Same with iOS */
  putenv((char *)"FONTCONFIG_PATH=etc");
#endif

  chdir_to_binary(argv[0]);
  setup_config(argv);

#ifdef WIN32
#ifndef DEBUG
  char stdout_win32[255], stderr_win32[255];

  safe_snprintf(stdout_win32, 255, "%s/stdout.txt", GetDefaultSaveDir("TuxPaint"));
  safe_snprintf(stderr_win32, 255, "%s/stderr.txt", GetDefaultSaveDir("TuxPaint"));
  freopen(stdout_win32, "w", stdout);   /* redirect stdout to a file */
  freopen(stderr_win32, "w", stderr);   /* redirect stderr to a file */
#endif
  printf("Tux Paint Version " VER_VERSION "");
#ifdef WIN64
  printf("- x86_64");
#else
  printf("- i686");
#endif
  printf(" (" VER_DATE ")\n");
  printf("Running on ");
  win32_print_version();
#endif

#ifdef DEBUG
  CLOCK_ASM(time2);
#endif

#if defined(FC_DEBUG)
  /*
   * Enable fontconfig debugging. See "debug.h"
   */
  mysetenv("FC_DEBUG", FC_DEBUG);
#endif

#ifdef FORKED_FONTS
  /* must start ASAP, but depends on locale which in turn needs the config */
  DEBUG_PRINTF("NOT running font scanner\n");
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

  DEBUG_PRINTF("Seconds in early start-up: %.3f\n", (double)(time2 - time1) / CLOCK_SPEED);
  DEBUG_PRINTF("Seconds in late start-up:  %.3f\n", (double)(time2 - time1) / CLOCK_SPEED);


#ifdef DEBUG
  /* Confirm pango's character set */
  if (1)
  {
    const char *charset;

    g_get_charset(&charset);
    printf("pango charset: %s\n", charset);
  }
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

/**
 * FIXME
 */
static int trash(char *path)
{
#if defined(UNLINK_ONLY)
  return (unlink(path));
#elif defined(WIN32)
  return win32_trash(path);
#elif defined(__APPLE__)
  return apple_trash(path);
#elif defined __BEOS__ || defined __HAIKU__
  return haiku_trash(path);
#else
  char fname[MAX_PATH], trashpath[MAX_PATH], dest[MAX_PATH], infoname[MAX_PATH], bname[MAX_PATH + 1], ext[MAX_PATH];
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

  DEBUG_PRINTF("trash: basename=%s", basename(path));   /* EP */
  safe_strncpy(fname, basename(path), sizeof(fname));

  if (!file_exists(path))
  {
    debug("Does't exist anyway, so skipping");
    return (1);
  }


  /* Move file into Trash folder */

  /* FIXME: Use xdg function */
  if (getenv("XDG_DATA_HOME") != NULL)
  {
    safe_snprintf(trashpath, sizeof(trashpath), "%s/Trash", getenv("XDG_DATA_HOME"));
  }
  else if (getenv("HOME") != NULL)
  {
    safe_snprintf(trashpath, sizeof(trashpath), "%s/.local/share/Trash", getenv("HOME"));
  }
  else
  {
    debug("Can't move to trash! Deleting instead.");
    return (unlink(path));
  }

  mkdir(trashpath, 0x777);
  safe_snprintf(dest, sizeof(dest), "%s/files", trashpath);
  mkdir(dest, 0x777);
  safe_snprintf(dest, sizeof(dest), "%s/info", trashpath);
  mkdir(dest, 0x777);

  safe_snprintf(dest, sizeof(dest), "%s/files/%s", trashpath, fname);

  safe_strncpy(bname, fname, sizeof(bname));
  if (strstr(bname, ".") != NULL)
  {
    strcpy(strstr(bname, "."), "\0");   /* FIXME: Use strncpy() (ugh, complicated) */
    safe_strncpy(ext, strstr(fname, ".") + 1, sizeof(ext));
  }
  else
  {
    debug("Filename format unfamiliar! Deleting instead.");
    return (unlink(path));
  }

  safe_snprintf(infoname, sizeof(infoname), "%s/info/%s.trashinfo", trashpath, fname);

  cnt = 1;
  while (file_exists(dest) && cnt < 100)
  {
    safe_snprintf(fname, sizeof(fname), "%s_%d.%s", bname, cnt, ext);

    safe_snprintf(dest, sizeof(dest), "%s/files/%s", trashpath, fname);
    safe_snprintf(infoname, sizeof(infoname), "%s/info/%s.trashinfo", trashpath, fname);
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
      len = fread(buf, sizeof(unsigned char), sizeof(buf), fi);
      if (len > 0)
      {
        fwrite(buf, sizeof(unsigned char), sizeof(buf), fo);
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

  /* FIXME: Haiku */

  return (0);
#endif
}

/**
 * FIXME
 */
int file_exists(char *path)
{
  struct stat buf;
  int res;

  res = stat(path, &buf);
  return (res == 0);
}

/**
 * FIXME
 */
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
    *motioner = SDL_FALSE;
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
      *motioner = SDL_TRUE;
    }
    else
      *motioner = SDL_FALSE;
  }
}

/**
 * FIXME
 */
static void handle_joyhatmotion(SDL_Event event, int oldpos_x, int oldpos_y,
                                int *valhat_x, int *valhat_y, int *hatmotioner, Uint32 *old_hat_ticks)
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

/**
 * FIXME
 */
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

/**
 * FIXME
 */
static void handle_motioners(int oldpos_x, int oldpos_y, int motioner,
                             int hatmotioner, int old_hat_ticks, int val_x, int val_y, int valhat_x, int valhat_y)
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

/**
 * FIXME
 */
static void handle_joybuttonupdown(SDL_Event event, int oldpos_x, int oldpos_y)
{
  handle_joybuttonupdownscl(event, oldpos_x, oldpos_y, r_tools);
}

/**
 * FIXME
 */
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
             event.button.button == joystick_button_selectfilltool ||
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

      else if (event.button.button == joystick_button_selectfilltool)
      {
        ev.button.x = (TOOL_FILL % 2) * button_w + button_w / 2;
        ev.button.y = real_r_tools.y + TOOL_FILL / 2 * button_h + button_h / 2;
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

  DEBUG_PRINTF("result %d %d\n", ev.button.x, ev.button.y);

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

#ifdef __ANDROID__
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
  else if (HIT(r_toolopt)
           && (cur_tool == TOOL_BRUSH || cur_tool == TOOL_STAMP
               || cur_tool == TOOL_LINES || cur_tool == TOOL_SHAPES
               || cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL || cur_tool == TOOL_MAGIC))
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
  else if (HIT(r_toolopt)
           && (cur_tool == TOOL_BRUSH || cur_tool == TOOL_STAMP
               || cur_tool == TOOL_LINES || cur_tool == TOOL_SHAPES
               || cur_tool == TOOL_TEXT || cur_tool == TOOL_LABEL || cur_tool == TOOL_MAGIC))
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

/**
 * Grab the user's XDG user dir for something (e.g., ~/Pictures)
 *
 * @param char * dir_type -- the thing to query, e.g. "PICTURES" or "VIDEOS"
 *   (note: currently, Tux Paint only puts things in the PICTURES one)
 * @param char * fallback -- path, under $HOME, to use instead (e.g., "Pictures")
 * @return char * path (caller is expected to free() it)
 */
char *get_xdg_user_dir(const char *dir_type, const char *fallback)
{
  FILE *fi;
  char *config_home, *found;
  char tmp_path[MAX_PATH], config_path[MAX_PATH], line[MAX_PATH], search[MAX_PATH], return_path[MAX_PATH];
  int found_it;

  found_it = SDL_FALSE;

  /* Figure out where XDG user-dirs config exists, and use it if possible */
  if (getenv("XDG_CONFIG_HOME") != NULL)
  {
    config_home = strdup(getenv("XDG_CONFIG_HOME"));
  }
  else
  {
#ifdef DEBUG
    fprintf(stderr, "XDG_CONFIG_HOME not set, checking $HOME/.config/\n");
#endif
    if (getenv("HOME") != NULL)
    {
      safe_snprintf(tmp_path, MAX_PATH, "%s/.config", getenv("HOME"));
      config_home = strdup(tmp_path);
    }
    else
    {
#ifdef DEBUG
      fprintf(stderr, "No HOME, either?! Returing fallback in current directory\n");
#endif
      return strdup(fallback);
    }
  }

  if (config_home[strlen(config_home) - 1] == '/')
  {
    config_home[strlen(config_home) - 1] = '\0';
  }
  safe_snprintf(config_path, MAX_PATH, "%s/user-dirs.dirs", config_home);
  free(config_home);

#ifdef DEBUG
  fprintf(stderr, "User dirs config = %s\n", config_path);
#endif

  safe_snprintf(search, MAX_PATH, "XDG_%s_DIR=\"", dir_type);

  /* Read the config to find the path we want */
  fi = fopen(config_path, "r");
  if (fi != NULL)
  {
    /* Search for a line in the form of either
       either XDG_PICTURES_DIR="$HOME/Pictures"
       or XDG_PICTURES_DIR="/Path/To/Pictures"
     */
#ifdef DEBUG
    fprintf(stderr, "Searching it for: %s\n", search);
#endif
    while (fgets(line, MAX_PATH, fi) && !found_it)
    {
      /* Trim trailing CR/LF */
      if (line[strlen(line) - 1] == '\n' || line[strlen(line) - 1] == '\r')
      {
        line[strlen(line) - 1] = '\0';
      }

      if (strstr(line, search) == line)
      {
        found = line + strlen(search);
#ifdef DEBUG
        fprintf(stderr, "Found it: %s\n", found);
#endif
        if (strstr(found, "$HOME/") == found)
        {
          safe_snprintf(return_path, MAX_PATH, "%s/%s", getenv("HOME"), found + 6 /* skip '$HOME/' */ );
        }
        else
        {
          safe_strncpy(return_path, found, MAX_PATH);
        }

        /* Trim trailing " */
        if (return_path[strlen(return_path) - 1] == '\"')
        {
          return_path[strlen(return_path) - 1] = '\0';
        }

        found_it = SDL_TRUE;
      }
    }

    fclose(fi);
#ifdef DEBUG
  }
  else
  {
    fprintf(stderr, "%s doesn't exist\n", config_path);
#endif
  }

  if (!found_it)
  {
#ifdef DEBUG
    fprintf(stderr, "Using fallback of $HOME/%s\n", fallback);
#endif
    safe_snprintf(return_path, MAX_PATH, "%s/%s", getenv("HOME"), fallback);
  }

#ifdef DEBUG
  fprintf(stderr, "Location for %s => %s\n", dir_type, return_path);
#endif

  return strdup(return_path);
}

/**
 * After 2+ images have been selected in the Open->Slideshow
 * dialog, they can be exported as an animated GIF.
 *
 * Params the same as play_slideshow(), except...
 *
 * @param int speed -- how fast to play the slideshow (0 and 1 both = slowest, 10 = fasted)
 * @return int -- 0 if export failed or was aborted, 1 if successful
 */
static int export_gif(int *selected, int num_selected, char *dirname,
                      char **d_names, char **d_exts, int speed, char **dest_fname)
{
  char *tmp_starter_id, *tmp_template_id, *tmp_file_id;
  int tmp_starter_mirrored, tmp_starter_flipped, tmp_starter_personal;
  char *gif_fname;
  char fname[MAX_PATH];
  int i, j, done, which, x, y;
  SDL_Surface *img;
  int overall_w, overall_h, overall_area;
  Uint8 *bitmap;
  Uint8 r, g, b, a;
  size_t pixels_size;
  unsigned char *raw_8bit_pixels;
  uint8_t gif_palette[768];     /* 256 x 3 */
  liq_attr *liq_handle;
  liq_image *input_image;
  liq_result *quantization_result;

  *dest_fname = NULL;

#if LIQ_VERSION >= 20800
  liq_error qtiz_status;
#endif
  const liq_palette *palette;
  int gif_speed;

  /* Back up the current image's IDs, because they will get
     clobbered below! */
  tmp_starter_id = strdup(starter_id);
  tmp_template_id = strdup(template_id);
  tmp_file_id = strdup(file_id);
  tmp_starter_mirrored = starter_mirrored;
  tmp_starter_flipped = starter_flipped;
  tmp_starter_personal = starter_personal;

  do_setcursor(cursor_watch);
  show_progress_bar(screen);

  gif_fname = get_export_filepath("gif");
  if (gif_fname == NULL)
  {
    /* Can't create export dir! Abort! */
    return SDL_FALSE;
  }

  *dest_fname = strdup(gif_fname);

  /* For now, always saving GIF using the size of Tux Paint's window,
     which is how images appear in the slide show */
  overall_w = screen->w;
  overall_h = screen->h;
  overall_area = overall_w * overall_h;

  if (speed == 0)
  {
    gif_speed = 1;
  }
  gif_speed = (10 - speed) * 50;

  bitmap = malloc(num_selected * overall_area * 4);
  if (bitmap != NULL)
  {
    done = 0;

    for (i = 0; i < num_selected && !done; i++)
    {
      which = selected[i];
      show_progress_bar(screen);


      /* Figure out filename: */
      safe_snprintf(fname, sizeof(fname), "%s/%s%s", dirname, d_names[which], d_exts[which]);

      /* Load and scale the image */
      img = myIMG_Load(fname);

      if (img != NULL)
      {
        DEBUG_PRINTF("Smearing image @ 12 (GIF export)\n");
        autoscale_copy_smear_free(img, screen, SDL_BlitSurface);

        safe_strncpy(file_id, d_names[which], sizeof(file_id));

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
      else
      {
        /* Error loading ! */
        fprintf(stderr, "Error loading %s!\n", fname);
        /* FIXME Abort? */
      }

      /* Record the raw RGB into a big strip, to be quantized and sliced later */
      for (y = 0; y < overall_h; y++)
      {
        for (x = 0; x < overall_w; x++)
        {
          SDL_GetRGBA(getpixels[screen->format->BytesPerPixel] (screen, x, y), screen->format, &r, &g, &b, &a);

          bitmap[((i * overall_area) + (y * overall_w) + x) * 4 + 0] = r;
          bitmap[((i * overall_area) + (y * overall_w) + x) * 4 + 1] = g;
          bitmap[((i * overall_area) + (y * overall_w) + x) * 4 + 2] = b;
          bitmap[((i * overall_area) + (y * overall_w) + x) * 4 + 3] = 255;
        }
      }

      SDL_Flip(screen);
      done = export_gif_monitor_events();
    }


    if (!done)
    {
      /* Quantize to max 256 (8bpp) colors and generate a suitable palette */
      liq_handle = liq_attr_create();
      input_image = liq_image_create_rgba(liq_handle, bitmap, overall_w, num_selected * overall_h, 0);
      liq_set_max_colors(liq_handle, 256);

      show_progress_bar(screen);
      done = export_gif_monitor_events();

#if LIQ_VERSION >= 20800
      qtiz_status = liq_image_quantize(input_image, liq_handle, &quantization_result);
      done = (qtiz_status != LIQ_OK);
#else
      quantization_result = liq_quantize_image(liq_handle, input_image);
      done = (quantization_result == NULL);
#endif
      if (!done)
      {
        // Use libimagequant to make new image pixels from the palette
        pixels_size = num_selected * overall_area;
        raw_8bit_pixels = malloc(pixels_size);
        liq_set_dithering_level(quantization_result, 1.0);

        liq_write_remapped_image(quantization_result, input_image, raw_8bit_pixels, pixels_size);
        palette = liq_get_palette(quantization_result);
        free(bitmap);

        for (j = 0; j < (int)palette->count; j++)
        {
          gif_palette[j * 3 + 0] = palette->entries[j].r;
          gif_palette[j * 3 + 1] = palette->entries[j].g;
          gif_palette[j * 3 + 2] = palette->entries[j].b;
        }

        /* Open GIF */
        ge_GIF *gif = ge_new_gif(gif_fname,
                                 overall_w, overall_h,
                                 gif_palette,
                                 8,     /* 256 colors */
                                 0      /* infinite loop */
          );

        /* Export each frame */
        for (i = 0; i < num_selected && !done; i++)
        {
          memcpy(gif->frame, raw_8bit_pixels + i * overall_area, overall_area);
          ge_add_frame(gif, gif_speed);

          show_progress_bar(screen);
          done = export_gif_monitor_events();
        }

        /* Close the GIF */
        ge_close_gif(gif);
      }
      else
      {
        fprintf(stderr, "Quantization failed\n");
        done = 1;
      }

      if (done)
      {
        /* Aborted; discard the partially-saved GIF */
        unlink(gif_fname);
      }
    }
  }
  else
  {
    /* Out of memory! */
    done = 1;
  }


  /* Restore everything about the currently-active image
     that got clobbered above */
  strcpy(starter_id, tmp_starter_id);   /* safe; originally strdup()'d from the dest. */
  free(tmp_starter_id);

  strcpy(template_id, tmp_template_id); /* safe; originally strdup()'d from the dest. */
  free(tmp_template_id);

  strcpy(file_id, tmp_file_id); /* safe; originally strdup()'d from the dest. */
  free(tmp_file_id);

  starter_mirrored = tmp_starter_mirrored;
  starter_flipped = tmp_starter_flipped;
  starter_personal = tmp_starter_personal;


  free(gif_fname);

  /* Success if we didn't have an error, and user didn't abort */
  if (!done && *dest_fname != NULL)
  {
    SDL_SetClipboardText(*dest_fname);
  }

  return (!done);
}

/**
 * Called by export_gif() while it's iterating through images
 * in a few different ways, to monitor SDL event queue for
 * any [Esc] keypress or quit event (e.g., closing window),
 * which triggers an abort of the export.
 *
 * @return int 0 = keep going, 1 = abort
 */
int export_gif_monitor_events(void)
{
  int done;
  SDL_Event event;
  SDLKey key;

  done = 0;
  while (SDL_PollEvent(&event))
  {
    if (event.type == SDL_QUIT)
    {
      done = 1;
    }
    else if (event.type == SDL_KEYDOWN)
    {
      key = event.key.keysym.sym;
      if (key == SDLK_ESCAPE)
      {
        done = 1;
      }
    }
  }
  SDL_Delay(10);
  return done;
}

/**
 * Copy an image (just the main PNG) from Tux Paint's "saved"
 * directory to either:
 *
 *  a. the user's chosen export directory
 *     (e.g., ~/Pictures, or whatever "--exportdir" says)
 *  b. the user's local templates directory
 *     (e.g., ~/.tuxpaint/templates, base dir can be set by "--datadir")
 *
 * Used when exporting, or making into a template, a single image
 * (via options presented in the "Open" dialog).
 *
 * @param char * fname -- full path to the image to export
 * @param int where -- where are we exporting to? (what are we exporting?)
 *  + EXPORT_LOC_PICTURES is for exporting to Pictures
 *  + EXPORT_LOC_TEMPLATES is for making a new personal Tux Paint template
 * @param char * orig_fname -- basename of original picture's filename
 *  + used by EXPORT_LOC_TEMPLATES as prefix of new template
 *  + unused by EXPORT_LOC_PICTURES (just send NULL)
 * @return EXPORT_SUCCESS on success, or one of the EXPORT_ERR_... values on failure
 */
static int export_pict(char *fname, int where, char *orig_fname, char **dest_fname)
{
  FILE *fi, *fo;
  size_t len;
  unsigned char buf[1024];
  char *pict_fname;
  Uint32 time_before, time_after;

  do_setcursor(cursor_watch);
  show_progress_bar(screen);

  fi = fopen(fname, "rb");
  if (fi == NULL)
  {
    fprintf(stderr,
            "Cannot export from saved Tux Paint file '%s'\nThe error that occurred was:\n%s\n\n",
            fname, strerror(errno));
    return EXPORT_ERR_CANNOT_OPEN_SOURCE;
  }

  time_before = SDL_GetTicks();
  if (where == EXPORT_LOC_PICTURES)
  {
    pict_fname = get_export_filepath("png");
  }
  else                          /* where == EXPORT_LOC_TEMPLATES */
  {
    char *dir;

    pict_fname = NULL;

    dir = get_fname("templates", DIR_DATA);
    if (dir != NULL)
    {
      time_t t;
      int len = (strlen(dir) + 128);
      char timestamp[16];
      DIR *d;
      struct dirent *f;
      SDL_bool any_identical;
      struct stat sbuf_orig, sbuf_test;
      int orig_w, orig_h;
      uLong orig_crc;
      int res;

      /* Make sure we have a directory to put the template into! */
      if (!make_directory(DIR_DATA, "templates", "Can't create 'templates' directory in specified datadir"))
        return EXPORT_ERR_CANNOT_MKDIR;

      /* We'll only calculate the saved image's dimensions or CRC if we find a
         template that is identical in other easier-to-test ways
         (filename prefix, file size) */
      orig_w = -1;
      orig_crc = 0;

      /* Get save image's file size (in bytes) */
      res = stat(fname, &sbuf_orig);
      if (res != 0)
      {
        free(dir);
        fclose(fi);
        return EXPORT_ERR_CANNOT_OPEN_SOURCE;
      }

      /* We'll use a filename prefix based on the picture being exported;
         if any other templates exist with this prefix, we'll check whether
         the image is still identical.  If so, we'll avoid creating a new
         template, since that's redundant, (EXPORT_ERR_ALREADY_EXPORTED),
         otherwise we can proceed with copying. */

      d = opendir(dir);
      any_identical = SDL_FALSE;

      if (d != NULL)
      {
        /* Iterate over list of all personal templates: */

        do
        {
          f = readdir(d);

          if (f != NULL)
          {
            if (strstr(f->d_name, orig_fname) == f->d_name)
            {
              /* Filename prefixes match!  It was based on this drawing!
                 (But this drawing may have changed since, so we'll check
                 other datapoints to confirm) */
              char templ_fname[FILENAME_MAX];

              snprintf(templ_fname, sizeof(templ_fname), "%s/%s", dir, f->d_name);

              DEBUG_PRINTF("%s prefix matches save file's filename %s!\n", templ_fname, orig_fname);

              res = stat(templ_fname, &sbuf_test);
              if (res == 0)
              {
                if (sbuf_test.st_size == sbuf_orig.st_size)
                {
                  int templ_w, templ_h;

                  /* File sizes match!  (But in case that's a coincidence,
                     we'll check yet more datapoints to confirm) */
                  DEBUG_PRINTF("  ...and is the same size (%ld bytes)\n", sbuf_orig.st_size);

                  if (orig_w == -1)
                    get_img_dimensions(fname, &orig_w, &orig_h);

                  get_img_dimensions(templ_fname, &templ_w, &templ_h);

                  if (templ_w == orig_w && templ_h == orig_h)
                  {
                    uLong templ_crc;

                    /* Image dimensions match!  (But in case that's a coincidence,
                       we'll check yet one final datapoint to confirm) */
                    DEBUG_PRINTF("  ...and is the same dimensions (%d x %d)\n", orig_w, orig_h);

                    if (orig_crc == 0)
                      orig_crc = get_img_crc(fname);

                    templ_crc = get_img_crc(templ_fname);

                    if (templ_crc == orig_crc)
                    {
                      /* Appears to be identical data; don't bother making a new template */
                      DEBUG_PRINTF("  ...and appear to have the same content (crc = %ld)\n", orig_crc);

                      any_identical = SDL_TRUE;
                    }
                    else
                    {
                      DEBUG_PRINTF
                        ("  ...but appear to have the different content (template crc = %ld, saved file's is now %ld)\n",
                         templ_crc, orig_crc);
                    }
                  }
                  else
                  {
                    DEBUG_PRINTF
                      ("  ...but dimensions differ (template = %d x %d, saved file is now %d x %d)\n",
                       templ_w, templ_h, orig_w, orig_h);
                  }
                }
                else
                {
                  DEBUG_PRINTF
                    ("  ...but file sizes differ (template = %ld bytes, saved file is now %ld bytes\n",
                     sbuf_test.st_size, sbuf_orig.st_size);
                }
              }
              else
              {
                fprintf(stderr, "Warning: Cannot stat %s! Can't test for identical-ness\n", templ_fname);
              }
            }
          }
          /* Stop once we've looked at all files, but we'll short-circuit
             and exit the loop if we've come across an identical template */
        }
        while (f != NULL && !any_identical);

        closedir(d);
      }

      if (any_identical)
      {
        fclose(fi);
        return EXPORT_ERR_ALREADY_EXPORTED;
      }

      /* Create a unique filename, within that dir */
      t = time(NULL);
      strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", localtime(&t));
      pict_fname = (char *)malloc(sizeof(char) * len);
      snprintf(pict_fname, len, "%s/%s-%s-%s.png", dir, EXPORTED_TEMPLATE_PREFIX, orig_fname, timestamp);
    }

    free(dir);
  }

  if (dest_fname != NULL)
  {
    *dest_fname = NULL;
  }

  if (pict_fname == NULL)
  {
    fclose(fi);
    return EXPORT_ERR_FILENAME_PROBLEM;
  }

  fo = fopen(pict_fname, "wb");
  if (fo == NULL)
  {
    fprintf(stderr,
            "Cannot export to new file '%s'\nThe error that occurred was:\n%s\n\n", pict_fname, strerror(errno));
    free(pict_fname);
    fclose(fi);
    return EXPORT_ERR_CANNOT_SAVE;
  }

  while (!feof(fi))
  {
    len = fread(buf, sizeof(unsigned char), sizeof(buf), fi);
    if (len > 0)
    {
      fwrite(buf, sizeof(unsigned char), len, fo);
    }
  }

  /* FIXME: Probably good to check for errors here and respond accordingly -bjk 2020.07.26 */

  fclose(fi);
  fclose(fo);

  if (dest_fname != NULL)
  {
    *dest_fname = strdup(pict_fname);
    SDL_SetClipboardText(pict_fname);
  }

  free(pict_fname);

  /* Unique filenames are timestamp-based, down to the second,
     so ensure at least one second has elapsed */
  time_after = SDL_GetTicks();
  if (time_after - time_before < 1000)
  {
    show_progress_bar(screen);
    SDL_Delay(time_after + 1000 - time_before);
  }

  return EXPORT_SUCCESS;
}

/**
 * Gets the dimensions (width & height, in pixels) of a PNG file
 *
 * @param char * fpath -- full path to the file
 * @param int * w -- pointer to an int where we'll fill in the width
 * @param int * h -- pointer to an int where we'll fill in the height
 */
void get_img_dimensions(char *fpath, int *w, int *h)
{
  FILE *fi;
  png_structp png;
  png_infop info;

  png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png == NULL)
  {
    fprintf(stderr, "get_img_dimensions() failed to png_create_read_struct() %s\n", fpath);
    return;
  }

  info = png_create_info_struct(png);
  if (info == NULL)
  {
    fprintf(stderr, "get_img_dimensions() failed to png_create_info_struct() %s\n", fpath);
    return;
  }

  if (setjmp(png_jmpbuf(png)))
  {
    fprintf(stderr, "get_img_dimensions() failed to png_jmpbuf() %s\n", fpath);
    return;
  }

  fi = fopen(fpath, "rb");
  if (fi == NULL)
  {
    fprintf(stderr, "get_img_dimensions() cannot open %s\n", fpath);
    return;
  }

  png_init_io(png, fi);

  png_read_info(png, info);

  *w = png_get_image_width(png, info);
  *h = png_get_image_height(png, info);

  png_destroy_read_struct(&png, &info, NULL);
}

uLong get_img_crc(char *fpath)
{
  uLong crc;
  FILE *fi;
  size_t len;
  unsigned char buf[1024];

  fi = fopen(fpath, "rb");
  if (fi == NULL)
  {
    fprintf(stderr, "Cannot open file; cannot calculate CRC for %s\n", fpath);
    return 0;
  }

  crc = crc32(0L, Z_NULL, 0);

  while (!feof(fi))
  {
    len = fread(buf, sizeof(unsigned char), sizeof(buf), fi);
    if (len > 0)
    {
      crc = crc32(crc, buf, len);
      update_progress_bar();
    }
  }

  fclose(fi);

  return crc;
}

/**
 * Returns the name of a new file, located in the user's chosen
 * export directory (e.g., ~/Pictures/TuxPaint, or whatever "--exportdir" says).
 *
 * Also ensures that the directory exists, in the first place.
 *
 * Used when exporting animated GIFs (via "Export GIF" in the
 * Open->Slideshow dialog) and static PNGs (via "Export" in the
 * main Open dialog).
 *
 * @param const char * ext -- extnesion of the file (e.g., "png" or "gif")
 * @return char * -- filepath for the new file to be created
 *   (e.g., /home/username/Pictures/TuxPaint/2020072620110100.gif")
 *   Or NULL if the directory cannot be created.
 */
static char *get_export_filepath(const char *ext)
{
  char *rname;
  char fname[FILENAME_MAX];
  char timestamp[16];
  time_t t;


  /* Make sure the export dir exists */
  if (!make_directory(DIR_EXPORT, "", "Can't create export directory; will try to make its parent (E016)"))
  {
    /* See if perhaps we need to try and make its parent directory first? */
    if (make_directory(DIR_EXPORT_PARENT, "", "Can't create export directory parent (E016b)"))
    {
      if (!make_directory(DIR_EXPORT, "", "Can't create export directory (E016c)"))
      {
        return NULL;
      }
    }
    else
    {
      return NULL;
    }
  }

  /* Create a unique filename, within that dir */
  t = time(NULL);
  strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", localtime(&t));
  safe_snprintf(fname, sizeof(fname), "%s.%s", timestamp, ext);
  rname = get_fname(fname, DIR_EXPORT);
  debug(rname);

  return (rname);
}

char *safe_strncat(char *dest, const char *src, size_t n)
{
  char *ptr;

  ptr = strncat(dest, src, n - 1);
  dest[n - 1] = '\0';
  return ptr;
}

char *safe_strncpy(char *dest, const char *src, size_t n)
{
  char *ptr;

  ptr = strncpy(dest, src, n - 1);      /* FIXME: Clean up, and keep safe, to avoid compiler warning (e.g., "output may be truncated copying 255 bytes from a string of length 255") */
  dest[n - 1] = '\0';
  return ptr;
}

int safe_snprintf(char *str, size_t size, const char *format, ...)
{
  int r;
  va_list ap;

  va_start(ap, format);
  r = vsnprintf(str, size - 1, format, ap);
  va_end(ap);

  str[size - 1] = '\0';
  return r;
}

static void sloppy_frac(float f, int *numer, int *denom)
{
  int n, d, fr, i, gcd, n_over_d_100;

  fr = round(f * 100.0);

  *numer = 0;
  *denom = 1;

  for (d = SLOPPY_FRAC_MIN; d <= SLOPPY_FRAC_MAX; d++)
  {
    for (n = 1; n < d; n++)
    {
      n_over_d_100 = ((n * 100) / d);
      if (n_over_d_100 >= fr && n_over_d_100 < (fr + ((SLOPPY_FRAC_MIN * 100) / SLOPPY_FRAC_MAX)))
      {
        *numer = n;
        *denom = d;
      }
    }
  }

  gcd = 1;
  for (i = 1; i <= *numer && i <= *denom; i++)
  {
    if ((*numer % i) == 0 && (*denom % i) == 0)
      gcd = i;
  }

  *numer /= gcd;
  *denom /= gcd;
}


/**
 * Selet a chosen Label node.
 *
 * @param int * old_x, old_y -- Pointers to feed the position of the chosen label
 */
static void select_label_node(int *old_x, int *old_y)
{
  unsigned int i;
  int j;

  /* Switch back into label entry mode */
  cur_label = LABEL_LABEL;

  /* Set font selector to the font used by this label */
  cur_thing = label_node_to_edit->save_cur_font;

  /* Disable the label node; we'll be replacing it (or removing it) */
  label_node_to_edit->is_enabled = SDL_FALSE;
  derender_node(&label_node_to_edit);

  /* Copy the label's text into the active text input */
  i = 0;
  texttool_len = select_texttool_len;
  while (i < texttool_len)
  {
    texttool_str[i] = select_texttool_str[i];
    i = i + 1;
  }
  texttool_str[i] = L'\0';

  cur_color = select_color;

  /* Send back the position of the selected label */
  *old_x = select_x;
  *old_y = select_y;

  /* Set active text input's attributes (font, italic/bold, size) */
  cur_font = select_cur_font;
  text_state = select_text_state;
  text_size = select_text_size;

  /* ??? */
  for (j = 0; j < num_font_families; j++)
  {
    if (user_font_families[j] && user_font_families[j]->handle)
    {
      TuxPaint_Font_CloseFont(user_font_families[j]->handle);
      user_font_families[j]->handle = NULL;
    }
  }

  update_screen_rect(&r_toolopt);

  /* Redraw color palette, fonts, and text controls */
  draw_colors(COLORSEL_REFRESH);
  draw_fonts();

  /* Set mouse pointer (cursor) shape to the text insertion bar */
  do_setcursor(cursor_insertion);

  update_canvas_ex_r(0, 0, canvas->w, canvas->h, 1);

  /* We have chosen a label; show some instructional text (Tux tip)
     and play a success sound */
  draw_tux_text(TUX_GREAT, TIP_LABEL_SELECTOR_LABEL_CHOSEN, 1);
  playsound(screen, 1, SND_TUXOK, 1, select_x, SNDDIST_NEAR);
}


/**
 * Apply a Label node to the canvas, and remove the node.
 *
 * FIXME: WIP
 *
 * @param int old_x, old_y - For insertion point cursor positioning
 *
 * Side effects:
 *  * Unsets `label_node_to_edit`
 *  * Sets Label tool mode back to `LABEL_LABEL`
 *  * Sets `have_to_rec_label_node`
 *  * Clears `been_saved` & sets `tool_avail[TOOL_SAVE]` (unless `disable_save` was set)
 *  * Redraws toolbar
 *  * Redraws fonts & text controls
 *  * Redraws color palette
 *  * Sets insertion point cursor location (`cursor_x`, `cursor_y`)
 *  * Displays Label tool's main instructions (Tux tip)
 */
static void apply_label_node(int old_x, int old_y)
{
  cursor_x = old_x;
  cursor_y = old_y;
  cursor_left = old_x;
  SDL_Rect rect;

  /* Capture into Undo buffer */
  rec_undo_buffer();

  /* Apply the text the canvas */
  do_render_cur_text(1);

  /* Switch to normal label adding mode;
     Update label control buttons */
  cur_label = LABEL_LABEL;
  draw_colors(COLORSEL_REFRESH);
  draw_fonts();
  update_screen_rect(&r_toolopt);

  have_to_rec_label_node = SDL_TRUE;

  /* [Best way to explain this?] */
  rect.x = label_node_to_edit->save_x;
  rect.y = label_node_to_edit->save_y;
  rect.w = label_node_to_edit->save_width;
  rect.h = label_node_to_edit->save_height;

  SDL_BlitSurface(label_node_to_edit->label_node_surface, NULL, canvas, &rect);
  label_node_to_edit->is_enabled = SDL_FALSE;

  /* [Best way to explain this?] */
  add_label_node(0, 0, 0, 0, NULL);
  derender_node(&label_node_to_edit);
  label_node_to_edit = NULL;

  /* [Best way to explain this?] */
  texttool_len = 0;
  cursor_textwidth = 0;

  /* Make "Save" button available after this change (if appropriate) */
  if (been_saved)
  {
    been_saved = 0;

    if (!disable_save)
      tool_avail[TOOL_SAVE] = 1;

    draw_toolbar();
    update_screen_rect(&r_tools);
  }

  /* Back to normal "how to use Label tool" tip; play sound */
  update_canvas_ex_r(rect.x, rect.y, rect.w, rect.h, 1);

  draw_tux_text(TUX_GREAT, tool_tips[TOOL_LABEL], 1);
  playsound(screen, 1, SND_RETURN, 1, cursor_x, SNDDIST_NEAR);
}


/**
 * Size & position the onscreen keyboard.
 *
 * @param int y -- If -1, don't reposition vertically; otherwise, if y
 *   is in top half of canvas, put keyboard on bottom & vice-versa
 */
static void reposition_onscreen_keyboard(int y)
{
  if (onscreen_keyboard && kbd)
  {
    kbd_rect.x = button_w * 2 + (canvas->w - kbd->surface->w) / 2;

    if (y != -1)
    {
      if (y < r_canvas.h / 2)
        kbd_rect.y = r_canvas.h - kbd->surface->h;
      else
        kbd_rect.y = 0;
    }

    kbd_rect.w = kbd->surface->w;
    kbd_rect.h = kbd->surface->h;

    SDL_BlitSurface(kbd->surface, &kbd->rect, screen, &kbd_rect);
    update_screen_rect(&kbd_rect);
  }
}

/**
 * How many rows of controls (not actual Magic tool items)
 * are to be displayed at the bottom of the selector?
 * (Based on whether magic controls and/or magic sizes are
 * disabled)
 *
 * @return int
 */
int calc_magic_control_rows(void)
{
  int r;

  r = 0;

  /* Add group changing (left/right) buttons */
  if (!no_magic_groups)
    r++;

  /* Add magic controls (paint vs fullscreen) */
  if (!disable_magic_controls)
    r++;

  /* Add magic size controls */
  if (!disable_magic_sizes)
    r++;

  return r;
}


/**
 * How many rows of controls (not actual Stamp items)
 * are to be displayed at the bottom of the selector?
 * (Based on whether stamp controls and/or stamp rotation are
 * disabled)
 *
 * @return int
 */
int calc_stamp_control_rows(void)
{
  int r;

  /* Start with group changing (left/right) buttons */
  r = 1;

  /* Add Stamp controls (one row flip/mirror, another size) */
  if (!disable_stamp_controls)
  {
    r += 2;

    /* Add Stamp rotation controls */
    if (!no_stamp_rotation)
      r++;
  }

  return r;
}

/**
 * Redraw the Eraser XOR shape around the cursor if it's within
 * the canvas and the current tool is the Eraser.
 * (Used after hitting Ctrl-Z to Undo or Ctlr-R to Redo)
 */
void maybe_redraw_eraser_xor(void)
{
  int mx, my, sz;

  if (cur_tool == TOOL_ERASER)
  {
    SDL_GetMouseState(&mx, &my);
    /* FIXME: If you're moving the mouse WHILE hitting Ctrl-Z,
       the XOR could happen in the wrong place :-/ -bjk 2023.05.22 */
    if (hit_test(&r_canvas, mx, my))
    {
      mx = mx - r_canvas.x;
      my = my - r_canvas.y;

      sz = calc_eraser_size(cur_eraser);
      if (cur_eraser >= NUM_ERASER_SIZES)
      {
        /* Circle eraser (sharp & fuzzy) */
        circle_xor(mx, my, sz / 2);
      }
      else
      {
        /* Square eraser */
        rect_xor(mx - sz / 2, my - sz / 2, mx + sz / 2, my + sz / 2);
      }

      update_screen(mx - sz / 2 + r_canvas.x, my - sz / 2 + r_canvas.y,
                    mx + sz / 2 + r_canvas.x, my + sz / 2 + r_canvas.y);
    }
  }
}

/**
 * Record an undo buffer snapshot, blit the current line of Text or Label
 * tool text onto the canvas, play the "carriage return" sound effect,
 * position the cursor down a line (or to the very bottom of the canvas),
 * and mark the current drawing as "unsaved".
 *
 * This happens when the user presses the [Enter]/[Return] key on
 * a physical keyboard, clicks it in the on-screen keyboard, or
 * a carriage return is part of some pasted clipboard text.
 *
 * If the cursor would surpass the bottom of the screen
 * (based on `font_height`), then this function returns true/1,
 * indicating the situation; this is used when the carriage return
 * occurs due to clipboard text being pasted, so that process can
 * be aborted.
 *
 * @param int font_height -- [line] height of the current font
 * @return int -- whether or not the cursor would be too far
 *   down the canvas' height
 */
static int text_label_tool_enter(int font_height)
{
  int exceeded;

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

  exceeded = 0;
  if (cursor_y + font_height >= canvas->h)
    exceeded = 1;
  cursor_y = min(cursor_y + font_height, canvas->h - font_height);

  /* Reposition the on-screen keyboard if we begin typing over it */
  update_canvas_ex(kbd_rect.x, kbd_rect.y, kbd_rect.x + kbd_rect.w, kbd_rect.y + kbd_rect.h, 0);
  update_screen_rect(&kbd_rect);
  reposition_onscreen_keyboard(cursor_y);

  playsound(screen, 0, SND_RETURN, 1, SNDPOS_RIGHT, SNDDIST_NEAR);

  return exceeded;
}
