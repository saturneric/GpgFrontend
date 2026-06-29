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

#include "GpgCoreEngineTest.h"

#include "core/GFConstants.h"
#include "core/GFCoreInit.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyGenerationOperation.h"
#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/function/rpgp/PasswordFetcher.h"
#include "core/model/GFEngineSupportIf.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKey.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

namespace {

/// @brief Feeds a fixed passphrase to GnuPG. Generated test keys carry no
/// passphrase, but the callback is installed defensively.
auto TestPassphraseCb(void* /*opaque*/, const char* /*uid_hint*/,
                      const char* /*passphrase_info*/, int /*last_was_bad*/,
                      int fd) -> gpgme_error_t {
  QString passphrase = "abcdefg";
  auto pass_bytes = passphrase.toLatin1();
  auto pass_size = pass_bytes.size();
  const auto* p_pass_bytes = pass_bytes.constData();

  qsizetype res = 0;
  if (pass_size > 0) {
    qsizetype off = 0;
    qsizetype ret = 0;
    do {
      ret = gpgme_io_write(fd, &p_pass_bytes[off], pass_size - off);
      if (ret > 0) off += ret;
    } while (ret > 0 && off != pass_size);
    res = off;
  }

  res += gpgme_io_write(fd, "\n", 1);
  return res == pass_size + 1 ? 0 : GPG_ERR_CANCELED;
}

/// @brief Build the engine context for a channel exactly once per process run.
auto EnsureEngine(const EngineTestParam& p) -> bool {
  static QSet<int> ready;
  if (ready.contains(p.channel)) return true;

  auto db_path = QDir(QDir::tempPath() + "/" + GenerateRandomString(12));
  if (db_path.exists()) db_path.rmdir(".");
  db_path.mkpath(".");
  Q_ASSERT(db_path.exists());

  auto succ =
      BuildOpenPGPContext(p.channel, OpenPGPContextInitArgs{
                                         .engine = p.engine,
                                         .db_name = "test-engine-db",
                                         .db_path = db_path.canonicalPath(),
                                         .offline_mode = true,
                                         .auto_import_missing_key = false,
                                     });
  if (!succ) {
    LOG_E() << "configure engine context for unit test failed, channel:"
            << p.channel;
    return false;
  }

  if (p.engine == OpenPGPEngine::kGNUPG) {
    auto& ctx = GpgCtx(OpenPGPContext::GetInstance(p.channel));
    ctx.SetPassphraseCb(ctx.DefaultContext(), TestPassphraseCb);
    ctx.SetPassphraseCb(ctx.BinaryContext(), TestPassphraseCb);
  } else if (p.engine == OpenPGPEngine::kRPGP) {
    const auto fetcher = [](const PassphraseState&) -> GFBuffer {
      return GFBuffer("123456");
    };
    // Normal crypto ops invoke the callback with the real channel. Key
    // generation, however, goes through gfr_crypto_generate_key(), which has no
    // channel parameter, so the Rust side passes the *key index* as the channel
    // to the password callback (0 = primary, 1 = first subkey, ...). Register
    // the fetcher for those pseudo-channels too so a passphrase-protected key
    // can be generated head-less. The fetcher ignores the channel anyway.
    SetChannelPasswordFetcher(p.channel, fetcher);
    SetChannelPasswordFetcher(0, fetcher);
    SetChannelPasswordFetcher(1, fetcher);
  }

  ready.insert(p.channel);
  return true;
}

/// @brief First algo from @p algos whose id appears earliest in @p prefs;
/// falls back to the first supported algo.
auto SelectAlgo(const QContainer<KeyAlgo>& algos, const QStringList& prefs)
    -> KeyAlgo {
  for (const auto& pref : prefs) {
    for (const auto& algo : algos) {
      if (algo.Id() == pref) return algo;
    }
  }
  return algos.front();
}

}  // namespace

void GpgCoreEngineTest::SetUp() {
  const auto& p = GetParam();
  if (!GetGSS().IsEngineSupported(p.engine)) {
    GTEST_SKIP() << p.name.toStdString() << " engine is not available.";
  }
  if (!EnsureEngine(p)) {
    GTEST_SKIP() << "failed to initialize " << p.name.toStdString()
                 << " engine.";
  }
}

void GpgCoreEngineTest::TearDown() {}

