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
   refer to https://tools.ietf.org/html/rfc3629
      Char. number range  |        UTF-8 octet sequence
      (hexadecimal)    |              (binary)
   --------------------+---------------------------------------------
   0000 0000-0000 007F | 0xxxxxxx
   0000 0080-0000 07FF | 110xxxxx 10xxxxxx
   0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
   0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
*/

#include "android_mbstowcs.h"
#include <string.h>

// This implementation may be simple, but can work fine for all of Android devices
size_t mbstowcs(wchar_t * __restrict pwcs, const char * __restrict s, size_t n){
	int length = strnlen (s, n);
	// w is the index of pwcs, s is the index of s
	int w = 0, c = 0;

	while (1) {
		pwcs[w] = '\0';
		char first = s[c];
		int len = 0;
		if ((first & 0x80) == 0) {
			pwcs[w] = (wchar_t)s[c];
			len = 1;
		}
		else if ((first & 0xe0) == 0xc0) {
			pwcs[w] |= first & 0x1f;
			pwcs[w] <<= 6;
			pwcs[w] |= s[c+1] & 0x3f;
			len = 2;
		}
		else if ((first & 0xf0) == 0xe0) {
			pwcs[w] |= first & 0x0f;
			pwcs[w] <<= 6;
			pwcs[w] |= s[c+1] & 0x3f;
			pwcs[w] <<= 6;
			pwcs[w] |= s[c+2] & 0x3f;
			len = 3;
		}
		else if ((first & 0xf8) == 0xf0) {
			pwcs[w] |= first & 0x07;
			pwcs[w] <<= 6;
			pwcs[w] |= s[c+1] & 0x3f;
			pwcs[w] <<= 6;
			pwcs[w] |= s[c+2] & 0x3f;
			pwcs[w] <<= 6;
			pwcs[w] |= s[c+3] & 0x3f;
			len = 4;
		}
		else {
			return -1;
		}

		c += len;
		w++;

		if (c > length){
			pwcs[w] = '\0';
			return -1;
		}
		if (c == length){
			pwcs[w] = '\0';
			return w;
		}
	}

}
