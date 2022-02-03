/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "ReceiveMailDialog.h"

#include "ui_ReceiveMailDialog.h"

GpgFrontend::UI::ReceiveMailDialog::ReceiveMailDialog(QWidget *parent)
    : QDialog(parent), ui_(std::make_shared<Ui_ReceiveMailDialog>()) {
  ui_->setupUi(this);
}

void GpgFrontend::UI::ReceiveMailDialog::slot_refresh_data() {}

void GpgFrontend::UI::ReceiveMailDialog::list_sub_folders(
    GpgFrontend::UI::IMAPFolder *parent_folder,
    const std::shared_ptr<vmime::net::folder> &) {}