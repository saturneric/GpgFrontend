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

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QFile>

#include "MobileTest.h"
#include "core/utils/TranslationUtils.h"

namespace GpgFrontend::Test::Mobile {

namespace {

auto Core(const char* text) -> QString {
  return QCoreApplication::translate("GpgFrontend::GpgEncryptResultAnalyse",
                                     text);
}

}  // namespace

TEST(TranslationsTest, TheCoresStringsAreTranslatedOnThePhone) {
  RunOnMainThread([]() {
    InstallTranslations(QLocale(QStringLiteral("de_DE")));
    // One catalog, which the desktop shares: the core's strings and the
    // phone's, where a QML file is the context.
    EXPECT_EQ(Core("Encrypt"), QStringLiteral("Verschlüsseln"));
    EXPECT_NE(QCoreApplication::translate("FilesPage", "Add files"),
              QStringLiteral("Add files"));

    // Installed again for another language, the first is gone.
    InstallTranslations(QLocale(QStringLiteral("en_US")));
    EXPECT_EQ(Core("Encrypt"), QStringLiteral("Encrypt"));
    EXPECT_EQ(QCoreApplication::translate("FilesPage", "Add files"),
              QStringLiteral("Add files"));

    // A language with no catalog leaves every string as written, but for
    // plurals, which the English catalog beneath spells out.
    InstallTranslations(QLocale(QStringLiteral("xx_XX")));
    EXPECT_EQ(Core("Encrypt"), QStringLiteral("Encrypt"));
    EXPECT_EQ(QCoreApplication::translate("Wording", "%n key(s)", nullptr, 1),
              QStringLiteral("1 key"));
    EXPECT_EQ(QCoreApplication::translate("Wording", "%n key(s)", nullptr, 2),
              QStringLiteral("2 keys"));
    InstallTranslations(QLocale::c());
  });
}

TEST(TranslationsTest, ThePhoneSharesTheApplicationsCatalog) {
  // One per supported language, the phone's strings in it.
  EXPECT_TRUE(QFile::exists(":/i18n/GpgFrontend.de_DE.qm"));
  EXPECT_TRUE(QFile::exists(":/i18n/GpgFrontend.zh_CN.qm"));
  EXPECT_FALSE(QFile::exists(":/i18n_mob"));
}

}  // namespace GpgFrontend::Test::Mobile
