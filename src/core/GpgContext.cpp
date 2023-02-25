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

#include "core/GpgContext.h"

#include <gpg-error.h>
#include <gpgme.h>
#include <unistd.h>

#include <mutex>
#include <shared_mutex>
#include <string>

#include "core/GpgConstants.h"
#include "core/GpgModel.h"
#include "core/common/CoreCommonUtil.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/thread/TaskRunnerGetter.h"
#include "thread/Task.h"

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
  static bool _first = true;

  if (_first) {
    /* Initialize the locale environment. */
    SPDLOG_DEBUG("locale: {}", setlocale(LC_CTYPE, nullptr));
    info_.GpgMEVersion = gpgme_check_version(nullptr);
    gpgme_set_locale(nullptr, LC_CTYPE, setlocale(LC_CTYPE, nullptr));
#ifdef LC_MESSAGES
    gpgme_set_locale(nullptr, LC_MESSAGES, setlocale(LC_MESSAGES, nullptr));
#endif
    _first = false;
  }

  gpgme_ctx_t _p_ctx;
  check_gpg_error(gpgme_new(&_p_ctx));
  _ctx_ref = CtxRefHandler(_p_ctx);

  if (args.gpg_alone) {
    info_.AppPath = args.gpg_path;
    auto err = gpgme_ctx_set_engine_info(_ctx_ref.get(), GPGME_PROTOCOL_OpenPGP,
                                         info_.AppPath.c_str(),
                                         info_.DatabasePath.c_str());
    assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);
  }

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
        info_.AppPath = engine_info->file_name;
        info_.GnupgVersion = engine_info->version;
        info_.DatabasePath = std::string(engine_info->home_dir == nullptr
                                             ? "default"
                                             : engine_info->home_dir);
        break;
      case GPGME_PROTOCOL_CMS:
        find_cms = true;
        info_.CMSPath = engine_info->file_name;
        break;
      case GPGME_PROTOCOL_GPGCONF:
        find_gpgconf = true;
        info_.GpgConfPath = engine_info->file_name;
        break;
      case GPGME_PROTOCOL_ASSUAN:
        info_.AssuanPath = engine_info->file_name;
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
    info_.DatabasePath = args.db_path;
    auto err = gpgme_ctx_set_engine_info(_ctx_ref.get(), GPGME_PROTOCOL_OpenPGP,
                                         info_.AppPath.c_str(),
                                         info_.DatabasePath.c_str());
    SPDLOG_DEBUG("ctx set custom key db path: {}", info_.DatabasePath);
    assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);
  }

  // conditional check
  if ((info_.GnupgVersion >= "2.0.0" && find_gpgconf && find_openpgp &&
       find_cms) ||
      (info_.GnupgVersion > "1.0.0" && find_gpgconf))
    check_passed = true;

  if (!check_passed) {
    this->good_ = false;
    SPDLOG_ERROR("env check failed");
    return;
  } else {
    // async, init context
    Thread::TaskRunnerGetter::GetInstance()
        .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
        ->PostTask(new Thread::Task(
            [=](Thread::Task::DataObjectPtr) -> int {
              post_init_ctx();
              return 0;
            },
            "post_init_ctx"));

    good_ = true;
  }
}

