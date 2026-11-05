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
#include "FileCryptoOpera.h"

#include <future>

#include "core/function/ArchiveFileOperator.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/gpg/GpgData.h"
#include "core/function/gpg/GpgErrorMap.h"
#include "core/function/gpg/GpgModelConvert.h"
#include "core/function/gpg/MessageCryptoOperation.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

namespace {

auto ExtractArchiveHelper(const QString& out_path,
                          std::function<void(GFOperaStatus)> on_done)
    -> QSharedPointer<GFDataExchanger> {
  auto ex = CreateStandardGFDataExchanger();
  ArchiveFileOperator::ExtractArchiveFromDataExchanger(
      ex, out_path,
      [on_done = std::move(on_done)](GFOperaStatus err, const DataObjectPtr&) {
        FLOG_D("extract archive from data exchanger operation, err: %d", err);
        if (on_done) on_done(err);
      });
  return ex;
}

/// Why @p a or @p b could not be set up, or GPG_ERR_NO_ERROR. Checked before
/// gpgme is called: a file that could not be opened is reported as that, not
/// as gpgme refusing to be handed no data.
auto DataError(const GpgData& a, const GpgData& b) -> GpgError {
  if (a.Error() != GPG_ERR_NO_ERROR) return a.Error();
  return b.Error();
}

// gpg succeeding says nothing about the archive: the tar inside may be
// malformed, or a write may have failed half way. Report that as a failure so
// the caller discards the staged output instead of committing a partial one.
// The same holds when gpg reads the archive rather than writes it: a file that
// could not be packed still ends in a clean EOF as far as gpg can tell.
auto JoinArchiveTask(GpgError gpg_err,
                     const std::shared_future<GFOperaStatus>& archive_future)
    -> GpgError {
  const auto archive_err = archive_future.get();
  if (CheckGpgError(gpg_err) != GPG_ERR_NO_ERROR) return gpg_err;
  if (archive_err != 0) {
    LOG_E() << "the archive half of the operation failed, err:" << archive_err;
    return GPG_ERR_INV_DATA;
  }
  return gpg_err;
}

void CreateArchiveHelper(const QString& in_path,
                         const QSharedPointer<GFDataExchanger>& ex,
                         std::function<void(GFOperaStatus)> on_done) {
  auto w_ex = QWeakPointer<GFDataExchanger>(ex);

  ArchiveFileOperator::NewArchive2DataExchanger(
      in_path, ex,
      [w_ex, on_done = std::move(on_done)](GFOperaStatus err,
                                           const DataObjectPtr&) {
        FLOG_D("new archive 2 data exchanger operation, err: %d", err);
        // `!= 0`, not `< 0`: GFOperaStatus is unsigned. The signed form was
        // always false, so a failed archive never closed the pipe and left
        // whoever was reading it waiting for bytes that were not coming.
        if (auto p_ex = w_ex.lock(); err != 0 && p_ex != nullptr) {
          p_ex->CloseWrite();
        }
        if (on_done) on_done(err);
      });
}

using ArchiveReadingOpera =
    std::function<GpgError(OpenPGPContext&, GpgData&, const DataObjectPtr&)>;

/// Encrypts an archive of @p in_path that is packed on a separate IO task
/// while gpg reads it, the mirror image of DecryptIntoArchive().
auto EncryptFromArchive(OpenPGPContext& ctx, const QString& in_path,
                        const GFOperationCallback& cb,
                        const ArchiveReadingOpera& opera, const QString& name,
                        const QList<EngineSupportIf>& support_ifs) -> GpgError {
  auto ex = CreateStandardGFDataExchanger();

  auto archive_done = std::make_shared<std::promise<GFOperaStatus>>();
  std::shared_future<GFOperaStatus> archive_future =
      archive_done->get_future().share();

  RunGpgOperaAsync(
      ctx.GetChannel(),
      [ctx_ptr = &ctx, ex, archive_future,
       opera](const DataObjectPtr& data_object) -> GFError {
        GpgError err;
        {
          GpgData data_in(ex);
          err = opera(*ctx_ptr, data_in, data_object);
        }  // data_in destroyed -> CloseWrite, so an archiver gpg stopped
           // reading early fails its next write instead of waiting forever
        return FromGpgError(JoinArchiveTask(err, archive_future));
      },
      // A task that never ran (channel retired, engine unsupported, cancelled
      // while queued) never opened the pipe either. Closing it here is what
      // keeps the archiver from filling it and then blocking the IO runner.
      [ex, cb](const GFError& err, const DataObjectPtr& data_object) {
        ex->CloseWrite();
        if (cb) cb(err, data_object);
      },
      name, support_ifs);

  CreateArchiveHelper(in_path, ex, [archive_done](GFOperaStatus err) {
    archive_done->set_value(err);
  });
  return GPG_ERR_NO_ERROR;  // The actual result will be delivered via callback
}
}  // namespace

