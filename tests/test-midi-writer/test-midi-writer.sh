#!/bin/bash

. ../include.sh

infile=$audiopath/20sec-silence.wav
tmpmidi=$mypath/tmp_1_$$.mid

trap "rm -f $tmpmidi" 0

for output in notes-regions curve-vsr; do

    flag=""

    $r -d "$testplug:$output" -w midi --midi-one-file "$tmpmidi" --midi-force "$infile" 2>/dev/null || \
	fail "Failed to run for plugin $testplug with output $output and no additional flags"

    midicompare "$tmpmidi" "$mypath/expected/$output.mid" || \
	faildiff_od "Output differs from expected for output $output" "$tmpmidi" "$mypath/expected/$output.mid"

done

exit 0

