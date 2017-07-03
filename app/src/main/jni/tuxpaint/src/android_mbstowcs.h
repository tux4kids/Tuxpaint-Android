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

/*
Why we need android_wchar.h here?
Actually, Android NDK has wchar.h header file, and implements related functions.
However, it is likely that Bionic C library implements those funtions in a very simple 
(or wrong ?) way on old Android devices ( API < 21), while they come to work on the new Android devices (API >= 21).
Thus, tuxpaint im.c file depends on "mbstowcs" function which cannot work properly on old Android devices.
Anyway, using our own implementation of "mbstowcs" can fix this problem.
*/
#ifndef TUXPAINT_ANDROID_SUPPORT_MBSTOWCS_H
#define TUXPAINT_ANDROID_SUPPORT_MBSTOWCS_H

#include <wchar.h>

#undef mbsrtowcs

// redefine mbstowcs function
size_t mbstowcs(wchar_t * __restrict pwcs, const char * __restrict s, size_t n);

#endif
