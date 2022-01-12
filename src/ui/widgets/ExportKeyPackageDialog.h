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

#ifndef GPGFRONTEND_EXPORTKEYPACKAGEDIALOG_H
#define GPGFRONTEND_EXPORTKEYPACKAGEDIALOG_H

#include "GpgFrontendUI.h"

class Ui_exportKeyPackageDialog;

namespace GpgFrontend::UI {

class ExportKeyPackageDialog : public QDialog {
  Q_OBJECT

 public:
  explicit ExportKeyPackageDialog(KeyIdArgsListPtr key_ids, QWidget* parent);

  std::string generate_passphrase(int len);

 private:
  std::shared_ptr<Ui_exportKeyPackageDialog> ui;
  KeyIdArgsListPtr key_ids_;

  std::random_device rd;
  std::mt19937 mt;

  std::string passphrase_;

  void generate_key_package_name();
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_EXPORTKEYPACKAGEDIALOG_H
