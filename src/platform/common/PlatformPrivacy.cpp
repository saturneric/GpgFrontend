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

#include "platform/common/PlatformPrivacy.h"

#include <QGuiApplication>
#include <QHash>
#include <QPointer>
#include <QSet>
#include <QWindow>

namespace GpgFrontend {

namespace {

// On Android the flag is the activity's, not a window's: two scopes that
// resolved to different QWindows still share it, and it may only be lifted
// when the last scope of all ends.
#ifdef Q_OS_ANDROID
constexpr bool kOneFlagPerProcess = true;
#else
constexpr bool kOneFlagPerProcess = false;
#endif

// Interface thread only, like the windows themselves.
auto Counts() -> QHash<QWindow*, int>& {
  static QHash<QWindow*, int> counts;
  return counts;
}

/// Forget @p window when it goes: a new window at the same address must not
/// inherit its count and stay unprotected.
void TrackDestruction(QWindow* window) {
  // Once per window. Not with Qt::UniqueConnection: that needs a member
  // function, and a debug Qt aborts on it with a lambda.
  static QSet<QWindow*> tracked;
  if (tracked.contains(window)) return;
  tracked.insert(window);
  QObject::connect(window, &QObject::destroyed, window, [window]() {
    tracked.remove(window);
    auto& counts = Counts();
    counts.remove(window);
    if (kOneFlagPerProcess && counts.isEmpty()) SetWindowSecure(nullptr, false);
  });
}

}  // namespace

auto PrimaryWindow() -> QWindow* {
  if (auto* focused = QGuiApplication::focusWindow(); focused != nullptr) {
    return focused;
  }
  const auto windows = QGuiApplication::topLevelWindows();
  return windows.isEmpty() ? nullptr : windows.constFirst();
}

SecureWindowScope::SecureWindowScope(QWindow* window) : window_(window) {
  if (window_ == nullptr) return;
  auto& counts = Counts();
  const bool first_anywhere = counts.isEmpty();
  if (counts[window_]++ == 0) {
    TrackDestruction(window_);
    if (!kOneFlagPerProcess || first_anywhere) SetWindowSecure(window_, true);
  }
}

SecureWindowScope::~SecureWindowScope() {
  // A window already gone needs no unprotecting; its count went with it.
  if (window_ == nullptr) return;
  auto& counts = Counts();
  if (!counts.contains(window_)) return;
  if (--counts[window_] == 0) {
    counts.remove(window_);
    if (!kOneFlagPerProcess || counts.isEmpty()) {
      SetWindowSecure(window_, false);
    }
  }
}

}  // namespace GpgFrontend
