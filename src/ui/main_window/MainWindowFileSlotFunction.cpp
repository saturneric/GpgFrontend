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

#include "MainWindow.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgFileOpera.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/module/ModuleManager.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/SignersPicker.h"
#include "ui/struct/GpgOperaResult.h"
#include "ui/struct/GpgOperaResultContext.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

auto MainWindow::check_read_file_paths_helper(const QStringList& paths)
    -> bool {
  QStringList invalid_files;
  for (const auto& path : paths) {
    auto result = TargetFilePreCheck(path, true);
    if (!std::get<0>(result)) {
      invalid_files.append(path);
    }
  }

  if (!invalid_files.empty()) {
    QString error_file_names;
    for (const auto& file_path : invalid_files) {
      error_file_names += QFileInfo(file_path).fileName() + "\n";
    }

    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot read from the following files:\n\n%1")
                              .arg(error_file_names.trimmed()));
    return false;
  }

  return true;
}

auto MainWindow::check_write_file_paths_helper(const QStringList& o_paths)
    -> bool {
  for (const auto& o_path : o_paths) {
    if (QFile::exists(o_path)) {
      auto out_file_name = tr("The target file %1 already exists, "
                              "do you need to overwrite it?")
                               .arg(QFileInfo(o_path).fileName());
      auto ret = QMessageBox::warning(this, tr("Warning"), out_file_name,
                                      QMessageBox::Ok | QMessageBox::Cancel);

      if (ret == QMessageBox::Cancel) return false;
    }
  }

  QStringList invalid_output_files;
  for (const auto& path : o_paths) {
    auto result = TargetFilePreCheck(path, false);
    if (!std::get<0>(result)) {
      invalid_output_files.append(path);
    }
  }

  if (!invalid_output_files.empty()) {
    QString error_file_names;
    for (const auto& file_path : invalid_output_files) {
      error_file_names += QFileInfo(file_path).fileName() + "\n";
    }

    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot write to the following files:\n\n%1")
                              .arg(error_file_names.trimmed()));
    return false;
  }

  return true;
}

auto MainWindow::check_keys_helper(
    const KeyIdArgsList& key_ids,
    const std::function<bool(const GpgKey&)>& capability_check,
    const QString& capability_err_string) -> GpgKeyList {
  if (key_ids.empty()) {
    QMessageBox::critical(
        this, tr("No Key Checked"),
        tr("Please check the key in the key toolbox on the right."));
    return {};
  }

  auto keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(key_ids);
  assert(std::all_of(keys.begin(), keys.end(),
                     [](const auto& key) { return key.IsGood(); }));

  // check key abilities
  for (const auto& key : keys) {
    if (!capability_check(key)) {
      QMessageBox::critical(nullptr, tr("Invalid KeyPair"),
                            capability_err_string + "<br/><br/>" +
                                tr("For example the Following Key:") +
                                " <br/>" + key.GetUIDs()->front().GetUID());
      return {};
    }
  }

  return keys;
}

auto MainWindow::execute_operas_helper(
    const QString& task, const QSharedPointer<GpgOperaContexts>& contexts) {
  CommonUtils::WaitForMultipleOperas(this, task, contexts->operas);
  slot_result_analyse_show_helper(contexts->opera_results);
}

void MainWindow::build_operas_file_symmetric_encrypt(
    QSharedPointer<GpgOperaContext>& context) {
  assert(context->paths.size() == context->o_paths.size());

  for (int i = 0; i < context->paths.size(); i++) {
    const auto& path = context->paths[i];
    const auto& o_path = context->o_paths[i];

    auto opera = [=, &opera_results =
                         context->opera_results](const OperaWaitingHd& op_hd) {
      GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .EncryptFileSymmetric(
              path, context->ascii, o_path,
              [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                    data_obj == nullptr ||
                    !data_obj->Check<GpgEncryptResult>()) {
                  QMessageBox::critical(this, tr("Error"),
                                        tr("Unknown error occurred"));
                  return;
                }

                auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
                auto result_analyse = GpgEncryptResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err, result);
                result_analyse.Analyse();

                opera_results.append({result_analyse.GetStatus(),
                                      result_analyse.GetResultReport(),
                                      QFileInfo(path).fileName()});
              });
    };

    context->operas.push_back(opera);
  }
}

