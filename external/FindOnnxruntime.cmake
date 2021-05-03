# Try to find onnxruntime library
# Once done this will define
#  Onnxruntime_FOUND - if system found onnruntime library
#  Onnxruntime_INCLUDE_DIRS - The onnruntime include directories
#  Onnxruntime_LIBRARIES - The libraries needed to use onnxruntime
#  Onnxruntime_LIBRARY_DIR - The directory of onnxruntime libraries

set(Onnxruntime_DIR_BUILD ${CMAKE_BINARY_DIR}/onnxruntime/)
if(APPLE AND EXISTS ${Onnxruntime_DIR_BUILD})
    file(GLOB Onnxruntime_LIBRARIES ${Onnxruntime_DIR_BUILD}/lib/lib*.a)
    set(Onnxruntime_INCLUDE_DIR ${Onnxruntime_DIR_BUILD}/include)
else()
    find_library(Onnxruntime_LIBRARIES
        NAMES
            onnxruntime
        PATHS
            ${Onnxruntime_DIR}
        DOC "Onnxruntime library")

    find_path(Onnxruntime_INCLUDE_DIR
        NAMES
            "onnxruntime_cxx_api.h"
            "onnxruntime/core/session/onnxruntime_cxx_api.h"
        PATHS
            ${Onnxruntime_INCLUDE_HINT}
        DOC "Onnxruntime include directory")
endif()



include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LOGGING_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Onnxruntime DEFAULT_MSG Onnxruntime_INCLUDE_DIR Onnxruntime_LIBRARIES)

if (Onnxruntime_FOUND)
    set(Onnxruntime_INCLUDE_DIRS ${Onnxruntime_INCLUDE_DIR} )
    if (APPLE)
        set(Onnxruntime_INCLUDE_DIRS ${Onnxruntime_INCLUDE_DIRS} ${Onnxruntime_INCLUDE_DIR}/onnxruntime/core/session)
    endif()
    get_filename_component(Onnxruntime_LIBRARY_DIR ${Onnxruntime_LIBRARY} DIRECTORY)
endif()

# Tell cmake GUIs to ignore the "local" variables.
mark_as_advanced(Onnxruntime_INCLUDE_DIR Onnxruntime_LIBRARY)
