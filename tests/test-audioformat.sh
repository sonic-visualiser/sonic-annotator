#!/bin/bash

mypath=`dirname $0`
r=$mypath/../sonic-annotator

inbase=$mypath/audio/3clicks
testplug=vamp:vamp-example-plugins:percussiononsets
tmpfile1=$mypath/tmp_1_$$
tmpfile2=$mypath/tmp_2_$$

trap "rm -f $tmpfile1 $tmpfile2" 0

. test-include.sh

for extension in wav ogg mp3 ; do

    transform=$mypath/transforms/transforms-audioformat-percussiononsets.n3 
    expected=$mypath/expected/transforms-audioformat-percussiononsets-$extension.csv

    test -f $transform || \
	fail "Internal error: no transforms file for suffix $suffix"

    test -f $expected || \
	fail "Internal error: no expected output file for suffix $suffix"

    infile=$inbase.$extension
    if [ "$extension" = "wav" ]; then infile=${inbase}8.$extension; fi

    test -f $infile || \
	fail "Internal error: no input audio file for extension $extension"

    $r -t $transform -w csv --csv-stdout $infile > $tmpfile2 2>/dev/null || \
	fail "Fails to run transform $transform against audio file $infile"

    if [ "$extension" = "wav" ]; then
	csvcompare $tmpfile2 $expected || \
	    fail "Output mismatch for transform $transform with audio file $infile"
    else
	csvcompare $tmpfile2 $expected || \
	    ( echo "NOTE: Output mismatch for transform $transform with audio file $infile" ; \
	      echo "This may be the result of differences in the audio file decoder, so I am not" ; \
	      echo "failing the test, but I recommend that you check the results." )
    fi
done

exit 0
