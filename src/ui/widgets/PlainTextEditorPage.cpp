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

#include "core/thread/FileReadTask.h"
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
  this->ui_->lfLabel->setHidden(true);
  this->ui_->encodingLabel->setText("Unicode");

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

void PlainTextEditorPage::NotifyFileSaved() {
  this->is_crlf_ = false;

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

  // insert the text to the text page
  this->GetTextPage()->insertPlainText(bytes_data);

  auto text = this->GetTextPage()->toPlainText();
  auto str = QString(_("%1 character(s)")).arg(text.size());
  this->ui_->characterLabel->setText(str);

  QTimer::singleShot(25, this, &PlainTextEditorPage::SignalUIBytesDisplayed);
}

}  // namespace GpgFrontend::UI
