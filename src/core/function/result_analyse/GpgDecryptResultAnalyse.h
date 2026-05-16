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
#include "core/model/GpgDecryptResult.h"

namespace GpgFrontend {

/**
 * @brief Analyses a GpgDecryptResult and formats a human-readable decryption
 * report.
 *
 * The generated report lists the overall success or failure, the symmetric
 * encryption algorithm, message integrity protection status, and details for
 * each recipient (key ID, name, public-key algorithm, and status).
 */
class GF_CORE_EXPORT GpgDecryptResultAnalyse : public GpgResultAnalyse {
  Q_OBJECT
 public:
  /**
   * @brief Construct the analyser with the decryption result to examine.
   *
   * @param channel OpenPGP context channel
   * @param m_error gpgme error code returned by the decrypt operation
   * @param m_result decryption result object containing recipients and metadata
   */
  explicit GpgDecryptResultAnalyse(int channel, GpgError m_error,
                                   GpgDecryptResult m_result);

 protected:
  /**
   * @brief Write the formatted decryption report to stream_.
   *
   * Reports success or failure, lists general state (filename, MIME flag,
   * message integrity protection, symmetric algorithm), and for each recipient
   * prints the key identity, key ID, public-key algorithm, and status.
   */
  void doAnalyse() final;

 private:
  /**
   * @brief Write a single recipient entry to @p stream.
   *
   * Looks up the recipient key via AbstractKeyRepository. If the key is found,
   * prints the name and email; otherwise prints "<unknown>" and lowers the
   * status to 0.
   *
   * @param stream output stream to write into
   * @param recipient recipient record from the decrypt result
   */
  void print_recipient(QTextStream& stream, const GpgRecipient& recipient);

  // gpgme error code from the decrypt operation
  GpgError error_;
  // Decryption result containing recipient and metadata info
  GpgDecryptResult result_;
};

}  // namespace GpgFrontend
