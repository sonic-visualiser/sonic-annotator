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

gatekeeper_key="Developer ID Application: Particular Programs Ltd (73F996B92S)"

codesign -s "$gatekeeper_key" -fv --options runtime --entitlements "$entitlements" "$exe"

