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

// The screenshot mode pins GnuPG's clock with faked-system-time in the
// homedir's conf files. These cases prove, through the same GPGME key
// listing and decrypt/verify operations the application uses, that the
// pinned clock is the one GnuPG judges by, and that gpgconf's verdict on
// each conf file is read correctly.

#include <gtest/gtest.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QProcess>

#include "GpgFrontendTest.h"
#include "core/GFCoreInit.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/model/GpgAbstractKey.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/module/ModuleManager.h"
#include "core/utils/GpgUtils.h"
#include "screenshot/ScreenshotEnv.h"
#include "screenshot/ScreenshotOptions.h"

namespace GpgFrontend::Test {

namespace {

using namespace GpgFrontend::Screenshot;  // NOLINT

auto ResourcePath(const QString& name) -> QString {
  return QString(GF_TEST_SOURCE_DIR) + "/resource/screenshot/" + name;
}

auto ReadFile(const QString& path) -> QByteArray {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return {};
  return file.readAll();
}

/// A built-in demo key's fingerprint, from demo-keys.json: regenerating the
/// demo data changes every one of them.
auto Fpr(const QString& name) -> QString {
  const auto demo =
      ParseDemoData(ReadFile(ResourcePath("demo-keys.json")), nullptr);
  if (!demo) return {};
  for (const auto& key : demo->keys) {
    if (key.name == name) return key.fingerprint;
  }
  return {};
}

auto Gpgconf(const QStringList& args) -> QString {
  const auto gpgconf = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});
  QProcess process;
  process.start(gpgconf, args);
  process.waitForFinished(15000);
  return QString::fromUtf8(process.readAllStandardOutput());
}

/**
 * @brief A copy of the demo homedir on its own channel, kept on a short
 *        path so gpg-agent's socket fits.
 */
class ScreenshotClockTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if (Module::RetrieveRTValueTypedOrDefault<>(
            "core", "gpgme.ctx.gpgconf_path", QString{})
            .isEmpty()) {
      GTEST_SKIP() << "GnuPG is not available";
    }

    home_ = QDir::tempPath() + "/gfc-" + GenerateRandomString(6);
    ASSERT_TRUE(QDir().mkpath(home_));
    const QDir source(ResourcePath("demo-gnupghome"));
    QDirIterator it(source.path(),
                    QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
      const auto path = it.next();
      const auto target = QDir(home_).filePath(source.relativeFilePath(path));
      if (it.fileInfo().isDir()) {
        ASSERT_TRUE(QDir().mkpath(target));
      } else {
        ASSERT_TRUE(QFile::copy(path, target));
      }
    }
    QFile::setPermissions(home_, QFileDevice::ReadOwner |
                                     QFileDevice::WriteOwner |
                                     QFileDevice::ExeOwner);

    set_clock("2026-01-15T10:00:00Z");
    channel_ = AllocateChannelHandle() + 0x200000;
    ASSERT_TRUE(BuildOpenPGPContext(
        channel_, OpenPGPContextInitArgs{.engine = OpenPGPEngine::kGNUPG,
                                         .db_name = "screenshot-demo",
                                         .db_path = home_,
                                         .offline_mode = true,
                                         .auto_import_missing_key = false}));
  }

  void TearDown() override {
    if (home_.isEmpty()) return;
    Gpgconf({"--homedir", home_, "--kill", "all"});
    QDir(home_).removeRecursively();
  }

  void set_clock(const QString& clock) {
    const auto line = QStringLiteral("faked-system-time %1\n")
                          .arg(GpgFakedSystemTime(*ParseClock(clock)));
    QFile file(QDir(home_).filePath("gpg.conf"));
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(line.toUtf8());
  }

  auto key(const QString& fpr) const -> GpgAbstractKeyPtr {
    auto& repo = AbstractKeyRepository::GetInstance(channel_);
    repo.FlushCache();
    return repo.GetKey(fpr);
  }

  auto verify_status(const QString& fixture) const -> gpgme_error_t {
    auto [err, data] = MessageCryptoOperation::GetInstance(channel_).VerifySync(
        GFBuffer(ReadFile(ResourcePath("fixtures/" + fixture))), GFBuffer());
    EXPECT_TRUE((data->Check<GpgVerifyResult, GFBuffer>()));
    const auto signatures =
        ExtractParams<GpgVerifyResult>(data, 0).GetSignature();
    EXPECT_EQ(signatures.size(), 1);
    return signatures.isEmpty() ? GPG_ERR_GENERAL
                                : gpg_err_code(signatures.front().GetStatus());
  }

  QString home_;
  int channel_ = 0;
};

}  // namespace

