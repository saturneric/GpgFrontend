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

#include "core/GpgFrontendCore.h"

namespace GpgFrontend {

class GPGFRONTEND_CORE_EXPORT CoreCommonUtil : public QObject {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new Core Common Util object
   *
   */
  static CoreCommonUtil *GetInstance();

  /**
   * @brief
   *
   */
  CoreCommonUtil() = default;

  /**
   * @brief set a temp cache under a certain key
   *
   */
  void SetTempCacheValue(const std::string &, const std::string &);

  /**
   * @brief after get the temp cache, its value will be imediately ease in
   * storage
   *
   * @return std::string
   */
  std::string GetTempCacheValue(const std::string &);

  /**
   * @brief imediately ease temp cache in storage
   *
   * @return std::string
   */
  void ResetTempCacheValue(const std::string &);

 signals:

  /**
   * @brief
   *
   */
  void SignalGnupgNotInstall();

 private:
  static std::unique_ptr<CoreCommonUtil> instance_;  ///<
  std::map<std::string, std::string> temp_cache_;    //<
};

}  // namespace GpgFrontend
