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

#include <sstream>

#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

class GPGFRONTEND_CORE_EXPORT GpgResultAnalyse : public QObject {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Result Analyse object
   *
   */
  GpgResultAnalyse() = default;

  /**
   * @brief Get the Result Report object
   *
   * @return const QString
   */
  [[nodiscard]] auto GetResultReport() const -> const QString;

  /**
   * @brief Get the Status object
   *
   * @return int
   */
  [[nodiscard]] auto GetStatus() const -> int;

  /**
   * @brief
   *
   */
  void Analyse();

 protected:
  /**
   * @brief
   *
   */
  virtual void doAnalyse() = 0;

  /**
   * @brief Set the status object
   *
   * @param m_status
   */
  void setStatus(int m_status);

  QString buffer_;
  QTextStream stream_ = QTextStream(&buffer_);  ///<
  int status_ = 1;                              ///<
  bool analysed_ = false;                       ///<
};

}  // namespace GpgFrontend
