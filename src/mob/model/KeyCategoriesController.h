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

namespace GpgFrontend::Mobile {

/**
 * @brief The key categories of the default key database, as the desktop's
 * key list keeps them: named, coloured groups of keys, a key in any number
 * of them, the order shared with the desktop's custom tabs.
 *
 * Every instance reads and writes the same store (KeyCategoryRepository),
 * and hears of what another changed: a page and a sheet stay in step.
 */
class KeyCategoriesController : public QObject {
  Q_OBJECT
  QML_ELEMENT

  /// [{id, name, color ("#rrggbb"), count}], in the shared order.
  Q_PROPERTY(QVariantList categories READ Categories NOTIFY SignalChanged)
  /// The built-in ones, which go by what keys are rather than by who put
  /// them there, as the desktop's built-in tabs (same ids): [{id, name,
  /// color, builtin}]. KeyFilterModel knows each id's rule.
  Q_PROPERTY(QVariantList builtins READ Builtins NOTIFY SignalChanged)
  /// The swatches offered: [{color, name}].
  Q_PROPERTY(QVariantList presetColors READ PresetColors CONSTANT)
  /// Bumped on every change, for what reads by call.
  Q_PROPERTY(int revision READ Revision NOTIFY SignalChanged)

 public:
  explicit KeyCategoriesController(QObject* parent = nullptr);

  [[nodiscard]] auto Categories() const -> QVariantList;
  [[nodiscard]] auto Builtins() const -> QVariantList;
  [[nodiscard]] auto PresetColors() const -> QVariantList;
  [[nodiscard]] auto Revision() const -> int;

  /// Tell every controller the categories may have changed (another key
  /// database is in use, say).
  static void NotifyChanged();

  /// The colour to offer for a new category: one not in use yet.
  [[nodiscard]] Q_INVOKABLE QString suggestColor() const;

  /// A new category; why not ("" when it was made). Its id in SignalAdded.
  Q_INVOKABLE QString add(const QString& name, const QString& color);

  /// Rename and recolour the category @p id; why not ("" when done).
  Q_INVOKABLE QString edit(const QString& id, const QString& name,
                           const QString& color);

  /// Delete the category @p id; its keys are not touched.
  Q_INVOKABLE void remove(const QString& id);

  /// Move the category @p id by @p by places in the shared order.
  Q_INVOKABLE void move(const QString& id, int by);

  /// The key ids (KeyRoleModel's keyId) in the category @p id.
  [[nodiscard]] Q_INVOKABLE QStringList keyIdsOf(const QString& id) const;

  /// The ids of the categories the key @p key_id is in.
  [[nodiscard]] Q_INVOKABLE QStringList
  categoriesOf(const QString& key_id) const;

  /// Put the key @p key_id in the category @p id, or take it out.
  Q_INVOKABLE void setMember(const QString& id, const QString& key_id,
                             bool member);

  /// Put all of @p key_ids into the category @p id (@p member), or take them
  /// out, at once: one change for the lot, not one per key.
  /// @return how many changed
  Q_INVOKABLE int setMembers(const QString& id, const QStringList& key_ids,
                             bool member);

 signals:
  void SignalChanged();
  void SignalAdded(const QString& id);

 private:
  [[nodiscard]] auto ordered_ids() const -> QStringList;
  [[nodiscard]] auto check_name(const QString& name,
                                const QString& except) const -> QString;
  void changed();
};

}  // namespace GpgFrontend::Mobile
