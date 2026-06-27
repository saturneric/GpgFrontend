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

#include "GpgUtils.h"

#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgComponentManager.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgKeyGroup.h"
#include "core/model/KeyDatabaseInfo.h"
#include "core/model/SettingsObject.h"
#include "core/module/ModuleManager.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend {

inline auto Trim(QString& s) -> QString { return s.trimmed(); }

auto GetGpgmeErrorString(size_t buffer_size, gpgme_error_t err) -> QString {
  QContainer<char> buffer(buffer_size);

  gpgme_error_t const ret = gpgme_strerror_r(err, buffer.data(), buffer.size());
  if (ret == ERANGE && buffer_size < 1024) {
    return GetGpgmeErrorString(buffer_size * 2, err);
  }

  return {buffer.data()};
}

auto GetGpgmeErrorString(gpgme_error_t err) -> QString {
  return GetGpgmeErrorString(64, err);
}

auto CheckGpgError(GpgError err) -> GpgError {
  auto err_code = gpg_err_code(err);
  if (err_code != GPG_ERR_NO_ERROR) {
    LOG_W() << "gpg operation failed [error code: " << err_code
            << "], source: " << gpgme_strsource(err)
            << " description: " << GetGpgmeErrorString(err);
  }
  return err_code;
}

auto CheckGpgError2ErrCode(GpgError err, GpgError predict) -> GpgErrorCode {
  auto err_code = gpg_err_code(err);
  if (err_code != gpg_err_code(predict)) {
    if (err_code == GPG_ERR_NO_ERROR) {
      LOG_I() << "[Warning " << gpg_err_code(err)
              << "] Source: " << gpgme_strsource(err)
              << " description: " << GetGpgmeErrorString(err)
              << " predict: " << GetGpgmeErrorString(predict);
    } else {
      LOG_W() << "[Error " << gpg_err_code(err)
              << "] Source: " << gpgme_strsource(err)
              << " description: " << GetGpgmeErrorString(err)
              << " predict: " << GetGpgmeErrorString(predict);
    }
  }
  return err_code;
}

auto DescribeGpgErrCode(GpgError err) -> GpgErrorDesc {
  return {gpgme_strsource(err), GetGpgmeErrorString(err)};
}

auto CheckGpgError(GpgError err, const QString& /*comment*/) -> GpgError {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    LOG_W() << "[Error " << gpg_err_code(err)
            << "] Source: " << gpgme_strsource(err)
            << " description: " << GetGpgmeErrorString(err);
  }
  return err;
}

auto TextIsSigned(QString text) -> int {
  auto trim_text = Trim(text);
  if (trim_text.startsWith(kPgpSignedBegin) &&
      trim_text.endsWith(kPgpSignedEnd)) {
    return 2;
  }
  if (text.contains(kPgpSignedBegin) && text.contains(kPgpSignedEnd)) {
    return 1;
  }
  return 0;
}

namespace {

auto ChopSuffixIfEndsWith(QString path, const QString& suffix)
    -> std::optional<QString> {
  if (!path.endsWith(suffix, Qt::CaseInsensitive)) {
    return std::nullopt;
  }

  path.chop(suffix.size());
  return path;
}

auto RemoveKnownArchiveEncryptedSuffix(const QString& input_path) -> QString {
  if (auto out = ChopSuffixIfEndsWith(input_path, QStringLiteral(".tar.gpg"))) {
    return *out;
  }

  if (auto out = ChopSuffixIfEndsWith(input_path, QStringLiteral(".tar.asc"))) {
    return *out;
  }

  if (auto out = ChopSuffixIfEndsWith(input_path, QStringLiteral(".tar.pgp"))) {
    return *out;
  }

  if (auto out = ChopSuffixIfEndsWith(input_path, QStringLiteral(".gpg"))) {
    if (out->endsWith(QStringLiteral(".tar"), Qt::CaseInsensitive)) {
      out->chop(QStringLiteral(".tar").size());
    }
    return *out;
  }

  if (auto out = ChopSuffixIfEndsWith(input_path, QStringLiteral(".asc"))) {
    if (out->endsWith(QStringLiteral(".tar"), Qt::CaseInsensitive)) {
      out->chop(QStringLiteral(".tar").size());
    }
    return *out;
  }

  if (auto out = ChopSuffixIfEndsWith(input_path, QStringLiteral(".pgp"))) {
    if (out->endsWith(QStringLiteral(".tar"), Qt::CaseInsensitive)) {
      out->chop(QStringLiteral(".tar").size());
    }
    return *out;
  }

  return input_path + QStringLiteral(".out");
}

}  // namespace

