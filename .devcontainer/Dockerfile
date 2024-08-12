FROM mcr.microsoft.com/devcontainers/cpp:1-debian-12

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="3.22.2"

# Optionally install the cmake for vcpkg
COPY ./reinstall-cmake.sh /tmp/

RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    git \
    autoconf \
    automake \
    gettext \
    texinfo \
    gcc \
    g++ \
    ninja-build \
    libarchive-dev \
    libssl-dev \
    libgpgme-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install Qt6
RUN git clone https://github.com/jurplel/install-qt-action.git /tmp/install-qt-action && \
    /tmp/install-qt-action/install-qt.sh -v "6.7.2" --no-cache && \
    rm -rf /tmp/install-qt-action