void MainWindow::build_operas_file_encrypt(
    QSharedPointer<GpgOperaContext>& context) {
  assert(context->paths.size() == context->o_paths.size());

  for (int i = 0; i < context->paths.size(); i++) {
    const auto& path = context->paths[i];
    const auto& o_path = context->o_paths[i];

    auto opera = [=, &opera_results =
                         context->opera_results](const OperaWaitingHd& op_hd) {
      GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .EncryptFile(
              context->keys, path, context->ascii, o_path,
              [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                    data_obj == nullptr ||
                    !data_obj->Check<GpgEncryptResult>()) {
                  QMessageBox::critical(this, tr("Error"),
                                        tr("Unknown error occurred"));
                  return;
                }

                auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
                auto result_analyse = GpgEncryptResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err, result);
                result_analyse.Analyse();

                opera_results.append({result_analyse.GetStatus(),
                                      result_analyse.GetResultReport(),
                                      QFileInfo(path).fileName()});
              });
    };

    context->operas.push_back(opera);
  }
}

void MainWindow::build_operas_directory_symmetric_encrypt(
    QSharedPointer<GpgOperaContext>& context) {
  assert(context->paths.size() == context->o_paths.size());

  for (int i = 0; i < context->paths.size(); i++) {
    const auto& path = context->paths[i];
    const auto& o_path = context->o_paths[i];

    auto opera = [=, &opera_results =
                         context->opera_results](const OperaWaitingHd& op_hd) {
      GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .EncryptDirectorySymmetric(
              path, context->ascii, o_path,
              [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (data_obj == nullptr ||
                    !data_obj->Check<GpgEncryptResult>()) {
                  QMessageBox::critical(this, tr("Error"),
                                        tr("Unknown error occurred"));
                  return;
                }

                auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
                auto result_analyse = GpgEncryptResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err, result);
                result_analyse.Analyse();

                opera_results.append({result_analyse.GetStatus(),
                                      result_analyse.GetResultReport(),
                                      QFileInfo(path).fileName()});
              });
    };

    context->operas.push_back(opera);
  }
}

void MainWindow::build_operas_directory_encrypt(
    QSharedPointer<GpgOperaContext>& context) {
  assert(context->paths.size() == context->o_paths.size());

  for (int i = 0; i < context->paths.size(); i++) {
    const auto& path = context->paths[i];
    const auto& o_path = context->o_paths[i];

    auto opera = [=, &opera_results =
                         context->opera_results](const OperaWaitingHd& op_hd) {
      GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .EncryptDirectory(
              context->keys, path, context->ascii, o_path,
              [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                    data_obj == nullptr ||
                    !data_obj->Check<GpgEncryptResult>()) {
                  QMessageBox::critical(this, tr("Error"),
                                        tr("Unknown error occurred"));
                  return;
                }

                auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
                auto result_analyse = GpgEncryptResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err, result);
                result_analyse.Analyse();

                opera_results.append({result_analyse.GetStatus(),
                                      result_analyse.GetResultReport(),
                                      QFileInfo(path).fileName()});
              });
    };

    context->operas.push_back(opera);
  }
}

