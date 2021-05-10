/*
  platform.h

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
#ifndef __PLATFORM_H__
#define __PLATFORM_H__


#if defined(__APPLE__)
    #include <TargetConditionals.h>

    /*
    * MAC test must be last because it tests true even on iOS / tvOS / watchOS.
    */

    #if TARGET_OS_IOS || TARGET_OS_IPHONE || TARGET_OS_SIMULATOR || TARGET_IPHONE_SIMULATOR || TARGET_OS_EMBEDDED
        #define __IOS__         1
    #elif TARGET_OS_OSX || TARGET_OS_MAC
        #define __MACOS__       1
    #else
        #define __OTHER_APPLE__ 1

        #warning "Unsupported Apple platform, will build on a best-effort basis"
    #endif
#endif /* __APPLE__ */


#endif /* __PLATFORM_H__ */
