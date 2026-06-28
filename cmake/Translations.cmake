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

# The single source of truth for the UI locales GpgFrontend ships. The
# application and every integrated module derive their Qt translation (.ts)
# file set from this list, so the app and its modules always advertise exactly
# the same set of languages. Add a locale here, then run
# scripts/update_module_translations.sh to create the matching .ts files
# everywhere.
set(GPGFRONTEND_SUPPORTED_LOCALES
  en_US
  de_DE
  fr_FR
  it_IT
  es_ES
  zh_CN
  zh_TW
  CACHE INTERNAL "UI locales supported by GpgFrontend and its modules")

# gpgfrontend_collect_ts_files(<out_var> <base_name> <ts_dir>)
#
# Expands GPGFRONTEND_SUPPORTED_LOCALES into the list of .ts paths
#   <ts_dir>/<base_name>.<locale>.ts
# and returns it in <out_var>. Use this instead of hand-listing TS_FILES so a
# locale only ever has to be declared once (above).
function(gpgfrontend_collect_ts_files out_var base_name ts_dir)
  set(_ts_files)
  foreach(_locale IN LISTS GPGFRONTEND_SUPPORTED_LOCALES)
    list(APPEND _ts_files "${ts_dir}/${base_name}.${_locale}.ts")
  endforeach()
  set(${out_var} "${_ts_files}" PARENT_SCOPE)
endfunction()
