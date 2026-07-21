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

#include "DataObjectOperator.h"

#include <sodium.h>

#include <cstring>

#include "core/GFCoreInit.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/PassphraseGenerator.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/IOUtils.h"

namespace {

constexpr std::string_view kObjectKeyDeriveDomain = "GpgFrontend.DataObject.v2";

enum class DataObjectHealth {
  kOK,
  kINVALID_FILE_NAME,
  kFILE_TOO_SMALL,
  kREAD_FAILED,
  kMISSING_KEY,
  kDECRYPT_FAILED,
  kINVALID_JSON,
};

struct DataObjectCheckResult {
  QString ref_hex;
  QString path;
  DataObjectHealth health;
  QByteArray key_id_hex;
  qint64 size = 0;
  QDateTime last_modified;
};

enum class DataObjectGCAction {
  kGCA_NONE,
  kGCA_QUARANTINE,
  kGCA_DELETE,
};

struct DataObjectGCPolicy {
  bool dry_run = true;
  bool delete_missing_key = false;
  bool delete_decrypt_failed = false;
  int missing_key_grace_days = 30;
  int decrypt_failed_grace_days = 30;
  int quarantine_grace_days = 7;
};

struct DataObjectGCReport {
  int total = 0;
  int ok = 0;
  int invalid_name = 0;
  int missing_key = 0;
  int decrypt_failed = 0;
  int file_too_small = 0;
  int read_failed = 0;
  int invalid_json = 0;
  int quarantined = 0;
  int deleted = 0;
};

struct DataObjectGCContext {
  DataObjectGCPolicy policy;
  DataObjectGCReport report;
  QJsonObject state;
  QString state_path;
  bool state_dirty = false;
};

auto DeriveObjectKey(const GpgFrontend::GFBuffer& key,
                     const GpgFrontend::GFBuffer& context)
    -> GpgFrontend::GFBufferOrNone {
  if (!GpgFrontend::EnsureSodiumInit()) return {};

  if (key.Empty() || context.Empty()) {
    LOG_E() << "DeriveObjectKey received empty key or context";
    return {};
  }

  GpgFrontend::GFBuffer normalized_key(crypto_generichash_KEYBYTES);

  if (key.Size() == crypto_generichash_KEYBYTES) {
    std::memcpy(normalized_key.Data(), key.Data(), normalized_key.Size());
  } else {
    const int key_hash_rc = crypto_generichash(
        reinterpret_cast<unsigned char*>(normalized_key.Data()),
        normalized_key.Size(),
        reinterpret_cast<const unsigned char*>(key.Data()),
        static_cast<unsigned long long>(key.Size()), nullptr, 0);

    if (key_hash_rc != 0) {
      LOG_E() << "crypto_generichash normalize key failed";
      return {};
    }
  }

  GpgFrontend::GFBuffer input;
  input.Combine({
      GpgFrontend::GFBuffer(kObjectKeyDeriveDomain.data(),
                            kObjectKeyDeriveDomain.size()),
      context,
  });

  GpgFrontend::GFBuffer out(crypto_aead_xchacha20poly1305_ietf_KEYBYTES);

  const int rc = crypto_generichash(
      reinterpret_cast<unsigned char*>(out.Data()), out.Size(),
      reinterpret_cast<const unsigned char*>(input.Data()),
      static_cast<unsigned long long>(input.Size()),
      reinterpret_cast<const unsigned char*>(normalized_key.Data()),
      normalized_key.Size());

  sodium_memzero(normalized_key.Data(), normalized_key.Size());

  if (rc != 0) {
    LOG_E() << "crypto_generichash derive object key failed";
    return {};
  }

  return out;
}

auto CheckDataObjectByRef(const QString& ref_hex, bool expect_json)
    -> DataObjectCheckResult {
  DataObjectCheckResult r;
  r.ref_hex = ref_hex;

  if (ref_hex.size() != 64 ||
      QByteArray::fromHex(ref_hex.toLatin1()).size() != 32) {
    r.health = DataObjectHealth::kINVALID_FILE_NAME;
    return r;
  }

  const auto ref =
      GpgFrontend::GFBuffer(QByteArray::fromHex(ref_hex.toLatin1()));
  const auto ref_path =
      GpgFrontend::GetGSS().GetDataObjectsDir() + "/" + ref_hex;
  r.path = ref_path;

  QFileInfo fi(ref_path);
  r.size = fi.size();
  r.last_modified = fi.lastModified();

  auto [succ, data] = GpgFrontend::ReadFileGFBuffer(ref_path);
  if (!succ) {
    r.health = DataObjectHealth::kREAD_FAILED;
    return r;
  }

  if (data.Size() <= 32) {
    r.health = DataObjectHealth::kFILE_TOO_SMALL;
    return r;
  }

  auto key_id = data.Left(32);
  r.key_id_hex = key_id.ConvertToQByteArray().toHex();

  auto key = GpgFrontend::AppSecureKeyManager::GetInstance().GetKey(key_id);
  if (key.Empty()) {
    r.health = DataObjectHealth::kMISSING_KEY;
    return r;
  }

  auto encrypted = data.Right(static_cast<int>(data.Size() - key_id.Size()));
  if (encrypted.Empty()) {
    r.health = DataObjectHealth::kFILE_TOO_SMALL;
    return r;
  }

  auto drv_key = DeriveObjectKey(key, ref);
  if (!drv_key) {
    r.health = DataObjectHealth::kDECRYPT_FAILED;
    return r;
  }

  auto plaintext =
      GpgFrontend::GFBufferFactory::DecryptLite(*drv_key, encrypted);
  if (!plaintext) {
    r.health = DataObjectHealth::kDECRYPT_FAILED;
    return r;
  }

  if (expect_json) {
    QJsonParseError err;
    QJsonDocument::fromJson(plaintext->ConvertToQByteArray(), &err);
    if (err.error != QJsonParseError::NoError) {
      r.health = DataObjectHealth::kINVALID_JSON;
      return r;
    }
  }

  r.health = DataObjectHealth::kOK;
  return r;
}

auto IsValidRefHex(const QString& s) -> bool {
  return s.size() == 64 && QByteArray::fromHex(s.toLatin1()).size() == 32;
}

auto NowUtc() -> QDateTime { return QDateTime::currentDateTimeUtc(); }

auto EnsureDir(const QString& path) -> bool {
  QDir dir(path);
  if (dir.exists()) return true;
  return dir.mkpath(".");
}

auto DataObjectQuarantineDir(GpgFrontend::GlobalSettingStation& gss)
    -> QString {
  return gss.GetDataObjectsDir() + ".quarantine";
}

auto DataObjectGcStatePath(GpgFrontend::GlobalSettingStation& gss) -> QString {
  return gss.GetDataObjectsDir() + ".gc.json";
}

auto HealthToString(DataObjectHealth h) -> QString {
  switch (h) {
    case DataObjectHealth::kOK:
      return "ok";
    case DataObjectHealth::kINVALID_FILE_NAME:
      return "invalid_file_name";
    case DataObjectHealth::kFILE_TOO_SMALL:
      return "file_too_small";
    case DataObjectHealth::kREAD_FAILED:
      return "read_failed";
    case DataObjectHealth::kMISSING_KEY:
      return "missing_key";
    case DataObjectHealth::kDECRYPT_FAILED:
      return "decrypt_failed";
    case DataObjectHealth::kINVALID_JSON:
      return "invalid_json";
  }
  return "unknown";
}

auto IsDeleteAllowed(DataObjectHealth h, const DataObjectGCPolicy& policy)
    -> bool {
  switch (h) {
    case DataObjectHealth::kINVALID_FILE_NAME:
    case DataObjectHealth::kFILE_TOO_SMALL:
      return true;

    case DataObjectHealth::kMISSING_KEY:
      return policy.delete_missing_key;

    case DataObjectHealth::kDECRYPT_FAILED:
      return policy.delete_decrypt_failed;

    case DataObjectHealth::kREAD_FAILED:
      return false;

    case DataObjectHealth::kINVALID_JSON:
      return false;

    case DataObjectHealth::kOK:
      return false;
  }

  return false;
}

auto GraceDaysFor(DataObjectHealth h, const DataObjectGCPolicy& policy) -> int {
  switch (h) {
    case DataObjectHealth::kMISSING_KEY:
      return policy.missing_key_grace_days;

    case DataObjectHealth::kDECRYPT_FAILED:
      return policy.decrypt_failed_grace_days;

    case DataObjectHealth::kFILE_TOO_SMALL:
    case DataObjectHealth::kINVALID_FILE_NAME:
    default:
      return policy.quarantine_grace_days;
  }
}

auto LoadGcState(const QString& path) -> QJsonObject {
  QFile f(path);
  if (!f.exists()) return {};

  if (!f.open(QIODevice::ReadOnly)) {
    LOG_W() << "failed to open data object gc state:" << path;
    return {};
  }

  QJsonParseError err;
  const auto doc = QJsonDocument::fromJson(f.readAll(), &err);
  if (err.error != QJsonParseError::NoError || !doc.isObject()) {
    LOG_W() << "failed to parse data object gc state:" << path
            << err.errorString();
    return {};
  }

  return doc.object();
}

auto SaveGcState(const QString& path, const QJsonObject& state) -> bool {
  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    LOG_W() << "failed to write data object gc state:" << path;
    return false;
  }

