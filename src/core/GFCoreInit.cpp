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
#include "GFCoreInit.h"

#include <gpgme.h>

#include <atomic>

#include "core/GFCoreRust.h"
#include "core/function/CacheManager.h"
#include "core/function/CoreInitProgress.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "core/function/gpg/GnuPGHome.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyDatabaseRuntime.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/function/openpgp/helper/Async.h"
#include "core/function/openpgp/traits/CommonTraits.h"
#include "core/function/rpgp/PasswordFetcher.h"
#include "core/model/SettingsObject.h"
#include "core/module/GlobalRegisterTableKeys.h"
#include "core/module/ModuleManager.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/RustUtils.h"

namespace GpgFrontend {

namespace {

std::atomic<bool> k_core_shutting_down = false;

auto VerifyGpgconfPath(const QFileInfo& gnupg_install_fs_path) -> bool {
  return gnupg_install_fs_path.isAbsolute() && gnupg_install_fs_path.exists() &&
         gnupg_install_fs_path.isFile();
}

auto SearchGpgconfPath(const QStringList& candidate_paths) -> QString {
  for (const auto& path : candidate_paths) {
    if (VerifyGpgconfPath(QFileInfo(path))) {
      // return a unify path
      return QFileInfo(path).absoluteFilePath();
    }
  }
  return {};
}

auto GetDefaultKeyDatabasePath() -> QString {
  QString default_db_path;
  auto target_gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});

  // A self-contained profile keeps its keys inside itself and never asks
  // gpgconf where the system keyring is. Portable installations reach this
  // through the same flag rather than through a mode of their own: the
  // location and the policy were always two separate things.
  if (GlobalSettingStation::GetInstance().IsSelfContainedProfile()) {
    default_db_path =
        GlobalSettingStation::GetInstance().GetAppDataPath() + "/db";
    LOG_D() << "default key db for a self-contained profile:"
            << default_db_path;
    goto out;
  }

  if (!target_gpgconf_path.isEmpty()) {
    QFileInfo info(target_gpgconf_path);
    if (info.exists() && info.isFile()) {
      auto* p = new QProcess();
      p->setProgram(info.absoluteFilePath());
      p->setArguments({"--list-dirs", "homedir"});
      p->start();

      p->waitForFinished();
      default_db_path = p->readAll().trimmed();
      p->deleteLater();

      LOG_D() << "default key db path from gpgconf: " << default_db_path;
      goto out;
    }
  }

  default_db_path =
      GlobalSettingStation::GetInstance().GetAppDataPath() + "/rpgp_db";
  LOG_D() << "default key db in fallback mode: " << default_db_path;

out:
  default_db_path = QFileInfo(default_db_path).absoluteFilePath();
  return default_db_path;
}

auto RefreshGpgMEBackendEngine(const QString& gpgconf_path,
                               const QString& gnupg_path,
                               const QString& home_path) -> bool {
  if (!gpgconf_path.isEmpty()) {
    auto err = CheckGpgError(gpgme_set_engine_info(
        GPGME_PROTOCOL_GPGCONF, gpgconf_path.toUtf8(), nullptr));
    if (err != GPG_ERR_NO_ERROR) {
      LOG_W() << "cannot set gpgconf path of gpgme, fallback using default "
                 "gpgconf path, target gpgconf path:"
              << gpgconf_path;
    }
  }

  if (!gnupg_path.isEmpty()) {
    auto err = CheckGpgError(gpgme_set_engine_info(
        GPGME_PROTOCOL_OpenPGP, gnupg_path.toUtf8(), home_path.toUtf8()));
    if (err != GPG_ERR_NO_ERROR) {
      LOG_W() << "cannot set gnupg path and home path of gpgme, fallback using "
                 "default gpgconf path, target gnupg path:"
              << gnupg_path << "target home path: " << home_path;
    }
  }

  return true;
}

auto GetComponentPathsByGpgConf(const QString& gpgconf_install_fs_path)
    -> bool {
  auto* process = new QProcess();
  process->setProgram(gpgconf_install_fs_path);
  process->setArguments({"--check-programs"});
  process->start();
  process->waitForFinished(30000);

  auto output_buffer = process->readAllStandardOutput();
  process->deleteLater();

  if (output_buffer.isEmpty()) return false;

  auto line_split_list = QString(output_buffer).split("\n");
  for (const auto& line : line_split_list) {
    auto info_split_list = line.split(":");

    if (info_split_list.size() != 6) continue;

    auto component_name = info_split_list[0].trimmed().toLower();
    auto component_desc = info_split_list[1].trimmed();
    auto component_path = info_split_list[2].trimmed();
    auto exists = info_split_list[3].trimmed();
    auto runnable = info_split_list[4].trimmed();

#ifdef Q_OS_WINDOWS
    // replace some special substrings on windows platform
    component_path.replace("%3a", ":");
#endif

    if (exists != "1" || runnable != "1") continue;

    QFileInfo file_info(component_path);
    if (!file_info.exists() || !file_info.isFile()) continue;

    Module::UpsertRTValue(
        "core", QString("gnupg.components.%1.checked").arg(component_name), 1);
    Module::UpsertRTValue(
        "core", QString("gnupg.components.%1.path").arg(component_name),
        file_info.absoluteFilePath());

    LOG_D() << "gpg components checked: " << component_name
            << "path: " << file_info.absoluteFilePath();
  }

  return true;
}

