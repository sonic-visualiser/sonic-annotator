#!/bin/bash

mypath=`dirname $0`
r=$mypath/../sonic-annotator

infile1=$mypath/audio/3clicks8.wav
infile2=$mypath/audio/6clicks8.wav

outfile1=$mypath/audio/3clicks8_vamp_vamp-example-plugins_percussiononsets_onsets.csv
outfile2=$mypath/audio/6clicks8_vamp_vamp-example-plugins_percussiononsets_onsets.csv

infile1dot=$mypath/audio/3.clicks.8.wav
outfile1dot=$mypath/audio/3.clicks.8_vamp_vamp-example-plugins_percussiononsets_onsets.csv

outfile3=$mypath/audio/3clicks8_vamp_vamp-example-plugins_percussiononsets_onsets.csv
outfile4=$mypath/audio/3clicks8_vamp_vamp-example-plugins_percussiononsets_detectionfunction.csv

testplug=vamp:vamp-example-plugins:percussiononsets
tmpcsv=$mypath/tmp_1_$$.csv

trap "rm -f $tmpcsv $outfile1 $outfile2 $outfile3 $outfile4 $infile1dot $outfile1dot" 0

fail() {
    echo "Test failed: $1"
    exit 1
}

transformpfx=$mypath/transforms/transforms-rdf-writer-percussiononsets

check_csv() {
    test -f $1 || \
	fail "Fails to write output to expected location $1 for $2"
    # every line must contain the same number of commas
    formats=`awk -F, '{ print NF; }' $1 | sort | uniq | wc | awk '{ print $1 }'`
    if [ "$formats" != "1" ]; then
	fail "Output is not consistently formatted comma-separated file for $2"
    fi
    rm -f $1
}    


ctx="onsets transform, one audio file, default CSV writer destination"

rm -f $outfile1

$r -t $transformpfx-onsets.n3 -w csv $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_csv $outfile1 "$ctx"


ctx="onsets transform, one audio file with dots in filename, default CSV writer destination"

rm -f $outfile1

cp $infile1 $infile1dot

$r -t $transformpfx-onsets.n3 -w csv $infile1dot 2>/dev/null || \
    fail "Fails to run with $ctx"

check_csv $outfile1dot "$ctx"

rm -f $infile1dot $outfile1dot


ctx="onsets and df transforms, one audio file, default CSV writer destination"

rm -f $outfile1

$r -t $transformpfx-onsets.n3 -t $transformpfx-detectionfunction.n3 -w csv $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_csv $outfile1 "$ctx"


ctx="onsets transform, two audio files, default CSV writer destination"

rm -f $outfile1
rm -f $outfile2

$r -t $transformpfx-onsets.n3 -w csv $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_csv $outfile1 "$ctx"
check_csv $outfile2 "$ctx"


ctx="onsets transform, two audio files, one-file CSV writer"

$r -t $transformpfx-onsets.n3 -w csv --csv-one-file $tmpcsv $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_csv $tmpcsv "$ctx"


ctx="onsets transform, two audio files, stdout CSV writer"

$r -t $transformpfx-onsets.n3 -w csv --csv-stdout $infile1 $infile2 2>/dev/null >$tmpcsv || \
    fail "Fails to run with $ctx"

check_csv $tmpcsv "$ctx"


ctx="existing output file and no --csv-force"

touch $outfile1

$r -t $transformpfx-onsets.n3 -w csv $infile1 2>/dev/null && \
    fail "Fails by completing successfully when output file already exists (should refuse and bail out)"


ctx="existing output file and --csv-force"

touch $outfile1

$r -t $transformpfx-onsets.n3 -w csv --csv-force $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_csv $outfile1 "$ctx"


exit 0
