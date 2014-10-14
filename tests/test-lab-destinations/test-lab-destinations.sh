#!/bin/bash

. ../include.sh

infile1=$audiopath/3clicks8.wav
infile2=$audiopath/6clicks8.wav

outfile1=$audiopath/3clicks8_vamp_vamp-example-plugins_percussiononsets_onsets.lab
outfile2=$audiopath/6clicks8_vamp_vamp-example-plugins_percussiononsets_onsets.lab

infile1dot=$audiopath/3.clicks.8.wav
outfile1dot=$audiopath/3.clicks.8_vamp_vamp-example-plugins_percussiononsets_onsets.lab

outfile3=$audiopath/3clicks8_vamp_vamp-example-plugins_percussiononsets_onsets.lab
outfile4=$audiopath/3clicks8_vamp_vamp-example-plugins_percussiononsets_detectionfunction.lab

tmplab=$mypath/tmp_1_$$.lab

trap "rm -f $tmplab $outfile1 $outfile2 $outfile3 $outfile4 $infile1dot $outfile1dot" 0

transformdir=$mypath/transforms

check_lab() {
    test -f $1 || \
	fail "Fails to write output to expected location $1 for $2"
    # every line must contain the same number of tabs
    formats=`awk -F'\t' '{ print NF; }' $1 | sort | uniq | wc | awk '{ print $1 }'`
    if [ "$formats" != "1" ]; then
	fail "Output is not consistently formatted tab-separated file for $2"
    fi
    rm -f $1
}    


ctx="onsets transform, one audio file, default LAB writer destination"

rm -f $outfile1

$r -t $transformdir/onsets.n3 -w lab $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_lab $outfile1 "$ctx"


ctx="onsets transform, one audio file with dots in filename, default LAB writer destination"

rm -f $outfile1

cp $infile1 $infile1dot

$r -t $transformdir/onsets.n3 -w lab $infile1dot 2>/dev/null || \
    fail "Fails to run with $ctx"

check_lab $outfile1dot "$ctx"

rm -f $infile1dot $outfile1dot


ctx="onsets and df transforms, one audio file, default LAB writer destination"

rm -f $outfile1

$r -t $transformdir/onsets.n3 -t $transformdir/detectionfunction.n3 -w lab $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_lab $outfile1 "$ctx"


ctx="onsets transform, two audio files, default LAB writer destination"

rm -f $outfile1
rm -f $outfile2

$r -t $transformdir/onsets.n3 -w lab $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_lab $outfile1 "$ctx"
check_lab $outfile2 "$ctx"


ctx="onsets transform, two audio files, one-file LAB writer"

# Writer should refuse to write results from more than one audio file
# together (as the audio file is not identified)

$r -t $transformdir/onsets.n3 -w lab --lab-one-file $tmplab $infile1 $infile2 2>/dev/null && \
    fail "Fails by completing successfully with $ctx"


ctx="onsets transform, two audio files, stdout LAB writer"

$r -t $transformdir/onsets.n3 -w lab --lab-stdout $infile1 $infile2 2>/dev/null >$tmplab || \
    fail "Fails to run with $ctx"

check_lab $tmplab "$ctx"


ctx="existing output file and no --lab-force"

touch $outfile1

$r -t $transformdir/onsets.n3 -w lab $infile1 2>/dev/null && \
    fail "Fails by completing successfully when output file already exists (should refuse and bail out)"


ctx="existing output file and --lab-force"

touch $outfile1

$r -t $transformdir/onsets.n3 -w lab --lab-force $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_lab $outfile1 "$ctx"

exit 0
