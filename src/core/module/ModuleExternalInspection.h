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

#include "core/module/ModuleDescriptor.h"
#include "core/module/ModuleEntryBinding.h"
#include "core/module/ModuleExternalTrust.h"

namespace GpgFrontend::Module {

/**
 * @file ModuleExternalInspection.h
 * @brief Everything the Host can say about one external module, kept apart.
 *
 * A module that will not load can fail for reasons that need different
 * responses, and one "refused" hides which. So each question is answered on
 * its own:
 *
 * | question | answered by | means |
 * |---|---|---|
 * | signature valid? | the package | intact, signed by the key it names |
 * | who signed it? | the package | the publisher key and its fingerprint |
 * | publisher trusted? | the USER's store | never by the package |
 * | module enabled? | the USER's store | never by the package |
 * | compatible? | the package vs this Host | platform, SDK ABI, min host
 * version | | right namespace? | the directory | the key the signed id derives
 * | | native bound? | the native vs the package | the entry is the one signed
 * for |
 *
 * A valid signature is NOT trust: it proves only that the package agrees
 * with the key it carries. `build_id` answers none of these questions; in
 * the external domain it is provenance only.
 *
 * A report, not a gate. ModuleManager keeps its own order, which asks the
 * user's decisions before a native is ever opened; this reads and hashes the
 * native regardless, which is fine for describing a module and never runs
 * it. It never loads code.
 */
struct GF_CORE_EXPORT ExternalModuleReport {
  QString descriptor_path;

  /// Structure, Ed25519 signature against the carried key, and resource
  /// integrity. The only thing the signature proves.
  bool signature_valid = false;

  /// Why the descriptor was refused, when it was: an authentication failure
  /// if !signature_valid, otherwise a compatibility one. kOK when neither.
  ModuleDescriptorStatus descriptor_status = ModuleDescriptorStatus::kOK;
  QString descriptor_reason;

  /// The carried publisher key and its display fingerprint. Empty unless
  /// @c signature_valid.
  QByteArray publisher_key;
  QString fingerprint;

  /// The user's two decisions, from the trust store and nothing else.
  bool publisher_trusted = false;
  bool module_enabled = false;
  ModuleAuthorizationState authorization =
      ModuleAuthorizationState::kPUBLISHER_UNTRUSTED;

  /// Platform, SDK ABI and minimum host version. Never build_id.
  bool compatible = false;

  /// The directory is the one the signed id derives.
  bool namespace_valid = false;

  /// The entry native exists, is contained, and is the one bound.
  bool native_binding_valid = false;
  ModuleEntryStatus native_status = ModuleEntryStatus::kOK;
  QString native_reason;

  /// Filled whenever @c signature_valid.
  ModuleManifest manifest;

  /// Every answer yes -- the conditions under which the Host loads it.
  [[nodiscard]] auto Loadable() const -> bool {
    return signature_valid && publisher_trusted && module_enabled &&
           compatible && namespace_valid && native_binding_valid;
  }
};

/**
 * @brief Answer each question about one external module separately.
 *
 * @param path the namespace directory, or the `module.gfmodule` in it
 */
auto GF_CORE_EXPORT InspectExternalModule(const QString& path)
    -> ExternalModuleReport;

/**
 * @brief The report as JSON, one member per question.
 *
 * For `--module-status`. The key appears only as its fingerprint, the form a
 * person compares.
 */
auto GF_CORE_EXPORT
ExternalModuleReportToJson(const ExternalModuleReport& report) -> QJsonObject;

}  // namespace GpgFrontend::Module
