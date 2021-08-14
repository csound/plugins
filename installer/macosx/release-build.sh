#!/bin/sh
# set -x
# directories
export RELEASE_DIR="`eval date +%Y-%m-%d-%H%M%S`"

# build
mkdir $RELEASE_DIR
cd $RELEASE_DIR
export BUILD_DIR=build
mkdir $BUILD_DIR
git clone https://github.com/csound/plugins
cd $BUILD_DIR
cmake ../plugins -DCMAKE_BUILD_TYPE=Release
make

# zip install

# image opcodes (LIB_PNG needs to be set correctly)
echo "--- making image zip ---"
cd ..
export IMAGE_DIR=$BUILD_DIR/image
export LIB_PNG=/usr/local/lib/libpng16.16.dylib
cp $IMAGE_DIR/libimage.dylib .
cp $LIB_PNG .
install_name_tool -id libpng16.16.dylib libpng16.16.dylib
install_name_tool -change /usr/local/lib/libpng16.16.dylib libpng16.16.dylib libimage.dylib
mkdir image
mv *.dylib image
zip image.zip image/*.dylib

# chua
echo "--- making chua zip ---"
export CHUA_DIR=$BUILD_DIR/chua
mkdir chua
cp $CHUA_DIR/libchua.dylib chua
zip chua.zip chua/*.dylib

# faustcsound
echo "--- making faustcsound zip ---"
export FCS_DIR=$BUILD_DIR/faustcsound
mkdir faustcsound
cp $FCS_DIR/libfaustcsound.dylib faustcsound
zip faustcsound.zip faustcsound/*.dylib

# python opcodes (python VERSION used needs to be set correctly)
echo "--- making py zip ---"
export VERSION=3.9
export PY_ZIP_DIR="python${VERSION}-opcodes"
export PY_DIR=$BUILD_DIR/py
mkdir $PY_ZIP_DIR
cp $PY_DIR/libpy.dylib $PY_ZIP_DIR
zip $PY_ZIP_DIR.zip $PY_ZIP_DIR/*.dylib

cd ..
open $RELEASE_DIR
