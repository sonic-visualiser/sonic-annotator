#!/bin/bash

mypath=`dirname $0`
r=$mypath/../sonic-annotator

infile=$mypath/audio/20sec-silence.wav
testplug=vamp:vamp-test-plugin:vamp-test-plugin
tmpcsv=$mypath/tmp_1_$$.csv

trap "rm -f $tmpcsv" 0

. test-include.sh

fail() {
    echo "Test failed: $1"
    exit 1
}

for output in instants curve-oss curve-fsr curve-fsr-timed curve-vsr grid-oss grid-fsr notes-regions; do
    
    $r -d "$testplug:$output" -w csv --csv-one-file "$tmpcsv" --csv-force "$infile" 2>/dev/null || \
	fail "Failed to run for plugin $testplug with output $output"

    csvcompare_ignorefirst "$tmpcsv" "$mypath/expected/vamp-test-plugin-$output.csv" || \
	fail "Output differs from expected for $output"

done

exit 0

