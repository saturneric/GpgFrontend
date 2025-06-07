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
#include <mutex>

#include "core/function/CoreSignalStation.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/model/GpgPassphraseContext.h"
#include "core/module/ModuleManager.h"
#include "core/utils/CacheUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"

#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#endif

namespace GpgFrontend {

class GpgAgentProcess {
 public:
  explicit GpgAgentProcess(int channel, QString gpg_agent_path, QString db_path)
      : channel_(channel),
        db_path_(std::move(db_path)),
        gpg_agent_path_(std::move(gpg_agent_path)) {}

  auto Start() -> bool {
    assert(!gpg_agent_path_.isEmpty());
    assert(!db_path_.isEmpty());

    if (gpg_agent_path_.trimmed().isEmpty()) {
      LOG_E() << "gpg-agent path is empty!";
      return false;
    }

    LOG_D() << "get gpg-agent path: " << gpg_agent_path_;
    QFileInfo info(gpg_agent_path_);
    if (!info.exists() || !info.isFile()) {
      LOG_E() << "gpg-agent is not exists or is not a binary file!";
      return false;
    }

    auto args = QStringList{};

    if (!db_path_.isEmpty()) {
      args.append({"--homedir", QDir::toNativeSeparators(db_path_)});
    }

    args.append({"--daemon", "--enable-ssh-support"});

    auto pinentry = DecidePinentry();
    if (!pinentry.trimmed().isEmpty()) {
      args.append({"--pinentry-program", pinentry});
    }

    if (channel_ != kGpgFrontendDefaultChannel) {
      args.append("--disable-scdaemon");
    }

    LOG_D() << "gpg-agent start args: " << args << "channel:" << channel_;

    process_.setProgram(info.absoluteFilePath());
    process_.setArguments(args);
    process_.setProcessChannelMode(QProcess::MergedChannels);
    process_.start();

    if (!process_.waitForStarted()) {
      LOG_W() << "timeout starting gpg-agent: " << gpg_agent_path_
              << "ags: " << args;
      return false;
    }

    return true;
  }

  ~GpgAgentProcess() {
    if (process_.state() != QProcess::NotRunning) {
      qInfo() << "killing gpg-agent, channel: " << channel_;
      process_.terminate();
      if (!process_.waitForFinished(3000)) {
        process_.kill();
        process_.waitForFinished();
      }
    }
  }

 private:
  int channel_;
  QProcess process_;
  QString db_path_;
  QString gpg_agent_path_;
};

class GpgContext::Impl {
 public:
  /**
   * @brief Construct a new Impl object
   *
   * @param parent
   * @param args
   */
  Impl(GpgContext *parent, const GpgContextInitArgs &args)
      : parent_(parent),
        args_(args),
        db_name_(args.db_name),
        gpgconf_path_(Module::RetrieveRTValueTypedOrDefault<>(
            "core", "gpgme.ctx.gpgconf_path", QString{})),
        database_path_(args.db_path),
        gpg_agent_path_(Module::RetrieveRTValueTypedOrDefault<>(
            "core", "gnupg.components.gpg-agent.path", QString{})),
        agent_(SecureCreateSharedObject<GpgAgentProcess>(
            parent->GetChannel(), gpg_agent_path_, database_path_)) {
    init(args);
  }

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

