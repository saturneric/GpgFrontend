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

#include <gtest/gtest.h>

#include "GpgFrontendTest.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/typedef/GFTypedef.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend::Test {

/// @brief Dedicated channels for the engine-independent test suite, kept
/// separate from the single-engine fixtures so each suite owns its own context.
const int kEngineTestGnupgChannel = 16;
const int kEngineTestRpgpChannel = 17;

/**
 * @brief Describes one OpenPGP engine the shared test body runs against.
 *
 * The whole point of this fixture is that the test bodies never name an engine;
 * they only ever touch GetParam().channel. Adding a new engine is a one-line
 * change to the INSTANTIATE_TEST_SUITE_P list.
 */
struct EngineTestParam {
  QString name;          ///< Human-readable + test-name suffix (e.g. "GnuPG").
  OpenPGPEngine engine;  ///< Engine to drive.
  int channel;           ///< Channel that hosts the engine context.
};

/**
 * @brief Value-parameterized fixture that runs the same crypto/key flows
 * against every supported OpenPGP engine.
 *
 * Switch engine at runtime with e.g. `--gtest_filter=*GnuPG*` or `*RPGP*`.
 */
class GpgCoreEngineTest : public ::testing::TestWithParam<EngineTestParam> {
 public:
  void SetUp() override;
  void TearDown() override;

 protected:
  /// @brief Channel of the engine under test.
  [[nodiscard]] auto Channel() const -> int { return GetParam().channel; }

  /// @brief Pick a fast primary-key algorithm the engine actually supports.
  [[nodiscard]] auto PickPrimaryAlgo() const -> KeyAlgo;

  /// @brief Pick an encryption-capable subkey algorithm the engine supports.
  [[nodiscard]] auto PickEncryptionSubAlgo() const -> KeyAlgo;

  /**
   * @brief Generate a full, ready-to-use key (signing primary + encryption
   * subkey) on the engine under test.
   *
   * @return The imported key, or nullptr on failure (the caller should assert).
   */
  [[nodiscard]] auto GenerateFullKey(const QString& uid_suffix) -> GpgKeyPtr;

  /// @brief Remove a key generated during a test from the engine's database.
  void DeleteKey(const GpgKeyPtr& key) const;

  /// @brief Stress iteration count, overridable via the GF_STRESS_ITER env var.
  [[nodiscard]] static auto StressIterations() -> int;
};

}  // namespace GpgFrontend::Test
