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

#include "model/KeyGroupsController.h"

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/KeyCategoryRepository.h"
#include "core/function/openpgp/KeyGroupRepository.h"
#include "core/model/GpgKeyGroup.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/GpgUtils.h"
#include "model/KeyCategoriesController.h"
#include "model/KeyListController.h"
#include "shell/rules/KeyGroupRules.h"

namespace GpgFrontend::Mobile {

namespace {

auto Groups() -> KeyGroupRepository& {
  return KeyGroupRepository::GetInstance(UsableDefaultChannelHandle());
}

/// Groups live in the open profile's key database: none before.
auto Ready() -> bool { return ProfileSession::Loaded(); }

}  // namespace

KeyGroupsController::KeyGroupsController(QObject* parent) : QObject(parent) {
  // Every change of the key list, a group's or a key's, is one here: a
  // member may have gone, or come.
  if (auto* keys = KeyListController::create(nullptr, nullptr);
      keys != nullptr) {
    connect(keys, &KeyListController::SignalRefreshed, this, [this]() {
      ++revision_;
      emit SignalChanged();
    });
  }
}

auto KeyGroupsController::Revision() const -> int { return revision_; }

auto KeyGroupsController::nameProblem(const QString& name) const -> QString {
  return DescribeKeyGroupMetadataProblem(ValidateKeyGroupMetadata(name, {}));
}

auto KeyGroupsController::problemOf(const QString& name,
                                    const QString& email) const -> QVariantMap {
  const auto problem = ValidateKeyGroupMetadata(name, email.trimmed());
  QString field;
  if (problem == KeyGroupMetadataProblem::kNameTooShort) {
    field = QStringLiteral("name");
  } else if (problem == KeyGroupMetadataProblem::kEmailMalformed) {
    field = QStringLiteral("email");
  }
  return {{QStringLiteral("field"), field},
          {QStringLiteral("text"), DescribeKeyGroupMetadataProblem(problem)}};
}

auto KeyGroupsController::create(const QString& name, const QString& email,
                                 const QString& comment) -> QString {
  if (!Ready() || !problemOf(name, email)
                       .value(QStringLiteral("field"))
                       .toString()
                       .isEmpty()) {
    return {};
  }
  const GpgKeyGroup group{
      name.trimmed(), email.trimmed(), comment.trimmed(), {}};
  Groups().AddKeyGroup(group);
  changed();
  return group.ID();
}

auto KeyGroupsController::group(const QString& id) const -> QVariantMap {
  if (!Ready()) return {};
  const auto group = Groups().KeyGroup(id);
  if (group == nullptr) return {};
  return {
      {QStringLiteral("id"), group->ID()},
      {QStringLiteral("name"), group->Name()},
      {QStringLiteral("email"), group->Email()},
      {QStringLiteral("comment"), group->Comment()},
      {QStringLiteral("memberCount"), group->KeyIds().size()},
  };
}

auto KeyGroupsController::memberIds(const QString& id) const -> QStringList {
  if (!Ready()) return {};
  const auto group = Groups().KeyGroup(id);
  return group == nullptr ? QStringList() : group->KeyIds();
}

auto KeyGroupsController::members(const QString& id) const -> QVariantList {
  QVariantList list;
  if (!Ready()) return list;
  auto& keys = AbstractKeyRepository::GetInstance(UsableDefaultChannelHandle());
  for (const auto& member : memberIds(id)) {
    const auto key = keys.GetKey(member);
    if (key == nullptr) {
      list.append(QVariantMap{
          {QStringLiteral("id"), member},
          {QStringLiteral("name"), member},
          {QStringLiteral("isGroup"), IsKeyGroupID(member)},
          {QStringLiteral("missing"), true},
      });
      continue;
    }
    list.append(QVariantMap{
        {QStringLiteral("id"), member},
        {QStringLiteral("fingerprint"), key->Fingerprint()},
        {QStringLiteral("name"), key->Name()},
        {QStringLiteral("email"), key->Email()},
        {QStringLiteral("keyId"), key->ID()},
        {QStringLiteral("isGroup"),
         key->KeyType() == GpgAbstractKeyType::kGPG_KEYGROUP},
        {QStringLiteral("missing"), false},
    });
  }
  return list;
}

auto KeyGroupsController::addMembers(const QString& id,
                                     const QStringList& key_ids) -> int {
  if (!Ready()) return 0;
  auto& keys = AbstractKeyRepository::GetInstance(UsableDefaultChannelHandle());
  int added = 0;
  for (const auto& key_id : key_ids) {
    // Keys only: the desktop nests groups, its own dialog does that.
    if (IsKeyGroupID(key_id)) continue;
    const auto key = keys.GetKey(key_id);
    // A group is a set of recipients: a key that cannot encrypt stays out,
    // as on the desktop.
    if (!Shell::CanJoinKeyGroup(UsableDefaultChannelHandle(), id, key.get())) {
      continue;
    }
    if (Groups().AddKey2KeyGroup(id, key)) ++added;
  }
  if (added > 0) changed();
  return added;
}

auto KeyGroupsController::removeMember(const QString& id, const QString& key_id)
    -> bool {
  if (!Ready() || !Groups().RemoveKeyFromKeyGroup(id, key_id)) return false;
  changed();
  return true;
}

auto KeyGroupsController::rename(const QString& id, const QString& name)
    -> QString {
  if (const auto problem = nameProblem(name); !problem.isEmpty()) {
    return problem;
  }
  if (!Ready() || !Groups().Rename(id, name.trimmed())) {
    return tr("The group could not be renamed.");
  }
  changed();
  return {};
}

auto KeyGroupsController::edit(const QString& id, const QString& name,
                               const QString& email, const QString& comment)
    -> QString {
  const auto problem = problemOf(name, email);
  if (const auto text = problem.value(QStringLiteral("text")).toString();
      !text.isEmpty()) {
    return text;
  }
  if (!Ready() || !Groups().UpdateMetadata(id, name.trimmed(), email.trimmed(),
                                           comment.trimmed())) {
    return tr("The group could not be changed.");
  }
  changed();
  return {};
}

auto KeyGroupsController::remove(const QString& id) -> bool {
  if (!Ready() || !Groups().Remove(id)) return false;
  // Gone from the categories it was put in too: a later group never
  // inherits them by chance.
  auto& categories =
      KeyCategoryRepository::GetInstance(UsableDefaultChannelHandle());
  for (const auto& category : categories.Fetch()) {
    if (!category.builtin && category.key_ids.contains(id)) {
      categories.RemoveKeyFromCategory(category.id, id);
    }
  }
  // Their counts and member lists, wherever shown.
  KeyCategoriesController::NotifyChanged();
  changed();
  return true;
}

void KeyGroupsController::changed() { KeyListController::Reload(); }

}  // namespace GpgFrontend::Mobile
