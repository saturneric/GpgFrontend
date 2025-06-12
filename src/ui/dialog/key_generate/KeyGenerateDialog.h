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
  void slot_easy_profile_changed(int index);

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

  /**
   * @brief
   *
   */
  void slot_save_as_easy_profile_config();

  /**
   * @brief
   *
   */
  void slot_delete_easy_profile_config();

 private:
  struct ExpireOption {
    QString key;      // "2y"
    QString display;  // tr("2 Years")
    bool non_expired;
    std::function<QDateTime()> calc_expire_time;
  };

  const ExpireOption k_default_expire_option_ = {
      "2y", tr("2 Years"), false,
      [] { return QDateTime::currentDateTime().addYears(2); }};

  const ExpireOption k_custom_expire_option_ = {
      "custom", tr("Custom"), false,
      [] { return QDateTime::currentDateTime(); }};

  const QMap<QString, ExpireOption> k_expire_options_ = {
      {"custom", k_custom_expire_option_},
      {"3m",
       {"3m", tr("3 Months"), false,
        [] { return QDateTime::currentDateTime().addMonths(3); }}},
      {"6m",
       {"6m", tr("6 Months"), false,
        [] { return QDateTime::currentDateTime().addMonths(6); }}},
      {"1y",
       {"1y", tr("1 Year"), false,
        [] { return QDateTime::currentDateTime().addYears(1); }}},
      {"2y", k_default_expire_option_},
      {"5y",
       {"5y", tr("5 Years"), false,
        [] { return QDateTime::currentDateTime().addYears(5); }}},
      {"10y",
       {"10y", tr("10 Years"), false,
        [] { return QDateTime::currentDateTime().addYears(10); }}},
      {"forever",
       {"forever", tr("Non Expired"), true,
        [] { return QDateTime::currentDateTime(); }}},
  };

  const QList<ExpireOption> k_expire_options_list_ = {
      k_expire_options_.value("custom"), k_expire_options_.value("3m"),
      k_expire_options_.value("6m"),     k_expire_options_.value("1y"),
      k_expire_options_.value("2y"),     k_expire_options_.value("5y"),
      k_expire_options_.value("10y"),    k_expire_options_.value("forever"),
  };

  int channel_;
  QStringList error_messages_;  ///< List of errors occurring when checking
                                ///< entries of line edits

  QSharedPointer<Ui_KeyGenDialog> ui_;
  QSharedPointer<KeyGenerateInfo> gen_key_info_;     ///<
  QSharedPointer<KeyGenerateInfo> gen_subkey_info_;  ///<

  QContainer<KeyAlgo> supported_primary_key_algos_;
  QContainer<KeyAlgo> supported_subkey_algos_;

  QContainer<EasyModeConf> easy_mode_conf_;
  QMap<int, EasyModeConf> easy_profile_conf_index_;

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
  void load_easy_profile_config();

  /**
   * @brief
   *
   */
  void flush_easy_profile_config_cache();
};

}  // namespace GpgFrontend::UI
