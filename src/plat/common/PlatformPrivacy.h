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

#include <QPointer>
#include <QWindow>

/**
 * @file PlatformPrivacy.h
 * @brief Keeping a window's content out of screenshots and app previews.
 *
 * Android sets FLAG_SECURE on the activity's window, which blocks
 * screenshots, screen recording and the recent-apps thumbnail. Platforms with
 * no such control do nothing (common/StubPrivacy.cpp).
 */

namespace GpgFrontend {

/**
 * @brief Whether this platform can protect a window at all.
 */
auto CanSecureWindow() -> bool;

/// The window a phone application shows: the focused one, else the first.
auto PrimaryWindow() -> QWindow*;

/**
 * @brief Protect @p window from screenshots while this object lives.
 *
 * Counted: while any scope for a window exists, it stays protected, so a
 * passphrase sheet over decrypted text does not unprotect the text when it
 * closes.
 */
class SecureWindowScope {
 public:
  explicit SecureWindowScope(QWindow* window);
  ~SecureWindowScope();

  SecureWindowScope(const SecureWindowScope&) = delete;
  auto operator=(const SecureWindowScope&) -> SecureWindowScope& = delete;

 private:
  QPointer<QWindow> window_;
};

/// The platform's switch; SecureWindowScope counts calls to it.
void SetWindowSecure(QWindow* window, bool secure);

#if !defined(Q_OS_ANDROID)
/// What the last SetWindowSecure() asked of @p window, where the platform
/// cannot do it (common/StubPrivacy.cpp): what a test on a desktop checks in
/// place of the device's FLAG_SECURE.
auto WindowSecureRequested(const QWindow* window) -> bool;
#endif

}  // namespace GpgFrontend