auto DecideGpgConfPath(const QString& default_gpgconf_path) -> QString {
  auto settings = GetSettings();
  auto use_custom_gnupg_install_path =
      SettingItems::kUseCustomGnuPGInstallPath.Read(settings);
  auto custom_gnupg_install_path =
      SettingItems::kCustomGnuPGInstallPath.Read(settings);

  QString gpgconf_install_fs_path;
  // user defined
  if (use_custom_gnupg_install_path && !custom_gnupg_install_path.isEmpty()) {
    // check gpgconf path
    gpgconf_install_fs_path = custom_gnupg_install_path;
#ifdef Q_OS_WINDOWS
    gpgconf_install_fs_path += "/gpgconf.exe";
#else
    gpgconf_install_fs_path += "/gpgconf";
#endif

    if (!VerifyGpgconfPath(QFileInfo(gpgconf_install_fs_path))) {
      LOG_W() << "the gpgconf path by settings is illegal, path: "
              << gpgconf_install_fs_path;
      gpgconf_install_fs_path = "";
    }
  }

  if (gpgconf_install_fs_path.isEmpty() &&
      !default_gpgconf_path.trimmed().isEmpty()) {
    LOG_I() << "using default gpgconf path found by gpgme: "
            << default_gpgconf_path;
    return QFileInfo(default_gpgconf_path).absoluteFilePath();
  }

  // custom not found or not defined then fallback to default candidate path
  if (gpgconf_install_fs_path.isEmpty()) {
    // platform detection
#ifdef Q_OS_MACOS
    gpgconf_install_fs_path = SearchGpgconfPath(
        {"/usr/local/bin/gpgconf", "/opt/homebrew/bin/gpgconf"});
#elif defined(Q_OS_WINDOWS)
    gpgconf_install_fs_path =
        SearchGpgconfPath({"C:/Program Files (x86)/gnupg/bin/gpgconf.exe"});
#else
    // unix or linux or another platforms
    gpgconf_install_fs_path =
        SearchGpgconfPath({"/usr/local/bin/gpgconf", "/usr/bin/gpgconf"});
#endif
  }

  if (!gpgconf_install_fs_path.isEmpty()) {
    return QFileInfo(gpgconf_install_fs_path).absoluteFilePath();
  }
  return "";
}

auto DecideGnuPGPath(const QString& default_gnupg_path) -> QString {
  QFileInfo info(default_gnupg_path);

  if (default_gnupg_path.isEmpty() || !info.exists() || !info.isFile()) {
    return Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gnupg.components.gpg.path", QString{});
  }

  return default_gnupg_path;
}

}  // namespace

auto InitBasicPathWithGpgConf() -> bool {
  auto default_gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});

  auto default_gnupg_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.app_path", QString{});

  LOG_I() << "default gpgconf path found by gpgme: " << default_gpgconf_path;
  LOG_I() << "default gnupg path found by gpgme: " << default_gnupg_path;

  if (default_gpgconf_path.isEmpty() || default_gnupg_path.isEmpty()) {
    LOG_W() << "cannot get gpgconf path or gnupg path from gpgme context";
    return false;
  }

  auto target_gpgconf_path = DecideGpgConfPath(default_gpgconf_path);

  // None of the failures below are fatal: rPGP is already registered as a
  // supported engine by the time this runs, so a broken or half-removed GnuPG
  // means "fall back", not "give up". Signalling a bad environment here used to
  // abort startup on machines that would have worked fine on rPGP.
  if (target_gpgconf_path.isEmpty()) {
    LOG_W() << "Cannot find gpgconf, falling back to the rPGP engine.";
    return false;
  }

  // Query the gpgconf we actually settled on. Using the gpgme-reported default
  // here meant a user-configured custom GnuPG install path was honoured for
  // everything except component discovery, which then failed against a stale
  // or removed installation.
  if (!GetComponentPathsByGpgConf(target_gpgconf_path)) {
    LOG_W() << "Cannot get components paths by gpgconf, falling back to the "
               "rPGP engine.";
    return false;
  }

  auto target_gnupg_path = default_gnupg_path;

  if (target_gpgconf_path != default_gpgconf_path) {
    target_gnupg_path = DecideGnuPGPath(target_gnupg_path);
  }

  LOG_I() << "gpgconf path used: " << target_gpgconf_path;
  LOG_I() << "gnupg path used: " << target_gnupg_path;

  if (target_gnupg_path.isEmpty()) {
    LOG_W() << "Cannot find GnuPG by gpgconf, falling back to the rPGP engine.";
    return false;
  }

  Module::UpsertRTValue("core", "gpgme.ctx.gpgconf_path",
                        QString(target_gpgconf_path));
  Module::UpsertRTValue("core", "gpgme.ctx.app_path",
                        QString(target_gnupg_path));

  return true;
}

auto InitBasicPath() -> bool {
  if (!InitBasicPathWithGpgConf()) {
    LOG_W()
        << "Init basic path with gpgconf failed, luckily, that's not critical";
  }

  auto default_home_path = GetDefaultKeyDatabasePath();
  LOG_I() << "default home path: " << default_home_path;
  assert(!default_home_path.isEmpty());

  if (!QDir(default_home_path).exists()) QDir(default_home_path).mkpath(".");

  // set default home path to GRT for later use, such as setting up the default
  // key database path
  Module::UpsertRTValue("core", "gpgme.ctx.default_database_path",
                        QString(default_home_path));

  return true;
}

