#!/bin/bash


# Location of tuxpaint-stamps and tuxpaint-config diredtories
stamps_directory=../../../tuxpaint-stamps
tpconfig_directory=../../../tuxpaint-config



NUMBER_OF_LANGUAGES=0
if [ "a$1b" == "a-hb" ] || [ "a$1b" == "a--helpb" ]; then
    echo "usage: $0   or"
    echo "usage: $0 [file1.po file2.po ...] "
    exit
fi

# TODO check inside tuxpaint-config, check inside manpage.1 and docs 
# check for valid po files and valid po dir


# FIXME Currently spanish checks for both spanish and mexican-spanish


if [ "a$1b" != "ab" ] ;then
    j=$*
else
    j=`ls *.po`
fi

#echo $j $i

stamps_directory_found=0
if [ -d $stamps_directory ];
then stamps_directory_found=1
fi

tpconfig_directory_found=0
if [ -d $tpconfig_directory ];
then 
    tpconfig_directory_found=1
    NUM_LANGS=0
    echo -n Checking NUM_LANGS in tuxpaint-config2.cxx...
    echo -n NUM_LANGS:_`cat $tpconfig_directory/src/tuxpaint-config2.cxx| grep "define NUM_LANGS"|sed 's/.*ANGS //g'`_

    for item in `cat $tpconfig_directory/src/tuxpaint-config2.cxx|sed -n '/Use system/,/};/{/NUM_LANGS/d;/};/d;s/.*, "//g;s/"}.*//g;p}'`
    do
	((NUM_LANGS++))
    done
    echo Counted $NUM_LANGS
fi


echo -n "Number of files "
# Only adding the .pot file if we check for all languages
if [ "ab" == "a$1b" ]; then ((NUMBER_OF_LANGUAGES++)); fi
for i in $j; do ((NUMBER_OF_LANGUAGES++)); done
echo $NUMBER_OF_LANGUAGES
NUMBER_OF_LANGUAGES=0


for i in $j
do
  #NUMBER_OF_LANGUAGES=$((NUMBER_OF_LANGUAGES + 1))
    ((NUMBER_OF_LANGUAGES++))
    echo  $NUMBER_OF_LANGUAGES Checking $i ...

    LANG=`basename $i .po`

    if [ $stamps_directory_found -eq 1 ] ;
    then
	echo -n Checking $stamps_directory/po/tuxpaint-stamps-$i
	if [ -e $stamps_directory/po/tuxpaint-stamps-$i ]
	then echo OK  $stamps_directory/po/tuxpaint-stamps-$i
	else echo _WARNING_ No stamps translation found
	fi
    fi

    if [ $tpconfig_directory_found -eq 1 ] ;
    then
	echo -n Checking $tpconfig_directory/src/po/$i
	if [ -e $tpconfig_directory/src/po/$i ]
	then echo OK  $tpconfig_directory/src/po/$i
	else echo _WARNING_ No translation found for tuxpaint-config
	fi
    fi







    echo checking in i18n.c...

    echo -n  Checking lang_prefixes in i18n.c:
    CHECK=0
    for lang1 in `cat ../i18n.c|sed -n '/lang_prefixes\[NUM_LANGS\]/,/};/{/lang_prefixes\[NUM_LANGS\]/d;/};/d;s/\"//g;s/,//g;p}'`
    do

    #if echo $lang1|grep `basename $i .po`
	if [ $LANG == $lang1 ]

	then
	    echo OK
	    CHECK=1
	    break
	fi
    done
    if [ $CHECK -eq 0 ]
    then
	echo _WARNING_ $LANG is missing in lang_prefixes in i18n.c
    fi
  # end of lang_prefixes



    echo -n  Checking language_to_locale_array in i18n.c:
    CHECK=0
    aux=0
    cat ../i18n.c|sed -n '/language_to_locale_array\[\]/,/};/{/language_to_locale/d;/american/d;/};/d;s/\"}.*//g;s/\"//g;s/{//g;s/,//g;p}'|while read
    do
	langaux=`echo $REPLY|sed 's/.* //g'`
	lang1=`echo $langaux|sed 's/_.*//g'`
    #echo reply $REPLY
	locale=`echo $REPLY|sed 's/.* //g;s/.UTF-8//g;'`

	if [ $LANG == $lang1 ] || [ $LANG == `echo $langaux|sed 's/.UTF-8.*//g'` ]
	then
	    if [ $aux -eq 0 ]; then echo OK $lang1; aux=1; fi



	    langname=`echo $REPLY|sed 's/ .*//g'`
	    echo -n Checking $langname in show_lang_usage in i18n.c ...
	    show_lang_usage=0
	    for item in `cat ../i18n.c|sed -n '/  english      american-english/,/exit(exitcode/{/american/d;/, prg/d;/exitcode/d;/\/\*.*\*\/$/d;s/.*" //g;s/\\\n".*//g;p}'`
	    do
