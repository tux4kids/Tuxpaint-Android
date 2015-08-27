/* BeOS_print.cpp */

/* printing support for Tux Paint */
/* Marcin 'Shard' Konicki <shard at beosjournal.org> */

/*
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

  $Id: BeOS_print.cpp,v 1.3 2009/06/03 20:46:07 wkendrick Exp $
*/
  
/* Jan. 17, 2003 */

#include "BeOS_print.h"

#include "Bitmap.h"
#include "Messenger.h"
#include "PrintJob.h"
#include "Window.h"
#include "View.h"

#include "dirent.h"
#include "string.h"


class PrintView : public BView
{
	public:
					PrintView( BBitmap *bitmap)
						: BView( bitmap->Bounds(), "TuxPaint Print", B_FOLLOW_NONE, B_WILL_DRAW)
						{
							b = bitmap;
						};
					~PrintView()
						{
							delete b;
						};
		void		Draw( BRect updateRect)
						{
							DrawBitmap( b);
						}
	private:
		BBitmap	*b;
};


BBitmap *SurfaceToBBitmap( SDL_Surface *surf)
{
	BBitmap				*bitmap = new BBitmap( BRect( 0, 0, surf->w, surf->h), B_RGBA32);
    SDL_PixelFormat     pixfmt;
    SDL_Surface         *surf32;
    Uint8               *src,*dst;
    Uint32              linesize;
    int                 i;

    memset( &pixfmt, 0, sizeof(pixfmt) );
    pixfmt.palette      = NULL;
    pixfmt.BitsPerPixel = 32;
    pixfmt.BytesPerPixel= 4;
    pixfmt.Rmask        = 0x00FF0000;
    pixfmt.Gmask        = 0x0000FF00;
    pixfmt.Bmask        = 0x000000FF;
    pixfmt.Amask        = 0xFF000000;
    pixfmt.Rshift       = 16;
    pixfmt.Gshift       = 8;
    pixfmt.Bshift       = 0;
    pixfmt.Ashift       = 24;
    pixfmt.Rloss        = 0;
    pixfmt.Gloss        = 0;
    pixfmt.Bloss        = 0;
    pixfmt.Aloss        = 0;
    pixfmt.colorkey     = 0;
    pixfmt.alpha        = 0;

    surf32 = SDL_ConvertSurface( surf, &pixfmt, SDL_SWSURFACE );
 
    linesize = surf32->w*sizeof(Uint32);
    dst = (Uint8*)bitmap->Bits();
    src = (Uint8*)surf32->pixels;
    for ( i = 0; i < surf32->h; i++ )
    {
        memcpy( dst, src, linesize );
        src += surf32->pitch-4;
        dst += linesize;
    }

    SDL_FreeSurface( surf32 );              /* Free temp surface */

    return bitmap;
}


int IsPrinterAvailable( void )
{
	// this code is a little hack, i don't like such hardcoded things
	// but i have no choice ;]
	DIR *d;
	struct dirent *f = NULL;
	int num_files = 0;
	d = opendir("/boot/home/config/settings/printers");
	if( d != NULL)
	{
		while( (f = readdir(d)) != NULL)
			num_files++;
		closedir( d);
		if( num_files > 2)
			return 1;
	}
	return 0;
}


int SurfacePrint( SDL_Surface *surf )
{
	BWindow *window = new BWindow( BRect( 0, 0, surf->w, surf->h), "TuxPaint Print", B_NO_BORDER_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_AVOID_FRONT | B_AVOID_FOCUS);
	PrintView *view = new PrintView( SurfaceToBBitmap( surf));
	window->AddChild(view);
	window->Run();

	BPrintJob job("TuxPaint");
	if( job.ConfigPage() == B_OK)
	{
		if( job.ConfigJob() == B_OK)
		{
			job.BeginJob();
			if( job.CanContinue())
			{
				job.DrawView(view, BRect( 0, 0, surf->w, surf->h), BPoint( 0, 0));
				job.SpoolPage();
			}
			if( job.CanContinue())
				job.CommitJob();
		}
	}

	BMessenger( window).SendMessage( B_QUIT_REQUESTED);

	return 0;
}