  f.write(QJsonDocument(state).toJson(QJsonDocument::Indented));
  return true;
}

auto DaysSince(const QString& iso_utc) -> int {
  const auto t = QDateTime::fromString(iso_utc, Qt::ISODateWithMs);
  if (!t.isValid()) return 0;

  return static_cast<int>(t.daysTo(NowUtc()));
}

auto MoveFileReplacing(const QString& src, const QString& dst) -> bool {
  QFile::remove(dst);

  if (QFile::rename(src, dst)) return true;

  if (!QFile::copy(src, dst)) return false;

  return QFile::remove(src);
}

auto MaybeQuarantineOrDelete(const DataObjectCheckResult& result,
                             DataObjectGCContext* ctx) -> DataObjectGCAction {
  if (ctx == nullptr) return DataObjectGCAction::kGCA_NONE;

  const auto& policy = ctx->policy;
  auto& report = ctx->report;
  auto& state = ctx->state;

  if (result.health == DataObjectHealth::kOK || result.path.isEmpty()) {
    return DataObjectGCAction::kGCA_NONE;
  }

  QFileInfo src_info(result.path);
  if (!src_info.exists()) {
    return DataObjectGCAction::kGCA_NONE;
  }

  const auto ref_hex =
      result.ref_hex.isEmpty() ? src_info.fileName() : result.ref_hex;

  auto& gss = GpgFrontend::GetGSS();

  const auto now = NowUtc().toString(Qt::ISODateWithMs);
  auto item = state.value(ref_hex).toObject();

  if (item.isEmpty()) {
    item.insert("ref", ref_hex);
    item.insert("first_seen", now);
  }

  item.insert("last_seen", now);
  item.insert("reason", HealthToString(result.health));
  item.insert("path", result.path);
  item.insert("size", static_cast<double>(result.size));

  if (!result.key_id_hex.isEmpty()) {
    item.insert("key_id", QString::fromLatin1(result.key_id_hex));
  }

  const auto first_seen = item.value("first_seen").toString(now);
  const auto grace_days = GraceDaysFor(result.health, policy);

  const auto quarantine_dir = DataObjectQuarantineDir(gss);
  const auto quarantine_abs = QFileInfo(quarantine_dir).absoluteFilePath();
  const bool already_quarantined = src_info.absolutePath() == quarantine_abs;

  const auto base_time_str =
      already_quarantined ? item.value("quarantined_at").toString(first_seen)
                          : first_seen;

  const auto age_days = DaysSince(base_time_str);

  // Files already in quarantine were approved for deletion on a previous run.
  // IsDeleteAllowed only applies to the decision to quarantine from the main
  // dir, so check this before the delete_allowed gate.
  if (already_quarantined) {
    if (policy.dry_run) {
      return DataObjectGCAction::kGCA_NONE;
    }

    if (age_days < policy.quarantine_grace_days) {
      item.insert("action", "quarantined_waiting_delete");
      item.insert("grace_days", policy.quarantine_grace_days);
      state.insert(ref_hex, item);
      ctx->state_dirty = true;
      return DataObjectGCAction::kGCA_NONE;
    }

    if (!QFile::remove(result.path)) {
      item.insert("action", "delete_failed");
      state.insert(ref_hex, item);
      ctx->state_dirty = true;

      LOG_W() << "failed to delete quarantined data object:" << result.path;
      return DataObjectGCAction::kGCA_NONE;
    }

    state.remove(ref_hex);
    ctx->state_dirty = true;

    report.deleted++;

    LOG_W() << "deleted quarantined data object:" << ref_hex;
    return DataObjectGCAction::kGCA_DELETE;
  }

  const bool delete_allowed = IsDeleteAllowed(result.health, policy);
  if (!delete_allowed) {
    item.insert("action", "record_only");
    state.insert(ref_hex, item);
    ctx->state_dirty = true;
    return DataObjectGCAction::kGCA_NONE;
  }

  if (policy.dry_run) {
    item.insert("action", "dry_run");
    item.insert("would_quarantine_or_delete", true);
    state.insert(ref_hex, item);
    ctx->state_dirty = true;
    return DataObjectGCAction::kGCA_NONE;
  }

  if (result.health == DataObjectHealth::kMISSING_KEY &&
      age_days < grace_days) {
    item.insert("action", "missing_key_waiting");
    item.insert("grace_days", grace_days);
    state.insert(ref_hex, item);
    ctx->state_dirty = true;

    LOG_W() << "data object missing key, waiting grace period, ref:" << ref_hex;
    return DataObjectGCAction::kGCA_NONE;
  }

  if (!EnsureDir(quarantine_dir)) {
    item.insert("action", "quarantine_failed");
    state.insert(ref_hex, item);
    ctx->state_dirty = true;

    LOG_W() << "failed to create quarantine dir:" << quarantine_dir;
    return DataObjectGCAction::kGCA_NONE;
  }

  const auto quarantine_path = quarantine_dir + "/" + src_info.fileName();

  if (!MoveFileReplacing(result.path, quarantine_path)) {
    item.insert("action", "quarantine_failed");
    state.insert(ref_hex, item);
    ctx->state_dirty = true;

    LOG_W() << "failed to quarantine data object:" << result.path
            << "to:" << quarantine_path;
    return DataObjectGCAction::kGCA_NONE;
  }

  item.insert("action", "quarantined");
  item.insert("quarantine_path", quarantine_path);
  item.insert("quarantined_at", now);
  state.insert(ref_hex, item);
  ctx->state_dirty = true;

  report.quarantined++;

  LOG_W() << "quarantined data object:" << ref_hex
          << "reason:" << HealthToString(result.health);

  return DataObjectGCAction::kGCA_QUARANTINE;
}

