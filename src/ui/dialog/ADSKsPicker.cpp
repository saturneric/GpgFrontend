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

#include "ADSKsPicker.h"

#include "core/function/gpg/GpgKeyOpera.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/widgets/KeyTreeView.h"

namespace GpgFrontend::UI {

ADSKsPicker::ADSKsPicker(int channel, GpgKeyPtr key,
                         const GpgKeyTreeProxyModel::KeyFilter& filter,
                         QWidget* parent)
    : GeneralDialog(typeid(ADSKsPicker).name(), parent),
      tree_view_(new KeyTreeView(
          channel,
          [](GpgAbstractKey* k) {
            return k->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY;
          },
          [=](const GpgAbstractKey* k) {
            return (k->KeyType() != GpgAbstractKeyType::kGPG_SUBKEY ||
                    (k->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY &&
                     k->IsHasEncrCap())) &&
                   filter(k);
          })),
      channel_(channel),
      key_(std::move(key)) {
  auto* confirm_button = new QPushButton(tr("Confirm"));
  auto* cancel_button = new QPushButton(tr("Cancel"));

  connect(confirm_button, &QPushButton::clicked, this, [=]() {
    if (tree_view_->GetAllCheckedSubKey().isEmpty()) {
      QMessageBox::information(this, tr("No Subkeys Selected"),
                               tr("Please select at least one Subkey."));

      return;
    }

    confirm_button->setDisabled(true);
    cancel_button->setDisabled(true);

    slot_add_adsk(tree_view_->GetAllCheckedSubKey());
  });
  connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);

  tree_view_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  auto* vbox2 = new QVBoxLayout();
  vbox2->addWidget(new QLabel(tr("Select ADSK(s)") + ": "));
  vbox2->addWidget(tree_view_);

  auto* tips_label = new QLabel(
      tr("ADSK (Additional Decryption Subkey) allows others to encrypt data "
         "for you without having access to your private key. You are only "
         "allow to check subkeys with encryption capability."));
  tips_label->setWordWrap(true);
  tips_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

  vbox2->addWidget(tips_label);
  vbox2->addWidget(confirm_button);
  vbox2->addWidget(cancel_button);
  setLayout(vbox2);

  this->setWindowFlags(Qt::Window | Qt::WindowTitleHint |
                       Qt::CustomizeWindowHint);

  this->setModal(true);
  this->setWindowTitle(tr("ADSKs Picker"));

  movePosition2CenterOfParent();

  this->show();
  this->raise();
  this->activateWindow();
}

void ADSKsPicker::slot_add_adsk(const QContainer<GpgSubKey>& s_keys) {
  auto* task = new Thread::Task(
      [=, channel = channel_,
       key = key_](const DataObjectPtr& data_object) -> int {
        QStringList err_sub_key_infos;
        for (const auto& s_key : s_keys) {
          auto [err, _] =
              GpgKeyOpera::GetInstance(channel).AddADSKSync(key, s_key);
          if (CheckGpgError(err) == GPG_ERR_NO_ERROR) continue;

          err_sub_key_infos.append(tr("Key ID: %1 Reason: %2")
                                       .arg(s_key.ID())
                                       .arg(DescribeGpgErrCode(err).second));
        }
        data_object->Swap({err_sub_key_infos});
        return 0;
      },
      "add_adsk", TransferParams(),
      [=, self = QPointer<ADSKsPicker>(this)](
          int ret, const DataObjectPtr& data_object) {
        if (ret < 0) {
          QMessageBox::critical(
              self, tr("Unknown Error"),
              tr("Caught unknown error while exporting the key."));
          return;
        }

        if (!data_object->Check<QStringList>()) return;
        auto err_sub_key_infos = ExtractParams<QStringList>(data_object, 0);

        if (!err_sub_key_infos.isEmpty()) {
          QStringList failed_info;
          for (const auto& info : err_sub_key_infos) {
            failed_info.append(info);
          }
          auto details = failed_info.join("\n\n");

          auto* msg_box = new QMessageBox(self);
          msg_box->setIcon(QMessageBox::Warning);
          msg_box->setWindowTitle(err_sub_key_infos.size() == s_keys.size()
                                      ? tr("Failed")
                                      : tr("Partially Failed"));
          msg_box->setText(
              err_sub_key_infos.size() == s_keys.size()
                  ? tr("Failed to add all selected subkeys.")
                  : tr("Some subkeys failed to be added as ADSKs."));
          msg_box->setDetailedText(details);
          msg_box->exec();
        }

        emit UISignalStation::GetInstance() -> SignalKeyDatabaseRefresh();

        if (self != nullptr) {
          self->accept();
          self->close();
        }
      });

  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
      ->PostTask(task);
}
}  // namespace GpgFrontend::UI
