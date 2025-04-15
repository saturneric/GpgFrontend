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

    key_groups_forest_.insert(
        key_group.id,
        QSharedPointer<GpgKeyGroupTreeNode>::create(GpgKeyGroup{key_group}));
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
    const QSharedPointer<GpgKeyGroup>& key_group) {
  if (key_group == nullptr || key_group->IsDisabled()) return;

  for (const auto& key_id : key_group->KeyIds()) {
    LOG_D() << "check" << key_id << "of" << key_group->UID();

    if (IsKeyGroupID(key_id) || key_id == key_group->ID()) {
      if (!key_groups_forest_.contains(key_id)) {
        key_group->SetDisabled(true);
        return;
      }

      auto s_node = key_groups_forest_.value(key_id, nullptr);
      check_key_group(s_node->key_group);

      if (s_node->key_group->IsDisabled()) {
        key_group->SetDisabled(true);
        return;
      }

    } else {
      auto key = GpgKeyGetter::GetInstance(GetChannel()).GetKeyPtr(key_id);
      if (key == nullptr || !key->IsHasEncrCap()) {
        key_group->SetDisabled(true);
        return;
      }
    }
  }
}

void GpgKeyGroupGetter::check_all_key_groups() {
  for (const auto& node : key_groups_forest_) {
    node->key_group->SetDisabled(false);
  }

  for (const auto& node : key_groups_forest_) {
    auto key_group = node->key_group;
    check_key_group(key_group);

    LOG_D() << "key group" << key_group->ID() << "ids: " << key_group->KeyIds()
            << "status: " << key_group->IsDisabled();
  }
}

auto GpgKeyGroupGetter::FlushCache() -> bool {
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

void GpgKeyGroupTreeNode::Apply() {
  QStringList key_ids;
  for (const auto& child : children) {
    key_ids.push_back(child->key_group->ID());
  }

  for (const auto& key_id : non_key_group_ids) {
    key_ids.push_back(key_id);
  }

  key_group->SetKeyIds(key_ids);
}

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
}  // namespace GpgFrontend