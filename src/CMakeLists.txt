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

# Introduce boost
if(NOT BOOST_ROOT)
    find_package(Boost COMPONENTS date_time system REQUIRED)
else()
    find_package(Boost
            COMPONENTS date_time system REQUIRED
            PATHS ${BOOST_ROOT} NO_DEFAULT_PATH)
endif()

# Introduce GpgME
find_package(Gpgme REQUIRED)

# Introduce Config++
find_package(Config++ REQUIRED)

# Introduce OpenSSL
if(APPLE)
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl@3)
endif()
find_package(OpenSSL REQUIRED)

# Introduce Qt
if (QT5_ENV_SUPPORT)
    # Support Qt version: 6.x, 5.12.x and 5.15.x
    find_package(Qt6 6 COMPONENTS Core Test Widgets PrintSupport Network Core5Compat)
    if(NOT Qt6_DIR)
        find_package(Qt5 5 COMPONENTS Core Test Widgets PrintSupport Network REQUIRED)
        message(STATUS "Use Qt5 for application building ${Qt5_DIR}")
    else()
        message(STATUS "Use Qt6 for application building ${Qt6_DIR}")
        add_definitions(-DGPGFRONTEND_GUI_QT6)
    endif()
    
    # Qt configuration
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    set(CMAKE_AUTORCC_OPTIONS "--compress;9")
    set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS} ${CMAKE_SOURCE_DIR}/ui)
endif ()

# configure for output path and resources
if (APPLICATION_BUILD)
    aux_source_directory(. BASE_SOURCE)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_SOURCE_DIR}/gpgfrontend.rc")
    set_property(SOURCE gpgfrontend.rc APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/gpgfrontend.ico)

    if (NOT XCODE_BUILD)
        # Set Binary Output Path
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/release)
    else ()
        # Set Binary Output Path
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    endif ()
endif ()


if (GPG_CORE)
    message(STATUS "Build Core")
    add_subdirectory(core)
endif ()

if (UI_CORE)
    message(STATUS "Build UI")
    add_subdirectory(ui)
endif ()

if (UI_CORE)
    message(STATUS "Build Plugin")
    add_subdirectory(plugin)
endif ()

if (APPLICATION_BUILD)
    # Set Resource Output Path
    if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
        if (APPLE)
            set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Resources)
        elseif (LINUX AND NOT LINUX_INSTALL_SOFTWARE)
            file(COPY ${CMAKE_SOURCE_DIR}/resource/lfs/app-image/gpgfrontend DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
            file(COPY ${CMAKE_SOURCE_DIR}/resource/appstream/com.bktus.gpgfrontend.metainfo.xml DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/gpgfrontend/usr/share/metainfo FOLLOW_SYMLINK_CHAIN)
            file(COPY ${CMAKE_SOURCE_DIR}/resource/appstream/com.bktus.gpgfrontend.desktop DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/gpgfrontend/usr/share/applications FOLLOW_SYMLINK_CHAIN)
            set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/gpgfrontend/usr/share)
        else ()
            set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        endif ()
    else ()
        set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    endif ()
endif ()

