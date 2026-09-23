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

#include "TextEdit.h"

#include <QtPrintSupport>

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/IOUtils.h"
#include "core/utils/MemoryUtils.h"
#include "ui/lua/LuaPlacements.h"
#include "ui/UIModuleManager.h"
#include "ui/dialog/QuitDialog.h"
#include "ui/function/FilePanelPath.h"
#include "ui/widgets/TextEditTabWidget.h"

namespace GpgFrontend::UI {

TextEdit::TextEdit(QWidget* parent) : QWidget(parent) {
  tab_widget_ = new TextEditTabWidget(this);

  auto* layout = new QVBoxLayout;
  layout->addWidget(tab_widget_);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  setLayout(layout);

  connect(tab_widget_, &QTabWidget::tabCloseRequested, this,
          &TextEdit::slot_remove_tab);

  setAcceptDrops(false);

  SlotNewDefaultWorkspaceTab();
}

auto TextEdit::SlotNewTab() -> QWidget* {
  return tab_widget_->SlotNewPlainTextTab();
}

void TextEdit::SlotNewTabWithContent(QString title, const QString& content) {
  tab_widget_->SlotNewTabWithContent(std::move(title), content);
}

void TextEdit::SlotNewDefaultWorkspaceTab() {
  const auto default_workspace_as =
      GetSettings()
          .value("basic/default_workspace_as", "file_panel")
          .toString();

  if (IsRunningInSandBox()) {
    // In a sandbox the file panel may not work properly because of the
    // sandbox's restrictions, so the text editor is used as the default
    // workspace instead.
    LOG_W() << "running in a sandbox; switching the default workspace to the "
               "text editor";
    tab_widget_->SlotNewPlainTextTab();

  } else if (default_workspace_as == "file_panel") {
    tab_widget_->SlotOpenDefaultPath();
  } else {
    tab_widget_->SlotNewPlainTextTab();
  }

  tab_widget_->SlotRestoreTextEditorsCache();
}

void TextEdit::SlotNewFileBrowserTab() {
  auto const target_path =
      QFileDialog::getOpenFileName(this, tr("Open File"), QDir::home().path());

  if (target_path.isEmpty()) return;
  tab_widget_->SlotOpenPath(target_path);
}

void TextEdit::SlotNewFileBrowserTabWithDirectory() {
  auto const target_path = QFileDialog::getExistingDirectory(
      this, tr("Open File"), QDir::home().path());
  if (target_path.isEmpty()) return;
  tab_widget_->SlotOpenPath(target_path);
}

void TextEdit::SlotOpenFile(const QString& path) {
  // All open-time validation (regular/readable file, size limit, binary
  // detection) and its user-facing messages live in
  // TextEditTabWidget::SlotOpenFile via can_open_as_text_file(), so delegate
  // here instead of duplicating the checks and their translation strings.
  tab_widget_->SlotOpenFile(path);
}

void TextEdit::SlotOpen() {
  QStringList file_names =
      QFileDialog::getOpenFileNames(this, tr("Open File"), QDir::currentPath());
  for (const auto& file_name : file_names) {
    if (!file_name.isEmpty()) {
      SlotOpenFile(file_name);
    }
  }
}

void TextEdit::SlotSave() {
  if (tab_widget_->count() == 0) {
    return;
  }

  auto* page = CurPage();
  if (page == nullptr) return;

  auto type = page->property("type").toString();
  if (type.isEmpty()) {
    QMessageBox::warning(
        this, tr("Unknown Tab Type"),
        tr("The current tab has an unknown type. Cannot save."));
    return;
  }

  LOG_D() << "Saving file for tab type: " << type
          << ", page object name: " << page->objectName();

  // A plain text tab, or a document whose view is a module's native widget:
  // either way the Host saves it, asking the view to prepare the bytes.
  if (type == "text" || (CurPageTextEdit() != nullptr &&
                         CurPageTextEdit()->NativeInstanceId() != 0)) {
    auto filename = CurPageTextEdit()->GetFilePath();
    filename.isEmpty() ? SlotSaveAs() : saveFile(filename);
    return;
  }

  if (type == "file") return;

  QMessageBox::warning(
      this, tr("Unsupported Operation"),
      tr("Saving is not supported for tabs of type '%1'.").arg(type));
}

auto TextEdit::saveFile(const QString& file_name) -> bool {
  if (file_name.isEmpty()) return false;

  PlainTextEditorPage* page = CurPageTextEdit();
  if (page == nullptr) return false;

  // A mounted view owns the document's real content until it hands it back, so
  // it is asked for it before the bytes are read. Without this a message edited
  // in the e-mail view and saved without leaving that view writes what the
  // document held BEFORE the edits -- which is what the SDK contract says this
  // call site guarantees against.
  page->FlushPrimaryView();

  // The view's last word on what is written: it may normalise the bytes, or
  // ask the user whether to go ahead at all. A plain tab is written as is.
  const auto bytes = page->PrimaryViewPrepareSave(page->DocumentBytes());
  if (!bytes.has_value()) return false;  // the user cancelled

  QFile file(file_name);
  // Written as bytes, and without QIODevice::Text: the document knows which
  // line endings it came with and DocumentBytes() has already applied them.
  // Going through a text-mode stream would translate them a second time.
  if (file.open(QIODevice::WriteOnly)) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    file.write(*bytes);
    QApplication::restoreOverrideCursor();
    QTextDocument* document = page->GetTextPage()->document();

    document->setModified(false);

    int cur_index = tab_widget_->currentIndex();
    tab_widget_->setTabText(cur_index, stripped_name(file_name));
    page->SetFilePath(file_name);
    page->NotifyFileSaved();

    Module::TriggerEvent("DOCUMENT_SAVED",
                         {{"file_path", GFBuffer{file_name}},
                          {"tab_index", GFBuffer{QString::number(cur_index)}}});
    Lua::LuaPlacements::Notify("document.saved", page);

    // The document has been saved. Rewrite recovery cache immediately so stale
    // unsaved content will not be restored on next startup.
    tab_widget_->SlotRefreshRecoveryCache();

    file.close();
    return true;
  }

