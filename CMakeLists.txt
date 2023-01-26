cmake_minimum_required(VERSION 3.16...3.21)

set(CMAKE_CXX_STANDARD 20)

# Change obs-plugintemplate to your plugin's name in a machine-readable format
# (e.g.: obs-myawesomeplugin) and set
project(obs-backgroundremoval VERSION 0.5.5)
add_library(${CMAKE_PROJECT_NAME} MODULE)

# Replace `Your Name Here` with the name (yours or your organization's) you want
# to see as the author of the plugin (in the plugin's metadata itself and in the
# installers)
set(PLUGIN_AUTHOR "Roy Shilkrot and Kaito Udagawa")

# Replace `com.example.obs-plugin-template` with a unique Bundle ID for macOS
# releases (used both in the installer and when submitting the installer for
# notarization)
set(MACOS_BUNDLEID "tokyo.kaito.obs-backgroundremoval")

# Replace `me@contoso.com` with the maintainer email address you want to put in
# Linux packages
set(LINUX_MAINTAINER_EMAIL "umreon@gmail.com")

# Add your custom source files here - header files are optional and only
# required for visibility e.g. in Xcode or Visual Studio
target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-main.cpp
                                             src/background-filter.cpp)

# Import libobs as main plugin dependency
find_package(libobs REQUIRED)
include(cmake/ObsPluginHelpers.cmake)

# Uncomment these lines if you want to use the OBS Frontend API in your plugin
find_package(obs-frontend-api REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)

# Uncomment those lines if you want to use Qt in your plugin
#[[
find_qt(COMPONENTS Widgets Core)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt::Core Qt::Widgets)
set_target_properties(
  ${CMAKE_PROJECT_NAME}
  PROPERTIES AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON)
#]]

configure_file(src/plugin-macros.h.in
               ${CMAKE_SOURCE_DIR}/src/plugin-macros.generated.h)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-macros.generated.h)

include(cmake/BuildMyOpenCV.cmake)
include(cmake/BuildMyOnnxruntime.cmake)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Onnxruntime OpenCV)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin|iOS")
  foreach(flags CMAKE_CXX_FLAGS CMAKE_OBJC_FLAGS CMAKE_OBJCXX_FLAGS)
    string(APPEND ${flags} " -fvisibility=hidden -fvisibility-inlines-hidden")
  endforeach()
endif()

# /!\ TAKE NOTE: No need to edit things past this point /!\

# --- Platform-independent build settings ---

target_include_directories(${CMAKE_PROJECT_NAME}
                           PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

# --- End of section ---

# --- Windows-specific build settings and tasks ---
if(OS_WINDOWS)
  configure_file(cmake/bundle/windows/installer-Windows.iss.in
                 ${CMAKE_BINARY_DIR}/installer-Windows.generated.iss)

  configure_file(cmake/bundle/windows/resource.rc.in
                 ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.rc)
  target_sources(${CMAKE_PROJECT_NAME}
                 PRIVATE ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.rc)

  if(MSVC)
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W4)
  endif()
  # --- End of section ---

  # -- macOS specific build settings and tasks --
elseif(OS_MACOS)
  configure_file(cmake/bundle/macos/installer-macos.pkgproj.in
                 ${CMAKE_BINARY_DIR}/installer-macos.generated.pkgproj)

  set(MACOSX_PLUGIN_GUI_IDENTIFIER "${MACOS_BUNDLEID}")
  set(MACOSX_PLUGIN_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION}")
  set(MACOSX_PLUGIN_SHORT_VERSION_STRING "1")

  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall)
  # --- End of section ---

  # --- Linux-specific build settings and tasks ---
else()
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall)
endif()
# --- End of section ---

setup_plugin_target(${CMAKE_PROJECT_NAME})

# option(WITH_CUDA "Build with CUDA support." OFF) if(WITH_CUDA)
# add_compile_definitions("WITH_CUDA=1") endif()

# if(WIN32 OR MSVC) set(VCPKG_DIR ${CMAKE_BINARY_DIR}/vcpkg)
# set(VCPKG_INSTALLED_DIR ${VCPKG_DIR}/installed/x64-windows-static)
# set(CMAKE_TOOLCHAIN_FILE ${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake)

# set(OpenCV_DIR ${VCPKG_INSTALLED_DIR}/share/opencv) set(Protobuf_DIR
# ${VCPKG_INSTALLED_DIR}/share/protobuf) set(LibLZMA_DIR
# ${VCPKG_INSTALLED_DIR}/share/liblzma) set(quirc_DIR
# ${VCPKG_INSTALLED_DIR}/share/quirc)

# if(NOT ${libobs_DIR}) message(FATAL "Please provide libobs_DIR to CMake.")
# endif(NOT ${libobs_DIR})

