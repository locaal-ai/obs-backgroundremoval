cmake_minimum_required(VERSION 3.16)

# Change obs-plugintemplate to your plugin's name in a machine-readable format (e.g.:
# obs-myawesomeplugin) and set
project(obs-backgroundremoval VERSION 0.5.0)
add_library(${CMAKE_PROJECT_NAME} MODULE)

# Replace `Your Name Here` with the name (yours or your organization's) you want to see as the
# author of the plugin (in the plugin's metadata itself and in the installers)
set(PLUGIN_AUTHOR "Roy Shilkrot")

# Replace `com.example.obs-plugin-template` with a unique Bundle ID for macOS releases (used both in
# the installer and when submitting the installer for notarization)
set(MACOS_BUNDLEID "com.royshilkrot.obs-backgroundremoval")

# Replace `me@contoso.com` with the maintainer email address you want to put in Linux packages
set(LINUX_MAINTAINER_EMAIL "roy.shil@gmail.com")

option(WITH_CUDA "Build with CUDA support." OFF)
if(WITH_CUDA)
  add_compile_definitions("WITH_CUDA=1")
endif()

if(WIN32 OR MSVC)
  set(VCPKG_DIR ${CMAKE_BINARY_DIR}/vcpkg)
  set(VCPKG_INSTALLED_DIR ${VCPKG_DIR}/installed/x64-windows-static)
  set(CMAKE_TOOLCHAIN_FILE ${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake)

  set(OpenCV_DIR ${VCPKG_INSTALLED_DIR}/share/opencv)
  set(Protobuf_DIR ${VCPKG_INSTALLED_DIR}/share/protobuf)
  set(LibLZMA_DIR ${VCPKG_INSTALLED_DIR}/share/liblzma)
  set(quirc_DIR ${VCPKG_INSTALLED_DIR}/share/quirc)

  if(NOT ${libobs_DIR})
    message(FATAL "Please provide libobs_DIR to CMake.")
  endif(NOT ${libobs_DIR})

  set(w32-pthreads_DIR ${libobs_DIR}/../deps/w32-pthreads)
  set(obs-frontend-api_DIR ${libobs_DIR}/../UI/obs-frontend-api)

  # Find lib TIFF
  find_library(TIFF_LIBRARY tiff PATHS ${VCPKG_INSTALLED_DIR}/lib)
  find_path(TIFF_INCLUDE_DIR tiff.h PATHS ${VCPKG_INSTALLED_DIR}/include)

  find_file(Protobuf_PROTOC_EXECUTABLE protoc.exe PATHS ${VCPKG_INSTALLED_DIR}/tools/protobuf)
elseif(APPLE)
  # Apple
  if(WITH_CUDA)
    message(FATAL "Cannot use CUDA on Apple builds")
  endif()
endif()

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake")

# In case you need C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PLUGIN_SOURCES ${CMAKE_SOURCE_DIR}/src/background-filter.cpp
                   ${CMAKE_SOURCE_DIR}/src/plugin-main.cpp)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${PLUGIN_SOURCES})

configure_file(src/plugin-macros.h.in ${CMAKE_SOURCE_DIR}/src/plugin-macros.generated.h)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-macros.generated.h)

if(UNIX)
  set(CMAKE_MODULE_PATH
      ${CMAKE_MODULE_PATH}
      "${CMAKE_SOURCE_DIR}/obs-build-dependencies/plugin-deps-2023-01-06-qt6-x86_64/lib/cmake/libobs/"
  )
endif()

# Import libobs as main plugin dependency
find_package(libobs REQUIRED)

include(cmake/ObsPluginHelpers.cmake)

find_package(Onnxruntime REQUIRED)

find_package(OpenCV 4.5 REQUIRED COMPONENTS core imgproc)

target_include_directories(
  ${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src ${Onnxruntime_INCLUDE_DIRS}
                                ${OpenCV_INCLUDE_DIRS})

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs ${Onnxruntime_LIBRARIES}
                                                    ${OpenCV_LIBRARIES})

# --- Windows-specific build settings and tasks ---
if(OS_WINDOWS)
  configure_file(cmake/bundle/windows/installer-Windows.iss.in
                 ${CMAKE_BINARY_DIR}/installer-Windows.generated.iss)

  configure_file(cmake/bundle/windows/resource.rc.in ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.rc)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.rc)

  if(MSVC)
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W4)
  endif()
  # --- End of section ---

  # -- macOS specific build settings and tasks --
elseif(OS_MACOS)
  configure_file(cmake/bundle/macos/installer-macos.pkgproj.in
                 ${CMAKE_BINARY_DIR}/installer-macos.generated.pkgproj)

  set(MACOSX_PLUGIN_GUI_IDENTIFIER "${MACOS_BUNDLEID}")
  set(MACOSX_PLUGIN_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION}")
  set(MACOSX_PLUGIN_SHORT_VERSION_STRING "1")

  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall)
  # --- End of section ---

  # --- Linux-specific build settings and tasks ---
else()
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall)
endif()
# --- End of section ---

setup_plugin_target(${CMAKE_PROJECT_NAME})

# # --- End of section ---
