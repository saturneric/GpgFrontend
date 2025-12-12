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

#include "GenerateCardKeyDialog.h"

#include "core/function/gpg/GpgSmartCardManager.h"
#include "core/utils/CommonUtils.h"
#include "ui/UISignalStation.h"
#include "ui/function/GpgOperaHelper.h"

//
#include "ui_GenerateCardKeyDialog.h"

namespace GpgFrontend::UI {

GenerateCardKeyDialog::GenerateCardKeyDialog(int channel,
                                             const QString& serial_number,
                                             QWidget* parent)
    : GeneralDialog("GenerateCardKeyDialog", parent),
      channel_(channel),
      serial_number_(serial_number),
      ui_(QSharedPointer<Ui_GenerateCardKeyDialog>::create()) {
  ui_->setupUi(this);

  ui_->nameLabel->setText(tr("Name"));
  ui_->emailLabel->setText(tr("Email"));
  ui_->commentLabel->setText(tr("Comment"));
  ui_->expireLabel->setText(tr("Expire Date"));
  ui_->nonExpireCheckBox->setText(tr("Non Expire"));
  ui_->tipLabel->setText(
      tr("Warning: This Operation will overwrite keys which exist on card!"));
  ui_->generateButton->setText(tr("Generate"));

  ui_->dateEdit->setMinimumDateTime(QDateTime::currentDateTime().addDays(3));
  ui_->dateEdit->setMinimumDateTime(QDateTime::currentDateTime().addYears(2));

  connect(ui_->generateButton, &QPushButton::clicked, this,
          &GenerateCardKeyDialog::slot_generate_card_key);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  connect(ui_->nonExpireCheckBox, &QCheckBox::checkStateChanged, this,
          [=](Qt::CheckState state) {
            ui_->dateEdit->setDisabled(state == Qt::Checked);
          });
#else
  connect(ui_->nonExpireCheckBox, &QCheckBox::stateChanged, this,
          [=](int state) { ui_->dateEdit->setDisabled(state == Qt::Checked); });
#endif

  setWindowTitle(tr("Card Key Generation"));
  movePosition2CenterOfParent();
  setModal(true);

  this->show();
  this->raise();
  this->activateWindow();
}

void GenerateCardKeyDialog::slot_generate_card_key() {
  QString buffer;
  QTextStream error_stream(&buffer);

  if ((ui_->nameEdit->text()).size() < 5) {
    error_stream << "  " << tr("Name must contain at least five characters.")
                 << Qt::endl;
  }

  if (ui_->nameEdit->text().isEmpty() ||
      !IsEmailAddress(ui_->emailEdit->text())) {
    error_stream << "  " << tr("Please give a email address.") << Qt::endl;
  }

  auto error_string = error_stream.readAll();
  if (!error_string.isEmpty()) {
    ui_->errLabel->setAutoFillBackground(true);
    QPalette error = ui_->errLabel->palette();
    error.setColor(QPalette::Window, "#ff8080");
    ui_->errLabel->setPalette(error);
    ui_->errLabel->setText(error_string);

    this->show();
    this->raise();
    this->activateWindow();
    return;
  }

  auto f = [=](const OperaWaitingHd& hd) {
    auto [ret, err] = GpgSmartCardManager::GetInstance(channel_).GenerateKey(
        serial_number_, ui_->nameEdit->text(), ui_->emailEdit->text(),
        ui_->commentEdit->text(), ui_->dateEdit->dateTime(),
        ui_->nonExpireCheckBox->isChecked());

    hd();

    connect(UISignalStation::GetInstance(),
            &UISignalStation::SignalKeyDatabaseRefreshDone, this,
            [=, ret = ret]() {
              emit finished(ret == GPG_ERR_NO_ERROR ? 1 : -1);
              this->close();
            });

    UISignalStation::GetInstance()->SignalKeyDatabaseRefresh();
  };

  GpgOperaHelper::WaitForOpera(this, tr("Generating"), f);
}
}  // namespace GpgFrontend::UI