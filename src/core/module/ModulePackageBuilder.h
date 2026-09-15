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
 * @brief One file to put into a package.
 */
struct GF_CORE_EXPORT ModulePackageSource {
  QString archive_path;  ///< where it goes, archive-relative, forward slashes
  QString source_file;   ///< read from here, when set
  QByteArray bytes;      ///< otherwise, these
};

/**
 * @brief Everything a package needs to be built.
 *
 * The platform and build fields are inputs rather than things this deduces,
 * because the tool that fills them in is the one that knows: a cross-built
 * package is described by its target, not by the machine that ran the
 * packager.
 */
struct GF_CORE_EXPORT ModulePackageBuildSpec {
  QString module_id;
  QString version;
  int sdk_abi = 0;
  QString min_host_version;
  int security_epoch = 0;
  QStringList capabilities;
  QMap<QString, QString> metadata;

  QString build_id;
  QString build_timestamp;
  QString build_source_commit;

  QString platform_os;
  QString platform_arch;
  QString platform_qt;

  QVector<ModulePackageSource> files;

  QString output_path;  ///< the `*.gfmodule` to write
};

/**
 * @brief What building a package produced.
 */
struct GF_CORE_EXPORT ModulePackageBuildResult {
  bool ok = false;
  QString reason;
  QByteArray build_public_key;  ///< the ephemeral key this build signed with
  QByteArray manifest_bytes;    ///< exactly the bytes the signature covers
};

/**
 * @brief Serialise a JSON value the way RFC 8785 says to.
 *
 * Restricted on purpose to what a manifest holds: objects, arrays, strings and
 * whole numbers. JCS also pins the shortest round-tripping form for
 * non-integral numbers, which is real work for a case that cannot occur here,
 * so a non-integral number is refused rather than approximated.
 *
 * ## This is a build-output-quality rule, not a security one
 *
 * Nothing in verification depends on it. The signature covers the bytes as
 * *stored*, so a manifest serialised any other way would verify perfectly
 * well, and there is deliberately no canonicaliser on the verifying side that
 * could disagree with this one. What canonical form buys is that the same
 * inputs produce byte-identical manifests: reproducible builds, diffable
 * manifests, a stable package digest across rebuilds. A bug here produces a
 * package that differs between builds, never one that fails to verify.
 *
 * Note that Qt's QJsonDocument::toJson(Compact) is not JCS. QJsonObject keeps
 * its keys sorted, which gets member ordering right by accident, but the
 * escaping and number rules are Qt's own.
 *
 * @param value object or array to serialise
 * @param[out] out the canonical bytes
 * @return false if the value holds something JCS-restricted cannot express
 */
auto GF_CORE_EXPORT CanonicalJson(const QJsonValue& value, QByteArray& out)
    -> bool;

/**
 * @brief Build and sign a `*.gfmodule` package.
 *
 * Each package is signed with a **freshly generated Ed25519 key pair** whose
 * private half exists only in this function's memory and is wiped before it
 * returns. It is never written into the package, and never into any build
 * artifact. That means there is no long-lived signing secret to steal, and
 * compromising one build cannot forge another -- which is also what lets a
 * catalog later record a per-build public key without changing anything here.
 *
 * What the resulting signature proves is narrow, and stated in full on
 * VerifyModulePackage(): the public key travels inside the package, so this
 * establishes self-consistency and nothing about who built it.
 *
 * @param spec what to package
 * @return whether it was written, and the public key it was signed with
 */
auto GF_CORE_EXPORT BuildModulePackage(const ModulePackageBuildSpec& spec)
    -> ModulePackageBuildResult;

}  // namespace GpgFrontend::Module