auto InitGpgME() -> bool {
  const auto* ver = gpgme_check_version(nullptr);
  if (ver == nullptr) {
    LOG_E() << "gpgme_check_version() failed, abort...";
    return false;
    ;
  }
  Module::UpsertRTValue("core", "gpgme.version", QString(ver));

  // require gnupg version > 2.1.0
  if (gpgme_set_global_flag("require-gnupg", "2.1.0") != 0) {
    LOG_E() << "gpgme_set_global_flag() with argument 'require-gnupg' failed, "
               "abort...";
    return false;
  }

#ifdef Q_OS_WINDOWS
  auto w32spawn_dir =
      GlobalSettingStation::GetInstance().GetAppDir() + "/../gnupg/bin";
  if (gpgme_set_global_flag("w32-inst-dir",
                            w32spawn_dir.toUtf8().constData()) != 0) {
    LOG_E() << "gpgme_set_global_flag() with argument 'w32spawn_dir' failed, "
               "abort...";
    return false;
  }
#endif

  if (CheckGpgError(
          gpgme_set_locale(nullptr, LC_CTYPE, setlocale(LC_CTYPE, nullptr))) !=
      GPG_ERR_NO_ERROR) {
    LOG_E() << "gpgme_set_locale() with argument LC_CTYPE failed, abort...";
    return false;
  }

#ifdef LC_MESSAGES
  if (CheckGpgError(gpgme_set_locale(nullptr, LC_MESSAGES,
                                     setlocale(LC_MESSAGES, nullptr))) !=
      GPG_ERR_NO_ERROR) {
    LOG_E() << "gpgme_set_locale() with argument LC_MESSAGES failed, abort...";
    return false;
  }
#endif

  gpgme_ctx_t p_ctx;
  if (CheckGpgError(gpgme_new(&p_ctx)) != GPG_ERR_NO_ERROR) {
    LOG_E() << "gpgme_new() failed, abort...";
    return false;
  }

  // get engine info
  auto* engine_info = gpgme_ctx_get_engine_info(p_ctx);
  if (engine_info == nullptr) {
    LOG_E() << "gpgme_ctx_get_engine_info() failed, abort...";
    return false;
  }

  // Check ENV before running
  bool has_gpgconf = false;
  bool has_openpgp = false;

  while (engine_info != nullptr) {
    if (strcmp(engine_info->version, "1.0.0") == 0) {
      engine_info = engine_info->next;
      continue;
    }

    switch (engine_info->protocol) {
      case GPGME_PROTOCOL_OpenPGP:
        has_openpgp = true;

        Module::UpsertRTValue("core", "gpgme.engine.openpgp", 1);
        Module::UpsertRTValue("core", "gpgme.ctx.app_path",
                              QString(engine_info->file_name));
        Module::UpsertRTValue("core", "gpgme.ctx.gnupg_version",
                              QString(engine_info->version));
        Module::UpsertRTValue(
            "core", "gpgme.ctx.default_database_path",
            QString(engine_info->home_dir == nullptr ? ""
                                                     : engine_info->home_dir));
        break;
      case GPGME_PROTOCOL_CMS:
        Module::UpsertRTValue("core", "gpgme.engine.cms", 1);
        Module::UpsertRTValue("core", "gpgme.ctx.cms_path",
                              QString(engine_info->file_name));

        break;
      case GPGME_PROTOCOL_GPGCONF:
        has_gpgconf = true;

        Module::UpsertRTValue("core", "gpgme.engine.gpgconf", 1);
        Module::UpsertRTValue("core", "gpgme.ctx.gpgconf_path",
                              QString(engine_info->file_name));
        break;
      case GPGME_PROTOCOL_ASSUAN:
      case GPGME_PROTOCOL_G13:
      case GPGME_PROTOCOL_UISERVER:
      case GPGME_PROTOCOL_SPAWN:
      case GPGME_PROTOCOL_DEFAULT:
      case GPGME_PROTOCOL_UNKNOWN:
        break;
    }
    engine_info = engine_info->next;
  }

  // release gpgme context
  gpgme_release(p_ctx);

  const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", QString{"0.0.0"});
  if (!has_gpgconf) {
    LOG_E() << "cannot get gpgconf backend engine, abort...";
    return false;
  }

  if (!has_openpgp) {
    LOG_E() << "cannot get openpgp backend engine, abort...";
    return false;
  }

  // ensure, and check twice: only support gpg > 2.1.0
  if (!(GFCompareSoftwareVersion(gnupg_version, "2.1.0") >= 0)) {
    FLOG_F("gpgme env check failed, abort");
    return false;
  }

  return true;
}

auto InitGnuPGEnv() -> bool {
  // initialize gpgme
  if (!InitGpgME()) {
    LOG_E() << "Oops, GpgME init failed!"
            << "But don't worry, GpgFrontend will use rPGP engine as fallback.";
    Module::UpsertRTValue("core", "env.state.gpgme", -1);
    return false;
  }

  Module::UpsertRTValue("core", "env.state.gpgme", 1);

  auto default_gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});
  auto default_gnupg_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.app_path", QString{});

  LOG_I() << "final gpgconf path: " << default_gpgconf_path;
  LOG_I() << "final gnupg path: " << default_gnupg_path;
  return true;
}

auto BuildOpenPGPContext(int channel, OpenPGPContextInitArgs args) -> bool {
  LOG_D() << "build context for channel: " << channel
          << "db name: " << args.db_name << "db path: " << args.db_path
          << "engine: "
          << (args.engine == OpenPGPEngine::kGNUPG ? "gnupg" : "rpgp")
          << "offline mode: " << args.offline_mode
          << "auto import missing key: " << args.auto_import_missing_key;
  return RunRegisteredRawForward<BuildOpenPGPContextOpTag>(args.engine, channel,
                                                           args);
}

