/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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
#include "core/GpgConstants.h"

namespace GpgFrontend {

/**
 * @brief
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgDecryptResultAnalyse
    : public GpgResultAnalyse {
 public:
  /**
   * @brief Construct a new Decrypt Result Analyse object
   *
   * @param m_error
   * @param m_result
   */
  explicit GpgDecryptResultAnalyse(GpgError m_error, GpgDecrResult m_result);

 protected:
  /**
   * @brief
   *
   */
  void do_analyse() final;

 private:
  /**
   * @brief
   *
   * @param stream
   * @param recipient
   */
  void print_recipient(std::stringstream &stream, gpgme_recipient_t recipient);

  GpgError error_;        ///<
  GpgDecrResult result_;  ///<
};

}  // namespace GpgFrontend
