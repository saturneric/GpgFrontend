/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "UserInterfaceUtils.h"

#include <QtNetwork>
#include <string>
#include <utility>
#include <vector>

#include "core/GpgConstants.h"
#include "core/common/CoreCommonUtil.h"
#include "core/function/CacheManager.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/FileOperator.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunner.h"
#include "core/thread/TaskRunnerGetter.h"
#include "dialog/gnupg/GnuPGControllerDialog.h"
#include "spdlog/spdlog.h"
#include "ui/SignalStation.h"
#include "ui/dialog/WaitingDialog.h"
#include "ui/struct/SettingsObject.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

std::unique_ptr<GpgFrontend::UI::CommonUtils>
    GpgFrontend::UI::CommonUtils::instance_ = nullptr;

void show_verify_details(QWidget *parent, InfoBoardWidget *info_board,
                         GpgError error, const GpgVerifyResult &verify_result) {
  // take out result
  info_board->ResetOptionActionsMenu();
  info_board->AddOptionalAction(_("Show Verify Details"), [=]() {
    VerifyDetailsDialog(parent, error, verify_result);
  });
}

void import_unknown_key_from_keyserver(
    QWidget *parent, const GpgVerifyResultAnalyse &verify_res) {
  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(
      parent, _("Public key not found locally"),
      _("There is no target public key content in local for GpgFrontend to "
        "gather enough information about this Signature. Do you want to "
        "import the public key from Keyserver now?"),
      QMessageBox::Yes | QMessageBox::No);
  if (reply == QMessageBox::Yes) {
    auto dialog = KeyServerImportDialog(true, parent);
    auto key_ids = std::make_unique<KeyIdArgsList>();
    auto *signature = verify_res.GetSignatures();
    while (signature != nullptr) {
      SPDLOG_DEBUG("signature fpr: {}", signature->fpr);
      key_ids->push_back(signature->fpr);
      signature = signature->next;
    }
    dialog.show();
    dialog.SlotImport(key_ids);
  }
}

void refresh_info_board(InfoBoardWidget *info_board, int status,
                        const std::string &report_text) {
  if (status < 0)
    info_board->SlotRefresh(QString::fromStdString(report_text),
                            INFO_ERROR_CRITICAL);
  else if (status > 0)
    info_board->SlotRefresh(QString::fromStdString(report_text), INFO_ERROR_OK);
  else
    info_board->SlotRefresh(QString::fromStdString(report_text),
                            INFO_ERROR_WARN);
}

void process_result_analyse(TextEdit *edit, InfoBoardWidget *info_board,
                            const GpgResultAnalyse &result_analyse) {
  info_board->AssociateTabWidget(edit->tab_widget_);
  refresh_info_board(info_board, result_analyse.GetStatus(),
                     result_analyse.GetResultReport());
}

void process_result_analyse(TextEdit *edit, InfoBoardWidget *info_board,
                            const GpgResultAnalyse &result_analyse_a,
                            const GpgResultAnalyse &result_analyse_b) {
  info_board->AssociateTabWidget(edit->tab_widget_);

  refresh_info_board(
      info_board,
      std::min(result_analyse_a.GetStatus(), result_analyse_b.GetStatus()),
      result_analyse_a.GetResultReport() + result_analyse_b.GetResultReport());
}

void process_operation(QWidget *parent, const std::string &waiting_title,
                       const Thread::Task::TaskRunnable func,
                       const Thread::Task::TaskCallback callback,
                       Thread::Task::DataObjectPtr data_object) {
  auto *dialog =
      new WaitingDialog(QString::fromStdString(waiting_title), parent);

  auto *process_task = new Thread::Task(std::move(func), waiting_title,
                                        data_object, std::move(callback));

  QApplication::connect(process_task, &Thread::Task::SignalTaskEnd, dialog,
                        &QDialog::close);
  QApplication::connect(process_task, &Thread::Task::SignalTaskEnd, dialog,
                        &QDialog::deleteLater);

  // a looper to wait for the operation
  QEventLoop looper;
  QApplication::connect(process_task, &Thread::Task::SignalTaskEnd, &looper,
                        &QEventLoop::quit);

  // post process task to task runner
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
      ->PostTask(process_task);

  // block until task finished
  // this is to keep reference vaild until task finished
  looper.exec();
}

CommonUtils *CommonUtils::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = std::make_unique<CommonUtils>();
  }
  return instance_.get();
}

