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

#include <optional>

#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgComponentManager.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgKeyGroup.h"
#include "core/model/KeyDatabaseInfo.h"
#include "core/model/SettingsObject.h"
#include "core/module/ModuleManager.h"
#include "core/profile/ProfileAreaTraits.h"
#include "core/profile/ProfileSession.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "core/utils/BuildInfoUtils.h"
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

auto GF_CORE_EXPORT
BuildGpgKeyDatabaseInfos(const QContainer<KeyDatabaseInfo>& reported)
    -> QContainer<KeyDatabaseInfo> {
  QContainer<KeyDatabaseInfo> infos;
  QSet<int> seen_channels;

  for (const auto& info : reported) {
    // An entry that names no channel names no context either. Keeping it would
    // put a KeyDatabaseInfo carrying channel -1 in a list every caller walks
    // asking OpenPGPContext::GetInstance() about, which lazily creates a
    // placeholder context for a channel that has none.
    if (info.channel < 0) {
      LOG_W() << "context reports no channel, skip. database name:"
              << info.name;
      continue;
    }

    // A channel is one context. Two entries claiming the same one is a
    // contradiction the list cannot express, so the first one wins rather than
    // the last silently replacing it.
    if (seen_channels.contains(info.channel)) {
      LOG_W() << "context reports an already claimed channel, skip:"
              << info.channel << "database name:" << info.name;
      continue;
    }

    seen_channels.insert(info.channel);
    infos.append(info);
  }

  std::sort(infos.begin(), infos.end(),
            [](const KeyDatabaseInfo& a, const KeyDatabaseInfo& b) -> bool {
              return a.channel < b.channel;
            });

  return infos;
}

auto GF_CORE_EXPORT GetGpgKeyDatabaseInfos() -> QContainer<KeyDatabaseInfo> {
  if (!gpg_key_database_info_cache.empty()) return gpg_key_database_info_cache;

  auto context_index_list = Module::ListRTChildKeys("core", "gpgme.ctx.list");

  QContainer<KeyDatabaseInfo> reported;
  reported.reserve(static_cast<qsizetype>(context_index_list.size()));

  for (auto& context_index : context_index_list) {
    LOG_D() << "context grt key: " << context_index;

    const auto grt_key_prefix = QString("gpgme.ctx.list.%1").arg(context_index);
    auto channel = Module::RetrieveRTValueTypedOrDefault(
        "core", grt_key_prefix + ".channel", -1);
    auto database_name = Module::RetrieveRTValueTypedOrDefault(
        "core", grt_key_prefix + ".database_name", QString{});
    auto database_path = Module::RetrieveRTValueTypedOrDefault(
        "core", grt_key_prefix + ".database_path", QString{});
    // Published by OpenPGPContext::Initialize() alongside the other three and
    // never read back until now, which left every caller to re-derive the
    // engine by constructing a context -- and constructing one for a channel
    // that has none lazily creates a placeholder that reports GnuPG.
    auto backend_type = Module::RetrieveRTValueTypedOrDefault(
        "core", grt_key_prefix + ".backend_type", QString{});

    LOG_D() << "context grt channel: " << channel
            << "GRT key prefix: " << grt_key_prefix
            << "database name: " << database_name;

    auto i = KeyDatabaseInfo();
    i.channel = channel;
    i.name = database_name;
    i.path = database_path;
    i.backend_type = backend_type;
    reported.append(i);
  }

  // Ordered by channel and holding one entry per live context, rather than
  // indexed by channel: a list indexed by channel has a slot for every channel
  // no context reported, and those slots are key databases that do not exist.
  gpg_key_database_info_cache = BuildGpgKeyDatabaseInfos(reported);

  return gpg_key_database_info_cache;
}

auto GF_CORE_EXPORT GetGpgKeyDatabaseName(int channel) -> QString {
  // Asked by channel, so answered by channel: the list is ordered by channel
  // but not indexed by it, and never was for a profile whose channels are not
  // the contiguous run starting at zero that GFCoreInit usually builds.
  for (const auto& info : GetGpgKeyDatabaseInfos()) {
    if (info.channel == channel) return info.name;
  }
  return {};
}

