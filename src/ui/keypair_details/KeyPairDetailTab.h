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
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#ifndef GPGFRONTEND_KEYPAIRDETAILTAB_H
#define GPGFRONTEND_KEYPAIRDETAILTAB_H

#include "gpg/GpgContext.h"
#include "ui/GpgFrontendUI.h"

#include "KeySetExpireDateDialog.h"
#include "ui/KeyServerImportDialog.h"
#include "ui/KeyUploadDialog.h"

namespace GpgFrontend::UI {

class KeyPairDetailTab : public QWidget {
  Q_OBJECT

  /**
   * @details Return QString with a space inserted at every fourth character
   *
   * @param fingerprint The fingerprint to be beautified
   */
  static QString beautifyFingerprint(QString fingerprint);

  void createKeyServerOperaMenu();

 private slots:

  /**
   * @details Export the key to a file, which is choosen in a file dialog
   */
  void slotExportPrivateKey();

  /**
   * @details Copy the fingerprint to clipboard
   */
  void slotCopyFingerprint();

  void slotModifyEditDatetime();

  void slotRefreshKeyInfo();

  void slotUploadKeyToServer();

  void slotUpdateKeyToServer();

  void slotGenRevokeCert();

 private:
  std::string keyid; /** The id of the key the details should be shown for */

  GpgKey mKey;

  QGroupBox* ownerBox;       /** Groupbox containing owner information */
  QGroupBox* keyBox;         /** Groupbox containing key information */
  QGroupBox* fingerprintBox; /** Groupbox containing fingerprint information */
  QGroupBox* additionalUidBox; /** Groupbox containing information about
                                  additional uids */

  QLabel* nameVarLabel;        /** Label containng the keys name */
  QLabel* emailVarLabel;       /** Label containng the keys email */
  QLabel* commentVarLabel;     /** Label containng the keys commment */
  QLabel* keySizeVarLabel;     /** Label containng the keys keysize */
  QLabel* expireVarLabel;      /** Label containng the keys expiration date */
  QLabel* createdVarLabel;     /** Label containng the keys creation date */
  QLabel* algorithmVarLabel;   /** Label containng the keys algorithm */
  QLabel* keyidVarLabel;       /** Label containng the keys keyid */
  QLabel* fingerPrintVarLabel; /** Label containng the keys fingerprint */
  QLabel* usageVarLabel;
  QLabel* actualUsageVarLabel;
  QLabel* masterKeyExistVarLabel;

  QMenu* keyServerOperaMenu;

 public:
  explicit KeyPairDetailTab(const std::string& key_id,
                            QWidget* parent = nullptr);
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_KEYPAIRDETAILTAB_H