auto InitGpgFrontendCore(CoreInitArgs args) -> int {
  InstallKeyDatabaseRuntimeReclaimer();

  auto& progress = CoreInitProgress::GetInstance();
  progress.Report(CoreInitStage::kCORE, 0.0, CoreInitStep::kSTARTING_UP);

  // check gpgme env
  progress.Report(CoreInitStage::kCORE, 0.3, CoreInitStep::kCHECKING_GNUPG_ENV);
  if (InitGnuPGEnv()) {
    GetGSS().AddSupportedEngine(OpenPGPEngine::kGNUPG);
    Module::UpsertRTValue(Module::kGRTCoreNamespace,
                          Module::GRTEngineKey("gnupg", "version"),
                          Module::RetrieveRTValueTypedOrDefault<>(
                              "core", "gpgme.ctx.gnupg_version", QString{}));
  }

  // check rpgp env, actually rpgp is always integrated and supported, but
  // just in case
  progress.Report(CoreInitStage::kCORE, 0.6,
                  CoreInitStep::kCHECKING_RUST_ENGINE);
  if (HasRustSupport()) {
    GpgFrontend::Rust::gfr_rust_hello();
    GpgFrontend::Rust::gfr_init_logger();
    LOG_I() << "Rust support detected, rPGP engine is available.";
    LOG_I() << "rPGP engine version: " << RustEngineVersion();

    // Configure the rPGP passphrase cache timeouts (seconds). Defaults mirror
    // gpg-agent: a 10-minute sliding idle window capped by a 2-hour absolute
    // maximum. A ttl of 0 leaves the engine's built-in defaults untouched.
    auto settings = GetSettings();
    auto cache_ttl =
        static_cast<quint64>(SettingItems::kPasswordCacheTtl.Read(settings));
    auto cache_max_ttl =
        static_cast<quint64>(SettingItems::kPasswordCacheMaxTtl.Read(settings));
    SetRpgpPasswordCacheTtl(cache_ttl, cache_max_ttl);

    // RFC 9106 section 4 parameter choice for the Argon2id S2K used when
    // encrypting with a passphrase. The first recommendation (2 GiB) is the
    // default; memory-constrained machines can select the second (64 MiB).
    auto argon2_profile = SettingItems::kArgon2S2kProfile.Read(settings);
    SetRpgpArgon2S2kParams(RpgpArgon2ParamsOfProfile(argon2_profile));

    GetGSS().AddSupportedEngine(OpenPGPEngine::kRPGP);
    Module::UpsertRTValue(Module::kGRTCoreNamespace,
                          Module::GRTEngineKey("rpgp", "version"),
                          RustEngineVersion());
  }

  // decide gpgconf, gnupg and default home path
  progress.Report(CoreInitStage::kCORE, 0.8, CoreInitStep::kRESOLVING_PATHS);
  if (!InitBasicPath()) {
    LOG_E() << "Oops, Basic Path init failed!"
            << "GpgFrontend cannot start under this situation!";
    Module::UpsertRTValue("core", "env.state.ctx", -1);
    CoreSignalStation::GetInstance()->SignalBadOpenPGPEnv(
        BadOpenPGPEnvReason::kBASIC_PATH_INIT_FAILED,
        QCoreApplication::tr("Basic Path Initiation Failed"));
    return -1;
  }

  progress.MarkStageDone(CoreInitStage::kCORE, CoreInitStep::kRESOLVING_PATHS);

  // refresh gpgme backend engine, if gnupg is supported
  progress.Report(CoreInitStage::kENGINE, 0.3,
                  CoreInitStep::kREFRESHING_BACKEND_ENGINE);
  if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)) {
    auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.gpgconf_path", QString{});
    auto gnupg_path = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.app_path", QString{});
    auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.default_database_path", QString{});

    if (!RefreshGpgMEBackendEngine(gpgconf_path, gnupg_path, home_path)) {
      LOG_E()
          << "Oops, Refresh GpgME Backend Engine failed!"
          << "But don't worry, GpgFrontend will use rPGP engine as fallback.";
      GetGSS().RemoveSupportedEngine(OpenPGPEngine::kGNUPG);
    }
  }

  // rarely happens, but just in case
  if (!GetGSS().HasSupportedEngine()) {
    LOG_E()
        << "No supported OpenPGP engine detected, GpgFrontend cannot start!";
    Module::UpsertRTValue("core", "env.state.openpgp_engine", -1);
    CoreSignalStation::GetInstance()->SignalBadOpenPGPEnv(
        BadOpenPGPEnvReason::kNO_SUPPORTED_ENGINE,
        QCoreApplication::tr("No Supported OpenPGP Engine Detected"));
    return -1;
  }

  auto settings = GetSettings();

  // read settings from config file
  auto forbid_all_gnupg_connection =
      SettingItems::kForbidAllGnuPGConnection.Read(settings);

  forbid_all_gnupg_connection = forbid_all_gnupg_connection ||
                                qApp->property("GFGnuPGOfflineMode").toBool();

  LOG_D() << "forbid all gnupg network connection:"
          << forbid_all_gnupg_connection;

  auto auto_import_missing_key =
      SettingItems::kAutoImportMissingKey.Read(settings);

  // user can also explicitly set default engine to rpgp if they want to, even
  // when gnupg is supported.
  auto default_engine = SettingItems::kDefaultEngine.Read(settings).toUpper();
  Module::UpsertRTValue(Module::kGRTCoreNamespace,
                        Module::GRTDefaultEngineKey(),
                        default_engine.toLower());

  // unit test mode
  if (args.unit_test_mode) {
    progress.MarkAllDone();
    Module::UpsertRTValue("core", "env.state.basic", 1);
    Module::UpsertRTValue("core", "env.state.key_dbs", 1);
    CoreSignalStation::GetInstance()->SignalGoodGnupgEnv();
    LOG_I() << "Basic ENV Checking Finished";
    return 0;
  }

  auto key_dbs = GetKeyDatabaseInfoBySettings();
  assert(!key_dbs.isEmpty());

  if (key_dbs.isEmpty()) {
    LOG_E() << "Cannot find any valid key database!"
            << "GpgFrontend cannot start under this situation!";
    Module::UpsertRTValue("core", "env.state.ctx", -1);
    CoreSignalStation::GetInstance()->SignalBadOpenPGPEnv(
        BadOpenPGPEnvReason::kNO_VALID_KEY_DATABASE,
        QCoreApplication::tr("No valid Key Database"));
    return -1;
  }

  const auto gnupg_supported =
      GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG);
  const auto rpgp_supported = GetGSS().IsEngineSupported(OpenPGPEngine::kRPGP);

  // The default database is a choice stored with the list, not whichever one
  // comes first, and it is opened on the handle its place in the list gives
  // it. Recorded before anything is built: the agent decides whether to run
  // scdaemon by asking which handle is the default.
  const auto default_db_id = GetDefaultKeyDatabaseId();
  int default_index = 0;
  for (int i = 0; i < key_dbs.size(); i++) {
    if (key_dbs[i].id == default_db_id) {
      default_index = i;
      break;
    }
  }
  const auto default_handle = kGpgFrontendDefaultChannel + default_index;
  const auto& default_key_db_info = key_dbs[default_index];
  SetDefaultChannelHandle(default_handle);

  // `basic/default_engine` is a statement about the DEFAULT database -- the one
  // every profile derives rather than stores, and the one that setting exists
  // to let a user open with rPGP even where GnuPG is available. It was being
  // applied to whichever database came first instead, which is the same thing
  // only while DEFAULT is usable. The moment it is not -- and
  // SelectUsableKeyDatabases() drops it, which is exactly what an imported
  // profile does while its DEFAULT still names a path on the machine it came
  // from -- the next database moves up into channel 0 and was opened with
  // GnuPG because the setting said so, whatever backend that database was
  // actually written with. An rPGP keyring handed to GnuPG.
  const auto default_choice = ChooseChannelZeroEngine(
      default_key_db_info.name, default_key_db_info.backend_type,
      default_engine, gnupg_supported, rpgp_supported);

  if (!default_choice.ok) {
    LOG_E() << "no usable OpenPGP engine for the default key database!"
            << "GpgFrontend cannot start under this situation!";
    Module::UpsertRTValue("core", "env.state.ctx", -1);
    CoreSignalStation::GetInstance()->SignalBadOpenPGPEnv(
        BadOpenPGPEnvReason::kNO_SUPPORTED_ENGINE,
        QCoreApplication::tr("No Supported OpenPGP Engine Detected"));
    return -1;
  }

  LOG_I() << "default openpgp engine:"
          << ConvertOpenPGPEngine2String(default_choice.engine);

  progress.Report(CoreInitStage::kENGINE, 0.7,
                  CoreInitStep::kBUILDING_DEFAULT_CONTEXT);

  // build default openpgp context
  auto succ = [=]() -> auto {
    OpenPGPContextInitArgs ctx_args;

    ctx_args.db_id = default_key_db_info.id;
    ctx_args.order = default_index;
    ctx_args.db_name = default_key_db_info.name;
    ctx_args.db_path = default_key_db_info.path;
    ctx_args.engine = default_choice.engine;

    ctx_args.offline_mode = forbid_all_gnupg_connection;
    ctx_args.auto_import_missing_key = auto_import_missing_key;

    LOG_I() << "gpgme default context at channel" << default_handle
            << ", key db name:" << ctx_args.db_name
            << "key db path:" << ctx_args.db_path;

    return RunRegisteredRawForward<BuildOpenPGPContextOpTag>(
        ctx_args.engine, default_handle, ctx_args);
  };

  if (!succ()) {
    LOG_E() << "Init GpgME Default Context failed!"
            << "GpgFrontend cannot start under this situation!";
    Module::UpsertRTValue("core", "env.state.ctx", -1);
    CoreSignalStation::GetInstance()->SignalBadOpenPGPEnv(
        BadOpenPGPEnvReason::kDEFAULT_CONTEXT_INIT_FAILED,
        QCoreApplication::tr("GpgME Default Context Initiation Failed"));
    return -1;
  }

  Module::UpsertRTValue("core", "env.state.ctx", 1);
  progress.MarkStageDone(CoreInitStage::kENGINE,
                         CoreInitStep::kBUILDING_DEFAULT_CONTEXT);

  // Every database counts for the same share of the key-database track, and
  // the default one is simply the first of them. Held in a local because the
  // second task below needs the same denominator.
  const auto key_db_count = static_cast<double>(key_dbs.size());
  progress.Report(CoreInitStage::kKEY_DATABASE, 0.0,
                  CoreInitStep::kLOADING_KEY_DATABASE,
                  default_key_db_info.name);

  if (!GpgKeyRepository::GetInstance(default_handle).FlushKeyCache()) {
    LOG_E() << "Init GpgME Default Key Database failed!"
            << "GpgFrontend cannot start under this situation!";
    Module::UpsertRTValue("core", "env.state.ctx", -1);
    CoreSignalStation::GetInstance()->SignalBadOpenPGPEnv(
        BadOpenPGPEnvReason::kKEY_CACHE_INIT_FAILED,
        QCoreApplication::tr("Gpg Default Key Database Initiation Failed"));
    return -1;
  };

  progress.Report(CoreInitStage::kKEY_DATABASE, 1.0 / key_db_count,
                  CoreInitStep::kLOADING_KEY_DATABASE,
                  default_key_db_info.name);

  Module::UpsertRTValue("core", "env.state.basic", 1);
  CoreSignalStation::GetInstance()->SignalGoodGnupgEnv();
  LOG_I() << "Basic ENV Checking Finished";

  auto* task = new Thread::Task(
      [=](const DataObjectPtr&) -> int {
        for (int i = 0; i < key_dbs.size(); i++) {
          if (i == default_index) continue;
          const auto& key_db = key_dbs[i];

          // The handle is the database's place in the list, whether or not it
          // opens. Advancing only on success moved every database after a
          // failed one onto the channel the failed one should have had, so
          // the same number meant a different database from one run to the
          // next.
          const auto channel_index = kGpgFrontendDefaultChannel + i;
          const auto give_up = [&]() {
            RegisterChannelHandle(channel_index, key_db.id, false);
            DescribeChannelHandle(channel_index, key_db.name, key_db.path,
                                  key_db.backend_type, i);
            Module::RemoveRTValue(
                "core", QString("gpgme.ctx.list.%1").arg(channel_index));
          };

          // Same check for both engines: guarding only GnuPG here meant a
          // database stored as "rpgp" was handed to an engine that might not
          // exist in this build. The same rule as channel 0, so that where a
          // database lands can never change which engine opens it.
          const auto choice =
              ChooseKeyDatabaseEngine(key_db.backend_type, default_engine,
                                      gnupg_supported, rpgp_supported);
          if (!choice.ok) {
            LOG_W() << "no usable engine for key db, skip:" << key_db.name
                    << "requested backend:" << key_db.backend_type;
            give_up();
            continue;
          }

          auto key_db_engine = choice.engine;

          if (key_db.path.isEmpty() || key_db.name.isEmpty()) {
            LOG_W() << "invalid key db info, skip, name:" << key_db.name
                    << key_db.path;
            give_up();
            continue;
          }

          CoreInitProgress::GetInstance().Report(
              CoreInitStage::kKEY_DATABASE, i / key_db_count,
              CoreInitStep::kLOADING_KEY_DATABASE, key_db.name);

          auto succ = [=]() -> auto {
            GpgFrontend::OpenPGPContextInitArgs args;

            args.db_id = key_db.id;
            args.order = i;
            args.db_name = key_db.name;
            args.db_path = key_db.path;
            args.engine = key_db_engine;
            args.offline_mode = forbid_all_gnupg_connection;
            args.auto_import_missing_key = auto_import_missing_key;

            LOG_D() << "new gpgme context, channel" << channel_index
                    << ", key db name" << args.db_name << "key db path"
                    << args.db_path << "";

            return RunRegisteredRawForward<BuildOpenPGPContextOpTag>(
                args.engine, channel_index, args);
          };

          if (!succ()) {
            LOG_E() << "gpgme context init failed, index:" << channel_index;
            give_up();
            continue;
          }

          if (!GpgKeyRepository::GetInstance(channel_index).FlushKeyCache()) {
            LOG_E() << "gpgme context init key cache failed, index:"
                    << channel_index;
            give_up();
            continue;
          }
        }

        Module::UpsertRTValue("core", "env.state.key_dbs", 1);
        CoreInitProgress::GetInstance().MarkStageDone(
            CoreInitStage::kKEY_DATABASE, CoreInitStep::kLOADING_KEY_DATABASE);

        return 0;
      },
      "core_key_dbs_init_task");

  QObject::connect(task, &Thread::Task::SignalTaskEnd, []() {
    LOG_I() << "All Key Database(s) Initialize Finished";
  });

  // start the thread to check ctx and gnupg state
  // it may take a few seconds or minutes
  GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Default)
      ->PostTask(task);

  return 0;
}

