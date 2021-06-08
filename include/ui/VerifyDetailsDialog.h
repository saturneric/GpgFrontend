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

#ifndef __VERIFYDETAILSDIALOG_H__
#define __VERIFYDETAILSDIALOG_H__

#include "ui/EditorPage.h"
#include "ui/widgets/VerifyKeyDetailBox.h"

class VerifyDetailsDialog : public QDialog {
Q_OBJECT
public:
    explicit VerifyDetailsDialog(QWidget *parent, GpgME::GpgContext *ctx, KeyList *keyList, gpg_error_t error,
                                 gpgme_verify_result_t result);

private slots:

    void slotRefresh();

private:
    GpgME::GpgContext *mCtx;
    KeyList *mKeyList;
    QHBoxLayout *mainLayout;
    QWidget *mVbox{};
    QByteArray *mInputData{}; /** Data to be verified */
    QByteArray *mInputSignature{}; /** Data to be verified */
    QDialogButtonBox *buttonBox{};
    gpgme_signature_t sign;
    gpgme_error_t error;
};

#endif // __VERIFYDETAILSDIALOG_H__
