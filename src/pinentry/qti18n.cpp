/* qti18n.cpp - Load qt translations for pinentry.
 * Copyright 2021 g10 Code GmbH
 * SPDX-FileCopyrightText: 2015 Lukáš Tinkl <ltinkl@redhat.com>
 * SPDX-FileCopyrightText: 2021 Ingo Klöcker <kloecker@kde.org>
 *
 * Copied from k18n under the terms of LGPLv2 or later.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <QDebug>
#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include <memory>

static bool loadCatalog(const QString &catalog, const QLocale &locale)
{
    auto translator = new QTranslator(QCoreApplication::instance());

    if (!translator->load(locale, catalog, QString(), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        qDebug() << "Loading the" << catalog << "catalog failed for locale" << locale;
        delete translator;
        return false;
    }
    QCoreApplication::instance()->installTranslator(translator);
    return true;
}

static bool loadCatalog(const QString &catalog, const QLocale &locale, const QLocale &fallbackLocale)
{
    // try to load the catalog for locale
    if (loadCatalog(catalog, locale)) {
        return true;
    }
    // if this fails, then try the fallback locale (if it's different from locale)
    if (fallbackLocale != locale) {
        return loadCatalog(catalog, fallbackLocale);
    }
    return false;
}

// load global Qt translation, needed in KDE e.g. by lots of builtin dialogs (QColorDialog, QFontDialog) that we use
static void loadTranslation(const QString &localeName, const QString &fallbackLocaleName)
{
    const QLocale locale{localeName};
    const QLocale fallbackLocale{fallbackLocaleName};
    // first, try to load the qt_ meta catalog
    if (loadCatalog(QStringLiteral("qt_"), locale, fallbackLocale)) {
        return;
    }
    // if loading the meta catalog failed, then try loading the four catalogs
    // it depends on, i.e. qtbase, qtscript, qtmultimedia, qtxmlpatterns, separately
    const auto catalogs = {
        QStringLiteral("qtbase_"),
        /* QStringLiteral("qtscript_"),
        QStringLiteral("qtmultimedia_"),
        QStringLiteral("qtxmlpatterns_"), */
    };
    for (const auto &catalog : catalogs) {
        loadCatalog(catalog, locale, fallbackLocale);
    }
}

static void load()
{
    // The way Qt translation system handles plural forms makes it necessary to
    // have a translation file which contains only plural forms for `en`. That's
    // why we load the `en` translation unconditionally, then load the
    // translation for the current locale to overload it.
    loadCatalog(QStringLiteral("qt_"), QLocale{QStringLiteral("en")});

    const QLocale locale = QLocale::system();
    if (locale.name() != QStringLiteral("en")) {
        loadTranslation(locale.name(), locale.bcp47Name());
    }
}

Q_COREAPP_STARTUP_FUNCTION(load)
