# - Try to find the gpgme library
#
# Algorithm:
#  - Windows:
#    On Windows, there's three gpgme variants: gpgme{,-glib,-qt}.
#    - The variant used determines the event loop integration possible:
#      - gpgme:      no event loop integration possible, only synchronous operations supported
#      - gpgme-glib: glib event loop integration possible, only asynchronous operations supported
#      - gpgme-qt:   qt event loop integration possible, only asynchronous operations supported
#    - GPGME_{VANILLA,GLIB,QT}_{FOUND,LIBRARIES} will be set for each of the above
#    - GPGME_INCLUDES is the same for all of the above
#    - GPGME_FOUND is set if any of the above was found
#  - *nix:
#    There's also three variants: gpgme{,-pthread,-pth}.
#    - The variant used determines the multithreaded use possible:
#      - gpgme:         no multithreading support available
#      - gpgme-pthread: multithreading available using POSIX threads
#      - gpgme-pth:     multithreading available using GNU PTH (cooperative multithreading)
#    - GPGME_{VANILLA,PTH,PTHREAD}_{FOUND,LIBRARIES} will be set for each of the above
#    - GPGME_INCLUDES is the same for all of the above
#    - GPGME_FOUND is set if any of the above was found
#
#  GPGME_LIBRARY_DIR - the directory where the libraries are located

#
# THIS IS ALMOST A 1:1 COPY OF FindAssuan.cmake in kdepim.
# Any changes here likely apply there, too.
#

find_package(PkgConfig)

#if this is built-in, please replace, if it isn't, export into a MacroToBool.cmake of it's own
macro( macro_bool_to_bool FOUND_VAR )
  foreach( _current_VAR ${ARGN} )
    if ( ${FOUND_VAR} )
      set( ${_current_VAR} TRUE )
    else()
      set( ${_current_VAR} FALSE )
    endif()
  endforeach()
endmacro()

#HACK: local copy...
MACRO(MACRO_BOOL_TO_01 FOUND_VAR )
   FOREACH (_current_VAR ${ARGN})
      IF(${FOUND_VAR})
         SET(${_current_VAR} 1)
      ELSE(${FOUND_VAR})
         SET(${_current_VAR} 0)
      ENDIF(${FOUND_VAR})
   ENDFOREACH(_current_VAR)
ENDMACRO(MACRO_BOOL_TO_01)

pkg_search_module(PC_GPGME gpgme)

if (PC_GPGME_FOUND)
  set(GPGME_INCLUDES ${PC_GPGME_INCLUDE_DIRS})
  set(GPGME_LIBRARIES ${PC_GPGME_LINK_LIBRARIES})
  set(GPGME_VERSION ${PC_GPGME_VERSION})
  set(GPGME_LIBRARY_DIR ${PC_GPGME_LIBRARY_DIR})