auto SetExtensionOfOutputFile(const QString& path, GpgOperation opera,
                              bool ascii) -> QString {
  const QFileInfo file_info(path);
  const auto input_path = file_info.absoluteFilePath();

  switch (opera) {
    case kENCRYPT:
    case kENCRYPT_SIGN:
      return input_path +
             (ascii ? QStringLiteral(".asc") : QStringLiteral(".gpg"));

    case kSIGN:
      return input_path +
             (ascii ? QStringLiteral(".asc") : QStringLiteral(".sig"));

    case kDECRYPT:
    case kDECRYPT_VERIFY: {
      auto out_path = input_path;

      if (out_path.endsWith(QStringLiteral(".tar.gpg"), Qt::CaseInsensitive)) {
        out_path.chop(QStringLiteral(".gpg").size());
      } else if (out_path.endsWith(QStringLiteral(".tar.asc"),
                                   Qt::CaseInsensitive)) {
        out_path.chop(QStringLiteral(".asc").size());
      } else if (out_path.endsWith(QStringLiteral(".gpg"),
                                   Qt::CaseInsensitive)) {
        out_path.chop(QStringLiteral(".gpg").size());
      } else if (out_path.endsWith(QStringLiteral(".pgp"),
                                   Qt::CaseInsensitive)) {
        out_path.chop(QStringLiteral(".pgp").size());
      } else if (out_path.endsWith(QStringLiteral(".asc"),
                                   Qt::CaseInsensitive)) {
        out_path.chop(QStringLiteral(".asc").size());
      } else {
        out_path += QStringLiteral(".out");
      }

      if (out_path == input_path || out_path.isEmpty()) {
        out_path = input_path + QStringLiteral(".out");
      }

      return out_path;
    }

    default:
      return input_path + QStringLiteral(".out");
  }
}

auto SetExtensionOfOutputFileForArchive(const QString& path, GpgOperation opera,
                                        bool ascii) -> QString {
  const QFileInfo file_info(path);
  const auto input_path = file_info.absoluteFilePath();

  switch (opera) {
    case kENCRYPT:
    case kENCRYPT_SIGN: {
      QString out_path = input_path;

      if (!file_info.fileName().endsWith(QStringLiteral(".tar"),
                                         Qt::CaseInsensitive)) {
        out_path += QStringLiteral(".tar");
      }

      out_path += ascii ? QStringLiteral(".asc") : QStringLiteral(".gpg");
      return out_path;
    }

    case kDECRYPT:
    case kDECRYPT_VERIFY: {
      return RemoveKnownArchiveEncryptedSuffix(input_path);
    }

    default:
      return input_path + QStringLiteral(".out");
  }
}

static QContainer<KeyDatabaseInfo> gpg_key_database_info_cache;

auto GF_CORE_EXPORT GetGpgKeyDatabaseInfos() -> QContainer<KeyDatabaseInfo> {
  if (!gpg_key_database_info_cache.empty()) return gpg_key_database_info_cache;

  auto context_index_list = Module::ListRTChildKeys("core", "gpgme.ctx.list");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  gpg_key_database_info_cache.resize(
      static_cast<qsizetype>(context_index_list.size()));
#else
  gpg_key_database_info_cache.reserve(
      static_cast<qsizetype>(context_index_list.size()));
  std::fill_n(std::back_inserter(gpg_key_database_info_cache),
              context_index_list.size(), KeyDatabaseInfo{});
#endif

  for (auto& context_index : context_index_list) {
    LOG_D() << "context grt key: " << context_index;

    const auto grt_key_prefix = QString("gpgme.ctx.list.%1").arg(context_index);
    auto channel = Module::RetrieveRTValueTypedOrDefault(
        "core", grt_key_prefix + ".channel", -1);
    auto database_name = Module::RetrieveRTValueTypedOrDefault(
        "core", grt_key_prefix + ".database_name", QString{});
    auto database_path = Module::RetrieveRTValueTypedOrDefault(
        "core", grt_key_prefix + ".database_path", QString{});

    LOG_D() << "context grt channel: " << channel
            << "GRT key prefix: " << grt_key_prefix
            << "database name: " << database_name;

    auto i = KeyDatabaseInfo();
    i.channel = channel;
    i.name = database_name;
    i.path = database_path;
    gpg_key_database_info_cache[channel] = i;
  }

  return gpg_key_database_info_cache;
}
auto GF_CORE_EXPORT GetGpgKeyDatabaseName(int channel) -> QString {
  auto info = GetGpgKeyDatabaseInfos();
  if (channel >= info.size()) return {};
  return info[channel].name;
}

