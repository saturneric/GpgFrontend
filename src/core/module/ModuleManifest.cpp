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

#include "ModuleManifest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <cmath>

#include "core/module/ModuleCapability.h"
#include "sdk/GFSDKBuildInfo.h"

namespace GpgFrontend::Module {

namespace {

/// Every refusal below returns through here, so none of them can forget to
/// clear `ok` or to say why.
auto Refuse(ModuleManifestStatus status, const QString& reason)
    -> ModuleManifestParseResult {
  ModuleManifestParseResult r;
  r.ok = false;
  r.status = status;
  r.reason = reason;
  return r;
}

auto Malformed(const QString& reason) -> ModuleManifestParseResult {
  return Refuse(ModuleManifestStatus::kMALFORMED, reason);
}

/// A required string, present and actually a string. An absent field and a
/// field holding the wrong type are reported apart, because they are different
/// mistakes for whoever has to fix the manifest.
auto TakeString(const QJsonObject& o, const QString& key, QString& out,
                QString& error) -> bool {
  const auto v = o.value(key);
  if (v.isUndefined()) {
    error = QString("\"%1\" is missing").arg(key);
    return false;
  }
  if (!v.isString()) {
    error = QString("\"%1\" is not a string").arg(key);
    return false;
  }
  out = v.toString();
  if (out.isEmpty()) {
    error = QString("\"%1\" is empty").arg(key);
    return false;
  }
  return true;
}

/// A required integer. QJsonValue::isDouble() is true for every number, so a
/// non-integral one is caught separately: a fractional `sdk_abi` is not a
/// version this build should quietly truncate into one.
auto TakeInt(const QJsonObject& o, const QString& key, int& out, QString& error)
    -> bool {
  const auto v = o.value(key);
  if (v.isUndefined()) {
    error = QString("\"%1\" is missing").arg(key);
    return false;
  }
  if (!v.isDouble()) {
    error = QString("\"%1\" is not a number").arg(key);
    return false;
  }
  const auto d = v.toDouble();
  if (d != static_cast<double>(static_cast<int>(d))) {
    error = QString("\"%1\" is not a whole number").arg(key);
    return false;
  }
  out = static_cast<int>(d);
  return true;
}

/// A required object.
auto TakeObject(const QJsonObject& o, const QString& key, QJsonObject& out,
                QString& error) -> bool {
  const auto v = o.value(key);
  if (v.isUndefined()) {
    error = QString("\"%1\" is missing").arg(key);
    return false;
  }
  if (!v.isObject()) {
    error = QString("\"%1\" is not an object").arg(key);
    return false;
  }
  out = v.toObject();
  return true;
}

/// A logical native name: no separator, no dot, no drive letter, no scheme.
///
/// Validated by construction rather than by blacklist. A name that matches
/// this cannot express a path at all, which is why the descriptor never needs
/// a rule about traversal.
auto IsLogicalNativeName(const QString& s) -> bool {
  if (s.isEmpty() || s.size() > 64) return false;
  if (s.front() < u'a' || s.front() > u'z') return false;
  for (const auto c : s) {
    const auto ch = c.unicode();
    const auto ok =
        (ch >= u'a' && ch <= u'z') || (ch >= u'0' && ch <= u'9') || ch == u'_';
    if (!ok) return false;
  }
  // The platform prefix is the Host's to add. A name carrying one is
  // ambiguous about whether it has already been mapped.
  return !s.startsWith("lib");
}

}  // namespace

auto IsModuleHexDigest(const QString& s) -> bool {
  if (s.size() != 64) return false;
  for (const auto c : s) {
    const auto ch = c.unicode();
    const auto is_digit = ch >= u'0' && ch <= u'9';
    const auto is_lower_hex = ch >= u'a' && ch <= u'f';
    if (!is_digit && !is_lower_hex) return false;
  }
  return true;
}

auto ModuleEntryVerificationModeKey(ModuleEntryVerificationMode mode)
    -> QString {
  switch (mode) {
    case ModuleEntryVerificationMode::kFILE_SHA256:
      return "file-sha256";
    case ModuleEntryVerificationMode::kPE_AUTHENTICODE_SHA256:
      return "pe-authenticode-sha256";
  }
  return {};
}

auto ModuleEntryVerificationModeFor(const QString& platform_os)
    -> ModuleEntryVerificationModeRule {
  if (platform_os == "linux") {
    return {true, ModuleEntryVerificationMode::kFILE_SHA256};
  }
  if (platform_os == "windows") {
    return {true, ModuleEntryVerificationMode::kPE_AUTHENTICODE_SHA256};
  }
  // Known, and deliberately modeless. Apple signs the module dylibs with the
  // application's own identity and dyld enforces that at map time, in the
  // kernel; a second GpgFrontend-side claim over the same bytes would be a
  // weaker restatement of it, checked later and by us.
  if (platform_os == "macos") return {true, std::nullopt};
  return {false, std::nullopt};
}

auto ParseModuleManifest(const QByteArray& bytes) -> ModuleManifestParseResult {
  QJsonParseError parse_error{};
  const auto doc = QJsonDocument::fromJson(bytes, &parse_error);
  if (parse_error.error != QJsonParseError::NoError) {
    return Malformed(
        QString("it is not valid JSON: %1").arg(parse_error.errorString()));
  }
  if (!doc.isObject()) return Malformed("it is not a JSON object");

  const auto root = doc.object();
  QString error;

  // Version first, and on its own: a manifest from the future must be reported
  // as such rather than as a pile of missing fields, because the two call for
  // opposite things from whoever reads the message.
  int schema_version = 0;
  if (!TakeInt(root, "schema_version", schema_version, error)) {
    return Malformed(error);
  }
  if (schema_version > kModuleManifestSchemaVersion) {
    return Refuse(ModuleManifestStatus::kTOO_NEW,
                  QString("it uses manifest schema %1, and this version of "
                          "GpgFrontend understands up to %2")
                      .arg(schema_version)
                      .arg(kModuleManifestSchemaVersion));
  }
  if (schema_version < kModuleManifestMinSupportedSchema) {
    // Named, not merely refused. A package built before schema 3 carries its
    // executable payload inside itself, which this build has no path for at
    // all, and "a field is missing" would be a worse sentence than saying so.
    return Malformed(
        QString("\"schema_version\" is %1; this build reads %2 and later, "
                "which is when the module binary moved out of the package")
            .arg(schema_version)
            .arg(kModuleManifestMinSupportedSchema));
  }

  ModuleManifest m;
  m.schema_version = schema_version;

  if (!TakeString(root, "id", m.id, error)) return Malformed(error);
  if (!TakeString(root, "version", m.version, error)) return Malformed(error);
  if (!TakeInt(root, "sdk_abi", m.sdk_abi, error)) return Malformed(error);
  if (!TakeString(root, "min_host_version", m.min_host_version, error)) {
    return Malformed(error);
  }
  if (!TakeInt(root, "security_epoch", m.security_epoch, error)) {
    return Malformed(error);
  }
  if (m.sdk_abi < 0 || m.security_epoch < 0) {
    return Malformed("a version number in it is negative");
  }

  // capabilities: required, an array, and every element a string from a
  // vocabulary this Host build knows. An empty array is legitimate -- a
  // module that asks for nothing.
  //
  // Refusing an unknown name matters more than it looks. The manifest is
  // signed, and the grant is computed from it: a name the Host cannot place
  // contributes nothing to the grant, so a typo would silently produce a
  // module with LESS access than its author declared and than the user
  // approved -- which then fails at run time, far from the cause. Refusing
  // the package says it once, at the point the name was written.
  {
    const auto v = root.value("capabilities");
    if (v.isUndefined()) return Malformed("\"capabilities\" is missing");
    if (!v.isArray()) return Malformed("\"capabilities\" is not an array");
    for (const auto& c : v.toArray()) {
      if (!c.isString()) {
        return Malformed("a value in \"capabilities\" is not a string");
      }
      const auto name = c.toString();
      if (ModuleCapabilityKindOf(name) == ModuleCapabilityKind::kUNKNOWN) {
        return Malformed(
            QString("\"capabilities\" names \"%1\", which this host does not "
                    "know; it grants %2 and records %3")
                .arg(name, EnforceableCapabilityNames().join(", "),
                     AdvisoryCapabilityNames().join(", ")));
      }
      m.capabilities.append(name);
    }
    // "ui.custom" extends "ui"; it is never implied. Refusing the lone form
    // keeps the signed statement explicit: a manifest that grants native
    // widgets in Host containers also says, in so many words, that the
    // module integrates with the Host UI at all.
    if (m.capabilities.contains("ui.custom") &&
        !m.capabilities.contains("ui")) {
      return Malformed("\"capabilities\" names \"ui.custom\" without \"ui\"");
    }
  }

  // events: required. Every element an upper-case identifier, because that is
  // the form the host dispatches on. This is the subscription allowlist, so a
  // package that omits it is a package whose signature covers no statement
  // about what the module may listen to -- which is what the field is for.
  {
    const auto v = root.value("events");
    if (v.isUndefined()) return Malformed("\"events\" is missing");
    {
      if (!v.isArray()) return Malformed("\"events\" is not an array");
      for (const auto& e : v.toArray()) {
        if (!e.isString()) {
          return Malformed("a value in \"events\" is not a string");
        }
        const auto id = e.toString();
        if (id.isEmpty()) return Malformed("an event id is empty");
        if (id != id.toUpper()) {
          return Malformed(
              QString("event id \"%1\" is not upper-case").arg(id));
        }
        if (m.events.contains(id)) {
          return Malformed(
              QString("event id \"%1\" is declared twice").arg(id));
        }
        m.events.append(id);
      }
    }
  }

  // commands: optional. The ids this module provides, each in the module's
  // own namespace: a manifest cannot claim a command id belonging to the Host
  // or to another module, whatever its signature says.
  {
    const auto v = root.value("commands");
    if (!v.isUndefined()) {
      if (!v.isArray()) return Malformed("\"commands\" is not an array");
      static const QRegularExpression kShape(
          QStringLiteral("^[a-z0-9_]+(\\.[a-z0-9_]+)*$"));
      for (const auto& c : v.toArray()) {
        if (!c.isString()) {
          return Malformed("a value in \"commands\" is not a string");
        }
        const auto id = c.toString();
        if (!kShape.match(id).hasMatch()) {
          return Malformed(
              QString("command id \"%1\" is not lower-case dotted").arg(id));
        }
        if (!m.id.isEmpty() && !id.startsWith(m.id + ".")) {
          return Malformed(
              QString("command id \"%1\" is outside the module's namespace")
                  .arg(id));
        }
        if (m.commands.contains(id)) {
          return Malformed(
              QString("command id \"%1\" is declared twice").arg(id));
        }
        m.commands.append(id);
      }
    }
  }

  // translation_context: required, so the runtime loads translations named by
  // a verified value rather than by a constant compiled into the module.
  {
    const auto v = root.value("translation_context");
    if (v.isUndefined()) {
      return Malformed("\"translation_context\" is missing");
    }
    {
      if (!v.isString()) {
        return Malformed("\"translation_context\" is not a string");
      }
      m.translation_context = v.toString();
      if (m.translation_context.isEmpty()) {
        return Malformed("\"translation_context\" is empty");
      }
    }
  }

  // metadata: required object, every value a string. This is what the module
  // used to answer by being loaded and asked -- which meant the only way to
  // learn what a module claimed to be was to run its initialisers first.
  {
    QJsonObject meta;
    if (!TakeObject(root, "metadata", meta, error)) return Malformed(error);
    for (auto it = meta.constBegin(); it != meta.constEnd(); ++it) {
      if (!it.value().isString()) {
        return Malformed(
            QString("\"metadata.%1\" is not a string").arg(it.key()));
      }
      m.metadata.insert(it.key(), it.value().toString());
    }
  }

  {
    QJsonObject build;
    if (!TakeObject(root, "build", build, error)) return Malformed(error);
    if (!TakeString(build, "id", m.build_id, error) ||
        !TakeString(build, "timestamp", m.build_timestamp, error) ||
        !TakeString(build, "source_commit", m.build_source_commit, error)) {
      return Malformed(QString("build.%1").arg(error));
    }
  }

  {
    QJsonObject platform;
    if (!TakeObject(root, "platform", platform, error)) return Malformed(error);
    if (!TakeString(platform, "os", m.platform_os, error) ||
        !TakeString(platform, "arch", m.platform_arch, error) ||
        !TakeString(platform, "qt", m.platform_qt, error)) {
      return Malformed(QString("platform.%1").arg(error));
    }
  }

  // entry_native: the one executable artifact this descriptor binds. It lives
  // OUTSIDE the package -- nothing executable is ever carried inside one --
  // so what is recorded here is a logical name and a verification value, and
  // the Host is what turns the first into a path.
  {
    QJsonObject entry;
    if (!TakeObject(root, "entry_native", entry, error)) {
      return Malformed(error);
    }

    if (!TakeString(entry, "name", m.entry_native.name, error)) {
      return Malformed(QString("entry_native.%1").arg(error));
    }
    if (!IsLogicalNativeName(m.entry_native.name)) {
      return Malformed(
          QString("\"%1\" is not a logical native name: it must match "
                  "[a-z][a-z0-9_]{0,63} and must not begin with \"lib\"")
              .arg(m.entry_native.name));
    }

    // The mode is not a choice, and neither is whether one is permitted at
    // all. Both follow platform.os, which was parsed above and which the
    // verifier separately checks against the host it is running on -- so a
    // descriptor cannot select a weaker mode by claiming a platform, because
    // the claim is refused first.
    const auto rule = ModuleEntryVerificationModeFor(m.platform_os);
    if (!rule.known_os) {
      return Malformed(
          QString("there is no entry verification mode for platform \"%1\"")
              .arg(m.platform_os));
    }

    const auto verification_value = entry.value("verification");

    if (verification_value.isUndefined()) {
      // No claim about the entry's bytes. Legal here, because this parser
      // describes structure and not trust: whether an unbound descriptor may
      // be LOADED is decided by origin and Host policy, in
      // ResolveAndVerifyNativeEntry(). Saying no here would put a trust
      // decision in the one place that cannot see who is asking.
      //
      // A bare size is refused with it. Without this, a descriptor could
      // carry a size and no binding and look like it proved something.
      if (!entry.value("size").isUndefined()) {
        return Malformed(
            "entry_native.size is only meaningful alongside an "
            "entry_native.verification");
      }
    } else {
      if (!verification_value.isObject()) {
        return Malformed("entry_native.verification is not an object");
      }
      // Present but wrong-typed is a corrupt descriptor, never an omission:
      // the two are distinguished above, by isUndefined() alone.
      const auto verification = verification_value.toObject();

      if (!rule.mode.has_value()) {
        return Malformed(
            QString("a \"%1\" module carries no entry verification; its "
                    "executable code is authenticated by the platform")
                .arg(m.platform_os));
      }

      ModuleEntryVerification v;

      QString mode_key;
      if (!TakeString(verification, "mode", mode_key, error) ||
          !TakeString(verification, "value", v.value, error)) {
        return Malformed(QString("entry_native.verification.%1").arg(error));
      }

      const auto expected_key = ModuleEntryVerificationModeKey(*rule.mode);
      if (mode_key != expected_key) {
        return Malformed(
            QString("entry_native.verification.mode is \"%1\", but a \"%2\" "
                    "module must use \"%3\"")
                .arg(mode_key, m.platform_os, expected_key));
      }
      v.mode = *rule.mode;

      // Both modes carry a 256-bit value as hex. They mean different things
      // -- a file digest and a PE image digest -- and the check here is only
      // that the spelling is one a comparison can be made against.
      if (!IsModuleHexDigest(v.value)) {
        return Malformed(
            "entry_native.verification.value is not 64 lower-case hexadecimal "
            "characters");
      }

      // size: optional, and only where it means anything. Windows
      // Authenticode signing appends a certificate table, so under that mode
      // a recorded size is an invariant that legitimately breaks.
      const auto size_value = entry.value("size");
      if (!size_value.isUndefined()) {
        if (v.mode != ModuleEntryVerificationMode::kFILE_SHA256) {
          return Malformed(
              QString("entry_native.size is only meaningful with \"%1\"; "
                      "platform signing legitimately changes the size of a "
                      "\"%2\" entry")
                  .arg(ModuleEntryVerificationModeKey(
                           ModuleEntryVerificationMode::kFILE_SHA256),
                       m.platform_os));
        }
        if (!size_value.isDouble()) {
          return Malformed("entry_native.size is not a number");
        }
        const auto as_double = size_value.toDouble();
        if (as_double < 0 || as_double != std::floor(as_double)) {
          return Malformed(
              "entry_native.size is not a whole, non-negative "
              "number");
        }
        v.size = static_cast<qint64>(as_double);
      }

      m.entry_native.verification = v;
    }
  }

  // resources: required, MAY BE EMPTY. Empty is a real statement -- this
  // module carries no non-executable members -- and is different from the
  // field being absent, which is a manifest that predates the distinction.
  {
    const auto v = root.value("resources");
    if (v.isUndefined()) return Malformed("\"resources\" is missing");
    if (!v.isArray()) return Malformed("\"resources\" is not an array");

    for (const auto& f : v.toArray()) {
      if (!f.isObject()) {
        return Malformed("a value in \"resources\" is not an object");
      }
      const auto fo = f.toObject();
      ModuleManifestResource entry;
      if (!TakeString(fo, "path", entry.path, error) ||
          !TakeString(fo, "sha256", entry.sha256, error)) {
        return Malformed(QString("resources.%1").arg(error));
      }
      if (!IsModuleHexDigest(entry.sha256)) {
        return Malformed(QString("the digest of \"%1\" is not 64 lower-case "
                                 "hexadecimal characters")
                             .arg(entry.path));
      }
      m.resources.append(entry);
    }
  }

  ModuleManifestParseResult result;
  result.ok = true;
  result.status = ModuleManifestStatus::kOK;
  result.manifest = m;
  return result;
}

auto ManifestHostOsName() -> QString {
#if defined(Q_OS_WIN)
  return "windows";
#elif defined(Q_OS_MACOS)
  return "macos";
#elif defined(Q_OS_LINUX)
  return "linux";
#else
  return QSysInfo::kernelType();
#endif
}

auto NormalizeManifestArch(const QString& arch) -> QString {
  const auto lower = arch.trimmed().toLower();

  // 64-bit ARM. `aarch64` is what CMake, gcc and the Linux kernel say;
  // `arm64` is what Qt and Apple say.
  if (lower == "aarch64" || lower == "arm64" || lower == "aarch64_be" ||
      lower == "armv8" || lower == "armv8-a") {
    return "arm64";
  }

  // 64-bit x86. `x86_64` from CMake and uname, `amd64` from Debian, `x64`
  // from MSVC.
  if (lower == "x86_64" || lower == "amd64" || lower == "x64") {
    return "x86_64";
  }

  // 32-bit x86, whichever of the several names the toolchain prefers.
  if (lower == "i386" || lower == "i486" || lower == "i586" ||
      lower == "i686" || lower == "x86") {
    return "i386";
  }

  // Anything else is passed through lower-cased rather than guessed at. An
  // unknown architecture that compares equal to itself still works; one this
  // function mangled would fail in a way nothing here could explain.
  return lower;
}

auto ManifestHostArchName() -> QString {
  return NormalizeManifestArch(QSysInfo::currentCpuArchitecture());
}

auto SdkAbiRejection(int abi) -> std::optional<QString> {
  if (abi >= GF_SDK_ABI_MIN_SUPPORTED && abi <= GF_SDK_ABI_VERSION) return {};

  return QString(
             "it was built against sdk abi %1, and this version of "
             "GpgFrontend supports %2 to %3")
      .arg(abi)
      .arg(GF_SDK_ABI_MIN_SUPPORTED)
      .arg(GF_SDK_ABI_VERSION);
}

}  // namespace GpgFrontend::Module
