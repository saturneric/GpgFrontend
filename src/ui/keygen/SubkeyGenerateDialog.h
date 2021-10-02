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

#ifndef GPGFRONTEND_SUBKEYGENERATEDIALOG_H
#define GPGFRONTEND_SUBKEYGENERATEDIALOG_H

#include <gpg/GpgGenKeyInfo.h>
#include "gpg/GpgContext.h"
#include "ui/GpgFrontendUI.h"

namespace GpgFrontend::UI {

class SubkeyGenerateDialog : public QDialog {
  Q_OBJECT

 public:
  explicit SubkeyGenerateDialog(const KeyId& key_id, QWidget* parent);

 private:
  GpgKey mKey;

  std::unique_ptr<GenKeyInfo> genKeyInfo = std::make_unique<GenKeyInfo>();

  QGroupBox* keyUsageGroupBox{};
  QDialogButtonBox* buttonBox;  /** Box for standardbuttons */
  QLabel* errorLabel{};         /** Label containing error message */
  QSpinBox* keySizeSpinBox{};   /** Spinbox for the keys size (in bit) */
  QComboBox* keyTypeComboBox{}; /** Combobox for Keytpe */
  QDateTimeEdit* dateEdit{};    /** Dateedit for expiration date */
  QCheckBox* expireCheckBox{};  /** Checkbox, if key should expire */

  // ENCR, SIGN, CERT, AUTH
  std::vector<QCheckBox*> keyUsageCheckBoxes;

  QGroupBox* create_key_usage_group_box();

  QGroupBox* create_basic_info_group_box();

  void set_signal_slot();

  /**
   * @details Refresh widgets state by GenKeyInfo
   */
  void refresh_widgets_state();

 private slots:

  /**
   * @details when expirebox was checked/unchecked, enable/disable the
   * expiration date box
   */
  void slotExpireBoxChanged();

  /**
   * @details check all lineedits for false entries. Show error, when there is
   * one, otherwise generate the key
   */
  void slotKeyGenAccept();

  void slotEncryptionBoxChanged(int state);

  void slotSigningBoxChanged(int state);

  void slotCertificationBoxChanged(int state);

  void slotAuthenticationBoxChanged(int state);

  void slotActivatedKeyType(int index);
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SUBKEYGENERATEDIALOG_H
