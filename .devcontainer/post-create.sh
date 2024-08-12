#! /bin/bash

git submodule update --recursive --init
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
ln -s build/compile_commands.json $(pwd)/compile_commands.json

# gather informations
PROJECT_VERSION=$(cmake --system-information | awk -F= '$1~/CMAKE_PROJECT_VERSION:STATIC/{print$2}')

# modules development
cmake -B build-sdk -G Ninja -DCMAKE_BUILD_TYPE=Debug -DGPGFRONTEND_BUILD_TYPE_ONLY_SDK=On -DCMAKE_INSTALL_PREFIX=$pwd/modules/sdk/$PROJECT_VERSION
cmake -B modules/build -S modules -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$pwd/build/artifacts 