/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

/**
 * @brief
 *
 */
#include "core/model/GpgKey.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

enum class GpgKeyTableColumn : unsigned int {
  kNone = 0,
  kType = 1 << 0,
  kName = 1 << 1,
  kEmailAddress = 1 << 2,
  kUsage = 1 << 3,
  kValidity = 1 << 4,
  kFingerPrint = 1 << 5,
  kKeyId = 1 << 6,
  kOwnerTrust = 1 << 7,
  kAll = ~0u
};

inline GpgKeyTableColumn operator|(GpgKeyTableColumn lhs,
                                   GpgKeyTableColumn rhs) {
  using T = std::underlying_type_t<GpgKeyTableColumn>;
  return static_cast<GpgKeyTableColumn>(static_cast<T>(lhs) |
                                        static_cast<T>(rhs));
}

inline GpgKeyTableColumn &operator|=(GpgKeyTableColumn &lhs,
                                     GpgKeyTableColumn rhs) {
  lhs = lhs | rhs;
  return lhs;
}

inline bool operator&(GpgKeyTableColumn lhs, GpgKeyTableColumn rhs) {
  using T = std::underlying_type_t<GpgKeyTableColumn>;
  return (static_cast<T>(lhs) & static_cast<T>(rhs)) != 0;
}

enum class GpgKeyTableDisplayMode : unsigned int {
  kNone = 0,
  kPublicKey = 1 << 0,
  kPrivateKey = 1 << 1,
  kFavorites = 1 << 2,
  kAll = ~0u
};

inline GpgKeyTableDisplayMode operator|(GpgKeyTableDisplayMode lhs,
                                        GpgKeyTableDisplayMode rhs) {
  using T = std::underlying_type_t<GpgKeyTableDisplayMode>;
  return static_cast<GpgKeyTableDisplayMode>(static_cast<T>(lhs) |
                                             static_cast<T>(rhs));
}

inline GpgKeyTableDisplayMode &operator|=(GpgKeyTableDisplayMode &lhs,
                                          GpgKeyTableDisplayMode rhs) {
  lhs = lhs | rhs;
  return lhs;
}

inline bool operator&(GpgKeyTableDisplayMode lhs, GpgKeyTableDisplayMode rhs) {
  using T = std::underlying_type_t<GpgKeyTableDisplayMode>;
  return (static_cast<T>(lhs) & static_cast<T>(rhs)) != 0;
}

class GPGFRONTEND_CORE_EXPORT GpgKeyTableModel : public QAbstractTableModel {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Gpg Key Table Model object
   *
   * @param keys
   * @param parent
   */
  explicit GpgKeyTableModel(GpgKeyList keys, QObject *parent = nullptr);

  /**
   * @brief
   *
   * @param parent
   * @return int
   */
  [[nodiscard]] auto rowCount(const QModelIndex &parent) const -> int override;

  /**
   * @brief
   *
   * @param parent
   * @return int
   */
  [[nodiscard]] auto columnCount(const QModelIndex &parent) const
      -> int override;

  /**
   * @brief
   *
   * @param index
   * @param role
   * @return QVariant
   */
  [[nodiscard]] auto data(const QModelIndex &index, int role) const
      -> QVariant override;

  /**
   * @brief
   *
   * @param section
   * @param orientation
   * @param role
   * @return QVariant
   */
  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role) const -> QVariant override;

  /**
   * @brief Set the Data object
   *
   * @param index
   * @param value
   * @param role
   * @return true
   * @return false
   */
  auto setData(const QModelIndex &index, const QVariant &value, int role)
      -> bool override;

  /**
   * @brief
   *
   * @param index
   * @return Qt::ItemFlags
   */
  [[nodiscard]] auto flags(const QModelIndex &index) const
      -> Qt::ItemFlags override;

  /**
   * @brief Get the All Key Ids object
   *
   * @return auto
   */
  auto GetAllKeyIds() -> GpgKeyIDList;

  /**
   * @brief Get the Key ID By Row object
   *
   * @return QString
   */
  [[nodiscard]] auto GetKeyIDByRow(int row) const -> QString;

  /**
   * @brief
   *
   * @param row
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsPrivateKeyByRow(int row) const -> bool;

 private:
  GpgKeyList buffered_keys_;
  QStringList column_headers_;
  QList<bool> key_check_state_;
};

}  // namespace GpgFrontend