cmake_minimum_required(VERSION 3.10)

include(external/MunkeiVersionFromGit.cmake)
version_from_git()

# Change obs-plugintemplate to your plugin's name in a machine-readable format
# (e.g.: obs-myawesomeplugin) and set
project(obs-backgroundremoval VERSION ${VERSION})

# Replace `Your Name Here` with the name (yours or your organization's) you want
# to see as the author of the plugin (in the plugin's metadata itself and in the installers)
set(PLUGIN_AUTHOR "Roy Shilkrot")

# Replace `com.example.obs-plugin-template` with a unique Bundle ID for macOS releases
# (used both in the installer and when submitting the installer for notarization)
set(MACOS_BUNDLEID "com.example.obs-backgroundremoval")

# Replace `me@contoso.com` with the maintainer email address you want to put in Linux packages
set(LINUX_MAINTAINER_EMAIL "roy.shil@gmail.com")

if(WIN32 OR MSVC)
    set(VCPKG_DIR ${CMAKE_BINARY_DIR}/vcpkg)
    set(VCPKG_INSTALLED_DIR ${VCPKG_DIR}/installed/x64-windows-static)
    set(CMAKE_TOOLCHAIN_FILE ${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake)

    set(OpenCV_DIR ${VCPKG_INSTALLED_DIR}/share/opencv)
    set(protobuf_DIR ${VCPKG_INSTALLED_DIR}/share/protobuf)
    set(LibLZMA_DIR ${VCPKG_INSTALLED_DIR}/share/liblzma)
    set(quirc_DIR ${VCPKG_INSTALLED_DIR}/share/quirc)

    # Find lib TIFF
    find_library(TIFF_LIBRARY tiff PATHS ${VCPKG_INSTALLED_DIR}/lib)
    find_path(TIFF_INCLUDE_DIR tiff.h PATHS ${VCPKG_INSTALLED_DIR}/include)

    find_file(Protobuf_PROTOC_EXECUTABLE protoc.exe PATHS ${VCPKG_INSTALLED_DIR}/tools)
endif()

if (UNIX)
    set(Onnxruntime_INCLUDE_HINT /usr/local/include/)
endif()

set(RELEASE_DIR "${PROJECT_SOURCE_DIR}/release" CACHE STRING "Directory for building release packages")

set(CMAKE_PREFIX_PATH "${QTDIR}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/external")

# In case you need C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
    include(external/FindLibObs.cmake)
else()
    find_package(LibObs REQUIRED)
endif()

# Qt is not required for this plugin, at the moment.
# set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTOUIC ON)
# find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

configure_file(
    src/plugin-macros.h.in
    ../src/plugin-macros.generated.h
)
configure_file(
    installer/installer-macOS.pkgproj.in
    ../installer/installer-macOS.generated.pkgproj
)
configure_file(
    installer/installer-Windows.iss.in
    ../installer/installer-Windows.generated.iss
)

configure_file(
    ci/ci_includes.sh.in
    ../ci/ci_includes.generated.sh
)
configure_file(
    ci/ci_includes.cmd.in
    ../ci/ci_includes.generated.cmd
)

set(PLUGIN_SOURCES
    src/background-filter.cpp
    src/plugin-main.cpp
    )

set(PLUGIN_HEADERS
	src/plugin-macros.generated.h)

# --- Platform-independent build settings ---
add_library(${CMAKE_PROJECT_NAME} MODULE ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

find_package(Onnxruntime REQUIRED)

if (APPLE)
    set(OpenCV_DIR ${CMAKE_BINARY_DIR}/opencv/lib/cmake/opencv4)
endif()
find_package(OpenCV 4.2 REQUIRED COMPONENTS core imgproc)

include_directories(
	${LIBOBS_INCLUDE_DIR}/../UI/obs-frontend-api
    ${LIBOBS_INCLUDE_DIR}
    ${LIBOBS_INCLUDE_DIRS}
    ${Onnxruntime_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(${CMAKE_PROJECT_NAME}
	${LIBOBS_LIBRARIES}
    ${Onnxruntime_LIBRARIES}
    ${OpenCV_LIBRARIES}
)

# --- End of section ---

# --- Windows-specific build settings and tasks ---
if(WIN32)
    find_library(OBS_FRONTEND_LIB obs-frontend-api PATHS ${obsPath}/build/UI/obs-frontend-api/Release)

	if(NOT DEFINED OBS_FRONTEND_LIB)
		set(OBS_FRONTEND_LIB "OBS_FRONTEND_LIB-NOTFOUND" CACHE FILEPATH "OBS frontend library")
		message(FATAL_ERROR "Could not find OBS Frontend API\'s library !")
	endif()

    # find the onnxruntime DLL for packaging with the plugin DLL
    set(Onnxruntime_DLL "${Onnxruntime_LIBRARY_DIR}/onnxruntime.dll")
    set(DirectML_DLL "${CMAKE_BINARY_DIR}/nuget/Microsoft.AI.DirectML.1.4.2/bin/x64-win/DirectML.dll")

    # Enable Multicore Builds and disable FH4 (to not depend on VCRUNTIME140_1.DLL when building with VS2019)
    if (MSVC)
        add_definitions(/MP /d2FH4- /std:c++latest /MT)
    endif()

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(ARCH_NAME "64bit")
		set(OBS_BUILDDIR_ARCH "build64")
	else()
		set(ARCH_NAME "32bit")
		set(OBS_BUILDDIR_ARCH "build32")
	endif()

	include_directories(
		"${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/UI"
	)

	target_link_libraries(${CMAKE_PROJECT_NAME}
		"${OBS_FRONTEND_LIB}"
    )

	# --- Release package helper ---
	# The "release" folder has a structure similar OBS' one on Windows
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        # If config is Release or RelWithDebInfo, package release files
        COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
            "${CMAKE_COMMAND}" -E make_directory
                "${RELEASE_DIR}/data/obs-plugins/${CMAKE_PROJECT_NAME}"
                "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}"
        )

        COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
            "${CMAKE_COMMAND}" -E copy_directory
                "${PROJECT_SOURCE_DIR}/data"
                "${RELEASE_DIR}/data/obs-plugins/${CMAKE_PROJECT_NAME}"
        )

        COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
                "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}"
        )

        COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
            "${CMAKE_COMMAND}" -E copy
                "${Onnxruntime_DLL}" "${DirectML_DLL}"
                "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}"
        )

        # If config is RelWithDebInfo, copy the pdb file
        COMMAND if $<CONFIG:RelWithDebInfo>==1 (
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}>"
                "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}"
        )

        # Copy to obs-studio dev environment for immediate testing
        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
                "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/obs-plugins/${ARCH_NAME}"
        )

        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}>"
                "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/obs-plugins/${ARCH_NAME}"
        )

        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E make_directory
                "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/data/obs-plugins/${CMAKE_PROJECT_NAME}"
        )

        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E copy_directory
                "${PROJECT_SOURCE_DIR}/data"
                "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/data/obs-plugins/${CMAKE_PROJECT_NAME}"
        )
    )
	# --- End of sub-section ---

    install(DIRECTORY "${RELEASE_DIR}/" DESTINATION .)
    # Tell CPack to create a zip file.
    set(CPACK_GENERATOR "ZIP")
    # Tell CPack what to name the zip file. It will append `.zip`.
    set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-win64")
    # Tell CPack not to put everything inside an enclosing directory.
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
    # Apparently this should be always on but isn't for backwards compatibility.
    set(CPACK_VERBATIM_VARIABLES YES)
    include(CPack)
