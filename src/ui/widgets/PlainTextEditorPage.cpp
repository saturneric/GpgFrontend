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

#include "core/function/GFBufferFactory.h"
#include "core/model/SettingsObject.h"
#include "core/thread/FileReadTask.h"
#include "core/thread/TaskRunnerGetter.h"
#include "ui/struct/settings_object/AppearanceSO.h"
#include "ui_PlainTextEditor.h"

namespace GpgFrontend::UI {

namespace {

auto FontFamilies() -> QStringList {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return QFontDatabase::families();
#else
  QFontDatabase font_database;
  return font_database.families();
#endif
}

auto ContainsFontFamily(const QString &family) -> bool {
  static const auto kFamilies = FontFamilies();

  return std::any_of(kFamilies.cbegin(), kFamilies.cend(),
                     [&family](const QString &item) {
                       return item.compare(family, Qt::CaseInsensitive) == 0;
                     });
}

auto DefaultMonospaceFont() -> QFont {
  QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  font.setStyleHint(QFont::Monospace);
  font.setFixedPitch(true);
  return font;
}

}  // namespace

PlainTextEditorPage::PlainTextEditorPage(QString file_path, QWidget *parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_PlainTextEditor>()),
      full_file_path_(std::move(file_path)) {
  ui_->setupUi(this);
  init_editor_style();

  ui_->textPage->setFocus();
  this->setAttribute(Qt::WA_DeleteOnClose);

  connect(ui_->textPage, &QPlainTextEdit::textChanged, this, [this]() {
    if (!read_done_) return;

    update_status_bar();
    set_editor_modified(ui_->textPage->document()->isModified());
    sha256_timer_->start();
  });

  connect(ui_->textPage, &QPlainTextEdit::cursorPositionChanged, this,
          [this]() {
            if (!read_done_) return;
            update_status_bar();
          });

  sha256_timer_ = new QTimer(this);
  sha256_timer_->setSingleShot(true);
  sha256_timer_->setInterval(500);
  connect(sha256_timer_, &QTimer::timeout, this,
          &PlainTextEditorPage::slot_update_sha256);

  if (full_file_path_.isEmpty()) {
    read_done_ = true;
    set_loading_state(false);
  } else {
    read_done_ = false;
    set_loading_state(true, tr("Loading..."));
  }
}

void PlainTextEditorPage::init_editor_style() {
  setObjectName(QStringLiteral("PlainTextEditorPage"));

  ui_->textPage->setObjectName(QStringLiteral("PlainTextEditor"));
  ui_->textPage->setAcceptDrops(false);
  ui_->textPage->setLineWrapMode(QPlainTextEdit::WidgetWidth);
  ui_->textPage->setUndoRedoEnabled(true);
  ui_->textPage->setCursorWidth(2);

  AppearanceSO appearance(SettingsObject("general_settings_state"));

  QFont editor_font = DefaultMonospaceFont();
  if (!appearance.text_editor_font_family.isEmpty() &&
      ContainsFontFamily(appearance.text_editor_font_family)) {
    editor_font.setFamily(appearance.text_editor_font_family);
  }
  editor_font.setPointSize(appearance.text_editor_font_size);
  editor_font.setFixedPitch(true);

  ui_->textPage->setFont(editor_font);
  ui_->textPage->setTabStopDistance(
      QFontMetricsF(editor_font).horizontalAdvance(' ') *
      appearance.text_editor_tab_size);

  auto setup_status_label = [](QLabel *label, const QString &width_sample) {
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);

#ifdef Q_OS_MACOS
    font.setPointSize(std::max(10, font.pointSize()));
#else
    font.setPointSize(std::max(9, font.pointSize()));
#endif

    label->setFont(font);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setAlignment(Qt::AlignCenter);
    label->setMinimumWidth(QFontMetrics(font).horizontalAdvance(width_sample));
  };

  ui_->loadingLabel->setHidden(true);
  ui_->loadingLabel->setAlignment(Qt::AlignCenter);
  ui_->loadingLabel->setText(tr("Loading..."));

  ui_->characterLabel->setText(tr("Ln 1, Col 1 · 0 chars"));
  ui_->characterLabel->setToolTip(tr("Number of characters in the editor."));

  ui_->lfLabel->setText(tr("LF"));
  ui_->lfLabel->setToolTip(tr("Line ending style."));
  ui_->lfLabel->setHidden(false);

  ui_->encodingLabel->setText(tr("UTF-8"));
  ui_->encodingLabel->setToolTip(tr("Text encoding."));

  setup_status_label(ui_->characterLabel,
                     QStringLiteral("Ln 9999, Col 999 · 999999 chars · *"));
  setup_status_label(ui_->lfLabel, QStringLiteral("CRLF"));
  setup_status_label(ui_->encodingLabel, QStringLiteral("UTF-8"));
  setup_status_label(ui_->sha256Label,
                     QStringLiteral("SHA256: a1b2c3d4e5f67890…"));

