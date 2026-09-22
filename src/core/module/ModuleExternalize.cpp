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

#include "core/module/ModuleExternalize.h"

#include <sodium.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <array>
#include <optional>

#include "core/module/ModuleDescriptor.h"
#include "core/module/ModuleDescriptorBuilder.h"
#include "core/module/ModuleEntryBinding.h"
#include "core/module/ModuleNamespace.h"
#include "core/module/ModulePublisherKey.h"
#include "core/module/ModuleSetVerification.h"
#include "core/module/ModuleTrustRoot.h"
#include "core/utils/CommonUtils.h"

// Deliberately absent, and held there by a test: settings, profiles, the
// trust store, the module manager and the task system. This file transforms
// artifacts; it never consults or changes runtime state.

namespace GpgFrontend::Module {

namespace {

using D = ModuleExternalizeDisposition;

struct ContractRow {
  const char* path;
  D disposition;
};

/// The preservation contract. Every manifest field the builder can write has
/// exactly one row; see ModuleExternalizeContract().
///
/// build.id is PRESERVED as provenance only. It names the build tree that
/// produced the binaries; the external verifier never compares it, and no
/// external trust or compatibility decision reads it.
constexpr std::array<ContractRow, 30> kContract{{
    {"schema_version", D::kPRESERVE},
    {"id", D::kPRESERVE},
    {"version", D::kPRESERVE},
    {"sdk_abi", D::kPRESERVE},
    {"min_host_version", D::kPRESERVE},
    {"security_epoch", D::kPRESERVE},
    {"capabilities", D::kPRESERVE},
    {"events", D::kPRESERVE},
    {"translation_context", D::kPRESERVE},

    {"build", D::kSTRUCTURE},
    {"build.id", D::kPRESERVE},
    {"build.timestamp", D::kPRESERVE},
    {"build.source_commit", D::kPRESERVE},

    {"platform", D::kSTRUCTURE},
    {"platform.os", D::kPRESERVE},
    {"platform.arch", D::kPRESERVE},
    {"platform.qt", D::kPRESERVE},

    {"resources", D::kSTRUCTURE},
    {"resources[].path", D::kPRESERVE},
    {"resources[].sha256", D::kPRESERVE},

    {"entry_native", D::kSTRUCTURE},
    {"entry_native.name", D::kPRESERVE},
    {"entry_native.verification", D::kSTRUCTURE},
    {"entry_native.verification.mode", D::kPRESERVE},
    {"entry_native.verification.value", D::kPRESERVE},
    {"entry_native.size", D::kPRESERVE},

    {"metadata", D::kSTRUCTURE},
    {"metadata.Publisher", D::kCHANGE},
    {"metadata.PublisherUrl", D::kCHANGE},
    {"metadata.*", D::kPRESERVE},
}};

auto Fail(const QString& reason) -> ModuleExternalizeResult {
  ModuleExternalizeResult r;
  r.reason = reason;
  return r;
}

/// Exact rows first, so a named CHANGE row wins over the map's wildcard.
auto RuleFor(const QString& path) -> std::optional<D> {
  for (const auto& row : kContract) {
    if (path == QLatin1String(row.path)) return row.disposition;
  }
  for (const auto& row : kContract) {
    const QString pattern = QLatin1String(row.path);
    if (!pattern.endsWith(".*")) continue;
    const auto prefix = pattern.chopped(1);
    if (path.size() > prefix.size() && path.startsWith(prefix)) {
      return row.disposition;
    }
  }
  return std::nullopt;
}

void Walk(const QJsonObject& object, const QString& prefix, QStringList& out) {
  for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
    const auto path = prefix.isEmpty() ? it.key() : prefix + "." + it.key();
    out.append(path);
    const auto value = it.value();
    if (value.isObject()) {
      Walk(value.toObject(), path, out);
    } else if (value.isArray()) {
      for (const auto& element : value.toArray()) {
        if (element.isObject()) Walk(element.toObject(), path + "[]", out);
      }
    }
  }
}

auto KeyPaths(const QJsonObject& manifest) -> QStringList {
  QStringList paths;
  Walk(manifest, {}, paths);
  paths.removeDuplicates();
  paths.sort();
  return paths;
}

/// Every value at @p path, in document order. `a[]` expands an array.
auto ValuesAt(const QJsonObject& root, const QString& path) -> QJsonArray {
  QJsonArray current{root};
  for (const auto& segment : path.split(u'.')) {
    const auto expand = segment.endsWith("[]");
    const auto key = expand ? segment.chopped(2) : segment;
    QJsonArray next;
    for (const auto& value : current) {
      const auto object = value.toObject();
      if (!object.contains(key)) continue;
      const auto found = object.value(key);
      if (!expand) {
        next.append(found);
        continue;
      }
      for (const auto& element : found.toArray()) next.append(element);
    }
    current = next;
  }
  return current;
}

/// Fail closed: every key path must have a row, or nothing crosses.
auto CheckContractCoverage(const QJsonObject& manifest, const char* side,
                           QString& reason) -> bool {
  for (const auto& path : KeyPaths(manifest)) {
    if (!RuleFor(path).has_value()) {
      reason = QString(
                   "manifest field \"%1\" in the %2 has no "
                   "externalization decision")
                   .arg(path, QString::fromLatin1(side));
      return false;
    }
  }
  // A dotted map key would be addressed as a nested field and compare as
  // absent on both sides -- equal, whatever its value. Refused, so the
  // contract never judges a field it cannot see.
  for (const auto& key : manifest.value("metadata").toObject().keys()) {
    if (key.contains(u'.') || key.contains(u'[')) {
      reason = QString(
                   "metadata key \"%1\" cannot be tracked by the "
                   "externalization contract")
                   .arg(key);
      return false;
    }
  }
  return true;
}

/// The whitelist, applied: PRESERVE rows equal, CHANGE rows exactly what was
/// asked for, STRUCTURE rows present on both sides or neither. Judged row by
/// row over the union of both manifests' fields -- never by diffing the
/// whole documents and subtracting what was expected to differ.
auto CheckPreserved(const QJsonObject& input, const QJsonObject& output,
                    const ModuleExternalizeSpec& spec, QString& reason)
    -> bool {
  const QMap<QString, QString> changes{
      {QString("metadata.") + kModuleMetadataPublisher, spec.publisher_name},
      {QString("metadata.") + kModuleMetadataPublisherUrl, spec.publisher_url},
  };

  // The CHANGE rows are judged whether or not either side has them, so one
  // that was asked for and silently not written is caught too.
  auto paths = KeyPaths(input) + KeyPaths(output) + changes.keys();
  paths.removeDuplicates();
  paths.sort();

  for (const auto& path : paths) {
    const auto rule = RuleFor(path);
    if (!rule.has_value()) {
      reason = QString("manifest field \"%1\" has no externalization decision")
                   .arg(path);
      return false;
    }

    const auto before = ValuesAt(input, path);
    const auto after = ValuesAt(output, path);

    switch (*rule) {
      case D::kPRESERVE:
        if (before != after) {
          reason = QString("\"%1\" changed during externalization").arg(path);
          return false;
        }
        break;
      case D::kSTRUCTURE:
        if (before.isEmpty() != after.isEmpty()) {
          reason = QString("\"%1\" was %2 during externalization")
                       .arg(path, before.isEmpty() ? "added" : "removed");
          return false;
        }
        break;
      case D::kCHANGE: {
        const auto expected = changes.value(path);
        const auto ok =
            before.isEmpty() &&
            (expected.isEmpty() ? after.isEmpty()
                                : after == QJsonArray{QJsonValue(expected)});
        if (!ok) {
          reason =
              QString("\"%1\" is not what externalization was asked to set")
                  .arg(path);
          return false;
        }
        break;
      }
    }
  }
  return true;
}

/// Removes the staging directory unless released. Nothing partial survives a
/// failure, and nothing this call did not create is ever removed.
class StagingGuard {
 public:
  explicit StagingGuard(QString path) : path_(std::move(path)) {}
  StagingGuard(const StagingGuard&) = delete;
  auto operator=(const StagingGuard&) -> StagingGuard& = delete;
  ~StagingGuard() {
    if (!released_) QDir(path_).removeRecursively();
  }
  void Release() { released_ = true; }

