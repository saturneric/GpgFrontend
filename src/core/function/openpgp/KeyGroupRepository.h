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
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/model/GpgKeyGroup.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief A node in the key group dependency tree.
 *
 * Each node wraps a GpgKeyGroup and tracks its parent and child nodes in the
 * group hierarchy. A node may also hold individual GPG key IDs that belong
 * to this group but are not themselves key groups.
 */
struct GpgKeyGroupTreeNode {
  // Parent nodes in the key group hierarchy.
  QContainer<GpgKeyGroupTreeNode*> parents;
  // Child nodes in the key group hierarchy.
  QContainer<GpgKeyGroupTreeNode*> children;
  // The key group this node represents.
  QSharedPointer<GpgKeyGroup> key_group;

  // Individual key IDs that belong to this group but are not key groups.
  QStringList non_key_group_ids;
  // True if this key group is disabled (all its keys are unavailable).
  bool disabled;

  /**
   * @brief
   *
   */
  GpgKeyGroupTreeNode() = default;

  /**
   * @brief Construct a tree node for the given key group.
   *
   * @param kg key group to wrap
   */
  explicit GpgKeyGroupTreeNode(GpgKeyGroup kg);

  /**
   * @brief Resolve and apply the key group's member list from the current
   * keyring.
   */
  void Apply();

  /**
   * @brief Add an individual key (not a key group) to this node.
   *
   * @param key abstract key pointer to add
   * @return true if the key was added, false if it was already present
   */
  auto AddNonKeyGroupKey(const GpgAbstractKeyPtr& key) -> bool;

  /**
   * @brief Return whether @p target is an ancestor of this node in the
   * hierarchy.
   *
   * @param target node to test
   * @return true if @p target is a direct or indirect parent
   */
  auto HasAncestor(GpgKeyGroupTreeNode* target) -> bool;

  /**
   * @brief Add @p target as a child of this node.
   *
   * @param target child node to add
   * @return true if added, false if @p target is already a child or would
   * create a cycle
   */
  auto AddChildren(GpgKeyGroupTreeNode* target) -> bool;

  /**
   * @brief Remove @p target from this node's children.
   *
   * @param target child node to remove
   * @return true if the child was found and removed, false if not present
   */
  auto RemoveChildren(GpgKeyGroupTreeNode* target) -> bool;

  /**
   * @brief Remove an individual key from this node's non-group key list.
   *
   * @param key key ID string to remove
   * @return true if the key was found and removed
   */
  auto RemoveNonKeyGroupKey(const QString& key) -> bool;

  /**
   * @brief Return the combined list of all key IDs belonging to this node.
   *
   * Includes both individual keys and the resolved keys of child groups.
   *
   * @return list of key ID strings
   */
  [[nodiscard]] auto KeyIds() const -> QStringList;
};

/**
 * @brief Singleton repository for managing GPG key groups on a given channel.
 *
 * Key groups are named collections of keys (and other key groups). This
 * repository loads groups from the durable cache, maintains a dependency tree,
 * and persists changes back to the cache.
 */
class GF_CORE_EXPORT KeyGroupRepository
    : public SingletonFunctionObject<KeyGroupRepository> {
 public:
  /**
   * @brief Construct the repository for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit KeyGroupRepository(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Return all key groups known to this channel.
   *
   * @return list of shared pointers to all GpgKeyGroup objects
   */
  auto Fetch() -> QContainer<QSharedPointer<GpgKeyGroup>>;

  /**
   * @brief Flush the key group cache, reloading from persistent storage on the
   * next Fetch().
   *
   * @return true if the cache was flushed successfully
   */
  auto FlushCache() -> bool;

  /**
   * @brief Add a new key group to the repository and persist it.
   *
   * @param group key group to add
   */
  void AddKeyGroup(const GpgKeyGroup& group);

  /**
   * @brief Add a key to an existing key group.
   *
   * @param id key group identifier
   * @param key abstract key pointer to add
   * @return true if the key was added, false if the group was not found or the
   * key is already a member
   */
  auto AddKey2KeyGroup(const QString& id, const GpgAbstractKeyPtr& key) -> bool;

  /**
   * @brief Remove a key from an existing key group.
   *
   * @param id key group identifier
   * @param key_id key ID to remove
   * @return true if the key was removed, false if the group or key was not
   * found
   */
  auto RemoveKeyFromKeyGroup(const QString& id, const QString& key_id) -> bool;

  /**
   * @brief Delete the key group with the given identifier.
   *
   * @param id key group identifier to remove
   */
  void Remove(const QString& id);

  /**
   * @brief Return the key group with the given identifier.
   *
   * @param id key group identifier
   * @return shared pointer to the key group, or nullptr if not found
   */
  auto KeyGroup(const QString& id) -> QSharedPointer<GpgKeyGroup>;

  /**
   * @brief Return whether the key group with the given identifier is disabled.
   *
   * A group is disabled when none of its resolved keys are available.
   *
   * @param id key group identifier
   * @return true if the group is disabled
   */
  auto IsKeyGroupDisabled(const QString& id) -> bool;

 private:
  // OpenPGP context for resolving keys within groups.
  OpenPGPContext& ctx_ =
      OpenPGPContext::GetInstance(SingletonFunctionObject::GetChannel());
  // Cache manager used to persist and restore key group data.
  CacheManager& cm_ =
      CacheManager::GetInstance(SingletonFunctionObject::GetChannel());

  // Forest of key group tree nodes indexed by group ID.
  QMap<QString, QSharedPointer<GpgKeyGroupTreeNode>> key_groups_forest_;

  // Load key groups from the durable cache into key_groups_forest_.
  void fetch_key_groups();

  // Write key_groups_forest_ back to the durable cache.
  void persist_key_groups();

  // Validate all key groups and update their disabled state.
  void check_all_key_groups();

  // Validate a single key group node and update its disabled state.
  void check_key_group(const QSharedPointer<GpgKeyGroupTreeNode>& node);

  // Build the dependency tree across all key groups in key_groups_forest_.
  void build_gpg_key_group_tree();
};

}  // namespace GpgFrontend
