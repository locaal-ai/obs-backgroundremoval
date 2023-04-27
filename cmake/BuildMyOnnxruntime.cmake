include(ExternalProject)

string(REPLACE ";" "$<SEMICOLON>" CMAKE_OSX_ARCHITECTURES_ "${CMAKE_OSX_ARCHITECTURES}")

if(${CMAKE_BUILD_TYPE} STREQUAL Release OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
  set(Onnxruntime_BUILD_TYPE Release)
else()
  set(Onnxruntime_BUILD_TYPE Debug)
endif()

if(MSVC)
  set(Onnxruntime_LIB_PREFIX ${Onnxruntime_BUILD_TYPE})
else()
  set(Onnxruntime_LIB_PREFIX "")
endif()

find_program(Onnxruntime_CCACHE_EXE ccache)

if(OS_WINDOWS)
  set(PYTHON3 python)
  set(Onnxruntime_PLATFORM_OPTIONS --cmake_generator ${CMAKE_GENERATOR} --use_dml)
  if(Onnxruntime_CCACHE_EXE)
    set(Onnxruntime_PLATFORM_CONFIGURE ${CMAKE_COMMAND} -E copy ${Onnxruntime_CCACHE_EXE}
                                       ${CMAKE_BINARY_DIR}/cl.exe)
    list(
      APPEND
      Onnxruntime_PLATFORM_OPTIONS
      --cmake_extra_defines
      CMAKE_VS_GLOBALS=CLToolExe=cl.exe$<SEMICOLON>CLToolPath=${CMAKE_BINARY_DIR}$<SEMICOLON>TrackFileAccess=false$<SEMICOLON>UseMultiToolTask=true$<SEMICOLON>DebugInformationFormat=OldStyle
    )
  endif()
  set(Onnxruntime_PLATFORM_BYPRODUCT <INSTALL_DIR>/lib/DirectML.lib <INSTALL_DIR>/lib/DirectML.dll
                                     <INSTALL_DIR>/lib/DirectML.pdb)
  set(Onnxruntime_PLATFORM_INSTALL_FILES
      <BINARY_DIR>/packages/Microsoft.AI.DirectML.1.10.1/bin/x64-win/DirectML.dll
      <BINARY_DIR>/packages/Microsoft.AI.DirectML.1.10.1/bin/x64-win/DirectML.lib
      <BINARY_DIR>/packages/Microsoft.AI.DirectML.1.10.1/bin/x64-win/DirectML.pdb)
  set(Onnxruntime_PROTOBUF_PREFIX lib)
elseif(OS_MACOS)
  set(PYTHON3 python3)
  set(Onnxruntime_PLATFORM_CONFIGURE "")
  set(Onnxruntime_PLATFORM_OPTIONS
      --cmake_generator
      Ninja
      --apple_deploy_target
      ${CMAKE_OSX_DEPLOYMENT_TARGET}
      --osx_arch
      ${CMAKE_OSX_ARCHITECTURES}
      --use_coreml)
  if(Onnxruntime_CCACHE_EXE)
    list(APPEND Onnxruntime_PLATFORM_OPTIONS --cmake_extra_defines
         CMAKE_C_COMPILER_LAUNCHER=${Onnxruntime_CCACHE_EXE} --cmake_extra_defines
         CMAKE_CXX_COMPILER_LAUNCHER=${Onnxruntime_CCACHE_EXE})
  endif()
  set(Onnxruntime_PLATFORM_BYPRODUCT
      <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_providers_coreml${CMAKE_STATIC_LIBRARY_SUFFIX}
      <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_coreml_proto${CMAKE_STATIC_LIBRARY_SUFFIX}
      <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}nsync_cpp${CMAKE_STATIC_LIBRARY_SUFFIX}
      <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}cpuinfo${CMAKE_STATIC_LIBRARY_SUFFIX}
      <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}clog${CMAKE_STATIC_LIBRARY_SUFFIX})
  set(Onnxruntime_PLATFORM_INSTALL_FILES
      <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/google_nsync-build/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}nsync_cpp${CMAKE_STATIC_LIBRARY_SUFFIX}
      <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/pytorch_cpuinfo-build/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}cpuinfo${CMAKE_STATIC_LIBRARY_SUFFIX}
      <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/pytorch_cpuinfo-build/${Onnxruntime_LIB_PREFIX}/deps/clog/${CMAKE_STATIC_LIBRARY_PREFIX}clog${CMAKE_STATIC_LIBRARY_SUFFIX}
  )
  set(Onnxruntime_PROTOBUF_PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
else()
  set(PYTHON3 python3)
  set(Onnxruntime_PLATFORM_CONFIGURE "")
  set(Onnxruntime_PLATFORM_OPTIONS --cmake_generator Ninja)

  if(Onnxruntime_CCACHE_EXE)
    list(APPEND Onnxruntime_PLATFORM_OPTIONS --cmake_extra_defines
         CMAKE_C_COMPILER_LAUNCHER=${Onnxruntime_CCACHE_EXE} --cmake_extra_defines
         CMAKE_CXX_COMPILER_LAUNCHER=${Onnxruntime_CCACHE_EXE})
  endif()
  set(Onnxruntime_PLATFORM_BYPRODUCT
      <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}nsync_cpp${CMAKE_STATIC_LIBRARY_SUFFIX}
      <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}cpuinfo${CMAKE_STATIC_LIBRARY_SUFFIX}
      <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}clog${CMAKE_STATIC_LIBRARY_SUFFIX})
  set(Onnxruntime_PLATFORM_INSTALL_FILES
      <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/google_nsync-build/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}nsync_cpp${CMAKE_STATIC_LIBRARY_SUFFIX}
      <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/pytorch_cpuinfo-build/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}cpuinfo${CMAKE_STATIC_LIBRARY_SUFFIX}
      <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/pytorch_cpuinfo-build/${Onnxruntime_LIB_PREFIX}/deps/clog/${CMAKE_STATIC_LIBRARY_PREFIX}clog${CMAKE_STATIC_LIBRARY_SUFFIX}
  )
  set(Onnxruntime_PROTOBUF_PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
endif()

ExternalProject_Add(
  Ort
  GIT_REPOSITORY https://github.com/microsoft/onnxruntime.git
  GIT_TAG v1.14.1
  GIT_SHALLOW ON
  CONFIGURE_COMMAND "${Onnxruntime_PLATFORM_CONFIGURE}"
  BUILD_COMMAND
    ${PYTHON3} <SOURCE_DIR>/tools/ci_build/build.py --build_dir <BINARY_DIR> --config
    ${Onnxruntime_BUILD_TYPE} --parallel --skip_tests --skip_submodule_sync
    ${Onnxruntime_PLATFORM_OPTIONS}
  BUILD_BYPRODUCTS
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_session${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_framework${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_mlas${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_common${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_graph${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_providers${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_optimizer${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_util${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_flatbuffers${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnx${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnx_proto${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${Onnxruntime_PROTOBUF_PREFIX}protobuf-lite${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}re2${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}absl_throw_delegate${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}absl_hash${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}absl_city${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}absl_low_level_hash${CMAKE_STATIC_LIBRARY_SUFFIX}
    <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}absl_raw_hash_set${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${Onnxruntime_PLATFORM_BYPRODUCT}
  INSTALL_COMMAND
    ${CMAKE_COMMAND} --install <BINARY_DIR>/${Onnxruntime_BUILD_TYPE} --config
    ${Onnxruntime_BUILD_TYPE} --prefix <INSTALL_DIR> && ${CMAKE_COMMAND} -E copy
    <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/onnx-build/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}onnx${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/onnx-build/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}onnx_proto${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/protobuf-build/${Onnxruntime_LIB_PREFIX}/libprotobuf-lite${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/re2-build/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}re2${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/abseil_cpp-build/absl/base/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}absl_throw_delegate${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/abseil_cpp-build/absl/hash/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}absl_hash${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/abseil_cpp-build/absl/hash/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}absl_city${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/abseil_cpp-build/absl/hash/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}absl_low_level_hash${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${Onnxruntime_BUILD_TYPE}/_deps/abseil_cpp-build/absl/container/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}absl_raw_hash_set${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${Onnxruntime_PLATFORM_INSTALL_FILES} <INSTALL_DIR>/lib && ${CMAKE_COMMAND} -E copy_directory
    <SOURCE_DIR>/include/onnxruntime/core/providers/coreml
    <INSTALL_DIR>/include/onnxruntime/core/providers/coreml)

ExternalProject_Get_Property(Ort INSTALL_DIR)

add_library(Onnxruntime INTERFACE)
add_dependencies(Onnxruntime Ort)
set(Onnxruntime_INCLUDE_PATH
    ${INSTALL_DIR}/include
    ${INSTALL_DIR}/include/onnxruntime
    ${INSTALL_DIR}/include/onnxruntime/core/session
    ${INSTALL_DIR}/include/onnxruntime/core/providers/cpu
    ${INSTALL_DIR}/include/onnxruntime/core/providers/cuda
    ${INSTALL_DIR}/include/onnxruntime/core/providers/coreml
    ${INSTALL_DIR}/include/onnxruntime/core/providers/dml)
if(OS_MACOS)
  target_link_libraries(Onnxruntime INTERFACE "-framework Foundation")
endif()

if(OS_WINDOWS)
  add_library(Onnxruntime::DirectML SHARED IMPORTED)
  set_target_properties(Onnxruntime::DirectML PROPERTIES IMPORTED_LOCATION
                                                         ${INSTALL_DIR}/lib/DirectML.dll)
  set_target_properties(Onnxruntime::DirectML PROPERTIES IMPORTED_IMPLIB
                                                         ${INSTALL_DIR}/lib/DirectML.lib)
  target_link_libraries(Onnxruntime INTERFACE Onnxruntime::DirectML d3d12.lib dxgi.lib dxguid.lib)
endif()

if(OS_WINDOWS)
  set(Onnxruntime_LIB_NAMES
      session;providers_shared;providers_dml;optimizer;providers;framework;graph;util;mlas;common;flatbuffers
  )
endif()
if(OS_MACOS)
  set(Onnxruntime_LIB_NAMES
      session;providers_coreml;coreml_proto;optimizer;providers;framework;graph;util;mlas;common;flatbuffers
  )
endif()
if(OS_LINUX)
  set(Onnxruntime_LIB_NAMES
      session;optimizer;providers;framework;graph;util;mlas;common;flatbuffers)
endif()

foreach(lib_name IN LISTS Onnxruntime_LIB_NAMES)
  add_library(Onnxruntime::${lib_name} STATIC IMPORTED)
  set_target_properties(
    Onnxruntime::${lib_name}
    PROPERTIES
      IMPORTED_LOCATION
      ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_${lib_name}${CMAKE_STATIC_LIBRARY_SUFFIX}
  )
  set_target_properties(Onnxruntime::${lib_name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                                            "${Onnxruntime_INCLUDE_PATH}")

  target_link_libraries(Onnxruntime INTERFACE Onnxruntime::${lib_name})
endforeach()

if(OS_WINDOWS)
  set(Onnxruntime_EXTERNAL_LIB_NAMES
      onnx;onnx_proto;libprotobuf-lite;re2;absl_throw_delegate;absl_hash;absl_city;absl_low_level_hash;absl_raw_hash_set
  )
else()
  set(Onnxruntime_EXTERNAL_LIB_NAMES
      onnx;onnx_proto;nsync_cpp;protobuf-lite;re2;absl_throw_delegate;absl_hash;absl_city;absl_low_level_hash;absl_raw_hash_set;cpuinfo;clog
  )
endif()
foreach(lib_name IN LISTS Onnxruntime_EXTERNAL_LIB_NAMES)
  add_library(Onnxruntime::${lib_name} STATIC IMPORTED)
  set_target_properties(
    Onnxruntime::${lib_name}
    PROPERTIES
      IMPORTED_LOCATION
      ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${lib_name}${CMAKE_STATIC_LIBRARY_SUFFIX})

  target_link_libraries(Onnxruntime INTERFACE Onnxruntime::${lib_name})
endforeach()