CommonUtils::CommonUtils() : QWidget(nullptr) {
  connect(CoreCommonUtil::GetInstance(), &CoreCommonUtil::SignalGnupgNotInstall,
          this, &CommonUtils::SignalGnupgNotInstall);
  connect(this, &CommonUtils::SignalKeyStatusUpdated,
          SignalStation::GetInstance(),
          &SignalStation::SignalKeyDatabaseRefresh);
  connect(this, &CommonUtils::SignalKeyDatabaseRefreshDone,
          SignalStation::GetInstance(),
          &SignalStation::SignalKeyDatabaseRefreshDone);
  connect(this, &CommonUtils::SignalUserInputPassphraseDone,
          CoreSignalStation::GetInstance(),
          &CoreSignalStation::SignalUserInputPassphraseDone);

  // directly connect to SignalKeyStatusUpdated
  // to avoid the delay of signal emitting
  // when the key database is refreshed
  connect(SignalStation::GetInstance(),
          &SignalStation::SignalKeyDatabaseRefresh, this,
          &CommonUtils::slot_update_key_status);

  connect(CoreSignalStation::GetInstance(),
          &CoreSignalStation::SignalNeedUserInputPassphrase, this,
          &CommonUtils::slot_popup_passphrase_input_dialog);

  connect(this, &CommonUtils::SignalRestartApplication,
          SignalStation::GetInstance(),
          &SignalStation::SignalRestartApplication);

  connect(SignalStation::GetInstance(),
          &SignalStation::SignalRestartApplication, this,
          &CommonUtils::SlotRestartApplication);

  connect(this, &CommonUtils::SignalGnupgNotInstall, this, [=]() {
    QMessageBox msgBox;
    msgBox.setText(_("GnuPG Context Loading Failed"));
    msgBox.setInformativeText(
        _("Gnupg(gpg) is not installed correctly, please follow "
          "<a href='https://www.gpgfrontend.bktus.com/#/"
          "faq?id=how-to-deal-with-39env-loading-failed39'>this notes</a>"
          " in FAQ to install Gnupg and then open "
          "GpgFrontend. Or, you can open GnuPG Controller to set a custom "
          "GnuPG "
          "which GpgFrontend should use. Then, GpgFrontend will restart."));
    msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    int ret = msgBox.exec();

    switch (ret) {
      case QMessageBox::Open:
        (new GnuPGControllerDialog(this))->exec();
        // restart application when loop start
        application_need_to_restart_at_once_ = true;
        // restart application, core and ui
        emit SignalRestartApplication(DEEP_RESTART_CODE);
        break;
      case QMessageBox::Cancel:
        // close application
        emit SignalRestartApplication(0);
        break;
      default:
        // close application
        emit SignalRestartApplication(0);
        break;
    }
  });
}

void CommonUtils::SlotImportKeys(QWidget *parent,
                                 const std::string &in_buffer) {
  GpgImportInformation result = GpgKeyImportExporter::GetInstance().ImportKey(
      std::make_unique<ByteArray>(in_buffer));
  emit SignalKeyStatusUpdated();
  new KeyImportDetailDialog(result, false, parent);
}

void CommonUtils::SlotImportKeyFromFile(QWidget *parent) {
  QString file_name = QFileDialog::getOpenFileName(
      this, _("Open Key"), QString(),
      QString(_("Key Files")) + " (*.asc *.txt);;" + _("Keyring files") +
          " (*.gpg);;All Files (*)");
  if (!file_name.isNull()) {
    QByteArray key_buffer;
    if (!FileOperator::ReadFile(file_name, key_buffer)) {
      QMessageBox::critical(nullptr, _("File Open Failed"),
                            _("Failed to open file: ") + file_name);
      return;
    }
    SlotImportKeys(parent, key_buffer.toStdString());
  }
}

void CommonUtils::SlotImportKeyFromKeyServer(QWidget *parent) {
  auto dialog = new KeyServerImportDialog(false, parent);
  dialog->show();
}

void CommonUtils::SlotImportKeyFromClipboard(QWidget *parent) {
  QClipboard *cb = QApplication::clipboard();
  SlotImportKeys(parent,
                 cb->text(QClipboard::Clipboard).toUtf8().toStdString());
}

