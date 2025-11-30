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
    stamps_version="2025.05.26"
    stampsversion=`echo $stamps_version|sed "s/\./-/g"`

    # The above sed line doesn't play fine with betas
    # https://sourceforge.net/projects/tuxpaint/files/tuxpaint-stamps/2024-10-XX-beta/tuxpaint-stamps-2024.10.16.tar.gz/download
    #https://sourceforge.net/projects/tuxpaint/files/tuxpaint-stamps/2025-04-XX-beta/tuxpaint-stamps-2025.03.31.tar.gz/download
    #stampsversion="2025-04-XX-beta"

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
	cp -r docs tmpzip/docs && \
	cd magic/magic-docs && \
	for langdir in *; do if [ -d $langdir ]; then mkdir ../../tmpzip/docs/$langdir/magic-docs && cp -r $langdir/html ../../tmpzip/docs/$langdir/magic-docs/; fi ; done && \
	cd ../.. && \
	cp -r fonts/locale tmpzip/data/fonts/locale && \
	cp -r im tmpzip/data/im && \
	cp -r osk tmpzip/data/osk && \
	rm tmpzip/data/osk/.indent.pro && \
	mkdir tmpzip/data/images/magic && \
	cd magic/icons && \
	for icon in *png; do convert $icon PNG32:../../tmpzip/data/images/magic/$icon; done && \
	cd ../.. && \
	mkdir tmpzip/data/sounds/magic && \
	cp magic/sounds/* tmpzip/data/sounds/magic && \
	cp -r stamps tmpzip/stamps && \
	cd starters && \
	for i in *svg; do if [ ! -f  `basename $i .svg`.png ]; then ../svg_to_png_alts.sh $i && rm $i && convert -bordercolor transparent -border 3 `basename $i .svg`.png tmp.png && mv tmp.png `basename $i .svg`.png; fi; done && \
	cd .. && \
	mv starters tmpzip/data/starters && git restore starters && \
	cp -r templates tmpzip/data/templates &&\
	mkdir tmpzip/etc && \
	cp src/tuxpaint.cfg-android tmpzip/etc/tuxpaint.cfg && \
	wget -O tuxpaint-stamps-$stamps_version.tar.gz https://sourceforge.net/projects/tuxpaint/files/tuxpaint-stamps/$stampsversion/tuxpaint-stamps-$stamps_version.tar.gz/download && \
	tar xfz tuxpaint-stamps-$stamps_version.tar.gz && \
	cd tuxpaint-stamps-$stamps_version && \
	../join_subdirs.sh && \
	rm -rf tmpzip/stamps/*/*svg && \
	cd .. && \
	mkdir -p ../../assets && \
	mv tmpzip/* ../../assets/ && \
	cp -r tuxpaint-stamps-$stamps_version/tmpzip/stamps/* ../../assets/stamps/ && \
	rmdir tmpzip && \
	rm -rf tuxpaint-stamps-$stamps_version && \
	rm tuxpaint-stamps-$stamps_version.tar.gz
fi
