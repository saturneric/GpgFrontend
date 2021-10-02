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

#ifndef __VERIFYKEYDETAILBOX_H__
#define __VERIFYKEYDETAILBOX_H__

#include "ui/KeyServerImportDialog.h"
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::UI {

class VerifyKeyDetailBox : public QGroupBox {
  Q_OBJECT
 public:
  explicit VerifyKeyDetailBox(QWidget* parent,
                              KeyList* mKeyList,
                              gpgme_signature_t signature);

 private slots:

  void slotImportFormKeyserver();

 private:
  KeyList* mKeyList;

  static QString beautifyFingerprint(QString fingerprint);

  static QGridLayout* createKeyInfoGrid(gpgme_signature_t& signature);

  QString fpr;
};

}  // namespace GpgFrontend::UI

#endif  // __VERIFYKEYDETAILBOX_H__