auto CompactGcState(DataObjectGCContext* ctx) -> void {
  if (ctx == nullptr || ctx->state.isEmpty()) return;

  QJsonObject compacted;
  const auto now = NowUtc();

  for (auto it = ctx->state.begin(); it != ctx->state.end(); ++it) {
    const auto ref = it.key();
    const auto item = it.value().toObject();

    if (item.isEmpty()) {
      ctx->state_dirty = true;
      continue;
    }

    const auto path = item.value("path").toString();
    const auto quarantine_path = item.value("quarantine_path").toString();
    const auto action = item.value("action").toString();
    const auto last_seen_str = item.value("last_seen").toString();

    const bool src_exists = !path.isEmpty() && QFileInfo::exists(path);
    const bool quarantine_exists =
        !quarantine_path.isEmpty() && QFileInfo::exists(quarantine_path);

    if (!src_exists && !quarantine_exists) {
      ctx->state_dirty = true;
      continue;
    }

    if (src_exists && IsValidRefHex(QFileInfo(path).fileName())) {
      const auto check =
          CheckDataObjectByRef(QFileInfo(path).fileName(), false);

      if (check.health == DataObjectHealth::kOK) {
        ctx->state_dirty = true;
        continue;
      }
    }

    const auto last_seen =
        QDateTime::fromString(last_seen_str, Qt::ISODateWithMs);

    if (last_seen.isValid()) {
      const auto age_days = static_cast<int>(last_seen.daysTo(now));

      if (action == "dry_run" && age_days >= 7) {
        ctx->state_dirty = true;
        continue;
      }

      if (action == "record_only" && age_days >= 90) {
        ctx->state_dirty = true;
        continue;
      }

      if ((action == "delete_failed" || action == "quarantine_failed") &&
          age_days >= 90) {
        ctx->state_dirty = true;
        continue;
      }
    }

    compacted.insert(ref, item);
  }

  if (compacted.size() != ctx->state.size()) {
    ctx->state = compacted;
    ctx->state_dirty = true;
  }
}

