include(FetchContent)

set(Onnxruntime_VERSION "1.14.1")
set(Onnxruntime_DirectML_VERSION "1.9.0")
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
    URL "https://github.com/microsoft/onnxruntime/releases/download/v${Onnxruntime_VERSION}/Microsoft.ML.OnnxRuntime.DirectML.${Onnxruntime_VERSION}.zip"
    URL_HASH MD5=dfdb875999b119f2b85a1f4d75b3e131)
  FetchContent_Declare(
    DirectML
    URL "https://globalcdn.nuget.org/packages/microsoft.ai.directml.${Onnxruntime_DirectML_VERSION}.nupkg"
    URL_HASH MD5=59dad6fc48cfd052bf0fdccfa7b35b72)
  FetchContent_MakeAvailable(Onnxruntime DirectML)
  set(Onnxruntime_LIB "${onnxruntime_SOURCE_DIR}/runtimes/win-x64/native/onnxruntime.dll")
  set(Onnxruntime_IMPLIB "${onnxruntime_SOURCE_DIR}/runtimes/win-x64/native/onnxruntime.lib")
  set(DirectML_LIB "${directml_SOURCE_DIR}/bin/x64-win/DirectML.dll")
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "${Onnxruntime_IMPLIB}")
  target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM
                             PUBLIC "${onnxruntime_SOURCE_DIR}/build/native/include")
  install(FILES "${Onnxruntime_LIB}" "${DirectML_LIB}" DESTINATION "${OBS_PLUGIN_DESTINATION}")
elseif(OS_LINUX)
  FetchContent_Declare(
    Onnxruntime
    URL "https://github.com/microsoft/onnxruntime/releases/download/v${Onnxruntime_VERSION}/onnxruntime-linux-x64-gpu-${Onnxruntime_VERSION}.tgz"
    URL_HASH MD5=6a3866eb7dce86a17922c0662623f77e)
  FetchContent_MakeAvailable(Onnxruntime)
  set(Onnxruntime_LINK_LIBS "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_shared.so" "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so.${Onnxruntime_VERSION}")
  set(Onnxruntime_INSTALL_LIBS "${Onnxruntime_LINK_LIBS}" "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_cuda.so" "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_tensorrt.so")
  target_link_libraries(${CMAKE_PROJECT_NAME} SYSTEM PUBLIC "${Onnxruntime_LINK_LIBS}")
  target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM
                             PUBLIC "${onnxruntime_SOURCE_DIR}/include")
  install(FILES "${Onnxruntime_INSTALL_LIBS}" DESTINATION ".")
endif()
