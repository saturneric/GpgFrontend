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
#include "core/model/GpgVerifyResult.h"

namespace GpgFrontend {
/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgVerifyResultAnalyse : public GpgResultAnalyse {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Verify Result Analyse object
   *
   * @param error
   * @param result
   */
  explicit GpgVerifyResultAnalyse(int channel, GpgError error,
                                  GpgVerifyResult result);

  /**
   * @brief Get the Signatures object
   *
   * @return gpgme_signature_t
   */
  [[nodiscard]] auto GetSignatures() const -> gpgme_signature_t;

  /**
   * @brief
   *
   * @return GpgVerifyResult
   */
  auto TakeChargeOfResult() -> GpgVerifyResult;

  /**
   * @brief Get the Unknown Signatures object
   *
   * @return QStringList
   */
  [[nodiscard]] auto GetUnknownSignatures() const -> QStringList;

 protected:
  /**
   * @brief
   *
   */
  void doAnalyse() final;

 private:
  /**
   * @brief
   *
   * @param stream
   * @param sign
   * @return true
   * @return false
   */
  auto print_signer(QTextStream &stream, GpgSignature sign) -> bool;

  /**
   * @brief
   *
   * @param stream
   * @param sign
   * @return true
   * @return false
   */
  auto print_signer_without_key(QTextStream &stream, GpgSignature sign) -> bool;

  GpgError error_;          ///<
  GpgVerifyResult result_;  ///<
  QStringList unknown_signer_fpr_list_;
};

}  // namespace GpgFrontend