# Get ALL SOURCE FILES
file(GLOB_RECURSE ALL_SOURCE_FILES RELACTIVE ${CMAKE_SOURCE_DIR}/src/*.cpp)

# i18n
if (MULTI_LANG_SUPPORT)
    message(STATUS "Build Multiply Languages Support")
    # Set Translation Files
    find_package(Gettext REQUIRED)
    FIND_PROGRAM(GETTEXT_MSGFMT_EXECUTABLE msgfmt)
    FIND_PROGRAM(GETTEXT_XGETTEXT_EXECUTABLE xgettext)

    set(LOCALE_OUTPUT_PATH ${RESOURCE_OUTPUT_DIRECTORY}/locales)

    if (NOT GETTEXT_MSGFMT_EXECUTABLE OR NOT GETTEXT_XGETTEXT_EXECUTABLE)
        message(ERROR "msgfmt or xgettext not found. Translations will *not* be installed")
    else()
        message(STATUS "Setting target translations")
        add_custom_target(translations)
        set(OUTPUT_POT_PATH ${CMAKE_SOURCE_DIR}/resource/lfs/locale/template/${PROJECT_NAME}.pot)
        add_custom_command(
                TARGET translations
                COMMAND find ${CMAKE_SOURCE_DIR}/src -iname \"*.cpp\" | xargs xgettext --package-name=${PROJECT_NAME} --copyright-holder=Saturneric --package-version=${PROJECT_VERSION} --msgid-bugs-address=eric@bktus.com --add-comments="/*" --c++ -k_ -o ${OUTPUT_POT_PATH}
        )

        file(GLOB ALL_PO_FILES ${CMAKE_SOURCE_DIR}/resource/lfs/locale/po/*.po)
        SET(GMO_FILES)

        foreach (_poFile ${ALL_PO_FILES})
            GET_FILENAME_COMPONENT(_poFileName ${_poFile} NAME)
            string(REGEX REPLACE "\\.[^.]*$" "" _langName ${_poFileName})
            message(STATUS "GNU gettext po file ${_langName}")
            make_directory(${RESOURCE_OUTPUT_DIRECTORY}/locales)
            make_directory(${RESOURCE_OUTPUT_DIRECTORY}/locales/${_langName}/LC_MESSAGES)
            add_custom_command(
                    TARGET translations
                    COMMAND echo Processing po LANG ${_langName}
            )
            add_custom_command(
                    TARGET translations
                    COMMAND msgfmt --check --verbose --output-file ${LOCALE_OUTPUT_PATH}/${_langName}/LC_MESSAGES/GpgFrontend.mo ${_poFile}
            )
        endforeach ()

    endif ()
endif ()

if (BASIC_ENV_CONFIG)
    # Set Build Information
    configure_file(${CMAKE_SOURCE_DIR}/src/GpgFrontend.h.in ${CMAKE_SOURCE_DIR}/src/GpgFrontend.h @ONLY)
    configure_file(${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInfo.h.in ${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInfo.h @ONLY)
    configure_file(${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInstallInfo.h.in ${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInstallInfo.h @ONLY)
    if (APPLE)
        configure_file(${CMAKE_SOURCE_DIR}/resource/plist/ExportOptions.plist.in ${CMAKE_BINARY_DIR}/ExportOptions.plist @ONLY)
    endif ()
endif ()

if (APPLICATION_BUILD)
    # Copy Resource Files
    file(COPY ${CMAKE_SOURCE_DIR}/resource/css DESTINATION ${RESOURCE_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/lfs/icons DESTINATION ${RESOURCE_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/TRANSLATORS DESTINATION ${RESOURCE_OUTPUT_DIRECTORY} FOLLOW_SYMLINK_CHAIN)
    if (GPG_STANDALONE_MODE)
        file(COPY ${CMAKE_SOURCE_DIR}/resource/gpg1.4 DESTINATION ${RESOURCE_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
    endif ()
endif ()

if (APPLICATION_BUILD)
    if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
        if (APPLE)
            file(COPY ${CMAKE_SOURCE_DIR}/gpgfrontend.icns DESTINATION ${RESOURCE_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
            # Refresh App Bundle
            file(REMOVE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${AppName}.app)
        elseif (LINUX AND NOT LINUX_INSTALL_SOFTWARE)
            file(REMOVE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/gpgfrontend/usr/bin/${AppName})
        endif ()
    endif ()
endif ()

if (APPLICATION_BUILD)
    # Copy Utils Files
    if (MINGW)
        message(STATUS "Copying Dependent DLL For Windows Runtime Env")

        set(ALL_RUNTIME_DEP_PATH_LIST "")

        # get mingw bin path
        find_file(_libGccDllPath NAMES libgcc_s_seh-1.dll NO_CACHE REQUIRED)
        cmake_path(GET _libGccDllPath PARENT_PATH _libDllBinPath)

        # find libicu*.dll
        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libicu[a-z]*[0-9][0-9].dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libconfig++-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libarchive-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libassuan-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libbz*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libcrypto-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libexpat-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libfreetype-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libgcc_s_seh-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libglib-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libgpg-error-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libgpgme-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libgraphite2.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libharfbuzz-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libiconv-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libintl-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/liblz4.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/liblzma-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libpcre-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libpcre2-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libpng*-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libstdc++-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libwinpthread-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/zlib*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libb2-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        # openssl
        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libssl-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        unset(_libDllPath)
        file(GLOB _libDllPath "${_libDllBinPath}/libcrypto-*.dll")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libDllPath})

        # gpgme-w32spawn.exe
        unset(_libExEPath)
        file(GLOB _libExEPath "${_libDllBinPath}/gpgme-w32spawn.exe")
        list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_libExEPath})

        set(ALL_RUNTIME_DLL_FILES "")
        list(APPEND ALL_RUNTIME_DLL_FILES "Qt6Core.dll;Qt6Core5Compat.dll;Qt6Gui.dll;Qt6Network.dll;Qt6PrintSupport.dll;Qt6Svg.dll;Qt6Widgets.dll;libbrotlicommon.dll;libbrotlidec.dll;libdouble-conversion.dll;libzstd.dll;libmd4c.dll;")
        # find the other dlls
        foreach (_dllFileName ${ALL_RUNTIME_DLL_FILES})
            message(STATUS "DLL FILE ${_dllFileName}")
            list(APPEND ALL_DLL_NAME ${_dllFileName})
            unset(_runtimeDllLocalPath)

            find_file(_runtimeDllLocalPath NAMES ${_dllFileName} NO_CACHE REQUIRED)
            list(APPEND ALL_RUNTIME_DEP_PATH_LIST ${_runtimeDllLocalPath})
            message(STATUS "DLL ${_dllFileName} Path is ${_runtimeDllLocalPath}")
        endforeach()

        message(STATUS "All Runtime Dependencies Path ${ALL_RUNTIME_DEP_PATH_LIST}")

        # copy dependencies to release path
        foreach (_dllRuntimeFilePath ${ALL_RUNTIME_DEP_PATH_LIST})
            file(COPY ${_dllRuntimeFilePath} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
        endforeach()

    endif ()
endif ()

if (APPLICATION_BUILD)
    set(RESOURCE_FILES ${CMAKE_SOURCE_DIR}/gpgfrontend.qrc ${APP_ICON_RESOURCE_WINDOWS} ${QON_QM_FILES})
    add_custom_target(resources ALL DEPENDS ${RESOURCE_FILES})
    if (MULTI_LANG_SUPPORT)
        add_dependencies(resources translations)
    endif ()
endif ()

if (APPLICATION_BUILD)
    if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
        if (MINGW)
            add_executable(${AppName} WIN32 ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
            # include qt dependencies
            if(NOT Qt6_DIR)
                add_custom_command(TARGET ${AppName} POST_BUILD
                        COMMAND windeployqt --force ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${AppName}.exe)
            else()
                add_custom_command(TARGET ${AppName} POST_BUILD
                        COMMAND windeployqt-qt6.exe --force ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${AppName}.exe)
            endif()
        elseif (APPLE AND NOT XCODE_BUILD)
            # custom app bundle packing
            add_executable(${AppName} MACOSX_BUNDLE ${ICON_RESOURCE} ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
            set_target_properties(${AppName} PROPERTIES
                    BUNDLE True
                    MACOSX_BUNDLE_GUI_IDENTIFIER com.bktus.gpgfrontend
                    MACOSX_BUNDLE_BUNDLE_NAME ${AppName}
                    MACOSX_BUNDLE_LONG_VERSION_STRING ${BUILD_VERSION}
                    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
                    MACOSX_BUNDLE_BUNDLE_VERSION ${BUILD_VERSION}
                    MACOSX_BUNDLE_ICON_FILE "gpgfrontend.icns")
            add_custom_command(TARGET ${AppName} POST_BUILD
                    COMMAND /bin/rm -rf ./${AppName}.app/Contents/Resources
                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                    COMMENT "Deleting Resources in App Bundle")
            add_custom_command(TARGET ${AppName} POST_BUILD
                    COMMAND /bin/mv -n ./Resources ./${AppName}.app/Contents/
                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                    COMMENT "Copying Resources into App Bundle Resource")
        elseif (LINUX AND NOT LINUX_INSTALL_SOFTWARE)
            add_executable(${AppName} ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
            add_custom_command(TARGET ${AppName} POST_BUILD
                    COMMAND /bin/mkdir -p ./gpgfrontend/usr/bin && /bin/mv -f ./${AppName} ./gpgfrontend/usr/bin/
                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                    COMMENT "Copying Binary into App Image")
            add_custom_command(TARGET ${AppName} POST_BUILD
                    COMMAND /bin/mkdir -p ./gpgfrontend/usr/lib
                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                    COMMENT "Complement to build the required architecture")
            # app bundle packing using xcode
        elseif (APPLE AND XCODE_BUILD)
            # standard app bundle packing
            add_executable(${AppName} MACOSX_BUNDLE ${ICON_RESOURCE} ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
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
                    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/resource/plist/MacOSXBundleInfo.plist.in
                    )
            add_custom_command(TARGET ${AppName} POST_BUILD
                    COMMAND /bin/rm -rf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${AppName}.app/Contents/Resources
                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                    COMMENT "Deleting Resources in App Bundle")
            add_custom_command(TARGET ${AppName} POST_BUILD
                    COMMAND /bin/cp -rf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Resources ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/${AppName}.app/Contents/
                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                    COMMENT "Copying Resources into App Bundle Resource")
            add_custom_command(TARGET ${AppName} POST_BUILD
                    COMMAND macdeployqt ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/${AppName}.app
                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                    COMMENT "Resolving Qt Dependency")
            
            # option for sandbox mode, still under test
            if(GPGFRONTEND_XOCDE_ENABLE_SANDBOX)
                message(STATUS "Build Application With App Sandbox")
                set(CUSTOM_ATTRIBUTE_ENABLE_APP_SANDBOX "Yes")
                add_custom_command(TARGET ${AppName} POST_BUILD
                    COMMAND /bin/cp -rf ${CMAKE_SOURCE_DIR}/resource/lfs/macOS/GnuPG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/${AppName}.app/Contents/
                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                    COMMENT "Copying Buddled GnuPG into App Bundle Resource")
                set_target_properties(${AppName} PROPERTIES
                    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/resource/entitlements/GpgFrontend.entitlements"
                )
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
                    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${GPGFRONTEND_XOCDE_CODE_SIGN_IDENTITY}"
                    )
        else ()
            add_executable(${AppName} ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
        endif ()
    else ()
        # if the status is debug
        add_executable(${AppName} ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
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
    endif ()

    # Make app build with resources
    add_dependencies(${AppName} resources)

endif ()

# link options for GpgFrontend
if (APPLICATION_BUILD)
    target_link_libraries(${AppName} gpgfrontend_ui)
    if (MINGW)
        message(STATUS "Link Application Library For MINGW")
        target_link_libraries(${AppName} crypto)
    elseif (APPLE)
        message(STATUS "Link Application Library For macOS")
        target_link_libraries(${AppName} intl)
    else ()
        message(STATUS "Link Application Library For Linux")
        target_link_libraries(${AppName} crypto pthread)
        # link for freebsd
        if(FREEBSD)
            target_link_libraries(${AppName} intl)
        endif()
        # issue on filesystem support of gcc
        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 9.0)
            target_link_libraries(${AppName} stdc++fs)
        endif ()
    endif ()
endif ()

# using c++ standard 17
target_compile_features(${AppName} PUBLIC cxx_std_17)

# if building linux package
if (LINUX AND LINUX_INSTALL_SOFTWARE)
    include(GNUInstallDirs)
    if (INSTALL_GPGFRONTEND_APP)
        install(TARGETS ${AppName} gpgfrontend_core gpgfrontend_ui
                EXPORT GpgFrontendTargets
                RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
        install(DIRECTORY ${PC_GPGME_LIBDIR}
                DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
                FILES_MATCHING PATTERN "libgpgme.so*"
                PATTERN "lib" EXCLUDE)
        install(DIRECTORY ${PC_GPGME_LIBDIR}
                DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
                FILES_MATCHING PATTERN "libassuan.so*"
                PATTERN "lib" EXCLUDE)
        install(DIRECTORY ${PC_GPGME_LIBDIR}
                DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
                FILES_MATCHING PATTERN "libgpg-error.so*"
                PATTERN "lib" EXCLUDE)
        install(FILES ${CMAKE_SOURCE_DIR}/TRANSLATORS
                DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/${AppName}/)
        install(FILES ${CMAKE_SOURCE_DIR}/resource/appstream/com.bktus.gpgfrontend.appdata.xml
                DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/metainfo/)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/desktop/
                DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/applications/)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/lfs/pixmaps/
                DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/pixmaps/)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/lfs/hicolor/
                DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/)
    endif ()
    if (MULTI_LANG_SUPPORT)
        install(DIRECTORY ${LOCALE_OUTPUT_PATH}/
                DESTINATION ${CMAKE_INSTALL_FULL_LOCALEDIR})
    endif ()

    if (APP_PACKAGE_DEB)
        message(STATUS "Configure DEB Package")
        SET(CPACK_GENERATOR "DEB")
        set(CPACK_INSTALL_PREFIX "/usr/local/")
        set(CPACK_PACKAGE_NAME "gpgfrontend")
        set(CPACK_DEBIAN_PACKAGE_NAME "gpgfrontend")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
        set(CPACK_PACKAGE_CONTACT "eric@bktus.com")
        SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Saturneric")

        if (${DISTRO_NAME} MATCHES "Ubuntu")
            if (${DISTRO_VERSION_ID} STREQUAL "22.04")
                set(CPACK_DEBIAN_PACKAGE_DEPENDS "gpg (>= 2.2), libqt5core5a (>= 5.9), libqt5gui5 (>= 5.9), libqt5widgets5 (>= 5.9), libqt5network5 (>= 5.9), libqt5printsupport5 (>= 5.9), libconfig++9v5 (>=1.5), libarchive13(>= 3.4), openssl(>= 1.1.1), libicu70")
            elseif (${DISTRO_VERSION_ID} STREQUAL "20.04")
                set(CPACK_DEBIAN_PACKAGE_DEPENDS "gpg (>= 2.2), libqt5core5a (>= 5.9), libqt5gui5 (>= 5.9), libqt5widgets5 (>= 5.9), libqt5network5 (>= 5.9), libqt5printsupport5 (>= 5.9), libconfig++9v5 (>=1.5), libarchive13(>= 3.4), openssl(>= 1.1.1), libicu66")
            endif ()
        endif ()

        set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
        set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
        set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
        include(CPack)
    endif ()

    if (APP_PACKAGE_RPM)
        message(STATUS "Configure RPM Package")
        SET(CPACK_GENERATOR "RPM")
        set(CPACK_INSTALL_PREFIX "/usr/local/")
        set(CPACK_PACKAGE_NAME "gpgfrontend")
        set(CPACK_DEBIAN_PACKAGE_NAME "gpgfrontend")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
        set(CPACK_PACKAGE_CONTACT "eric@bktus.com")
        SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Saturneric")

        set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
        set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
        set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
        include(CPack)
    endif ()

    if (APP_PACKAGE_FREEBSD)
        message(STATUS "Configure PKG Package")
        SET(CPACK_GENERATOR "FREEBSD")
        set(CPACK_INSTALL_PREFIX "/usr/local/")
        set(CPACK_FREEBSD_PACKAGE_NAME "gpgfrontend")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
        set(CPACK_PACKAGE_CONTACT "eric@bktus.com")
        SET(CPACK_FREEBSD_PACKAGE_MAINTAINER "Saturneric")

        set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
        set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
        set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
        include(CPack)
    endif ()
endif ()

message(STATUS "Resource Files: ${RESOURCE_OUTPUT_DIRECTORY}")
message(STATUS "Locale Files: ${LOCALE_OUTPUT_PATH}")
message(STATUS "Runtime Files: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
