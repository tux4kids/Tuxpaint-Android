#!/bin/sh
cd $(dirname "$0")

sed -e 's/^/..\//' POTFILES.in.in > POTFILES.in
ls ../../magic/src/*.c >> POTFILES.in

xgettext \
  --package-name=tuxpaint \
  --files-from=POTFILES.in \
  --from-code=UTF-8 \
  --keyword=gettext_noop \
  --add-comments \
  --output=tuxpaint.pot

for i in *.po ; do
  echo $i
  msgmerge --update --previous --backup=none $i tuxpaint.pot
done