void MainWindow::SlotFileEncrypt(const QStringList& paths) {
  auto contexts = QSharedPointer<GpgOperaContexts>::create();

  bool const non_ascii_at_file_operation =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("gnupg/non_ascii_at_file_operation", true)
          .toBool();

  contexts->ascii = !non_ascii_at_file_operation;

  bool is_symmetric = false;
  auto key_ids = m_key_list_->GetChecked();

  // Symmetric Encrypt
  if (key_ids.isEmpty()) {
    auto ret = QMessageBox::information(
        this, tr("Symmetric Encryption"),
        tr("No Key Selected. Do you want to encrypt with a "
           "symmetric cipher using a passphrase?"),
        QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Cancel) return;
    is_symmetric = true;
  } else {
    contexts->keys = check_keys_helper(
        key_ids,
        [](const GpgKey& key) { return key.IsHasActualEncryptionCapability(); },
        tr("The selected keypair cannot be used for encryption."));
    if (contexts->keys.empty()) return;
  }

  if (!check_read_file_paths_helper(paths)) return;

  for (const auto& path : paths) {
    QFileInfo info(path);
    if (info.isDir()) {
      contexts->GetContextPath(1).append(path);
      contexts->GetContextOutPath(1).append(
          SetExtensionOfOutputFileForArchive(path, kENCRYPT, contexts->ascii));
    } else {
      contexts->GetContextPath(0).append(path);
      contexts->GetContextOutPath(0).append(
          SetExtensionOfOutputFile(path, kENCRYPT, contexts->ascii));
    }
  }

  if (!check_write_file_paths_helper(contexts->GetAllOutPath())) return;

  // Symmetric Encrypt
  if (is_symmetric) {
    auto f_context = contexts->GetContext(0);
    if (f_context != nullptr) build_operas_file_symmetric_encrypt(f_context);

    auto d_context = contexts->GetContext(1);
    if (d_context != nullptr) {
      build_operas_directory_symmetric_encrypt(d_context);
    }
  } else {
    auto f_context = contexts->GetContext(0);
    if (f_context != nullptr) build_operas_file_encrypt(f_context);

    auto d_context = contexts->GetContext(1);
    if (d_context != nullptr) build_operas_directory_encrypt(d_context);
  }

  execute_operas_helper(tr("Encrypting"), contexts);
}

/**
 * @brief
 *
 * @param context
 */
void MainWindow::build_operas_file_decrypt(
    QSharedPointer<GpgOperaContext>& context) {
  assert(context->paths.size() == context->o_paths.size());

  for (int i = 0; i < context->paths.size(); i++) {
    const auto& path = context->paths[i];
    const auto& o_path = context->o_paths[i];

    auto opera = [=, &opera_results =
                         context->opera_results](const OperaWaitingHd& op_hd) {
      GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .DecryptFile(
              path, o_path,
              [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                    data_obj == nullptr ||
                    !data_obj->Check<GpgDecryptResult>()) {
                  QMessageBox::critical(this, tr("Error"),
                                        tr("Unknown error occurred"));
                  return;
                }

                auto result = ExtractParams<GpgDecryptResult>(data_obj, 0);
                auto result_analyse = GpgDecryptResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err, result);
                result_analyse.Analyse();

                opera_results.append({result_analyse.GetStatus(),
                                      result_analyse.GetResultReport(),
                                      QFileInfo(path).fileName()});
              });
    };

    context->operas.push_back(opera);
  }
}

void MainWindow::build_operas_archive_decrypt(
    QSharedPointer<GpgOperaContext>& context) {
  assert(context->paths.size() == context->o_paths.size());

  for (int i = 0; i < context->paths.size(); i++) {
    const auto& path = context->paths[i];
    const auto& o_path = context->o_paths[i];

    auto opera = [=, &opera_results =
                         context->opera_results](const OperaWaitingHd& op_hd) {
      GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .DecryptArchive(
              path, o_path,
              [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                    data_obj == nullptr ||
                    !data_obj->Check<GpgDecryptResult>()) {
                  QMessageBox::critical(this, tr("Error"),
                                        tr("Unknown error occurred"));
                  return;
                }

                auto result = ExtractParams<GpgDecryptResult>(data_obj, 0);
                auto result_analyse = GpgDecryptResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err, result);
                result_analyse.Analyse();

                opera_results.append({result_analyse.GetStatus(),
                                      result_analyse.GetResultReport(),
                                      QFileInfo(path).fileName()});
              });
    };

    context->operas.push_back(opera);
  }
}

void MainWindow::SlotFileDecrypt(const QStringList& paths) {
  auto contexts = QSharedPointer<GpgOperaContexts>::create();

  contexts->ascii = true;

  if (!check_read_file_paths_helper(paths)) return;

  for (const auto& path : paths) {
    QFileInfo info(path);
    const auto extension = info.completeSuffix();

    if (extension == "tar.gpg" || extension == "tar.asc") {
      contexts->GetContextPath(1).append(path);
      contexts->GetContextOutPath(1).append(
          SetExtensionOfOutputFileForArchive(path, kDECRYPT, contexts->ascii));
    } else {
      contexts->GetContextPath(0).append(path);
      contexts->GetContextOutPath(0).append(
          SetExtensionOfOutputFile(path, kDECRYPT, contexts->ascii));
    }
  }

  if (!check_write_file_paths_helper(contexts->GetAllOutPath())) return;

  auto f_context = contexts->GetContext(0);
  if (f_context != nullptr) build_operas_file_decrypt(f_context);

  auto d_context = contexts->GetContext(1);
  if (d_context != nullptr) {
    build_operas_archive_decrypt(d_context);
  }

  execute_operas_helper(tr("Decrypting"), contexts);
}

