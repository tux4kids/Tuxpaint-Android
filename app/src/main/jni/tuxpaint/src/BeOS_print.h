/* BeOS_print.h */  
  
/* printing support for Tux Paint */ 
/* Marcin 'Shard' Konicki <shard@beosjournal.org> */ 
  
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
*/ 
  
/* Jan. 17, 2003 */ 
/* $Id: BeOS_print.h,v 1.6 2009/12/08 08:38:31 scottmc Exp $ */ 

#ifndef __BEOS_PRINT_H__
#define __BEOS_PRINT_H__
  
#include "SDL.h"
  
#ifdef __cplusplus
extern "C"
{
  
#endif	/* __cplusplus */
  extern int SurfacePrint(SDL_Surface * surf);
  extern int IsPrinterAvailable();
#ifdef __cplusplus
} 
#endif	/* __cplusplus */
 
#endif	/* __BEOS_PRINT__ */
