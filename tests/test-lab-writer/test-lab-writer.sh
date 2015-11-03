#!/bin/bash

. ../include.sh

infile=$audiopath/20sec-silence.wav
tmplab=$mypath/tmp_1_$$.lab

trap "rm -f $tmplab" 0

for output in notes-regions curve-vsr grid-oss; do

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

# Do it all over again, but using the CSV writer. The Lab writer is
# actually redundant, it's equivalent to -w csv --csv-separator '\t'
# --csv-end-times --csv-omit-filename

for output in notes-regions curve-vsr grid-oss; do

    flag=""

    $r -d "$testplug:$output" -w csv --csv-separator '\t' --csv-end-times --csv-omit-filename --csv-stdout "$infile" 2>/dev/null > "$tmplab" || \
	fail "Failed to run for plugin $testplug and CSV writer with output $output and no additional flags"

    csvcompare "$tmplab" "$mypath/expected/$output-no-flags.lab" || \
	faildiff "Output differs from expected for CSV writer with output $output and no additional flags" "$tmplab" "$mypath/expected/$output-no-flags.lab"

    flag=fill-ends

    $r -d "$testplug:$output" -w csv --csv-separator '\t' --csv-end-times --csv-omit-filename --csv-$flag --csv-stdout "$infile" 2>/dev/null > "$tmplab" || \
	fail "Failed to run for plugin $testplug and CSV writer with output $output and $flag flag"

    csvcompare "$tmplab" "$mypath/expected/$output-$flag.lab" || \
	faildiff "Output differs from expected for CSV writer with output $output and $flag flag" "$tmplab" "$mypath/expected/$output-$flag.lab"
done

for output in grid-oss; do
    for digits in 0 6 2; do

	$r -d "$testplug:$output" -w lab --lab-stdout --lab-digits "$digits" "$infile" 2>/dev/null > "$tmplab" || \
	    fail "Failed to run for plugin $testplug with output $output and digits $digits"

	# no fuzz here
	cmp -s "$tmplab" "$mypath/expected/$output-$digits.lab" || \
	    faildiff "Output differs from expected for CSV writer with output $output and digits $digits" "$tmplab" "$mypath/expected/$output-$digits.lab"

    done
done

exit 0

