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

# gf_mob, the phone application.
#
# Everything but main() is in gf_mob_lib, the target of the one QML module
# (GpgFrontend.Mobile): the application and its tests link the same library and
# import the same plugin, so they register the same types.

include(${CMAKE_SOURCE_DIR}/src/plat/PlatformSources.cmake)

qt_add_library(gf_mob_lib STATIC
  AppServices.cpp
  startup/InteractionBridge.cpp
  startup/MobileProfileLoaderDelegate.cpp
  startup/StartupController.cpp
  lifecycle/AppLifecycle.cpp
  model/KeyRoleModel.cpp
  model/KeyFilterModel.cpp
  model/KeyListController.cpp
  model/KeyImportController.cpp
  model/KeyActionsController.cpp
  model/ErrorWording.cpp
  model/KeyGenProfilesController.cpp
  model/KeyCategoriesController.cpp
  model/KeyGroupsController.cpp
  model/AboutInfo.cpp
  model/KeyDatabasesController.cpp
  model/ProfilesController.cpp
  model/ReportSeal.cpp
  model/SettingsModel.cpp
  model/ChangeRelay.h
  crypto/ContentKind.cpp
  crypto/KeySelection.cpp
  crypto/OperationLog.cpp
  crypto/OperationQueue.cpp
  crypto/OutputActions.cpp
  crypto/PassphraseBroker.cpp
  crypto/ResultPresenter.cpp
  crypto/TextCryptoController.cpp
  crypto/EditorHistory.cpp
  crypto/WorkSelection.cpp
  files/StagingArea.cpp
  files/ShareRetentionStore.cpp
  files/FileOperations.cpp
  files/FileFilterModel.cpp
  files/FileWorkspace.cpp
  files/ShareInbox.cpp
  ${GF_PLATFORM_APP_SOURCES}
  ${GF_PLATFORM_MOBILE_SOURCES})

