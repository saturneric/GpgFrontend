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

#include "ui/function/ExportKey.h"

#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "ui/UserInterfaceUtils.h"

namespace GpgFrontend::UI {

namespace {

/**
 * @brief Strip what no filesystem will accept in a name.
 *
 * A user ID may legitimately contain a slash or a colon, and substituting one
 * straight into the template produced a default filename the save dialog would
 * reject — on every platform, not just Windows.
 */
auto SanitiseFileNameField(const QString& field) -> QString {
  QString out;
  out.reserve(field.size());

  for (const auto c : field) {
    if (c == '/' || c == '\\' || c == ':' || c == '"' || c == '|' || c == '?' ||
        c == '*' || c == '<' || c == '>' ||
        c.category() == QChar::Other_Control) {
      continue;
    }
    out.append(c == ' ' ? QChar('_') : c);
  }
  return out.trimmed();
}

}  // namespace

auto ExportKeyFileName(const QString& name, const QString& email,
                       const QString& id, const QString& type) -> QString {
  // The angle brackets are part of the conventional "Name <email>" shape, so
  // they go in from the template rather than surviving inside a field; Windows
  // will not take them at all, hence the square brackets there.
#ifdef Q_OS_WINDOWS
  auto file_name = QString("%1[%2](%3)_%4.asc");
#else
  auto file_name = QString("%1<%2>(%3)_%4.asc");
#endif

  return file_name.arg(SanitiseFileNameField(name),
                       SanitiseFileNameField(email), SanitiseFileNameField(id),
                       SanitiseFileNameField(type));
}

ExportKey::ExportKey(QWidget* parent) : QWidget(parent) {}

void ExportKey::exec_export(int channel, const GpgKeyPtr& key, bool secret,
                            bool ascii, bool shortest, const QString& type) {
  if (key == nullptr) {
    deleteLater();
    return;
  }

  // Dialogs are parented to the widget that asked for the export, not to this
  // object: this one deletes itself as soon as the callback is done, and a
  // dialog outliving its parent would take the parent's window with it.
  auto* dialog_parent = parentWidget();
  QPointer<ExportKey> self(this);

  auto* task = new Thread::Task(
      [=](const DataObjectPtr& data_object) -> int {
        auto [err, gf_buffer] =
            KeyImportExportOperation::GetInstance(channel).ExportKey(
                key, secret, ascii, shortest);
        data_object->Swap({err, gf_buffer});
        return 0;
      },
      "key_export", TransferParams(),
      [=](int ret, const DataObjectPtr& data_object) {
        // Everything below runs long after exec_export() returned; if the
        // window went away in between there is nobody left to show a dialog to.
        if (self.isNull()) return;

        const auto cleanup = qScopeGuard([self]() {
          if (!self.isNull()) self->deleteLater();
        });

        if (ret < 0) {
          QMessageBox::critical(
              dialog_parent, tr("Unknown Error"),
              tr("Caught unknown error while exporting the key."));
          return;
        }

        if (!data_object->Check<GpgError, GFBuffer>()) return;

        auto err = ExtractParams<GpgError>(data_object, 0);
        auto gf_buffer = ExtractParams<GFBuffer>(data_object, 1);

        if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
          CommonUtils::RaiseMessageBox(dialog_parent, err);
          return;
        }

        const auto file_name =
            ExportKeyFileName(key->Name(), key->Email(), key->ID(), type);

        auto filepath = QFileDialog::getSaveFileName(
            dialog_parent, tr("Export Key To File"), file_name,
            tr("Key Files") + " (*.asc *.txt);;All Files (*)");

        if (filepath.isEmpty()) return;

        if (!WriteFileGFBuffer(filepath, gf_buffer)) {
          QMessageBox::critical(
              dialog_parent, tr("Export Error"),
              tr("Couldn't open %1 for writing").arg(filepath));
          return;
        }

        QMessageBox::information(
            dialog_parent, tr("Export Successful"),
            tr("The key has been successfully exported to %1.").arg(filepath));
      });

  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
      ->PostTask(task);
}

void ExportKey::ExecPublic(int channel, const GpgKeyPtr& key) {
  exec_export(channel, key, false, true, false, "pub");
}

void ExportKey::ExecShortPrivate(int channel, const GpgKeyPtr& key) {
  QString warning_message =
      "<h3><b>" + tr("WARNING: You are about to export your") + " " +
      "<font color=\"red\">" + tr("PRIVATE KEY") + "</font>!</b></h3>" + "<p>" +
      tr("This is NOT your Public Key, so <b>DO NOT</b> share it with "
         "anyone.") +
      "</p>" + "<p>" +
      tr("You are exporting a <b>minimum size</b> private key, which "
         "removes all signatures except for the latest self-signatures.") +
      "</p>" + "<p>" + tr("Do you <b>REALLY</b> want to proceed?") + "</p>";

  int ret = QMessageBox::warning(
      parentWidget(), tr("Exporting Short Private Key"), warning_message,
      QMessageBox::Cancel | QMessageBox::Ok);
  if (ret != QMessageBox::Ok) {
    deleteLater();
    return;
  }

  exec_export(channel, key, true, true, false, "short_secret");
}

void ExportKey::ExecPrivate(int channel, const GpgKeyPtr& key) {
  QString warning_message =
      "<h3><b>" + tr("WARNING: You are about to export your") + " " +
      "<font color=\"red\">" + tr("PRIVATE KEY") + "</font>!</b></h3>" + "<p>" +
      tr("This operation will export your <b>private key</b>, including both "
         "the main key and all subkeys, "
         "into an external file. This key is extremely sensitive, and anyone "
         "with access to it can impersonate you. "
         "DO NOT share this file with anyone!") +
      "</p>" + "<p>" +
      tr("Are you <b>ABSOLUTELY SURE</b> you want to proceed?") + "</p>";

  int ret = QMessageBox::warning(parentWidget(), tr("Exporting Private Key"),
                                 warning_message,
                                 QMessageBox::Cancel | QMessageBox::Ok);
  if (ret != QMessageBox::Ok) {
    deleteLater();
    return;
  }

  exec_export(channel, key, true, true, false, "full_secret");
}

}  // namespace GpgFrontend::UI
