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

#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/model/GpgImportInformation.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/IOUtils.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/import_export/KeyImportDetailDialog.h"
#include "ui/function/GpgOperaHelper.h"

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
  // The parent is held weakly for the same reason: a raw pointer here was a
  // dialog parented to a widget that might already be gone.
  const QPointer<QWidget> weak_parent = parent;
  auto* connection = new QMetaObject::Connection;
  *connection = QObject::connect(
      UISignalStation::GetInstance(),
      &UISignalStation::SignalKeyDatabaseRefreshDone,
      UISignalStation::GetInstance(), [=]() -> void {
        (new KeyImportDetailDialog(channel, info, weak_parent.data()));
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

  if (file_info.size() > kMaxKeyFileBytes) {
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
             GFBuffer{cb->text(QClipboard::Clipboard).toUtf8()});
}

namespace {

/// Ask before importing what was dropped, when the user wants to be asked.
auto ConfirmDroppedKeyImport(QWidget* parent) -> bool {
  if (!SettingItems::kConfirmImportKeys.Read(GetSettings())) return true;

  QDialog dialog(parent);
  dialog.setWindowTitle(
      QCoreApplication::translate("GpgFrontend::UI", "Import Keys"));
  auto* label = new QLabel(
      QCoreApplication::translate("GpgFrontend::UI",
                                  "You've dropped something on the key list.\n"
                                  "GpgFrontend will now try to import key(s)."),
      &dialog);
  auto* check_box = new QCheckBox(
      QCoreApplication::translate("GpgFrontend::UI",
                                  "Ask before importing keys next time."),
      &dialog);
  check_box->setChecked(true);
  auto* button_box = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  QObject::connect(button_box, &QDialogButtonBox::accepted, &dialog,
                   &QDialog::accept);
  QObject::connect(button_box, &QDialogButtonBox::rejected, &dialog,
                   &QDialog::reject);
  auto* vbox = new QVBoxLayout(&dialog);
  vbox->addWidget(label);
  vbox->addWidget(check_box);
  vbox->addWidget(button_box);

  if (dialog.exec() != QDialog::Accepted) return false;
  auto settings = GetSettings();
  SettingItems::kConfirmImportKeys.Write(settings, check_box->isChecked());
  return true;
}

}  // namespace

auto ImportDroppedKeys(QWidget* parent, int channel, const QMimeData* mime)
    -> bool {
  if (mime == nullptr || (!mime->hasUrls() && !mime->hasText())) return false;
  if (!ConfirmDroppedKeyImport(parent)) return false;

  if (!mime->hasUrls()) {
    ImportKeys(parent, channel, GFBuffer(mime->text().toUtf8()));
    return true;
  }

  QStringList paths;
  for (const auto& url : mime->urls()) {
    if (url.isLocalFile()) paths.append(url.toLocalFile());
  }

  // Read off the GUI thread, each file under the keyring size cap that
  // picking a file already had; dropped files used to be read whole, however
  // large, with the window frozen meanwhile.
  struct Read {
    QByteArray data;
    QStringList skipped;
  };
  auto read = std::make_shared<Read>();
  GpgOperaHelper::WaitForOpera(
      parent, QCoreApplication::translate("GpgFrontend::UI", "Reading Keys"),
      [read, paths](const OperaWaitingHd& hd) {
        RunOperaAsync(
            [read, paths](const DataObjectPtr&) -> GFOperaStatus {
              for (const auto& path : paths) {
                const QFileInfo info(path);
                QFile file(path);
                if (!info.isFile() || info.size() > kMaxKeyFileBytes ||
                    !file.open(QIODevice::ReadOnly)) {
                  read->skipped.append(info.fileName());
                  continue;
                }
                read->data += file.readAll();
                read->data += '\n';
              }
              return 0;
            },
            [hd](GFOperaStatus, const DataObjectPtr&) { hd(); },
            "read_dropped_keys");
      });

  if (!read->skipped.isEmpty()) {
    QMessageBox::warning(
        parent, QCoreApplication::translate("GpgFrontend::UI", "Import Keys"),
        QCoreApplication::translate(
            "GpgFrontend::UI",
            "These files were not imported: they are not readable files, or "
            "too large for a keyring.\n\n%1")
            .arg(read->skipped.join(QChar::LineFeed)));
  }
  // One import for everything dropped: one refresh and one report, not one
  // per file.
  if (!read->data.isEmpty()) {
    ImportKeys(parent, channel, GFBuffer(read->data));
    WipeByteArray(read->data);
  }
  return true;
}

}  // namespace GpgFrontend::UI
