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

#pragma once

#include <optional>

namespace GpgFrontend::Module {

/// The only manifest schema this build understands.
constexpr int kModuleManifestSchemaVersion = 3;

/// The oldest schema this build will read.
///
/// Equal to the current one, deliberately: schema 3 moved the native image out
/// of the package entirely, so a schema 2 manifest does not describe a
/// different arrangement of the same thing -- it describes a package whose
/// executable payload is inside it, which this build will not load at all.
/// Reading one and refusing it later, on a field it does not have, would be a
/// worse sentence than refusing it by version here.
constexpr int kModuleManifestMinSupportedSchema = 3;

/**
 * @brief Why a manifest was refused.
 *
 * The taxonomy deliberately separates "written by a newer GpgFrontend" from
 * "wrong", because only one of the two is worth telling a user to upgrade
 * over. It mirrors ProfilePackage's (format_version, min_reader) discipline,
 * with the simplification that a module manifest has one version rather than
 * two: it is never re-serialised, so there is no forward-compatible read.
 */
enum class ModuleManifestStatus {
  kOK,         ///< parsed and structurally valid
  kMALFORMED,  ///< not JSON, not an object, or a field is wrong
  kTOO_NEW,    ///< schema_version beyond what this build knows
};

/**
 * @brief One non-executable member the manifest covers, and its digest.
 *
 * Resources live *inside* the package. Executable code never does: the entry
 * native is an external platform file named by ModuleEntryNative.
 */
struct GF_CORE_EXPORT ModuleManifestResource {
  QString path;    ///< archive-relative, forward slashes
  QString sha256;  ///< lower-case hex, 64 characters
};

/**
 * @brief How a platform decides that an entry native is the right one.
 *
 * The mode is a function of `platform.os` and is never chosen by a module
 * author. It differs per platform because the platforms differ: Windows
 * Authenticode signing and macOS code signing both rewrite bytes of a file
 * that is otherwise unchanged, and a full-file digest would either forbid
 * normal platform signing or have to be regenerated after it.
 */
enum class ModuleEntryVerificationMode {
  kFILE_SHA256,             ///< linux: the exact final ELF bytes
  kPE_AUTHENTICODE_SHA256,  ///< windows: PE image content, certificates
                            ///< excluded
};

/**
 * @brief Whether @p s is exactly 64 lower-case hex characters.
 *
 * Every 256-bit value in this subsystem is spelled that way -- a resource
 * digest, an entry binding value, a sealed preparation value -- so there is
 * one predicate for all of them. Three that agree today are three chances to
 * disagree later.
 */
auto GF_CORE_EXPORT IsModuleHexDigest(const QString& s) -> bool;

/// The wire spelling of a mode, as it appears in `verification.mode`.
auto GF_CORE_EXPORT
ModuleEntryVerificationModeKey(ModuleEntryVerificationMode mode) -> QString;

/**
 * @brief What a manifest for @p platform_os may carry, if anything.
 *
 * Two different "no" answers, which must not be conflated:
 *
 *   known_os == false   an os this build has never heard of. A refusal.
 *   mode == nullopt     a known os with no entry binding mode at all. macOS
 *                       is the only one: its module dylibs are signed by
 *                       Apple with the application's own identity and checked
 *                       by dyld at map time, so the descriptor makes no claim
 *                       about their bytes and must not pretend to.
 *
 * Returning a bare nullopt for both would mean a descriptor claiming
 * `platform.os = "haiku"` became an unbound module instead of a refused one.
 */
struct GF_CORE_EXPORT ModuleEntryVerificationModeRule {
  bool known_os = false;
  std::optional<ModuleEntryVerificationMode> mode;
};

auto GF_CORE_EXPORT ModuleEntryVerificationModeFor(const QString& platform_os)
    -> ModuleEntryVerificationModeRule;

/**
 * @brief The one executable artifact a descriptor binds.
 *
 * `name` is a *logical* name, never a path and never a platform filename. The
 * Host maps it to a file, which is what keeps a descriptor from influencing
 * where the loader looks.
 */
struct GF_CORE_EXPORT ModuleEntryVerification {
  ModuleEntryVerificationMode mode = ModuleEntryVerificationMode::kFILE_SHA256;
  QString value;  ///< meaning fixed by `mode`; 64 lower-case hex for both

  /// An early-mismatch optimisation and NEVER a security proof: a size
  /// disagreement refuses before hashing a large file. Permitted only with
  /// kFILE_SHA256, because Windows signing changes file size and an invariant
  /// that legitimately breaks is a trap rather than a check. Negative means
  /// absent.
  qint64 size = -1;
};

struct GF_CORE_EXPORT ModuleEntryNative {
  QString name;

  /// Absent means this descriptor makes NO claim about the bytes of its
  /// entry. Whether that is acceptable is the READER's question, answered
  /// from origin and Host policy -- never from here. See ModuleHostPolicy.h.
  ///
  /// std::optional rather than a `has_verification` flag, because a flag
  /// leaves {mode = kFILE_SHA256, value = ""} representable, and an empty
  /// value that looks like a mode is precisely the shape a skipped check
  /// would wear.
  ///
  /// `size` lives inside it, so "a size with no binding" cannot be spelled
  /// at all rather than merely being rejected by the parser.
  std::optional<ModuleEntryVerification> verification;
};

/**
 * @brief What a `*.gfmodule` package says about itself.
 *
 * Read *before* anything is executed, which is the whole reason it exists:
 * identity, ABI and platform used to be answerable only by loading the module
 * and calling into it, which is the wrong order for any decision about whether
 * the module should be loaded at all.
 */
struct GF_CORE_EXPORT ModuleManifest {
  int schema_version = 0;
  QString id;
  QString version;
  int sdk_abi = 0;
  QString min_host_version;

