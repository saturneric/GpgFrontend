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
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/widgets/TextEdit.h"

#include <boost/format.hpp>

namespace GpgFrontend::UI {

TextEdit::TextEdit(QWidget* parent) : QWidget(parent) {
  countPage = 0;
  tabWidget = new QTabWidget(this);
  tabWidget->setMovable(true);
  tabWidget->setTabsClosable(true);
  tabWidget->setDocumentMode(true);

  auto* layout = new QVBoxLayout;
  layout->addWidget(tabWidget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  setLayout(layout);

  connect(tabWidget, SIGNAL(tabCloseRequested(int)), this,
          SLOT(removeTab(int)));
  slotNewTab();
  setAcceptDrops(false);
}

void TextEdit::slotNewTab() {
  QString header = _("untitled") + QString::number(++countPage) + ".txt";

  auto* page = new EditorPage();
  tabWidget->addTab(page, header);
  tabWidget->setCurrentIndex(tabWidget->count() - 1);
  page->getTextPage()->setFocus();
  connect(page->getTextPage()->document(), SIGNAL(modificationChanged(bool)),
          this, SLOT(slotShowModified()));
}

void TextEdit::slotNewHelpTab(const QString& title, const QString& path) const {
  auto* page = new HelpPage(path);
  tabWidget->addTab(page, title);
  tabWidget->setCurrentIndex(tabWidget->count() - 1);
}

void TextEdit::slotNewFileTab() const {
  auto* page = new FilePage(qobject_cast<QWidget*>(parent()));
  tabWidget->addTab(page, "[Browser]");
  tabWidget->setCurrentIndex(tabWidget->count() - 1);
  connect(page, SIGNAL(pathChanged(const QString&)), this,
          SLOT(slotFilePagePathChanged(const QString&)));
}

void TextEdit::slotOpenFile(QString& path) {
  QFile file(path);
  LOG(INFO) << " path" << path.toStdString();
  auto result = file.open(QIODevice::ReadOnly | QIODevice::Text);
  if (result) {
    auto* page = new EditorPage(path);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    tabWidget->addTab(page, strippedName(path));
    tabWidget->setCurrentIndex(tabWidget->count() - 1);
    QApplication::restoreOverrideCursor();
    page->getTextPage()->setFocus();
    page->ReadFile();
  } else {
    QMessageBox::warning(this, _("Warning"),
                         (boost::format(_("Cannot read file %1%:\n%2%.")) %
                          path.toStdString() % file.errorString().toStdString())
                             .str()
                             .c_str());
  }

  file.close();
  LOG(INFO) << "done";
}

void TextEdit::slotOpen() {
  QStringList fileNames =
      QFileDialog::getOpenFileNames(this, _("Open file"), QDir::currentPath());
  for (const auto& fileName : fileNames) {
    if (!fileName.isEmpty()) {
      QFile file(fileName);

      if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto* page = new EditorPage(fileName);

        QTextStream in(&file);
        QApplication::setOverrideCursor(Qt::WaitCursor);
        page->getTextPage()->setPlainText(in.readAll());
        page->setFilePath(fileName);
        QTextDocument* document = page->getTextPage()->document();
        document->setModified(false);

        tabWidget->addTab(page, strippedName(fileName));
        tabWidget->setCurrentIndex(tabWidget->count() - 1);
        QApplication::restoreOverrideCursor();
        page->getTextPage()->setFocus();
        connect(page->getTextPage()->document(),
                SIGNAL(modificationChanged(bool)), this,
                SLOT(slotShowModified()));
        // enableAction(true)
        file.close();
      } else {
        QMessageBox::warning(
            this, _("Warning"),
            (boost::format(_("Cannot read file %1%:\n%2%.")) %
             fileName.toStdString() % file.errorString().toStdString())
                .str()
                .c_str());
      }
    }
  }
}

void TextEdit::slotSave() {
  if (tabWidget->count() == 0 || slotCurPageTextEdit() == 0) {
    return;
  }

  QString fileName = slotCurPageTextEdit()->getFilePath();

  if (fileName.isEmpty()) {
    // QString docname = tabWidget->tabText(tabWidget->currentIndex());
    // docname.remove(0,2);
    slotSaveAs();
  } else {
    saveFile(fileName);
  }
}

bool TextEdit::saveFile(const QString& fileName) {
  if (fileName.isEmpty()) {
    return false;
  }

  QFile file(fileName);

  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    EditorPage* page = slotCurPageTextEdit();

    QTextStream outputStream(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    outputStream << page->getTextPage()->toPlainText();
    QApplication::restoreOverrideCursor();
    QTextDocument* document = page->getTextPage()->document();

    document->setModified(false);

    int curIndex = tabWidget->currentIndex();
    tabWidget->setTabText(curIndex, strippedName(fileName));
    page->setFilePath(fileName);
    //      statusBar()->showMessage(_("File saved"), 2000);
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

bool TextEdit::slotSaveAs() {
  if (tabWidget->count() == 0 || slotCurPageTextEdit() == nullptr) {
    return true;
  }

  EditorPage* page = slotCurPageTextEdit();
  QString path;
  if (!page->getFilePath().isEmpty()) {
    path = page->getFilePath();
  } else {
    path = tabWidget->tabText(tabWidget->currentIndex()).remove(0, 2);
  }

  QString fileName = QFileDialog::getSaveFileName(this, _("Save file"), path);
  return saveFile(fileName);
}

void TextEdit::slotCloseTab() {
  removeTab(tabWidget->currentIndex());
  if (tabWidget->count() != 0) {
    slotCurPageTextEdit()->getTextPage()->setFocus();
  }
}

void TextEdit::removeTab(int index) {
  // Do nothing, if no tab is opened
  if (tabWidget->count() == 0) {
    return;
  }

  // get the index of the actual current tab
  int lastIndex = tabWidget->currentIndex();

  // set the focus to argument index
  tabWidget->setCurrentIndex(index);

  if (maybeSaveCurrentTab(true)) {
    tabWidget->removeTab(index);

    if (index >= lastIndex) {
      tabWidget->setCurrentIndex(lastIndex);
    } else {
      tabWidget->setCurrentIndex(lastIndex - 1);
    }
  }

  if (tabWidget->count() == 0) {
    //  enableAction(false);
  }
}

/**
 * Check if current may need to be saved.
 * Call this function before closing the currently active tab-
 *
 * If it returns false, the close event should be aborted.
 */
bool TextEdit::maybeSaveCurrentTab(bool askToSave) {
  EditorPage* page = slotCurPageTextEdit();
  // if this page is no textedit, there should be nothing to save
  if (page == nullptr) {
    return true;
  }
  QTextDocument* document = page->getTextPage()->document();

  if (page->ReadDone() && document->isModified()) {
    QMessageBox::StandardButton result = QMessageBox::Cancel;

    // write title of tab to docname and remove the leading *
    QString docname = tabWidget->tabText(tabWidget->currentIndex());
    docname.remove(0, 2);

    const QString& filePath = page->getFilePath();
    if (askToSave) {
      result = QMessageBox::warning(
          this, _("Unsaved document"),
          QString(_("The document \"%1\" has been modified. Do you want to "
                    "save your changes?"))
                  .arg(docname) +
              "<br/><b>" + _("Note:") + "</b>" +
              _("If you don't save these files, all changes are "
                "lost.") +
              "<br/>",
          QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    }
    if ((result == QMessageBox::Save) || (!askToSave)) {
      if (filePath.isEmpty()) {
        // QString docname = tabWidget->tabText(tabWidget->currentIndex());
        // docname.remove(0,2);
        return slotSaveAs();
      } else {
        return saveFile(filePath);
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

bool TextEdit::maybeSaveAnyTab() {
  // get a list of all unsaved documents and their tabids
  QHash<int, QString> unsavedDocs = this->unsavedDocuments();

  /*
   * no unsaved documents, so app can be closed
   */
  if (unsavedDocs.empty()) {
    return true;
  }
  /*
   * only 1 unsaved document -> set modified tab as current
   * and show normal unsaved doc dialog
   */
  if (unsavedDocs.size() == 1) {
    int modifiedTab = unsavedDocs.keys().at(0);
    tabWidget->setCurrentIndex(modifiedTab);
    return maybeSaveCurrentTab(true);
  }

  /*
   * more than one unsaved documents
   */
  if (unsavedDocs.size() > 1) {
    QHashIterator<int, QString> i(unsavedDocs);

    QuitDialog* dialog;
    dialog = new QuitDialog(this, unsavedDocs);
    int result = dialog->exec();

    // if result is QDialog::Rejected, discard or cancel was clicked
    if (result == QDialog::Rejected) {
      // return true, if discard is clicked, so app can be closed
      if (dialog->isDiscarded()) {
        return true;
      } else {
        return false;
      }
    } else {
      bool all_saved = true;
      QList<int> tabIdsToSave = dialog->getTabIdsToSave();

      for (const auto& tabId : tabIdsToSave) {
        tabWidget->setCurrentIndex(tabId);
        if (!maybeSaveCurrentTab(false)) {
          all_saved = false;
        }
      }
      return all_saved;
    }
  }
  // code should never reach this statement
  return false;
}

QTextEdit* TextEdit::curTextPage() const {
  auto* curTextPage = qobject_cast<EditorPage*>(tabWidget->currentWidget());
  if (curTextPage != nullptr) {
    return curTextPage->getTextPage();
  } else {
    return nullptr;
  }
}

FilePage* TextEdit::curFilePage() const {
  auto* curFilePage = qobject_cast<FilePage*>(tabWidget->currentWidget());
  if (curFilePage != nullptr) {
    return curFilePage;
  } else {
    return nullptr;
  }
}

int TextEdit::tabCount() const { return tabWidget->count(); }

EditorPage* TextEdit::slotCurPageTextEdit() const {
  auto* curPage = qobject_cast<EditorPage*>(tabWidget->currentWidget());
  return curPage;
}

FilePage* TextEdit::slotCurPageFileTreeView() const {
  auto* curPage = qobject_cast<FilePage*>(tabWidget->currentWidget());
  return curPage;
}

void TextEdit::slotQuote() const {
  if (tabWidget->count() == 0 || curTextPage() == nullptr) {
    return;
  }

  QTextCursor cursor(curTextPage()->document());

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

void TextEdit::slotFillTextEditWithText(const QString& text) const {
  QTextCursor cursor(curTextPage()->document());
  cursor.beginEditBlock();
  this->curTextPage()->selectAll();
  this->curTextPage()->insertPlainText(text);
  cursor.endEditBlock();
}

void TextEdit::loadFile(const QString& fileName) {
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
  curTextPage()->setPlainText(in.readAll());
  QApplication::restoreOverrideCursor();
  slotCurPageTextEdit()->setFilePath(fileName);
  tabWidget->setTabText(tabWidget->currentIndex(), strippedName(fileName));
  file.close();
  // statusBar()->showMessage(_("File loaded"), 2000);
}

QString TextEdit::strippedName(const QString& fullFileName) {
  return QFileInfo(fullFileName).fileName();
}

void TextEdit::slotPrint() {
  if (tabWidget->count() == 0) {
    return;
  }

#ifndef QT_NO_PRINTER
  QTextDocument* document;
  if (curTextPage() != nullptr) {
    document = curTextPage()->document();
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

void TextEdit::slotShowModified() const {
  int index = tabWidget->currentIndex();
  QString title = tabWidget->tabText(index);
  // if doc is modified now, add leading * to title,
  // otherwise remove the leading * from the title
  if (curTextPage()->document()->isModified()) {
    tabWidget->setTabText(index, title.prepend("* "));
  } else {
    tabWidget->setTabText(index, title.remove(0, 2));
  }
}

void TextEdit::slotSwitchTabUp() const {
  if (tabWidget->count() > 1) {
    int newindex = (tabWidget->currentIndex() + 1) % (tabWidget->count());
    tabWidget->setCurrentIndex(newindex);
  }
}

void TextEdit::slotSwitchTabDown() const {
  if (tabWidget->count() > 1) {
    int newindex = (tabWidget->currentIndex() - 1 + tabWidget->count()) %
                   tabWidget->count();
    tabWidget->setCurrentIndex(newindex);
  }
}

/*
 *   return a hash of tabindexes and title of unsaved tabs
 */
QHash<int, QString> TextEdit::unsavedDocuments() const {
  QHash<int, QString> unsavedDocs;  // this list could be used to implement
                                    // gedit like "unsaved changed"-dialog

  for (int i = 0; i < tabWidget->count(); i++) {
    auto* ep = qobject_cast<EditorPage*>(tabWidget->widget(i));
    if (ep != nullptr && ep->ReadDone() &&
        ep->getTextPage()->document()->isModified()) {
      QString doc_name = tabWidget->tabText(i);
      LOG(INFO) << "unsaved" << doc_name.toStdString();

      // remove * before name of modified doc
      doc_name.remove(0, 2);
      unsavedDocs.insert(i, doc_name);
    }
  }
  return unsavedDocs;
}

void TextEdit::slotCut() const {
  if (tabWidget->count() == 0 || curTextPage() == nullptr) {
    return;
  }

  curTextPage()->cut();
}

void TextEdit::slotCopy() const {
  if (tabWidget->count() == 0) {
    return;
  }

  if (curTextPage() != nullptr) {
    curTextPage()->copy();
  }
}

void TextEdit::slotPaste() const {
  if (tabWidget->count() == 0 || curTextPage() == nullptr) {
    return;
  }

  curTextPage()->paste();
}

void TextEdit::slotUndo() const {
  if (tabWidget->count() == 0 || curTextPage() == nullptr) {
    return;
  }

  curTextPage()->undo();
}

void TextEdit::slotRedo() const {
  if (tabWidget->count() == 0 || curTextPage() == nullptr) {
    return;
  }

  curTextPage()->redo();
}

void TextEdit::slotZoomIn() const {
  if (tabWidget->count() == 0) {
    return;
  }

  if (curTextPage() != nullptr) {
    curTextPage()->zoomIn();
  }
}

void TextEdit::slotZoomOut() const {
  if (tabWidget->count() == 0) {
    return;
  }

  if (curTextPage() != nullptr) {
    curTextPage()->zoomOut();
  }
}

void TextEdit::slotSelectAll() const {
  if (tabWidget->count() == 0 || curTextPage() == nullptr) {
    return;
  }
  curTextPage()->selectAll();
}

void TextEdit::slotFilePagePathChanged(const QString& path) const {
  int index = tabWidget->currentIndex();
  QString mPath;
  QFileInfo fileInfo(path);
  QString tPath = fileInfo.absoluteFilePath();
  if (path.size() > 18) {
    mPath = tPath.mid(tPath.size() - 18, 18).prepend("...");
  } else {
    mPath = tPath;
  }
  mPath.prepend("[Browser] ");
  tabWidget->setTabText(index, mPath);
}

}  // namespace GpgFrontend::UI
