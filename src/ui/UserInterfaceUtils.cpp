/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "UserInterfaceUtils.h"

#include <utility>

#include "gpg/result_analyse/ResultAnalyse.h"
#include "ui/SignalStation.h"
#include "ui/WaitingDialog.h"
#include "ui/settings/GlobalSettingStation.h"
#include "ui/smtp/SendMailDialog.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

std::unique_ptr<GpgFrontend::UI::CommonUtils>
    GpgFrontend::UI::CommonUtils::_instance = nullptr;

#ifdef SMTP_SUPPORT
void send_an_email(QWidget* parent, InfoBoardWidget* info_board,
                   const QString& text, bool attach_signature) {
  info_board->addOptionalAction(_("Send Encrypted Mail"), [=]() {
    bool smtp_enabled = false;
    try {
      smtp_enabled = GlobalSettingStation::GetInstance().GetUISettings().lookup(
          "smtp.enable");
    } catch (...) {
      LOG(INFO) << "Reading smtp settings error";
    }
    if (smtp_enabled) {
      auto dialog = new SendMailDialog(text, parent);
      dialog->setContentEncryption(false);
      dialog->setAttachSignature(attach_signature);
      dialog->show();
    } else {
      QMessageBox::warning(nullptr, _("Function Disabled"),
                           _("Please go to the settings interface to "
                             "enable and configure this function."));
    }
  });
}
#endif

void show_verify_details(QWidget* parent, InfoBoardWidget* info_board,
                         GpgError error, const GpgVerifyResult& verify_result) {
  // take out result
  info_board->resetOptionActionsMenu();
  info_board->addOptionalAction("Show Verify Details", [=]() {
    VerifyDetailsDialog(parent, error, verify_result);
  });
}

void import_unknown_key_from_keyserver(QWidget* parent,
                                       const VerifyResultAnalyse& verify_res) {
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
    auto* signature = verify_res.GetSignatures();
    while (signature != nullptr) {
      LOG(INFO) << "signature fpr" << signature->fpr;
      key_ids->push_back(signature->fpr);
      signature = signature->next;
    }
    dialog.show();
    dialog.slotImport(key_ids);
  }
}

void refresh_info_board(InfoBoardWidget* info_board, int status,
                        const std::string& report_text) {
  if (status < 0)
    info_board->slotRefresh(QString::fromStdString(report_text),
                            INFO_ERROR_CRITICAL);
  else if (status > 0)
    info_board->slotRefresh(QString::fromStdString(report_text), INFO_ERROR_OK);
  else
    info_board->slotRefresh(QString::fromStdString(report_text),
                            INFO_ERROR_WARN);
}

void process_result_analyse(TextEdit* edit, InfoBoardWidget* info_board,
                            const ResultAnalyse& result_analyse) {
  info_board->associateTabWidget(edit->tabWidget);
  refresh_info_board(info_board, result_analyse.getStatus(),
                     result_analyse.getResultReport());
}

void process_result_analyse(TextEdit* edit, InfoBoardWidget* info_board,
                            const ResultAnalyse& result_analyse_a,
                            const ResultAnalyse& result_analyse_b) {
  LOG(INFO) << "process_result_analyse Started";

  info_board->associateTabWidget(edit->tabWidget);

  refresh_info_board(
      info_board,
      std::min(result_analyse_a.getStatus(), result_analyse_b.getStatus()),
      result_analyse_a.getResultReport() + result_analyse_b.getResultReport());
}

void process_operation(QWidget* parent, const std::string& waiting_title,
                       const std::function<void()>& func) {
  auto thread = QThread::create(func);
  QApplication::connect(thread, SIGNAL(finished()), thread,
                        SLOT(deleteLater()));
  thread->start();

  auto* dialog =
      new WaitingDialog(QString::fromStdString(waiting_title), parent);
  while (thread->isRunning()) {
    QApplication::processEvents();
  }
  dialog->close();
}

CommonUtils* CommonUtils::GetInstance() {
  if (_instance == nullptr) {
    _instance = std::make_unique<CommonUtils>();
  }
  return _instance.get();
}

CommonUtils::CommonUtils() : QWidget(nullptr) {
  connect(this, SIGNAL(signalKeyStatusUpdated()), SignalStation::GetInstance(),
          SIGNAL(KeyDatabaseRefresh()));
  connect(this, &CommonUtils::signalGnupgNotInstall, this, []() {
    QMessageBox::critical(
        nullptr, _("ENV Loading Failed"),
        _("Gnupg(gpg) is not installed correctly, please follow the "
          "ReadME "
          "instructions in Github to install Gnupg and then open "
          "GpgFrontend."));
    QCoreApplication::quit();
  });
}

