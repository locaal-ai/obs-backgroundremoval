cmake_minimum_required(VERSION 3.16...3.21)

set(CMAKE_CXX_STANDARD 20)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(obs-backgroundremoval VERSION 0.5.18)
add_library(${CMAKE_PROJECT_NAME} MODULE)

set(PLUGIN_AUTHOR "Roy Shilkrot")
set(MACOS_BUNDLEID "com.royshilkrot.obs-backgroundremoval")
set(LINUX_MAINTAINER_EMAIL "roy.shil@gmail.com")

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
  src/plugin-main.cpp
  src/ort-utils/ort-session-utils.cpp
  src/obs-utils/obs-utils.cpp
  src/background-filter.cpp
  src/enhance-filter.cpp
  )

# Import libobs as main plugin dependency
find_package(libobs REQUIRED)
include(cmake/ObsPluginHelpers.cmake)

# Uncomment these lines if you want to use the OBS Frontend API in your plugin
find_package(obs-frontend-api REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)

configure_file(src/plugin-macros.h.in ${CMAKE_SOURCE_DIR}/src/plugin-macros.generated.h)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-macros.generated.h)

set(USE_SYSTEM_ONNXRUNTIME
    OFF
    CACHE STRING "Use system ONNX Runtime")
if(USE_SYSTEM_ONNXRUNTIME)
  if(OS_LINUX)
    find_package(Onnxruntime 1.14.1 REQUIRED)
    set(Onnxruntime_INCLUDE_PATH
        ${Onnxruntime_INCLUDE_DIR} ${Onnxruntime_INCLUDE_DIR}/onnxruntime
        ${Onnxruntime_INCLUDE_DIR}/onnxruntime/core/session
        ${Onnxruntime_INCLUDE_DIR}/onnxruntime/core/providers/cpu)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "${Onnxruntime_LIBRARIES}")
    target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PUBLIC "${Onnxruntime_INCLUDE_PATH}")
  else()
    message(FATAL_ERROR "System ONNX Runtime is only supported on Linux!")
  endif()
else()
  include(cmake/FetchOnnxruntime.cmake)
endif()

set(USE_SYSTEM_OPENCV
    OFF
    CACHE STRING "Use system OpenCV")
if(USE_SYSTEM_OPENCV)
  if(OS_LINUX)
    find_package(OpenCV REQUIRED COMPONENTS core imgproc)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "${OpenCV_LIBRARIES}")
    target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PUBLIC "${OpenCV_INCLUDE_DIRS}")
  else()
    message(FATAL_ERROR "System OpenCV is only supported on Linux!")
  endif()
else()
  include(cmake/BuildMyOpenCV.cmake)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OpenCV)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin|iOS")
  foreach(flags CMAKE_CXX_FLAGS CMAKE_OBJC_FLAGS CMAKE_OBJCXX_FLAGS)
    string(APPEND ${flags} " -fvisibility=hidden -fvisibility-inlines-hidden")
  endforeach()
endif()

# /!\ TAKE NOTE: No need to edit things past this point /!\

# --- Platform-independent build settings ---

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

# --- End of section ---

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
