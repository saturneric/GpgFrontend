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

#include <QtQml/qqmlregistration.h>

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace GpgFrontend::Mobile {

/**
 * @brief The key groups of the key database in use, as the desktop keeps
 * them (KeyGroupRepository): made, renamed, filled, emptied and deleted.
 *
 * Encrypting to a group encrypts to every key it holds; the engine expands
 * it. A group's members are key ids (KeyRoleModel's keyId), or other
 * groups' ids, which the desktop can nest; here only keys are added, and a
 * nested group is shown and can be taken out.
 *
 * Every change shows in the key list at once (KeyListController::Reload):
 * groups live in memory, so the databases are not read again.
 */
class KeyGroupsController : public QObject {
  Q_OBJECT
  QML_ELEMENT

  /// Bumped on every change to the key list, for what reads by call.
  Q_PROPERTY(int revision READ Revision NOTIFY SignalChanged)

 public:
  explicit KeyGroupsController(QObject* parent = nullptr);

  [[nodiscard]] auto Revision() const -> int;

  /// What is wrong with @p name for a group, by the desktop's rule:
  /// "too_short" (fewer than five characters); "" when it will do.
  [[nodiscard]] Q_INVOKABLE QString nameProblem(const QString& name) const;

  /**
   * @brief What is wrong with a group's @p name and @p email, by the
   * desktop's rule (ValidateKeyGroupMetadata): {field: "name" or "email",
   * text}; field "" when they will do. The email may be empty.
   */
  [[nodiscard]] Q_INVOKABLE QVariantMap problemOf(const QString& name,
                                                  const QString& email) const;

  /// A new, empty group named @p name, with an @p email and a @p comment
  /// if given, as the desktop's dialog makes one; its id, or "" when it
  /// was not made.
  Q_INVOKABLE QString create(const QString& name, const QString& email = {},
                             const QString& comment = {});

  /// The group @p id: {id, name, email, comment, memberCount}; empty when
  /// there is none.
  [[nodiscard]] Q_INVOKABLE QVariantMap group(const QString& id) const;

  /// Its direct members, in its order: [{id, fingerprint, name, email,
  /// keyId, isGroup, missing}]. A member no longer in the keyring is
  /// listed with missing true and its id as its name.
  [[nodiscard]] Q_INVOKABLE QVariantList members(const QString& id) const;

  /// The ids of its direct members, as the key picker takes them.
  [[nodiscard]] Q_INVOKABLE QStringList memberIds(const QString& id) const;

  /// Put the keys @p key_ids into the group @p id, at once.
  /// @return how many went in
  Q_INVOKABLE int addMembers(const QString& id, const QStringList& key_ids);

  /// Take the member @p key_id (a key or a nested group) out of @p id.
  Q_INVOKABLE bool removeMember(const QString& id, const QString& key_id);

  /// Rename the group @p id; why not ("" when done), as nameProblem().
  Q_INVOKABLE QString rename(const QString& id, const QString& name);

  /// Change the group @p id's name, email and comment at once, as the
  /// desktop's edit dialog does; why not ("" when done), as problemOf().
  Q_INVOKABLE QString edit(const QString& id, const QString& name,
                           const QString& email, const QString& comment);

  /// Delete the group @p id; its keys are not touched.
  Q_INVOKABLE bool remove(const QString& id);

 signals:
  void SignalChanged();

 private:
  int revision_ = 0;

  /// Show the change in the key list.
  static void changed();
};

}  // namespace GpgFrontend::Mobile
