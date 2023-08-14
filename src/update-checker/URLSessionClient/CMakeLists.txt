add_library(URLSessionClient URLSessionClient.mm)
target_link_libraries(URLSessionClient PRIVATE OBS::libobs plugin-support)
target_include_directories(URLSessionClient PRIVATE "${CMAKE_SOURCE_DIR}/src")
