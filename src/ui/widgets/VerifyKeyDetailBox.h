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

#include "core/model/GpgSignature.h"

namespace GpgFrontend::UI {
/**
 * @brief
 *
 */
class VerifyKeyDetailBox : public QGroupBox {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Verify Key Detail Box object
   *
   * @param signature
   * @param parent
   */
  explicit VerifyKeyDetailBox(const GpgSignature& signature, QWidget* parent);

 private slots:

  /**
   * @brief
   *
   */
  void slot_import_form_key_server();

 private:
  /**
   * @brief Create a key info grid object
   *
   * @param signature
   * @return QGridLayout*
   */
  QGridLayout* create_key_info_grid(const GpgSignature& signature);

  QString fpr_;  ///< fingerprint of the key
};

}  // namespace GpgFrontend::UI
