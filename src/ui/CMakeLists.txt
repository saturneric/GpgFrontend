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

# tracking source files
aux_source_directory(. UI_SOURCE)
aux_source_directory(keypair_details UI_SOURCE)
aux_source_directory(widgets UI_SOURCE)
aux_source_directory(key_generate UI_SOURCE)
aux_source_directory(main_window UI_SOURCE)
aux_source_directory(help UI_SOURCE)
aux_source_directory(settings UI_SOURCE)
aux_source_directory(thread UI_SOURCE)
aux_source_directory(details UI_SOURCE)
aux_source_directory(struct UI_SOURCE)
aux_source_directory(import_export UI_SOURCE)
aux_source_directory(encoding UI_SOURCE)
aux_source_directory(dialog UI_SOURCE)

if (SMTP_SUPPORT)
    aux_source_directory(mail UI_SOURCE)
endif ()

add_library(gpgfrontend_ui STATIC ${UI_SOURCE})
set(GPGFRONTEND_UI_LIB_NAME gpgfrontend_ui)

# link smtp-mime
if (SMTP_SUPPORT)
    target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
            smtp-mime)
endif ()
# link json
target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
        nlohmann_json::nlohmann_json)
# Qt AES
target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
        QtAES)
# link Qt
target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
        Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)

# link vmime
if (NOT LINUX)
    target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
            gpgfrontend_vmime ssl crypto intl iconv)
    if (MINGW)
        target_link_libraries(${GPGFRONTEND_UI_LIB_NAME} ws2_32)
    endif ()
else ()
    target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
            gpgfrontend_vmime anl ssl crypto)
endif ()

# link easyloggingpp
target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
        easyloggingpp)

# link gpgfrontend_core
target_link_libraries(${GPGFRONTEND_UI_LIB_NAME}
        gpgfrontend_core)

# link openssl
target_link_libraries(${GPGFRONTEND_UI_LIB_NAME} OpenSSL::SSL OpenSSL::Crypto)

target_include_directories(gpgfrontend_ui PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/${GPGFRONTEND_UI_LIB_NAME}_autogen/include)


if (XCODE_BUILD)
    set_target_properties(${GPGFRONTEND_UI_LIB_NAME}
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
endif ()
target_compile_features(gpgfrontend_ui PUBLIC cxx_std_17)