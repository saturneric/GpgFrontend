/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include <boost/format.hpp>

#include "MainWindow.h"
#include "core/function/ArchiveFileOperator.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgFileOpera.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/thread/Task.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/SignersPicker.h"

namespace GpgFrontend::UI {

auto PathPreCheck(QWidget* parent, const std::filesystem::path& path) -> bool {
  QFileInfo const file_info(path);
  QFileInfo const path_info(file_info.absolutePath());

  if (!path_info.exists()) {
    QMessageBox::critical(
        parent, _("Error"),
        QString(_("The path %1 does not exist.")).arg(path.c_str()));
    return false;
  }
  if (!file_info.isReadable()) {
    QMessageBox::critical(parent, _("Error"),
                          _("No permission to read this file."));
    return false;
  }
  if (!path_info.isWritable()) {
    QMessageBox::critical(parent, _("Error"),
                          _("No permission to create file."));
    return false;
  }
  return true;
}

/**
 * @brief convert directory into tarball
 *
 * @param parent parent widget
 * @param path the directory to be converted
 * @return
 */
auto ProcessTarballIntoDirectory(QWidget* parent,
                                 const std::filesystem::path& path)
    -> std::tuple<bool, std::filesystem::path> {
  SPDLOG_DEBUG("converting directory into tarball: {}", path.u8string());
  auto selected_dir_path = std::filesystem::path(path);

  if (selected_dir_path.extension() != ".tar") {
    QMessageBox::critical(parent, _("Error"), _("The file is not a tarball."));
    return {false, path};
  }

  try {
    auto base_path = selected_dir_path.parent_path();

    auto target_path = selected_dir_path;
    target_path.replace_extension(".tar");

    SPDLOG_DEBUG("base path: {} target archive path: {]", base_path.u8string(),
                 target_path.u8string());

    bool if_error = false;
    process_operation(parent, _("Extracting Tarball"),
                      [&](DataObjectPtr) -> int {
                        try {
                          GpgFrontend::ArchiveFileOperator::ExtractArchive(
                              target_path, base_path);
                        } catch (const std::runtime_error& e) {
                          if_error = true;
                        }
                        return 0;
                      });

    if (if_error || !exists(target_path)) {
      throw std::runtime_error("Decompress Failed");
    }
    return {true, target_path.u8string()};
  } catch (...) {
    SPDLOG_ERROR("decompress error");
    return {false, path};
  }
}

/**
 * @brief convert tarball into directory
 *
 * @param parent parent widget
 * @param path the tarball to be converted
 */
auto ProcessDirectoryIntoTarball(QWidget* parent, std::filesystem::path path)
    -> bool {
  try {
    auto base_path = path.parent_path();
    auto target_path = path;
    path = path.replace_extension("");

    SPDLOG_DEBUG("base path: {} target archive path: {} selected_dir_path: {}",
                 base_path.u8string(), target_path.u8string(), path.u8string());

    bool if_error = false;
    process_operation(parent, _("Making Tarball"), [&](DataObjectPtr) -> int {
      try {
        GpgFrontend::ArchiveFileOperator::CreateArchive(base_path, target_path,
                                                        0, {path});
      } catch (const std::runtime_error& e) {
        if_error = true;
      }
      return 0;
    });

    if (if_error || !exists(target_path)) {
      throw std::runtime_error("Compress Failed");
    }
    path = target_path.u8string().c_str();
  } catch (...) {
    SPDLOG_ERROR("compress error");
    return false;
  }
  return true;
}

void MainWindow::SlotFileEncrypt() {
  auto* file_tree_view = edit_->SlotCurPageFileTreeView();
  auto path_qstr = file_tree_view->GetSelected();

  auto path = ConvertPathByOS(path_qstr);
  if (!PathPreCheck(this, path)) {
    SPDLOG_ERROR("path pre check failed");
    return;
  }

  // check selected keys
  auto key_ids = m_key_list_->GetChecked();
  bool const non_ascii_when_export =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.non_ascii_when_export", true);

  // get file info
  QFileInfo const file_info(path);
  if (file_info.isDir()) {
    path = path.replace_extension(".tar");
  }

  const auto* extension = ".asc";
  if (non_ascii_when_export || file_info.isDir()) {
    extension = ".gpg";
  }

  auto out_path = path.replace_extension(path.extension().string() + extension);
  if (QFile::exists(out_path)) {
    auto out_file_name = boost::format(_("The target file %1% already exists, "
                                         "do you need to overwrite it?")) %
                         out_path.filename();
    auto ret =
        QMessageBox::warning(this, _("Warning"), out_file_name.str().c_str(),
                             QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  if (file_info.isDir()) {
    // stop if the process making tarball failed
    if (!ProcessDirectoryIntoTarball(this, path)) {
      QMessageBox::critical(this, _("Error"),
                            _("Unable to convert the folder into tarball."));
      return;
    }
  }

  if (key_ids->empty()) {
    // Symmetric Encrypt
    auto ret = QMessageBox::information(
        this, _("Symmetric Encryption"),
        _("No Key Selected. Do you want to encrypt with a "
          "symmetric cipher using a passphrase?"),
        QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Cancel) return;

    CommonUtils::WaitForOpera(
        this, _("Symmetrically Encrypting"), [=](const OperaWaitingHd& op_hd) {
          GpgFileOpera::EncryptFileSymmetric(
              path, !non_ascii_when_export, out_path,
              [=](GpgError err, const DataObjectPtr& data_obj) {
                // stop waiting
                op_hd();

                if (data_obj == nullptr ||
                    !data_obj->Check<GpgEncryptResult>()) {
                  throw std::runtime_error("data object doesn't pass checking");
                }

                auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
                auto result_analyse = GpgEncryptResultAnalyse(err, result);
                result_analyse.Analyse();

                process_result_analyse(edit_, info_board_, result_analyse);
                file_tree_view->update();

                // remove xxx.tar and only left xxx.tar.gpg
                if (file_info.isDir()) {
                  auto selected_dir_path = path;
                  auto target_path =
                      selected_dir_path.replace_extension(".tar");
                  if (exists(target_path)) {
                    std::filesystem::remove(target_path);
                  }
                }
              });
        });

    return;
  }
  auto p_keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  // check key abilities
  for (const auto& key : *p_keys) {
    bool const key_can_encrypt = key.IsHasActualEncryptionCapability();

    if (!key_can_encrypt) {
      QMessageBox::critical(
          nullptr, _("Invalid KeyPair"),
          QString(_("The selected keypair cannot be used for encryption.")) +
              "<br/><br/>" + _("For example the Following Key:") + " <br/>" +
              QString::fromStdString(key.GetUIDs()->front().GetUID()));
      return;
    }
  }

  CommonUtils::WaitForOpera(
      this, _("Encrypting"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::EncryptFile(
            {p_keys->begin(), p_keys->end()}, path, !non_ascii_when_export,
            out_path, [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr || !data_obj->Check<GpgEncryptResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }

              auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
              auto result_analyse = GpgEncryptResultAnalyse(err, result);
              result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, result_analyse);
              file_tree_view->update();

              // remove xxx.tar and only left xxx.tar.gpg
              if (file_info.isDir()) {
                auto selected_dir_path = path;
                auto target_path = selected_dir_path.replace_extension(".tar");
                if (exists(target_path)) {
                  std::filesystem::remove(target_path);
                }
              }
            });
      });
}

void MainWindow::SlotFileDecrypt() {
  auto* file_tree_view = edit_->SlotCurPageFileTreeView();
  auto path_qstr = file_tree_view->GetSelected();
  auto path = ConvertPathByOS(path_qstr);
  if (!PathPreCheck(this, path)) return;

  auto out_path = path;
  if (out_path.extension() == ".asc" || out_path.extension() == ".gpg") {
    out_path = out_path.parent_path() / out_path.stem();
  } else {
    out_path += ".out";
  }

  if (exists(out_path)) {
    auto ret = QMessageBox::warning(
        this, _("Warning"),
        _("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  CommonUtils::WaitForOpera(
      this, _("Decrypting"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::DecryptFile(
            path, out_path, [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr || !data_obj->Check<GpgDecryptResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }

              auto result = ExtractParams<GpgDecryptResult>(data_obj, 0);

              auto result_analyse = GpgDecryptResultAnalyse(err, result);
              result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, result_analyse);
              file_tree_view->update();

              // extract the tarball
              if (out_path.extension() == ".tar" && exists(out_path)) {
                bool const ret =
                    QMessageBox::information(
                        this, _("Decrypting"),
                        _("Do you want to extract and delete the decrypted "
                          "tarball?"),
                        QMessageBox::Ok | QMessageBox::Cancel) != 0;
                if (ret) {
                  auto archieve_result =
                      ProcessTarballIntoDirectory(this, out_path);
                  if (std::get<0>(archieve_result)) {
                    QMessageBox::information(
                        this, _("Decrypting"),
                        _("Extracting tarball succeeded."));
                    // remove tarball
                    std::filesystem::remove(out_path);
                  } else {
                    QMessageBox::critical(this, _("Decrypting"),
                                          _("Extracting tarball failed."));
                  }
                }
              }
            });
      });
}

void MainWindow::SlotFileSign() {
  auto* file_tree_view = edit_->SlotCurPageFileTreeView();
  auto path_qstr = file_tree_view->GetSelected();
  auto path = ConvertPathByOS(path_qstr);
  if (!PathPreCheck(this, path)) return;

  auto key_ids = m_key_list_->GetChecked();
  auto keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  if (keys->empty()) {
    QMessageBox::critical(
        this, _("No Key Checked"),
        _("Please check the key in the key toolbox on the right."));
    return;
  }

  for (const auto& key : *keys) {
    if (!key.IsHasActualSigningCapability()) {
      QMessageBox::information(
          this, _("Invalid Operation"),
          QString(_("The selected key contains a key that does not actually "
                    "have a sign usage.")) +
              "<br/><br/>" + _("for example the Following Key:") + " <br/>" +
              QString::fromStdString(key.GetUIDs()->front().GetUID()));
      return;
    }
  }

  bool const non_ascii_when_export =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.non_ascii_when_export", true);

  const auto* extension = ".asc";
  if (non_ascii_when_export) {
    extension = ".sig";
  }

  auto sig_file_path = path;
  sig_file_path += extension;
  if (exists(sig_file_path)) {
    auto ret = QMessageBox::warning(
        this, _("Warning"),
        QString(_("The signature file \"%1\" exists, "
                  "do you need to overwrite it?"))
            .arg(sig_file_path.filename().u8string().c_str()),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  CommonUtils::WaitForOpera(
      this, _("Signing"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::EncryptFile(
            {keys->begin(), keys->end()}, path, !non_ascii_when_export,
            sig_file_path, [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr || !data_obj->Check<GpgSignResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }

              auto result = ExtractParams<GpgSignResult>(data_obj, 0);
              auto result_analyse = GpgSignResultAnalyse(err, result);
              result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, result_analyse);
              file_tree_view->update();
            });
      });
}

void MainWindow::SlotFileVerify() {
  auto* file_tree_view = edit_->SlotCurPageFileTreeView();
  auto path = file_tree_view->GetSelected();

#ifdef WINDOWS
  std::filesystem::path in_path = path.toStdU16String();
#else
  std::filesystem::path in_path = path.toStdString();
#endif

  std::filesystem::path sign_file_path = in_path;
  std::filesystem::path data_file_path;

  if (in_path.extension() == ".gpg") {
    swap(data_file_path, sign_file_path);
  } else if (in_path.extension() == ".sig" || in_path.extension() == ".asc") {
    data_file_path = sign_file_path.parent_path() / sign_file_path.stem();
  }

  SPDLOG_DEBUG("sign_file_path: {} {}", sign_file_path.u8string(),
               sign_file_path.extension().u8string());

  if (in_path.extension() != ".gpg") {
    bool ok;
    QString const text = QInputDialog::getText(
        this, _("Origin file to verify"), _("Filepath"), QLineEdit::Normal,
        data_file_path.u8string().c_str(), &ok);
    if (ok && !text.isEmpty()) {
      data_file_path = text.toStdString();
    } else {
      return;
    }
  }

  if (!is_regular_file(data_file_path) ||
      (!sign_file_path.empty() && !is_regular_file(sign_file_path))) {
    QMessageBox::critical(
        this, _("Error"),
        _("Please select the appropriate origin file or signature file. "
          "Ensure that both are in this directory."));
    return;
  }

  SPDLOG_DEBUG("data path: {}", data_file_path.u8string());
  SPDLOG_DEBUG("sign path: {}", sign_file_path.u8string());

  CommonUtils::WaitForOpera(
      this, _("Verifying"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::DecryptFile(
            data_file_path.u8string(), sign_file_path.u8string(),
            [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr || !data_obj->Check<GpgVerifyResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }

              auto result = ExtractParams<GpgVerifyResult>(data_obj, 0);
              auto result_analyse = GpgVerifyResultAnalyse(err, result);
              result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, result_analyse);
              if (result_analyse.GetStatus() == -2) {
                import_unknown_key_from_keyserver(this, result_analyse);
              }
              if (result_analyse.GetStatus() >= 0) {
                show_verify_details(this, info_board_, err, result);
              }

              file_tree_view->update();
            });
      });
}