void MainWindow::build_operas_file_sign(
    QSharedPointer<GpgOperaContext>& context) {
  assert(context->paths.size() == context->o_paths.size());

  for (int i = 0; i < context->paths.size(); i++) {
    const auto& path = context->paths[i];
    const auto& o_path = context->o_paths[i];

    auto opera = [=, &opera_results =
                         context->opera_results](const OperaWaitingHd& op_hd) {
      GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .SignFile(
              context->keys, path, context->ascii, o_path,
              [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                    data_obj == nullptr || !data_obj->Check<GpgSignResult>()) {
                  QMessageBox::critical(this, tr("Error"),
                                        tr("Unknown error occurred"));
                  return;
                }

                auto result = ExtractParams<GpgSignResult>(data_obj, 0);
                auto result_analyse = GpgSignResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err, result);
                result_analyse.Analyse();

                opera_results.append({result_analyse.GetStatus(),
                                      result_analyse.GetResultReport(),
                                      QFileInfo(path).fileName()});
              });
    };

    context->operas.push_back(opera);
  }
}

void MainWindow::SlotFileSign(const QStringList& paths) {
  auto contexts = QSharedPointer<GpgOperaContexts>::create();

  bool const non_ascii_at_file_operation =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("gnupg/non_ascii_at_file_operation", true)
          .toBool();

  contexts->ascii = !non_ascii_at_file_operation;

  auto key_ids = m_key_list_->GetChecked();

  contexts->keys = check_keys_helper(
      key_ids,
      [](const GpgKey& key) { return key.IsHasActualSigningCapability(); },
      tr("The selected key contains a key that does not actually have a sign "
         "usage."));
  if (contexts->keys.empty()) return;

  if (!check_read_file_paths_helper(paths)) return;

  for (const auto& path : paths) {
    QFileInfo info(path);
    contexts->GetContextPath(0).append(path);
    contexts->GetContextOutPath(0).append(
        SetExtensionOfOutputFile(path, kSIGN, contexts->ascii));
  }

  if (!check_write_file_paths_helper(contexts->GetAllOutPath())) return;

  auto f_context = contexts->GetContext(0);
  if (f_context != nullptr) build_operas_file_sign(f_context);

  execute_operas_helper(tr("Signing"), contexts);
}

void MainWindow::build_operas_file_verify(
    QSharedPointer<GpgOperaContext>& context) {
  assert(context->paths.size() == context->o_paths.size());

  for (int i = 0; i < context->paths.size(); i++) {
    const auto& path = context->paths[i];
    const auto& o_path = context->o_paths[i];

    auto opera = [=, &opera_results =
                         context->opera_results](const OperaWaitingHd& op_hd) {
      GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .VerifyFile(
              o_path, path,
              [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                    data_obj == nullptr ||
                    !data_obj->Check<GpgVerifyResult>()) {
                  QMessageBox::critical(this, tr("Error"),
                                        tr("Unknown error occurred"));
                  return;
                }

                auto result = ExtractParams<GpgVerifyResult>(data_obj, 0);
                auto result_analyse = GpgVerifyResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err, result);
                result_analyse.Analyse();

                slot_result_analyse_show_helper(result_analyse);

                opera_results.append(
                    {result_analyse.GetStatus(),
                     result_analyse.GetResultReport(),
                     QFileInfo(path.isEmpty() ? o_path : path).fileName()});

                context->unknown_fprs = result_analyse.GetUnknownSignatures();
              });
    };

    context->operas.push_back(opera);
  }
}