void CommonUtils::SlotExecuteCommand(
    const std::string &cmd, const QStringList &arguments,
    const std::function<void(QProcess *)> &interact_func) {
  QEventLoop looper;
  auto *cmd_process = new QProcess(&looper);
  cmd_process->setProcessChannelMode(QProcess::MergedChannels);

  connect(cmd_process,
          qOverload<int, QProcess::ExitStatus>(&QProcess::finished), &looper,
          &QEventLoop::quit);
  connect(cmd_process, &QProcess::errorOccurred, &looper, &QEventLoop::quit);
  connect(cmd_process, &QProcess::started,
          []() -> void { SPDLOG_DEBUG("process started"); });
  connect(cmd_process, &QProcess::readyReadStandardOutput,
          [interact_func, cmd_process]() { interact_func(cmd_process); });
  connect(cmd_process, &QProcess::errorOccurred, this,
          [=]() -> void { SPDLOG_ERROR("error in process"); });
  connect(cmd_process,
          qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
          [=](int, QProcess::ExitStatus status) {
            if (status == QProcess::NormalExit)
              SPDLOG_DEBUG("succeed in executing command: {}", cmd);
            else
              SPDLOG_WARN("error in executing command: {}", cmd);
          });

  cmd_process->setProgram(QString::fromStdString(cmd));
  cmd_process->setArguments(arguments);
  cmd_process->start();
  looper.exec();
}

void CommonUtils::SlotExecuteGpgCommand(
    const QStringList &arguments,
    const std::function<void(QProcess *)> &interact_func) {
  QEventLoop looper;
  auto dialog = new WaitingDialog(_("Processing"), nullptr);
  dialog->show();
  auto *gpg_process = new QProcess(&looper);
  gpg_process->setProcessChannelMode(QProcess::MergedChannels);

  connect(gpg_process,
          qOverload<int, QProcess::ExitStatus>(&QProcess::finished), &looper,
          &QEventLoop::quit);
  connect(gpg_process,
          qOverload<int, QProcess::ExitStatus>(&QProcess::finished), dialog,
          &WaitingDialog::deleteLater);
  connect(gpg_process, &QProcess::errorOccurred, &looper, &QEventLoop::quit);
  connect(gpg_process, &QProcess::started,
          []() -> void { SPDLOG_DEBUG("gpg process started"); });
  connect(gpg_process, &QProcess::readyReadStandardOutput,
          [interact_func, gpg_process]() { interact_func(gpg_process); });
  connect(gpg_process, &QProcess::errorOccurred, this, [=]() -> void {
    SPDLOG_ERROR("Error in Process");
    dialog->close();
    QMessageBox::critical(nullptr, _("Failure"),
                          _("Failed to execute command."));
  });
  connect(gpg_process,
          qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
          [=](int, QProcess::ExitStatus status) {
            dialog->close();
            if (status == QProcess::NormalExit)
              QMessageBox::information(nullptr, _("Success"),
                                       _("Succeed in executing command."));
            else
              QMessageBox::information(nullptr, _("Warning"),
                                       _("Finished executing command."));
          });

  gpg_process->setProgram(
      GpgContext::GetInstance().GetInfo(false).AppPath.c_str());
  gpg_process->setArguments(arguments);
  gpg_process->start();
  looper.exec();
  dialog->close();
  dialog->deleteLater();
}

void CommonUtils::SlotImportKeyFromKeyServer(
    const KeyIdArgsList &key_ids, const ImportCallbackFunctiopn &callback) {
  // target key server that we need to import key from it
  std::string target_keyserver;

  try {
    auto &settings = GlobalSettingStation::GetInstance().GetUISettings();
    SettingsObject key_server_json("key_server");

    // get key servers from settings
    const auto key_server_list =
        key_server_json.Check("server_list", nlohmann::json::array());
    if (key_server_list.empty()) {
      throw std::runtime_error("No key server configured");
    }

    const int target_key_server_index =
        key_server_json.Check("default_server", 0);
    if (target_key_server_index >= key_server_list.size()) {
      throw std::runtime_error("default_server index out of range");
    }
    target_keyserver =
        key_server_list[target_key_server_index].get<std::string>();

    SPDLOG_DEBUG("set target key server to default Key Server: {}",
                 target_keyserver);
  } catch (...) {
    SPDLOG_ERROR(_("Cannot read default_keyserver From Settings"));
    QMessageBox::critical(nullptr, _("Default Keyserver Not Found"),
                          _("Cannot read default keyserver from your settings, "
                            "please set a default keyserver first"));
    return;
  }

  auto thread = QThread::create([target_keyserver, key_ids, callback]() {
    QUrl target_keyserver_url(target_keyserver.c_str());

    auto network_manager = std::make_unique<QNetworkAccessManager>();
    // LOOP
    decltype(key_ids.size()) current_index = 1, all_index = key_ids.size();
    for (const auto &key_id : key_ids) {
      // New Req Url
      QUrl req_url(
          target_keyserver_url.scheme() + "://" + target_keyserver_url.host() +
          "/pks/lookup?op=get&search=0x" + key_id.c_str() + "&options=mr");

      SPDLOG_DEBUG("request url: {}", req_url.toString().toStdString());

      // Waiting for reply
      QNetworkReply *reply = network_manager->get(QNetworkRequest(req_url));
      QEventLoop loop;
      connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
      loop.exec();

      // Get Data
      auto key_data = reply->readAll();
      auto key_data_ptr =
          std::make_unique<ByteArray>(key_data.data(), key_data.size());

      // Detect status
      std::string status;
      auto error = reply->error();
      if (error != QNetworkReply::NoError) {
        switch (error) {
          case QNetworkReply::ContentNotFoundError:
            status = _("Key Not Found");
            break;
          case QNetworkReply::TimeoutError:
            status = _("Timeout");
            break;
          case QNetworkReply::HostNotFoundError:
            status = _("Key Server Not Found");
            break;
          default:
            status = _("Connection Error");
        }
      }

      reply->deleteLater();

      // Try importing
      GpgImportInformation result =
          GpgKeyImportExporter::GetInstance().ImportKey(
              std::move(key_data_ptr));

      if (result.imported == 1) {
        status = _("The key has been updated");
      } else {
        status = _("No need to update the key");
      }
      callback(key_id, status, current_index, all_index);
      current_index++;
    }
  });
  connect(thread, &QThread::finished, thread, &QThread::deleteLater);
  thread->start();
}

