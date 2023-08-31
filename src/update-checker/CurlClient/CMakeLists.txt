add_library(CurlClient STATIC CurlClient.cpp)
target_link_libraries(CurlClient PRIVATE OBS::libobs plugin-support)
target_include_directories(CurlClient PRIVATE "${CMAKE_SOURCE_DIR}/src")

find_package(CURL REQUIRED)
target_link_libraries(CurlClient PRIVATE "${CURL_LIBRARIES}")
target_include_directories(CurlClient SYSTEM PRIVATE "${CURL_INCLUDE_DIRS}")
