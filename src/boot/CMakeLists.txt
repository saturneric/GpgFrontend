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

# Startup and shutdown shared by every program that opens a profile and starts
# the core, without an interface: the desktop application, and the core's own
# test runner. Static, so it needs no export header and lands in each program
# that links it.
aux_source_directory(. BOOT_SOURCE)

add_library(gf_boot STATIC ${BOOT_SOURCE})

# The system keychain backends, picked per platform. Here rather than in the
# application, because opening a profile installs them.
include(${CMAKE_SOURCE_DIR}/src/plat/PlatformSources.cmake)
target_sources(gf_boot PRIVATE ${GF_PLATFORM_APP_SOURCES})

target_link_libraries(gf_boot PUBLIC gf_core sodium)

if(MINGW OR WIN32)
  # Advapi32 provides the Credential Manager API (CredReadW and friends).
  target_link_libraries(gf_boot PRIVATE Advapi32)
elseif(APPLE)
  find_library(SECURITY_FRAMEWORK Security REQUIRED)
  find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
  target_link_libraries(gf_boot PRIVATE
    ${SECURITY_FRAMEWORK}
    ${COREFOUNDATION_FRAMEWORK})
elseif(NOT ANDROID)
  # libsecret is resolved at runtime through QLibrary, so nothing to link.
  # Android's libc carries the threads itself.
  target_link_libraries(gf_boot PRIVATE pthread)
endif()
