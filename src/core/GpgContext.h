/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef __SGPGMEPP_CONTEXT_H__
#define __SGPGMEPP_CONTEXT_H__

#include <optional>
#include <string>

#include "GpgConstants.h"
#include "GpgFunctionObject.h"
#include "GpgInfo.h"
#include "GpgModel.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
struct GpgContextInitArgs {
  // make no sense for gpg2
  bool independent_database = false;  ///<
  std::string db_path = {};

  bool gpg_alone = false;
  std::string gpg_path = {};

  bool test_mode = false;
  bool ascii = true;
  bool offline_mode = false;
  bool auto_import_missing_key = false;

  bool custom_gpgconf = false;
  std::string custom_gpgconf_path;

  GpgContextInitArgs() = default;
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
  /**
   * @brief Construct a new Gpg Context object
   *
   * @param args
   */
  explicit GpgContext(const GpgContextInitArgs& args = {});

  /**
   * @brief Construct a new Gpg Context object
   *
   * @param channel
   */
  explicit GpgContext(int channel);

  /**
   * @brief Destroy the Gpg Context object
   *
   */
  ~GpgContext() override = default;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool good() const;

  /**
   * @brief Get the Info object
   *
   * @return const GpgInfo&
   */
  [[nodiscard]] const GpgInfo& GetInfo(bool refresh = false);

  /**
   * @brief
   *
   * @return gpgme_ctx_t
   */
  operator gpgme_ctx_t() const { return _ctx_ref.get(); }

 private:
  GpgInfo info_{};             ///<
  GpgContextInitArgs args_{};  ///<
  bool extend_info_loaded_ = false;
  std::shared_mutex preload_lock_{};

  /**
   * @brief
   *
   */
  void post_init_ctx();

  /**
   * @brief
   *
   * @return std::string
   */
  std::string need_user_input_passphrase();

  /**
   * @brief Construct a new std::check component existence object
   *
   */
  std::optional<std::string> check_binary_chacksum(std::filesystem::path);

  /**
   * @brief
   *
   */
  struct _ctx_ref_deleter {
    void operator()(gpgme_ctx_t _ctx);
  };

  using CtxRefHandler =
      std::unique_ptr<struct gpgme_context, _ctx_ref_deleter>;  ///<
  CtxRefHandler _ctx_ref = nullptr;                             ///<
  bool good_ = true;                                            ///<

 signals:
  /**
   * @brief
   *
   */
  void SignalNeedUserInputPassphrase();

 public:
  /**
   * @brief
   *
   * @param opaque
   * @param uid_hint
   * @param passphrase_info
   * @param last_was_bad
   * @param fd
   * @return gpgme_error_t
   */
  static gpgme_error_t test_passphrase_cb(void* opaque, const char* uid_hint,
                                          const char* passphrase_info,
                                          int last_was_bad, int fd);

  /**
   * @brief
   *
   * @param opaque
   * @param uid_hint
   * @param passphrase_info
   * @param last_was_bad
   * @param fd
   * @return gpgme_error_t
   */
  static gpgme_error_t custom_passphrase_cb(void* opaque, const char* uid_hint,
                                            const char* passphrase_info,
                                            int last_was_bad, int fd);

  /**
   * @brief
   *
   * @param hook
   * @param keyword
   * @param args
   * @return gpgme_error_t
   */
  static gpgme_error_t test_status_cb(void* hook, const char* keyword,
                                      const char* args);

  /**
   * @brief Set the Passphrase Cb object
   *
   * @param func
   */
  void SetPassphraseCb(gpgme_passphrase_cb_t func) const;
};
}  // namespace GpgFrontend

#endif  // __SGPGMEPP_CONTEXT_H__