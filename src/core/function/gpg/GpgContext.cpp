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
#include <qeventloop.h>
#include <qobject.h>
#include <unistd.h>

#include "core/function/CoreSignalStation.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/CacheUtils.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "spdlog/spdlog.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace GpgFrontend {

class GpgContext::Impl : public SingletonFunctionObject<GpgContext::Impl> {
 public:
  /**
   * Constructor
   *  Set up gpgme-context, set paths to app-run path
   */
  Impl(GpgContext *parent, const GpgContextInitArgs &args, int channel)
      : SingletonFunctionObject<GpgContext::Impl>(channel),
        parent_(parent),
        args_(args) {
    gpgme_ctx_t p_ctx;

    // get gpgme library version
    Module::UpsertRTValue("core", "gpgme.version",
                          std::string(gpgme_check_version(nullptr)));

    // create a new context
    CheckGpgError(gpgme_new(&p_ctx));
    ctx_ref_ = CtxRefHandler(p_ctx);

    if (args.custom_gpgconf && !args.custom_gpgconf_path.empty()) {
      SPDLOG_DEBUG("set custom gpgconf path: {}", args.custom_gpgconf_path);
      auto err =
          gpgme_ctx_set_engine_info(ctx_ref_.get(), GPGME_PROTOCOL_GPGCONF,
                                    args.custom_gpgconf_path.c_str(), nullptr);
      assert(CheckGpgError(err) == GPG_ERR_NO_ERROR);
    }

    // set context offline mode
    SPDLOG_DEBUG("gpg context offline mode: {}", args_.offline_mode);
    gpgme_set_offline(ctx_ref_.get(), args_.offline_mode ? 1 : 0);

    // set option auto import missing key
    // invalid at offline mode
    SPDLOG_DEBUG("gpg context auto import missing key: {}", args_.offline_mode);
    if (!args.offline_mode && args.auto_import_missing_key) {
      CheckGpgError(gpgme_set_ctx_flag(ctx_ref_.get(), "auto-key-import", "1"));
    }

    // get engine info
    auto *engine_info = gpgme_ctx_get_engine_info(*this);
    // Check ENV before running
    bool check_passed = false;
    bool find_openpgp = false;
    bool find_gpgconf = false;
    bool find_cms = false;

    while (engine_info != nullptr) {
      if (strcmp(engine_info->version, "1.0.0") == 0) {
        engine_info = engine_info->next;
        continue;
      }

      SPDLOG_DEBUG(
          "gpg context engine info: {} {} {} {}",
          gpgme_get_protocol_name(engine_info->protocol),
          std::string(engine_info->file_name == nullptr
                          ? "null"
                          : engine_info->file_name),
          std::string(engine_info->home_dir == nullptr ? "null"
                                                       : engine_info->home_dir),
          std::string(engine_info->version ? "null" : engine_info->version));

      switch (engine_info->protocol) {
        case GPGME_PROTOCOL_OpenPGP:
          find_openpgp = true;

          Module::UpsertRTValue("core", "gpgme.ctx.app_path",
                                std::string(engine_info->file_name));
          Module::UpsertRTValue("core", "gpgme.ctx.gnupg_version",
                                std::string(engine_info->version));
          Module::UpsertRTValue("core", "gpgme.ctx.database_path",
                                std::string(engine_info->home_dir == nullptr
                                                ? "default"
                                                : engine_info->home_dir));
          break;
        case GPGME_PROTOCOL_CMS:
          find_cms = true;
          Module::UpsertRTValue("core", "gpgme.ctx.cms_path",
                                std::string(engine_info->file_name));

          break;
        case GPGME_PROTOCOL_GPGCONF:
          find_gpgconf = true;
          Module::UpsertRTValue("core", "gpgme.ctx.gpgconf_path",
                                std::string(engine_info->file_name));
          break;
        case GPGME_PROTOCOL_ASSUAN:
          Module::UpsertRTValue("core", "gpgme.ctx.assuan_path",
                                std::string(engine_info->file_name));
          break;
        case GPGME_PROTOCOL_G13:
          break;
        case GPGME_PROTOCOL_UISERVER:
          break;
        case GPGME_PROTOCOL_SPAWN:
          break;
        case GPGME_PROTOCOL_DEFAULT:
          break;
        case GPGME_PROTOCOL_UNKNOWN:
          break;
      }
      engine_info = engine_info->next;
    }

    const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.gnupg_version", std::string{"0.0.0"});
    SPDLOG_DEBUG("got gnupg version from rt: {}", gnupg_version);

    // conditional check: only support gpg 2.x now
    if ((CompareSoftwareVersion(gnupg_version, "2.0.0") >= 0 && find_gpgconf &&
         find_openpgp && find_cms)) {
      check_passed = true;
    }

    if (!check_passed) {
      this->good_ = false;
      SPDLOG_ERROR("env check failed");
      return;
    }

    // speed up loading process
    gpgme_set_offline(*this, 1);

    // set keylist mode
    if (gnupg_version >= "2.0.0") {
      CheckGpgError(gpgme_set_keylist_mode(
          *this, GPGME_KEYLIST_MODE_LOCAL | GPGME_KEYLIST_MODE_WITH_SECRET |
                     GPGME_KEYLIST_MODE_SIGS |
                     GPGME_KEYLIST_MODE_SIG_NOTATIONS |
                     GPGME_KEYLIST_MODE_WITH_TOFU));
    } else {
      CheckGpgError(gpgme_set_keylist_mode(
          *this, GPGME_KEYLIST_MODE_LOCAL | GPGME_KEYLIST_MODE_SIGS |
                     GPGME_KEYLIST_MODE_SIG_NOTATIONS |
                     GPGME_KEYLIST_MODE_WITH_TOFU));
    }

    // async, init context
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
        ->PostTask(new Thread::Task(
            [=](const DataObjectPtr &) -> int {
              ctx_post_initialize();
              return 0;
            },
            "ctx_post_initialize"));

    good_ = true;
  }

