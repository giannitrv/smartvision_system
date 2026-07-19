#!/bin/bash
set -e

# ==============================================================================
# SmartVision Cross-Compilation Build Script
# This script uses Docker to cross-compile the SmartVision System for Rockchip RK3576
# without polluting the host machine's environment.
# ==============================================================================

IMAGE_NAME="smartvision_cross_compiler"
BUILD_DIR="build_aarch64"

# 1. Build/update the Docker image (uses cache if no Dockerfile changes exist)
echo "Ensuring Docker image '${IMAGE_NAME}' is built and up-to-date..."
docker build -t ${IMAGE_NAME} .

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
echo "The cross-compiled executable is located at: ./${BUILD_DIR}/smartvision"
echo "You can copy this file to your Rockchip board via SCP."
echo "========================================================"
