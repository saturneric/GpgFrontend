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

#include <QApplication>

#include "core/GFConstants.h"

namespace GpgFrontend {

struct GpgFrontendContext;

using GFCxtWPtr = QWeakPointer<GpgFrontendContext>;
using GFCxtSPtr = QSharedPointer<GpgFrontendContext>;

struct GpgFrontendContext {
  int argc;
  char** argv;

  bool gather_external_gnupg_info;
  bool unit_test_mode;

  int rtn = GpgFrontend::kCrashCode;

  /**
   * @brief Why the profile could not be resolved, empty when it could.
   *
   * The bootstrap runs before there is any sensible way to show a dialog or
   * write a log, so a failure is recorded rather than reported. The runtime is
   * still established, on the classic location, so nothing downstream has to
   * cope with a half-resolved process; main() halts on this before anything
   * touches key material.
   */
  QString profile_error;

  /**
   * @brief Construct a new Gpg Frontend Context object
   *
   * @param argc
   * @param argv
   */
  GpgFrontendContext(int argc, char** argv);

  /**
   * @brief Destroy the Gpg Frontend Context object
   *
   */
  ~GpgFrontendContext();

  /**
   * @brief
   *
   */
  void InitApplication();

  /**
   * @brief Re-resolve the ENV.ini-backed properties against the current root.
   *
   * A profile opened from a package is only known after the passphrase prompt,
   * by which point the layered properties — key protection, secure level, log
   * level — have already been resolved against the *previous* root. Re-running
   * the resolution is what stops such a profile running under another
   * profile's key protection.
   */
  void ReloadEnvProperties();

  /**
   * @brief Get the App object
   *
   * @return QCoreApplication*
   */
  auto GetApp() -> QApplication*;

 private:
  QApplication* app_ = nullptr;

  /**
   * @brief
   *
   */
  void load_env_conf_set_properties();

  /**
   * @brief Resolve the profile and fix it for the lifetime of the process.
   *
   * Runs between constructing the QApplication and reading any settings,
   * because everything the latter touches is keyed by the profile root.
   */
  void establish_profile_runtime();

  /**
   * @brief Publish the resolved profile as qApp properties.
   *
   * A one-way mirror for the module and RT layers, which read properties and
   * cannot link against the core. ProfileRuntime remains the authority.
   */
  void mirror_profile_properties();

  /**
   * @brief
   *
   * @param name
   * @return QVariant
   */
  auto property(const char* name) -> QVariant;

  /**
   * @brief
   *
   * @param name
   * @return auto
   */
  auto property(const char* name, const QVariant& value) -> bool;
};

}  // namespace GpgFrontend