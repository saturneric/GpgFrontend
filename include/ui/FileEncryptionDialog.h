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

#ifndef __FILEENCRYPTIONDIALOG_H__
#define __FILEENCRYPTIONDIALOG_H__

#include "gpg/GpgContext.h"
#include "ui/widgets/KeyList.h"
#include "VerifyDetailsDialog.h"


/**
 * @brief
 *
 * @class FileEncryptionDialog fileencryptiondialog.h "fileencryptiondialog.h"
 */
class FileEncryptionDialog : public QDialog {
Q_OBJECT

public:

    enum DialogAction {
        Encrypt,
        Decrypt,
        Sign,
        Verify
    };

    /**
     * @brief
     *
     * @fn FileEncryptionDialog
     * @param ctx
     * @param keyList
     * @param parent
     */
    FileEncryptionDialog(GpgME::GpgContext *ctx, QStringList keyList, DialogAction action, QWidget *parent = nullptr);

public slots:

    /**
     * @details
     *
     * @fn selectInputFile
     */
    void slotSelectInputFile();

    /**
     * @brief
     *
     * @fn selectOutputFile
     */
    void slotSelectOutputFile();

    /**
     * @brief
     *
     * @fn selectSignFile
     */
    void slotSelectSignFile();

    /**
     * @brief
     *
     * @fn executeAction
     */
    void slotExecuteAction();

    /**
     * @brief
     *
     * @fn hideKeyList
     */
    void slotHideKeyList();

    /**
     * @brief
     *
     * @fn showKeyList
     */
    void slotShowKeyList();

private:
    QLineEdit *outputFileEdit; /**< TODO */
    QLineEdit *inputFileEdit; /**< TODO */
    QLineEdit *signFileEdit; /**< TODO */
    DialogAction mAction; /**< TODO */
    QLabel *statusLabel; /**< TODO */
protected:
    GpgME::GpgContext *mCtx; /**< TODO */
    KeyList *mKeyList; /**< TODO */

};

#endif // __FILEENCRYPTIONDIALOG_H__