namespace {

/// One look at how far the core has got; true once it is fully initialized
/// (or will never be), false while it is still on its way.
auto CheckCoreInitializationOnce() -> bool {
  if (IsCoreShuttingDown()) return true;

  int core_init_state =
      Module::RetrieveRTValueTypedOrDefault<>("core", "env.state.basic", 0);
  if (core_init_state < 0) return true;
  if (core_init_state == 0) {
    LOG_D() << "monitor: core env is still initializing, waiting...";
    return false;
  }

  if (!Module::ModuleManager::GetInstance().IsAllModulesRegistered()) {
    LOG_D() << "monitor: some modules are still going to be registered, "
               "waiting...";
    return false;
  }

  int key_db_init_state =
      Module::RetrieveRTValueTypedOrDefault<>("core", "env.state.key_dbs", 0);
  if (key_db_init_state == 0) {
    LOG_D() << "monitor: key dbs are still initializing, waiting...";
    return false;
  }

  LOG_D() << "monitor: core is fully initialized, sending signal to ui...";
  // Before the RT value, which is what the startup dialog re-reads to close
  // the edge-triggered gap around its own show(). By then the bar must already
  // say 100, or it is left showing a fraction forever.
  CoreInitProgress::GetInstance().MarkAllDone();
  Module::UpsertRTValue("core", "env.state.all", 1);
  CoreSignalStation::GetInstance()->SignalCoreFullyLoaded();
  return true;
}

/// Look once, and look again later by posting a new task rather than by
/// waiting inside this one.
///
/// It used to spin in one task until the core was ready. That task held the
/// external process runner the whole time, and every command posted there --
/// a module running gpgconf, a test running a shell -- waited behind it; were
/// the core never to finish (a module that never registers), they waited
/// forever. Each look is now a short task of its own, so whatever is queued
/// runs in between, and teardown never has a long-running task to join.
void PostCoreInitializationCheck(int delay_ms) {
  auto* task = new Thread::Task(
      [delay_ms](const DataObjectPtr&) -> int {
        if (delay_ms > 0) QThread::msleep(delay_ms);
        if (!CheckCoreInitializationOnce()) PostCoreInitializationCheck(15);
        return 0;
      },
      "waiting_core_init_task");

  GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_External_Process)
      ->PostTask(task);
}

}  // namespace

