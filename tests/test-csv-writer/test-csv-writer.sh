#!/bin/bash

. ../include.sh

infile=$audiopath/20sec-silence.wav
tmpcsv=$mypath/tmp_1_$$.csv

trap "rm -f $tmpcsv" 0

for output in notes-regions curve-vsr; do

    flag=""

    $r -d "$testplug:$output" -w csv --csv-stdout "$infile" 2>/dev/null > "$tmpcsv" || \
	fail "Failed to run for plugin $testplug with output $output and no additional flags"

    csvcompare "$tmpcsv" "$mypath/expected/$output-no-flags.csv" || \
	faildiff "Output differs from expected for output $output and no additional flags" "$tmpcsv" "$mypath/expected/$output-no-flags.csv"

    flag=sample-timing

    $r -d "$testplug:$output" -w csv --csv-$flag --csv-stdout "$infile" 2>/dev/null > "$tmpcsv" || \
	fail "Failed to run for plugin $testplug with output $output and $flag flag"

    csvcompare "$tmpcsv" "$mypath/expected/$output-$flag.csv" || \
	faildiff "Output differs from expected for output $output and $flag flag" "$tmpcsv" "$mypath/expected/$output-$flag.csv"

    flag=fill-ends

    $r -d "$testplug:$output" -w csv --csv-$flag --csv-stdout "$infile" 2>/dev/null > "$tmpcsv" || \
	fail "Failed to run for plugin $testplug with output $output and $flag flag"

    csvcompare "$tmpcsv" "$mypath/expected/$output-$flag.csv" || \
	faildiff "Output differs from expected for output $output and $flag flag" "$tmpcsv" "$mypath/expected/$output-$flag.csv"

    flag=end-times

    $r -d "$testplug:$output" -w csv --csv-$flag --csv-stdout "$infile" 2>/dev/null > "$tmpcsv" || \
	fail "Failed to run for plugin $testplug with output $output and $flag flag"

    csvcompare "$tmpcsv" "$mypath/expected/$output-$flag.csv" || \
	faildiff "Output differs from expected for output $output and $flag flag" "$tmpcsv" "$mypath/expected/$output-$flag.csv"

    flag=separator

    $r -d "$testplug:$output" -w csv --csv-$flag '@' --csv-stdout "$infile" 2>/dev/null > "$tmpcsv" || \
	fail "Failed to run for plugin $testplug with output $output and $flag flag"

    csvcompare "$tmpcsv" "$mypath/expected/$output-$flag.csv" || \
	faildiff "Output differs from expected for output $output and $flag flag" "$tmpcsv" "$mypath/expected/$output-$flag.csv"

    flag=all

    $r -d "$testplug:$output" -w csv --csv-sample-timing --csv-fill-ends --csv-end-times --csv-separator '@' --csv-stdout "$infile" 2>/dev/null > "$tmpcsv" || \
	fail "Failed to run for plugin $testplug with output $output and all flags"

    csvcompare "$tmpcsv" "$mypath/expected/$output-$flag.csv" || \
	faildiff "Output differs from expected for output $output and all flags" "$tmpcsv" "$mypath/expected/$output-$flag.csv"

done

exit 0