auto SaveOrRemoveGcState(const QString& path, const QJsonObject& state)
    -> bool {
  if (state.isEmpty()) {
    if (QFileInfo::exists(path)) {
      return QFile::remove(path);
    }
    return true;
  }

  return SaveGcState(path, state);
}

auto RunDataObjectGC(const DataObjectGCPolicy& policy) -> DataObjectGCReport {
  auto& gss = GpgFrontend::GetGSS();

  DataObjectGCContext ctx;
  ctx.policy = policy;
  ctx.state_path = DataObjectGcStatePath(gss);
  ctx.state = LoadGcState(ctx.state_path);

  const QDir dir(gss.GetDataObjectsDir());

  if (dir.exists()) {
    const auto entries =
        dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    for (const auto& info : entries) {
      ctx.report.total++;

      const auto name = info.fileName();

      DataObjectCheckResult result;

      if (IsValidRefHex(name)) {
        result = CheckDataObjectByRef(name, false);
      } else {
        result.ref_hex = name;
        result.path = info.absoluteFilePath();
        result.health = DataObjectHealth::kINVALID_FILE_NAME;
        result.size = info.size();
        result.last_modified = info.lastModified();
      }

      switch (result.health) {
        case DataObjectHealth::kOK:
          ctx.report.ok++;

          // If the object is healthy but has an existing GC state entry, it
          // means the object was previously unhealthy but has since recovered
          // (e.g. the key was restored). In this case we can clean up the GC
          // state entry.
          if (ctx.state.contains(name)) {
            ctx.state.remove(name);
            ctx.state_dirty = true;
          }
          continue;

        case DataObjectHealth::kINVALID_FILE_NAME:
          ctx.report.invalid_name++;
          break;

        case DataObjectHealth::kFILE_TOO_SMALL:
          ctx.report.file_too_small++;
          break;

        case DataObjectHealth::kREAD_FAILED:
          ctx.report.read_failed++;
          break;

        case DataObjectHealth::kMISSING_KEY:
          ctx.report.missing_key++;
          break;

        case DataObjectHealth::kDECRYPT_FAILED:
          ctx.report.decrypt_failed++;
          break;

        case DataObjectHealth::kINVALID_JSON:
          ctx.report.invalid_json++;
          break;
      }

      MaybeQuarantineOrDelete(result, &ctx);
    }
  }

  const auto quarantine_dir = DataObjectQuarantineDir(gss);
  const QDir qdir(quarantine_dir);

  if (qdir.exists()) {
    const auto quarantine_entries =
        qdir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    for (const auto& info : quarantine_entries) {
      ctx.report.total++;

      DataObjectCheckResult result;
      result.ref_hex = info.fileName();
      result.path = info.absoluteFilePath();
      result.health = IsValidRefHex(info.fileName())
                          ? DataObjectHealth::kDECRYPT_FAILED
                          : DataObjectHealth::kINVALID_FILE_NAME;
      result.size = info.size();
      result.last_modified = info.lastModified();

      MaybeQuarantineOrDelete(result, &ctx);
    }
  }

  CompactGcState(&ctx);

  if (ctx.state_dirty) {
    SaveOrRemoveGcState(ctx.state_path, ctx.state);
  }

  return ctx.report;
}

