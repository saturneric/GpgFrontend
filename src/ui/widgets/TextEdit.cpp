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
#include <cstddef>

#include "core/function/CacheManager.h"
#include "core/function/GlobalSettingStation.h"
#include "core/utils/IOUtils.h"
#include "ui/dialog/QuitDialog.h"
#include "ui/widgets/HelpPage.h"
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

void TextEdit::SlotNewTab() { tab_widget_->SlotNewTab(); }

void TextEdit::SlotNewTabWithContent(QString title, const QString& content) {
  tab_widget_->SlotNewTabWithContent(std::move(title), content);
}

void TextEdit::SlotNewHelpTab(const QString& title, const QString& path) const {
  auto* page = new HelpPage(path);
  tab_widget_->addTab(page, title);
  tab_widget_->setCurrentIndex(tab_widget_->count() - 1);
}

void TextEdit::SlotNewDefaultWorkspaceTab() {
  const auto default_workspace_as =
      GetSettings()
          .value("basic/default_workspace_as", "file_panel")
          .toString();

  if (default_workspace_as == "file_panel") {
    tab_widget_->SlotOpenDefaultPath();
  } else if (default_workspace_as == "email_editor") {
    tab_widget_->SlotNewEMailTab();
  } else {
    tab_widget_->SlotNewTab();
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
  QFileInfo const info(path);
  if (!info.isFile() || !info.isReadable()) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot open this file. Please make sure that this "
           "is a regular file and it's readable."));
    return;
  }

  if (info.size() > static_cast<qint64>(1024 * 1024)) {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Cannot open this file. The file is TOO LARGE (>1MB) for "
           "GpgFrontend Text Editor."));
    return;
  }

  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(
        this, tr("File Open Error"),
        tr("The file \"%1\" could not be opened.").arg(info.fileName()));
    return;
  }

  QByteArray file_data = file.read(1024);
  file.close();

  if (file_data.contains('\0')) {
    QMessageBox::warning(this, tr("Binary File Detected"),
                         tr("The file \"%1\" appears to be a binary file "
                            "and will not be opened.")
                             .arg(info.fileName()));
    return;
  }
  tab_widget_->SlotOpenFile(path);
}

void TextEdit::SlotOpen() {
  QStringList file_names =
      QFileDialog::getOpenFileNames(this, tr("Open file"), QDir::currentPath());
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

  if (CurEMailPage() != nullptr) {
    QString file_name = CurEMailPage()->GetFilePath();

    if (file_name.isEmpty()) {
      SlotSaveAsEML();
    } else {
      saveEMLFile(file_name);
    }
    return;
  }

  if (CurTextPage() != nullptr) {
    QString file_name = CurPageTextEdit()->GetFilePath();

    if (file_name.isEmpty()) {
      // QString docname = tabWidget->tabText(tabWidget->currentIndex());
      // docname.remove(0,2);
      SlotSaveAs();
    } else {
      saveFile(file_name);
    }
  }
}

auto TextEdit::saveFile(const QString& file_name) -> bool {
  if (file_name.isEmpty()) return false;

  PlainTextEditorPage* page = CurPageTextEdit();
  if (page == nullptr) return false;

  QFile file(file_name);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream output_stream(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    output_stream << page->GetTextPage()->toPlainText();
    QApplication::restoreOverrideCursor();
    QTextDocument* document = page->GetTextPage()->document();

    document->setModified(false);

    int cur_index = tab_widget_->currentIndex();
    tab_widget_->setTabText(cur_index, stripped_name(file_name));
    page->SetFilePath(file_name);
    page->NotifyFileSaved();

    file.close();
    return true;
  }
  QMessageBox::warning(
      this, tr("Warning"),
      tr("Cannot read file %1:\n%2.").arg(file_name).arg(file.errorString()));
  return false;
}