 private:
  QString path_;
  bool released_ = false;
};

}  // namespace

auto ModuleExternalizeContract() -> QVector<ModuleExternalizeField> {
  QVector<ModuleExternalizeField> rows;
  rows.reserve(static_cast<qsizetype>(kContract.size()));
  for (const auto& row : kContract) {
    rows.append({QString::fromLatin1(row.path), row.disposition});
  }
  return rows;
}

auto ModuleManifestKeyPaths(const QByteArray& manifest_bytes) -> QStringList {
  return KeyPaths(QJsonDocument::fromJson(manifest_bytes).object());
}

auto ExternalizeModule(const ModuleExternalizeSpec& spec)
    -> ModuleExternalizeResult {
  if (!EnsureSodiumInit()) {
    return Fail("the cryptography library could not be started");
  }

  // The key first: it is the cheapest thing to be wrong, and the one whose
  // being wrong matters most.
  const auto publisher_key =
      ModulePublisherPublicKeyFromSeed(spec.publisher_seed);
  if (publisher_key.isEmpty()) return Fail("no usable publisher key was given");
  if (publisher_key == ModuleBuildPublicKey()) {
    return Fail(
        "the publisher key is this build's own module-build key, which is "
        "never a publisher identity");
  }
  if (spec.output_root.isEmpty()) return Fail("no output directory was given");

  // --- 1. The input, verified as integrated ------------------------------

  const QFileInfo input_info(spec.input);
  if (!input_info.exists()) {
    return Fail(QString("\"%1\" does not exist").arg(spec.input));
  }
  if (input_info.isFile() &&
      input_info.fileName() != QLatin1String(kModuleDescriptorFileName)) {
    return Fail(QString("\"%1\" is not a module namespace or its %2")
                    .arg(spec.input, kModuleDescriptorFileName));
  }
  const auto namespace_dir =
      QDir::cleanPath(input_info.isFile() ? input_info.absolutePath()
                                          : input_info.absoluteFilePath());

  // Binding REQUIRED whatever this build's own policy is: an input that makes
  // no claim about its native carries nothing verified to hand forward.
  const auto checked = VerifyModuleNamespace(
      {ModuleOrigin::kINTEGRATED, ModuleBindingRequirement::kREQUIRED},
      namespace_dir);
  if (!checked.ok) {
    // The one refusal with a build-side remedy, so it says what it is.
    const auto hint =
        checked.entry.status == ModuleEntryStatus::kENTRY_BINDING_ABSENT
            ? QString(
                  "; build with "
                  "GPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING=REQUIRED "
                  "so the integrated descriptor binds its native")
            : QString();
    return Fail(QString("the input is not a verified integrated module: %1%2")
                    .arg(checked.reason, hint));
  }
  const auto& input = checked.descriptor.manifest;

  const auto rule = ModuleEntryVerificationModeFor(input.platform_os);
  if (!rule.mode.has_value()) {
    return Fail(QString("\"%1\" modules cannot be externalized: the platform "
                        "has no entry binding mode, and the Host refuses "
                        "external modules there")
                    .arg(input.platform_os));
  }

  // An external module is exactly its descriptor and its entry. A helper
  // beside it would ship inside a publisher-signed artifact with nothing
  // signed about it.
  if (!checked.helpers.isEmpty()) {
    QStringList names;
    for (const auto& helper : checked.helpers) {
      names.append(QFileInfo(helper).fileName());
    }
    return Fail(QString("its native directory holds files the descriptor does "
                        "not bind (%1); an external module ships only its "
                        "entry native")
                    .arg(names.join(", ")));
  }

  const auto input_json =
      QJsonDocument::fromJson(checked.descriptor.manifest_bytes).object();
  QString why;
  if (!CheckContractCoverage(input_json, "input", why)) return Fail(why);

  for (const auto* key :
       {kModuleMetadataPublisher, kModuleMetadataPublisherUrl}) {
    if (input.metadata.contains(QString::fromLatin1(key))) {
      return Fail(QString("the input already declares \"%1\"; only "
                          "externalization sets it")
                      .arg(key));
    }
  }

  // --- Where the output goes ---------------------------------------------

  // Checked before anything is created, so a refusal leaves nothing inside
  // the input -- and again once the directory exists, when symbolic links in
  // the path have something to resolve to.
  const auto input_root = QFileInfo(namespace_dir).canonicalFilePath();
  const auto inside_input = [&input_root](const QString& path) {
    return path == input_root || path.startsWith(input_root + "/");
  };
  if (inside_input(
          QDir::cleanPath(QFileInfo(spec.output_root).absoluteFilePath()))) {
    return Fail("the output directory is inside the input namespace");
  }
  if (!QDir().mkpath(spec.output_root)) {
    return Fail(QString("\"%1\" could not be created").arg(spec.output_root));
  }
  const auto output_root = QFileInfo(spec.output_root).canonicalFilePath();
  if (inside_input(output_root)) {
    return Fail("the output directory is inside the input namespace");
  }

  const auto key = ModuleDirectoryKey(input.id);
  const auto final_dir = output_root + "/" + key;
  const auto staging_dir = output_root + "/." + key + ".staging";
  if (QFileInfo::exists(final_dir)) {
    return Fail(QString("\"%1\" already exists; an external module is never "
                        "overwritten")
                    .arg(final_dir));
  }
  if (QFileInfo::exists(staging_dir)) {
    return Fail(QString("\"%1\" is left over from an earlier run; remove it "
                        "first")
                    .arg(staging_dir));
  }
  if (!QDir().mkpath(staging_dir)) {
    return Fail(QString("\"%1\" could not be created").arg(staging_dir));
  }
  StagingGuard guard(staging_dir);

  const auto staged_descriptor =
      staging_dir + "/" + QString::fromLatin1(kModuleDescriptorFileName);
  const auto staged_native_root = ModuleNativeRootFor(staged_descriptor);
  const auto staged_native =
      staged_native_root + "/" + QFileInfo(checked.entry.path).fileName();

  // --- 2. Resources, from the bytes the integrated signature covers ------

  QMap<QString, QByteArray> resources;
  if (!ReadModuleDescriptorResources(
          namespace_dir + "/" + QString::fromLatin1(kModuleDescriptorFileName),
          resources, why)) {
    return Fail(QString("its resources could not be read: %1").arg(why));
  }

  // --- 3. The native, copied, and the COPY verified ----------------------

  if (!QDir().mkpath(staged_native_root) ||
      !QFile::copy(checked.entry.path, staged_native)) {
    return Fail("the entry native could not be copied");
  }
  // The bytes that ship are the bytes checked, with nothing in between: a
  // native swapped after step 1 fails here rather than being published.
  const auto staged_entry = ResolveAndVerifyNativeEntry(
      input, ModuleNativeRoot{staged_native_root},
      {ModuleOrigin::kINTEGRATED, ModuleBindingRequirement::kREQUIRED});
  if (!staged_entry.ok) {
    return Fail(QString("the copied entry native is not the one the "
                        "integrated descriptor binds: %1")
                    .arg(staged_entry.reason));
  }

  // --- 4. Build, field by field from the PRESERVE rows -------------------

  ModuleDescriptorBuildSpec build;
  build.origin = ModuleOrigin::kEXTERNAL;
  build.module_id = input.id;
  build.version = input.version;
  build.sdk_abi = input.sdk_abi;
  build.min_host_version = input.min_host_version;
  build.security_epoch = input.security_epoch;
  build.capabilities = input.capabilities;
  build.events = input.events;
  build.commands = input.commands;
  build.translation_context = input.translation_context;
  build.metadata = input.metadata;
  build.build_id = input.build_id;  // provenance only
  build.build_timestamp = input.build_timestamp;
  build.build_source_commit = input.build_source_commit;
  build.platform_os = input.platform_os;
  build.platform_arch = input.platform_arch;
  build.platform_qt = input.platform_qt;
  build.entry_native_name = input.entry_native.name;
  build.entry_native_file = staged_native;
  build.entry_binding = ModuleBindingRequirement::kREQUIRED;
  build.signing_seed = spec.publisher_seed;
  build.output_path = staged_descriptor;

  // The CHANGE rows, and nothing else.
  if (!spec.publisher_name.isEmpty()) {
    build.metadata.insert(kModuleMetadataPublisher, spec.publisher_name);
  }
  if (!spec.publisher_url.isEmpty()) {
    build.metadata.insert(kModuleMetadataPublisherUrl, spec.publisher_url);
  }

  // In declared order, so resources[] compares equal element by element.
  for (const auto& declared : input.resources) {
    const auto it = resources.constFind(declared.path);
    if (it == resources.constEnd()) {
      return Fail(QString("its resource \"%1\" could not be read back")
                      .arg(declared.path));
    }
    build.resources.append({declared.path, {}, *it});
  }

  const auto built = BuildModuleDescriptor(build);
  if (!built.ok) return Fail(built.reason);

  // --- 5. Self-check, through the verifiers the Host uses ---------------

  const auto external = VerifyExternalModuleDescriptor(staged_descriptor);
  if (!external.ok) {
    return Fail(QString("the external descriptor this produced does not "
                        "verify: %1")
                    .arg(external.reason));
  }
  if (external.signer_public_key != publisher_key) {
    return Fail(
        "the external descriptor names a key other than the publisher's");
  }
  if (VerifyModuleDescriptor(staged_descriptor).ok) {
    return Fail("the external descriptor this produced verifies as integrated");
  }

  const auto external_entry = ResolveAndVerifyNativeEntry(
      external.manifest, ModuleNativeRoot{staged_native_root},
      {ModuleOrigin::kEXTERNAL, ModuleBindingRequirement::kREQUIRED});
  if (!external_entry.ok) {
    return Fail(QString("the external descriptor does not bind the shipped "
                        "entry native: %1")
                    .arg(external_entry.reason));
  }

  const auto output_json =
      QJsonDocument::fromJson(external.manifest_bytes).object();
  if (!CheckContractCoverage(output_json, "output", why) ||
      !CheckPreserved(input_json, output_json, spec, why)) {
    return Fail(why);
  }

  QMap<QString, QByteArray> output_resources;
  if (!ReadModuleDescriptorResources(staged_descriptor, output_resources,
                                     why) ||
      output_resources != resources) {
    return Fail("the external descriptor does not carry the input's resources");
  }

  // --- 6. Publish, all at once and never over anything -------------------

  if (!QDir().rename(staging_dir, final_dir)) {
    return Fail(QString("\"%1\" could not be published").arg(final_dir));
  }
  guard.Release();

  ModuleExternalizeResult result;
  result.ok = true;
  result.output_namespace = final_dir;
  result.publisher_key = publisher_key;
  result.manifest = external.manifest;
  return result;
}

}  // namespace GpgFrontend::Module
