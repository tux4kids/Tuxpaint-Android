#!/bin/bash

# Script to join all the stamps in a subdir (animals, sport, etc.)
# in the root of that subdir, keeping the stamps ordered as close as possible
# to how Tux Paint orders them.

WORKDIR=$PWD
mkdir -p tmpzip/stamps
index=({000..999})

scandir () {
    if [ -d $1 ]
    then
	for item in $1/*
	do
	    # first process the files inside the dir
	    if [ ! -d $item ]
	    then
		# echo file $item $dircount
		if (echo $item | grep -iq "svg" - )
		then
		    $WORKDIR/../svg_to_png_alts.sh $item
		    cp $item "tmpzip"/$subdir/${index[$dircount]}_`basename $item .svg`.png
		fi
		cp $item "tmpzip"/$subdir/${index[$dircount]}_`basename $item`
	    fi
	done

	for dir in $1/*
	do
	    # later scan the dirs
	    if [ -d $dir ]
	    then
		echo $dir ${index[$dircount]}
		echo -n "."
		dircount=$(($dircount+1))
		scandir $dir
	    fi

	done
    fi
}

for subdir in stamps/*
do
    echo $subdir
    mkdir -p tmpzip/$subdir
    dircount="1"
    scandir $subdir
    echo
done

    
	   
