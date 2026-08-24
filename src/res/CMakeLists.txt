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

# gather source files (GpgFrontendResource.cpp -- satisfies register_library's
# >=1 source requirement)
aux_source_directory(. RES_SOURCE)

# register the resource library (target gf_res)
register_library(res LIBRARY_TARGET ${RES_SOURCE})

# let the executable (and anyone linking gf_res) find GpgFrontendResource.h;
# GFResExport.h's dir (CMAKE_CURRENT_BINARY_DIR) is already PUBLIC via
# register_library.
target_include_directories(${LIBRARY_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# gf_res only needs QtCore (Q_INIT_RESOURCE + resource machinery)
target_link_libraries(${LIBRARY_TARGET} PRIVATE Qt::Core)

# --- primary static resources ---
# Split into focused manifests for easier management (see resource/qrc/):
#   resource/qrc/icons.qrc -> /icons           (UI icons + app .ico)
#   resource/qrc/test.qrc  -> /test/*          (gpg/rpgp test keys and RFC 9580 vectors)
#   resource/qrc/app.qrc   -> / (TRANSLATORS)  (shipped app metadata)
#   resource/qrc/fonts.qrc -> /fonts           (bundled JetBrains Mono NL + OFL)
# CMAKE_AUTORCC is globally ON, so listing each .qrc as a source RCC-compiles it
# with base name = file stem -> qInitResources_icons / _test / _app / _fonts,
# matching the Q_INIT_RESOURCE calls in GpgFrontendResource.cpp. rcc resolves
# each <file> body relative to the .qrc's own directory (resource/qrc/), so the
# "../lfs/..." paths (and the "../../" paths to the root-level .ico /
# TRANSLATORS) stay valid even though the qrc is referenced from src/res.
target_sources(${LIBRARY_TARGET} PRIVATE
  ${CMAKE_SOURCE_DIR}/resource/qrc/icons.qrc
  ${CMAKE_SOURCE_DIR}/resource/qrc/test.qrc
  ${CMAKE_SOURCE_DIR}/resource/qrc/app.qrc
  ${CMAKE_SOURCE_DIR}/resource/qrc/fonts.qrc)

# --- translations (moved out of the executable) ---
set(LOCALE_TS_PATH ${CMAKE_SOURCE_DIR}/resource/lfs/locale/ts)
# Derived from the shared locale list (cmake/Translations.cmake) so the app and
# every module ship the same set of languages.
gpgfrontend_collect_ts_files(TS_FILES "GpgFrontend" "${LOCALE_TS_PATH}")

if(GPGFRONTEND_QT5_BUILD)
  # TODO
  qt5_add_translation(GF_TRANSLATIONS_QM ${TS_FILES})
  message(STATUS "GF_TRANSLATIONS_QM ${GF_TRANSLATIONS_QM}")

  # Name the generated qrc "gftranslations.qrc" so the init symbol is
  # qInitResources_gftranslations, matching the Qt6 path and the shim in
  # GpgFrontendResource.cpp. The internal prefix stays "i18n", so the runtime
  # ":/i18n" paths are unchanged.
  set(GF_QM_TRANSLATIONS_RESOURCE_FILE "${CMAKE_CURRENT_BINARY_DIR}/gftranslations.qrc")
  file(WRITE ${GF_QM_TRANSLATIONS_RESOURCE_FILE} "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n  <qresource prefix=\"i18n\">\n")

  foreach(QM_FILE ${GF_TRANSLATIONS_QM})
    file(RELATIVE_PATH QM_FILENAME ${CMAKE_CURRENT_BINARY_DIR} ${QM_FILE})
    file(APPEND ${GF_QM_TRANSLATIONS_RESOURCE_FILE} "    <file alias=\"${QM_FILENAME}\">${QM_FILE}</file>\n")
  endforeach()

  file(APPEND ${GF_QM_TRANSLATIONS_RESOURCE_FILE} "  </qresource>\n</RCC>\n")
  qt5_add_resources(GF_I18N_RESOURCES ${GF_QM_TRANSLATIONS_RESOURCE_FILE})

  # Absolute paths (current source dir is src/res, not the qttranslations dir).
  file(GLOB QT_TRANSLATIONS_TS ${CMAKE_SOURCE_DIR}/third_party/qttranslations/translations/*.ts)
  list(FILTER QT_TRANSLATIONS_TS INCLUDE REGEX ".*(qt|qtbase)_.*\.ts$")
  qt5_add_translation(QT_TRANSLATIONS_QM ${QT_TRANSLATIONS_TS})

  set(QT_QM_TRANSLATIONS_RESOURCE_FILE "${CMAKE_CURRENT_BINARY_DIR}/qttranslations.qrc")
  file(WRITE ${QT_QM_TRANSLATIONS_RESOURCE_FILE} "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n  <qresource prefix=\"qt_i18n\">\n")

  foreach(QM_FILE ${QT_TRANSLATIONS_QM})
    file(RELATIVE_PATH QM_FILENAME ${CMAKE_CURRENT_BINARY_DIR} ${QM_FILE})
    file(APPEND ${QT_QM_TRANSLATIONS_RESOURCE_FILE} "    <file alias=\"${QM_FILENAME}\">${QM_FILE}</file>\n")
  endforeach()

  file(APPEND ${QT_QM_TRANSLATIONS_RESOURCE_FILE} "  </qresource>\n</RCC>\n")
  qt5_add_resources(GF_I18N_RESOURCES ${QT_QM_TRANSLATIONS_RESOURCE_FILE})

  message(STATUS "GF_I18N_RESOURCES ${GF_I18N_RESOURCES}")
  target_sources(${LIBRARY_TARGET} PRIVATE ${GF_I18N_RESOURCES})
else()
  add_custom_target(gftranslations ALL)

  qt_add_lrelease(gftranslations
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE GF_TRANSLATIONS_QM)

  qt_add_resources(${LIBRARY_TARGET} "gftranslations"
    PREFIX "/i18n"
    BASE ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${GF_TRANSLATIONS_QM})

  add_custom_target(qttranslations ALL)

  # Absolute paths (current source dir is src/res, not the qttranslations dir).
  file(GLOB QT_TRANSLATIONS_TS ${CMAKE_SOURCE_DIR}/third_party/qttranslations/translations/*.ts)
  list(FILTER QT_TRANSLATIONS_TS INCLUDE REGEX ".*(qt|qtbase)_.*\.ts$")
  qt_add_lrelease(qttranslations
    TS_FILES ${QT_TRANSLATIONS_TS}
    QM_FILES_OUTPUT_VARIABLE QT_TRANSLATIONS_QM)
  qt_add_resources(${LIBRARY_TARGET} "qttranslations"
    PREFIX "/i18n_qt"
    BASE ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${QT_TRANSLATIONS_QM})
endif()
