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
    ccache \
    ninja-build \
    libgtest-dev \
    libarchive-dev \
    libssl-dev \
    libgpgme-dev \
    qt6-base-dev \
    qt6-tools-dev \
    clangd \ 
  && apt-get clean \
    && rm -rf /var/lib/apt/lists/*