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

#include "ui/function/MoveKeyToCard.h"

#include <optional>

#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/function/gpg/GpgSmartCardManager.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgSubKey.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "ui/UISignalStation.h"
#include "ui/function/GpgErrorMessageBox.h"

namespace GpgFrontend::UI {

namespace {

auto CardSlotDisplayName(int slot) -> QString {
  switch (slot) {
    case 1:
      return QCoreApplication::translate("GpgFrontend::UI",
                                         "Signature (OPENPGP.1)");
    case 2:
      return QCoreApplication::translate("GpgFrontend::UI",
                                         "Encryption (OPENPGP.2)");
    case 3:
      return QCoreApplication::translate("GpgFrontend::UI",
                                         "Authentication (OPENPGP.3)");
    default:
      return {};
  }
}

/// Ask the user, when a (sub)key fits more than one slot, which one to use.
/// Returns the chosen slot or std::nullopt if cancelled.
auto ResolveCardSlot(QWidget* parent, const QList<int>& candidates)
    -> std::optional<int> {
  if (candidates.size() == 1) return candidates.front();

  QStringList options;
  for (int slot : candidates) options << CardSlotDisplayName(slot);

  bool ok = false;
  const auto choice = QInputDialog::getItem(
      parent,
      QCoreApplication::translate("GpgFrontend::UI", "Select Card Slot"),
      QCoreApplication::translate(
          "GpgFrontend::UI",
          "This key can be stored in more than one slot. "
          "Where should it be stored?"),
      options, 0, false, &ok);
  if (!ok) return std::nullopt;

  const auto index = options.indexOf(choice);
  if (index < 0) return std::nullopt;
  return candidates.at(index);
}

/// Resolve the target card serial: use @p preselected when set, otherwise pick
/// from the inserted cards (auto when one, ask when several). Empty on abort.
auto ResolveCardSerial(QWidget* parent, int channel, const QString& preselected)
    -> QString {
  if (!preselected.isEmpty()) return preselected;

  auto serials = GpgSmartCardManager::GetInstance(channel).GetSerialNumbers();
  if (serials.isEmpty()) {
    QMessageBox::information(
        parent, QCoreApplication::translate("GpgFrontend::UI", "No Smart Card"),
        QCoreApplication::translate("GpgFrontend::UI",
                                    "No OpenPGP smart card was detected. "
                                    "Insert a card and try again."));
    return {};
  }
  if (serials.size() == 1) return serials.front();

  bool ok = false;
  auto serial = QInputDialog::getItem(
      parent,
      QCoreApplication::translate("GpgFrontend::UI", "Select Smart Card"),
      QCoreApplication::translate("GpgFrontend::UI",
                                  "Move the key to which card?"),
      serials, 0, false, &ok);
  if (!ok) return {};
  return serial;
}

/// Offer to export a secret-key backup before the destructive move. Returns
/// false only when the user cancels the whole operation (a failed/aborted
/// backup counts as a cancel); true means "proceed with the move".
auto OfferSecretKeyBackup(QWidget* parent, int channel, const GpgKeyPtr& key)
    -> bool {
  QMessageBox box(parent);
  box.setIcon(QMessageBox::Question);
  box.setWindowTitle(
      QCoreApplication::translate("GpgFrontend::UI", "Back Up Secret Key"));
  box.setText(QCoreApplication::translate(
      "GpgFrontend::UI",
      "Do you want to export a backup of the secret key before moving it to "
      "the card? After the move the key can only be used through the card."));
  auto* backup_btn = box.addButton(
      QCoreApplication::translate("GpgFrontend::UI", "Back Up First"),
      QMessageBox::AcceptRole);
  box.addButton(
      QCoreApplication::translate("GpgFrontend::UI", "Continue Without Backup"),
      QMessageBox::DestructiveRole);
  auto* cancel_btn = box.addButton(QMessageBox::Cancel);
  box.setDefaultButton(backup_btn);
  box.exec();

  if (box.clickedButton() == cancel_btn) return false;
  if (box.clickedButton() != backup_btn)
    return true;  // continue without backup

  auto [err, buffer] = KeyImportExportOperation::GetInstance(channel).ExportKey(
      key, true, true, false);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    RaiseMessageBox(parent, err);
    return false;
  }

