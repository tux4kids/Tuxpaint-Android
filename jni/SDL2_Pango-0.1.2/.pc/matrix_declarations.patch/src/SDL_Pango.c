/*  SDL_Pango.c -- A companion library to SDL for working with Pango.
    Copyright (C) 2004 NAKAMURA Ken'ichi

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
*/

/*!
    \mainpage

    \section intro Introduction

    Pango is the text rendering engine of GNOME 2.x. SDL_Pango connects the 
    engine to SDL. In Windows, pre-built binary package (MSI and merge module) 
    is provided.

    \subsection dist Distribution

    If you are a game software developer, you should know the difficulties of 
    distribution. So I will start to introduce SDL_Pango from the viewpoint 
    of distribution.

    In Un*x, SDL_Pango is hard to use as system-independent module, because 
    it depends on fontconfig and Pango which are designed as system-singleton 
    modules. If you use SDL_Pango, your software will require those modules 
    installed to target system. If your software is shipped as shrink-wrap 
    package, it may cause much problem on your support desk. You should 
    carefully design your installation process.

    In Windows, SDL_Pango is distributed as "merge module" which contains 
    fontconfig and Pango. Those binaries are modified as side-by-side components.
    You should use Windows Installer and merge the module 
    on your MSI package. The merge module not only contains files, but also includes 
    custom action which must be run at installation.

    \subsection api High-level API

    From the viewpoint of text rendering, the heart of SDL_Pango is high-level API. 
    Other text rendering APIs, like DrawText() of Windows, font and text must be 
    specified separately. In SDL_Pango, font specification is embedded in text like 
    HTML:

    \code
    <span font_family="Courier New"><i>This is Courier New and italic.</i></span>
    \endcode

    Color, size, subscript/superscript, obliquing, weight, and other many features 
    are also available in same way.

    \subsection i18n Internationalized Text

    Internationalized text is another key feature. Text is specified by UTF-8. RTL 
    script (Arabic and Hebrew) and complicated rendering (Arabic, Indic and Thai) are 
    supported. You can see it with GNOME 2.x.

    \section get Getting Started

    \subsection getlatest Get latest files

    Get latest files from http://sourceforge.net/projects/sdlpango/ .

    \subsection install Install Header and Library

    In Windows and VS2003, I strongly recommend you to install MSI package. It contains Pango 
    and fontconfig binaries which are modified as side-by-side components. It is 
    nearly impossible to build them. (I spent much time to build them...)

    In MinGW, I recommend you to use VS2003. Otherwise you may run into the maze of 
    distribution. If you insist MinGW, you should use MinGW binary archive.

    In Un*x, installation consists of: 

    \code
    ./configure
    make
    make install
    \endcode

    \subsection inc Includes

    To use SDL_Pango functions in a C/C++ source code file, you must use the SDL_Pango.h 
    include file:

    \code
    #include "SDL_Pango.h"
    \endcode

    In Windows, SDL_Pango.h is installed on \c \%ProgramFiles\%\\SDL_Pango \c Development\\include 
    (usually \c C:\\Program \c Files\\SDL_Pango \c Development\\include). You should add this 
    directory to include path.

    \subsection comp Compiling

    In Un*x, to link with SDL_Pango you should use sdl-config to get the required SDL 
    compilation options. After that, compiling with SDL_Pango is quite easy.

    Note: Some systems may not have the SDL_Pango library and include file in the same 
    place as the SDL library and includes are located, in that case you will need to 
    add more -I and -L paths to these command lines.

    Simple Example for compiling an object file:

    \code
    cc -c `sdl-config --cflags` mysource.c
    \endcode

    Simple Example for linking an object file:

    \code
    cc -o myprogram mysource.o `sdl-config --libs` -lSDL_Pango
    \endcode

    Now myprogram is ready to run. 
    
    You can see a sample of autoconfiscation in 'test' directory.

    In Windows, MSI package installs many dlls to \c \%ProgramFiles\%\\SDL_Pango \c Development\\import_lib. 
    To link with SDL_Pango you should use SDL_Pango.lib.

    SDL_Pango.dll depends on many dlls and other many files. Those dlls are installed on 
    \c \%ProgramFiles\%\\SDL_Pango \c Development\\bin. MSI package adds the directory to PATH environment 
    variable.

    \section devel Development

    \subsection font Font Handling

    In Un*x, font handling depends on fontconfig of your system.

    In Windows, local.conf of fontconfig is placed on \c \%ProgramFiles\%\\SDL_Pango \c Development\\etc\\fonts. 
    You should know about fontconfig's font cache mechanism.

    \subsection example Step-by-step Example

    The operation of SDL_Pango is done via context.

    \code
    SDLPango_Context *context = SDLPango_CreateContext();
    \endcode

    Specify default colors and minimum surface size.

    \code
    SDLPango_SetDefaultColor(context, MATRIX_TRANSPARENT_BACK_WHITE_LETTER);
    SDLPango_SetMinimumSize(context, 640, 0);
    \endcode

    Set markup text.

    \code
    SDLPango_SetMarkup(context, "This is <i>markup</i> text.", -1);
    \endcode

    Now you can get the size of surface.

    \code
    int w = SDLPango_GetLayoutWidth(context);
    int h = SDLPango_GetLayoutHeight(context);
    \endcode

    Create surface to draw.

    \code
    int margin_x = 10;
    int margin_y = 10;
    SDL_Surface *surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 
	w + margin_x * 2, h + margin_y * 2,
	32, (Uint32)(255 << (8 * 3)), (Uint32)(255 << (8 * 2)),
	(Uint32)(255 << (8 * 1)), 255);
    \endcode

    And draw on it.

    \code
    SDLPango_Draw(context, surface, margin_x, margin_y);
    \endcode

    You must free the surface by yourself.

    \code
    SDL_FreeSurface(surface);
    \endcode

    Free context.

    \code
    SDLPango_FreeContext(context);
    \endcode

    You can see actual code in \c test/testbench.cpp.

    \subsection pack Packaging

    In Un*x, do it yourself.

    In Windows, font files must be installed on apprication folder (usually 
    \c C:\\Program \c Files\\[Manufacturer]\\[ProductName]). The property of 
    apprication folder must be \c TARGETDIR (this is default setting of VS2003). 
    SDL.dll also must be installed on apprication folder. Add SDL_Pango.msm to 
    your MSI package.

    \section ack Acknowledgment

    SDL_Pango is developed with financial assistance of Information-technology Promotion Agency, Japan.

- NAKAMURA Ken'ichi <nakamura@sbp.fp.a.u-tokyo.ac.jp>
    
*/