  QMessageBox::warning(
      this, tr("Warning"),
      tr("Cannot read file %1:\n%2.").arg(file_name).arg(file.errorString()));

  return false;
}

auto TextEdit::SlotSaveAs() -> bool {
  if (tab_widget_->count() == 0 || CurPageTextEdit() == nullptr) {
    return true;
  }

  PlainTextEditorPage* page = CurPageTextEdit();
  QString path;
  if (!page->GetFilePath().isEmpty()) {
    path = page->GetFilePath();
  } else {
    // What the content calls itself, asked of the view that knows: a message
    // has a subject, a tab title does not.
    const auto suggested = page->PrimaryViewSuggestedFileName();

    // The tab's own title, from the property that holds it WITHOUT the
    // modified marker. This used to chop two characters off the tab text to
    // drop a "* " that is only there while the tab is modified -- so an
    // unmodified "untitled.eml" was offered as "titled.eml".
    auto title = page->property("base_title").toString().trimmed();
    if (title.isEmpty())
      title = tab_widget_->tabText(tab_widget_->currentIndex());
    while (title.startsWith('*')) title = title.remove(0, 1).trimmed();

    path = suggested.isEmpty() ? title : suggested;

    // Offered in the folder this application saves things in, the same one
    // every other export dialog opens at, rather than wherever the process
    // happens to be.
    if (!path.isEmpty()) {
      const auto dir = GetDefaultUserFilePath();
      if (!dir.isEmpty()) path = QDir(dir).filePath(path);
    }
  }

  auto chosen = QFileDialog::getSaveFileName(this, tr("Save File"), path,
                                             page->PrimaryViewFileTypeFilter());
  // The document type's own suffix, when the user typed a bare name.
  const auto suffix = page->PrimaryViewDefaultSuffix();
  if (!chosen.isEmpty() && !suffix.isEmpty() &&
      QFileInfo(chosen).suffix().isEmpty()) {
    chosen += "." + suffix;
  }
  return saveFile(chosen);
}

