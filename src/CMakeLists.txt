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

#
if(APPLE)
  add_compile_definitions("_GNU_SOURCE")
endif()

# Introduce GpgME
find_package(Gpgme REQUIRED)

# Introduce OpenSSL
if(APPLE)
  # Define possible OpenSSL directories
  set(OPENSSL_DIR_CANDIDATES
    /usr/local/opt/openssl@3
    /opt/homebrew/opt/openssl@3
  )

  # Find the valid OpenSSL directory
  foreach(DIR IN LISTS OPENSSL_DIR_CANDIDATES)
    if(IS_DIRECTORY "${DIR}" OR EXISTS "${DIR}")
      set(OPENSSL_ROOT_DIR "${DIR}")
      break() # Stop loop once a valid directory is found
    endif()
  endforeach()

  # If not found, throw an error or warning
  if(NOT OPENSSL_ROOT_DIR)
    message(FATAL_ERROR "OpenSSL not found in the standard directories. Please install it or set OPENSSL_ROOT_DIR manually.")
  endif()
endif()

find_package(OpenSSL REQUIRED)

add_subdirectory(core)
add_subdirectory(ui)
add_subdirectory(sdk)
add_subdirectory(test)

# Collecting sources file of app
aux_source_directory(. APP_SOURCE)

# Make app build with resources
set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_SOURCE_DIR}/gpgfrontend.rc")
set_property(SOURCE gpgfrontend.rc APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/gpgfrontend.ico)
set(RESOURCE_FILES ${CMAKE_SOURCE_DIR}/gpgfrontend.qrc ${APP_ICON_RESOURCE_WINDOWS} ${QON_QM_FILES})