void CommonUtils::slotImportKeys(QWidget* parent,
                                 const std::string& in_buffer) {
  GpgImportInformation result = GpgKeyImportExporter::GetInstance().ImportKey(
      std::make_unique<ByteArray>(in_buffer));
  emit signalKeyStatusUpdated();
  new KeyImportDetailDialog(result, false, parent);
}

void CommonUtils::slotImportKeyFromFile(QWidget* parent) {
  QString file_name = QFileDialog::getOpenFileName(
      this, _("Open Key"), QString(),
      QString(_("Key Files")) + " (*.asc *.txt);;" + _("Keyring files") +
          " (*.gpg);;All Files (*)");
  if (!file_name.isNull()) {
    slotImportKeys(parent, read_all_data_in_file(file_name.toStdString()));
  }
}

void CommonUtils::slotImportKeyFromKeyServer(QWidget* parent) {
  auto dialog = new KeyServerImportDialog(false, parent);
  dialog->show();
}

void CommonUtils::slotImportKeyFromClipboard(QWidget* parent) {
  QClipboard* cb = QApplication::clipboard();
  slotImportKeys(parent,
                 cb->text(QClipboard::Clipboard).toUtf8().toStdString());
}

void CommonUtils::slotExecuteGpgCommand(
    const QStringList& arguments,
    const std::function<void(QProcess*)>& interact_func) {
  QEventLoop looper;
  auto dialog = new WaitingDialog(_("Processing"), nullptr);
  dialog->show();
  auto* gpg_process = new QProcess(&looper);
  gpg_process->setProcessChannelMode(QProcess::MergedChannels);

  connect(gpg_process,
          qOverload<int, QProcess::ExitStatus>(&QProcess::finished), &looper,
          &QEventLoop::quit);
  connect(gpg_process,
          qOverload<int, QProcess::ExitStatus>(&QProcess::finished), dialog,
          &WaitingDialog::deleteLater);
  connect(gpg_process, &QProcess::errorOccurred, &looper, &QEventLoop::quit);
  connect(gpg_process, &QProcess::started,
          []() -> void { LOG(ERROR) << "Gpg Process Started Success"; });
  connect(gpg_process, &QProcess::readyReadStandardOutput,
          [interact_func, gpg_process]() { interact_func(gpg_process); });
  connect(gpg_process, &QProcess::errorOccurred, this, [=]() -> void {
    LOG(ERROR) << "Error in Process";
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

  gpg_process->setProgram(GpgContext::GetInstance().GetInfo().AppPath.c_str());
  gpg_process->setArguments(arguments);
  gpg_process->start();
  looper.exec();
  dialog->close();
  dialog->deleteLater();
}

void CommonUtils::slotImportKeyFromKeyServer(
    int ctx_channel, const KeyIdArgsList& key_ids,
    const ImportCallbackFunctiopn& callback) {
  std::string target_keyserver;
  if (target_keyserver.empty()) {
    try {
      auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

      target_keyserver = settings.lookup("keyserver.default_server").c_str();

      LOG(INFO) << _("Set target Key Server to default Key Server")
                << target_keyserver;
    } catch (...) {
      LOG(ERROR) << _("Cannot read default_keyserver From Settings");
      QMessageBox::critical(
          nullptr, _("Default Keyserver Not Found"),
          _("Cannot read default keyserver from your settings, "
            "please set a default keyserver first"));
      return;
    }
  }

  auto thread =
      QThread::create([target_keyserver, key_ids, callback, ctx_channel]() {
        QUrl target_keyserver_url(target_keyserver.c_str());

        auto network_manager = std::make_unique<QNetworkAccessManager>();
        // LOOP
        decltype(key_ids.size()) current_index = 1, all_index = key_ids.size();
        for (const auto& key_id : key_ids) {
          // New Req Url
          QUrl req_url(target_keyserver_url.scheme() + "://" +
                       target_keyserver_url.host() +
                       "/pks/lookup?op=get&search=0x" + key_id.c_str() +
                       "&options=mr");

          LOG(INFO) << "request url" << req_url.toString().toStdString();

          // Waiting for reply
          QNetworkReply* reply = network_manager->get(QNetworkRequest(req_url));
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
              GpgKeyImportExporter::GetInstance(ctx_channel)
                  .ImportKey(std::move(key_data_ptr));

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

}  // namespace GpgFrontend::UI