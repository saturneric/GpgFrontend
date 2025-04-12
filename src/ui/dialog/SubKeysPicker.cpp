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

#include "SubKeysPicker.h"

#include "core/GpgModel.h"
#include "ui/widgets/KeyTreeView.h"

namespace GpgFrontend::UI {

SubKeysPicker::SubKeysPicker(int channel,
                             const GpgKeyTreeModel::Detector& enable_detector,
                             QWidget* parent)
    : GeneralDialog(typeid(SubKeysPicker).name(), parent),
      tree_view_(new KeyTreeView(
          channel, [](GpgAbstractKey* k) { return k->IsSubKey(); },
          [=](GpgAbstractKey* k) {
            return (!k->IsSubKey() || (k->IsSubKey() && !k->IsPrimaryKey() &&
                                       k->IsHasEncrCap())) &&
                   enable_detector(k);
          })) {
  auto* confirm_button = new QPushButton(tr("Confirm"));
  auto* cancel_button = new QPushButton(tr("Cancel"));

  connect(confirm_button, &QPushButton::clicked,
          [=]() { this->accepted_ = true; });
  connect(confirm_button, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);

  auto* vbox2 = new QVBoxLayout();
  vbox2->addWidget(new QLabel(tr("Select Subkey(s)") + ": "));
  vbox2->addWidget(tree_view_);
  vbox2->addWidget(new QLabel(
      tr("Please select one or more subkeys you use for operation.")));
  vbox2->addWidget(confirm_button);
  vbox2->addWidget(cancel_button);
  setLayout(vbox2);

  tree_view_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  this->setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                       Qt::CustomizeWindowHint);

  this->setModal(true);
  this->setWindowTitle(tr("Subkeys Picker"));

  movePosition2CenterOfParent();
  this->show();
}

auto SubKeysPicker::GetCheckedSubkeys() -> QContainer<GpgSubKey> {
  return tree_view_->GetAllCheckedSubKey();
}

auto SubKeysPicker::GetStatus() const -> bool { return this->accepted_; }

}  // namespace GpgFrontend::UI
