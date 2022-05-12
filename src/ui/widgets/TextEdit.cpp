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
 * All the source code of GpgFrontend was modified and released by
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "ui/widgets/TextEdit.h"

#include <boost/format.hpp>

namespace GpgFrontend::UI {

TextEdit::TextEdit(QWidget* parent) : QWidget(parent) {
  count_page_ = 0;
  tab_widget_ = new QTabWidget(this);
  tab_widget_->setMovable(true);
  tab_widget_->setTabsClosable(true);
  tab_widget_->setDocumentMode(true);

  auto* layout = new QVBoxLayout;
  layout->addWidget(tab_widget_);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  setLayout(layout);

  connect(tab_widget_, &QTabWidget::tabCloseRequested, this,
          &TextEdit::slot_remove_tab);
  SlotNewTab();
  setAcceptDrops(false);
}

void TextEdit::SlotNewTab() {
  QString header = _("untitled") + QString::number(++count_page_) + ".txt";

  auto* page = new PlainTextEditorPage();
  auto index = tab_widget_->addTab(page, header);
  tab_widget_->setTabIcon(index, QIcon(":file.png"));
  tab_widget_->setCurrentIndex(tab_widget_->count() - 1);
  page->GetTextPage()->setFocus();
  connect(page->GetTextPage()->document(), &QTextDocument::modificationChanged,
          this, &TextEdit::SlotShowModified);
}

void TextEdit::slotNewHelpTab(const QString& title, const QString& path) const {
  auto* page = new HelpPage(path);
  tab_widget_->addTab(page, title);
  tab_widget_->setCurrentIndex(tab_widget_->count() - 1);
}

void TextEdit::SlotNewFileTab() const {
  auto* page = new FilePage(qobject_cast<QWidget*>(parent()));
  auto index = tab_widget_->addTab(page, QString());
  tab_widget_->setTabIcon(index, QIcon(":file-browser.png"));
  tab_widget_->setCurrentIndex(tab_widget_->count() - 1);
  connect(page, &FilePage::SignalPathChanged, this,
          &TextEdit::slot_file_page_path_changed);
  page->SlotGoPath();
}

void TextEdit::SlotOpenFile(QString& path) {
  QFile file(path);
  LOG(INFO) << "path" << path.toStdString();
  auto result = file.open(QIODevice::ReadOnly | QIODevice::Text);
  if (result) {
    auto* page = new PlainTextEditorPage(path);
    connect(page->GetTextPage()->document(),
            &QTextDocument::modificationChanged, this,
            &TextEdit::SlotShowModified);

    QApplication::setOverrideCursor(Qt::WaitCursor);
    auto index = tab_widget_->addTab(page, stripped_name(path));
    tab_widget_->setTabIcon(index, QIcon(":file.png"));
    tab_widget_->setCurrentIndex(tab_widget_->count() - 1);
    QApplication::restoreOverrideCursor();
    page->GetTextPage()->setFocus();
    page->ReadFile();
  } else {
    QMessageBox::warning(this, _("Warning"),
                         (boost::format(_("Cannot read file %1%:\n%2%.")) %
                          path.toStdString() % file.errorString().toStdString())
                             .str()
                             .c_str());
  }

  file.close();
}

void TextEdit::SlotOpen() {
  QStringList file_names =
      QFileDialog::getOpenFileNames(this, _("Open file"), QDir::currentPath());
  for (const auto& file_name : file_names) {
    if (!file_name.isEmpty()) {
      QFile file(file_name);

      if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto* page = new PlainTextEditorPage(file_name);

        QTextStream in(&file);
        QApplication::setOverrideCursor(Qt::WaitCursor);
        page->GetTextPage()->setPlainText(in.readAll());
        page->SetFilePath(file_name);
        QTextDocument* document = page->GetTextPage()->document();
        document->setModified(false);

        tab_widget_->addTab(page, stripped_name(file_name));
        tab_widget_->setCurrentIndex(tab_widget_->count() - 1);
        QApplication::restoreOverrideCursor();
        page->GetTextPage()->setFocus();
        connect(page->GetTextPage()->document(),
                &QTextDocument::modificationChanged, this,
                &TextEdit::SlotShowModified);
        // enableAction(true)
        file.close();
      } else {
        QMessageBox::warning(
            this, _("Warning"),
            (boost::format(_("Cannot read file %1%:\n%2%.")) %
             file_name.toStdString() % file.errorString().toStdString())
                .str()
                .c_str());
      }
    }
  }
}

void TextEdit::SlotSave() {
  if (tab_widget_->count() == 0 || SlotCurPageTextEdit() == 0) {
    return;
  }

  QString fileName = SlotCurPageTextEdit()->GetFilePath();

  if (fileName.isEmpty()) {
    // QString docname = tabWidget->tabText(tabWidget->currentIndex());
    // docname.remove(0,2);
    SlotSaveAs();
  } else {
    save_file(fileName);
  }
}

