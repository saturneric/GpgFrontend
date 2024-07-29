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
  kNONE = 0,
  kTYPE = 1 << 0,
  kNAME = 1 << 1,
  kEMAIL_ADDRESS = 1 << 2,
  kUSAGE = 1 << 3,
  kKEY_ID = 1 << 4,
  kOWNER_TRUST = 1 << 5,
  kCREATE_DATE = 1 << 6,
  kALGO = 1 << 7,
  kSUBKEYS_NUMBER = 1 << 8,
  kCOMMENT = 1 << 9,
  kALL = ~0U
};

inline auto operator|(GpgKeyTableColumn lhs,
                      GpgKeyTableColumn rhs) -> GpgKeyTableColumn {
  using T = std::underlying_type_t<GpgKeyTableColumn>;
  return static_cast<GpgKeyTableColumn>(static_cast<T>(lhs) |
                                        static_cast<T>(rhs));
}

inline auto operator|=(GpgKeyTableColumn &lhs,
                       GpgKeyTableColumn rhs) -> GpgKeyTableColumn & {
  lhs = lhs | rhs;
  return lhs;
}

inline auto operator&(GpgKeyTableColumn lhs,
                      GpgKeyTableColumn rhs) -> GpgKeyTableColumn {
  using T = std::underlying_type_t<GpgKeyTableColumn>;
  return static_cast<GpgKeyTableColumn>(static_cast<T>(lhs) &
                                        static_cast<T>(rhs));
}

inline auto operator&=(GpgKeyTableColumn &lhs,
                       GpgKeyTableColumn rhs) -> GpgKeyTableColumn & {
  lhs = lhs & rhs;
  return lhs;
}

inline auto operator~(GpgKeyTableColumn hs) -> GpgKeyTableColumn {
  using T = std::underlying_type_t<GpgKeyTableColumn>;
  return static_cast<GpgKeyTableColumn>(~static_cast<T>(hs));
}

enum class GpgKeyTableDisplayMode : unsigned int {
  kNONE = 0,
  kPUBLIC_KEY = 1 << 0,
  kPRIVATE_KEY = 1 << 1,
  kFAVORITES = 1 << 2,
  kALL = ~0U
};

inline auto operator|(GpgKeyTableDisplayMode lhs,
                      GpgKeyTableDisplayMode rhs) -> GpgKeyTableDisplayMode {
  using T = std::underlying_type_t<GpgKeyTableDisplayMode>;
  return static_cast<GpgKeyTableDisplayMode>(static_cast<T>(lhs) |
                                             static_cast<T>(rhs));
}

inline auto operator|=(GpgKeyTableDisplayMode &lhs,
                       GpgKeyTableDisplayMode rhs) -> GpgKeyTableDisplayMode & {
  lhs = lhs | rhs;
  return lhs;
}

inline auto operator&(GpgKeyTableDisplayMode lhs,
                      GpgKeyTableDisplayMode rhs) -> bool {
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
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;

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
  auto setData(const QModelIndex &index, const QVariant &value,
               int role) -> bool override;

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
#ifdef QT5_BUILD
  QVector<bool> key_check_state_;
#else
  QList<bool> key_check_state_;
#endif
};

}  // namespace GpgFrontend