void StartMonitorCoreInitializationStatus() { PostCoreInitializationCheck(0); }

auto IsCoreShuttingDown() -> bool { return k_core_shutting_down.load(); }

void DestroyGpgFrontendCore() {
  // Mark teardown as in progress before touching anything. Background
  // subsystems check this so they do not spawn new work (e.g. the
  // DataObjectOperator GC task) that could outlive the singleton storage we
  // are about to destroy.
  k_core_shutting_down.store(true);

  // stop all task runner
  qDebug() << "Stopping all task runner...";
  Thread::TaskRunnerGetter::GetInstance().StopAllTeakRunner();

  qDebug() << "Flushing all cache storage...";

  CacheManager::GetInstance().FlushCacheStorage();

  // Remove the short links handed to gnupg before the store that owns them
  // goes. Explicit rather than destructor-driven: teardown ordering here is
  // already delicate, and a link outliving the run it belongs to is litter.
  qDebug() << "Releasing gnupg home links...";
  GnuPGHomeLinkStore::GetInstance().ReleaseAll();

  // destroy all singleton objects
  qDebug() << "Destroying all singleton objects...";
  SingletonStorageCollection::Destroy();
}

// -----------------------------------------------------------------------------
// Applying the key database configuration without a restart.
// -----------------------------------------------------------------------------

