pkgname=obs-backgroundremoval
pkgver=0.2.5_beta
_ONNXRUNTIME_FILE=onnxruntime-linux-x64-1.7.0.tgz
_onnxdir=onnxruntime-linux-x64-1.7.0
_bgdir=$pkgname-${pkgver//_/-}
pkgrel=1
arch=(x86_64)
url='https://github.com/royshil/obs-backgroundremoval'
pkgdesc="Background removal plugin for OBS studio (precomopiled onnxruntime)"
license=(MIT custom)
depends=(obs-studio opencv)
makedepends=(cmake)
source=("${_bgdir}.tar.gz::https://github.com/royshil/$pkgname/archive/refs/tags/v${pkgver//_/-}.tar.gz"
        "https://github.com/microsoft/onnxruntime/releases/download/v1.7.0/$_ONNXRUNTIME_FILE")
sha256sums=('c22280bfa5341e371c4d095717063fe04b15a2f529a531205e8df860d70aacb0'
            '0345f45f222208344406d79a6db3280ed2ccc884dc1e064ce6e6951ed4c70606')

prepare() {
  # build from archive, not git. Version set during build()
  sed -i "s/^version_from_git/#&/" $_bgdir/CMakeLists.txt
}

build() {
  cd "$_bgdir"
  # set rpath to avoid installing onnxruntime globally
  cmake -B build -DVERSION="${pkgver//_*/.0}" -DobsIncludePath=/usr/include/obs/ \
           -DCMAKE_INSTALL_PREFIX:PATH=/usr \
           -DOnnxruntime_INCLUDE_DIRS="$srcdir/$_onnxdir/include/" -DOnnxruntime_LIBRARIES="$srcdir/$_onnxdir/lib/libonnxruntime.so" \
           -DOnnxruntime_INCLUDE_DIR="$srcdir/$_onnxdir/include/" \
           -DCMAKE_INSTALL_RPATH="/usr/lib/obs-backgroundremoval"
  cd build
  make
}

package() {
  make -C "$_bgdir/build" DESTDIR="$pkgdir" install
  install -Dt "$pkgdir/usr/share/licenses/obs-backgroundremoval" "$_bgdir/LICENSE"

  # install onnxruntime
  cd "$_onnxdir"
  install -Dt "$pkgdir/usr/share/licenses/obs-backgroundremoval/onnxruntime" \
          LICENSE  Privacy.md  README.md  ThirdPartyNotices.txt
  install -Dt "$pkgdir/usr/lib/obs-backgroundremoval" lib/*
}
