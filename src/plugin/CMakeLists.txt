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

# define libgpgfrontend_plugin_sdk
aux_source_directory(sdk PLUGIN_SDK_SOURCE)
add_library(gpgfrontend_plugin_sdk SHARED ${PLUGIN_SDK_SOURCE})
set(_export_file "${CMAKE_CURRENT_SOURCE_DIR}/sdk/GpgFrontendPluginSDKExport.h")
generate_export_header(gpgfrontend_plugin_sdk EXPORT_FILE_NAME "${_export_file}")
target_include_directories(gpgfrontend_plugin_sdk PUBLIC sdk)

# link json
target_link_libraries(gpgfrontend_plugin_sdk
        PUBLIC nlohmann_json::nlohmann_json)

# spdlog
target_link_libraries(gpgfrontend_plugin_sdk PUBLIC spdlog)

# link Qt core
if(Qt6_DIR)
    target_link_libraries(gpgfrontend_plugin_sdk PUBLIC Qt6::Core)
else()
    target_link_libraries(gpgfrontend_plugin_sdk PUBLIC Qt5::Core)
endif()

# link core and ui module
target_link_libraries(gpgfrontend_plugin_sdk PRIVATE
        gpgfrontend_core gpgfrontend_ui)

# tracking integrated plugins
set(all_integrated_plugins_libraries "")
file(GLOB children LIST_DIRECTORIES true "integrated_plugins/*")
foreach(child ${children})
    if(IS_DIRECTORY ${child})
        get_filename_component(dirName ${child} NAME)
        add_subdirectory("integrated_plugins/${dirName}")

        string(REPLACE "_plugin" "" stripped_plugin ${dirName})
        set(integrated_lib_name "gpgfrontend_integrated_plugin_${stripped_plugin}")
        list(APPEND all_integrated_plugins_libraries ${integrated_lib_name})
    endif()
endforeach()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
aux_source_directory(. PLUGIN_SOURCE)
add_library(gpgfrontend_plugin SHARED ${PLUGIN_SOURCE})

set(_export_file "${CMAKE_CURRENT_SOURCE_DIR}/GpgFrontendPluginExport.h")
generate_export_header(gpgfrontend_plugin EXPORT_FILE_NAME "${_export_file}")

# set up pch
target_precompile_headers(gpgfrontend_plugin PUBLIC GpgFrontendPlugin.h)

# add ui generator include path
target_include_directories(gpgfrontend_plugin PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/gpgfrontend_plugin_autogen/include
        ${CMAKE_SOURCE_DIR}/third_party/spdlog/include)

target_link_libraries(gpgfrontend_plugin PRIVATE
        gpgfrontend_core gpgfrontend_ui)

# link gpgfrontend_plugin_sdk
target_link_libraries(gpgfrontend_plugin PRIVATE
       gpgfrontend_plugin_sdk)

# link all integrated plugins
message(STATUS "All Integrated Plugin Libraries: ${all_integrated_plugins_libraries}")
target_link_libraries(gpgfrontend_plugin PRIVATE ${all_integrated_plugins_libraries})

# using std c++ 17
target_compile_features(gpgfrontend_plugin PUBLIC cxx_std_17)