auto TextEdit::saveEMLFile(const QString& file_name) -> bool {
  if (file_name.isEmpty()) return false;

  PlainTextEditorPage* page = CurEMailPage();
  if (page == nullptr) return false;

  QFile file(file_name);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(
        this, tr("Warning"),
        tr("Cannot read file %1:\n%2.").arg(file_name).arg(file.errorString()));
    return false;
  }

  QTextStream output_stream(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  output_stream
      << page->GetTextPage()->toPlainText().replace("\n", "\r\n").toLatin1();
  QApplication::restoreOverrideCursor();
  QTextDocument* document = page->GetTextPage()->document();

  document->setModified(false);

  int cur_index = tab_widget_->currentIndex();
  tab_widget_->setTabText(cur_index, stripped_name(file_name));
  page->SetFilePath(file_name);
  page->NotifyFileSaved();

  file.close();
  return true;
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
    path = tab_widget_->tabText(tab_widget_->currentIndex()).remove(0, 2);
  }

  return saveFile(QFileDialog::getSaveFileName(this, tr("Save file"), path));
}

auto TextEdit::SlotSaveAsEML() -> bool {
  if (tab_widget_->count() == 0 || CurEMailPage() == nullptr) {
    return true;
  }

  PlainTextEditorPage* page = CurEMailPage();
  QString path;
  if (!page->GetFilePath().isEmpty()) {
    path = page->GetFilePath();
  } else {
    path = tab_widget_->tabText(tab_widget_->currentIndex()).remove(0, 2);
  }

  return saveEMLFile(QFileDialog::getSaveFileName(this, tr("Save file"), path));
}

void TextEdit::SlotCloseTab() { slot_remove_tab(tab_widget_->currentIndex()); }

void TextEdit::slot_remove_tab(int index) {
  // Do nothing, if no tab is opened
  if (tab_widget_->count() == 0) {
    return;
  }

  // get the index of the actual current tab
  int last_index = tab_widget_->currentIndex();

  // set the focus to argument index
  tab_widget_->setCurrentIndex(index);

  if (maybe_save_current_tab(true)) {
    auto* tab = tab_widget_->widget(index);
    tab_widget_->removeTab(index);

    // if the tab was the last one, set the current index to the last tab
    tab_widget_->setCurrentIndex(index >= last_index ? last_index
                                                     : last_index - 1);

    // close and destroy tab
    if (tab != nullptr) {
      tab->close();
      tab->deleteLater();
    }
  }
}

/**
 * Check if current may need to be saved.
 * Call this function before closing the currently active tab-
 *
 * If it returns false, the close event should be aborted.
 */
