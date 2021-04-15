# OBS Plugin: Background Removal

## Introduction

This plugin is meant to make it easy to replace the background in portrait images and video.
It is using a neural network to predict the mask of the portrait and remove the background pixels.
It's easily composable with other OBS plugins to replace the background with e.g. an image or
a transparent color.

![](demo.gif)

The model used for background detection is SINet: https://arxiv.org/abs/1911.09099
The pre-trained model weights were taken from: https://github.com/anilsathyan7/Portrait-Segmentation/tree/master/SINet

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

Build:
```
$ mkdir build && cd build
$ cmake ..
$ cmake --build ..
```

Add it to your OBS install, e.g.
```
$ cp obs-backgroundremoval.so /Applications/OBS.app/Contents/PlugIns
$ cp ../data/SINet_Softmax.onnx /Applications/OBS.app/Contents/Resources/data/obs-plugins/obs-backgroundremoval/
```
