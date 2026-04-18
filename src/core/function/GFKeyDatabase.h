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

#include <QSqlDatabase>

#include "core/typedef/GFTypedef.h"

namespace GpgFrontend {

class GF_CORE_EXPORT GFKeyDatabase {
 public:
  /**
   * @brief Construct a new GFKeyDatabase object
   *
   */
  GFKeyDatabase() = default;

  /**
   * @brief Destroy the GFKeyDatabase object
   *
   */
  ~GFKeyDatabase();

  /**
   * @brief
   *
   * @param path
   * @return true
   * @return false
   */
  auto Init(const QString& path) -> bool;

  /**
   * @brief Get the Metadata List object
   *
   * @return QList<GFKeyMetadata>
   */
  auto GetMetadataList() -> QList<GFKeyMetadata>;

  /**
   * @brief Get the Key Blocks object
   *
   * @param identifier
   * @return std::optional<GFKeyBlocks>
   */
  auto GetKeyBlocks(const QString& identifier) -> std::optional<GFKeyBlocks>;

  /**
   * @brief Get the Key object by identifier (can be either fingerprint or key
   * ID)
   *
   * @param identifier
   * @return std::optional<GFKey>
   */
  auto GetKeyByIdentifier(const QString& identifier) -> std::optional<GFKey>;

  /**
   * @brief
   *
   * @param metadata
   * @param blocks
   * @return true
   * @return false
   */
  auto SaveKey(const GFKeyMetadata& metadata, const GFKeyBlocks& blocks)
      -> bool;

  /**
   * @brief Get the Key object
   *
   * @param identifier
   * @return QString
   */
  auto GetKeyMetadata(const QString& identifier)
      -> std::optional<GFKeyMetadata>;

  /**
   * @brief Delete a key and its associated subkeys and blocks from the database
   *
   * @param fpr
   * @return true
   * @return false
   */
  auto DeleteKey(const QString& fpr) -> bool;

  /**
   * @brief
   *
   * @param identifier
   * @return QString
   */
  auto ResolvePrimaryFpr(const QString& identifier) -> QString;

 private:
  QSqlDatabase db_;  ///<

  /**
   * @brief
   *
   * @param path
   * @return true
   * @return false
   */
  auto connect_db(const QString& path) -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto create_table() -> bool;

  /**
   * @brief Load subkeys for a given primary key fingerprint
   *
   * @param db
   * @param parent_fpr
   * @return QList<GFSubKeyMetadata>
   */
  auto load_subkeys_for_parent(const QString& parent_fpr)
      -> QList<GFSubKeyMetadata>;

  /**
   * @brief
   *
   * @param fpr
   * @return QContainer<GFUserId>
   */
  auto load_user_ids_for_parent(const QString& fpr) -> QContainer<GFUserId>;
};

}  // namespace GpgFrontend