  ui_->sha256Label->setText(QStringLiteral("SHA256: —"));
  ui_->sha256Label->setToolTip(tr("SHA-256 checksum of editor content."));

  setAcceptDrops(false);

  setStyleSheet(R"(
QWidget#PlainTextEditorPage QPlainTextEdit#PlainTextEditor {
  border: 1px solid palette(mid);
  background: palette(base);
  selection-background-color: palette(highlight);
  selection-color: palette(highlighted-text);
}

QWidget#PlainTextEditorPage QLabel {
  padding: 1px 5px;
}

QWidget#PlainTextEditorPage QLabel[loading="true"] {
  color: palette(highlight);
}
)");

  auto polish_label = [](QLabel *label) {
    label->style()->unpolish(label);
    label->style()->polish(label);
  };

  ui_->characterLabel->setProperty("statusBadge", true);
  ui_->lfLabel->setProperty("statusBadge", true);
  ui_->encodingLabel->setProperty("statusBadge", true);
  ui_->sha256Label->setProperty("statusBadge", true);

  polish_label(ui_->characterLabel);
  polish_label(ui_->lfLabel);
  polish_label(ui_->encodingLabel);
  polish_label(ui_->sha256Label);
}

void PlainTextEditorPage::set_loading_state(bool loading,
                                            const QString &message) {
  ui_->loadingLabel->setHidden(!loading);
  ui_->sha256Label->setHidden(loading);
  ui_->loadingLabel->setProperty("loading", loading);
  ui_->loadingLabel->style()->unpolish(ui_->loadingLabel);
  ui_->loadingLabel->style()->polish(ui_->loadingLabel);

  if (loading) {
    ui_->loadingLabel->setText(message.isEmpty() ? tr("Loading...") : message);
  }

  ui_->textPage->setEnabled(!loading);
  ui_->textPage->setReadOnly(loading);
}

void PlainTextEditorPage::update_status_bar() {
  const auto char_count =
      std::max(0, ui_->textPage->document()->characterCount() - 1);

  const auto cursor = ui_->textPage->textCursor();
  const int line = cursor.blockNumber() + 1;
  const int column = cursor.positionInBlock() + 1;
  const auto modified_mark = ui_->textPage->document()->isModified()
                                 ? QStringLiteral(" · *")
                                 : QString();

  ui_->characterLabel->setText(tr("Ln %1, Col %2 · %3 chars%4")
                                   .arg(line)
                                   .arg(column)
                                   .arg(char_count)
                                   .arg(modified_mark));

  ui_->lfLabel->setText(is_crlf_ ? tr("CRLF") : tr("LF"));
  ui_->encodingLabel->setText(tr("UTF-8"));
}

void PlainTextEditorPage::set_editor_modified(bool modified) {
  ui_->characterLabel->setToolTip(modified
                                      ? tr("The document has unsaved changes.")
                                      : tr("The document is unchanged."));
}

void PlainTextEditorPage::closeEvent(QCloseEvent *event) {
  if (ui_ && (ui_->textPage != nullptr)) Clear();
  QWidget::closeEvent(event);
}

auto PlainTextEditorPage::GetFilePath() -> QString { return full_file_path_; }

auto PlainTextEditorPage::GetTextPage() -> QPlainTextEdit * {
  return ui_->textPage;
}

auto PlainTextEditorPage::GetPlainText() -> QString {
  return ui_->textPage->toPlainText();
}

void PlainTextEditorPage::NotifyFileSaved() {
  ui_->textPage->document()->setModified(false);
  set_editor_modified(false);
  update_status_bar();
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
  const QString content = ui_->textPage->toPlainText();

  const auto start = content.indexOf(GpgFrontend::kPgpSignedBegin);
  const auto start_sig = content.indexOf(GpgFrontend::kPgpSignatureBegin);
  const auto end_sig = content.indexOf(GpgFrontend::kPgpSignatureEnd);

  if (start < 0 || start_sig < 0 || end_sig < 0 || sign_marked_) {
    return;
  }

  sign_marked_ = true;

  QTextCharFormat sign_format;
  sign_format.setForeground(QBrush(QColor::fromRgb(110, 110, 110)));
  sign_format.setFontPointSize(
      std::max(8, ui_->textPage->font().pointSize() - 1));

  QTextCursor cursor(ui_->textPage->document());

  const int signature_end =
      end_sig + QString(GpgFrontend::kPgpSignatureEnd).size();

  cursor.setPosition(start_sig, QTextCursor::MoveAnchor);
  cursor.setPosition(signature_end, QTextCursor::KeepAnchor);
  cursor.setCharFormat(sign_format);

  const int head_end = content.indexOf("\n\n", start);
  if (head_end > start) {
    cursor.setPosition(start, QTextCursor::MoveAnchor);
    cursor.setPosition(head_end, QTextCursor::KeepAnchor);
    cursor.setCharFormat(sign_format);
  }
}

