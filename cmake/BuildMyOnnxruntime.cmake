include(ExternalProject)

string(REPLACE ";" "$<SEMICOLON>" CMAKE_OSX_ARCHITECTURES_
               "${CMAKE_OSX_ARCHITECTURES}")

if(MSVC)
  set(Onnxruntime_LIB_PREFIX ${CMAKE_BUILD_TYPE})
else()
  set(Onnxruntime_LIB_PREFIX "")
endif()

if(OS_WINDOWS)
  set(PYTHON3 python)
  set(Onnxruntime_PLATFORM_OPTIONS --cmake_generator ${CMAKE_GENERATOR}
                                   --use_dml)
  set(Onnxruntime_PLATFORM_BYPRODUCT
      <INSTALL_DIR>/lib/DirectML.lib <INSTALL_DIR>/lib/DirectML.dll
      <INSTALL_DIR>/lib/DirectML.pdb)
  set(Onnxruntime_PLATFORM_INSTALL_FILES
      <BINARY_DIR>/packages/Microsoft.AI.DirectML.1.9.1/bin/x64-win/DirectML.dll
      <BINARY_DIR>/packages/Microsoft.AI.DirectML.1.9.1/bin/x64-win/DirectML.lib
      <BINARY_DIR>/packages/Microsoft.AI.DirectML.1.9.1/bin/x64-win/DirectML.pdb
  )
  set(Onnxruntime_PROTOBUF_PREFIX lib)
elseif(OS_MACOS)
  set(PYTHON3 python3)
  set(Onnxruntime_PLATFORM_OPTIONS
      --cmake_generator
      Ninja
      --apple_deploy_target
      ${CMAKE_OSX_DEPLOYMENT_TARGET}
      --osx_arch
      ${CMAKE_OSX_ARCHITECTURES}
      --cmake_extra_defines
      CMAKE_C_COMPILER_LAUNCHER=ccache
      --cmake_extra_defines
      CMAKE_CXX_COMPILER_LAUNCHER=ccache)
  set(Onnxruntime_PLATFORM_BYPRODUCT
      <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}nsync_cpp${CMAKE_STATIC_LIBRARY_SUFFIX}
  )
  set(Onnxruntime_PLATFORM_INSTALL_FILES
      <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/nsync/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}nsync_cpp${CMAKE_STATIC_LIBRARY_SUFFIX}
  )
  set(Onnxruntime_PROTOBUF_PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
endif()

ExternalProject_Add(
  Ort
  GIT_REPOSITORY https://github.com/microsoft/onnxruntime.git
  GIT_TAG v1.13.1
  CONFIGURE_COMMAND ""
  BUILD_COMMAND
    ${PYTHON3} <SOURCE_DIR>/tools/ci_build/build.py --build_dir <BINARY_DIR>
    --config ${CMAKE_BUILD_TYPE} --parallel --skip_tests
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
    ${CMAKE_COMMAND} --install <BINARY_DIR>/${CMAKE_BUILD_TYPE} --config
    ${CMAKE_BUILD_TYPE} --prefix <INSTALL_DIR> && ${CMAKE_COMMAND} -E copy
    <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/onnx/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}onnx${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/onnx/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}onnx_proto${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/protobuf/cmake/${Onnxruntime_LIB_PREFIX}/libprotobuf-lite${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/re2/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}re2${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/abseil-cpp/absl/base/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}absl_throw_delegate${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/abseil-cpp/absl/hash/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}absl_hash${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/abseil-cpp/absl/hash/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}absl_city${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/abseil-cpp/absl/hash/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}absl_low_level_hash${CMAKE_STATIC_LIBRARY_SUFFIX}
    <BINARY_DIR>/${CMAKE_BUILD_TYPE}/external/abseil-cpp/absl/container/${Onnxruntime_LIB_PREFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}absl_raw_hash_set${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${Onnxruntime_PLATFORM_INSTALL_FILES} <INSTALL_DIR>/lib)

ExternalProject_Get_Property(Ort INSTALL_DIR)

add_library(Onnxruntime INTERFACE)
add_dependencies(Onnxruntime Ort)
target_include_directories(
  Onnxruntime
  INTERFACE ${INSTALL_DIR}/include
            ${INSTALL_DIR}/include/onnxruntime/core/session
            ${INSTALL_DIR}/include/onnxruntime/core/providers/cpu
            ${INSTALL_DIR}/include/onnxruntime/core/providers/cuda
            ${INSTALL_DIR}/include/onnxruntime/core/providers/dml)
if(OS_MACOS)
  target_link_libraries(Onnxruntime INTERFACE "-framework Foundation")
endif()

if(OS_WINDOWS)
  set(Onnxruntime_LIB_NAMES
      session;framework;mlas;common;graph;providers;optimizer;util;flatbuffers;providers_shared;providers_dml
  )
else()
  set(Onnxruntime_LIB_NAMES
      session;framework;mlas;common;graph;providers;optimizer;util;flatbuffers)
endif()
foreach(lib_name IN LISTS Onnxruntime_LIB_NAMES)
  add_library(Onnxruntime::${lib_name} STATIC IMPORTED)
  set_target_properties(
    Onnxruntime::${lib_name}
    PROPERTIES
      IMPORTED_LOCATION
      ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_${lib_name}${CMAKE_STATIC_LIBRARY_SUFFIX}
  )

  target_link_libraries(Onnxruntime INTERFACE Onnxruntime::${lib_name})
endforeach()

if(OS_WINDOWS)
  set(Onnxruntime_EXTERNAL_LIB_NAMES
      onnx;onnx_proto;libprotobuf-lite;re2;absl_throw_delegate;absl_hash;absl_city;absl_low_level_hash;absl_raw_hash_set
  )
else()
  set(Onnxruntime_EXTERNAL_LIB_NAMES
      onnx;onnx_proto;nsync_cpp;protobuf-lite;re2;absl_throw_delegate;absl_hash;absl_city;absl_low_level_hash;absl_raw_hash_set
  )
endif()
foreach(lib_name IN LISTS Onnxruntime_EXTERNAL_LIB_NAMES)
  add_library(Onnxruntime::${lib_name} STATIC IMPORTED)
  set_target_properties(
    Onnxruntime::${lib_name}
    PROPERTIES
      IMPORTED_LOCATION
      ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${lib_name}${CMAKE_STATIC_LIBRARY_SUFFIX}
  )

  target_link_libraries(Onnxruntime INTERFACE Onnxruntime::${lib_name})
endforeach()

if(OS_WINDOWS)
  add_library(Onnxruntime::DirectML SHARED IMPORTED)
  set_target_properties(
    Onnxruntime::DirectML PROPERTIES IMPORTED_LOCATION
                                     ${INSTALL_DIR}/lib/DirectML.dll)
  set_target_properties(
    Onnxruntime::DirectML PROPERTIES IMPORTED_IMPLIB
                                     ${INSTALL_DIR}/lib/DirectML.lib)
  target_link_libraries(Onnxruntime INTERFACE Onnxruntime::DirectML d3d12.lib
                                              dxgi.lib dxguid.lib)
endif()