namespace {

/// One apply at a time; a second waits.
std::mutex g_key_db_apply_lock;

/// How long a folder move waits for the work on it to finish.
constexpr int kKeyDatabaseMoveWaitMs = 15000;

/// What the stored configuration is right now, as far as a Runtime cares: the
/// list, and the settings every context is built with. Compared before
/// publishing, so a candidate built from a configuration that has since
/// changed is never put in use.
auto StoredKeyDatabaseConfigurationDigest() -> QByteArray {
  QCryptographicHash hash(QCryptographicHash::Sha256);
  hash.addData(
      QJsonDocument(
          KeyDatabaseListSO(SettingsObject(kKeyDatabaseListObject)).ToJson())
          .toJson(QJsonDocument::Compact));

  auto settings = GetSettings();
  hash.addData(QByteArray::number(
      SettingItems::kForbidAllGnuPGConnection.Read(settings) ? 1 : 0));
  hash.addData(QByteArray::number(
      SettingItems::kAutoImportMissingKey.Read(settings) ? 1 : 0));
  hash.addData(SettingItems::kDefaultEngine.Read(settings).toUpper().toUtf8());
  return hash.result();
}

/// Put a moved database's stored entry back the way it was, when its folder
/// could not be moved. Leaves the list alone if the entry has meanwhile been
/// changed by someone else.
void RevertKeyDatabaseMoveHere(const KeyDatabaseMove& move);

void RevertKeyDatabaseMove(const KeyDatabaseMove& move) {
  // On the GUI thread, where the list is otherwise read and written: a
  // SettingsObject writes when it is destroyed, so a read-modify-write here
  // and one there would each overwrite the other's change.
  auto* app = QCoreApplication::instance();
  if (app != nullptr && QThread::currentThread() != app->thread() &&
      !IsCoreShuttingDown()) {
    QMetaObject::invokeMethod(
        app, [move]() { RevertKeyDatabaseMoveHere(move); },
        Qt::BlockingQueuedConnection);
    return;
  }
  RevertKeyDatabaseMoveHere(move);
}

void RevertKeyDatabaseMoveHere(const KeyDatabaseMove& move) {
  auto so = SettingsObject(kKeyDatabaseListObject);
  if (so.LoadFailed()) return;

  auto list = KeyDatabaseListSO(so);
  for (auto& item : list.key_databases) {
    if (item.id != move.database_id) continue;
    LOG_W() << "restoring key database" << item.name << "to"
            << move.old_stored_path;
    item.name = move.old_name;
    item.path = move.old_stored_path;
    so.Store(list.ToJson());
    return;
  }
}

/// Carry out one folder move once nothing has the folder open.
void PerformKeyDatabaseMove(const KeyDatabaseMove& move,
                            KeyDatabaseApplyResult& result) {
  LOG_I() << "moving key database folder:" << move.from << "->" << move.to;

  // No new work resolves onto the folder from here on, whichever Runtime it
  // asks through. Work already holding it runs on and is waited for below.
  if (!RetireChannelsAtLocation(move.from).isEmpty()) {
    emit CoreSignalStation::GetInstance() -> SignalKeyDatabaseChannelsRetired();
  }

  QElapsedTimer timer;
  timer.start();
  while (IsKeyDatabaseLocationInUse(move.from)) {
    if (timer.elapsed() > kKeyDatabaseMoveWaitMs) {
      LOG_W() << "key database folder still in use, not moving it:"
              << move.from;
      result.errors.append(
          QCoreApplication::tr(
              "The key database \"%1\" was still in use, so its folder was not "
              "renamed. It keeps its previous name.")
              .arg(move.old_name));
      RevertKeyDatabaseMove(move);
      return;
    }
    QThread::msleep(20);
  }

  // An empty folder at the target can only have been made by resolving the
  // new path before the move; it holds nothing and is in the way.
  if (QFileInfo(move.to).isDir()) QDir().rmdir(move.to);

  if (!QDir().rename(move.from, move.to)) {
    LOG_W() << "failed to move key database folder:" << move.from << "->"
            << move.to;
    result.errors.append(
        QCoreApplication::tr("The folder of the key database \"%1\" could not "
                             "be renamed. It keeps its previous name.")
            .arg(move.old_name));
    RevertKeyDatabaseMove(move);
  }
}

/// Build a Runtime from the stored configuration.
auto BuildCandidateRuntime(KeyDatabaseApplyResult& result)
    -> KeyDatabaseRuntimeCandidate {
  auto settings = GetSettings();

  KeyDatabaseRuntimeOptions options;
  options.offline_mode =
      SettingItems::kForbidAllGnuPGConnection.Read(settings) ||
      qApp->property("GFGnuPGOfflineMode").toBool();
  options.auto_import_missing_key =
      SettingItems::kAutoImportMissingKey.Read(settings);
  options.default_engine =
      SettingItems::kDefaultEngine.Read(settings).toUpper();
  options.gnupg_supported = GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG);
  options.rpgp_supported = GetGSS().IsEngineSupported(OpenPGPEngine::kRPGP);

  const auto key_dbs = GetKeyDatabaseInfoBySettings();
  const auto default_id = GetDefaultKeyDatabaseId();

  // As at startup: the chosen default when it is usable, else the first.
  int default_index = 0;
  for (int i = 0; i < key_dbs.size(); i++) {
    if (key_dbs[i].id == default_id) {
      default_index = i;
      break;
    }
  }

  return PrepareKeyDatabaseRuntime(key_dbs, default_index, options, result);
}

}  // namespace