endif()
# --- End of section ---

# --- Linux-specific build settings and tasks ---
if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)

	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "")
	target_link_libraries(${CMAKE_PROJECT_NAME} obs-frontend-api)

	file(GLOB locale_files data/locale/*.ini)
	file(GLOB onnxmodels data/*.onnx)

	install(TARGETS ${CMAKE_PROJECT_NAME}
		LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/obs-plugins")

	install(FILES ${locale_files}
		DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/obs/obs-plugins/${CMAKE_PROJECT_NAME}/locale")

	install(FILES ${onnxmodels}
		DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/obs/obs-plugins/${CMAKE_PROJECT_NAME}/")
endif()
# --- End of section ---

# -- OS X specific build settings and tasks --
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -fvisibility=default")

	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "")
	target_link_libraries(${CMAKE_PROJECT_NAME} "${OBS_FRONTEND_LIB}")

    # add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    add_custom_target(dist DEPENDS ${CMAKE_PROJECT_NAME}
        COMMAND
            "${CMAKE_COMMAND}" -E rm -rf "${RELEASE_DIR}"

        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
                "${RELEASE_DIR}/Resources/data/obs-plugins/${CMAKE_PROJECT_NAME}"
                "${RELEASE_DIR}/Plugins"

        COMMAND
            "${CMAKE_COMMAND}" -E copy_directory
                "${PROJECT_SOURCE_DIR}/data"
                "${RELEASE_DIR}/Resources/data/obs-plugins/${CMAKE_PROJECT_NAME}"

        COMMAND
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
                "${RELEASE_DIR}/Plugins/"

        COMMAND
            "${CMAKE_COMMAND}" -E rm -f
                "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-macosx.zip"

    )

    install(DIRECTORY "${RELEASE_DIR}/" DESTINATION .)

    # Tell CPack to create a zip file.
    set(CPACK_GENERATOR "ZIP")
    # Tell CPack what to name the zip file. It will append `.zip`.
    set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-macosx")
    # Tell CPack not to put everything inside an enclosing directory.
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
    # Apparently this should be always on but isn't for backwards compatibility.
    set(CPACK_VERBATIM_VARIABLES YES)
    include(CPack)
endif()
# -- End of section --
