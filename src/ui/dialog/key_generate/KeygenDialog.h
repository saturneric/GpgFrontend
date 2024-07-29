/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <memory>

#include "core/model/GpgGenKeyInfo.h"
#include "core/utils/MemoryUtils.h"
#include "ui/GpgFrontendUI.h"
#include "ui/dialog/GeneralDialog.h"

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class KeyGenDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @details Constructor of this class
   *
   * @param ctx The current GpgME context
   * @param key The key to show details of
   * @param parent The parent of this widget
   */
  explicit KeyGenDialog(QWidget* parent = nullptr);

 signals:
  /**
   * @brief
   *
   */
  void SignalKeyGenerated();

 private:
  /**
   * @brief Create a key usage group box object
   *
   * @return QGroupBox*
   */
  QGroupBox* create_key_usage_group_box();

  /**
   * @brief Create a basic info group box object
   *
   * @return QGroupBox*
   */
  QGroupBox* create_basic_info_group_box();

  /**
   * @brief
   *
   */
  QRegularExpression re_email_{
      R"((?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\.){3}(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\]))"};

  /**
   * @brief
   *
   */
  QStringList error_messages_;  ///< List of errors occurring when checking
                                ///< entries of line edits

  std::shared_ptr<GenKeyInfo> gen_key_info_ =
      SecureCreateSharedObject<GenKeyInfo>();              ///<
  std::shared_ptr<GenKeyInfo> gen_subkey_info_ = nullptr;  ///<

  QDialogButtonBox* button_box_;     ///< Box for standard buttons
  QLabel* error_label_{};            ///< Label containing error message
  QLineEdit* name_edit_{};           ///< Line edit for the keys name
  QLineEdit* email_edit_{};          ///< Line edit for the keys email
  QLineEdit* comment_edit_{};        ///< Line edit for the keys comment
  QSpinBox* key_size_spin_box_{};    ///< Spinbox for the keys size (in bit)
  QComboBox* key_type_combo_box_{};  ///< Combobox for Key type
  QDateTimeEdit* date_edit_{};       ///< Date edit for expiration date
  QCheckBox* expire_check_box_{};    ///< Checkbox, if key should expire
  QCheckBox* no_pass_phrase_check_box_{};
  QGroupBox* key_usage_group_box_{};  ///< Group of Widgets detecting the usage
                                      ///< of the Key
  QDateTime max_date_time_;           ///<
  std::vector<QCheckBox*> key_usage_check_boxes_;  ///< ENCR, SIGN, CERT, AUTH

  /**
   * @brief
   *
   */
  void generate_key_dialog();

  /**
   * @details Refresh widgets state by GenKeyInfo
   */
  void refresh_widgets_state();

  /**
   * @brief Set the signal slot object
   *
   */
  void set_signal_slot();

  /**
   * @brief
   *
   * @param str
   * @return true
   * @return false
   */
  bool check_email_address(const QString& str);

 private slots:

  /**
   * @details when expirebox was checked/unchecked, enable/disable the
   * expiration date box
   */
  void slot_expire_box_changed();

  /**
   * @details check all lineedits for false entries. Show error, when there is
   * one, otherwise generate the key
   */
  void slot_key_gen_accept();

  /**
   * @brief
   *
   * @param state
   */
  void slot_encryption_box_changed(int state);

  /**
   * @brief
   *
   * @param state
   */
  void slot_signing_box_changed(int state);

  /**
   * @brief
   *
   * @param state
   */
  void slot_certification_box_changed(int state);

  /**
   * @brief
   *
   * @param state
   */
  void slot_authentication_box_changed(int state);

  /**
   * @brief
   *
   * @param index
   */
  void slot_activated_key_type(int index);
};

}  // namespace GpgFrontend::UI
