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

#ifndef GPGFRONTEND_KEYSETEXPIREDATEDIALOG_H
#define GPGFRONTEND_KEYSETEXPIREDATEDIALOG_H

#include "GpgFrontend.h"
#include "gpg/GpgContext.h"
#include "gpg/model/GpgKey.h"
#include "gpg/model/GpgSubKey.h"

class KeySetExpireDateDialog : public QDialog {
Q_OBJECT
public:
    explicit KeySetExpireDateDialog(GpgFrontend::GpgContext *ctx, const GpgKey &key, const GpgSubKey *subkey, QWidget *parent = nullptr);

private:
    GpgFrontend::GpgContext *mCtx;
    const GpgKey &mKey;
    const GpgSubKey *mSubkey;

    QDateTimeEdit *dateTimeEdit{};
    QPushButton *confirmButton{};
    QCheckBox *nonExpiredCheck{};

private slots:
    void slotConfirm();
    void slotNonExpiredChecked(int state);
};


#endif //GPGFRONTEND_KEYSETEXPIREDATEDIALOG_H
