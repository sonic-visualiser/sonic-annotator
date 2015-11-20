#!/bin/bash

mypath=`dirname $0`

for x in \
    supportprogs \
    helpfulflags \
    transforms-basic \
    audioformat \
    vamp-test-plugin \
    as-advertised \
    summaries \
    multiple-audio \
    csv-writer \
    csv-destinations \
    lab-writer \
    lab-destinations \
    rdf-writer \
    rdf-destinations \
    midi-writer \
    midi-destinations \
    jams-writer \
    jams-destinations \
    ; do

    echo -n "$x: "
    if ( cd $mypath/test-$x ; bash ./test-$x.sh ); then
	echo test succeeded
    else
	echo "*** Test FAILED"
	exit 1
    fi

done

