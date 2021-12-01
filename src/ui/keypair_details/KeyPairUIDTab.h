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

#ifndef GPGFRONTEND_KEYPAIRUIDTAB_H
#define GPGFRONTEND_KEYPAIRUIDTAB_H

#include "KeyNewUIDDialog.h"
#include "KeyUIDSignDialog.h"
#include "gpg/GpgContext.h"
#include "ui/GpgFrontendUI.h"

namespace GpgFrontend::UI {

class KeyPairUIDTab : public QWidget {
  Q_OBJECT

 public:
  KeyPairUIDTab(const std::string& key_id, QWidget* parent);

 signals:
  void signalUpdateUIDInfo();

 private:
  GpgKey mKey;
  QTableWidget* uidList{};
  QTableWidget* sigList{};
  QMenu* manageSelectedUIDMenu{};
  QMenu* uidPopupMenu{};
  QMenu* signPopupMenu{};
  std::vector<GpgUID> buffered_uids;
  std::vector<GpgKeySignature> buffered_signatures;

  void createUIDList();

  void createSignList();

  void createManageUIDMenu();

  void createUIDPopupMenu();

  void createSignPopupMenu();

  UIDArgsListPtr getUIDChecked();

  UIDArgsListPtr getUIDSelected();

  SignIdArgsListPtr getSignSelected();

 private slots:

  void slotRefreshUIDList();

  void slotRefreshSigList();

  void slotAddSign();

  void slotAddSignSingle();

  void slotAddUID();

  void slotDelUID();

  void slotDelUIDSingle();

  void slotSetPrimaryUID();

  void slotDelSign();

  void slotRefreshKey();

  static void slotAddUIDResult(int result);

 protected:
  void contextMenuEvent(QContextMenuEvent* event) override;
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_KEYPAIRUIDTAB_H
