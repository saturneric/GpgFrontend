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
#include <QVariantList>

namespace GpgFrontend::Mobile {

/**
 * @brief The key databases of the open profile, and which one the
 * application works in.
 *
 * Switching among them, and making a new one kept in the profile, named and
 * nothing more, as the App Store build does. Moving, renaming, removing and
 * databases elsewhere are the desktop's (Settings > Key Databases). A switch
 * is what the
 * desktop's Settings do: the choice is stored, then the core opens the
 * databases again on new channels (ApplyKeyDatabaseRuntime()), off the
 * interface thread. The key list follows on its own (AppServices listens for
 * the new runtime).
 */
class KeyDatabasesController : public QObject {
  Q_OBJECT
  QML_ELEMENT

  /// [{id, name, path, current}], the databases open now.
  Q_PROPERTY(QVariantList databases READ Databases NOTIFY SignalChanged)
  /// The name of the one in use.
  Q_PROPERTY(QString currentName READ CurrentName NOTIFY SignalChanged)
  Q_PROPERTY(bool busy READ Busy NOTIFY SignalBusyChanged)

 public:
  explicit KeyDatabasesController(QObject* parent = nullptr);

  [[nodiscard]] auto Databases() const -> QVariantList;
  [[nodiscard]] auto CurrentName() const -> QString;
  [[nodiscard]] auto Busy() const -> bool;

  /// Work in the database @p id from now on, and from the next start.
  /// Reported by SignalSwitched.
  Q_INVOKABLE void switchTo(const QString& id);

  /// Why @p name cannot be a new database's, empty when it can.
  Q_INVOKABLE [[nodiscard]] QString nameProblem(const QString& name) const;
  /// A free name to start from.
  Q_INVOKABLE [[nodiscard]] QString suggestName() const;
  /// Make a new database named @p name in the profile and work in it.
  /// Reported by SignalCreated.
  Q_INVOKABLE void create(const QString& name);

 signals:
  void SignalChanged();
  void SignalBusyChanged();
  /// @p succeeded when the database @p id is the one in use now; @p error
  /// says why not.
  void SignalSwitched(bool succeeded, const QString& error);
  /// @p succeeded when the new database is made and in use.
  void SignalCreated(bool succeeded, const QString& error);

 private:
  /// Open the databases again, as stored, and report how it went.
  void apply(const QString& id);

  bool busy_ = false;
  bool creating_ = false;  ///< the running apply is for create()
};

}  // namespace GpgFrontend::Mobile
