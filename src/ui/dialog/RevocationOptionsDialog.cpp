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

#include "RevocationOptionsDialog.h"

#include "core/utils/MemoryUtils.h"
#include "ui_RevocationOptionsDialog.h"

GpgFrontend::UI::RevocationOptionsDialog::RevocationOptionsDialog(
    QWidget *parent)
    : GeneralDialog("RevocationOptionsDialog", parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_RevocationOptionsDialog>()) {
  ui_->setupUi(this);

  QStringList codes;

  codes << tr("0 -> No Reason.") << tr("1 -> This key is no more safe.")
        << tr("2 -> Key is outdated.") << tr("3 -> Key is no longer used");

  ui_->rRcodeComboBox->addItems(codes);

  ui_->revocationReasonCodeLabel->setText(tr("Revocation Reason (Code)"));
  ui_->revocationReasonTextLabel->setText(tr("Revocation Reason (Text)"));
  this->setWindowTitle(tr("Revocation Options"));

  connect(ui_->buttonBox, &QDialogButtonBox::accepted, this,
          &RevocationOptionsDialog::slot_button_box_accepted);
  connect(ui_->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(ui_->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  setAttribute(Qt::WA_DeleteOnClose);
  setModal(true);
}

auto GpgFrontend::UI::RevocationOptionsDialog::Code() const -> int {
  return code_;
}

auto GpgFrontend::UI::RevocationOptionsDialog::Text() const -> QString {
  return text_;
}

void GpgFrontend::UI::RevocationOptionsDialog::slot_button_box_accepted() {
  code_ = ui_->rRcodeComboBox->currentIndex();
  text_ = ui_->rRPlainTextEdit->toPlainText()
              .trimmed()
              .split('\n', Qt::SkipEmptyParts)
              .join("\n");
  emit SignalRevokeOptionAccepted(code_, text_);
}
