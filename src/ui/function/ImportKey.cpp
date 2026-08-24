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

#include "ui/function/ImportKey.h"

#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/model/GpgImportInformation.h"
#include "core/utils/IOUtils.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/import_export/KeyImportDetailDialog.h"

namespace GpgFrontend::UI {

void ImportKeys(QWidget* parent, int channel, const GFBuffer& in_buffer,
                bool rev_cert) {
  auto info =
      rev_cert
          ? KeyImportExportOperation::GetInstance(channel).ImportRevCert(
                in_buffer)
          : KeyImportExportOperation::GetInstance(channel).ImportKey(in_buffer);

  // Deferred until the database has actually reloaded, and disconnected as soon
  // as it fires so a later refresh does not open the same dialog again. The
  // signal station is the context object because it outlives every caller --
  // the import can be started from a widget that is gone by the time the
  // refresh lands.
  auto* connection = new QMetaObject::Connection;
  *connection =
      QObject::connect(UISignalStation::GetInstance(),
                       &UISignalStation::SignalKeyDatabaseRefreshDone,
                       UISignalStation::GetInstance(), [=]() -> void {
                         (new KeyImportDetailDialog(channel, info, parent));
                         QObject::disconnect(*connection);
                         delete connection;
                       });

  emit UISignalStation::GetInstance() -> SignalKeyDatabaseRefresh();
}

void ImportKeyFromFile(QWidget* parent, int channel) {
  auto file_name = QFileDialog::getOpenFileName(
      parent, QCoreApplication::translate("GpgFrontend::UI", "Open Key"),
      QString(),
      QCoreApplication::translate("GpgFrontend::UI", "Keyring files") +
          " (*.asc *.gpg)");
  if (file_name.isEmpty()) return;

  QFileInfo file_info(file_name);

  if (!file_info.isFile() || !file_info.isReadable()) {
    QMessageBox::critical(
        parent, QCoreApplication::translate("GpgFrontend::UI", "Error"),
        QCoreApplication::translate(
            "GpgFrontend::UI",
            "Cannot open this file. Please make sure that this "
            "is a regular file and it's readable."));
    return;
  }

  if (file_info.size() > static_cast<qint64>(1024 * 1024)) {
    QMessageBox::critical(
        parent, QCoreApplication::translate("GpgFrontend::UI", "Error"),
        QCoreApplication::translate(
            "GpgFrontend::UI", "The target file is too large for a keyring."));
    return;
  }

  auto [succ, buffer] = ReadFileGFBuffer(file_name);
  if (!succ) {
    QMessageBox::critical(
        nullptr,
        QCoreApplication::translate("GpgFrontend::UI", "File Open Failed"),
        QCoreApplication::translate("GpgFrontend::UI",
                                    "Failed to open file: ") +
            file_name);
    return;
  }
  ImportKeys(parent, channel, buffer);
}

void ImportKeyFromClipboard(QWidget* parent, int channel) {
  QClipboard* cb = QApplication::clipboard();
  ImportKeys(parent, channel,
             GFBuffer{cb->text(QClipboard::Clipboard).toLatin1()});
}

}  // namespace GpgFrontend::UI