  auto file_string = key->Name() + "_" + key->Email() + "_secret_backup.asc";
  file_string.replace(' ', '_');

  const auto file_name = QFileDialog::getSaveFileName(
      parent,
      QCoreApplication::translate("GpgFrontend::UI",
                                  "Export Secret Key Backup"),
      file_string,
      QCoreApplication::translate("GpgFrontend::UI", "Key Files") +
          " (*.asc *.txt);;All Files (*)");
  if (file_name.isEmpty()) return false;  // backup declined -> abort the move

  if (!WriteFileGFBuffer(file_name, buffer)) {
    QMessageBox::critical(
        parent, QCoreApplication::translate("GpgFrontend::UI", "Export Error"),
        QCoreApplication::translate("GpgFrontend::UI",
                                    "Couldn't open %1 for writing")
            .arg(file_name));
    return false;
  }
  return true;
}

}  // namespace

auto MoveKeyToCardInteractive(QWidget* parent, int channel,
                              const GpgKeyPtr& key, int subkey_index,
                              const QString& preselected_serial) -> bool {
  if (key == nullptr) return false;

  const auto subkeys = key->SubKeys();
  if (subkey_index < 0 || subkey_index >= static_cast<int>(subkeys.size())) {
    return false;
  }
  const auto& skey = subkeys[subkey_index];

  const auto title =
      QCoreApplication::translate("GpgFrontend::UI", "Move Key to Smart Card");

  // 1. warn about the destructive move
  const auto warn_body =
      QCoreApplication::translate(
          "GpgFrontend::UI",
          "<h3>You are about to move a private key onto a smart card.</h3>"
          "<b>KeyID:</b> %1<br/><br/>"
          "This <b>moves</b> the key: its private part is removed from this "
          "computer and only a card reference (stub) remains. Afterwards the "
          "key can only be used through the card. This action is "
          "<b>irreversible</b>.<br/><br/>Do you want to continue?")
          .arg(skey.ID());
  if (QMessageBox::warning(parent, title, warn_body,
                           QMessageBox::Cancel | QMessageBox::Yes,
                           QMessageBox::Cancel) != QMessageBox::Yes) {
    return false;
  }

  // 2. offer a secret-key backup first
  if (!OfferSecretKeyBackup(parent, channel, key)) return false;

  // 3. resolve the target slot from the key's capabilities
  const auto candidates = GpgSmartCardManager::CandidateSlots(skey);
  if (candidates.isEmpty()) {
    QMessageBox::critical(
        parent, title,
        QCoreApplication::translate(
            "GpgFrontend::UI",
            "This key has no capability that can be stored on a smart card."));
    return false;
  }
  const auto slot = ResolveCardSlot(parent, candidates);
  if (!slot) return false;

  // 4. resolve the target card
  const auto serial = ResolveCardSerial(parent, channel, preselected_serial);
  if (serial.isEmpty()) return false;

  // 5. perform the move
  auto [err, status] = GpgSmartCardManager::GetInstance(channel).MoveKeyToCard(
      key, subkey_index, serial, *slot);
  if (err != GPG_ERR_NO_ERROR) {
    RaiseFailureMessageBox(parent, err, status);
    return false;
  }

  // 6. reconcile the on-disk stub, then reload the key database so the UI shows
  // the key as card-resident (mirrors the fetch flow in the card controller).
  GpgCommandExecutor::GetInstance(channel).GpgExecuteSync(
      {{}, {"--card-status"}, [](int, const QString&, const QString&) {}});
  emit UISignalStation::GetInstance() -> SignalKeyDatabaseRefresh();

  QMessageBox::information(
      parent, title,
      QCoreApplication::translate(
          "GpgFrontend::UI",
          "The key was moved to the smart card successfully."));
  return true;
}

}  // namespace GpgFrontend::UI
