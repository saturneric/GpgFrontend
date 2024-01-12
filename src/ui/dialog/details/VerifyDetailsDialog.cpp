/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "VerifyDetailsDialog.h"

#include "core/GpgModel.h"

namespace GpgFrontend::UI {

VerifyDetailsDialog::VerifyDetailsDialog(QWidget* parent, GpgError error,
                                         GpgVerifyResult result)
    : QDialog(parent), m_result_(result), error_(error) {
  this->setWindowTitle(_("Signatures Details"));

  main_layout_ = new QHBoxLayout();
  this->setLayout(main_layout_);

  slot_refresh();

  this->exec();
}

void VerifyDetailsDialog::slot_refresh() {
  m_vbox_ = new QWidget();
  auto* m_vbox_layout = new QVBoxLayout(m_vbox_);
  main_layout_->addWidget(m_vbox_);

  // Button Box for close button
  button_box_ = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(button_box_, &QDialogButtonBox::rejected, this,
          &VerifyDetailsDialog::close);

  auto signatures = m_result_.GetSignature();

  if (signatures.empty()) {
    m_vbox_layout->addWidget(new QLabel(_("No valid input found")));
    m_vbox_layout->addWidget(button_box_);
    return;
  }

  // Get timestamp of signature of current text
  QDateTime timestamp = signatures[0].GetCreateTime();

  // Set the title widget depending on sign status
  if (gpg_err_code(signatures[0].GetStatus()) == GPG_ERR_BAD_SIGNATURE) {
    m_vbox_layout->addWidget(new QLabel(_("Error Validating signature")));
  } else if (input_signature_ != nullptr) {
    const auto info = QString(_("File was signed on %1"))
                          .arg(QLocale::system().toString(timestamp)) +
                      "<br/>" + _("It Contains") + ": " + "<br/><br/>";
    m_vbox_layout->addWidget(new QLabel(info));
  } else {
    const auto info =
        QString(_("Signed on %1")).arg(QLocale::system().toString(timestamp)) +
        "<br/>" + _("It Contains") + ": " + "<br/><br/>";
    m_vbox_layout->addWidget(new QLabel(info));
  }
  // Add information box for every single key
  for (const auto& signature : signatures) {
    auto* detail_box = new VerifyKeyDetailBox(signature, this);
    m_vbox_layout->addWidget(detail_box);
  }

  m_vbox_layout->addWidget(button_box_);
}

}  // namespace GpgFrontend::UI