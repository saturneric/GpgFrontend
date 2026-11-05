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

#include "shell/present/TranslationUtils.h"

#include <QDir>
#include <QTranslator>
#include <algorithm>

#include "core/utils/BuildInfoUtils.h"

namespace GpgFrontend {

void InstallTranslations(const QLocale &locale) {
  // Uninstall *and* destroy: merely forgetting the pointers would pile up
  // translators that the application still consults.
  static QList<QTranslator *> installed;
  for (auto *translator : installed) {
    QCoreApplication::removeTranslator(translator);
    delete translator;
  }
  installed.clear();

  // Consulted last installed first. The application's English catalog sits
  // beneath the one asked for: it spells out each plural ("%n key(s)") for a
  // language with no catalog of its own.
  const QList<TranslationCatalog> catalogs{
      {u"qt"_s, u"_"_s, u":/i18n_qt"_s},
      {u"qtbase"_s, u"_"_s, u":/i18n_qt"_s},
      {GetProjectName(), u"."_s, u":/i18n"_s, u"en_US"_s},
      {GetProjectName(), u"."_s, u":/i18n"_s},
  };

  for (const auto &catalog : catalogs) {
    auto *translator = new QTranslator(QCoreApplication::instance());
    const auto language =
        catalog.locale.isEmpty() ? locale : QLocale(catalog.locale);
    if (translator->load(language, catalog.name, catalog.separator,
                         catalog.directory, u".qm"_s) &&
        QCoreApplication::installTranslator(translator)) {
      installed.append(translator);
    } else {
      delete translator;
    }
  }
  qInfo() << "translations installed for" << locale.name() << ":"
          << installed.size() << "catalogs";
}

auto AvailableUILanguages() -> QList<QPair<QString, QString>> {
  // "<name>.<locale>.qm": the application's catalogs and the modules', each
  // locale once.
  QHash<QString, QString> languages;
  for (const auto &file : QDir(u":/i18n"_s).entryList()) {
    const auto start = file.indexOf('.') + 1;
    const auto end = file.lastIndexOf('.');
    if (start <= 0 || end < 0 || start >= end) continue;

    const auto name = file.mid(start, end - start);
    const QLocale locale(name);
    if (locale.nativeTerritoryName().isEmpty()) continue;
    languages.insert(locale.name(),
                     locale.nativeLanguageName() + " (" + name + ")");
  }

  QList<QPair<QString, QString>> entries;
  for (auto it = languages.constBegin(); it != languages.constEnd(); ++it) {
    entries.append({it.key(), it.value()});
  }
  std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
    return a.second.localeAwareCompare(b.second) < 0;
  });
  return entries;
}

}  // namespace GpgFrontend
