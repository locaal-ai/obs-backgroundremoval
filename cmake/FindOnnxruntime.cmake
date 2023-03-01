# Try to find onnxruntime library Once done this will define Onnxruntime_FOUND - if system found
# onnruntime library Onnxruntime_INCLUDE_DIRS - The onnruntime include directories
# Onnxruntime_LIBRARIES - The libraries needed to use onnxruntime Onnxruntime_LIBRARY_DIR - The
# directory of onnxruntime libraries

if(APPLE)
  # use pkg-config to find the onnxruntime library
  find_package(PkgConfig REQUIRED)

  pkg_check_modules(Onnxruntime REQUIRED IMPORTED_TARGET libonnxruntime)
  if(NOT Onnxruntime_FOUND)
    message(FATAL_ERROR "Onnxruntime not found by pkg-config (as expected from homebrew install)")
  endif()
  message(STATUS "Onnxruntime found by pkg-config")
  set(Onnxruntime_INCLUDE_DIR
      ${Onnxruntime_INCLUDE_DIRS}
      CACHE STRING "Onnxruntime include directory")
  set(Onnxruntime_LIBRARY_DIR
      ${Onnxruntime_LIBRARY_DIRS}
      CACHE STRING "Onnxruntime library directory")
  set(Onnxruntime_LIBRARIES "${pkgcfg_lib_Onnxruntime_onnxruntime}")
  message(STATUS "Onnxruntime_INCLUDE_DIRS: ${Onnxruntime_INCLUDE_DIRS}")
  message(STATUS "Onnxruntime_LIBRARY_DIRS: ${Onnxruntime_LIBRARY_DIRS}")
  message(STATUS "Onnxruntime_LIBRARIES: ${Onnxruntime_LIBRARIES}")
else()
  if(WITH_CUDA)
    set(ONNXRUNTIME_DIR_HINT ${CMAKE_BINARY_DIR}/nuget/Microsoft.ML.OnnxRuntime.Gpu.1.11.0/)
  else()
    set(ONNXRUNTIME_DIR_HINT ${CMAKE_BINARY_DIR}/nuget/Microsoft.ML.OnnxRuntime.DirectML.1.11.0/)
  endif()

  find_library(
    Onnxruntime_LIBRARIES
    NAMES onnxruntime
    PATHS ${Onnxruntime_DIR} ${ONNXRUNTIME_DIR_HINT}runtimes/win-x64/native
    DOC "Onnxruntime library")

  find_path(
    Onnxruntime_INCLUDE_DIR
    NAMES "onnxruntime_cxx_api.h" "onnxruntime/core/session/onnxruntime_cxx_api.h"
    PATHS ${Onnxruntime_INCLUDE_HINT} ${ONNXRUNTIME_DIR_HINT}build/native/include
    DOC "Onnxruntime include directory")
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LOGGING_FOUND to TRUE if all listed variables
# are TRUE
find_package_handle_standard_args(Onnxruntime DEFAULT_MSG Onnxruntime_INCLUDE_DIR
                                  Onnxruntime_LIBRARIES)

if(Onnxruntime_FOUND)
  set(Onnxruntime_INCLUDE_DIRS
      ${Onnxruntime_INCLUDE_DIR}
      CACHE STRING "Onnxruntime include directories")
  if(APPLE)
    set(Onnxruntime_INCLUDE_DIRS ${Onnxruntime_INCLUDE_DIRS}
                                 ${Onnxruntime_INCLUDE_DIR}/core/session)
  endif()
  list(GET Onnxruntime_LIBRARIES 0 Onnxruntime_LIBRARY)
  get_filename_component(Onnxruntime_LIBRARY_DIR_EX ${Onnxruntime_LIBRARY} DIRECTORY)
  set(Onnxruntime_LIBRARY_DIR
      ${Onnxruntime_LIBRARY_DIR_EX}
      CACHE STRING "Onnxruntime library directory")
endif()

# Tell cmake GUIs to ignore the "local" variables.
mark_as_advanced(Onnxruntime_INCLUDE_DIR Onnxruntime_LIBRARY Onnxruntime_INCLUDE_DIRS
                 Onnxruntime_LIBRARY_DIR Onnxruntime_LIBRARIES)
