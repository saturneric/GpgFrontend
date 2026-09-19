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

namespace GpgFrontend::Module {

/// The only manifest schema this build understands.
constexpr int kModuleManifestSchemaVersion = 1;

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
 * @brief One file the manifest covers, and the digest it must have.
 */
struct GF_CORE_EXPORT ModuleManifestFile {
  QString path;    ///< archive-relative, forward slashes
  QString sha256;  ///< lower-case hex, 64 characters
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

  /// Display metadata: Name, Description, Author. Free-form by design.
  QMap<QString, QString> metadata;

  QString build_id;
  QString build_timestamp;
  QString build_source_commit;

  QString platform_os;
  QString platform_arch;
  QString platform_qt;

  QVector<ModuleManifestFile> files;
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

}  // namespace GpgFrontend::Module
