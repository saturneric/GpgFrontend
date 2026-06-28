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

#include "GpgOperaHelper.h"

#include "core/function/GFBufferFactory.h"
#include "core/function/openpgp/FileCryptoOperation.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgSignResult.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/dialog/WaitingDialog.h"

namespace {

using HashCallback = std::function<void(const QString&)>;

// Crypto operations that complete quickly should not flash a waiting dialog.
// Defer presenting the dialog until the operation has been running this long.
constexpr int kWaitingDialogShowDelayMs = 1000;

// When the dialog is due but another application currently holds focus, poll at
// this interval until focus returns before presenting it (see below).
constexpr int kWaitingDialogFocusRecheckMs = 250;

// True when a modal dialog other than `dialog` (and other than the context the
// waiting dialog is meant to cover) is currently on top. The waiting dialog's
// parent window, and any ancestor of it, are the expected modal context the
// progress window should appear over; a *different* modal dialog is something
// the operation popped up mid-flight that needs the user's input — the
// passphrase prompt, or a module's own input dialog (e.g. the EML module asking
// for sender/recipient/subject/cc/bcc). Presenting the (also modal) waiting
// window now would stack above it and block its input.
auto ForeignModalDialogIsActive(GpgFrontend::UI::WaitingDialog* dialog) -> bool {
  auto* active_modal = QApplication::activeModalWidget();
  if (active_modal == nullptr || active_modal == dialog) return false;

  auto* parent = dialog->parentWidget();
  if (parent == nullptr) return true;

  return active_modal != parent->window() &&
         !active_modal->isAncestorOf(parent);
}

// Create a single-shot timer (owned by the dialog) that shows the dialog only
// after kWaitingDialogShowDelayMs has elapsed. If the operation finishes first,
// the caller stops the timer and the dialog is never shown.
//
// Two conditions defer the show (re-checked shortly after):
//   1. Another application holds focus: mapping the modal waiting window then
//      makes the window manager switch focus to GpgFrontend, interrupting
//      passphrase entry in GnuPG's external pinentry (a separate process).
//      QApplication::activeWindow() is null exactly when no GpgFrontend window
//      is active, i.e. another app (pinentry) owns focus.
//   2. A foreign modal dialog is up (passphrase prompt, a module's input dialog
//      such as the EML header editor): the waiting window would stack above it
//      and block its input. See ForeignModalDialogIsActive().
auto StartDeferredShowTimer(GpgFrontend::UI::WaitingDialog* dialog) -> QTimer* {
  auto* timer = new QTimer(dialog);
  timer->setSingleShot(true);
  QObject::connect(timer, &QTimer::timeout, dialog, [dialog, timer]() {
    if (QApplication::activeWindow() == nullptr ||
        ForeignModalDialogIsActive(dialog)) {
      timer->start(kWaitingDialogFocusRecheckMs);
      return;
    }
    dialog->show();
  });
  timer->start(kWaitingDialogShowDelayMs);
  return timer;
}

void StartFileHashComputation(const QString& path, HashCallback callback) {
  auto hash_holder = GpgFrontend::SecureCreateSharedObject<QString>();
  GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(GpgFrontend::Thread::TaskRunnerGetter::kTaskRunnerType_IO)
      ->PostTask(
          QStringLiteral("ComputeFileSha256"),
          [path, hash_holder](const GpgFrontend::DataObjectPtr&) -> int {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) return 0;

            constexpr auto kBufferSize = static_cast<const qint64>(64 * 1024);
            const auto result = GpgFrontend::GFBufferFactory::ToSha256(
                [&file](const GpgFrontend::GFBufferFactory::Sha256Chunk& update)
                    -> void {
                  while (!file.atEnd()) {
                    const QByteArray chunk = file.read(kBufferSize);
                    if (chunk.isEmpty()) break;
                    update(chunk.constData(),
                           static_cast<size_t>(chunk.size()));
                  }
                });

            if (result) *hash_holder = result->ConvertToQByteArray().toHex();
            return 0;
          },
          [hash_holder, callback = std::move(callback)](
              int, const GpgFrontend::DataObjectPtr&) {
            if (callback) callback(*hash_holder);
          },
          nullptr);
}

}  // namespace

