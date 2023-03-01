# Patches for Qt must be at the very least submitted to Qt's Gerrit codereview
# rather than their bug-report Jira. The latter is rarely reviewed by Qt.
class Qt < Formula
	desc "Cross-platform application and UI framework"
	homepage "https://www.qt.io/"
	url "https://download.qt.io/archive/qt/5.10/5.10.1/single/qt-everywhere-src-5.10.1.tar.xz"
	mirror "https://mirrorservice.org/sites/download.qt-project.org/official_releases/qt/5.10/5.10.1/single/qt-everywhere-src-5.10.1.tar.xz"
	sha256 "05ffba7b811b854ed558abf2be2ddbd3bb6ddd0b60ea4b5da75d277ac15e740a"
	head "https://code.qt.io/qt/qt5.git", :branch => "5.10.1", :shallow => false

	bottle do
		sha256 "8b4bad005596a5f8790150fe455db998ac2406f4e0f04140d6656205d844d266" => :high_sierra
		sha256 "9c488554935fb573554a4e36d36d3c81e47245b7fefc4b61edef894e67ba1740" => :sierra
		sha256 "c0407afba5951df6cc4c6f6c1c315972bd41c99cecb4e029919c4c15ab6f7bdc" => :el_capitan
	end

	keg_only "Qt 5 has CMake issues when linked"

	option "with-docs", "Build documentation"
	option "with-examples", "Build examples"

	deprecated_option "with-mysql" => "with-mysql-client"

	# OS X 10.7 Lion is still supported in Qt 5.5, but is no longer a reference
	# configuration and thus untested in practice. Builds on OS X 10.7 have been
	# reported to fail: <https://github.com/Homebrew/homebrew/issues/45284>.
	depends_on :macos => :mountain_lion

	depends_on "pkg-config" => :build
	depends_on :xcode => :build
	depends_on "mysql-client" => :optional
	depends_on "postgresql" => :optional

	# Restore `.pc` files for framework-based build of Qt 5 on OS X. This
	# partially reverts <https://codereview.qt-project.org/#/c/140954/> merged
	# between the 5.5.1 and 5.6.0 releases. (Remove this as soon as feasible!)
	#
	# Core formulae known to fail without this patch (as of 2016-10-15):
	#   * gnuplot  (with `--with-qt` option)
	#   * mkvtoolnix (with `--with-qt` option, silent build failure)
	#   * poppler    (with `--with-qt` option)
	patch do
		url "https://raw.githubusercontent.com/Homebrew/formula-patches/e8fe6567/qt5/restore-pc-files.patch"
		sha256 "48ff18be2f4050de7288bddbae7f47e949512ac4bcd126c2f504be2ac701158b"
	end

	# Fix compile error on macOS 10.13 around QFixed:
	# https://github.com/Homebrew/homebrew-core/issues/27095
	# https://bugreports.qt.io/browse/QTBUG-67545
	patch do
		url "https://raw.githubusercontent.com/z00m1n/formula-patches/0de0e229/qt/QTBUG-67545.patch"
		sha256 "4a115097c7582c7dce4207f5500d13feb8c990eb8a05a43f41953985976ebe6c"
	end

	# Fix compile error on macOS 10.13 caused by qtlocation dependency
	# mapbox-gl-native using Boost 1.62.0 does not build with C++ 17:
	# https://github.com/Homebrew/homebrew-core/issues/27095
	# https://bugreports.qt.io/browse/QTBUG-67810
	patch do
		url "https://raw.githubusercontent.com/z00m1n/formula-patches/a1a1f0dd/qt/QTBUG-67810.patch"
		sha256 "8ee0bf71df1043f08ebae3aa35036be29c4d9ebff8a27e3b0411a6bd635e9382"
	end

	def install
		args = %W[
			-verbose
			-prefix #{prefix}
			-release
			-opensource -confirm-license
			-system-zlib
			-qt-libpng
			-qt-libjpeg
			-qt-freetype
			-qt-pcre
			-nomake tests
			-no-rpath
			-pkg-config
			-dbus-runtime
			-no-assimp
		]

		args << "-nomake" << "examples" if build.without? "examples"

		if build.with? "mysql-client"
			args << "-plugin-sql-mysql"
			(buildpath/"brew_shim/mysql_config").write <<~EOS
				#!/bin/sh
				if [ x"$1" = x"--libs" ]; then
					mysql_config --libs | sed "s/-lssl -lcrypto//"
				else
					exec mysql_config "$@"
				fi
			EOS
			chmod 0755, "brew_shim/mysql_config"
			args << "-mysql_config" << buildpath/"brew_shim/mysql_config"
		end

		args << "-plugin-sql-psql" if build.with? "postgresql"

		system "./configure", *args
		system "make"
		ENV.deparallelize
		system "make", "install"

		if build.with? "docs"
			system "make", "docs"
			system "make", "install_docs"
		end

		# Some config scripts will only find Qt in a "Frameworks" folder
		frameworks.install_symlink Dir["#{lib}/*.framework"]

		# The pkg-config files installed suggest that headers can be found in the
		# `include` directory. Make this so by creating symlinks from `include` to
		# the Frameworks' Headers folders.
		Pathname.glob("#{lib}/*.framework/Headers") do |path|
			include.install_symlink path => path.parent.basename(".framework")
		end

		# Move `*.app` bundles into `libexec` to expose them to `brew linkapps` and
		# because we don't like having them in `bin`.
		# (Note: This move breaks invocation of Assistant via the Help menu
		# of both Designer and Linguist as that relies on Assistant being in `bin`.)
		libexec.mkpath
		Pathname.glob("#{bin}/*.app") { |app| mv app, libexec }
	end

	def caveats; <<~EOS
		We agreed to the Qt opensource license for you.
		If this is unacceptable you should uninstall.
		EOS
	end

	test do
		(testpath/"hello.pro").write <<~EOS
			QT       += core
			QT       -= gui
			TARGET = hello
			CONFIG   += console
			CONFIG   -= app_bundle
			TEMPLATE = app
			SOURCES += main.cpp
		EOS

		(testpath/"main.cpp").write <<~EOS
			#include <QCoreApplication>
			#include <QDebug>

			int main(int argc, char *argv[])
			{
				QCoreApplication a(argc, argv);
				qDebug() << "Hello World!";
				return 0;
			}
		EOS

		system bin/"qmake", testpath/"hello.pro"
		system "make"
		assert_predicate testpath/"hello", :exist?
		assert_predicate testpath/"main.o", :exist?
		system "./hello"
	end
end