/*! @file
    @brief Implementation of SDL_Pango

    @author NAKAMURA Ken'ichi
    @date   2004/12/07
    $Revision: 1.6 $
*/

#include <pango/pango.h>
#include <pango/pangoft2.h>

#include "SDL_Pango.h"

//! non-zero if initialized
static int IS_INITIALIZED = 0;

#define DEFAULT_FONT_FAMILY "Sans"
#define DEFAULT_FONT_SIZE 12
#define DEFAULT_DPI 96
#define _MAKE_FONT_NAME(family, size) family " " #size
#define MAKE_FONT_NAME(family, size) _MAKE_FONT_NAME(family, size)
#define DEFAULT_DEPTH 32
#define DEFAULT_RMASK (Uint32)(255 << (8 * 3))
#define DEFAULT_GMASK (Uint32)(255 << (8 * 2))
#define DEFAULT_BMASK (Uint32)(255 << (8 * 1))
#define DEFAULT_AMASK (Uint32)255

static FT_Bitmap *createFTBitmap(int width, int height);

static void freeFTBitmap(FT_Bitmap *bitmap);

static void getItemProperties (
    PangoItem      *item,
    PangoUnderline *uline,
    gboolean       *strikethrough,
    gint           *rise,
    PangoColor     *fg_color,
    gboolean       *fg_set,
    PangoColor     *bg_color,
    gboolean       *bg_set,
    gboolean       *shape_set,
    PangoRectangle *ink_rect,
    PangoRectangle *logical_rect);

static void clearFTBitmap(FT_Bitmap *bitmap);

typedef struct _surfaceArgs {
    Uint32 flags;
    int depth;
    Uint32 Rmask;
    Uint32 Gmask;
    Uint32 Bmask;
    Uint32 Amask;
} surfaceArgs;

typedef struct _contextImpl {
    PangoContext *context;
    PangoFontMap *font_map;
    PangoFontDescription *font_desc;
    PangoLayout *layout;
    surfaceArgs surface_args;
    FT_Bitmap *tmp_ftbitmap;
    SDLPango_Matrix color_matrix;
    int min_width;
    int min_height;
} contextImpl;


