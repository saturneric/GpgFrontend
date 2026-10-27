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

namespace GpgFrontend::UI {

class ExpiryPicker;

/**
 * @brief Every choice that makes up one key or subkey, bound to its
 * parameters.
 *
 * The algorithm, its key length, the second half of a hybrid, what the key may
 * be used for and when it expires. Edits go straight into the bound
 * KeyGenerateInfo; Refresh() shows it again after something else changed it.
 *
 * The key generation wizard, the profile editor and the subkey dialog all use
 * this one, so the rules for which choices follow from which live in one
 * place.
 */
class GF_UI_EXPORT KeyAlgoEditor : public QWidget {
  Q_OBJECT

 public:
  /**
   * @param channel the key database, which decides what a hybrid pairs with
   * @param algos the algorithms on offer
   * @param parent parent widget
   */
  KeyAlgoEditor(int channel, QList<KeyAlgo> algos, QWidget* parent = nullptr);

  /// @brief Edit these parameters from now on, and show them.
  void SetInfo(QSharedPointer<KeyGenerateInfo> info);

  [[nodiscard]] auto Info() const -> QSharedPointer<KeyGenerateInfo>;

  /// @brief Show the bound parameters again.
  void Refresh();

  /// @brief Whether the expiry row is offered; not every engine can set one.
  void SetExpiryVisible(bool visible);

  /// @brief Whether the usage row is offered.
  void SetUsageVisible(bool visible);

  /// @brief Whether an algorithm is chosen in full, both halves of a hybrid.
  [[nodiscard]] auto IsComplete() const -> bool;

 signals:
  /**
   * @brief The user changed something.
   */
  void SignalChanged();

 private:
  int channel_;
  QList<KeyAlgo> algos_;
  QSharedPointer<KeyGenerateInfo> info_;
  bool refreshing_ = false;

  QFormLayout* form_;
  QComboBox* algo_combo_;
  QComboBox* length_combo_;
  QComboBox* second_algo_combo_;
  QComboBox* second_length_combo_;
  QWidget* usage_row_;
  QCheckBox* encr_check_;
  QCheckBox* sign_check_;
  QCheckBox* auth_check_;
  ExpiryPicker* expiry_;

  void set_row_visible(QWidget* field, bool visible);

  void changed();
};

}  // namespace GpgFrontend::UI
