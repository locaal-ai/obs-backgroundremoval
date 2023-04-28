include(FetchContent)

set(Onnxruntime_VERSION "1.14.1")
set(Onnxruntime_DirectML_VERSION "1.10.1")
if(OS_MACOS)
  FetchContent_Declare(
    Onnxruntime
    URL "https://github.com/microsoft/onnxruntime/releases/download/v${Onnxruntime_VERSION}/onnxruntime-osx-universal2-${Onnxruntime_VERSION}.tgz"
    URL_HASH MD5=9725836c49deb09fc352a57dc8a1b806)
  FetchContent_MakeAvailable(Onnxruntime)
  set(Onnxruntime_LIB "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.${Onnxruntime_VERSION}.dylib")
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "${Onnxruntime_LIB}")
  target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM
                             PUBLIC "${onnxruntime_SOURCE_DIR}/include")
  install(FILES "${Onnxruntime_LIB}" DESTINATION "${CMAKE_PROJECT_NAME}.plugin/Contents/Frameworks")
  add_custom_command(
    TARGET "${CMAKE_PROJECT_NAME}"
    POST_BUILD
    COMMAND
      ${CMAKE_INSTALL_NAME_TOOL} -change "@rpath/libonnxruntime.${Onnxruntime_VERSION}.dylib"
      "@loader_path/../Frameworks/libonnxruntime.${Onnxruntime_VERSION}.dylib"
      $<TARGET_FILE:${CMAKE_PROJECT_NAME}>)
elseif(OS_WINDOWS)
  FetchContent_Declare(
    Onnxruntime
    URL "https://github.com/umireon/onnxruntime-static-win/releases/download/v${Onnxruntime_VERSION}-4/onnxruntime-static-win.zip"
    URL_HASH MD5=a8ae8f5707b651347a5bb8a1fac159bf)
  FetchContent_MakeAvailable(Onnxruntime)
  set(DirectML_LIB "${directml_SOURCE_DIR}/bin/DirectML.dll")

  add_library(Ort INTERFACE)
  set(Onnxruntime_LIB_NAMES
      session;providers_shared;providers_dml;optimizer;providers;framework;graph;util;mlas;common;flatbuffers
  )
  foreach(lib_name IN LISTS Onnxruntime_LIB_NAMES)
    add_library(Ort::${lib_name} STATIC IMPORTED)
    set_target_properties(
      Ort::${lib_name} PROPERTIES IMPORTED_LOCATION
                                  ${onnxruntime_SOURCE_DIR}/lib/onnxruntime_${lib_name}.lib)
    set_target_properties(Ort::${lib_name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                                      ${onnxruntime_SOURCE_DIR}/include)
    target_link_libraries(Ort INTERFACE Ort::${lib_name})
  endforeach()

  set(Onnxruntime_EXTERNAL_LIB_NAMES
      onnx;onnx_proto;libprotobuf-lite;re2;absl_throw_delegate;absl_hash;absl_city;absl_low_level_hash;absl_raw_hash_set
  )
  foreach(lib_name IN LISTS Onnxruntime_EXTERNAL_LIB_NAMES)
    add_library(Ort::${lib_name} STATIC IMPORTED)
    set_target_properties(Ort::${lib_name} PROPERTIES IMPORTED_LOCATION
                                                      ${onnxruntime_SOURCE_DIR}/lib/${lib_name}.lib)
    target_link_libraries(Ort INTERFACE Ort::${lib_name})
  endforeach()

  add_library(Ort::DirectML SHARED IMPORTED)
  set_target_properties(Ort::DirectML PROPERTIES IMPORTED_LOCATION
                                                 ${onnxruntime_SOURCE_DIR}/lib/DirectML.dll)
  set_target_properties(Ort::DirectML PROPERTIES IMPORTED_IMPLIB
                                                 ${onnxruntime_SOURCE_DIR}/lib/DirectML.lib)

  target_link_libraries(Ort INTERFACE Ort::DirectML d3d12.lib dxgi.lib dxguid.lib)

  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Ort)

  install(FILES "${DirectML_LIB}" DESTINATION "${OBS_PLUGIN_DESTINATION}")
elseif(OS_LINUX)
  FetchContent_Declare(
    Onnxruntime
    URL "https://github.com/microsoft/onnxruntime/releases/download/v${Onnxruntime_VERSION}/onnxruntime-linux-x64-gpu-${Onnxruntime_VERSION}.tgz"
    URL_HASH MD5=6a3866eb7dce86a17922c0662623f77e)
  FetchContent_MakeAvailable(Onnxruntime)
  set(Onnxruntime_LINK_LIBS
      "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so.${Onnxruntime_VERSION}")
  set(Onnxruntime_INSTALL_LIBS
      ${Onnxruntime_LINK_LIBS} "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_shared.so"
      "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_cuda.so"
      "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_tensorrt.so")
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${Onnxruntime_LINK_LIBS})
  target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM
                             PUBLIC "${onnxruntime_SOURCE_DIR}/include")
  install(FILES ${Onnxruntime_INSTALL_LIBS}
          DESTINATION "${OBS_PLUGIN_DESTINATION}/${CMAKE_PROJECT_NAME}")
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES INSTALL_RPATH
                                                         "$ORIGIN/${CMAKE_PROJECT_NAME}")
endif()