/*!
    Initialize the Glib and Pango API.
    This must be called before using other functions in this library,
    excepting SDLPango_WasInit.
    SDL does not have to be initialized before this call.


    @return always 0.
*/
int
SDLPango_Init()
{
    g_type_init();

    IS_INITIALIZED = -1;

    return 0;
}

/*!
    Query the initilization status of the Glib and Pango API.
    You may, of course, use this before SDLPango_Init to avoid
    initilizing twice in a row.

    @return zero when already initialized.
    non-zero when not initialized.
*/
int
SDLPango_WasInit()
{
    return IS_INITIALIZED;
}

/*!
    Draw glyphs on rect.

    @param *context [in] Context
    @param *surface [out] Surface to draw on it
    @param *color_matrix [in] Foreground and background color
    @param *font [in] Innter variable of Pango
    @param *glyphs [in] Innter variable of Pango
    @param *rect [in] Draw on this area
    @param baseline [in] Horizontal location of glyphs
*/
static void
drawGlyphString(
    SDLPango_Context *context,
    SDL_Surface *surface,
    SDLPango_Matrix *color_matrix,
    PangoFont *font,
    PangoGlyphString *glyphs,
    SDL_Rect *rect,
    int baseline)
{
    pango_ft2_render(context->tmp_ftbitmap, font, glyphs, rect->x, rect->y + baseline);

    SDLPango_CopyFTBitmapToSurface(
	context->tmp_ftbitmap,
	surface,
	color_matrix,
	rect);

    clearFTBitmap(context->tmp_ftbitmap);
}

/*!
    Draw horizontal line of a pixel.

    @param *surface [out] Surface to draw on it
    @param *color_matrix [in] Foreground and background color
    @param y [in] Y location of line
    @param start [in] Left of line
    @param end [in] Right of line
*/
static void drawHLine(
    SDL_Surface *surface,
    SDLPango_Matrix *color_matrix,
    int y,
    int start,
    int end)
{
    Uint8 *p;
    Uint16 *p16;
    Uint32 *p32;
    Uint32 color;
    int ix;
    int pixel_bytes = surface->format->BytesPerPixel;

    if (y < 0 || y >= surface->h)
	return;

    if (end <= 0 || start >= surface->w)
	return;

    if (start < 0)
	start = 0;

    if (end >= surface->w)
	end = surface->w;

    p = (Uint8 *)(surface->pixels) + y * surface->pitch + start * pixel_bytes;
    color = SDL_MapRGBA(surface->format,
	color_matrix->m[0][1],
	color_matrix->m[1][1], 
	color_matrix->m[2][1], 
	color_matrix->m[3][1]);

    switch(pixel_bytes) {
    case 2:
	p16 = (Uint16 *)p;
	for (ix = 0; ix < end - start; ix++)
	    *p16++ = (Uint16)color;
	break;
    case 4:
	p32 = (Uint32 *)p;
	for (ix = 0; ix < end - start; ix++)
	    *p32++ = color;
	break;
    default:
	SDL_SetError("surface->format->BytesPerPixel is invalid value");
	break;
    }
}