  /**
   * @brief Reserved for downgrade protection; NO policy is applied to it here.
   *
   * Type-checked like every other field -- a non-integer is a refusal -- and
   * then ignored. Nothing compares it against a stored high-water mark and
   * nothing persists it. It is present so that publisher trust can begin
   * enforcing an epoch without a schema bump.
   */
  int security_epoch = 0;

  QStringList capabilities;

  /// Event ids this module subscribes to, UPPER-CASE. An allowlist: the
  /// runtime subscribes to exactly these and refuses to start if its handler
  /// table disagrees, which is what stops a subscription and a handler from
  /// drifting apart unnoticed.
  QStringList events;

  /// Names this module's compiled .qm files. Verified, so the runtime can
  /// load translations without trusting a constant compiled into the module.
  QString translation_context;

  /// Display metadata: Name, Description, Author. Free-form by design.
  QMap<QString, QString> metadata;

  QString build_id;
  QString build_timestamp;
  QString build_source_commit;

  QString platform_os;
  QString platform_arch;
  QString platform_qt;

  /// The external executable this descriptor binds.
  ModuleEntryNative entry_native;

  /// Non-executable members carried inside the package. May be empty: no
  /// module ships one today, and the field exists so the first that does
  /// needs no schema change.
  QVector<ModuleManifestResource> resources;
};

/**
 * @brief Outcome of parsing a manifest.
 */
struct GF_CORE_EXPORT ModuleManifestParseResult {
  bool ok = false;
  ModuleManifestStatus status = ModuleManifestStatus::kOK;
  QString reason;  ///< human-readable, for the log and the UI
  ModuleManifest manifest;
};

/**
 * @brief Parse and validate a manifest, strictly.
 *
 * This inverts the habit of the settings layer, and the inversion is the
 * point. A SettingsObject that finds a string where it wanted a number keeps
 * its default and says nothing, which is right for a preference and wrong for
 * this: a mistyped `sdk_abi` must be a refusal, not a zero that happens to
 * compare as compatible with nothing.
 *
 * So: an unsupported schema version, a missing required field, or a field of
 * the wrong JSON type are each a hard failure. Unknown fields inside a
 * supported schema version are tolerated, so the format can grow additively,
 * and they need no round-trip storage because this manifest is never
 * re-serialised -- the signature covers the bytes as stored.
 *
 * Pure: no filesystem, no globals, no logging.
 *
 * @param bytes the raw manifest bytes, exactly as the package stores them
 * @return the parsed manifest, or why it was refused
 */
auto GF_CORE_EXPORT ParseModuleManifest(const QByteArray& bytes)
    -> ModuleManifestParseResult;

/**
 * @brief The os string a manifest must carry to run on this machine.
 *
 * One spelling, used by BOTH sides: the packager stamps `platform.os` with it
 * and the verifier compares against it. They used to be independent -- the
 * packager took the value from its command line -- so a packaging script with
 * a typo produced a package that was refused everywhere, for a reason that
 * named neither the script nor the typo.
 *
 * @return "windows", "macos", "linux", or the kernel type elsewhere
 */
auto GF_CORE_EXPORT ManifestHostOsName() -> QString;

/**
 * @brief The canonical spelling of a CPU architecture name.
 *
 * The same machine has more than one name depending on who is asked, and the
 * two that matter here disagree: CMake's `CMAKE_SYSTEM_PROCESSOR` says
 * `aarch64` on 64-bit ARM Linux, while `QSysInfo::currentCpuArchitecture()`
 * says `arm64`. The descriptor was stamped by the first and verified against
 * the second, so every module was refused on ARM Linux -- for a mismatch
 * between two names for one machine.
 *
 * Normalising rather than picking a side, and applied on BOTH sides, is what
 * makes this robust against the next toolchain that invents a third spelling:
 * a descriptor stamped `aarch64`, `arm64` or `ARM64` verifies on a host that
 * calls itself any of them, and an unrecognised name is passed through
 * unchanged rather than mangled.
 *
 * @param arch a name from any source, in any case
 * @return the canonical spelling, or @p arch lower-cased if it is unknown
 */
auto GF_CORE_EXPORT NormalizeManifestArch(const QString& arch) -> QString;

/**
 * @brief The architecture a manifest must name to run on this machine.
 *
 * The counterpart of ManifestHostOsName(), and it exists for exactly the same
 * reason: one spelling, used by both the packager and the verifier.
 */
auto GF_CORE_EXPORT ManifestHostArchName() -> QString;

/**
 * @brief Why this SDK ABI generation is unacceptable to this host.
 *
 * THE decision about ABI compatibility. It is deliberately one function
 * returning a reason rather than a predicate plus a message builder: the
 * range was previously compared in two places, in the verifier against the
 * manifest and in the loader against the module's own table, with two
 * differently-worded messages that could drift apart. A caller wanting a
 * yes/no asks `.has_value()` -- there is no second predicate to disagree with
 * this one.
 *
 * @param abi the ABI generation the module or manifest claims
 * @return nullopt when supported, else a sentence naming the supported range
 */
auto GF_CORE_EXPORT SdkAbiRejection(int abi) -> std::optional<QString>;

}  // namespace GpgFrontend::Module
