#!/bin/sh

cp POTFILES.in.in POTFILES.in
ls ../../magic/src/*.c | cut -b 4- >> POTFILES.in

intltool-update --pot
msguniq tuxpaint.pot > temp.tmp && mv -f temp.tmp tuxpaint.pot
for i in *.po ; do
  echo $i
  msgmerge --update --previous --backup=none $i tuxpaint.pot
done
cd ..
intltool-merge -d -u po tuxpaint.desktop.in tuxpaint.desktop
cd po