void MainWindow::SlotFileVerify(const QStringList& paths) {
  auto contexts = QSharedPointer<GpgOperaContexts>::create();

  if (!check_read_file_paths_helper(paths)) return;

  for (const auto& path : paths) {
    QFileInfo info(path);

    QString sign_file_path = path;
    QString data_file_path;

    bool const possible_singleton_target =
        info.suffix() == "gpg" || info.suffix() == "pgp";
    if (possible_singleton_target) {
      swap(data_file_path, sign_file_path);
    } else {
      data_file_path = info.path() + "/" + info.completeBaseName();
    }

    auto data_file_info = QFileInfo(data_file_path);
    if (!possible_singleton_target && !data_file_info.exists()) {
      bool ok;
      QString const text = QInputDialog::getText(
          this, tr("File to be Verified"),
          tr("Please provide An ABSOLUTE Path \n"
             "If Data And Signature is COMBINED within a single file, "
             "KEEP THIS EMPTY: "),
          QLineEdit::Normal, data_file_path, &ok);

      if (!ok) return;

      data_file_path = text.isEmpty() ? data_file_path : text;
      data_file_info = QFileInfo(data_file_path);
    }

    contexts->GetContextPath(0).append(sign_file_path);
    contexts->GetContextOutPath(0).append(data_file_path);
  }

  auto f_context = contexts->GetContext(0);
  if (f_context != nullptr) build_operas_file_verify(f_context);

  execute_operas_helper(tr("Verifying"), contexts);

  if (!contexts->unknown_fprs.isEmpty() &&
      Module::IsModuleActivate(kKeyServerSyncModuleID)) {
    slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
  }
}

void MainWindow::build_operas_file_encrypt_sign(
    QSharedPointer<GpgOperaContext>& context) {
  assert(context->paths.size() == context->o_paths.size());

  for (int i = 0; i < context->paths.size(); i++) {
    const auto& path = context->paths[i];
    const auto& o_path = context->o_paths[i];

    auto opera = [=, &opera_results =
                         context->opera_results](const OperaWaitingHd& op_hd) {
      GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .EncryptSignFile(
              context->keys, context->singer_keys, path, context->ascii, o_path,
              [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                    data_obj == nullptr ||
                    !data_obj->Check<GpgEncryptResult, GpgSignResult>()) {
                  QMessageBox::critical(this, tr("Error"),
                                        tr("Unknown error occurred"));
                  return;
                }
                auto encrypt_result =
                    ExtractParams<GpgEncryptResult>(data_obj, 0);
                auto sign_result = ExtractParams<GpgSignResult>(data_obj, 1);

                auto encrypt_result_analyse = GpgEncryptResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err,
                    encrypt_result);
                encrypt_result_analyse.Analyse();

                auto sign_result_analyse = GpgSignResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err,
                    sign_result);
                sign_result_analyse.Analyse();

                opera_results.append(
                    {std::min(encrypt_result_analyse.GetStatus(),
                              sign_result_analyse.GetStatus()),
                     encrypt_result_analyse.GetResultReport() +
                         sign_result_analyse.GetResultReport(),
                     QFileInfo(path).fileName()});
              });
    };

    context->operas.push_back(opera);
  }
}

void MainWindow::build_operas_directory_encrypt_sign(
    QSharedPointer<GpgOperaContext>& context) {
  assert(context->paths.size() == context->o_paths.size());

  for (int i = 0; i < context->paths.size(); i++) {
    const auto& path = context->paths[i];
    const auto& o_path = context->o_paths[i];

    auto opera = [=, &opera_results =
                         context->opera_results](const OperaWaitingHd& op_hd) {
      GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .EncryptSignDirectory(
              context->keys, context->singer_keys, path, context->ascii, o_path,
              [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                    data_obj == nullptr ||
                    !data_obj->Check<GpgEncryptResult, GpgSignResult>()) {
                  QMessageBox::critical(this, tr("Error"),
                                        tr("Unknown error occurred"));
                  return;
                }
                auto encrypt_result =
                    ExtractParams<GpgEncryptResult>(data_obj, 0);
                auto sign_result = ExtractParams<GpgSignResult>(data_obj, 1);

                auto encrypt_result_analyse = GpgEncryptResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err,
                    encrypt_result);
                encrypt_result_analyse.Analyse();

                auto sign_result_analyse = GpgSignResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err,
                    sign_result);
                sign_result_analyse.Analyse();

                opera_results.append(
                    {std::min(encrypt_result_analyse.GetStatus(),
                              sign_result_analyse.GetStatus()),
                     encrypt_result_analyse.GetResultReport() +
                         sign_result_analyse.GetResultReport(),
                     QFileInfo(path).fileName()});
              });
    };

    context->operas.push_back(opera);
  }
}