auto NormalPolicy() -> DataObjectGCPolicy {
  return {
      .dry_run = false,
      .delete_missing_key = true,
      .delete_decrypt_failed = false,
      .missing_key_grace_days = 7,
      .decrypt_failed_grace_days = 7,
      .quarantine_grace_days = 30,
  };
}

auto HighSecurityPolicy() -> DataObjectGCPolicy {
  return {
      .dry_run = false,
      .delete_missing_key = true,
      .delete_decrypt_failed = true,
      .missing_key_grace_days = 3,
      .decrypt_failed_grace_days = 3,
      .quarantine_grace_days = 7,
  };
}

}  // namespace

namespace GpgFrontend {
DataObjectOperator::DataObjectOperator(int channel)
    : SingletonFunctionObject<DataObjectOperator>(channel) {
  key_ = key_mgr_.GetActiveKey();
  Q_ASSERT(!key_.Empty());

  key_id_ = key_mgr_.GetActiveKeyId();
  Q_ASSERT(!key_id_.Empty());

  l_key_ = key_mgr_.GetLegacyKey();
  Q_ASSERT(!l_key_.Empty());

  auto secure_level = SecureLevelFromApp();

  // Do not spawn the background GC task during shutdown. Teardown can lazily
  // reconstruct this operator (e.g. via CacheManager flush) after the task
  // runners were stopped; the GC task would then run on a freshly-started IO
  // thread and dereference singleton storage that DestroyGpgFrontendCore() is
  // concurrently destroying — a use-after-free.
  if (IsCoreShuttingDown()) {
    FLOG_D("core is shutting down, skip posting data object gc task");
    return;
  }

  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_IO)
      ->PostTask("data_object_gc",
                 [secure_level](const DataObjectPtr&) -> int {
                   auto report =
                       RunDataObjectGC(secure_level >= 2 ? HighSecurityPolicy()
                                                         : NormalPolicy());
                   LOG_I() << "data object GC run report: total="
                           << report.total << " ok=" << report.ok
                           << " invalid_name=" << report.invalid_name
                           << " file_too_small=" << report.file_too_small
                           << " read_failed=" << report.read_failed
                           << " missing_key=" << report.missing_key
                           << " decrypt_failed=" << report.decrypt_failed
                           << " invalid_json=" << report.invalid_json
                           << " quarantined=" << report.quarantined
                           << " deleted=" << report.deleted;
                   return 0;
                 },
                 Thread::Task::TaskCallback{}, {});
}

