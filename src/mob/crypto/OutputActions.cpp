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

#include "crypto/OutputActions.h"

#include <QPointer>

#include "plat/common/PlatformClipboard.h"
#include "plat/common/PlatformShare.h"

namespace GpgFrontend::Mobile {

namespace {

auto OutcomeName(ShareOutcome outcome) -> QString {
  switch (outcome) {
    case ShareOutcome::kHandedOff:
      return QStringLiteral("handed_off");
    case ShareOutcome::kDismissed:
      return QStringLiteral("dismissed");
    case ShareOutcome::kFailed:
      return QStringLiteral("failed");
    case ShareOutcome::kUnsupported:
      return QStringLiteral("unsupported");
  }
  return QStringLiteral("failed");
}

}  // namespace

OutputActions::OutputActions(QObject* parent) : QObject(parent) {}

auto OutputActions::CanShare() const -> bool { return GpgFrontend::CanShare(); }

void OutputActions::copy(const QString& text, bool sensitive) {
  if (sensitive) {
    CopySensitiveText(text, kSensitiveCopyExpiry);
  } else {
    CopyText(text);
  }
}

void OutputActions::share(const QString& text, const QString& title) {
  QPointer<OutputActions> self(this);
  ShareText(text, title, [self](const ShareResult& result) {
    if (self == nullptr) return;
    emit self->SignalShared(OutcomeName(result.outcome));
  });
}

}  // namespace GpgFrontend::Mobile
