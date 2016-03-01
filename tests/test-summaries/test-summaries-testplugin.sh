#!/bin/bash

. ../include.sh

infile=$audiopath/3clicks8.wav
tmpfile=$mypath/tmp_1_$$
tmpfile2=$mypath/tmp_2_$$

trap "rm -f $tmpfile $tmpfile2" 0

for output in curve-vsr grid-fsr; do

    for summary in min max mean median mode sum variance sd count; do

	# grid-fsr is bulkier, and we're only really concerned that
	# we're getting a sane result per bin, so just do min and max
	# there
	if [ "$output" = "grid-fsr" ]; then
	    case "$summary" in
		mean|median|mode|sum|variance|sd|count) continue;;
	    esac
	fi
	
	id="$testplug:$output"
	expected="$mypath/expected/testplug-$output-$summary.csv"

	cat "$expected" | grep -v '^#' > "$tmpfile2"
	
	$r -d $id -w csv --csv-stdout -S $summary --summary-only --csv-omit-filename $infile > $tmpfile 2>/dev/null || \
	    fail "Fails to run transform id $id with summary type $summary"

	csvcompare "$tmpfile" "$tmpfile2" ||
	    faildiff "Output mismatch for output $output with summary type $summary" "$tmpfile" "$tmpfile2"

    done
done
