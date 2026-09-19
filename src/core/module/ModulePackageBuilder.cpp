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

#include "ModulePackageBuilder.h"

#include "core/module/ModuleManifest.h"

#include <sodium.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QSaveFile>
#include <thread>

#include "core/function/ArchiveFileOperator.h"
#include "core/function/GFBufferFactory.h"
#include "core/module/ModulePackageVerifier.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend::Module {

namespace {

auto Fail(const QString& reason) -> ModulePackageBuildResult {
  ModulePackageBuildResult r;
  r.ok = false;
  r.reason = reason;
  return r;
}

/// JCS string escaping: the two-character forms where JSON defines them, \u
/// for the remaining control characters, and the raw code unit otherwise. In
/// particular non-ASCII is NOT escaped -- JCS serialises UTF-8.
void AppendCanonicalString(const QString& s, QByteArray& out) {
  out.append('"');
  for (const auto ch : s) {
    const auto u = ch.unicode();
    switch (u) {
      case u'"':
        out.append("\\\"");
        continue;
      case u'\\':
        out.append("\\\\");
        continue;
      case u'\b':
        out.append("\\b");
        continue;
      case u'\f':
        out.append("\\f");
        continue;
      case u'\n':
        out.append("\\n");
        continue;
      case u'\r':
        out.append("\\r");
        continue;
      case u'\t':
        out.append("\\t");
        continue;
      default:
        break;
    }
    if (u < 0x20) {
      constexpr auto* kHex = "0123456789abcdef";
      out.append("\\u00");
      out.append(kHex[(u >> 4U) & 0xFU]);
      out.append(kHex[u & 0xFU]);
      continue;
    }
    out.append(QString(ch).toUtf8());
  }
  out.append('"');
}

auto AppendCanonicalValue(const QJsonValue& value, QByteArray& out) -> bool {
  switch (value.type()) {
    case QJsonValue::Null:
      out.append("null");
      return true;
    case QJsonValue::Bool:
      out.append(value.toBool() ? "true" : "false");
      return true;
    case QJsonValue::Double: {
      const auto d = value.toDouble();
      if (d != static_cast<double>(static_cast<qint64>(d))) return false;
      out.append(QByteArray::number(static_cast<qint64>(d)));
      return true;
    }
    case QJsonValue::String:
      AppendCanonicalString(value.toString(), out);
      return true;
    case QJsonValue::Array: {
      out.append('[');
      const auto array = value.toArray();
      for (auto i = 0; i < array.size(); ++i) {
        if (i != 0) out.append(',');
        if (!AppendCanonicalValue(array.at(i), out)) return false;
      }
      out.append(']');
      return true;
    }
    case QJsonValue::Object: {
      out.append('{');
      const auto object = value.toObject();
      // JCS orders members by their UTF-16 code units, which is exactly the
      // order QJsonObject already keeps its keys in. Relying on that rather
      // than re-sorting keeps the two from drifting apart.
      auto first = true;
      for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (!first) out.append(',');
        first = false;
        AppendCanonicalString(it.key(), out);
        out.append(':');
        if (!AppendCanonicalValue(it.value(), out)) return false;
      }
      out.append('}');
      return true;
    }
    default:
      return false;
  }
}

/// SHA-256 of a file, as lower-case hex. The digest the signature will cover,
/// so it comes from the same place the verifier's does.
auto HashFile(const QString& path, QString& out) -> bool {
  out = GFBufferFactory::Sha256HexOfFile(path);
  return !out.isEmpty();
}

auto HashBytes(const QByteArray& bytes, QString& out) -> bool {
  out = GFBufferFactory::Sha256Hex(bytes);
  return !out.isEmpty();
}

}  // namespace

auto CanonicalJson(const QJsonValue& value, QByteArray& out) -> bool {
  out.clear();
  return AppendCanonicalValue(value, out);
}

