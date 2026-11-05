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
#include <QPointer>
#include <QSharedPointer>
#include <QTimer>
#include <functional>

#include "model/KeyRoleModel.h"
#include "shell/models/GpgKeyTableModel.h"

class QQmlEngine;
class QJSEngine;

namespace GpgFrontend {
class GpgKeyTableModel;
}

namespace GpgFrontend::Mobile {

/**
 * @brief The keys of the default key database, for every page that lists them.
 *
 * One KeyRoleModel, re-pointed at a fresh GpgKeyTableModel after each
 * refresh(), which re-reads the key databases the same way the desktop does
 * (RefreshKeyDatabases()). Pages filter it with their own KeyFilterModel.
 *
 * A refresh that does not report back within kRefreshTimeoutMs ends anyway,
 * showing what the default database holds, and says so (error): the list
 * never stays "refreshing" for good. (It did, twice, on a freshly booted
 * emulator, with every thread idle: the end of the refresh task was lost.)
 *
 * Refreshes never overlap: the core's refresh reads whichever key database
 * Runtime is in use when it runs, which a switch replaces, so a second one
 * at the same time would read the same repositories twice. A refresh asked
 * for while one runs is not dropped either: it is run once that one ends,
 * and only the latest one asked for (its generation) changes what is shown
 * -- the list, refreshing, loaded and error. One that ends with another
 * database in use than when it started is run again: what it read may be
 * the old database's.
 */
class KeyListController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(GpgFrontend::Mobile::KeyRoleModel* keys READ Keys CONSTANT)
  Q_PROPERTY(bool refreshing READ Refreshing NOTIFY SignalRefreshingChanged)
  /// The current database's keys have been read at least once: an empty
  /// list is then really empty, not still loading.
  Q_PROPERTY(bool loaded READ Loaded NOTIFY SignalRefreshingChanged)
  /// Why the last refresh did not end as it should; empty when it did.
  Q_PROPERTY(QString error READ Error NOTIFY SignalRefreshingChanged)
  /// The database the list shown was read from: its name goes with the list.
  Q_PROPERTY(
      QString loadedDatabase READ LoadedDatabase NOTIFY SignalRefreshingChanged)

 public:
  // Deliberately no default constructor: given one, QML constructs a
  // singleton of its own and never calls create(), so the interface would
  // talk to a different object than the application.
  explicit KeyListController(QObject* parent);
  ~KeyListController() override;

  static void SetInstance(KeyListController* instance);
  static auto create(QQmlEngine*, QJSEngine*) -> KeyListController*;

  [[nodiscard]] auto Keys() const -> KeyRoleModel*;
  [[nodiscard]] auto Refreshing() const -> bool;
  [[nodiscard]] auto Loaded() const -> bool;
  [[nodiscard]] auto Error() const -> QString;
  /// The database the list shown was read from.
  [[nodiscard]] auto LoadedDatabase() const -> QString;

  /// Re-read the key databases, then show what they hold. Needs a ready core.
  Q_INVOKABLE void refresh();

  /// Show the keys the core has read already, without reading the databases
  /// again: at the end of startup, when it has just read every one of them
  /// (a second read of a thousand keys took the phone a second). Nothing
  /// while a refresh() is under way: it shows them.
  void ShowLoaded();

  /// Show the default database's keys and groups again, without re-reading
  /// the databases: what a change of key groups needs, which lives in
  /// memory already. Nothing while a refresh() is under way: it shows them.
  static void Reload();

  /// Show @p table's keys; refresh() does this with the default database's.
  void SetTable(QSharedPointer<GpgKeyTableModel> table);

  /// How long refresh() waits for the key databases before it ends anyway.
  static constexpr int kRefreshTimeoutMs = 20000;

  /// What re-reads the key databases: RefreshKeyDatabases(), unless a test
  /// says else. @p done is to be called once, on the interface thread.
  using Refresher =
      std::function<void(QObject* receiver, std::function<void()> done)>;
  static void SetRefresherForTesting(Refresher refresher);

  /// refresh()'s wait, for tests.
  void SetRefreshTimeoutForTesting(int ms);

  /// Which key database is in use: the default channel's, unless a test
  /// says else.
  using DatabaseId = std::function<QString()>;
  static void SetDatabaseIdForTesting(DatabaseId database_id);

 signals:
  void SignalRefreshingChanged();
  void SignalRefreshed();

 private:
  KeyRoleModel* keys_;
  QSharedPointer<GpgKeyTableModel> table_;
  bool refreshing_ = false;
  bool loaded_ = false;
  QString error_;
  QString loaded_db_;
  /// The core is refreshing (also after a timeout, until it reports back).
  bool running_ = false;
  /// Another refresh was asked for while one ran.
  bool rerun_ = false;
  /// The run under way did not report back in time.
  bool timed_out_ = false;
  /// The latest refresh asked for; only its end changes what is shown.
  int generation_ = 0;
  /// The run under way: its generation and the database it started on.
  int running_generation_ = 0;
  QString running_db_;
  QTimer timeout_;

  /// Have the core refresh, for the latest generation.
  void start();
  /// The run for @p generation reported back (or, @p timed_out, did not).
  void finish(int generation, bool timed_out);
  /// Show the default database now, as read for @p database.
  void show(const QString& database, const QString& error);
  void setRefreshing(bool refreshing);
};

}  // namespace GpgFrontend::Mobile
