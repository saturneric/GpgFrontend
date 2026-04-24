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

#include "GpgContext.h"

#include <gpg-error.h>
#include <gpgme.h>

#include "core/module/ModuleManager.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgContext::GpgContext(int channel) : OpenPGPContext(channel) {}

GpgContext::GpgContext(const OpenPGPContextInitArgs& args, int channel)
    : OpenPGPContext(args, channel) {}

GpgContext::~GpgContext() {
  if (ctx_ref_ != nullptr) {
    gpgme_release(ctx_ref_);
  }

  if (binary_ctx_ref_ != nullptr) {
    gpgme_release(binary_ctx_ref_);
  }
}

[[nodiscard]] auto GpgContext::BinaryContext() const -> gpgme_ctx_t {
  return binary_ctx_ref_;
}

[[nodiscard]] auto GpgContext::DefaultContext() const -> gpgme_ctx_t {
  return ctx_ref_;
}

auto GpgContext::RestartGpgAgent() -> bool {
  if (agent_ != nullptr) {
    agent_ = SecureCreateSharedObject<GpgAgentProcess>(
        GetChannel(), gpg_agent_path_, KeyDBPath());
  }

  // ensure all gpg-agent are killed.
  kill_gpg_agent();

  return launch_gpg_agent();
}

auto GpgContext::KillGpgAgent() -> bool {
  if (agent_ != nullptr) {
    agent_ = SecureCreateSharedObject<GpgAgentProcess>(
        GetChannel(), gpg_agent_path_, KeyDBPath());
  }

  // ensure all gpg-agent are killed.
  return kill_gpg_agent();
}

[[nodiscard]] auto GpgContext::ComponentDirectory(GpgComponentType type) const
    -> QString {
  return component_dirs_.value(ConvertComponentType2String(type), "");
}

auto GpgContext::SetPassphraseCb(const gpgme_ctx_t& ctx,
                                 gpgme_passphrase_cb_t cb) -> bool {
  if (gpgme_get_pinentry_mode(ctx) != GPGME_PINENTRY_MODE_LOOPBACK) {
    if (CheckGpgError(gpgme_set_pinentry_mode(
            ctx, GPGME_PINENTRY_MODE_LOOPBACK)) != GPG_ERR_NO_ERROR) {
      return false;
    }
  }
  gpgme_set_passphrase_cb(ctx, cb, reinterpret_cast<void*>(this));
  return true;
}

auto GpgContext::init(const OpenPGPContextInitArgs& args) -> bool {
  LOG_D() << "initializing gpg context, channel: " << GetChannel()
          << ", key db name: " << KeyDBName();

  assert(Engine() == OpenPGPEngine::kGNUPG);
  assert(KeyDBPath().isEmpty() == false);

  gpgconf_path_ = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{}),
  gpg_agent_path_ = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gnupg.components.gpg-agent.path", QString{});
  agent_ = SecureCreateSharedObject<GpgAgentProcess>(
      GetChannel(), gpg_agent_path_, KeyDBPath());

  assert(!gpgconf_path_.isEmpty());

  // init
  get_gpg_conf_dirs();
  kill_gpg_agent();

  return launch_gpg_agent() && default_ctx_initialize(args) &&
         binary_ctx_initialize(args) && cms_default_ctx_initialize(args) &&
         cms_binary_ctx_initialize(args);
}

auto GpgContext::set_ctx_key_list_mode(const gpgme_ctx_t& ctx) -> bool {
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
                      GPGME_KEYLIST_MODE_SIG_NOTATIONS)) == GPG_ERR_NO_ERROR;
}

