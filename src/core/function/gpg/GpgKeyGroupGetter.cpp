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

#include "GpgKeyGroupGetter.h"

#include "core/function/CacheManager.h"
#include "core/function/gpg/GpgAbstractKeyGetter.h"
#include "core/struct/cache_object/KeyGroupsCO.h"
#include "utils/GpgUtils.h"

namespace GpgFrontend {

GpgKeyGroupGetter::GpgKeyGroupGetter(int channel)
    : SingletonFunctionObject<GpgKeyGroupGetter>(channel) {
  fetch_key_groups();
}

auto GpgKeyGroupGetter::Fetch() -> QContainer<QSharedPointer<GpgKeyGroup>> {
  QContainer<QSharedPointer<GpgKeyGroup>> ret;
  for (const auto& node : key_groups_forest_) {
    ret.push_back(node->key_group);
  }
  return ret;
}

void GpgKeyGroupGetter::Remove(const QString& id) {
  if (id.isEmpty() || !key_groups_forest_.contains(id)) return;

  auto target_node = key_groups_forest_.value(id);

  for (const auto& node : key_groups_forest_) {
    if (target_node == node) continue;
    node->RemoveChildren(target_node.get());
  }

  key_groups_forest_.remove(id);
  FlushCache();
}

void GpgKeyGroupGetter::fetch_key_groups() {
  key_groups_forest_.clear();
  auto key = QString("kgs:%1").arg(ctx_.KeyDBName());
  auto json = cm_.LoadDurableCache(key);

  auto key_groups = KeyGroupsCO(json.object());
  if (key_groups.key_db_name != ctx_.KeyDBName()) return;

  for (const auto& key_group : key_groups.key_groups) {
    if (key_group.id.isEmpty()) continue;

    LOG_D() << "load raw key group:" << key_group.id
            << "key ids: " << key_group.key_ids;

    auto node =
        QSharedPointer<GpgKeyGroupTreeNode>::create(GpgKeyGroup{key_group});
    node->key_group->SetKeyGroupGetter(this);
    key_groups_forest_.insert(key_group.id, node);
  }

  build_gpg_key_group_tree();
  check_all_key_groups();
}

void GpgKeyGroupGetter::persist_key_groups() {
  auto key = QString("kgs:%1").arg(ctx_.KeyDBName());

  KeyGroupsCO key_groups;
  key_groups.key_db_name = ctx_.KeyDBName();

  for (const auto& node : key_groups_forest_) {
    if (node->key_group == nullptr) continue;
    LOG_D() << "persist key group: " << node->key_group->ID()
            << "key ids:" << node->key_group->KeyIds();
    key_groups.key_groups.push_back(node->key_group->ToCacheObject());
  }

  cm_.SaveDurableCache(key, QJsonDocument{key_groups.ToJson()}, true);
}

auto GpgKeyGroupGetter::KeyGroup(const QString& id)
    -> QSharedPointer<GpgKeyGroup> {
  if (!key_groups_forest_.contains(id)) return nullptr;
  return key_groups_forest_.value(id)->key_group;
}

void GpgKeyGroupGetter::check_key_group(
    const QSharedPointer<GpgKeyGroupTreeNode>& node) {
  if (node == nullptr || node->disabled) return;

  for (const auto& key_id : node->KeyIds()) {
    assert(key_id != node->key_group->ID());
    if (key_id == node->key_group->ID()) continue;

    if (IsKeyGroupID(key_id)) {
      if (!key_groups_forest_.contains(key_id)) {
        return;
      }

      auto s_node = key_groups_forest_.value(key_id, nullptr);
      check_key_group(s_node);

      if (s_node->key_group->IsDisabled()) {
        node->disabled = true;
        return;
      }

    } else {
      auto key = GpgKeyGetter::GetInstance(GetChannel()).GetKeyPtr(key_id);
      if (key == nullptr || !key->IsHasEncrCap()) {
        node->disabled = true;
        return;
      }
    }
  }
}

void GpgKeyGroupGetter::check_all_key_groups() {
  for (const auto& node : key_groups_forest_) node->disabled = false;

  for (const auto& node : key_groups_forest_) {
    check_key_group(node);

    assert(node->key_group != nullptr);
    LOG_D() << "key group" << node->key_group->ID()
            << "ids: " << node->key_group->KeyIds()
            << "status: " << node->disabled;
  }
}

auto GpgKeyGroupGetter::FlushCache() -> bool {
  for (const auto& node : key_groups_forest_.values()) {
    node->Apply();
  }

  check_all_key_groups();
  persist_key_groups();
  return true;
}

void GpgKeyGroupGetter::build_gpg_key_group_tree() {
  for (const auto& node : key_groups_forest_) {
    LOG_D() << "load key group: " << node->key_group->ID()
            << "ids: " << node->key_group->KeyIds();
    for (const auto& key_id : node->key_group->KeyIds()) {
      if (!IsKeyGroupID(key_id) || !key_groups_forest_.contains(key_id)) {
        continue;
      }

      auto target = key_groups_forest_.value(key_id);
      if (!node->AddChildren(target.get())) {
        LOG_E() << "found ring in key groups relations, key group:"
                << node->key_group->ID() << "child: " << key_id;
        continue;
      }
    }
    node->Apply();
  }
}

void GpgKeyGroupGetter::AddKeyGroup(const GpgKeyGroup& key_group) {
  auto node = QSharedPointer<GpgKeyGroupTreeNode>::create(key_group);
  node->key_group->SetKeyGroupGetter(this);

  key_groups_forest_.insert(node->key_group->ID(), node);
  LOG_D() << "add new key group" << key_group.ID()
          << "key ids:" << key_group.KeyIds();

  for (const auto& key_id : node->key_group->KeyIds()) {
    if (!IsKeyGroupID(key_id) || !key_groups_forest_.contains(key_id)) continue;

    auto target = key_groups_forest_.value(key_id);
    node->AddChildren(target.get());
  }
  node->Apply();
  FlushCache();
}

auto GpgKeyGroupGetter::AddKey2KeyGroup(const QString& id,
                                        const GpgAbstractKeyPtr& key) -> bool {
  if (!key_groups_forest_.contains(id)) return false;
  auto key_group = key_groups_forest_.value(id);

  if (key->KeyType() != GpgAbstractKeyType::kGPG_KEYGROUP) {
    auto ret = key_group->AddNonKeyGroupKey(key);
    FlushCache();
    return ret;
  }

  if (!key_groups_forest_.contains(key->ID())) {
    LOG_E() << "try adding invalid key group id:" << key->ID();
    return false;
  }

  auto s_key_group = key_groups_forest_.value(key->ID());
  auto ret = key_group->AddChildren(s_key_group.get());
  FlushCache();
  return ret;
}

auto GpgKeyGroupGetter::RemoveKeyFromKeyGroup(const QString& id,
                                              const QString& key_id) -> bool {
  if (!key_groups_forest_.contains(id)) return false;
  auto key_group = key_groups_forest_.value(id);

  if (!IsKeyGroupID(key_id)) {
    LOG_D() << "removing non key group id" << key_id << "form key group" << id;
    key_group->RemoveNonKeyGroupKey(key_id);
    FlushCache();
    return true;
  }

  if (!key_groups_forest_.contains(key_id)) {
    LOG_E() << "try remove invalid key group id:" << key_id;
    return false;
  }

  auto s_key_group = key_groups_forest_.value(key_id);
  auto ret = key_group->RemoveChildren(s_key_group.get());
  FlushCache();
  return ret;
}

GpgKeyGroupTreeNode::GpgKeyGroupTreeNode(GpgKeyGroup kg)
    : key_group(QSharedPointer<GpgKeyGroup>::create(kg)) {
  for (const auto& key_id : key_group->KeyIds()) {
    if (!IsKeyGroupID(key_id)) {
      non_key_group_ids.push_back(key_id);
    }
  }
}

void GpgKeyGroupTreeNode::Apply() { key_group->SetKeyIds(KeyIds()); }

auto GpgKeyGroupTreeNode::AddNonKeyGroupKey(const GpgAbstractKeyPtr& key)
    -> bool {
  if (key->KeyType() == GpgAbstractKeyType::kGPG_KEYGROUP ||
      non_key_group_ids.contains(key->ID())) {
    return false;
  }
  non_key_group_ids.push_back(key->ID());
  non_key_group_ids.removeDuplicates();
  Apply();
  return true;
}

auto GpgKeyGroupTreeNode::HasAncestor(GpgKeyGroupTreeNode* target) -> bool {
  for (const auto& parent : parents) {
    if (parent == target || parent->HasAncestor(target)) {
      return true;
    }
  }
  return false;
}

auto GpgKeyGroupTreeNode::AddChildren(GpgKeyGroupTreeNode* target) -> bool {
  if (target == this || children.contains(target) || HasAncestor(target)) {
    return false;
  }

  children.push_back(target);
  target->parents.push_back(this);
  Apply();
  return true;
}

auto GpgKeyGroupTreeNode::RemoveChildren(GpgKeyGroupTreeNode* target) -> bool {
  if (!children.contains(target)) return false;
  children.removeAll(target);
  target->parents.removeAll(this);
  Apply();
  return true;
}

auto GpgKeyGroupTreeNode::RemoveNonKeyGroupKey(const QString& key) -> bool {
  if (!non_key_group_ids.contains(key)) return false;
  non_key_group_ids.removeAll(key);
  Apply();
  return true;
}

auto GpgKeyGroupGetter::IsKeyGroupDisabled(const QString& id) -> bool {
  if (!key_groups_forest_.contains(id)) return false;
  auto node = key_groups_forest_.value(id);
  return node->disabled;
}

auto GpgKeyGroupTreeNode::KeyIds() const -> QStringList {
  QStringList ret;
  for (const auto& child : children) {
    ret.push_back(child->key_group->ID());
  }
  ret.append(non_key_group_ids);
  return ret;
}
}  // namespace GpgFrontend