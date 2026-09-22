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

#include "core/module/ModuleHostPolicy.h"
#include "core/module/ModuleManifest.h"

namespace GpgFrontend::Module {

/**
 * @brief One non-executable resource to put into a descriptor.
 */
struct GF_CORE_EXPORT ModuleResourceSource {
  QString archive_path;  ///< where it goes, archive-relative, forward slashes
  QString source_file;   ///< read from here, when set
  QByteArray bytes;      ///< otherwise, these
};

/**
 * @brief Everything a descriptor needs to be built.
 *
 * The platform and build fields are inputs rather than things this deduces,
 * because the tool that fills them in is the one that knows: a cross-built
 * descriptor is described by its target, not by the machine that ran the
 * packager.
 */
struct GF_CORE_EXPORT ModuleDescriptorBuildSpec {
  QString module_id;
  QString version;
  int sdk_abi = 0;
  QString min_host_version;
  int security_epoch = 0;
  QStringList capabilities;

  /// Event ids the module subscribes to. Sorted by the caller so the canonical
  /// manifest is byte-identical across rebuilds.
  QStringList events;

  /// Names the module's .qm files.
  QString translation_context;
  QMap<QString, QString> metadata;

  QString build_id;
  QString build_timestamp;
  QString build_source_commit;

  /// Left empty, this defaults to ManifestHostOsName() -- the same spelling
  /// the verifier compares against, so a native build cannot produce a package
  /// its own host would refuse. Set it explicitly only to cross-package.
  QString platform_os;
  QString platform_arch;
  QString platform_qt;

  /// Non-executable members carried inside the descriptor. May be empty, and
  /// is in every descriptor the tree currently produces.
  QVector<ModuleResourceSource> resources;

  /// The logical name of the module's entry native, matching
  /// `[a-z][a-z0-9_]{0,63}` and not beginning with `lib`. This is what the
  /// descriptor records; the Host maps it to a filename.
  QString entry_native_name;

  /// The finished entry native on disk, read to compute the binding value.
  /// It is NOT packaged: executable code never travels inside a descriptor.
  /// It must already have had every platform preparation step applied to it,
  /// because what is recorded is what is there now.
  ///
  /// Not read at all when @c entry_binding is kNOT_REQUIRED: a value that
  /// would be recorded and never consulted is work, and a claim nobody
  /// checks is worse than no claim.
  QString entry_native_file;

  /// Whether this descriptor must bind the bytes of its entry.
  ///
  /// kREQUIRED is the default so that a caller which forgets produces the
  /// STRONGER descriptor. The packager spells it `--entry-binding`, as a
  /// statement about the descriptor rather than as an algorithm name: there
  /// is deliberately no way to ask for "algorithm: none", because absence is
  /// not an algorithm and modelling it as one would put the choice in the
  /// hands of whoever writes the manifest.
  ///
  /// On macOS kREQUIRED is refused: there is no mode to honour.
  ModuleBindingRequirement entry_binding = ModuleBindingRequirement::kREQUIRED;

  /// Which trust domain this descriptor is built for.
  ///
  /// Named for what it selects, which is the whole contract and not just the
  /// signer -- the same enum, and the same word, the verifier and the entry
  /// policy take, so producer and verifier share one vocabulary:
  ///
  /// | | kINTEGRATED | kEXTERNAL |
  /// |---|---|---|
  /// | seed must derive | ModuleBuildPublicKey() | anything BUT it |
  /// | key member | none | `META-INF/publisher.pub` |
  /// | entry binding | as @c entry_binding says | kREQUIRED, or refused |
  /// | accepted by | VerifyModuleDescriptor() |
  /// VerifyExternalModuleDescriptor() | | trusted because | this Host was built
  /// with the key | the user trusted the key |
  ///
  /// kINTEGRATED is the default and the only value the packager ever sets.
  /// kEXTERNAL is gf_module_externalize's, and only after the input verified
  /// as integrated.
  ModuleOrigin origin = ModuleOrigin::kINTEGRATED;

  /// The 32-byte Ed25519 seed to sign with.
  ///
  /// Required. For kINTEGRATED it must derive the public key the Host was
  /// built with; for kEXTERNAL it is a publisher's seed and must NOT derive
  /// that key. BuildModuleDescriptor() checks both rather than trusting the
  /// caller to have passed the right file.
  QByteArray signing_seed;

  QString output_path;  ///< the `*.gfmodule` to write
};

/**
 * @brief What building a descriptor produced.
 */
struct GF_CORE_EXPORT ModuleDescriptorBuildResult {
  bool ok = false;
  QString reason;
  QByteArray signer_public_key;  ///< the key this descriptor was signed with
  QByteArray manifest_bytes;     ///< exactly the bytes the signature covers
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
 * @brief Build and sign a `*.gfmodule` descriptor.
 *
 * The signing key is **not** generated here. For an integrated descriptor
 * @ref ModuleDescriptorBuildSpec::signing_seed must derive the public key this
 * Host build was compiled with, and this function checks that before writing
 * anything -- so a descriptor that verifies at all was signed by the key the
 * Host carries, and `pack` structurally cannot produce one the Host would
 * refuse. For an external one the rule inverts: the build key is never a
 * publisher identity, so a seed deriving it is refused.
 *
 * No executable code goes in. The entry native is read only to compute its
 * binding value under the mode its platform mandates; the file itself stays
 * where it was built, and the descriptor names it logically.
 *
 * @param spec what to describe, and what to sign it with
 * @return whether it was written, and the public key it was signed with
 */
auto GF_CORE_EXPORT BuildModuleDescriptor(const ModuleDescriptorBuildSpec& spec)
    -> ModuleDescriptorBuildResult;

}  // namespace GpgFrontend::Module
