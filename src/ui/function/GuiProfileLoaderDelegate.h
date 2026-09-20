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

#pragma once

#include <optional>

#include "core/profile/ProfileLoader.h"
#include "ui/widgets/MetaListPanel.h"

namespace GpgFrontend::UI {

/**
 * @brief What to show when another process already holds the profile.
 *
 * Split out of the dialog so the wording and the shape of the list can be
 * asserted in a test -- gtest bodies run off the GUI thread, where
 * constructing a widget is a crash, so this stays plain data, rendered by
 * MetaListDialog rather than describing its own QMessageBox paragraphs.
 */
struct GF_UI_EXPORT ProfileLockConflictTexts {
  QString title;                 ///< header title, and the window title
  QString subtitle;              ///< one wrapped line saying what is wrong
  QContainer<MetaListRow> rows;  ///< the profile, and who holds it
  QString note;                  ///< what forcing the lock past this costs
};

/**
 * @brief Build the one prompt shown for a profile held by another process.
 *
 * @param held the refused lock, with whatever it knows about its holder
 * @return the strings for that prompt
 */
auto GF_UI_EXPORT BuildProfileLockConflictTexts(const ProfileLockResult &held)
    -> ProfileLockConflictTexts;

/**
 * @brief The profile loader's questions, asked with dialogs.
 *
 * Every decision this makes is the loader's; everything here is presentation.
 * The split is what lets the whole startup sequence be exercised in a test
 * against a scripted delegate, which is otherwise only reachable by launching
 * the application and typing.
 */
class GF_UI_EXPORT GuiProfileLoaderDelegate final
    : public ProfileLoaderDelegate {
 public:
  auto AskPackagePassphrase(const QString &package, bool retry)
      -> std::optional<GFBuffer> override;

  auto AskAppKeyPin(const AppKeyPinRequest &request)
      -> AppKeyPinAnswer override;

  auto ConfirmForceUnlock(const ProfileLockResult &held) -> bool override;

  auto ConfirmKeyReset(ProfileKeyResetReason reason) -> bool override;

  void Report(const ProfileLoadError &error) override;

  void Note(ProfileLoadNotice notice, const QString &detail) override;
};

}  // namespace GpgFrontend::UI
