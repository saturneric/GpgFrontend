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

# mimalloc
find_package(mimalloc REQUIRED)

# Set Build Information
configure_file(${CMAKE_SOURCE_DIR}/src/GpgFrontend.h.in ${CMAKE_SOURCE_DIR}/src/GpgFrontend.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInfo.h.in ${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInfo.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInstallInfo.h.in ${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInstallInfo.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/src/sdk/GFSDKBuildInfo.h.in ${CMAKE_SOURCE_DIR}/src/sdk/GFSDKBuildInfo.h @ONLY)

if(APPLE)
  configure_file(${CMAKE_SOURCE_DIR}/resource/plist/ExportOptions.plist.in ${CMAKE_BINARY_DIR}/ExportOptions.plist @ONLY)
endif()

# Set Runtime Output Directory
if(NOT XCODE_BUILD)
  # Set Binary Output Path
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts)
else()
  # Set Binary Output Path
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
endif()

# Print modules
if(BUILD_CORE)
  message("[+] Build Core")
  add_subdirectory(core)
endif()

if(BUILD_UI)
  message("[+] Build UI")
  add_subdirectory(ui)
endif()

if(BUILD_SDK)
  message("[+] Build SDK")
  add_subdirectory(sdk)
endif()

# build to test gpgfrontend core
if(BUILD_TEST)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()

# configure for output path and resources
if(BUILD_APPLICATION)
  aux_source_directory(. BASE_SOURCE)
  set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_SOURCE_DIR}/gpgfrontend.rc")
  set_property(SOURCE gpgfrontend.rc APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/gpgfrontend.ico)
endif()

if(BUILD_APPLICATION)
  # Set Resource Output Path
  if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    if(APPLE)
      set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Resources)
    elseif(LINUX AND NOT LINUX_INSTALL_SOFTWARE)
      file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/AppDir)
      file(COPY ${CMAKE_SOURCE_DIR}/resource/lfs/hicolor DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/AppDir/usr/share/icons)
      file(COPY ${CMAKE_SOURCE_DIR}/resource/appstream/com.bktus.gpgfrontend.metainfo.xml DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/AppDir/usr/share/metainfo FOLLOW_SYMLINK_CHAIN)
      file(COPY ${CMAKE_SOURCE_DIR}/resource/appstream/com.bktus.gpgfrontend.desktop DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/AppDir/usr/share/applications FOLLOW_SYMLINK_CHAIN)
      file(COPY ${CMAKE_SOURCE_DIR}/resource/lfs/pixmaps/com.bktus.gpgfrontend.png DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/AppDir FOLLOW_SYMLINK_CHAIN)
      set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/AppDir/usr/share)
    else()
      set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    endif()
  else()
    set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  endif()
endif()