# What the style does not give: GpgFrontend's own touches (the desktop's
# status colours, see src/ui/function/UIStyle.cpp), spacing and the keyboard's
# reach, one instance for the whole application.
set_source_files_properties(qml/Theme.qml qml/Wording.qml
  PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# Material Symbols (Apache-2.0, the license is next to them), drawn in the
# colour of whatever shows them. Kept with the other icons under resource/,
# and served at qml/icons/ in the module, where Theme.qml looks for them.
set(_gf_mob_icon_dir ${CMAKE_SOURCE_DIR}/resource/lfs/icons/mob)
file(GLOB _gf_mob_icons CONFIGURE_DEPENDS ${_gf_mob_icon_dir}/*.svg)
list(APPEND _gf_mob_icons ${_gf_mob_icon_dir}/LICENSE-Apache-2.0.txt)
foreach(_gf_icon IN LISTS _gf_mob_icons)
  get_filename_component(_gf_icon_name "${_gf_icon}" NAME)
  set_source_files_properties("${_gf_icon}" PROPERTIES
    QT_RESOURCE_ALIAS "qml/icons/${_gf_icon_name}")
endforeach()

qt_add_qml_module(gf_mob_lib
  URI GpgFrontend.Mobile
  VERSION 1.0
  # Where QQmlEngine looks by default (the QTP0001 layout).
  RESOURCE_PREFIX /qt/qml
  RESOURCES
    ${_gf_mob_icons}
  QML_FILES
    qml/Theme.qml
    qml/Wording.qml
    qml/Shell.qml
    qml/SplashPage.qml
    qml/ErrorPage.qml
    qml/MainPage.qml
    qml/TextPage.qml
    qml/FilesPage.qml
    qml/SettingsPage.qml
    qml/KeysPage.qml
    qml/KeyDetailPage.qml
    qml/AboutPage.qml
    qml/components/KeyRow.qml
    qml/components/PinSheet.qml
    qml/components/ConfirmSheet.qml
    qml/components/PassphraseSheet.qml
    qml/components/ImportSheet.qml
    qml/components/GenerateSheet.qml
    qml/components/Icon.qml
    qml/components/AppBar.qml
    qml/components/NavBar.qml
    qml/components/Chip.qml
    qml/components/EmptyState.qml
    qml/components/BottomSheet.qml
    qml/components/OperationMenu.qml
    qml/components/ActionButton.qml
    qml/components/InfoRow.qml
    qml/components/FitMenu.qml
    qml/components/SettingSwitch.qml
    qml/components/Snackbar.qml
    qml/components/KeyAvatar.qml
    qml/components/SettingSheet.qml
    qml/components/SecretField.qml
    qml/components/Alert.qml
    qml/components/MessageAlert.qml
    qml/components/BarButton.qml
    qml/components/ListRow.qml
    qml/components/SectionHeader.qml
    qml/components/SearchField.qml
    qml/components/Fab.qml
    qml/components/OverflowButton.qml
    qml/components/FileDetailsSheet.qml
    qml/components/StatusLine.qml
    qml/components/KeyFacts.qml
    qml/components/KeyToolbox.qml
    qml/components/StatusSheet.qml
    qml/components/SheetHost.qml
    qml/components/DetachedVerifySheet.qml
    qml/components/Fact.qml
    qml/components/FingerprintView.qml
    qml/components/SubkeySheet.qml
    qml/components/ValiditySheet.qml
    qml/components/AddSubkeySheet.qml
    qml/components/RevokeSubkeySheet.qml
    qml/components/KeyAlgoChooser.qml
    qml/components/UidSheet.qml
    qml/components/AddUidSheet.qml
    qml/components/CategorySheet.qml
    qml/components/KeyGroupSheet.qml
    qml/components/KeyDatabaseSheet.qml
    qml/components/ProfileSheet.qml
    qml/components/OperationFlow.qml
    qml/components/ProblemLabel.qml
    qml/components/KeyPartFacts.qml
    qml/components/KeyChoiceFace.qml
    qml/components/ResultCard.qml
    qml/components/KeyCategoriesSheet.qml
    qml/components/KeyPickerSheet.qml
    qml/CategoryKeysPage.qml
    qml/KeyGroupPage.qml
    qml/FactsPage.qml
    qml/TranslatorsPage.qml
    qml/ProfilesPage.qml
    qml/KeyCategoriesPage.qml
    qml/KeyGenProfilesPage.qml
    qml/KeyGenProfileEditPage.qml)

# Which style the controls take is the platform's (Material on Android); this
# only gives it GpgFrontend's colours and lets it follow the system's dark mode.
qt_add_resources(gf_mob_lib "gf_mob_style"
  PREFIX "/"
  FILES qtquickcontrols2.conf)

# The phone application's strings, C++ and QML, are in the application's
# catalog (resource/lfs/locale/ts/GpgFrontend.<locale>.ts), which src/res
# builds for the desktop and the phone alike. scripts/update_translations.sh
# collects them from here.

target_include_directories(gf_mob_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# The generated type registrations include each header by its bare name.
target_include_directories(gf_mob_lib PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/startup
  ${CMAKE_CURRENT_SOURCE_DIR}/lifecycle
  ${CMAKE_CURRENT_SOURCE_DIR}/model
  ${CMAKE_CURRENT_SOURCE_DIR}/crypto
  ${CMAKE_CURRENT_SOURCE_DIR}/files)
target_link_libraries(gf_mob_lib PUBLIC
  gf_boot gf_core gf_res Qt::Quick Qt::QuickControls2 Qt::Svg)

qt_add_executable(gf_mob main.cpp)
target_link_libraries(gf_mob PRIVATE gf_mob_lib gf_mob_libplugin)

if(ANDROID)
  # The newest API level the Qt in use is documented to support: Android 16
  # (API 36, what Google Play requires) from Qt 6.10 on, Android 15 before.
  if(Qt6_VERSION VERSION_GREATER_EQUAL 6.10)
    set(_gf_android_target_sdk 36)
  else()
    set(_gf_android_target_sdk 35)
  endif()

  # The package Android builds from: the manifest, its resources and the Java
  # side of the platform layer (src/plat/android/package), with the
  # launcher icon, which is kept with the other icons under resource/.
  set(_gf_android_package ${CMAKE_CURRENT_BINARY_DIR}/android-package)
  set(_gf_android_package_src ${CMAKE_SOURCE_DIR}/src/plat/android/package)
  file(GLOB_RECURSE _gf_android_package_files CONFIGURE_DEPENDS
    RELATIVE ${_gf_android_package_src} ${_gf_android_package_src}/*)
  foreach(_gf_file IN LISTS _gf_android_package_files)
    configure_file(${_gf_android_package_src}/${_gf_file}
      ${_gf_android_package}/${_gf_file} COPYONLY)
  endforeach()
  configure_file(${_gf_mob_icon_dir}/gf_launcher_foreground.png
    ${_gf_android_package}/res/drawable-nodpi/gf_launcher_foreground.png
    COPYONLY)
  # The loading page's logo, for the window shown before it (gf_splash).
  configure_file(${CMAKE_SOURCE_DIR}/resource/lfs/icons/gpgfrontend_logo.png
    ${_gf_android_package}/res/drawable-nodpi/gf_splash_logo.png COPYONLY)

  # Android installs an update only over a higher versionCode, so every build
  # gets the configure time, as the macOS bundle version does. That one's
  # %Y%m%d%H%M is too large for a versionCode (at most 2100000000), so this
  # is year, day of year, hour and minute: fixed width, so it still grows
  # with time, and it fits until 2099.
  string(TIMESTAMP _gf_android_version_code "%y%j%H%M" UTC)
  message(STATUS "GpgFrontend Android Version Code ${_gf_android_version_code}")

  # The application id GpgFrontend uses everywhere else (appstream, Flathub).
  # Android 9 is the oldest Qt 6 supports.
  set(_gf_android_min_sdk 28)
  # What the package says of itself, for the About page's build information:
  # on its one file only, so a new version code rebuilds nothing else.
  set_property(SOURCE model/AboutInfo.cpp TARGET_DIRECTORY gf_mob_lib APPEND
    PROPERTY COMPILE_DEFINITIONS
      GF_ANDROID_MIN_SDK=${_gf_android_min_sdk}
      GF_ANDROID_TARGET_SDK=${_gf_android_target_sdk}
      GF_ANDROID_VERSION_CODE=${_gf_android_version_code})
  set_target_properties(gf_mob PROPERTIES
    QT_ANDROID_PACKAGE_NAME "com.bktus.gpgfrontend"
    QT_ANDROID_APP_NAME "GpgFrontend"
    QT_ANDROID_MIN_SDK_VERSION ${_gf_android_min_sdk}
    QT_ANDROID_TARGET_SDK_VERSION ${_gf_android_target_sdk}
    QT_ANDROID_VERSION_NAME "${PROJECT_VERSION}"
    QT_ANDROID_VERSION_CODE ${_gf_android_version_code}
    # res/mipmap-anydpi-v26: the desktop's logo as an adaptive icon.
    QT_ANDROID_APP_ICON "@mipmap/ic_launcher"
    QT_ANDROID_PACKAGE_SOURCE_DIR "${_gf_android_package}")

  # The APK beside everything else the build makes. Gradle writes it deep
  # inside Qt's android-build project, whatever CMake's output directories
  # say; Qt keeps a copy there as gf_mob.apk, whatever the configuration.
  # Named unsigned, so it is never taken for a release. Deferred: Qt makes
  # gf_mob_make_apk when it finalizes gf_mob, at the end of this directory.
  function(_gf_mob_copy_apk_to_artifacts)
    add_custom_command(TARGET gf_mob_make_apk POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_PROPERTY:gf_mob,BINARY_DIR>/android-build/gf_mob.apk"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/GpgFrontend-${ANDROID_ABI}-unsigned.apk"
      VERBATIM)
  endfunction()
  cmake_language(DEFER CALL _gf_mob_copy_apk_to_artifacts)
endif()
