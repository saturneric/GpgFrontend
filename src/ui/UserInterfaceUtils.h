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

#ifndef GPGFRONTEND_USER_INTERFACE_UTILS_H
#define GPGFRONTEND_USER_INTERFACE_UTILS_H

#include "core/GpgModel.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "ui/GpgFrontendUI.h"

namespace GpgFrontend {
class GpgResultAnalyse;
}

namespace GpgFrontend::UI {

class InfoBoardWidget;
class TextEdit;

/**
 * @brief
 *
 * @param parent
 * @param info_board
 * @param error
 * @param verify_result
 */
void show_verify_details(QWidget* parent, InfoBoardWidget* info_board,
                         GpgError error, const GpgVerifyResult& verify_result);

/**
 * @brief
 *
 * @param parent
 * @param verify_res
 */
void import_unknown_key_from_keyserver(
    QWidget* parent, const GpgVerifyResultAnalyse& verify_res);

/**
 * @brief
 *
 * @param info_board
 * @param status
 * @param report_text
 */
void refresh_info_board(InfoBoardWidget* info_board, int status,
                        const std::string& report_text);

/**
 * @brief
 *
 * @param edit
 * @param info_board
 * @param result_analyse
 */
void process_result_analyse(TextEdit* edit, InfoBoardWidget* info_board,
                            const GpgResultAnalyse& result_analyse);

/**
 * @brief
 *
 * @param edit
 * @param info_board
 * @param result_analyse_a
 * @param result_analyse_b
 */
void process_result_analyse(TextEdit* edit, InfoBoardWidget* info_board,
                            const GpgResultAnalyse& result_analyse_a,
                            const GpgResultAnalyse& result_analyse_b);

/**
 * @brief
 *
 * @param parent
 * @param waiting_title
 * @param func
 */
void process_operation(
    QWidget* parent, const std::string& waiting_title,
    GpgFrontend::Thread::Task::TaskRunnable func,
    GpgFrontend::Thread::Task::TaskCallback callback = nullptr,
    Thread::Task::DataObjectPtr data_object = nullptr);

/**
 * @brief
 *
 * @param parent
 * @param key_id
 * @param key_server
 */
void import_key_from_keyserver(QWidget* parent, const std::string& key_id,
                               const std::string& key_server);

/**
 * @brief
 *
 */
class CommonUtils : public QWidget {
  Q_OBJECT
 public:
  /**
   * @brief
   *
   */
  using ImportCallbackFunctiopn = std::function<void(
      const std::string&, const std::string&, size_t, size_t)>;

  /**
   * @brief Construct a new Common Utils object
   *
   */
  CommonUtils();

  /**
   * @brief Get the Instance object
   *
   * @return CommonUtils*
   */
  static CommonUtils* GetInstance();

 signals:
  /**
   * @brief
   *
   */
  void SignalKeyStatusUpdated();

  /**
   * @brief
   *
   */
  void SignalGnupgNotInstall();

  /**
   * @brief emit when the key database is refreshed
   *
   */
  void SignalKeyDatabaseRefreshDone();

 public slots:
  /**
   * @brief
   *
   * @param parent
   * @param in_buffer
   */
  void SlotImportKeys(QWidget* parent, const std::string& in_buffer);

  /**
   * @brief
   *
   * @param parent
   */
  void SlotImportKeyFromFile(QWidget* parent);

  /**
   * @brief
   *
   * @param parent
   */
  void SlotImportKeyFromKeyServer(QWidget* parent);

  /**
   * @brief
   *
   * @param parent
   */
  void SlotImportKeyFromClipboard(QWidget* parent);

  /**
   * @brief
   *
   * @param ctx_channel
   * @param key_ids
   * @param callback
   */
  static void SlotImportKeyFromKeyServer(
      const GpgFrontend::KeyIdArgsList& key_ids,
      const GpgFrontend::UI::CommonUtils::ImportCallbackFunctiopn& callback);

  /**
   * @brief
   *
   * @param arguments
   * @param interact_func
   */
  void SlotExecuteGpgCommand(
      const QStringList& arguments,
      const std::function<void(QProcess*)>& interact_func);

  /**
   * @brief
   *
   * @param arguments
   * @param interact_func
   */
  void SlotExecuteCommand(
      const std::string& cmd,
      const QStringList& arguments,
      const std::function<void(QProcess*)>& interact_func);

 private slots:

  /**
   * @brief update the key status when signal is emitted
   *
   */
  void slot_update_key_status();

 private:
  static std::unique_ptr<CommonUtils> instance_;  ///<
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_USER_INTERFACE_UTILS_H
