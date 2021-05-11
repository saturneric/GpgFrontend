/*
 *      textedit.h
 *
 *      Copyright 2008 gpg4usb-team <gpg4usb@cpunk.de>
 *
 *      This file is part of gpg4usb.
 *
 *      Gpg4usb is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      Gpg4usb is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with gpg4usb.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef __TEXTEDIT_H__
#define __TEXTEDIT_H__

#include "editorpage.h"
#include "helppage.h"
#include "quitdialog.h"

QT_BEGIN_NAMESPACE
class QDebug;
class QtGui;
class QFileDialog;
class QMessageBox;
class QFileInfo;
class QApplication;
class QFile;
class QTextEdit;
class QFileDialog;
class QMessageBox;
class QWidget;
class QString;
class QTabWidget;
QT_END_NAMESPACE

/**
 * @brief TextEdit class
 */
class TextEdit : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief
     */
    TextEdit();

    /**
     * @details Load the content of file into the current textpage
     *
     * @param fileName QString containing the filename to load
     * @return nothing
     */
    void loadFile(const QString &fileName);


    /**
     * @details Checks if there are unsaved documents in any tab,
     *  which may need to be saved. Call this function before
     *  closing the programme or all tabs.
     * @return \li false, if the close event should be aborted.
     *         \li true, otherwise
     */
    bool maybeSaveAnyTab();

    int tabCount();
    /**
     * @details textpage of the currently activated tab
     * @return \li reference to QTextEdit if tab has one
     *         \li 0 otherwise (e.g. if helppage)
     */
    QTextEdit* curTextPage();

    QTextBrowser* curHelpPage();

    /**
     * @details  List of currently unsaved tabs.
     * @returns QHash<int, QString> Hash of tabindexes and title of unsaved tabs.
     */
    QHash<int, QString> unsavedDocuments();

    QTabWidget *tabWidget; /** Widget containing the tabs of the editor */

public slots:
    /**
     * @details Return pointer to the currently activated tabpage.
     *
     */
    EditorPage *slotCurPage();

    /**
     * @details Insert a ">" at the begining of every line of current textedit.
     */
    void slotQuote();

    /**
      * @details replace the text of currently active textedit with given text.
      * @param text to fill on.
      */
    void slotFillTextEditWithText(QString text);

    /**
     * @details Saves the content of the current tab, if it has a filepath
     * otherwise it calls saveAs for the current tab
     */
    void slotSave();

    /**
     * @details Opens a savefiledialog and calls saveFile with the choosen filename.
     *
     * @return Return the return value of the savefile method
     */
    bool slotSaveAs();

    /**
     * @details Show an OpenFileDoalog and open the file in a new tab.
     * Shows an error dialog, if the open fails.
     * Set the focus to the tab of the opened file.
     */
    void slotOpen();

    /**
     * @details Open a print-dialog for the current tab
     */
    void slotPrint();

    /**
     * @details Adds a new tab with the title "untitled"+countpage+".txt"
     *                      Sets the focus to the new tab. Increase Tab-Count by one
     */
    void slotNewTab();

    /**
     * @details Adds a new tab with the given title and opens given html file.
     * Increase Tab-Count by one
     * @param title title for the tab
     * @param path  path for html file to show
     */
    void slotNewHelpTab(QString title, QString path);

    /**
     * @details put a * in front of current tabs title, if current textedit is modified
     */
    void slotShowModified();

    /**
     * @details close the current tab and decrease TabWidget->count by \a 1
     *
     */
    void slotCloseTab();

    /**
     * @details Switch to the next tab.
     *
     */
    void slotSwitchTabUp();

    /**
     * @details Switch to the previous tab.
     *
     */
    void slotSwitchTabDown();

private:
    /**
     * @details return just a filename stripped of a whole path
     *
     * @param a filename path
     * @return QString containing the filename
     */
    QString strippedName(const QString &fullFileName);

    /**
     * @brief
     *
     */
    bool maybeSaveFile();

    /**
     * @brief
     *
     * @param askToSave
     */
    bool maybeSaveCurrentTab(bool askToSave);

    /****************************************************************************************
     * Name:                countPage
     * Description:         int cotaining the number of added tabs
    */
    int countPage; /* TODO */

private slots:
    /**
     * @details Remove the tab with given index
     *
     * @param index Tab-number to remove
     */
    void removeTab(int index);

    /**
     * @details Cut selected text in current textpage.
     */
    void slotCut();

    /**
     * @details Copy selected text of current textpage to clipboard.
     */
    void slotCopy();

    /**
     * @details Paste text from clipboard to current textpage.
     */
    void slotPaste();

    /**
     * @details Undo last change in current textpage.
     *
     */
    void slotUndo();
    /****************************************************************************************
     * Name:                redo
     * Description:         redo last change in current textpage
     * Parameters:          none
     * Return Values:       none
     * Change on members:   none
    */
    /**
     * @brief
     *
     */
    void slotRedo();

    void slotZoomIn();
    void slotZoomOut();
    /****************************************************************************************
     * Name:                selectAll
     * Description:         select all in current textpage
     * Parameters:          none
     * Return Values:       none
     * Change on members:   none
    */
    /**
     * @brief
     *
     */
    void slotSelectAll();

protected:
    // void dragEnterEvent(QDragEnterEvent *event);
    // void dropEvent(QDropEvent* event);
    /****************************************************************************************
     * Name:                saveFile
     * Description:         Saves the content of currentTab to the file filename
     * Parameters:          QString filename contains the full path of the file to save
     * Return Values:       true, if the file was saved succesfully
     *                      false, if parameter filename is empty or the saving failed
     * Change on members:   sets isModified of the current tab to false
    */
    /**
     * @brief
     *
     * @param fileName
     */
    bool saveFile(const QString &fileName);
};
#endif // __TEXTEDIT_H__