auto EncryptFileGpgDataImpl(OpenPGPContext& ctx_,
                            const GpgAbstractKeyPtrList& keys, GpgData& data_in,
                            bool ascii, GpgData& data_out,
                            const DataObjectPtr& data_object) -> GpgError {
  if (const auto data_err = DataError(data_in, data_out);
      data_err != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgEncryptResult(GFEncryptResult{})});
    return data_err;
  }

  auto& g_ctx = GpgCtx(ctx_);
  // Converting leaves disabled and missing keys out without a word, and the
  // file would be encrypted without them.
  if (HasUnusableKey(ctx_.GetChannel(), keys)) {
    data_object->Swap({GpgEncryptResult(GFEncryptResult{})});
    return GPG_ERR_UNUSABLE_PUBKEY;
  }
  auto recipients = Convert2GpgKeyList(ctx_.GetChannel(), keys);

  QList<gpgme_key_t> gpg_recipients;
  for (const auto& key : recipients) {
    LOG_D() << "recipient key fpr: " << key.Fingerprint();
    gpg_recipients.push_back(GpgmeKeyOf(key));
  }
  gpg_recipients.push_back(nullptr);

  auto* ctx = ascii ? g_ctx.DefaultContext() : g_ctx.BinaryContext();

  auto err = CheckGpgError(
      gpgme_op_encrypt(ctx, keys.isEmpty() ? nullptr : gpg_recipients.data(),
                       GPGME_ENCRYPT_ALWAYS_TRUST, data_in, data_out));
  data_object->Swap({EncryptResultOf(gpgme_op_encrypt_result(ctx))});
  return err;
}

auto EncryptFileGnuPGImpl(OpenPGPContext& ctx_,
                          const GpgAbstractKeyPtrList& keys,
                          const QString& in_path, bool ascii,
                          const QString& out_path,
                          const DataObjectPtr& data_object) -> GpgError {
  GpgData data_in(in_path, true);
  GpgData data_out(out_path, false);

  return EncryptFileGpgDataImpl(ctx_, keys, data_in, ascii, data_out,
                                data_object);
}

auto EncryptDirGnuPGImpl(OpenPGPContext& ctx, const GpgAbstractKeyPtrList& keys,
                         const QString& in_path, bool ascii,
                         const QString& out_path, const GFOperationCallback& cb)
    -> GpgError {
  return EncryptFromArchive(
      ctx, in_path, cb,
      [keys, ascii, out_path](OpenPGPContext& c, GpgData& data_in,
                              const DataObjectPtr& data_object) -> GpgError {
        GpgData data_out(out_path, false);
        return EncryptFileGpgDataImpl(c, keys, data_in, ascii, data_out,
                                      data_object);
      },
      "gpgme_op_encrypt", {{OpenPGPEngine::kGNUPG, "2.2.0"}});
}

auto EncryptSymmetricFileGnuPGImpl(OpenPGPContext& ctx_, const QString& in_path,
                                   bool ascii, const QString& out_path,
                                   const DataObjectPtr& data_object)
    -> GpgError {
  return EncryptFileGnuPGImpl(ctx_, {}, in_path, ascii, out_path, data_object);
}

