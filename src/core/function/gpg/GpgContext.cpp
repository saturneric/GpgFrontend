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

#include "core/function/gpg/GpgContext.h"

#include <gpg-error.h>
#include <gpgme.h>

#include <cassert>
#include <cstddef>
#include <mutex>

#include "core/function/CoreSignalStation.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/GpgPassphraseContext.h"
#include "core/module/ModuleManager.h"
#include "core/utils/CacheUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace GpgFrontend {

class GpgContext::Impl {
 public:
  /**
   * Constructor
   *  Set up gpgme-context, set paths to app-run path
   */
  Impl(GpgContext *parent, const GpgContextInitArgs &args)
      : parent_(parent),
        args_(args),
        good_(default_ctx_initialize(args) && binary_ctx_initialize(args)) {}

  ~Impl() {
    if (ctx_ref_ != nullptr) {
      gpgme_release(ctx_ref_);
    }

    if (binary_ctx_ref_ != nullptr) {
      gpgme_release(binary_ctx_ref_);
    }
  }

  [[nodiscard]] auto BinaryContext() const -> gpgme_ctx_t {
    return binary_ctx_ref_;
  }

  [[nodiscard]] auto DefaultContext() const -> gpgme_ctx_t { return ctx_ref_; }

  [[nodiscard]] auto Good() const -> bool { return good_; }

  auto SetPassphraseCb(const gpgme_ctx_t &ctx,
                       gpgme_passphrase_cb_t cb) -> bool {
    if (gpgme_get_pinentry_mode(ctx) != GPGME_PINENTRY_MODE_LOOPBACK) {
      if (CheckGpgError(gpgme_set_pinentry_mode(
              ctx, GPGME_PINENTRY_MODE_LOOPBACK)) != GPG_ERR_NO_ERROR) {
        return false;
      }
    }
    gpgme_set_passphrase_cb(ctx, cb, reinterpret_cast<void *>(parent_));
    return true;
  }

  static auto TestPassphraseCb(void *opaque, const char *uid_hint,
                               const char *passphrase_info, int last_was_bad,
                               int fd) -> gpgme_error_t {
    QString passphrase = "abcdefg";
    auto pass_bytes = passphrase.toLatin1();
    auto pass_size = pass_bytes.size();
    const auto *p_pass_bytes = pass_bytes.constData();

    ssize_t res = 0;
    if (pass_size > 0) {
      ssize_t off = 0;
      ssize_t ret = 0;
      do {
        ret = gpgme_io_write(fd, &p_pass_bytes[off], pass_size - off);
        if (ret > 0) off += ret;
      } while (ret > 0 && off != pass_size);
      res = off;
    }

    res += gpgme_io_write(fd, "\n", 1);
    return res == pass_size + 1 ? 0 : GPG_ERR_CANCELED;
  }

