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

#include "core/function/gpg/GpgAgentProcess.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

class GF_CORE_EXPORT GpgContext : public OpenPGPContext {
 public:
  /**
   * @brief Construct a new Gpg Context object
   *
   * @param channel
   */
  explicit GpgContext(int channel);

  /**
   * @brief Construct a new Gpg Context object
   *
   * @param args
   * @param channel
   */
  explicit GpgContext(const OpenPGPContextInitArgs &args, int channel);

  /**
   * @brief Destroy the Gpg Context object
   *
   */
  ~GpgContext() override;

  /**
   * @brief
   *
   * @return gpgme_ctx_t
   */
  [[nodiscard]] auto BinaryContext() const -> gpgme_ctx_t;

  /**
   * @brief
   *
   * @return gpgme_ctx_t
   */
  [[nodiscard]] auto DefaultContext() const -> gpgme_ctx_t;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto RestartGpgAgent() -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto KillGpgAgent() -> bool;

  /**
   * @brief
   *
   * @param type
   * @return QString
   */
  [[nodiscard]] auto ComponentDirectory(GpgComponentType type) const -> QString;

  /**
   * @brief Set the Passphrase Cb object
   *
   * @param ctx
   * @param cb
   * @return true
   * @return false
   */
  auto SetPassphraseCb(const gpgme_ctx_t &ctx, gpgme_passphrase_cb_t cb)
      -> bool;

 protected:
  /**
   * @brief
   *
   * @param args
   * @return true
   * @return false
   */
  auto init(const OpenPGPContextInitArgs &args) -> bool override;

 private:
  gpgme_ctx_t ctx_ref_ = nullptr;             ///<
  gpgme_ctx_t binary_ctx_ref_ = nullptr;      ///<
  gpgme_ctx_t cms_ctx_ref_ = nullptr;         ///<
  gpgme_ctx_t cms_binary_ctx_ref_ = nullptr;  ///<
  std::mutex ctx_ref_lock_;
  std::mutex binary_ctx_ref_lock_;
  QString gpg_agent_path_;
  QString gpgconf_path_;
  QSharedPointer<GpgAgentProcess> agent_;
  QMap<QString, QString> component_dirs_;

  static auto set_ctx_key_list_mode(const gpgme_ctx_t &ctx) -> bool;

  auto set_ctx_openpgp_engine_info(gpgme_ctx_t ctx) -> bool;

  auto common_ctx_initialize(const gpgme_ctx_t &ctx,
                             const OpenPGPContextInitArgs &args) -> bool;

  auto cms_default_ctx_initialize(const OpenPGPContextInitArgs &args) -> bool;

  auto cms_binary_ctx_initialize(const OpenPGPContextInitArgs &args) -> bool;

  auto binary_ctx_initialize(const OpenPGPContextInitArgs &args) -> bool;

  auto default_ctx_initialize(const OpenPGPContextInitArgs &args) -> bool;

  void get_gpg_conf_dirs();

  auto kill_gpg_agent() -> bool;

  auto launch_gpg_agent() -> bool;
};

auto GF_CORE_EXPORT GpgCtx(OpenPGPContext &parent) -> GpgContext &;

}  // namespace GpgFrontend