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

#include "core/function/basic/GpgFunctionObject.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
struct GpgContextInitArgs {
  std::string db_path = {};

  bool test_mode = false;
  bool ascii = true;
  bool offline_mode = false;
  bool auto_import_missing_key = false;

  bool custom_gpgconf = false;
  std::string custom_gpgconf_path;

  bool use_pinentry = false;
};

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgContext
    : public QObject,
      public SingletonFunctionObject<GpgContext> {
  Q_OBJECT
 public:
  explicit GpgContext(int channel);

  explicit GpgContext(const GpgContextInitArgs &args, int channel);

  ~GpgContext() override;

  [[nodiscard]] auto Good() const -> bool;

  operator gpgme_ctx_t() const;

  void SetPassphraseCb(gpgme_passphrase_cb_t passphrase_cb) const;

  void ShowPasswordInputDialog();

 signals:
  void SignalNeedUserInputPassphrase();

 private:
  class Impl;
  std::unique_ptr<Impl> p_;
};
}  // namespace GpgFrontend