elseif ( WIN32 )

  # On Windows, we don't have a gpgme-config script, so we need to
  # look for the stuff ourselves:

  # in cmake, AND and OR have the same precedence, there's no
  # subexpressions, and expressions are evaluated short-circuit'ed
  # IOW: CMake if() suxx.
  # Starting with CMake 2.6.3 you can group if expressions with (), but we 
  # don't require 2.6.3 but 2.6.2, we can't use it. Alex
  set( _seem_to_have_cached_gpgme false )
  if ( GPGME_INCLUDES )
    if ( GPGME_VANILLA_LIBRARIES OR GPGME_QT_LIBRARIES OR GPGME_GLIB_LIBRARIES )
      set( _seem_to_have_cached_gpgme true )
    endif()
  endif()

  if ( _seem_to_have_cached_gpgme )

    macro_bool_to_bool( GPGME_VANILLA_LIBRARIES  GPGME_VANILLA_FOUND )
    macro_bool_to_bool( GPGME_GLIB_LIBRARIES     GPGME_GLIB_FOUND    )
    macro_bool_to_bool( GPGME_QT_LIBRARIES       GPGME_QT_FOUND      )
    # this would have been preferred:
    #set( GPGME_*_FOUND macro_bool_to_bool(GPGME_*_LIBRARIES) )

    if ( GPGME_VANILLA_FOUND OR GPGME_GLIB_FOUND OR GPGME_QT_FOUND )
      set( GPGME_FOUND true )
    else()
      set( GPGME_FOUND false )
    endif()

  else()

    set( GPGME_FOUND         false )
    set( GPGME_VANILLA_FOUND false )
    set( GPGME_GLIB_FOUND    false )
    set( GPGME_QT_FOUND      false )

    find_path( GPGME_INCLUDES gpgme.h
      ${CMAKE_INCLUDE_PATH}
      ${CMAKE_INSTALL_PREFIX}/include
    )

    find_library( _gpgme_vanilla_library NAMES gpgme libgpgme gpgme-11 libgpgme-11
      PATHS 
        ${CMAKE_LIBRARY_PATH}
        ${CMAKE_INSTALL_PREFIX}/lib
    )

    find_library( _gpgme_glib_library    NAMES gpgme-glib libgpgme-glib gpgme-glib-11 libgpgme-glib-11
      PATHS 
        ${CMAKE_LIBRARY_PATH}
        ${CMAKE_INSTALL_PREFIX}/lib
    )

    find_library( _gpgme_qt_library      NAMES gpgme-qt libgpgme-qt gpgme-qt-11 libgpgme-qt-11
      PATHS 
        ${CMAKE_LIBRARY_PATH}
        ${CMAKE_INSTALL_PREFIX}/lib
    )

    find_library( _gpg_error_library     NAMES gpg-error libgpg-error gpg-error-0 libgpg-error-0
       PATHS
            ${CMAKE_LIBRARY_PATH}
            ${CMAKE_INSTALL_PREFIX}/lib
    )

    set( GPGME_INCLUDES ${GPGME_INCLUDES} )

    if ( _gpgme_vanilla_library AND _gpg_error_library )
      set( GPGME_VANILLA_LIBRARIES ${_gpgme_vanilla_library} ${_gpg_error_library} )
      set( GPGME_VANILLA_FOUND     true )
      set( GPGME_FOUND             true )
    endif()

    if ( _gpgme_glib_library AND _gpg_error_library )
      set( GPGME_GLIB_LIBRARIES    ${_gpgme_glib_library}    ${_gpg_error_library} )
      set( GPGME_GLIB_FOUND        true )
      set( GPGME_FOUND             true )
    endif()

    if ( _gpgme_qt_library AND _gpg_error_library )
      set( GPGME_QT_LIBRARIES      ${_gpgme_qt_library}      ${_gpg_error_library} )
      set( GPGME_QT_FOUND          true )
      set( GPGME_FOUND             true )
    endif()

  endif()

  # these are Unix-only:
  set( GPGME_PTHREAD_FOUND false )
  set( GPGME_PTH_FOUND     false )
  set( HAVE_GPGME_PTHREAD  0     )
  set( HAVE_GPGME_PTH      0     )

  macro_bool_to_01( GPGME_FOUND         HAVE_GPGME         )
  macro_bool_to_01( GPGME_VANILLA_FOUND HAVE_GPGME_VANILLA )
  macro_bool_to_01( GPGME_GLIB_FOUND    HAVE_GPGME_GLIB    )
  macro_bool_to_01( GPGME_QT_FOUND      HAVE_GPGME_QT      )