void MainWindow::SlotFileEncryptSign() {
  auto* file_tree_view = edit_->SlotCurPageFileTreeView();
  auto path_qstr = file_tree_view->GetSelected();
  auto path = ConvertPathByOS(path_qstr);
  if (!PathPreCheck(this, path)) return;

  // check selected keys
  auto key_ids = m_key_list_->GetChecked();
  auto p_keys = GpgKeyGetter::GetInstance().GetKeys(key_ids);

  if (p_keys->empty()) {
    QMessageBox::critical(
        this, _("No Key Checked"),
        _("Please check the key in the key toolbox on the right."));
    return;
  }

  // check key abilities
  for (const auto& key : *p_keys) {
    bool const key_can_encrypt = key.IsHasActualEncryptionCapability();

    if (!key_can_encrypt) {
      QMessageBox::critical(
          nullptr, _("Invalid KeyPair"),
          QString(_("The selected keypair cannot be used for encryption.")) +
              "<br/><br/>" + _("For example the Following Key:") + " <br/>" +
              QString::fromStdString(key.GetUIDs()->front().GetUID()));
      return;
    }
  }

  bool const non_ascii_when_export =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.non_ascii_when_export", true);

  // get file info
  QFileInfo const file_info(path);
  if (file_info.isDir()) {
    path = path.replace_extension(".tar");
  }

  const auto* extension = ".asc";
  if (non_ascii_when_export || file_info.isDir()) {
    extension = ".gpg";
  }

  auto out_path = path.replace_extension(path.extension().string() + extension);
  if (QFile::exists(out_path)) {
    auto ret = QMessageBox::warning(
        this, _("Warning"),
        _("The target file already exists, do you need to overwrite it?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  auto* signers_picker = new SignersPicker(this);
  QEventLoop loop;
  connect(signers_picker, &SignersPicker::finished, &loop, &QEventLoop::quit);
  loop.exec();

  // return when canceled
  if (!signers_picker->GetStatus()) return;

  auto signer_key_ids = signers_picker->GetCheckedSigners();
  auto p_signer_keys = GpgKeyGetter::GetInstance().GetKeys(signer_key_ids);

  // convert directory into tarball
  if (file_info.isDir()) {
    // stop if the process making tarball failed
    if (!ProcessDirectoryIntoTarball(this, path)) {
      QMessageBox::critical(this, _("Error"),
                            _("Unable to convert the folder into tarball."));
      return;
    }
  }

  CommonUtils::WaitForOpera(
      this, _("Encrypting and Signing"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::EncryptSignFile(
            {p_keys->begin(), p_keys->end()},
            {p_signer_keys->begin(), p_signer_keys->end()}, path,
            !non_ascii_when_export, out_path,
            [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr ||
                  !data_obj->Check<GpgEncryptResult, GpgSignResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }
              auto encrypt_result =
                  ExtractParams<GpgEncryptResult>(data_obj, 0);
              auto sign_result = ExtractParams<GpgSignResult>(data_obj, 1);

              auto encrypt_result_analyse =
                  GpgEncryptResultAnalyse(err, encrypt_result);
              encrypt_result_analyse.Analyse();

              auto sign_result_analyse = GpgSignResultAnalyse(err, sign_result);
              sign_result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, encrypt_result_analyse,
                                     sign_result_analyse);

              file_tree_view->update();

              // remove xxx.tar and only left xxx.tar.gpg
              if (file_info.isDir()) {
                auto selected_dir_path = path;
                auto target_path = selected_dir_path.replace_extension(".tar");
                if (exists(target_path)) {
                  std::filesystem::remove(target_path);
                }
              }
            });
      });
}

void MainWindow::SlotFileDecryptVerify() {
  auto* file_tree_view = edit_->SlotCurPageFileTreeView();
  auto path_qstr = file_tree_view->GetSelected();
  auto path = ConvertPathByOS(path_qstr);
  if (!PathPreCheck(this, path)) return;

  std::filesystem::path out_path = path;
  if (path.extension() == ".asc" || path.extension() == ".gpg") {
    out_path = path.parent_path() / out_path.stem();
  } else {
    out_path += ".out";
  }
  SPDLOG_DEBUG("out path: {}", out_path.u8string());

  if (QFile::exists(out_path.u8string().c_str())) {
    auto ret =
        QMessageBox::warning(this, _("Warning"),
                             QString(_("The output file %1 already exists, do "
                                       "you need to overwrite it?"))
                                 .arg(out_path.filename().u8string().c_str()),
                             QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) return;
  }

  CommonUtils::WaitForOpera(
      this, _("Decrypting and Verifying"), [=](const OperaWaitingHd& op_hd) {
        GpgFileOpera::DecryptVerifyFile(
            path, out_path, [=](GpgError err, const DataObjectPtr& data_obj) {
              // stop waiting
              op_hd();

              if (data_obj == nullptr ||
                  !data_obj->Check<GpgDecryptResult, GpgVerifyResult>()) {
                throw std::runtime_error("data object doesn't pass checking");
              }
              auto decrypt_result =
                  ExtractParams<GpgDecryptResult>(data_obj, 0);
              auto verify_result = ExtractParams<GpgVerifyResult>(data_obj, 1);

              auto decrypt_result_analyse =
                  GpgDecryptResultAnalyse(err, decrypt_result);
              decrypt_result_analyse.Analyse();

              auto verify_result_analyse =
                  GpgVerifyResultAnalyse(err, verify_result);
              verify_result_analyse.Analyse();

              process_result_analyse(edit_, info_board_, decrypt_result_analyse,
                                     verify_result_analyse);
              if (verify_result_analyse.GetStatus() == -2) {
                import_unknown_key_from_keyserver(this, verify_result_analyse);
              }
              if (verify_result_analyse.GetStatus() >= 0) {
                show_verify_details(this, info_board_, err, verify_result);
              }
              file_tree_view->update();

              // extract the tarball
              if (out_path.extension() == ".tar" && exists(out_path)) {
                bool const ret =
                    QMessageBox::information(
                        this, _("Decrypting"),
                        _("Do you want to extract and delete the decrypted "
                          "tarball?"),
                        QMessageBox::Ok | QMessageBox::Cancel) != 0;
                if (ret) {
                  auto archieve_result =
                      ProcessTarballIntoDirectory(this, out_path);
                  if (std::get<0>(archieve_result)) {
                    QMessageBox::information(
                        this, _("Decrypting"),
                        _("Extracting tarball succeeded."));
                    // remove tarball
                    std::filesystem::remove(out_path);
                  } else {
                    QMessageBox::critical(this, _("Decrypting"),
                                          _("Extracting tarball failed."));
                  }
                }
              }
            });
      });
}

}  // namespace GpgFrontend::UI
