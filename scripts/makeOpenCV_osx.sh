#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
WORK_DIR="${SCRIPT_DIR}/../build"
# use realpath to get absolute path
WORK_DIR=$(realpath $WORK_DIR)
CURRENT_DIR=$(pwd)
OUTPUT_DIR=$WORK_DIR/opencv

function verify_opencv_exists() {
    # check that opencv libraries and include files exist
    if [[ ! -d $OUTPUT_DIR ]]; then
        echo "opencv not found in $OUTPUT_DIR"
        return 1
    fi
    if [[ ! -d $OUTPUT_DIR/include ]]; then
        echo "opencv include files not found in $OUTPUT_DIR/include"
        return 1
    fi
    if [[ ! -d $OUTPUT_DIR/lib ]]; then
        echo "opencv libraries not found in $OUTPUT_DIR/lib"
        return 1
    fi
    if [[ ! -f $OUTPUT_DIR/lib/libopencv_core.a ]]; then
        echo "opencv core library not found in $OUTPUT_DIR/lib/libopencv_core.a"
        return 1
    fi
    if [[ ! -f $OUTPUT_DIR/lib/libopencv_imgproc.a ]]; then
        echo "opencv imgproc library not found in $OUTPUT_DIR/lib/libopencv_imgproc.a"
        return 1
    fi
    if [[ ! -f $OUTPUT_DIR/include/opencv4/opencv2/opencv.hpp ]]; then
        echo "opencv includes not found in $OUTPUT_DIR/include/opencv4/opencv2/opencv.hpp"
        return 1
    fi
    echo "opencv found in $OUTPUT_DIR"
    return 0
}

# check if opencv is already built using verify_opencv_exists
if verify_opencv_exists; then
    echo "opencv already built"
    exit 0
fi

# try to download opencv from s3 if not already downloaded, use wget if available
if [[ ! $( which wget ) ]]; then
    echo "wget is not available, please install it e.g. `$ brew install wget`"
    exit 1
fi
OPENCV_TAR_FILENAME="opencv-4.5.2-x86_64.tar.gz"
OPENCV_TAR_LOCATION="${WORK_DIR}/${OPENCV_TAR_FILENAME}"
wget https://obs-backgroundremoval-build.s3.amazonaws.com/opencv-4.5.2-x86_64.tar.gz -O ${OPENCV_TAR_LOCATION}

cd "${SCRIPT_DIR}/../"
tar xzf ${OPENCV_TAR_LOCATION}
rm ${OPENCV_TAR_LOCATION}
cd "${CURRENT_DIR}"

if verify_opencv_exists; then
    echo "opencv successfully downloaded from s3"
    exit 0
fi

# build opencv from source
echo "building opencv from source"

if [[ -d $OUTPUT_DIR ]]; then
    rm -rf $OUTPUT_DIR
fi

if [[ ! -d $WORK_DIR ]]; then
    mkdir -p $WORK_DIR
fi

cd $WORK_DIR

# download opencv if not already downloaded
if [[ ! -d opencv-4.5.2 ]]; then
    if [[ ! $( which wget ) ]]; then
        echo "wget is not available, please install it e.g. `$ brew install wget`"
        exit 1
    fi
    wget https://github.com/opencv/opencv/archive/refs/tags/4.5.2.tar.gz
    tar xzf 4.5.2.tar.gz
    rm 4.5.2.tar.gz
fi

cd opencv-4.5.2/

if [[ ! -d build ]]; then
    mkdir build
else
    rm -f build/CMakeCache.txt
fi

cd build

# build opencv minimal (just core and imgproc)
cmake .. \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13 \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_opencv_apps=OFF \
    -DBUILD_opencv_js=OFF \
    -DBUILD_ANDROID_PROJECTS=OFF \
    -DBUILD_ANDROID_EXAMPLES=OFF \
    -DBUILD_DOCS=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_PACKAGE=OFF \
    -DBUILD_PERF_TESTS=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_WITH_DEBUG_INFO=OFF \
    -DBUILD_WITH_STATIC_CRT=OFF \
    -DBUILD_WITH_DYNAMIC_IPP=OFF \
    -DBUILD_FAT_JAVA_LIB=OFF \
    -DBUILD_ANDROID_SERVICE=OFF \
    -DBUILD_CUDA_STUBS=OFF \
    -DBUILD_JAVA=OFF \
    -DBUILD_OBJC=OFF \
    -DBUILD_opencv_python3=OFF \
    -DINSTALL_CREATE_DISTRIB=OFF \
    -DINSTALL_BIN_EXAMPLES=OFF \
    -DINSTALL_C_EXAMPLES=OFF \
    -DINSTALL_PYTHON_EXAMPLES=OFF \
    -DINSTALL_ANDROID_EXAMPLES=OFF \
    -DINSTALL_TO_MANGLED_PATHS=OFF \
    -DINSTALL_TESTS=OFF \
    -DBUILD_opencv_calib3d=OFF \
    -DBUILD_opencv_core=ON \
    -DBUILD_opencv_dnn=OFF \
    -DBUILD_opencv_features2d=OFF \
    -DBUILD_opencv_flann=OFF \
    -DBUILD_opencv_gapi=OFF \
    -DBUILD_opencv_highgui=OFF \
    -DBUILD_opencv_imgcodecs=OFF \
    -DBUILD_opencv_imgproc=ON \
    -DBUILD_opencv_ml=OFF \
    -DBUILD_opencv_objdetect=OFF \
    -DBUILD_opencv_photo=OFF \
    -DBUILD_opencv_stitching=OFF \
    -DBUILD_opencv_video=OFF \
    -DBUILD_opencv_videoio=OFF \
	-DWITH_PNG=OFF \
	-DWITH_JPEG=OFF \
	-DWITH_TIFF=OFF \
	-DWITH_WEBP=OFF \
    -DWITH_OPENJPEG=OFF \
    -DWITH_JASPER=OFF \
	-DWITH_OPENEXR=OFF \
    -DWITH_FFMPEG=OFF \
    -DWITH_GSTREAMER=OFF \
    -DWITH_1394=OFF \
    -DCMAKE_INSTALL_PREFIX=$OUTPUT_DIR \
    -DWITH_PROTOBUF=OFF \
    -DBUILD_PROTOBUF=OFF \
    -DWITH_CAROTENE=OFF \
    -DWITH_EIGEN=OFF \
    -DWITH_OPENVX=OFF \
    -DWITH_CLP=OFF \
    -DWITH_DIRECTX=OFF \
    -DWITH_VA=OFF \
    -DWITH_LAPACK=OFF \
    -DWITH_QUIRC=OFF \
    -DWITH_ADE=OFF \
    && \
    cmake --build . --target install -- -j8

cd "$CURRENT_DIR"