void MainWindow::SlotFileEncryptSign(const QStringList& paths) {
  auto contexts = QSharedPointer<GpgOperaContexts>::create();

  bool const non_ascii_at_file_operation =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("gnupg/non_ascii_at_file_operation", true)
          .toBool();

  contexts->ascii = !non_ascii_at_file_operation;

  auto key_ids = m_key_list_->GetChecked();

  contexts->keys = check_keys_helper(
      key_ids,
      [](const GpgKey& key) { return key.IsHasActualEncryptionCapability(); },
      tr("The selected keypair cannot be used for encryption."));
  if (contexts->keys.empty()) return;

  auto* signers_picker =
      new SignersPicker(m_key_list_->GetCurrentGpgContextChannel(), this);
  QEventLoop loop;
  connect(signers_picker, &SignersPicker::finished, &loop, &QEventLoop::quit);
  loop.exec();

  // return when canceled
  if (!signers_picker->GetStatus()) return;

  auto signer_key_ids = signers_picker->GetCheckedSigners();
  auto signer_keys =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKeys(signer_key_ids);
  assert(std::all_of(signer_keys.begin(), signer_keys.end(),
                     [](const auto& key) { return key.IsGood(); }));

  contexts->singer_keys = signer_keys;

  if (!check_read_file_paths_helper(paths)) return;

  for (const auto& path : paths) {
    QFileInfo info(path);
    if (info.isDir()) {
      contexts->GetContextPath(1).append(path);
      contexts->GetContextOutPath(1).append(
          SetExtensionOfOutputFileForArchive(path, kENCRYPT, contexts->ascii));
    } else {
      contexts->GetContextPath(0).append(path);
      contexts->GetContextOutPath(0).append(
          SetExtensionOfOutputFile(path, kENCRYPT, contexts->ascii));
    }
  }

  if (!check_write_file_paths_helper(contexts->GetAllOutPath())) return;

  auto f_context = contexts->GetContext(0);
  if (f_context != nullptr) build_operas_file_encrypt_sign(f_context);

  auto d_context = contexts->GetContext(1);
  if (d_context != nullptr) build_operas_directory_encrypt_sign(d_context);

  execute_operas_helper(tr("Encrypting and Signing"), contexts);
}

void MainWindow::build_operas_file_decrypt_verify(
    QSharedPointer<GpgOperaContext>& context) {
  assert(context->paths.size() == context->o_paths.size());

  for (int i = 0; i < context->paths.size(); i++) {
    const auto& path = context->paths[i];
    const auto& o_path = context->o_paths[i];

    auto opera = [=, &opera_results =
                         context->opera_results](const OperaWaitingHd& op_hd) {
      GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .DecryptVerifyFile(
              path, o_path,
              [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                    data_obj == nullptr ||
                    !data_obj->Check<GpgDecryptResult, GpgVerifyResult>()) {
                  QMessageBox::critical(this, tr("Error"),
                                        tr("Unknown error occurred"));
                  return;
                }
                auto decrypt_result =
                    ExtractParams<GpgDecryptResult>(data_obj, 0);
                auto verify_result =
                    ExtractParams<GpgVerifyResult>(data_obj, 1);

                auto decrypt_result_analyse = GpgDecryptResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err,
                    decrypt_result);
                decrypt_result_analyse.Analyse();

                auto verify_result_analyse = GpgVerifyResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err,
                    verify_result);
                verify_result_analyse.Analyse();

                opera_results.append(
                    {std::min(decrypt_result_analyse.GetStatus(),
                              verify_result_analyse.GetStatus()),
                     decrypt_result_analyse.GetResultReport() +
                         verify_result_analyse.GetResultReport(),
                     QFileInfo(path).fileName()});

                context->unknown_fprs =
                    verify_result_analyse.GetUnknownSignatures();
              });
    };

    context->operas.push_back(opera);
  }
}

