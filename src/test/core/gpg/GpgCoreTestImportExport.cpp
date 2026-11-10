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

#include "GpgCoreTest.h"
#include "core/GFConstants.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

namespace {

auto WrongPassphraseCb(void* /*opaque*/, const char* /*uid_hint*/,
                       const char* /*passphrase_info*/, int /*last_was_bad*/,
                       int fd) -> gpgme_error_t {
  const std::string_view line = "not-the-passphrase\n";
  qsizetype off = 0;
  while (off < static_cast<qsizetype>(line.size())) {
    const auto ret = gpgme_io_write(fd, line.data() + off, line.size() - off);
    if (ret <= 0) return GPG_ERR_CANCELED;
    off += ret;
  }
  return GPG_ERR_NO_ERROR;
}

/// What a dismissed pinentry answers.
auto CancelingPassphraseCb(void* /*opaque*/, const char* /*uid_hint*/,
                           const char* /*passphrase_info*/,
                           int /*last_was_bad*/, int /*fd*/) -> gpgme_error_t {
  return GPG_ERR_CANCELED;
}

/**
 * @brief A throwaway key whose secret parts are protected by the fixture's
 * passphrase; the imported test keys are all unprotected, so exporting them
 * never asks for one.
 */
class ProtectedTestKey {
 public:
  ProtectedTestKey()
      : ctx_(GpgCtx(OpenPGPContext::GetInstance(kGpgChannelForUnitTest))) {
    auto* g_ctx = ctx_.DefaultContext();
    const auto uid = QString("Export Skip Test <%1@example.org>")
                         .arg(GenerateRandomString(8));
    if (gpgme_err_code(gpgme_op_createkey(
            g_ctx, uid.toUtf8().constData(), "future-default", 0, 0, nullptr,
            GPGME_CREATE_NOEXPIRE)) != GPG_ERR_NO_ERROR) {
      return;
    }
    const auto* result = gpgme_op_genkey_result(g_ctx);
    if (result == nullptr || result->fpr == nullptr) return;
    fpr_ = result->fpr;

    gpgme_key_t key = nullptr;
    if (gpgme_err_code(gpgme_get_key(g_ctx, result->fpr, &key, 1)) !=
            GPG_ERR_NO_ERROR ||
        key == nullptr) {
      return;
    }
    if (key->subkeys != nullptr && key->subkeys->next != nullptr) {
      subkey_fpr_ = key->subkeys->next->fpr;
    }
    gpgme_key_unref(key);
  }

  ~ProtectedTestKey() {
    if (fpr_.isEmpty()) return;
    auto* g_ctx = ctx_.DefaultContext();
    gpgme_key_t key = nullptr;
    if (gpgme_get_key(g_ctx, fpr_.toLatin1().constData(), &key, 1) ==
            GPG_ERR_NO_ERROR &&
        key != nullptr) {
      gpgme_op_delete_ext(g_ctx, key,
                          GPGME_DELETE_ALLOW_SECRET | GPGME_DELETE_FORCE);
      gpgme_key_unref(key);
    }
    GpgKeyRepository::GetInstance(kGpgChannelForUnitTest).FlushKeyCache();
  }

  ProtectedTestKey(const ProtectedTestKey&) = delete;
  auto operator=(const ProtectedTestKey&) -> ProtectedTestKey& = delete;

  [[nodiscard]] auto Fpr() const -> const QString& { return fpr_; }
  [[nodiscard]] auto SubkeyFpr() const -> const QString& { return subkey_fpr_; }

  /**
   * @brief Answer the passphrase prompts with @p cb until the returned guard
   * goes, then with the fixture's callback again.
   */
  [[nodiscard]] auto AnswerWith(gpgme_passphrase_cb_t cb) {
    gpgme_passphrase_cb_t previous = nullptr;
    void* previous_value = nullptr;
    gpgme_get_passphrase_cb(ctx_.DefaultContext(), &previous, &previous_value);
    ctx_.SetPassphraseCb(ctx_.DefaultContext(), cb);
    return qScopeGuard([this, previous]() {
      ctx_.SetPassphraseCb(ctx_.DefaultContext(), previous);
    });
  }

 private:
  GpgContext& ctx_;
  QString fpr_;
  QString subkey_fpr_;
};

}  // namespace

// The agent hands gpg a protected secret key only after a passphrase prompt.
// When that prompt fails, gpg leaves the key out of the export, reports it in
// a status line, and gpgme may still call the export a success. These pin that
// such an export fails instead of returning a file without the key.

TEST_F(GpgCoreTest, CoreExportSubkeyProtectedTestA) {
  ProtectedTestKey key;
  ASSERT_FALSE(key.SubkeyFpr().isEmpty());

  // With the right passphrase, the subkey is exported: the baseline the two
  // tests below fail against.
  auto [err, gf_buffer] =
      KeyImportExportOperation::GetInstance(kGpgChannelForUnitTest)
          .ExportSubkey(key.SubkeyFpr(), true);
  ASSERT_TRUE(err.Ok());
  ASSERT_FALSE(gf_buffer.Empty());
}

