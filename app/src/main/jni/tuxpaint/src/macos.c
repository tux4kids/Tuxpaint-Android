/*
  macos.c

  Copyright (c) 2021
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
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "macos.h"

#define MACOS_FONTS_PATH              "%s/Library/Fonts"
#define MACOS_PREFERENCES_PATH        "%s/Library/Application Support/TuxPaint"
#define MACOS_GLOBAL_PREFERENCES_PATH "/Library/Application Support/TuxPaint"
#define MACOS_PICTURES_PATH           "%s/Pictures"


const char *apple_fontsPath(void)
{
    static char *p = NULL;

    if(!p) {
        const char *home = getenv("HOME");

        p = malloc(strlen(home) + strlen(MACOS_FONTS_PATH) + 1);

        if(p) sprintf(p, MACOS_FONTS_PATH, getenv("HOME"));
        else perror("apple_fontsPath");
    }

    return p;
}


const char *apple_preferencesPath(void)
{
    static char *p = NULL;

    if(!p) {
        const char *home = getenv("HOME");

        p = malloc(strlen(home) + strlen(MACOS_PREFERENCES_PATH) + 1);

        if(p) sprintf(p, MACOS_PREFERENCES_PATH, getenv("HOME"));
        else perror("apple_preferencesPath");
    }

    return p;
}


const char *apple_globalPreferencesPath(void)
{
    return MACOS_GLOBAL_PREFERENCES_PATH;
}


const char *apple_picturesPath(void)
{
    static char *p = NULL;

    if(!p) {
        const char *home = getenv("HOME");

        p = malloc(strlen(home) + strlen(MACOS_PICTURES_PATH) + 1);

        if(p) sprintf(p, MACOS_PICTURES_PATH, getenv("HOME"));
        else perror("apple_picturesPath");
    }

    return p;
}
