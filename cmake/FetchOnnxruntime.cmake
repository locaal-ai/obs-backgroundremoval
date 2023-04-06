include(FetchContent)

if(OS_MACOS)
  FetchContent_Declare(
    Onnxruntime
    URL https://github.com/microsoft/onnxruntime/releases/download/v1.14.1/onnxruntime-osx-universal2-1.14.1.tgz
    URL_HASH MD5=9725836c49deb09fc352a57dc8a1b806
  )
  FetchContent_MakeAvailable(Onnxruntime)
  set(Onnxruntime_LIB "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.1.14.1.dylib")
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "${Onnxruntime_LIB}")
  target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PUBLIC "${onnxruntime_SOURCE_DIR}/include")
  install(FILES "${Onnxruntime_LIB}" DESTINATION "${CMAKE_PROJECT_NAME}.plugin/Contents/Frameworks")
endif()
