#!/bin/bash

# tuxpaint-import

# "Tux Paint Import"
# Import an arbitrary GIF, JPEG or PNG into Tux Paint

# (c) Copyright 2002-2009, by Bill Kendrick and others
# bill@newbreedsoftware.com
# http://www.newbreedsoftware.com/tuxpaint/

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# (See COPYING.txt)


# September 21, 2002 - November 4, 2009


SAVEDIR=$HOME/.tuxpaint
TMPDIR=$SAVEDIR


if [ $# -ne 0 ]; then
  if [ $1 = "--help" ]; then
    # --help, show usage:
    echo
    echo "tuxpaint-import"
    echo
    echo "Imports an arbitrary image (GIF, JPEG, PNG, etc. format)"
    echo "into Tux Paint (see: tuxpaint(1)) so that it appears in the"
    echo "'Open' dialog."
    echo 
    echo "Usage: tuxpaint-import filename(s)"
    echo "       tuxpaint-import --help"
    echo
    exit
  fi
fi


# Determine preferred savedir

# First, check /usr/local/etc/
x=`grep -m 1 "^savedir=" /usr/local/etc/tuxpaint/tuxpaint.conf`
if test $? = 0 ; then
  SAVEDIR=`echo $x | cut -d = -f 2-99`
fi

# First, check /etc/
x=`grep -m 1 "^savedir=" /etc/tuxpaint/tuxpaint.conf`
if test $? = 0 ; then
  SAVEDIR=`echo $x | cut -d = -f 2-99`
fi

# First, check $HOME
x=`grep -m 1 "^savedir=" $HOME/.tuxpaintrc`
if test $? = 0 ; then
  SAVEDIR=`echo $x | cut -d = -f 2-99`
fi


echo "Using save directory: $SAVEDIR/saved"


# Make sure savedir exists!
if [ ! -d $SAVEDIR ]; then
  echo "Creating $SAVEDIR"
  mkdir -p $SAVEDIR
fi

# Make sure savedir/saved exists!
if [ ! -d $SAVEDIR/saved ]; then
  echo "Creating $SAVEDIR/saved"
  mkdir -p $SAVEDIR/saved
fi

# Make sure savedir thumbs directory exists!
if [ ! -d $SAVEDIR/saved/.thumbs ]; then
  echo "Creating $SAVEDIR/saved/.thumbs"
  mkdir -p $SAVEDIR/saved/.thumbs
fi


# Determine appropriate size for images, based on Tux Paint's current
# configuration

# First, assume 800x600 Tux Paint
window_width=800
window_height=600

# First, check /usr/local/etc/
x=`grep -m 1 "^windowsize=" /usr/local/etc/tuxpaint/tuxpaint.conf`
if test $? = 0 ; then
  window_width=`echo $x | cut -d = -f 2 | cut -d x -f 1`
  window_height=`echo $x | cut -d = -f 2 | cut -d x -f 2`
fi

# First, check /etc/
x=`grep -m 1 "^windowsize=" /etc/tuxpaint/tuxpaint.conf`
if test $? = 0 ; then
  window_width=`echo $x | cut -d = -f 2 | cut -d x -f 1`
  window_height=`echo $x | cut -d = -f 2 | cut -d x -f 2`
fi

# First, check $HOME
x=`grep -m 1 "^windowsize=" $HOME/.tuxpaintrc`
if test $? = 0 ; then
  window_width=`echo $x | cut -d = -f 2 | cut -d x -f 1`
  window_height=`echo $x | cut -d = -f 2 | cut -d x -f 2`
fi


# (Image width is window width minus 192,
# image height is window height minus 104)

width=`expr $window_width - 192`
height=`expr $window_height - 104`


echo "Using $width x $height images (for $window_width x $window_height Tux Paint"

if [ $# -eq 0 ]; then
  # No arguments provided (sorry, you can't pipe into this script's stdin!)
  echo
  echo "Usage: tuxpaint-import filename(s)"
  echo "       tuxpaint-import --help"
  exit
fi


# For each picture list...
for x in $(seq 1 $#)
do
  i="${!x}"

  if [ -e "$i" ]; then
    # Determine a filename for it:
    NEWFILENAME=`date "+%Y%m%d%H%M%S"`
    echo "$i -> $SAVEDIR/saved/$NEWFILENAME.png"

    # Convert and scale down, save as a temp file:
    anytopnm "$i" | pnmscale -xysize $width $height > $TMPDIR/saved/$NEWFILENAME.ppm
    
    # Place inside the correctly-sized canvas:
    # FIXME: Center, instead of placing at upper right
    ppmmake "#FFFFFF" $width $height \
    | pnmpaste -replace $TMPDIR/saved/$NEWFILENAME.ppm 0 0 \
    | pnmtopng > $SAVEDIR/saved/$NEWFILENAME.png
    
    # Remove temp file:
    rm $TMPDIR/saved/$NEWFILENAME.ppm

    # Create thumbnail for 'Open' dialog:
    pngtopnm $SAVEDIR/saved/$NEWFILENAME.png | pnmscale -xysize 92 56 \
    | pnmtopng > $SAVEDIR/saved/.thumbs/$NEWFILENAME-t.png
    
  else
    # File wasn't there!
    echo "$i - File not found"
  fi
done