void CommonUtils::slot_update_key_status() {
  auto refresh_task = new Thread::Task(
      [](Thread::Task::DataObjectPtr) -> int {
        // flush key cache for all GpgKeyGetter Intances.
        for (const auto &channel_id : GpgKeyGetter::GetAllChannelId()) {
          GpgKeyGetter::GetInstance(channel_id).FlushKeyCache();
        }
        return 0;
      },
      "update_key_database_task");
  connect(refresh_task, &Thread::Task::SignalTaskEnd, this,
          &CommonUtils::SignalKeyDatabaseRefreshDone,
          Qt::BlockingQueuedConnection);

  // post the task to the default task runner
  Thread::TaskRunnerGetter::GetInstance().GetTaskRunner()->PostTask(
      refresh_task);
}

void CommonUtils::slot_popup_passphrase_input_dialog() {
  auto *dialog = new QInputDialog(QApplication::activeWindow(), Qt::Dialog);
  dialog->setModal(true);
  dialog->setWindowTitle(_("Password Input Dialog"));
  dialog->setInputMode(QInputDialog::TextInput);
  dialog->setTextEchoMode(QLineEdit::Password);
  dialog->setLabelText(_("Please Input The Password"));
  dialog->resize(500, 80);
  dialog->exec();

  QString password = dialog->textValue();
  dialog->deleteLater();

  // send signal
  emit SignalUserInputPassphraseDone(password);
}

void CommonUtils::SlotRestartApplication(int code) {
  SPDLOG_DEBUG("application need restart, code: {}", code);

  if (code == 0) {
    std::exit(0);
  } else {
    QCoreApplication::exit(code);
  }
}

bool CommonUtils::isApplicationNeedRestart() {
  return application_need_to_restart_at_once_;
}

bool CommonUtils::KeyExistsinFavouriteList(const GpgKey &key) {
  // load cache
  auto key_array = CacheManager::GetInstance().LoadCache("favourite_key_pair");
  if (!key_array.is_array()) {
    CacheManager::GetInstance().SaveCache("favourite_key_pair",
                                          nlohmann::json::array());
  }
  return std::find(key_array.begin(), key_array.end(), key.GetFingerprint()) !=
         key_array.end();
}

void CommonUtils::AddKey2Favourtie(const GpgKey &key) {
  auto key_array = CacheManager::GetInstance().LoadCache("favourite_key_pair");
  if (!key_array.is_array()) {
    CacheManager::GetInstance().SaveCache("favourite_key_pair",
                                          nlohmann::json::array());
  }
  key_array.push_back(key.GetFingerprint());
  CacheManager::GetInstance().SaveCache("favourite_key_pair", key_array, true);
}

void CommonUtils::RemoveKeyFromFavourite(const GpgKey &key) {
  auto key_array = CacheManager::GetInstance().LoadCache("favourite_key_pair");
  if (!key_array.is_array()) {
    CacheManager::GetInstance().SaveCache("favourite_key_pair",
                                          nlohmann::json::array(), true);
    return;
  }
  auto it = std::find(key_array.begin(), key_array.end(), key.GetFingerprint());
  if (it != key_array.end()) {
    auto rm_it =
        std::remove(key_array.begin(), key_array.end(), key.GetFingerprint());
    key_array.erase(rm_it, key_array.end());
    CacheManager::GetInstance().SaveCache("favourite_key_pair", key_array);
  }
}

}  // namespace GpgFrontend::UI