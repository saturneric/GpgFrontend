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
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/widgets/PlainTextEditorPage.h"

#include <encoding-detect/TextEncodingDetect.h>

#include <boost/format.hpp>
#include <utility>

#include "ui/thread/FileReadThread.h"
#include "ui_PlainTextEditor.h"

namespace GpgFrontend::UI {

PlainTextEditorPage::PlainTextEditorPage(QString filePath, QWidget *parent)
    : QWidget(parent),
      ui_(std::make_shared<Ui_PlainTextEditor>()),
      full_file_path_(std::move(filePath)) {
  ui_->setupUi(this);

  if (full_file_path_.isEmpty()) read_done_ = true;

  ui_->textPage->setFocus();
  ui_->loadingLabel->setHidden(true);

  // Front in same width
  this->setFont({"Courier"});
  this->setAttribute(Qt::WA_DeleteOnClose);

  this->ui_->characterLabel->setText(_("0 character"));
  this->ui_->lfLabel->setText(_("None"));
  this->ui_->encodingLabel->setText(_("Binary"));

  connect(ui_->textPage, &QPlainTextEdit::textChanged, this, [=]() {
    if (!read_done_) return;

    auto text = ui_->textPage->document()->toPlainText();
    auto str = boost::format(_("%1% character(s)")) % text.size();
    this->ui_->characterLabel->setText(str.str().c_str());

    detect_cr_lf(text);
    detect_encoding(text.toStdString());
  });

  ui_->loadingLabel->setText(_("Loading..."));
}

const QString &PlainTextEditorPage::GetFilePath() const {
  return full_file_path_;
}

QPlainTextEdit *PlainTextEditorPage::GetTextPage() { return ui_->textPage; }

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
  int start = content.indexOf(GpgFrontend::GpgConstants::PGP_SIGNED_BEGIN);
  int startSig =
      content.indexOf(GpgFrontend::GpgConstants::PGP_SIGNATURE_BEGIN);
  int endSig = content.indexOf(GpgFrontend::GpgConstants::PGP_SIGNATURE_END);

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
  LOG(INFO) << "called";

  read_done_ = false;
  read_bytes_ = 0;
  ui_->textPage->setEnabled(false);
  ui_->textPage->setReadOnly(true);
  ui_->textPage->blockSignals(true);
  ui_->loadingLabel->setHidden(false);
  ui_->textPage->document()->blockSignals(true);

  auto text_page = this->GetTextPage();
  text_page->setReadOnly(true);
  auto thread = new FileReadThread(this->full_file_path_.toStdString());

  connect(thread, &FileReadThread::SignalSendReadBlock, this,
          &PlainTextEditorPage::slot_insert_text);

  connect(thread, &FileReadThread::SignalReadDone, this, [=]() {
    LOG(INFO) << "thread read done";
    if (!binary_mode_) {
      text_page->setReadOnly(false);
    }
  });

  connect(thread, &FileReadThread::finished, this, [=]() {
    LOG(INFO) << "thread finished";
    thread->deleteLater();
    read_done_ = true;
    read_thread_ = nullptr;
    ui_->textPage->setEnabled(true);
    text_page->document()->setModified(false);
    ui_->textPage->blockSignals(false);
    ui_->textPage->document()->blockSignals(false);
    ui_->loadingLabel->setHidden(true);
  });

  connect(this, &PlainTextEditorPage::destroyed, [=]() {
    LOG(INFO) << "request interruption for read thread";
    if (read_thread_ && thread->isRunning()) thread->requestInterruption();
    read_thread_ = nullptr;
  });
  this->read_thread_ = thread;
  thread->start();
}

std::string binary_to_string(const std::string &source) {
  static char syms[] = "0123456789ABCDEF";
  std::stringstream ss;
  for (unsigned char c : source)
    ss << syms[((c >> 4) & 0xf)] << syms[c & 0xf] << " ";
  return ss.str();
}

void PlainTextEditorPage::slot_insert_text(const std::string &data) {
  LOG(INFO) << "data size" << data.size();
  read_bytes_ += data.size();
  // If binary format is detected, the entire file is converted to binary format
  // for display
  bool if_last_binary_mode = binary_mode_;
  if (!binary_mode_) {
    detect_encoding(data);
  }

  if (binary_mode_) {
    if (if_last_binary_mode != binary_mode_) {
      auto text_buffer =
          ui_->textPage->document()->toRawText().toLocal8Bit().toStdString();
      ui_->textPage->clear();
      this->GetTextPage()->insertPlainText(
          binary_to_string(text_buffer).c_str());
      this->ui_->lfLabel->setText("None");
    }
    this->GetTextPage()->insertPlainText(binary_to_string(data).c_str());

    auto str = boost::format(_("%1% byte(s)")) % read_bytes_;
    this->ui_->characterLabel->setText(str.str().c_str());
  } else {
    this->GetTextPage()->insertPlainText(data.c_str());

    auto text = this->GetTextPage()->toPlainText();
    auto str = boost::format(_("%1% character(s)")) % text.size();
    this->ui_->characterLabel->setText(str.str().c_str());
    detect_cr_lf(text);
  }
}

void PlainTextEditorPage::PrepareToDestroy() {
  if (read_thread_) {
    read_thread_->requestInterruption();
    read_thread_ = nullptr;
  }
}

void PlainTextEditorPage::detect_encoding(const std::string &data) {
  AutoIt::Common::TextEncodingDetect text_detect;
  AutoIt::Common::TextEncodingDetect::Encoding encoding =
      text_detect.DetectEncoding((unsigned char *)(data.data()), data.size());

  if (encoding == AutoIt::Common::TextEncodingDetect::None) {
    binary_mode_ = true;
    ui_->encodingLabel->setText(_("Binary"));
  } else if (encoding == AutoIt::Common::TextEncodingDetect::ASCII) {
    ui_->encodingLabel->setText(_("ASCII(7 bits)"));
  } else if (encoding == AutoIt::Common::TextEncodingDetect::ANSI) {
    ui_->encodingLabel->setText(_("ASCII(8 bits)"));
  } else if (encoding == AutoIt::Common::TextEncodingDetect::UTF8_BOM ||
             encoding == AutoIt::Common::TextEncodingDetect::UTF8_NOBOM) {
    ui_->encodingLabel->setText(_("UTF-8"));
  } else if (encoding == AutoIt::Common::TextEncodingDetect::UTF16_LE_BOM ||
             encoding == AutoIt::Common::TextEncodingDetect::UTF16_LE_NOBOM) {
    ui_->encodingLabel->setText(_("UTF-16"));
  } else if (encoding == AutoIt::Common::TextEncodingDetect::UTF16_BE_BOM ||
             encoding == AutoIt::Common::TextEncodingDetect::UTF16_BE_NOBOM) {
    ui_->encodingLabel->setText(_("UTF-16(BE)"));
  }
}

void PlainTextEditorPage::detect_cr_lf(const QString &data) {
  if (binary_mode_) {
    this->ui_->lfLabel->setText("None");
    return;
  }
  if (data.contains("\r\n")) {
    this->ui_->lfLabel->setText("CRLF");
  } else {
    this->ui_->lfLabel->setText("LF");
  }
}

}  // namespace GpgFrontend::UI