namespace {

// Set of backend types whose engine is actually available in this build. The
// macOS app sandbox ships the rpgp-only "lite" variant, while Flathub carries
// both gnupg and rpgp.
auto SupportedKeyDatabaseBackends() -> QSet<QString> {
  QSet<QString> backends;
  if (GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG))
    backends.insert("gnupg");
  if (GetGSS().IsEngineSupported(OpenPGPEngine::kRPGP)) backends.insert("rpgp");
  return backends;
}

}  // namespace

auto DefaultKeyDatabaseCandidate() -> KeyDatabaseItemSO {
  KeyDatabaseItemSO key_db;
  key_db.channel = 0;
  key_db.name = QLatin1String(kDefaultKeyDatabaseName);
  key_db.kind = KeyDatabaseKind::kDEFAULT;
  key_db.backend_type = GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)
                            ? QString("gnupg")
                            : QString("rpgp");

  auto home_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.default_database_path", QString{});

  // Left empty rather than invented. This answers a question a caller is
  // allowed to hear "no" to -- the dialog offering to add the DEFAULT database
  // has to be able to say the engine does not name one -- which is the
  // difference between this and MakeDefaultKeyDatabaseItem() below.
  if (home_path.isEmpty()) return key_db;

  // A self-contained profile records its keyring as profile-relative, so the
  // whole profile can be moved, copied or packaged without the path going
  // stale.
  if (GetGSS().IsSelfContainedProfile()) {
    home_path =
        ToProfileRelativeKeyDatabasePath(home_path, GetGSS().GetAppDataPath());
  }

  key_db.path = home_path;
  return key_db;
}