auto PrepareKeyDatabaseRuntime(const QContainer<KeyDatabaseInfo>& key_dbs,
                               int default_index,
                               const KeyDatabaseRuntimeOptions& options,
                               KeyDatabaseApplyResult& result)
    -> KeyDatabaseRuntimeCandidate {
  KeyDatabaseRuntimeCandidate candidate;
  candidate.generation = AllocateRuntimeGeneration();

  for (int i = 0; i < key_dbs.size(); i++) {
    const auto& key_db = key_dbs[i];
    const auto handle = AllocateChannelHandle();
    const auto is_default = i == default_index;

    const auto choice =
        is_default
            ? ChooseChannelZeroEngine(
                  key_db.name, key_db.backend_type, options.default_engine,
                  options.gnupg_supported, options.rpgp_supported)
            : ChooseKeyDatabaseEngine(
                  key_db.backend_type, options.default_engine,
                  options.gnupg_supported, options.rpgp_supported);

    auto opened = false;
    if (choice.ok && !key_db.path.isEmpty() && !key_db.name.isEmpty()) {
      OpenPGPContextInitArgs args;
      args.db_id = key_db.id;
      args.db_name = key_db.name;
      args.db_path = key_db.path;
      args.engine = choice.engine;
      args.generation = candidate.generation;
      args.order = i;
      args.is_default = is_default;
      args.offline_mode = options.offline_mode;
      args.auto_import_missing_key = options.auto_import_missing_key;

      opened = RunRegisteredRawForward<BuildOpenPGPContextOpTag>(
                   args.engine, handle, args) &&
               GpgKeyRepository::GetInstance(handle).FlushKeyCache();
    }

    if (opened) {
      if (is_default) candidate.default_handle = handle;
      continue;
    }

    LOG_W() << "key database did not open in the new runtime:" << key_db.name;
    RegisterChannelHandle(handle, key_db.id, false, candidate.generation);
    DescribeChannelHandle(handle, key_db.name, key_db.path, key_db.backend_type,
                          i);
    result.unavailable.append(key_db.name);
    if (is_default) {
      result.errors.append(
          QCoreApplication::tr("The default key database \"%1\" could not be "
                               "opened.")
              .arg(key_db.name));
      return candidate;
    }
  }

  candidate.ok = candidate.default_handle >= 0;
  if (!candidate.ok) {
    result.errors.append(
        QCoreApplication::tr("No key database could be opened."));
  }
  return candidate;
}

void InstallKeyDatabaseRuntimeReclaimer() {
  SetRetiredChannelReclaimer([](int handle) {
    auto reclaim = [handle]() {
      const auto released = ReleaseEngineScopedChannel(handle);
#ifdef HAS_RUST_SUPPORT
      Rust::gfr_set_channel_cache_scope(handle, nullptr);
      SetChannelPasswordFetcher(handle, {});
#endif
      MarkChannelReclaimed(handle);
      LOG_D() << "reclaimed retired channel" << handle
              << "objects:" << released;
    };

    // On the GUI thread, which every widget reads engine objects from, so a
    // reclamation can never land in the middle of one of those reads.
    //
    // Never during teardown: the last lease may drop on a worker thread that
    // outlived its runner's stop, while the GUI thread is destroying the very
    // storages a reclamation would reach into. Destroy() frees them anyway.
    if (IsCoreShuttingDown()) return;
    if (auto* app = QCoreApplication::instance(); app != nullptr) {
      QMetaObject::invokeMethod(app, reclaim, Qt::QueuedConnection);
    } else {
      reclaim();
    }
  });
}

auto ApplyKeyDatabaseRuntime(const QContainer<KeyDatabaseMove>& moves)
    -> KeyDatabaseApplyResult {
  std::lock_guard<std::mutex> lock(g_key_db_apply_lock);

  KeyDatabaseApplyResult result;
  result.from_generation = CurrentRuntimeGeneration();

  for (const auto& move : moves) PerformKeyDatabaseMove(move, result);

  // Settings saved again while a candidate was being built make it obsolete.
  // Built again from the newer configuration a bounded number of times rather
  // than chasing a configuration that keeps changing.
  for (int attempt = 0; attempt < 3; ++attempt) {
    // Taken before building: the configuration the candidate is built from is
    // the one it is checked against once it is ready.
    const auto digest = StoredKeyDatabaseConfigurationDigest();
    auto candidate = BuildCandidateRuntime(result);

    if (!candidate.ok) {
      RetireRuntimeGeneration(candidate.generation);
      // A folder already moved had its channels retired for the move; with no
      // new Runtime to take it up, it stays closed until the next apply.
      if (!moves.isEmpty()) {
        result.errors.append(QCoreApplication::tr(
            "Renamed key databases stay unavailable until the key database "
            "settings are applied successfully."));
      }
      return result;
    }

    if (attempt < 2 && digest != StoredKeyDatabaseConfigurationDigest()) {
      LOG_I() << "key database configuration changed while building, "
                 "building again";
      RetireRuntimeGeneration(candidate.generation);
      result.unavailable.clear();
      continue;
    }

    PublishRuntimeGeneration(candidate.generation, candidate.default_handle);
    PublishRuntimeToRegisterTable();
    Module::UpsertRTValue(
        Module::kGRTCoreNamespace, Module::GRTDefaultEngineKey(),
        SettingItems::kDefaultEngine.Read(GetSettings()).toLower());

    result.published = true;
    result.to_generation = candidate.generation;

    LOG_I() << "key database runtime published, generation:"
            << result.from_generation << "->" << result.to_generation;

    emit CoreSignalStation::GetInstance() -> SignalKeyDatabaseRuntimePublished(
        result.from_generation, result.to_generation);
    Module::TriggerEvent(
        "KEY_DATABASES_RELOADED",
        {{"from_generation", GFBuffer{QString::number(result.from_generation)}},
         {"to_generation", GFBuffer{QString::number(result.to_generation)}}});
    return result;
  }

  return result;
}

}  // namespace GpgFrontend