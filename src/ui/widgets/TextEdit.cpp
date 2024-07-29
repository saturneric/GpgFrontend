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

#include "ui/widgets/TextEdit.h"

#include <QtPrintSupport>
#include <tuple>
#include <vector>

#include "core/GpgModel.h"
#include "core/function/GlobalSettingStation.h"
#include "core/model/CacheObject.h"
#include "ui/UISignalStation.h"

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
  QString header = tr("untitled") + QString::number(++count_page_) + ".txt";

  auto* page = new PlainTextEditorPage();
  auto index = tab_widget_->addTab(page, header);
  tab_widget_->setTabIcon(index, QIcon(":/icons/file.png"));
  tab_widget_->setCurrentIndex(tab_widget_->count() - 1);
  page->GetTextPage()->setFocus();
  connect(page->GetTextPage()->document(), &QTextDocument::modificationChanged,
          this, &TextEdit::SlotShowModified);
  connect(page->GetTextPage()->document(), &QTextDocument::contentsChanged,
          this, &TextEdit::slot_save_status_to_cache_for_revovery);
}

void TextEdit::SlotNewTabWithContent(QString title, const QString& content) {
  QString header = tr("untitled") + QString::number(++count_page_) + ".txt";
  if (!title.isEmpty()) {
    // modify title
    if (!title.isEmpty() && title[0] == '*') {
      title.remove(0, 1);
    }
    // set title
    header = title;
  }

  auto* page = new PlainTextEditorPage();
  auto index = tab_widget_->addTab(page, header);
  tab_widget_->setTabIcon(index, QIcon(":/icons/file.png"));
  tab_widget_->setCurrentIndex(tab_widget_->count() - 1);
  page->GetTextPage()->setFocus();
  connect(page->GetTextPage()->document(), &QTextDocument::modificationChanged,
          this, &TextEdit::SlotShowModified);
  connect(page->GetTextPage()->document(), &QTextDocument::contentsChanged,
          this, &TextEdit::slot_save_status_to_cache_for_revovery);

  // set content with modified status
  page->GetTextPage()->document()->setPlainText(content);
}

void TextEdit::slotNewHelpTab(const QString& title, const QString& path) const {
  auto* page = new HelpPage(path);
  tab_widget_->addTab(page, title);
  tab_widget_->setCurrentIndex(tab_widget_->count() - 1);
}