void PlainTextEditorPage::slot_update_sha256() {
  if (!read_done_) return;

  const auto* doc = ui_->textPage->document();
  const auto result =
      GFBufferFactory::ToSha256([doc](const GFBufferFactory::Sha256Chunk& update) -> void {
        auto block = doc->begin();
        while (block != doc->end()) {
          const auto utf8 = block.text().toUtf8();
          update(utf8.constData(), static_cast<size_t>(utf8.size()));
          block = block.next();
          if (block != doc->end()) {
            const char nl = '\n';
            update(&nl, 1);
          }
        }
      });

  if (!result) return;

  const auto hex = result->ConvertToQByteArray().toHex();
  ui_->sha256Label->setText(
      QStringLiteral("SHA256: %1…").arg(QString(hex.left(16))));
  ui_->sha256Label->setToolTip(
      QStringLiteral("SHA256: %1").arg(QString(hex)));
}

void PlainTextEditorPage::ReadFile() {
  read_done_ = false;
  read_bytes_ = 0;
  last_insert_has_partial_cr_ = false;
  sign_marked_ = false;
  is_crlf_ = false;

  auto *text_page = this->GetTextPage();

  set_loading_state(true, tr("Loading..."));

  text_page->clear();
  text_page->blockSignals(true);
  text_page->document()->blockSignals(true);
  text_page->setUndoRedoEnabled(false);
  text_page->document()->setModified(false);

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
    FLOG_D("file read done");

    this->read_done_ = true;

    text_page->blockSignals(false);
    text_page->document()->blockSignals(false);
    text_page->setUndoRedoEnabled(true);
    text_page->document()->setModified(false);
    text_page->document()->clearUndoRedoStacks();

    set_loading_state(false);
    update_status_bar();
    slot_format_gpg_header();
    slot_update_sha256();

    text_page->setFocus();
  });

  task_runner->PostTask(read_task);
}

void PlainTextEditorPage::slot_insert_text(QByteArray bytes_data) {
  if (last_insert_has_partial_cr_ && !bytes_data.isEmpty() &&
      bytes_data.startsWith('\n')) {
    bytes_data.prepend('\r');
  }

  if (!bytes_data.isEmpty() && bytes_data.endsWith('\r')) {
    last_insert_has_partial_cr_ = true;
    bytes_data.chop(1);
  } else {
    last_insert_has_partial_cr_ = false;
  }

  if (!is_crlf_ && bytes_data.contains("\r\n")) {
    is_crlf_ = true;
  }

  read_bytes_ += static_cast<size_t>(bytes_data.size());

  ui_->textPage->insertPlainText(QString::fromUtf8(bytes_data));

  update_status_bar();

  if (read_bytes_ > 0) {
    ui_->loadingLabel->setText(tr("Loading... %1 KB").arg(read_bytes_ / 1024));
  }

  QTimer::singleShot(25, this, &PlainTextEditorPage::SignalUIBytesDisplayed);
}

auto PlainTextEditorPage::ReadDone() const -> bool { return this->read_done_; }

void PlainTextEditorPage::Clear() {
  if (ui_ == nullptr || ui_->textPage == nullptr) return;

  auto *editor = ui_->textPage;

  editor->setUndoRedoEnabled(false);

  const auto char_count = editor->document()->characterCount();
  if (char_count > 1) {
    editor->selectAll();
    editor->insertPlainText(QString(char_count - 1, QChar(0x2022)));
  }

  editor->clear();
  editor->document()->clearUndoRedoStacks();
  editor->document()->setModified(false);
  editor->setUndoRedoEnabled(true);

  update_status_bar();
  set_editor_modified(false);
}

void PlainTextEditorPage::ApplyAppearanceSettings() {
  AppearanceSO appearance(SettingsObject("general_settings_state"));

  QFont editor_font = DefaultMonospaceFont();
  if (!appearance.text_editor_font_family.isEmpty() &&
      ContainsFontFamily(appearance.text_editor_font_family)) {
    editor_font.setFamily(appearance.text_editor_font_family);
  }
  editor_font.setPointSize(appearance.text_editor_font_size);
  editor_font.setFixedPitch(true);

  ui_->textPage->setFont(editor_font);
  ui_->textPage->setTabStopDistance(
      QFontMetricsF(editor_font).horizontalAdvance(QLatin1Char(' ')) *
      appearance.text_editor_tab_size);
}

}  // namespace GpgFrontend::UI
