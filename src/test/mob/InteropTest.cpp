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

#include <QFile>

#include "MobileCryptoTest.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "crypto/TextCryptoController.h"

/**
 * @file InteropTest.cpp
 * @brief The mobile application's stages of the profile interop check.
 *
 * Disabled, so no ordinary run sees them: scripts/run_interop.sh runs the
 * first, then the desktop's stage (GpgCoreTestProfileInterop.cpp) on the
 * same profile, then the second. The mobile stages go through the
 * application's own TextCryptoController, so what is checked is what the
 * phone does. GF_INTEROP_DIR is where the stages leave their files.
 */

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::TextCryptoController;

namespace {

auto ExchangeFile(const QString& name) -> QString {
  return qEnvironmentVariable("GF_INTEROP_DIR") + QLatin1Char('/') + name;
}

void WriteExchange(const QString& name, const QByteArray& content) {
  QFile file(ExchangeFile(name));
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write(content);
}

auto ReadExchange(const QString& name) -> QByteArray {
  QFile file(ExchangeFile(name));
  return file.open(QIODevice::ReadOnly) ? file.readAll().trimmed()
                                        : QByteArray();
}

/// Run @p op on a controller of the interface thread and wait for it.
auto RunText(const std::function<void(TextCryptoController*)>& op)
    -> std::tuple<QString, QVariantMap> {
  TextCryptoController* crypto = nullptr;
  RunOnMainThread([&crypto, &op]() {
    crypto = new TextCryptoController();
    op(crypto);
  });
  EXPECT_TRUE(WaitFor(
      [crypto]() {
        bool busy = true;
        RunOnMainThread([crypto, &busy]() { busy = crypto->Busy(); });
        return !busy;
      },
      30000));
  QString output;
  QVariantMap result;
  RunOnMainThread([crypto, &output, &result]() {
    output = crypto->Output();
    result = crypto->Result();
    delete crypto;
  });
  return {output, result};
}

auto GoodSignature(const QVariantMap& result) -> bool {
  const auto signatures = result.value("signatures").toList();
  return signatures.size() == 1 &&
         signatures.front().toMap().value("good").toBool();
}

}  // namespace

TEST(ProfileInteropTest, DISABLED_MobileWritesAMessage) {
  ASSERT_FALSE(qEnvironmentVariable("GF_INTEROP_DIR").isEmpty());
  const auto fpr = GenerateKey("Interop Phone", {});
  ASSERT_FALSE(fpr.isEmpty());

  const QString message = "Written on the phone, " + fpr;
  auto [armored, result] =
      RunText([&](auto* c) { c->encrypt(message, {fpr}, fpr); });
  ASSERT_GE(result.value("status").toInt(), 0)
      << result.value("errors").toStringList().join("; ").toStdString();
  ASSERT_TRUE(armored.startsWith("-----BEGIN PGP MESSAGE-----"));

  WriteExchange("mobile.fpr", fpr.toUtf8());
  WriteExchange("mobile.asc", armored.toUtf8());
  WriteExchange("mobile.txt", message.toUtf8());
}

TEST(ProfileInteropTest, DISABLED_MobileReadsTheReply) {
  const auto reply = ReadExchange("desktop.asc");
  const auto expected = ReadExchange("desktop.txt");
  ASSERT_FALSE(reply.isEmpty()) << "the desktop stage left no reply";

  auto [plaintext, result] =
      RunText([&](auto* c) { c->decrypt(QString::fromUtf8(reply)); });
  EXPECT_EQ(result.value("status").toInt(), 1);
  EXPECT_EQ(plaintext.toUtf8(), expected);
  EXPECT_TRUE(GoodSignature(result));
}

}  // namespace GpgFrontend::Test::Mobile
