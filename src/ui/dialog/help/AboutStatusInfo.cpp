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

auto BuildProfileIdentityRows(ProfileKind kind, const QString& display_name,
                              bool transient) -> QVector<MetaListRow> {
  QVector<MetaListRow> rows;

  const auto name_caption = QCoreApplication::translate(
      "GpgFrontend::UI::AboutStatusInfo", "Profile:");
  const auto type_caption = QCoreApplication::translate(
      "GpgFrontend::UI::AboutStatusInfo", "Profile Type:");

  // A root profile is not named by its user, so CurrentProfileDisplayName()
  // answers with the kind -- and a type row under it then said the same word a
  // second time. What that row would have carried becomes the sentence here,
  // which is the part that was never on the page at all.
  if (kind == ProfileKind::kINSTALLED_ROOT) {
    rows.append({.caption = name_caption,
                 .value = display_name,
                 .detail = QCoreApplication::translate(
                     "GpgFrontend::UI::AboutStatusInfo",
                     "The profile this computer starts on, in your user data "
                     "folder."),
                 .emphasis = true});
    return rows;
  }

  if (kind == ProfileKind::kPORTABLE_ROOT) {
    rows.append({.caption = name_caption,
                 .value = display_name,
                 .detail = QCoreApplication::translate(
                     "GpgFrontend::UI::AboutStatusInfo",
                     "Kept beside the application, so it travels with it."),
                 .emphasis = true});
    return rows;
  }

  rows.append(
      {.caption = name_caption, .value = display_name, .emphasis = true});

  if (kind == ProfileKind::kPACKAGED) {
    rows.append(
        {.caption = type_caption,
         .value = QCoreApplication::translate(
             "GpgFrontend::UI::AboutStatusInfo", "Opened from a profile file"),
         // Asked of the profile rather than of the kind, so a shape
         // added later cannot inherit the sentence by being packaged.
         .detail = transient ? QCoreApplication::translate(
                                   "GpgFrontend::UI::AboutStatusInfo",
                                   "Temporary. It disappears when this "
                                   "window closes, and closing asks whether "
                                   "to save your changes back into the file.")
                             : QString()});
    return rows;
  }

  rows.append(
      {.caption = type_caption,
       .value = QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                            "Kept on this computer")});
  return rows;
}

auto DescribeAppKeyProtection(AppKeyProtection protection, bool allows_keychain)
    -> AboutStatusValue {
  QString value;
  switch (protection) {
    case AppKeyProtection::kKEYCHAIN:
      value = QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                          "System keychain");
      break;
    case AppKeyProtection::kPIN:
      value = QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                          "PIN at startup");
      break;
    case AppKeyProtection::kNONE:
      value = QCoreApplication::translate("GpgFrontend::UI::AboutStatusInfo",
                                          "No extra protection");
      break;
  }

  // Not a fallback, so not degraded: the rule is what keeps the profile
  // openable elsewhere. It is still the answer to a question the settings page
  // raises by greying the option out and then not explaining itself.
  if (!allows_keychain) {
    return {value,
            QCoreApplication::translate(
                "GpgFrontend::UI::AboutStatusInfo",
                "This profile can leave this computer, so the system keychain "
                "is not offered: a key sealed with one computer's keychain "
                "cannot be opened on another."),
            false};
  }

  return {value, {}, false};
}

auto ShowsDetailInline(const QString& detail, bool degraded) -> bool {
  return !detail.isEmpty() && degraded;
}

}  // namespace GpgFrontend::UI