auto TextEdit::maybe_save_current_tab(bool ask_to_save) -> bool {
  PlainTextEditorPage* page = CurPageTextEdit();
  // if this page is no textedit, there should be nothing to save
  if (page == nullptr) return true;

  QTextDocument* document = page->GetTextPage()->document();

  if (page->ReadDone() && document->isModified()) {
    QMessageBox::StandardButton result = QMessageBox::Cancel;

    // write title of tab to docname and remove the leading *
    QString doc_name = tab_widget_->tabText(tab_widget_->currentIndex());
    doc_name.remove(0, 2);

    const QString& file_path = page->GetFilePath();
    if (ask_to_save) {
      result = QMessageBox::warning(
          this, tr("Unsaved document"),
          tr("The document \"%1\" has been modified. Do you want to "
             "save your changes?")
                  .arg(doc_name) +
              "<br/><b>" + tr("Note:") + "</b>" +
              tr("If you don't save these files, all changes are "
                 "lost.") +
              "<br/>",
          QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    }
    if ((result == QMessageBox::Save) || (!ask_to_save)) {
      if (file_path.isEmpty()) return SlotSaveAs();
      return saveFile(file_path);
    }
    return result == QMessageBox::Discard;
  }
  page->deleteLater();
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
    FLOG_D("restore_text_editor_page is true, caching messages and exit...");
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

  // more than one unsaved documents

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

void TextEdit::SlotSetGFBuffer2CurEMailPage(const GFBuffer& buffer) {
  if (CurTextPage() == nullptr) SlotNewEMailTab();
  auto* edit = CurTextPage()->GetTextPage();
  edit->setPlainText(buffer.ConvertToQString());
}

void TextEdit::SlotAppendText2CurTextPage(const QString& text) {
  if (CurTextPage() == nullptr) SlotNewTab();
  CurTextPage()->GetTextPage()->appendPlainText(text);
}

auto TextEdit::CurTextPage() const -> PlainTextEditorPage* {
  return tab_widget_->CurTextPage();
}

auto TextEdit::CurPageTextEdit() const -> PlainTextEditorPage* {
  return tab_widget_->CurPageTextEdit();
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

  // beginEditBlock and endEditBlock() let operation look like single
  // undo/redo operation
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
  auto* edit = this->CurTextPage()->GetTextPage();
  edit->setUndoRedoEnabled(false);
  edit->setPlainText(buffer.ConvertToQString());
  edit->setUndoRedoEnabled(true);
  edit->document()->setModified(true);
}

void TextEdit::LoadFile(const QString& fileName) {
  auto [succ, buffer] = ReadFileGFBuffer(fileName);

  QApplication::setOverrideCursor(Qt::WaitCursor);
  CurTextPage()->GetTextPage()->setPlainText(buffer.ConvertToQString());
  QApplication::restoreOverrideCursor();
  CurPageTextEdit()->SetFilePath(fileName);
  tab_widget_->setTabText(tab_widget_->currentIndex(), stripped_name(fileName));
  // statusBar()->showMessage(tr("File loaded"), 2000);
}

auto TextEdit::stripped_name(const QString& full_file_name) -> QString {
  return QFileInfo(full_file_name).fileName();
}

void TextEdit::SlotPrint() {
  if (tab_widget_->count() == 0) {
    return;
  }

#ifndef QT_NO_PRINTER
  QTextDocument* document;
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
    QMessageBox::warning(this, tr("Warning"), tr("No document to print"));
  }

  // statusBar()->showMessage(tr("Ready"), 2000);
#endif
}

void TextEdit::SlotShowModified(bool changed) const {
  // get current tab
  int index = tab_widget_->currentIndex();
  QString title = tab_widget_->tabText(index);

  // if changed
  if (!changed) {
    tab_widget_->setTabText(index, title.remove(0, 2));
    return;
  }

  // if doc is modified now, add leading * to title,
  // otherwise remove the leading * from the title
  if (CurTextPage()->GetTextPage()->document()->isModified()) {
    tab_widget_->setTabText(index, title.trimmed().prepend("* "));
  } else {
    tab_widget_->setTabText(index, title.remove(0, 2));
  }
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
 *   return a hash of tabindexes and title of unsaved tabs
 */
auto TextEdit::UnsavedDocuments() const -> QHash<int, QString> {
  QHash<int, QString> unsaved_docs;  // this list could be used to implement
                                     // gedit like "unsaved changed"-dialog

  for (int i = 0; i < tab_widget_->count(); i++) {
    auto* ep = qobject_cast<PlainTextEditorPage*>(tab_widget_->widget(i));
    if (ep != nullptr && ep->ReadDone() &&
        ep->GetTextPage()->document()->isModified()) {
      QString doc_name = tab_widget_->tabText(i);

      // remove * before name of modified doc
      doc_name.remove(0, 2);
      unsaved_docs.insert(i, doc_name);
    }
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

auto TextEdit::TabWidget() const -> QTabWidget* { return tab_widget_; }

auto TextEdit::CurEMailPage() const -> EMailEditorPage* {
  return tab_widget_->CurEMailPage();
}

void TextEdit::SlotNewEMailTab() { tab_widget_->SlotNewEMailTab(); }

void TextEdit::SlotOpenDefaultFileBrowserTab() {
  tab_widget_->SlotOpenDefaultPath();
}
}  // namespace GpgFrontend::UI