# set(w32-pthreads_DIR ${libobs_DIR}/../deps/w32-pthreads)
# set(obs-frontend-api_DIR ${libobs_DIR}/../UI/obs-frontend-api)

# # Find lib TIFF find_library(TIFF_LIBRARY tiff PATHS
# ${VCPKG_INSTALLED_DIR}/lib) find_path(TIFF_INCLUDE_DIR tiff.h PATHS
# ${VCPKG_INSTALLED_DIR}/include)

# find_file(Protobuf_PROTOC_EXECUTABLE protoc.exe PATHS
# ${VCPKG_INSTALLED_DIR}/tools/protobuf) elseif(APPLE) # Apple if (WITH_CUDA)
# message(FATAL "Cannot use CUDA on Apple builds") endif() endif()

# if (UNIX) set(Onnxruntime_INCLUDE_HINT /usr/local/include/) endif()

# if (APPLE) set(OpenCV_DIR ${CMAKE_BINARY_DIR}/opencv/lib/cmake/opencv4)
# endif()

# set(RELEASE_DIR "${PROJECT_SOURCE_DIR}/release" CACHE STRING "Directory for
# building release packages")

# set(CMAKE_PREFIX_PATH "${QTDIR}") set(CMAKE_INCLUDE_CURRENT_DIR ON)
# set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/external")

# # In case you need C++ set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # configure_file( #     src/plugin-macros.h.in #
# ../src/plugin-macros.generated.h # ) configure_file(
# installer/installer-macOS.pkgproj.in
# ../installer/installer-macOS.generated.pkgproj ) configure_file(
# installer/installer-Windows.iss.in
# ../installer/installer-Windows.generated.iss )

# configure_file( ci/ci_includes.sh.in ../ci/ci_includes.generated.sh )
# configure_file( ci/ci_includes.cmd.in ../ci/ci_includes.generated.cmd )

# # set(PLUGIN_SOURCES #     src/background-filter.cpp #     src/plugin-main.cpp
# #     )

# # set(PLUGIN_HEADERS #       src/plugin-macros.generated.h)

# # --- Platform-independent build settings ---
# #add_library(${CMAKE_PROJECT_NAME} MODULE ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

# # find_package(libobs REQUIRED) # find_package(obs-frontend-api REQUIRED) #
# include(external/ObsPluginHelpers.cmake)

# find_package(Onnxruntime REQUIRED)

# find_package(OpenCV 4.5 REQUIRED COMPONENTS core imgproc)

# include_directories( ${LIBOBS_INCLUDE_DIR}/../UI/obs-frontend-api
# ${LIBOBS_INCLUDE_DIR} ${LIBOBS_INCLUDE_DIRS} ${Onnxruntime_INCLUDE_DIRS}
# ${OpenCV_INCLUDE_DIRS} )

# target_link_libraries(${CMAKE_PROJECT_NAME} OBS::libobs OBS::obs-frontend-api
# ${Onnxruntime_LIBRARIES} ${OpenCV_LIBRARIES} )

# # --- End of section ---

# # --- Windows-specific build settings and tasks --- if(WIN32) # Enable
# Multicore Builds and disable FH4 (to not depend on VCRUNTIME140_1.DLL when
# building with VS2019) if (MSVC) add_definitions(/MP /d2FH4- /std:c++latest
# /MT) endif()

# # --- Release package helper --- # The "release" folder has a structure
# similar OBS' one on Windows add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
# POST_BUILD COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
# "${CMAKE_COMMAND}" -E remove_directory "${RELEASE_DIR}" )

# # If config is Release or RelWithDebInfo, package release files COMMAND if
# $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 ( "${CMAKE_COMMAND}" -E
# make_directory "${RELEASE_DIR}/data/obs-plugins/${CMAKE_PROJECT_NAME}"
# "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}" )

# COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
# "${CMAKE_COMMAND}" -E copy_directory "${PROJECT_SOURCE_DIR}/data"
# "${RELEASE_DIR}/data/obs-plugins/${CMAKE_PROJECT_NAME}" )

# COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
# "${CMAKE_COMMAND}" -E copy
# "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
# "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}" )

# # package the DirectML DLL with the plugin DLL unless using CUDA COMMAND if
# $<AND:$<NOT:$<BOOL:${WITH_CUDA}>>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>>==1
# ( "${CMAKE_COMMAND}" -E copy
# "${CMAKE_BINARY_DIR}/nuget/Microsoft.AI.DirectML.1.8.0/bin/x64-win/DirectML.dll"
# "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}" )

