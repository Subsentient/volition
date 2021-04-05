#!/bin/bash
set -e
git submodule init
git submodule update
cd $1
rm -f luaffi.a || true
cd cffi-lua/
rm -rf build || true
mkdir build
cd build
meson .. -Dstatic=true
ninja all
cp -fv `find . -name "*.a" -type f` ../../luaffi.a
