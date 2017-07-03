#!/bin/sh
if [ a$1b = "a--helpb" ]
then
    echo " $1 A shell script to put things into the assets dir used in the Android port
Same license  as Tux Paint (GPL2+ at this writing)
Usage:
    $1
    $1 --force
         to overwrite the tmpzip directory"
fi


if [ ! -d tmpzip ]
then
    RUN=true
else
    if [ a$1b = "a--forceb" ]
    then
	RUN=true
    else
	RUN=false
	echo "tmpzip directory already existing, stopping.
Use the --force option if you want to overwrite it."
    fi
fi


if [ $RUN = true ]
then
    if [ -d tmpzip ]
    then
	rm -rf tmpzip
    fi
    if [ -f tuxpaint-tmp.zip ]
    then
	rm tuxpaint-tmp.zip
    fi

    mkdir -p trans

    make LOCALE_PREFIX=tmpzip/locale install-gettext && \
	cp -r data tmpzip/data && \
	cp -r fonts/locale tmpzip/data/fonts/locale && \
	cp -r im tmpzip/data/im && \
	cp -r osk tmpzip/data/osk && \
	mkdir tmpzip/data/images/magic && \
	cp magic/icons/* tmpzip/data/images/magic && \
	mkdir tmpzip/data/sounds/magic && \
	cp magic/sounds/* tmpzip/data/sounds/magic && \
	cp -r stamps tmpzip/stamps && \
	cp -r starters tmpzip/data/starters && \
	mkdir tmpzip/etc && \
	cp src/tuxpaint.cfg-android tmpzip/etc/tuxpaint.cfg && \
	mkdir -p ../../assets && \
	mv tmpzip/* ../../assets/ && \
	rmdir tmpzip
fi
