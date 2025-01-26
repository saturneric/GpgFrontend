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

#include "WaitingDialog.h"

#include "ui/dialog/GeneralDialog.h"

namespace GpgFrontend::UI {

WaitingDialog::WaitingDialog(const QString& title, bool range, QWidget* parent)
    : GeneralDialog("WaitingDialog", parent), pb_(new QProgressBar()) {
  pb_->setRange(0, range ? 100 : 0);
  pb_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  pb_->setTextVisible(false);

  auto* layout = new QVBoxLayout();
  layout->addWidget(pb_);
  this->setLayout(layout);

  this->setModal(true);
  this->setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                       Qt::CustomizeWindowHint);
  this->setWindowTitle(title);
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setFixedSize(240, 42);

  connect(this, &WaitingDialog::SignalUpdateValue, this,
          &WaitingDialog::SlotUpdateValue);

  this->movePosition2CenterOfParent();
  this->show();
}

void WaitingDialog::SlotUpdateValue(int value) { pb_->setValue(value); }
}  // namespace GpgFrontend::UI
