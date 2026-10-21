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
 * @brief What decides whether one subkey can take an encryption.
 */
struct EncryptSubkeyFacts {
  bool can_encrypt = false;  ///< the raw capability flag
  bool revoked = false;
  bool disabled = false;
  bool expired = false;
  QDateTime expires;  ///< invalid or epoch 0 when it never expires
};

/**
 * @brief Everything the explanation below is allowed to look at.
 *
 * No gpgme and no rPGP: the wording that tells a user why Encrypt is greyed
 * out is worth testing for every case, including an expired subkey under a
 * still-valid primary, which no fixture key happens to have.
 */
struct EncryptKeyFacts {
  QString label;        ///< "Name <email>", as the user knows the key
  bool usable = false;  ///< the key can encrypt right now
  bool revoked = false;
  bool expired = false;
  QDateTime expires;
  QContainer<EncryptSubkeyFacts> subkeys;  ///< includes the primary
};

/**
 * @brief Why this recipient cannot be encrypted to, in one sentence naming
 * the key and, for an expiry, the date. Empty when it can be.
 *
 * @param facts the key
 * @return QString
 */
auto GF_UI_EXPORT DescribeEncryptBlock(const EncryptKeyFacts& facts) -> QString;

/**
 * @brief Read the facts off a checked entry. A key group always counts as
 * usable: its members are resolved later.
 *
 * @param key a checked entry
 * @return EncryptKeyFacts
 */
auto GF_UI_EXPORT CollectEncryptKeyFacts(const GpgAbstractKeyPtr& key)
    -> EncryptKeyFacts;

/**
 * @brief DescribeEncryptBlock(CollectEncryptKeyFacts(key)).
 *
 * @param key a checked entry
 * @return QString
 */
auto GF_UI_EXPORT DescribeEncryptBlock(const GpgAbstractKeyPtr& key) -> QString;

}  // namespace GpgFrontend::UI
