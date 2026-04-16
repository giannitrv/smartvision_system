cmake_minimum_required(VERSION 3.10)
project(SmartVisionRTSP LANGUAGES CXX)

# Set standard C++ features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure executables and libraries are placed in the top-level build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find PkgConfig (required to find GStreamer)
find_package(PkgConfig REQUIRED)

# Find OpenCV via pkg-config — more reliable than find_package for arm64 cross-compilation
# because it uses the PKG_CONFIG_PATH already set in the Docker environment.
pkg_check_modules(OpenCV REQUIRED opencv4)

# Use pkg-config to find GStreamer libraries
pkg_check_modules(GST            REQUIRED gstreamer-1.0)
pkg_check_modules(GST_RTSP_SERVER REQUIRED gstreamer-rtsp-server-1.0)
pkg_check_modules(GST_APP        REQUIRED gstreamer-app-1.0)

# Source files and targets moved to subdirectories
if(NOT TARGET_SOC)
    set(TARGET_SOC "rk3576") # Default to RK3576 as used in build.sh
    message(STATUS "TARGET_SOC not set, defaulting to ${TARGET_SOC}")
endif()

add_subdirectory(smartvision_system/3rdparty)
add_subdirectory(smartvision_system/utils)
add_subdirectory(smartvision_system/ai)
add_subdirectory(smartvision_system/pantilt)
add_subdirectory(smartvision_system/core)
