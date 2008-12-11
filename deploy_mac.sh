#!/bin/bash

# this script should be executed from the directory that contains the app directory (application bundle)
# it copies the required 3rd party libraries into the application bundle and corrects the library install names and references

TARGETPATH="sonic-annotator.app/Contents/Frameworks/"

mkdir "$TARGETPATH"

QTPREFIX=/Library/Frameworks/
QTFWKS="QtXml QtCore QtNetwork"

# copy the dynamic libraries into the app bundle

for FWK in $QTFWKS; do
  cp ${QTPREFIX}${FWK}.framework/Versions/4/${FWK} "${TARGETPATH}"
done

# change the id's of the dylibs
for FWK in $QTFWKS; do
  install_name_tool -id @executable_path/../Frameworks/${FWK} "$TARGETPATH/$FWK"
done

# tell the linker to look for dylibs in the app bundle
for FWK in $QTFWKS; do
  install_name_tool -change ${FWK}.framework/Versions/4/${FWK} @executable_path/../Frameworks/${FWK} "sonic-annotator.app/Contents/MacOS/sonic-annotator"
done

# correct dependencies between QT dylibs
for FWK in $QTFWKS; do
  case $FWK in QtCore) continue;; esac
  install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore "$TARGETPATH/${FWK}"
done

