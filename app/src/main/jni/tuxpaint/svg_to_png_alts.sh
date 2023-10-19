#!/bin/sh

# Script copied from tuxpaint-stamps/tools
# Changed pngout to optipng to be able to run in the F-droid's box buildserver.

# Given an SVG, generate a reasonably-sized PNG
# alternative that can be used on platforms where
# SVG-based Stamps are not supported.
#
# Uses `rsvg-convert` (from `librsvg2-bin` package),
# `convert` (from ImageMagick), and `pngout`
# (http://www.jonof.id.au/kenutils.html)
# to first convert from SVG to PNG, then trim any whitespace,
# and finally compress/optimize the size of the final PNG.
#
# Invoke this tool from the top level of `tuxpaint-stamps`.
#
# 2023-10-05 - 2023-10-05

for i in $*; do
  dn=`dirname $i`
  bn=`basename $i .svg`
  png="$dn/$bn.png"
  echo "$i => $png"
  if test -f "$png"; then
    echo "ALREADY EXISTS"
  else
    rsvg-convert "$i" > "$png-tmp"
    convert "$png-tmp" -trim "$png"
    rm "$png-tmp"
    optipng "$png"
  fi
done

