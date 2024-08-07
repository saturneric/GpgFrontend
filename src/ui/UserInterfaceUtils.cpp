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

#include "UserInterfaceUtils.h"

#include <cstddef>

#include "core/GpgConstants.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/model/CacheObject.h"
#include "core/model/GpgImportInformation.h"
#include "core/model/SettingsObject.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "thread/KeyServerImportTask.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/WaitingDialog.h"
#include "ui/dialog/controller/GnuPGControllerDialog.h"
#include "ui/dialog/import_export/KeyServerImportDialog.h"
#include "ui/struct/settings_object/KeyServerSO.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

std::unique_ptr<GpgFrontend::UI::CommonUtils>
    GpgFrontend::UI::CommonUtils::instance_ = nullptr;

void show_verify_details(QWidget *parent, InfoBoardWidget *info_board,
                         GpgError error, const GpgVerifyResult &verify_result) {
  // take out result
  info_board->ResetOptionActionsMenu();
  info_board->AddOptionalAction(
      QCoreApplication::tr("Show Verify Details"),
      [=]() { VerifyDetailsDialog(parent, error, verify_result); });
}

void ImportUnknownKeyFromKeyserver(
    QWidget *parent, const GpgVerifyResultAnalyse &verify_result) {
  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(
      parent, QCoreApplication::tr("Public key not found locally"),
      QCoreApplication::tr(
          "There is no target public key content in local for GpgFrontend to "
          "gather enough information about this Signature. Do you want to "
          "import the public key from Keyserver now?"),
      QMessageBox::Yes | QMessageBox::No);
  if (reply == QMessageBox::Yes) {
    auto dialog = KeyServerImportDialog(parent);
    auto key_ids = std::make_unique<KeyIdArgsList>();
    auto *signature = verify_result.GetSignatures();
    while (signature != nullptr) {
      key_ids->push_back(signature->fpr);
      signature = signature->next;
    }
    dialog.show();
    dialog.SlotImport(key_ids);
  }
}