#echo $item
#if echo $item|grep $langname; then echo $langname $item; fi 
		if [ $item == $langname ]; then echo OK $item; show_lang_usage=1; break; fi 
	    done
	    if [ $show_lang_usage -eq 0 ]; then echo _WARNING_ $langname is missing in show_lang_usage in i18n.c; fi



	    if [ $tpconfig_directory_found ]
	    then
		echo -n Checking $langname in langs in tuxpaint-config2.cxx
		configlang=0
		for item in `cat $tpconfig_directory/src/tuxpaint-config2.cxx|sed -n '/Use system/,/};/{/american/d;/NUM_LANGS/d;/gettext/d;/};/d;s/.*, "//g;s/"}.*//g;p}'`
		do
		    if [ $item == $langname ]; then echo OK $item; configlang=1; break; fi
		done
		if [ $configlang -eq 0 ]; then echo _WARNING_ $langname is missing in lang in tuxpaint-config2.cxx; fi
	    fi

	    echo -n Checking in the manpage...
	    manlang=0
	    for item in `cat ../manpage/tuxpaint.1|sed -n '/american-english/,/.RE/{/RE/d;/TP/d;/^-/d;s/|//g;p}'`
	    do
		if [ $item == $langname ]; then echo OK $item; manlang=1; break; fi
	    done
	    if [ $manlang -eq 0 ]; then echo _WARNING_ $langname is missing in lang in ../manpage/tuxpaint.1; fi


	    echo -n Checking LANGUAGE table in OPTIONS.html...
	    OPTIONSlang=0
	    for item in `cat ../../docs/html/OPTIONS.html|sed -n '/<td><code>english/,/table>/{/tr>/d;/table>/d;/nbsp/d;s/<td><code>//g;s/<\/code><\/td>//g;p}'`
	    do
		if [ $item == $langname ]; then echo OK $item; OPTIONSlang=1; break; fi
	    done
	    if [ $OPTIONSlang -eq 0 ]; then echo _WARNING_ $langname is missing in "Available Options" in the lang=LANGUAGE table in ../../docs/html/OPTIONS.html; fi




	    echo -n Checking Locale Code in OPTIONS.html...
	    OPTIONSlocale=0
	    for item in `cat ../../docs/html/OPTIONS.html|sed -n '/<td>English/,/table>/{/tr>/d;/table>/d;/nbsp/d;s/<td><code>//g;s/<\/code>.*//g;/<td>/d;p}'`
	    do
		if [ $item == $locale ]; then echo OK $item; OPTIONSlocale=1; break; fi
	    done
	    if [ $OPTIONSlocale -eq 0 ]; then echo _WARNING_ $locale is missing in the "Available Languages" table in the "Locale Code" field in ../../docs/html/OPTIONS.html; fi





	    echo -n Checking $locale in show_locale_usage ... 
	    show_locale_usage=0
	    for item in `cat ../i18n.c|sed -n '/English      American English/,/, prg);/{/, prog/d;/English      American English/d;s/(.*//g;s/"//g;p}'`

	    do
#echo $item $locale
		if [ $item == $locale ] ; then echo OK $locale; show_locale_usage=1; fi
	    done
	    if [ $show_locale_usage -eq 0 ]; then echo _WARNING_ $locale is missing in show_locale_usage in i18n.c; fi




	fi
    done

    echo -n Checking $i in i18n.h ...
    LANG_in_i18ndoth=0
    for item in `cat ../i18n.h|sed -n '/enum/,/NUM_LANGS/{/enum/d;/{/d;/NUM_LANGS/d;s/.*LANG/LANG/g;s/,.*//g;p}'`
    do
	if [ "LANG_${LANG^^}" == $item ]; then echo OK $item; LANG_in_i18ndoth=1; break; fi
    done

    if [ $LANG_in_i18ndoth == 0 ]; then echo _WARNING_ could not find "\"LANG_${LANG^^}\"" in i18n.h, please manually review it ; fi







#done







    echo
done