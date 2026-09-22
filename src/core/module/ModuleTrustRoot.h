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

/**
 * @file ModuleTrustRoot.h
 * @brief What this Host will accept a module descriptor from.
 *
 * Compiled in, not configured, not looked up. Verification is therefore
 * offline by construction: there is no catalog to reach, no key to distribute
 * and no network call anywhere on the module load path.
 *
 * ## What it is
 *
 * The public half of an ephemeral Ed25519 keypair generated once per build
 * tree, whose private half signed every descriptor that build produced and
 * then stayed in the workspace. It answers one question -- *was this module
 * produced by this exact Host build* -- and deliberately not "who published
 * this", which is a different question needing a key delivered out of band.
 *
 * ## Why the package no longer carries one
 *
 * It used to. `META-INF/build-key.pub` travelled inside the archive, so anyone
 * who could replace the package could also generate a keypair, re-sign an
 * altered manifest and ship the matching key. That established internal
 * consistency and nothing else. A trust root that travels with the thing it
 * vouches for vouches for nothing.
 *
 * An EXTERNAL descriptor does carry a key again -- `META-INF/publisher.pub`
 * -- but as the name of its signer, never as a trust root: external trust
 * comes only from the user's decisions (ModuleExternalTrust.h), and this
 * build key is refused wherever a publisher key is expected.
 */

/// The 32 raw bytes this build verifies descriptor signatures with.
auto GF_CORE_EXPORT ModuleBuildPublicKey() -> const QByteArray&;

/// The build identity every descriptor this Host loads must name.
///
/// Checked in addition to the signature, not instead of it. Two build trees
/// with the same product configuration share a build id and differ by key; the
/// pair is what identifies an instance.
auto GF_CORE_EXPORT ModuleBuildId() -> const QString&;

}  // namespace GpgFrontend::Module