void refresh_info_board(InfoBoardWidget *info_board, int status,
                        const QString &report_text) {
  if (status < 0) {
    info_board->SlotRefresh(report_text, INFO_ERROR_CRITICAL);
  } else if (status > 0) {
    info_board->SlotRefresh(report_text, INFO_ERROR_OK);
  } else {
    info_board->SlotRefresh(report_text, INFO_ERROR_WARN);
  }
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

void process_operation(QWidget *parent, const QString &waiting_title,
                       const Thread::Task::TaskRunnable func,
                       const Thread::Task::TaskCallback callback,
                       DataObjectPtr data_object) {
  auto *dialog = new WaitingDialog(waiting_title, parent);

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

auto CommonUtils::GetInstance() -> CommonUtils * {
  if (instance_ == nullptr) {
    instance_ = std::make_unique<CommonUtils>();
  }
  return instance_.get();
}

CommonUtils::CommonUtils() : QWidget(nullptr) {
  connect(CoreSignalStation::GetInstance(),
          &CoreSignalStation::SignalBadGnupgEnv, this,
          &CommonUtils::SignalBadGnupgEnv);
  connect(this, &CommonUtils::SignalKeyStatusUpdated,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);
  connect(this, &CommonUtils::SignalKeyDatabaseRefreshDone,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone);

  // directly connect to SignalKeyStatusUpdated
  // to avoid the delay of signal emitting
  // when the key database is refreshed
  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh, this,
          &CommonUtils::slot_update_key_status);

  connect(this, &CommonUtils::SignalRestartApplication,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalRestartApplication);

  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalRestartApplication, this,
          &CommonUtils::SlotRestartApplication);

  connect(this, &CommonUtils::SignalBadGnupgEnv, this,
          [=](const QString &reason) {
            QMessageBox msg_box;
            msg_box.setText(tr("GnuPG Context Loading Failed"));
            msg_box.setInformativeText(
                tr("Gnupg(gpg) is not installed correctly, please follow "
                   "<a href='https://www.gpgfrontend.bktus.com/#/"
                   "faq?id=how-to-deal-with-39env-loading-failed39'>this "
                   "notes</a> in FAQ to install Gnupg and then open "
                   "GpgFrontend. <br />"
                   "Or, you can open GnuPG Controller to set a "
                   "custom GnuPG which GpgFrontend should use. Then, "
                   "GpgFrontend will restart. <br /><br />"
                   "Breif Reason: %1")
                    .arg(reason));
            msg_box.setStandardButtons(QMessageBox::Open | QMessageBox::Cancel);
            msg_box.setDefaultButton(QMessageBox::Save);
            int ret = msg_box.exec();

            switch (ret) {
              case QMessageBox::Open:
                (new GnuPGControllerDialog(this))->exec();
                // restart application when loop start
                application_need_to_restart_at_once_ = true;
                // restart application, core and ui
                emit SignalRestartApplication(kDeepRestartCode);
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

void CommonUtils::WaitForOpera(QWidget *parent,
                               const QString &waiting_dialog_title,
                               const OperaWaitingCb &opera) {
  QEventLoop looper;
  QPointer<WaitingDialog> const dialog =
      new WaitingDialog(waiting_dialog_title, parent);
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

void CommonUtils::RaiseMessageBox(QWidget *parent, GpgError err) {
  GpgErrorDesc desc = DescribeGpgErrCode(err);
  GpgErrorCode err_code = CheckGpgError2ErrCode(err);

  if (err_code == GPG_ERR_NO_ERROR) {
    QMessageBox::information(parent, tr("Success"),
                             tr("Gpg Operation succeed."));
  } else {
    RaiseFailureMessageBox(parent, err);
  }
}

void CommonUtils::RaiseFailureMessageBox(QWidget *parent, GpgError err) {
  GpgErrorDesc desc = DescribeGpgErrCode(err);
  GpgErrorCode err_code = CheckGpgError2ErrCode(err);

  QMessageBox::critical(parent, tr("Failure"),
                        tr("Gpg Operation failed.\n\nError code: %1\nSource: "
                           " %2\nDescription: %3")
                            .arg(err_code)
                            .arg(desc.first)
                            .arg(desc.second));
}

void CommonUtils::SlotImportKeys(QWidget *parent, const QByteArray &in_buffer) {
  auto info =
      GpgKeyImportExporter::GetInstance().ImportKey(GFBuffer(in_buffer));
  emit SignalKeyStatusUpdated();

  (new KeyImportDetailDialog(info, parent));
}

void CommonUtils::SlotImportKeyFromFile(QWidget *parent) {
  auto file_name =
      QFileDialog::getOpenFileName(parent, tr("Open Key"), QString(),
                                   tr("Keyring files") + " (*.asc *.gpg)");
  if (file_name.isEmpty()) return;

  QFileInfo file_info(file_name);

  if (!file_info.isFile() || !file_info.isReadable()) {
    QMessageBox::critical(
        parent, tr("Error"),
        tr("Cannot open this file. Please make sure that this "
           "is a regular file and it's readable."));
    return;
  }

  if (file_info.size() > static_cast<qint64>(1024 * 1024)) {
    QMessageBox::critical(parent, tr("Error"),
                          tr("The target file is too large for a keyring."));
    return;
  }

  QByteArray key_buffer;
  if (!ReadFile(file_name, key_buffer)) {
    QMessageBox::critical(nullptr, tr("File Open Failed"),
                          tr("Failed to open file: ") + file_name);
    return;
  }
  SlotImportKeys(parent, key_buffer);
}

void CommonUtils::SlotImportKeyFromKeyServer(QWidget *parent) {
  auto *dialog = new KeyServerImportDialog(parent);
  dialog->show();
}

void CommonUtils::SlotImportKeyFromClipboard(QWidget *parent) {
  QClipboard *cb = QApplication::clipboard();
  SlotImportKeys(parent, cb->text(QClipboard::Clipboard).toLatin1());
}

void CommonUtils::SlotExecuteCommand(
    const QString &cmd, const QStringList &arguments,
    const std::function<void(QProcess *)> &interact_func) {
  QEventLoop looper;
  auto *cmd_process = new QProcess(&looper);
  cmd_process->setProcessChannelMode(QProcess::MergedChannels);

  connect(cmd_process,
          qOverload<int, QProcess::ExitStatus>(&QProcess::finished), &looper,
          &QEventLoop::quit);
  connect(cmd_process, &QProcess::errorOccurred, &looper, &QEventLoop::quit);
  connect(cmd_process, &QProcess::started,
          []() -> void { FLOG_D("process started"); });
  connect(cmd_process, &QProcess::readyReadStandardOutput,
          [interact_func, cmd_process]() { interact_func(cmd_process); });
  connect(cmd_process, &QProcess::errorOccurred, this,
          [=]() -> void { FLOG_W("error in process"); });
  connect(cmd_process,
          qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
          [=](int, QProcess::ExitStatus status) {
            if (status != QProcess::NormalExit) {
              LOG_W() << "error in executing command: " << cmd;
            }
          });

  cmd_process->setProgram(cmd);
  cmd_process->setArguments(arguments);
  cmd_process->start();
  looper.exec();
}

void CommonUtils::SlotExecuteGpgCommand(
    const QStringList &arguments,
    const std::function<void(QProcess *)> &interact_func) {
  QEventLoop looper;
  auto *dialog = new WaitingDialog(tr("Processing"), nullptr);
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
          []() -> void { FLOG_D("gpg process started"); });
  connect(gpg_process, &QProcess::readyReadStandardOutput,
          [interact_func, gpg_process]() { interact_func(gpg_process); });
  connect(gpg_process, &QProcess::errorOccurred, this, [=]() -> void {
    FLOG_W("Error in Process");
    dialog->close();
    QMessageBox::critical(nullptr, tr("Failure"),
                          tr("Failed to execute command."));
  });
  connect(gpg_process,
          qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
          [=](int, QProcess::ExitStatus status) {
            dialog->close();
            if (status == QProcess::NormalExit)
              QMessageBox::information(nullptr, tr("Success"),
                                       tr("Succeed in executing command."));
            else
              QMessageBox::information(nullptr, tr("Warning"),
                                       tr("Finished executing command."));
          });

  const auto app_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.app_path", QString{});

  gpg_process->setProgram(app_path);
  gpg_process->setArguments(arguments);
  gpg_process->start();
  looper.exec();
  dialog->close();
  dialog->deleteLater();
}

void CommonUtils::SlotImportKeyFromKeyServer(
    const KeyIdArgsList &key_ids, const ImportCallbackFunctiopn &callback) {
  auto target_keyserver =
      KeyServerSO(SettingsObject("key_server")).GetTargetServer();
  if (target_keyserver.isEmpty()) {
    QMessageBox::critical(
        nullptr, tr("Default Keyserver Not Found"),
        tr("Cannot read default keyserver from your settings, "
           "please set a default keyserver first"));
    return;
  }

  auto *thread = QThread::create([target_keyserver, key_ids, callback]() {
    QUrl target_keyserver_url(target_keyserver);

    auto network_manager = std::make_unique<QNetworkAccessManager>();
    // LOOP
    decltype(key_ids.size()) current_index = 1;
    decltype(key_ids.size()) all_index = key_ids.size();

    for (const auto &key_id : key_ids) {
      // New Req Url
      QUrl req_url(target_keyserver_url.scheme() + "://" +
                   target_keyserver_url.host() +
                   "/pks/lookup?op=get&search=0x" + key_id + "&options=mr");

      // Waiting for reply
      auto request = QNetworkRequest(req_url);
      request.setHeader(QNetworkRequest::UserAgentHeader,
                        GetHttpRequestUserAgent());

      QNetworkReply *reply = network_manager->get(request);
      QEventLoop loop;
      connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
      loop.exec();

      // Detect status
      QString status;
      auto error = reply->error();
      if (error != QNetworkReply::NoError) {
        switch (error) {
          case QNetworkReply::ContentNotFoundError:
            status = tr("Key Not Found");
            break;
          case QNetworkReply::TimeoutError:
            status = tr("Timeout");
            break;
          case QNetworkReply::HostNotFoundError:
            status = tr("Key Server Not Found");
            break;
          default:
            status = tr("Connection Error");
        }
      }

      reply->deleteLater();

      // Try importing
      auto result = GpgKeyImportExporter::GetInstance().ImportKey(
          GFBuffer(reply->readAll()));

      if (result->imported == 1) {
        status = tr("The key has been updated");
      } else {
        status = tr("No need to update the key");
      }
      callback(key_id, status, current_index, all_index);
      current_index++;
    }
  });
  connect(thread, &QThread::finished, thread, &QThread::deleteLater);
  thread->start();
}

void CommonUtils::slot_update_key_status() {
  auto *refresh_task = new Thread::Task(
      [](DataObjectPtr) -> int {
        // flush key cache for all GpgKeyGetter Intances.
        for (const auto &channel_id : GpgKeyGetter::GetAllChannelId()) {
          GpgKeyGetter::GetInstance(channel_id).FlushKeyCache();
        }
        return 0;
      },
      "update_key_database_task");
  connect(refresh_task, &Thread::Task::SignalTaskEnd, this,
          &CommonUtils::SignalKeyDatabaseRefreshDone);

  // post the task to the default task runner
  Thread::TaskRunnerGetter::GetInstance().GetTaskRunner()->PostTask(
      refresh_task);
}

void CommonUtils::slot_update_key_from_server_finished(
    bool success, QString err_msg, QByteArray buffer,
    std::shared_ptr<GpgImportInformation> info) {
  if (!success) {
    LOG_W() << "get err from reply: " << buffer;
    QMessageBox::critical(nullptr, tr("Error"), err_msg);
    return;
  }

  // refresh the key database
  emit UISignalStation::GetInstance() -> SignalKeyDatabaseRefresh();

  // show details
  (new KeyImportDetailDialog(std::move(info), this))->exec();
}

void CommonUtils::SlotRestartApplication(int code) {
  if (code == 0) {
    std::exit(0);
  } else {
    QCoreApplication::exit(code);
  }
}

auto CommonUtils::isApplicationNeedRestart() -> bool {
  return application_need_to_restart_at_once_;
}

auto CommonUtils::KeyExistsinFavouriteList(const GpgKey &key) -> bool {
  // load cache
  auto json_data = CacheObject("favourite_key_pair");
  if (!json_data.isArray()) json_data.setArray(QJsonArray());

  auto key_fpr_array = json_data.array();
  return std::find(key_fpr_array.begin(), key_fpr_array.end(),
                   key.GetFingerprint()) != key_fpr_array.end();
}

void CommonUtils::AddKey2Favourtie(const GpgKey &key) {
  {
    auto json_data = CacheObject("favourite_key_pair");
    QJsonArray key_array;
    if (json_data.isArray()) key_array = json_data.array();

    key_array.push_back(key.GetFingerprint());
    json_data.setArray(key_array);
  }

  emit SignalFavoritesChanged();
}

void CommonUtils::RemoveKeyFromFavourite(const GpgKey &key) {
  {
    auto json_data = CacheObject("favourite_key_pair");
    QJsonArray key_array;
    if (json_data.isArray()) key_array = json_data.array();

    auto fingerprint = key.GetFingerprint();
    QJsonArray new_key_array;
    for (auto &&item : key_array) {
      if (item.isString() && item.toString() != fingerprint) {
        new_key_array.append(item);
      }
    }
    json_data.setArray(new_key_array);
  }

  emit SignalFavoritesChanged();
}

/**
 * @brief
 *
 */
void CommonUtils::ImportKeyFromKeyServer(const KeyIdArgsList &key_ids) {
  KeyServerSO key_server(SettingsObject("key_server"));
  auto target_keyserver = key_server.GetTargetServer();

  auto *task = new KeyServerImportTask(target_keyserver, key_ids);
  connect(task, &KeyServerImportTask::SignalKeyServerImportResult, this,
          &CommonUtils::slot_update_key_from_server_finished);
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Network)
      ->PostTask(task);
}

}  // namespace GpgFrontend::UI