namespace {

// Set of backend types whose engine is actually available in this build. The
// macOS app sandbox ships the rpgp-only "lite" variant, while Flathub carries
// both gnupg and rpgp.
auto SupportedKeyDatabaseBackends() -> QSet<QString> {
  QSet<QString> backends;
  if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)) backends.insert("gnupg");
  if (GetGSS().IsEngineSupported(OpenPGPEngine::kRPGP)) backends.insert("rpgp");
  return backends;
}

// Build the channel-0 "DEFAULT" key database entry. Its path is derived from
// the gpgme context (or an app-data fallback), never from a fixed user dir.
auto MakeDefaultKeyDatabaseItem() -> KeyDatabaseItemSO {
  KeyDatabaseItemSO key_db;

  // try to get default key database path from gpgme context
  // if gpgme checking failed, this is likely to be empty
  auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.default_database_path", QString{});
  LOG_D() << "got default key database path from context: " << home_path;
  assert(!home_path.isEmpty());

  if (home_path.isEmpty()) {
    LOG_E() << "failed to get default key database path from gpgme context, "
               "fallback to default app data path";

    // this should not happen, but just in case, fallback to app data path
    home_path =
        GlobalSettingStation::GetInstance().GetAppDataPath() + "/rpgp_db";

    // since we cannot get default key database path from gpgme context, it's
    // likely that gpgme is not working properly, we should fallback to rpgp
    GetGSS().AddSupportedEngine(OpenPGPEngine::kRPGP);
  }

  key_db.channel = 0;
  key_db.name = "DEFAULT";

  // default to gnupg backend if possible if gnupg backend is not supported,
  // fallback to rpgp backend, and set the default key database path to rpgp
  // default path
  if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)) {
    key_db.backend_type = "gnupg";
    LOG_I() << "gnupg backend is supported, use gnupg backend as default";
  } else {
    // fallback to rpgp backend
    key_db.backend_type = "rpgp";
    LOG_W() << "gnupg backend is not supported, fallback to rpgp backend";
  }

  // in portable mode, convert home path to relative path to app dir
  if (GlobalSettingStation::GetInstance().IsProtableMode()) {
    home_path = QDir(GlobalSettingStation::GetInstance().GetAppDir())
                    .relativeFilePath(home_path);
  }

  key_db.path = home_path;
  return key_db;
}

// Sort entries by channel, then resolve duplicate channels by incrementing so
// every key database ends up with a unique, ascending channel.
void NormalizeKeyDatabaseChannels(QContainer<KeyDatabaseItemSO>& key_dbs) {
  std::sort(key_dbs.begin(), key_dbs.end(),
            [](const auto& a, const auto& b) -> bool {
              return a.channel < b.channel;
            });

  for (auto it = key_dbs.begin(); it != key_dbs.end(); ++it) {
    auto next_it = std::next(it);
    while (next_it != key_dbs.end() && next_it->channel == it->channel) {
      next_it->channel = it->channel + 1;
      ++next_it;
    }
  }
}

