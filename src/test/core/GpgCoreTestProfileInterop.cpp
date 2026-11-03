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

/**
 * @file GpgCoreTestProfileInterop.cpp
 * @brief The desktop's stages of the profile interop check.
 *
 * Disabled, so no ordinary run sees them: scripts/run_interop.sh runs them in
 * gf_core_tests started as the application (GF_TEST_AS_APPLICATION=1),
 * against the profile the phone's tests (gf_mob_tests) have just used.
 * GF_INTEROP_DIR is where the stages leave their files.
 *
 * A profile the phone created records rPGP as its default engine, and its
 * keyring is opened as an rPGP database of its own
 * (AdoptLocalDefaultKeyDatabase), since the desktop's DEFAULT database is its
 * own keyring's place. The stage checks both, finds the phone's key there,
 * decrypts and verifies its message, and answers it.
 */

#include <QFile>

#include "GpgFrontendTest.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

namespace {

auto ExchangeFile(const QString& name) -> QString {
  return qEnvironmentVariable("GF_INTEROP_DIR") + QLatin1Char('/') + name;
}

auto ReadExchange(const QString& name) -> QByteArray {
  QFile file(ExchangeFile(name));
  return file.open(QIODevice::ReadOnly) ? file.readAll().trimmed()
                                        : QByteArray();
}

}  // namespace

TEST(ProfileInteropTest, DISABLED_DesktopReadsAndReplies) {
  ASSERT_FALSE(qEnvironmentVariable("GF_INTEROP_DIR").isEmpty());
  const auto fpr = QString::fromUtf8(ReadExchange("mobile.fpr")).toUpper();
  const auto message = ReadExchange("mobile.asc");
  const auto expected = ReadExchange("mobile.txt");
  ASSERT_FALSE(fpr.isEmpty());
  ASSERT_FALSE(message.isEmpty());

  // The phone created the profile and recorded its engine there.
  EXPECT_EQ(SettingItems::kDefaultEngine.Read(GetSettings()).toUpper(),
            QStringLiteral("RPGP"));

  // Whichever database the desktop opened the phone's keyring as.
  int channel = -1;
  QString backend;
  QStringList opened;
  for (const auto& info : GetGpgKeyDatabaseInfos()) {
    opened << QStringLiteral("%1 %2 (%3) at %4")
                  .arg(info.channel)
                  .arg(info.name, info.backend_type, info.path);
    GpgKeyRepository::GetInstance(info.channel).FlushKeyCache();
    if (GpgKeyRepository::GetInstance(info.channel).GetKeyPtr(fpr) != nullptr) {
      channel = info.channel;
      backend = info.backend_type;
      break;
    }
  }
  ASSERT_GE(channel, 0) << "no key database the desktop opened holds "
                        << fpr.toStdString()
                        << "; opened: " << opened.join("; ").toStdString();
  std::printf("interop: mobile key found on channel %d (%s)\n", channel,
              backend.toUtf8().constData());
  // An rPGP keyring is only ever opened as an rPGP database.
  EXPECT_EQ(backend, QStringLiteral("rpgp"));

  auto& opera = MessageCryptoOperation::GetInstance(channel);
  {
    auto [err, data] = opera.DecryptVerifySync(GFBuffer(message));
    ASSERT_TRUE(err.Ok());
    ASSERT_TRUE((data->Check<GpgDecryptResult, GpgVerifyResult, GFBuffer>()));

    GpgDecryptResultAnalyse decrypt(channel, err,
                                    ExtractParams<GpgDecryptResult>(data, 0));
    decrypt.Analyse();
    GpgVerifyResultAnalyse verify(channel, err,
                                  ExtractParams<GpgVerifyResult>(data, 1));
    verify.Analyse();
    EXPECT_GT(decrypt.GetStatus(), 0);
    EXPECT_GT(verify.GetStatus(), 0) << verify.GetResultReport().toStdString();
    EXPECT_EQ(ExtractParams<GFBuffer>(data, 2).ConvertToQByteArray(), expected);
  }

  // The answer, to the same key and signed by it.
  const auto key = GpgKeyRepository::GetInstance(channel).GetKeyPtr(fpr);
  const QByteArray reply = "Reply from the desktop, " + fpr.toUtf8();
  auto [err, data] = opera.EncryptSignSync({key}, {key}, GFBuffer(reply), true);
  ASSERT_TRUE(err.Ok());
  ASSERT_TRUE((data->Check<GpgEncryptResult, GpgSignResult, GFBuffer>()));

  QFile out(ExchangeFile("desktop.asc"));
  ASSERT_TRUE(out.open(QIODevice::WriteOnly));
  out.write(ExtractParams<GFBuffer>(data, 2).ConvertToQByteArray());
  QFile text(ExchangeFile("desktop.txt"));
  ASSERT_TRUE(text.open(QIODevice::WriteOnly));
  text.write(reply);
}

TEST(ProfileInteropTest, DISABLED_DesktopKeepsItsEngine) {
  // Before and after the phone opened the profile this desktop made: no
  // engine stored, so the default keyring stays the desktop's choice.
  EXPECT_FALSE(GetSettings().contains(SettingItems::kDefaultEngine.Key()));
}

}  // namespace GpgFrontend::Test
