## Building on openSUSE

NOTE: This instruction is unofficial and we will appreciate any pull requests for this instructions.

### Installing OBS

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
  ffmpeg-6-libavcodes-devel ffmpeg-6-libavdevice-devel ffmpeg-6-libavformat-devel \
  libcurl-devel Mesa-libEGL-devel \
  libpulse-devel libxkbcommon-devel
</dev/null >.github/scripts/utils.zsh/check_linux
CI=1 .github/scripts/build-linux.zsh --skip-deps
sudo cmake --install build_x86_64 --prefix /usr
```

