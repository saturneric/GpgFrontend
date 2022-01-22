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

#ifndef GPGFRONTEND_KEYSETEXPIREDATEDIALOG_H
#define GPGFRONTEND_KEYSETEXPIREDATEDIALOG_H

#include "gpg/GpgContext.h"
#include "gpg/model/GpgKey.h"
#include "gpg/model/GpgSubKey.h"
#include "ui/GpgFrontendUI.h"

class Ui_ModifiedExpirationDateTime;

namespace GpgFrontend::UI {

class KeySetExpireDateDialog : public QDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Key Set Expire Date Dialog object
   *
   * @param key_id
   * @param parent
   */
  explicit KeySetExpireDateDialog(const KeyId& key_id,
                                  QWidget* parent = nullptr);

  /**
   * @brief Construct a new Key Set Expire Date Dialog object
   *
   * @param key_id
   * @param subkey_fpr
   * @param parent
   */
  explicit KeySetExpireDateDialog(const KeyId& key_id, std::string subkey_fpr,
                                  QWidget* parent = nullptr);

 signals:
  /**
   * @brief
   *
   */
  void SignalKeyExpireDateUpdated();

 private:
  /**
   * @brief
   *
   */
  void init();

  std::shared_ptr<Ui_ModifiedExpirationDateTime> ui_;  ///<
  const GpgKey m_key_;                                 ///<
  const SubkeyId m_subkey_;                            ///<

 private slots:
  /**
   * @brief
   *
   */
  void slot_confirm();

  /**
   * @brief
   *
   * @param state
   */
  void slot_non_expired_checked(int state);
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_KEYSETEXPIREDATEDIALOG_H
