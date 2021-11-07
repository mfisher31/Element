#!/bin/sh
# Builds Element standalone for windows using mingw32 gcc

set -e
here=`pwd`

./waf configure --depends="/depends/x86_64-w64-mingw32" \
    --prefix="/" \
    --bindir="/" \
    --libdir="/" \
    --scriptsdir="/scripts" \
    --luadir="/lua"
./waf build -j4

pkgname="element-win64-mingw32-`./waf version`"
win64dir="/dist/${pkgname}"
./waf install --destdir="$win64dir"
cp build/bin/*.dll "$win64dir"

find "$win64dir" -name \*.exe -exec x86_64-w64-mingw32-strip {} \;
find "$win64dir" -name \*.dll -exec x86_64-w64-mingw32-strip {} \;

cd /dist && zip -rvT "${pkgname}.zip" "${pkgname}"