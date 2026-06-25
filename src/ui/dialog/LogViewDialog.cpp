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

class LogHighlighter : public QSyntaxHighlighter {
 public:
  explicit LogHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {}

 protected:
  void highlightBlock(const QString& text) override {
    static const QRegularExpression kLevelRe(R"(\[([DIWCF])\])");
    const auto match = kLevelRe.match(text);
    if (!match.hasMatch()) return;

    QTextCharFormat fmt;
    const auto level = match.captured(1);

    if (level == "D") {
      fmt.setForeground(QColor(130, 130, 130));
    } else if (level == "W") {
      fmt.setForeground(QColor(200, 130, 0));
    } else if (level == "C") {
      fmt.setForeground(QColor(220, 60, 60));
    } else if (level == "F") {
      fmt.setForeground(QColor(200, 0, 0));
      fmt.setFontWeight(QFont::Bold);
    } else {
      return;
    }

    setFormat(0, static_cast<int>(text.length()), fmt);
  }
};

}  // namespace

namespace GpgFrontend::UI {

LogViewDialog::LogViewDialog(QWidget* parent)
    : GeneralDialog(typeid(LogViewDialog).name(), parent) {
  init_ui();
  ReloadLogs();
}

void LogViewDialog::init_ui() {
  setWindowTitle(tr("Application Logs"));
  resize(960, 640);

  // Header row: bold title + live entry count
  auto* title_label = new QLabel(tr("Application Logs"), this);
  QFont title_font = title_label->font();
  title_font.setPointSize(title_font.pointSize() + 2);
  title_font.setBold(true);
  title_label->setFont(title_font);

  status_label_ = new QLabel(tr("No entries"), this);
  status_label_->setStyleSheet(QStringLiteral("color: gray;"));

  auto* header_layout = new QHBoxLayout();
  header_layout->addWidget(title_label);
  header_layout->addStretch();
  header_layout->addWidget(status_label_);

  // Filter bar
  filter_edit_ = new QLineEdit(this);
  filter_edit_->setPlaceholderText(tr("Filter logs…"));
  filter_edit_->setClearButtonEnabled(true);

  // Log text area with monospace font and syntax highlighting
  log_text_edit_ = new QPlainTextEdit(this);
  log_text_edit_->setReadOnly(true);
  log_text_edit_->setLineWrapMode(QPlainTextEdit::NoWrap);

  QFont mono_font(QStringLiteral("Monospace"));
  mono_font.setStyleHint(QFont::TypeWriter);
  log_text_edit_->setFont(mono_font);

  new LogHighlighter(log_text_edit_->document());

  // Visual separator above button bar
  auto* separator = new QFrame(this);
  separator->setFrameShape(QFrame::HLine);
  separator->setFrameShadow(QFrame::Sunken);

  // Buttons
  refresh_button_ = new QPushButton(tr("Refresh"), this);
  copy_button_ = new QPushButton(tr("Copy"), this);
  save_button_ = new QPushButton(tr("Save"), this);
  clear_button_ = new QPushButton(tr("Clear View"), this);
  close_button_ = new QPushButton(tr("Close"), this);
  close_button_->setDefault(true);

  auto_refresh_checkbox_ = new QCheckBox(tr("Auto Refresh"), this);
  auto_refresh_timer_ = new QTimer(this);
  auto_refresh_timer_->setInterval(1000);

  auto* button_layout = new QHBoxLayout();
  button_layout->setSpacing(6);
  button_layout->addWidget(refresh_button_);
  button_layout->addWidget(copy_button_);
  button_layout->addWidget(save_button_);
  button_layout->addWidget(clear_button_);
  button_layout->addSpacing(8);
  button_layout->addWidget(auto_refresh_checkbox_);
  button_layout->addStretch();
  button_layout->addWidget(close_button_);

  auto* main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(16, 16, 16, 12);
  main_layout->setSpacing(10);
  main_layout->addLayout(header_layout);
  main_layout->addWidget(filter_edit_);
  main_layout->addWidget(log_text_edit_);
  main_layout->addWidget(separator);
  main_layout->addLayout(button_layout);

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
  connect(filter_edit_, &QLineEdit::textChanged, this, [this]() -> void {
    log_text_edit_->clear();
    last_log_count_ = 0;
    ReloadLogs();
  });

  setAttribute(Qt::WA_DeleteOnClose);
}

auto LogViewDialog::format_entry(const GFLogEntry& entry) -> QString {
  if (!entry.formatted_message.isEmpty()) return entry.formatted_message;

  const QString ts = entry.timestamp.isValid()
                         ? entry.timestamp.toString("yyyyMMdd hh:mm:ss.zzz")
                         : "00000000 00:00:00.000";

  return QString("[%1] [%2] [%3] %4")
      .arg(ts, LogTypeToString(entry.type), entry.category, entry.raw_message);
}

void LogViewDialog::ReloadLogs() {
  const auto logs = lm_.Snapshot();
  const QString filter = filter_edit_->text().trimmed();

  const bool at_bottom = log_text_edit_->verticalScrollBar()->value() ==
                         log_text_edit_->verticalScrollBar()->maximum();

  if (filter.isEmpty()) {
    if (logs.size() == last_log_count_) {
      update_status_label(logs.size());
      return;
    }

    if (logs.size() < last_log_count_) {
      log_text_edit_->clear();
      last_log_count_ = 0;
    }

    for (qsizetype i = last_log_count_; i < logs.size(); ++i) {
      log_text_edit_->appendPlainText(format_entry(logs[i]));
    }
    last_log_count_ = logs.size();
  } else {
    log_text_edit_->clear();
    for (const auto& entry : logs) {
      const auto line = format_entry(entry);
      if (line.contains(filter, Qt::CaseInsensitive)) {
        log_text_edit_->appendPlainText(line);
      }
    }
  }

  if (at_bottom) {
    log_text_edit_->verticalScrollBar()->setValue(
        log_text_edit_->verticalScrollBar()->maximum());
  }

  update_status_label(logs.size());
}

void LogViewDialog::update_status_label(qsizetype total) {
  status_label_->setText(tr("%1 entries").arg(total));
}

void LogViewDialog::slot_copy_to_clipboard() {
  auto* clipboard = QGuiApplication::clipboard();
  if (clipboard == nullptr) return;
  clipboard->setText(log_text_edit_->toPlainText());
}

void LogViewDialog::slot_save_to_file() {
  const QString file_path = QFileDialog::getSaveFileName(
      this, tr("Save Logs"),
      QString("gf_logs_%1.txt")
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  out.setEncoding(QStringConverter::Utf8);
#else
  out.setCodec(QTextCodec::codecForName("UTF-8"));
#endif
  out << log_text_edit_->toPlainText();

  if (!file.commit()) {
    QMessageBox::warning(this, tr("Save Failed"),
                         tr("Failed to save the log file."));
    return;
  }

  QMessageBox::information(this, tr("Saved"),
                           tr("Logs have been saved successfully."));
}

void LogViewDialog::slot_clear_view() {
  log_text_edit_->clear();
  last_log_count_ = 0;
}

void LogViewDialog::slot_auto_refresh_toggled(bool checked) {
  clear_button_->setEnabled(!checked);
  if (checked) {
    auto_refresh_timer_->start();
  } else {
    auto_refresh_timer_->stop();
  }
}

}  // namespace GpgFrontend::UI