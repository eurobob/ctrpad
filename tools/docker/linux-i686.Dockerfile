# syntax=docker/dockerfile:1

# Pin the image used for the first retail-parity baseline. The amd64 userspace
# is deliberate: ctr-native's supported Linux target is i686, while the
# development host is ARM64.
FROM ubuntu@sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90

ARG DEBIAN_FRONTEND=noninteractive

RUN dpkg --add-architecture i386 \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        file \
        gcc-multilib \
        git \
        libasound2-dev:i386 \
        libdbus-1-dev:i386 \
        libgl1-mesa-dev:i386 \
        libudev-dev:i386 \
        libx11-dev:i386 \
        libxcursor-dev:i386 \
        libxext-dev:i386 \
        libxfixes-dev:i386 \
        libxi-dev:i386 \
        libxrandr-dev:i386 \
        ninja-build \
        pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Headless X11 is used only for deterministic baseline capture on CI or an
# ARM64 development host. The game itself remains an unchanged 32-bit X11/GL
# client.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        xauth \
        xdotool \
        xvfb \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
