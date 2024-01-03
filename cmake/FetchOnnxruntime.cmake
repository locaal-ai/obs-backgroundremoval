include(FetchContent)

set(CUSTOM_ONNXRUNTIME_URL
    ""
    CACHE STRING "URL of a downloaded ONNX Runtime tarball")

set(CUSTOM_ONNXRUNTIME_HASH
    ""
    CACHE STRING "Hash of a downloaded ONNX Runtime tarball")

if(CUSTOM_ONNXRUNTIME_URL STREQUAL "")
  set(USE_PREDEFINED_ONNXRUNTIME ON)
else()
  if(CUSTOM_ONNXRUNTIME_HASH STREQUAL "")
    message(FATAL_ERROR "Both of CUSTOM_ONNXRUNTIME_URL and CUSTOM_ONNXRUNTIME_HASH must be present!")
  else()
    set(USE_PREDEFINED_ONNXRUNTIME OFF)
  endif()
endif()

if(USE_PREDEFINED_ONNXRUNTIME)
  set(Onnxruntime_VERSION "1.16.3")
  set(Onnxruntime_BASEURL "https://github.com/microsoft/onnxruntime/releases/download/v${Onnxruntime_VERSION}")
  set(Onnxruntime_WINDOWS_VERSION "v${Onnxruntime_VERSION}-2")
  set(Onnxruntime_WINDOWS_BASEURL
      "https://github.com/occ-ai/onnxruntime-static-win/releases/download/${Onnxruntime_WINDOWS_VERSION}")

  if(APPLE)
    set(Onnxruntime_URL "${Onnxruntime_BASEURL}/onnxruntime-osx-universal2-${Onnxruntime_VERSION}.tgz")
    set(Onnxruntime_HASH SHA256=6428d0f0ff1386e8e8256a708e187c1f8861387c9554bfc9c5f3390ffa0df5cc)
  elseif(MSVC)
    set(Onnxruntime_URL "${Onnxruntime_WINDOWS_BASEURL}/onnxruntime-windows-${Onnxruntime_WINDOWS_VERSION}-Release.zip")
    set(OOnnxruntime_HASH SHA256=d545d1040be3c5c173b4f03a58f391f47fb039f1e799c4f2be706c678d8444f4)
  else()
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
      set(Onnxruntime_URL "${Onnxruntime_BASEURL}/onnxruntime-linux-aarch64-${Onnxruntime_VERSION}.tgz")
      set(Onnxruntime_HASH SHA256=784dbef93b40196aa668d29d78294a81c0d21361d36530b817bb24d87e8730e8)
    else()
      set(Onnxruntime_URL "${Onnxruntime_BASEURL}/onnxruntime-linux-x64-gpu-${Onnxruntime_VERSION}.tgz")
      set(Onnxruntime_HASH SHA256=bbdc33367c056029b3ac0c042cbca2236b8f59a3a53b4daf23432ef1d8bf52de)
    endif()
  endif()
else()
  set(Onnxruntime_URL "${CUSTOM_ONNXRUNTIME_URL}")
  set(Onnxruntime_HASH "${CUSTOM_ONNXRUNTIME_HASH}")
endif()

FetchContent_Declare(
  onnxruntime
  URL ${Onnxruntime_URL}
  URL_HASH ${Onnxruntime_HASH})
FetchContent_MakeAvailable(onnxruntime)

if(APPLE)
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
elseif(MSVC)
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
else()
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(Onnxruntime_LINK_LIBS "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so.${Onnxruntime_VERSION}")
    set(Onnxruntime_INSTALL_LIBS ${Onnxruntime_LINK_LIBS})
  else()
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
