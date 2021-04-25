# OBS Plugin: Background Removal

- [Introduction](#introduction)
- [Building](#building)
  - [MacOSX](#mac-osx)
  - [Linux / Ubuntu](#linux-ubuntu)

## Introduction

This plugin is meant to make it easy to replace the background in portrait images and video.
It is using a neural network to predict the mask of the portrait and remove the background pixels.
It's easily composable with other OBS plugins to replace the background with e.g. an image or
a transparent color.

![](demo.gif)

The model used for background detection is SINet: https://arxiv.org/abs/1911.09099
The pre-trained model weights were taken from: https://github.com/anilsathyan7/Portrait-Segmentation/tree/master/SINet

Some more information about how I built it: https://www.morethantechnical.com/2021/04/15/obs-plugin-for-portrait-background-removal-with-onnx-sinet-model/

## Building

The plugin was built and tested on Mac OSX. Building for Windows is outstanding, and help is appreciated.

### Prerequisites for building
- OpenCV v4.5+: https://github.com/opencv/opencv/
- ONNXRuntime: https://github.com/microsoft/onnxruntime

### Mac OSX

Install dependencies:
```
$ brew install opencv onnxruntime
```

#### Finding libobs

If you install the desktop OBS app (https://obsproject.com/download) you already have the binaries for libobs (e.g. `/Applications/OBS.app/Contents/Frameworks/libobs.0.dylib`)
But you don't have the headers - so clone the main obs repo e.g. `git clone git@github.com:obsproject/obs-studio.git`

Then you'd need to point `FindLibObs.cmake` to find the lib and headers, for me (user `roy_shilkrot`) this was:
```
find_path(LIBOBS_INCLUDE_DIR
	NAMES obs.h
	HINTS
		ENV obsPath${_lib_suffix}
		ENV obsPath
		${obsPath}
	PATHS
		/Users/roy_shilkrot/Downloads/obs-studio/libobs/
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES
		libobs
	)
```
and
```
	find_library(${base_name_u}_LIB
		NAMES ${_${base_name_u}_LIBRARIES} ${lib_name} lib${lib_name} ${lib_name}.0
		HINTS
			ENV obsPath${_lib_suffix}
			ENV obsPath
			${obsPath}
			${_${base_name_u}_LIBRARY_DIRS}
		PATHS
			/Applications/OBS.app/Contents/Frameworks
			/usr/lib /usr/local/lib /opt/local/lib /sw/lib
# ...
```

Build:
```
$ mkdir build && cd build
$ cmake ..
$ cmake --build .
```

Add it to your OBS install, e.g.
```
$ cp obs-backgroundremoval.so /Applications/OBS.app/Contents/PlugIns
$ mkdir -p /Applications/OBS.app/Contents/Resources/data/obs-plugins/obs-backgroundremoval/
$ cp ../data/SINet_Softmax.onnx /Applications/OBS.app/Contents/Resources/data/obs-plugins/obs-backgroundremoval/
```

### Linux / Ubuntu

Install dependencies:
```
$ apt install -y libobs-dev libopencv-dev language-pack-en wget git build-essential cmake
$ wget https://github.com/microsoft/onnxruntime/releases/download/v1.7.0/onnxruntime-linux-x64-1.7.0.tgz
$ tar xzvf onnxruntime-linux-x64-1.7.0.tgz --strip-components=1 -C /usr/local/ --wildcards "*/include/*" "*/lib*/"
```

Build and install:
```
$ mkdir build && cd build
$ cmake .. && cmake --build . && cmake --install .
```