/*!
    Draw a line.

    @param *context [in] Context
    @param *surface [out] Surface to draw on it
    @param *line [in] Innter variable of Pango
    @param x [in] X location of line
    @param y [in] Y location of line
    @param height [in] Height of line
    @param baseline [in] Rise / sink of line (for super/subscript)
*/
static void
drawLine(
    SDLPango_Context *context,
    SDL_Surface *surface,
    PangoLayoutLine *line,
    gint x, 
    gint y, 
    gint height,
    gint baseline)
{
    GSList *tmp_list = line->runs;
    PangoColor fg_color, bg_color;
    PangoRectangle logical_rect;
    PangoRectangle ink_rect;
    int x_off = 0;

    while (tmp_list) {
	SDLPango_Matrix color_matrix = context->color_matrix;
	PangoUnderline uline = PANGO_UNDERLINE_NONE;
	gboolean strike, fg_set, bg_set, shape_set;
	gint rise, risen_y;
	PangoLayoutRun *run = tmp_list->data;
	SDL_Rect d_rect;

	tmp_list = tmp_list->next;

	getItemProperties(run->item,
	    &uline, &strike, &rise,
	    &fg_color, &fg_set, &bg_color, &bg_set,
	    &shape_set, &ink_rect, &logical_rect);

	risen_y = y + baseline - PANGO_PIXELS (rise);

	if(fg_set) {
	    color_matrix.m[0][1] = (Uint8)(fg_color.red >> 8);
	    color_matrix.m[1][1] = (Uint8)(fg_color.green >> 8);
	    color_matrix.m[2][1] = (Uint8)(fg_color.blue >> 8);
	    color_matrix.m[3][1] = 255;
	    if(color_matrix.m[3][0] == 0) {
		color_matrix.m[0][0] = (Uint8)(fg_color.red >> 8);
		color_matrix.m[1][0] = (Uint8)(fg_color.green >> 8);
		color_matrix.m[2][0] = (Uint8)(fg_color.blue >> 8);
	    }
	}

	if (bg_set) {
	    color_matrix.m[0][0] = (Uint8)(bg_color.red >> 8);
	    color_matrix.m[1][0] = (Uint8)(bg_color.green >> 8);
	    color_matrix.m[2][0] = (Uint8)(bg_color.blue >> 8);
	    color_matrix.m[3][0] = 255;
	}

	if(! shape_set) {
	    if (uline == PANGO_UNDERLINE_NONE)
		pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
					    NULL, &logical_rect);
	    else
		pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
					    &ink_rect, &logical_rect);

	    d_rect.w = (Uint16)PANGO_PIXELS(logical_rect.width);
	    d_rect.h = (Uint16)height;
	    d_rect.x = (Uint16)(x + PANGO_PIXELS (x_off));
	    d_rect.y = (Uint16)(risen_y - baseline);

	    if((! context->tmp_ftbitmap) || d_rect.w + d_rect.x > context->tmp_ftbitmap->width
		|| d_rect.h + d_rect.y > context->tmp_ftbitmap->rows)
	    {
		freeFTBitmap(context->tmp_ftbitmap);
		context->tmp_ftbitmap = createFTBitmap(d_rect.w + d_rect.x, d_rect.h + d_rect.y);
	    }

	    drawGlyphString(context, surface, 
		&color_matrix, 
		run->item->analysis.font, run->glyphs, &d_rect, baseline);
	}
        switch (uline) {
	case PANGO_UNDERLINE_NONE:
	    break;
	case PANGO_UNDERLINE_DOUBLE:
	    drawHLine(surface, &color_matrix,
		risen_y + 4,
		x + PANGO_PIXELS (x_off + ink_rect.x),
		x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width));
	  /* Fall through */
	case PANGO_UNDERLINE_SINGLE:
	    drawHLine(surface, &color_matrix,
		risen_y + 2,
		x + PANGO_PIXELS (x_off + ink_rect.x),
		x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width));
	    break;
	case PANGO_UNDERLINE_ERROR:
	    {
		int point_x;
		int counter = 0;
		int end_x = x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width);

		for (point_x = x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
		    point_x <= end_x;
		    point_x += 2)
		{
		    if (counter)
			drawHLine(surface, &color_matrix,
			    risen_y + 2,
			    point_x, MIN (point_x + 1, end_x));
		    else
			drawHLine(surface, &color_matrix,
			    risen_y + 3,
			    point_x, MIN (point_x + 1, end_x));
    		
		    counter = (counter + 1) % 2;
		}
	    }
	    break;
	case PANGO_UNDERLINE_LOW:
	    drawHLine(surface, &color_matrix,
		risen_y + PANGO_PIXELS (ink_rect.y + ink_rect.height),
		x + PANGO_PIXELS (x_off + ink_rect.x),
		x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width));
	  break;
	}

        if (strike)
	    drawHLine(surface, &color_matrix,
		risen_y + PANGO_PIXELS (logical_rect.y + logical_rect.height / 2),
		x + PANGO_PIXELS (x_off + logical_rect.x),
		x + PANGO_PIXELS (x_off + logical_rect.x + logical_rect.width));

	x_off += logical_rect.width;
    }
}

