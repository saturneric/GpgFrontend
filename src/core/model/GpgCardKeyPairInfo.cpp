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

#include "GpgCardKeyPairInfo.h"

namespace GpgFrontend {

GpgCardKeyPairInfo::GpgCardKeyPairInfo(const QString &status) {
  const auto values = status.split(QLatin1Char(' '));
  if (values.size() < 2) {
    return;
  }

  grip = values[0];
  key_ref = values[1];
  if (values.size() >= 3) {
    usage = values[2];
  }

  if (values.size() >= 4 && !values[3].isEmpty() && values[3] != "-") {
    bool ok;
    const qint64 seconds_since_epoch = values[3].toLongLong(&ok);
    if (ok) {
      time =
          QDateTime::fromSecsSinceEpoch(seconds_since_epoch, QTimeZone::utc());
    }
  }

  if (values.size() >= 5) {
    algorithm = values[4];
  }
}

auto GpgCardKeyPairInfo::CanAuthenticate() const -> bool {
  return usage.contains('a');
}

auto GpgCardKeyPairInfo::CanCertify() const -> bool {
  return usage.contains('c');
}

auto GpgCardKeyPairInfo::CanEncrypt() const -> bool {
  return usage.contains('e');
}

auto GpgCardKeyPairInfo::CanSign() const -> bool { return usage.contains('s'); }

}  // namespace GpgFrontend
