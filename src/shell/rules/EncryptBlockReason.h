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

#include "GFShellExport.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend::Shell {

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
  bool disabled = false;
  /// A key group with nobody in it: it would encrypt to nobody.
  bool empty_group = false;
  QDateTime expires;
  QList<EncryptSubkeyFacts> subkeys;  ///< includes the primary
};

/**
 * @brief Why this recipient cannot be encrypted to, in one sentence naming
 * the key and, for an expiry, the date. Empty when it can be.
 *
 * @param facts the key
 * @return QString
 */
auto GF_SHELL_EXPORT DescribeEncryptBlock(const EncryptKeyFacts& facts)
    -> QString;

/**
 * @brief The same reason in a word or two, for a note under the key's name
 * in a list ("Expired", "Has no keys"). Empty when it can be encrypted to.
 *
 * @param facts the key
 * @return QString
 */
auto GF_SHELL_EXPORT EncryptBlockNote(const EncryptKeyFacts& facts) -> QString;

/**
 * @brief Read the facts off a key or key group. A key group counts as usable
 * while it has members: they are resolved later.
 *
 * @param key a key, subkey or key group; nullptr gives empty facts
 * @return EncryptKeyFacts
 */
auto GF_SHELL_EXPORT CollectEncryptKeyFacts(const GpgAbstractKey* key)
    -> EncryptKeyFacts;

/**
 * @brief CollectEncryptKeyFacts() for a shared key.
 */
auto GF_SHELL_EXPORT CollectEncryptKeyFacts(const GpgAbstractKeyPtr& key)
    -> EncryptKeyFacts;

/**
 * @brief DescribeEncryptBlock(CollectEncryptKeyFacts(key)).
 *
 * @param key a checked entry
 * @return QString
 */
auto GF_SHELL_EXPORT DescribeEncryptBlock(const GpgAbstractKeyPtr& key)
    -> QString;

/**
 * @brief What decides whether a key can sign.
 */
struct SignKeyFacts {
  QString label;        ///< "Name <email>"
  bool usable = false;  ///< the key can sign right now, here
  bool secret = false;  ///< its secret key is here
  bool revoked = false;
  bool expired = false;
  bool disabled = false;
  bool group = false;  ///< a key group, which never signs
  QDateTime expires;
};

/**
 * @brief Read the facts off a key or key group.
 *
 * @param key a key; nullptr gives empty facts
 * @return SignKeyFacts
 */
auto GF_SHELL_EXPORT CollectSignKeyFacts(const GpgAbstractKey* key)
    -> SignKeyFacts;

/**
 * @brief Why this key cannot sign, in one sentence naming it. Empty when it
 * can.
 *
 * @param facts the key
 * @return QString
 */
auto GF_SHELL_EXPORT DescribeSignBlock(const SignKeyFacts& facts) -> QString;

/**
 * @brief The same reason in a word or two, for a note in a list. Empty when
 * it can sign.
 *
 * @param facts the key
 * @return QString
 */
auto GF_SHELL_EXPORT SignBlockNote(const SignKeyFacts& facts) -> QString;

}  // namespace GpgFrontend::Shell
