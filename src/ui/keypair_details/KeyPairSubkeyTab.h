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

#ifndef GPGFRONTEND_KEYPAIRSUBKEYTAB_H
#define GPGFRONTEND_KEYPAIRSUBKEYTAB_H

#include "KeySetExpireDateDialog.h"
#include "gpg/GpgContext.h"
#include "ui/GpgFrontendUI.h"
#include "ui/keygen/SubkeyGenerateDialog.h"

namespace GpgFrontend::UI {

class KeyPairSubkeyTab : public QWidget {
  Q_OBJECT

 public:
  KeyPairSubkeyTab(const std::string& key, QWidget* parent);

 private:
  void createSubkeyList();

  void createSubkeyOperaMenu();

  const GpgSubKey& getSelectedSubkey();

  GpgKey key_;
  QTableWidget* subkeyList{};
  std::vector<GpgSubKey> buffered_subkeys;

  QGroupBox* listBox;
  QGroupBox* detailBox;

  QMenu* subkeyOperaMenu{};

  QLabel* keySizeVarLabel;     /** Label containing the keys key size */
  QLabel* expireVarLabel;      /** Label containing the keys expiration date */
  QLabel* createdVarLabel;     /** Label containing the keys creation date */
  QLabel* algorithmVarLabel;   /** Label containing the keys algorithm */
  QLabel* keyidVarLabel;       /** Label containing the keys keyid */
  QLabel* fingerPrintVarLabel; /** Label containing the keys fingerprint */
  QLabel* usageVarLabel;
  QLabel* masterKeyExistVarLabel;
  QLabel* cardKeyLabel;

 private slots:

  void slotAddSubkey();

  void slotRefreshSubkeyList();

  void slotRefreshSubkeyDetail();

  void slotEditSubkey();

  void slotRevokeSubkey();

  void slotRefreshKeyInfo();

 protected:
  void contextMenuEvent(QContextMenuEvent* event) override;
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_KEYPAIRSUBKEYTAB_H
