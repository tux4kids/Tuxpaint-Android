# tuxpaint(1) completion 
# put this file in /etc/bash_completion.d/ 
# Bill Kendrick <bill@newbreedsoftware.com>; http://www.tuxpaint.org/
# Based on inkscape's completion file, by allali@univ-mlv.fr
#
# $Id: tuxpaint-completion.bash,v 1.6 2014/03/31 05:54:33 wkendrick Exp $

# FIXME: See http://www.debian-administration.org/articles/316 for an intro
# to how we should be doing this... -bjk 2009.09.09

have tuxpaint &&
_tuxpaint()
{
        local cur

        COMPREPLY=()
        cur=${COMP_WORDS[COMP_CWORD]}

        if [[ "$cur" == -* ]]; then
                COMPREPLY=( $( compgen -W '\
			-h --help \
			-v --version -vv --version-verbose \
			-c --copying \
			-u --usage \
			-w --windowed -f --fullscreen \
			--native \
			--disablescreensaver --allowscreensaver \
			--orient=landscape --orient=portrait \
			-b --startblank --startlast \
			--sound -q --nosound \
			-x --noquit --quit
			-p --print --noprint \
			--complexshapes -s --simpleshapes \
			-m --mixedcase -u --uppercase \
			--fancycursors --nofancycursors \
			--hidecursor --showcursor \
			--mouse --keyboard \
			--dontgrab --grab \
			--noshortcuts --shortcuts \
			--wheelmouse --nowheelmouse \
			--nobuttondistinction --buttondistinction \
			--outlines --nooutlines \
			--stamps --nostamps \
			--sysfonts --nosysfonts \
			--nostampcontrols --stampcontrols \
			--nomagiccontrols --magiccontrols \
			--mirrorstamps --dontmirrorstamps \
			--stampsize=0 --stampsize=1 \
			--stampsize=2 --stampsize=3 \
			--stampsize=4 --stampsize=5 \
			--stampsize=6 --stampsize=7 \
			--stampsize=8 --stampsize=9 --stampsize=default \
			--saveoverask --saveover --saveovernew \
			--nosave --save \
			--autosave --noautosave \
			--savedir --datadir \
			--printdelay= \
			--altprintmod --altprintalways --altprintnever \
			--papersize \
			-l --lang -L --locale \
			--nosysconfig \
			--nolockfile \
			--mouse-accessibility \
			--onscreen-keyboard \
			--onscreen-keyboard-layout \
			--onscreen-keyboard-disable-change \
			--joystick-dev \
			--joystick-slowness \
			--joystick-threshold \
			--joystick-maxsteps \
			--joystick-hat-slowness \
			--joystick-hat-timeout \
			--joystick-btn-escape \
			--joystick-btn-brush \
			--joystick-btn-stamp \
			--joystick-btn-lines \
			--joystick-btn-shapes\
			--joystick-btn-text \
			--joystick-btn-label \
			--joystick-btn-magic \
			--joystick-btn-undo \
			--joystick-btn-redo \
			--joystick-btn-eraser \
			--joystick-btn-new \
			--joystick-btn-open \
			--joystick-btn-save \
			--joystick-btn-pgsetup \
			--joystick-btn-print \
			--joystick-buttons-ignore \
			--colorfile' -- $cur ) ) 
# We don't accept filenames on the command-line yet -bjk 2009.09.09
#        else
#                _filedir '@(ai|ani|bmp|cur|dia|eps|gif|ggr|ico|jpe|jpeg|jpg|pbm|pcx|pdf|pgm|png|ppm|pnm|ps|ras|sk|svg|svgz|targa|tga|tif|tiff|txt|wbmp|wmf|xbm|xpm)'
#
# FIXME: Allwo tab completion to show options after --lang (though may need to add support for --lang= for that to work?) -bjk 2009.09.09
        fi

}
[ "${have:-}" ] && complete -F _tuxpaint $filenames tuxpaint
