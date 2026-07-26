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

namespace GpgFrontend {

/**
 * @brief The SettingsObject class
 * This class is used to store data for the application securely.
 *
 */
class GF_CORE_EXPORT SettingsObject : public QJsonObject {
 public:
  /**
   * @brief Construct a new Settings Object object
   *
   * @param settings_name The name of the settings object
   */
  explicit SettingsObject(QString settings_name);

  /**
   * @brief Construct a new Settings Object object
   *
   * @param _sub_json
   */
  explicit SettingsObject(QJsonObject sub_json);

  /**
   * @brief Destroy the Settings Object object
   *
   */
  ~SettingsObject();

  /**
   * @brief Replace the held content.
   *
   * Does nothing when the object failed to load -- see LoadFailed().
   *
   * @return false when the store was refused
   */
  auto Store(const QJsonObject&) -> bool;

  /**
   * @brief Replace the held content even though the load failed.
   *
   * The refusal above protects data that may still be recoverable, but it also
   * blocks the one screen a user would use to repair a broken profile. This is
   * the deliberate way out: only for an explicit, user-confirmed reset of this
   * one object, never as a fallback when Store() returns false.
   */
  void StoreOverridingUnreadable(const QJsonObject&);

  /**
   * @brief Whether a stored object exists but could not be read at load time.
   *
   * True means the settings are on disk, encrypted under a key this session
   * does not have. The in-memory object is empty, but that emptiness is an
   * artefact of the failure, not the user's data -- so it must never be
   * written back.
   */
  [[nodiscard]] auto LoadFailed() const -> bool { return load_failed_; }

 private:
  QString settings_name_;  ///<
  QJsonObject
      original_;  ///< snapshot at load time; write only if *this differs
  bool load_failed_ =
      false;  ///< stored object exists but was unreadable; never overwrite
  bool override_load_failure_ =
      false;  ///< user explicitly chose to reset the unreadable object
};
}  // namespace GpgFrontend