auto BuildModulePackage(const ModulePackageBuildSpec& spec)
    -> ModulePackageBuildResult {
  // The os string the verifier will compare against, unless the caller is
  // deliberately cross-packaging. Taking it from the command line by default
  // meant a packaging script could stamp a spelling nothing accepts.
  const auto platform_os =
      spec.platform_os.isEmpty() ? ManifestHostOsName() : spec.platform_os;

  if (!EnsureSodiumInit()) {
    return Fail("the cryptography library could not be started");
  }
  if (spec.output_path.isEmpty()) return Fail("no output path was given");
  if (spec.files.isEmpty()) return Fail("a package with no files is not one");

  // The archive writer trusts whatever paths it is handed -- it is the
  // extractor that validates, and it is not running yet. Validating here is
  // what stops the producer minting a package its own verifier would refuse.
  ArchiveExtractPolicy naming;
  naming.reject_duplicate_paths = true;

  QJsonArray files_json;
  QVector<ModulePackageSource> staged;
  QSet<QString> seen;

  for (const auto& source : spec.files) {
    QString normalised;
    const auto verdict =
        ValidateArchiveEntryPath(source.archive_path, naming, normalised);
    if (verdict != ArchiveEntryVerdict::kACCEPT) {
      return Fail(
          QString("\"%1\" is not a usable name inside a package: %2")
              .arg(source.archive_path,
                   QString::fromUtf8(ArchiveEntryVerdictToString(verdict))));
    }
    if (normalised.startsWith("META-INF/")) {
      return Fail(QString("\"%1\" is reserved").arg(normalised));
    }
    if (seen.contains(normalised.toCaseFolded())) {
      return Fail(QString("\"%1\" is packaged more than once, or differs from "
                          "another entry only by case")
                      .arg(normalised));
    }
    seen.insert(normalised.toCaseFolded());

    QString digest;
    const auto hashed = source.source_file.isEmpty()
                            ? HashBytes(source.bytes, digest)
                            : HashFile(source.source_file, digest);
    if (!hashed) {
      return Fail(QString("\"%1\" could not be read")
                      .arg(source.source_file.isEmpty() ? normalised
                                                        : source.source_file));
    }

    files_json.append(QJsonObject{{"path", normalised}, {"sha256", digest}});

    auto entry = source;
    entry.archive_path = normalised;
    staged.append(entry);
  }

  QJsonObject metadata;
  for (auto it = spec.metadata.constBegin(); it != spec.metadata.constEnd();
       ++it) {
    metadata.insert(it.key(), it.value());
  }

  QJsonObject manifest{
      {"schema_version", kModuleManifestSchemaVersion},
      {"id", spec.module_id},
      {"version", spec.version},
      {"sdk_abi", spec.sdk_abi},
      {"min_host_version", spec.min_host_version},
      {"security_epoch", spec.security_epoch},
      {"capabilities", QJsonArray::fromStringList(spec.capabilities)},
      {"metadata", metadata},
      {"build", QJsonObject{{"id", spec.build_id},
                            {"timestamp", spec.build_timestamp},
                            {"source_commit", spec.build_source_commit}}},
      {"platform", QJsonObject{{"os", platform_os},
                               {"arch", spec.platform_arch},
                               {"qt", spec.platform_qt}}},
      {"files", files_json},
  };

  // Both are required at schema 2. An empty events array is still a legitimate
  // statement -- a module that subscribes to nothing -- and is written as such;
  // what is refused below is a manifest that says nothing at all.
  manifest.insert("events", QJsonArray::fromStringList(spec.events));
  manifest.insert("translation_context", spec.translation_context);

  QByteArray manifest_bytes;
  if (!CanonicalJson(manifest, manifest_bytes)) {
    return Fail("this manifest cannot be written in canonical form");
  }

  // Refuse to ship a manifest this build's own verifier would not accept.
  // Producer and verifier drifting apart is the failure mode every unit test
  // on either side individually survives.
  const auto self_check = ParseModuleManifest(manifest_bytes);
  if (!self_check.ok) {
    return Fail(QString("the manifest this produced is not valid: %1")
                    .arg(self_check.reason));
  }

  // Ephemeral, and destroyed before this function returns by whichever path.
  std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> public_key{};
  std::array<unsigned char, crypto_sign_SECRETKEYBYTES> secret_key{};
  const auto forget_secret = qScopeGuard([&secret_key]() {
    sodium_memzero(secret_key.data(), secret_key.size());
  });

  if (crypto_sign_keypair(public_key.data(), secret_key.data()) != 0) {
    return Fail("a signing key could not be generated");
  }

  std::array<unsigned char, crypto_sign_BYTES> signature{};
  unsigned long long signature_length = 0;
  if (crypto_sign_detached(
          signature.data(), &signature_length,
          reinterpret_cast<const unsigned char*>(manifest_bytes.constData()),
          static_cast<unsigned long long>(manifest_bytes.size()),
          secret_key.data()) != 0) {
    return Fail("the manifest could not be signed");
  }

  QVector<ModulePackageSource> members;
  members.append({kModulePackageManifestPath, {}, manifest_bytes});
  members.append({kModulePackageSignaturePath,
                  {},
                  QByteArray(reinterpret_cast<const char*>(signature.data()),
                             static_cast<qsizetype>(signature_length))});
  members.append({kModulePackageBuildKeyPath,
                  {},
                  QByteArray(reinterpret_cast<const char*>(public_key.data()),
                             static_cast<qsizetype>(public_key.size()))});
  members.append(staged);

  QSaveFile out(spec.output_path);
  if (!out.open(QIODevice::WriteOnly)) {
    return Fail(QString("\"%1\" could not be written").arg(spec.output_path));
  }

  auto exchanger = CreateStandardGFDataExchanger();

  // Set by the producer, read after it is joined.
  GFError archive_error = 0;

  std::thread producer([&]() {
    qsizetype index = 0;
    archive_error = ArchiveFileOperator::NewArchiveFromMembersSync(
        [&](ArchiveMemberEntry& entry) {
          if (index >= members.size()) return false;
          const auto& source = members.at(index++);
          entry.relative_path = source.archive_path;
          if (source.source_file.isEmpty()) {
            entry.bytes = GFBuffer(source.bytes);
          } else {
            entry.source_file = source.source_file;
          }
          return true;
        },
        exchanger, ArchiveCompression::kNONE, ArchiveFormat::kZIP);
  });

  auto write_ok = true;
  {
    std::array<std::byte, 64 * 1024> chunk{};
    while (true) {
      const auto n = exchanger->Read(chunk.data(), chunk.size());
      if (n <= 0) break;
      // A failed write does NOT stop the loop. There is no way to close the
      // read side of this pipe, so a producer that is still pushing into a
      // full one blocks forever -- and the join below would then never
      // return. Draining to the end costs a rebuild of an archive that is
      // about to be discarded, which is cheaper than a hung build.
      if (!write_ok) continue;
      if (out.write(reinterpret_cast<const char*>(chunk.data()), n) != n) {
        write_ok = false;
      }
    }
  }

  producer.join();

  if (!write_ok || archive_error != 0) {
    out.cancelWriting();
    return Fail(QString("\"%1\" could not be written").arg(spec.output_path));
  }
  if (!out.commit()) {
    return Fail(QString("\"%1\" could not be written").arg(spec.output_path));
  }

  ModulePackageBuildResult result;
  result.ok = true;
  result.build_public_key =
      QByteArray(reinterpret_cast<const char*>(public_key.data()),
                 static_cast<qsizetype>(public_key.size()));
  result.manifest_bytes = manifest_bytes;
  return result;
}

}  // namespace GpgFrontend::Module
