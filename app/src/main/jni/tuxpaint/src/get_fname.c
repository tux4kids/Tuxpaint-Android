/*
  get_fname.c

  Copyright (c) 2009 - 2020
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "get_fname.h"
#include "debug.h"
#include "compiler.h"

/*
  See tuxpaint.c for the OS-specific defaults.

  * DIR_SAVE: Where does the user's drawings get saved?

    This is where their saved files (PNG) are stored, and where the
    "current_id.txt" file is saved (so we can re-load the latest
    picture upon a subsequent launch).  Generally, end users aren't
    expected to access the files in here directly, but they can.

    The defaults may be overridden with the "--savedir" option.

  * DIR_DATA: Where is the user's data directory?

    This is where local (user-specific) fonts, brushes, stamps,
    starter images, etc., can be found.  End users only put things
    here if they wish to extend their Tux Paint experience.

    The defaults may be overridden with the "--datadir" option.

  * DIR_EXPORT: Where does Tux Paint export drawings / animations?

    This is where single images, or animated GIF slideshows,
    will be exported.  It is expected that this is an obvious,
    and easily-accessible place for end users to retrieve the exports.

    The defaults may be overridden with the "--exportdir" option.
*/


const char *savedir;
const char *datadir;
const char *exportdir;

// FIXME: We shouldn't be allocating memory all the time.
//        There should be distinct functions for each directory.
//        There should be distinct functions for each thread,
//        for caller-provided space, and maybe callee strdup.
//        That's at most 4 functions per Tux Paint thread.

/**
 * Construct a filepath, given a filename, and what kind of file
 * (data file, or saved images?)
 *
 * @param name Filaneme
 * @param kind What kind of file? (DIR_SAVE, DIR_DATA, or DIR_EXPORT?)
 * @return Full fillpath
 */
char *get_fname(const char *const name, int kind)
{
  char f[512];
  // const char *restrict const dir;
  const char * dir;

  if (kind == DIR_SAVE) {
    dir = savedir;
  } else if (kind == DIR_DATA) {
    dir = datadir;
  } else if (kind == DIR_EXPORT) {
    dir = exportdir;
  }

  snprintf(f, sizeof(f),
           "%s%c%s",
	   dir, (*name) ? '/' : '\0', /* Some mkdir()'s don't like trailing slashes */
	   name);

  return strdup(f);
}

