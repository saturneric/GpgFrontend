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

#include "core/function/gpg/GpgContext.h"

#include <gpg-error.h>
#include <gpgme.h>

#include <cassert>

#include "core/function/CoreSignalStation.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/module/ModuleManager.h"
#include "core/utils/GpgUtils.h"
#include "spdlog/spdlog.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace GpgFrontend {

struct CtxRefDeleter {
  void operator()(gpgme_ctx_t _ctx) {
    if (_ctx != nullptr) gpgme_release(_ctx);
  }
};

using CtxRefHandler =
    std::unique_ptr<struct gpgme_context, CtxRefDeleter>;  ///<

class GpgContext::Impl : public SingletonFunctionObject<GpgContext::Impl> {
 public:
  /**
   * Constructor
   *  Set up gpgme-context, set paths to app-run path
   */
  Impl(GpgContext *parent, const GpgContextInitArgs &args, int channel)
      : SingletonFunctionObject<GpgContext::Impl>(channel),
        parent_(parent),
        args_(args),
        good_(default_ctx_initialize(args) && binary_ctx_initialize(args)) {}

  [[nodiscard]] auto BinaryContext() const -> gpgme_ctx_t {
    return binary_ctx_ref_.get();
  }

  [[nodiscard]] auto DefaultContext() const -> gpgme_ctx_t {
    return ctx_ref_.get();
  }

  [[nodiscard]] auto Good() const -> bool { return good_; }

  auto SetPassphraseCb(const CtxRefHandler &ctx, gpgme_passphrase_cb_t cb)
      -> bool {
    if (gpgme_get_pinentry_mode(ctx.get()) != GPGME_PINENTRY_MODE_LOOPBACK) {
      if (CheckGpgError(gpgme_set_pinentry_mode(
              ctx.get(), GPGME_PINENTRY_MODE_LOOPBACK)) != GPG_ERR_NO_ERROR) {
        return false;
      }
    }
    gpgme_set_passphrase_cb(ctx.get(), cb, reinterpret_cast<void *>(parent_));
    return true;
  }

  static auto TestPassphraseCb(void *opaque, const char *uid_hint,
                               const char *passphrase_info, int last_was_bad,
                               int fd) -> gpgme_error_t {
    size_t res;
    std::string pass = "abcdefg\n";
    auto pass_len = pass.size();

    size_t off = 0;

    do {
      res = gpgme_io_write(fd, &pass[off], pass_len - off);
      if (res > 0) off += res;
    } while (res > 0 && off != pass_len);

    return off == pass_len ? 0 : gpgme_error_from_errno(errno);
  }

  static auto CustomPassphraseCb(void *hook, const char *uid_hint,
                                 const char *passphrase_info, int last_was_bad,
                                 int fd) -> gpgme_error_t {
    std::string passphrase;

    SPDLOG_DEBUG(
        "custom passphrase cb called, uid: {}, info: {}, last_was_bad: {}",
        uid_hint == nullptr ? "<empty>" : std::string{uid_hint},
        passphrase_info == nullptr ? "<empty>" : std::string{passphrase_info},
        last_was_bad);

    emit CoreSignalStation::GetInstance()->SignalNeedUserInputPassphrase();

    QEventLoop looper;
    QObject::connect(
        CoreSignalStation::GetInstance(),
        &CoreSignalStation::SignalUserInputPassphraseCallback, &looper,
        [&](const QByteArray &buffer) { passphrase = buffer.toStdString(); });
    QObject::connect(CoreSignalStation::GetInstance(),
                     &CoreSignalStation::SignalUserInputPassphraseCallback,
                     &looper, &QEventLoop::quit);
    looper.exec();

    auto passpahrase_size = passphrase.size();
    SPDLOG_DEBUG("get passphrase from pinentry size: {}", passpahrase_size);

    size_t off = 0;
    size_t res = 0;
    do {
      res = gpgme_io_write(fd, &passphrase[off], passpahrase_size - off);
      if (res > 0) off += res;
    } while (res > 0 && off != passpahrase_size);

    res += gpgme_io_write(fd, "\n", 1);

    SPDLOG_DEBUG("custom passphrase cd is about to return, res: {}", res);
    return res == passpahrase_size + 1
               ? 0
               : gpgme_error_from_errno(GPG_ERR_CANCELED);
  }

  static auto TestStatusCb(void *hook, const char *keyword, const char *args)
      -> gpgme_error_t {
    SPDLOG_DEBUG("keyword {}", keyword);
    return GPG_ERR_NO_ERROR;
  }

 private:
  GpgContext *parent_;
  GpgContextInitArgs args_{};               ///<
  CtxRefHandler ctx_ref_ = nullptr;         ///<
  CtxRefHandler binary_ctx_ref_ = nullptr;  ///<
  bool good_ = true;

  static auto set_ctx_key_list_mode(const CtxRefHandler &ctx) -> bool {
    assert(ctx.get() != nullptr);

    const auto gpgme_version = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.version", std::string{"0.0.0"});
    SPDLOG_DEBUG("got gpgme version version from rt: {}", gpgme_version);

    if (gpgme_get_keylist_mode(ctx.get()) == 0) {
      SPDLOG_ERROR(
          "ctx is not a valid pointer, reported by gpgme_get_keylist_mode");
      return false;
    }

    // set keylist mode
    return CheckGpgError(gpgme_set_keylist_mode(
               ctx.get(),
               GPGME_KEYLIST_MODE_LOCAL | GPGME_KEYLIST_MODE_WITH_SECRET |
                   GPGME_KEYLIST_MODE_SIGS | GPGME_KEYLIST_MODE_SIG_NOTATIONS |
                   GPGME_KEYLIST_MODE_WITH_TOFU)) == GPG_ERR_NO_ERROR;
  }

