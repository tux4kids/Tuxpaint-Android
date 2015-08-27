/*
  postscript_print.h


  For Tux Paint
  PostScript(r) printing routine.
  (for non-Windows, non-Mac OS X, non-BeOS platforms, e.g. Linux)
  (moved from tuxpaint.c in 0.9.17)

  Copyright (c) 2008 by Bill Kendrick and others
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

  June 24, 2007 - December 7, 2008
  $Id: postscript_print.h,v 1.5 2009/11/23 07:45:25 albert Exp $
*/

#ifndef POSTSCRIPT_PRINT_H
#define POSTSCRIPT_PRINT_H

#include <stdio.h>
#include <sys/wait.h>
#include "SDL2/SDL.h"
#include "compiler.h"


/* Method for printing images: */

/* FIXME: We should either settle on direct PostScript printing and remove
   the other options, or move these settings to Makefile -bjk 2007.06.25 */

#define PRINTMETHOD_PS          /* Direct to PostScript */
//#define PRINTMETHOD_PNM_PS       /* Output PNM, assuming it gets printed */
//#define PRINTMETHOD_PNG_PNM_PS   /* Output PNG, assuming it gets printed */



/* Default print and alt-print command, depending on the print method: */

#define DEFAULT_PRINTCOMMAND "lpr"
#define DEFAULT_ALTPRINTCOMMAND "kprinter"

#ifdef PRINTMETHOD_PNG_PNM_PS
#define PRINTCOMMAND "pngtopnm | pnmtops | " DEFAULT_PRINTCOMMAND
#elif defined(PRINTMETHOD_PNM_PS)
#define PRINTCOMMAND "pnmtops | " DEFAULT_PRINTCOMMAND
#elif defined(PRINTMETHOD_PS)
#define PRINTCOMMAND DEFAULT_PRINTCOMMAND
#else
#error No print method defined!
#endif

#ifdef PRINTMETHOD_PNG_PNM_PS
#define ALTPRINTCOMMAND "pngtopnm | pnmtops | " DEFAULT_ALTPRINTCOMMAND
#elif defined(PRINTMETHOD_PNM_PS)
#define ALTPRINTCOMMAND "pnmtops | " DEFAULT_ALTPRINTCOMMAND
#elif defined(PRINTMETHOD_PS)
#define ALTPRINTCOMMAND DEFAULT_ALTPRINTCOMMAND
#else
#error No alt print method defined!
#endif


#ifdef PRINTMETHOD_PS

int do_ps_save(FILE * fi,
		const char *restrict const fname,
		SDL_Surface * surf,
	        const char *restrict pprsize,
                int is_pipe);

#endif

#endif /* POSTSCRIPT_PRINT_H */