  static auto CustomPassphraseCb(void *hook, const char *uid_hint,
                                 const char *passphrase_info, int prev_was_bad,
                                 int fd) -> gpgme_error_t {
    auto context_cache = GetCacheValue("PinentryContext");
    bool ask_for_new = context_cache == "NEW_PASSPHRASE";
    auto context =
        QSharedPointer<GpgPassphraseContext>(new GpgPassphraseContext(
            uid_hint != nullptr ? uid_hint : "",
            passphrase_info != nullptr ? passphrase_info : "",
            prev_was_bad != 0, ask_for_new));

    LOG_D() << "custom passphrase cb called, uid: "
            << (uid_hint == nullptr ? "<empty>" : QString{uid_hint})
            << ", info: "
            << (passphrase_info == nullptr ? "<empty>"
                                           : QString{passphrase_info})
            << ", last_was_bad: " << prev_was_bad;

    QEventLoop looper;
    QString passphrase = "";

    Module::TriggerEvent(
        "REQUEST_PIN_ENTRY",
        {{"uid_hint", uid_hint != nullptr ? uid_hint : ""},
         {"passphrase_info", passphrase_info != nullptr ? passphrase_info : ""},
         {"prev_was_bad", (prev_was_bad != 0) ? "1" : "0"},
         {"ask_for_new", ask_for_new ? "1" : "0"}},
        [&passphrase, &looper](Module::EventIdentifier i,
                               Module::Event::ListenerIdentifier ei,
                               Module::Event::Params p) {
          if (p["ret"] == "0") passphrase = p["passphrase"];
          looper.quit();
        });

    looper.exec();
    ResetCacheValue("PinentryContext");

    LOG_D() << "passphrase size:" << passphrase.size();

    // empty passphrase is not allowed
    if (passphrase.isEmpty()) return GPG_ERR_CANCELED;

    auto pass_bytes = passphrase.toLatin1();
    auto pass_size = pass_bytes.size();
    const auto *p_pass_bytes = pass_bytes.constData();

    ssize_t res = 0;
    if (pass_size > 0) {
      ssize_t off = 0;
      ssize_t ret = 0;
      do {
        ret = gpgme_io_write(fd, &p_pass_bytes[off], pass_size - off);
        if (ret > 0) off += ret;
      } while (ret > 0 && off != pass_size);
      res = off;
    }

    res += gpgme_io_write(fd, "\n", 1);
    return res == pass_size + 1 ? 0 : GPG_ERR_CANCELED;
  }

  static auto TestStatusCb(void *hook, const char *keyword,
                           const char *args) -> gpgme_error_t {
    FLOG_D("keyword %s", keyword);
    return GPG_ERR_NO_ERROR;
  }

 private:
  GpgContext *parent_;
  GpgContextInitArgs args_{};             ///<
  gpgme_ctx_t ctx_ref_ = nullptr;         ///<
  gpgme_ctx_t binary_ctx_ref_ = nullptr;  ///<
  bool good_ = true;
  std::mutex ctx_ref_lock_;
  std::mutex binary_ctx_ref_lock_;

  static auto set_ctx_key_list_mode(const gpgme_ctx_t &ctx) -> bool {
    assert(ctx != nullptr);

    const auto gpgme_version = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.version", QString{"0.0.0"});
    LOG_D() << "got gpgme version version from rt: " << gpgme_version;

    if (gpgme_get_keylist_mode(ctx) == 0) {
      FLOG_W("ctx is not a valid pointer, reported by gpgme_get_keylist_mode");
      return false;
    }

    // set keylist mode
    return CheckGpgError(gpgme_set_keylist_mode(
               ctx, GPGME_KEYLIST_MODE_LOCAL | GPGME_KEYLIST_MODE_WITH_SECRET |
                        GPGME_KEYLIST_MODE_SIGS |
                        GPGME_KEYLIST_MODE_SIG_NOTATIONS |
                        GPGME_KEYLIST_MODE_WITH_TOFU)) == GPG_ERR_NO_ERROR;
  }

  static auto set_ctx_openpgp_engine_info(gpgme_ctx_t ctx) -> bool {
    const auto app_path = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.app_path", QString{});
    const auto database_path = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.database_path", QString{});

    LOG_D() << "ctx set engine info, db path: " << database_path
            << ", app path: " << app_path;

    auto app_path_buffer = app_path.toUtf8();
    auto database_path_buffer = database_path.toUtf8();

    auto err = gpgme_ctx_set_engine_info(
        ctx, gpgme_get_protocol(ctx),
        app_path.isEmpty() ? nullptr : app_path_buffer,
        database_path.isEmpty() ? nullptr : database_path_buffer);

    assert(CheckGpgError(err) == GPG_ERR_NO_ERROR);
    return CheckGpgError(err) == GPG_ERR_NO_ERROR;

    return true;
  }

