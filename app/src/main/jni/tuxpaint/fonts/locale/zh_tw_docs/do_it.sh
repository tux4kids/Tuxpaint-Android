#!/bin/sh

# Quick script to generate a fresh 'subset font'
# 2008.06.21 / 2022.02.13

chmod 755 maketuxfont.py
chmod 755 tuxpaintsubset.pe
./maketuxfont.py \
	-l zh_TW \
	-p ../../../src/po/zh_TW.po \
	-p ../../../../tuxpaint-stamps/po/tuxpaint-stamps-zh_TW.po \
	../../../../tuxpaint-ttf-chinese-traditional-2004.06.05/zh_tw.ttf

ls -l zh_TW.ttf
ls -l ../zh_TW.ttf
echo
echo "If it looks ok, replace the old one and commit it back to the code repository!"