void GpgContext::post_init_ctx() {
  // Set Independent Database
  if (info_.GnupgVersion <= "2.0.0" && args_.independent_database) {
    info_.DatabasePath = args_.db_path;
    SPDLOG_DEBUG("custom key db path {}", info_.DatabasePath);
    auto err = gpgme_ctx_set_engine_info(_ctx_ref.get(), GPGME_PROTOCOL_OpenPGP,
                                         info_.AppPath.c_str(),
                                         info_.DatabasePath.c_str());
    assert(check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR);
  } else {
    info_.DatabasePath = "default";
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

  // Speed up loading process
  gpgme_set_offline(*this, 1);

  if (info_.GnupgVersion >= "2.0.0") {
    check_gpg_error(gpgme_set_keylist_mode(
        *this, GPGME_KEYLIST_MODE_LOCAL | GPGME_KEYLIST_MODE_WITH_SECRET |
                   GPGME_KEYLIST_MODE_SIGS | GPGME_KEYLIST_MODE_SIG_NOTATIONS |
                   GPGME_KEYLIST_MODE_WITH_TOFU));
  } else {
    check_gpg_error(gpgme_set_keylist_mode(
        *this, GPGME_KEYLIST_MODE_LOCAL | GPGME_KEYLIST_MODE_SIGS |
                   GPGME_KEYLIST_MODE_SIG_NOTATIONS |
                   GPGME_KEYLIST_MODE_WITH_TOFU));
  }

  // for unit test
  if (args_.test_mode) {
    if (info_.GnupgVersion >= "2.1.0") SetPassphraseCb(test_passphrase_cb);
    gpgme_set_status_cb(*this, test_status_cb, nullptr);
  }

  // preload info
  auto &info = GetInfo();

  // listen passphrase input event
  SetPassphraseCb(custom_passphrase_cb);
  connect(this, &GpgContext::SignalNeedUserInputPassphrase,
          CoreSignalStation::GetInstance(),
          &CoreSignalStation::SignalNeedUserInputPassphrase);
}

bool GpgContext::good() const { return good_; }

void GpgContext::SetPassphraseCb(gpgme_passphrase_cb_t cb) const {
  if (info_.GnupgVersion >= "2.1.0") {
    if (gpgme_get_pinentry_mode(*this) != GPGME_PINENTRY_MODE_LOOPBACK) {
      gpgme_set_pinentry_mode(*this, GPGME_PINENTRY_MODE_LOOPBACK);
    }
    gpgme_set_passphrase_cb(*this, cb, nullptr);
  } else {
    SPDLOG_ERROR("not supported for gnupg version: {}", info_.GnupgVersion);
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

const GpgInfo &GpgContext::GetInfo(bool refresh) {
  if (!extend_info_loaded_ || refresh) {
    // try lock
    std::unique_lock lock(preload_lock_);

    // check twice
    if (extend_info_loaded_ && !refresh) return info_;

    SPDLOG_DEBUG("start to load extra info");

    // get all components
    GpgCommandExecutor::GetInstance().Execute(
        info_.GpgConfPath, {"--list-components"},
        [=](int exit_code, const std::string &p_out, const std::string &p_err) {
          SPDLOG_DEBUG(
              "gpgconf components exit_code: {} process stdout size: {}",
              exit_code, p_out.size());

          if (exit_code != 0) {
            SPDLOG_ERROR(
                "gpgconf execute error, process stderr: {} ,process stdout: "
                "{}",
                p_err, p_out);
            return;
          }

          auto &components_info = info_.ComponentsInfo;
          components_info["gpgme"] = {"GPG Made Easy", info_.GpgMEVersion,
                                      _("Embedded In"), "/"};

          auto gpgconf_binary_checksum =
              check_binary_chacksum(info_.GpgConfPath);
          components_info["gpgconf"] = {"GPG Configure", "/", info_.GpgConfPath,
                                        gpgconf_binary_checksum.has_value()
                                            ? gpgconf_binary_checksum.value()
                                            : "/"};

          std::vector<std::string> line_split_list;
          boost::split(line_split_list, p_out, boost::is_any_of("\n"));

          for (const auto &line : line_split_list) {
            std::vector<std::string> info_split_list;
            boost::split(info_split_list, line, boost::is_any_of(":"));

            if (info_split_list.size() != 3) continue;

            auto component_name = info_split_list[0];
            auto component_desc = info_split_list[1];
            auto component_path = info_split_list[2];
            auto binary_checksum = check_binary_chacksum(component_path);

            SPDLOG_DEBUG(
                "gnupg component name: {} desc: {} checksum: {} path: {} ",
                component_name, component_desc,
                binary_checksum.has_value() ? binary_checksum.value() : "/",
                component_path);

            std::string version = "/";

            if (component_name == "gpg") {
              version = info_.GnupgVersion;
            }
            if (component_name == "gpg-agent") {
              info_.GpgAgentPath = info_split_list[2];
            }
            if (component_name == "dirmngr") {
              info_.DirmngrPath = info_split_list[2];
            }
            if (component_name == "keyboxd") {
              info_.KeyboxdPath = info_split_list[2];
            }

            {
              // try lock
              std::unique_lock lock(info_.Lock);
              // add component info to list
              components_info[component_name] = {
                  component_desc, version, component_path,
                  binary_checksum.has_value() ? binary_checksum.value() : "/"};
            }
          }
        });

    SPDLOG_DEBUG("start to get dirs info");

    GpgCommandExecutor::GetInstance().ExecuteConcurrently(
        info_.GpgConfPath, {"--list-dirs"},
        [=](int exit_code, const std::string &p_out, const std::string &p_err) {
          SPDLOG_DEBUG(
              "gpgconf configurations exit_code: {} process stdout size: {}",
              exit_code, p_out.size());

          if (exit_code != 0) {
            SPDLOG_ERROR(
                "gpgconf execute error, process stderr: {} process stdout: "
                "{}",
                p_err, p_out);
            return;
          }

          auto &configurations_info = info_.ConfigurationsInfo;

          std::vector<std::string> line_split_list;
          boost::split(line_split_list, p_out, boost::is_any_of("\n"));

          for (const auto &line : line_split_list) {
            std::vector<std::string> info_split_list;
            boost::split(info_split_list, line, boost::is_any_of(":"));
            SPDLOG_DEBUG("gpgconf info line: {} info size: {}", line,
                         info_split_list.size());

            if (info_split_list.size() != 2) continue;

            // record gnupg home path
            if (info_split_list[0] == "homedir") {
              info_.GnuPGHomePath = info_split_list[1];
            }

            auto configuration_name = info_split_list[0];
            {
              // try lock
              std::unique_lock lock(info_.Lock);
              configurations_info[configuration_name] = {info_split_list[1]};
            }
          }
        });

    SPDLOG_DEBUG("start to get components info");

    for (const auto &component : info_.ComponentsInfo) {
      SPDLOG_DEBUG("gpgconf check options ready", "component", component.first);

      if (component.first == "gpgme" || component.first == "gpgconf") continue;

      GpgCommandExecutor::GetInstance().ExecuteConcurrently(
          info_.GpgConfPath, {"--check-options", component.first},
          [=](int exit_code, const std::string &p_out,
              const std::string &p_err) {
            SPDLOG_DEBUG(
                "gpgconf {} options exit_code: {} process stdout "
                "size: {} ",
                component.first, exit_code, p_out.size());

            if (exit_code != 0) {
              SPDLOG_ERROR(
                  "gpgconf {} options execute error, process "
                  "stderr: {} , process stdout:",
                  component.first, p_err, p_out);
              return;
            }

            auto &options_info = info_.OptionsInfo;

            std::vector<std::string> line_split_list;
            boost::split(line_split_list, p_out, boost::is_any_of("\n"));

            for (const auto &line : line_split_list) {
              std::vector<std::string> info_split_list;
              boost::split(info_split_list, line, boost::is_any_of(":"));

              SPDLOG_DEBUG("component {} options line: {} info size: {}",
                           component.first, line, info_split_list.size());

              if (info_split_list.size() != 6) continue;

              auto configuration_name = info_split_list[0];
              {
                // try lock
                std::unique_lock lock(info_.Lock);
                options_info[configuration_name] = {
                    info_split_list[1], info_split_list[2], info_split_list[3],
                    info_split_list[4], info_split_list[5]};
              }
            }
          });
    }

    SPDLOG_DEBUG("start to get avaliable component options info");

    for (const auto &component : info_.ComponentsInfo) {
      SPDLOG_DEBUG("gpgconf list options ready", "component", component.first);

      if (component.first == "gpgme" || component.first == "gpgconf") continue;

      GpgCommandExecutor::GetInstance().ExecuteConcurrently(
          info_.GpgConfPath, {"--list-options", component.first},
          [=](int exit_code, const std::string &p_out,
              const std::string &p_err) {
            SPDLOG_DEBUG(
                "gpgconf {} avaliable options exit_code: {} process stdout "
                "size: {} ",
                component.first, exit_code, p_out.size());

            if (exit_code != 0) {
              SPDLOG_ERROR(
                  "gpgconf {} avaliable options execute error, process stderr: "
                  "{} , process stdout:",
                  component.first, p_err, p_out);
              return;
            }

            auto &available_options_info = info_.AvailableOptionsInfo;

            std::vector<std::string> line_split_list;
            boost::split(line_split_list, p_out, boost::is_any_of("\n"));

            for (const auto &line : line_split_list) {
              std::vector<std::string> info_split_list;
              boost::split(info_split_list, line, boost::is_any_of(":"));

              SPDLOG_DEBUG(
                  "component {} avaliable options line: {} info size: {}",
                  component.first, line, info_split_list.size());

              if (info_split_list.size() != 10) continue;

              auto configuration_name = info_split_list[0];
              {
                // try lock
                std::unique_lock lock(info_.Lock);
                available_options_info[configuration_name] = {
                    info_split_list[1], info_split_list[2], info_split_list[3],
                    info_split_list[4], info_split_list[5], info_split_list[6],
                    info_split_list[7], info_split_list[8], info_split_list[9]};
              }
            }
          });
    }
    extend_info_loaded_ = true;
  }

  // ensure nothing is changing now
  std::shared_lock lock(preload_lock_);
  return info_;
}

std::optional<std::string> GpgContext::check_binary_chacksum(
    std::filesystem::path path) {
  QFile f(QString::fromStdString(path.u8string()));
  if (!f.open(QFile::ReadOnly)) return {};

  // read all data from file
  auto buffer = f.readAll();
  f.close();

  auto hash_md5 = QCryptographicHash(QCryptographicHash::Md5);
  // md5
  hash_md5.addData(buffer);
  auto md5 = hash_md5.result().toHex().toStdString();
  SPDLOG_DEBUG("md5 {}", md5);

  return md5.substr(0, 6);
}

void GpgContext::_ctx_ref_deleter::operator()(gpgme_ctx_t _ctx) {
  if (_ctx != nullptr) gpgme_release(_ctx);
}

}  // namespace GpgFrontend