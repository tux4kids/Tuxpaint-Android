#!/bin/sh

# Quick script to just do it for me -bjk 2008.06.21

chmod 755 maketuxfont.py
chmod 755 tuxpaintsubset.pe
./maketuxfont.py \
	-l zh_tw \
	-p ../../../src/po/zh_tw.po \
	-p ../../../../tuxpaint-stamps/po/tuxpaint-stamps-zh_tw.po \
	../../../../tuxpaint-ttf-chinese-traditional-2004.06.05/zh_tw.ttf

ls -l zh_tw.ttf
ls -l ../zh_tw.ttf
echo
echo "If it looks ok, replace the old one and commit it back to CVS!"
