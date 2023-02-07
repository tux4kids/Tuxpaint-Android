/*
  sounds.c

  For Tux Paint
  List of sound effects.

  Copyright (c) 2002-2022 by Bill Kendrick and others
  bill@newbreedsoftware.com
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

  June 15, 2002 - December 11, 2022
  $Id$
*/

#include "sounds.h"

/* Sound file filenames: */

const char *sound_fnames[NUM_SOUNDS] = {
  DATA_PREFIX "sounds/harp.wav",
  DATA_PREFIX "sounds/click.wav",
  DATA_PREFIX "sounds/bleep.wav",
  DATA_PREFIX "sounds/bubble.wav",
  DATA_PREFIX "sounds/stamp.wav",
  DATA_PREFIX "sounds/line_start.wav",
  DATA_PREFIX "sounds/line_end.wav",
  DATA_PREFIX "sounds/scroll.wav",
  DATA_PREFIX "sounds/paint1.wav",
  DATA_PREFIX "sounds/paint2.wav",
  DATA_PREFIX "sounds/paint3.wav",
  DATA_PREFIX "sounds/paint4.wav",
  DATA_PREFIX "sounds/eraser1.wav",
  DATA_PREFIX "sounds/eraser2.wav",
  DATA_PREFIX "sounds/save.wav",
  DATA_PREFIX "sounds/prompt.wav",
  DATA_PREFIX "sounds/flip.wav",
  DATA_PREFIX "sounds/mirror.wav",
  DATA_PREFIX "sounds/keyclick.wav",
  DATA_PREFIX "sounds/typewriterbell.wav",
  DATA_PREFIX "sounds/return.wav",
  DATA_PREFIX "sounds/shrink.wav",
  DATA_PREFIX "sounds/grow.wav",
  DATA_PREFIX "sounds/italic_on.wav",
  DATA_PREFIX "sounds/italic_off.wav",
  DATA_PREFIX "sounds/areyousure.wav",
  DATA_PREFIX "sounds/youcannot.wav",
  DATA_PREFIX "sounds/tuxok.wav",
  DATA_PREFIX "sounds/thick.wav",
  DATA_PREFIX "sounds/thin.wav",
  DATA_PREFIX "sounds/fill.wav"
};