/*!
    Innter function of Pango. Stolen from GDK.

    @param *item [in] The item to get property
    @param *uline [out] Kind of underline
    @param *strikethrough [out] Strike-through line
    @param *rise [out] Rise/sink of line (for super/subscript)
    @param *fg_color [out] Color of foreground
    @param *fg_set [out] True if fg_color set
    @param *bg_color [out] Color of background
    @param *bg_set [out] True if bg_color valid
    @param *shape_set [out] True if ink_rect and logical_rect valid
    @param *ink_rect [out] Ink rect
    @param *logical_rect [out] Logical rect
*/
static void
getItemProperties (
    PangoItem *item,
    PangoUnderline *uline,
    gboolean *strikethrough,
    gint *rise,
    PangoColor *fg_color,
    gboolean *fg_set,
    PangoColor *bg_color,
    gboolean *bg_set,
    gboolean *shape_set,
    PangoRectangle *ink_rect,
    PangoRectangle *logical_rect)
{
    GSList *tmp_list = item->analysis.extra_attrs;

    if (strikethrough)
	*strikethrough = FALSE;
  
    if (fg_set)
        *fg_set = FALSE;

    if (bg_set)
	*bg_set = FALSE;

    if (shape_set)
	*shape_set = FALSE;

    if (rise)
	*rise = 0;

    while (tmp_list) {
	PangoAttribute *attr = tmp_list->data;

	switch (attr->klass->type) {
	case PANGO_ATTR_UNDERLINE:
	    if (uline)
		*uline = ((PangoAttrInt *)attr)->value;
	    break;

	case PANGO_ATTR_STRIKETHROUGH:
	    if (strikethrough)
		*strikethrough = ((PangoAttrInt *)attr)->value;
	    break;
    	
	case PANGO_ATTR_FOREGROUND:
	    if (fg_color)
		*fg_color = ((PangoAttrColor *)attr)->color;
	    if (fg_set)
		*fg_set = TRUE;
	    break;
    	
	case PANGO_ATTR_BACKGROUND:
	    if (bg_color)
		*bg_color = ((PangoAttrColor *)attr)->color;
	    if (bg_set)
		*bg_set = TRUE;
	    break;

	case PANGO_ATTR_SHAPE:
	    if (shape_set)
		*shape_set = TRUE;
	    if (logical_rect)
		*logical_rect = ((PangoAttrShape *)attr)->logical_rect;
	    if (ink_rect)
		*ink_rect = ((PangoAttrShape *)attr)->ink_rect;
	    break;

	case PANGO_ATTR_RISE:
	    if (rise)
		*rise = ((PangoAttrInt *)attr)->value;
	    break;
    	
	default:
	    break;
	}
	tmp_list = tmp_list->next;
    }
}

/*!
    Copy bitmap to surface. 
    From (x, y)-(w, h) to (x, y)-(w, h) of rect. 

    @param *bitmap [in] Grayscale bitmap
    @param *surface [out] Surface
    @param *matrix [in] Foreground and background color
    @param *rect [in] Rect to copy
*/
void
SDLPango_CopyFTBitmapToSurface(
    const FT_Bitmap *bitmap,
    SDL_Surface *surface,
    const SDLPango_Matrix *matrix,
    SDL_Rect *rect)
{
    int i;
    Uint8 *p_ft;
    Uint8 *p_sdl;
    int width = rect->w;
    int height = rect->h;
    int x = rect->x;
    int y = rect->y;

    if(x + width > surface->w) {
	width = surface->w - x;
	if(width <= 0)
	    return;
    }
    if(y + height > surface->h) {
	height = surface->h - y;
	if(height <= 0)
	    return;
    }

    if(SDL_LockSurface(surface)) {
	SDL_SetError("surface lock failed");
	SDL_FreeSurface(surface);
	return;
    }

    p_ft = (Uint8 *)bitmap->buffer + (bitmap->pitch * y);
    p_sdl = (Uint8 *)surface->pixels + (surface->pitch * y);
    for(i = 0; i < height; i ++) {
	int k;
	for(k = 0; k < width; k ++) {
	    /* TODO: rewrite by matrix calculation library */
	    Uint8 pixel[4];	/* 4: RGBA */
	    int n;

	    for(n = 0; n < 4; n ++) {
		Uint16 w;
		w = ((Uint16)matrix->m[n][0] * (256 - p_ft[k + x])) + ((Uint16)matrix->m[n][1] * p_ft[k + x]);
		pixel[n] = (Uint8)(w >> 8);
	    }

	    switch(surface->format->BytesPerPixel) {
	    case 2:
		((Uint16 *)p_sdl)[k + x] = (Uint16)SDL_MapRGBA(surface->format, pixel[0], pixel[1], pixel[2], pixel[3]);
		break;
	    case 4:
		((Uint32 *)p_sdl)[k + x] = SDL_MapRGBA(surface->format, pixel[0], pixel[1], pixel[2], pixel[3]);
		break;
	    default:
		SDL_SetError("surface->format->BytesPerPixel is invalid value");
		return;
	    }
	}
	p_ft += bitmap->pitch;
	p_sdl += surface->pitch;
    }

    SDL_UnlockSurface(surface);
}


