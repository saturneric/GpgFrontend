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

#include "ui/function/ShowKeyDetails.h"

#include "core/model/GpgKey.h"
#include "ui/dialog/KeyGroupManageDialog.h"
#include "ui/dialog/keypair_details/KeyDetailsDialog.h"

namespace GpgFrontend::UI {

void ShowKeyDetails(QWidget* parent, int channel,
                    const GpgAbstractKeyPtr& key) {
  if (key == nullptr) {
    QMessageBox::critical(
        parent, QCoreApplication::translate("GpgFrontend::UI", "Error"),
        QCoreApplication::translate("GpgFrontend::UI", "Key Not Found."));
    return;
  }

  switch (key->KeyType()) {
    case GpgAbstractKeyType::kGPG_KEY:
      new KeyDetailsDialog(channel, qSharedPointerDynamicCast<GpgKey>(key),
                           parent);
      break;
    case GpgAbstractKeyType::kGPG_KEYGROUP:
      new KeyGroupManageDialog(
          channel, qSharedPointerDynamicCast<GpgKeyGroup>(key), parent);
      break;
    case GpgAbstractKeyType::kNONE:
    case GpgAbstractKeyType::kGPG_SUBKEY:
      break;
  }
}

}  // namespace GpgFrontend::UI
