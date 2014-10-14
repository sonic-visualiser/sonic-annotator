#!/bin/bash

. ../include.sh

infile1=$audiopath/3clicks8.wav
infile2=$audiopath/6clicks8.wav

infile1dot=$audiopath/3.clicks.8.wav

outfile1=3clicks8.json
outfile2=6clicks8.json

outfile3=3clicks8_vamp_vamp-example-plugins_percussiononsets_onsets.json
outfile4=3clicks8_vamp_vamp-example-plugins_percussiononsets_detectionfunction.json
outfile5=6clicks8_vamp_vamp-example-plugins_percussiononsets_onsets.json
outfile6=6clicks8_vamp_vamp-example-plugins_percussiononsets_detectionfunction.json

outfile1dot=3.clicks.8.json

tmpjson=$mypath/tmp_1_$$.json

trap "rm -f $tmpjson $outfile1 $outfile2 $outfile3 $outfile4 $outfile5 $outfile6 $infile1dot $outfile1dot $audiopath/$outfile1 $audiopath/$outfile2 $audiopath/$outfile3 $audiopath/$outfile4 $audiopath/$outfile5 $audiopath/$outfile6 $audiopath/$outfile1dot" 0

transformdir=$mypath/transforms

check_json() {
    test -f $1 || \
	fail "Fails to write output to expected location $1 for $2"
    cat $1 | json_verify || \
	fail "Writes invalid JSON to location $1 for $2"
    rm -f $1
}    


ctx="onsets transform, one audio file, default JSON writer destination"

rm -f $audiopath/$outfile1

$r -t $transformdir/onsets.n3 -w json $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_json $audiopath/$outfile1 "$ctx"


ctx="onsets transform, one audio file with dots in filename, default JSON writer destination"

rm -f $audiopath/$outfile1

cp $infile1 $infile1dot

$r -t $transformdir/onsets.n3 -w json $infile1dot 2>/dev/null || \
    fail "Fails to run with $ctx"

check_json $audiopath/$outfile1dot "$ctx"

rm -f $infile1dot $audiopath/$outfile1dot


ctx="onsets and df transforms, one audio file, default JSON writer destination"

rm -f $audiopath/$outfile1

$r -t $transformdir/onsets.n3 -t $transformdir/detectionfunction.n3 -w json $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_json $audiopath/$outfile1 "$ctx"


ctx="onsets transform, two audio files, default JSON writer destination"

rm -f $audiopath/$outfile1
rm -f $audiopath/$outfile2

$r -t $transformdir/onsets.n3 -w json $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_json $audiopath/$outfile1 "$ctx"
check_json $audiopath/$outfile2 "$ctx"


ctx="onsets transform, two audio files, one-file JSON writer"

$r -t $transformdir/onsets.n3 -w json --json-one-file $tmpjson $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_json $tmpjson "$ctx"


ctx="onsets transform, two audio files, stdout JSON writer"

$r -t $transformdir/onsets.n3 -w json --json-stdout $infile1 $infile2 2>/dev/null >$tmpjson || \
    fail "Fails to run with $ctx"

check_json $tmpjson "$ctx"


ctx="onsets transform, one audio file, many-files JSON writer"

rm -f $audiopath/$outfile3

$r -t $transformdir/onsets.n3 -w json --json-many-files $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_json $audiopath/$outfile3 "$ctx"


ctx="onsets transform, two audio files, many-files JSON writer"

rm -f $audiopath/$outfile3
rm -f $audiopath/$outfile5

$r -t $transformdir/onsets.n3 -w json --json-many-files $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_json $audiopath/$outfile3 "$ctx"
check_json $audiopath/$outfile5 "$ctx"


ctx="onsets and df transforms, two audio files, many-files JSON writer"

rm -f $audiopath/$outfile3
rm -f $audiopath/$outfile4
rm -f $audiopath/$outfile5
rm -f $audiopath/$outfile6

$r -t $transformdir/onsets.n3 -t $transformdir/detectionfunction.n3 -w json --json-many-files $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_json $audiopath/$outfile3 "$ctx"
check_json $audiopath/$outfile4 "$ctx"
check_json $audiopath/$outfile5 "$ctx"
check_json $audiopath/$outfile6 "$ctx"


ctx="output base directory"

rm -f ./$outfile1

$r -t $transformdir/onsets.n3 -t $transformdir/detectionfunction.n3 -w json --json-basedir . $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_json ./$outfile1 "$ctx"


ctx="output base directory and many-files"

rm -f ./$outfile3
rm -f ./$outfile5

$r -t $transformdir/onsets.n3 -w json --json-basedir . --json-many-files $infile1 $infile2 2>/dev/null || \
    fail "Fails to run with $ctx"

check_json ./$outfile3 "$ctx"
check_json ./$outfile5 "$ctx"


ctx="nonexistent output base directory"

$r -t $transformdir/onsets.n3 -w json --json-basedir ./DOES_NOT_EXIST $infile1 2>/dev/null && \
    fail "Fails with $ctx by completing successfully (should refuse and bail out)"


ctx="existing output file and no --json-force"

touch $audiopath/$outfile1

$r -t $transformdir/onsets.n3 -w json $infile1 2>/dev/null && \
    fail "Fails by completing successfully when output file already exists (should refuse and bail out)"


ctx="existing output file and --json-force"

touch $audiopath/$outfile1

$r -t $transformdir/onsets.n3 -w json --json-force $infile1 2>/dev/null || \
    fail "Fails to run with $ctx"

check_json $audiopath/$outfile1 "$ctx"


exit 0
