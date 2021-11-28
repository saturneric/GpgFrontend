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
    add_subdirectory(smtp)
endif ()

if (SERVER_SUPPORT)
    message(STATUS "Build Server Support")
    add_subdirectory(server)
endif ()

if (ADVANCE_SUPPORT)
    message(STATUS "Build Advance Support")
    add_subdirectory(advance)
endif ()

if (APPLICATION_BUILD)
    aux_source_directory(. BASE_SOURCE)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_SOURCE_DIR}/gpgfrontend.rc")
    set_property(SOURCE gpgfrontend.rc APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/gpgfrontend.ico)

    # Set Binary Output Path
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/release)
    message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif ()


if (APPLICATION_BUILD)
    # Set Resource Output Path
    if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
        if (APPLE)
            set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Resources)
        elseif (LINUX)
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

if (MULTI_LANG_SUPPORT)
    # Set Translation Files
    set(QT_TS_FILES
            gpgfrontend_en_us.ts gpgfrontend_zh_cn.ts
            gpgfrontend_fr.ts gpgfrontend_ru.ts gpgfrontend_es.ts)
    list(TRANSFORM QT_TS_FILES PREPEND ${CMAKE_SOURCE_DIR}/resource/ts/)
    message(STATUS "QT_TS_FILES ${QT_TS_FILES}")
    set(QT_QM_FILES_OUTPUT_DIR ${RESOURCE_OUTPUT_DIRECTORY}/ts)
    set_source_files_properties(${QT_TS_FILES} PROPERTIES OUTPUT_LOCATION ${QT_QM_FILES_OUTPUT_DIR})
    QT5_create_translation(QON_QM_FILES ${CMAKE_SOURCE_DIR} ${QT_TS_FILES})
    message(STATUS "QON_QM_FILES ${QON_QM_FILES}")
    add_custom_target(translations DEPENDS ${QON_QM_FILES})
endif ()

if (BASIC_ENV_CONFIG)
    # Set Build Information
    configure_file(${CMAKE_SOURCE_DIR}/src/GpgFrontend.h.in ${CMAKE_SOURCE_DIR}/src/GpgFrontend.h @ONLY)
    configure_file(${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInfo.h.in ${CMAKE_SOURCE_DIR}/src/GpgFrontendBuildInfo.h @ONLY)
endif ()

if (APPLICATION_BUILD)
    # Copy Resource Files
    file(COPY ${CMAKE_SOURCE_DIR}/resource/css DESTINATION ${RESOURCE_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/icons DESTINATION ${RESOURCE_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/conf DESTINATION ${RESOURCE_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
endif ()

if (APPLICATION_BUILD)
    if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
        if (APPLE)
            file(COPY ${CMAKE_SOURCE_DIR}/gpgfrontend.icns DESTINATION ${RESOURCE_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
            # Refresh App Bundle
            file(REMOVE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${AppName}.app)
        elseif (LINUX)
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
        elseif (APPLE)
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
        elseif (LINUX)
            add_executable(${AppName} ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
            add_custom_command(TARGET ${AppName} POST_BUILD
                    COMMAND /bin/mkdir ./gpgfrontend/usr/bin && /bin/mv -f ./${AppName} ./gpgfrontend/usr/bin/
                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                    COMMENT "Copying Binary into App Image")
            add_custom_command(TARGET ${AppName} POST_BUILD
                    COMMAND /bin/mkdir ./gpgfrontend/usr/lib
                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                    COMMENT "Complement to build the required architecture")
        else ()
            add_executable(${AppName} ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
        endif ()
    else ()
        add_executable(${AppName} ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
    endif ()
endif ()

if (APPLICATION_BUILD)

    if (ADVANCE_SUPPORT)
        set(GPGFRONTEND_BEFORE_UI_LIBS ${GPGFRONTEND_BEFORE_UI_LIBS} advance)
    endif ()
    if (ADVANCE_SUPPORT)
        set(GPGFRONTEND_BEFORE_UI_LIBS ${GPGFRONTEND_BEFORE_UI_LIBS} server)
    endif ()
    if (SMTP_SUPPORT)
        set(GPGFRONTEND_AFTER_UI_LIBS ${GPGFRONTEND_AFTER_UI_LIBS} server)
    endif ()


    set(GPGFRONTEND_LIBS ${GPGFRONTEND_AFTER_UI_LIBS} gpgfrontend-ui ${GPGFRONTEND_BEFORE_UI_LIBS} gpg_core easy_logging_pp)

    set(QT_DEPENDENCY_LIBS Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)
    IF (MINGW)
        message(STATUS "Link Application Static Library For MINGW")
        target_link_libraries(${AppName}
                ${GPGFRONTEND_LIBS}
                ${QT_DEPENDENCY_LIBS}
                crypto ssl)
    elseif (APPLE)
        message(STATUS "Link Application Static Library For macOS")
        target_link_libraries(${AppName}
                ${GPGFRONTEND_LIBS}
                ${QT_DEPENDENCY_LIBS}
                crypto ssl)
    else ()
        message(STATUS "Link Application Static Library For UNIX")
        target_link_libraries(${AppName}
                ${GPGFRONTEND_LIBS}
                ${QT_DEPENDENCY_LIBS}
                crypto ssl pthread)
    endif ()
endif ()

