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

#pragma once

#include "core/GFCoreLog.h"
#include "ui/dialog/GeneralDialog.h"

namespace GpgFrontend::UI {

class LogViewDialog : public GeneralDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Log View Dialog object
   *
   * @param parent
   */
  explicit LogViewDialog(QWidget* parent = nullptr);

  /**
   * @brief Destroy the Log View Dialog object
   *
   */
  ~LogViewDialog() override = default;

 public slots:
  void ReloadLogs();

 private slots:
  void slot_copy_to_clipboard();
  void slot_save_to_file();
  void slot_clear_view();
  void slot_auto_refresh_toggled(bool checked);

 private:
  /**
   * @brief
   *
   */
  void init_ui();

  static auto format_entry(const GFLogEntry& entry) -> QString;
  void update_status_label(qsizetype total);

 private:
  GFLogManager& lm_ = GFLogManager::Instance();
  QPlainTextEdit* log_text_edit_ = nullptr;
  QLineEdit* filter_edit_ = nullptr;
  QLabel* status_label_ = nullptr;
  qsizetype last_log_count_ = 0;

  QPushButton* refresh_button_ = nullptr;
  QPushButton* copy_button_ = nullptr;
  QPushButton* save_button_ = nullptr;
  QPushButton* clear_button_ = nullptr;
  QPushButton* close_button_ = nullptr;

  QCheckBox* auto_refresh_checkbox_ = nullptr;
  QTimer* auto_refresh_timer_ = nullptr;
};
}  // namespace GpgFrontend::UI