auto EncryptSymmetricDirGnuPGImpl(OpenPGPContext& ctx, const QString& in_path,
                                  bool ascii, const QString& out_path,
                                  const GFOperationCallback& cb) -> GpgError {
  return EncryptFromArchive(
      ctx, in_path, cb,
      [ascii, out_path](OpenPGPContext& c, GpgData& data_in,
                        const DataObjectPtr& data_object) -> GpgError {
        GpgData data_out(out_path, false);
        return EncryptFileGpgDataImpl(c, {}, data_in, ascii, data_out,
                                      data_object);
      },
      "gpgme_op_encrypt_symmetric",
      {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}});
}

namespace {
auto DecryptVerifyFileGpgDataImpl(OpenPGPContext& ctx_, GpgData& data_in,
                                  GpgData& data_out,
                                  const DataObjectPtr& data_object)
    -> GpgError {
  if (const auto data_err = DataError(data_in, data_out);
      data_err != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgDecryptResult(GFDecryptResult{}),
                       GpgVerifyResult(GFVerifyResult{})});
    return data_err;
  }

  auto& g_ctx = GpgCtx(ctx_);
  auto err = CheckGpgError(
      gpgme_op_decrypt_verify(g_ctx.DefaultContext(), data_in, data_out));

  data_object->Swap({
      DecryptResultOf(gpgme_op_decrypt_result(g_ctx.DefaultContext())),
      VerifyResultOf(gpgme_op_verify_result(g_ctx.DefaultContext())),
  });
  return err;
}
}  // namespace

auto DecryptVerifyFileGnuPGImpl(OpenPGPContext& ctx_, const QString& in_path,
                                const QString& out_path,
                                const DataObjectPtr& data_object) -> GpgError {
  GpgData data_in(in_path, true);
  GpgData data_out(out_path, false);

  return DecryptVerifyFileGpgDataImpl(ctx_, data_in, data_out, data_object);
}

namespace {
using GpgDataOpera = GpgError (*)(OpenPGPContext&, GpgData&, GpgData&,
                                  const DataObjectPtr&);

/// Decrypts into an archive extractor running on a separate IO task, which
/// reads the plaintext from the exchanger and writes the files to disk.
auto DecryptIntoArchive(OpenPGPContext& ctx_, const QString& in_path,
                        const QString& out_path, const GFOperationCallback& cb,
                        GpgDataOpera opera, const QString& name) -> GpgError {
  // gpgme finishing only means all bytes have been pushed into the queue; the
  // extractor may still be draining it and writing the trailing files. Join on
  // the extractor so the opera does not report completion (and the caller does
  // not commit / rename the output directory) until every file is on disk.
  auto extract_done = std::make_shared<std::promise<GFOperaStatus>>();
  std::shared_future<GFOperaStatus> extract_future =
      extract_done->get_future().share();

  auto ex = ExtractArchiveHelper(out_path, [extract_done](GFOperaStatus err) {
    extract_done->set_value(err);
  });

  RunGpgOperaAsync(
      ctx_.GetChannel(),
      [ctx_ptr = &ctx_, in_path, ex, extract_future,
       opera](const DataObjectPtr& data_object) -> GFError {
        GpgError err;
        {
          GpgData data_in(in_path, true);
          GpgData data_out(ex);
          err = opera(*ctx_ptr, data_in, data_out, data_object);
        }  // data_out destroyed -> CloseWrite, lets extractor reach EOF
        return FromGpgError(JoinArchiveTask(err, extract_future));
      },
      cb, name, {{OpenPGPEngine::kGNUPG, "2.2.0"}});
  return GPG_ERR_NO_ERROR;  // The actual result will be delivered via callback
}
}  // namespace

auto DecryptVerifyArchiveGnuPGImpl(OpenPGPContext& ctx_, const QString& in_path,
                                   const QString& out_path,
                                   const GFOperationCallback& cb) -> GpgError {
  return DecryptIntoArchive(ctx_, in_path, out_path, cb,
                            DecryptVerifyFileGpgDataImpl,
                            "gpgme_op_decrypt_verify");
}

