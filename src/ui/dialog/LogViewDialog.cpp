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

#include "LogViewDialog.h"

namespace {

auto LogTypeToString(QtMsgType type) -> QString {
  switch (type) {
    case QtDebugMsg:
      return "D";
    case QtInfoMsg:
      return "I";
    case QtWarningMsg:
      return "W";
    case QtCriticalMsg:
      return "C";
    case QtFatalMsg:
      return "F";
  }
  return "?";
}

auto GetRecentLogs() -> QVector<GpgFrontend::GFLogEntry> {
  return GpgFrontend::GFLogManager::Instance().Snapshot();
}
}  // namespace

namespace GpgFrontend::UI {

LogViewDialog::LogViewDialog(QWidget* parent)
    : GeneralDialog(typeid(LogViewDialog).name(), parent) {
  init_ui();
  ReloadLogs();
}

void LogViewDialog::init_ui() {
  setWindowTitle(tr("Application Logs"));
  resize(900, 600);

  log_text_edit_ = new QPlainTextEdit(this);
  log_text_edit_->setReadOnly(true);
  log_text_edit_->setLineWrapMode(QPlainTextEdit::NoWrap);

  refresh_button_ = new QPushButton(tr("Refresh"), this);
  copy_button_ = new QPushButton(tr("Copy"), this);
  save_button_ = new QPushButton(tr("Save"), this);
  clear_button_ = new QPushButton(tr("Clear View"), this);
  close_button_ = new QPushButton(tr("Close"), this);

  auto_refresh_checkbox_ = new QCheckBox(tr("Auto Refresh"), this);

  auto_refresh_timer_ = new QTimer(this);
  auto_refresh_timer_->setInterval(1000);

  auto* button_layout = new QHBoxLayout();
  button_layout->addWidget(refresh_button_);
  button_layout->addWidget(copy_button_);
  button_layout->addWidget(save_button_);
  button_layout->addWidget(clear_button_);
  button_layout->addWidget(auto_refresh_checkbox_);
  button_layout->addStretch();
  button_layout->addWidget(close_button_);

  auto* main_layout = new QVBoxLayout();
  main_layout->addWidget(log_text_edit_);
  main_layout->addLayout(button_layout);

  setLayout(main_layout);

  connect(refresh_button_, &QPushButton::clicked, this,
          &LogViewDialog::ReloadLogs);
  connect(copy_button_, &QPushButton::clicked, this,
          &LogViewDialog::slot_copy_to_clipboard);
  connect(save_button_, &QPushButton::clicked, this,
          &LogViewDialog::slot_save_to_file);
  connect(clear_button_, &QPushButton::clicked, this,
          &LogViewDialog::slot_clear_view);
  connect(close_button_, &QPushButton::clicked, this, &QDialog::accept);

  connect(auto_refresh_checkbox_, &QCheckBox::toggled, this,
          &LogViewDialog::slot_auto_refresh_toggled);
  connect(auto_refresh_timer_, &QTimer::timeout, this,
          &LogViewDialog::ReloadLogs);

  setAttribute(Qt::WA_DeleteOnClose);
}

auto LogViewDialog::build_log_text(const QVector<GFLogEntry>& logs) -> QString {
  QString text;
  text.reserve(logs.size() * 128);

  for (const auto& entry : logs) {
    if (!entry.formatted_message.isEmpty()) {
      text += entry.formatted_message;
      text += '\n';
      continue;
    }

    const QString ts = entry.timestamp.isValid()
                           ? entry.timestamp.toString("yyyyMMdd hh:mm:ss.zzz")
                           : "00000000 00:00:00.000";

    text +=
        QString("[%1] [%2] [%3] %4\n")
            .arg(ts, LogTypeToString(entry.type), entry.category, QString());
  }

  return text;
}

void LogViewDialog::ReloadLogs() {
  const auto logs = GetRecentLogs();
  const QString text = build_log_text(logs);

  const auto old_scroll = log_text_edit_->verticalScrollBar()->value();
  const bool at_bottom =
      old_scroll == log_text_edit_->verticalScrollBar()->maximum();

  log_text_edit_->setPlainText(text);

  if (at_bottom) {
    log_text_edit_->verticalScrollBar()->setValue(
        log_text_edit_->verticalScrollBar()->maximum());
  } else {
    log_text_edit_->verticalScrollBar()->setValue(old_scroll);
  }
}

void LogViewDialog::slot_copy_to_clipboard() {
  auto* clipboard = QGuiApplication::clipboard();
  if (clipboard == nullptr) return;

  clipboard->setText(log_text_edit_->toPlainText());

  QMessageBox::information(this, tr("Copied"),
                           tr("Logs have been copied to the clipboard."));
}

void LogViewDialog::slot_save_to_file() {
  const QString file_path = QFileDialog::getSaveFileName(
      this, tr("Save Logs"),
      QString("logs_%1.txt")
          .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
      tr("Text Files (*.txt);;All Files (*)"));

  if (file_path.isEmpty()) return;

  QSaveFile file(file_path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(this, tr("Save Failed"),
                         tr("Unable to open file for writing."));
    return;
  }

  QTextStream out(&file);
  out.setEncoding(QStringConverter::Utf8);
  out << log_text_edit_->toPlainText();

  if (!file.commit()) {
    QMessageBox::warning(this, tr("Save Failed"),
                         tr("Failed to save the log file."));
    return;
  }

  QMessageBox::information(this, tr("Saved"),
                           tr("Logs have been saved successfully."));
}

void LogViewDialog::slot_clear_view() { log_text_edit_->clear(); }

void LogViewDialog::slot_auto_refresh_toggled(bool checked) {
  if (checked) {
    auto_refresh_timer_->start();
  } else {
    auto_refresh_timer_->stop();
  }
}

}  // namespace GpgFrontend::UI