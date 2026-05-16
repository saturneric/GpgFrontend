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

#include <gpgme.h>

#include <cstdint>

#include "core/function/basic/GpgFunctionObject.h"
#include "core/typedef/GFTypedef.h"

namespace GpgFrontend {

class GFKeyDatabase;

/**
 * @brief Initialisation arguments for an OpenPGP context.
 *
 * Passed to the OpenPGPContext constructor to select the engine, the key
 * database to use, and optional behavioural flags.
 */
struct OpenPGPContextInitArgs {
  // OpenPGP engine to use (e.g. GnuPG or rPGP)
  OpenPGPEngine engine;
  // Logical name for the key database
  QString db_name;
  // Filesystem path to the key database directory
  QString db_path;
  // If true, run in test mode (relaxed constraints)
  bool test_mode = false;
  // If true, disable network access for key retrieval
  bool offline_mode = false;
  // If true, automatically import missing keys during verification
  bool auto_import_missing_key = false;
};

/**
 * @brief Singleton holding an initialised OpenPGP engine context for a channel.
 *
 * Each channel has exactly one OpenPGPContext. The context owns the engine
 * session, the key database handle, and the associated configuration.
 * Call Initialize() after construction; check Good() before use.
 */
class GF_CORE_EXPORT OpenPGPContext
    : public SingletonFunctionObject<OpenPGPContext> {
 public:
  /**
   * @brief Construct a context on the given channel with default initialisation
   * arguments.
   *
   * @param channel singleton channel identifier
   */
  explicit OpenPGPContext(int channel);

  /**
   * @brief Construct a context on the given channel with explicit
   * initialisation arguments.
   *
   * @param args engine, database, and flag configuration
   * @param channel singleton channel identifier
   */
  explicit OpenPGPContext(const OpenPGPContextInitArgs& args, int channel);

  ~OpenPGPContext();

  /**
   * @brief Initialise the engine context using the stored arguments.
   *
   * @return true if initialisation succeeded, false otherwise
   */
  auto Initialize() -> bool;

  /**
   * @brief Return whether the context was successfully initialised.
   *
   * @return true if Initialize() completed without error
   */
  [[nodiscard]] auto Good() const -> bool;

  /**
   * @brief Return the OpenPGP engine type this context uses.
   *
   * @return engine enum value (e.g. kGNUPG or kRPGP)
   */
  [[nodiscard]] auto Engine() const -> OpenPGPEngine;

  /**
   * @brief Return the version string of the active engine.
   *
   * @return engine version string (e.g. "2.4.3")
   */
  [[nodiscard]] auto EngineVersion() const -> QString;

  /**
   * @brief Return the logical name of the key database for this context.
   *
   * @return key database name string
   */
  [[nodiscard]] auto KeyDBName() const -> QString;

  /**
   * @brief Return the filesystem path of the key database directory.
   *
   * @return absolute path to the key database
   */
  [[nodiscard]] auto KeyDBPath() const -> QString;

  /**
   * @brief Return a shared pointer to the GFKeyDatabase for this context.
   *
   * @return shared pointer to the key database; never null after successful
   * initialisation
   */
  auto KeyDatabase() -> QSharedPointer<GFKeyDatabase>;

 protected:
  /**
   * @brief Engine-specific initialisation hook called by Initialize().
   *
   * Subclasses may override to perform additional setup after the base context
   * is prepared.
   *
   * @param args initialisation arguments
   * @return true on success, false on failure
   */
  virtual auto init(const OpenPGPContextInitArgs& args) -> bool;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
  // True after Initialize() completes successfully.
  bool good_ = false;
};

/**
 * @brief Construct a T (OpenPGPContext subclass) on @p channel and initialise
 * it.
 *
 * Allocates a T via SecureCreateUniqueObject, calls Initialize(), converts the
 * unique_ptr to a ChannelObjectPtr, and returns it for registration with the
 * singleton system.
 *
 * @tparam T concrete OpenPGPContext subclass to instantiate
 * @param channel singleton channel identifier
 * @param args initialisation arguments forwarded to T's constructor
 * @return ChannelObjectPtr owning the constructed and initialised T
 */
template <typename T>
auto BuildContext(int channel, OpenPGPContextInitArgs args)
    -> ChannelObjectPtr {
  auto o = SecureCreateUniqueObject<T>(args, channel);
  o->Initialize();

  auto c_o = ConvertToChannelObjectPtr<>(std::move(o));
  return c_o;
}
}  // namespace GpgFrontend
