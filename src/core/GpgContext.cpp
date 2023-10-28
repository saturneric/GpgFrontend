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

#include "core/GpgContext.h"

#include <gpg-error.h>
#include <gpgme.h>
#include <unistd.h>

#include "core/GpgConstants.h"
#include "core/common/CoreCommonUtil.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace GpgFrontend {

GpgContext::GpgContext(int channel)
    : SingletonFunctionObject<GpgContext>(channel) {}

/**
 * Constructor
 *  Set up gpgme-context, set paths to app-run path
 */
GpgContext::GpgContext(const GpgContextInitArgs &args) : args_(args) {
  gpgme_ctx_t _p_ctx;

  // get gpgme library version
  Module::UpsertRTValue("core", "gpgme.version",
                        std::string(gpgme_check_version(nullptr)));

  // create a new context
  check_gpg_error(gpgme_new(&_p_ctx));
  _ctx_ref = CtxRefHandler(_p_ctx);

  if (args.custom_gpgconf && !args.custom_gpgconf_path.empty()) {
    SPDLOG_DEBUG("set custom gpgconf path: {}", args.custom_gpgconf_path);
    auto err =
        gpgme_ctx_set_engine_info(_ctx_ref.get(), GPGME_PROTOCOL_GPGCONF,
                                  args.custom_gpgconf_path.c_str(), nullptr);
    assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);
  }

  // set context offline mode
  SPDLOG_DEBUG("gpg context offline mode: {}", args_.offline_mode);
  gpgme_set_offline(_ctx_ref.get(), args_.offline_mode ? 1 : 0);

  // set option auto import missing key
  // invalid at offline mode
  SPDLOG_DEBUG("gpg context auto import missing key: {}", args_.offline_mode);
  if (!args.offline_mode && args.auto_import_missing_key)
    check_gpg_error(gpgme_set_ctx_flag(_ctx_ref.get(), "auto-key-import", "1"));

  // get engine info
  auto engine_info = gpgme_ctx_get_engine_info(*this);
  // Check ENV before running
  bool check_passed = false, find_openpgp = false, find_gpgconf = false,
       find_cms = false;

  while (engine_info != nullptr) {
    if (!strcmp(engine_info->version, "1.0.0")) {
      engine_info = engine_info->next;
      continue;
    }

    SPDLOG_DEBUG(
        "gpg context engine info: {} {} {} {}",
        gpgme_get_protocol_name(engine_info->protocol),
        std::string(engine_info->file_name == nullptr ? "null"
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

  // set custom key db path
  if (!args.db_path.empty()) {
    Module::UpsertRTValue("core", "gpgme.ctx.database_path",
                          std::string(args.db_path));

    const auto app_path = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.app_path", std::string{});
    const auto database_path = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.database_path", std::string{});

    auto err =
        gpgme_ctx_set_engine_info(_ctx_ref.get(), GPGME_PROTOCOL_OpenPGP,
                                  app_path.c_str(), database_path.c_str());
    SPDLOG_DEBUG("ctx set custom key db path: {}", database_path);
    assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);
  }

  const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", std::string{"0.0.0"});
  SPDLOG_DEBUG("got gnupg version from rt: {}", gnupg_version);

  // conditional check: only support gpg 2.x now
  if ((software_version_compare(gnupg_version, "2.0.0") >= 0 && find_gpgconf &&
       find_openpgp && find_cms))
    check_passed = true;

  if (!check_passed) {
    this->good_ = false;
    SPDLOG_ERROR("env check failed");
    return;
  } else {
    // speed up loading process
    gpgme_set_offline(*this, 1);

    // set keylist mode
    if (gnupg_version >= "2.0.0") {
      check_gpg_error(gpgme_set_keylist_mode(
          *this, GPGME_KEYLIST_MODE_LOCAL | GPGME_KEYLIST_MODE_WITH_SECRET |
                     GPGME_KEYLIST_MODE_SIGS |
                     GPGME_KEYLIST_MODE_SIG_NOTATIONS |
                     GPGME_KEYLIST_MODE_WITH_TOFU));
    } else {
      check_gpg_error(gpgme_set_keylist_mode(
          *this, GPGME_KEYLIST_MODE_LOCAL | GPGME_KEYLIST_MODE_SIGS |
                     GPGME_KEYLIST_MODE_SIG_NOTATIONS |
                     GPGME_KEYLIST_MODE_WITH_TOFU));
    }

    // async, init context
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
        ->PostTask(new Thread::Task(
            [=](DataObjectPtr) -> int {
              post_init_ctx();
              return 0;
            },
            "post_init_ctx"));

    good_ = true;
  }
}

void GpgContext::post_init_ctx() {
  const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", std::string{"2.0.0"});

  // Set Independent Database
  if (software_version_compare(gnupg_version, "2.0.0") >= 0 &&
      args_.independent_database) {
    const auto app_path = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.app_path", std::string{});

    Module::UpsertRTValue("core", "gpgme.ctx.database_path",
                          std::string(args_.db_path));
    SPDLOG_DEBUG("set custom key db path to: {}", args_.db_path);

    auto err =
        gpgme_ctx_set_engine_info(_ctx_ref.get(), GPGME_PROTOCOL_OpenPGP,
                                  app_path.c_str(), args_.db_path.c_str());
    assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);
  } else {
    Module::UpsertRTValue("core", "gpgme.ctx.database_path",
                          std::string("default"));
  }

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
    if (software_version_compare(gnupg_version, "2.1.0") >= 0)
      SetPassphraseCb(test_passphrase_cb);
    gpgme_set_status_cb(*this, test_status_cb, nullptr);
  }

  // // use custom qt dialog to replace pinentry
  if (!args_.use_pinentry) {
    SetPassphraseCb(custom_passphrase_cb);
  }

  connect(this, &GpgContext::SignalNeedUserInputPassphrase,
          CoreSignalStation::GetInstance(),
          &CoreSignalStation::SignalNeedUserInputPassphrase);
}

