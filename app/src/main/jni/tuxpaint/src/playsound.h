/* playsound.h

  Copyright (c) 2002-2022
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

  Last updated: December 11, 2022
  $Id$
*/

#ifndef PLAYSOUND_H
#define PLAYSOUND_H

#include "SDL.h"
#include "SDL_mixer.h"
#include "sounds.h"

#define SNDPOS_LEFT -997
#define SNDPOS_CENTER -998
#define SNDPOS_RIGHT -999

#define SNDDIST_NEAR -999

extern Mix_Chunk *sounds[NUM_SOUNDS];
extern int use_sound, use_stereo;

void playsound(SDL_Surface * screen, int chan, int s, int override, int x, int y);

#endif
