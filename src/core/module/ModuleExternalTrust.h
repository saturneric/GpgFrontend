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

#include "GFCoreExport.h"

namespace GpgFrontend::Module {

/**
 * @file ModuleExternalTrust.h
 * @brief The user's decisions about external modules, and nothing else.
 *
 * External modules cross a different boundary from the ones shipped inside
 * this application, and the difference is not a stricter check -- it is that
 * a person has to say yes. Discovery finds them; it never runs them.
 *
 * ```
 * Discovered -> Descriptor Verified -> Build Key Shown -> Build Key Trusted
 *            -> Module Enabled -> Strict Native Binding -> Loaded
 * ```
 *
 * Two decisions, kept apart on purpose:
 *
 *   trust a build key   "signatures by this key are worth considering"
 *   enable a module     "run this one"
 *
 * Neither implies the other. Trusting a key does not enable every module
 * signed by it, and enabling a module does not bypass key trust -- so a
 * single careless click cannot admit a set of modules the user never looked
 * at. The Host owns both; a module can influence neither.
 *
 * ## Build keys, not publishers
 *
 * These keys are ephemeral and per build tree. Trust is therefore specific to
 * one build key: a module re-signed under a different one matches nothing
 * here and returns to Pending. There is deliberately no publisher identity,
 * no rotation, no succession and no catalog.
 */

/// How this module stands with the user right now.
enum class ModuleAuthorizationState {
  kTRUSTED_AND_ENABLED,  ///< both decisions made; loading may proceed
  kKEY_UNTRUSTED,        ///< the build key has not been accepted
  kNOT_ENABLED,          ///< key accepted, but this module was not enabled
};

/// A build key the user has accepted, for listing and revocation.
struct GF_CORE_EXPORT ModuleTrustedBuildKey {
  QByteArray build_key;  ///< the 32 raw bytes; canonical
  QString label;
  QString first_trusted;
};

/**
 * @brief The fingerprint of a build key, for showing a person.
 *
 * Derived, never stored: see ModuleTrustSO.h. Grouped the way every other
 * fingerprint in this application is, so it reads the same way.
 */
auto GF_CORE_EXPORT ModuleBuildKeyFingerprint(const QByteArray& build_key)
    -> QString;

/// Every build key the user has trusted.
auto GF_CORE_EXPORT ListTrustedModuleBuildKeys()
    -> QList<ModuleTrustedBuildKey>;

/// Whether @p build_key is one of them.
auto GF_CORE_EXPORT IsModuleBuildKeyTrusted(const QByteArray& build_key)
    -> bool;

/// Record a decision to trust @p build_key. Enables nothing.
auto GF_CORE_EXPORT TrustModuleBuildKey(const QByteArray& build_key,
                                        const QString& label) -> bool;

/**
 * @brief Withdraw trust from @p build_key.
 *
 * Every module authorized under it returns to Pending, because an approval
 * granted on the strength of a key cannot outlive the decision it rested on.
 * The authorizations are left in place rather than deleted, so re-trusting
 * the key does not silently re-enable them: they are still gated on the key.
 */
auto GF_CORE_EXPORT RevokeModuleBuildKey(const QByteArray& build_key) -> bool;

/// Whether the user enabled @p module_id under exactly @p build_key.
auto GF_CORE_EXPORT IsExternalModuleEnabled(const QString& module_id,
                                            const QByteArray& build_key)
    -> bool;

/// Record the second decision. Does not trust the key.
auto GF_CORE_EXPORT SetExternalModuleEnabled(const QString& module_id,
                                             const QByteArray& build_key,
                                             bool enabled) -> bool;

/**
 * @brief Both decisions, as one answer.
 *
 * Order matters and is not arbitrary: the key is asked about first, so a
 * module whose key was never accepted reports that rather than "not enabled"
 * -- which would send the user to the wrong control.
 */
auto GF_CORE_EXPORT ExternalModuleAuthorization(const QString& module_id,
                                                const QByteArray& build_key)
    -> ModuleAuthorizationState;

/// For logs and reports.
auto GF_CORE_EXPORT ModuleAuthorizationStateToString(
    ModuleAuthorizationState state) -> const char*;

}  // namespace GpgFrontend::Module
