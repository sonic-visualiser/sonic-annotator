#!/bin/bash

. ../include.sh

tmpjson=$mypath/tmp_1_$$.json

silentfile=$audiopath/20sec-silence.wav
infile=$audiopath/3clicks8.wav

trap "rm -f $tmpjson" 0

transformdir=$mypath/transforms

mandatory="-w jams"

# First check that the JSON is valid for all outputs

for output in instants curve-oss curve-fsr curve-fsr-timed curve-vsr grid-oss grid-fsr notes-regions; do

    $r -d "$testplug:$output" $mandatory --jams-one-file "$tmpjson" --jams-force "$silentfile" 2>/dev/null || \
	fail "Failed to run for plugin $testplug with output $output"

    check_json "$tmpjson" "test plugin output $output"

done

# Now check for valid results, for a subset

for output in instants curve-fsr grid-oss notes-regions; do

    $r -d "$testplug:$output" $mandatory --jams-one-file "$tmpjson" --jams-force "$silentfile" 2>/dev/null || \
	fail "Failed to run for plugin $testplug with output $output"

    expected="$mypath/expected/$output.json"
    jsoncompare "$tmpjson" "$expected" || \
	faildiff "Output differs from expected for $output" "$tmpjson" "$expected"

done

# Test digits option, with an output that has lots of digits to round

for digits in 0 6 2; do

    $r -t "$transformdir/af.n3" $mandatory --jams-digits "$digits" --jams-one-file "$tmpjson" --jams-force "$infile" 2>/dev/null || \
	fail "Failed to run for af with digits = $digits"

    expected="$mypath/expected/af-$digits.json"
    jsoncompare "$tmpjson" "$expected" || \
	faildiff "Output differs from expected for af with digits = $digits" "$tmpjson" "$expected"

done

