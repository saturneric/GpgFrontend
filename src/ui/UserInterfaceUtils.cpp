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

#include "core/GFConstants.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/function/gpg/GpgSmartCardManager.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/KeyCategoryRepository.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/model/CacheObject.h"
#include "core/model/GpgImportInformation.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgPassphraseContext.h"
#include "core/model/GpgSubKey.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "ui/UISignalStation.h"
#include "ui/dialog/KeyGroupManageDialog.h"
#include "ui/dialog/PassphraseDialog.h"
#include "ui/dialog/WaitingDialog.h"
#include "ui/dialog/import_export/KeyImportDetailDialog.h"
#include "ui/dialog/keypair_details/KeyDetailsDialog.h"

namespace GpgFrontend::UI {

QScopedPointer<CommonUtils> CommonUtils::instance =
    QScopedPointer<CommonUtils>(nullptr);

auto CommonUtils::DescribeBadOpenPGPEnv(GpgFrontend::BadOpenPGPEnvReason reason,
                                        const QString &detail)
    -> CommonUtils::BadOpenPGPEnvText {
  // No default label on purpose: -Wswitch then turns a new enumerator into a
  // compile error rather than a message that silently says the wrong thing.
  switch (reason) {
    case GpgFrontend::BadOpenPGPEnvReason::kNO_VALID_KEY_DATABASE:
      return {tr("No Usable Key Database"),
              tr("None of the configured key databases could be opened. This "
                 "usually means the folder was moved or deleted, or is on a "
                 "drive that is not currently available.") +
                  "\n\n" +
                  tr("You can change where your key databases live in "
                     "Settings, under Key Databases. Details: %1")
                      .arg(detail),
              // A restart re-reads the same unusable configuration.
              false};

    case GpgFrontend::BadOpenPGPEnvReason::kBASIC_PATH_INIT_FAILED:
      return {tr("Cannot Prepare Application Data"),
              tr("GpgFrontend could not set up the folders it needs to store "
                 "its data. Please check that the application data folder is "
                 "writable. Details: %1")
                  .arg(detail)};

    case GpgFrontend::BadOpenPGPEnvReason::kDEFAULT_CONTEXT_INIT_FAILED:
    case GpgFrontend::BadOpenPGPEnvReason::kKEY_CACHE_INIT_FAILED:
      return {tr("Key Database Could Not Be Opened"),
              tr("The key database was found but could not be loaded. It may "
                 "be in use by another program, or its permissions may have "
                 "changed. Details: %1")
                  .arg(detail)};

    case GpgFrontend::BadOpenPGPEnvReason::kNO_SUPPORTED_ENGINE:
    case GpgFrontend::BadOpenPGPEnvReason::kUNKNOWN:
      break;
  }

  return {tr("No Supported OpenPGP Engine Found"),
          tr("It seems that no supported OpenPGP engine is available. "
             "Please check your if GpgFrontend is properly installed and try "
             "again. Reason: %1")
              .arg(detail)};
}

auto CommonUtils::GetInstance() -> CommonUtils * {
  if (!instance) {
    instance.reset(new CommonUtils());
  }
  return instance.get();
}

CommonUtils::CommonUtils() : QWidget(nullptr) {
  connect(CoreSignalStation::GetInstance(),
          &CoreSignalStation::SignalBadOpenPGPEnv, this,
          &CommonUtils::SignalBadOpenPGPEnv);
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

  connect(this, &CommonUtils::SignalBadOpenPGPEnv, this,
          [=](GpgFrontend::BadOpenPGPEnvReason reason,
              const QString &detail) -> void {
            // The unit test runner shares this process, so a modal dialog
            // ending in std::exit(0) would take the whole test run down with
            // it.
            if (Module::RetrieveRTValueTypedOrDefault<>(
                    "core", "env.state.unit_test_mode", 0) == 1) {
              LOG_W() << "bad openpgp env in unit test mode, reason:"
                      << static_cast<int>(reason) << "detail:" << detail;
              return;
            }

            const auto text = DescribeBadOpenPGPEnv(reason, detail);

            QMessageBox msg_box;
            msg_box.setText(text.title);
            msg_box.setInformativeText(text.body);

            // Offering "Retry" for a failure that a restart cannot change just
            // invites the user to loop.
            msg_box.setStandardButtons(
                text.offer_retry ? QMessageBox::Retry | QMessageBox::Cancel
                                 : QMessageBox::Close);
            msg_box.setDefaultButton(text.offer_retry ? QMessageBox::Retry
                                                      : QMessageBox::Close);
            int ret = msg_box.exec();

            switch (ret) {
              case QMessageBox::Retry:
                // Mark application for immediate restart
                application_need_to_restart_at_once_ = true;
                // Trigger application restart with deep restart code
                emit SignalRestartApplication(kDeepRestartCode);
                break;
              default:
                // Default action: close application
                emit SignalRestartApplication(0);
                break;
            }
          });

  connect(
      CoreSignalStation::GetInstance(),
      &CoreSignalStation::SignalNeedUserInputPassphrase,
      QApplication::instance(),
      [](const QSharedPointer<GpgPassphraseContext> &c) {
        if (!c) return;

        // The unit test runner shares this process and goes through
        // PreInitGpgFrontendUI(), so this handler is live there too. A test has
        // nobody to type into a modal prompt; leave the request unanswered
        // instead, and let a test that cares about passphrases answer it by
        // handling the same signal itself.
        if (Module::RetrieveRTValueTypedOrDefault<>(
                "core", "env.state.unit_test_mode", 0) == 1) {
          LOG_W() << "passphrase requested in unit test mode; no prompt shown";
          return;
        }

        // Parent to the modal dialog on top when there is one — typically the
        // (also modal) waiting dialog of a running operation. A sibling of it
        // can end up below it in Qt's modal stack and then refuses input, which
        // reads as a frozen prompt; a modal child always stays above.
        QWidget *parent_widget = QApplication::activeModalWidget();
        if (parent_widget == nullptr)
          parent_widget = QApplication::activeWindow();

        PassphraseDialog dialog(c, parent_widget);

        if (dialog.exec() == QDialog::Accepted) {
          c->SetPassphrase(dialog.Passphrase());
        } else {
          // Set empty passphrase and flag the explicit cancellation so the
          // engine can surface GPG_ERR_CANCELED instead of a generic failure.
          c->SetPassphrase(GFBuffer());
          c->SetCancelled(true);
        }

        dialog.Clear();  // Clear the passphrase from memory as soon as possible

        emit CoreSignalStation::GetInstance()
            -> SignalUserInputPassphraseReady(c);
      });
}

void CommonUtils::RaiseMessageBox(QWidget *parent, GpgError err) {
  GpgErrorCode err_code = CheckGpgError2ErrCode(err);

  if (err_code == GPG_ERR_NO_ERROR) {
    QMessageBox::information(parent, tr("Success"),
                             tr("Operation completed successfully."));
  } else {
    RaiseFailureMessageBox(parent, err);
  }
}

void CommonUtils::RaiseMessageBoxNotSupported(QWidget *parent) {
  QMessageBox::warning(
      parent, tr("Operation Not Supported"),
      tr("The current OpenPGP engine does not support this operation. "
         "Please use a supported engine or upgrade the engine version."));
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
                                 const GFBuffer &in_buffer, bool rev_cert) {
  auto info =
      rev_cert
          ? KeyImportExportOperation::GetInstance(channel).ImportRevCert(
                in_buffer)
          : KeyImportExportOperation::GetInstance(channel).ImportKey(in_buffer);
  auto *connection = new QMetaObject::Connection;
  *connection = connect(UISignalStation::GetInstance(),
                        &UISignalStation::SignalKeyDatabaseRefreshDone, this,
                        [=]() -> void {
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
            if (status == QProcess::NormalExit) {
              QMessageBox::information(nullptr, tr("Success"),
                                       tr("Succeed in executing command."));
            } else {
              QMessageBox::information(nullptr, tr("Warning"),
                                       tr("Finished executing command."));
            }
          });

  const auto app_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.app_path", QString{});

  gpg_process->setProgram(app_path);
  gpg_process->setArguments(arguments);
  gpg_process->start();
  looper.exec();
  dialog->close();
}

void CommonUtils::slot_update_key_status() {
  auto *refresh_task = new Thread::Task(
      [](DataObjectPtr) -> int {
        // flush key cache for all GpgKeyGetter Intances.
        for (const auto &channel_id : OpenPGPContext::GetAllChannelId()) {
          LOG_D() << "refreshing key database at channel: " << channel_id;
          AbstractKeyRepository::GetInstance(channel_id).FlushCache();
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

void CommonUtils::NotifyCategoriesChanged() { emit SignalCategoriesChanged(); }

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
      break;
    case GpgAbstractKeyType::kNONE:
    case GpgAbstractKeyType::kGPG_SUBKEY:
      break;
  }
}

void CommonUtils::ImportKeys(QWidget *parent, int channel,
                             const GFBuffer &in_buffer) {
  SlotImportKeys(parent, channel, in_buffer);
}

auto ClampRectToAvailableGeometry(QRect rect, const QRect &available) -> QRect {
  const int max_width = static_cast<int>(available.width() * 0.95);
  const int max_height = static_cast<int>(available.height() * 0.95);

  if (rect.width() > max_width) {
    rect.setWidth(max_width);
  }

  if (rect.height() > max_height) {
    rect.setHeight(max_height);
  }

  if (rect.left() < available.left()) {
    rect.moveLeft(available.left());
  }

  if (rect.top() < available.top()) {
    rect.moveTop(available.top());
  }

  if (rect.right() > available.right()) {
    rect.moveRight(available.right());
  }

  if (rect.bottom() > available.bottom()) {
    rect.moveBottom(available.bottom());
  }

  return rect;
}

auto ConfirmShortUserIdName(QWidget *parent, const QString &name) -> bool {
  if (name.size() >= 5) return true;

  return QMessageBox::warning(
             parent,
             QCoreApplication::translate("GpgFrontend::UI", "Short Name"),
             QCoreApplication::translate(
                 "GpgFrontend::UI",
                 "The name \"%1\" is shorter than five characters. Short names "
                 "are allowed, but they are often a typo and make the key "
                 "harder for others to recognise.\n\nDo you want to continue?")
                 .arg(name),
             QMessageBox::Yes | QMessageBox::No,
             QMessageBox::No) == QMessageBox::Yes;
}

namespace {

auto CardSlotDisplayName(int slot) -> QString {
  switch (slot) {
    case 1:
      return QCoreApplication::translate("GpgFrontend::UI",
                                         "Signature (OPENPGP.1)");
    case 2:
      return QCoreApplication::translate("GpgFrontend::UI",
                                         "Encryption (OPENPGP.2)");
    case 3:
      return QCoreApplication::translate("GpgFrontend::UI",
                                         "Authentication (OPENPGP.3)");
    default:
      return {};
  }
}

/// Ask the user, when a (sub)key fits more than one slot, which one to use.
/// Returns the chosen slot or std::nullopt if cancelled.
auto ResolveCardSlot(QWidget *parent, const QList<int> &candidates)
    -> std::optional<int> {
  if (candidates.size() == 1) return candidates.front();

  QStringList options;
  for (int slot : candidates) options << CardSlotDisplayName(slot);

  bool ok = false;
  const auto choice = QInputDialog::getItem(
      parent,
      QCoreApplication::translate("GpgFrontend::UI", "Select Card Slot"),
      QCoreApplication::translate(
          "GpgFrontend::UI",
          "This key can be stored in more than one slot. "
          "Where should it be stored?"),
      options, 0, false, &ok);
  if (!ok) return std::nullopt;

  const auto index = options.indexOf(choice);
  if (index < 0) return std::nullopt;
  return candidates.at(index);
}

/// Resolve the target card serial: use @p preselected when set, otherwise pick
/// from the inserted cards (auto when one, ask when several). Empty on abort.
auto ResolveCardSerial(QWidget *parent, int channel, const QString &preselected)
    -> QString {
  if (!preselected.isEmpty()) return preselected;

  auto serials = GpgSmartCardManager::GetInstance(channel).GetSerialNumbers();
  if (serials.isEmpty()) {
    QMessageBox::information(
        parent, QCoreApplication::translate("GpgFrontend::UI", "No Smart Card"),
        QCoreApplication::translate("GpgFrontend::UI",
                                    "No OpenPGP smart card was detected. "
                                    "Insert a card and try again."));
    return {};
  }
  if (serials.size() == 1) return serials.front();

  bool ok = false;
  auto serial = QInputDialog::getItem(
      parent,
      QCoreApplication::translate("GpgFrontend::UI", "Select Smart Card"),
      QCoreApplication::translate("GpgFrontend::UI",
                                  "Move the key to which card?"),
      serials, 0, false, &ok);
  if (!ok) return {};
  return serial;
}

/// Offer to export a secret-key backup before the destructive move. Returns
/// false only when the user cancels the whole operation (a failed/aborted
/// backup counts as a cancel); true means "proceed with the move".
auto OfferSecretKeyBackup(QWidget *parent, int channel, const GpgKeyPtr &key)
    -> bool {
  QMessageBox box(parent);
  box.setIcon(QMessageBox::Question);
  box.setWindowTitle(
      QCoreApplication::translate("GpgFrontend::UI", "Back Up Secret Key"));
  box.setText(QCoreApplication::translate(
      "GpgFrontend::UI",
      "Do you want to export a backup of the secret key before moving it to "
      "the card? After the move the key can only be used through the card."));
  auto *backup_btn = box.addButton(
      QCoreApplication::translate("GpgFrontend::UI", "Back Up First"),
      QMessageBox::AcceptRole);
  box.addButton(
      QCoreApplication::translate("GpgFrontend::UI", "Continue Without Backup"),
      QMessageBox::DestructiveRole);
  auto *cancel_btn = box.addButton(QMessageBox::Cancel);
  box.setDefaultButton(backup_btn);
  box.exec();

  if (box.clickedButton() == cancel_btn) return false;
  if (box.clickedButton() != backup_btn)
    return true;  // continue without backup

  auto [err, buffer] = KeyImportExportOperation::GetInstance(channel).ExportKey(
      key, true, true, false);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    CommonUtils::RaiseMessageBox(parent, err);
    return false;
  }

  auto file_string = key->Name() + "_" + key->Email() + "_secret_backup.asc";
  file_string.replace(' ', '_');

  const auto file_name = QFileDialog::getSaveFileName(
      parent,
      QCoreApplication::translate("GpgFrontend::UI",
                                  "Export Secret Key Backup"),
      file_string,
      QCoreApplication::translate("GpgFrontend::UI", "Key Files") +
          " (*.asc *.txt);;All Files (*)");
  if (file_name.isEmpty()) return false;  // backup declined -> abort the move

  if (!WriteFileGFBuffer(file_name, buffer)) {
    QMessageBox::critical(
        parent, QCoreApplication::translate("GpgFrontend::UI", "Export Error"),
        QCoreApplication::translate("GpgFrontend::UI",
                                    "Couldn't open %1 for writing")
            .arg(file_name));
    return false;
  }
  return true;
}

}  // namespace

auto MoveKeyToCardInteractive(QWidget *parent, int channel,
                              const GpgKeyPtr &key, int subkey_index,
                              const QString &preselected_serial) -> bool {
  if (key == nullptr) return false;

  const auto subkeys = key->SubKeys();
  if (subkey_index < 0 || subkey_index >= static_cast<int>(subkeys.size())) {
    return false;
  }
  const auto &skey = subkeys[subkey_index];

  const auto title =
      QCoreApplication::translate("GpgFrontend::UI", "Move Key to Smart Card");

  // 1. warn about the destructive move
  const auto warn_body =
      QCoreApplication::translate(
          "GpgFrontend::UI",
          "<h3>You are about to move a private key onto a smart card.</h3>"
          "<b>KeyID:</b> %1<br/><br/>"
          "This <b>moves</b> the key: its private part is removed from this "
          "computer and only a card reference (stub) remains. Afterwards the "
          "key can only be used through the card. This action is "
          "<b>irreversible</b>.<br/><br/>Do you want to continue?")
          .arg(skey.ID());
  if (QMessageBox::warning(parent, title, warn_body,
                           QMessageBox::Cancel | QMessageBox::Yes,
                           QMessageBox::Cancel) != QMessageBox::Yes) {
    return false;
  }

  // 2. offer a secret-key backup first
  if (!OfferSecretKeyBackup(parent, channel, key)) return false;

  // 3. resolve the target slot from the key's capabilities
  const auto candidates = GpgSmartCardManager::CandidateSlots(skey);
  if (candidates.isEmpty()) {
    QMessageBox::critical(
        parent, title,
        QCoreApplication::translate(
            "GpgFrontend::UI",
            "This key has no capability that can be stored on a smart card."));
    return false;
  }
  const auto slot = ResolveCardSlot(parent, candidates);
  if (!slot) return false;

  // 4. resolve the target card
  const auto serial = ResolveCardSerial(parent, channel, preselected_serial);
  if (serial.isEmpty()) return false;

  // 5. perform the move
  auto [err, status] = GpgSmartCardManager::GetInstance(channel).MoveKeyToCard(
      key, subkey_index, serial, *slot);
  if (err != GPG_ERR_NO_ERROR) {
    CommonUtils::RaiseFailureMessageBox(parent, err, status);
    return false;
  }

  // 6. reconcile the on-disk stub, then reload the key database so the UI shows
  // the key as card-resident (mirrors the fetch flow in the card controller).
  GpgCommandExecutor::GetInstance(channel).GpgExecuteSync(
      {{}, {"--card-status"}, [](int, const QString &, const QString &) {}});
  emit UISignalStation::GetInstance() -> SignalKeyDatabaseRefresh();

  QMessageBox::information(
      parent, title,
      QCoreApplication::translate(
          "GpgFrontend::UI",
          "The key was moved to the smart card successfully."));
  return true;
}

auto SecureLevelDisplayName(int level) -> QString {
  // Brief, plainly escalating names for the status readout. What each level
  // actually does is spelled out on the Advanced settings page, where the
  // level is chosen; here a one-word tier reads at a glance.
  switch (level) {
    case 0:
      return QObject::tr("Standard");
    case 1:
      return QObject::tr("Enhanced");
    case 2:
      return QObject::tr("Strong");
    case 3:
      return QObject::tr("Maximum");
    default:
      return QObject::tr("Unknown");
  }
}

auto AppKeyProtectionDisplayName(AppKeyProtection protection) -> QString {
  switch (protection) {
    case AppKeyProtection::kKEYCHAIN:
      return QObject::tr("System keychain");
    case AppKeyProtection::kPIN:
      return QObject::tr("PIN at startup");
    case AppKeyProtection::kNONE:
      break;
  }
  return QObject::tr("No extra protection");
}

auto LowerSuffix(const QFileInfo &info) -> QString {
  return info.suffix().toLower();
}

auto IsOpenPGPMessageFile(const QFileInfo &info) -> bool {
  const auto suffix = LowerSuffix(info);
  return suffix == "gpg" || suffix == "pgp" || suffix == "asc";
}

auto IsOpenPGPRelatedFile(const QFileInfo &info) -> bool {
  return IsOpenPGPMessageFile(info) || IsOpenPGPSignatureFile(info);
}

auto IsOpenPGPSignatureFile(const QFileInfo &info) -> bool {
  return LowerSuffix(info) == "sig";
}

auto AccentColor(const QPalette &palette, bool positive) -> QColor {
  const auto dark = palette.color(QPalette::Base).lightness() < 128;
  if (!positive) {
    auto color = palette.color(QPalette::Text);
    color.setAlpha(150);
    return color;
  }
  return dark ? QColor(102, 187, 106) : QColor(46, 125, 50);
}

void SetChip(QLabel *label, const QString &text, const QColor &color) {
  label->setText(QString("<span style=\"color:%1;\">%2</span>")
                     .arg(color.name(QColor::HexRgb), text.toHtmlEscaped()));
}

auto ResolveAppearanceFont(const QString &family, int point_size) -> QFont {
  // Cached: this is asked again for every text surface each time appearance
  // settings are applied, and enumerating the installed families is not cheap.
  static const auto kFamilies = []() -> QStringList {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return QFontDatabase::families();
#else
    QFontDatabase font_database;
    return font_database.families();
#endif
  }();

  QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  font.setStyleHint(QFont::Monospace);
  font.setFixedPitch(true);

  const auto installed =
      !family.isEmpty() &&
      std::any_of(kFamilies.cbegin(), kFamilies.cend(),
                  [&family](const QString &item) {
                    return item.compare(family, Qt::CaseInsensitive) == 0;
                  });
  if (installed) font.setFamily(family);

  font.setPointSize(point_size);
  return font;
}
}  // namespace GpgFrontend::UI