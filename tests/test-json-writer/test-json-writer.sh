#!/bin/bash

. ../include.sh

tmpjson=$mypath/tmp_1_$$.json

silentfile=$audiopath/20sec-silence.wav

trap "rm -f $tmpjson" 0

transformdir=$mypath/transforms

mandatory="-w json --json-format jams"

# This does not yet test for correct values, only for parseable json

for output in instants curve-oss curve-fsr curve-fsr-timed curve-vsr grid-oss grid-fsr notes-regions; do

    $r -d "$testplug:$output" $mandatory --json-one-file "$tmpjson" --json-force "$silentfile" 2>/dev/null || \
	fail "Failed to run for plugin $testplug with output $output"

    check_json "$tmpjson" "test plugin output $output"
done