// Sort entries by channel, then resolve duplicate channels by incrementing so
// every key database ends up with a unique, ascending channel.
void GF_CORE_EXPORT
NormalizeKeyDatabaseChannels(QContainer<KeyDatabaseItemSO>& key_dbs) {
  // Stable, because two entries may legitimately arrive holding the same
  // channel -- a list that has never been normalised, or one just given a
  // DEFAULT at channel 0 -- and an unstable sort would decide between them
  // differently from one run to the next. Which database is channel 0 would
  // then not be a fact about the profile at all.
  std::stable_sort(key_dbs.begin(), key_dbs.end(),
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

namespace {

// Build the channel-0 "DEFAULT" key database entry. Its path is derived from
// the gpgme context (or an app-data fallback), never from a fixed user dir.
//
// Unlike DefaultKeyDatabaseCandidate(), this one must always answer: it is what
// a profile falls back to when nothing else is usable, and returning nothing
// there means refusing to start.
auto MakeDefaultKeyDatabaseItem() -> KeyDatabaseItemSO {
  auto key_db = DefaultKeyDatabaseCandidate();
  if (!key_db.path.isEmpty()) return key_db;

  LOG_E() << "failed to get default key database path from gpgme context, "
             "fallback to default app data path";

  // this should not happen, but just in case, fallback to app data path
  auto home_path = GetGSS().GetAppDataPath() + "/rpgp_db";

  // since we cannot get default key database path from gpgme context, it's
  // likely that gpgme is not working properly, we should fallback to rpgp --
  // but only if this build actually has it. Claiming support unconditionally
  // would hand out an engine that does not exist.
  if (HasRustSupport()) {
    GetGSS().AddSupportedEngine(OpenPGPEngine::kRPGP);
  } else {
    LOG_E() << "gpgme is not usable and this build has no rPGP support";
  }

  // Re-picked after the fallback: registering rPGP above can change which
  // engines this build reports as supported.
  key_db.backend_type = GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG)
                            ? QString("gnupg")
                            : QString("rpgp");

  if (GetGSS().IsSelfContainedProfile()) {
    home_path =
        ToProfileRelativeKeyDatabasePath(home_path, GetGSS().GetAppDataPath());
  }

  key_db.path = home_path;
  return key_db;
}

// Scan the fixed sandbox directory (<app-data>/dbs) for user key databases,
// returning a name/path entry per subdirectory found.
auto ScanSandboxKeyDatabaseDir() -> QContainer<KeyDatabaseItemSO> {
  QContainer<KeyDatabaseItemSO> discovered;

  const auto dbs_root =
      GlobalSettingStation::GetInstance().GetAppDataPath() + "/dbs";
  QDir dbs_dir(dbs_root);
  if (!dbs_dir.exists()) {
    LOG_D() << "sandbox key database dir does not exist, skip scan:"
            << dbs_root;
    return discovered;
  }

  const auto entries =
      dbs_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
  for (const auto& entry : entries) {
    KeyDatabaseItemSO key_db;
    key_db.name = entry.fileName();
    key_db.path = entry.absoluteFilePath();
    // Scanned out of the profile's own dbs/ directory, so managed by
    // construction -- there is nowhere else a sandbox database can be.
    key_db.kind = KeyDatabaseKind::kMANAGED;
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
// available, else rpgp) - this is what keeps the rpgp-only macOS lite build
// from honouring a stale "gnupg" type carried over in settings.
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
  default_db.kind = KeyDatabaseKind::kDEFAULT;
  key_dbs.append(default_db);

  int next_channel = 1;
  for (auto& key_db : discovered) {
    if (key_db.name.isEmpty()) continue;
    // Never let a scanned dir shadow the channel-0 DEFAULT database. Compared
    // by the reserved-name rule rather than by string equality, because the
    // directory this came from is on a filesystem where "Default" and "DEFAULT"
    // are the same folder.
    if (IsReservedKeyDatabaseName(key_db.name)) continue;

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
auto ReanchorKeyDatabasePath(const QString& stored_path,
                             const QString& profile_root) -> QString {
  if (profile_root.isEmpty() || stored_path.isEmpty()) return stored_path;
  if (stored_path.startsWith(QLatin1String(kProfilePathToken))) {
    return stored_path;
  }

  // Inside this profile already, just spelled the long way. Tokenising it is
  // the hygiene an export used to do to the live list behind the user's back.
  const auto relative =
      ToProfileRelativeKeyDatabasePath(stored_path, profile_root);
  if (relative.startsWith(QLatin1String(kProfilePathToken))) return relative;

  if (QFileInfo(stored_path).isDir()) return stored_path;

  const auto tail = ForeignKeyDatabasePathTail(stored_path);
  if (tail.isEmpty()) return stored_path;

  const auto candidate = QDir::cleanPath(profile_root + "/" + tail);
  if (!QFileInfo(candidate).isDir()) return stored_path;

  LOG_I() << "re-anchoring a key database path written elsewhere:"
          << stored_path << "->" << candidate;
  return QString::fromLatin1(kProfilePathToken) + "/" + tail;
}

auto LoadKeyDatabaseList() -> QContainer<KeyDatabaseItemSO> {
  return KeyDatabaseListSO(SettingsObject(kKeyDatabaseListObject))
      .key_databases;
}

auto ReconcileKeyDatabaseList(const QContainer<KeyDatabaseItemSO>& stored,
                              const KeyDatabaseItemSO& local_default,
                              const KeyDatabaseItemSO& fallback,
                              const QString& profile_root,
                              const QString& app_data_path)
    -> QContainer<KeyDatabaseItemSO> {
  // Two things answering to one identity is not a state anything below can make
  // sense of, and a stored list is not only ever written by the dialog: it can
  // arrive in a package, be hand-edited, or come from a build older than the
  // rule.
  auto key_dbs = DropDuplicateDefaultKeyDatabases(stored);

  // And the one that remains names this computer's keyring, whatever the
  // settings say it named. That entry is derived rather than stored, so a
  // stored path for it only ever records where it was on whichever machine last
  // wrote the settings -- which, for a profile opened from a package, is
  // somewhere else entirely.
  key_dbs = AdoptLocalDefaultKeyDatabase(key_dbs, local_default.path,
                                         local_default.backend_type);

  // What each database is, settled once here so nothing below has to guess it
  // from a path. An entry from a build before the field existed gets its kind
  // derived the way those builds inferred it.
  key_dbs = ResolveKeyDatabaseKinds(key_dbs, app_data_path);

  QContainer<KeyDatabaseItemSO> kept;
  for (auto key_db : key_dbs) {
    if (key_db.path.isEmpty() || key_db.name.isEmpty()) {
      LOG_W() << "invalid key db info, skip, name:" << key_db.name
              << "key db path:" << key_db.path;
      continue;
    }

    key_db.path = ReanchorKeyDatabasePath(key_db.path, profile_root);
    kept.push_back(key_db);
  }

  // Never hand back nothing. A profile with no usable database cannot open a
  // key list at all, and the honest recovery is this computer's own keyring
  // rather than a refusal to start.
  if (kept.empty()) kept.append(fallback);

  NormalizeKeyDatabaseChannels(kept);
  return kept;
}

void PersistKeyDatabaseList(const QContainer<KeyDatabaseItemSO>& key_dbs) {
  KeyDatabaseListSO list;
  list.key_databases = key_dbs;

  auto so = SettingsObject(kKeyDatabaseListObject);
  so.Store(list.ToJson());
}

auto GetKeyDatabasesBySettings() -> QContainer<KeyDatabaseItemSO> {
  const auto stored = LoadKeyDatabaseList();

  // In the macOS app sandbox the stored settings paths may not match the
  // databases that actually exist under the fixed dbs/ path, so which list is
  // authoritative differs -- but the shape does not: load, reconcile against
  // this computer, persist what came back.
  const auto reconciled =
      IsRunningInAppSandbox()
          ? ReconcileSandboxKeyDatabaseList(MakeDefaultKeyDatabaseItem(),
                                            ScanSandboxKeyDatabaseDir(), stored,
                                            SupportedKeyDatabaseBackends())
          : ReconcileKeyDatabaseList(stored, DefaultKeyDatabaseCandidate(),
                                     MakeDefaultKeyDatabaseItem(),
                                     ProfileSession::Instance().Root(),
                                     GetGSS().GetAppDataPath());

  PersistKeyDatabaseList(reconciled);

  for (const auto& key_db : reconciled) {
    LOG_I() << "got key database from settings:" << key_db.name
            << ", path:" << key_db.path;
  }

  return reconciled;
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

auto DecideKeyDatabasePathAction(bool exists_as_dir, bool exists_as_file,
                                 bool parent_exists, bool inside_app_data)
    -> KeyDatabasePathAction {
  if (exists_as_dir) return KeyDatabasePathAction::kUSE_AS_IS;

  // Something is there and it is not a directory. Never touch it.
  if (exists_as_file) return KeyDatabasePathAction::kREJECT;

  if (parent_exists) return KeyDatabasePathAction::kCREATE_LEAF;

  return inside_app_data ? KeyDatabasePathAction::kCREATE_FULL
                         : KeyDatabasePathAction::kREJECT;
}

auto ToProfileRelativeKeyDatabasePath(const QString& absolute_path,
                                      const QString& profile_root) -> QString {
  const auto root = QDir::cleanPath(profile_root);
  const auto target = QDir::cleanPath(absolute_path);

  if (target == root) return QString::fromLatin1(kProfilePathToken);
  if (!target.startsWith(root + "/")) return absolute_path;

  return QString::fromLatin1(kProfilePathToken) + "/" +
         target.mid(root.size() + 1);
}

auto FromProfileRelativeKeyDatabasePath(const QString& stored_path,
                                        const QString& profile_root)
    -> QString {
  if (stored_path == QLatin1String(kProfilePathToken)) {
    return QDir::cleanPath(profile_root);
  }
  if (!stored_path.startsWith(QLatin1String(kProfilePathToken) + "/")) {
    return stored_path;
  }

  const auto root = QDir::cleanPath(profile_root);
  const auto suffix = stored_path.mid(qstrlen(kProfilePathToken) + 1);
  const auto resolved = QDir::cleanPath(root + "/" + suffix);

  // A stored value is not necessarily one we wrote: a package can carry
  // "@profile/../../elsewhere", and resolving it would put a key database
  // outside the profile that is supposed to contain it.
  if (resolved != root && !resolved.startsWith(root + "/")) {
    LOG_W() << "refusing profile-relative path that escapes the profile:"
            << stored_path;
    return {};
  }
  return resolved;
}

auto ForeignKeyDatabasePathTail(const QString& stored_path) -> QString {
  if (stored_path.isEmpty()) return {};
  if (stored_path.startsWith(QLatin1String(kProfilePathToken))) return {};

  // Both separators, because a Windows path is not absolute to a POSIX build
  // and QDir::cleanPath() therefore leaves its backslashes alone.
  static const QRegularExpression kSeparator(R"([/\\])");
  const auto components = stored_path.split(kSeparator, Qt::SkipEmptyParts);
  if (components.contains("..")) return {};

  const auto managed = ManagedKeyDatabaseDirs();

  // The last match, not the first: a profile lives under a folder that may
  // itself be called anything, and only the innermost one can be the key
  // database directory the profile actually owns.
  //
  // The final component counts too. "db" and "rpgp_db" are key databases in
  // their own right -- the DEFAULT one is exactly that -- and only "dbs" is a
  // container that needs a name after it. Not special-cased, because the
  // caller will not act on a tail the local profile does not actually have.
  for (int i = components.size() - 1; i >= 0; --i) {
    if (!managed.contains(components.at(i))) continue;
    return components.mid(i).join('/');
  }
  return {};
}

auto GetCanonicalKeyDatabasePath(const QDir& app_path, const QString& path)
    -> QString {
  auto target_path = path;

  // Resolved before anything else looks at the string: the token is the only
  // spelling that survives a profile being moved or packaged, and it must not
  // fall through to the relative-to-the-executable branch below, whose anchor
  // is exactly the thing a relocatable profile cannot rely on.
  if (target_path.startsWith(QLatin1String(kProfilePathToken))) {
    target_path = FromProfileRelativeKeyDatabasePath(
        target_path, GlobalSettingStation::GetInstance().GetAppDataPath());
    if (target_path.isEmpty()) return {};
  }

  if (!QDir::isAbsolutePath(target_path)) {
    target_path = app_path.absoluteFilePath(target_path);
    LOG_D() << "convert relative path: " << path
            << "to absolute path: " << target_path;
  }

  QFileInfo info(target_path);

  const auto app_data_path =
      GlobalSettingStation::GetInstance().GetAppDataPath();
  const auto action = DecideKeyDatabasePathAction(
      info.exists() && info.isDir(), info.exists() && !info.isDir(),
      QFileInfo(info.absolutePath()).isDir(),
      info.absoluteFilePath().startsWith(app_data_path));

  switch (action) {
    case KeyDatabasePathAction::kUSE_AS_IS:
      break;

    case KeyDatabasePathAction::kCREATE_LEAF:
    case KeyDatabasePathAction::kCREATE_FULL:
      LOG_W() << "key database dir does not exist:" << info.absoluteFilePath()
              << ", creating...";
      if (!QDir().mkpath(info.absoluteFilePath())) {
        LOG_E() << "failed to create key database dir:"
                << info.absoluteFilePath();
      }
      // QFileInfo caches its stat on first use, so the checks below would
      // still see the directory as missing without this.
      info.refresh();
      break;

    case KeyDatabasePathAction::kREJECT:
      LOG_W() << "refusing to create key database dir:"
              << info.absoluteFilePath()
              << "- its parent is missing or a file is in the way";
      break;
  }

  if (VerifyKeyDatabasePath(info)) {
    auto key_database_fs_path = info.canonicalFilePath();
    LOG_D() << "load gpg key database:" << key_database_fs_path;

    return key_database_fs_path;
  }

  LOG_W() << "gpg key database path is invalid: " << path;
  return {};
}

namespace {

// The channel-0 DEFAULT database, resolved the same way a configured one is.
// Derived fresh rather than read from settings, because it is needed precisely
// when what is in settings turned out to be unusable.
auto MakeDefaultKeyDatabaseInfo() -> KeyDatabaseInfo {
  const auto item = MakeDefaultKeyDatabaseItem();
  const auto app_path = QDir(GlobalSettingStation::GetInstance().GetAppDir());
  const auto fs_path = GetCanonicalKeyDatabasePath(app_path, item.path);

  KeyDatabaseInfo info;
  info.name = item.name;
  info.backend_type = item.backend_type;
  info.path = fs_path;
  info.origin_path = item.path;
  info.channel = 0;
  info.valid = !fs_path.isEmpty();
  info.kind = KeyDatabaseKind::kDEFAULT;
  return info;
}

}  // namespace

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
    // Carried over rather than left unset: the stored entry is where the
    // channel of a configured database comes from, and an info that does not
    // know its channel is one no caller can ask a context about.
    key_db_info.channel = key_database.channel;
    key_db_info.name = key_database.name;
    key_db_info.backend_type = key_database.backend_type;
    key_db_info.path = key_database_fs_path;
    key_db_info.origin_path = key_database.path;
    key_db_info.valid = !key_database_fs_path.isEmpty();
    key_db_info.kind = key_database.kind.value_or(KeyDatabaseKind::kEXTERNAL);
    key_db_infos.append(key_db_info);

    LOG_D() << "plan to load gpg key database at:" << key_database_fs_path;
  }

  return key_db_infos;
}

auto SelectUsableKeyDatabases(const QContainer<KeyDatabaseInfo>& all,
                              const KeyDatabaseInfo& fallback)
    -> QContainer<KeyDatabaseInfo> {
  QContainer<KeyDatabaseInfo> usable;

  for (const auto& key_db_info : all) {
    if (key_db_info.valid) usable.append(key_db_info);
  }

  if (!usable.isEmpty()) return usable;

  // Nothing configured resolved to a real directory. Coming up on a freshly
  // derived DEFAULT beats refusing to start: an empty keyring is visible and
  // recoverable, a dead application is neither.
  if (fallback.valid) {
    LOG_W() << "no configured key database is usable, falling back to the "
               "default database at:"
            << fallback.path;
    usable.append(fallback);
    return usable;
  }

  LOG_E() << "no configured key database is usable and the default database "
             "could not be derived either";
  return usable;
}

auto GetKeyDatabaseInfoBySettings() -> QContainer<KeyDatabaseInfo> {
  auto fallback = MakeDefaultKeyDatabaseInfo();
  auto key_db_infos =
      SelectUsableKeyDatabases(GetAllKeyDatabaseInfoBySettings(), fallback);

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

auto ChooseOpenPGPEngine(const QString& preferred, bool gnupg_supported,
                         bool rpgp_supported) -> EngineChoice {
  const auto token = preferred.trimmed().toLower();

  if (token == "gnupg" && gnupg_supported) {
    return {true, OpenPGPEngine::kGNUPG};
  }
  if (token == "rpgp" && rpgp_supported) {
    return {true, OpenPGPEngine::kRPGP};
  }

  // Preference unusable or unstated: take whatever this build actually has,
  // GnuPG first to preserve the historical default.
  if (gnupg_supported) return {true, OpenPGPEngine::kGNUPG};
  if (rpgp_supported) return {true, OpenPGPEngine::kRPGP};

  return {};
}

auto ChooseKeyDatabaseEngine(const QString& backend_type,
                             const QString& fallback_engine,
                             bool gnupg_supported, bool rpgp_supported)
    -> EngineChoice {
  return ChooseOpenPGPEngine(
      backend_type.trimmed().isEmpty() ? fallback_engine : backend_type,
      gnupg_supported, rpgp_supported);
}

auto DropDuplicateDefaultKeyDatabases(
    const QContainer<KeyDatabaseItemSO>& databases)
    -> QContainer<KeyDatabaseItemSO> {
  QContainer<KeyDatabaseItemSO> out;
  out.reserve(databases.size());

  auto seen_default = false;
  for (const auto& item : databases) {
    if (!IsReservedKeyDatabaseName(item.name)) {
      out.push_back(item);
      continue;
    }
    if (seen_default) {
      LOG_W() << "a second key database is named for the default one, "
                 "dropping the entry (its folder is untouched):"
              << item.path;
      continue;
    }
    seen_default = true;
    out.push_back(item);
  }
  return out;
}

auto AdoptLocalDefaultKeyDatabase(
    const QContainer<KeyDatabaseItemSO>& databases, const QString& local_path,
    const QString& local_backend) -> QContainer<KeyDatabaseItemSO> {
  if (local_path.isEmpty()) return databases;

  QContainer<KeyDatabaseItemSO> out;
  out.reserve(databases.size());

  for (auto item : databases) {
    if (IsReservedKeyDatabaseName(item.name) && item.path != local_path) {
      LOG_I() << "pointing the default key database at this computer's own:"
              << item.path << "->" << local_path;
      item.path = local_path;
      if (!local_backend.isEmpty()) item.backend_type = local_backend;
    }
    out.push_back(item);
  }
  return out;
}

auto IsReservedKeyDatabaseName(const QString& name) -> bool {
  return name.trimmed().compare(QLatin1String(kDefaultKeyDatabaseName),
                                Qt::CaseInsensitive) == 0;
}

auto ChooseChannelZeroEngine(const QString& db_name,
                             const QString& backend_type,
                             const QString& default_engine,
                             bool gnupg_supported, bool rpgp_supported)
    -> EngineChoice {
  // The reserved-name rule, not string equality: every other site that asks
  // "is this the default one" trims and ignores case, and a channel-0 entry
  // spelled "default" would otherwise be handed the wrong engine.
  const auto is_default = IsReservedKeyDatabaseName(db_name);
  return ChooseKeyDatabaseEngine(is_default ? default_engine : backend_type,
                                 default_engine, gnupg_supported,
                                 rpgp_supported);
}

auto ClassifyKeyDatabase(const QString& name, const QString& stored_path,
                         const QString& profile_root) -> KeyDatabaseKind {
  // The name settles it on its own. The DEFAULT database is an identity rather
  // than a location, and its stored path is replaced on every read, so asking
  // where it sits would be asking about a value nothing depends on.
  if (IsReservedKeyDatabaseName(name)) return KeyDatabaseKind::kDEFAULT;

  if (stored_path.isEmpty() || profile_root.isEmpty()) {
    return KeyDatabaseKind::kEXTERNAL;
  }

  // Tokenised first so the long spelling and the `@profile` one are recognised
  // as the same directory; which of them is in the settings file is an accident
  // of which build wrote it.
  const auto relative =
      ToProfileRelativeKeyDatabasePath(stored_path, profile_root);
  if (!relative.startsWith(QLatin1String(kProfilePathToken) + "/")) {
    return KeyDatabaseKind::kEXTERNAL;
  }

  const auto tail =
      relative.mid(static_cast<int>(qstrlen(kProfilePathToken)) + 1);
  return IsManagedKeyDatabasePath(tail) ? KeyDatabaseKind::kMANAGED
                                        : KeyDatabaseKind::kEXTERNAL;
}

auto ResolveKeyDatabaseKinds(const QContainer<KeyDatabaseItemSO>& databases,
                             const QString& profile_root)
    -> QContainer<KeyDatabaseItemSO> {
  QContainer<KeyDatabaseItemSO> out;
  out.reserve(databases.size());

  for (auto item : databases) {
    // The name wins over anything recorded. Every other rule about the default
    // database goes by the name, and a kind that disagreed with it would put
    // this one check out of step with all of them.
    if (IsReservedKeyDatabaseName(item.name)) {
      item.kind = KeyDatabaseKind::kDEFAULT;
      out.push_back(item);
      continue;
    }

    if (!item.kind) {
      item.kind = ClassifyKeyDatabase(item.name, item.path, profile_root);
      LOG_D() << "settled the kind of an untyped key database:" << item.name
              << "->" << ConvertKeyDatabaseKind2String(*item.kind);
    } else if (*item.kind == KeyDatabaseKind::kDEFAULT) {
      // Recorded as the default one while not wearing the reserved name. It
      // cannot be: the name is the identity, and only one thing holds it.
      item.kind = ClassifyKeyDatabase(item.name, item.path, profile_root);
    }

    out.push_back(item);
  }
  return out;
}

auto ManagedKeyDatabasePath(const QString& app_data_path, const QString& name)
    -> QString {
  const auto trimmed = name.trimmed();
  if (app_data_path.isEmpty() || trimmed.isEmpty()) return {};

  return QDir::cleanPath(app_data_path + "/dbs/" + trimmed);
}

auto ComposeKeyDatabaseList(const std::optional<KeyDatabaseItemSO>& default_db,
                            const QContainer<KeyDatabaseItemSO>& managed,
                            const QContainer<KeyDatabaseItemSO>& external)
    -> QContainer<KeyDatabaseItemSO> {
  QContainer<KeyDatabaseItemSO> out;
  out.reserve((default_db ? 1 : 0) + managed.size() + external.size());

  if (default_db) out.push_back(*default_db);
  for (const auto& item : managed) out.push_back(item);
  for (const auto& item : external) out.push_back(item);

  // Numbered here rather than left to NormalizeKeyDatabaseChannels(): the order
  // is the whole point of this function, and handing the normaliser a list of
  // entries carrying stale channels would let it reorder what was just
  // arranged.
  for (int i = 0; i < out.size(); ++i) out[i].channel = i;

  return out;
}

auto DecideManagedRename(bool old_exists, bool new_exists)
    -> ManagedRenameAction {
  if (new_exists) return ManagedRenameAction::kTARGET_TAKEN;
  return old_exists ? ManagedRenameAction::kRENAME
                    : ManagedRenameAction::kNOTHING_TO_MOVE;
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

auto IsKeyNeverExpires(const GpgAbstractKey* key) -> bool {
  if (key == nullptr) return true;
  return key->ExpirationTime().toSecsSinceEpoch() == 0;
}

auto GetKeyExpiringSoonDays() -> int {
  auto days = GetSettings().value("keys/expiring_soon_days", 30).toInt();
  return std::clamp(days, 1, 365);
}

auto IsKeyExpiringSoon(const GpgAbstractKey* key) -> bool {
  if (key == nullptr) return false;
  if (key->IsRevoked() || key->IsDisabled() || key->IsExpired()) return false;
  if (IsKeyNeverExpires(key)) return false;

  const auto now = QDateTime::currentDateTime();
  const auto expires = key->ExpirationTime();
  return expires > now && now.daysTo(expires) <= GetKeyExpiringSoonDays();
}

auto ClassifyKeyStatus(bool revoked, bool disabled, bool expired,
                       bool expiring_soon) -> GpgKeyStatus {
  // Order matters and is the whole point of this function: a key can be
  // several of these at once, and the row tint has always shown disabled
  // first, then expired-or-revoked, then expiring-soon.
  if (disabled) return GpgKeyStatus::kDisabled;
  if (revoked) return GpgKeyStatus::kRevoked;
  if (expired) return GpgKeyStatus::kExpired;
  if (expiring_soon) return GpgKeyStatus::kExpiringSoon;
  return GpgKeyStatus::kOk;
}

auto KeyStatusSortRank(GpgKeyStatus status) -> int {
  switch (status) {
    case GpgKeyStatus::kOk:
      return 0;
    case GpgKeyStatus::kExpiringSoon:
      return 1;
    case GpgKeyStatus::kExpired:
      return 2;
    case GpgKeyStatus::kRevoked:
      return 3;
    case GpgKeyStatus::kDisabled:
      return 4;
  }
  return 0;
}

auto DescribeKeyStatus(GpgKeyStatus status) -> QString {
  switch (status) {
    case GpgKeyStatus::kOk:
      return QCoreApplication::translate("GpgFrontend", "OK");
    case GpgKeyStatus::kExpiringSoon:
      return QCoreApplication::translate("GpgFrontend", "Expiring Soon");
    case GpgKeyStatus::kExpired:
      return QCoreApplication::translate("GpgFrontend", "Expired");
    case GpgKeyStatus::kRevoked:
      return QCoreApplication::translate("GpgFrontend", "Revoked");
    case GpgKeyStatus::kDisabled:
      return QCoreApplication::translate("GpgFrontend", "Disabled");
  }
  return {};
}

auto AggregateOwnerTrustRank(const QContainer<int>& levels) -> int {
  if (levels.isEmpty()) return -1;

  const auto first = levels.front();
  for (const auto level : levels) {
    if (level != first) return -1;
  }
  return first;
}

auto GetKeyStatus(const GpgAbstractKey* key) -> GpgKeyStatus {
  if (key == nullptr) return GpgKeyStatus::kOk;
  return ClassifyKeyStatus(key->IsRevoked(), key->IsDisabled(),
                           key->IsExpired(), IsKeyExpiringSoon(key));
}
}  // namespace GpgFrontend
