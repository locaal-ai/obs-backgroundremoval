#!/bin/bash

set -e

# get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
WORK_DIR="${SCRIPT_DIR}/../build"
# get absolute path, without realpath
WORK_DIR=$(cd $WORK_DIR && pwd)
CURRENT_DIR=$(pwd)
OUTPUT_DIR=$WORK_DIR/onnxruntime

function verify_onnxruntime_exists() {
    # check that onnxruntime libraries and include files exist
    if [[ ! -d $OUTPUT_DIR ]]; then
        echo "onnxruntime not found in $OUTPUT_DIR"
        return 1
    fi
    if [[ ! -d $OUTPUT_DIR/include ]]; then
        echo "onnxruntime include files not found in $OUTPUT_DIR/include"
        return 1
    fi
    if [[ ! -d $OUTPUT_DIR/lib ]]; then
        echo "onnxruntime libraries not found in $OUTPUT_DIR/lib"
        return 1
    fi
    if [[ ! -f $OUTPUT_DIR/lib/libonnx.a ]]; then
        echo "onnxruntime library not found in $OUTPUT_DIR/lib/libonnx.a"
        return 1
    fi
    if [[ ! -f $OUTPUT_DIR/include/onnxruntime/core/session/onnxruntime_c_api.h ]]; then
        echo "onnxruntime includes not found in $OUTPUT_DIR/include/onnxruntime/core/session/onnxruntime_c_api.h"
        return 1
    fi
    echo "onnxruntime found in $OUTPUT_DIR"
    return 0
}

# check if onnxruntime is already built using verify_onnxruntime_exists
if verify_onnxruntime_exists; then
    echo "onnxruntime already built"
    exit 0
fi

# try to download onnxruntime from s3 if not already downloaded, use wget if available
# https://obs-backgroundremoval-build.s3.amazonaws.com/onnxruntime-1.14.0-x86_64.tar.gz
if [[ ! $( which wget ) ]]; then
    echo "wget is not available, please install it e.g. `$ brew install wget`"
    exit 1
fi
ONNXRT_TAR_FILENAME="onnxruntime-1.14.0-x86_64.tar.gz"
ONNXRT_TAR_LOCATION="${WORK_DIR}/${ONNXRT_TAR_FILENAME}"
if [[ ! -f $ONNXRT_TAR_LOCATION ]]; then
    echo "onnxruntime tar file not found in $ONNXRT_TAR_LOCATION, downloading from s3"
    wget -O $ONNXRT_TAR_LOCATION https://obs-backgroundremoval-build.s3.amazonaws.com/$ONNXRT_TAR_FILENAME
fi
if [[ ! -f $ONNXRT_TAR_LOCATION ]]; then
    echo "onnxruntime tar file not found in $ONNXRT_TAR_LOCATION"
    exit 1
fi
echo "onnxruntime tar file found in $ONNXRT_TAR_LOCATION"

cd "${WORK_DIR}/.."
echo "extracting onnxruntime tar file"
tar -xzf $ONNXRT_TAR_LOCATION
rm $ONNXRT_TAR_LOCATION
cd $CURRENT_DIR

# verify onnxruntime exists
if verify_onnxruntime_exists; then
    echo "onnxruntime successfully downloaded and extracted"
    exit 0
fi

# build onnxruntime from source
echo "building onnxruntime from source"

if [[ -d $OUTPUT_DIR ]]; then
    rm -rf $OUTPUT_DIR
fi

if [[ ! -d $WORK_DIR ]]; then
    mkdir -p $WORK_DIR
fi

cd $WORK_DIR

ONNXRT_VER=1.14.0
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
