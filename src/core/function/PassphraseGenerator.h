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

#ifndef GPGFRONTEND_PASSPHRASEGENERATOR_H
#define GPGFRONTEND_PASSPHRASEGENERATOR_H

#include "core/GpgFrontendCore.h"
#include "core/GpgFunctionObject.h"

namespace GpgFrontend {

/**
 * @brief The PassphraseGenerator class
 *
 * This class is used to generate a passphrase.
 */
class PassphraseGenerator
    : public SingletonFunctionObject<PassphraseGenerator> {
 public:
  /**
   * @brief PassphraseGenerator constructor
   *
   * @param channel The channel to use
   */
  explicit PassphraseGenerator(
      int channel = SingletonFunctionObject::GetDefaultChannel())
      : SingletonFunctionObject<PassphraseGenerator>(channel) {}

  /**
   * @brief generate passphrase
   *
   * @param len length of the passphrase
   * @return std::string passphrase
   */
  std::string Generate(int len) {
    std::uniform_int_distribution<int> dist(999, 99999);

    auto file_string = boost::format("KeyPackage_%1%") % dist(mt_);
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_str;
    tmp_str.reserve(len);

    for (int i = 0; i < len; ++i) {
      tmp_str += alphanum[dist(mt_) % (sizeof(alphanum) - 1)];
    }
    return tmp_str;
  }

  std::random_device rd_;                  ///< Random device
  std::mt19937 mt_ = std::mt19937(rd_());  ///< Mersenne twister
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_PASSPHRASEGENERATOR_H
