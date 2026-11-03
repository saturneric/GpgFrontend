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

#include "core/profile/ProfileLoader.h"

namespace GpgFrontend::Mobile {

class InteractionBridge;

/**
 * @brief The profile loader's questions, answered by the person using the app.
 *
 * Called on the startup worker. Each question goes to the interface through
 * the InteractionBridge and blocks only the worker until it is answered; a
 * question the bridge cancels gets the safe answer, as from the headless
 * delegate this builds on.
 *
 * Forcing the profile lock is never decided here. The lock is probed again
 * first -- if its holder has gone since, nothing needs forcing -- and only a
 * person who saw who holds it can choose to break it.
 */
class MobileProfileLoaderDelegate : public ProfileLoaderDelegate {
 public:
  explicit MobileProfileLoaderDelegate(InteractionBridge* bridge);

  auto AskAppKeyPin(const AppKeyPinRequest& request)
      -> AppKeyPinAnswer override;
  auto ConfirmForceUnlock(const ProfileLockResult& held) -> bool override;
  auto ConfirmKeyReset(ProfileKeyResetReason reason) -> bool override;
  void Report(const ProfileLoadError& error) override;

  /// A phone profile is a directory, never a package: nothing to ask.
  auto AskPackagePassphrase(const QString& package, bool retry)
      -> std::optional<GFBuffer> override;
  void Note(ProfileLoadNotice notice, const QString& detail) override;

  /// The last failure reported, if any.
  [[nodiscard]] auto LastError() const -> std::optional<ProfileLoadError>;

 private:
  InteractionBridge* bridge_;
  std::optional<ProfileLoadError> last_error_;
};

}  // namespace GpgFrontend::Mobile