  /**
   * @brief
   *
   * @return gpgme_ctx_t
   */
  operator gpgme_ctx_t() const { return ctx_ref_.get(); }

  [[nodiscard]] auto Good() const -> bool { return good_; }

  void SetPassphraseCb(gpgme_passphrase_cb_t cb) {
    const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.gnupg_version", std::string{"2.0.0"});

    if (CompareSoftwareVersion(gnupg_version, "2.1.0") >= 0) {
      if (gpgme_get_pinentry_mode(*this) != GPGME_PINENTRY_MODE_LOOPBACK) {
        gpgme_set_pinentry_mode(*this, GPGME_PINENTRY_MODE_LOOPBACK);
      }
      gpgme_set_passphrase_cb(*this, cb, reinterpret_cast<void *>(parent_));
    } else {
      SPDLOG_ERROR("not supported for gnupg version: {}", gnupg_version);
    }
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
    auto *p_ctx = static_cast<GpgContext *>(hook);
    SPDLOG_DEBUG("custom passphrase cb called, bad times: {}", last_was_bad);

    if (last_was_bad > 3) {
      SPDLOG_WARN("failure_counts is over three times");
      return gpgme_error_from_errno(GPG_ERR_CANCELED);
    }

    std::string passphrase = GetTempCacheValue("__key_passphrase");
    // no pawword is an error situation
    if (passphrase.empty()) {
      // user input passphrase
      SPDLOG_DEBUG("might need user to input passparase");

      p_ctx->ShowPasswordInputDialog();
      passphrase = GetTempCacheValue("__key_passphrase");

      SPDLOG_DEBUG("use may has inputed the passphrase");

      if (passphrase.empty()) {
        SPDLOG_ERROR("cannot get passphrase from use or passphrase is empty");

        gpgme_io_write(fd, "\n", 1);
        return gpgme_error_from_errno(GPG_ERR_CANCELED);
      }
    }

    // the user must at least write a newline character before returning from
    // the callback.
    passphrase = passphrase.append("\n");
    auto passpahrase_size = passphrase.size();

    size_t off = 0;
    size_t res = 0;
    do {
      res = gpgme_io_write(fd, &passphrase[off], passpahrase_size - off);
      if (res > 0) off += res;
    } while (res > 0 && off != passpahrase_size);

    return off == passpahrase_size ? 0
                                   : gpgme_error_from_errno(GPG_ERR_CANCELED);
  }

  static auto TestStatusCb(void *hook, const char *keyword, const char *args)
      -> gpgme_error_t {
    SPDLOG_DEBUG("keyword {}", keyword);
    return GPG_ERR_NO_ERROR;
  }

  void ShowPasswordInputDialog() {
    emit parent_->SignalNeedUserInputPassphrase();

    QEventLoop looper;
    QObject::connect(CoreSignalStation::GetInstance(),
                     &CoreSignalStation::SignalUserInputPassphraseDone, &looper,
                     &QEventLoop::quit);
    looper.exec();

    SPDLOG_DEBUG("show password input dialog done");
  }

 private:
  struct CtxRefDeleter {
    void operator()(gpgme_ctx_t _ctx) {
      if (_ctx != nullptr) gpgme_release(_ctx);
    }
  };

  using CtxRefHandler =
      std::unique_ptr<struct gpgme_context, CtxRefDeleter>;  ///<

  GpgContext *parent_;
  GpgContextInitArgs args_{};        ///<
  CtxRefHandler ctx_ref_ = nullptr;  ///<
  bool good_ = true;

  void ctx_post_initialize() {
    const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.gnupg_version", std::string{"2.0.0"});

    if (args_.ascii) {
      /** Setting the output type must be done at the beginning */
      /** think this means ascii-armor --> ? */
      gpgme_set_armor(*this, 1);
    } else {
      /** Setting the output type must be done at the beginning */
      /** think this means ascii-armor --> ? */
      gpgme_set_armor(*this, 0);
    }

    // for unit test
    if (args_.test_mode) {
      if (CompareSoftwareVersion(gnupg_version, "2.1.0") >= 0) {
        SetPassphraseCb(TestPassphraseCb);
      }
      gpgme_set_status_cb(*this, TestStatusCb, nullptr);
    }

    if (!args_.use_pinentry) {
      SetPassphraseCb(CustomPassphraseCb);
    }

    // set custom key db path
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
    }

    QObject::connect(parent_, &GpgContext::SignalNeedUserInputPassphrase,
                     CoreSignalStation::GetInstance(),
                     &CoreSignalStation::SignalNeedUserInputPassphrase);
  }
};

GpgContext::GpgContext(int channel)
    : SingletonFunctionObject<GpgContext>(channel),
      p_(std::make_unique<Impl>(this, GpgContextInitArgs{}, channel)) {}

GpgContext::GpgContext(const GpgContextInitArgs &args, int channel)
    : SingletonFunctionObject<GpgContext>(channel),
      p_(std::make_unique<Impl>(this, args, channel)) {}

auto GpgContext::Good() const -> bool { return p_->Good(); }

void GpgContext::SetPassphraseCb(gpgme_passphrase_cb_t passphrase_cb) const {
  p_->SetPassphraseCb(passphrase_cb);
}

GpgContext::operator gpgme_ctx_t() const {
  return static_cast<gpgme_ctx_t>(*p_);
}

void GpgContext::ShowPasswordInputDialog() {
  return p_->ShowPasswordInputDialog();
}

GpgContext::~GpgContext() = default;

}  // namespace GpgFrontend