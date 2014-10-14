#!/bin/bash

. ../include.sh

infile=$audiopath/20sec-silence.wav
tmplab=$mypath/tmp_1_$$.lab

trap "rm -f $tmplab" 0

for output in notes-regions curve-vsr; do

    flag=""

    $r -d "$testplug:$output" -w lab --lab-stdout "$infile" 2>/dev/null > "$tmplab" || \
	fail "Failed to run for plugin $testplug with output $output and no additional flags"

    csvcompare "$tmplab" "$mypath/expected/$output-no-flags.lab" || \
	faildiff "Output differs from expected for output $output and no additional flags" "$tmplab" "$mypath/expected/$output-no-flags.lab"

    flag=fill-ends

    $r -d "$testplug:$output" -w lab --lab-$flag --lab-stdout "$infile" 2>/dev/null > "$tmplab" || \
	fail "Failed to run for plugin $testplug with output $output and $flag flag"

    csvcompare "$tmplab" "$mypath/expected/$output-$flag.lab" || \
	faildiff "Output differs from expected for output $output and $flag flag" "$tmplab" "$mypath/expected/$output-$flag.lab"
done

exit 0

