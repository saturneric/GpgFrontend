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

#include <QActionGroup>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QToolButton>

#include "core/function/GFBufferFactory.h"
#include "core/model/SettingsObject.h"
#include "core/thread/FileReadTask.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/MemoryUtils.h"
#include "ui/function/AppearanceFont.h"
#include "ui/function/SecureWipe.h"
#include "ui/function/TextDirection.h"
#include "ui/function/UIStyle.h"
#include "ui/struct/settings_object/AppearanceSO.h"
#include "ui_PlainTextEditor.h"

namespace GpgFrontend::UI {

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

    // The paragraphs re-resolve themselves during layout, so this is only here
    // for the document anchor: which edge the lines align to and which side the
    // gutter sits on. Automatic is the only mode that has an anchor to move,
    // and resolving it stops at the first strong character.
    if (text_direction_mode_ == kTEXT_DIRECTION_AUTO) apply_text_direction();
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

  QFont const editor_font = ResolveAppearanceFont(
      appearance.text_editor_font_family, appearance.text_editor_font_size);

  ui_->textPage->setFont(editor_font);
  ui_->textPage->setTabStopDistance(
      QFontMetricsF(editor_font).horizontalAdvance(' ') *
      appearance.text_editor_tab_size);

  build_text_direction_menu();
  apply_text_direction();

