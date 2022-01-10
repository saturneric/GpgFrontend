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

#ifndef GPGFRONTEND_RECEIVEMAILDIALOG_H
#define GPGFRONTEND_RECEIVEMAILDIALOG_H

#include "ui/GpgFrontendUI.h"

class Ui_ReceiveMailDialog;

namespace vmime::net {
class folder;
};

namespace GpgFrontend::UI {

class IMAPFolder;

class ReceiveMailDialog : public QDialog {
  Q_OBJECT
 public:
  explicit ReceiveMailDialog(QWidget* parent);

 private slots:
  void slotRefreshData();

 private:
  std::shared_ptr<Ui_ReceiveMailDialog> ui;

  void list_sub_folders(IMAPFolder* parent_folder,
                        const std::shared_ptr<vmime::net::folder>&);

  std::vector<std::shared_ptr<IMAPFolder>> folders;
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_RECEIVEMAILDIALOG_H
