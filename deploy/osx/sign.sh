#!/bin/bash 

set -e

exe="$1"

if [ ! -f "$exe" ] || [ -n "$2" ]; then
    echo "Usage: $0 <executable>"
    echo "  e.g. $0 my-program"
    exit 2
fi

set -u

entitlements=deploy/osx/Entitlements.plist

codesign -s "Developer ID Application: Chris Cannam" -fv --options runtime --entitlements "$entitlements" "$exe"

