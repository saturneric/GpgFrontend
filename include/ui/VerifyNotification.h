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

#include "ui/EditorPage.h"
#include "VerifyDetailsDialog.h"

QT_BEGIN_NAMESPACE
class QLabel;

class QHBoxLayout;

class QMenu;

class QPushButton;

QT_END_NAMESPACE

/**
 * @details Enumeration for the status of Verifylabel
 */
typedef enum {
    VERIFY_ERROR_OK = 0,
    VERIFY_ERROR_WARN = 1,
    VERIFY_ERROR_CRITICAL = 2,
    VERIFY_ERROR_NEUTRAL [[maybe_unused]] = 3,
} verify_label_status;

/**
 * @brief Class for handling the verifylabel shown at buttom of a textedit-page
 */
class VerifyNotification : public QWidget {
Q_OBJECT
public:
    /**
     * @brief
     *
     * @param ctx The GPGme-Context
     * @param parent The parent widget
     */
    explicit VerifyNotification(QWidget *parent, GpgME::GpgContext *ctx, KeyList *keyList, QTextEdit *edit);

    /**
     * @details Set the text and background-color of verify notification.
     *
     * @param text The text to be set.
     * @param verifyLabelStatus The status of label to set the specified color.
     */
    void setVerifyLabel(const QString& text, verify_label_status verifyLabelStatus);

    /**
     * @details Show the import from keyserver-action in detailsmenu.
     * @param visible show the action, if visible is true, otherwise hide it.
     */
    void showImportAction(bool visible);

    QStringList *keysNotInList; /** List with keys, which are in signature but not in keylist */


public slots:

    /**
     * @details Import the keys contained in keysNotInList from keyserver
     *
     */
    void slotImportFromKeyserver();

    /**
     * @details Show a dialog with signing details.
     */
    void slotShowVerifyDetails();

    /**
     * @details Refresh the contents of dialog.
     */
    bool slotRefresh();

private:
    QMenu *detailMenu; /** Menu for te Button in verfiyNotification */
    QAction *importFromKeyserverAct; /** Action for importing keys from keyserver which are notin keylist */
    QAction *showVerifyDetailsAct; /** Action for showing verify detail dialog */
    QPushButton *detailsButton; /** Button shown in verifynotification */
    QLabel *verifyLabel; /** Label holding the text shown in verifyNotification */
    GpgME::GpgContext *mCtx; /** GpgME Context */
    KeyList *mKeyList; /** Table holding the keys */
    QTextEdit *mTextpage; /** Textedit associated to the notification */
    [[maybe_unused]] QVector<QString> verifyDetailStringVector; /** Vector containing the text for labels in verifydetaildialog */
    [[maybe_unused]] QVector<verify_label_status> verifyDetailStatusVector; /** Vector containing the status for labels in verifydetaildialog */

};

#endif // __VERIFYNOTIFICATION_H__