auto GpgCoreEngineTest::PickPrimaryAlgo() const -> KeyAlgo {
  auto algos = KeyGenerateInfo::GetSupportedKeyAlgo(Channel());
  EXPECT_FALSE(algos.empty());
  return SelectAlgo(algos, {"ed25519", "nistp256", "rsa2048"});
}

auto GpgCoreEngineTest::PickEncryptionSubAlgo() const -> KeyAlgo {
  auto algos = KeyGenerateInfo::GetSupportedSubkeyAlgo(Channel());
  EXPECT_FALSE(algos.empty());
  return SelectAlgo(algos, {"cv25519", "nistp256", "rsa2048"});
}

auto GpgCoreEngineTest::GenerateFullKey(const QString& uid_suffix,
                                        bool with_passphrase) -> GpgKeyPtr {
  const bool non_passphrase = !with_passphrase;
  auto p_info = QSharedPointer<KeyGenerateInfo>::create();
  p_info->SetName("engine_test_" + uid_suffix);
  p_info->SetEmail("engine_test@gpgfrontend.bktus.com");
  p_info->SetComment("engine independent test");
  p_info->SetAlgo(PickPrimaryAlgo());
  p_info->SetNonExpired(true);
  p_info->SetNonPassPhrase(non_passphrase);

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);
  s_info->SetAlgo(PickEncryptionSubAlgo());
  s_info->SetNonExpired(true);
  s_info->SetNonPassPhrase(non_passphrase);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(Channel()).GenerateKeyWithSubkeySync(
          p_info, s_info);

  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return nullptr;
  if (!data_object->Check<GpgGenerateKeyResult, GpgGenerateKeyResult>()) {
    return nullptr;
  }

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  if (!result.IsGood() || result.GetFingerprint().isEmpty()) return nullptr;

  GpgKeyRepository::GetInstance(Channel()).FlushKeyCache();
  // Engines disagree on fingerprint casing (GnuPG upper, rPGP lower) while the
  // repository keys by upper-case; normalise so lookup is engine-independent.
  return GpgKeyRepository::GetInstance(Channel()).GetKeyPtr(
      result.GetFingerprint().toUpper());
}

void GpgCoreEngineTest::DeleteKey(const GpgKeyPtr& key) const {
  if (key == nullptr) return;
  KeyManagementOperation::GetInstance(Channel()).DeleteKey(key);
}

auto GpgCoreEngineTest::StressIterations() -> int {
  constexpr int kDefaultIterations = 50;
  auto iter = qEnvironmentVariableIntValue("GF_STRESS_ITER");
  return iter > 0 ? iter : kDefaultIterations;
}

INSTANTIATE_TEST_SUITE_P(
    Engines, GpgCoreEngineTest,
    ::testing::Values(EngineTestParam{"GnuPG", OpenPGPEngine::kGNUPG,
                                      kEngineTestGnupgChannel},
                      EngineTestParam{"RPGP", OpenPGPEngine::kRPGP,
                                      kEngineTestRpgpChannel}),
    [](const ::testing::TestParamInfo<EngineTestParam>& info) {
      return info.param.name.toStdString();
    });

// Regression test for the deep-restart crash that prevented the app from
// relaunching after the first key database was added (macOS sandbox / rPGP
// fallback). OpenPGPContext::GetInstance() lazily creates a placeholder context
// for a channel that has no real context (e.g. one already torn down during the
// restart). If that placeholder defaulted to the GnuPG engine, an engine
// support check would call EngineVersion(), route into the live gpg-agent query
// and dynamic_cast the placeholder to GpgContext, throwing and killing the
// process before it could relaunch. The placeholder must instead report the
// always-available rPGP engine and a GnuPG support check must simply fail.
TEST(GpgCorePlaceholderContextTest, PlaceholderChannelIsNotGnuPG) {
  // A channel never configured with a real context in any test.
  constexpr int kUnusedChannel = 4096;

  auto& ctx = OpenPGPContext::GetInstance(kUnusedChannel);
  EXPECT_EQ(ctx.Engine(), OpenPGPEngine::kRPGP);

  // Must not throw, and must report that GnuPG is not supported on this
  // channel.
  bool supported = true;
  EXPECT_NO_THROW(supported = GpgContextSupportIf(
                      kUnusedChannel,
                      {{OpenPGPEngine::kGNUPG, kGpgMinimalSupportVersion}}));
  EXPECT_FALSE(supported);
}

}  // namespace GpgFrontend::Test
