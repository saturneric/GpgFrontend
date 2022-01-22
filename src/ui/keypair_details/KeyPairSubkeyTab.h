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
  /**
   * @brief Construct a new Key Pair Subkey Tab object
   *
   * @param key
   * @param parent
   */
  KeyPairSubkeyTab(const std::string& key, QWidget* parent);

 private:
  /**
   * @brief Create a subkey list object
   *
   */
  void create_subkey_list();

  /**
   * @brief Create a subkey opera menu object
   *
   */
  void create_subkey_opera_menu();

  /**
   * @brief Get the selected subkey object
   *
   * @return const GpgSubKey&
   */
  const GpgSubKey& get_selected_subkey();

  GpgKey key_;                               ///<
  QTableWidget* subkey_list_{};              ///<
  std::vector<GpgSubKey> buffered_subkeys_;  ///<

  QGroupBox* list_box_;    ///<
  QGroupBox* detail_box_;  ///<

  QMenu* subkey_opera_menu_{};  ///<

  QLabel* key_size_var_label_;   ///< Label containing the keys key size
  QLabel* expire_var_label_;     ///< Label containing the keys expiration date
  QLabel* created_var_label_;    ///< Label containing the keys creation date
  QLabel* algorithm_var_label_;  ///< Label containing the keys algorithm
  QLabel* key_id_var_label_;     ///< Label containing the keys keyid
  QLabel* fingerprint_var_label_;  ///< Label containing the keys fingerprint
  QLabel* usage_var_label_;        ///<
  QLabel* master_key_exist_var_label_;  ///<
  QLabel* card_key_label_;              ///<

 private slots:

  /**
   * @brief
   *
   */
  void slot_add_subkey();

  /**
   * @brief
   *
   */
  void slot_refresh_subkey_list();

  /**
   * @brief
   *
   */
  void slot_refresh_subkey_detail();

  /**
   * @brief
   *
   */
  void slot_edit_subkey();

  /**
   * @brief
   *
   */
  void slot_revoke_subkey();

  /**
   * @brief
   *
   */
  void slot_refresh_key_info();

 protected:
  /**
   * @brief
   *
   * @param event
   */
  void contextMenuEvent(QContextMenuEvent* event) override;
};

}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_KEYPAIRSUBKEYTAB_H
