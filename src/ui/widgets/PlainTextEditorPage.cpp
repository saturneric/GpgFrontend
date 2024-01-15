/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "PlainTextEditorPage.h"

#include <utility>

#include "core/function/CharsetOperator.h"
#include "core/thread/FileReadTask.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "ui/struct/SettingsObject.h"
#include "ui/struct/settings/AppearanceSO.h"
#include "ui_PlainTextEditor.h"

namespace GpgFrontend::UI {

PlainTextEditorPage::PlainTextEditorPage(QString file_path, QWidget *parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_PlainTextEditor>()),
      full_file_path_(std::move(file_path)) {
  ui_->setupUi(this);

  ui_->textPage->setFocus();
  ui_->loadingLabel->setHidden(true);

  // font size
  AppearanceSO appearance(SettingsObject("general_settings_state"));
  ui_->textPage->setFont(QFont("Courier", appearance.text_editor_font_size));

  this->setAttribute(Qt::WA_DeleteOnClose);

  this->ui_->characterLabel->setText(_("0 character"));
  this->ui_->lfLabel->setText(_("lf"));
  this->ui_->encodingLabel->setText(_("utf-8"));

  connect(ui_->textPage, &QPlainTextEdit::textChanged, this, [=]() {
    // if file is loading
    if (!read_done_) return;

    auto text = ui_->textPage->document()->toPlainText();
    auto str = QString(_("%1 character(s)")).arg(text.size());
    this->ui_->characterLabel->setText(str);
  });

  if (full_file_path_.isEmpty()) {
    read_done_ = true;
    ui_->loadingLabel->setHidden(true);
  } else {
    read_done_ = false;
    ui_->loadingLabel->setText(_("Loading..."));
    ui_->loadingLabel->setHidden(false);
  }
}

const QString &PlainTextEditorPage::GetFilePath() const {
  return full_file_path_;
}

QPlainTextEdit *PlainTextEditorPage::GetTextPage() { return ui_->textPage; }

bool PlainTextEditorPage::WillCharsetChange() const {
  // detect if the line-ending will change
  if (is_crlf_) return true;

  // detect if the charset of the file will change
  return charset_name_ != "UTF-8" && charset_name_ != "ISO-8859-1";
}

void PlainTextEditorPage::NotifyFileSaved() {
  this->is_crlf_ = false;
  this->charset_confidence_ = 100;
  this->charset_name_ = "UTF-8";

  this->ui_->lfLabel->setText(_("lf"));
  this->ui_->encodingLabel->setText(_("UTF-8"));
}

void PlainTextEditorPage::SetFilePath(const QString &filePath) {
  full_file_path_ = filePath;
}

void PlainTextEditorPage::ShowNotificationWidget(QWidget *widget,
                                                 const char *className) {
  widget->setProperty(className, true);
  ui_->verticalLayout->addWidget(widget);
}

void PlainTextEditorPage::CloseNoteByClass(const char *className) {
  QList<QWidget *> widgets = findChildren<QWidget *>();
  for (QWidget *widget : widgets) {
    if (widget->property(className) == true) {
      widget->close();
    }
  }
}

void PlainTextEditorPage::slot_format_gpg_header() {
  QString content = ui_->textPage->toPlainText();

  // Get positions of the gpg-headers, if they exist
  int start = content.indexOf(GpgFrontend::PGP_SIGNED_BEGIN);
  int startSig = content.indexOf(GpgFrontend::PGP_SIGNATURE_BEGIN);
  int endSig = content.indexOf(GpgFrontend::PGP_SIGNATURE_END);

  if (start < 0 || startSig < 0 || endSig < 0 || sign_marked_) {
    return;
  }

  sign_marked_ = true;

  // Set the fontstyle for the header
  QTextCharFormat signFormat;
  signFormat.setForeground(QBrush(QColor::fromRgb(80, 80, 80)));
  signFormat.setFontPointSize(9);

  // set font style for the signature
  QTextCursor cursor(ui_->textPage->document());
  cursor.setPosition(startSig, QTextCursor::MoveAnchor);
  cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, endSig);
  cursor.setCharFormat(signFormat);

  // set the font style for the header
  int headEnd = content.indexOf("\n\n", start);
  cursor.setPosition(start, QTextCursor::MoveAnchor);
  cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, headEnd);
  cursor.setCharFormat(signFormat);
}

