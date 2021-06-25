set(ALL_SOURCE_FILE)

add_subdirectory(gpg)
add_subdirectory(ui)

aux_source_directory(. BASE_SOURCE)

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_SOURCE_DIR}/gpgfrontend.rc")
set_property(SOURCE gpgfrontend.rc APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/gpgfrontend.ico)


file(GLOB_RECURSE GPGFRONTEND_HEADER_FILES RELACTIVE ${CMAKE_SOURCE_DIR}/include/*.h)
qt5_wrap_cpp(QT5_MOCS ${GPGFRONTEND_HEADER_FILES} TARGET gpgfrontend)

# Set Binary Output Path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/release)
message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# Set Resource Output Path
if(APPLE)
    set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Resources)
else()
    set(RESOURCE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()
message(STATUS "RESOURCE_OUTPUT_DIRECTORY ${RESOURCE_OUTPUT_DIRECTORY}")

# Get ALL SOURCE FILES
file(GLOB_RECURSE ALL_SOURCE_FILES RELACTIVE ${CMAKE_SOURCE_DIR}/src/*.cpp)

# Set Translation Files
set(QT_TS_FILES gpgfrontend_en_us.ts gpgfrontend_zh_chs.ts gpgfrontend_zh_cht.ts gpg_frontend_fr.ts gpg_frontend_ru.ts)
list(TRANSFORM QT_TS_FILES PREPEND ${CMAKE_SOURCE_DIR}/resource/ts/)
message(STATUS "QT_TS_FILES ${QT_TS_FILES}")
set(QT_QM_FILES_OUTPUT_DIR ${RESOURCE_OUTPUT_DIRECTORY}/ts)
set_source_files_properties(${QT_TS_FILES} PROPERTIES OUTPUT_LOCATION ${QT_QM_FILES_OUTPUT_DIR})
QT5_create_translation(QON_QM_FILES ${CMAKE_SOURCE_DIR} ${QT_TS_FILES})
message(STATUS "QON_QM_FILES ${QON_QM_FILES}")
add_custom_target(translations DEPENDS ${QON_QM_FILES})

# Set Build Information
configure_file(${CMAKE_SOURCE_DIR}/include/GpgFrontend.h.in ${CMAKE_SOURCE_DIR}/include/GpgFrontend.h @ONLY)

# Copy Resource Files
file(COPY ${CMAKE_SOURCE_DIR}/resource/css DESTINATION ${RESOURCE_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
file(COPY ${CMAKE_SOURCE_DIR}/resource/icons DESTINATION ${RESOURCE_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
file(COPY ${CMAKE_SOURCE_DIR}/resource/conf DESTINATION ${RESOURCE_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)

if(APPLE)
    file(COPY ${CMAKE_SOURCE_DIR}/gpgfrontend.icns DESTINATION ${RESOURCE_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
    # Refresh App Bundle
    file(REMOVE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/gpgfrontend.app)
endif()

# Copy Utils Files
if(MINGW)
    message(STATUS "Copying Dependent DLL For Windows Runtime Env")
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/lib/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/gpgme/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/bearer DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/iconengines DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/imageformats DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/printsupport DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/platforms DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/openssl/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ FOLLOW_SYMLINK_CHAIN)
endif()

set(RESOURCE_FILES ${CMAKE_SOURCE_DIR}/gpgfrontend.qrc ${APP_ICON_RESOURCE_WINDOWS} ${QON_QM_FILES})
add_custom_target(resources ALL DEPENDS ${RESOURCE_FILES})
add_dependencies(resources translations)

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    if(MINGW)
        add_executable(gpgfrontend WIN32 ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
    elseif(APPLE)
        add_executable(gpgfrontend MACOSX_BUNDLE ${ICON_RESOURCE} ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
        set_target_properties(gpgfrontend PROPERTIES
            BUNDLE True
            MACOSX_BUNDLE_GUI_IDENTIFIER org.gnupg.gpgfrontend
            MACOSX_BUNDLE_BUNDLE_NAME "GpgFrontend"
            MACOSX_BUNDLE_LONG_VERSION_STRING ${BUILD_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
            MACOSX_BUNDLE_BUNDLE_VERSION ${BUILD_VERSION}
            MACOSX_BUNDLE_ICON_FILE "gpgfrontend.icns"
            )
        add_custom_command(TARGET gpgfrontend POST_BUILD
            COMMAND  /bin/rm -rf ./gpgfrontend.app/Contents/Resources
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            COMMENT "Deleting Resources IN App Bundle")
        add_custom_command(TARGET gpgfrontend POST_BUILD
            COMMAND  /bin/mv -n ./Resources ./gpgfrontend.app/Contents/
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            COMMENT "Copying Resources INTO App Bundle Resource")
        add_custom_command(TARGET gpgfrontend POST_BUILD
                COMMAND  /bin/rm -rf ./GpgFrontend.app
                WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                COMMENT "Deleting Old Final App Bundle")
        add_custom_command(TARGET gpgfrontend POST_BUILD
                COMMAND  /bin/mv -n ./gpgfrontend.app ./GpgFrontend.app
                WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                COMMENT "Rename Build App Bundle to Final App Bundle")
    endif()
else()
    add_executable(gpgfrontend ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})
endif()

IF (MINGW)
    message(STATUS "Link Application Static Library For MINGW")
    target_link_libraries(gpgfrontend
            gpgfrontend-ui gpg
            Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)


else()
    message(STATUS "Link Application Static Library For UNIX")
    target_link_libraries(gpgfrontend
            gpgfrontend-ui gpg
            Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)
endif()

