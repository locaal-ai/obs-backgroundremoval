# OBS Plugin: Background Removal

- [Introduction](#introduction)
- [Building](#building)
  - [MacOSX](#mac-osx)
  - [Linux (Ubuntu, Arch)](#linux)
  - [Windows](#windows)

## Download
Check out the [latest releases](https://github.com/royshil/obs-backgroundremoval/releases) for downloads and install instructions.

## Introduction

This plugin is meant to make it easy to replace the background in portrait images and video.
It is using a neural network to predict the mask of the portrait and remove the background pixels.
It's easily composable with other OBS plugins to replace the background with e.g. an image or
a transparent color.

OBS Plugins forum: https://obsproject.com/forum/resources/background-removal-portrait-segmentation.1260/

![](demo.gif)

GPU support:
* Currently on Windows we support DirectML, which should reduce CPU usage by 95% and effectively use the systems accelerators (GPUs if available).
* On Mac we rely on CPU, but we are looking into CoreML for acceleration. Pure CPU inference will bring the CPU usage to ~40%, which may get the machine warm or spin the cooling fans.
* CUDA is not directly supported in this plugin, however it is supported by ONNX Runtime which we use. Perhaps in the future we will add CUDA support.
* The goal of this plugin is to be available for everyone on every system, even if they don't own a GPU.

The models used for background detection are SINet: https://arxiv.org/abs/1911.09099 and MODNet: https://arxiv.org/pdf/2011.11961.pdf
The pre-trained model weights were taken from:
- https://github.com/anilsathyan7/Portrait-Segmentation/tree/master/SINet
- https://github.com/ZHKKKe/MODNet

Some more information about how I built it: https://www.morethantechnical.com/2021/04/15/obs-plugin-for-portrait-background-removal-with-onnx-sinet-model/

## Building

The plugin was built and tested on Mac OSX, Windows and Ubuntu Linux. Help is appreciated in building on other OSs and packages.

The building pipelines in CI take care of the heavy lifting. Use them in order to build the plugin locally. Note that due to the fact we're building and packaging OpenCV and ONNX Runtime the build times are quite long.

Start by cloning this repo to a directory of your choice.

### Mac OSX

Using the CI pipeline scripts, locally you would just call the zsh script.

```sh
$ ./.github/scripts/build-macosx.zsh
```

#### Install
The above script should succeed and the plugin files will reside in the `./release` folder off of the root. Copy the files to the OBS directory e.g. `/Applications/OBS.app/Contents/`.

### Linux

#### Ubuntu

Use the CI scripts again
```sh
$ ./.github/scripts/build-linux.sh
```

#### Archlinux (DEPRECATED: Need to revise this section)
A `PKGBUILD` file is provided for making the plugin package
```sh
$ cd scripts
$ makepkg -s
```

Building for Arch in Docker (host OS e.g. MacOSX):
```sh
$ docker pull archlinux:latest
$ docker run -it -v $(pwd):/src archlinux:latest /bin/bash
# pacman -Sy --needed --noconfirm sudo fakeroot binutils gcc make
# useradd builduser -m
# passwd -d builduser
# printf 'builduser ALL=(ALL) ALL\n' | tee -a /etc/sudoers
# sudo -u builduser bash -c 'cd /src/scripts && makepkg -s'
```

### Windows

Use the CI scripts again, for example:

```powershell
> .github/scripts/Build-Windows.ps1 -Target x64 -CMakeGenerator "Visual Studio 17 2022"
```

The build should exist in the `./release` folder off the root. You can manually install the files in the OBS directory.