SDLPango_Context*
SDLPango_CreateContext_GivenFontDesc(const char* font_desc)
{
    SDLPango_Context *context = g_malloc(sizeof(SDLPango_Context));
    G_CONST_RETURN char *charset;

    context->font_map = pango_ft2_font_map_new ();
    pango_ft2_font_map_set_resolution (PANGO_FT2_FONT_MAP (context->font_map), DEFAULT_DPI, DEFAULT_DPI);

    context->context = pango_ft2_font_map_create_context (PANGO_FT2_FONT_MAP (context->font_map));

    g_get_charset(&charset);
    pango_context_set_language (context->context, pango_language_from_string (charset));
    pango_context_set_base_dir (context->context, PANGO_DIRECTION_LTR);

    context->font_desc = pango_font_description_from_string(font_desc);

    context->layout = pango_layout_new (context->context);

    SDLPango_SetSurfaceCreateArgs(context, SDL_SWSURFACE | SDL_SRCALPHA, DEFAULT_DEPTH,
	DEFAULT_RMASK, DEFAULT_GMASK, DEFAULT_BMASK, DEFAULT_AMASK);

    context->tmp_ftbitmap = NULL;

    context->color_matrix = *MATRIX_TRANSPARENT_BACK_BLACK_LETTER;

    context->min_height = 0;
    context->min_width = 0;

    return context;
}

/*!
    Create a context which contains Pango objects.

    @return A pointer to the context as a SDLPango_Context*.
*/
SDLPango_Context*
SDLPango_CreateContext()
{
    SDLPango_CreateContext_GivenFontDesc(MAKE_FONT_NAME(DEFAULT_FONT_FAMILY, DEFAULT_FONT_SIZE));
}

/*!
    Free a context.

    @param *context [i/o] Context to be free
*/
void
SDLPango_FreeContext(SDLPango_Context *context)
{
    freeFTBitmap(context->tmp_ftbitmap);

    g_object_unref (context->layout);

    pango_font_description_free(context->font_desc);

    g_object_unref(context->context);

    g_object_unref(context->font_map);

    g_free(context);
}

/*!
    Specify Arguments when create a surface.
    When SDL_Pango creates a surface, the arguments are used.

    @param *context [i/o] Context
    @param flags [in] Same as SDL_CreateRGBSurface()
    @param depth [in] Same as SDL_CreateRGBSurface()
    @param Rmask [in] Same as SDL_CreateRGBSurface()
    @param Gmask [in] Same as SDL_CreateRGBSurface()
    @param Bmask [in] Same as SDL_CreateRGBSurface()
    @param Amask [in] Same as SDL_CreateRGBSurface()
*/
void
SDLPango_SetSurfaceCreateArgs(
    SDLPango_Context *context,
    Uint32 flags,
    int depth,
    Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    context->surface_args.flags = flags;
    context->surface_args.depth = depth;
    context->surface_args.Rmask = Rmask;
    context->surface_args.Gmask = Gmask;
    context->surface_args.Bmask = Bmask;
    context->surface_args.Amask = Amask;
}

/*!
    Create a surface and draw text on it.
    The size of surface is same as lauout size.

    @param *context [in] Context
    @return A newly created surface
*/
SDL_Surface * SDLPango_CreateSurfaceDraw(
    SDLPango_Context *context)
{
    PangoRectangle logical_rect;
    SDL_Surface *surface;
    int width, height;

    pango_layout_get_extents (context->layout, NULL, &logical_rect);
    width = PANGO_PIXELS (logical_rect.width);
    height = PANGO_PIXELS (logical_rect.height);
    if(width < context->min_width)
	width = context->min_width;
    if(height < context->min_height)
	height = context->min_height;

    surface = SDL_CreateRGBSurface(
	context->surface_args.flags,
	width, height, context->surface_args.depth,
	context->surface_args.Rmask,
	context->surface_args.Gmask,
	context->surface_args.Bmask,
	context->surface_args.Amask);

    SDLPango_Draw(context, surface, 0, 0);

    return surface;
}

