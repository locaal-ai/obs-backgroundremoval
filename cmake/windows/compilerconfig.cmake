# CMake Windows compiler configuration module

include_guard(GLOBAL)

include(compiler_common)

# CMake 3.24 introduces a bug mistakenly interpreting MSVC as supporting the '-pthread' compiler flag
if(CMAKE_VERSION VERSION_EQUAL 3.24.0)
  set(THREADS_HAVE_PTHREAD_ARG FALSE)
endif()

message(DEBUG "Current Windows API version: ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM)
  message(DEBUG "Maximum Windows API version: ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM}")
endif()

if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION VERSION_LESS 10.0.20348)
  message(FATAL_ERROR "OBS requires Windows 10 SDK version 10.0.20348.0 or more recent.\n"
                      "Please download and install the most recent Windows platform SDK.")
endif()

add_compile_options(
  /W3 /utf-8 "$<$<COMPILE_LANG_AND_ID:C,MSVC>:/MP>" "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/MP>"
  "$<$<COMPILE_LANG_AND_ID:C,Clang>:${_obs_clang_c_options}>"
  "$<$<COMPILE_LANG_AND_ID:CXX,Clang>:${_obs_clang_cxx_options}>")

add_compile_definitions(UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS $<$<CONFIG:DEBUG>:DEBUG>
                        $<$<CONFIG:DEBUG>:_DEBUG>)

add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:REF>" "$<$<CONFIG:Debug>:/INCREMENTAL:NO>"
                 "$<$<CONFIG:RelWithDebInfo>:/INCREMENTAL:NO>" "$<$<CONFIG:RelWithDebInfo>:/OPT:ICF>")

if(CMAKE_COMPILE_WARNING_AS_ERROR)
  add_link_options(/WX)
endif()