namespace {
auto DecryptFileGpgDataImpl(OpenPGPContext& ctx_, GpgData& data_in,
                            GpgData& data_out, const DataObjectPtr& data_object)
    -> GpgError {
  if (const auto data_err = DataError(data_in, data_out);
      data_err != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgDecryptResult(GFDecryptResult{})});
    return data_err;
  }

  auto& g_ctx = GpgCtx(ctx_);
  auto err = CheckGpgError(
      gpgme_op_decrypt(g_ctx.DefaultContext(), data_in, data_out));
  data_object->Swap(
      {DecryptResultOf(gpgme_op_decrypt_result(g_ctx.DefaultContext()))});

  return err;
}
}  // namespace

auto DecryptFileGnuPGImpl(OpenPGPContext& ctx_, const QString& in_path,
                          const QString& out_path,
                          const DataObjectPtr& data_object) -> GpgError {
  GpgData data_in(in_path, true);
  GpgData data_out(out_path, false);

  return DecryptFileGpgDataImpl(ctx_, data_in, data_out, data_object);
}

auto DecryptArchiveGnuPGImpl(OpenPGPContext& ctx_, const QString& in_path,
                             const QString& out_path,
                             const GFOperationCallback& cb) -> GpgError {
  return DecryptIntoArchive(ctx_, in_path, out_path, cb, DecryptFileGpgDataImpl,
                            "gpgme_op_decrypt");
}

auto SignFileGpgDataGnuPGImpl(OpenPGPContext& ctx_,
                              const GpgAbstractKeyPtrList& keys,
                              GpgData& data_in, bool ascii, GpgData& data_out,
                              const DataObjectPtr& data_object) -> GpgError {
  GpgError err;

  GpgContext& g_ctx = GpgCtx(ctx_);

  if (const auto data_err = DataError(data_in, data_out);
      data_err != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgSignResult(GFSignResult{})});
    return data_err;
  }

  // Set Singers of this opera
  err = SetSignersGnuPGImpl(ctx_, keys, ascii);
  if (err != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgSignResult(GFSignResult{})});
    return err;
  }

  auto* ctx = ascii ? g_ctx.DefaultContext() : g_ctx.BinaryContext();
  err = CheckGpgError(
      gpgme_op_sign(ctx, data_in, data_out, GPGME_SIG_MODE_DETACH));

  data_object->Swap({
      SignResultOf(gpgme_op_sign_result(ctx)),
  });
  return err;
}

auto SignFileGnuPGImpl(OpenPGPContext& ctx_, const GpgAbstractKeyPtrList& keys,
                       const QString& in_path, bool ascii,
                       const QString& out_path,
                       const DataObjectPtr& data_object) -> GpgError {
  GpgData data_in(in_path, true);
  GpgData data_out(out_path, false);

  return SignFileGpgDataGnuPGImpl(ctx_, keys, data_in, ascii, data_out,
                                  data_object);
}

auto VerifyFileGnuPGImpl(OpenPGPContext& ctx_, const QString& data_path,
                         const QString& sign_path,
                         const DataObjectPtr& data_object) -> GpgError {
  GpgError err;

  auto& g_ctx = GpgCtx(ctx_);
  GpgData data_in(data_path, true);
  GpgData data_out;
  if (const auto data_err = DataError(data_in, data_out);
      data_err != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgVerifyResult(GFVerifyResult{})});
    return data_err;
  }
  // A combined file: gpg verifies the armored block in it and skips anything
  // around it, as for text (see MessageCryptoOperation's verify).
  if (sign_path.isEmpty() && HasTextOutsideSignedArmorInFile(data_path)) {
    LOG_W() << "refusing to verify: the file has text outside its signature";
    data_object->Swap({GpgVerifyResult(GFVerifyResult{})});
    return GPG_ERR_BAD_DATA;
  }
  if (!sign_path.isEmpty()) {
    GpgData sig_data(sign_path, true);
    if (sig_data.Error() != GPG_ERR_NO_ERROR) {
      data_object->Swap({GpgVerifyResult(GFVerifyResult{})});
      return sig_data.Error();
    }
    err = CheckGpgError(
        gpgme_op_verify(g_ctx.DefaultContext(), sig_data, data_in, nullptr));
  } else {
    err = CheckGpgError(
        gpgme_op_verify(g_ctx.DefaultContext(), data_in, nullptr, data_out));
  }

  data_object->Swap({
      VerifyResultOf(gpgme_op_verify_result(g_ctx.DefaultContext())),
  });

  return err;
}

