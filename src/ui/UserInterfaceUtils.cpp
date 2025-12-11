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
#include "core/function/gpg/GpgAbstractKeyGetter.h"
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
#include "dialog/import_export/KeyImportDetailDialog.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/KeyGroupManageDialog.h"
#include "ui/dialog/WaitingDialog.h"
#include "ui/dialog/controller/GnuPGControllerDialog.h"
#include "ui/dialog/keypair_details/KeyDetailsDialog.h"
#include "ui/struct/settings_object/KeyServerSO.h"

namespace GpgFrontend::UI {

QScopedPointer<CommonUtils> CommonUtils::instance =
    QScopedPointer<CommonUtils>(nullptr);

auto CommonUtils::GetInstance() -> CommonUtils * {
  if (!instance) {
    instance.reset(new CommonUtils());
  }
  return instance.get();
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

void CommonUtils::RaiseMessageBoxNotSupported(QWidget *parent) {
  QMessageBox::warning(
      parent, tr("Operation Not Supported"),
      tr("The current GnuPG version is too low and does not support this "
         "operation. Please upgrade your GnuPG version to continue."));
}

void CommonUtils::RaiseFailureMessageBox(QWidget *parent, GpgError err,
                                         const QString &msg) {
  GpgErrorDesc desc = DescribeGpgErrCode(err);
  GpgErrorCode err_code = CheckGpgError2ErrCode(err);

  QMessageBox::critical(parent, tr("Failure"),
                        tr("Gpg Operation failed.") + "\n\n" +
                            tr("Error code: %1").arg(err_code) + "\n\n\n" +
                            tr("Source:  %1").arg(desc.first) + "\n" +
                            tr("Description: %1").arg(desc.second) + "\n" +
                            tr("Error Message: %1").arg(msg));
}

void CommonUtils::SlotImportKeys(QWidget *parent, int channel,
                                 const GFBuffer &in_buffer) {
  LOG_D() << "try to import key(s) to channel: " << channel;
  auto info = GpgKeyImportExporter::GetInstance(channel).ImportKey(in_buffer);

  auto *connection = new QMetaObject::Connection;
  *connection =
      connect(UISignalStation::GetInstance(),
              &UISignalStation::SignalKeyDatabaseRefreshDone, this, [=]() {
                (new KeyImportDetailDialog(channel, info, parent));
                QObject::disconnect(*connection);
                delete connection;
              });

  emit SignalKeyStatusUpdated();
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

  auto [succ, buffer] = ReadFileGFBuffer(file_name);
  if (!succ) {
    QMessageBox::critical(nullptr, tr("File Open Failed"),
                          tr("Failed to open file: ") + file_name);
    return;
  }
  SlotImportKeys(parent, channel, buffer);
}

void CommonUtils::SlotImportKeyFromClipboard(QWidget *parent, int channel) {
  QClipboard *cb = QApplication::clipboard();
  SlotImportKeys(parent, channel,
                 GFBuffer{cb->text(QClipboard::Clipboard).toLatin1()});
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
  auto *dialog = new WaitingDialog(tr("Processing"), false);
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
}

void CommonUtils::SlotUpdateKeysFromKeyServer(
    int channel, const KeyIdArgsList &key_ids,
    const ImportCallbackFunction &callback) {
  if (!Module::IsModuleActivate(kKeyServerSyncModuleID)) {
    QMessageBox::critical(
        nullptr, tr("Keyserver Sync Module Not Activated"),
        tr("The Keyserver Sync Module is not activated, "
           "please activate it in the Module Manager first."));
    return;
  }

  // LOOP
  decltype(key_ids.size()) current_index = 1;
  decltype(key_ids.size()) all_index = key_ids.size();

  for (const auto &key_id : key_ids) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Network)
        ->PostTask(new Thread::Task(
            [=](const DataObjectPtr &data_obj) -> int {
              // rate limit
              QThread::msleep(200);
              // call
              Module::TriggerEvent(
                  "REQUEST_GET_PUBLIC_KEY_BY_KEY_ID",
                  {
                      {"key_id", GFBuffer{key_id}},
                  },
                  [key_id, channel, callback, current_index, all_index](
                      Module::EventIdentifier i,
                      Module::Event::ListenerIdentifier ei,
                      Module::Event::Params p) {
                    LOG_D()
                        << "REQUEST_GET_PUBLIC_KEY_BY_FINGERPRINT callback: "
                        << i << ei;

                    QString status;

                    if (p["ret"] != "0" || !p["error_msg"].Empty()) {
                      LOG_E()
                          << "An error occurred trying to get data from key:"
                          << key_id << "error message: "
                          << p["error_msg"].ConvertToQString() << "reply data: "
                          << p["reply_data"].ConvertToQString();
                      status = p["error_msg"].ConvertToQString() +
                               p["reply_data"].ConvertToQString();
                    } else if (p.contains("key_data")) {
                      const auto key_data = p["key_data"];
                      LOG_D() << "got key data of key " << key_id
                              << " from key server: "
                              << key_data.ConvertToQString();

                      auto result =
                          GpgKeyImportExporter::GetInstance(channel).ImportKey(
                              GFBuffer(key_data));
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

    current_index++;
  }
}

void CommonUtils::slot_update_key_status() {
  auto *refresh_task = new Thread::Task(
      [](DataObjectPtr) -> int {
        // flush key cache for all GpgKeyGetter Intances.
        for (const auto &channel_id : GpgContext::GetAllChannelId()) {
          LOG_D() << "refreshing key database at channel: " << channel_id;
          GpgAbstractKeyGetter::GetInstance(channel_id).FlushCache();
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
    QSharedPointer<GpgImportInformation> info) {
  if (!success) {
    LOG_W() << "get err from reply: " << buffer;
    QMessageBox::critical(nullptr, tr("Error"), err_msg);
    return;
  }

  // refresh the key database
  emit UISignalStation::GetInstance() -> SignalKeyDatabaseRefresh();

  auto *connection = new QMetaObject::Connection;
  *connection =
      connect(UISignalStation::GetInstance(),
              &UISignalStation::SignalKeyDatabaseRefreshDone, this, [=]() {
                (new KeyImportDetailDialog(channel, info, this));
                QObject::disconnect(*connection);
                delete connection;
              });
}

void CommonUtils::SlotRestartApplication(int code) {
  if (code == 0) {
    std::exit(0);
  } else {
    QCoreApplication::exit(code);
  }
}

auto CommonUtils::IsApplicationNeedRestart() -> bool {
  return application_need_to_restart_at_once_;
}

auto CommonUtils::KeyExistsInFavoriteList(const QString &key_db_name,
                                          const GpgKey &key) -> bool {
  // load cache
  auto json_data = CacheObject("all_favorite_key_pairs");
  auto cache_obj = AllFavoriteKeyPairsCO(json_data.object());

  if (!cache_obj.key_dbs.contains(key_db_name)) return false;

  auto &key_ids = cache_obj.key_dbs[key_db_name].key_ids;

  return key_ids.contains(key.ID());
}

void CommonUtils::AddKey2Favorite(const QString &key_db_name,
                                  const GpgAbstractKeyPtr &key) {
  {
    auto json_data = CacheObject("all_favorite_key_pairs");
    auto cache_obj = AllFavoriteKeyPairsCO(json_data.object());

    if (!cache_obj.key_dbs.contains(key_db_name)) {
      cache_obj.key_dbs[key_db_name] = FavoriteKeyPairsByKeyDatabaseCO();
    }

    auto &key_ids = cache_obj.key_dbs[key_db_name].key_ids;
    if (!key_ids.contains(key->ID())) key_ids.append(key->ID());

    json_data.setObject(cache_obj.ToJson());
    LOG_D() << "current favorite key pairs: " << json_data;
  }

  emit SignalFavoritesChanged();
}

void CommonUtils::RemoveKeyFromFavorite(const QString &key_db_name,
                                        const GpgAbstractKeyPtr &key) {
  {
    auto json_data = CacheObject("all_favorite_key_pairs");
    auto cache_obj = AllFavoriteKeyPairsCO(json_data.object());

    if (!cache_obj.key_dbs.contains(key_db_name)) return;

    QMutableListIterator<QString> i(cache_obj.key_dbs[key_db_name].key_ids);
    while (i.hasNext()) {
      if (i.next() == key->ID()) i.remove();
    }
    json_data.setObject(cache_obj.ToJson());
    LOG_D() << "current favorite key pairs: " << json_data;
  }

  emit SignalFavoritesChanged();
}

void CommonUtils::ImportKeysFromKeyServer(QWidget *parent, int channel,
                                          const QStringList &fprs) {
  if (!Module::IsModuleActivate(kKeyServerSyncModuleID)) {
    QMessageBox::critical(
        parent, tr("Key Server Sync Module Not Activated"),
        tr("The Key Server Sync Module is not activated. Please activate it "
           "first in order to sync keys from key servers."));
    return;
  }

  auto all_key_data = SecureCreateSharedObject<GFBuffer>();
  auto remaining_tasks = SecureCreateSharedObject<int>(fprs.size());

  for (const auto &fpr : fprs) {
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Network)
        ->PostTask(new Thread::Task(
            [=](const DataObjectPtr &data_obj) -> int {
              Module::TriggerEvent(
                  "REQUEST_GET_PUBLIC_KEY_BY_FINGERPRINT",
                  {
                      {"fingerprint", GFBuffer{fpr}},
                  },
                  [fpr, all_key_data, remaining_tasks, this, parent, channel](
                      Module::EventIdentifier i,
                      Module::Event::ListenerIdentifier ei,
                      Module::Event::Params p) {
                    LOG_D()
                        << "REQUEST_GET_PUBLIC_KEY_BY_FINGERPRINT callback: "
                        << i << ei;

                    if (p["ret"] != "0" || !p["error_msg"].Empty()) {
                      LOG_E()
                          << "An error occurred trying to get data from key:"
                          << fpr << "error message: "
                          << p["error_msg"].ConvertToQString() << "reply data: "
                          << p["reply_data"].ConvertToQString();
                    } else if (p.contains("key_data")) {
                      all_key_data->Append(p["key_data"]);
                    }

                    // it only uses one thread for these operations
                    // so that is no need for locking now
                    (*remaining_tasks)--;

                    if (*remaining_tasks == 0) {
                      this->SlotImportKeys(parent, channel, *all_key_data);
                    }
                  });

              return 0;
            },
            QString("key_%1_import_task").arg(fpr)));
  }
}

void CommonUtils::OpenDetailsDialogByKey(QWidget *parent, int channel,
                                         const GpgAbstractKeyPtr &key) {
  if (key == nullptr) {
    QMessageBox::critical(parent, tr("Error"), tr("Key Not Found."));
    return;
  }

  switch (key->KeyType()) {
    case GpgAbstractKeyType::kGPG_KEY:
      new KeyDetailsDialog(channel, qSharedPointerDynamicCast<GpgKey>(key),
                           parent);
      break;
    case GpgAbstractKeyType::kGPG_KEYGROUP:
      new KeyGroupManageDialog(
          channel, qSharedPointerDynamicCast<GpgKeyGroup>(key), parent);
    case GpgAbstractKeyType::kNONE:
    case GpgAbstractKeyType::kGPG_SUBKEY:
      break;
  }
}

void GF_UI_EXPORT CommonUtils::ImportKeys(QWidget *parent, int channel,
                                          const GFBuffer &in_buffer) {
  SlotImportKeys(parent, channel, in_buffer);
}
}  // namespace GpgFrontend::UI