//
//  macosx_print.h
//  Tux Paint
//
//  Created by Darrell Walisser on Sat Mar 15 2003.
//  Modified by Martin Fuhrer 2007.
//  Copyright (c) 2007 Darrell Walisser, Martin Fuhrer. All rights reserved.
//  $Id: macosx_print.h,v 1.7 2008/01/04 05:39:16 mfuhrer Exp $
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  (See COPYING.txt)
//

#include "SDL.h"

const char *SurfacePrint(SDL_Surface *surface, int showDialog);
int DisplayPageSetup(const SDL_Surface *surface);

#ifdef OBJECTIVEC

@interface PrintSheetController : NSObject
{
	bool displayPrintSetupSheet;
	bool displayPrintSheet;
}
-
@end

#endif OBJECTIVEC