# # package the onnxruntime DLL with the plugin DLL COMMAND if
# $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 ( "${CMAKE_COMMAND}" -E
# copy "${Onnxruntime_LIBRARY_DIR}/onnxruntime.dll"
# "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}" )

# # If config is RelWithDebInfo, copy the pdb file COMMAND if
# $<CONFIG:RelWithDebInfo>==1 ( "${CMAKE_COMMAND}" -E copy
# "$<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}>"
# "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}" )

# # Copy to obs-studio dev environment for immediate testing COMMAND if
# $<CONFIG:Debug>==1 ( "${CMAKE_COMMAND}" -E copy
# "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
# "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/obs-plugins/${ARCH_NAME}"
# )

# COMMAND if $<CONFIG:Debug>==1 ( "${CMAKE_COMMAND}" -E copy
# "$<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}>"
# "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/obs-plugins/${ARCH_NAME}"
# )

# COMMAND if $<CONFIG:Debug>==1 ( "${CMAKE_COMMAND}" -E make_directory
# "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/data/obs-plugins/${CMAKE_PROJECT_NAME}"
# )

# COMMAND if $<CONFIG:Debug>==1 ( "${CMAKE_COMMAND}" -E copy_directory
# "${PROJECT_SOURCE_DIR}/data"
# "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/data/obs-plugins/${CMAKE_PROJECT_NAME}"
# ) ) # --- End of sub-section ---

# install(DIRECTORY "${RELEASE_DIR}/" DESTINATION .) # Tell CPack to create a
# zip file. set(CPACK_GENERATOR "ZIP") # Tell CPack what to name the zip file.
# It will append `.zip`. if (WITH_CUDA) set(CPACK_PACKAGE_NAME
# "${CMAKE_PROJECT_NAME}-cuda") endif()

# # Tell CPack not to put everything inside an enclosing directory.
# set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF) # Apparently this should be always
# on but isn't for backwards compatibility. set(CPACK_VERBATIM_VARIABLES YES)
# include(CPack) endif() # --- End of section ---

# # --- Linux-specific build settings and tasks --- if(UNIX AND NOT APPLE)
# include(GNUInstallDirs)

# set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "")
# target_link_libraries(${CMAKE_PROJECT_NAME} obs-frontend-api)

# file(GLOB locale_files data/locale/*.ini) file(GLOB onnxmodels data/*.onnx)

# install(TARGETS ${CMAKE_PROJECT_NAME} LIBRARY DESTINATION
# "${CMAKE_INSTALL_FULL_LIBDIR}/obs-plugins")

# install(FILES ${locale_files} DESTINATION
# "${CMAKE_INSTALL_FULL_DATAROOTDIR}/obs/obs-plugins/${CMAKE_PROJECT_NAME}/locale")

# install(FILES ${onnxmodels} DESTINATION
# "${CMAKE_INSTALL_FULL_DATAROOTDIR}/obs/obs-plugins/${CMAKE_PROJECT_NAME}/")
# endif() # --- End of section ---

# # -- OS X specific build settings and tasks -- if(APPLE)
# set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13) set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}
# -stdlib=libc++ -fvisibility=default -framework Foundation")

# set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "")
# target_link_libraries(${CMAKE_PROJECT_NAME} "${OBS_FRONTEND_LIB}")

# # add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
# add_custom_target(dist DEPENDS ${CMAKE_PROJECT_NAME} COMMAND
# "${CMAKE_COMMAND}" -E rm -rf "${RELEASE_DIR}"

# COMMAND "${CMAKE_COMMAND}" -E make_directory
# "${RELEASE_DIR}/Resources/data/obs-plugins/${CMAKE_PROJECT_NAME}"
# "${RELEASE_DIR}/Plugins"

# COMMAND "${CMAKE_COMMAND}" -E copy_directory "${PROJECT_SOURCE_DIR}/data"
# "${RELEASE_DIR}/Resources/data/obs-plugins/${CMAKE_PROJECT_NAME}"

# COMMAND "${CMAKE_COMMAND}" -E copy
# "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
# "${RELEASE_DIR}/Plugins/"

# COMMAND "${CMAKE_COMMAND}" -E rm -f
# "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-macosx.zip"

# )

# install(DIRECTORY "${RELEASE_DIR}/" DESTINATION .)

# # Tell CPack to create a zip file. set(CPACK_GENERATOR "ZIP") # Tell CPack
# what to name the zip file. It will append `.zip`. set(CPACK_PACKAGE_FILE_NAME
# "${CMAKE_PROJECT_NAME}-macosx") # Tell CPack not to put everything inside an
# enclosing directory. set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF) # Apparently
# this should be always on but isn't for backwards compatibility.
# set(CPACK_VERBATIM_VARIABLES YES) include(CPack) endif() # -- End of section
# --