/*!
    Draw text on a existing surface.

    @param *context [in] Context
    @param *surface [i/o] Surface to draw on it
    @param x [in] X of left-top of drawing area
    @param y [in] Y of left-top of drawing area
*/
void
SDLPango_Draw(
    SDLPango_Context *context,
    SDL_Surface *surface,
    int x, int y)
{
    PangoLayoutIter *iter;
    PangoRectangle logical_rect;
    int width, height;

    if(! surface) {
	SDL_SetError("surface is NULL");
	return;
    }

    iter = pango_layout_get_iter (context->layout);

    pango_layout_get_extents (context->layout, NULL, &logical_rect);
    width = PANGO_PIXELS (logical_rect.width);
    height = PANGO_PIXELS (logical_rect.height);

    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));

    if((! context->tmp_ftbitmap) || context->tmp_ftbitmap->width < width
	|| context->tmp_ftbitmap->rows < height)
    {
	freeFTBitmap(context->tmp_ftbitmap);
        context->tmp_ftbitmap = createFTBitmap(width, height);
    }

    do {
	PangoLayoutLine *line;
	int baseline;

	line = pango_layout_iter_get_line (iter);

	pango_layout_iter_get_line_extents (iter, NULL, &logical_rect);
	baseline = pango_layout_iter_get_baseline (iter);

	drawLine(
	    context,
	    surface,
	    line,
	    x + PANGO_PIXELS (logical_rect.x),
	    y + PANGO_PIXELS (logical_rect.y),
	    PANGO_PIXELS (logical_rect.height),
	    PANGO_PIXELS (baseline - logical_rect.y));
    } while (pango_layout_iter_next_line (iter));

    pango_layout_iter_free (iter);
}

/*!
    Allocate buffer and create a FTBitmap object.

    @param width [in] Width
    @param height [in] Height
    @return FTBitmap object
*/
static FT_Bitmap *
createFTBitmap(
    int width, int height)
{
    FT_Bitmap *bitmap;
    guchar *buf;

    bitmap = g_malloc(sizeof(FT_Bitmap));
    bitmap->width = width;
    bitmap->rows = height;
    bitmap->pitch = (width + 3) & ~3;
    bitmap->num_grays = 256;
    bitmap->pixel_mode = FT_PIXEL_MODE_GRAY;
    buf = g_malloc (bitmap->pitch * bitmap->rows);
    memset (buf, 0x00, bitmap->pitch * bitmap->rows);
    bitmap->buffer = buf;

    return bitmap;
}

/*!
    Free a FTBitmap object.

    @param *bitmap [i/o] FTbitmap to be free
*/
static void
freeFTBitmap(
    FT_Bitmap *bitmap)
{
    if(bitmap) {
	g_free(bitmap->buffer);
	g_free(bitmap);
    }
}

/*!
    Clear a FTBitmap object.

    @param *bitmap [i/o] FTbitmap to be clear
*/
static void
clearFTBitmap(
    FT_Bitmap *bitmap)
{
    Uint8 *p = (Uint8 *)bitmap->buffer;
    int length = bitmap->pitch * bitmap->rows;

    memset(p, 0, length);
}

/*!
    Specify minimum size of drawing rect.

    @param *context [i/o] Context
    @param width [in] Width. -1 means no wrapping mode.
    @param height [in] Height. zero/minus value means non-specified.
*/
void
SDLPango_SetMinimumSize(
    SDLPango_Context *context,
    int width, int height)
{
    int pango_width;
    if(width > 0)
	pango_width = width * PANGO_SCALE;
    else
	pango_width = -1;
    pango_layout_set_width(context->layout, pango_width);

    context->min_width = width;
    context->min_height = height;
}

/*!
    Specify default color.

    @param *context [i/o] Context
    @param *color_matrix [in] Foreground and background color
*/
void
SDLPango_SetDefaultColor(
    SDLPango_Context *context,
    const SDLPango_Matrix *color_matrix)
{
    context->color_matrix = *color_matrix;
}

/*!
    Get layout width.

    @param *context [in] Context
    @return Width
*/
int
SDLPango_GetLayoutWidth(
    SDLPango_Context *context)
{
    PangoRectangle logical_rect;

    pango_layout_get_extents (context->layout, NULL, &logical_rect);

    return PANGO_PIXELS (logical_rect.width);
}

