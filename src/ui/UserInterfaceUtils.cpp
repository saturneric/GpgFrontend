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
#include "ui/WaitingDialog.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {
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

}  // namespace GpgFrontend::UI