## Instructions for building the plugin on Fedora 37

Make sure you have the following packages installed:

- cmake
- curl-devel
- gcc-c++
- libxkbcommon-devel
- ninja-build
- obs-studio-devel
- opencv-devel
- qt6-qtbase-devel

_As an unprivileged user:_  
Clone the repository. Don't forget to checkout the submodules:
```
git submodule init
git submodule update
```
Before you compile, edit `cmake/linux/compilerconfig.cmake`, find this line: `set(_obs_gcc_c_options` and add the following line to the list of options:
```
    -fPIC
```
Then use the following command to compile: `.github/scripts/build-linux --skip-deps`  

_As a privileged user (root):_  
Finally, install the necessary files into the system directories, by issuing this command: `cmake --install build_x86_64 --prefix /usr`
