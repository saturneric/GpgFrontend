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

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace GpgFrontend::Mobile {

/**
 * @brief Make, export and delete keys, for the Keys pages.
 *
 * Each action runs on the GPG task runner and ends with SignalFinished(
 * action, succeeded, detail), action being "generate", "export" or "delete":
 *
 *  - generate: detail {fingerprint}; a new key's passphrase is asked through
 *    the PassphraseBroker.
 *  - export: detail {armored}, the public key as text.
 *  - delete: detail {}.
 *
 * A failed action's detail has {error}. After generate and delete the key
 * list wants a refresh; the page asks for it.
 */
class KeyActionsController : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool busy READ Busy NOTIFY SignalBusyChanged)
  /// What generate() can make here: [{name, description}], the recommended
  /// one first.
  Q_PROPERTY(QVariantList profiles READ Profiles CONSTANT)

 public:
  explicit KeyActionsController(QObject* parent = nullptr);

  [[nodiscard]] auto Busy() const -> bool;
  [[nodiscard]] auto Profiles() const -> QVariantList;

  /**
   * @brief Generate a key for @p name and @p email with the profile at
   * @p profile in Profiles(); protected by a passphrase unless
   * @p with_passphrase is false.
   */
  Q_INVOKABLE void generate(const QString& name, const QString& email,
                            int profile, bool with_passphrase);

  /// The public key @p fingerprint, armored.
  Q_INVOKABLE void exportPublic(const QString& fingerprint);

  /// Delete the key @p fingerprint, its secret part too.
  Q_INVOKABLE void remove(const QString& fingerprint);

 signals:
  void SignalBusyChanged();
  void SignalFinished(const QString& action, bool succeeded,
                      const QVariantMap& detail);

 private:
  bool busy_ = false;

  auto begin() -> bool;
  void run(const QString& action, std::function<QVariantMap()> work);
};

}  // namespace GpgFrontend::Mobile
