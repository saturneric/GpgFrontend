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

#include "SetOwnerTrustLevel.h"

#include "core/GpgModel.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyManager.h"
#include "ui/UISignalStation.h"

namespace GpgFrontend::UI {

SetOwnerTrustLevel::SetOwnerTrustLevel(QWidget* parent) : QWidget(parent) {}

auto SetOwnerTrustLevel::Exec(int channel, const QString& key_id) -> bool {
  if (key_id.isEmpty()) {
    return false;
  }

  auto key = GpgKeyGetter::GetInstance(channel).GetKey(key_id);
  assert(key.IsGood());

  QStringList items;

  items << tr("Unknown") << tr("Undefined") << tr("Never") << tr("Marginal")
        << tr("Full") << tr("Ultimate");
  bool ok;
  QString item = QInputDialog::getItem(this, tr("Modify Owner Trust Level"),
                                       tr("Trust for the Key Pair:"), items,
                                       key.GetOwnerTrustLevel(), false, &ok);

  if (ok && !item.isEmpty()) {
    int trust_level = 0;  // Unknown Level
    if (item == tr("Ultimate")) {
      trust_level = 5;
    } else if (item == tr("Full")) {
      trust_level = 4;
    } else if (item == tr("Marginal")) {
      trust_level = 3;
    } else if (item == tr("Never")) {
      trust_level = 2;
    } else if (item == tr("Undefined")) {
      trust_level = 1;
    }

    if (trust_level == 0) {
      QMessageBox::warning(
          this, tr("Warning"),
          QString(
              tr("Owner Trust Level cannot set to Unknown level, automately "
                 "changing it into Undefined level.")));
      trust_level = 1;
    }

    bool status =
        GpgKeyManager::GetInstance().SetOwnerTrustLevel(key, trust_level);
    if (!status) {
      QMessageBox::critical(this, tr("Failed"),
                            tr("Modify Owner Trust Level failed."));
      return false;
    }

    // update key database and refresh ui
    emit UISignalStation::GetInstance() -> SignalKeyDatabaseRefresh();
    return true;
  }

  return false;
}

}  // namespace GpgFrontend::UI