void TextEdit::SlotNewFileTab() {
  auto const target_dir = QFileDialog::getExistingDirectory(
      this, tr("Open Directory"), QDir::home().path(),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (target_dir.isEmpty()) return;

  auto* page = new FilePage(qobject_cast<QWidget*>(parent()), target_dir);
  auto index = tab_widget_->addTab(page, QString());
  tab_widget_->setTabIcon(index, QIcon(":/icons/file-browser.png"));
  tab_widget_->setCurrentIndex(tab_widget_->count() - 1);
  connect(page, &FilePage::SignalPathChanged, this,
          &TextEdit::slot_file_page_path_changed);
  page->SlotGoPath();
}

void TextEdit::SlotOpenFile(const QString& path) {
  QFile file(path);
  auto result = file.open(QIODevice::ReadOnly | QIODevice::Text);
  if (result) {
    auto* page = new PlainTextEditorPage(path);
    connect(page->GetTextPage()->document(),
            &QTextDocument::modificationChanged, this,
            &TextEdit::SlotShowModified);
    // connect to cache recovery fucntion
    connect(page->GetTextPage()->document(), &QTextDocument::contentsChanged,
            this, &TextEdit::slot_save_status_to_cache_for_revovery);

    QApplication::setOverrideCursor(Qt::WaitCursor);
    auto index = tab_widget_->addTab(page, stripped_name(path));
    tab_widget_->setTabIcon(index, QIcon(":/icons/file.png"));
    tab_widget_->setCurrentIndex(tab_widget_->count() - 1);
    QApplication::restoreOverrideCursor();
    page->GetTextPage()->setFocus();
    page->ReadFile();
  } else {
    QMessageBox::warning(
        this, tr("Warning"),
        tr("Cannot read file %1:\n%2.").arg(path).arg(file.errorString()));
  }

  file.close();
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
  if (tab_widget_->count() == 0 || SlotCurPageTextEdit() == 0) {
    return;
  }

  QString file_name = SlotCurPageTextEdit()->GetFilePath();

  if (file_name.isEmpty()) {
    // QString docname = tabWidget->tabText(tabWidget->currentIndex());
    // docname.remove(0,2);
    SlotSaveAs();
  } else {
    save_file(file_name);
  }
}

bool TextEdit::save_file(const QString& fileName) {
  if (fileName.isEmpty()) {
    return false;
  }

  PlainTextEditorPage* page = SlotCurPageTextEdit();
  if (page == nullptr) return false;

  QFile file(fileName);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream output_stream(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    output_stream << page->GetTextPage()->toPlainText();
    QApplication::restoreOverrideCursor();
    QTextDocument* document = page->GetTextPage()->document();

    document->setModified(false);

    int cur_index = tab_widget_->currentIndex();
    tab_widget_->setTabText(cur_index, stripped_name(fileName));
    page->SetFilePath(fileName);
    page->NotifyFileSaved();

    file.close();
    return true;
  }
  QMessageBox::warning(
      this, tr("Warning"),
      tr("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
  return false;
}

auto TextEdit::SlotSaveAs() -> bool {
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

  return save_file(QFileDialog::getSaveFileName(this, tr("Save file"), path));
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
auto TextEdit::maybe_save_current_tab(bool askToSave) -> bool {
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
    if ((result == QMessageBox::Save) || (!askToSave)) {
      if (file_path.isEmpty()) {
        // QString docname = tabWidget->tabText(tabWidget->currentIndex());
        // docname.remove(0,2);
        return SlotSaveAs();
      }
      return save_file(file_path);
    }
    return result == QMessageBox::Discard;
  }
  page->deleteLater();
  return true;
}

auto TextEdit::MaybeSaveAnyTab() -> bool {
  // get a list of all unsaved documents and their tab ids
  QHash<int, QString> const unsaved_docs = this->UnsavedDocuments();

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
    int const modified_tab = unsaved_docs.keys().at(0);
    tab_widget_->setCurrentIndex(modified_tab);
    return maybe_save_current_tab(true);
  }

  /*
   * more than one unsaved documents
   */
  if (unsaved_docs.size() > 1) {
    QHashIterator<int, QString> const i(unsaved_docs);

    QuitDialog* dialog;
    dialog = new QuitDialog(
        this->parentWidget() != nullptr ? this->parentWidget() : this,
        unsaved_docs);
    int const result = dialog->exec();

    // if result is QDialog::Rejected, discard or cancel was clicked
    if (result == QDialog::Rejected) {
      // return true, if discard is clicked, so app can be closed
      return dialog->IsDiscarded();
    }

    bool all_saved = true;
    QList<int> const tab_ids_to_save = dialog->GetTabIdsToSave();
    for (const auto& tab_id : tab_ids_to_save) {
      tab_widget_->setCurrentIndex(tab_id);
      if (!maybe_save_current_tab(false)) {
        all_saved = false;
      }
    }
    return all_saved;
  }
  // code should never reach this statement
  return false;
}

PlainTextEditorPage* TextEdit::CurTextPage() const {
  return qobject_cast<PlainTextEditorPage*>(tab_widget_->currentWidget());
}

void TextEdit::SlotAppendText2CurTextPage(const QString& text) {
  if (CurTextPage() == nullptr) SlotNewTab();
  CurTextPage()->GetTextPage()->appendPlainText(text);
}

FilePage* TextEdit::CurFilePage() const {
  auto* cur_file_page = qobject_cast<FilePage*>(tab_widget_->currentWidget());
  if (cur_file_page != nullptr) {
    return cur_file_page;
  }
  return nullptr;
}

int TextEdit::TabCount() const { return tab_widget_->count(); }

PlainTextEditorPage* TextEdit::SlotCurPageTextEdit() const {
  auto* cur_page =
      qobject_cast<PlainTextEditorPage*>(tab_widget_->currentWidget());
  return cur_page;
}

FilePage* TextEdit::SlotCurPageFileTreeView() const {
  auto* cur_page = qobject_cast<FilePage*>(tab_widget_->currentWidget());
  return cur_page;
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
        this, tr("Warning"),
        tr("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
    return;
  }
  QTextStream in(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  CurTextPage()->GetTextPage()->setPlainText(in.readAll());
  QApplication::restoreOverrideCursor();
  SlotCurPageTextEdit()->SetFilePath(fileName);
  tab_widget_->setTabText(tab_widget_->currentIndex(), stripped_name(fileName));
  file.close();
  // statusBar()->showMessage(tr("File loaded"), 2000);
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

void TextEdit::slot_file_page_path_changed(const QString& path) const {
  int index = tab_widget_->currentIndex();
  QString m_path;
  QFileInfo file_info(path);
  QString t_path = file_info.absoluteFilePath();
  if (path.size() > 18) {
    m_path = t_path.mid(t_path.size() - 18, 18).prepend("...");
  } else {
    m_path = t_path;
  }
  tab_widget_->setTabText(index, m_path);

  emit UISignalStation::GetInstance()
      -> SignalMainWindowlUpdateBasicalOperaMenu(0);
}

void TextEdit::slot_save_status_to_cache_for_revovery() {
  if (this->text_page_data_modified_count_++ % 8 != 0) return;

  auto settings =
      GpgFrontend::GlobalSettingStation::GetInstance().GetSettings();

  bool restore_text_editor_page =
      settings.value("basic/restore_text_editor_page", false).toBool();
  if (!restore_text_editor_page) {
    qCDebug(ui, "restore_text_editor_page is false, ignoring...");
    return;
  }

  int tab_count = tab_widget_->count();
  std::vector<std::tuple<int, QString, QString>> unsaved_pages;

  for (int i = 0; i < tab_count; i++) {
    auto* target_page =
        qobject_cast<PlainTextEditorPage*>(tab_widget_->widget(i));

    // if this page is no textedit, there should be nothing to save
    if (target_page == nullptr) {
      continue;
    }

    auto* document = target_page->GetTextPage()->document();
    auto tab_title = tab_widget_->tabText(i);
    if (!target_page->ReadDone() || !target_page->isEnabled() ||
        !document->isModified()) {
      continue;
    }

    unsaved_pages.emplace_back(i, tab_title, document->toRawText());
  }

  CacheObject cache("editor_unsaved_pages");
  QJsonArray unsaved_page_array;
  for (const auto& page : unsaved_pages) {
    const auto [index, title, content] = page;

    QJsonObject page_json;
    page_json["index"] = index;
    page_json["title"] = title;
    page_json["content"] = content;

    unsaved_page_array.push_back(page_json);
  }

  cache.setArray(unsaved_page_array);
}

}  // namespace GpgFrontend::UI
