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

#include "GpgResultAnalyse.h"
#include "core/model/GpgEncryptResult.h"

namespace GpgFrontend {

/**
 * @brief Analyses a GpgEncryptResult and formats a human-readable encryption
 * report.
 *
 * The generated report shows overall success or failure and, on failure,
 * lists any invalid recipients together with their fingerprints and error
 * reasons.
 */
class GF_CORE_EXPORT GpgEncryptResultAnalyse : public GpgResultAnalyse {
  Q_OBJECT
 public:
  /**
   * @brief Construct the analyser with the encryption result to examine.
   *
   * @param channel OpenPGP context channel
   * @param error gpgme error code returned by the encrypt operation
   * @param result encryption result object containing invalid-recipient
   * information
   */
  explicit GpgEncryptResultAnalyse(int channel, GpgError error,
                                   GpgEncryptResult result);

  /// The underlying encryption result. Used by UI consumers to read the actual
  /// recipient subkeys reported by the engine (rPGP).
  [[nodiscard]] auto GetResult() const -> GpgEncryptResult { return result_; }

 protected:
  /**
   * @brief Write the formatted encryption report to stream_.
   *
   * Reports success or failure. On failure, lists each invalid recipient
   * with its fingerprint and the reason the encryption was rejected.
   */
  void doAnalyse() final;

 private:
  // gpgme error code from the encrypt operation
  GpgError error_;
  // Encryption result containing invalid-recipient details
  GpgEncryptResult result_;
};
}  // namespace GpgFrontend
