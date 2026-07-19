FROM debian:bookworm

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# 1. Enable arm64 architecture for multiarch support
RUN dpkg --add-architecture arm64

# 2. Configure apt sources for both amd64 (host tools) and arm64 (target libraries).
#    We overwrite sources completely to handle both old one-liner and new DEB822 formats
#    that may exist in different versions of the debian:bookworm Docker image.
RUN rm -f /etc/apt/sources.list.d/*.sources && \
    echo "deb [arch=amd64] http://deb.debian.org/debian bookworm main contrib non-free" > /etc/apt/sources.list && \
    echo "deb [arch=amd64] http://deb.debian.org/debian bookworm-updates main contrib non-free" >> /etc/apt/sources.list && \
    echo "deb [arch=amd64] http://deb.debian.org/debian-security bookworm-security main contrib non-free" >> /etc/apt/sources.list && \
    echo "deb [arch=arm64] http://deb.debian.org/debian bookworm main contrib non-free" >> /etc/apt/sources.list && \
    echo "deb [arch=arm64] http://deb.debian.org/debian bookworm-updates main contrib non-free" >> /etc/apt/sources.list && \
    echo "deb [arch=arm64] http://deb.debian.org/debian-security bookworm-security main contrib non-free" >> /etc/apt/sources.list

# 3. Install host tools (compilers, build system — amd64)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    crossbuild-essential-arm64 \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    rsync \
    cppcheck \
    python3 \
    python3-pygments

# 4. Install target (arm64) libraries — same versions as on the Radxa board (Debian 12)
RUN apt-get install -y \
    libgstreamer1.0-dev:arm64 \
    libgstreamer-plugins-base1.0-dev:arm64 \
    libgstreamer-plugins-bad1.0-dev:arm64 \
    libgstrtspserver-1.0-dev:arm64 \
    libopencv-dev:arm64

# 5. Tell pkg-config to look for the arm64 libraries instead of amd64
ENV PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig
ENV PKG_CONFIG_LIBDIR=/usr/lib/aarch64-linux-gnu/pkgconfig

WORKDIR /work