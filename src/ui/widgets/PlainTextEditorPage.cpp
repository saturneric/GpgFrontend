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

#include "PlainTextEditorPage.h"

#include "core/model/SettingsObject.h"
#include "core/thread/FileReadTask.h"
#include "core/thread/TaskRunnerGetter.h"
#include "ui/struct/settings_object/AppearanceSO.h"
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

  this->ui_->characterLabel->setText(tr("0 character"));
  this->ui_->lfLabel->setHidden(true);
  this->ui_->encodingLabel->setText("Unicode");
  this->ui_->textPage->setAcceptDrops(false);

  setAcceptDrops(false);

  connect(ui_->textPage, &QPlainTextEdit::textChanged, this, [=]() {
    // if file is loading
    if (!read_done_) return;

    auto text = ui_->textPage->document()->toPlainText();
    auto str = tr("%1 character(s)").arg(text.size());
    this->ui_->characterLabel->setText(str);
  });

  if (full_file_path_.isEmpty()) {
    read_done_ = true;
    ui_->loadingLabel->setHidden(true);
  } else {
    read_done_ = false;
    ui_->loadingLabel->setText(tr("Loading..."));
    ui_->loadingLabel->setHidden(false);
  }
}

const QString &PlainTextEditorPage::GetFilePath() const {
  return full_file_path_;
}

auto PlainTextEditorPage::GetTextPage() -> QPlainTextEdit * {
  return ui_->textPage;
}

auto PlainTextEditorPage::GetPlainText() -> QString {
  return ui_->textPage->toPlainText();
}

void PlainTextEditorPage::NotifyFileSaved() {
  this->is_crlf_ = false;

  this->ui_->lfLabel->setText(tr("lf"));
  this->ui_->encodingLabel->setText(tr("UTF-8"));
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
  auto widgets = findChildren<QWidget *>();
  for (auto *widget : widgets) {
    if (widget->property(className) == true) {
      widget->close();
    }
  }
}

void PlainTextEditorPage::slot_format_gpg_header() {
  QString content = ui_->textPage->toPlainText();

  // Get positions of the gpg-headers, if they exist
  auto start = content.indexOf(GpgFrontend::PGP_SIGNED_BEGIN);
  auto start_sig = content.indexOf(GpgFrontend::PGP_SIGNATURE_BEGIN);
  auto end_sig = content.indexOf(GpgFrontend::PGP_SIGNATURE_END);

  if (start < 0 || start_sig < 0 || end_sig < 0 || sign_marked_) {
    return;
  }

  sign_marked_ = true;

  // Set the fontstyle for the header
  QTextCharFormat sign_format;
  sign_format.setForeground(QBrush(QColor::fromRgb(80, 80, 80)));
  sign_format.setFontPointSize(9);

  // set font style for the signature
  QTextCursor cursor(ui_->textPage->document());
  cursor.setPosition(start_sig, QTextCursor::MoveAnchor);
  cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, end_sig);
  cursor.setCharFormat(sign_format);

  // set the font style for the header
  int head_end = content.indexOf("\n\n", start);
  cursor.setPosition(start, QTextCursor::MoveAnchor);
  cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, head_end);
  cursor.setCharFormat(sign_format);
}

void PlainTextEditorPage::ReadFile() {
  read_done_ = false;
  read_bytes_ = 0;

  auto *text_page = this->GetTextPage();
  text_page->setEnabled(false);
  text_page->setReadOnly(true);
  text_page->blockSignals(true);
  text_page->document()->blockSignals(true);
  ui_->loadingLabel->setHidden(false);

  const auto target_path = this->full_file_path_;

  auto task_runner =
      GpgFrontend::Thread::TaskRunnerGetter::GetInstance().GetTaskRunner();

  auto *read_task = new FileReadTask(target_path);
  connect(read_task, &FileReadTask::SignalFileBytesRead, this,
          &PlainTextEditorPage::slot_insert_text, Qt::QueuedConnection);
  connect(this, &PlainTextEditorPage::SignalUIBytesDisplayed, read_task,
          &FileReadTask::SignalFileBytesReadNext, Qt::QueuedConnection);

  connect(this, &PlainTextEditorPage::close, read_task,
          [=]() { emit read_task->SignalTaskShouldEnd(0); });
  connect(read_task, &FileReadTask::SignalFileBytesReadEnd, this, [=]() {
    // set the UI
    FLOG_D("file read done");
    this->read_done_ = true;
    text_page->setEnabled(true);
    text_page->document()->setModified(false);
    text_page->blockSignals(false);
    text_page->document()->blockSignals(false);
    text_page->setReadOnly(false);
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
  // If the previous data ended with a '\r' and the current data starts with
  // '\n', combine them to form a complete '\r\n' to avoid incorrect line
  // breaks.
  if (last_insert_has_partial_cr_ && !bytes_data.isEmpty() &&
      bytes_data.startsWith('\n')) {
    bytes_data.prepend('\r');  // Prepend '\r' to ensure '\r\n' stays together.
  }

  // Check if the current data ends with '\r'.
  if (!bytes_data.isEmpty() && bytes_data.endsWith('\r')) {
    // If the data ends with '\r', set the flag indicating an incomplete line
    // ending.
    last_insert_has_partial_cr_ = true;
    // Remove the trailing '\r' and hold it for the next chunk of data.
    bytes_data.chop(1);
  } else {
    // If the data does not end with '\r', reset the flag.
    last_insert_has_partial_cr_ = false;
  }

  read_bytes_ += bytes_data.size();

  // insert the text to the text page
  this->ui_->textPage->insertPlainText(bytes_data);
  this->ui_->characterLabel->setText(
      tr("%1 character(s)").arg(this->GetTextPage()->toPlainText().size()));

  QTimer::singleShot(25, this, &PlainTextEditorPage::SignalUIBytesDisplayed);
}

}  // namespace GpgFrontend::UI
