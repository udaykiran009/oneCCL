#!/bin/bash

rm -r doxygen/xml

echo "Building API using Doxygen..."

doxygen Doxyfile

echo "Building Sphinx HTML output..."

pushd rst/source
  sphinx-build -b html ./ ../build/html
popd
