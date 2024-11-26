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
#include <utility>

#include "core/GpgConstants.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/model/CacheObject.h"
#include "core/model/GpgImportInformation.h"
#include "core/model/SettingsObject.h"
#include "core/module/ModuleManager.h"
#include "core/struct/cache_object/AllFavoriteKeyPairsCO.h"
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

void show_verify_details(QWidget *parent, int channel,
                         InfoBoardWidget *info_board, GpgError error,
                         const GpgVerifyResult &verify_result) {
  // take out result
  info_board->ResetOptionActionsMenu();
  info_board->AddOptionalAction(
      QCoreApplication::tr("Show Verify Details"),
      [=]() { VerifyDetailsDialog(parent, channel, error, verify_result); });
}

void ImportUnknownKeyFromKeyserver(
    QWidget *parent, int channel, const GpgVerifyResultAnalyse &verify_result) {
  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(
      parent, QCoreApplication::tr("Public key not found locally"),
      QCoreApplication::tr(
          "There is no target public key content in local for GpgFrontend to "
          "gather enough information about this Signature. Do you want to "
          "import the public key from Keyserver now?"),
      QMessageBox::Yes | QMessageBox::No);
  if (reply == QMessageBox::Yes) {
    auto dialog = KeyServerImportDialog(channel, parent);
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

  connect(
      this, &CommonUtils::SignalBadGnupgEnv, this, [=](const QString &reason) {
        QMessageBox msg_box;
        msg_box.setText(tr("Failed to Load GnuPG Context"));
        msg_box.setInformativeText(
            tr("It seems that GnuPG (gpg) is not properly installed. "
               "Please refer to the <a "
               "href='https://www.gpgfrontend.bktus.com/overview/faq/"
               "#troubleshooting-gnupg-installation-issues'>FAQ</a> for "
               "instructions on fixing the installation. After resolving the "
               "issue, "
               "relaunch GpgFrontend.<br /><br />"
               "Alternatively, you can open the GnuPG Controller to configure "
               "a custom GnuPG installation for GpgFrontend to use. Once set, "
               "GpgFrontend will restart automatically.<br /><br />"
               "Details: %1")
                .arg(reason));
        msg_box.setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel);
        msg_box.setDefaultButton(QMessageBox::Retry);
        int ret = msg_box.exec();

        switch (ret) {
          case QMessageBox::Retry:
            (new GnuPGControllerDialog(this))->exec();
            // Mark application for immediate restart
            application_need_to_restart_at_once_ = true;
            // Trigger application restart with deep restart code
            emit SignalRestartApplication(kDeepRestartCode);
            break;
          case QMessageBox::Cancel:
            // Close application
            emit SignalRestartApplication(0);
            break;
          default:
            // Default action: close application
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

void CommonUtils::SlotImportKeys(QWidget *parent, int channel,
                                 const QByteArray &in_buffer) {
  LOG_D() << "try to import key(s) to channel: " << channel;
  auto info =
      GpgKeyImportExporter::GetInstance(channel).ImportKey(GFBuffer(in_buffer));
  emit SignalKeyStatusUpdated();

  (new KeyImportDetailDialog(channel, info, parent));
}

void CommonUtils::SlotImportKeyFromFile(QWidget *parent, int channel) {
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
  SlotImportKeys(parent, channel, key_buffer);
}

void CommonUtils::SlotImportKeyFromKeyServer(QWidget *parent, int channel) {
  auto *dialog = new KeyServerImportDialog(channel, parent);
  dialog->show();
}

void CommonUtils::SlotImportKeyFromClipboard(QWidget *parent, int channel) {
  QClipboard *cb = QApplication::clipboard();
  SlotImportKeys(parent, channel, cb->text(QClipboard::Clipboard).toLatin1());
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
    int channel, const KeyIdArgsList &key_ids,
    const ImportCallbackFunctiopn &callback) {
  auto target_keyserver =
      KeyServerSO(SettingsObject("key_server")).GetTargetServer();
  if (target_keyserver.isEmpty()) {
    QMessageBox::critical(
        nullptr, tr("Default Keyserver Not Found"),
        tr("Cannot read default keyserver from your settings, "
           "please set a default keyserver first"));
    return;
  }

  if (Module::IsModuleActivate(kKeyServerSyncModuleID)) {
    // LOOP
    decltype(key_ids.size()) current_index = 1;
    decltype(key_ids.size()) all_index = key_ids.size();

    for (const auto &key_id : key_ids) {
      Thread::TaskRunnerGetter::GetInstance()
          .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Network)
          ->PostTask(new Thread::Task(
              [=](const DataObjectPtr &data_obj) -> int {
                Module::TriggerEvent(
                    "REQUEST_GET_PUBLIC_KEY_BY_KEY_ID",
                    {
                        {"key_id", QString(key_id)},
                    },
                    [key_id, channel, callback, current_index, all_index](
                        Module::EventIdentifier i,
                        Module::Event::ListenerIdentifier ei,
                        Module::Event::Params p) {
                      LOG_D()
                          << "REQUEST_GET_PUBLIC_KEY_BY_FINGERPRINT callback: "
                          << i << ei;

                      QString status;

                      if (p["ret"] != "0" || !p["error_msg"].isEmpty()) {
                        LOG_E()
                            << "An error occurred trying to get data from key:"
                            << key_id << "error message: " << p["error_msg"]
                            << "reply data: " << p["reply_data"];
                        status = p["error_msg"] + p["reply_data"];
                      } else if (p.contains("key_data")) {
                        const auto key_data = p["key_data"];
                        LOG_D() << "got key data of key " << key_id
                                << " from key server: " << key_data;

                        auto result = GpgKeyImportExporter::GetInstance(channel)
                                          .ImportKey(GFBuffer(key_data));
                        if (result->imported == 1) {
                          status = tr("The key has been updated");
                        } else {
                          status = tr("No need to update the key");
                        }
                      }

                      callback(key_id, status, current_index, all_index);
                    });

                return 0;
              },
              QString("key_%1_import_task").arg(key_id)));

      // not too fast to hit rate limit
      QThread::msleep(200);
      current_index++;
    }

    return;
  }

  auto *thread =
      QThread::create([target_keyserver, key_ids, callback, channel]() {
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
          auto result = GpgKeyImportExporter::GetInstance(channel).ImportKey(
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
        for (const auto &channel_id : GpgContext::GetAllChannelId()) {
          LOG_D() << "refreshing key database at channel: " << channel_id;
          GpgKeyGetter::GetInstance(channel_id).FlushKeyCache();
        }
        LOG_D() << "refreshing key database at all channel done";
        return 0;
      },
      "update_key_database_task");

  connect(refresh_task, &Thread::Task::SignalTaskEnd, this,
          &CommonUtils::SignalKeyDatabaseRefreshDone);

  // post the task to the default task runner
  LOG_D() << "sending key database refresh task to gpg task runner...";
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
      ->PostTask(refresh_task);
}

void CommonUtils::slot_update_key_from_server_finished(
    int channel, bool success, QString err_msg, QByteArray buffer,
    std::shared_ptr<GpgImportInformation> info) {
  if (!success) {
    LOG_W() << "get err from reply: " << buffer;
    QMessageBox::critical(nullptr, tr("Error"), err_msg);
    return;
  }

  // refresh the key database
  emit UISignalStation::GetInstance() -> SignalKeyDatabaseRefresh();

  // show details
  (new KeyImportDetailDialog(channel, std::move(info), this))->exec();
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

auto CommonUtils::KeyExistsInFavoriteList(const QString &key_db_name,
                                          const GpgKey &key) -> bool {
  // load cache
  auto json_data = CacheObject("all_favorite_key_pairs");
  auto cache_obj = AllFavoriteKeyPairsCO(json_data.object());

  if (!cache_obj.key_dbs.contains(key_db_name)) return false;

  auto &key_ids = cache_obj.key_dbs[key_db_name].key_ids;

  return key_ids.contains(key.GetId());
}

void CommonUtils::AddKey2Favorite(const QString &key_db_name,
                                  const GpgKey &key) {
  {
    auto json_data = CacheObject("all_favorite_key_pairs");
    auto cache_obj = AllFavoriteKeyPairsCO(json_data.object());

    if (!cache_obj.key_dbs.contains(key_db_name)) {
      cache_obj.key_dbs[key_db_name] = FavoriteKeyPairsByKeyDatabaseCO();
    }

    auto &key_ids = cache_obj.key_dbs[key_db_name].key_ids;
    if (!key_ids.contains(key.GetId())) key_ids.append(key.GetId());

    json_data.setObject(cache_obj.ToJson());
    LOG_D() << "current favorite key pairs: " << json_data;
  }

  emit SignalFavoritesChanged();
}

void CommonUtils::RemoveKeyFromFavorite(const QString &key_db_name,
                                        const GpgKey &key) {
  {
    auto json_data = CacheObject("all_favorite_key_pairs");
    auto cache_obj = AllFavoriteKeyPairsCO(json_data.object());

    if (!cache_obj.key_dbs.contains(key_db_name)) return;

    QMutableListIterator<QString> i(cache_obj.key_dbs[key_db_name].key_ids);
    while (i.hasNext()) {
      if (i.next() == key.GetId()) i.remove();
    }
    json_data.setObject(cache_obj.ToJson());
    LOG_D() << "current favorite key pairs: " << json_data;
  }

  emit SignalFavoritesChanged();
}

/**
 * @brief
 *
 */
void CommonUtils::ImportKeyFromKeyServer(int channel,
                                         const KeyIdArgsList &key_ids) {
  KeyServerSO key_server(SettingsObject("key_server"));
  auto target_keyserver = key_server.GetTargetServer();

  auto *task = new KeyServerImportTask(target_keyserver, channel, key_ids);
  connect(task, &KeyServerImportTask::SignalKeyServerImportResult, this,
          &CommonUtils::slot_update_key_from_server_finished);
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Network)
      ->PostTask(task);
}

void CommonUtils::ImportKeyByKeyServerSyncModule(QWidget *parent, int channel,
                                                 const QList<QString> &fprs) {
  if (!Module::IsModuleActivate(kKeyServerSyncModuleID)) {
    return;
  }

  auto all_key_data = QSharedPointer<QString>::create();
  auto remaining_tasks = QSharedPointer<int>::create(fprs.size());

  for (const auto &fpr : fprs) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Network)
        ->PostTask(new Thread::Task(
            [=](const DataObjectPtr &data_obj) -> int {
              Module::TriggerEvent(
                  "REQUEST_GET_PUBLIC_KEY_BY_FINGERPRINT",
                  {
                      {"fingerprint", QString(fpr)},
                  },
                  [fpr, all_key_data, remaining_tasks, this, parent, channel](
                      Module::EventIdentifier i,
                      Module::Event::ListenerIdentifier ei,
                      Module::Event::Params p) {
                    LOG_D()
                        << "REQUEST_GET_PUBLIC_KEY_BY_FINGERPRINT callback: "
                        << i << ei;

                    if (p["ret"] != "0" || !p["error_msg"].isEmpty()) {
                      LOG_E()
                          << "An error occurred trying to get data from key:"
                          << fpr << "error message: " << p["error_msg"]
                          << "reply data: " << p["reply_data"];
                    } else if (p.contains("key_data")) {
                      const auto key_data = p["key_data"];
                      LOG_D() << "got key data of key " << fpr
                              << " from key server: " << key_data;

                      *all_key_data += key_data;
                    }

                    // it only uses one thread for these operations
                    // so that is no need for locking now
                    (*remaining_tasks)--;

                    if (*remaining_tasks == 0) {
                      this->SlotImportKeys(parent, channel,
                                           all_key_data->toUtf8());
                    }
                  });

              return 0;
            },
            QString("key_%1_import_task").arg(fpr)));
  }
}
}  // namespace GpgFrontend::UI