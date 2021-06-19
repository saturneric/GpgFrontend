add_subdirectory(gpg)
add_subdirectory(ui)

aux_source_directory(. BASE_SOURCE)

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_SOURCE_DIR}/gpgfrontend.rc")
set_property(SOURCE gpgfrontend.rc APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/gpgfrontend.ico)


file(GLOB_RECURSE GPGFRONTEND_HEADER_FILES RELACTIVE ${CMAKE_SOURCE_DIR}/include/*.h)
qt5_wrap_cpp(QT5_MOCS ${GPGFRONTEND_HEADER_FILES} TARGET gpgfrontend)

# Set Translation Files
file(GLOB_RECURSE QT_TS_FILES RELACTIVE ${CMAKE_SOURCE_DIR}/resource/ts/*.ts)
set(QT_QM_FILES_OUTPUT_DIR ${CMAKE_BINARY_DIR}/src/ts)
set_source_files_properties(${QT_TS_FILES} PROPERTIES OUTPUT_LOCATION ${QT_QM_FILES_OUTPUT_DIR})
QT5_create_translation(QON_QM_FILES ${BASE_SOURCE} ${QT_TS_FILES})
message(STATUS ${QON_QM_FILES})
add_custom_target(translations DEPENDS ${QON_QM_FILES})


# Set Build Information
configure_file(${CMAKE_SOURCE_DIR}/include/GpgFrontend.h.in ${CMAKE_SOURCE_DIR}/include/GpgFrontend.h @ONLY)

# Copy Resource Files
file(COPY ${CMAKE_SOURCE_DIR}/resource/css DESTINATION ${EXECUTABLE_OUTPUT_PATH}/ FOLLOW_SYMLINK_CHAIN)
file(COPY ${CMAKE_SOURCE_DIR}/resource/icons DESTINATION ${EXECUTABLE_OUTPUT_PATH}/ FOLLOW_SYMLINK_CHAIN)
file(COPY ${CMAKE_SOURCE_DIR}/resource/help DESTINATION ${EXECUTABLE_OUTPUT_PATH}/ FOLLOW_SYMLINK_CHAIN)
file(COPY ${CMAKE_SOURCE_DIR}/resource/conf DESTINATION ${EXECUTABLE_OUTPUT_PATH}/ FOLLOW_SYMLINK_CHAIN)
if(MINGW)
    message(STATUS "Copying Dependent DLL For Windows Runtime Env")
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/lib/ DESTINATION ${EXECUTABLE_OUTPUT_PATH}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/gpgme/ DESTINATION ${EXECUTABLE_OUTPUT_PATH}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/bearer DESTINATION ${EXECUTABLE_OUTPUT_PATH}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/iconengines DESTINATION ${EXECUTABLE_OUTPUT_PATH}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/imageformats DESTINATION ${EXECUTABLE_OUTPUT_PATH}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/printsupport DESTINATION ${EXECUTABLE_OUTPUT_PATH}/ FOLLOW_SYMLINK_CHAIN)
    file(COPY ${CMAKE_SOURCE_DIR}/resource/utils/platforms DESTINATION ${EXECUTABLE_OUTPUT_PATH}/ FOLLOW_SYMLINK_CHAIN)
endif()

set(RESOURCE_FILES ${CMAKE_SOURCE_DIR}/gpgfrontend.qrc ${APP_ICON_RESOURCE_WINDOWS} ${QON_QM_FILES})
add_custom_target(resources ALL DEPENDS ${RESOURCE_FILES})
add_dependencies(resources translations)

add_executable(gpgfrontend ${BASE_SOURCE} ${RESOURCE_FILES} ${QT5_MOCS})

IF (MINGW)
    message(STATUS "Link Application Static Library For MINGW")
    target_link_libraries(gpgfrontend
            qtui gpg
            Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)


else()
    message(STATUS "Link Application Static Library For UNIX")
    target_link_libraries(gpgfrontend
            qtui gpg
            Qt5::Network Qt5::PrintSupport Qt5::Widgets Qt5::Test Qt5::Core)
endif()