void MainWindow::build_operas_archive_decrypt_verify(
    QSharedPointer<GpgOperaContext>& context) {
  assert(context->paths.size() == context->o_paths.size());

  for (int i = 0; i < context->paths.size(); i++) {
    const auto& path = context->paths[i];
    const auto& o_path = context->o_paths[i];

    auto opera = [=, &opera_results =
                         context->opera_results](const OperaWaitingHd& op_hd) {
      GpgFileOpera::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .DecryptVerifyArchive(
              path, o_path,
              [=, &opera_results](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (CheckGpgError(err) == GPG_ERR_USER_1 ||
                    data_obj == nullptr ||
                    !data_obj->Check<GpgDecryptResult, GpgVerifyResult>()) {
                  QMessageBox::critical(this, tr("Error"),
                                        tr("Unknown error occurred"));
                  return;
                }
                auto decrypt_result =
                    ExtractParams<GpgDecryptResult>(data_obj, 0);
                auto verify_result =
                    ExtractParams<GpgVerifyResult>(data_obj, 1);

                auto decrypt_result_analyse = GpgDecryptResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err,
                    decrypt_result);
                decrypt_result_analyse.Analyse();

                auto verify_result_analyse = GpgVerifyResultAnalyse(
                    m_key_list_->GetCurrentGpgContextChannel(), err,
                    verify_result);
                verify_result_analyse.Analyse();

                opera_results.append(
                    {std::min(decrypt_result_analyse.GetStatus(),
                              verify_result_analyse.GetStatus()),
                     decrypt_result_analyse.GetResultReport() +
                         verify_result_analyse.GetResultReport(),
                     QFileInfo(path).fileName()});

                context->unknown_fprs =
                    verify_result_analyse.GetUnknownSignatures();
              });
    };

    context->operas.push_back(opera);
  }
}

void MainWindow::SlotFileDecryptVerify(const QStringList& paths) {
  auto contexts = QSharedPointer<GpgOperaContexts>::create();

  contexts->ascii = true;

  if (!check_read_file_paths_helper(paths)) return;

  for (const auto& path : paths) {
    QFileInfo info(path);
    const auto extension = info.completeSuffix();

    if (extension == "tar.gpg" || extension == "tar.asc") {
      contexts->GetContextPath(1).append(path);
      contexts->GetContextOutPath(1).append(
          SetExtensionOfOutputFileForArchive(path, kDECRYPT, contexts->ascii));
    } else {
      contexts->GetContextPath(0).append(path);
      contexts->GetContextOutPath(0).append(
          SetExtensionOfOutputFile(path, kDECRYPT, contexts->ascii));
    }
  }

  if (!check_write_file_paths_helper(contexts->GetAllOutPath())) return;

  auto f_context = contexts->GetContext(0);
  if (f_context != nullptr) build_operas_file_decrypt_verify(f_context);

  auto d_context = contexts->GetContext(1);
  if (d_context != nullptr) {
    build_operas_archive_decrypt_verify(d_context);
  }

  execute_operas_helper(tr("Decrypting and Verifying"), contexts);

  if (!contexts->unknown_fprs.isEmpty() &&
      Module::IsModuleActivate(kKeyServerSyncModuleID)) {
    slot_verifying_unknown_signature_helper(contexts->unknown_fprs);
  }
};

void MainWindow::SlotFileVerifyEML(const QString& path) {
  auto check_result = TargetFilePreCheck(path, true);
  if (!std::get<0>(check_result)) {
    QMessageBox::critical(this, tr("Error"),
                          tr("Cannot read from file: %1").arg(path));
    return;
  }

  QFileInfo file_info(path);
  if (file_info.size() > static_cast<qint64>(1024 * 1024 * 32)) {
    QMessageBox::warning(
        this, tr("EML File Too Large"),
        tr("The EML file \"%1\" is larger than 32MB and will not be opened.")
            .arg(file_info.fileName()));
    return;
  }

  QFile eml_file(path);
  if (!eml_file.open(QIODevice::ReadOnly)) return;
  auto buffer = eml_file.readAll();

  // LOG_D() << "EML BUFFER (FILE): " << buffer;

  slot_verify_email_by_eml_data(buffer);
}

}  // namespace GpgFrontend::UI
