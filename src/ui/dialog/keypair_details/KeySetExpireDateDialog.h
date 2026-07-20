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

#include "core/function/openpgp/OpenPGPContext.h"
#include "core/model/GpgKey.h"
#include "core/typedef/GpgTypedef.h"
#include "ui/dialog/GeneralDialog.h"

namespace GpgFrontend::UI {

class KeySetExpireDateDialog : public GeneralDialog {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Key Set Expire Date Dialog object
   *
   * @param key_id
   * @param parent
   */
  explicit KeySetExpireDateDialog(int channel, GpgKeyPtr key_id,
                                  QWidget* parent = nullptr);

  /**
   * @brief Construct a new Key Set Expire Date Dialog object
   *
   * @param key_id
   * @param subkey_fpr
   * @param parent
   */
  explicit KeySetExpireDateDialog(int channel, GpgKeyPtr key,
                                  QString subkey_fpr,
                                  QWidget* parent = nullptr);

 signals:
  /**
   * @brief
   *
   */
  void SignalKeyExpireDateUpdated();

 private:
  /**
   * @brief A selectable validity period. "custom" and "never" carry no offset.
   *
   */
  struct ValidityPeriod {
    QString key;                           ///< stable, untranslated id
    QString display;                       ///< translated combo box entry
    std::function<QDateTime()> calc_time;  ///< null for "custom" and "never"
  };

  /**
   * @brief Build the widgets and wire them up.
   *
   */
  void init();

  /**
   * @brief Push chosen_date_time_ / non_expired_ into the widgets.
   *
   * The widgets are a view of these two members, never the storage. That is
   * what keeps toggling "Never Expires" from disturbing the picked date.
   */
  void refresh_widgets_state();

  /**
   * @brief The expiration currently stored in the key or subkey being edited.
   *
   */
  [[nodiscard]] auto current_expiration_time() const -> QDateTime;

  /**
   * @brief Human readable "1 year, 2 months" until a future point in time.
   *
   */
  [[nodiscard]] static auto humanize_remaining(const QDateTime& target)
      -> QString;

  QComboBox* validity_period_combo_box_{};  ///<
  QDateTimeEdit* expire_date_time_edit_{};  ///<
  QLabel* current_label_{};                 ///<
  QLabel* summary_label_{};                 ///<
  QDialogButtonBox* button_box_{};          ///<

  QDateTime chosen_date_time_;  ///< single source of truth for the date shown
  bool non_expired_{false};     ///< single source of truth for "never expires"

  int current_gpg_context_channel_;  ///<
  const GpgKeyPtr m_key_;            ///<
  const SubkeyId m_subkey_;          ///<

  const QContainer<ValidityPeriod> k_validity_periods_ = {
      {"custom", tr("Custom Date"), nullptr},
      {"3m", tr("3 Months"),
       [] { return QDateTime::currentDateTime().addMonths(3); }},
      {"6m", tr("6 Months"),
       [] { return QDateTime::currentDateTime().addMonths(6); }},
      {"1y", tr("1 Year"),
       [] { return QDateTime::currentDateTime().addYears(1); }},
      {"2y", tr("2 Years"),
       [] { return QDateTime::currentDateTime().addYears(2); }},
      {"5y", tr("5 Years"),
       [] { return QDateTime::currentDateTime().addYears(5); }},
      {"10y", tr("10 Years"),
       [] { return QDateTime::currentDateTime().addYears(10); }},
      {"never", tr("Never Expires"), nullptr},
  };

 private slots:
  /**
   * @brief
   *
   */
  void slot_confirm();

  /**
   * @brief
   *
   * @param index
   */
  void slot_validity_period_changed(int index);

  /**
   * @brief
   *
   * @param date_time
   */
  void slot_expire_date_time_edited(const QDateTime& date_time);
};

}  // namespace GpgFrontend::UI