TEST_F(GpgCoreTest, CoreExportSubkeyProtectedTestB) {
  ProtectedTestKey key;
  ASSERT_FALSE(key.SubkeyFpr().isEmpty());

  const auto answer = key.AnswerWith(WrongPassphraseCb);
  auto [err, gf_buffer] =
      KeyImportExportOperation::GetInstance(kGpgChannelForUnitTest)
          .ExportSubkey(key.SubkeyFpr(), true);
  EXPECT_FALSE(err.Ok());
  EXPECT_TRUE(gf_buffer.Empty());
}

TEST_F(GpgCoreTest, CoreExportSubkeyProtectedTestC) {
  ProtectedTestKey key;
  ASSERT_FALSE(key.SubkeyFpr().isEmpty());

  const auto answer = key.AnswerWith(CancelingPassphraseCb);
  auto [err, gf_buffer] =
      KeyImportExportOperation::GetInstance(kGpgChannelForUnitTest)
          .ExportSubkey(key.SubkeyFpr(), true);
  EXPECT_TRUE(err.Canceled());
  EXPECT_TRUE(gf_buffer.Empty());
}

TEST_F(GpgCoreTest, CoreExportSecretKeyProtectedTestA) {
  ProtectedTestKey key;
  ASSERT_FALSE(key.Fpr().isEmpty());

  auto gpg_key = GpgKeyRepository::GetInstance(kGpgChannelForUnitTest)
                     .GetKeyPtr(key.Fpr(), false);
  ASSERT_TRUE(gpg_key != nullptr);

  const auto answer = key.AnswerWith(WrongPassphraseCb);
  auto [err, gf_buffer] =
      KeyImportExportOperation::GetInstance(kGpgChannelForUnitTest)
          .ExportKey(gpg_key, true, true, false);
  EXPECT_FALSE(err.Ok());
  EXPECT_TRUE(gf_buffer.Empty());
}

TEST_F(GpgCoreTest, CoreExportSubkeyTestA) {
  auto [err, gf_buffer] = KeyImportExportOperation::GetInstance().ExportSubkey(
      "F89C95A05088CC93", true);

  ASSERT_TRUE(err.Ok());
  ASSERT_FALSE(gf_buffer.Empty());
  ASSERT_EQ(
      QCryptographicHash::hash(
          gf_buffer.ConvertToQByteArray().replace("\r\n", "\n"),
          QCryptographicHash::Sha256)
          .toHex(),
      QByteArray(
          "6e3375060aa889d9eb61e2966eabb31eb6b5359a7742ee7adeedec09e6afa36a"));
}

TEST_F(GpgCoreTest, CoreExportSubkeyTestB) {
  auto [err, gf_buffer] = KeyImportExportOperation::GetInstance().ExportSubkey(
      "F89C95A05088CC93!", true);

  ASSERT_TRUE(err.Ok());
  ASSERT_FALSE(gf_buffer.Empty());
  ASSERT_EQ(
      QCryptographicHash::hash(
          gf_buffer.ConvertToQByteArray().replace("\r\n", "\n"),
          QCryptographicHash::Sha256)
          .toHex(),
      QByteArray(
          "6e3375060aa889d9eb61e2966eabb31eb6b5359a7742ee7adeedec09e6afa36a"));
}

TEST_F(GpgCoreTest, CoreExportSubkeyTestC) {
  auto [err, gf_buffer] = KeyImportExportOperation::GetInstance().ExportSubkey(
      "CFF986E51BBC2F46064C2136F89C95A05088CC93", true);

  ASSERT_TRUE(err.Ok());
  ASSERT_FALSE(gf_buffer.Empty());
  ASSERT_EQ(
      QCryptographicHash::hash(
          gf_buffer.ConvertToQByteArray().replace("\r\n", "\n"),
          QCryptographicHash::Sha256)
          .toHex(),
      QByteArray(
          "6e3375060aa889d9eb61e2966eabb31eb6b5359a7742ee7adeedec09e6afa36a"));
}

TEST_F(GpgCoreTest, CoreExportSubkeyTestD) {
  auto [err, gf_buffer] = KeyImportExportOperation::GetInstance().ExportSubkey(
      "CFF986E51BBC2F46064C2136F89C95A05088CC93!", true);

  ASSERT_TRUE(err.Ok());
  ASSERT_FALSE(gf_buffer.Empty());
  ASSERT_EQ(
      QCryptographicHash::hash(
          gf_buffer.ConvertToQByteArray().replace("\r\n", "\n"),
          QCryptographicHash::Sha256)
          .toHex(),
      QByteArray(
          "6e3375060aa889d9eb61e2966eabb31eb6b5359a7742ee7adeedec09e6afa36a"));
}

}  // namespace GpgFrontend::Test