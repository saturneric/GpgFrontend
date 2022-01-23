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

aux_source_directory(./result_analyse GPG_SOURCE)
aux_source_directory(./function GPG_SOURCE)
aux_source_directory(./model GPG_SOURCE)
aux_source_directory(. GPG_SOURCE)

add_library(gpgfrontend_core STATIC ${GPG_SOURCE})

set(UTILS_DIR ${CMAKE_SOURCE_DIR}/utils)

set(GPGME_LIB_DIR ${UTILS_DIR}/gpgme/lib)

set(THIRD_PARTY_LIBS easyloggingpp config++)
set(BOOST_LIBS Boost::date_time Boost::filesystem)

if (MINGW)
    message(STATUS "Link GPG Static Library For MINGW")
    target_link_libraries(gpgfrontend_core ${THIRD_PARTY_LIBS}
            ${BOOST_LIBS}
            gpgme assuan gpg-error wsock32)
    target_compile_features(gpgfrontend_core PUBLIC cxx_std_17)
elseif (APPLE)
    message(STATUS "Link GPG Static Library For macOS")
    target_link_libraries(gpgfrontend_core ${THIRD_PARTY_LIBS}
            gpgme assuan gpg-error
            ${BOOST_LIBS}
            dl)
    if (XCODE_BUILD)
        set_target_properties(gpgfrontend_core
                PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    endif ()
else ()
    message(STATUS "Link GPG Static Library For Unix")
    target_link_libraries(gpgfrontend_core ${THIRD_PARTY_LIBS}
            gpgme assuan gpg-error
            ${BOOST_LIBS}
            pthread dl)
endif ()
