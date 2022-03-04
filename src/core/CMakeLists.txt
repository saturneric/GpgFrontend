#
# Copyright (C) 2021 Saturneric
#
# This file is part of GpgFrontend.
#
# GpgFrontend is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GpgFrontend is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
#
# The initial version of the source code is inherited from
# the gpg4usb project, which is under GPL-3.0-or-later.
#
# All the source code of GpgFrontend was modified and released by
# Saturneric<eric@bktus.com> starting on May 12, 2021.
#
# SPDX-License-Identifier: GPL-3.0-or-later

aux_source_directory(./function/result_analyse GPG_SOURCE)
aux_source_directory(./function/gpg GPG_SOURCE)
aux_source_directory(./function/aes GPG_SOURCE)
aux_source_directory(./function GPG_SOURCE)
aux_source_directory(./model GPG_SOURCE)
aux_source_directory(. GPG_SOURCE)

# define libgpgfrontend_core
add_library(gpgfrontend_core STATIC ${GPG_SOURCE})

set(UTILS_DIR ${CMAKE_SOURCE_DIR}/utils)
set(GPGME_LIB_DIR ${UTILS_DIR}/gpgme/lib)

# link third-party libraries
target_link_libraries(gpgfrontend_core easyloggingpp config++)
# link boost libraries
target_link_libraries(gpgfrontend_core Boost::date_time Boost::system)
# link gnupg libraries
target_link_libraries(gpgfrontend_core gpgme assuan gpg-error)
# link openssl
target_link_libraries(gpgfrontend_core OpenSSL::SSL OpenSSL::Crypto)
# link Qt AES
target_link_libraries(gpgfrontend_core QtAES)
# link vmime
if (NOT LINUX)
    # macOS
    target_link_libraries(gpgfrontend_core
            gpgfrontend_vmime intl iconv)
    if (MINGW)
        target_link_libraries(gpgfrontend_core ws2_32)
    endif ()
else ()
    target_link_libraries(gpgfrontend_core
            gpgfrontend_vmime anl ssl crypto)
endif ()
# link libarchive
if (MINGW)
    find_library(LIBARCHIVE_LIB libarchive.a)
    target_link_libraries(gpgfrontend_core ${LIBARCHIVE_LIB})
else ()
    target_link_libraries(gpgfrontend_core archive_static)
endif ()
# link json
target_link_libraries(gpgfrontend_core
        nlohmann_json::nlohmann_json)
# link Qt core
target_link_libraries(gpgfrontend_core Qt5::Core)

# set up pch
target_precompile_headers(gpgfrontend_core
        PUBLIC ${CMAKE_SOURCE_DIR}/src/GpgFrontend.h
        PUBLIC GpgFrontendCore.h)

# using std c++ 17
target_compile_features(gpgfrontend_core PUBLIC cxx_std_17)

# link for different platforms
if (MINGW)
    message(STATUS "Link GPG Static Library For MINGW")
    target_link_libraries(gpgfrontend_core wsock32)
elseif (APPLE)
    message(STATUS "Link GPG Static Library For macOS")
    target_link_libraries(gpgfrontend_core dl)
    if (XCODE_BUILD)
        set_target_properties(gpgfrontend_core
                PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    endif ()
else ()
    # linux
    message(STATUS "Link GPG Static Library For Unix")
    target_link_libraries(gpgfrontend_core pthread dl)
endif ()
