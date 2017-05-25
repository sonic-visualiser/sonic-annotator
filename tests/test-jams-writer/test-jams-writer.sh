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

# If JAMS is installed, we can report on whether the outputs are valid
# JAMS schema files or not -- unfortunately we can't comply with the
# schema for most real plugins, so we can only make indicative reports
# for most. This is such a limited test that we make it optional; it's
# a bit much to expect everyone to have JAMS installed just for
# this. Also the JAMS verifier doesn't currently always work for me
# (e.g. it doesn't seem to work correctly with Python 3 at the moment)
# so let's not make this fatal either.

if jams_to_lab.py --help >/dev/null 2>&1; then

    $r -t "$transformdir/onsets.n3" $mandatory --jams-one-file "$tmpjson" --jams-force "$infile" 2>/dev/null || \
	fail "Failed to run for onsets"

    if ! jams_to_lab.py "$tmpjson" test; then
	echo "WARNING: JAMS schema verification failed for onsets"
    fi
fi

# Now check against expected output, for a subset

for output in instants curve-fsr grid-oss notes-regions; do

    $r -d "$testplug:$output" $mandatory --jams-one-file "$tmpjson" --jams-force "$silentfile" 2>/dev/null || \
	fail "Failed to run for plugin $testplug with output $output"

    expected="$mypath/expected/$output.json"
    jsoncompare "$tmpjson" "$expected" || \
	faildiff "Output differs from expected for $output" "$tmpjson" "$expected"

done

# Test digits option, with an output that has lots of digits to round

for digits in 0 6 2; do

    $r -d "$testplug:grid-oss" $mandatory --jams-digits "$digits" --jams-one-file "$tmpjson" --jams-force "$silentfile" 2>/dev/null || \
	fail "Failed to run for af with digits = $digits"

    expected="$mypath/expected/grid-oss-$digits.json"
    jsoncompare "$tmpjson" "$expected" || \
	faildiff "Output differs from expected for af with digits = $digits" "$tmpjson" "$expected"

done

