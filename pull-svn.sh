#!/bin/sh

svrepo=https://sv1.svn.sourceforge.net/svnroot/sv1/sonic-visualiser/trunk

svn co $svrepo/audioio
svn co $svrepo/base
svn co $svrepo/data
svn co $svrepo/plugin
svn co $svrepo/rdf
svn co $svrepo/system
svn co $svrepo/transform


