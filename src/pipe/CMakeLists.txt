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

# gfpipe: basic OpenPGP operations with the keys of a key package, for pipes
# and for git. Its logic is in the Rust engine (rust/src/pipe); main.cpp only
# calls it. It links the engine and the C library and nothing else -- no Qt,
# no library of GpgFrontend's -- so a copy runs on its own, and it starts in
# about a millisecond.
add_executable(gfpipe main.cpp PipeHost.cpp)
# No Qt: nothing for moc, uic or rcc.
set_target_properties(gfpipe PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

# GpgFrontendBuildInfo.h gives the version --version prints.
target_compile_definitions(gfpipe PRIVATE HAS_RUST_SUPPORT)
target_include_directories(gfpipe PRIVATE
  ${CMAKE_SOURCE_DIR}/src)
target_compile_features(gfpipe PRIVATE cxx_std_17)
target_link_libraries(gfpipe PRIVATE gf_rust)

# Most of the engine is key management gfpipe never calls; leave it out.
if(APPLE)
  target_link_options(gfpipe PRIVATE "LINKER:-dead_strip")
elseif(MSVC)
  target_link_options(gfpipe PRIVATE "/OPT:REF")
else()
  target_link_options(gfpipe PRIVATE "LINKER:--gc-sections")
endif()

# The C++ runtime too, where the toolchain allows it: a copy put next to a
# portable install then needs only the C library.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT APPLE)
  target_link_options(gfpipe PRIVATE -static-libstdc++ -static-libgcc)
endif()

if(NOT APPLE)
  install(TARGETS gfpipe RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
