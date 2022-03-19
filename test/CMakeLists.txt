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

find_package(GTest REQUIRED)

# Set configure for test
file(COPY ${CMAKE_SOURCE_DIR}/test/conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ FOLLOW_SYMLINK_CHAIN)
file(COPY ${CMAKE_SOURCE_DIR}/test/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ FOLLOW_SYMLINK_CHAIN)

aux_source_directory(. TEST_SOURCE)

add_executable(
        ${AppName}
        ${TEST_SOURCE}
)

if (GPG_CORE)
    target_link_libraries(${AppName} gpg_core)
endif ()

if (GPG_STANDALONE_MODE)
    file(COPY ${CMAKE_SOURCE_DIR}/test/gpg DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ FOLLOW_SYMLINK_CHAIN)
endif ()

target_link_libraries(${AppName} ${Boost_LIBRARIES} gtest gtest_main)
if (APPLE)
    target_link_libraries(${AppName} intl)
endif ()

add_test(AllTestsInGpgFrontend ${AppName})
