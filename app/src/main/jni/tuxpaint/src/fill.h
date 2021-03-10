/*
  fill.h

  Fill tool
  Tux Paint - A simple drawing program for children.

  Copyright (c) 2002-2019 by Bill Kendrick and others; see AUTHORS.txt
  bill@newbreedsoftware.com
  http://www.tuxpaint.org/

  Flood fill code based on Wikipedia example:
  http://www.wikipedia.org/wiki/Flood_fill/C_example
  by Damian Yerrick - http://www.wikipedia.org/wiki/Damian_Yerrick

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

  Last updated: February 20, 2021
  $Id$
*/

#ifndef FILL_H
#define FILL_H

#include "SDL.h"

int would_flood_fill(SDL_Surface * canvas, Uint32 cur_colr, Uint32 old_colr);
void do_flood_fill(SDL_Surface * canvas, int x, int y, Uint32 cur_colr, Uint32 old_colr, int * x1, int * y1, int * x2, int * y2);
void simulate_flood_fill(SDL_Surface * canvas, int x, int y, Uint32 cur_colr, Uint32 old_colr, int * x1, int * y1, int * x2, int * y2, Uint8 * touched);
void draw_linear_gradient(SDL_Surface * canvas, SDL_Surface * last,
  int x_left, int y_top, int x_right, int y_bottom,
  int x1, int y1, int x2, int y2, Uint32 draw_color, Uint8 * touched);
void draw_radial_gradient(SDL_Surface * canvas, int x_left, int y_top, int x_right, int y_bottom,
  int x, int y, Uint32 draw_color, Uint8 * touched);

#endif

