#!/bin/bash

# script to create a correct *.pot file
#
# the problem is that the i18n() functions can be of
# one argument: i18n("translatable string")
# or of two arguments: i18n("context", "translatable string")
# this script rewrite the source files changing that second form
# into i18n("_: context\ntranslatable string") that xgettext can grok
# and produce the same kind of *.pot as expected by KDE 

exit

rm -f POTFILES.new
(for i in `grep -v "encoding" POTFILES.in | sed 's:^:../:'`
do
 j="${i}_"
 cat ${i} | \
 sed 's|\(i18n[^(]*([^"]*"\)\([^"]*\)"[^")]*,[^")]*"|\1_: \2\\n|' > ${j}
 echo ${j} | sed 's:^...::' >> POTFILES.new
done )

intltool-update --pot && mv -f tuxpaint.pot tuxpaint_tmp.pot
/usr/bin/xgettext --from-code=UTF-8 -o tuxpaint_tmp_C.pot --directory=..  \
 --add-comments --keyword=I_ --keyword=i18n \
 --keyword=I18N_NOOP \
 --language=C \
 --files-from=./POTFILES.new
msgcat --use-first tuxpaint_tmp.pot tuxpaint_tmp_C.pot > tuxpaint.pot

( cd .. ; rm -f `cat po/POTFILES.new` )
rm -f POTFILES.new tuxpaint_tmp*.pot 

