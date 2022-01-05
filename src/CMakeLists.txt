if (GPG_CORE)
    message(STATUS "Build Gpg Core")
    add_subdirectory(gpg)
endif ()

if (UI_CORE)
    message(STATUS "Build UI Core")
    add_subdirectory(ui)
endif ()

if (SMTP_SUPPORT)
    message(STATUS "Build SMTP Support")
    add_compile_definitions(SMTP_SUPPORT)
    add_subdirectory(smtp)
endif ()

if (SERVER_SUPPORT)
    message(STATUS "Build Server Support")
    add_compile_definitions(SERVER_SUPPORT)
    add_subdirectory(server)
endif ()

if (ADVANCE_SUPPORT)
    message(STATUS "Build Advance Support")
    add_compile_definitions(ADVANCE_SUPPORT)
    add_subdirectory(advance)
endif ()

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
    message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif ()


if (APPLICATION_BUILD)
    # Set Resource Output Path
    if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
        if (APPLE)
            set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Resources)
        elseif (LINUX AND NOT LINUX_INSTALL_SOFTWARE)
            file(COPY ${CMAKE_SOURCE_DIR}/resource/gpgfrontend DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
            set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/gpgfrontend/usr/share)
        else ()
            set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        endif ()
    else ()
        set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    endif ()
    message(STATUS "RESOURCE_OUTPUT_DIRECTORY ${RESOURCE_OUTPUT_DIRECTORY}")
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
    message(STATUS "LOCALE_OUTPUT_PATH ${LOCALE_OUTPUT_PATH}")

    if (NOT GETTEXT_MSGFMT_EXECUTABLE OR NOT GETTEXT_XGETTEXT_EXECUTABLE)
        message(ERROR "msgfmt or xgettext not found. Translations will *not* be installed")
    else (NOT GETTEXT_MSGFMT_EXECUTABLE)
        message(STATUS "Setting target translations")
        add_custom_target(translations)
        set(OUTPUT_POT_PATH ${CMAKE_SOURCE_DIR}/resource/locale/template/${PROJECT_NAME}.pot)
        add_custom_command(
                TARGET translations
                COMMAND find ${CMAKE_SOURCE_DIR}/src -iname \"*.cpp\" | xargs xgettext --package-name=${PROJECT_NAME} --copyright-holder=Saturneric --package-version=${PROJECT_VERSION} --msgid-bugs-address=eric@bktus.com --add-comments="/*" --c++ -k_ -o ${OUTPUT_POT_PATH}
        )

        file(GLOB ALL_PO_FILES ${CMAKE_SOURCE_DIR}/resource/locale/po/*.po)
        SET(GMO_FILES)
        message(STATUS "ALL_PO_FILES ${ALL_PO_FILES}")

        foreach (_poFile ${ALL_PO_FILES})
            GET_FILENAME_COMPONENT(_poFileName ${_poFile} NAME)
            string(REGEX REPLACE "\\.[^.]*$" "" _langName ${_poFileName})
            message(STATUS "_poFileName ${_langName}")
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
endif ()

if (APPLICATION_BUILD)
    # Copy Resource Files
    file(COPY ${CMAKE_SOURCE_DIR}/resource/css DESTINATION ${RESOURCE_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/icons DESTINATION ${RESOURCE_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
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
        file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/lib/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
        file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/gpgme/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
        file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/bearer DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
        file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/iconengines DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
        file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/imageformats DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
        file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/printsupport DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
        file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/platforms DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
        file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/openssl/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
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
        elseif (APPLE AND NOT XCODE_BUILD)
            add_executable(${AppName} MACOSX_BUNDLE ${ICON_RESOURCE} ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
            set_target_properties(${AppName} PROPERTIES
                    BUNDLE True
                    MACOSX_BUNDLE_GUI_IDENTIFIER pub.gpgfrontend.gpgfrontend
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
        elseif (APPLE AND XCODE_BUILD)
            add_executable(${AppName} MACOSX_BUNDLE ${ICON_RESOURCE} ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
            set_target_properties(${AppName} PROPERTIES
                    BUNDLE True
                    MACOSX_BUNDLE_GUI_IDENTIFIER pub.gpgfrontend.gpgfrontend
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
            set_target_properties(${AppName} PROPERTIES
                    XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
                    XCODE_ATTRIBUTE_SKIP_INSTALL "No"
                    XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--deep --entitlements ${CMAKE_SOURCE_DIR}/resource/plist/entitlement.plist"
                    XCODE_ATTRIBUTE_ENABLE_APP_SANDBOX "Yes"
                    )
        else ()
            add_executable(${AppName} ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
        endif ()
    else ()
        add_executable(${AppName} ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
    endif ()

    # Make app build with resources
    add_dependencies(${AppName} resources)

endif ()

if (APPLICATION_BUILD)

    if (ADVANCE_SUPPORT)
        set(GPGFRONTEND_BEFORE_UI_LIBS ${GPGFRONTEND_BEFORE_UI_LIBS} advance)
    endif ()
    if (ADVANCE_SUPPORT)
        set(GPGFRONTEND_BEFORE_UI_LIBS ${GPGFRONTEND_BEFORE_UI_LIBS} server)
    endif ()
    if (SMTP_SUPPORT)
        set(GPGFRONTEND_BEFORE_UI_LIBS ${GPGFRONTEND_BEFORE_UI_LIBS} smtp)
    endif ()

    set(GPGFRONTEND_LIBS ${GPGFRONTEND_AFTER_UI_LIBS} gpgfrontend_ui gpgfrontend_core ${GPGFRONTEND_BEFORE_UI_LIBS} easy_logging_pp)
    set(QT_DEPENDENCY_LIBS Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)
    message(STATUS "Boost Libraries ${Boost_LIBRARIES}")

    IF (MINGW)
        message(STATUS "Link Application Static Library For MINGW")
        find_library(libconfig NAMES libconfig++.a)
        find_library(libintl NAMES libintl.a)
        find_library(libiconv NAMES libiconv.a)

        target_link_libraries(${AppName}
                ${GPGFRONTEND_LIBS}
                ${QT_DEPENDENCY_LIBS}
                ${Boost_LIBRARIES}
                ${libintl} ${libiconv} ${libconfig} crypto ssl)
    elseif (APPLE)
        message(STATUS "Link Application Static Library For macOS")
        target_link_libraries(${AppName}
                ${GPGFRONTEND_LIBS}
                ${QT_DEPENDENCY_LIBS}
                ${Boost_LIBRARIES}
                crypto ssl intl)
    else ()
        message(STATUS "Link Application Static Library For UNIX ")
        if (APP_IMAGE_UPDATE)
            target_link_libraries(${AppName}
                    libappimageupdate)
        endif ()
        target_link_libraries(${AppName}
                ${GPGFRONTEND_LIBS}
                ${QT_DEPENDENCY_LIBS}
                ${Boost_LIBRARIES}
                crypto ssl pthread)
    endif ()
endif ()

if (LINUX_INSTALL_SOFTWARE)
    if (LINUX)
        if (INSTALL_GPGFRONTEND_APP)
            install(TARGETS ${AppName}
                    EXPORT GpgFrontendTargets
                    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                    )
            install(FILES ${CMAKE_SOURCE_DIR}/TRANSLATORS
                    DESTINATION /usr/local/share/${AppName}/)
            install(FILES ${CMAKE_SOURCE_DIR}/resource/meta/pub.gpgfrontend.gpgfrontend.appdata.xml
                    DESTINATION /usr/share/metainfo/)
            install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/desktop/
                    DESTINATION /usr/share/applications/)
            install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/pixmaps/
                    DESTINATION /usr/share/pixmaps/)
            install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/hicolor/
                    DESTINATION /usr/share/icons/hicolor/)
        endif ()
        if (MULTI_LANG_SUPPORT)
            message(STATUS "Local Output Path ${LOCALE_OUTPUT_PATH}")
            install(DIRECTORY ${LOCALE_OUTPUT_PATH}/
                    DESTINATION ${CMAKE_INSTALL_FULL_LOCALEDIR})
        endif ()
        if (APP_PACKAGE_DEB)
            SET(CPACK_GENERATOR "DEB")
            set(CPACK_INSTALL_PREFIX "/usr/local/")
            set(CPACK_PACKAGE_NAME "gpgfrontend")
            set(CPACK_DEBIAN_PACKAGE_NAME "gpgfrontend")
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
            set(CPACK_PACKAGE_CONTACT "eric@bktus.com")
            SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Saturneric")
            set(CPACK_DEBIAN_PACKAGE_DEPENDS "gpg (>= 2.2), libqt5core5a (>= 5.9), libqt5gui5 (>= 5.9), libqt5widgets5 (>= 5.9), libqt5network5 (>= 5.9), libqt5printsupport5 (>= 5.9), libconfig++-dev (>=1.5)")
            set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
            set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
            set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
            include(CPack)
        endif ()
    endif ()
endif ()