/*!
    Get layout height.

    @param *context [in] Context
    @return Height
*/
int
SDLPango_GetLayoutHeight(
    SDLPango_Context *context)
{
    PangoRectangle logical_rect;

    pango_layout_get_extents (context->layout, NULL, &logical_rect);

    return PANGO_PIXELS (logical_rect.height);
}

/*!
    Set markup text to context.
    Text must be utf-8.
    Markup format is same as pango.

    @param *context [i/o] Context
    @param *markup [in] Markup text
    @param length [in] Text length. -1 means NULL-terminated text.
*/
void
SDLPango_SetMarkup(
    SDLPango_Context *context,
    const char *markup,
    int length)
{
    pango_layout_set_markup (context->layout, markup, length);
    pango_layout_set_auto_dir (context->layout, TRUE);
    pango_layout_set_alignment (context->layout, PANGO_ALIGN_LEFT);
    pango_layout_set_font_description (context->layout, context->font_desc);
}

void
SDLPango_SetText_GivenAlignment(
    SDLPango_Context *context,
    const char *text,
    int length,
    SDLPango_Alignment alignment)
{
    pango_layout_set_attributes(context->layout, NULL);
    pango_layout_set_text (context->layout, text, length);
    pango_layout_set_auto_dir (context->layout, TRUE);
    pango_layout_set_alignment (context->layout, alignment);
    pango_layout_set_font_description (context->layout, context->font_desc);
}

/*!
    Set plain text to context.
    Text must be utf-8.

    @param *context [i/o] Context
    @param *text [in] Plain text
    @param length [in] Text length. -1 means NULL-terminated text.
*/
void
SDLPango_SetText(
    SDLPango_Context *context,
    const char *text,
    int length)
{
    SDLPango_SetText_GivenAlignment(context, text, length, SDLPANGO_ALIGN_LEFT);
}

/*!
    Set DPI to context.

    @param *context [i/o] Context
    @param dpi_x [in] X dpi
    @param dpi_y [in] Y dpi
*/
void
SDLPango_SetDpi(
    SDLPango_Context *context,
    double dpi_x, double dpi_y)
{
    pango_ft2_font_map_set_resolution (PANGO_FT2_FONT_MAP (context->font_map), dpi_x, dpi_y);
}

/*!
    Set language to context.

    @param *context [i/o] Context
    @param *language_tag [in] A RFC-3066 format language tag 
*/
void SDLCALL SDLPango_SetLanguage(
    SDLPango_Context *context,
    const char *language_tag)
{
    pango_context_set_language (context->context, pango_language_from_string (language_tag));
}

/*!
    Set base direction to context.

    @param *context [i/o] Context
    @param direction [in] Direction
*/
void SDLCALL SDLPango_SetBaseDirection(
    SDLPango_Context *context,
    SDLPango_Direction direction)
{
    PangoDirection pango_dir;

    switch(direction) {
    case SDLPANGO_DIRECTION_LTR:
	pango_dir = PANGO_DIRECTION_LTR;
	break;
    case SDLPANGO_DIRECTION_RTL:
	pango_dir = PANGO_DIRECTION_RTL;
	break;
    case SDLPANGO_DIRECTION_WEAK_LTR:
	pango_dir = PANGO_DIRECTION_WEAK_LTR;
	break;
    case SDLPANGO_DIRECTION_WEAK_RTL:
	pango_dir = PANGO_DIRECTION_WEAK_RTL;
	break;
    case SDLPANGO_DIRECTION_NEUTRAL:
	pango_dir = PANGO_DIRECTION_NEUTRAL;
	break;
    default:
	SDL_SetError("unknown direction value");
	return;
    }

    pango_context_set_base_dir (context->context, pango_dir);
}

/*!
    Get font map from context.

    @param *context [in] Context
    @return Font map
*/
PangoFontMap* SDLCALL SDLPango_GetPangoFontMap(
    SDLPango_Context *context)
{
    return context->font_map;
}

/*!
    Get font description from context.

    @param *context [in] Context
    @return Font description
*/
PangoFontDescription* SDLCALL SDLPango_GetPangoFontDescription(
    SDLPango_Context *context)
{
    return context->font_desc;
}

/*!
    Get layout from context.

    @param *context [in] Context
    @return Layout
*/
PangoLayout* SDLCALL SDLPango_GetPangoLayout(
    SDLPango_Context *context)
{
    return context->layout;
}
