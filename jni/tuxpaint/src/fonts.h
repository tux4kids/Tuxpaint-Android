/*
  fonts.h

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

  $Id: fonts.h,v 1.21 2014/04/19 18:04:55 wkendrick Exp $
*/

#ifndef FONTS_H
#define FONTS_H

// plan to rip this out as soon as it is considered stable
//#define THREADED_FONTS
#define FORKED_FONTS
#if defined(WIN32) || defined(__BEOS__)
#undef FORKED_FONTS
#endif
#ifdef __APPLE__
#undef FORKED_FONTS
#endif


#include "SDL.h"
#include "SDL_ttf.h"

#ifndef NO_SDLPANGO
#include "SDL2_Pango.h"
#endif

#define PANGO_DEFAULT_FONT "BitStream Vera"

#include "compiler.h"

/* Disable threaded font loading on Windows */
#if !defined(FORKED_FONTS) && !defined(WIN32)
#include "SDL_thread.h"
#include "SDL_mutex.h"
#else
/* This shouldn't really be here :-)
 * Move into 'fonts.c' and the code in 'tuxpaint.c' 
 * that uses this lot should be put into 'fonts.c' as well.
 */

#ifdef NO_SDLPANGO
/* Only kill SDL_thread stuff when we're not using Pango, because we need it to let fontconfig make its cache (takes a long time the first time) -bjk 2010.04.27 */
#define SDL_CreateThread(fn,vp) (void*)(long)(fn(vp))
#define SDL_WaitThread(tid,rcp) do{(void)tid;(void)rcp;}while(0)
#define SDL_Thread int
#define SDL_mutex int
#define SDL_CreateMutex() 0	// creates in released state
#define SDL_DestroyMutex(lock)
#define SDL_mutexP(lock)	// take lock
#define SDL_mutexV(lock)	// release lock
#endif

#endif

extern SDL_Thread *font_thread;

extern volatile long font_thread_done;
extern volatile long font_thread_aborted;
extern volatile long waiting_for_fonts;
extern int font_socket_fd;

extern int no_system_fonts;
extern int all_locale_fonts;

/* FIXME: SDL_ttf is up to 2.0.8, so we can probably fully remove this;
   -bjk 2007.06.05 */
/*
TTF_Font *BUGFIX_TTF_OpenFont206(const char *const file, int ptsize);
#define TTF_OpenFont    BUGFIX_TTF_OpenFont206
*/


/* Stuff that wraps either SDL_Pango or SDL_TTF for font rendering: */

enum {
#ifndef NO_SDLPANGO
  FONT_TYPE_PANGO,
#endif
  FONT_TYPE_TTF
};

typedef struct TuxPaint_Font_s {
#ifndef NO_SDLPANGO
  SDLPango_Context * pango_context;
#endif
  int typ;
  TTF_Font * ttf_font;
  int height;
  char * desc;
} TuxPaint_Font;

int TuxPaint_Font_FontHeight(TuxPaint_Font * tpf);


#ifdef FORKED_FONTS
void reliable_write(int fd, const void *buf, size_t count);
static void reliable_read(int fd, void *buf, size_t count);
void run_font_scanner(SDL_Surface * screen, SDL_Texture * texture, SDL_Renderer * renderer, const char *restrict const locale);
void receive_some_font_info(SDL_Surface * screen, SDL_Texture * texture, SDL_Renderer * renderer);
#endif

//////////////////////////////////////////////////////////////////////
// font stuff

// example from a Debian box with MS fonts:
// start with 232 files
// remove "Cursor", "Webdings", "Dingbats", "Standard Symbols L"
// split "Condensed" faces out into own family
// group by family
// end up with 34 user choices

extern int text_state;
extern unsigned text_size;

// nice progression (alternating 33% and 25%) 9 12 18 24 36 48 72 96 144 192
// commonly hinted sizes seem to be: 9, 10, 12, 14, 18, 20 (less so), 24
// reasonable: 9,12,18... and 10,14,18...
static int text_sizes[] = {
#ifndef OLPC_XO
  9,
#endif
  12, 18, 24, 36, 48,
  56, 64, 96, 112, 128, 160
};				// point sizes

#define MIN_TEXT_SIZE 0u
#define MAX_TEXT_SIZE (sizeof text_sizes / sizeof text_sizes[0] - 1)

// for sorting through the font files at startup
typedef struct style_info
{
  char *filename;
  char *directory;
  char *family;			// name like "FooCorp Thunderstruck"
  char *style;			// junk like "Oblique Demi-Bold"
  int italic;
  int boldness;
  int score;
  int truetype;			// Is it? (TrueType gets priority)
} style_info;

// user's notion of a font
typedef struct family_info
{
  char *directory;
  char *family;
  char *filename[4];
  TuxPaint_Font *handle;
  int score;
} family_info;

extern TuxPaint_Font *medium_font, *small_font, *large_font, *locale_font;

extern family_info **user_font_families;
extern int num_font_families;

extern style_info **user_font_styles;
extern int num_font_styles;
extern int num_font_styles_max;

extern int button_label_y_nudge;

TuxPaint_Font *getfonthandle(int desire);

int charset_works(TuxPaint_Font * font, const char *s);

TuxPaint_Font * TuxPaint_Font_OpenFont(const char * pangodesc, const char * ttffilename, int size);
void TuxPaint_Font_CloseFont(TuxPaint_Font * tpf);
const char * TuxPaint_Font_FontFaceFamilyName(TuxPaint_Font * tpf);
const char * TuxPaint_Font_FontFaceStyleName(TuxPaint_Font * tpf);

#ifdef NO_SDLPANGO
TuxPaint_Font *load_locale_font(TuxPaint_Font * fallback, int size);
#else
void sdl_color_to_pango_color(SDL_Color sdl_color, SDLPango_Matrix *pango_color);
#endif

int load_user_fonts(SDL_Surface * screen, SDL_Texture * texture, SDL_Renderer * renderer, void *vp, const char *restrict const locale);

#endif
