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
#include "core/GpgConstants.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

namespace {

auto WrongPassphraseCb(void* /*opaque*/, const char* /*uid_hint*/,
                       const char* /*passphrase_info*/, int /*last_was_bad*/,
                       int fd) -> gpgme_error_t {
  const QByteArray line = "not-the-passphrase\n";
  qsizetype off = 0;
  while (off < line.size()) {
    const auto ret = gpgme_io_write(fd, line.constData() + off,
                                    static_cast<size_t>(line.size() - off));
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
 * @brief A throwaway key whose secret parts are protected by the test
 * context's passphrase; the imported test keys are all unprotected, so
 * exporting them never asks for one.
 */
class ProtectedTestKey {
 public:
  ProtectedTestKey() : ctx_(GpgContext::GetInstance()) {
    auto* g_ctx = ctx_.DefaultContext();
    const auto uid =
        QString("Export Skip Test <%1@example.org>")
            .arg(QUuid::createUuid().toString(QUuid::Id128).left(8));
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
    GpgKeyGetter::GetInstance().FlushKeyCache();
  }

  ProtectedTestKey(const ProtectedTestKey&) = delete;
  auto operator=(const ProtectedTestKey&) -> ProtectedTestKey& = delete;

  [[nodiscard]] auto SubkeyFpr() const -> const QString& { return subkey_fpr_; }

  /**
   * @brief Answer this thread's armored passphrase prompts with @p cb until
   * the returned guard goes, then with the test callback again.
   */
  [[nodiscard]] auto AnswerWith(gpgme_passphrase_cb_t cb) {
    auto* g_ctx = ctx_.DefaultContext();
    gpgme_passphrase_cb_t previous = nullptr;
    void* previous_value = nullptr;
    gpgme_get_passphrase_cb(g_ctx, &previous, &previous_value);
    gpgme_set_passphrase_cb(g_ctx, cb, previous_value);
    return qScopeGuard([g_ctx, previous, previous_value]() {
      gpgme_set_passphrase_cb(g_ctx, previous, previous_value);
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
// a status line, and gpgme still calls the export a success. These pin that
// such an export fails instead of returning a file without the key.

TEST_F(GpgCoreTest, CoreExportSubkeyProtectedTestA) {
  ProtectedTestKey key;
  ASSERT_FALSE(key.SubkeyFpr().isEmpty());

  // With the right passphrase, the subkey is exported: the baseline the two
  // tests below fail against.
  auto [err, gf_buffer] =
      GpgKeyImportExporter::GetInstance().ExportSubkey(key.SubkeyFpr(), true);
  ASSERT_EQ(gpg_err_code(err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(gf_buffer.Empty());
}

TEST_F(GpgCoreTest, CoreExportSubkeyProtectedTestB) {
  ProtectedTestKey key;
  ASSERT_FALSE(key.SubkeyFpr().isEmpty());

  const auto answer = key.AnswerWith(WrongPassphraseCb);
  auto [err, gf_buffer] =
      GpgKeyImportExporter::GetInstance().ExportSubkey(key.SubkeyFpr(), true);
  EXPECT_NE(gpg_err_code(err), GPG_ERR_NO_ERROR);
  EXPECT_TRUE(gf_buffer.Empty());
}

TEST_F(GpgCoreTest, CoreExportSubkeyProtectedTestC) {
  ProtectedTestKey key;
  ASSERT_FALSE(key.SubkeyFpr().isEmpty());

  const auto answer = key.AnswerWith(CancelingPassphraseCb);
  auto [err, gf_buffer] =
      GpgKeyImportExporter::GetInstance().ExportSubkey(key.SubkeyFpr(), true);
  EXPECT_EQ(gpg_err_code(err), GPG_ERR_CANCELED);
  EXPECT_TRUE(gf_buffer.Empty());
}

// Key Details imported whatever key a chosen revocation certificate named.
// The target is read first; only a single key revocation signature has one.
TEST(RevocationTargetTest, ReadsTheOneKeyRevocationSignature) {
  const QString rev =
      ":signature packet: algo 22, keyid A11849836F435C61\n"
      "\tversion 4, created 1794922902, md5len 0, sigclass 0x20\n"
      "\tdigest algo 10, begin of digest f3 67\n"
      "\thashed subpkt 33 len 21 (issuer fpr v4 "
      "714d1ae12d40c76faeae5e7ca11849836f435c61)\n"
      "\thashed subpkt 29 len 1 (revocation reason 0x00 ())\n"
      "\tsubpkt 16 len 8 (issuer key ID A11849836F435C61)\n";
  EXPECT_EQ(RevocationTargetFromPacketList(rev),
            "714D1AE12D40C76FAEAE5E7CA11849836F435C61");

  // Without the fingerprint subpacket, the key ID it was made by.
  QString no_fpr = rev;
  no_fpr.remove(QRegularExpression(".*issuer fpr.*\n"));
  EXPECT_EQ(RevocationTargetFromPacketList(no_fpr), "A11849836F435C61");

  // A certification, not a revocation.
  QString certification = rev;
  certification.replace("sigclass 0x20", "sigclass 0x13");
  EXPECT_TRUE(RevocationTargetFromPacketList(certification).isEmpty());

  // Anything beside the signature: a key, or a second signature.
  EXPECT_TRUE(
      RevocationTargetFromPacketList(":public key packet:\n" + rev).isEmpty());
  EXPECT_TRUE(RevocationTargetFromPacketList(rev + rev).isEmpty());
  EXPECT_TRUE(RevocationTargetFromPacketList({}).isEmpty());
}

TEST_F(GpgCoreTest, CoreRevocationTargetIsTheRevokedKey) {
  // gpg's own revocation certificate of a throwaway key, made for this test.
  const auto cert = GFBuffer(QString(
      "-----BEGIN PGP PUBLIC KEY BLOCK-----\n"
      "Comment: This is a revocation certificate\n"
      "\n"
      "iHgEIBYKACAWIQRxTRrhLUDHb66uXnyhGEmDb0NcYQUCavxZlgIdAAAKCRChGEmD\n"
      "b0NcYfNnAP93Ogf/KrbYzIxvMS7zg3EJsnnx9+cOwYrAwkV65Z6G8AEAtdARArJw\n"
      "itz2N1sNh+jdFh3QFoOW/kMZ3bheuPs8eQY=\n"
      "=AXNd\n"
      "-----END PGP PUBLIC KEY BLOCK-----\n"));
  EXPECT_EQ(GpgKeyImportExporter::GetInstance().RevocationCertificateTarget(
                cert),
            "714D1AE12D40C76FAEAE5E7CA11849836F435C61");
}

TEST_F(GpgCoreTest, CoreRevocationTargetOfSomethingElseIsEmpty) {
  auto& importer = GpgKeyImportExporter::GetInstance();
  EXPECT_TRUE(importer.RevocationCertificateTarget({}).isEmpty());
  EXPECT_TRUE(
      importer.RevocationCertificateTarget(GFBuffer(QString("not a cert")))
          .isEmpty());

  // A public key block revokes nothing, though gpg reads it fine.
  auto key = GpgKeyGetter::GetInstance().GetPubkeyPtr(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(key != nullptr);
  auto [err, block] = importer.ExportKey(key, false, true, false);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  EXPECT_TRUE(importer.RevocationCertificateTarget(block).isEmpty());
}

TEST_F(GpgCoreTest, CoreExportSubkeyTestA) {
  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance().ExportSubkey(
      "F89C95A05088CC93", true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
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
  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance().ExportSubkey(
      "F89C95A05088CC93!", true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
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
  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance().ExportSubkey(
      "CFF986E51BBC2F46064C2136F89C95A05088CC93", true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
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
  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance().ExportSubkey(
      "CFF986E51BBC2F46064C2136F89C95A05088CC93!", true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
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