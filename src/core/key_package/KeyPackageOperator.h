/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef GPGFRONTEND_KEYPACKAGEOPERATOR_H
#define GPGFRONTEND_KEYPACKAGEOPERATOR_H

#include "core/GpgFrontendCore.h"
#include "core/function/GpgKeyImportExporter.h"

namespace GpgFrontend {

/**
 * @brief give the possibility to import or export a key package
 *
 */
class KeyPackageOperator {
public:
  /**
   * @brief generate passphrase for key package and save it to file
   *
   * @param phrase_path path to passphrase file
   * @param phrase passphrase generated
   * @return true if passphrase was generated and saved
   * @return false if passphrase was not generated and saved
   */
  static bool GeneratePassphrase(const std::filesystem::path &phrase_path,
                                 std::string &phrase);

  /**
   * @brief generate the name of the key package
   *
   * @return std::string name of the key package
   */
  static std::string GenerateKeyPackageName();

  /**
   * @brief generate key package
   *
   * @param key_package_path path to key package
   * @param key_package_name name of the key package
   * @param key_ids key ids to export
   * @param phrase passphrase to encrypt key package
   * @param secret true if secret key should be exported
   * @return true if key package was generated
   * @return false if key package was not generated
   */
  static bool GenerateKeyPackage(const std::filesystem::path &key_package_path,
                                 const std::string &key_package_name,
                                 KeyIdArgsListPtr &key_ids, std::string &phrase,
                                 bool secret);

  /**
   * @brief import key package
   *
   * @param key_package_path path to key package
   * @param phrase_path path to passphrase file
   * @param import_info import info
   * @return true if key package was imported
   * @return false if key package was not imported
   */
  static bool ImportKeyPackage(const std::filesystem::path &key_package_path,
                               const std::filesystem::path &phrase_path,
                               GpgFrontend::GpgImportInformation &import_info);

private:
  /**
   * @brief genearte passphrase
   *
   * @param len length of the passphrase
   * @return std::string passphrase
   */
  static std::string generate_passphrase(const int len) {
    std::random_device rd_;         ///< Random device
    auto mt_ = std::mt19937(rd_()); ///< Mersenne twister

    std::uniform_int_distribution<int> dist(999, 99999);
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_str;
    tmp_str.reserve(len);

    for (int i = 0; i < len; ++i) {
      tmp_str += alphanum[dist(mt_) % (sizeof(alphanum) - 1)];
    }
    return tmp_str;
  }

  /**
   * @brief generate key package name
   *
   * @return std::string key package name
   */
  static std::string generate_key_package_name() {
    std::random_device rd_;         ///< Random device
    auto mt_ = std::mt19937(rd_()); ///< Mersenne twister

    std::uniform_int_distribution<int> dist(999, 99999);
    auto file_string = boost::format("KeyPackage_%1%") % dist(mt_);
    return file_string.str();
  }
};
} // namespace GpgFrontend

#endif // GPGFRONTEND_KEYPACKAGEOPERATOR_H