bool TextEdit::save_file(const QString& fileName) {
  if (fileName.isEmpty()) {
    return false;
  }

  PlainTextEditorPage* page = SlotCurPageTextEdit();
  if (page == nullptr) return false;

  if (page->WillCharsetChange()) {
    auto result = QMessageBox::warning(
        this, _("Save"),
        QString("<p>") +
            _("After saving, the encoding of the current file will be "
              "converted to UTF-8 and the line endings will be changed to "
              "LF. ") +
            "</p>" + "<p>" +
            _("If this is not the result you expect, please use \"save "
              "as\".") +
            "</p>",
        QMessageBox::Save | QMessageBox::Cancel, QMessageBox::Cancel);

    if (result == QMessageBox::Cancel) {
      return false;
    }
  }

  QFile file(fileName);

  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream outputStream(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    outputStream << page->GetTextPage()->toPlainText();
    QApplication::restoreOverrideCursor();
    QTextDocument* document = page->GetTextPage()->document();

    document->setModified(false);

    int curIndex = tab_widget_->currentIndex();
    tab_widget_->setTabText(curIndex, stripped_name(fileName));
    page->SetFilePath(fileName);
    page->NotifyFileSaved();

    file.close();
    return true;
  } else {
    QMessageBox::warning(
        this, _("Warning"),
        (boost::format(_("Cannot read file %1%:\n%2%.")) %
         fileName.toStdString() % file.errorString().toStdString())
            .str()
            .c_str());
    return false;
  }
}

bool TextEdit::SlotSaveAs() {
  if (tab_widget_->count() == 0 || SlotCurPageTextEdit() == nullptr) {
    return true;
  }

  PlainTextEditorPage* page = SlotCurPageTextEdit();
  QString path;
  if (!page->GetFilePath().isEmpty()) {
    path = page->GetFilePath();
  } else {
    path = tab_widget_->tabText(tab_widget_->currentIndex()).remove(0, 2);
  }

  QString fileName = QFileDialog::getSaveFileName(this, _("Save file"), path);
  return save_file(fileName);
}

void TextEdit::SlotCloseTab() {
  slot_remove_tab(tab_widget_->currentIndex());
  if (tab_widget_->count() != 0) {
    SlotCurPageTextEdit()->GetTextPage()->setFocus();
  }
}

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
    tab_widget_->removeTab(index);

    if (index >= last_index) {
      tab_widget_->setCurrentIndex(last_index);
    } else {
      tab_widget_->setCurrentIndex(last_index - 1);
    }
  }

  if (tab_widget_->count() == 0) {
    //  enableAction(false);
  }
}

/**
 * Check if current may need to be saved.
 * Call this function before closing the currently active tab-
 *
 * If it returns false, the close event should be aborted.
 */
