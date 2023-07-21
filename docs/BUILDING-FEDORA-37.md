## Instructions for building the plugin on Fedora 37

Make sure you have the following packages installed:

- clang
- cmake
- obs-studio-devel
- opencv-devel
- qt6-qtbase-devel

_As an unprivileged user:_  
Clone the repository, then use the following command to compile: `.github/scripts/build-linux --skip-deps`  

_As a privileged user (root):_  
You then need to copy the plugin binary to `/usr/lib64/obs-plugins`:
```
# cp build_x86_64/obs-backgroundremoval.so /usr/lib64/obs-plugins/
```

And copy the contents of the [data](../data) directory to `/usr/share/obs/obs-plugins/obs-backgroundremoval`:
```
# cp -r data /usr/share/obs/obs-plugins/obs-backgroundremoval
```
