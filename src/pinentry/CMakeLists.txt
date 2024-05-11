# Copyright (C) 2021 Saturneric <eric@bktus.com>
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
# Saturneric <eric@bktus.com> starting on May 12, 2021.
#
# SPDX-License-Identifier: GPL-3.0-or-later

aux_source_directory(. PINENTRY_SOURCE)

# capslock
list(APPEND PINENTRY_SOURCE "capslock/capslock.cpp")
if (MINGW)
  list(APPEND PINENTRY_SOURCE "capslock/capslock_win.cpp")
else()
  list(APPEND PINENTRY_SOURCE "capslock/capslock_unix.cpp")
endif()

add_library(gpgfrontend_pinentry SHARED ${PINENTRY_SOURCE})

target_link_libraries(gpgfrontend_pinentry PUBLIC gpgfrontend_core)

if(GPGFRONTEND_QT5_BUILD)
  # link Qt core
  target_link_libraries(gpgfrontend_pinentry PUBLIC Qt5::Widgets)
else()
  # link Qt core
  target_link_libraries(gpgfrontend_pinentry PUBLIC Qt6::Widgets)
endif()

if (XCODE_BUILD)
  set_target_properties(gpgfrontend_pinentry
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    XCODE_ATTRIBUTE_SKIP_INSTALL "Yes"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XOCDE_CODE_SIGN_IDENTITY}")
endif ()

# spdlog
target_link_libraries(gpgfrontend_pinentry PRIVATE spdlog)

# using std c++ 17
target_compile_features(gpgfrontend_pinentry PUBLIC cxx_std_17)
