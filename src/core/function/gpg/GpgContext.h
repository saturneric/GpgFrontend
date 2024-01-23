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

#include "core/function/SecureMemoryAllocator.h"
#include "core/function/basic/GpgFunctionObject.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
struct GpgContextInitArgs {
  QString db_path = {};  ///<

  bool test_mode = false;                ///<
  bool offline_mode = false;             ///<
  bool auto_import_missing_key = false;  ///<

  bool custom_gpgconf = false;  ///<
  QString custom_gpgconf_path;  ///<

  bool use_pinentry = false;  ///<
};

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgContext
    : public SingletonFunctionObject<GpgContext> {
 public:
  explicit GpgContext(int channel);

  explicit GpgContext(GpgContextInitArgs args, int channel);

  virtual ~GpgContext() override;

  [[nodiscard]] auto Good() const -> bool;

  auto BinaryContext() -> gpgme_ctx_t;

  auto DefaultContext() -> gpgme_ctx_t;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};
}  // namespace GpgFrontend
