#!/bin/bash

. ../include.sh

infile=$audiopath/3clicks8.wav
tmpfile1=$mypath/tmp_1_$$
tmpfile2=$mypath/tmp_2_$$

trap "rm -f $tmpfile1 $tmpfile2" 0

$r --skeleton $percplug > $tmpfile1 2>/dev/null || \
    fail "Fails to run with --skeleton $percplug"

$r -t $tmpfile1 -w csv --csv-stdout $infile > $tmpfile2 2>/dev/null || \
    fail "Fails to run with -t $tmpfile1 -w csv --csv-stdout $infile"

csvcompare $tmpfile2 $mypath/expected/skeleton-1.csv || \
    faildiff "Output mismatch for skeleton-1.csv" $tmpfile2 $mypath/expected/skeleton-1.csv

for suffix in \
    -no-parameters-default-output \
    -no-parameters \
    "" \
    -start-and-duration \
    -set-parameters \
    -set-step-and-block-size \
    -set-sample-rate \
    -df-windowtype-default \
    -df-windowtype-hanning \
    -df-windowtype-hamming \
    -df-start-and-duration \
    -multiple-outputs \
    -multiple-outputs-start-and-duration \
    ; do

    for type in xml n3 ; do 

	transform=$mypath/transforms/percussiononsets$suffix.$type
	expected=$mypath/expected/percussiononsets$suffix.csv

	if [ ! -f $transform ]; then
	    if [ $type = "xml" ]; then
		continue # not everything can be expressed in the XML
			 # format, e.g. the multiple output test can't
	    fi
	fi

	test -f $transform || \
	    fail "Internal error: no transforms file for suffix $suffix (looking for $transform)"

	test -f $expected || \
	    fail "Internal error: no expected output file for suffix $suffix (looking for $expected)"

	$r -t $transform -w csv --csv-stdout $infile > $tmpfile2 2>/dev/null || \
	    fail "Fails to run transform $transform"

	csvcompare $tmpfile2 $expected || \
	    faildiff "Output mismatch for transform $transform" $tmpfile2 $expected
    done
done

exit 0