if(BUILD_APPLICATION)
  if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    if(APPLE)
      set(GPGFRONTEND_MACOS_ICON ${CMAKE_SOURCE_DIR}/gpgfrontend.icns)
      set_source_files_properties(${GPGFRONTEND_MACOS_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
    elseif(LINUX AND NOT LINUX_INSTALL_SOFTWARE)
      file(REMOVE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/AppDir/usr/bin/${AppName})
    endif()
  endif()
endif()

if(BUILD_APPLICATION AND MINGW)
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

  # /mingw64/libexec
  execute_process(
    COMMAND cygpath -m /mingw64/libexec
    OUTPUT_VARIABLE MSYS64_LIBEXEC_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # gpgme-w32spawn.exe
  unset(_libExEPath)
  file(GLOB _libExEPath "${MSYS64_LIBEXEC_PATH}/gpgme-*.exe")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libExEPath})

  unset(_libExEPath)
  file(GLOB _libExEPath "${MSYS64_BIN_PATH}/gpgme-*.exe")
  list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libExEPath})

  # add gpgfrontend libraries
  if(STABLE_BUILD_ONLY_APPLICATION)
    unset(_libDllPath)
    file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libgpgfrontend_*.dll")
    list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

    unset(_libDllPath)
    file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libmimalloc*.dll")
    list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

    unset(_libDllPath)
    file(GLOB _libDllPath "${MSYS64_BIN_PATH}/mimalloc*.dll")
    list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

    unset(_libDllPath)
    file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libarchive*.dll")
    list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

    unset(_libDllPath)
    file(GLOB _libDllPath "${MSYS64_BIN_PATH}/libgtest*.dll")
    list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})
  endif()

  # dll with only name
  set(ALL_RUNTIME_DLL_FILES "")

  if(GPGFRONTEND_QT5_BUILD)
    list(APPEND ALL_RUNTIME_DLL_FILES "Qt5Core.dll;Qt5Gui.dll;Qt5Network.dll;Qt5PrintSupport.dll;Qt5Svg.dll;Qt5Widgets.dll;libbrotlicommon.dll;libbrotlidec.dll;libdouble-conversion.dll;libzstd.dll;libmd4c.dll;")
  else()
    list(APPEND ALL_RUNTIME_DLL_FILES "Qt6Core.dll;Qt6Core5Compat.dll;Qt6Gui.dll;Qt6Network.dll;Qt6PrintSupport.dll;Qt6Svg.dll;Qt6Widgets.dll;libbrotlicommon.dll;libbrotlidec.dll;libdouble-conversion.dll;libzstd.dll;libmd4c.dll;")
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

  message(STATUS "All the Application Runtime Dependencies Path: ${ALL_RUNTIME_DEP_PATH_LIST}")

  # copy dependencies to release path
  foreach(_dllRuntimeFilePath ${ALL_RUNTIME_DEP_PATH_LIST})
    file(COPY ${_dllRuntimeFilePath} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
  endforeach()
endif()

if(BUILD_APPLICATION)
  set(RESOURCE_FILES ${CMAKE_SOURCE_DIR}/gpgfrontend.qrc ${APP_ICON_RESOURCE_WINDOWS} ${QON_QM_FILES})
  add_custom_target(resources ALL DEPENDS ${RESOURCE_FILES})
endif()

if(BUILD_APPLICATION)
  if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    if(MINGW)
      add_executable(${AppName} WIN32 ${BASE_SOURCE} ${RESOURCE_FILES})
    elseif(LINUX AND NOT LINUX_INSTALL_SOFTWARE)
      add_executable(${AppName} ${BASE_SOURCE} ${RESOURCE_FILES})

      add_custom_command(TARGET ${AppName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/AppDir/usr/bin"
        COMMAND ${CMAKE_COMMAND} -E rename "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${AppName}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/AppDir/usr/bin/${AppName}"
        COMMENT "Copying Binary into App Image"
      )

    # app bundle packing using xcode
    elseif(APPLE AND XCODE_BUILD)
      # standard app bundle packing
      add_executable(${AppName} MACOSX_BUNDLE ${GPGFRONTEND_MACOS_ICON} ${BASE_SOURCE} ${RESOURCE_FILES})

      set_target_properties(${AppName} PROPERTIES
        BUNDLE True
        MACOSX_BUNDLE_GUI_IDENTIFIER com.bktus.gpgfrontend
        MACOSX_BUNDLE_BUNDLE_NAME ${AppName}
        MACOSX_BUNDLE_LONG_VERSION_STRING ${BUILD_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_COPYRIGHT "GPL-3.0"
        MACOSX_BUNDLE_INFO_STRING "An OpenPGP Crypto Tool"
        MACOSX_BUNDLE_ICON_FILE "gpgfrontend.icns"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/resource/plist/MacOSXBundleInfo.plist.in)

      # option for sandbox mode, still under test
      if(GPGFRONTEND_XOCDE_ENABLE_SANDBOX)
        message(STATUS "Build Application With App Sandbox")
        set(CUSTOM_ATTRIBUTE_ENABLE_APP_SANDBOX "Yes")
        add_custom_command(TARGET ${AppName} POST_BUILD
          COMMAND /bin/cp -rf ${CMAKE_SOURCE_DIR}/resource/lfs/macOS/GnuPG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/${AppName}.app/Contents/
          WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
          COMMENT "Copying Buddled GnuPG into App Bundle Resource")
        set_target_properties(${AppName} PROPERTIES
          XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/resource/entitlements/GpgFrontend.entitlements")
      else()
        message(STATUS "Build Application Without App Sandbox")
        set(CUSTOM_ATTRIBUTE_ENABLE_APP_SANDBOX "No")
      endif()

      set_target_properties(${AppName} PROPERTIES
        XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
        XCODE_ATTRIBUTE_SKIP_INSTALL "No"
        XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--deep --entitlements ${CMAKE_SOURCE_DIR}/resource/plist/entitlement.plist"
        XCODE_ATTRIBUTE_ENABLE_APP_SANDBOX "${CUSTOM_ATTRIBUTE_ENABLE_APP_SANDBOX}"
        XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "Yes"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XOCDE_CODE_SIGN_IDENTITY}")
    elseif(LINUX AND LINUX_INSTALL_SOFTWARE)
      add_executable(${AppName} ${BASE_SOURCE} ${RESOURCE_FILES})
    else()
      message(FATAL_ERROR "cannot find a defined method to release application.")
    endif()
  else()
    # if the status is debug
    add_executable(${AppName} ${BASE_SOURCE} ${RESOURCE_FILES})

    if(MINGW)
      # include qt dependencies
      if(NOT Qt6_DIR)
        add_custom_command(TARGET ${AppName} POST_BUILD
          COMMAND windeployqt --force ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${AppName}.exe)
      else()
        add_custom_command(TARGET ${AppName} POST_BUILD
          COMMAND windeployqt-qt6.exe --force ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${AppName}.exe)
      endif()
    endif()
  endif()

  # Make app build with resources
  add_dependencies(${AppName} resources)

  # using c++ standard 17
  target_compile_features(${AppName} PUBLIC cxx_std_17)
endif()

if(STABLE_BUILD_ONLY_APPLICATION)
  message(STATUS "Application Headers Search Path: ${CMAKE_INSTALL_PREFIX}/include/gpgfrontend")
  target_include_directories(${AppName} PUBLIC "${CMAKE_INSTALL_PREFIX}/include/gpgfrontend")

  # link qt
  if(GPGFRONTEND_QT5_BUILD)
    target_link_libraries(${AppName} Qt5::Core Qt5::Widgets)
  else()
    target_link_libraries(${AppName} Qt6::Core Qt6::Widgets)
  endif()
endif()

# link options for GpgFrontend
if(BUILD_APPLICATION)
  target_link_libraries(${AppName} gpgfrontend_core gpgfrontend_ui gpgfrontend_test)

  if(MINGW)
    message(STATUS "Link Application Library For MINGW")
    target_link_libraries(${AppName} crypto)
  elseif(APPLE)
    message(STATUS "Link Application Library For macOS")
  else()
    message(STATUS "Link Application Library For Linux")
    target_link_libraries(${AppName} crypto pthread)
  endif()
endif()

# add i18n support
if(BUILD_APPLICATION)
  set(LOCALE_TS_PATH ${CMAKE_SOURCE_DIR}/resource/lfs/locale/ts)
  set(TS_FILES "${LOCALE_TS_PATH}/GpgFrontend.en_US.ts"
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

    target_sources(${AppName} PRIVATE ${GF_I18N_RESOURCES})
  else()
    add_custom_target(gftranslations ALL)

    qt_add_lrelease(gftranslations
      TS_FILES ${TS_FILES}
      QM_FILES_OUTPUT_VARIABLE GF_TRANSLATIONS_QM)
    qt_add_resources(${AppName} "gftranslations"
      PREFIX "/i18n"
      BASE ${CMAKE_CURRENT_BINARY_DIR}
      FILES ${GF_TRANSLATIONS_QM})

    add_custom_target(qttranslations ALL)

    file(GLOB QT_TRANSLATIONS_TS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/third_party/qttranslations/translations/*.ts)
    list(FILTER QT_TRANSLATIONS_TS INCLUDE REGEX ".*(qt|qtbase)_.*\.ts$")
    qt_add_lrelease(qttranslations
      TS_FILES ${QT_TRANSLATIONS_TS}
      QM_FILES_OUTPUT_VARIABLE QT_TRANSLATIONS_QM)
    qt_add_resources(${AppName} "qttranslations"
      PREFIX "/i18n_qt"
      BASE ${CMAKE_CURRENT_BINARY_DIR}
      FILES ${QT_TRANSLATIONS_QM})
  endif()
endif()

# if building linux package
if(LINUX AND LINUX_INSTALL_SOFTWARE)
  include(GNUInstallDirs)

  if(STABLE_BUILD_ONLY_APPLICATION)
    set(GPGFRONTEND_INSTALL_LIBRARIES "")
  else()
    set(GPGFRONTEND_INSTALL_LIBRARIES
      gpgfrontend_core
      gpgfrontend_ui
      gpgfrontend_test
      gpgfrontend_module_sdk)

    if(NOT APPLE)
      list(APPEND GPGFRONTEND_INSTALL_LIBRARIES mimalloc)
    endif()
  endif()

  message(STATUS "GpgFrontend Install Libraries: ${GPGFRONTEND_INSTALL_LIBRARIES}")
  install(TARGETS ${AppName} ${GPGFRONTEND_INSTALL_LIBRARIES}
    EXPORT GpgFrontendTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

  install(FILES ${CMAKE_SOURCE_DIR}/TRANSLATORS
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${AppName}/)
  install(FILES ${CMAKE_SOURCE_DIR}/resource/appstream/com.bktus.gpgfrontend.appdata.xml
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo/)
  install(FILES ${CMAKE_SOURCE_DIR}/resource/appstream/com.bktus.gpgfrontend.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/lfs/pixmaps/
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps/)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/lfs/hicolor/
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/)
endif()

# if only build sdk
if(STABLE_BUILD_ONLY_SDK)
  include(GNUInstallDirs)
  set(GPGFRONTEND_SDK_INSTALL_LIBRARIES
    gpgfrontend_module_sdk)

  install(TARGETS ${GPGFRONTEND_SDK_INSTALL_LIBRARIES}
    EXPORT GpgFrontendTargets
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# if build full sdk
if(STABLE_BUILD_FULL_SDK)
  include(GNUInstallDirs)

  set(GPGFRONTEND_SDK_INSTALL_LIBRARIES
    gpgfrontend_core
    gpgfrontend_ui
    gpgfrontend_test
    gpgfrontend_module_sdk)

  if(NOT APPLE)
    list(APPEND GPGFRONTEND_SDK_INSTALL_LIBRARIES mimalloc)
  endif()

  install(TARGETS ${GPGFRONTEND_SDK_INSTALL_LIBRARIES}
    EXPORT GpgFrontendTargets
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/core"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/gpgfrontend"
    FILES_MATCHING
    PATTERN "*.h"
  )

  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ui"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/gpgfrontend"
    FILES_MATCHING
    PATTERN "*.h"
  )

  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/gpgfrontend"
    FILES_MATCHING
    PATTERN "*.h"
  )
endif()