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

#include "core/GFConstants.h"
#include "core/profile/Profile.h"

namespace GpgFrontend {

struct BootContext;

using BootCxtWPtr = QWeakPointer<BootContext>;
using BootCxtSPtr = QSharedPointer<BootContext>;

/**
 * @brief What startup carries from main() to shutdown, without an interface.
 *
 * Shared by every program that opens a profile and starts the core. A program
 * with an interface derives from it for what only that interface has.
 */
struct BootContext {
  int argc;
  char** argv;

  bool gather_external_gnupg_info;
  bool unit_test_mode;

  int rtn = GpgFrontend::kCrashCode;

  /// The exit code when shutdown wedges and the watchdog ends the process: a
  /// test program sets its result here, so a hang at exit does not turn a
  /// failed run into a passing one.
  int watchdog_exit_code = 0;

  /**
   * @brief Why the profile could not be selected, empty when it could.
   *
   * Selection runs before there is any sensible way to show a dialog or write
   * a log, so a failure is recorded rather than reported. The fallback
   * selection is still usable, so nothing downstream has to cope with a
   * half-resolved process; main() halts on this before anything touches key
   * material.
   */
  QString profile_error;

  /// What the command line, the environment and the registry asked for.
  ProfileSelection profile_selection;

  /**
   * @brief The log level `--log-level` asked for, invalid when it was not set.
   *
   * Carried rather than applied and forgotten: it is the top layer of the same
   * ladder every other knob resolves through, so a stored `advanced/log_level`
   * cannot quietly win over a flag the user typed on this very run.
   */
  QVariant cli_log_level;

  /**
   * @brief Why startup cannot go on at all, empty while it can.
   *
   * Set by PreInit() when the process is missing something nothing can work
   * without. Recorded rather than shown, because there may be no interface to
   * show it in; the program that has one shows it.
   */
  QString fatal_error;

  BootContext(int argc, char** argv);

  virtual ~BootContext();

  /**
   * @brief Work out which profile was asked for, without opening anything.
   *
   * Pure with respect to the profile system: it reads argv, the environment and
   * a scan of the profiles root, and leaves opening to the loader.
   *
   * @param handed_package a package the platform handed over outside argv, as
   * macOS does for a double-clicked document; empty when there is none
   */
  void SelectProfile(const QString& handed_package = {});

  /**
   * @brief Read the marker-backed properties against the mounted profile.
   *
   * Runs once the loader has published a session, never before: every layered
   * property — key protection, secure level, log level — is resolved partly
   * from that profile's own settings, and resolving them against the wrong
   * root is how a profile ends up running under another one's key protection.
   */
  void LoadEnvProperties();

 private:
  /**
   * @brief
   *
   */
  void load_env_conf_set_properties();

  /**
   * @brief Publish the resolved profile as qApp properties.
   *
   * A one-way mirror for the module and RT layers, which read properties and
   * cannot link against the core. ProfileSession remains the authority.
   */
  void mirror_profile_properties();

  auto property(const char* name) -> QVariant;

  auto property(const char* name, const QVariant& value) -> bool;
};

}  // namespace GpgFrontend
