# The MIT License (MIT)
#
# Copyright (c) 2016-2017 Theo Willows
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.0.0)

include(CMakeParseArguments)

function(version_from_git)
  # Parse arguments
  set(options OPTIONAL FAST)
  set(oneValueArgs GIT_EXECUTABLE INCLUDE_HASH LOG TIMESTAMP)
  set(multiValueArgs)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})

  # Defaults
  if(NOT DEFINED ARG_INCLUDE_HASH)
    set(ARG_INCLUDE_HASH ON)
  endif()

  if(DEFINED ARG_GIT_EXECUTABLE)
    set(GIT_EXECUTABLE "${ARG_GIT_EXECUTABLE}")
  else()
    # Find Git or bail out
    find_package(Git)
    if(NOT GIT_FOUND)
      message(FATAL_ERROR "[MunkeiVersionFromGit] Git not found")
    endif(NOT GIT_FOUND)
  endif()

  # Git describe
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE git_result
    OUTPUT_VARIABLE git_describe
    ERROR_VARIABLE git_error
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE)
  if(NOT git_result EQUAL 0)
    message(
      FATAL_ERROR "[MunkeiVersionFromGit] Failed to execute Git: ${git_error}")
  endif()

  # Get Git tag
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags --abbrev=0
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE git_result
    OUTPUT_VARIABLE git_tag
    ERROR_VARIABLE git_error
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE)
  if(NOT git_result EQUAL 0)
    message(
      FATAL_ERROR "[MunkeiVersionFromGit] Failed to execute Git: ${git_error}")
  endif()

  if(git_tag
     MATCHES
     "^v(0|[1-9][0-9]*)[.](0|[1-9][0-9]*)[.](0|[1-9][0-9]*)(-[.0-9A-Za-z-]+)?([+][.0-9A-Za-z-]+)?$"
  )
    set(version_major "${CMAKE_MATCH_1}")
    set(version_minor "${CMAKE_MATCH_2}")
    set(version_patch "${CMAKE_MATCH_3}")
    set(identifiers "${CMAKE_MATCH_4}")
    set(metadata "${CMAKE_MATCH_5}")
  else()
    message(
      FATAL_ERROR
        "[MunkeiVersionFromGit] Git tag isn't valid semantic version: [${git_tag}]"
    )
  endif()

  if("${git_tag}" STREQUAL "${git_describe}")
    set(git_at_a_tag ON)
  endif()

  if(NOT git_at_a_tag)
    # Extract the Git hash (if one exists)
    string(REGEX MATCH "g[0-9a-f]+$" git_hash "${git_describe}")
  endif()

  # Construct the version variables
  set(version ${version_major}.${version_minor}.${version_patch})
  set(semver ${version})

  # Identifiers
  if(identifiers MATCHES ".+")
    string(SUBSTRING "${identifiers}" 1 -1 identifiers)
    set(semver "${semver}-${identifiers}")
  endif()

  # Metadata TODO Split and join (add Git hash inbetween)
  if(metadata MATCHES ".+")
    string(SUBSTRING "${metadata}" 1 -1 metadata)
    # Split
    string(REPLACE "." ";" metadata "${metadata}")
  endif()

  if(NOT git_at_a_tag)

    if(ARG_INCLUDE_HASH)
      list(APPEND metadata "${git_hash}")
    endif(ARG_INCLUDE_HASH)

    # Timestamp
    if(DEFINED ARG_TIMESTAMP)
      string(TIMESTAMP timestamp "${ARG_TIMESTAMP}" ${ARG_UTC})
      list(APPEND metadata "${timestamp}")
    endif(DEFINED ARG_TIMESTAMP)

  endif()

  # Join
  string(REPLACE ";" "." metadata "${metadata}")

  if(metadata MATCHES ".+")
    set(semver "${semver}+${metadata}")
  endif()

  # Log the results
  if(ARG_LOG)
    message(
      STATUS
        "[MunkeiVersionFromGit] Version: ${version}
     Git tag:     [${git_tag}]
     Git hash:    [${git_hash}]
     Decorated:   [${git_describe}]
     Identifiers: [${identifiers}]
     Metadata:    [${metadata}]
     SemVer:      [${semver}]")
  endif(ARG_LOG)

  # Set parent scope variables
  set(GIT_TAG
      ${git_tag}
      PARENT_SCOPE)
  set(SEMVER
      ${semver}
      PARENT_SCOPE)
  set(VERSION
      ${version}
      PARENT_SCOPE)
  set(VERSION_MAJOR
      ${version_major}
      PARENT_SCOPE)
  set(VERSION_MINOR
      ${version_minor}
      PARENT_SCOPE)
  set(VERSION_PATCH
      ${version_patch}
      PARENT_SCOPE)

endfunction(version_from_git)
