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

#include "ui/VerifyDetailsDialog.h"

VerifyDetailsDialog::VerifyDetailsDialog(QWidget *parent, GpgME::GpgContext *ctx, KeyList *keyList, gpg_error_t error,
                                         gpgme_verify_result_t result) :
        QDialog(parent), mCtx(ctx), mKeyList(keyList), sign(result->signatures), error(error) {


    this->setWindowTitle(tr("Signature Details"));

    connect(mCtx, SIGNAL(signalKeyInfoChanged()), this, SLOT(slotRefresh()));
    mainLayout = new QHBoxLayout();
    this->setLayout(mainLayout);

    slotRefresh();

    this->exec();
}

void VerifyDetailsDialog::slotRefresh() {

    mVbox = new QWidget();
    auto *mVboxLayout = new QVBoxLayout(mVbox);
    mainLayout->addWidget(mVbox);

    // Button Box for close button
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));

    mVboxLayout->addWidget(new QLabel(tr("Status: ") + gpgme_strerror(error)));

    if (sign == nullptr) {
        mVboxLayout->addWidget(new QLabel(tr("No valid input found")));
        mVboxLayout->addWidget(buttonBox);
        return;
    }

    // Get timestamp of signature of current text
    QDateTime timestamp;
    timestamp.setTime_t(sign->timestamp);

    // Set the title widget depending on sign status
    if (gpg_err_code(sign->status) == GPG_ERR_BAD_SIGNATURE) {
        mVboxLayout->addWidget(new QLabel(tr("Error Validating signature")));
    } else if (mInputSignature != nullptr) {
        mVboxLayout->addWidget(new QLabel(
                tr("File was signed on %1 <br/> It Contains:<br/><br/>").arg(QLocale::system().toString(timestamp))));
    } else {
        mVboxLayout->addWidget(new QLabel(tr("Signed on %1 <br/> It Contains:<br /><br/>").arg(
                QLocale::system().toString(timestamp))));
    }
    // Add informationbox for every single key
    while (sign) {
        auto *sbox = new VerifyKeyDetailBox(this, mCtx, mKeyList, sign);
        sign = sign->next;
        mVboxLayout->addWidget(sbox);
    }

    mVboxLayout->addWidget(buttonBox);
}
