#!/bin/bash

WORK_DIR=$(git rev-parse --show-toplevel)/build_arm64
CURRENT_DIR=$(pwd)
OUTPUT_DIR=$WORK_DIR/onnxruntime

if [[ -d $OUTPUT_DIR ]]; then
    rm -rf $OUTPUT_DIR
fi

if [[ ! -d $WORK_DIR ]]; then
    mkdir -p $WORK_DIR
fi

cd $WORK_DIR

ONNXRT_VER=1.7.2
ONNXRT_DIR=$WORK_DIR/onnxruntime-$ONNXRT_VER

if [[ ! -d $ONNXRT_DIR ]]; then
    git clone --single-branch -b v$ONNXRT_VER https://github.com/microsoft/onnxruntime $ONNXRT_DIR
fi

cd $ONNXRT_DIR

./build.sh --config RelWithDebInfo --parallel --skip_tests

cd build/MacOS/RelWithDebInfo

cmake $WORK_DIR/onnxruntime-$ONNXRT_VER/cmake -DCMAKE_INSTALL_PREFIX=$OUTPUT_DIR && \
    cmake --build . --target install

mkdir -p $OUTPUT_DIR/lib
for f in $( find . -name "lib*.a" ); do
    cp $f $OUTPUT_DIR/lib
done

cd $CURRENT_DIR