TEST_F(ScreenshotClockTest, ConfFilesAreVettedSeparately) {
  QFile agent(QDir(home_).filePath("gpg-agent.conf"));
  ASSERT_TRUE(agent.open(QIODevice::WriteOnly));
  agent.write("faked-system-time 20260115T100000!\n");
  agent.close();

  EXPECT_EQ(ParseCheckOptions(
                Gpgconf({"--homedir", home_, "--check-options", "gpg"}), "gpg"),
            true);
  EXPECT_EQ(ParseCheckOptions(
                Gpgconf({"--homedir", home_, "--check-options", "gpg-agent"}),
                "gpg-agent"),
            true);

  // gpgconf exits 0 here too; only the okay field says no.
  QFile gpg(QDir(home_).filePath("gpg.conf"));
  ASSERT_TRUE(gpg.open(QIODevice::Append));
  gpg.write("no-such-option-injected\n");
  gpg.close();
  EXPECT_EQ(ParseCheckOptions(
                Gpgconf({"--homedir", home_, "--check-options", "gpg"}), "gpg"),
            false);
  EXPECT_EQ(ParseCheckOptions(
                Gpgconf({"--homedir", home_, "--check-options", "gpg-agent"}),
                "gpg-agent"),
            true);
}

TEST_F(ScreenshotClockTest, KeyListingFollowsThePinnedClock) {
  set_clock("2026-01-15T10:00:00Z");
  auto dave = key(Fpr("Dave"));
  ASSERT_NE(dave, nullptr);
  EXPECT_TRUE(dave->IsExpired());

  set_clock("2025-04-15T10:00:00Z");
  dave = key(Fpr("Dave"));
  ASSERT_NE(dave, nullptr);
  EXPECT_FALSE(dave->IsExpired());
}

TEST_F(ScreenshotClockTest, VerifyFollowsThePinnedClock) {
  // Signed by Dave on 2025-03-01; his key expires on 2025-06-01.
  set_clock("2026-01-15T10:00:00Z");
  key(Fpr("Dave"));
  EXPECT_EQ(verify_status("clock-probe-dave.asc"), GPG_ERR_KEY_EXPIRED);

  set_clock("2025-04-15T10:00:00Z");
  key(Fpr("Dave"));
  EXPECT_EQ(verify_status("clock-probe-dave.asc"), GPG_ERR_NO_ERROR);
}

TEST_F(ScreenshotClockTest, DecryptVerifyFixtureIsReadableAtTheDefaultClock) {
  set_clock(kDefaultClock);
  key(Fpr("Bob"));
  auto [err, data] =
      MessageCryptoOperation::GetInstance(channel_).DecryptVerifySync(
          GFBuffer(ReadFile(ResourcePath("fixtures/decrypt-verify.asc"))));
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data->Check<GpgDecryptResult, GpgVerifyResult, GFBuffer>()));

  const auto plain = ExtractParams<GFBuffer>(data, 2).ConvertToQByteArray();
  EXPECT_TRUE(plain.contains("the quarterly report is attached"));

  const auto signatures =
      ExtractParams<GpgVerifyResult>(data, 1).GetSignature();
  ASSERT_EQ(signatures.size(), 1);
  EXPECT_EQ(signatures.front().GetFingerprint(), Fpr("Bob"));
  EXPECT_EQ(gpg_err_code(signatures.front().GetStatus()), GPG_ERR_NO_ERROR);
}

}  // namespace GpgFrontend::Test
