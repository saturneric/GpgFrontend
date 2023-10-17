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

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# define libgpgfrontend_module_sdk
aux_source_directory(sdk MODULE_SDK_SOURCE)

add_library(gpgfrontend_module_sdk SHARED ${MODULE_SDK_SOURCE})
set(_export_file_sdk "${CMAKE_CURRENT_SOURCE_DIR}/sdk/GpgFrontendModuleSDKExport.h")
generate_export_header(gpgfrontend_module_sdk EXPORT_FILE_NAME "${_export_file_sdk}")
target_include_directories(gpgfrontend_module_sdk PUBLIC 
        sdk 
        ${CMAKE_CURRENT_BINARY_DIR}/gpgfrontend_module_sdk_autogen/include
        ${CMAKE_SOURCE_DIR}/third_party/spdlog/include)

# link module system
target_link_libraries(gpgfrontend_module_sdk
        PUBLIC gpgfrontend_core)

# tracking integrated modules
set(all_integrated_module_libraries "")
file(GLOB children LIST_DIRECTORIES true "integrated/*")
foreach(child ${children})
    if(IS_DIRECTORY ${child})
        get_filename_component(dirName ${child} NAME)
        add_subdirectory("integrated/${dirName}")

        string(REPLACE "_module" "" stripped_module ${dirName})
        set(integrated_lib_name "gpgfrontend_integrated_module_${stripped_module}")
        list(APPEND all_integrated_module_libraries ${integrated_lib_name})
    endif()
endforeach()

aux_source_directory(. MODULE_SOURCE)
add_library(gpgfrontend_module SHARED ${MODULE_SOURCE})

set(_export_file "${CMAKE_CURRENT_SOURCE_DIR}/GpgFrontendModuleExport.h")
generate_export_header(gpgfrontend_module EXPORT_FILE_NAME "${_export_file}")

# set up pch
target_precompile_headers(gpgfrontend_module PUBLIC GpgFrontendModule.h)

# add ui generator include path
target_include_directories(gpgfrontend_module PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/gpgfrontend_module_autogen/include
        ${CMAKE_SOURCE_DIR}/third_party/spdlog/include)

# link gpgfrontend_module_sdk
target_link_libraries(gpgfrontend_module PRIVATE gpgfrontend_module_sdk)

# link all integrated modules
message(STATUS "All Module Libraries: ${all_integrated_module_libraries}")
target_link_libraries(gpgfrontend_module PRIVATE ${all_integrated_module_libraries})

# using std c++ 17
target_compile_features(gpgfrontend_module PUBLIC cxx_std_17)