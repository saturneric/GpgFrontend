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

# The interface logic the desktop (gf_ui, widgets) and the phone (gf_mob, QML)
# share: what may be done and why not, the steps of an operation, what a result
# means and the words that say so. Above the core, which stays the engines and
# the data, and below both interfaces. Nothing here includes a widget or a QML
# type, and nothing in the core includes anything here.
#
# Shared, not static: what keeps state here exists once per process, however
# many libraries reach it.
aux_source_directory(. SHELL_SOURCE)
aux_source_directory(rules SHELL_SOURCE)
aux_source_directory(models SHELL_SOURCE)
aux_source_directory(present SHELL_SOURCE)
aux_source_directory(result_analyse SHELL_SOURCE)

register_library(shell LIBRARY_TARGET ${SHELL_SOURCE})

target_link_libraries(${LIBRARY_TARGET} PUBLIC gf_core Qt::Core Qt::Gui
  Qt::Network)

target_precompile_headers(${LIBRARY_TARGET} PUBLIC GpgFrontendShell.h)