void TextEdit::SlotCloseTab() { slot_remove_tab(tab_widget_->currentIndex()); }

void TextEdit::slot_remove_tab(int index) {
  // Do nothing if no tab is open
  if (tab_widget_->count() == 0) {
    return;
  }

  // set the focus to argument index
  tab_widget_->setCurrentIndex(index);

  if (maybe_save_current_tab(true)) {
    auto* tab = tab_widget_->widget(index);
    const auto closed_type =
        tab == nullptr ? QString() : tab->property("type").toString();
    tab_widget_->removeTab(index);

    // After the removal, because the fact being reported is that the tab is
    // gone. A module told beforehand would still see it in the widget and
    // could act on something about to be destroyed.
    Module::TriggerEvent("TAB_CLOSED",
                         {{"tab_index", GFBuffer{QString::number(index)}},
                          {"tab_type", GFBuffer{closed_type}}});

    if (tab_widget_->count() > 0) {
      const int new_index = std::min(index, tab_widget_->count() - 1);
      tab_widget_->setCurrentIndex(new_index);
    }

    // The user intentionally closed this tab. Rewrite recovery cache after
    // removeTab(), so the closed tab will not be recovered again.
    tab_widget_->SlotRefreshRecoveryCache();

    if (tab != nullptr) {
      // Wiped through the page rather than left to close(): by now the widget
      // is removed, hidden and unparented, and hanging the erasure of a
      // decrypted message on a QCloseEvent still reaching it is not a bet
      // worth taking.
      if (auto* page = qobject_cast<PlainTextEditorPage*>(tab)) {
        page->WipeContent();
      }

      tab->close();
      tab->deleteLater();
    }
  }
}

/**
 * Check whether the current tab needs saving.
 * Call this before closing the active tab.
 *
 * If it returns false, the close event should be aborted.
 */
