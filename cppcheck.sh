#!/bin/bash
set -e

# ==============================================================================
# SmartVision Static Analysis Script
# This script runs cppcheck inside the Docker container using compile_commands.json
# to ensure precise analysis including all OpenCV and GStreamer dependencies.
# ==============================================================================

IMAGE_NAME="smartvision_cross_compiler"
BUILD_DIR="build_aarch64"
REPORT_DIR="cppcheck_reports"

# 1. Build/update the Docker image (uses cache if no Dockerfile changes exist)
echo "Ensuring Docker image '${IMAGE_NAME}' is built and up-to-date..."
docker build -t ${IMAGE_NAME} .

# 2. Re-create directories on the host
mkdir -p ${REPORT_DIR}
mkdir -p ${BUILD_DIR}

echo "Running static analysis inside the cross-compiler container..."

# 3. Run CMake configuration to generate compile_commands.json, then run cppcheck
docker run --rm \
    --user $(id -u):$(id -g) \
    -v "$(pwd)":/work \
    ${IMAGE_NAME} \
    bash -c "
        # Generate the compilation database (compile_commands.json)
        cd /work/${BUILD_DIR} && \
        cmake .. -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
                 -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
                 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && \
        cd /work && \
        
        # Run cppcheck using the compilation database
        cppcheck --project=${BUILD_DIR}/compile_commands.json \
                 --enable=all \
                 --inconclusive \
                 --xml \
                 --xml-version=2 \
                 --output-file=${REPORT_DIR}/report.xml \
                 --suppress=missingIncludeSystem \
                 -i smartvision_system/3rdparty \
                 -i ${BUILD_DIR} && \
        
        # Generate the HTML report
        cppcheck-htmlreport --file=${REPORT_DIR}/report.xml \
                            --report-dir=${REPORT_DIR} \
                            --source-dir=.
    "

echo -e "\n========================================================"
echo "Static analysis completed!"
echo "The HTML report is located at: ./${REPORT_DIR}/index.html"
echo "========================================================"
