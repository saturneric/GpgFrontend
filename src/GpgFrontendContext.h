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

#include "core/GpgConstants.h"

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