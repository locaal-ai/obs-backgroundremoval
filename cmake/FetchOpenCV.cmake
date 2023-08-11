include(FetchContent)

set(OpenCV_VERSION "4.8.0-1")

set(OpenCV_BASEURL "https://github.com/umireon/obs-backgroundremoval-dep-opencv/releases/download/${OpenCV_VERSION}")

if(${CMAKE_BUILD_TYPE} STREQUAL Release OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
  set(OpenCV_BUILD_TYPE Release)
else()
  set(OpenCV_BUILD_TYPE Debug)
endif()

if(APPLE)
  if(OpenCV_BUILD_TYPE STREQUAL Release)
    set(OpenCV_URL "${OpenCV_BASEURL}/opencv-macos-Release.tar.gz")
    set(OpenCV_HASH MD5=0875366a03aa44def76ab5a12d3e7b8f)
  else()
    set(OpenCV_URL "${OpenCV_BASEURL}/opencv-macos-Debug.tar.gz")
    set(OpenCV_HASH MD5=9ae59653c7f9a4c991fbf59018e45d2c)
  endif()
elseif(MSVC)
  if(OpenCV_BUILD_TYPE STREQUAL Release)
    set(OpenCV_URL "${OpenCV_BASEURL}/opencv-windows-Release.zip")
    set(OpenCV_HASH MD5=e653d590cfbcc3a9bf15ef20e64a6e32)
  else()
    set(OpenCV_URL "${OpenCV_BASEURL}/opencv-windows-Debug.zip")
    set(OpenCV_HASH MD5=abad340ccb73da2924544ec9066afcb3)
  endif()
else()
  if(OpenCV_BUILD_TYPE STREQUAL Release)
    set(OpenCV_URL "${OpenCV_BASEURL}/opencv-linux-Release.tar.gz")
    set(OpenCV_HASH MD5=7a668fbc3ac536812643c6b8c8f96be9)
  else()
    set(OpenCV_URL "${OpenCV_BASEURL}/opencv-linux-Debug.tar.gz")
    set(OpenCV_HASH MD5=259699c71055ff748c200e62af059104)
  endif()
endif()

FetchContent_Declare(
  opencv
  URL ${OpenCV_URL}
  URL_HASH ${OpenCV_HASH})
FetchContent_MakeAvailable(opencv)

add_library(OpenCV INTERFACE)
if(MSVC)
  target_link_libraries(OpenCV INTERFACE ${opencv_SOURCE_DIR}/x64/vc17/staticlib/opencv_imgproc480.lib
                                         ${opencv_SOURCE_DIR}/x64/vc17/staticlib/opencv_core480.lib)
  target_include_directories(OpenCV SYSTEM INTERFACE ${opencv_SOURCE_DIR}/include)
else()
  target_link_libraries(OpenCV INTERFACE ${opencv_SOURCE_DIR}/lib/libopencv_imgproc.a
                                         ${opencv_SOURCE_DIR}/lib/libopencv_core.a)
  target_include_directories(OpenCV SYSTEM INTERFACE ${opencv_SOURCE_DIR}/include/opencv4)
endif()