void PlainTextEditorPage::ReadFile() {
  read_done_ = false;
  read_bytes_ = 0;
  ui_->textPage->setEnabled(false);
  ui_->textPage->setReadOnly(true);
  ui_->textPage->blockSignals(true);
  ui_->loadingLabel->setHidden(false);
  ui_->textPage->document()->blockSignals(true);

  auto *text_page = this->GetTextPage();
  text_page->setReadOnly(true);

  const auto target_path = this->full_file_path_;

  auto task_runner =
      GpgFrontend::Thread::TaskRunnerGetter::GetInstance().GetTaskRunner();

  auto *read_task = new FileReadTask(target_path);
  connect(read_task, &FileReadTask::SignalFileBytesRead, this,
          &PlainTextEditorPage::slot_insert_text, Qt::QueuedConnection);
  connect(this, &PlainTextEditorPage::SignalUIBytesDisplayed, read_task,
          &FileReadTask::SignalFileBytesReadNext, Qt::QueuedConnection);

  connect(read_task, &FileReadTask::SignalTaskShouldEnd, this,
          []() { GF_UI_LOG_DEBUG("read thread closed"); });
  connect(this, &PlainTextEditorPage::close, read_task,
          [=]() { read_task->SignalTaskShouldEnd(0); });
  connect(read_task, &FileReadTask::SignalFileBytesReadEnd, this, [=]() {
    // set the UI
    if (!binary_mode_) text_page->setReadOnly(false);
    this->read_done_ = true;
    this->ui_->textPage->setEnabled(true);
    text_page->document()->setModified(false);
    this->ui_->textPage->blockSignals(false);
    this->ui_->textPage->document()->blockSignals(false);
    this->ui_->loadingLabel->setHidden(true);
  });

  task_runner->PostTask(read_task);
}

auto BinaryToString(const QByteArray &source) -> QString {
  static const char kSyms[] = "0123456789ABCDEF";
  QString buffer;
  QTextStream ss(&buffer);
  for (auto c : source) ss << kSyms[((c >> 4) & 0xf)] << kSyms[c & 0xf] << " ";
  return buffer;
}

void PlainTextEditorPage::slot_insert_text(QByteArray bytes_data) {
  GF_UI_LOG_TRACE("inserting data read to editor, data size: {}",
                  bytes_data.size());
  read_bytes_ += bytes_data.size();
  // If binary format is detected, the entire file is converted to binary
  // format for display.
  bool if_last_binary_mode = binary_mode_;
  if (!binary_mode_ && !read_done_) {
    detect_encoding(bytes_data);
  }

  if (binary_mode_) {
    // change formery displayed text to binary format
    if (if_last_binary_mode != binary_mode_) {
      auto text_buffer = ui_->textPage->document()->toRawText().toLocal8Bit();
      ui_->textPage->clear();
      this->GetTextPage()->insertPlainText(BinaryToString(text_buffer));
      this->ui_->lfLabel->setText("None");
    }

    // insert new data
    this->GetTextPage()->insertPlainText(BinaryToString(bytes_data));

    // update the size of the file
    auto str = QString(_("%1 byte(s)")).arg(read_bytes_);
    this->ui_->characterLabel->setText(str);
  } else {
    // detect crlf/lf line ending
    detect_cr_lf(bytes_data);

    // when reding from a text file
    // try convert the any of thetext to utf8
    QString utf8_data;
    if (!read_done_ && charset_confidence_ > 25) {
      CharsetOperator::Convert2Utf8(bytes_data, utf8_data, charset_name_);
    } else {
      // when editing a text file, do nothing.
      utf8_data = bytes_data;
    }

    // insert the text to the text page
    this->GetTextPage()->insertPlainText(utf8_data);

    auto text = this->GetTextPage()->toPlainText();
    auto str = QString(_("%1 character(s)")).arg(text.size());
    this->ui_->characterLabel->setText(str);
  }
  QTimer::singleShot(25, this, &PlainTextEditorPage::SignalUIBytesDisplayed);
}

void PlainTextEditorPage::detect_encoding(const QString &data) {
  // skip the binary data to avoid the false detection of the encoding
  if (binary_mode_) return;

  // detect the encoding
  auto charset = CharsetOperator::Detect(data);
  this->charset_name_ = std::get<0>(charset);
  this->language_name_ = std::get<1>(charset);
  this->charset_confidence_ = std::get<2>(charset);

  // probably there is no need to detect the encoding again
  if (this->charset_confidence_ < 10) {
    binary_mode_ = true;
  }

  if (binary_mode_) {
    // hide the line ending label, when the file is binary
    this->ui_->lfLabel->setHidden(true);
    this->ui_->encodingLabel->setText(_("binary"));
  } else {
    ui_->encodingLabel->setText(this->charset_name_);
  }
}

void PlainTextEditorPage::detect_cr_lf(const QString &data) {
  if (binary_mode_) {
    return;
  }

  // if contain crlf, set the label to crlf
  if (is_crlf_) return;

  if (data.contains("\r\n")) {
    this->ui_->lfLabel->setText("crlf");
    is_crlf_ = true;
  } else {
    this->ui_->lfLabel->setText("lf");
  }
}

}  // namespace GpgFrontend::UI
