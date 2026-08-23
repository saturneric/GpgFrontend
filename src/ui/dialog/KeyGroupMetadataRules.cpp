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

#include "KeyGroupMetadataRules.h"

namespace GpgFrontend::UI {

namespace {

constexpr int kMinKeyGroupNameLength = 5;

// The translate() context below is spelled out at every call site on purpose.
// lupdate is a static parser: hoisting it into a constant silently drops every
// string here from the .ts files, with no warning and no compile error.

}  // namespace

auto ValidateKeyGroupMetadata(const QString& name, const QString& email)
    -> KeyGroupMetadataProblem {
  if (name.trimmed().size() < kMinKeyGroupNameLength) {
    return KeyGroupMetadataProblem::kNameTooShort;
  }

  const auto trimmed_email = email.trimmed();
  if (trimmed_email.isEmpty()) return KeyGroupMetadataProblem::kNone;

  const auto at = trimmed_email.indexOf('@');
  if (at <= 0 || at == trimmed_email.size() - 1 ||
      trimmed_email.count('@') != 1 || trimmed_email.contains(' ')) {
    return KeyGroupMetadataProblem::kEmailMalformed;
  }

  return KeyGroupMetadataProblem::kNone;
}

auto DescribeKeyGroupMetadataProblem(KeyGroupMetadataProblem problem)
    -> QString {
  switch (problem) {
    case KeyGroupMetadataProblem::kNone:
      return {};

    case KeyGroupMetadataProblem::kNameTooShort:
      return QCoreApplication::translate(
          "GpgFrontend::UI::KeyGroupMetadataRules",
          "Name must contain at least five characters.");

    case KeyGroupMetadataProblem::kEmailMalformed:
      return QCoreApplication::translate(
          "GpgFrontend::UI::KeyGroupMetadataRules",
          "Email does not look like an address. Leave it empty if the group "
          "has none.");
  }

  return {};
}

auto DescribeKeyGroupMembership(int direct, int nested, int missing)
    -> QString {
  if (direct <= 0 && nested <= 0 && missing <= 0) {
    return QCoreApplication::translate("GpgFrontend::UI::KeyGroupMetadataRules",
                                       "This group is empty.");
  }

  // Spelled out per number instead of "%n key(s)": with no translator loaded
  // Qt substitutes the count but keeps the literal "(s)", which reads as
  // "1 key(s)". Same reasoning as KeySetExpireDateDialog.
  QStringList parts;

  const auto keys = qMax(0, direct);
  parts << (keys == 1 ? QCoreApplication::translate(
                            "GpgFrontend::UI::KeyGroupMetadataRules", "1 key")
                      : QCoreApplication::translate(
                            "GpgFrontend::UI::KeyGroupMetadataRules", "%1 keys")
                            .arg(keys));

  if (nested > 0) {
    parts << (nested == 1 ? QCoreApplication::translate(
                                "GpgFrontend::UI::KeyGroupMetadataRules",
                                "1 nested group")
                          : QCoreApplication::translate(
                                "GpgFrontend::UI::KeyGroupMetadataRules",
                                "%1 nested groups")
                                .arg(nested));
  }

  const auto joiner = QCoreApplication::translate(
      "GpgFrontend::UI::KeyGroupMetadataRules", " and ");
  const auto summary = parts.join(joiner);

  auto ret = QCoreApplication::translate(
                 "GpgFrontend::UI::KeyGroupMetadataRules", "Contains %1.")
                 .arg(summary);

  if (missing > 0) {
    const auto warning = missing == 1
                             ? QCoreApplication::translate(
                                   "GpgFrontend::UI::KeyGroupMetadataRules",
                                   "1 member is no longer in your keyring.")
                             : QCoreApplication::translate(
                                   "GpgFrontend::UI::KeyGroupMetadataRules",
                                   "%1 members are no longer in your keyring.")
                                   .arg(missing);
    ret += " " + warning;
  }

  return ret;
}

auto DescribeKeyGroupCreation(int keys) -> QString {
  const auto count = qMax(0, keys);

  return count == 1 ? QCoreApplication::translate(
                          "GpgFrontend::UI::KeyGroupMetadataRules",
                          "1 checked key will be added to this group.")
                    : QCoreApplication::translate(
                          "GpgFrontend::UI::KeyGroupMetadataRules",
                          "%1 checked keys will be added to this group.")
                          .arg(count);
}

auto DescribeKeyGroupDeletion(const QString& name,
                              const QStringList& parent_names) -> QString {
  auto ret = QCoreApplication::translate(
                 "GpgFrontend::UI::KeyGroupMetadataRules",
                 "Delete the key group \"%1\"?\n\nThe group is removed. The "
                 "keys in it are not touched and stay in your keyring.")
                 .arg(name);

  if (!parent_names.isEmpty()) {
    ret +=
        "\n\n" + QCoreApplication::translate(
                     "GpgFrontend::UI::KeyGroupMetadataRules",
                     "It is also a member of: %1. Those groups will lose it.")
                     .arg(parent_names.join(", "));
  }

  return ret;
}

}  // namespace GpgFrontend::UI
