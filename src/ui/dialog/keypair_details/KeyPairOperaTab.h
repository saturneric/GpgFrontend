/**
 * Copyright (C) 2021 Saturneric
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#ifndef GPGFRONTEND_KEYPAIROPERATAB_H
#define GPGFRONTEND_KEYPAIROPERATAB_H

#include "core/function/gpg/GpgKeyGetter.h"
#include "ui/GpgFrontendUI.h"

namespace GpgFrontend::UI {
class KeyPairOperaTab : public QWidget {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Key Pair Opera Tab object
   *
   * @param key_id
   * @param parent
   */
  KeyPairOperaTab(const std::string& key_id, QWidget* parent);

  /**
   * @brief Create a Opera Menu object
   *
   */
  void CreateOperaMenu();

 signals:
  /**
   * @brief
   *
   */
  void SignalKeyDatabaseRefresh();

 private slots:

  /**
   * @details Export the key to a file, which is chosen in a file dialog
   */
  void slot_export_private_key();

  /**
   * @brief
   *
   */
  void slot_export_short_private_key();

  /**
   * @brief
   *
   */
  void slot_export_public_key();

  /**
   * @brief
   *
   */
  void slot_modify_edit_datetime();

  /**
   * @brief
   *
   */
  void slot_modify_password();

  /**
   * @brief
   *
   */
  void slot_upload_key_to_server();

  /**
   * @brief
   *
   */
  void slot_update_key_from_server();

  /**
   * @brief
   *
   */
  void slot_gen_revoke_cert();

  /**
   * @brief
   *
   */
  void slot_modify_tofu_policy();

  /**
   * @brief
   *
   */
  void slot_set_owner_trust_level();

 private:
  GpgKey m_key_;                           ///<
  QMenu* key_server_opera_menu_{};         ///<
  QMenu* secret_key_export_opera_menu_{};  ///<
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_KEYPAIROPERATAB_H
