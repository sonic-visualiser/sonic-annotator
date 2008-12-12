#!/bin/sh

svrepo=https://sv1.svn.sourceforge.net/svnroot/sv1/sonic-visualiser/trunk

for x in audioio base data plugin rdf system transform; do
    svn co $svrepo/$x
done

