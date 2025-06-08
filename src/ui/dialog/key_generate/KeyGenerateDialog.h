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

#include "core/model/GpgKeyGenerateInfo.h"
#include "ui/dialog/GeneralDialog.h"

class Ui_KeyGenDialog;

namespace GpgFrontend::UI {

/**
 * @brief
 *
 */
class KeyGenerateDialog : public GeneralDialog {
  Q_OBJECT

 public:
  struct EasyModeConf {
    QString name;
    QString key_algo;
    QString key_validity;

    bool has_s_key;
    QString s_key_algo;
    QString s_key_validity;

    bool hidden;
  };

  /**
   * @details Constructor of this class
   *
   * @param ctx The current GpgME context
   * @param key The key to show details of
   * @param parent The parent of this widget
   */
  explicit KeyGenerateDialog(int channel, QWidget* parent = nullptr);

 signals:
  /**
   * @brief
   *
   */
  void SignalKeyGenerated();

 private slots:

  /**
   * @details check all lineedits for false entries. Show error, when there
   * is one, otherwise generate the key
   */
  void slot_key_gen_accept();

  /**
   * @brief
   *
   * @param mode
   */
  void slot_easy_mode_changed(int index);

  /**
   * @brief
   *
   * @param mode
   */
  void slot_easy_valid_date_changed(const QString& mode);

  /**
   * @brief
   *
   */
  void slot_set_easy_valid_date_2_custom();

  /**
   * @brief
   *
   */
  void slot_set_easy_key_algo_2_custom();

  /**
   * @brief
   *
   * @param mode
   */
  void slot_easy_combination_changed(const QString& mode);

 private:
  int channel_;

  QStringList error_messages_;  ///< List of errors occurring when checking
                                ///< entries of line edits

  QSharedPointer<Ui_KeyGenDialog> ui_;
  QSharedPointer<KeyGenerateInfo> gen_key_info_;     ///<
  QSharedPointer<KeyGenerateInfo> gen_subkey_info_;  ///<

  QContainer<KeyAlgo> supported_primary_key_algos_;
  QContainer<KeyAlgo> supported_subkey_algos_;

  QMap<int, EasyModeConf> easy_mode_conf_;

  /**
   * @details Refresh widgets state by GenKeyInfo
   */
  void refresh_widgets_state();

  /**
   * @brief Set the signal slot object
   *
   */
  void set_signal_slot_config();

  /**
   * @brief
   *
   */
  void sync_gen_key_algo_info();

  /**
   * @brief
   *
   */
  void sync_gen_subkey_algo_info();

  /**
   * @brief
   *
   */
  void create_sync_gen_subkey_info();

  /**
   * @brief
   *
   */
  void do_generate();

  /**
   * @brief
   *
   */
  void load_easy_mode_config();
};

}  // namespace GpgFrontend::UI
