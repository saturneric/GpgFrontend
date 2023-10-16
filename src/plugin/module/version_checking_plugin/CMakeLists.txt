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


aux_source_directory(. INTEGRATED_PLUGIN_SOURCE)

# define libgpgfrontend_plugin
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
add_library(gpgfrontend_integrated_plugin_version_checking SHARED ${INTEGRATED_PLUGIN_SOURCE})

# link sdk
target_link_libraries(gpgfrontend_integrated_plugin_version_checking PRIVATE
        gpgfrontend_plugin_sdk)

# link Qt
if(Qt6_DIR)
        target_link_libraries(gpgfrontend_integrated_plugin_version_checking PRIVATE Qt6::Network)
else()
        target_link_libraries(gpgfrontend_integrated_plugin_version_checking PRIVATE Qt5::Network)
endif()

# using std c++ 17
target_compile_features(gpgfrontend_integrated_plugin_version_checking PRIVATE cxx_std_17)