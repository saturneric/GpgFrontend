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

/// The only catalog schema this build understands.
constexpr int kModuleCatalogSchemaVersion = 1;

/**
 * @brief What a catalog says about one build of one module.
 *
 * Keyed by the package digest rather than by the version string: a version is
 * what a human wrote, the digest is what the bytes are, and the whole point of
 * a catalog is to say something about specific bytes.
 */
struct GF_CORE_EXPORT ModuleCatalogEntry {
  QString module_id;
  QString version;
  QString package_sha256;    ///< lower-case hex, 64 characters
  QString build_public_key;  ///< lower-case hex, 64 characters (32 bytes)
  int security_epoch = 0;
  bool revoked = false;
};

/**
 * @brief A parsed, signature-verified catalog.
 */
struct GF_CORE_EXPORT ModuleCatalog {
  int schema_version = 0;
  QString issued_at;
  QVector<ModuleCatalogEntry> entries;

  /**
   * @brief The entry for a specific package, by its digest.
   *
   * @param package_sha256 the digest of the whole `*.gfmodule`
   * @return the entry, or nothing when the catalog does not mention it
   */
  [[nodiscard]] auto FindByDigest(const QString& package_sha256) const
      -> std::optional<ModuleCatalogEntry>;

  /**
   * @brief Every entry for one module, in the order the catalog listed them.
   *
   * @param module_id the module wanted
   * @return its entries, possibly empty
   */
  [[nodiscard]] auto EntriesFor(const QString& module_id) const
      -> QVector<ModuleCatalogEntry>;
};

/**
 * @brief Why a catalog was refused.
 */
enum class ModuleCatalogStatus {
  kOK,
  kMALFORMED,      ///< not JSON, not an object, or a field is wrong
  kTOO_NEW,        ///< schema_version beyond what this build knows
  kBAD_SIGNATURE,  ///< it was not signed by the root key given
};

/**
 * @brief Human-readable spelling of a status, for logs and messages.
 *
 * @param s status to spell
 * @return a short static string
 */
auto GF_CORE_EXPORT ModuleCatalogStatusToString(ModuleCatalogStatus s) -> const
    char*;

/**
 * @brief The outcome of reading a catalog.
 */
struct GF_CORE_EXPORT ModuleCatalogVerification {
  bool ok = false;
  ModuleCatalogStatus status = ModuleCatalogStatus::kOK;
  QString reason;
  ModuleCatalog catalog;  ///< parsed only after the signature verified
};

/**
 * @brief Verify a catalog against a root key, then parse it.
 *
 * ## This is not wired to anything, deliberately
 *
 * Nothing in the application reads a catalog yet, and no root key is embedded
 * anywhere. What exists here is the format and the reader, so that the trust
 * decision -- whose key, published where -- can be made without the format
 * being designed under time pressure afterwards.
 *
 * Until that decision is made, package verification remains what it was: a
 * package establishes that it agrees with itself, and says nothing about who
 * built it. A catalog is what would change that, because it supplies the
 * expected key from outside the package.
 *
 * ## Why there is no function here that writes one
 *
 * Signing a catalog needs the root secret key, and that is a publishing
 * operation rather than something an application does. An application that
 * could mint a catalog would be an application carrying the secret that makes
 * catalogs meaningful.
 *
 * ## Ordering
 *
 * The same rule the manifest follows, for the same reason: the signature is
 * verified over the bytes exactly as supplied, and only then are they parsed.
 * So there is no canonicaliser on this side to disagree with whatever produced
 * the catalog, and the JSON parser only ever sees signature-intact bytes.
 *
 * @param catalog_bytes the catalog exactly as stored
 * @param signature detached Ed25519 signature over those bytes
 * @param root_public_key the key the catalog must have been signed with
 * @return the parsed catalog, or why it was refused
 */
auto GF_CORE_EXPORT VerifyModuleCatalog(const QByteArray& catalog_bytes,
                                        const QByteArray& signature,
                                        const QByteArray& root_public_key)
    -> ModuleCatalogVerification;

}  // namespace GpgFrontend::Module
