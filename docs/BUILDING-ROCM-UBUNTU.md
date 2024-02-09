# Build with ROCm on Ubuntu


## Prequisites

- Linux Kernel 5 is required. 6 cannot install ROCm.
- You must use Ubuntu 22.04 LTS to follow this instructions.
- 50 GB+ space is required for building ONNX Runtime.

## Install ROCm

https://docs.amd.com/en/docs-5.4.0/deploy/linux/os-native/install.html


```
sudo mkdir --parents --mode=0755 /etc/apt/keyrings
wget https://repo.radeon.com/rocm/rocm.gpg.key -O - | \
    gpg --dearmor | sudo tee /etc/apt/keyrings/rocm.gpg > /dev/null
echo 'deb [arch=amd64 signed-by=/etc/apt/keyrings/rocm.gpg] https://repo.radeon.com/amdgpu/5.4/ubuntu jammy main' \
    | sudo tee /etc/apt/sources.list.d/amdgpu.list
sudo apt update
sudo apt install amdgpu-dkms
sudo reboot
```

```
for ver in 5.3.3 5.4; do
echo "deb [arch=amd64 signed-by=/etc/apt/keyrings/rocm.gpg] https://repo.radeon.com/rocm/apt/$ver jammy main" \
    | sudo tee --append /etc/apt/sources.list.d/rocm.list
done
echo -e 'Package: *\nPin: release o=repo.radeon.com\nPin-Priority: 600' \
    | sudo tee /etc/apt/preferences.d/rocm-pin-600
sudo apt update
sudo apt install rocm-hip-sdk5.4.0 miopen-hip-dev5.4.0 roctracer-dev5.4.0 rocm-dev5.4.0
```

## Build and install ONNX Runtime

```
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
  gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
sudo apt update
sudo apt install kitware-archive-keyring
sudo apt install cmake
```

```
sudo apt install build-essential libstdc++-12-dev
git clone --recursive https://github.com/microsoft/onnxruntime.git -b v1.16.3
cd onnxruntime
./build.sh --config RelWithDebInfo --use_rocm --rocm_home /opt/rocm-5.4.0 --skip_tests --parallel --build_shared_lib
sudo cmake --install build/Linux/RelWithDebInfo
cd ..
```

## Build and install obs-backgroundremoval

```
sudo add-apt-repository ppa:obsproject/obs-studio
sudo apt update
sudo apt install obs-studio qt6-base-dev pkg-config libcurl4-openssl-dev
```

```
git clone --recursive https://github.com/occ-ai/obs-backgroundremoval.git
cd obs-backgroundremoval
cmake . -B build_x86_64 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DENABLE_FRONTEND_API=ON \
  -DENABLE_QT=ON \
  -DCMAKE_COMPILE_WARNING_AS_ERROR=ON \
  -DUSE_SYSTEM_ONNXRUNTIME=ON \
  -DENABLE_ROCM=ON
cmake --build build_x86_64
cmake --install build_x86_64
```