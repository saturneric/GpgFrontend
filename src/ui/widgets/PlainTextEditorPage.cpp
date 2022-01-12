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
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/widgets/PlainTextEditorPage.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <utility>

#include "ui/encoding/TextEncodingDetect.h"
#include "ui/thread/FileReadThread.h"
#include "ui_PlainTextEditor.h"

namespace GpgFrontend::UI {

PlainTextEditorPage::PlainTextEditorPage(QString filePath, QWidget* parent)
    : QWidget(parent),
      ui(std::make_shared<Ui_PlainTextEditor>()),
      full_file_path_(std::move(filePath)) {
  ui->setupUi(this);

  if (full_file_path_.isEmpty()) read_done_ = true;

  ui->textPage->setFocus();
  ui->loadingLabel->setHidden(true);

  // Front in same width
  this->setFont({"Courier"});
  this->setAttribute(Qt::WA_DeleteOnClose);

  this->ui->characterLabel->setText(_("0 character"));
  this->ui->lfLabel->setText(_("None"));
  this->ui->encodingLabel->setText(_("Binary"));

  connect(ui->textPage, &QPlainTextEdit::textChanged, this, [=]() {
    if (!read_done_) return;

    auto text = ui->textPage->document()->toPlainText();
    auto str = boost::format(_("%1% character(s)")) % text.size();
    this->ui->characterLabel->setText(str.str().c_str());

    detect_encoding(text.toStdString());
  });

  ui->loadingLabel->setText(_("Loading..."));
}

const QString& PlainTextEditorPage::getFilePath() const {
  return full_file_path_;
}

QPlainTextEdit* PlainTextEditorPage::getTextPage() { return ui->textPage; }

void PlainTextEditorPage::setFilePath(const QString& filePath) {
  full_file_path_ = filePath;
}

void PlainTextEditorPage::showNotificationWidget(QWidget* widget,
                                                 const char* className) {
  widget->setProperty(className, true);
  ui->verticalLayout->addWidget(widget);
}

void PlainTextEditorPage::closeNoteByClass(const char* className) {
  QList<QWidget*> widgets = findChildren<QWidget*>();
  for (QWidget* widget : widgets) {
    if (widget->property(className) == true) {
      widget->close();
    }
  }
}

void PlainTextEditorPage::slotFormatGpgHeader() {
  QString content = ui->textPage->toPlainText();

  // Get positions of the gpg-headers, if they exist
  int start = content.indexOf(GpgFrontend::GpgConstants::PGP_SIGNED_BEGIN);
  int startSig =
      content.indexOf(GpgFrontend::GpgConstants::PGP_SIGNATURE_BEGIN);
  int endSig = content.indexOf(GpgFrontend::GpgConstants::PGP_SIGNATURE_END);

  if (start < 0 || startSig < 0 || endSig < 0 || signMarked) {
    return;
  }

  signMarked = true;

  // Set the fontstyle for the header
  QTextCharFormat signFormat;
  signFormat.setForeground(QBrush(QColor::fromRgb(80, 80, 80)));
  signFormat.setFontPointSize(9);

  // set font style for the signature
  QTextCursor cursor(ui->textPage->document());
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
  ui->textPage->setEnabled(false);
  ui->textPage->setReadOnly(true);
  ui->textPage->blockSignals(true);
  ui->loadingLabel->setHidden(false);
  ui->textPage->document()->blockSignals(true);

  auto text_page = this->getTextPage();
  text_page->setReadOnly(true);
  auto thread = new FileReadThread(this->full_file_path_.toStdString());

  connect(thread, &FileReadThread::sendReadBlock, this,
          &PlainTextEditorPage::slotInsertText);

  connect(thread, &FileReadThread::readDone, this, [=]() {
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
    ui->textPage->setEnabled(true);
    text_page->document()->setModified(false);
    ui->textPage->blockSignals(false);
    ui->textPage->document()->blockSignals(false);
    ui->loadingLabel->setHidden(true);
  });

  connect(this, &PlainTextEditorPage::destroyed, [=]() {
    LOG(INFO) << "request interruption for read thread";
    thread->requestInterruption();
    read_thread_ = nullptr;
  });
  this->read_thread_ = thread;
  thread->start();
}

std::string binary_to_string(const std::string& source) {
  static char syms[] = "0123456789ABCDEF";
  std::stringstream ss;
  for (unsigned char c : source)
    ss << syms[((c >> 4) & 0xf)] << syms[c & 0xf] << " ";
  return ss.str();
}

void PlainTextEditorPage::slotInsertText(const std::string& data) {
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
          ui->textPage->document()->toRawText().toLocal8Bit().toStdString();
      ui->textPage->clear();
      this->getTextPage()->insertPlainText(
          binary_to_string(text_buffer).c_str());
      this->ui->lfLabel->setText("None");
    }
    this->getTextPage()->insertPlainText(binary_to_string(data).c_str());

    auto str = boost::format(_("%1% byte(s)")) % read_bytes_;
    this->ui->characterLabel->setText(str.str().c_str());
  } else {
    this->getTextPage()->insertPlainText(data.c_str());

    auto text = this->getTextPage()->toPlainText();
    auto str = boost::format(_("%1% character(s)")) % text.size();
    this->ui->characterLabel->setText(str.str().c_str());
    detect_cr_lf(text);
  }
}

void PlainTextEditorPage::PrepareToDestroy() {
  if (read_thread_) {
    read_thread_->requestInterruption();
    read_thread_ = nullptr;
  }
}

void PlainTextEditorPage::detect_encoding(const std::string& data) {
  AutoIt::Common::TextEncodingDetect text_detect;
  AutoIt::Common::TextEncodingDetect::Encoding encoding =
      text_detect.DetectEncoding((unsigned char*)(data.data()), data.size());

  if (encoding == AutoIt::Common::TextEncodingDetect::None) {
    binary_mode_ = true;
    ui->encodingLabel->setText(_("Binary"));
  } else if (encoding == AutoIt::Common::TextEncodingDetect::ASCII) {
    ui->encodingLabel->setText(_("ASCII(7 bits)"));
  } else if (encoding == AutoIt::Common::TextEncodingDetect::ANSI) {
    ui->encodingLabel->setText(_("ASCII(8 bits)"));
  } else if (encoding == AutoIt::Common::TextEncodingDetect::UTF8_BOM ||
             encoding == AutoIt::Common::TextEncodingDetect::UTF8_NOBOM) {
    ui->encodingLabel->setText(_("UTF-8"));
  } else if (encoding == AutoIt::Common::TextEncodingDetect::UTF16_LE_BOM ||
             encoding == AutoIt::Common::TextEncodingDetect::UTF16_LE_NOBOM) {
    ui->encodingLabel->setText(_("UTF-16"));
  } else if (encoding == AutoIt::Common::TextEncodingDetect::UTF16_BE_BOM ||
             encoding == AutoIt::Common::TextEncodingDetect::UTF16_BE_NOBOM) {
    ui->encodingLabel->setText(_("UTF-16(BE)"));
  }
}

void PlainTextEditorPage::detect_cr_lf(const QString& data) {
  if (binary_mode_) {
    this->ui->lfLabel->setText("None");
    return;
  }
  if (data.contains("\r\n")) {
    this->ui->lfLabel->setText("CRLF");
  } else {
    this->ui->lfLabel->setText("LF");
  }
}

}  // namespace GpgFrontend::UI
