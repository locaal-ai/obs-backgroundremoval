include(FetchContent)

set(CUSTOM_ONNXRUNTIME_URL
    ""
    CACHE STRING "URL of a downloaded ONNX Runtime tarball")

set(CUSTOM_ONNXRUNTIME_MD5
    ""
    CACHE STRING "MD5 Hash of a downloaded ONNX Runtime tarball")

if(CUSTOM_ONNXRUNTIME_URL STREQUAL "")
  set(USE_PREDEFINED_ONNXRUNTIME ON)
else()
  if(CUSTOM_ONNXRUNTIME_MD5 STREQUAL "")
    message(FATAL_ERROR "Both of CUSTOM_ONNXRUNTIME_URL and CUSTOM_ONNXRUNTIME_MD5 must be present!")
  else()
    set(USE_PREDEFINED_ONNXRUNTIME OFF)
  endif()
endif()

set(Onnxruntime_VERSION "1.16.3")

if(OS_MACOS)
  if(USE_PREDEFINED_ONNXRUNTIME)
    FetchContent_Declare(
      Onnxruntime
      URL "https://github.com/microsoft/onnxruntime/releases/download/v${Onnxruntime_VERSION}/onnxruntime-osx-universal2-${Onnxruntime_VERSION}.tgz"
      URL_HASH MD5=645bc827bf41646e6644de674d945528)
  else()
    FetchContent_Declare(
      Onnxruntime
      URL "${CUSTOM_ONNXRUNTIME_URL}"
      URL_HASH MD5=${CUSTOM_ONNXRUNTIME_MD5})
  endif()
  FetchContent_MakeAvailable(Onnxruntime)
  set(Onnxruntime_LIB "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.${Onnxruntime_VERSION}.dylib")
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "${Onnxruntime_LIB}")
  target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PUBLIC "${onnxruntime_SOURCE_DIR}/include")
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${Onnxruntime_LIB}")
  set_property(SOURCE "${Onnxruntime_LIB}" PROPERTY MACOSX_PACKAGE_LOCATION Frameworks)
  source_group("Frameworks" FILES "${Onnxruntime_LIB}")
  add_custom_command(
    TARGET "${CMAKE_PROJECT_NAME}"
    POST_BUILD
    COMMAND
      ${CMAKE_INSTALL_NAME_TOOL} -change "@rpath/libonnxruntime.${Onnxruntime_VERSION}.dylib"
      "@loader_path/../Frameworks/libonnxruntime.${Onnxruntime_VERSION}.dylib" $<TARGET_FILE:${CMAKE_PROJECT_NAME}>)
