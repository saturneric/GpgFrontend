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

#include "TOFUInfoPage.h"

GpgFrontend::UI::TOFUInfoPage::TOFUInfoPage(
    const GpgFrontend::GpgTOFUInfo & /*tofu_info*/, QWidget *parent)
    : QWidget(parent) {
  auto grid_layout = new QGridLayout();

  grid_layout->addWidget(new QLabel(tr("Key ID") + ": "), 0, 0);
  grid_layout->addWidget(new QLabel(tr("Algorithm") + ": "), 1, 0);
  grid_layout->addWidget(new QLabel(tr("Key Size") + ": "), 2, 0);
  grid_layout->addWidget(new QLabel(tr("Nominal Usage") + ": "), 3, 0);
  grid_layout->addWidget(new QLabel(tr("Actual Usage") + ": "), 4, 0);
  grid_layout->addWidget(new QLabel(tr("Expires on") + ": "), 5, 0);
  grid_layout->addWidget(new QLabel(tr("Last Update") + ": "), 6, 0);
  grid_layout->addWidget(new QLabel(tr("Secret Key Existence") + ": "), 7, 0);

  setLayout(grid_layout);
}
