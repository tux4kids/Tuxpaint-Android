/*
  get_fname.h

  Copyright (c) 2009 - July 25, 2020
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

#ifndef GET_FNAME_H
#define GET_FNAME_H

extern const char *savedir;
extern const char *datadir;
extern const char *exportdir;

enum
{
  /* (See get_fname.c for details) */
  DIR_SAVE,
  DIR_DATA,
  DIR_EXPORT
};

char *get_fname(const char *const name, int kind);


#endif
