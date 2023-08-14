add_library(WinRTHttpClient WinRTHttpClient.cpp)
set_target_properties(
  WinRTHttpClient
  PROPERTIES CXX_STANDARD 17
             CXX_STANDARD_REQUIRED ON
             CXX_EXTENSIONS OFF
             VS_GLOBAL_CppWinRTOptimized true
             VS_GLOBAL_CppWinRTRootNamespaceAutoMerge true)
target_link_libraries(WinRTHttpClient PRIVATE OBS::libobs plugin-support)
target_include_directories(WinRTHttpClient PRIVATE "${CMAKE_SOURCE_DIR}/src")