namespace {
auto EncryptSignFileGpgDataGnuPGImpl(
    OpenPGPContext& ctx_, const GpgAbstractKeyPtrList& keys,
    const GpgAbstractKeyPtrList& signer_keys, GpgData& data_in, bool ascii,
    GpgData& data_out, const DataObjectPtr& data_object) -> GpgError {
  GpgError err;
  auto& g_ctx = GpgCtx(ctx_);
  if (const auto data_err = DataError(data_in, data_out);
      data_err != GPG_ERR_NO_ERROR) {
    data_object->Swap(
        {GpgEncryptResult(GFEncryptResult{}), GpgSignResult(GFSignResult{})});
    return data_err;
  }
  if (HasUnusableKey(ctx_.GetChannel(), keys)) {
    data_object->Swap(
        {GpgEncryptResult(GFEncryptResult{}), GpgSignResult(GFSignResult{})});
    return GPG_ERR_UNUSABLE_PUBKEY;
  }
  auto recipients = Convert2GpgKeyList(ctx_.GetChannel(), keys);

  QList<gpgme_key_t> gpg_recipients;
  for (const auto& key : recipients) {
    LOG_D() << "recipient key fpr: " << key.Fingerprint();
    gpg_recipients.push_back(GpgmeKeyOf(key));
  }
  gpg_recipients.push_back(nullptr);

  err = SetSignersGnuPGImpl(ctx_, signer_keys, ascii);
  if (err != GPG_ERR_NO_ERROR) {
    data_object->Swap(
        {GpgEncryptResult(GFEncryptResult{}), GpgSignResult(GFSignResult{})});
    return err;
  }

  auto* ctx = ascii ? g_ctx.DefaultContext() : g_ctx.BinaryContext();
  err = CheckGpgError(gpgme_op_encrypt_sign(ctx, gpg_recipients.data(),
                                            GPGME_ENCRYPT_ALWAYS_TRUST, data_in,
                                            data_out));

  data_object->Swap({
      EncryptResultOf(gpgme_op_encrypt_result(ctx)),
      SignResultOf(gpgme_op_sign_result(ctx)),
  });

  return err;
}
}  // namespace

auto EncryptSignFileGnuPGImpl(OpenPGPContext& ctx_,
                              const GpgAbstractKeyPtrList& keys,
                              const GpgAbstractKeyPtrList& signer_keys,
                              const QString& in_path, bool ascii,
                              const QString& out_path,
                              const DataObjectPtr& data_object) -> GpgError {
  GpgData data_in(in_path, true);
  GpgData data_out(out_path, false);

  return EncryptSignFileGpgDataGnuPGImpl(ctx_, keys, signer_keys, data_in,
                                         ascii, data_out, data_object);
}

auto EncryptSignDirGnuPGImpl(OpenPGPContext& ctx,
                             const GpgAbstractKeyPtrList& keys,
                             const GpgAbstractKeyPtrList& signer_keys,
                             const QString& in_path, bool ascii,
                             const QString& out_path,
                             const GFOperationCallback& cb) -> GpgError {
  return EncryptFromArchive(
      ctx, in_path, cb,
      [keys, signer_keys, ascii, out_path](
          OpenPGPContext& c, GpgData& data_in,
          const DataObjectPtr& data_object) -> GpgError {
        GpgData data_out(out_path, false);
        return EncryptSignFileGpgDataGnuPGImpl(c, keys, signer_keys, data_in,
                                               ascii, data_out, data_object);
      },
      "gpgme_op_encrypt_sign_dir", {{OpenPGPEngine::kGNUPG, "2.2.0"}});
}

}  // namespace GpgFrontend