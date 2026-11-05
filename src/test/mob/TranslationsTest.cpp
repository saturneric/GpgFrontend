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
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include "MobileTest.h"
#include "shell/present/TranslationUtils.h"

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

// Every word a page writes (a qsTr() of one literal) is in the catalog: one
// that is the same in German and in Chinese was never translated. The
// Language setting itself was English on a German phone, and a sample of
// two strings could not tell.
TEST(TranslationsTest, EveryPageStringIsTranslated) {
  // The same in every language: names, units.
  const QSet<QString> kSameEverywhere = {
      QStringLiteral("GpgFrontend"), QStringLiteral("SHA-256"),
      QStringLiteral("PIN"),         QStringLiteral("%1 kB"),
      QStringLiteral("%1 MB"),       QStringLiteral("%1 GB"),
  };
  // One literal, closed at once: no "a" + "b", no plural's comment.
  static const QRegularExpression kLiteral(
      QStringLiteral(R"re(qsTr\(\s*"((?:[^"\\]|\\.)*)"\s*\))re"));
  static const QRegularExpression kWords(QStringLiteral("[A-Za-z]{2,}"));

  QList<QPair<QString, QString>> strings;  ///< context, source
  QDirIterator it(QStringLiteral(":/qt/qml/GpgFrontend/Mobile"),
                  {QStringLiteral("*.qml")}, QDir::Files,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const auto path = it.next();
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly)) << path.toStdString();
    const auto source = QString::fromUtf8(file.readAll());
    const auto context = QFileInfo(path).completeBaseName();
    auto match = kLiteral.globalMatch(source);
    while (match.hasNext()) {
      auto text = match.next().captured(1);
      text.replace(QStringLiteral("\\\""), QStringLiteral("\""));
      if (text.contains(QStringLiteral("%n"))) continue;
      auto words = text;
      words.remove(QRegularExpression(QStringLiteral("%\\d")));
      if (!kWords.match(words).hasMatch()) continue;
      if (kSameEverywhere.contains(text)) continue;
      strings.append({context, text});
    }
  }
  ASSERT_GT(strings.size(), 300) << "the pages' sources were not found";

  const auto in = [&strings](const char* locale) {
    QSet<QString> same;
    RunOnMainThread([&]() {
      InstallTranslations(QLocale(QString::fromLatin1(locale)));
      for (const auto& [context, text] : strings) {
        const auto said = QCoreApplication::translate(
            context.toUtf8().constData(), text.toUtf8().constData());
        if (said == text) same.insert(context + QStringLiteral(": ") + text);
      }
      InstallTranslations(QLocale::c());
    });
    return same;
  };
  const auto untranslated = in("de_DE") & in("zh_CN");
  EXPECT_TRUE(untranslated.isEmpty())
      << QStringList(untranslated.begin(), untranslated.end())
             .join(QStringLiteral("\n"))
             .toStdString();
  // The setting that was found in English: in German now.
  RunOnMainThread([]() {
    InstallTranslations(QLocale(QStringLiteral("de_DE")));
    EXPECT_EQ(QCoreApplication::translate("SettingsPage", "Language"),
              QStringLiteral("Sprache"));
    EXPECT_EQ(QCoreApplication::translate("GpgFrontend::Mobile::SettingsModel",
                                          "System default"),
              QStringLiteral("Systemstandard"));
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