// Scan the fixed sandbox directory (<app-data>/dbs) for user key databases,
// returning a name/path entry per subdirectory found.
auto ScanSandboxKeyDatabaseDir() -> QContainer<KeyDatabaseItemSO> {
  QContainer<KeyDatabaseItemSO> discovered;

  const auto dbs_root =
      GlobalSettingStation::GetInstance().GetAppDataPath() + "/dbs";
  QDir dbs_dir(dbs_root);
  if (!dbs_dir.exists()) {
    LOG_D() << "sandbox key database dir does not exist, skip scan:" << dbs_root;
    return discovered;
  }

  const auto entries =
      dbs_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
  for (const auto& entry : entries) {
    KeyDatabaseItemSO key_db;
    key_db.name = entry.fileName();
    key_db.path = entry.absoluteFilePath();
    LOG_I() << "discovered sandbox key database:" << key_db.name
            << "path:" << key_db.path;
    discovered.append(key_db);
  }
  return discovered;
}

}  // namespace

// Reconcile the sandbox key database list from the filesystem scan. The
// filesystem (`discovered`) is authoritative for which user databases exist and
// their paths; `stored` only supplies recoverable metadata (backend type,
// channel/order) matched by name. The channel-0 `default_db` is always kept and
// never sourced from the scan. Any backend type not present in
// `supported_backends` is dropped in favour of a supported default (gnupg when
// available, else rpgp) - this is what keeps the rpgp-only macOS lite build from
// honouring a stale "gnupg" type carried over in settings.
auto GF_CORE_EXPORT ReconcileSandboxKeyDatabaseList(
    KeyDatabaseItemSO default_db, QContainer<KeyDatabaseItemSO> discovered,
    const QContainer<KeyDatabaseItemSO>& stored,
    const QSet<QString>& supported_backends) -> QContainer<KeyDatabaseItemSO> {
  const auto pick_default_backend = [&]() -> QString {
    return supported_backends.contains("gnupg") ? "gnupg" : "rpgp";
  };
  const auto is_supported = [&](const QString& type) -> bool {
    return supported_backends.contains(type.toLower().trimmed());
  };

  // index stored entries by name to recover metadata for discovered databases
  QMap<QString, KeyDatabaseItemSO> stored_by_name;
  for (const auto& db : stored) {
    if (!db.name.isEmpty()) stored_by_name.insert(db.name, db);
  }

  QContainer<KeyDatabaseItemSO> key_dbs;

  // channel-0 DEFAULT database keeps its derived path; recover/validate backend
  if (const auto it = stored_by_name.constFind(default_db.name);
      it != stored_by_name.constEnd() && is_supported(it->backend_type)) {
    default_db.backend_type = it->backend_type;
  }
  if (!is_supported(default_db.backend_type)) {
    default_db.backend_type = pick_default_backend();
  }
  default_db.channel = 0;
  key_dbs.append(default_db);

  int next_channel = 1;
  for (auto& key_db : discovered) {
    if (key_db.name.isEmpty()) continue;
    // never let a scanned dir shadow the channel-0 DEFAULT database
    if (key_db.name == default_db.name) continue;

    if (const auto it = stored_by_name.constFind(key_db.name);
        it != stored_by_name.constEnd()) {
      key_db.backend_type = is_supported(it->backend_type)
                                ? it->backend_type
                                : pick_default_backend();
      key_db.channel = it->channel;
    } else {
      key_db.backend_type = pick_default_backend();
      key_db.channel = next_channel;
    }
    next_channel = std::max(next_channel, key_db.channel) + 1;
    key_dbs.append(key_db);
  }

  NormalizeKeyDatabaseChannels(key_dbs);
  return key_dbs;
}

