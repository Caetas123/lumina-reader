if(NOT DEFINED RELEASE_TAG OR RELEASE_TAG STREQUAL "")
    message(FATAL_ERROR "RELEASE_TAG is required, for example v0.2.0.")
endif()

if(NOT RELEASE_TAG MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "Release tag '${RELEASE_TAG}' is not a semantic version tag such as v0.2.0.")
endif()

if(NOT DEFINED PROJECT_BUILD_DIR OR PROJECT_BUILD_DIR STREQUAL "")
    message(FATAL_ERROR "PROJECT_BUILD_DIR is required and must point to a configured build directory.")
endif()

set(cache_file "${PROJECT_BUILD_DIR}/CMakeCache.txt")
if(NOT EXISTS "${cache_file}")
    message(FATAL_ERROR "CMake cache not found at '${cache_file}'. Configure the project before checking the release version.")
endif()

file(READ "${cache_file}" cache_contents)
string(REGEX MATCH "CMAKE_PROJECT_VERSION:STATIC=([0-9]+\\.[0-9]+\\.[0-9]+)" _version_match "${cache_contents}")
if(NOT _version_match)
    message(FATAL_ERROR "The configured project version could not be read from '${cache_file}'.")
endif()

set(project_version "${CMAKE_MATCH_1}")
string(REGEX REPLACE "^v" "" release_version "${RELEASE_TAG}")

if(NOT project_version STREQUAL release_version)
    message(FATAL_ERROR
        "Release version mismatch: tag '${RELEASE_TAG}' but configured project version is '${project_version}'. "
        "Update project-version.txt before creating the release tag.")
endif()

message(STATUS "Release tag '${RELEASE_TAG}' matches Lumina Reader version '${project_version}'.")