auto TextEdit::maybe_save_current_tab(bool ask_to_save) -> bool {
  PlainTextEditorPage* page = CurPageTextEdit();
  if (page == nullptr) return true;

  QTextDocument* document = page->GetTextPage()->document();
  if (document == nullptr) return true;

  if (document->isModified()) {
    QMessageBox::StandardButton result = QMessageBox::Cancel;

    QString doc_name = tab_widget_->tabText(tab_widget_->currentIndex());
    doc_name = doc_name.trimmed();
    while (doc_name.startsWith("*")) {
      doc_name.remove(0, 1);
      doc_name = doc_name.trimmed();
    }

    const QString& file_path = page->GetFilePath();
    if (ask_to_save) {
      result = QMessageBox::warning(
          this, tr("Unsaved Document"),
          tr("The document \"%1\" has been modified. Do you want to "
             "save your changes?")
                  .arg(doc_name) +
              "<br/><b>" + tr("Note:") + "</b> " +
              tr("If you don't save, your changes will be lost.") + "<br/>",
          QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    }

    if ((result == QMessageBox::Save) || (!ask_to_save)) {
      if (file_path.isEmpty()) return SlotSaveAs();
      return saveFile(file_path);
    }

    return result == QMessageBox::Discard;
  }

  return true;
}

auto TextEdit::MaybeSaveAnyTab() -> bool {
  // get a list of all unsaved documents and their tab ids
  auto const unsaved_docs = this->UnsavedDocuments();

  // no unsaved documents, so app can be closed
  if (unsaved_docs.empty()) return true;

  bool restore_text_editor_page =
      GetSettings().value("basic/restore_text_editor_page", true).toBool();
  if (restore_text_editor_page) {
    FLOG_D("restore_text_editor_page is true, caching messages and exiting");
    tab_widget_->SlotCacheTextEditors();
    return true;
  }

  // only 1 unsaved document -> set modified tab as current and show normal
  // unsaved doc dialog

  if (unsaved_docs.size() == 1) {
    int const modified_tab = unsaved_docs.keys().at(0);
    tab_widget_->setCurrentIndex(modified_tab);

    auto maybe_save = maybe_save_current_tab(true);
    return maybe_save;
  }

  // more than one unsaved document

  bool can_close = false;
  auto* dialog = new QuitDialog(
      this->parentWidget() != nullptr ? this->parentWidget() : this,
      unsaved_docs);

  connect(dialog, &QuitDialog::SignalDiscard, this,
          [&]() { can_close = true; });

  connect(dialog, &QuitDialog::SignalSave, this,
          [&](const QContainer<int>& ids) {
            bool all_saved = true;
            for (const auto& tab_id : ids) {
              tab_widget_->setCurrentIndex(tab_id);
              if (!maybe_save_current_tab(false)) all_saved = false;
            }
            can_close = all_saved;
          });

  dialog->exec();
  dialog->deleteLater();
  return can_close;
}

namespace {

/**
 * @brief Hand an operation's result bytes to @p page, leaving no copy behind.
 *
 * Bytes rather than text, and through the page rather than into its editor:
 * the page is what records which line endings the document has, and a write
 * that goes straight to the QPlainTextEdit throws that away -- which is how a
 * message signed over CRLF came back out as bare LF and stopped verifying.
 *
 * ConvertToQByteArray() hands back an ordinary QByteArray, which cannot be
 * wiped once it has been shared. Wiping the intermediate removes one of the
 * two copies; the document keeps its own, and only clearing the document
 * releases that.
 */
void SetOperationResultFromBuffer(PlainTextEditorPage* page,
                                  const GFBuffer& buffer) {
  auto bytes = buffer.ConvertToQByteArray();
  page->SetOperationResultBytes(bytes);
  WipeByteArray(bytes);
}

}  // namespace

void TextEdit::WipeAllTabs() {
  for (int i = 0; i < tab_widget_->count(); ++i) {
    if (auto* page =
            qobject_cast<PlainTextEditorPage*>(tab_widget_->widget(i))) {
      page->WipeContent();
    }
  }
}

void TextEdit::SlotSetGFBuffer2CurTextPage(const GFBuffer& buffer) {
  if (CurTextPage() == nullptr) SlotNewTab();
  SetOperationResultFromBuffer(CurTextPage(), buffer);
}

auto TextEdit::ClassifyResultTarget(bool page_alive, int tab_index,
                                    bool is_text_page) -> ResultTarget {
  if (!page_alive) return ResultTarget::kPageDestroyed;
  // Still alive is not the same as still ours: a page removed from the tab
  // widget but not yet deleted must not be written to either.
  if (tab_index < 0) return ResultTarget::kPageDetached;
  if (!is_text_page) return ResultTarget::kNotATextPage;
  return ResultTarget::kDeliver;
}

auto TextEdit::SetGFBuffer2Page(const QPointer<QWidget>& page,
                                const GFBuffer& buffer) -> bool {
  // QPointer, so a destroyed page reads as null rather than as a stale
  // address. That is the whole reason the caller hands one over.
  auto* text_page = qobject_cast<PlainTextEditorPage*>(page.data());
  const auto target = ClassifyResultTarget(
      !page.isNull(), page.isNull() ? -1 : tab_widget_->indexOf(page),
      text_page != nullptr && text_page->GetTextPage() != nullptr);

  if (target != ResultTarget::kDeliver) {
    LOG_W() << "discarding an operation result; target state: "
            << static_cast<int>(target);
    return false;
  }

  SetOperationResultFromBuffer(text_page, buffer);
  return true;
}

auto TextEdit::ApplyVerificationToPage(const QPointer<QWidget>& page,
                                       const QByteArray& payload) -> bool {
  auto* text_page = qobject_cast<PlainTextEditorPage*>(page.data());
  const auto target = ClassifyResultTarget(
      !page.isNull(), page.isNull() ? -1 : tab_widget_->indexOf(page),
      text_page != nullptr && text_page->GetTextPage() != nullptr);

  if (target != ResultTarget::kDeliver) {
    LOG_W() << "discarding a verification result; target state: "
            << static_cast<int>(target);
    return false;
  }

  return text_page->ApplyVerificationToPrimaryView(payload);
}

void TextEdit::SlotAppendText2CurTextPage(const QString& text) {
  SlotAppendText2CurTextPage(text, false);
}

void TextEdit::SlotAppendText2CurTextPageAndReveal(const QString& text) {
  SlotAppendText2CurTextPage(text, true);
}

void TextEdit::SlotAppendText2CurTextPage(const QString& text, bool reveal) {
  if (text.isEmpty()) return;

  if (CurTextPage() == nullptr) {
    SlotNewTab();
  }

  auto* page = CurTextPage();
  if (page == nullptr) return;

  // A page with a mounted view gets asked first. "Append this" means "put it
  // in what I am writing", and for a structured document that is somewhere
  // inside the message -- never a paste into the middle of its raw bytes.
  // Only a view that declares no opinion falls through to the document.
  if (page->AppendTextToPrimaryView(text) != 0) return;

  auto* edit = page->GetTextPage();
  if (edit == nullptr) return;

  auto* scroll_bar = edit->verticalScrollBar();
  const int old_scroll_value = scroll_bar != nullptr ? scroll_bar->value() : 0;
  const QTextCursor old_cursor = edit->textCursor();

  QTextCursor append_cursor(edit->document());
  append_cursor.movePosition(QTextCursor::End);

  append_cursor.beginEditBlock();

  const auto current_text = edit->toPlainText();
  if (!current_text.isEmpty() && !current_text.endsWith('\n') &&
      !text.startsWith('\n')) {
    append_cursor.insertText(QStringLiteral("\n"));
  }

  append_cursor.insertText(text);

  if (!text.endsWith('\n')) {
    append_cursor.insertText(QStringLiteral("\n"));
  }

  append_cursor.endEditBlock();

  if (reveal) {
    edit->setTextCursor(append_cursor);
    edit->ensureCursorVisible();
  } else {
    edit->setTextCursor(old_cursor);
    if (scroll_bar != nullptr) {
      scroll_bar->setValue(old_scroll_value);
      QTimer::singleShot(0, edit, [edit, old_scroll_value]() {
        if (edit == nullptr || edit->verticalScrollBar() == nullptr) return;
        edit->verticalScrollBar()->setValue(old_scroll_value);
      });
    }
  }

  edit->document()->setModified(true);
}

auto TextEdit::CurTextPage() const -> PlainTextEditorPage* {
  return tab_widget_->CurTextPage();
}

auto TextEdit::CurPageTextEdit() const -> PlainTextEditorPage* {
  return tab_widget_->CurPageTextEdit();
}

auto TextEdit::CurPageIsPlainText() const -> bool {
  return tab_widget_->CurPageIsPlainText();
}

auto TextEdit::CurPageCryptoOperations(bool& has_opinion) const -> QStringList {
  return tab_widget_->CurPageCryptoOperations(has_opinion);
}

auto TextEdit::AttachPublicKeyToCurPage(const QByteArray& key,
                                        const QString& name) -> int {
  auto* page = CurTextPage();
  if (page == nullptr) return 0;
  return page->AttachPublicKeyToPrimaryView(key, name);
}
auto TextEdit::CurFilePage() const -> FilePage* {
  return tab_widget_->CurFilePage();
}

auto TextEdit::TabCount() const -> int { return tab_widget_->count(); }

auto TextEdit::CurPageFileTreeView() const -> FilePage* {
  return tab_widget_->CurFilePage();
}

void TextEdit::SlotQuote() const {
  if (tab_widget_->count() == 0 || CurTextPage() == nullptr) {
    return;
  }

  QTextCursor cursor(CurTextPage()->GetTextPage()->document());

  // beginEditBlock() and endEditBlock() make the operation a single undo/redo
  // step
  cursor.beginEditBlock();
  cursor.setPosition(0);
  cursor.insertText("> ");
  while (!cursor.isNull() && !cursor.atEnd()) {
    cursor.movePosition(QTextCursor::EndOfLine);
    cursor.movePosition(QTextCursor::NextCharacter);
    if (!cursor.atEnd()) {
      cursor.insertText("> ");
    }
  }
  cursor.endEditBlock();
}

void TextEdit::SlotFillTextEditWithText(const QString& text) const {
  auto* edit = this->CurTextPage()->GetTextPage();
  edit->setUndoRedoEnabled(false);
  edit->setPlainText(text);
  edit->setUndoRedoEnabled(true);
  edit->document()->setModified(true);
}

void TextEdit::SlotFillTextEditWithText(const GFBuffer& buffer) const {
  auto* page = this->CurTextPage();
  auto* edit = page->GetTextPage();
  edit->setUndoRedoEnabled(false);
  SetOperationResultFromBuffer(page, buffer);
  edit->setUndoRedoEnabled(true);
}

void TextEdit::LoadFile(const QString& fileName) {
  auto [succ, buffer] = ReadFileGFBuffer(fileName);
  if (!succ) {
    QMessageBox::warning(this, tr("File Open Error"),
                         tr("The file \"%1\" could not be opened.")
                             .arg(QFileInfo(fileName).fileName()));
    return;
  }

  if (CurTextPage() == nullptr) {
    SlotNewTab();
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);
  // Through the page, not its editor: a file opened here keeps the line
  // endings it has on disk, which is what lets a signed message it carries
  // still verify. This one has a file behind it, so the document comes out
  // clean rather than modified.
  {
    auto bytes = buffer.ConvertToQByteArray();
    CurTextPage()->SetContentFromBytes(bytes);
    WipeByteArray(bytes);
  }
  QApplication::restoreOverrideCursor();

  CurPageTextEdit()->SetFilePath(fileName);
  tab_widget_->setTabText(tab_widget_->currentIndex(), stripped_name(fileName));
}

auto TextEdit::stripped_name(const QString& full_file_name) -> QString {
  return QFileInfo(full_file_name).fileName();
}

void TextEdit::SlotPrint() {
  if (tab_widget_->count() == 0) {
    return;
  }

#ifndef QT_NO_PRINTER
  QTextDocument* document = nullptr;
  if (CurTextPage() != nullptr) {
    document = CurTextPage()->GetTextPage()->document();
  }
  QPrinter printer;

  auto* dlg = new QPrintDialog(&printer, this);
  if (dlg->exec() != QDialog::Accepted) {
    return;
  }
  if (document != nullptr) {
    document->print(&printer);
  } else {
    QMessageBox::warning(this, tr("Warning"), tr("No document to print."));
  }

  // statusBar()->showMessage(tr("Ready"), 2000);
#endif
}

void TextEdit::SlotSwitchTabUp() const {
  if (tab_widget_->count() > 1) {
    int new_index = (tab_widget_->currentIndex() + 1) % (tab_widget_->count());
    tab_widget_->setCurrentIndex(new_index);
  }
}

void TextEdit::SlotSwitchTabDown() const {
  if (tab_widget_->count() > 1) {
    int newindex = (tab_widget_->currentIndex() - 1 + tab_widget_->count()) %
                   tab_widget_->count();
    tab_widget_->setCurrentIndex(newindex);
  }
}

/*
 *   return a hash of the tab indexes and titles of unsaved tabs
 */
auto TextEdit::UnsavedDocuments() const -> QHash<int, QString> {
  QHash<int, QString> unsaved_docs;

  for (int i = 0; i < tab_widget_->count(); i++) {
    auto* ep = qobject_cast<PlainTextEditorPage*>(tab_widget_->widget(i));
    if (ep == nullptr || ep->GetTextPage() == nullptr ||
        ep->GetTextPage()->document() == nullptr ||
        !ep->GetTextPage()->document()->isModified()) {
      continue;
    }

    QString doc_name = tab_widget_->tabText(i).trimmed();
    while (doc_name.startsWith("*")) {
      doc_name.remove(0, 1);
      doc_name = doc_name.trimmed();
    }

    unsaved_docs.insert(i, doc_name);
  }

  return unsaved_docs;
}

void TextEdit::SlotCut() const {
  if (tab_widget_->count() == 0 || CurTextPage() == nullptr) {
    return;
  }

  CurTextPage()->GetTextPage()->cut();
}

void TextEdit::SlotCopy() const {
  if (tab_widget_->count() == 0) {
    return;
  }

  if (CurTextPage() != nullptr) {
    CurTextPage()->GetTextPage()->copy();
  }
}

void TextEdit::SlotPaste() const {
  if (tab_widget_->count() == 0 || CurTextPage() == nullptr) {
    return;
  }

  CurTextPage()->GetTextPage()->paste();
}

void TextEdit::SlotUndo() const {
  if (tab_widget_->count() == 0 || CurTextPage() == nullptr) {
    return;
  }

  CurTextPage()->GetTextPage()->undo();
}

void TextEdit::SlotRedo() const {
  if (tab_widget_->count() == 0 || CurTextPage() == nullptr) {
    return;
  }

  CurTextPage()->GetTextPage()->redo();
}

void TextEdit::SlotZoomIn() const {
  if (tab_widget_->count() == 0) {
    return;
  }

  if (CurTextPage() != nullptr) {
    CurTextPage()->GetTextPage()->zoomIn();
  }
}

void TextEdit::SlotZoomOut() const {
  if (tab_widget_->count() == 0) {
    return;
  }

  if (CurTextPage() != nullptr) {
    CurTextPage()->GetTextPage()->zoomOut();
  }
}

void TextEdit::SlotSelectAll() const {
  if (tab_widget_->count() == 0 || CurTextPage() == nullptr) {
    return;
  }
  CurTextPage()->GetTextPage()->selectAll();
}

auto TextEdit::CurPlainText() const -> QString {
  auto* plain_text_tab = CurTextPage();
  if (plain_text_tab == nullptr) return {};
  return plain_text_tab->GetPlainText();
}

auto TextEdit::CurPlainTextForOperation() const -> QString {
  auto* plain_text_tab = CurTextPage();
  if (plain_text_tab == nullptr) return {};
  plain_text_tab->FlushPrimaryView();
  return plain_text_tab->GetPlainText();
}

auto TextEdit::CurDocumentBytesForOperation() const -> QByteArray {
  auto* plain_text_tab = CurTextPage();
  if (plain_text_tab == nullptr) return {};
  plain_text_tab->FlushPrimaryView();

  // DocumentBytes(), not the text: it puts back the line endings the document
  // actually has, which for anything carrying a signature is the difference
  // between the bytes that were signed and a rewrite of them.
  return plain_text_tab->DocumentBytes();
}

auto TextEdit::TabWidget() const -> TextEditTabWidget* { return tab_widget_; }

void TextEdit::SlotOpenDefaultFileBrowserTab() {
  tab_widget_->SlotOpenDefaultPath();
}

auto TextEdit::CurPage() -> QWidget* { return tab_widget_->CurPage(); }

auto TextEdit::SlotNewCustomTab(const QString& type, const QString& title,
                                const QIcon& icon, const QString& icon_name)
    -> QWidget* {
  return tab_widget_->SlotNewTab(type, title, icon, icon_name);
}

auto TextEdit::SlotGetTabWidget() -> QTabWidget* { return tab_widget_; }
auto TextEdit::OpenDocument(const QString& type, const QString& title,
                            const QString& path, const GFBuffer& content,
                            bool saved, bool modified) -> qint64 {
  auto* page = qobject_cast<PlainTextEditorPage*>(
      tab_widget_->SlotNewTab(type.isEmpty() ? QStringLiteral("text") : type,
                              title, QIcon(), {}));
  if (page == nullptr) return 0;

  page->SetContentFromBytes(content.ConvertToQByteArray());
  if (!path.isEmpty()) page->SetFilePath(path);
  // Loading content never marks a document modified -- the same as a tab a
  // module filled by hand used to be -- so `saved` only adds the file's
  // saved state on top.
  if (saved) page->NotifyFileSaved();
  // A draft -- a reply, say -- exists nowhere yet, so closing it asks first.
  if (modified) page->GetTextPage()->document()->setModified(true);
  return TextEditTabWidget::DocumentIdOf(page);
}

}  // namespace GpgFrontend::UI
