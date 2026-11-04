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

#include "core/utils/TranslationUtils.h"

#include <QTranslator>

#include "core/utils/BuildInfoUtils.h"

namespace GpgFrontend {

void InstallTranslations(const QLocale &locale,
                         const QList<TranslationCatalog> &extra) {
  // Uninstall *and* destroy: merely forgetting the pointers would pile up
  // translators that the application still consults.
  static QList<QTranslator *> installed;
  for (auto *translator : installed) {
    QCoreApplication::removeTranslator(translator);
    delete translator;
  }
  installed.clear();

  // Consulted last installed first.
  QList<TranslationCatalog> catalogs{
      {u"qt"_s, u"_"_s, u":/i18n_qt"_s},
      {u"qtbase"_s, u"_"_s, u":/i18n_qt"_s},
      {GetProjectName(), u"."_s, u":/i18n"_s},
  };
  catalogs += extra;

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

}  // namespace GpgFrontend
