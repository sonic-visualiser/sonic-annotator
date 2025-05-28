#!/bin/bash

## The following assumes we have generated an app password at
## appleid.apple.com, then
## xcrun notarytool store-credentials notarytool-cannam --apple-id X --team-id Y
## Note! if the machine is locked when this gets run, it will fail as if no
## credentials had ever been stored. Don't be deceived

set -e

exe="$1"

if [ ! -f "$exe" ] || [ -n "$2" ]; then
    echo "Usage: $0 <executable>"
    echo "  e.g. $0 my-program"
    exit 2
fi

set -u

user="appstore@particularprograms.co.uk"
team_id="73F996B92S"

. deploy/metadata.sh

rm -f bundle.zip
rm -rf bundle
mkdir bundle
cp "$exe" bundle/
ditto -c -k bundle bundle.zip

echo
echo "Uploading for notarization..."

xcrun notarytool submit \
    bundle.zip \
    --apple-id "$user" \
    --team-id "$team_id" \
    --keychain-profile notarytool-cannam \
    --wait --progress

echo
echo "Done, not stapling as just an executable"

