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

#include "gpg/result_analyse/ResultAnalyse.h"
#include "ui/SignalStation.h"
#include "ui/WaitingDialog.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

std::unique_ptr<GpgFrontend::UI::CommonUtils>
    GpgFrontend::UI::CommonUtils::_instance = nullptr;

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
  info_board->associateFileTreeView(edit->curFilePage());
  refresh_info_board(info_board, result_analyse.getStatus(),
                     result_analyse.getResultReport());
}

void process_result_analyse(TextEdit* edit, InfoBoardWidget* info_board,
                            const ResultAnalyse& result_analyse_a,
                            const ResultAnalyse& result_analyse_b) {
  LOG(INFO) << "process_result_analyse Started";

  info_board->associateTabWidget(edit->tabWidget);
  info_board->associateFileTreeView(edit->curFilePage());

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
}

void CommonUtils::slotImportKeys(QWidget* parent,
                                 const std::string& in_buffer) {
  GpgImportInformation result = GpgKeyImportExportor::GetInstance().ImportKey(
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

}  // namespace GpgFrontend::UI