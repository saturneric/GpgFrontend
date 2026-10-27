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

#include "ui/dialog/profile/ProfileCreateSummary.h"

#include "ui/function/KeyDatabaseDisplayNames.h"

namespace GpgFrontend::UI {

auto ProfileCreateStepTitle(ProfileCreateStep step) -> QString {
  switch (step) {
    case ProfileCreateStep::kName:
      return QObject::tr("Name");
    case ProfileCreateStep::kKeys:
      return QObject::tr("Keys");
    case ProfileCreateStep::kSettings:
      return QObject::tr("Settings");
  }
  return {};
}

auto IsProfileNameAcceptable(const QString& name) -> bool {
  return !name.trimmed().isEmpty();
}

auto CanAdvanceProfileCreate(ProfileCreateStep step, const QString& name)
    -> bool {
  switch (step) {
    case ProfileCreateStep::kName:
      return IsProfileNameAcceptable(name);
    case ProfileCreateStep::kKeys:
      return true;
    case ProfileCreateStep::kSettings:
      // This step's button creates the profile, so the name is asked again:
      // the wizard must not be more lenient than the dialog it replaced.
      return IsProfileNameAcceptable(name);
  }
  return false;
}

auto DescribeProfileKeyringChoice(bool self_contained, bool gnupg_available)
    -> ProfileKeyringChoiceText {
  const auto title =
      KeyDatabaseKindDisplayName(KeyDatabaseKind::kDEFAULT, self_contained);

  if (self_contained) {
    return {title, QObject::tr("Starts empty. Its keys are exported with the "
                               "profile as one file.")};
  }

  // What "this computer's" database is depends on what this computer has, and
  // promising a GnuPG keyring on a build without GnuPG would be wrong.
  return {title,
          gnupg_available
              ? QObject::tr("Your existing keys in the GnuPG home directory, "
                            "shared with other programs. Not carried in a "
                            "profile file.")
              : QObject::tr("Shared with the other profiles on this computer "
                            "that use it. Not carried in a profile file.")};
}

}  // namespace GpgFrontend::UI
