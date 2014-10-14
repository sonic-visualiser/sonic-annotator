#!/bin/bash

. ../include.sh

infile1=$audiopath/3clicks8.wav
infile2=$audiopath/6clicks8.wav

infile1dot=$audiopath/3.clicks.8.wav

outfile1=3clicks8.n3
outfile2=6clicks8.n3

outfile3=3clicks8_vamp_vamp-example-plugins_percussiononsets_onsets.n3
outfile4=3clicks8_vamp_vamp-example-plugins_percussiononsets_detectionfunction.n3
outfile5=6clicks8_vamp_vamp-example-plugins_percussiononsets_onsets.n3
outfile6=6clicks8_vamp_vamp-example-plugins_percussiononsets_detectionfunction.n3

outfile1dot=3.clicks.8.n3

tmpttl=$mypath/tmp_1_$$.ttl

trap "rm -f $tmpttl $outfile1 $outfile2 $outfile3 $outfile4 $outfile5 $outfile6 $infile1dot $outfile1dot $audiopath/$outfile1 $audiopath/$outfile2 $audiopath/$outfile3 $audiopath/$outfile4 $audiopath/$outfile5 $audiopath/$outfile6 $audiopath/$outfile1dot" 0

transformdir=$mypath/transforms

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

rm -f $audiopath/$outfile1

$r -t $transformdir/onsets.n3 -w rdf $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $audiopath/$outfile1 "$ctx"


ctx="onsets transform, one audio file with dots in filename, default RDF writer destination"

rm -f $audiopath/$outfile1

cp $infile1 $infile1dot

$r -t $transformdir/onsets.n3 -w rdf $infile1dot 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $audiopath/$outfile1dot "$ctx"

rm -f $infile1dot $audiopath/$outfile1dot


ctx="onsets and df transforms, one audio file, default RDF writer destination"

rm -f $audiopath/$outfile1

$r -t $transformdir/onsets.n3 -t $transformdir/detectionfunction.n3 -w rdf $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $audiopath/$outfile1 "$ctx"


ctx="onsets transform, two audio files, default RDF writer destination"

rm -f $audiopath/$outfile1
rm -f $audiopath/$outfile2

$r -t $transformdir/onsets.n3 -w rdf $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $audiopath/$outfile1 "$ctx"
check_rdf $audiopath/$outfile2 "$ctx"


ctx="onsets transform, two audio files, one-file RDF writer"

$r -t $transformdir/onsets.n3 -w rdf --rdf-one-file $tmpttl $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $tmpttl "$ctx"


ctx="onsets transform, two audio files, stdout RDF writer"

$r -t $transformdir/onsets.n3 -w rdf --rdf-stdout $infile1 $infile2 2>/dev/null >$tmpttl || \
    fail "Fails to run with $ctx"

check_rdf $tmpttl "$ctx"


ctx="onsets transform, one audio file, many-files RDF writer"

rm -f $audiopath/$outfile3

$r -t $transformdir/onsets.n3 -w rdf --rdf-many-files $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $audiopath/$outfile3 "$ctx"


ctx="onsets transform, two audio files, many-files RDF writer"

rm -f $audiopath/$outfile3
rm -f $audiopath/$outfile5

$r -t $transformdir/onsets.n3 -w rdf --rdf-many-files $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $audiopath/$outfile3 "$ctx"
check_rdf $audiopath/$outfile5 "$ctx"


ctx="onsets and df transforms, two audio files, many-files RDF writer"

rm -f $audiopath/$outfile3
rm -f $audiopath/$outfile4
rm -f $audiopath/$outfile5
rm -f $audiopath/$outfile6

$r -t $transformdir/onsets.n3 -t $transformdir/detectionfunction.n3 -w rdf --rdf-many-files $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $audiopath/$outfile3 "$ctx"
check_rdf $audiopath/$outfile4 "$ctx"
check_rdf $audiopath/$outfile5 "$ctx"
check_rdf $audiopath/$outfile6 "$ctx"


ctx="output base directory"

rm -f ./$outfile1

$r -t $transformdir/onsets.n3 -t $transformdir/detectionfunction.n3 -w rdf --rdf-basedir . $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf ./$outfile1 "$ctx"


ctx="output base directory and many-files"

rm -f ./$outfile3
rm -f ./$outfile5

$r -t $transformdir/onsets.n3 -w rdf --rdf-basedir . --rdf-many-files $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf ./$outfile3 "$ctx"
check_rdf ./$outfile5 "$ctx"


ctx="nonexistent output base directory"

$r -t $transformdir/onsets.n3 -w rdf --rdf-basedir ./DOES_NOT_EXIST $infile1 2>/dev/null && \
    fail "Fails with $ctx by completing successfully (should refuse and bail out)"


ctx="existing output file and no --rdf-force"

touch $audiopath/$outfile1

$r -t $transformdir/onsets.n3 -w rdf $infile1 2>/dev/null && \
    fail "Fails by completing successfully when output file already exists (should refuse and bail out)"


ctx="existing output file and --rdf-force"

touch $audiopath/$outfile1

$r -t $transformdir/onsets.n3 -w rdf --rdf-force $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_rdf $audiopath/$outfile1 "$ctx"


exit 0
