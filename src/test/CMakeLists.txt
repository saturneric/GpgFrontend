# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

# Set configure for test
include(CTest)
enable_testing()
find_package(GTest REQUIRED)

aux_source_directory(./core TEST_SOURCE)
aux_source_directory(. TEST_SOURCE)

# define test library
register_library(test LIBRARY_TARGET ${TEST_SOURCE})

# set up pch
target_precompile_headers(${LIBRARY_TARGET} PUBLIC GpgFrontendTest.h)

# link options
target_link_libraries(${LIBRARY_TARGET} PRIVATE GTest::gtest)
target_link_libraries(${LIBRARY_TARGET} PRIVATE gf_core)

add_test(AllTestsInGpgFrontend ${LIBRARY_TARGET})
