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
#include <QUrl>
#include <QVariantList>
#include <functional>

namespace GpgFrontend::Mobile {

/**
 * @brief The profiles on this phone: which one is open, a new one, a profile
 * exported to a file and one imported from a file, and opening another.
 *
 * A profile is chosen as the application starts, before anything else is set
 * up, so opening another one remembers the choice (ApplyChosenProfile() reads
 * it at the next start) and starts the application again. The rest is the
 * desktop's own: CreateProfile(), the package an export writes
 * (GatherOpenProfileExport(), ExportProfilePackage()) and the steps an import
 * takes (InspectProfilePackage(), ReadProfilePackage(),
 * AdoptExtractedProfile()). Renaming and deleting profiles are the desktop's.
 */
class ProfilesController : public QObject {
  Q_OBJECT
  QML_ELEMENT

  /// [{id, name, current}], the open one marked.
  Q_PROPERTY(QVariantList profiles READ Profiles NOTIFY SignalChanged)
  Q_PROPERTY(QString currentName READ CurrentName NOTIFY SignalChanged)
  /// An export or an import is under way.
  Q_PROPERTY(bool busy READ Busy NOTIFY SignalBusyChanged)
  /// The shortest passphrase an export takes, as on the desktop.
  Q_PROPERTY(int minPassphraseLength READ MinPassphraseLength CONSTANT)

 public:
  explicit ProfilesController(QObject* parent = nullptr);
  ~ProfilesController() override;

  [[nodiscard]] auto Profiles() const -> QVariantList;
  [[nodiscard]] auto CurrentName() const -> QString;
  [[nodiscard]] auto Busy() const -> bool;
  [[nodiscard]] static auto MinPassphraseLength() -> int;

  /// Why @p name cannot be a new profile's, empty when it can.
  Q_INVOKABLE [[nodiscard]] QString nameProblem(const QString& name) const;

  /// Make an empty profile named @p name, with keys of its own.
  /// @return its id, empty when it could not be made (SignalFailed says why)
  Q_INVOKABLE QString create(const QString& name);

  /// The file name an export of the open profile is offered under.
  Q_INVOKABLE [[nodiscard]] QString exportFileName() const;

  /// Write the open profile, sealed with @p passphrase, to @p url (a
  /// document the system's picker gave). Reported by SignalExported.
  Q_INVOKABLE void exportTo(const QUrl& url, const QString& passphrase);

  /// Read the package at @p url. A sealed one asks for its passphrase
  /// (SignalPassphraseNeeded, then importWith()); reported by
  /// SignalImported.
  Q_INVOKABLE void importFrom(const QUrl& url);
  /// Go on with the import waiting for its passphrase.
  Q_INVOKABLE void importWith(const QString& passphrase);
  /// Drop the import waiting for its passphrase.
  Q_INVOKABLE void cancelImport();

  /// Open the profile @p id from the next start on, and start again now.
  /// @return false when there is no such profile or no restart could begin
  Q_INVOKABLE bool switchTo(const QString& id);

  /**
   * @brief Before a profile is chosen: name the one asked for last
   * (GF_PROFILE), when it is still there.
   *
   * Called by the startup worker before BootContext::SelectProfile().
   *
   * @param installed_root the installed data location, whose profiles
   * folder holds the profiles
   * @return whether another profile than the default was named
   */
  static auto ApplyChosenProfile(const QString& installed_root) -> bool;

  /**
   * @brief Forget the profile chosen and start again with the default one.
   *
   * The way out when the chosen profile will not open: a phone has no
   * command line to name another.
   * @return false when the application could not start again
   */
  static auto OpenDefaultProfile(const QString& installed_root) -> bool;

  /// Remove every package left in the cache by an earlier run (one ended
  /// while a package was written or read); called once at start.
  static void RemoveLeftPackages();

  /// Where the choice is kept, under @p installed_root.
  static auto ChosenProfileFile(const QString& installed_root) -> QString;

  /// How the application starts again: shut down, then restart. Set by
  /// AppServices; tests set one that records.
  static void SetRestarter(std::function<bool()> restarter);

 signals:
  void SignalChanged();
  void SignalBusyChanged();
  void SignalFailed(const QString& error);
  void SignalExported(bool succeeded, const QString& error);
  /// The package at the URL given is sealed: importWith() goes on. @p error
  /// says why the last passphrase did not open it, empty the first time.
  void SignalPassphraseNeeded(const QString& error);
  /// @p id is the new profile's, @p name its name.
  void SignalImported(bool succeeded, const QString& error, const QString& id,
                      const QString& name);

 private:
  struct Pending;

  void set_busy(bool busy);
  void read_package(const QString& passphrase);
  void finish_import(bool succeeded, const QString& error,
                     const QString& id = {}, const QString& name = {});

  bool busy_ = false;
  std::unique_ptr<Pending> pending_;  ///< the import being read
};

}  // namespace GpgFrontend::Mobile
