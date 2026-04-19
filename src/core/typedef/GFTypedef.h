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

#include "core/model/GFBuffer.h"
#include "core/typedef/CoreTypedef.h"
#include "core/typedef/GpgErrorTypedef.h"

namespace GpgFrontend {

enum class OpenPGPEngine : std::uint8_t {
  kGNUPG,
  kRPGP,
};

struct GFUserId {
  QString name;
  QString email;
  QString comment;
  bool is_primary = false;
  bool is_revoked = false;

  GFUserId() = default;

  explicit GFUserId(const QString& uid);

  GFUserId(const GFUserId&) = default;

  auto operator=(const GFUserId&) -> GFUserId& = default;

  GFUserId(QString name, QString email, QString comment, bool is_primary);

  [[nodiscard]] auto ToString() const -> QString;
};

struct GFSubKeyMetadata {
  QString fpr;
  QString key_id;
  int algo;
  unsigned key_length;

  qint64 created_at;
  bool has_secret;
  bool is_revoked;

  bool can_sign;
  bool can_encrypt;
  bool can_auth;
  bool can_certify;

  bool marked;
};

struct GFKeyMetadata {
  QString fpr;
  QString key_id;

  QContainer<GFUserId> user_ids;

  int algo;
  unsigned key_length;
  qint64 created_at;
  qint64 update_time;
  bool has_secret;
  bool is_revoked;

  bool can_sign;
  bool can_encrypt;
  bool can_auth;
  bool can_certify;

  QContainer<GFSubKeyMetadata> subkeys;
};

struct GFKeyBlocks {
  QString public_key;
  QString secret_key;
};

struct GFKey {
  GFKeyMetadata metadata;
  GFKeyBlocks blocks;
};

enum class GFSignatureStatus : uint8_t {
  kVALID = 0,
  kBAD_SIGNATURE = 1,
  kNO_KEY = 2,
  kUNKNOWN_ERROR = 3,
};

struct GFSignature {
  QString issuer_fpr;
  GFSignatureStatus status;
  uint32_t created_at;
  QString pub_algo;
  QString hash_algo;
  QString sig_type;
};

struct GFRecipient {
  QString key_id;
  QString pub_algo;
  GpgError status;
};

struct GFInvalidRecipient {
  QString fpr;
  GpgError reason;
};

struct GFEncryptResult {
  GFBuffer data;
  QContainer<GFInvalidRecipient> invalid_recipients;
};

struct GFDecryptResult {
  GFBuffer data;
  QString filename;
  QContainer<GFRecipient> recipients;
};

struct GFSignResult {
  GFBuffer data;
  QContainer<GFSignature> signatures;
};

struct GFVerifyResult {
  QByteArray data;
  QContainer<GFSignature> signatures;
  bool is_verified;
};

struct GFEncryptAndSignResult {
  GFBuffer data;
  GFSignResult sign_result;
  GFEncryptResult encrypt_result;
};

struct GFDecryptAndVerifyResult {
  GFBuffer data;
  GFDecryptResult decrypt_result;
  GFVerifyResult verify_result;
};

}  // namespace GpgFrontend