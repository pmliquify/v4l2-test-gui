cmake_minimum_required(VERSION 3.16)

project(V4L2-Test
        VERSION 0.3.0
        DESCRIPTION "V4L2 testing application"
        HOMEPAGE_URL "https://github.com/pmliquify/v4l2-test")
configure_file (
        "${PROJECT_SOURCE_DIR}/src/version.h.in"
        "${PROJECT_BINARY_DIR}/src/version.h"
        )

add_subdirectory(src)