/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "ui/dialog/help/AboutStatusInfo.h"

namespace GpgFrontend::UI {

// Note the translation context spelled out at every call below rather than
// hoisted into a constant: lupdate silently skips every translate() call whose
// context is a named constant instead of a literal, which compiles cleanly and
// leaves the strings untranslated at run time.

auto DescribeSessionStorage(bool is_volatile, bool encrypted_at_rest)
    -> AboutStatusValue {
  if (is_volatile) {
    return {QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                        "Memory"),
            QCoreApplication::translate(
                "GpgFrontend::UI::AboutStatusInfo",
                "Not written to this disk in the normal course of things."),
            false};
  }

  if (encrypted_at_rest) {
    return {QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                        "An encrypted folder"),
            QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                        "Only this session can read it."),
            false};
  }

  return {QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                      "An ordinary folder on this disk"),
          QCoreApplication::translate(
              "GpgFrontend::UI::AboutStatusInfo",
              "Nothing here could hold it in memory or encrypt it."),
          true};
}

auto DescribeKeySource(bool self_contained, ProfileKind kind)
    -> AboutStatusValue {
  if (self_contained) {
    return {QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                        "Inside this profile"),
            {},
            false};
  }

  return {
      QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                  "This computer's GnuPG keyring"),
      kind == ProfileKind::kPACKAGED
          ? QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                        "The package carries none of its own.")
          : QString(),
      false};
}

auto ShowsDetailInline(const QString& detail, bool degraded) -> bool {
  return !detail.isEmpty() && degraded;
}

}  // namespace GpgFrontend::UI
