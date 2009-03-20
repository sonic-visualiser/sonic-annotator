#!/bin/bash

mypath=`dirname $0`
r=$mypath/../runner/sonic-annotator

infile=$mypath/audio/3clicks8.wav
tmpfile=$mypath/tmp_1_$$
tmpcanonical=$mypath/tmp_2_$$
tmpcmp1=$mypath/tmp_3_$$
tmpcmp2=$mypath/tmp_4_$$

trap "rm -f $tmpfile $tmpcanonical $tmpcmp1 $tmpcmp2" 0

fail() {
    echo "Test failed: $1"
    if [ -n "$2" -a -n "$3" ]; then
	echo "Output follows:"
	echo "--"
	cat $2
	echo "--"
	echo "Expected output follows:"
	echo "--"
	cat $3
	echo "--"
	echo "Diff:"
	echo "--"
	diff -u $2 $3
	echo "--"
    fi
    exit 1
}

compare() {
    a=$1
    b=$2
    sort $a > $tmpcmp1
    sort $b > $tmpcmp2
    cmp -s $tmpcmp1 $tmpcmp2
}

# transform to which we have to add summarisation on command line
transform=$mypath/transforms/transforms-nosummaries-percussiononsets-detectionfunction.n3 
expected=$mypath/expected/transforms-summaries-percussiononsets

stransform=$mypath/transforms/transforms-summaries-percussiononsets-detectionfunction.n3 
sexpected=$mypath/expected/transforms-summaries-percussiononsets-from-rdf

$r -t $transform -w csv --csv-stdout $infile > $tmpfile 2>/dev/null || \
    fail "Fails to run transform $transform"

compare $tmpfile ${expected}.csv || \
    fail "Output mismatch for transform $transform" $tmpfile ${expected}.csv

$r -t $transform -w csv --csv-stdout -S mean $infile > $tmpfile 2>/dev/null || \
    fail "Fails to run transform $transform with summary type mean"

compare $tmpfile ${expected}-with-mean.csv || \
    fail "Output mismatch for transform $transform with summary type mean" $tmpfile ${expected}-with-mean.csv

$r -t $transform -w csv --csv-stdout -S min -S max -S mean -S median -S mode -S sum -S variance -S sd -S count --summary-only $infile > $tmpfile 2>/dev/null || \
    fail "Fails to run transform $transform with all summary types and summary-only"

compare $tmpfile ${expected}-all-summaries-only.csv || \
    fail "Output mismatch for transform $transform with all summary types and summary-only" $tmpfile ${expected}-all-summaries-only.csv

$r -t $stransform -w csv --csv-stdout $infile > $tmpfile 2>/dev/null || \
    fail "Fails to run transform $stransform with CSV output"

compare $tmpfile ${sexpected}.csv || \
    fail "Output mismatch for transform $stransform" $tmpfile ${sexpected}.csv

$r -t $stransform -w rdf --rdf-stdout $infile > $tmpfile 2>/dev/null || \
    fail "Fails to run transform $stransform with RDF output"

rapper -i turtle $tmpfile -o turtle 2>/dev/null | grep -v '^@prefix :' | grep -v 'file:/' > $tmpcanonical ||
    fail "Fails to produce parseable RDF/TTL for transform $stransform"

compare $tmpcanonical ${sexpected}.n3 || \
    fail "Output mismatch for canonicalised version of transform $stransform" $tmpcanonical ${sexpected}.n3

exit 0

