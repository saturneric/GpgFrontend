/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ReceiveMailDialog.h"

#include "ui_ReceiveMailDialog.h"

GpgFrontend::UI::ReceiveMailDialog::ReceiveMailDialog(QWidget *parent)
    : QDialog(parent), ui(std::make_shared<Ui_ReceiveMailDialog>()) {
  ui->setupUi(this);
}

void GpgFrontend::UI::ReceiveMailDialog::slotRefreshData() {}

void GpgFrontend::UI::ReceiveMailDialog::list_sub_folders(
    GpgFrontend::UI::IMAPFolder *parent_folder,
    const std::shared_ptr<vmime::net::folder> &) {}