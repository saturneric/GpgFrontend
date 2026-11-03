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
#include "ui/widgets/TabClosePrompter.h"

#include "ui/dialog/QuitDialog.h"

namespace GpgFrontend::UI {

auto TabClosePrompter::AskSaveOne(QWidget* parent, const UnsavedTab& tab)
    -> SaveChoice {
  const auto result = QMessageBox::warning(
      parent,
      QCoreApplication::translate("GpgFrontend::UI::TextEdit",
                                  "Unsaved Document"),
      QCoreApplication::translate(
          "GpgFrontend::UI::TextEdit",
          "The document \"%1\" has been modified. Do you want to "
          "save your changes?")
              .arg(tab.name.toHtmlEscaped()) +
          "<br/><b>" +
          QCoreApplication::translate("GpgFrontend::UI::TextEdit", "Note:") +
          "</b> " +
          QCoreApplication::translate(
              "GpgFrontend::UI::TextEdit",
              "If you don't save, your changes will be lost.") +
          "<br/>",
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

  switch (result) {
    case QMessageBox::Save:
      return SaveChoice::kSAVE;
    case QMessageBox::Discard:
      return SaveChoice::kDISCARD;
    default:
      return SaveChoice::kCANCEL;
  }
}

auto TabClosePrompter::AskSaveMany(QWidget* parent,
                                   const QList<UnsavedTab>& tabs)
    -> std::optional<QSet<quint64>> {
  QList<QPair<quint64, QString>> docs;
  for (const auto& t : tabs) docs.append({t.id, t.name});

  std::optional<QSet<quint64>> answer;
  QuitDialog dialog(parent, docs);
  QObject::connect(&dialog, &QuitDialog::SignalDiscard, &dialog,
                   [&]() { answer = QSet<quint64>{}; });
  QObject::connect(&dialog, &QuitDialog::SignalSave, &dialog,
                   [&](const QList<quint64>& ids) {
                     answer = QSet<quint64>(ids.cbegin(), ids.cend());
                   });
  dialog.exec();
  return answer;
}

auto TabClosePrompter::ChooseSavePath(QWidget* parent, const QString& suggested,
                                      const QString& filter) -> QString {
  return QFileDialog::getSaveFileName(
      parent,
      QCoreApplication::translate("GpgFrontend::UI::TextEdit", "Save File"),
      suggested, filter);
}

void TabClosePrompter::SaveFailed(QWidget* parent, const QString& path,
                                  const QString& error) {
  QMessageBox::warning(
      parent,
      QCoreApplication::translate("GpgFrontend::UI::TextEdit", "Warning"),
      QCoreApplication::translate("GpgFrontend::UI::TextEdit",
                                  "Cannot read file %1:\n%2.")
          .arg(path)
          .arg(error));
}

}  // namespace GpgFrontend::UI
