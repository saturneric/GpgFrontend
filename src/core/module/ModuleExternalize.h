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

#include "core/module/ModuleManifest.h"

namespace GpgFrontend::Module {

/**
 * @file ModuleExternalize.h
 * @brief From a verified integrated module to a publisher-signed external one.
 *
 * ## A trust-domain transition, not a re-signature
 *
 * The input is trusted because this Host build signed it. The output is
 * trusted only if a user decides to trust its publisher. Crossing between the
 * two is allowed in one direction, once, and only for facts the integrated
 * signature already established:
 *
 * ```
 * verify input as integrated (build key, build id, namespace, bound native)
 *  -> refuse unbound helpers
 *  -> copy the entry native to staging, verify the COPY against the binding
 *  -> check every manifest field against the preservation contract
 *  -> build the external descriptor field by field, publisher-signed
 *  -> verify the result as external, and as NOT integrated
 *  -> publish by rename, never over an existing namespace
 * ```
 *
 * Externalization is the last cryptographic step. Nothing it covers may be
 * rewritten afterwards -- the one exception the format itself defines is
 * Authenticode-signing a Windows entry, whose digest excludes the
 * certificate table -- and there is no external reseal: that would need the
 * publisher's private key wherever the rewrite happened.
 *
 * ## Deterministic and stateless
 *
 * A pure transformation of (input namespace, publisher seed, flags) into
 * bytes. No settings, no profile, no module runtime, no trust store: the
 * user's decisions belong to the Host that loads the result, not to the tool
 * that makes it.
 */

/// Manifest metadata keys externalization may add. Claims, never identity.
constexpr auto kModuleMetadataPublisher = "Publisher";
constexpr auto kModuleMetadataPublisherUrl = "PublisherUrl";

/// What externalization may do with one manifest field.
enum class ModuleExternalizeDisposition {
  /// Must be equal between input and output.
  kPRESERVE,
  /// May be set by externalization, and only by it.
  kCHANGE,
  /// A container whose members carry their own rows.
  kSTRUCTURE,
};

/// One row of the preservation contract.
struct GF_CORE_EXPORT ModuleExternalizeField {
  /// A key path: `build.id`, `resources[].sha256`. A trailing `.*` covers
  /// every key of a free-form map, `metadata.*`.
  QString path;
  ModuleExternalizeDisposition disposition;
};

/**
 * @brief The preservation contract: every manifest field, and its fate.
 *
 * An explicit whitelist. A field with no row here stops externalization; a
 * field is never carried across because nobody thought to exclude it. Adding
 * a field to the manifest therefore requires deciding, here, what crossing
 * the trust boundary does to it.
 */
auto GF_CORE_EXPORT ModuleExternalizeContract()
    -> QVector<ModuleExternalizeField>;

/**
 * @brief Every key path in a manifest, as the contract spells them.
 *
 * Objects contribute `a.b`, arrays of objects `a[].b`, and an array of
 * scalars is one leaf. Exposed so a test can prove the contract covers
 * exactly what the builder writes.
 */
auto GF_CORE_EXPORT ModuleManifestKeyPaths(const QByteArray& manifest_bytes)
    -> QStringList;

struct GF_CORE_EXPORT ModuleExternalizeSpec {
  /// The integrated namespace directory, or the `module.gfmodule` in it.
  QString input;

  /// The publisher's 32-byte Ed25519 seed. Never the build seed: the builder
  /// refuses one that derives this Host's build key.
  QByteArray publisher_seed;

  /// Where the external namespace is created, as `<root>/<directory key>/`.
  QString output_root;

  /// Optional, unverified claims recorded as manifest metadata.
  QString publisher_name;
  QString publisher_url;
};

struct GF_CORE_EXPORT ModuleExternalizeResult {
  bool ok = false;
  QString reason;

  /// The published external namespace directory.
  QString output_namespace;

  /// The publisher key the output is signed with, and carries.
  QByteArray publisher_key;

  /// The output's manifest, as verified by the external verifier.
  ModuleManifest manifest;
};

/**
 * @brief Turn a verified integrated module namespace into an external one.
 *
 * Nothing partial is ever left in @p spec.output_root: the namespace appears
 * complete or not at all.
 */
auto GF_CORE_EXPORT ExternalizeModule(const ModuleExternalizeSpec& spec)
    -> ModuleExternalizeResult;

}  // namespace GpgFrontend::Module
