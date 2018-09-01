/*
  playsound.c

  Copyright (c) 2002-2009
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

  $Id$
*/

#include "playsound.h"
#include "debug.h"

#ifndef NOSOUND
Mix_Chunk *sounds[NUM_SOUNDS];
#endif

int mute;
int use_sound = 1;
static int old_sound[4] = { -1, -1, -1, -1 };

/**
 * Play a sound.
 *
 * @param screen Screen surface (for dealing with panning & volume)
 * @param chan Channel to play on (-1 for first free unused channel)
 * @param s Which sound to play (integer index of `sounds[]` array)
 * @param override 1 to override an already-playing sound, 0 otherwise
 * @param x X coordinate within the screen surface, for left/right panning
 *   effect; or SNDPOS_LEFT, SNDPOS_CENTER, or SNDPOS_RIGHT for
 *   far left, center, or far right panning, respectively.
 * @param y Y coordinate within the screen surface, for volume control
 *   (low values, near the top of the window, are quieter), or
 *   SNDDIST_NEAR for full volume
 */
void playsound(SDL_Surface * screen, int chan, int s, int override, int x, int y)
{
#ifndef NOSOUND
  int left, dist;

  if (!mute && use_sound && s != SND_NONE)
    {
#ifdef DEBUG
  printf("playsound #%d in channel %d, pos (%d,%d), %soverride, ptr=%p\n", s, chan, x, y, override ? "" : "no ", sounds[s]);
  fflush(stdout);
#endif
      if (override || !Mix_Playing(chan))
        {
          Mix_PlayChannel(chan, sounds[s], 0);

          old_sound[chan] = s;
        }

      if (old_sound[chan] == s)
        {
          if (y == SNDDIST_NEAR)
            dist = 0;
          else
            {
              if (y < 0)
                y = 0;
              else if (y >= screen->h - 1)
                y = screen->h - 1;

              dist = (255 * ((screen->h - 1) - y)) / (screen->h - 1);
            }

          if (x == SNDPOS_LEFT)
            left = 255 - dist;
          else if (x == SNDPOS_CENTER)
            left = (255 - dist) / 2;
          else if (x == SNDPOS_RIGHT)
            left = 0;
          else
            {
              if (x < 0)
                x = 0;
              else if (x >= screen->w)
                x = screen->w - 1;

              left = ((255 - dist) * ((screen->w - 1) - x)) / (screen->w - 1);
            }

#ifdef DEBUG
  printf("Panning of sound #%d in channel %d, left=%d, right=%d\n", s, chan, left, (255-dist)-left);
  fflush(stdout);
#endif
          Mix_SetPanning(chan, left, (255 - dist) - left);
        }
    }
#endif
}