bool TextEdit::maybe_save_current_tab(bool askToSave) {
  PlainTextEditorPage* page = SlotCurPageTextEdit();
  // if this page is no textedit, there should be nothing to save
  if (page == nullptr) {
    return true;
  }
  QTextDocument* document = page->GetTextPage()->document();

  if (page->ReadDone() && document->isModified()) {
    QMessageBox::StandardButton result = QMessageBox::Cancel;

    // write title of tab to docname and remove the leading *
    QString doc_name = tab_widget_->tabText(tab_widget_->currentIndex());
    doc_name.remove(0, 2);

    const QString& file_path = page->GetFilePath();
    if (askToSave) {
      result = QMessageBox::warning(
          this, _("Unsaved document"),
          QString(_("The document \"%1\" has been modified. Do you want to "
                    "save your changes?"))
                  .arg(doc_name) +
              "<br/><b>" + _("Note:") + "</b>" +
              _("If you don't save these files, all changes are "
                "lost.") +
              "<br/>",
          QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    }
    if ((result == QMessageBox::Save) || (!askToSave)) {
      if (file_path.isEmpty()) {
        // QString docname = tabWidget->tabText(tabWidget->currentIndex());
        // docname.remove(0,2);
        return SlotSaveAs();
      } else {
        return save_file(file_path);
      }
    } else if (result == QMessageBox::Discard) {
      return true;
    } else {
      return false;
    }
  }

  // destroy
  page->PrepareToDestroy();
  return true;
}

bool TextEdit::MaybeSaveAnyTab() {
  // get a list of all unsaved documents and their tabids
  QHash<int, QString> unsaved_docs = this->UnsavedDocuments();

  /*
   * no unsaved documents, so app can be closed
   */
  if (unsaved_docs.empty()) {
    return true;
  }
  /*
   * only 1 unsaved document -> set modified tab as current
   * and show normal unsaved doc dialog
   */
  if (unsaved_docs.size() == 1) {
    int modifiedTab = unsaved_docs.keys().at(0);
    tab_widget_->setCurrentIndex(modifiedTab);
    return maybe_save_current_tab(true);
  }

  /*
   * more than one unsaved documents
   */
  if (unsaved_docs.size() > 1) {
    QHashIterator<int, QString> i(unsaved_docs);

    QuitDialog* dialog;
    dialog = new QuitDialog(this, unsaved_docs);
    int result = dialog->exec();

    // if result is QDialog::Rejected, discard or cancel was clicked
    if (result == QDialog::Rejected) {
      // return true, if discard is clicked, so app can be closed
      if (dialog->IsDiscarded()) {
        return true;
      } else {
        return false;
      }
    } else {
      bool all_saved = true;
      QList<int> tabIdsToSave = dialog->GetTabIdsToSave();

      for (const auto& tabId : tabIdsToSave) {
        tab_widget_->setCurrentIndex(tabId);
        if (!maybe_save_current_tab(false)) {
          all_saved = false;
        }
      }
      return all_saved;
    }
  }
  // code should never reach this statement
  return false;
}

PlainTextEditorPage* TextEdit::CurTextPage() const {
  return qobject_cast<PlainTextEditorPage*>(tab_widget_->currentWidget());
}

FilePage* TextEdit::CurFilePage() const {
  auto* curFilePage = qobject_cast<FilePage*>(tab_widget_->currentWidget());
  if (curFilePage != nullptr) {
    return curFilePage;
  } else {
    return nullptr;
  }
}

int TextEdit::TabCount() const { return tab_widget_->count(); }

PlainTextEditorPage* TextEdit::SlotCurPageTextEdit() const {
  auto* curPage =
      qobject_cast<PlainTextEditorPage*>(tab_widget_->currentWidget());
  return curPage;
}

FilePage* TextEdit::SlotCurPageFileTreeView() const {
  auto* curPage = qobject_cast<FilePage*>(tab_widget_->currentWidget());
  return curPage;
}

void TextEdit::SlotQuote() const {
  if (tab_widget_->count() == 0 || CurTextPage() == nullptr) {
    return;
  }

  QTextCursor cursor(CurTextPage()->GetTextPage()->document());

  // beginEditBlock and endEditBlock() let operation look like single undo/redo
  // operation
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
  QTextCursor cursor(CurTextPage()->GetTextPage()->document());
  cursor.beginEditBlock();
  this->CurTextPage()->GetTextPage()->selectAll();
  this->CurTextPage()->GetTextPage()->insertPlainText(text);
  cursor.endEditBlock();
}

void TextEdit::LoadFile(const QString& fileName) {
  QFile file(fileName);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    QMessageBox::warning(
        this, _("Warning"),
        (boost::format(_("Cannot read file %1%:\n%2%.")) %
         fileName.toStdString() % file.errorString().toStdString())
            .str()
            .c_str());
    return;
  }
  QTextStream in(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  CurTextPage()->GetTextPage()->setPlainText(in.readAll());
  QApplication::restoreOverrideCursor();
  SlotCurPageTextEdit()->SetFilePath(fileName);
  tab_widget_->setTabText(tab_widget_->currentIndex(), stripped_name(fileName));
  file.close();
  // statusBar()->showMessage(_("File loaded"), 2000);
}

QString TextEdit::stripped_name(const QString& full_file_name) {
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
  document->print(&printer);

  // statusBar()->showMessage(_("Ready"), 2000);
#endif
}

void TextEdit::SlotShowModified() const {
  int index = tab_widget_->currentIndex();
  QString title = tab_widget_->tabText(index);
  // if doc is modified now, add leading * to title,
  // otherwise remove the leading * from the title
  if (CurTextPage()->GetTextPage()->document()->isModified()) {
    tab_widget_->setTabText(index, title.prepend("* "));
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
QHash<int, QString> TextEdit::UnsavedDocuments() const {
  QHash<int, QString> unsavedDocs;  // this list could be used to implement
                                    // gedit like "unsaved changed"-dialog

  for (int i = 0; i < tab_widget_->count(); i++) {
    auto* ep = qobject_cast<PlainTextEditorPage*>(tab_widget_->widget(i));
    if (ep != nullptr && ep->ReadDone() &&
        ep->GetTextPage()->document()->isModified()) {
      QString doc_name = tab_widget_->tabText(i);
      LOG(INFO) << "unsaved" << doc_name.toStdString();

      // remove * before name of modified doc
      doc_name.remove(0, 2);
      unsavedDocs.insert(i, doc_name);
    }
  }
  return unsavedDocs;
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

void TextEdit::slot_file_page_path_changed(const QString& path) const {
  int index = tab_widget_->currentIndex();
  QString mPath;
  QFileInfo fileInfo(path);
  QString tPath = fileInfo.absoluteFilePath();
  if (path.size() > 18) {
    mPath = tPath.mid(tPath.size() - 18, 18).prepend("...");
  } else {
    mPath = tPath;
  }
  tab_widget_->setTabText(index, mPath);
}

}  // namespace GpgFrontend::UI
