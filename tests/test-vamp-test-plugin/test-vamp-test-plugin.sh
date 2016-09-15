#!/bin/bash

. ../include.sh

infile=$audiopath/20sec-silence.wav
tmpcsv=$mypath/tmp_1_$$.csv

trap "rm -f $tmpcsv" 0

$r --transform-minversion $testplug 4 || \
    fail "Vamp Test Plugin version is too old (at least v4 required)"

for output in instants curve-oss curve-fsr curve-fsr-timed curve-fsr-mixed curve-vsr grid-oss grid-fsr notes-regions; do
    
    $r -d "$testplug:$output" -w csv --csv-one-file "$tmpcsv" --csv-force "$infile" 2>/dev/null || \
	fail "Failed to run for plugin $testplug with output $output"

    csvcompare_ignorefirst "$tmpcsv" "$mypath/expected/vamp-test-plugin-$output.csv" || \
	faildiff "Output differs from expected for $output" "$tmpcsv" "$mypath/expected/vamp-test-plugin-$output.csv"

done

exit 0

