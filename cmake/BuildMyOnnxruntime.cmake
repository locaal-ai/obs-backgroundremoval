include(ExternalProject)

if(MSVC)
  if(${CMAKE_GENERATOR_PLATFORM} STREQUAL x64
     AND ${MSVC_VERSION} GREATER_EQUAL 1910
     AND ${MSVC_VERSION} LESS_EQUAL 1939)
    set(Onnxruntime_LIB_PATH x64/vc17/staticlib)
    set(Onnxruntime_LIB_PATH_3RD x64/vc17/staticlib)
    set(Onnxruntime_LIB_SUFFIX 470)
  else()
    message(FATAL_ERROR "Unsupported MSVC!")
  endif()
else()
  set(Onnxruntime_LIB_PATH lib)
  set(Onnxruntime_LIB_PATH_3RD lib/opencv4/3rdparty)
  set(Onnxruntime_LIB_SUFFIX "")
endif()

ExternalProject_Add(Onnxruntime_Build
  GIT_REPOSITORY https://github.com/microsoft/onnxruntime.git
  GIT_TAG v1.13.1
  CONFIGURE_COMMAND ""
  BUILD_COMMAND
    python3 <SOURCE_DIR>/tools/ci_build/build.py
    --build_dir <BINARY_DIR>
    --config ${CMAKE_BUILD_TYPE}
    --parallel
    --skip_tests
  BUILD_BYPRODUCTS
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnx${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnx_proto${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}protobuf${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}re2${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}absl_throw_delegate${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}absl_hash${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}absl_city${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}absl_low_level_hash${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}absl_raw_hash_set${CMAKE_STATIC_LIBRARY_SUFFIX}
  INSTALL_COMMAND
    cmake --install <BINARY_DIR>/${CMAKE_BUILD_TYPE} --config ${CMAKE_BUILD_TYPE} --prefix <INSTALL_DIR> &&
    cp <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/onnx/${CMAKE_STATIC_LIBRARY_PREFIX}onnx${CMAKE_STATIC_LIBRARY_SUFFIX} <INSTALL_DIR>/lib &&
    cp <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/onnx/${CMAKE_STATIC_LIBRARY_PREFIX}onnx_proto${CMAKE_STATIC_LIBRARY_SUFFIX} <INSTALL_DIR>/lib &&
    cp <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/protobuf/cmake/${CMAKE_STATIC_LIBRARY_PREFIX}protobuf${CMAKE_STATIC_LIBRARY_SUFFIX} <INSTALL_DIR>/lib &&
    cp <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/re2/${CMAKE_STATIC_LIBRARY_PREFIX}re2${CMAKE_STATIC_LIBRARY_SUFFIX} <INSTALL_DIR>/lib &&
    cp <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/abseil-cpp/absl/base/${CMAKE_STATIC_LIBRARY_PREFIX}absl_throw_delegate${CMAKE_STATIC_LIBRARY_SUFFIX} <INSTALL_DIR>/lib &&
    cp <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/abseil-cpp/absl/hash/${CMAKE_STATIC_LIBRARY_PREFIX}absl_hash${CMAKE_STATIC_LIBRARY_SUFFIX} <INSTALL_DIR>/lib &&
    cp <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/abseil-cpp/absl/hash/${CMAKE_STATIC_LIBRARY_PREFIX}absl_city${CMAKE_STATIC_LIBRARY_SUFFIX} <INSTALL_DIR>/lib &&
    cp <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/abseil-cpp/absl/hash/${CMAKE_STATIC_LIBRARY_PREFIX}absl_low_level_hash${CMAKE_STATIC_LIBRARY_SUFFIX} <INSTALL_DIR>/lib &&
    cp <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/abseil-cpp/absl/container/${CMAKE_STATIC_LIBRARY_PREFIX}absl_raw_hash_set${CMAKE_STATIC_LIBRARY_SUFFIX} <INSTALL_DIR>/lib &&
    :
)

ExternalProject_Get_Property(Onnxruntime_Build INSTALL_DIR)

add_library(Onnxruntime INTERFACE)
add_dependencies(Onnxruntime Onnxruntime_Build)
target_include_directories(Onnxruntime INTERFACE ${INSTALL_DIR}/include ${INSTALL_DIR}/include/onnxruntime/core/session)
if(OS_MACOS)
  target_link_libraries(Onnxruntime INTERFACE "-framework Foundation")
endif()

set(Onnxruntime_EXT_LIB_NAMES session;framework;mlas;common;graph;providers;optimizer;util;flatbuffers)
foreach(lib_name IN LISTS Onnxruntime_EXT_LIB_NAMES)
  add_library(Onnxruntime::${lib_name} STATIC IMPORTED)
  set_target_properties(
    Onnxruntime::${lib_name}
    PROPERTIES
      IMPORTED_LOCATION
      ${INSTALL_DIR}/${Onnxruntime_LIB_PATH}/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_${lib_name}${CMAKE_STATIC_LIBRARY_SUFFIX}
  )

  target_link_libraries(Onnxruntime INTERFACE Onnxruntime::${lib_name})
endforeach()

set(Onnxruntime_EXTERNAL_LIB_NAMES onnx;onnx_proto;nsync_cpp;protobuf;re2;absl_throw_delegate;absl_hash;absl_city;absl_low_level_hash;absl_raw_hash_set)
foreach(lib_name IN LISTS Onnxruntime_EXTERNAL_LIB_NAMES)
  add_library(Onnxruntime::${lib_name} STATIC IMPORTED)
  set_target_properties(
    Onnxruntime::${lib_name}
    PROPERTIES
      IMPORTED_LOCATION
      ${INSTALL_DIR}/${Onnxruntime_LIB_PATH}/${CMAKE_STATIC_LIBRARY_PREFIX}${lib_name}${CMAKE_STATIC_LIBRARY_SUFFIX}
  )

  target_link_libraries(Onnxruntime INTERFACE Onnxruntime::${lib_name})
endforeach()