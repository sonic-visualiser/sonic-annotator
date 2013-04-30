#!/bin/bash

mypath=`dirname $0`



for x in \
    supportprogs \
    helpfulflags \
    transforms-basic \
    audioformat \
    vamp-test-plugin \
    as-advertised \
    rdf-writer \
    rdf-destinations \
    csv-destinations \
    summaries \
    ; do

    echo -n "$x: "
    if ( cd $mypath ; bash ./test-$x.sh ); then
	echo test succeeded
    else
	echo "*** Test FAILED"
	exit 1
    fi

done

