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

#include "ui/function/LanguageOptions.h"

#include "ui/dialog/settings/SettingsDialog.h"

namespace GpgFrontend::UI {

auto SortedLanguageEntries(const QHash<QString, QString>& languages)
    -> QContainer<QPair<QString, QString>> {
  QContainer<QPair<QString, QString>> entries;
  for (auto it = languages.constBegin(); it != languages.constEnd(); ++it) {
    if (it.key().isEmpty()) continue;
    entries.append({it.key(), it.value()});
  }

  std::sort(
      entries.begin(), entries.end(),
      [](const QPair<QString, QString>& a, const QPair<QString, QString>& b) {
        return a.second.localeAwareCompare(b.second) < 0;
      });

  return entries;
}

void PopulateLanguageComboBox(QComboBox* box, const QString& current_lang) {
  const auto languages = SettingsDialog::ListLanguages();

  // ListLanguages() already carries the translated label for the empty key, so
  // the system default entry needs no string of its own here.
  box->clear();
  box->addItem(languages.value(QString()), QString());
  for (const auto& entry : SortedLanguageEntries(languages)) {
    box->addItem(entry.second, entry.first);
  }

  // Matched on the stored locale key rather than on the display text, so a
  // stale or unknown setting falls back to the system default instead of
  // leaving the box with nothing selected at all.
  const auto index = box->findData(current_lang);
  box->setCurrentIndex(index >= 0 ? index : 0);
}

}  // namespace GpgFrontend::UI
