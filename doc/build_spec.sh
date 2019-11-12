#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SPEC_DIR=$SCRIPT_DIR/spec
SPEC_SOURCE_DIR=$SPEC_DIR/source
SPEC_RST_BUILD_DIR=$SPEC_DIR/build
SPEC_RST_HTML_BUILD_DIR=$SPEC_RST_BUILD_DIR/html
RST_DIR=$SCRIPT_DIR/rst
RST_SOURCE_DIR=$RST_DIR/source

if ! [ -x "$(command -v sphinx-build)" ]
then
  echo 'Error: Sphinx not found.'
  exit 1
fi

echo "Copying RST files for spec..."
cp -a $RST_SOURCE_DIR/spec/. $SPEC_SOURCE_DIR/spec

if [ -d "$SPEC_RST_HTML_BUILD_DIR" ]
then
  echo "Removing $SPEC_RST_HTML_BUILD_DIR"
  rm -r $SPEC_RST_HTML_BUILD_DIR
fi

echo "Generating HTML using Sphinx..."
sphinx-build -b html $SPEC_SOURCE_DIR $SPEC_RST_HTML_BUILD_DIR

echo "Removing files from build/html"

rm -r $SPEC_RST_HTML_BUILD_DIR/.doctrees
rm -r $SPEC_RST_HTML_BUILD_DIR/.buildinfo
