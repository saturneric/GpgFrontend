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

//
// Created by eric on 07.01.2023.
//

#ifndef GPGFRONTEND_GPGADVANCEDOPERATOR_H
#define GPGFRONTEND_GPGADVANCEDOPERATOR_H

#include "core/GpgConstants.h"
#include "core/GpgContext.h"
#include "core/GpgFunctionObject.h"

namespace GpgFrontend {

class GPGFRONTEND_CORE_EXPORT GpgAdvancedOperator
    : public SingletonFunctionObject<GpgAdvancedOperator> {
 public:
  /**
   * @brief Construct a new Basic Operator object
   *
   * @param channel Channel corresponding to the context
   */
  explicit GpgAdvancedOperator(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  bool ClearGpgPasswordCache();

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  bool ReloadGpgComponents();

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  bool RestartGpgComponents();

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  bool ResetConfigures();

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  bool StartGpgAgent();

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  bool StartDirmngr();

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  bool StartKeyBoxd();

 private:
  GpgContext& ctx_ = GpgContext::GetInstance(
      SingletonFunctionObject::GetChannel());  ///< Corresponding context
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGADVANCEDOPERATOR_H
