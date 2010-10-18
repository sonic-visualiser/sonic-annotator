#!/bin/bash

mypath=`dirname $0`
r=$mypath/../sonic-annotator

infile1=$mypath/audio/3clicks8.wav
infile2=$mypath/audio/6clicks8.wav

outfile1=$mypath/audio/3clicks8.n3
outfile2=$mypath/audio/6clicks8.n3

outfile3=$mypath/audio/3clicks8_vamp_vamp-example-plugins_percussiononsets_onsets.n3
outfile4=$mypath/audio/3clicks8_vamp_vamp-example-plugins_percussiononsets_detectionfunction.n3
outfile5=$mypath/audio/6clicks8_vamp_vamp-example-plugins_percussiononsets_onsets.n3
outfile6=$mypath/audio/6clicks8_vamp_vamp-example-plugins_percussiononsets_detectionfunction.n3

testplug=vamp:vamp-example-plugins:percussiononsets
tmpttl=$mypath/tmp_1_$$.ttl

trap "rm -f $tmpttl $outfile1 $outfile2 $outfile3 $outfile4 $outfile5 $outfile6" 0

fail() {
    echo "Test failed: $1"
    exit 1
}

transformpfx=$mypath/transforms/transforms-rdf-writer-percussiononsets

check_rdf() {
    test -f $1 || \
	fail "Fails to write output to expected location $1 for $2"
    rapper -i turtle $1 >/dev/null 2>&1 || \
	fail "Fails to produce parseable RDF/TTL for $2"
    rapper -i turtle -c $1 2>&1 | egrep -q 'Parsing returned [1-9][0-9]+ triples' || \
	fail "RDF output contains no triples (?) for $2"
    rm -f $1
}    


ctx="onsets transform, one audio file, default RDF writer destination"

rm -f $outfile1

$r -t $transformpfx-onsets.n3 -w rdf $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $outfile1 "$ctx"


ctx="onsets and df transforms, one audio file, default RDF writer destination"

rm -f $outfile1

$r -t $transformpfx-onsets.n3 -t $transformpfx-detectionfunction.n3 -w rdf $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $outfile1 "$ctx"


ctx="onsets transform, two audio files, default RDF writer destination"

rm -f $outfile1
rm -f $outfile2

$r -t $transformpfx-onsets.n3 -w rdf $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $outfile1 "$ctx"
check_rdf $outfile2 "$ctx"


ctx="onsets transform, two audio files, one-file RDF writer"

$r -t $transformpfx-onsets.n3 -w rdf --rdf-one-file $tmpttl $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $tmpttl "$ctx"


ctx="onsets transform, two audio files, stdout RDF writer"

$r -t $transformpfx-onsets.n3 -w rdf --rdf-stdout $infile1 $infile2 2>/dev/null >$tmpttl || \
    fail "Fails to run with $ctx"

check_rdf $tmpttl "$ctx"


ctx="onsets transform, one audio file, many-files RDF writer"

rm -f $outfile3

$r -t $transformpfx-onsets.n3 -w rdf --rdf-many-files $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $outfile3 "$ctx"


ctx="onsets transform, two audio files, many-files RDF writer"

rm -f $outfile3
rm -f $outfile5

$r -t $transformpfx-onsets.n3 -w rdf --rdf-many-files $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $outfile3 "$ctx"
check_rdf $outfile5 "$ctx"


ctx="onsets and df transforms, two audio files, many-files RDF writer"

rm -f $outfile3
rm -f $outfile4
rm -f $outfile5
rm -f $outfile6

$r -t $transformpfx-onsets.n3 -t $transformpfx-detectionfunction.n3 -w rdf --rdf-many-files $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $outfile3 "$ctx"
check_rdf $outfile4 "$ctx"
check_rdf $outfile5 "$ctx"
check_rdf $outfile6 "$ctx"


ctx="existing output file and no --rdf-force"

touch $outfile1

$r -t $transformpfx-onsets.n3 -w rdf $infile1 2>/dev/null && \
    fail "Fails by completing successfully when output file already exists (should refuse and bail out)"


ctx="existing output file and --rdf-force"

touch $outfile1

$r -t $transformpfx-onsets.n3 -w rdf --rdf-force $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $outfile1 "$ctx"


exit 0