auto DataObjectOperator::StoreDataObj(const QString& key,
                                      const QJsonDocument& value) -> QString {
  return StoreSecDataObj(key, GFBuffer(value.toJson()));
}

auto DataObjectOperator::GetDataObject(const QString& key)
    -> std::optional<QJsonDocument> {
  if (key_.Empty()) return {};
  return read_decr_json_object(get_object_ref(key));
}

auto DataObjectOperator::GetDataObjectByRef(const QString& ref)
    -> std::optional<QJsonDocument> {
  if (key_.Empty() || !IsValidRefHex(ref)) return {};
  return read_decr_json_object(GFBuffer(QByteArray::fromHex(ref.toLatin1())));
}

auto DataObjectOperator::StoreSecDataObj(const QString& key,
                                         const GFBuffer& value) -> QString {
  if (key_.Empty()) return {};

  // recreate if not exists
  if (!QDir(gss_.GetDataObjectsDir()).exists()) {
    QDir(gss_.GetDataObjectsDir()).mkpath(".");
  }

  auto ref = get_object_ref(key);
  return write_encr_object(ref, value);
}

auto DataObjectOperator::GetSecDataObject(const QString& key)
    -> GFBufferOrNone {
  if (key_.Empty()) return {};
  return read_decr_object(get_object_ref(key));
}

auto DataObjectOperator::GetSecDataObjectByRef(const QString& ref)
    -> GFBufferOrNone {
  if (key_.Empty() || !IsValidRefHex(ref)) return {};
  return read_decr_object(GFBuffer(QByteArray::fromHex(ref.toLatin1())));
}

