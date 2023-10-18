#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
BUILD_DIR=$SCRIPT_DIR/build

rm -rf $BUILD_DIR
mkdir $BUILD_DIR
pushd $BUILD_DIR
cmake -GNinja $SCRIPT_DIR -DATOMIC_TESTS_PERFORM=1
cmake --build .
ctest . --verbose
popd
rm -rf $BUILD_DIR