namespace GpgFrontend::UI {

void GpgOperaHelper::BuildOperas(QSharedPointer<GpgOperaContextBasement>& base,
                                 int category, int channel,
                                 const GpgOperaFactory& f) {
  assert(base != nullptr);

  auto context = GetGpgOperaContextFromBasement(base, category);
  if (context == nullptr) return;

  if (!context->paths.isEmpty()) {
    assert(context->paths.size() == context->o_paths.size());

    for (int i = 0; i < context->paths.size(); i++) {
      context->base->operas.push_back(f(context, channel, i));
    }
  }

  if (!context->buffers.isEmpty()) {
    for (int i = 0; i < context->buffers.size(); i++) {
      context->base->operas.push_back(f(context, channel, i));
    }
  }
}

template <typename AnalyseType>
void HandleExtraLogicIfNeeded(const QSharedPointer<GpgOperaContext>&,
                              const AnalyseType&, GpgOpResultInfo&) {}

template <>
void HandleExtraLogicIfNeeded<GpgVerifyResultAnalyse>(
    const QSharedPointer<GpgOperaContext>& context,
    const GpgVerifyResultAnalyse& analyse, GpgOpResultInfo&) {
  context->base->unknown_fprs.append(analyse.GetUnknownSignatures());
}

template <>
void HandleExtraLogicIfNeeded<GpgEncryptResultAnalyse>(
    const QSharedPointer<GpgOperaContext>& context,
    const GpgEncryptResultAnalyse& analyse, GpgOpResultInfo& info) {
  if (context->base->keys.isEmpty()) return;

  // Resolve an encryption (sub)key ID back to a recipient UID and the matching
  // (sub)key fingerprint among the selected recipient keys.
  auto resolve = [&](const QString& key_id, QString& uid, QString& fpr) {
    for (const auto& key : context->base->keys) {
      auto gpg_key = qSharedPointerDynamicCast<GpgKey>(key);
      if (gpg_key == nullptr) continue;
      for (const auto& sub : gpg_key->SubKeys()) {
        if (sub.ID().compare(key_id, Qt::CaseInsensitive) == 0) {
          uid = gpg_key->UID();
          fpr = sub.Fingerprint();
          return;
        }
      }
      if (gpg_key->ID().compare(key_id, Qt::CaseInsensitive) == 0) {
        uid = gpg_key->UID();
        fpr = gpg_key->Fingerprint();
        return;
      }
    }
  };

  // The rPGP engine reports the subkeys it actually encrypted to; show them
  // verbatim so the key ID and algorithm match the produced ciphertext.
  const auto engine_recipients = analyse.GetResult().Recipients();
  if (!engine_recipients.isEmpty()) {
    for (const auto& er : engine_recipients) {
      GpgRecipientInfo ri;
      ri.keyId = er.keyid;
      ri.pubkeyAlgo = er.pubkey_algo;
      ri.keyFound = true;
      resolve(er.keyid, ri.uid, ri.fingerprint);
      info.recipients.append(ri);
    }
    return;
  }

  // The GnuPG engine does not report which subkey was used, so we cannot show
  // the real encryption-subkey algorithm. Fall back to the primary key and mark
  // the algorithm so the UI does not present it as the subkey actually used.
  for (const auto& key : context->base->keys) {
    if (key == nullptr) continue;
    GpgRecipientInfo ri;
    ri.uid = key->UID();
    ri.fingerprint = key->Fingerprint();
    ri.keyId = key->ID();
    ri.pubkeyAlgo = key->PublicKeyAlgo();
    ri.keyFound = true;
    ri.algoIsPrimaryKey = qSharedPointerDynamicCast<GpgKey>(key) != nullptr;
    info.recipients.append(ri);
  }
}

template <typename ResultType, typename AnalyseType, typename OperaFunc>
auto GpgOperaHelper::BuildSimpleGpgFileOperasHelper(
    QSharedPointer<GpgOperaContext>& context, int channel, int index,
    OperaFunc opera_func) -> OperaWaitingCb {
  const auto& path = context->paths[index];
  const auto& o_path = context->o_paths[index];
  auto& opera_results = context->base->opera_results;

  auto input_hash = SecureCreateSharedObject<QString>();

  // Start hash computation with callback
  StartFileHashComputation(
      path, [input_hash](const QString& hash) { *input_hash = hash; });

  return [=, &opera_results](const OperaWaitingHd& op_hd) {
    opera_func(
        path, o_path,
        [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
          op_hd();

          if (CheckGpgError(err) == GPG_ERR_NOT_SUPPORTED) {
            GpgOpResultInfo info;
            info.status = -1;
            info.report = "# " + tr("Operation Not Supported");
            opera_results.append(
                GpgOperaResult{std::move(info), QFileInfo(path).fileName()});
            return;
          }

          if (CheckGpgError(err) == GPG_ERR_CANCELED) {
            GpgOpResultInfo info;
            info.status = -1;
            info.report = "# " + tr("Operation Cancelled");
            opera_results.append(
                GpgOperaResult{std::move(info), QFileInfo(path).fileName()});
            return;
          }

          if (CheckGpgError(err) == GPG_ERR_USER_1 || data_obj == nullptr ||
              !data_obj->Check<ResultType>()) {
            GpgOpResultInfo info;
            info.status = -1;
            info.report = "# " + tr("Critical Error");
            opera_results.append(
                GpgOperaResult{std::move(info), QFileInfo(path).fileName()});
            return;
          }

          auto result = ExtractParams<ResultType>(data_obj, 0);
          auto result_analyse = AnalyseType(channel, err, result);
          result_analyse.Analyse();

          auto opera_result = GpgOperaResult{
              result_analyse.GetOpInfo(),
              QFileInfo(path.isEmpty() ? o_path : path).fileName()};
          opera_result.op_info.inputHash = *input_hash;
          HandleExtraLogicIfNeeded<AnalyseType>(context, result_analyse,
                                                opera_result.op_info);

          opera_results.append(opera_result);
        });
  };
}

template <typename ResultTypeA, typename AnalyseTypeA, typename ResultTypeB,
          typename AnalyseTypeB, typename OperaFunc>
auto GpgOperaHelper::BuildComplexGpgFileOperasHelper(
    QSharedPointer<GpgOperaContext>& context, int channel, int index,
    OperaFunc opera_func) -> OperaWaitingCb {
  const auto& path = context->paths[index];
  const auto& o_path = context->o_paths[index];
  auto& opera_results = context->base->opera_results;

  auto input_hash = SecureCreateSharedObject<QString>();

  // Start hash computation with callback
  StartFileHashComputation(
      path, [input_hash](const QString& hash) { *input_hash = hash; });

  return [=, &opera_results](const OperaWaitingHd& op_hd) {
    opera_func(
        path, o_path,
        [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
          op_hd();

          if (CheckGpgError(err) == GPG_ERR_NOT_SUPPORTED) {
            GpgOpResultInfo info;
            info.status = -1;
            info.report = "# " + tr("Operation Not Supported");
            opera_results.append(
                GpgOperaResult{std::move(info), QFileInfo(path).fileName()});
            return;
          }

          if (CheckGpgError(err) == GPG_ERR_CANCELED) {
            GpgOpResultInfo info;
            info.status = -1;
            info.report = "# " + tr("Operation Cancelled");
            opera_results.append(
                GpgOperaResult{std::move(info), QFileInfo(path).fileName()});
            return;
          }

          if (CheckGpgError(err) == GPG_ERR_USER_1 || data_obj == nullptr ||
              !data_obj->Check<ResultTypeA, ResultTypeB>()) {
            GpgOpResultInfo info;
            info.status = -1;
            info.report = "# " + tr("Critical Error");
            opera_results.append(
                GpgOperaResult{std::move(info), QFileInfo(path).fileName()});
            return;
          }

          auto result_1 = ExtractParams<ResultTypeA>(data_obj, 0);
          auto result_2 = ExtractParams<ResultTypeB>(data_obj, 1);

          auto result_analyse_1 = AnalyseTypeA(channel, err, result_1);
          result_analyse_1.Analyse();

          auto result_analyse_2 = AnalyseTypeB(channel, err, result_2);
          result_analyse_2.Analyse();

          auto info = result_analyse_1.GetOpInfo();
          HandleExtraLogicIfNeeded<AnalyseTypeA>(context, result_analyse_1,
                                                 info);
          info.Merge(result_analyse_2.GetOpInfo());
          HandleExtraLogicIfNeeded<AnalyseTypeB>(context, result_analyse_2,
                                                 info);
          info.inputHash = *input_hash;

          opera_results.append(GpgOperaResult{
              std::move(info),
              QFileInfo(path.isEmpty() ? o_path : path).fileName()});
        });
  };
}

template <typename ResultType, typename AnalyseType, typename OperaFunc>
auto GpgOperaHelper::BuildSimpleGpgOperasHelper(
    QSharedPointer<GpgOperaContext>& context, int channel, int index,
    OperaFunc opera_func) -> OperaWaitingCb {
  const auto& buffer = context->buffers[index];
  auto& opera_results = context->base->opera_results;

  const auto hash_result = GFBufferFactory::ToSha256(buffer);
  const QString input_hash =
      hash_result ? hash_result->ConvertToQByteArray().toHex() : QString();

  return [=, &opera_results](const OperaWaitingHd& op_hd) {
    opera_func(buffer, [=, &opera_results](GpgError err,
                                           const DataObjectPtr& data_obj) {
      // stop waiting
      op_hd();

      if (CheckGpgError(err) == GPG_ERR_NOT_SUPPORTED) {
        GpgOpResultInfo info;
        info.status = -1;
        info.report = "# " + tr("Operation Not Supported");
        opera_results.append(GpgOperaResult{std::move(info)});
        return;
      }

      if (CheckGpgError(err) == GPG_ERR_CANCELED) {
        GpgOpResultInfo info;
        info.status = -1;
        info.report = "# " + tr("Operation Cancelled");
        opera_results.append(GpgOperaResult{std::move(info)});
        return;
      }

      if (CheckGpgError(err) == GPG_ERR_USER_1 || data_obj == nullptr ||
          !data_obj->Check<ResultType, GFBuffer>()) {
        GpgOpResultInfo info;
        info.status = -1;
        info.report = "# " + tr("Critical Error");
        opera_results.append(GpgOperaResult{std::move(info)});
        return;
      }

      auto result = ExtractParams<ResultType>(data_obj, 0);

      auto result_analyse = AnalyseType(channel, err, result);
      result_analyse.Analyse();

      auto opera_result = GpgOperaResult{result_analyse.GetOpInfo()};
      opera_result.op_info.inputHash = input_hash;
      HandleExtraLogicIfNeeded<AnalyseType>(context, result_analyse,
                                            opera_result.op_info);

      auto o_buffer = ExtractParams<GFBuffer>(data_obj, 1);
      opera_result.o_buffer = o_buffer;

      opera_results.append(opera_result);
    });
  };
}

template <typename ResultTypeA, typename AnalyseTypeA, typename ResultTypeB,
          typename AnalyseTypeB, typename OperaFunc>
auto GpgOperaHelper::BuildComplexGpgOperasHelper(
    QSharedPointer<GpgOperaContext>& context, int channel, int index,
    OperaFunc opera_func) -> OperaWaitingCb {
  const auto& buffer = context->buffers[index];
  auto& opera_results = context->base->opera_results;

  const auto hash_result = GFBufferFactory::ToSha256(buffer);
  const QString input_hash =
      hash_result ? hash_result->ConvertToQByteArray().toHex() : QString();

  return [=, &opera_results](const OperaWaitingHd& op_hd) {
    opera_func(buffer, [=, &opera_results](GpgError err,
                                           const DataObjectPtr& data_obj) {
      // stop waiting
      op_hd();

      if (CheckGpgError(err) == GPG_ERR_NOT_SUPPORTED) {
        GpgOpResultInfo info;
        info.status = -1;
        info.report = "# " + tr("Operation Not Supported");
        opera_results.append(GpgOperaResult{std::move(info)});
        return;
      }

      if (CheckGpgError(err) == GPG_ERR_CANCELED) {
        GpgOpResultInfo info;
        info.status = -1;
        info.report = "# " + tr("Operation Cancelled");
        opera_results.append(GpgOperaResult{std::move(info)});
        return;
      }

      if (CheckGpgError(err) == GPG_ERR_USER_1 || data_obj == nullptr ||
          !data_obj->Check<ResultTypeA, ResultTypeB, GFBuffer>()) {
        GpgOpResultInfo info;
        info.status = -1;
        info.report = "# " + tr("Critical Error");
        opera_results.append(GpgOperaResult{std::move(info)});
        return;
      }

      auto result_1 = ExtractParams<ResultTypeA>(data_obj, 0);
      auto result_2 = ExtractParams<ResultTypeB>(data_obj, 1);

      auto result_analyse_1 = AnalyseTypeA(channel, err, result_1);
      result_analyse_1.Analyse();

      auto result_analyse_2 = AnalyseTypeB(channel, err, result_2);
      result_analyse_2.Analyse();

      auto info = result_analyse_1.GetOpInfo();
      HandleExtraLogicIfNeeded<AnalyseTypeA>(context, result_analyse_1, info);
      info.Merge(result_analyse_2.GetOpInfo());
      HandleExtraLogicIfNeeded<AnalyseTypeB>(context, result_analyse_2, info);
      info.inputHash = input_hash;

      auto opera_result = GpgOperaResult{std::move(info)};

      auto o_buffer = ExtractParams<GFBuffer>(data_obj, 2);
      opera_result.o_buffer = o_buffer;

      opera_results.append(opera_result);
    });
  };
}

template <typename EncryptFuncSymmetric, typename EncryptFuncKeys>
auto BuildOperasFileEncryptHelper(QSharedPointer<GpgOperaContext>& context,
                                  int channel, int index,
                                  EncryptFuncSymmetric encrypt_symmetric,
                                  EncryptFuncKeys encrypt_with_keys)
    -> OperaWaitingCb {
  if (context->base->keys.isEmpty()) {
    return GpgOperaHelper::BuildSimpleGpgFileOperasHelper<
        GpgEncryptResult, GpgEncryptResultAnalyse>(
        context, channel, index,
        [=](const QString& path, const QString& o_path, const auto& callback) {
          encrypt_symmetric(path, o_path, callback);
        });
  }

  return GpgOperaHelper::BuildSimpleGpgFileOperasHelper<
      GpgEncryptResult, GpgEncryptResultAnalyse>(
      context, channel, index,
      [=](const QString& path, const QString& o_path, const auto& callback) {
        encrypt_with_keys(path, o_path, callback);
      });
}

template <typename EncryptFuncSymmetric, typename EncryptFuncKeys>
auto BuildOperasEncryptHelper(QSharedPointer<GpgOperaContext>& context,
                              int channel, int index,
                              EncryptFuncSymmetric encrypt_symmetric,
                              EncryptFuncKeys encrypt_with_keys)
    -> OperaWaitingCb {
  if (context->base->keys.isEmpty()) {
    return GpgOperaHelper::BuildSimpleGpgOperasHelper<GpgEncryptResult,
                                                      GpgEncryptResultAnalyse>(
        context, channel, index,
        [=](const GFBuffer& buffer, const auto& callback) {
          encrypt_symmetric(buffer, callback);
        });
  }

  return GpgOperaHelper::BuildSimpleGpgOperasHelper<GpgEncryptResult,
                                                    GpgEncryptResultAnalyse>(
      context, channel, index,
      [=](const GFBuffer& buffer, const auto& callback) {
        encrypt_with_keys(buffer, callback);
      });
}

auto GpgOperaHelper::BuildOperasFileEncrypt(
    QSharedPointer<GpgOperaContext>& context, int channel, int index)
    -> OperaWaitingCb {
  return BuildOperasFileEncryptHelper(
      context, channel, index,
      [context, channel](const QString& path, const QString& o_path,
                         const auto& callback) {
        FileCryptoOperation::GetInstance(channel).EncryptFileSymmetric(
            path, context->base->ascii, o_path, callback);
      },
      [context, channel](const QString& path, const QString& o_path,
                         const auto& callback) {
        FileCryptoOperation::GetInstance(channel).EncryptFile(
            context->base->keys, path, context->base->ascii, o_path, callback);
      });
}

auto GpgOperaHelper::BuildOperasDirectoryEncrypt(
    QSharedPointer<GpgOperaContext>& context, int channel, int index)
    -> OperaWaitingCb {
  return BuildOperasFileEncryptHelper(
      context, channel, index,
      [context, channel](const QString& path, const QString& o_path,
                         const auto& callback) {
        FileCryptoOperation::GetInstance(channel).EncryptDirectorySymmetric(
            path, context->base->ascii, o_path, callback);
      },
      [context, channel](const QString& path, const QString& o_path,
                         const auto& callback) {
        FileCryptoOperation::GetInstance(channel).EncryptDirectory(
            context->base->keys, path, context->base->ascii, o_path, callback);
      });
}

auto GpgOperaHelper::BuildOperasFileDecrypt(
    QSharedPointer<GpgOperaContext>& context, int channel, int index)
    -> OperaWaitingCb {
  return BuildSimpleGpgFileOperasHelper<GpgDecryptResult,
                                        GpgDecryptResultAnalyse>(
      context, channel, index,
      [channel](const QString& path, const QString& o_path,
                const auto& callback) {
        FileCryptoOperation::GetInstance(channel).DecryptFile(path, o_path,
                                                              callback);
      });
}

auto GpgOperaHelper::BuildOperasArchiveDecrypt(
    QSharedPointer<GpgOperaContext>& context, int channel, int index)
    -> OperaWaitingCb {
  return BuildSimpleGpgFileOperasHelper<GpgDecryptResult,
                                        GpgDecryptResultAnalyse>(
      context, channel, index,
      [channel](const QString& path, const QString& o_path,
                const auto& callback) {
        FileCryptoOperation::GetInstance(channel).DecryptArchive(path, o_path,
                                                                 callback);
      });
}

auto GpgOperaHelper::BuildOperasFileSign(
    QSharedPointer<GpgOperaContext>& context, int channel, int index)
    -> OperaWaitingCb {
  return BuildSimpleGpgFileOperasHelper<GpgSignResult, GpgSignResultAnalyse>(
      context, channel, index,
      [channel, context](const QString& path, const QString& o_path,
                         const auto& callback) {
        FileCryptoOperation::GetInstance(channel).SignFile(
            context->base->keys, path, context->base->ascii, o_path, callback);
      });
}

auto GpgOperaHelper::BuildOperasFileVerify(
    QSharedPointer<GpgOperaContext>& context, int channel, int index)
    -> OperaWaitingCb {
  return BuildSimpleGpgFileOperasHelper<GpgVerifyResult,
                                        GpgVerifyResultAnalyse>(
      context, channel, index,
      [channel](const QString& path, const QString& o_path,
                const auto& callback) {
        FileCryptoOperation::GetInstance(channel).VerifyFile(o_path, path,
                                                             callback);
      });
}

auto GpgOperaHelper::BuildOperasFileEncryptSign(
    QSharedPointer<GpgOperaContext>& context, int channel, int index)
    -> OperaWaitingCb {
  return BuildComplexGpgFileOperasHelper<GpgEncryptResult,
                                         GpgEncryptResultAnalyse, GpgSignResult,
                                         GpgSignResultAnalyse>(
      context, channel, index,
      [channel, context](const QString& path, const QString& o_path,
                         const auto& callback) {
        FileCryptoOperation::GetInstance(channel).EncryptSignFile(
            context->base->keys, context->base->singer_keys, path,
            context->base->ascii, o_path, callback);
      });
}

auto GpgOperaHelper::BuildOperasDirectoryEncryptSign(
    QSharedPointer<GpgOperaContext>& context, int channel, int index)
    -> OperaWaitingCb {
  return BuildComplexGpgFileOperasHelper<GpgEncryptResult,
                                         GpgEncryptResultAnalyse, GpgSignResult,
                                         GpgSignResultAnalyse>(
      context, channel, index,
      [channel, context](const QString& path, const QString& o_path,
                         const auto& callback) {
        FileCryptoOperation::GetInstance(channel).EncryptSignDirectory(
            context->base->keys, context->base->singer_keys, path,
            context->base->ascii, o_path, callback);
      });
}

auto GpgOperaHelper::BuildOperasFileDecryptVerify(
    QSharedPointer<GpgOperaContext>& context, int channel, int index)
    -> OperaWaitingCb {
  return BuildComplexGpgFileOperasHelper<
      GpgDecryptResult, GpgDecryptResultAnalyse, GpgVerifyResult,
      GpgVerifyResultAnalyse>(
      context, channel, index,
      [channel](const QString& path, const QString& o_path,
                const auto& callback) {
        FileCryptoOperation::GetInstance(channel).DecryptVerifyFile(
            path, o_path, callback);
      });
}

auto GpgOperaHelper::BuildOperasArchiveDecryptVerify(
    QSharedPointer<GpgOperaContext>& context, int channel, int index)
    -> OperaWaitingCb {
  return BuildComplexGpgFileOperasHelper<
      GpgDecryptResult, GpgDecryptResultAnalyse, GpgVerifyResult,
      GpgVerifyResultAnalyse>(
      context, channel, index,
      [channel](const QString& path, const QString& o_path,
                const auto& callback) {
        FileCryptoOperation::GetInstance(channel).DecryptVerifyArchive(
            path, o_path, callback);
      });
}

void GpgOperaHelper::WaitForMultipleOperas(
    QWidget* parent, const QString& title,
    const QContainer<OperaWaitingCb>& operas, int cancel_channel) {
  if (operas.isEmpty()) return;

  // Clear any leftover cancellation request for this channel before starting
  // the batch. Only cancelable operations carry a channel to reset.
  if (cancel_channel >= 0) ResetGpgOperationCancelState(cancel_channel);

  QEventLoop looper;
  QPointer<WaitingDialog> const dialog =
      new WaitingDialog(title, operas.size() > 1, parent, cancel_channel >= 0);
  connect(dialog, &QDialog::finished, &looper, &QEventLoop::quit);
  if (cancel_channel >= 0) {
    connect(dialog, &WaitingDialog::SignalCancelRequested, dialog,
            [cancel_channel]() { RequestCancelGpgOperation(cancel_channel); });
  }

  // Only present the dialog if the batch runs longer than the threshold.
  QPointer<QTimer> const show_timer = StartDeferredShowTimer(dialog);

  std::atomic<int> remaining_tasks(static_cast<int>(operas.size()));
  const auto tasks_count = operas.size();

  for (const auto& opera : operas) {
    QMetaObject::invokeMethod(
        parent,
        [=, &remaining_tasks]() {
          opera([dialog, show_timer, &remaining_tasks, tasks_count]() {
            if (dialog == nullptr) return;
            const auto pg_value =
                static_cast<double>(tasks_count - remaining_tasks + 1) * 100.0 /
                static_cast<double>(tasks_count);
            emit dialog->SignalUpdateValue(static_cast<int>(pg_value));
            QCoreApplication::processEvents();
            if (--remaining_tasks == 0) {
              if (show_timer) show_timer->stop();
              dialog->close();
              dialog->accept();
            }
          });
        },
        Qt::QueuedConnection);
  }

  looper.exec();
}

auto GpgOperaHelper::BuildOperasEncrypt(
    QSharedPointer<GpgOperaContext>& context, int channel, int index)
    -> OperaWaitingCb {
  return BuildOperasEncryptHelper(
      context, channel, index,
      [context, channel](const GFBuffer& buffer, const auto& callback) {
        MessageCryptoOperation::GetInstance(channel).EncryptSymmetric(
            buffer, context->base->ascii, callback);
      },
      [context, channel](const GFBuffer& buffer, const auto& callback) {
        MessageCryptoOperation::GetInstance(channel).Encrypt(
            context->base->keys, buffer, context->base->ascii, callback);
      });
}

auto GpgOperaHelper::BuildOperasDecrypt(
    QSharedPointer<GpgOperaContext>& context, int channel, int index)
    -> OperaWaitingCb {
  return BuildSimpleGpgOperasHelper<GpgDecryptResult, GpgDecryptResultAnalyse>(
      context, channel, index,
      [channel](const GFBuffer& buffer, const auto& callback) {
        MessageCryptoOperation::GetInstance(channel).Decrypt(buffer, callback);
      });
}

auto GpgOperaHelper::BuildOperasSign(QSharedPointer<GpgOperaContext>& context,
                                     int channel, int index) -> OperaWaitingCb {
  return BuildSimpleGpgOperasHelper<GpgSignResult, GpgSignResultAnalyse>(
      context, channel, index,
      [channel, context](const GFBuffer& buffer, const auto& callback) {
        MessageCryptoOperation::GetInstance(channel).Sign(
            context->base->keys, buffer, GPGME_SIG_MODE_CLEAR,
            context->base->ascii, callback);
      });
}

auto GpgOperaHelper::BuildOperasVerify(QSharedPointer<GpgOperaContext>& context,
                                       int channel, int index)
    -> OperaWaitingCb {
  return BuildSimpleGpgOperasHelper<GpgVerifyResult, GpgVerifyResultAnalyse>(
      context, channel, index,
      [channel, context](const GFBuffer& buffer, const auto& callback) {
        MessageCryptoOperation::GetInstance(channel).Verify(buffer, GFBuffer(),
                                                            callback);
      });
}

auto GpgOperaHelper::BuildOperasEncryptSign(
    QSharedPointer<GpgOperaContext>& context, int channel, int index)
    -> OperaWaitingCb {
  return BuildComplexGpgOperasHelper<GpgEncryptResult, GpgEncryptResultAnalyse,
                                     GpgSignResult, GpgSignResultAnalyse>(
      context, channel, index,
      [channel, context](const GFBuffer& buffer, const auto& callback) {
        MessageCryptoOperation::GetInstance(channel).EncryptSign(
            context->base->keys, context->base->singer_keys, buffer,
            context->base->ascii, callback);
      });
}

auto GpgOperaHelper::BuildOperasDecryptVerify(
    QSharedPointer<GpgOperaContext>& context, int channel, int index)
    -> OperaWaitingCb {
  return BuildComplexGpgOperasHelper<GpgDecryptResult, GpgDecryptResultAnalyse,
                                     GpgVerifyResult, GpgVerifyResultAnalyse>(
      context, channel, index,
      [channel](const GFBuffer& buffer, const auto& callback) {
        MessageCryptoOperation::GetInstance(channel).DecryptVerify(buffer,
                                                                   callback);
      });
}

void GpgOperaHelper::WaitForOpera(QWidget* parent, const QString& title,
                                  const OperaWaitingCb& opera,
                                  int cancel_channel) {
  // Clear any leftover cancellation request for this channel before starting
  // the operation. Only cancelable operations carry a channel to reset.
  if (cancel_channel >= 0) ResetGpgOperationCancelState(cancel_channel);

  QEventLoop looper;
  QPointer<WaitingDialog> const dialog =
      new WaitingDialog(title, false, parent, cancel_channel >= 0);

  QObject::connect(dialog, &QDialog::finished, &looper, &QEventLoop::quit);
  if (cancel_channel >= 0) {
    QObject::connect(
        dialog, &WaitingDialog::SignalCancelRequested, dialog,
        [cancel_channel]() { RequestCancelGpgOperation(cancel_channel); });
  }

  // Only present the dialog if the operation runs longer than the threshold.
  QPointer<QTimer> const show_timer = StartDeferredShowTimer(dialog);

  QMetaObject::invokeMethod(
      parent,
      [=]() {
        opera([dialog, show_timer]() {
          if (show_timer) show_timer->stop();
          if (dialog) {
            dialog->close();
            dialog->accept();
          }
        });
      },
      Qt::QueuedConnection);

  looper.exec();
}
}  // namespace GpgFrontend::UI
