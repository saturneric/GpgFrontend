/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

#ifndef __VERIFYNOTIFICATION_H__
#define __VERIFYNOTIFICATION_H__

#include "EditorPage.h"
#include "FilePage.h"
#include "ui/VerifyDetailsDialog.h"
#include "gpg/result_analyse/VerifyResultAnalyse.h"

/**
 * @details Enumeration for the status of Verifylabel
 */
typedef enum {
    INFO_ERROR_OK = 0,
    INFO_ERROR_WARN = 1,
    INFO_ERROR_CRITICAL = 2,
    INFO_ERROR_NEUTRAL = 3,
} InfoBoardStatus;

/**
 * @brief Class for handling the verifylabel shown at buttom of a textedit-page
 */
class InfoBoardWidget : public QWidget {
Q_OBJECT
public:
    /**
     * @brief
     *
     * @param ctx The GPGme-Context
     * @param parent The parent widget
     */
    explicit InfoBoardWidget(QWidget *parent, GpgFrontend::GpgContext *ctx, KeyList *keyList);


    void associateTextEdit(QTextEdit *edit);

    void associateFileTreeView(FilePage *treeView);

    void associateTabWidget(QTabWidget *tab);

    void addOptionalAction(const QString& name, const std::function<void()>& action);

    void resetOptionActionsMenu();



    /**
     * @details Set the text and background-color of verify notification.
     *
     * @param text The text to be set.
     * @param verifyLabelStatus The status of label to set the specified color.
     */
    void setInfoBoard(const QString& text, InfoBoardStatus verifyLabelStatus);


    QStringList *keysNotInList; /** List with keys, which are in signature but not in keylist */


public slots:

    /**
     * @details Import the keys contained in keysNotInList from keyserver
     *
     */
    void slotImportFromKeyserver();

    void slotReset();

    /**
     * @details Refresh the contents of dialog.
     */
    void slotRefresh(const QString &text, InfoBoardStatus status);

private:

    QString appPath;
    QSettings settings;

    QMenu *detailMenu; /** Menu for te Button in verfiyNotification */
    QAction *importFromKeyserverAct; /** Action for importing keys from keyserver which are notin keylist */
    QTextEdit *infoBoard;
    GpgFrontend::GpgContext *mCtx; /** GpgME Context */
    KeyList *mKeyList; /** Table holding the keys */

    QTextEdit *mTextPage{ nullptr }; /** TextEdit associated to the notification */
    FilePage *mFileTreeView{nullptr }; /** TreeView associated to the notification */
    QTabWidget *mTabWidget{ nullptr }; /** TreeView associated to the notification */

    QHBoxLayout *actionButtonLayout;

    void deleteWidgetsInLayout(QLayout* layout, int start_index = 0);


};

#endif // __VERIFYNOTIFICATION_H__