// macOS app sandbox: user key databases live under a fixed directory
// (<app-data>/dbs/<name>). The absolute container path can drift from what was
// persisted, so the filesystem - not the stored settings paths - is the source
// of truth for which databases actually exist. We scan that fixed directory and
// only recover metadata (backend type, channel/order) from settings by name.
//
// NOTE: this is intentionally gated on the macOS-only app sandbox, not the
// broader IsRunningInSandBox(): the Flatpak data dir is a stable bind-mount, so
// stored paths stay valid there and a rescan would be both unnecessary and
// wrong (Flatpak databases are not constrained to the dbs/ layout).
namespace {

auto GetKeyDatabasesByAppSandboxScan() -> QContainer<KeyDatabaseItemSO> {
  auto key_db_list_so = SettingsObject("key_database_list");
  auto stored = KeyDatabaseListSO(key_db_list_so);

  auto key_dbs = ReconcileSandboxKeyDatabaseList(
      MakeDefaultKeyDatabaseItem(), ScanSandboxKeyDatabaseDir(),
      stored.key_databases, SupportedKeyDatabaseBackends());

  // persist the reconciled list so settings stay in sync with disk
  KeyDatabaseListSO reconciled;
  reconciled.key_databases = key_dbs;
  key_db_list_so.Store(reconciled.ToJson());

  for (const auto& key_db : key_dbs) {
    LOG_I() << "got sandbox key database:" << key_db.name
            << ", path:" << key_db.path;
  }

  return key_dbs;
}

}  // namespace

auto GetKeyDatabasesBySettings() -> QContainer<KeyDatabaseItemSO> {
  // In the macOS app sandbox the stored settings paths may not match the
  // databases that actually exist under the fixed dbs/ path, so rebuild the
  // list from a filesystem scan instead of trusting settings.
  if (IsRunningInAppSandbox()) {
    return GetKeyDatabasesByAppSandboxScan();
  }

  auto key_db_list_so = SettingsObject("key_database_list");
  auto key_db_list = KeyDatabaseListSO(key_db_list_so);
  auto& key_dbs = key_db_list.key_databases;

  QContainer<KeyDatabaseItemSO> tmp_key_dbs;
  for (const auto& key_db : key_dbs) {
    LOG_D() << "filtering key database from settings:" << key_db.name
            << ", path:" << key_db.path;

    if (key_db.path.isEmpty() || key_db.name.isEmpty()) {
      LOG_W() << "invalid key db info, skip, name:" << key_db.name
              << "key db path:" << key_db.path;
      continue;
    }

    tmp_key_dbs.push_back(key_db);
  }
  key_dbs = std::move(tmp_key_dbs);

  if (key_dbs.empty()) {
    key_dbs.append(MakeDefaultKeyDatabaseItem());
  }

  NormalizeKeyDatabaseChannels(key_dbs);

  key_db_list_so.Store(key_db_list.ToJson());

  for (const auto& key_db : key_db_list.key_databases) {
    LOG_I() << "got key database from settings:" << key_db.name
            << ", path:" << key_db.path;
  }

  return key_db_list.key_databases;
}

auto VerifyKeyDatabasePath(const QFileInfo& key_database_fs_path) -> bool {
  return key_database_fs_path.isAbsolute() && key_database_fs_path.exists() &&
         key_database_fs_path.isDir();
}

auto SearchKeyDatabasePath(const QStringList& candidate_paths) -> QString {
  for (const auto& path : candidate_paths) {
    if (VerifyKeyDatabasePath(QFileInfo(path))) {
      // return a unify path
      return QFileInfo(path).absoluteFilePath();
    }
  }
  return {};
}

auto GetCanonicalKeyDatabasePath(const QDir& app_path, const QString& path)
    -> QString {
  auto target_path = path;
  if (!QDir::isAbsolutePath(target_path)) {
    target_path = app_path.absoluteFilePath(target_path);
    LOG_D() << "convert relative path: " << path
            << "to absolute path: " << target_path;
  }

  QFileInfo info(target_path);
  auto dir_path = info.absolutePath();
  QDir dir;
  if (!dir.exists(dir_path)) {
    LOG_W() << "key database not exists:" << dir_path << ", creating...";
    if (!dir.mkpath(dir_path)) {
      LOG_E() << "failed to recreate key database:" << dir_path;
    }
  }

  if (VerifyKeyDatabasePath(info)) {
    auto key_database_fs_path = info.canonicalFilePath();
    LOG_D() << "load gpg key database:" << key_database_fs_path;

    return key_database_fs_path;
  }

  LOG_W() << "gpg key database path is invalid: " << path;
  return {};
}

