#!/bin/sh

svn commit || exit

for x in base data plugin rdf system transform; do
    svn commit $x || exit
done

