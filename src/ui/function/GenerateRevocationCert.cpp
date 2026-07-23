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

#include "GenerateRevocationCert.h"

#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/model/GpgKey.h"
#include "ui/dialog/RevocationOptionsDialog.h"

namespace GpgFrontend::UI {

GenerateRevocationCert::GenerateRevocationCert(QWidget* parent)
    : QWidget(parent) {}

void GenerateRevocationCert::Exec(int channel, const GpgKeyPtr& key) {
  if (key == nullptr) return;

  QStringList codes;
  codes << tr("0 -> No Reason.") << tr("1 -> This key is no more safe.")
        << tr("2 -> Key is outdated.") << tr("3 -> Key is no longer used");
  auto* revocation_options_dialog =
      new RevocationOptionsDialog(codes, qobject_cast<QWidget*>(parent()));

  connect(
      revocation_options_dialog,
      &RevocationOptionsDialog::SignalRevokeOptionAccepted, this,
      [this, channel, key](int code, const QString& text) {
        auto literal = QString("%1 (*.rev)").arg(tr("Revocation Certificates"));
        auto* parent_widget = qobject_cast<QWidget*>(parent());

#ifdef Q_OS_WINDOWS
        auto file_string =
            key->Name() + "[" + key->Email() + "](" + key->ID() + ").rev";
#else
        auto file_string =
            key->Name() + "<" + key->Email() + ">(" + key->ID() + ").rev";
#endif

        QString output_file_name;
        QFileDialog dialog(parent_widget, tr("Generate revocation certificate"),
                           file_string, literal);
        dialog.setDefaultSuffix(".rev");
        dialog.setAcceptMode(QFileDialog::AcceptSave);

        if (dialog.exec() != QFileDialog::Reject) {
          output_file_name = dialog.selectedFiles().front();
        }

        if (!output_file_name.isEmpty()) {
          KeyManagementOperation::GetInstance(channel).GenerateRevokeCert(
              key, output_file_name, code, text);
        }

        deleteLater();
      });

  revocation_options_dialog->show();
}

}  // namespace GpgFrontend::UI