auto DataObjectOperator::RemoveDataObj(const QString& key) -> bool {
  if (key_.Empty() || key.isEmpty()) return false;

  const auto ref_hex = get_object_ref(key).ConvertToQByteArray().toHex();
  const auto ref_path = gss_.GetDataObjectsDir() + "/" + ref_hex;

  if (!QFileInfo::exists(ref_path)) return true;

  if (!QFile::remove(ref_path)) {
    LOG_E() << "failed to remove data object from disk: " << ref_hex;
    return false;
  }

  return true;
}

auto DataObjectOperator::get_object_ref(const QString& obj_name) -> GFBuffer {
  if (obj_name.isEmpty()) {
    auto random =
        PassphraseGenerator::GetInstance().Generate(32).value_or(GFBuffer(
            QString::number(QRandomGenerator64::securelySeeded().generate())));

    return GFBufferFactory::ToHMACSha256(l_key_, random).value_or(GFBuffer{});
  }

  return GFBufferFactory::ToHMACSha256(l_key_, GFBuffer(obj_name))
      .value_or(GFBuffer{});
}

auto DataObjectOperator::read_decr_object(const GFBuffer& ref)
    -> GFBufferOrNone {
  const auto ref_hex = ref.ConvertToQByteArray().toHex();
  const auto ref_path = gss_.GetDataObjectsDir() + "/" + ref_hex;
  if (!QFileInfo(ref_path).exists()) {
    LOG_W() << "data object not found from disk, ref: " << ref_hex;
    return {};
  }

  auto [succ, data] = ReadFileGFBuffer(ref_path);
  if (!succ) {
    LOG_W() << "failed to read data object from disk, ref: " << ref_hex;
    return {};
  }

  auto key_id = data.Left(32);
  auto key = key_mgr_.GetKey(key_id);

  if (key.Empty()) {
    LOG_W() << "fail to find key of data object, key"
            << key_id.ConvertToQByteArray().toHex() << " ref: " << ref_hex;
    return {};
  }

  auto encrypted = data.Right(static_cast<int>(data.Size() - key_id.Size()));
  if (encrypted.Empty()) {
    LOG_W() << "data object from disk is empty, ref: " << ref_hex;
    return {};
  }

  auto drv_key = DeriveObjectKey(key, ref);
  if (!drv_key) {
    LOG_W() << "failed to derive key from ref: " << ref_hex;
    return {};
  }

  auto plaintext = GFBufferFactory::DecryptLite(*drv_key, encrypted);
  if (!plaintext) {
    LOG_W() << "failed to decrypt data object ref: " << ref_hex;
    return {};
  }

  return plaintext;
}

auto DataObjectOperator::read_decr_json_object(const GFBuffer& ref)
    -> std::optional<QJsonDocument> {
  auto plaintext = read_decr_object(ref);
  if (!plaintext) return {};

  try {
    return QJsonDocument::fromJson(plaintext->ConvertToQByteArray());
  } catch (...) {
    const auto ref_hex = ref.ConvertToQByteArray().toHex();
    LOG_W() << "failed to get data object:" << ref_hex << " caught exception.";
    return {};
  }
}

auto DataObjectOperator::write_encr_object(const GFBuffer& ref,
                                           const GFBuffer& value) -> QString {
  const auto ref_hex = ref.ConvertToQByteArray().toHex();
  const auto ref_path = gss_.GetDataObjectsDir() + "/" + ref_hex;

  auto drv_key = DeriveObjectKey(key_, ref);
  if (!drv_key) {
    LOG_W() << "failed to derive key from ref: " << ref_hex;
    return {};
  }

  auto encrypted = GFBufferFactory::EncryptLite(*drv_key, value);
  if (!encrypted) {
    LOG_E() << "failed to encrypt data object: " << ref_hex;
    return {};
  }

  GFBuffer data;
  data.Combine({key_id_, *encrypted});

  if (!WriteFileGFBuffer(ref_path, data)) {
    LOG_E() << "failed to write data object to disk: " << ref_hex;
  }

  return ref_hex;
}
}  // namespace GpgFrontend