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

#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend::UI {

/**
 * @brief Drive the "generate a revocation certificate" flow for one key.
 *
 * Asks the user for a revocation reason, then for a destination file, and
 * writes the certificate. Reused by the KeyPair operations tab and the Key
 * Management window so the flow lives in exactly one place.
 */
class GenerateRevocationCert : public QWidget {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Generate Revocation Cert object.
   *
   * @param parent parent widget used for the dialogs
   */
  explicit GenerateRevocationCert(QWidget* parent);

  /**
   * @brief Run the interactive revocation-certificate generation for @p key.
   *
   * @param channel gpg context channel
   * @param key primary key to generate the certificate for
   */
  void Exec(int channel, const GpgKeyPtr& key);
};

}  // namespace GpgFrontend::UI
