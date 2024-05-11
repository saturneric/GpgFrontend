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

# com.bktus.gpgfrontend.module.integrated.gnupg_info_gathering

aux_source_directory(. INTEGRATED_MODULE_SOURCE)

# define libgpgfrontend_module
add_library(mod_gpg_info SHARED ${INTEGRATED_MODULE_SOURCE})
set(_export_file "${CMAKE_CURRENT_SOURCE_DIR}/GFModuleExport.h")
generate_export_header(mod_gpg_info
  BASE_NAME "GF_MODULE"
  EXPORT_FILE_NAME "${_export_file}")

target_include_directories(mod_gpg_info PRIVATE 
  ${CMAKE_SOURCE_DIR}/third_party/spdlog/include)

# set output directory
set_target_properties(mod_gpg_info PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mods
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mods)

if (XCODE_BUILD)
  set_target_properties(mod_gpg_info
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    XCODE_ATTRIBUTE_SKIP_INSTALL "Yes"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XOCDE_CODE_SIGN_IDENTITY}")
endif ()

if (LINUX AND LINUX_INSTALL_SOFTWARE)
  install(TARGETS mod_gpg_info 
    LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/mods/")
endif()

# link sdk
target_link_libraries(mod_gpg_info PRIVATE
  gpgfrontend_module_sdk)

if(GPGFRONTEND_QT5_BUILD)
  # link Qt core
  target_link_libraries(mod_gpg_info PRIVATE Qt5::Core)
else()
  # link Qt core
  target_link_libraries(mod_gpg_info PRIVATE Qt6::Core)
endif()

# property
set_property(TARGET mod_gpg_info PROPERTY AUTOMOC ON)

# using std c++ 17
target_compile_features(mod_gpg_info PRIVATE cxx_std_17)