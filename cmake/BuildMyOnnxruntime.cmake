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
  INSTALL_COMMAND cmake --install <BINARY_DIR>/${CMAKE_BUILD_TYPE} --config ${CMAKE_BUILD_TYPE} --prefix <INSTALL_DIR>
)

ExternalProject_Get_Property(Onnxruntime_Build INSTALL_DIR)

add_library(Onnxruntime INTERFACE)
add_dependencies(Onnxruntime Onnxruntime_Build)
target_include_directories(Onnxruntime INTERFACE ${INSTALL_DIR}/include ${INSTALL_DIR}/include/onnxruntime/session)
if(APPLE)
  target_link_libraries(Onnxruntime INTERFACE "-framework Accelerate")
endif(APPLE)

set(Onnxruntime_LIB_NAMES session)
foreach(lib_name in LISTS Onnxruntime_LIB_NAMES)
  add_library(Onnxruntime::${lib_name} STATIC IMPORTED)
  set_target_properties(
    Onnxruntime::${lib_name}
    PROPERTIES
      IMPORTED_LOCATION
      ${INSTALL_DIR}/${OpenCV_LIB_PATH_3RD}/${CMAKE_STATIC_LIBRARY_PREFIX}onnxruntime_${lib_name}${CMAKE_STATIC_LIBRARY_SUFFIX}
  )

  target_link_libraries(Onnxruntime INTERFACE Onnxruntime::${lib_name})
endforeach()

