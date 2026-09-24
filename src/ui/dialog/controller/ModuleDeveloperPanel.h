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

#include <optional>

#include "core/module/Event.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;

namespace GpgFrontend::UI {

/**
 * @brief The Module Controller's developer tab: fire an event and watch its
 *        answers, see every module's lifecycle, and invoke a command.
 *
 * Debug builds only -- the dialog hides the tab in a RELEASE build. Nothing
 * here writes module state: events go through the Host's own dispatch and
 * commands through the one registry, as the Host itself; lifecycle changes
 * stay on the first tab.
 */
class GF_UI_EXPORT ModuleDeveloperPanel : public QWidget {
  Q_OBJECT
 public:
  explicit ModuleDeveloperPanel(QWidget* parent = nullptr);

 protected:
  void showEvent(QShowEvent* event) override;

 private slots:
  void slot_event_text_changed(const QString& text);
  void slot_fire_event();
  void slot_refresh_modules();
  void slot_filter_commands(const QString& text);
  void slot_select_command();
  void slot_invoke_command();

 private:
  // Events
  QComboBox* event_box_ = nullptr;
  QLabel* event_summary_ = nullptr;
  QLabel* event_listeners_ = nullptr;
  QPlainTextEdit* event_params_ = nullptr;
  QPushButton* fire_button_ = nullptr;
  QPlainTextEdit* event_log_ = nullptr;

  // Modules
  QTableWidget* module_table_ = nullptr;

  // Commands
  QLineEdit* command_filter_ = nullptr;
  QListWidget* command_list_ = nullptr;
  QPlainTextEdit* command_descriptor_ = nullptr;
  QPlainTextEdit* command_args_ = nullptr;
  QPushButton* invoke_button_ = nullptr;
  QPlainTextEdit* command_log_ = nullptr;

  auto build_events_page() -> QWidget*;
  auto build_modules_page() -> QWidget*;
  auto build_commands_page() -> QWidget*;
  void refresh_commands();

  static void append_log(QPlainTextEdit* log, const QString& line);
};

/**
 * @brief Parse event parameters written one `key=value` per line.
 *
 * Blank lines are skipped. A value may itself contain `=`; the first one
 * splits. A line without `=`, an empty key, or a key given twice is refused.
 *
 * @param error set to the reason, with the 1-based line number, on refusal
 * @return the parameters, or nullopt when a line was refused
 */
auto GF_UI_EXPORT ParseEventParams(const QString& text, QString* error)
    -> std::optional<Module::Event::Params>;

/**
 * @brief Parse command arguments written as one JSON object.
 *
 * Empty text is an empty map: a command that takes no arguments.
 *
 * @param error set to the reason on refusal
 */
auto GF_UI_EXPORT ParseCommandArgs(const QString& text, QString* error)
    -> std::optional<QCborMap>;

/**
 * @brief One log line for one answer to an event.
 *
 * `ret` and the error keys are shown as text; every other parameter only by
 * name and size. An answer may carry the user's plaintext -- a decrypted
 * document comes back as `data` -- and a debug log is no place for it.
 *
 * @param listener the answering module; empty for an answer the Host made
 *        because no module could
 */
auto GF_UI_EXPORT DescribeEventAnswer(const QString& listener,
                                      const Module::Event::Params& params)
    -> QString;

/// The name of a GF_CMD_* status, or "status <n>" for one it does not know.
auto GF_UI_EXPORT DescribeCommandStatus(int status) -> QString;

}  // namespace GpgFrontend::UI
