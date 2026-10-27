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

namespace GpgFrontend::UI {

/**
 * @brief When a key expires: a short list of lifetimes, or a date.
 *
 * Choosing "Custom date" shows the date field beside the list. Every other
 * choice is counted from the moment it is read, so a wizard left open over
 * night still gives a key the whole lifetime it says.
 */
class GF_UI_EXPORT ExpiryPicker : public QWidget {
  Q_OBJECT

 public:
  explicit ExpiryPicker(QWidget* parent = nullptr);

  /// @brief Show an expiry; a lifetime when it is one, else the date.
  void SetExpiry(bool non_expired, const QDateTime& expire);

  /// @brief Show a stored validity ("2y", "forever", "90d").
  void SetValidity(const QString& validity);

  [[nodiscard]] auto NonExpired() const -> bool;

  /// @brief When the key expires; meaningless when NonExpired().
  [[nodiscard]] auto Expire() const -> QDateTime;

  /// @brief The choice as a stored validity.
  [[nodiscard]] auto Validity() const -> QString;

 signals:
  /**
   * @brief The user changed the choice.
   */
  void SignalChanged();

 private:
  QComboBox* combo_;
  QDateTimeEdit* date_;

  void sync_date_visibility();
};

}  // namespace GpgFrontend::UI
