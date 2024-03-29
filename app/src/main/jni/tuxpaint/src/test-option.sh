#!/bin/sh
# 
# test-option.sh
# 
# Tests the GCC compiler for a particular option and returns that option,
# if successful
#
# for Tux Paint
# by TOYAMA Shin-ichi <dolphin6k@wmail.plala.or.jp>
#  
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#  (See COPYING.txt)
# 
# Last modified 2006.Feb.17

gcc $1 dummy.c > /dev/null 2>&1 && echo $1
rm -f a.out

