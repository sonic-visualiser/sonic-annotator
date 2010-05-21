#!/bin/sh

svn diff || exit

for x in base data plugin rdf system transform; do
    svn diff $x || exit
done

