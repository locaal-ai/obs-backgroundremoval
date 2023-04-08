include(FetchContent)

set(Onnxruntime_VERSION "1.14.1")
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
elseif(OS_LINUX)
  FetchContent_Declare(
    Onnxruntime
    URL "https://github.com/microsoft/onnxruntime/releases/download/v${Onnxruntime_VERSION}/onnxruntime-linux-x64-gpu-${Onnxruntime_VERSION}.tgz"
    URL_HASH MD5=9725836c49deb09fc352a57dc8a1b806)
  FetchContent_MakeAvailable(Onnxruntime)
  set(Onnxruntime_LIBS
      "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_cuda.so"
      "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_tensorrt.so"
      "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_shared.so"
      "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.${Onnxruntime_VERSION}.so")
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "${Onnxruntime_LIB}")
  target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM
                             PUBLIC "${onnxruntime_SOURCE_DIR}/include")
  install(FILES "${Onnxruntime_LIBS}"
          DESTINATION ".")
endif()