  auto setup_status_label = [](QLabel *label, const QString &width_sample) {
    QFont font = DefaultMonospaceFont();

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
  // The whole status row stays collapsed once a module view is mounted (see
  // MountPrimaryView()), loading or not.
  const bool status_row_hidden = !primary_view_.isNull();
  ui_->loadingLabel->setHidden(status_row_hidden || !loading);
  ui_->sha256Label->setHidden(status_row_hidden || loading);
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
  // Collapsed for good once a module view is mounted (see MountPrimaryView())
  // -- line/column, char count and line-ending all describe the raw editor
  // text, which nobody is looking at behind that view.
  if (!primary_view_.isNull()) return;

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
  // A backstop for close routes other than the tab bar; the tab close and the
  // window close both call WipeContent() directly.
  WipeContent();
  QWidget::closeEvent(event);
}

auto PlainTextEditorPage::GetFilePath() -> QString { return full_file_path_; }

auto PlainTextEditorPage::GetTextPage() -> QPlainTextEdit * {
  return ui_->textPage;
}

auto PlainTextEditorPage::GetPlainText() -> QString {
  return ui_->textPage->toPlainText();
}

void PlainTextEditorPage::SetContentFromBytes(const QByteArray &bytes) {
  // Recorded before the text goes in, because once it is in the document the
  // evidence is gone: the editor stores bare LF either way.
  is_crlf_ = bytes.contains("\r\n");

  ui_->textPage->setPlainText(QString::fromUtf8(bytes));
  ui_->textPage->document()->setModified(false);

  read_done_ = true;
  read_bytes_ = static_cast<size_t>(bytes.size());

  update_status_bar();
}

auto PlainTextEditorPage::DocumentBytes() const -> QByteArray {
  // QPlainTextEdit stores line breaks as paragraph separators, so whatever
  // the document was loaded from, toPlainText() hands back bare LF. Writing
  // that out, or handing it to a view, silently rewrites every line ending in
  // the file.
  //
  // For prose that is merely untidy. For a PGP/MIME message it is fatal: the
  // signature covers exact octets in canonical CRLF form, so a document that
  // came in as CRLF must go back out as CRLF or nothing it carries can ever
  // verify again.
  auto bytes = ui_->textPage->toPlainText().toUtf8();
  if (is_crlf_) bytes.replace('\n', "\r\n");
  return bytes;
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

  // Hidden behind a mounted module view (see MountPrimaryView()) -- skip the
  // full-document hash, there is no label left to show it in.
  if (!primary_view_.isNull()) return;

  const auto *doc = ui_->textPage->document();
  const auto result = GFBufferFactory::ToSha256(
      [doc](const GFBufferFactory::Sha256Chunk &update) -> void {
        auto block = doc->begin();
        while (block != doc->end()) {
          // Wiped rather than dropped: this runs on a debounce timer while
          // the user types, so leaving the copies behind would strew the whole
          // document across the heap every few keystrokes.
          auto utf8 = block.text().toUtf8();
          update(utf8.constData(), static_cast<size_t>(utf8.size()));
          WipeByteArray(utf8);
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
  ui_->sha256Label->setToolTip(QStringLiteral("SHA256: %1").arg(QString(hex)));
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
    apply_text_direction();

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

  WipeTextDocument(ui_->textPage->document());

  update_status_bar();
  set_editor_modified(false);
}

void PlainTextEditorPage::WipeContent() {
  // The primary view holds its own copy of whatever it parsed out of the
  // document -- a decrypted body, attachment buffers -- and clearing the
  // document alone would leave all of that in memory. Ask it first, while it
  // still exists, then clear the document.
  invoke_primary_view("WipeContent");
  Clear();
}

auto PlainTextEditorPage::MountPrimaryView(QWidget *view) -> bool {
  if (view == nullptr) {
    LOG_W() << "refusing to mount a null primary view";
    return false;
  }

  if (primary_view_ != nullptr) {
    LOG_W() << "page already has a primary view";
    return false;
  }

  primary_view_ = view;
  view->setParent(this);

  // A mounted module view (currently just email) replaces the raw editor as
  // what the tab is actually about: line/column, char count, line-ending and
  // checksum all describe the raw editor text, which nobody is looking at
  // behind that view. Collapse the whole status row rather than leave an
  // empty strip -- shrink the spacer and margins along with the labels, or
  // the row keeps its height even with every label hidden.
  ui_->loadingLabel->setHidden(true);
  ui_->sha256Label->setHidden(true);
  ui_->characterLabel->setHidden(true);
  ui_->lfLabel->setHidden(true);
  ui_->encodingLabel->setHidden(true);
  ui_->horizontalSpacer->changeSize(0, 0, QSizePolicy::Fixed,
                                    QSizePolicy::Fixed);
  ui_->horizontalLayout->setContentsMargins(0, 0, 0, 0);
  ui_->horizontalLayout->invalidate();

  // A view may present the raw document itself, as one more tab beside its own
  // rather than behind a switcher the page puts above it. It is handed the
  // real editor, not a copy: the document stays the one canonical content of
  // the tab, and editing the raw source keeps working exactly as before.
  source_view_adopted_ =
      view->metaObject()->indexOfMethod("AdoptSourceView(QWidget*)") >= 0;

  const int editor_index = ui_->verticalLayout->indexOf(ui_->textPage);
  ui_->verticalLayout->insertWidget(editor_index, view);

  if (source_view_adopted_) {
    QMetaObject::invokeMethod(view, "AdoptSourceView", Qt::DirectConnection,
                              Q_ARG(QWidget *, ui_->textPage));

    // The view owns the choice of what is on screen, so the page stops
    // hiding either side. It only needs to hear when the raw document is
    // about to be read, so it can be made current first.
    if (view->metaObject()->indexOfSignal("SignalSourceViewRequested()") >= 0) {
      connect(view, SIGNAL(SignalSourceViewRequested()), this,
              SLOT(slot_flush_before_source_view()));
    }
  } else {
    build_view_switcher();
    // The switcher sits above both views, the mounted view directly above the
    // editor. The editor keeps its place in the layout so everything that
    // reaches for it by name still finds it.
    ui_->verticalLayout->insertWidget(editor_index, view_switcher_);
  }

  // Content can also arrive from outside the view: a file being opened, or the
  // result of a crypto operation replacing the whole document. The view has to
  // follow that, but must not react to its own writes coming back -- hence the
  // flag and, for the queued case where the flag has already been cleared, the
  // revision check.
  connect(ui_->textPage->document(), &QTextDocument::contentsChanged, this,
          [this]() {
            if (primary_view_syncing_ || primary_view_.isNull()) return;
            if (ui_->textPage->document()->revision() == source_generation_) {
              return;
            }
            ReloadPrimaryView();
          });

  // Eager marking, lazy serialization: the view says "I changed" the moment an
  // edit happens, which is what makes closing the tab right afterwards prompt
  // to save. Reserializing the document is deferred until something actually
  // needs to read it.
  if (view->metaObject()->indexOfSignal("SignalContentModified()") >= 0) {
    connect(view, SIGNAL(SignalContentModified()), this,
            SLOT(slot_primary_view_modified()));
  }

  // A view may offer the crypto operations from inside the message itself --
  // a Decrypt button on an encrypted mail, say. It only names the operation;
  // running it stays with the host, so there is exactly one implementation of
  // each and no way for the two entry points to diverge.
  if (view->metaObject()->indexOfSignal(
          "SignalCryptoOperationRequested(QString)") >= 0) {
    connect(view, SIGNAL(SignalCryptoOperationRequested(QString)), this,
            SLOT(slot_primary_view_crypto_operation_requested(QString)));
  }

  // A view may also say which of those operations mean anything for what it
  // currently holds, and tell us when that changes.
  if (view->metaObject()->indexOfSignal("SignalCryptoOperationsChanged()") >=
      0) {
    connect(view, SIGNAL(SignalCryptoOperationsChanged()), this,
            SIGNAL(SignalCryptoOperationsChanged()));
  }

  if (!source_view_adopted_) show_primary_view(true);

  // The editor font was resolved in the constructor, before this view existed.
  // Re-applying now is what gets the user's chosen font and size into it from
  // the start rather than only after they next change a setting.
  ApplyAppearanceSettings();

  ReloadPrimaryView();
  return true;
}

void PlainTextEditorPage::slot_flush_before_source_view() {
  FlushPrimaryView();
}

void PlainTextEditorPage::slot_primary_view_crypto_operation_requested(
    const QString &operation) {
  emit SignalCryptoOperationRequested(operation);
}

auto PlainTextEditorPage::AttachPublicKeyToPrimaryView(const QByteArray &key,
                                                       const QString &name)
    -> int {
  if (primary_view_.isNull()) return 0;

  auto *view = primary_view_.data();
  if (view->metaObject()->indexOfMethod("AttachPublicKey(QByteArray,QString)") <
      0) {
    return 0;
  }

  int result = 0;
  QMetaObject::invokeMethod(view, "AttachPublicKey", Qt::DirectConnection,
                            Q_RETURN_ARG(int, result), Q_ARG(QByteArray, key),
                            Q_ARG(QString, name));
  return result;
}

auto PlainTextEditorPage::PrimaryViewCryptoOperations(bool &has_opinion) const
    -> QStringList {
  has_opinion = false;
  if (primary_view_.isNull()) return {};

  auto *view = primary_view_.data();
  if (view->metaObject()->indexOfMethod("AvailableCryptoOperations()") < 0) {
    return {};
  }

  QStringList operations;
  if (!QMetaObject::invokeMethod(view, "AvailableCryptoOperations",
                                 Qt::DirectConnection,
                                 Q_RETURN_ARG(QStringList, operations))) {
    return {};
  }

  has_opinion = true;
  return operations;
}

auto PlainTextEditorPage::PrimaryViewSuggestedFileName() const -> QString {
  if (primary_view_.isNull()) return {};

  auto *view = primary_view_.data();
  if (view->metaObject()->indexOfMethod("SuggestedFileName()") < 0) return {};

  QString name;
  if (!QMetaObject::invokeMethod(view, "SuggestedFileName",
                                 Qt::DirectConnection,
                                 Q_RETURN_ARG(QString, name))) {
    return {};
  }
  return name;
}

auto PlainTextEditorPage::PrimaryViewFileTypeFilter() const -> QString {
  if (primary_view_.isNull()) return {};

  auto *view = primary_view_.data();
  if (view->metaObject()->indexOfMethod("FileTypeFilter()") < 0) return {};

  QString filter;
  if (!QMetaObject::invokeMethod(view, "FileTypeFilter", Qt::DirectConnection,
                                 Q_RETURN_ARG(QString, filter))) {
    return {};
  }
  return filter;
}

auto PlainTextEditorPage::AppendTextToPrimaryView(const QString &text) -> int {
  if (primary_view_.isNull()) return 0;

  auto *view = primary_view_.data();
  if (view->metaObject()->indexOfMethod("AppendBodyText(QString)") < 0) {
    return 0;
  }

  int result = 0;
  QMetaObject::invokeMethod(view, "AppendBodyText", Qt::DirectConnection,
                            Q_RETURN_ARG(int, result), Q_ARG(QString, text));
  return result;
}

void PlainTextEditorPage::slot_primary_view_modified() {
  if (primary_view_syncing_) return;
  ui_->textPage->document()->setModified(true);
  set_editor_modified(true);
}

auto PlainTextEditorPage::PrimaryView() const -> QWidget * {
  return primary_view_.data();
}

void PlainTextEditorPage::FlushPrimaryView() {
  if (primary_view_.isNull() || primary_view_syncing_) return;
  if (!PrimaryViewIsDirty()) return;

  const auto *meta = primary_view_->metaObject();
  if (meta->indexOfMethod("SaveToSource()") < 0) return;

  QByteArray bytes;
  if (!QMetaObject::invokeMethod(primary_view_.data(), "SaveToSource",
                                 Qt::DirectConnection,
                                 Q_RETURN_ARG(QByteArray, bytes))) {
    LOG_W() << "primary view SaveToSource failed";
    return;
  }

  // The view has just told us what the message's canonical form is. A message
  // it built with CRLF endings has to be remembered as a CRLF document, or
  // saving it would write back the LF the editor stores internally and break
  // the signature that was just made over it.
  if (bytes.contains("\r\n")) is_crlf_ = true;

  // Guarded on both sides: this write raises contentsChanged, and without the
  // flag the handler would push the text straight back into the view, which
  // would mark it dirty again and flush again.
  {
    primary_view_syncing_ = true;
    ui_->textPage->setPlainText(QString::fromUtf8(bytes));
    primary_view_syncing_ = false;
  }
  source_generation_ = ui_->textPage->document()->revision();

  // setPlainText() CLEARS the modified flag, which would say this document
  // matches a file on disk. Re-asserted rather than restored: a flush only
  // happens when the view had edits the document did not have, so the document
  // is modified afterwards whatever it was before.
  ui_->textPage->document()->setModified(true);
}

auto PlainTextEditorPage::PrimaryViewIsDirty() const -> bool {
  if (primary_view_.isNull()) return false;

  const auto *meta = primary_view_->metaObject();
  if (meta->indexOfMethod("IsDirty()") < 0) return false;

  bool dirty = false;
  QMetaObject::invokeMethod(primary_view_.data(), "IsDirty",
                            Qt::DirectConnection, Q_RETURN_ARG(bool, dirty));
  return dirty;
}

void PlainTextEditorPage::ReloadPrimaryView() {
  if (primary_view_.isNull()) return;

  const auto *meta = primary_view_->metaObject();
  if (meta->indexOfMethod("LoadFromSource(QByteArray)") < 0) return;

  if (primary_view_syncing_) return;

  const auto bytes = DocumentBytes();

  primary_view_syncing_ = true;
  QMetaObject::invokeMethod(primary_view_.data(), "LoadFromSource",
                            Qt::DirectConnection, Q_ARG(QByteArray, bytes));
  primary_view_syncing_ = false;

  source_generation_ = ui_->textPage->document()->revision();
}

void PlainTextEditorPage::invoke_primary_view(const char *method) {
  if (primary_view_.isNull()) return;

  // Probed rather than assumed: the contract is additive, so a view that does
  // not declare a member simply does not take part in that step.
  const auto signature = QByteArray(method) + "()";
  if (primary_view_->metaObject()->indexOfMethod(signature.constData()) < 0) {
    return;
  }

  QMetaObject::invokeMethod(primary_view_.data(), method, Qt::DirectConnection);
}

void PlainTextEditorPage::build_view_switcher() {
  view_switcher_ = new QWidget(this);
  auto *layout = new QHBoxLayout();
  layout->setContentsMargins(6, 2, 6, 0);
  layout->setSpacing(2);

  // Flat and adjacent, so the pair reads as two views of one document rather
  // than as two buttons that do something.
  const auto make_tab = [this](const QString &text, bool checked) {
    auto *button = new QToolButton(view_switcher_);
    button->setText(text);
    button->setCheckable(true);
    button->setChecked(checked);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setCursor(Qt::PointingHandCursor);
    return button;
  };

  auto *message_button = make_tab(tr("Message"), true);
  auto *source_button = make_tab(tr("Raw Source"), false);

  auto *group = new QButtonGroup(view_switcher_);
  group->setExclusive(true);
  group->addButton(message_button);
  group->addButton(source_button);

  layout->addWidget(message_button);
  layout->addWidget(source_button);
  layout->addStretch();

  // A hairline under the row, which is what turns two flat buttons into a tab
  // strip. Drawn from the palette so it follows the theme, like every other
  // rule in the application.
  auto *rule = new QFrame(view_switcher_);
  rule->setFrameShape(QFrame::HLine);
  rule->setFrameShadow(QFrame::Plain);
  rule->setFixedHeight(1);
  auto rule_palette = rule->palette();
  rule_palette.setColor(QPalette::WindowText, BorderColor(palette()));
  rule->setPalette(rule_palette);

  auto *column = new QVBoxLayout();
  column->setContentsMargins(0, 0, 0, 0);
  column->setSpacing(0);
  column->addLayout(layout);
  column->addWidget(rule);

  auto *wrapper = new QVBoxLayout(view_switcher_);
  wrapper->setContentsMargins(0, 0, 0, 0);
  wrapper->setSpacing(0);
  wrapper->addLayout(column);

  connect(message_button, &QToolButton::clicked, this,
          [this]() { show_primary_view(true); });
  connect(source_button, &QToolButton::clicked, this,
          [this]() { show_primary_view(false); });
}

void PlainTextEditorPage::show_primary_view(bool primary) {
  if (primary_view_.isNull()) return;

  // Leaving the structured view means the raw document is about to be read by
  // a human, so it has to be current first.
  if (!primary) FlushPrimaryView();

  primary_view_->setVisible(primary);
  ui_->textPage->setVisible(!primary);

  if (primary) {
    ReloadPrimaryView();
  } else {
    ui_->textPage->setFocus();
  }
}

void PlainTextEditorPage::ApplyAppearanceSettings() {
  AppearanceSO appearance(SettingsObject("general_settings_state"));

  QFont const editor_font = ResolveAppearanceFont(
      appearance.text_editor_font_family, appearance.text_editor_font_size);

  ui_->textPage->setFont(editor_font);
  ui_->textPage->setTabStopDistance(
      QFontMetricsF(editor_font).horizontalAdvance(QLatin1Char(' ')) *
      appearance.text_editor_tab_size);

  // A mounted view edits the same document in the same tab, so the editor
  // font the user chose has to reach it too -- otherwise the setting appears
  // to do nothing for exactly the tabs that have a view. Only the view knows
  // which of its widgets are editors rather than chrome, so it is asked
  // rather than restyled from here.
  if (!primary_view_.isNull() && primary_view_->metaObject()->indexOfMethod(
                                     "ApplyEditorFont(QFont)") >= 0) {
    QMetaObject::invokeMethod(primary_view_.data(), "ApplyEditorFont",
                              Qt::DirectConnection, Q_ARG(QFont, editor_font));
  }
}

void PlainTextEditorPage::SetTextDirectionMode(TextDirectionMode mode) {
  const auto changed = mode != text_direction_mode_;

  text_direction_mode_ = mode;
  apply_text_direction();

  for (auto *action : text_direction_group_->actions()) {
    if (action->data().toInt() == static_cast<int>(mode)) {
      action->setChecked(true);
      break;
    }
  }

  if (changed) emit SignalTextDirectionModeChanged();
}

auto PlainTextEditorPage::GetTextDirectionMode() const -> TextDirectionMode {
  return text_direction_mode_;
}

auto PlainTextEditorPage::TextDirectionMenuAction() const -> QAction * {
  return text_direction_menu_->menuAction();
}

void PlainTextEditorPage::apply_text_direction() {
  ApplyTextDirectionToDocument(ui_->textPage, ui_->textPage->document(),
                               text_direction_mode_);
}

void PlainTextEditorPage::build_text_direction_menu() {
  text_direction_menu_ = new QMenu(tr("Text Direction"), this);
  text_direction_group_ = new QActionGroup(this);
  text_direction_group_->setExclusive(true);

  // Carried as a plain int: QVariant would otherwise hold an unregistered enum
  // type, which toInt() handles badly.
  const std::array<std::pair<TextDirectionMode, QString>, 3> modes{{
      {kTEXT_DIRECTION_AUTO, tr("Automatic")},
      {kTEXT_DIRECTION_LTR, tr("Left-to-Right")},
      {kTEXT_DIRECTION_RTL, tr("Right-to-Left")},
  }};

  for (const auto &[mode, text] : modes) {
    auto *action = text_direction_group_->addAction(text);
    action->setCheckable(true);
    action->setData(static_cast<int>(mode));
    action->setChecked(mode == text_direction_mode_);
    connect(action, &QAction::triggered, this,
            [this, mode = mode]() { SetTextDirectionMode(mode); });
    text_direction_menu_->addAction(action);
  }

  text_direction_menu_->setToolTip(
      tr("Which way the text runs. Automatic gives every line the direction of "
         "its own first letter."));

  // This is what puts the submenu in the editor's context menu.
  ui_->textPage->addAction(text_direction_menu_->menuAction());
}

}  // namespace GpgFrontend::UI