  auto common_ctx_initialize(const gpgme_ctx_t &ctx,
                             const GpgContextInitArgs &args) -> bool {
    assert(ctx != nullptr);

    if (!args.gpgconf_path.isEmpty()) {
      LOG_D() << "set custom gpgconf path: " << args.gpgconf_path;
      auto err = gpgme_ctx_set_engine_info(ctx, GPGME_PROTOCOL_GPGCONF,
                                           args.gpgconf_path.toUtf8(), nullptr);

      if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
        LOG_W() << "set gpg context engine info error: "
                << DescribeGpgErrCode(err).second;
        return false;
      }
    }

    // set context offline mode
    FLOG_D("gpg context: offline mode: %d", args_.offline_mode);
    FLOG_D("gpg context: auto import missing key: %d",
           args_.auto_import_missing_key);
    gpgme_set_offline(ctx, args_.offline_mode ? 1 : 0);

    // set option auto import missing key
    if (!args_.offline_mode && args.auto_import_missing_key) {
      if (CheckGpgError(gpgme_set_ctx_flag(ctx, "auto-key-retrieve", "1")) !=
          GPG_ERR_NO_ERROR) {
        return false;
      }
    }

    if (!set_ctx_key_list_mode(ctx)) {
      FLOG_D("set ctx key list mode failed");
      return false;
    }

    // for unit test
    if (args_.test_mode) {
      if (!SetPassphraseCb(ctx, TestPassphraseCb)) {
        FLOG_W("set passphrase cb failed, test");
        return false;
      };
    } else if (!args_.use_pinentry &&
               Module::IsModuleActivate(kPinentryModuleID)) {
      if (!SetPassphraseCb(ctx, CustomPassphraseCb)) {
        FLOG_D("set passphrase cb failed, custom");
        return false;
      }
    }

    // set custom gpg key db path
    if (!args_.db_path.isEmpty()) {
      LOG_D() << "set context database path to" << args_.db_path;
      Module::UpsertRTValue("core", "gpgme.ctx.database_path", args_.db_path);
    }

    if (!set_ctx_openpgp_engine_info(ctx)) {
      FLOG_W("set gpgme context openpgp engine info failed");
      return false;
    }

    return true;
  }

  auto binary_ctx_initialize(const GpgContextInitArgs &args) -> bool {
    gpgme_ctx_t p_ctx;
    if (auto err = CheckGpgError(gpgme_new(&p_ctx)); err != GPG_ERR_NO_ERROR) {
      LOG_W() << "get new gpg context error: "
              << DescribeGpgErrCode(err).second;
      return false;
    }
    assert(p_ctx != nullptr);
    binary_ctx_ref_ = p_ctx;

    if (!common_ctx_initialize(binary_ctx_ref_, args)) {
      FLOG_W("get new ctx failed, binary");
      return false;
    }

    gpgme_set_armor(binary_ctx_ref_, 0);
    return true;
  }

  auto default_ctx_initialize(const GpgContextInitArgs &args) -> bool {
    gpgme_ctx_t p_ctx;
    if (CheckGpgError(gpgme_new(&p_ctx)) != GPG_ERR_NO_ERROR) {
      FLOG_W("get new ctx failed, default");
      return false;
    }
    assert(p_ctx != nullptr);
    ctx_ref_ = p_ctx;

    if (!common_ctx_initialize(ctx_ref_, args)) {
      return false;
    }

    gpgme_set_armor(ctx_ref_, 1);
    return true;
  }
};

GpgContext::GpgContext(int channel)
    : SingletonFunctionObject<GpgContext>(channel),
      p_(SecureCreateUniqueObject<Impl>(this, GpgContextInitArgs{})) {}

GpgContext::GpgContext(GpgContextInitArgs args, int channel)
    : SingletonFunctionObject<GpgContext>(channel),
      p_(SecureCreateUniqueObject<Impl>(this, args)) {}

auto GpgContext::Good() const -> bool { return p_->Good(); }

auto GpgContext::BinaryContext() -> gpgme_ctx_t { return p_->BinaryContext(); }

auto GpgContext::DefaultContext() -> gpgme_ctx_t {
  return p_->DefaultContext();
}

GpgContext::~GpgContext() = default;

}  // namespace GpgFrontend