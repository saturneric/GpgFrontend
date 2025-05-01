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

#include "core/function/CacheManager.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/function/gpg/GpgContext.h"
#include "core/model/GpgKeyGroup.h"

namespace GpgFrontend {

struct GpgKeyGroupTreeNode {
  QContainer<GpgKeyGroupTreeNode*> parents;
  QContainer<GpgKeyGroupTreeNode*> children;
  QSharedPointer<GpgKeyGroup> key_group;

  // over take
  QStringList non_key_group_ids;
  bool disabled;

  /**
   * @brief Construct a new Gpg Key Group Tree Node object
   *
   */
  GpgKeyGroupTreeNode() = default;

  /**
   * @brief Construct a new Gpg Key Group Tree Node object
   *
   * @param kg
   */
  explicit GpgKeyGroupTreeNode(GpgKeyGroup kg);

  /**
   * @brief
   *
   */
  void Apply();

  /**
   * @brief
   *
   * @param key
   * @return true
   * @return false
   */
  auto AddNonKeyGroupKey(const GpgAbstractKeyPtr& key) -> bool;

  /**
   * @brief
   *
   * @param target
   * @return true
   * @return false
   */
  auto HasAncestor(GpgKeyGroupTreeNode* target) -> bool;

  /**
   * @brief
   *
   * @param target
   * @return true
   * @return false
   */
  auto AddChildren(GpgKeyGroupTreeNode* target) -> bool;

  /**
   * @brief
   *
   * @param target
   * @return true
   * @return false
   */
  auto RemoveChildren(GpgKeyGroupTreeNode* target) -> bool;

  /**
   * @brief
   *
   * @param key
   * @return true
   * @return false
   */
  auto RemoveNonKeyGroupKey(const QString& key) -> bool;

  /**
   * @brief
   *
   * @return QStringList
   */
  [[nodiscard]] auto KeyIds() const -> QStringList;
};

class GF_CORE_EXPORT GpgKeyGroupGetter
    : public SingletonFunctionObject<GpgKeyGroupGetter> {
 public:
  /**
   * @brief GpgKeyGroupGetter constructor
   *
   * @param channel channel
   */
  explicit GpgKeyGroupGetter(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief
   *
   * @return QContainer<GpgKeyGroup>
   */
  auto Fetch() -> QContainer<QSharedPointer<GpgKeyGroup>>;

  /**
   * @brief
   *
   * @return QContainer<GpgKeyGroup>
   */
  auto FlushCache() -> bool;

  /**
   * @brief
   *
   */
  void AddKeyGroup(const GpgKeyGroup&);

  /**
   * @brief
   *
   * @param id
   * @param key
   * @return true
   * @return false
   */
  auto AddKey2KeyGroup(const QString& id, const GpgAbstractKeyPtr& key) -> bool;

  /**
   * @brief
   *
   * @param id
   * @param key
   * @return true
   * @return false
   */
  auto RemoveKeyFromKeyGroup(const QString& id, const QString& key_id) -> bool;

  /**
   * @brief
   *
   * @param id
   */
  void Remove(const QString& id);

  /**
   * @brief
   *
   * @param id
   */
  auto KeyGroup(const QString& id) -> QSharedPointer<GpgKeyGroup>;

  /**
   * @brief
   *
   * @param id
   */
  auto IsKeyGroupDisabled(const QString& id) -> bool;

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());
  CacheManager& cm_ =
      CacheManager::GetInstance(SingletonFunctionObject::GetChannel());

  QMap<QString, QSharedPointer<GpgKeyGroupTreeNode>> key_groups_forest_;

  /**
   * @brief
   *
   */
  void fetch_key_groups();

  /**
   * @brief
   *
   */
  void persist_key_groups();

  /**
   * @brief
   *
   */
  void check_all_key_groups();

  /**
   * @brief
   *
   */
  void check_key_group(const QSharedPointer<GpgKeyGroupTreeNode>&);

  /**
   * @brief
   *
   */
  void build_gpg_key_group_tree();
};

}  // namespace GpgFrontend
