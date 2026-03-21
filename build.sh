#!/bin/bash
set -e

# ==============================================================================
# SmartVision Cross-Compilation Build Script
# This script uses Docker to cross-compile the RTSP Server for Rockchip RK3576
# without polluting the host machine's environment.
# ==============================================================================

IMAGE_NAME="rtsp-cross-compiler"
BUILD_DIR="build_aarch64"

# 1. Ensure the Docker image exists. If not, build it.
if [[ "$(docker images -q ${IMAGE_NAME} 2> /dev/null)" == "" ]]; then
    echo "Docker image '${IMAGE_NAME}' not found. Building it now..."
    docker build -t ${IMAGE_NAME} .
else
    echo "Using existing Docker image: ${IMAGE_NAME}"
fi

# 2. Re-create the isolated build directory
mkdir -p ${BUILD_DIR}

echo "Starting cross-compilation inside Docker container..."

# 3. Run CMake and Make inside the container
# The container mounts the current directory $(pwd) to /work
docker run --rm \
    --user $(id -u):$(id -g) \
    -v "$(pwd)":/work \
    ${IMAGE_NAME} \
    bash -c "
        cd /work/${BUILD_DIR} && \
        cmake .. -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc && \
        make -j\$(nproc)
    "

echo -e "\n========================================================"
echo "Build finished successfully!"
echo "The cross-compiled executable is located at: ./${BUILD_DIR}/rtsp_server"
echo "You can copy this file to your Rockchip board via SCP."
echo "========================================================"