  auto common_ctx_initialize(const CtxRefHandler &ctx,
                             const GpgContextInitArgs &args) -> bool {
    assert(ctx.get() != nullptr);

    if (args.custom_gpgconf && !args.custom_gpgconf_path.empty()) {
      SPDLOG_DEBUG("set custom gpgconf path: {}", args.custom_gpgconf_path);
      auto err =
          gpgme_ctx_set_engine_info(ctx.get(), GPGME_PROTOCOL_GPGCONF,
                                    args.custom_gpgconf_path.c_str(), nullptr);

      assert(CheckGpgError(err) == GPG_ERR_NO_ERROR);
      if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
        return false;
      }
    }

    // set context offline mode
    SPDLOG_DEBUG("gpg context offline mode: {}", args_.offline_mode);
    gpgme_set_offline(ctx.get(), args_.offline_mode ? 1 : 0);

    // set option auto import missing key
    // invalid at offline mode
    SPDLOG_DEBUG("gpg context auto import missing key: {}", args_.offline_mode);
    if (!args.offline_mode && args.auto_import_missing_key) {
      if (CheckGpgError(gpgme_set_ctx_flag(ctx.get(), "auto-key-import",
                                           "1")) != GPG_ERR_NO_ERROR) {
        return false;
      }
    }

    if (!set_ctx_key_list_mode(ctx)) {
      SPDLOG_DEBUG("set ctx key list mode failed");
      return false;
    }

    // for unit test
    if (args_.test_mode) {
      if (!SetPassphraseCb(ctx, TestPassphraseCb)) {
        SPDLOG_ERROR("set passphrase cb failed, test");
        return false;
      };
    } else if (!args_.use_pinentry) {
      if (!SetPassphraseCb(ctx, CustomPassphraseCb)) {
        SPDLOG_DEBUG("set passphrase cb failed, custom");
        return false;
      }
    }

    // set custom gpg key db path
    if (!args_.db_path.empty()) {
      Module::UpsertRTValue("core", "gpgme.ctx.database_path",
                            std::string(args_.db_path));

      const auto app_path = Module::RetrieveRTValueTypedOrDefault<>(
          "core", "gpgme.ctx.app_path", std::string{});
      const auto database_path = Module::RetrieveRTValueTypedOrDefault<>(
          "core", "gpgme.ctx.database_path", std::string{});

      auto err =
          gpgme_ctx_set_engine_info(ctx_ref_.get(), GPGME_PROTOCOL_OpenPGP,
                                    app_path.c_str(), database_path.c_str());
      SPDLOG_DEBUG("ctx set custom key db path: {}", database_path);

      assert(CheckGpgError(err) == GPG_ERR_NO_ERROR);
      if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
        return false;
      }
    }

    return true;
  }

  auto binary_ctx_initialize(const GpgContextInitArgs &args) -> bool {
    gpgme_ctx_t p_ctx;
    if (CheckGpgError(gpgme_new(&p_ctx)) != GPG_ERR_NO_ERROR) {
      return false;
    }
    assert(p_ctx != nullptr);
    binary_ctx_ref_ = CtxRefHandler(p_ctx);

    if (!common_ctx_initialize(binary_ctx_ref_, args)) {
      SPDLOG_ERROR("get new ctx failed, binary");
      return false;
    }

    /** Setting the output type must be done at the beginning */
    /** think this means ascii-armor --> ? */
    gpgme_set_armor(binary_ctx_ref_.get(), 0);
    return true;
  }

  auto default_ctx_initialize(const GpgContextInitArgs &args) -> bool {
    gpgme_ctx_t p_ctx;
    if (CheckGpgError(gpgme_new(&p_ctx)) != GPG_ERR_NO_ERROR) {
      SPDLOG_ERROR("get new ctx failed, default");
      return false;
    }
    assert(p_ctx != nullptr);
    ctx_ref_ = CtxRefHandler(p_ctx);

    if (!common_ctx_initialize(ctx_ref_, args)) {
      return false;
    }

    gpgme_set_armor(ctx_ref_.get(), 1);
    return true;
  }
};

GpgContext::GpgContext(int channel)
    : SingletonFunctionObject<GpgContext>(channel),
      p_(std::make_unique<Impl>(this, GpgContextInitArgs{}, channel)) {}

GpgContext::GpgContext(const GpgContextInitArgs &args, int channel)
    : SingletonFunctionObject<GpgContext>(channel),
      p_(std::make_unique<Impl>(this, args, channel)) {}

auto GpgContext::Good() const -> bool { return p_->Good(); }

auto GpgContext::BinaryContext() -> gpgme_ctx_t { return p_->BinaryContext(); }

auto GpgContext::DefaultContext() -> gpgme_ctx_t {
  return p_->DefaultContext();
}

GpgContext::~GpgContext() = default;

}  // namespace GpgFrontend