# rpath
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
  if(MINGW)
    add_executable(${APP_NAME} WIN32 ${APP_SOURCE} ${RESOURCE_FILES})
  elseif(APPLE AND XCODE_BUILD)
    # standard app bundle packing
    add_executable(${APP_NAME} MACOSX_BUNDLE ${GPGFRONTEND_MACOS_ICON} ${APP_SOURCE} ${RESOURCE_FILES})

    set_target_properties(${APP_NAME} PROPERTIES
      BUNDLE True
      MACOSX_BUNDLE_GUI_IDENTIFIER com.bktus.gpgfrontend
      MACOSX_BUNDLE_BUNDLE_NAME ${APP_NAME}
      MACOSX_BUNDLE_LONG_VERSION_STRING ${BUILD_VERSION}
      MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
      MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
      MACOSX_BUNDLE_COPYRIGHT "GPL-3.0"
      MACOSX_BUNDLE_INFO_STRING "An OpenPGP Crypto Tool"
      MACOSX_BUNDLE_ICON_FILE "gpgfrontend.icns"
      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/resource/plist/MacOSXBundleInfo.plist.in)

    # option for sandbox mode, still under test
    if(GPGFRONTEND_XCODE_ENABLE_SANDBOX)
      message(STATUS "Build Application With App Sandbox")
      set(CUSTOM_ATTRIBUTE_ENABLE_APP_SANDBOX "Yes")
      add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND /bin/cp -rf ${CMAKE_SOURCE_DIR}/resource/lfs/macOS/GnuPG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/${APP_NAME}.app/Contents/
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMENT "Copying Bundle GnuPG into App Bundle Resource")
      set_target_properties(${APP_NAME} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/resource/entitlements/GpgFrontend.entitlements")
    else()
      message(STATUS "Build Application Without App Sandbox")
      set(CUSTOM_ATTRIBUTE_ENABLE_APP_SANDBOX "No")
    endif()

    set_target_properties(${APP_NAME} PROPERTIES
      XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
      XCODE_ATTRIBUTE_SKIP_INSTALL "No"
      XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--deep --entitlements ${CMAKE_SOURCE_DIR}/resource/plist/entitlement.plist"
      XCODE_ATTRIBUTE_ENABLE_APP_SANDBOX "${CUSTOM_ATTRIBUTE_ENABLE_APP_SANDBOX}"
      XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "Yes"
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XCODE_CODE_SIGN_IDENTITY}")
  else()
    add_executable(${APP_NAME} ${APP_SOURCE} ${RESOURCE_FILES})
  endif()
else()
  add_executable(${APP_NAME} ${APP_SOURCE} ${RESOURCE_FILES})
endif()

# using c++ standard 17
target_compile_features(${APP_NAME} PUBLIC cxx_std_17)

# Set Resource Output Path
if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
  if(APPLE)
    set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Resources)
    set(GPGFRONTEND_MACOS_ICON ${CMAKE_SOURCE_DIR}/gpgfrontend.icns)
    set_source_files_properties(${GPGFRONTEND_MACOS_ICON} PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources")
  else()
    set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  endif()
else()
  set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

if(MINGW)
  message(STATUS "Copying Dependent DLL For Windows Runtime Env")

  # all dep dll list
  set(ALL_RUNTIME_DEP_PATH_LIST "")

  # /mingw64/libexec
  execute_process(
    COMMAND cygpath -m /mingw64/bin
    OUTPUT_VARIABLE MSYS64_BIN_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # find libicu*.dll
  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libicu[a-z]*[0-9][0-9].dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libassuan*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libbz*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libcrypto-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libexpat-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libfreetype-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libgcc_s_seh-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libglib-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libgpg-error*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libgpgme*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libgraphite2.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libharfbuzz-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libiconv-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libintl-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/liblz4.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/liblzma-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libpcre-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libpcre2-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libpng*-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libstdc++-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libwinpthread-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/zlib*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libb2-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  # openssl
  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libssl-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libcrypto-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libxml2-*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libarchive*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  unset(_libDllPath)
  file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libgtest*.dll")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

  # dll with only name
  set(ALL_RUNTIME_DLL_FILES "")

  if(GPGFRONTEND_QT5_BUILD)
    list(APPEND ALL_RUNTIME_DLL_FILES
      "Qt5Core.dll"
      "Qt5Gui.dll"
      "Qt5Network.dll"
      "Qt5PrintSupport.dll"
      "Qt5Svg.dll"
      "Qt5Widgets.dll"
      "libbrotlicommon.dll"
      "libbrotlidec.dll"
      "libdouble-conversion.dll"
      "libzstd.dll"
      "libmd4c.dll"
    )
  else()
    list(APPEND ALL_RUNTIME_DLL_FILES
      "Qt6Core.dll"
      "Qt6Core5Compat.dll"
      "Qt6Gui.dll"
      "Qt6Network.dll"
      "Qt6PrintSupport.dll"
      "Qt6Svg.dll"
      "Qt6Widgets.dll"
      "libbrotlicommon.dll"
      "libbrotlidec.dll"
      "libdouble-conversion.dll"
      "libzstd.dll"
      "libmd4c.dll"
    )
  endif()

  # find the other dlls
  foreach(_dllFileName ${ALL_RUNTIME_DLL_FILES})
    message(STATUS "DLL FILE ${_dllFileName}")
    list(APPEND ALL_DLL_NAME ${_dllFileName})
    unset(_runtimeDllLocalPath)

    find_file(_runtimeDllLocalPath NAMES ${_dllFileName} NO_CACHE REQUIRED)
    list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_runtimeDllLocalPath})
    message(STATUS "DLL: ${_dllFileName} , ITS Full Path is ${_runtimeDllLocalPath}")
  endforeach()

  foreach(_dllDepPath ${ALL_RUNTIME_DEP_PATH_LIST})
    message(STATUS "Runtime DLL Dependency Path: ${_dllDepPath}")
  endforeach()

  # copy dependencies to release path
  foreach(_dllRuntimeFilePath ${ALL_RUNTIME_DEP_PATH_LIST})
    file(COPY ${_dllRuntimeFilePath} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
  endforeach()
endif()

# link options for GpgFrontend
target_link_libraries(${APP_NAME} gpgfrontend_core gpgfrontend_ui gpgfrontend_test)

if(MINGW)
  message(STATUS "Link Application Library For MINGW")
  target_link_libraries(${APP_NAME} crypto)
elseif(APPLE)
  message(STATUS "Link Application Library For macOS")
else()
  message(STATUS "Link Application Library For Linux")
  target_link_libraries(${APP_NAME} crypto pthread)
endif()

# add i18n support
set(LOCALE_TS_PATH ${CMAKE_SOURCE_DIR}/resource/lfs/locale/ts)
set(TS_FILES
  "${LOCALE_TS_PATH}/GpgFrontend.en_US.ts"
  "${LOCALE_TS_PATH}/GpgFrontend.de_DE.ts"
  "${LOCALE_TS_PATH}/GpgFrontend.fr_FR.ts"
  "${LOCALE_TS_PATH}/GpgFrontend.zh_CN.ts"
  "${LOCALE_TS_PATH}/GpgFrontend.zh_TW.ts"
  "${LOCALE_TS_PATH}/GpgFrontend.it_IT.ts")
file(GLOB_RECURSE ALL_SOURCE_FILES RELACTIVE ${CMAKE_SOURCE_DIR}/src/*.cpp)

if(GPGFRONTEND_QT5_BUILD)
  # TODO
  qt5_add_translation(GF_TRANSLATIONS_QM ${TS_FILES})
  message(STATUS "GF_TRANSLATIONS_QM ${GF_TRANSLATIONS_QM}")

  set(GF_QM_TRANSLATIONS_RESOURCE_FILE "${CMAKE_CURRENT_BINARY_DIR}/i18n.qrc")
  file(WRITE ${GF_QM_TRANSLATIONS_RESOURCE_FILE} "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n  <qresource prefix=\"i18n\">\n")

  foreach(QM_FILE ${GF_TRANSLATIONS_QM})
    file(RELATIVE_PATH QM_FILENAME ${CMAKE_CURRENT_BINARY_DIR} ${QM_FILE})
    file(APPEND ${GF_QM_TRANSLATIONS_RESOURCE_FILE} "    <file alias=\"${QM_FILENAME}\">${QM_FILE}</file>\n")
  endforeach()

  file(APPEND ${GF_QM_TRANSLATIONS_RESOURCE_FILE} "  </qresource>\n</RCC>\n")
  qt5_add_resources(GF_I18N_RESOURCES ${GF_QM_TRANSLATIONS_RESOURCE_FILE})

  file(GLOB QT_TRANSLATIONS_TS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/third_party/qttranslations/translations/*.ts)
  list(FILTER QT_TRANSLATIONS_TS INCLUDE REGEX ".*(qt|qtbase)_.*\.ts$")
  qt5_add_translation(QT_TRANSLATIONS_QM ${QT_TRANSLATIONS_TS})

  set(QT_QM_TRANSLATIONS_RESOURCE_FILE "${CMAKE_CURRENT_BINARY_DIR}/qt_i18n.qrc")
  file(WRITE ${QT_QM_TRANSLATIONS_RESOURCE_FILE} "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n  <qresource prefix=\"qt_i18n\">\n")

  foreach(QM_FILE ${QT_TRANSLATIONS_QM})
    file(RELATIVE_PATH QM_FILENAME ${CMAKE_CURRENT_BINARY_DIR} ${QM_FILE})
    file(APPEND ${QT_QM_TRANSLATIONS_RESOURCE_FILE} "    <file alias=\"${QM_FILENAME}\">${QM_FILE}</file>\n")
  endforeach()

  file(APPEND ${QT_QM_TRANSLATIONS_RESOURCE_FILE} "  </qresource>\n</RCC>\n")
  qt5_add_resources(GF_I18N_RESOURCES ${QT_QM_TRANSLATIONS_RESOURCE_FILE})

  message(STATUS "GF_I18N_RESOURCES ${GF_I18N_RESOURCES}")
  target_sources(${APP_NAME} PRIVATE ${GF_I18N_RESOURCES})
else()
  add_custom_target(gftranslations ALL)

  qt_add_lrelease(gftranslations
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE GF_TRANSLATIONS_QM)
  qt_add_resources(${APP_NAME} "gftranslations"
    PREFIX "/i18n"
    BASE ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${GF_TRANSLATIONS_QM})

  add_custom_target(qttranslations ALL)

  file(GLOB QT_TRANSLATIONS_TS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/third_party/qttranslations/translations/*.ts)
  list(FILTER QT_TRANSLATIONS_TS INCLUDE REGEX ".*(qt|qtbase)_.*\.ts$")
  qt_add_lrelease(qttranslations
    TS_FILES ${QT_TRANSLATIONS_TS}
    QM_FILES_OUTPUT_VARIABLE QT_TRANSLATIONS_QM)
  qt_add_resources(${APP_NAME} "qttranslations"
    PREFIX "/i18n_qt"
    BASE ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${QT_TRANSLATIONS_QM})
endif()

include(GNUInstallDirs)

#
# Install information read by application
#
configure_file(${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInstallInfo.h.in ${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInstallInfo.h @ONLY)

#
# Install generated libraries and application
#
set(GPGFRONTEND_INSTALL_LIBRARIES
  gpgfrontend_core
  gpgfrontend_ui
  gpgfrontend_test
  gpgfrontend_module_sdk)

install(TARGETS ${APP_NAME} ${GPGFRONTEND_INSTALL_LIBRARIES}
  EXPORT GpgFrontendTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

#
# Install resources and translation
#
install(FILES ${CMAKE_SOURCE_DIR}/TRANSLATORS
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${APP_NAME_LOWER})

#
# License and documentation
#
install(FILES "${CMAKE_SOURCE_DIR}/COPYING"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${APP_NAME_LOWER}"
  COMPONENT runtime)

install(FILES "${CMAKE_SOURCE_DIR}/README.md"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${APP_NAME_LOWER}"
  COMPONENT runtime)

#
# Desktop integration
#
install(FILES "${CMAKE_SOURCE_DIR}/resource/appstream/com.bktus.gpgfrontend.desktop"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
  COMPONENT runtime)

install(FILES "${CMAKE_SOURCE_DIR}/resource/appstream/com.bktus.gpgfrontend.metainfo.xml"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo"
  COMPONENT runtime)

#
# Icons and pixmaps
#
install(DIRECTORY "${CMAKE_SOURCE_DIR}/resource/lfs/pixmaps/"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/pixmaps")

install(DIRECTORY "${CMAKE_SOURCE_DIR}/resource/lfs/hicolor/"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor")

# Optional
foreach(size IN ITEMS 32 48 64 128 256 512)
  install(FILES "${CMAKE_SOURCE_DIR}/resource/lfs/hicolor/${size}x${size}/apps/com.bktus.gpgfrontend.png"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${size}x${size}/apps"
    RENAME "gpgfrontend.png"
    COMPONENT runtime)
endforeach()

# Building linux package on linux (DEB or RPM)
if(BUILD_APP_FOR_PACKAGE)
  set(CPACK_PACKAGE_NAME "${APP_NAME_LOWER}")
  set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
  set(CPACK_PACKAGE_RELEASE "1")
  set(CPACK_PACKAGE_CONTACT "eric@bktus.com")
  set(CPACK_PACKAGE_DESCRIPTION
    "GpgFrontend is a free, open-source, robust yet user-friendly, compact and cross-platform tool for OpenPGP encryption.")
  set(CPACK_PACKAGE_VENDOR "Saturneric")
  set(CPACK_PACKAGE_HOMEPAGE_URL "https://gpgfrontend.bktus.com")

  set(CPACK_PACKAGE_FILE_NAME "${APP_NAME_LOWER}-v${PROJECT_VERSION}")
  set(CPACK_GENERATOR "RPM;DEB")

  #
  # RPM Settings
  #
  set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
  set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
  set(CPACK_RPM_PACKAGE_REQUIRES
    "libassuan >= 2.5.0, gpgme >= 1.12.0, qt6-qtbase >= 6.0.0, gtk-update-icon-cache")
  set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
  set(CPACK_RPM_PACKAGE_LICENSE_FILE "${CMAKE_SOURCE_DIR}/COPYING")
  set(CPACK_RPM_PACKAGE_DOCUMENTATION "${CMAKE_SOURCE_DIR}/README.md")

  # Exclude auto-filelist paths
  set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    "/usr/share/doc"
    "/usr/share/licenses"
    "/usr/share/applications"
    "/usr/share/metainfo"
    "/usr/share/icons"
    "/usr/share/icons/hicolor"
    "/usr/share/icons/hicolor/*")

  #
  # DEB Settings
  #
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "eric@bktus.com")
  set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libassuan0 (>= 2.5.0), libgpgme11 (>= 1.12.0), qt6-base-dev (>= 6.0.0), gtk-update-icon-cache")
  set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
  set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
  set(CPACK_DEBIAN_PACKAGE_LICENSE "GPLv3")

  #
  # Install scripts
  #
  configure_file("${CMAKE_SOURCE_DIR}/cmake/rpm/post-install.sh"
    "${CMAKE_BINARY_DIR}/rpm-post-install.sh" @ONLY)
  configure_file("${CMAKE_SOURCE_DIR}/cmake/rpm/post-uninstall.sh"
    "${CMAKE_BINARY_DIR}/rpm-post-uninstall.sh" @ONLY)

  set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_BINARY_DIR}/rpm-post-install.sh")
  set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_BINARY_DIR}/rpm-post-uninstall.sh")

  file(CHMOD "${CMAKE_BINARY_DIR}/rpm-post-install.sh"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  file(CHMOD "${CMAKE_BINARY_DIR}/rpm-post-uninstall.sh"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

  include(CPack)
endif()