auto GetAllKeyDatabaseInfoBySettings() -> QContainer<KeyDatabaseInfo> {
  auto key_dbs = GetKeyDatabasesBySettings();

  QContainer<KeyDatabaseInfo> key_db_infos;

  const auto app_path = QDir(GlobalSettingStation::GetInstance().GetAppDir());

  // try to use user defined key database
  for (const auto& key_database : key_dbs) {
    if (key_database.path.isEmpty()) continue;

    LOG_D() << "got key database:" << key_database.name
            << "path:" << key_database.path;

    auto key_database_fs_path =
        GetCanonicalKeyDatabasePath(app_path, key_database.path);

    KeyDatabaseInfo key_db_info;
    key_db_info.name = key_database.name;
    key_db_info.backend_type = key_database.backend_type;
    key_db_info.path = key_database_fs_path;
    key_db_info.origin_path = key_database.path;
    key_db_info.valid = !key_database_fs_path.isEmpty();
    key_db_infos.append(key_db_info);

    LOG_D() << "plan to load gpg key database at:" << key_database_fs_path;
  }

  return key_db_infos;
}

auto GetKeyDatabaseInfoBySettings() -> QContainer<KeyDatabaseInfo> {
  auto key_db_infos = GetAllKeyDatabaseInfoBySettings();

  // filter out invalid key databases
  key_db_infos.erase(
      std::remove_if(
          key_db_infos.begin(), key_db_infos.end(),
          [](const auto& key_db_info) -> auto{ return !key_db_info.valid; }),
      key_db_infos.end());

  LOG_I() << "valid key database count: " << key_db_infos.size();
  return key_db_infos;
}

auto GF_CORE_EXPORT ConvertKey2GpgKeyIdList(int channel,
                                            const GpgAbstractKeyPtrList& keys)
    -> KeyIdArgsList {
  KeyIdArgsList ret;
  for (const auto& key : ConvertKey2GpgKeyList(channel, keys)) {
    ret.push_back(key->ID());
  }
  return ret;
}

auto GF_CORE_EXPORT ConvertKey2GpgKeyList(int channel,
                                          const GpgAbstractKeyPtrList& keys)
    -> GpgKeyPtrList {
  GpgKeyPtrList recipients;

  QSet<QString> s;
  for (const auto& key : keys) {
    if (key == nullptr || key->IsDisabled() || s.contains(key->ID())) continue;

    if (key->KeyType() == GpgAbstractKeyType::kGPG_KEY) {
      recipients.push_back(qSharedPointerDynamicCast<GpgKey>(key));
    } else if (key->KeyType() == GpgAbstractKeyType::kGPG_KEYGROUP) {
      auto key_ids = qSharedPointerDynamicCast<GpgKeyGroup>(key)->KeyIds();
      recipients += ConvertKey2GpgKeyList(
          channel,
          AbstractKeyRepository::GetInstance(channel).GetKeys(key_ids));
    }

    s.insert(key->ID());
  }

  assert(std::all_of(keys.begin(), keys.end(),
                     [](const auto& key) { return key->IsGood(); }));

  return recipients;
}

auto GF_CORE_EXPORT Convert2GpgKeyList(int channel,
                                       const GpgAbstractKeyPtrList& keys)
    -> QContainer<GpgKey> {
  QContainer<GpgKey> recipients;

  auto g_keys = ConvertKey2GpgKeyList(channel, keys);
  for (const auto& key : g_keys) {
    recipients.push_back(*qSharedPointerDynamicCast<GpgKey>(key));
  }

  return recipients;
}

auto GF_CORE_EXPORT GetUsagesByAbstractKey(const GpgAbstractKey* key)
    -> QString {
  QString usages;
  if (key->IsHasCertCap()) usages += "C";
  if (key->IsHasEncrCap()) usages += "E";
  if (key->IsHasSignCap()) usages += "S";
  if (key->IsHasAuthCap()) usages += "A";

  if (key->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY) {
    if (dynamic_cast<const GpgSubKey*>(key)->IsADSK()) usages += "R";
  }
  return usages;
}

auto GF_CORE_EXPORT GetGpgKeyByGpgAbstractKey(GpgAbstractKey* ab_key)
    -> GpgKey {
  if (!ab_key->IsGood()) return {};

  if (ab_key->KeyType() == GpgAbstractKeyType::kGPG_SUBKEY) {
    auto* s_key = dynamic_cast<GpgSubKey*>(ab_key);

    assert(s_key != nullptr);
    if (s_key == nullptr) return {};

    return *s_key->Convert2GpgKey();
  }

  if (ab_key->KeyType() == GpgAbstractKeyType::kGPG_KEY) {
    auto* key = dynamic_cast<GpgKey*>(ab_key);
    return *key;
  }

  return {};
}