else() # not WIN32

  # On *nix, we have the gpgme-config script which can tell us all we
  # need to know:

  # see WIN32 case for an explanation of what this does:
  set( _seem_to_have_cached_gpgme false )
  if ( GPGME_INCLUDES )
    if ( GPGME_VANILLA_LIBRARIES OR GPGME_PTHREAD_LIBRARIES OR GPGME_PTH_LIBRARIES )
      set( _seem_to_have_cached_gpgme true )
    endif()
  endif()

  if ( _seem_to_have_cached_gpgme )

    macro_bool_to_bool( GPGME_VANILLA_LIBRARIES GPGME_VANILLA_FOUND )
    macro_bool_to_bool( GPGME_PTHREAD_LIBRARIES GPGME_PTHREAD_FOUND )
    macro_bool_to_bool( GPGME_PTH_LIBRARIES     GPGME_PTH_FOUND     )

    if ( GPGME_VANILLA_FOUND OR GPGME_PTHREAD_FOUND OR GPGME_PTH_FOUND )
      set( GPGME_FOUND true )
    else()
      set( GPGME_FOUND false )
    endif()

  else()

    set( GPGME_FOUND         false )
    set( GPGME_VANILLA_FOUND false )
    set( GPGME_PTHREAD_FOUND false )
    set( GPGME_PTH_FOUND     false )

    find_program( _GPGMECONFIG_EXECUTABLE NAMES gpgme-config )

    # if gpgme-config has been found
    if ( _GPGMECONFIG_EXECUTABLE )

      message( STATUS "Found gpgme-config at ${_GPGMECONFIG_EXECUTABLE}" )

      exec_program( ${_GPGMECONFIG_EXECUTABLE} ARGS --version OUTPUT_VARIABLE GPGME_VERSION )

      set( _GPGME_MIN_VERSION "1.4.3" )

      if ( ${GPGME_VERSION} VERSION_LESS ${_GPGME_MIN_VERSION} )

        message( STATUS "The installed version of gpgme is too old: ${GPGME_VERSION} (required: >= ${_GPGME_MIN_VERSION})" )

      else()

        message( STATUS "Found gpgme v${GPGME_VERSION}, checking for flavors..." )

        exec_program( ${_GPGMECONFIG_EXECUTABLE} ARGS                  --libs OUTPUT_VARIABLE _gpgme_config_vanilla_libs RETURN_VALUE _ret )
	if ( _ret )
	  set( _gpgme_config_vanilla_libs )
	endif()

        exec_program( ${_GPGMECONFIG_EXECUTABLE} ARGS --thread=pthread --libs OUTPUT_VARIABLE _gpgme_config_pthread_libs RETURN_VALUE _ret )
	if ( _ret )
	  set( _gpgme_config_pthread_libs )
	endif()

        exec_program( ${_GPGMECONFIG_EXECUTABLE} ARGS --thread=pth     --libs OUTPUT_VARIABLE _gpgme_config_pth_libs     RETURN_VALUE _ret )
	if ( _ret )
	  set( _gpgme_config_pth_libs )
	endif()

        # append -lgpg-error to the list of libraries, if necessary
        foreach ( _flavour vanilla pthread pth )
          if ( _gpgme_config_${_flavour}_libs AND NOT _gpgme_config_${_flavour}_libs MATCHES "lgpg-error" )
            set( _gpgme_config_${_flavour}_libs "${_gpgme_config_${_flavour}_libs} -lgpg-error" )
          endif()
        endforeach()

        if ( _gpgme_config_vanilla_libs OR _gpgme_config_pthread_libs OR _gpgme_config_pth_libs )

          exec_program( ${_GPGMECONFIG_EXECUTABLE} ARGS --cflags OUTPUT_VARIABLE _GPGME_CFLAGS )

          if ( _GPGME_CFLAGS )
            string( REGEX REPLACE "(\r?\n)+$" " " _GPGME_CFLAGS  "${_GPGME_CFLAGS}" )
            string( REGEX REPLACE " *-I"      ";" GPGME_INCLUDES "${_GPGME_CFLAGS}" )
          endif()

          foreach ( _flavour vanilla pthread pth )
            if ( _gpgme_config_${_flavour}_libs )

              set( _gpgme_library_dirs )
              set( _gpgme_library_names )
              string( TOUPPER "${_flavour}" _FLAVOUR )

              string( REGEX REPLACE " +" ";" _gpgme_config_${_flavour}_libs "${_gpgme_config_${_flavour}_libs}" )

              foreach( _flag ${_gpgme_config_${_flavour}_libs} )
                if ( "${_flag}" MATCHES "^-L" )
                  string( REGEX REPLACE "^-L" "" _dir "${_flag}" )
                  file( TO_CMAKE_PATH "${_dir}" _dir )
                  set( _gpgme_library_dirs ${_gpgme_library_dirs} "${_dir}" )
                elseif( "${_flag}" MATCHES "^-l" )
                  string( REGEX REPLACE "^-l" "" _name "${_flag}" )
                  set( _gpgme_library_names ${_gpgme_library_names} "${_name}" )
                endif()
              endforeach()

              set( GPGME_${_FLAVOUR}_FOUND true )

              foreach( _name ${_gpgme_library_names} )
                set( _gpgme_${_name}_lib )

                # if -L options were given, look only there
                if ( _gpgme_library_dirs )
                  find_library( _gpgme_${_name}_lib NAMES ${_name} PATHS ${_gpgme_library_dirs} NO_DEFAULT_PATH )
                endif()

                # if not found there, look in system directories
                if ( NOT _gpgme_${_name}_lib )
                  find_library( _gpgme_${_name}_lib NAMES ${_name} )
                endif()

                # if still not found, then the whole flavor isn't found
                if ( NOT _gpgme_${_name}_lib )
                  if ( GPGME_${_FLAVOUR}_FOUND )
                    set( GPGME_${_FLAVOUR}_FOUND false )
                    set( _not_found_reason "dependent library ${_name} wasn't found" )
                  endif()
                endif()

                set( GPGME_${_FLAVOUR}_LIBRARIES ${GPGME_${_FLAVOUR}_LIBRARIES} "${_gpgme_${_name}_lib}" )
              endforeach()

              #check_c_library_exists_explicit( gpgme         gpgme_check_version "${_GPGME_CFLAGS}" "${GPGME_LIBRARIES}"         GPGME_FOUND         )
              if ( GPGME_${_FLAVOUR}_FOUND )
                message( STATUS " Found flavor '${_flavour}', checking whether it's usable...yes" )
              else()
                message( STATUS " Found flavor '${_flavour}', checking whether it's usable...no" )
                message( STATUS "  (${_not_found_reason})" )
              endif()
            endif()

          endforeach( _flavour )

          # ensure that they are cached
          # This comment above doesn't make sense, the four following lines seem to do nothing. Alex
          set( GPGME_INCLUDES          ${GPGME_INCLUDES} )
          set( GPGME_VANILLA_LIBRARIES ${GPGME_VANILLA_LIBRARIES} )
          set( GPGME_PTHREAD_LIBRARIES ${GPGME_PTHREAD_LIBRARIES} )
          set( GPGME_PTH_LIBRARIES     ${GPGME_PTH_LIBRARIES} )

          if ( GPGME_VANILLA_FOUND OR GPGME_PTHREAD_FOUND OR GPGME_PTH_FOUND )
            set( GPGME_FOUND true )
          else()
            set( GPGME_FOUND false )
          endif()

        endif()

      endif()

    endif()

  endif()

  # these are Windows-only:
  set( GPGME_GLIB_FOUND false )
  set( GPGME_QT_FOUND   false )
  set( HAVE_GPGME_GLIB  0     )
  set( HAVE_GPGME_QT    0     )

  macro_bool_to_01( GPGME_FOUND         HAVE_GPGME         )
  macro_bool_to_01( GPGME_VANILLA_FOUND HAVE_GPGME_VANILLA )
  macro_bool_to_01( GPGME_PTHREAD_FOUND HAVE_GPGME_PTHREAD )
  macro_bool_to_01( GPGME_PTH_FOUND     HAVE_GPGME_PTH     )

