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

#ifndef GPGFRONTEND_FILEOPERATOR_H
#define GPGFRONTEND_FILEOPERATOR_H

#include "core/GpgFrontendCore.h"

namespace GpgFrontend {

/**
 * @brief provides file operations
 *
 */
class GPGFRONTEND_CORE_EXPORT FileOperator {
 public:
  /**
   * @brief read file content using std struct
   *
   *
   * @param file_name file name
   * @param data data read from file
   * @return
   */
  static bool ReadFileStd(const std::filesystem::path &file_name,
                          std::string &data);

  /**
   * @brief write file content using std struct
   *
   * @param file_name file name
   * @param data data to write to file
   * @return
   */
  static bool WriteFileStd(const std::filesystem::path &file_name,
                           const std::string &data);

  /**
   * @brief read file content
   *
   * @param file_name file name
   * @param data data read from file
   * @return true if success
   * @return false if failed
   */
  static bool ReadFile(const QString &file_name, QByteArray &data);

  /**
   * @brief write file content
   *
   * @param file_name file name
   * @param data data to write to file
   * @return true if success
   * @return false if failed
   */
  static bool WriteFile(const QString &file_name, const QByteArray &data);

  /**
   * calculate the hash of a file
   * @param file_path
   * @return
   */
  static std::string CalculateHash(const std::filesystem::path &file_path);
};
}  // namespace GpgFrontend

#endif  // GPGFRONTEND_FILEOPERATOR_H
