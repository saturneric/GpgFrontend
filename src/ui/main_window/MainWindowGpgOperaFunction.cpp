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

#include "MainWindow.h"
#include "core/function/gpg/GpgBasicOperator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/model/DataObject.h"
#include "core/model/GpgEncryptResult.h"
#include "core/module/ModuleManager.h"
#include "core/utils/GpgUtils.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/SignersPicker.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/struct/GpgOperaResultContext.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

void MainWindow::SlotEncrypt() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
  contexts->ascii = true;

  auto key_ids = m_key_list_->GetChecked();

  // Symmetric Encrypt
  if (key_ids.isEmpty()) {
    auto ret = QMessageBox::information(
        this, tr("Symmetric Encryption"),
        tr("No Key Selected. Do you want to encrypt with a "
           "symmetric cipher using a passphrase?"),
        QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Cancel) return;

    contexts->keys = {};
  } else {
    contexts->keys = check_keys_helper(
        key_ids,
        [](const GpgKey& key) { return key.IsHasActualEncryptionCapability(); },
        tr("The selected keypair cannot be used for encryption."));
    if (contexts->keys.empty()) return;
  }

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasEncrypt);

  exec_operas_helper(tr("Encrypting"), contexts);
}

void MainWindow::SlotSign() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
  contexts->ascii = true;

  auto key_ids = m_key_list_->GetChecked();
  contexts->keys = check_keys_helper(
      key_ids,
      [](const GpgKey& key) { return key.IsHasActualSigningCapability(); },
      tr("The selected key contains a key that does not actually have a "
         "sign usage."));
  if (contexts->keys.empty()) return;

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasSign);

  exec_operas_helper(tr("Signing"), contexts);
}

void MainWindow::SlotDecrypt() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
  contexts->ascii = true;

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasDecrypt);

  exec_operas_helper(tr("Decrypting"), contexts);
}

void MainWindow::SlotVerify() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
  contexts->ascii = true;

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasVerify);

  exec_operas_helper(tr("Verifying"), contexts);

  if (!contexts->unknown_fprs.isEmpty()) {
    slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
  }
}

void MainWindow::SlotEncryptSign() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
  contexts->ascii = true;

  auto key_ids = m_key_list_->GetChecked();

  // Symmetric Encrypt
  if (key_ids.isEmpty()) {
    auto ret = QMessageBox::information(
        this, tr("Symmetric Encryption"),
        tr("No Key Selected. Do you want to encrypt with a "
           "symmetric cipher using a passphrase?"),
        QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Cancel) return;

    contexts->keys = {};
  } else {
    contexts->keys = check_keys_helper(
        key_ids,
        [](const GpgKey& key) { return key.IsHasActualEncryptionCapability(); },
        tr("The selected keypair cannot be used for encryption."));
    if (contexts->keys.empty()) return;
  }

  auto* signers_picker =
      new SignersPicker(m_key_list_->GetCurrentGpgContextChannel(), this);
  QEventLoop loop;
  connect(signers_picker, &SignersPicker::finished, &loop, &QEventLoop::quit);
  loop.exec();

  // return when canceled
  if (!signers_picker->GetStatus()) return;

  auto signer_key_ids = signers_picker->GetCheckedSigners();
  auto signer_keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(signer_key_ids);
  assert(std::all_of(signer_keys.begin(), signer_keys.end(),
                     [](const auto& key) { return key.IsGood(); }));

  contexts->singer_keys = signer_keys;

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasEncryptSign);

  exec_operas_helper(tr("Encrypting and Signing"), contexts);
}

void MainWindow::SlotDecryptVerify() {
  if (edit_->CurPageTextEdit() == nullptr) return;

  auto contexts = QSharedPointer<GpgOperaContextBasement>::create();
  contexts->ascii = true;

  contexts->GetContextBuffer(0).append(GFBuffer(edit_->CurPlainText()));
  GpgOperaHelper::BuildOperas(contexts, 0,
                              m_key_list_->GetCurrentGpgContextChannel(),
                              GpgOperaHelper::BuildOperasDecryptVerify);

  exec_operas_helper(tr("Decrypting and Verifying"), contexts);

  if (!contexts->unknown_fprs.isEmpty()) {
    slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
  }
}

}  // namespace GpgFrontend::UI