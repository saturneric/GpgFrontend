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

#include <memory>
#include <optional>

#include "core/module/Event.h"
#include "core/module/ModuleManifest.h"

struct GFModuleApi;

namespace GpgFrontend::Module {

class Module;

using ModuleIdentifier = QString;
using ModuleVersion = QString;
using ModuleMetaData = QMap<QString, QString>;
using ModulePtr = QSharedPointer<Module>;

/**
 * @brief One loaded module: its library, its self-description and its
 *        signed manifest, and the calls that enter it.
 *
 * A module describes itself through the single table GFModuleGetApi returns;
 * this class owns the library that table lives in and is the only thing that
 * calls through it. Lifecycle ORDER is decided by the ModuleManager; what each
 * transition means for the host -- minting and revoking the grant, opening and
 * closing the module's entry gate, withdrawing what the host holds for it --
 * is decided here, once.
 *
 * Activation needs a signed manifest. Every module the host loads comes from a
 * verified package, and a module with no manifest has nothing to derive a
 * grant from.
 */
class GF_CORE_EXPORT Module {
 public:
  /**
   * @brief Adopt a loaded library, bootstrapping it through GFModuleGetApi.
   *
   * Call IsGood() afterwards.
   *
   * @param module_library the loaded library; owned from here on
   * @param module_hash checksum of the bytes that were verified before load
   */
  Module(std::unique_ptr<QLibrary> module_library, QString module_hash);

  /**
   * @brief Adopt a module table that is not in a library of its own.
   *
   * For tests: the same validation as a library's table, with no image to
   * unload. @p api must outlive this object.
   */
  Module(const GFModuleApi* api, QString module_hash);

  ~Module();

  Module(const Module&) = delete;
  auto operator=(const Module&) -> Module& = delete;

  /// Whether the module's table was accepted.
  [[nodiscard]] auto IsGood() const -> bool;

  /**
   * @brief Mint the grant, open the entry gate, and call the module's
   *        activate().
   *
   * On failure the module is left exactly as before: the gate closed,
   * whatever it registered withdrawn, the grant revoked.
   *
   * @return 0 on success
   */
  auto Active() -> int;

  /// Deliver one event. The caller has already passed the entry gates.
  auto Exec(const EventReference& event) -> int;

  /**
   * @brief Close the entry gate, withdraw what the host holds for the
   *        module, wait for calls already inside it, then call its
   *        deactivate().
   *
   * Cannot be refused: whatever the module returns, it is inactive after
   * this. If calls already inside it do not finish in time, its deactivate()
   * is NOT called -- it would free state those calls are still using -- and
   * the module stays inert behind its closed gate.
   *
   * @param revoke also revoke the grant. False only at shutdown, where the
   *        module's final unregister hook still runs after this.
   * @return the module's own result, or -1 on a drain timeout
   */
  auto Deactivate(bool revoke) -> int;

  /// Final teardown hook. No module code runs after it.
  auto UnRegister() -> int;

  /// Revoke the grant, if this instance holds one. Idempotent.
  void ReleaseGrant();

  /**
   * @brief Drop the module's code and unmap its library.
   *
   * Only valid once nothing can call into the module: the function table
   * lives inside the image. The module is inert afterwards.
   *
   * @return true if a library was unloaded
   */
  auto UnloadLibrary() -> bool;

  [[nodiscard]] auto GetModuleIdentifier() const -> ModuleIdentifier;

  [[nodiscard]] auto GetModuleVersion() const -> ModuleVersion;

  /// Name / Description / Author, from the signed manifest.
  [[nodiscard]] auto GetModuleMetaData() const -> ModuleMetaData;

  void SetModuleMetaData(const ModuleMetaData& meta_data);

  /// The `*.gfmodule` this was verified from, or the library path.
  [[nodiscard]] auto GetModulePath() const -> QString;

  void SetSourcePackagePath(const QString& path);

  /// Whether a signed manifest was recorded for this module.
  [[nodiscard]] auto IsPackaged() const -> bool;

  [[nodiscard]] auto GetModuleManifest() const -> std::optional<ModuleManifest>;

  void SetModuleManifest(const ModuleManifest& manifest);

  /// The module's OWN ABI generation, as its table reported it.
  [[nodiscard]] auto GetModuleSDKABIVersion() const -> int;

  /// Digest of the verified bytes. Used to notice a changed module.
  [[nodiscard]] auto GetModuleHash() const -> QString;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

}  // namespace GpgFrontend::Module