auto GF_CORE_EXPORT IsKeyGroupID(const KeyId& id) -> bool {
  return id.startsWith("#&");
}

auto GF_CORE_EXPORT GpgAgentVersionGreaterThan(int channel, const QString& v)
    -> bool {
  return GFSoftwareVersionGreaterThan(
      GpgComponentManager::GetInstance(channel).GetGpgAgentVersion(), v);
}

auto GF_CORE_EXPORT DecidePinentry() -> QString {
#ifdef Q_OS_LINUX
  QStringList preferred_list = {"pinentry-gnome3", "pinentry-qt",
                                "pinentry-gtk", "pinentry-gtk2"};
  QStringList search_paths = {"/bin", "/usr/bin", "/usr/local/bin"};
#elif defined(Q_OS_MACOS)
  QStringList preferred_list = {"pinentry-mac", "pinentry-qt"};
  QStringList search_paths = {"/opt/homebrew/bin", "/usr/local/bin"};
#else
  QStringList preferred_list = {"pinentry-qt"};
  QStringList search_paths = {};
#endif

  if (IsFlatpakENV()) {
    LOG_D() << "set flatpak pinentry to /app/bin/pinentry-qt";
    return "/app/bin/pinentry-qt";
  }

  for (const QString& name : preferred_list) {
    auto path = QStandardPaths::findExecutable(name);
    if (!path.isEmpty()) {
      LOG_D() << "find pinentry path: " << path;
      return path;
    }
  }

  if (search_paths.isEmpty()) return {};

  for (const QString& name : preferred_list) {
    auto path = QStandardPaths::findExecutable(name, search_paths);
    if (!path.isEmpty()) {
      LOG_D() << "find pinentry path by search path: " << path;
      return path;
    }
  }

  return {};
}

auto GnuPGVersion() -> QString {
  auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", QString{"0.0.0"});
  return gnupg_version;
}

const QRegularExpression kRegexUserId(
    R"(^(.*?)(?:\s*\((.*?)\))?(?:\s*<(.*?)>)?$)");

auto ParseUserId(const QString& raw_id) -> GFUserId {
  GFUserId uid;
  uid.is_primary = false;

  // Standard PGP UID Regex format

  QRegularExpressionMatch match = kRegexUserId.match(raw_id);

  if (match.hasMatch()) {
    uid.name = match.captured(1).trimmed();
    uid.comment = match.captured(2).trimmed();
    uid.email = match.captured(3).trimmed();
  } else {
    // Fallback if it's just a raw string without standard formatting
    uid.name = raw_id;
  }

  return uid;
}

auto AssembleUserId(const QString& name, const QString& comment,
                    const QString& email) -> QString {
  const auto trimmed_name = name.trimmed();
  const auto trimmed_comment = comment.trimmed();
  const auto trimmed_email = email.trimmed();

  QString uid = trimmed_name;
  if (!trimmed_comment.isEmpty()) {
    if (!uid.isEmpty()) uid += ' ';
    uid += QString("(%1)").arg(trimmed_comment);
  }
  if (!trimmed_email.isEmpty()) {
    if (!uid.isEmpty()) uid += ' ';
    uid += QString("<%1>").arg(trimmed_email);
  }
  return uid;
}

auto IsValidUserIdComponent(const QString& component) -> bool {
  static const QRegularExpression kForbidden(R"([()<>\x00-\x1F\x7F])");
  return !component.contains(kForbidden);
}

auto ConvertOpenPGPEngine2String(OpenPGPEngine type) -> QString {
  switch (type) {
    case OpenPGPEngine::kGNUPG:
      return "GnuPG";
    case OpenPGPEngine::kRPGP:
      return "rPGP";
    default:
      return "Unknown";
  }
}

auto ConvertComponentType2String(GpgComponentType type) -> QString {
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
}  // namespace GpgFrontend