endif() # WIN32 | Unix


set( _gpgme_flavours "" )

if ( GPGME_VANILLA_FOUND )
  set( _gpgme_flavours "${_gpgme_flavours} vanilla" )
endif()

if ( GPGME_GLIB_FOUND )
  set( _gpgme_flavours "${_gpgme_flavours} Glib" )
endif()

if ( GPGME_QT_FOUND )
  set( _gpgme_flavours "${_gpgme_flavours} Qt" )
endif()

if ( GPGME_PTHREAD_FOUND )
  set( _gpgme_flavours "${_gpgme_flavours} pthread" )
endif()

if ( GPGME_PTH_FOUND )
  set( _gpgme_flavours "${_gpgme_flavours} pth" )
endif()

# determine the library in one of the found flavors, can be reused e.g. by FindQgpgme.cmake, Alex
foreach(_currentFlavour vanilla glib qt pth pthread)
   if(NOT GPGME_LIBRARY_DIR)
      get_filename_component(GPGME_LIBRARY_DIR "${_gpgme_${_currentFlavour}_lib}" PATH)
   endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gpgme
                                  REQUIRED_VARS GPGME_LIBRARIES
                                  VERSION_VAR GPGME_VERSION
                                  )

if ( WIN32 )
  set( _gpgme_homepage "https://www.gpg4win.org" )
else()
  set( _gpgme_homepage "https://www.gnupg.org/related_software/gpgme" )
endif()
