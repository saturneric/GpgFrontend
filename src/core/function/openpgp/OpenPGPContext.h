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

#include <gpgme.h>

#include <cstdint>

#include "core/function/basic/GpgFunctionObject.h"

namespace GpgFrontend {

class GFKeyDatabase;

/**
 * @brief
 *
 */
struct OpenPGPContextInitArgs {
  OpenPGPEngine engine;  ///<
  QString db_name;       ///<
  QString db_path;       ///<

  bool test_mode = false;                ///<
  bool offline_mode = false;             ///<
  bool auto_import_missing_key = false;  ///<
};

/**
 * @brief
 *
 */
class GF_CORE_EXPORT OpenPGPContext
    : public SingletonFunctionObject<OpenPGPContext> {
 public:
  /**
   * @brief Construct a new Open P G P Context object
   *
   * @param channel
   */
  explicit OpenPGPContext(int channel);

  /**
   * @brief Construct a new Open P G P Context object
   *
   * @param args
   * @param channel
   */
  explicit OpenPGPContext(const OpenPGPContextInitArgs &args, int channel);

  /**
   * @brief Destroy the Open P G P Context object
   *
   */
  ~OpenPGPContext();

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto Initialize() -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto Good() const -> bool;

  /**
   * @brief
   *
   * @return OpenPGPEngine
   */
  [[nodiscard]] auto Engine() const -> OpenPGPEngine;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto EngineVersion() const -> QString;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto KeyDBName() const -> QString;

  /**
   * @brief
   *
   * @return QString
   */
  [[nodiscard]] auto KeyDBPath() const -> QString;

  /**
   * @brief
   *
   * @return QSharedPointer<GFKeyDatabase>
   */
  auto KeyDatabase() -> QSharedPointer<GFKeyDatabase>;

 protected:
  /**
   * @brief
   *
   * @param args
   * @return true
   * @return false
   */
  virtual auto init(const OpenPGPContextInitArgs &args) -> bool;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
  bool good_ = false;  ///<
};

template <typename T>
auto BuildContext(int channel, OpenPGPContextInitArgs args)
    -> ChannelObjectPtr {
  auto o = SecureCreateUniqueObject<T>(args, channel);
  o->Initialize();

  auto c_o = ConvertToChannelObjectPtr<>(std::move(o));
  return c_o;
}
}  // namespace GpgFrontend