elseif(OS_WINDOWS)
  if(USE_PREDEFINED_ONNXRUNTIME)
    FetchContent_Declare(
      Onnxruntime
      URL "https://github.com/occ-ai/onnxruntime-static-win/releases/download/v${Onnxruntime_VERSION}-1/onnxruntime-windows-Release.zip"
      URL_HASH MD5=4e880d7fbf40c18ead4e71f41167e2bf)
  else()
    FetchContent_Declare(
      Onnxruntime
      URL "${CUSTOM_ONNXRUNTIME_URL}"
      URL_HASH MD5=${CUSTOM_ONNXRUNTIME_MD5})
  endif()
  FetchContent_MakeAvailable(Onnxruntime)

  add_library(Ort INTERFACE)
  set(Onnxruntime_LIB_NAMES
      session;providers_shared;providers_dml;optimizer;providers;framework;graph;util;mlas;common;flatbuffers)
  foreach(lib_name IN LISTS Onnxruntime_LIB_NAMES)
    add_library(Ort::${lib_name} STATIC IMPORTED)
    set_target_properties(Ort::${lib_name} PROPERTIES IMPORTED_LOCATION
                                                      ${onnxruntime_SOURCE_DIR}/lib/onnxruntime_${lib_name}.lib)
    set_target_properties(Ort::${lib_name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${onnxruntime_SOURCE_DIR}/include)
    target_link_libraries(Ort INTERFACE Ort::${lib_name})
  endforeach()

  set(Onnxruntime_EXTERNAL_LIB_NAMES
      onnx;onnx_proto;libprotobuf-lite;re2;absl_throw_delegate;absl_hash;absl_city;absl_low_level_hash;absl_raw_hash_set
  )
  foreach(lib_name IN LISTS Onnxruntime_EXTERNAL_LIB_NAMES)
    add_library(Ort::${lib_name} STATIC IMPORTED)
    set_target_properties(Ort::${lib_name} PROPERTIES IMPORTED_LOCATION ${onnxruntime_SOURCE_DIR}/lib/${lib_name}.lib)
    target_link_libraries(Ort INTERFACE Ort::${lib_name})
  endforeach()

  add_library(Ort::DirectML SHARED IMPORTED)
  set_target_properties(Ort::DirectML PROPERTIES IMPORTED_LOCATION ${onnxruntime_SOURCE_DIR}/bin/DirectML.dll)
  set_target_properties(Ort::DirectML PROPERTIES IMPORTED_IMPLIB ${onnxruntime_SOURCE_DIR}/bin/DirectML.lib)

  target_link_libraries(Ort INTERFACE Ort::DirectML d3d12.lib dxgi.lib dxguid.lib)

  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Ort)

  install(IMPORTED_RUNTIME_ARTIFACTS Ort::DirectML DESTINATION "obs-plugins/64bit")
elseif(OS_LINUX)
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    if(USE_PREDEFINED_ONNXRUNTIME)
      FetchContent_Declare(
        Onnxruntime
        URL "https://github.com/microsoft/onnxruntime/releases/download/v${Onnxruntime_VERSION}/onnxruntime-linux-aarch64-${Onnxruntime_VERSION}.tgz"
        URL_HASH MD5=c2112b66c99f9b85d4d71e5c61712034)
    else()
      FetchContent_Declare(
        Onnxruntime
        URL "${CUSTOM_ONNXRUNTIME_URL}"
        URL_HASH MD5=${CUSTOM_ONNXRUNTIME_MD5})
    endif()
    FetchContent_MakeAvailable(Onnxruntime)
    set(Onnxruntime_LINK_LIBS "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so.${Onnxruntime_VERSION}")
    set(Onnxruntime_INSTALL_LIBS ${Onnxruntime_LINK_LIBS})
  else()
    if(USE_PREDEFINED_ONNXRUNTIME)
      FetchContent_Declare(
        Onnxruntime
        URL "https://github.com/microsoft/onnxruntime/releases/download/v${Onnxruntime_VERSION}/onnxruntime-linux-x64-gpu-${Onnxruntime_VERSION}.tgz"
        URL_HASH MD5=21b3eb42e0bee9e7b51cbf508352f188)
    else()
      FetchContent_Declare(
        Onnxruntime
        URL "${CUSTOM_ONNXRUNTIME_URL}"
        URL_HASH MD5=${CUSTOM_ONNXRUNTIME_MD5})
    endif()
    FetchContent_MakeAvailable(Onnxruntime)
    set(Onnxruntime_LINK_LIBS "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so.${Onnxruntime_VERSION}")
    set(Onnxruntime_INSTALL_LIBS
        ${Onnxruntime_LINK_LIBS} "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_shared.so"
        "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_cuda.so"
        "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_tensorrt.so")
  endif()
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${Onnxruntime_LINK_LIBS})
  target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PUBLIC "${onnxruntime_SOURCE_DIR}/include")
  install(FILES ${Onnxruntime_INSTALL_LIBS} DESTINATION "${CMAKE_INSTALL_LIBDIR}/obs-plugins/${CMAKE_PROJECT_NAME}")
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN/${CMAKE_PROJECT_NAME}")
endif()
