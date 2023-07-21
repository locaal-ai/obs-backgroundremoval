## Instructions for building the plugin on Fedora 37

First, you have to install the development tools:
```
sudo dnf groupinstall "Development Tools"
```

Then, make sure you have the dependencies of this plugin installed:

```
sudo dnf install cmake gcc-c++ libxkbcommon-devel ninja-build obs-studio-devel opencv-devel qt6-qtbase-devel
```

Clone the repository and set up the submodules:
```
git clone https://github.com/royshil/obs-backgroundremoval.git
cd obs-backgroundremoval
git submodule update --init
```

Before you compile, edit `cmake/linux/compilerconfig.cmake`, find this line: `set(_obs_gcc_c_options` and add the following line to the list of options:
```
    -fPIC
```
Then use the following command to compile: 
```
.github/scripts/build-linux --skip-deps
```

Finally, install the necessary files into the system directories, by issuing this command:
```
sudo cmake --install build_x86_64 --prefix /usr
```