  auto SetPassphraseCb(const gpgme_ctx_t &ctx, gpgme_passphrase_cb_t cb)
      -> bool {
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

    qsizetype res = 0;
    if (pass_size > 0) {
      qsizetype off = 0;
      qsizetype ret = 0;
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
    GFBuffer passphrase;

    Module::TriggerEvent(
        "REQUEST_PIN_ENTRY",
        {{"uid_hint", GFBuffer{uid_hint != nullptr ? uid_hint : ""}},
         {"passphrase_info",
          GFBuffer{passphrase_info != nullptr ? passphrase_info : ""}},
         {"prev_was_bad", GFBuffer{(prev_was_bad != 0) ? "1" : "0"}},
         {"ask_for_new", GFBuffer{ask_for_new ? "1" : "0"}}},
        [&passphrase, &looper](Module::EventIdentifier i,
                               Module::Event::ListenerIdentifier ei,
                               Module::Event::Params p) {
          if (p["ret"] == "0") passphrase = p["passphrase"];
          looper.quit();
        });

    looper.exec();
    ResetCacheValue("PinentryContext");

    // empty passphrase is not allowed
    if (passphrase.Empty()) return GPG_ERR_CANCELED;

    auto pass_size = passphrase.Size();
    const auto *p_pass_bytes = passphrase.Data();

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

  static auto TestStatusCb(void *hook, const char *keyword, const char *args)
      -> gpgme_error_t {
    FLOG_D("keyword %s", keyword);
    return GPG_ERR_NO_ERROR;
  }

  [[nodiscard]] auto HomeDirectory() const -> QString { return database_path_; }

  [[nodiscard]] auto ComponentDirectories(GpgComponentType type) const
      -> QString {
    return component_dirs_.value(component_type_to_q_string(type), "");
  }

  [[nodiscard]] auto KeyDBName() const -> QString { return db_name_; }

  auto RestartGpgAgent() -> bool {
    if (agent_ != nullptr) {
      agent_ = SecureCreateSharedObject<GpgAgentProcess>(
          parent_->GetChannel(), gpg_agent_path_, database_path_);
    }

    // ensure all gpg-agent are killed.
    kill_gpg_agent();

    return launch_gpg_agent();
  }

 private:
  GpgContext *parent_;
  GpgContextInitArgs args_{};             ///<
  gpgme_ctx_t ctx_ref_ = nullptr;         ///<
  gpgme_ctx_t binary_ctx_ref_ = nullptr;  ///<
  bool good_ = true;

  std::mutex ctx_ref_lock_;
  std::mutex binary_ctx_ref_lock_;

  QString db_name_;
  QString gpgconf_path_;
  QString database_path_;
  QString gpg_agent_path_;
  QMap<QString, QString> component_dirs_;
  QSharedPointer<GpgAgentProcess> agent_;

  void init(const GpgContextInitArgs &args) {
    assert(!gpgconf_path_.isEmpty());
    assert(!database_path_.isEmpty());

    // init
    get_gpg_conf_dirs();
    kill_gpg_agent();

    good_ = launch_gpg_agent() && default_ctx_initialize(args) &&
            binary_ctx_initialize(args);
  }

  static auto component_type_to_q_string(GpgComponentType type) -> QString {
    switch (type) {
      case GpgComponentType::kGPG_AGENT:
        return "agent-socket";
      case GpgComponentType::kGPG_AGENT_SSH:
        return "agent-ssh-socket";
      case GpgComponentType::kDIRMNGR:
        return "dirmngr-socket";
      case GpgComponentType::kKEYBOXD:
        return "keyboxd-socket";
      default:
        return "";
    }
  }

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

  auto set_ctx_openpgp_engine_info(gpgme_ctx_t ctx) -> bool {
    const auto app_path = Module::RetrieveRTValueTypedOrDefault<>(
        "core", QString("gpgme.ctx.app_path"), QString{});

    LOG_D() << "ctx set engine info, channel: " << parent_->GetChannel()
            << ", db name: " << db_name_ << ", db path: " << database_path_
            << ", app path: " << app_path;

    assert(!db_name_.isEmpty());
    assert(!database_path_.isEmpty());

    auto app_path_buffer = app_path.toUtf8();
    auto database_path_buffer = database_path_.toUtf8();

    auto err = gpgme_ctx_set_engine_info(
        ctx, gpgme_get_protocol(ctx),
        app_path.isEmpty() ? nullptr : app_path_buffer,
        database_path_.isEmpty() ? nullptr : database_path_buffer);

    assert(CheckGpgError(err) == GPG_ERR_NO_ERROR);
    return CheckGpgError(err) == GPG_ERR_NO_ERROR;

    return true;
  }

  auto common_ctx_initialize(const gpgme_ctx_t &ctx,
                             const GpgContextInitArgs &args) -> bool {
    assert(ctx != nullptr);

    if (!gpgconf_path_.isEmpty()) {
      LOG_D() << "set gpgconf path: " << gpgconf_path_;
      auto err = gpgme_ctx_set_engine_info(ctx, GPGME_PROTOCOL_GPGCONF,
                                           gpgconf_path_.toUtf8(), nullptr);

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

    if (!set_ctx_openpgp_engine_info(ctx)) {
      FLOG_W("set gpgme context openpgp engine info failed");
      return false;
    }

    Module::UpsertRTValue(
        "core", QString("gpgme.ctx.list.%1.channel").arg(parent_->GetChannel()),
        parent_->GetChannel());
    Module::UpsertRTValue(
        "core",
        QString("gpgme.ctx.list.%1.database_name").arg(parent_->GetChannel()),
        args_.db_name);
    Module::UpsertRTValue(
        "core",
        QString("gpgme.ctx.list.%1.database_path").arg(parent_->GetChannel()),
        args_.db_path);

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

  void get_gpg_conf_dirs() {
    auto gpgconf_path = QFileInfo(this->gpgconf_path_).absoluteFilePath();
    LOG_D() << "context: " << parent_->GetChannel()
            << "gpgconf path: " << gpgconf_path;

    auto args = QStringList{};

    if (!HomeDirectory().isEmpty()) {
      args.append({"--homedir", QDir::toNativeSeparators(HomeDirectory())});
    }

    args.append("--list-dirs");

    QProcess process;
    process.setProgram(gpgconf_path);
    process.setArguments(args);
    process.start();

    if (!process.waitForFinished()) {
      LOG_W() << "failed to execute gpgconf --list-dirs";
      return;
    }

    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
      auto info_split_list = line.split(":");
      if (info_split_list.size() != 2) continue;

      auto configuration_name = info_split_list[0].trimmed();
      auto configuration_value = info_split_list[1].trimmed();

#if defined(_WIN32) || defined(WIN32)
      // replace some special substrings on windows
      // platform
      configuration_value.replace("%3a", ":");
#endif

      LOG_D() << "channel: " << parent_->GetChannel()
              << "component: " << configuration_name
              << "dir:" << configuration_value;

      component_dirs_[configuration_name] = configuration_value;
    }
  }

  auto kill_gpg_agent() -> bool {
    auto gpgconf_path = QFileInfo(this->gpgconf_path_).absoluteFilePath();

    if (gpgconf_path.trimmed().isEmpty()) {
      LOG_E() << "gpgconf path is empty!";
      return false;
    }

    LOG_D() << "get gpgconf path: " << gpgconf_path;

    auto args = QStringList{};

    if (!HomeDirectory().isEmpty()) {
      args.append({"--homedir", QDir::toNativeSeparators(HomeDirectory())});
    }

    args.append({"--kill", "gpg-agent"});

    LOG_D() << "gpgconf kill args: " << args
            << "channel:" << parent_->GetChannel();

    QProcess process;
    process.setProgram(gpgconf_path);
    process.setArguments(args);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start();
    if (!process.waitForFinished(3000)) {
      LOG_W() << "timeout executing gpgconf: " << gpgconf_path
              << "ags: " << args;
      return false;
    }

    LOG_D() << "try to kill gpg-agent before launch: " << process.exitCode()
            << "output: " << process.readAll();
    return process.exitCode() == 0;
  }

  auto launch_gpg_agent() -> bool {
    if (agent_ == nullptr) return false;
    return agent_->Start();
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

auto GpgContext::HomeDirectory() const -> QString {
  return p_->HomeDirectory();
}

auto GpgContext::ComponentDirectory(GpgComponentType type) const -> QString {
  return p_->ComponentDirectories(type);
}

auto GpgContext::KeyDBName() const -> QString { return p_->KeyDBName(); }

auto GpgContext::RestartGpgAgent() -> bool { return p_->RestartGpgAgent(); }
}  // namespace GpgFrontend