/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

namespace GpgFrontend {

struct GpgFrontendContext {
  int argc;
  char** argv;
  spdlog::level::level_enum log_level;

  bool load_ui_env;
  std::unique_ptr<QCoreApplication> app;

  /**
   * @brief Create a Instance object
   *
   * @param argc
   * @param argv
   * @return std::weak_ptr<GpgFrontendContext>
   */
  static auto CreateInstance(int argc, char** argv)
      -> std::weak_ptr<GpgFrontendContext>;

  /**
   * @brief Get the Instance object
   *
   * @return std::weak_ptr<GpgFrontendContext>
   */
  static auto GetInstance() -> std::weak_ptr<GpgFrontendContext>;

  /**
   * @brief
   *
   */
  void InitGUIApplication();

  /**
   * @brief
   *
   */
  void InitCoreApplication();

 private:
  static std::shared_ptr<GpgFrontendContext> global_context;
};

using GFCxtWPtr = std::weak_ptr<GpgFrontendContext>;
using GFCxtSPtr = std::shared_ptr<GpgFrontendContext>;

}  // namespace GpgFrontend