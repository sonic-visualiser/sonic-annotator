#!/bin/bash

. ../include.sh

inbase=$audiopath/3clicks
tmpfile1=$mypath/tmp_1_$$
tmpfile2=$mypath/tmp_2_$$

trap "rm -f $tmpfile1 $tmpfile2" 0

for extension in wav ogg mp3 ; do

    transform=$mypath/transforms/percussiononsets.n3 
    expected=$mypath/expected/percussiononsets-$extension.csv

    test -f $transform || \
	fail "Internal error: no transforms file"

    test -f $expected || \
	fail "Internal error: no expected output file for extension $extension"

    infile=$inbase.$extension
    if [ "$extension" = "wav" ]; then infile=${inbase}8.$extension; fi

    test -f $infile || \
	fail "Internal error: no input audio file for extension $extension"

    $r -t $transform -w csv --csv-stdout $infile > $tmpfile2 2>/dev/null || \
	fail "Fails to run transform $transform against audio file $infile"

    if [ "$extension" = "wav" ]; then
	csvcompare $tmpfile2 $expected || \
	    faildiff "Output mismatch for transform $transform with audio file $infile" $tmpfile2 $expected
    else
	csvcompare $tmpfile2 $expected || \
	    ( echo "NOTE: Output mismatch for transform $transform with audio file $infile" ; \
	      echo "This may be the result of differences in the audio file decoder, so I am not" ; \
	      echo "failing the test, but I recommend that you check the results." )
    fi
done

# Check the normalise flag

$r -d $amplplug -w csv --csv-stdout ${inbase}8quiet.wav 2>/dev/null | head > $tmpfile1 || \
    fail "Fails to run default transform for plugin $amplplug against audio file ${inbase}8quiet.wav"
csvcompare $tmpfile1 $mypath/expected/norm-off.csv || \
    faildiff "Output mismatch for default transform for plugin $amplplug against audio file ${inbase}8quiet.wav without normalisation" $tmpfile1 $mypath/expected/norm-off.csv

$r -d $amplplug -n -w csv --csv-stdout ${inbase}8quiet.wav 2>/dev/null | head > $tmpfile1 || \
    fail "Fails to run default transform for plugin $amplplug against audio file ${inbase}8quiet.wav with normalisation"
csvcompare $tmpfile1 $mypath/expected/norm-on.csv || \
    faildiff "Output mismatch for default transform for plugin $amplplug against audio file ${inbase}8quiet.wav with normalisation" $tmpfile1 $mypath/expected/norm-on.csv

$r -d $amplplug --normalise -w csv --csv-stdout ${inbase}8quiet.wav 2>/dev/null | head > $tmpfile1 || \
    fail "Fails to run default transform for plugin $amplplug against audio file ${inbase}8quiet.wav with normalisation"
csvcompare $tmpfile1 $mypath/expected/norm-on.csv || \
    faildiff "Output mismatch for default transform for plugin $amplplug against audio file ${inbase}8quiet.wav with normalisation" $tmpfile1 $mypath/expected/norm-on.csv

exit 0
