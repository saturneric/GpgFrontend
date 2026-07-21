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

class QCheckBox;
class QComboBox;
class QSpinBox;
class QLabel;

namespace GpgFrontend::UI {

/**
 * @brief Settings tab for the startup-time security and diagnostic knobs.
 *
 * These four settings are read once during startup, before the UI exists, so
 * every change here needs a restart to take effect — hence one tab rather than
 * four widgets scattered across the other pages.
 *
 * ENV.ini stays the deployment override: any key it pins is shown here
 * read-only, because accepting an edit that reverts on the next start would be
 * worse than refusing it.
 */
class AdvancedTab : public QWidget {
  Q_OBJECT

 public:
  explicit AdvancedTab(QWidget* parent = nullptr);

  /// Load current values from the global settings into the controls.
  void SetSettings();

  /// Persist the controls' values back to the global settings.
  void ApplySettings();

 signals:

  /**
   * @brief Emitted when a changed value only takes effect on the next start.
   *
   * A deep restart relaunches the process, which is what re-runs the ENV/user
   * settings resolution these knobs come from.
   */
  void SignalDeepRestartNeeded();

 private:
  /**
   * @brief Disable a control pinned by ENV.ini and explain why.
   *
   * @param widget control to lock
   * @param user_key settings key to test against GFEnvLockedKeys
   * @return true when the key is pinned by ENV.ini
   */
  auto lock_if_pinned(QWidget* widget, const QString& user_key) -> bool;

  QCheckBox* self_check_box_{};       ///< verify signed libraries at start
  QCheckBox* os_secret_store_box_{};  ///< wrap the app key with an OS secret
  QComboBox* secure_level_combo_{};   ///< app secure key protection level
  QComboBox* log_level_combo_{};      ///< minimum severity that gets logged
  QSpinBox* ring_capacity_spin_{};    ///< in-memory log ring buffer entries
  QLabel* env_notice_label_{};        ///< shown only when ENV.ini pins a key
  QStringList env_locked_keys_;       ///< keys ENV.ini currently overrides
};

}  // namespace GpgFrontend::UI