bool GpgContext::good() const { return good_; }

void GpgContext::SetPassphraseCb(gpgme_passphrase_cb_t cb) const {
  const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", std::string{"2.0.0"});

  if (software_version_compare(gnupg_version, "2.1.0") >= 0) {
    if (gpgme_get_pinentry_mode(*this) != GPGME_PINENTRY_MODE_LOOPBACK) {
      gpgme_set_pinentry_mode(*this, GPGME_PINENTRY_MODE_LOOPBACK);
    }
    gpgme_set_passphrase_cb(*this, cb, nullptr);
  } else {
    SPDLOG_ERROR("not supported for gnupg version: {}", gnupg_version);
  }
}

gpgme_error_t GpgContext::test_passphrase_cb(void *opaque, const char *uid_hint,
                                             const char *passphrase_info,
                                             int last_was_bad, int fd) {
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

gpgme_error_t GpgContext::custom_passphrase_cb(void *opaque,
                                               const char *uid_hint,
                                               const char *passphrase_info,
                                               int last_was_bad, int fd) {
  SPDLOG_DEBUG("custom passphrase cb called, bad times: {}", last_was_bad);

  if (last_was_bad > 3) {
    SPDLOG_WARN("failure_counts is over three times");
    return gpgme_error_from_errno(GPG_ERR_CANCELED);
  }

  std::string passphrase =
      CoreCommonUtil::GetInstance()->GetTempCacheValue("__key_passphrase");
  // no pawword is an error situation
  if (passphrase.empty()) {
    // user input passphrase
    SPDLOG_DEBUG("might need user to input passparase");
    passphrase = GpgContext::GetInstance().need_user_input_passphrase();
    if (passphrase.empty()) {
      gpgme_io_write(fd, "\n", 1);
      return gpgme_error_from_errno(GPG_ERR_CANCELED);
    }
  }

  // the user must at least write a newline character before returning from the
  // callback.
  passphrase = passphrase.append("\n");
  auto passpahrase_size = passphrase.size();

  size_t off = 0, res = 0;
  do {
    res = gpgme_io_write(fd, &passphrase[off], passpahrase_size - off);
    if (res > 0) off += res;
  } while (res > 0 && off != passpahrase_size);

  return off == passpahrase_size ? 0 : gpgme_error_from_errno(GPG_ERR_CANCELED);
}

gpgme_error_t GpgContext::test_status_cb(void *hook, const char *keyword,
                                         const char *args) {
  SPDLOG_DEBUG("keyword {}", keyword);
  return GPG_ERR_NO_ERROR;
}

std::string GpgContext::need_user_input_passphrase() {
  emit SignalNeedUserInputPassphrase();

  std::string final_passphrase;
  bool input_done = false;
  SPDLOG_DEBUG("loop start to wait from user");
  auto connection =
      connect(CoreSignalStation::GetInstance(),
              &CoreSignalStation::SignalUserInputPassphraseDone, this,
              [&](QString passphrase) {
                SPDLOG_DEBUG("SignalUserInputPassphraseDone emitted");
                final_passphrase = passphrase.toStdString();
                input_done = true;
              });
  while (!input_done) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 800);
  }
  disconnect(connection);

  SPDLOG_DEBUG("lopper end");
  return final_passphrase;
}

void GpgContext::_ctx_ref_deleter::operator()(gpgme_ctx_t _ctx) {
  if (_ctx != nullptr) gpgme_release(_ctx);
}

}  // namespace GpgFrontend