auto GpgContext::set_ctx_openpgp_engine_info(gpgme_ctx_t ctx) -> bool {
  const auto app_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", QString("gpgme.ctx.app_path"), QString{});

  LOG_D() << "ctx set engine info, channel: " << GetChannel()
          << ", db name: " << KeyDBName() << ", db path: " << KeyDBPath()
          << ", app path: " << app_path;

  assert(!KeyDBName().isEmpty());
  assert(!KeyDBPath().isEmpty());

  auto database_path = KeyDBPath();

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

auto GpgContext::common_ctx_initialize(const gpgme_ctx_t& ctx,
                                       const OpenPGPContextInitArgs& args)
    -> bool {
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
  FLOG_D("gpg context: offline mode: %d", args.offline_mode);
  FLOG_D("gpg context: auto import missing key: %d",
         args.auto_import_missing_key);
  gpgme_set_offline(ctx, args.offline_mode ? 1 : 0);

  // set option auto import missing key
  if (!args.offline_mode && args.auto_import_missing_key) {
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
  if (args.test_mode) {
    if (!SetPassphraseCb(ctx, test_passphrase_cb)) {
      FLOG_W("set passphrase cb failed, test");
      return false;
    }
  }

  if (!set_ctx_openpgp_engine_info(ctx)) {
    FLOG_W("set gpgme context openpgp engine info failed");
    return false;
  }

  return true;
}

auto GpgContext::cms_default_ctx_initialize(const OpenPGPContextInitArgs& args)
    -> bool {
  gpgme_ctx_t p_ctx;
  if (auto err = CheckGpgError(gpgme_new(&p_ctx)); err != GPG_ERR_NO_ERROR) {
    LOG_W() << "get new gpg context error: " << DescribeGpgErrCode(err).second;
    return false;
  }

  assert(p_ctx != nullptr);
  cms_ctx_ref_ = p_ctx;

  if (auto err =
          CheckGpgError(gpgme_set_protocol(cms_ctx_ref_, GPGME_PROTOCOL_CMS));
      err != GPG_ERR_NO_ERROR) {
    LOG_W() << "get new gpg context error: " << DescribeGpgErrCode(err).second;
    return false;
  }

  if (!common_ctx_initialize(cms_ctx_ref_, args)) {
    FLOG_W("get new ctx failed, binary");
    return false;
  }

  gpgme_set_armor(cms_ctx_ref_, 1);
  return true;
}

auto GpgContext::cms_binary_ctx_initialize(const OpenPGPContextInitArgs& args)
    -> bool {
  gpgme_ctx_t p_ctx;
  if (auto err = CheckGpgError(gpgme_new(&p_ctx)); err != GPG_ERR_NO_ERROR) {
    LOG_W() << "get new gpg context error: " << DescribeGpgErrCode(err).second;
    return false;
  }

  assert(p_ctx != nullptr);
  cms_binary_ctx_ref_ = p_ctx;

  if (auto err = CheckGpgError(
          gpgme_set_protocol(cms_binary_ctx_ref_, GPGME_PROTOCOL_CMS));
      err != GPG_ERR_NO_ERROR) {
    LOG_W() << "get new gpg context error: " << DescribeGpgErrCode(err).second;
    return false;
  }

  if (!common_ctx_initialize(cms_binary_ctx_ref_, args)) {
    FLOG_W("get new ctx failed, binary");
    return false;
  }

  gpgme_set_armor(cms_binary_ctx_ref_, 0);
  return true;
}
auto GpgContext::binary_ctx_initialize(const OpenPGPContextInitArgs& args)
    -> bool {
  gpgme_ctx_t p_ctx;
  if (auto err = CheckGpgError(gpgme_new(&p_ctx)); err != GPG_ERR_NO_ERROR) {
    LOG_W() << "get new gpg context error: " << DescribeGpgErrCode(err).second;
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

auto GpgContext::default_ctx_initialize(const OpenPGPContextInitArgs& args)
    -> bool {
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

void GpgContext::get_gpg_conf_dirs() {
  auto gpgconf_path = QFileInfo(this->gpgconf_path_).absoluteFilePath();
  LOG_D() << "context: " << GetChannel() << "gpgconf path: " << gpgconf_path;

  auto args = QStringList{};

  if (!KeyDBPath().isEmpty()) {
    args.append({"--homedir", QDir::toNativeSeparators(KeyDBPath())});
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

  for (const QString& line : lines) {
    auto info_split_list = line.split(":");
    if (info_split_list.size() != 2) continue;

    auto configuration_name = info_split_list[0].trimmed();
    auto configuration_value = info_split_list[1].trimmed();

#ifdef Q_OS_WINDOWS
    // replace some special substrings on windows
    // platform
    configuration_value.replace("%3a", ":");
#endif

    LOG_D() << "channel: " << GetChannel()
            << "component: " << configuration_name
            << "dir:" << configuration_value;

    component_dirs_[configuration_name] = configuration_value;
  }
}

auto GpgContext::kill_gpg_agent() -> bool {
  auto gpgconf_path = QFileInfo(this->gpgconf_path_).absoluteFilePath();

  if (gpgconf_path.trimmed().isEmpty()) {
    LOG_E() << "gpgconf path is empty!";
    return false;
  }

  LOG_D() << "get gpgconf path: " << gpgconf_path;

  auto args = QStringList{};

  if (!KeyDBPath().isEmpty()) {
    args.append({"--homedir", QDir::toNativeSeparators(KeyDBPath())});
  }

  args.append({"--kill", "gpg-agent"});

  LOG_D() << "gpgconf kill args: " << args << "channel:" << GetChannel();

  QProcess process;
  process.setProgram(gpgconf_path);
  process.setArguments(args);
  process.setProcessChannelMode(QProcess::MergedChannels);
  process.start();
  if (!process.waitForFinished(3000)) {
    LOG_W() << "timeout executing gpgconf: " << gpgconf_path << "ags: " << args;
    return false;
  }

  LOG_D() << "try to kill gpg-agent before launch: " << process.exitCode()
          << "output: " << process.readAll();
  return process.exitCode() == 0;
}

auto GpgContext::launch_gpg_agent() -> bool {
  if (agent_ == nullptr) return false;
  return agent_->Start();
}

auto GpgContext::test_passphrase_cb(void* opaque, const char* uid_hint,
                                    const char* passphrase_info,
                                    int last_was_bad, int fd) -> gpgme_error_t {
  QString passphrase = "abcdefg";
  auto pass_bytes = passphrase.toLatin1();
  auto pass_size = pass_bytes.size();
  const auto* p_pass_bytes = pass_bytes.constData();

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

auto GpgContext::test_status_cb(void* hook, const char* keyword,
                                const char* args) -> gpgme_error_t {
  FLOG_D("keyword %s", keyword);
  return GPG_ERR_NO_ERROR;
}

auto GF_CORE_EXPORT GpgCtx(OpenPGPContext& parent) -> GpgContext& {
  try {
    return dynamic_cast<GpgContext&>(parent);
  } catch (const std::bad_cast&) {
    LOG_E() << "Failed to cast OpenPGPContext to GpgContext, channel: "
            << parent.GetChannel();
    throw std::runtime_error("Failed to cast OpenPGPContext to GpgContext");
  }
}
}  // namespace GpgFrontend