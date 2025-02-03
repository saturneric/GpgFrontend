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

#include "core/GpgModel.h"
#include "core/function/gpg/GpgFileOpera.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/utils/GpgUtils.h"
#include "ui/dialog/WaitingDialog.h"

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
void HandleExtraLogicIfNeeded(const QSharedPointer<GpgOperaContext>& context,
                              const AnalyseType& analyse) {}

template <>
void HandleExtraLogicIfNeeded<GpgVerifyResultAnalyse>(
    const QSharedPointer<GpgOperaContext>& context,
    const GpgVerifyResultAnalyse& analyse) {
  context->base->unknown_fprs.append(analyse.GetUnknownSignatures());
}

template <typename ResultType, typename AnalyseType, typename OperaFunc>
auto GpgOperaHelper::BuildSimpleGpgFileOperasHelper(
    QSharedPointer<GpgOperaContext>& context, int channel, int index,
    OperaFunc opera_func) -> OperaWaitingCb {
  const auto& path = context->paths[index];
  const auto& o_path = context->o_paths[index];
  auto& opera_results = context->base->opera_results;

  return [=, &opera_results](const OperaWaitingHd& op_hd) {
    opera_func(
        path, o_path,
        [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
          // stop waiting
          op_hd();

          if (CheckGpgError(err) == GPG_ERR_USER_1 || data_obj == nullptr ||
              !data_obj->Check<ResultType>()) {
            opera_results.append(
                {-1, "# " + tr("Critical Error"), QFileInfo(path).fileName()});
            return;
          }

          auto result = ExtractParams<ResultType>(data_obj, 0);
          auto result_analyse = AnalyseType(channel, err, result);
          result_analyse.Analyse();

          HandleExtraLogicIfNeeded(context, result_analyse);

          opera_results.append(
              {result_analyse.GetStatus(), result_analyse.GetResultReport(),
               QFileInfo(path.isEmpty() ? o_path : path).fileName()});
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

  return [=, &opera_results](const OperaWaitingHd& op_hd) {
    opera_func(
        path, o_path,
        [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
          // stop waiting
          op_hd();

          if (CheckGpgError(err) == GPG_ERR_USER_1 || data_obj == nullptr ||
              !data_obj->Check<ResultTypeA, ResultTypeB>()) {
            opera_results.append(
                {-1, "# " + tr("Critical Error"), QFileInfo(path).fileName()});
            return;
          }

          auto result_1 = ExtractParams<ResultTypeA>(data_obj, 0);
          auto result_2 = ExtractParams<ResultTypeB>(data_obj, 1);

          auto result_analyse_1 = AnalyseTypeA(channel, err, result_1);
          result_analyse_1.Analyse();

          HandleExtraLogicIfNeeded(context, result_analyse_1);

          auto result_analyse_2 = AnalyseTypeB(channel, err, result_2);
          result_analyse_2.Analyse();

          HandleExtraLogicIfNeeded(context, result_analyse_2);

          opera_results.append(
              {std::min(result_analyse_1.GetStatus(),
                        result_analyse_2.GetStatus()),
               result_analyse_1.GetResultReport() +
                   result_analyse_2.GetResultReport(),
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

  return [=, &opera_results](const OperaWaitingHd& op_hd) {
    opera_func(buffer, [=, &opera_results](GpgError err,
                                           const DataObjectPtr& data_obj) {
      // stop waiting
      op_hd();

      if (CheckGpgError(err) == GPG_ERR_USER_1 || data_obj == nullptr ||
          !data_obj->Check<ResultType, GFBuffer>()) {
        opera_results.append({-1, "# " + tr("Critical Error"), {}});
        return;
      }

      auto result = ExtractParams<ResultType>(data_obj, 0);

      auto result_analyse = AnalyseType(channel, err, result);
      result_analyse.Analyse();

      HandleExtraLogicIfNeeded(context, result_analyse);

      auto opera_result = GpgOperaResult{
          result_analyse.GetStatus(), result_analyse.GetResultReport(), {}};

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

  return [=, &opera_results](const OperaWaitingHd& op_hd) {
    opera_func(buffer, [=, &opera_results](GpgError err,
                                           const DataObjectPtr& data_obj) {
      // stop waiting
      op_hd();

      if (CheckGpgError(err) == GPG_ERR_USER_1 || data_obj == nullptr ||
          !data_obj->Check<ResultTypeA, ResultTypeB, GFBuffer>()) {
        opera_results.append({-1, "# " + tr("Critical Error"), {}});
        return;
      }

      auto result_1 = ExtractParams<ResultTypeA>(data_obj, 0);
      auto result_2 = ExtractParams<ResultTypeB>(data_obj, 1);

      auto result_analyse_1 = AnalyseTypeA(channel, err, result_1);
      result_analyse_1.Analyse();

      HandleExtraLogicIfNeeded(context, result_analyse_1);

      auto result_analyse_2 = AnalyseTypeB(channel, err, result_2);
      result_analyse_2.Analyse();

      HandleExtraLogicIfNeeded(context, result_analyse_2);

      auto opera_result = GpgOperaResult{
          std::min(result_analyse_1.GetStatus(), result_analyse_2.GetStatus()),
          result_analyse_1.GetResultReport() +
              result_analyse_2.GetResultReport(),
          {}};

      auto o_buffer = ExtractParams<GFBuffer>(data_obj, 2);
      opera_result.o_buffer = o_buffer;

      opera_results.append(opera_result);
    });
  };
}

template <typename EncryptFuncSymmetric, typename EncryptFuncKeys>
auto BuildOperasFileEncryptHelper(
    QSharedPointer<GpgOperaContext>& context, int channel, int index,
    EncryptFuncSymmetric encrypt_symmetric,
    EncryptFuncKeys encrypt_with_keys) -> OperaWaitingCb {
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
auto BuildOperasEncryptHelper(
    QSharedPointer<GpgOperaContext>& context, int channel, int index,
    EncryptFuncSymmetric encrypt_symmetric,
    EncryptFuncKeys encrypt_with_keys) -> OperaWaitingCb {
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
    QSharedPointer<GpgOperaContext>& context, int channel,
    int index) -> OperaWaitingCb {
  return BuildOperasFileEncryptHelper(
      context, channel, index,
      [context, channel](const QString& path, const QString& o_path,
                         const auto& callback) {
        GpgFileOpera::GetInstance(channel).EncryptFileSymmetric(
            path, context->base->ascii, o_path, callback);
      },
      [context, channel](const QString& path, const QString& o_path,
                         const auto& callback) {
        GpgFileOpera::GetInstance(channel).EncryptFile(
            context->base->keys, path, context->base->ascii, o_path, callback);
      });
}

auto GpgOperaHelper::BuildOperasDirectoryEncrypt(
    QSharedPointer<GpgOperaContext>& context, int channel,
    int index) -> OperaWaitingCb {
  return BuildOperasFileEncryptHelper(
      context, channel, index,
      [context, channel](const QString& path, const QString& o_path,
                         const auto& callback) {
        GpgFileOpera::GetInstance(channel).EncryptDirectorySymmetric(
            path, context->base->ascii, o_path, callback);
      },
      [context, channel](const QString& path, const QString& o_path,
                         const auto& callback) {
        GpgFileOpera::GetInstance(channel).EncryptDirectory(
            context->base->keys, path, context->base->ascii, o_path, callback);
      });
}

auto GpgOperaHelper::BuildOperasFileDecrypt(
    QSharedPointer<GpgOperaContext>& context, int channel,
    int index) -> OperaWaitingCb {
  return BuildSimpleGpgFileOperasHelper<GpgDecryptResult,
                                        GpgDecryptResultAnalyse>(
      context, channel, index,
      [channel](const QString& path, const QString& o_path,
                const auto& callback) {
        GpgFileOpera::GetInstance(channel).DecryptFile(path, o_path, callback);
      });
}

auto GpgOperaHelper::BuildOperasArchiveDecrypt(
    QSharedPointer<GpgOperaContext>& context, int channel,
    int index) -> OperaWaitingCb {
  return BuildSimpleGpgFileOperasHelper<GpgDecryptResult,
                                        GpgDecryptResultAnalyse>(
      context, channel, index,
      [channel](const QString& path, const QString& o_path,
                const auto& callback) {
        GpgFileOpera::GetInstance(channel).DecryptArchive(path, o_path,
                                                          callback);
      });
}

auto GpgOperaHelper::BuildOperasFileSign(
    QSharedPointer<GpgOperaContext>& context, int channel,
    int index) -> OperaWaitingCb {
  return BuildSimpleGpgFileOperasHelper<GpgSignResult, GpgSignResultAnalyse>(
      context, channel, index,
      [channel, context](const QString& path, const QString& o_path,
                         const auto& callback) {
        GpgFileOpera::GetInstance(channel).SignFile(
            context->base->keys, path, context->base->ascii, o_path, callback);
      });
}

auto GpgOperaHelper::BuildOperasFileVerify(
    QSharedPointer<GpgOperaContext>& context, int channel,
    int index) -> OperaWaitingCb {
  return BuildSimpleGpgFileOperasHelper<GpgVerifyResult,
                                        GpgVerifyResultAnalyse>(
      context, channel, index,
      [channel](const QString& path, const QString& o_path,
                const auto& callback) {
        GpgFileOpera::GetInstance(channel).VerifyFile(o_path, path, callback);
      });
}

auto GpgOperaHelper::BuildOperasFileEncryptSign(
    QSharedPointer<GpgOperaContext>& context, int channel,
    int index) -> OperaWaitingCb {
  return BuildComplexGpgFileOperasHelper<GpgEncryptResult,
                                         GpgEncryptResultAnalyse, GpgSignResult,
                                         GpgSignResultAnalyse>(
      context, channel, index,
      [channel, context](const QString& path, const QString& o_path,
                         const auto& callback) {
        GpgFileOpera::GetInstance(channel).EncryptSignFile(
            context->base->keys, context->base->singer_keys, path,
            context->base->ascii, o_path, callback);
      });
}

auto GpgOperaHelper::BuildOperasDirectoryEncryptSign(
    QSharedPointer<GpgOperaContext>& context, int channel,
    int index) -> OperaWaitingCb {
  return BuildComplexGpgFileOperasHelper<GpgEncryptResult,
                                         GpgEncryptResultAnalyse, GpgSignResult,
                                         GpgSignResultAnalyse>(
      context, channel, index,
      [channel, context](const QString& path, const QString& o_path,
                         const auto& callback) {
        GpgFileOpera::GetInstance(channel).EncryptSignDirectory(
            context->base->keys, context->base->singer_keys, path,
            context->base->ascii, o_path, callback);
      });
}

auto GpgOperaHelper::BuildOperasFileDecryptVerify(
    QSharedPointer<GpgOperaContext>& context, int channel,
    int index) -> OperaWaitingCb {
  return BuildComplexGpgFileOperasHelper<
      GpgDecryptResult, GpgDecryptResultAnalyse, GpgVerifyResult,
      GpgVerifyResultAnalyse>(
      context, channel, index,
      [channel](const QString& path, const QString& o_path,
                const auto& callback) {
        GpgFileOpera::GetInstance(channel).DecryptVerifyFile(path, o_path,
                                                             callback);
      });
}

auto GpgOperaHelper::BuildOperasArchiveDecryptVerify(
    QSharedPointer<GpgOperaContext>& context, int channel,
    int index) -> OperaWaitingCb {
  return BuildComplexGpgFileOperasHelper<
      GpgDecryptResult, GpgDecryptResultAnalyse, GpgVerifyResult,
      GpgVerifyResultAnalyse>(
      context, channel, index,
      [channel](const QString& path, const QString& o_path,
                const auto& callback) {
        GpgFileOpera::GetInstance(channel).DecryptVerifyArchive(path, o_path,
                                                                callback);
      });
}

void GpgOperaHelper::WaitForMultipleOperas(
    QWidget* parent, const QString& title,
    const QContainer<OperaWaitingCb>& operas) {
  if (operas.isEmpty()) return;

  QEventLoop looper;
  QPointer<WaitingDialog> const dialog =
      new WaitingDialog(title, operas.size() > 1, parent);
  connect(dialog, &QDialog::finished, &looper, &QEventLoop::quit);
  connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
  dialog->show();

  std::atomic<int> remaining_tasks(static_cast<int>(operas.size()));
  const auto tasks_count = operas.size();

  for (const auto& opera : operas) {
    QTimer::singleShot(64, parent, [=, &remaining_tasks]() {
      opera([dialog, &remaining_tasks, tasks_count]() {
        if (dialog == nullptr) return;

        const auto pg_value =
            static_cast<double>(tasks_count - remaining_tasks + 1) * 100.0 /
            static_cast<double>(tasks_count);
        emit dialog->SignalUpdateValue(static_cast<int>(pg_value));
        QCoreApplication::processEvents();

        if (--remaining_tasks == 0) {
          dialog->close();
          dialog->accept();
        }
      });
    });
  }

  looper.exec();
}

auto GpgOperaHelper::BuildOperasEncrypt(
    QSharedPointer<GpgOperaContext>& context, int channel,
    int index) -> OperaWaitingCb {
  return BuildOperasEncryptHelper(
      context, channel, index,
      [context, channel](const GFBuffer& buffer, const auto& callback) {
        GpgBasicOperator::GetInstance(channel).EncryptSymmetric(
            buffer, context->base->ascii, callback);
      },
      [context, channel](const GFBuffer& buffer, const auto& callback) {
        GpgBasicOperator::GetInstance(channel).Encrypt(
            context->base->keys, buffer, context->base->ascii, callback);
      });
}

auto GpgOperaHelper::BuildOperasDecrypt(
    QSharedPointer<GpgOperaContext>& context, int channel,
    int index) -> OperaWaitingCb {
  return BuildSimpleGpgOperasHelper<GpgDecryptResult, GpgDecryptResultAnalyse>(
      context, channel, index,
      [channel](const GFBuffer& buffer, const auto& callback) {
        GpgBasicOperator::GetInstance(channel).Decrypt(buffer, callback);
      });
}

auto GpgOperaHelper::BuildOperasSign(QSharedPointer<GpgOperaContext>& context,
                                     int channel, int index) -> OperaWaitingCb {
  return BuildSimpleGpgOperasHelper<GpgSignResult, GpgSignResultAnalyse>(
      context, channel, index,
      [channel, context](const GFBuffer& buffer, const auto& callback) {
        GpgBasicOperator::GetInstance(channel).Sign(
            context->base->keys, buffer, GPGME_SIG_MODE_CLEAR,
            context->base->ascii, callback);
      });
}

auto GpgOperaHelper::BuildOperasVerify(QSharedPointer<GpgOperaContext>& context,
                                       int channel,
                                       int index) -> OperaWaitingCb {
  return BuildSimpleGpgOperasHelper<GpgVerifyResult, GpgVerifyResultAnalyse>(
      context, channel, index,
      [channel, context](const GFBuffer& buffer, const auto& callback) {
        GpgBasicOperator::GetInstance(channel).Verify(buffer, GFBuffer(),
                                                      callback);
      });
}

auto GpgOperaHelper::BuildOperasEncryptSign(
    QSharedPointer<GpgOperaContext>& context, int channel,
    int index) -> OperaWaitingCb {
  return BuildComplexGpgOperasHelper<GpgEncryptResult, GpgEncryptResultAnalyse,
                                     GpgSignResult, GpgSignResultAnalyse>(
      context, channel, index,
      [channel, context](const GFBuffer& buffer, const auto& callback) {
        GpgBasicOperator::GetInstance(channel).EncryptSign(
            context->base->keys, context->base->singer_keys, buffer,
            context->base->ascii, callback);
      });
}

auto GpgOperaHelper::BuildOperasDecryptVerify(
    QSharedPointer<GpgOperaContext>& context, int channel,
    int index) -> OperaWaitingCb {
  return BuildComplexGpgOperasHelper<GpgDecryptResult, GpgDecryptResultAnalyse,
                                     GpgVerifyResult, GpgVerifyResultAnalyse>(
      context, channel, index,
      [channel](const GFBuffer& buffer, const auto& callback) {
        GpgBasicOperator::GetInstance(channel).DecryptVerify(buffer, callback);
      });
}

void GpgOperaHelper::WaitForOpera(QWidget* parent, const QString& title,
                                  const OperaWaitingCb& opera) {
  QEventLoop looper;
  QPointer<WaitingDialog> const dialog =
      new WaitingDialog(title, false, parent);
  connect(dialog, &QDialog::finished, &looper, &QEventLoop::quit);
  connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
  dialog->show();

  QTimer::singleShot(64, parent, [=]() {
    opera([dialog]() {
      if (dialog != nullptr) {
        dialog->close();
        dialog->accept();
      }
    });
  });

  looper.exec();
}
}  // namespace GpgFrontend::UI
