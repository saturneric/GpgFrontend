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

#ifndef GPGFRONTEND_USER_INTERFACE_UTILS_H
#define GPGFRONTEND_USER_INTERFACE_UTILS_H

#include "gpg/result_analyse/VerifyResultAnalyse.h"
#include "ui/GpgFrontendUI.h"

namespace GpgFrontend {
class ResultAnalyse;
}

namespace GpgFrontend::UI {

class InfoBoardWidget;
class TextEdit;

void show_verify_details(QWidget* parent, InfoBoardWidget* info_board,
                         GpgError error, const GpgVerifyResult& verify_result);

void import_unknown_key_from_keyserver(QWidget* parent,
                                       const VerifyResultAnalyse& verify_res);

void refresh_info_board(InfoBoardWidget* info_board, int status,
                        const std::string& report_text);

void process_result_analyse(TextEdit* edit, InfoBoardWidget* info_board,
                            const ResultAnalyse& result_analyse);

void process_result_analyse(TextEdit* edit, InfoBoardWidget* info_board,
                            const ResultAnalyse& result_analyse_a,
                            const ResultAnalyse& result_analyse_b);

void process_operation(QWidget* parent, const std::string& waiting_title,
                       const std::function<void()>& func);

class CommonUtils : public QWidget {
  Q_OBJECT
 public:
  static CommonUtils* GetInstance();

  CommonUtils();

 signals:
  void signalKeyStatusUpdated();

 public slots:

  void slotImportKeys(QWidget* parent, const std::string& in_buffer);

  void slotImportKeyFromFile(QWidget* parent);

  void slotImportKeyFromKeyServer(QWidget* parent);

  void slotImportKeyFromClipboard(QWidget* parent);

  void slotExecuteGpgCommand(
      const QStringList& arguments,
      const std::function<void(QProcess*)>& interact_func);

 private:
  static std::unique_ptr<CommonUtils> _instance;
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_USER_INTERFACE_UTILS_H
