#!/bin/bash

. ../include.sh

infile=$audiopath/3clicks8.wav
infile2=$audiopath/6clicks8.wav
tmpfile=$mypath/tmp_1_$$
tmpcanonical=$mypath/tmp_2_$$
expcanonical=$mypath/tmp_exp_2_$$
tmpcmp1=$mypath/tmp_3_$$
tmpcmp2=$mypath/tmp_4_$$

trap "rm -f $tmpfile $tmpcanonical $expcanonical $tmpcmp1 $tmpcmp2" 0

compare() {
    a=$1
    b=$2
    sort $a > $tmpcmp1
    sort $b > $tmpcmp2
    csvcompare $tmpcmp1 $tmpcmp2
}

# transform to which we have to add summarisation on command line
transform=$mypath/transforms/detectionfunction-nosummaries.n3 
expected=$mypath/expected/summaries

stransform=$mypath/transforms/detectionfunction.n3 
sexpected=$mypath/expected/summaries-from-rdf

seglist=$mypath/transforms/segmentlist

$r -t $transform -w csv --csv-stdout $infile > $tmpfile 2>/dev/null || \
    fail "Fails to run transform $transform"

compare $tmpfile ${expected}.csv || \
    faildiff "Output mismatch for transform $transform" $tmpfile ${expected}.csv

$r -t $transform -w csv --csv-stdout -S mean $infile > $tmpfile 2>/dev/null || \
    fail "Fails to run transform $transform with summary type mean"

compare $tmpfile ${expected}-with-mean.csv || \
    faildiff "Output mismatch for transform $transform with summary type mean" $tmpfile ${expected}-with-mean.csv

$r -t $transform -w csv --csv-stdout -S min -S max -S mean -S median -S mode -S sum -S variance -S sd -S count --summary-only $infile > $tmpfile 2>/dev/null || \
    fail "Fails to run transform $transform with all summary types and summary-only"

compare $tmpfile ${expected}-all-summaries-only.csv || \
    faildiff "Output mismatch for transform $transform with all summary types and summary-only" $tmpfile ${expected}-all-summaries-only.csv

$r -t $stransform -w csv --csv-stdout $infile > $tmpfile 2>/dev/null || \
    fail "Fails to run transform $stransform with CSV output"

compare $tmpfile ${sexpected}.csv || \
    faildiff "Output mismatch for transform $stransform" $tmpfile ${sexpected}.csv

$r -t $stransform -w csv --csv-stdout --summary-only $infile > $tmpfile 2>/dev/null || \
    fail "Fails to run transform $stransform with CSV output and summary-only"

compare $tmpfile ${expected}-from-rdf-summaries-only.csv || \
    faildiff "Output mismatch for transform $stransform with summary-only" $tmpfile ${expected}-from-rdf-summaries-only.csv

$r -t $transform -w csv --csv-stdout --summary-only -S median --segments 0,9.9 $infile2 > $tmpfile 2>/dev/null || \
    fail "Fails to run transform $stransform with CSV output and segments"

compare $tmpfile ${expected}-segments.csv || \
    faildiff "Output mismatch for transform $stransform with segments" $tmpfile ${expected}-segments.csv

$r -t $transform -w csv --csv-stdout --summary-only -S median --segments-from $seglist $infile2 > $tmpfile 2>/dev/null || \
    fail "Fails to run transform $stransform with CSV output and segments from segment list"

compare $tmpfile ${expected}-segments.csv || \
    faildiff "Output mismatch for transform $stransform with segments from segment list" $tmpfile ${expected}-segments.csv

$r -t $stransform -w rdf --rdf-stdout $infile > $tmpfile 2>/dev/null || \
    fail "Fails to run transform $stransform with RDF output"

rapper -i turtle $tmpfile -o turtle 2>/dev/null | grep -v '^@prefix :' | grep -v 'file:/' > $tmpcanonical ||
    fail "Fails to produce parseable RDF/TTL for transform $stransform"

rapper -i turtle ${sexpected}.n3 -o turtle 2>/dev/null | grep -v '^@prefix :' | grep -v 'file:/' > $expcanonical ||
    fail "Internal error: Failed to canonicalise expected output file $sexpected.n3"

compare $tmpcanonical $expcanonical || \
    faildiff "Output mismatch against expected $sexpected.n3 for canonicalised version of transform $stransform" $tmpcanonical $expcanonical

exit 0

