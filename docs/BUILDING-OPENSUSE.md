## Building on openSUSE

NOTE: These instructions are unofficial and we will appreciate any pull requests to improve them.

Please follow the steps to install obs-backgroundremoval plugin on OpenSUSE.

### Install OBS

https://obsproject.com/wiki/unofficial-linux-builds#opensuse

```
sudo zypper ar -cfp 90 http://ftp.gwdg.de/pub/linux/misc/packman/suse/openSUSE_Tumbleweed/ packman
sudo zypper dup --from packman --allow-vendor-change
sudo zypper in obs-studio
```

### Building this plugin

```
sudo zypper install -t pattern devel_basis
sudo zypper install zsh cmake Mesa-libGL-devel \
  ffmpeg-6-libavcodec-devel ffmpeg-6-libavdevice-devel ffmpeg-6-libavformat-devel \
  libcurl-devel Mesa-libEGL-devel \
  libpulse-devel libxkbcommon-devel
sudo zypper in cmake gcc-c++ ninja obs-studio-devel opencv-devel qt6-base-devel zsh curl-devel jq

.github/scripts/build-linux --skip-deps
sudo cmake --install build_x86_64 --prefix /usr
```

