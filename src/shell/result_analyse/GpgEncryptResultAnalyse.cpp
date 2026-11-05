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

#include "GpgEncryptResultAnalyse.h"

#include <QCoreApplication>

#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgKey.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgEncryptResultAnalyse::GpgEncryptResultAnalyse(int channel,
                                                 const GFError& error,
                                                 GpgEncryptResult result)
    : GpgResultAnalyse(channel), error_(error), result_(result) {}

void GpgEncryptResultAnalyse::doAnalyse() {
  setOperation(kOperation);
  recordEngineError(error_, result_.ErrorDetail());

  stream_ << "# " << tr("Encrypt Operation") << " (" << EngineInfo() << ") ";

  if (error_.Ok()) {
    stream_ << "- " << tr("Success") << " " << Qt::endl;
  } else {
    stream_ << "- " << tr("Failed") << ": " << DescribeGFError(error_).second
            << Qt::endl;
    if (!result_.ErrorDetail().isEmpty()) {
      stream_ << "    - " << tr("Detail") << ": " << result_.ErrorDetail()
              << Qt::endl;
    }
    setStatus(-1);
  }

  // Read the engine-agnostic list: rPGP results carry no gpgme handle, so
  // GetRaw() alone would drop every recipient the rPGP engine rejected.
  const auto invalid_recipients = result_.InvalidRecipients();

  // Data encrypted to fewer recipients than asked for is a warning, not a
  // clean success.
  if (status_ > 0 && !invalid_recipients.empty()) setStatus(0);

  if (status_ <= 0) {
    stream_ << Qt::endl;

    if (!invalid_recipients.empty()) {
      stream_ << "## " << tr("Invalid Recipients") << ": " << Qt::endl
              << Qt::endl;

      auto index = 0;
      for (const auto& [fpr, reason] : invalid_recipients) {
        stream_ << "### " << tr("Recipients") << " " << ++index << ": "
                << Qt::endl;
        stream_ << "- " << tr("Fingerprint") << ": " << fpr << Qt::endl;
        stream_ << "- " << tr("Reason") << ": "
                << DescribeGFError(reason).second << Qt::endl;
        stream_ << Qt::endl << Qt::endl;
      }
    }
    stream_ << Qt::endl;
  }

  stream_ << Qt::endl;

  if (status_ >= 0) {
    op_info_.description =
        !invalid_recipients.empty()
            ? tr("Encrypted with warnings. Some recipients could not be "
                 "added. Please review the invalid recipient list.")
            : tr("Your data has been encrypted. Only the intended recipients "
                 "can decrypt and read it.");
  } else {
    const auto detail = result_.ErrorDetail();
    op_info_.description =
        tr("Encryption failed: %1.")
            .arg(detail.isEmpty() ? DescribeGFError(error_).second : detail);
  }
}

void AttachEncryptRecipients(GpgOpResultInfo& info, GpgEncryptResult result,
                             const GpgAbstractKeyPtrList& keys) {
  // No keys: encrypted to a passphrase, not to recipients. The analyser
  // cannot tell (a GnuPG result names no recipients either), so the summary
  // it wrote is put right here.
  if (keys.isEmpty()) {
    if (info.status > 0) {
      info.description = QCoreApplication::translate(
          "GpgFrontend::GpgEncryptResultAnalyse",
          "Your data has been encrypted with a passphrase. Anyone who has "
          "the passphrase can decrypt and read it.");
    }
    return;
  }

  // Resolve an encryption (sub)key ID back to a recipient UID and the matching
  // (sub)key fingerprint among the selected recipient keys.
  auto resolve = [&](const QString& key_id, QString& uid, QString& fpr) {
    for (const auto& key : keys) {
      auto gpg_key = qSharedPointerDynamicCast<GpgKey>(key);
      if (gpg_key == nullptr) continue;
      for (const auto& sub : gpg_key->SubKeys()) {
        if (sub.ID().compare(key_id, Qt::CaseInsensitive) == 0) {
          uid = gpg_key->UID();
          fpr = sub.Fingerprint();
          return;
        }
      }
      if (gpg_key->ID().compare(key_id, Qt::CaseInsensitive) == 0) {
        uid = gpg_key->UID();
        fpr = gpg_key->Fingerprint();
        return;
      }
    }
  };

  // The rPGP engine reports the subkeys it actually encrypted to; show them
  // verbatim so the key ID and algorithm match the produced ciphertext.
  const auto engine_recipients = result.Recipients();
  if (!engine_recipients.isEmpty()) {
    for (const auto& er : engine_recipients) {
      GpgRecipientInfo ri;
      ri.keyId = er.keyid;
      ri.pubkeyAlgo = er.pubkey_algo;
      ri.keyFound = true;
      resolve(er.keyid, ri.uid, ri.fingerprint);
      info.recipients.append(ri);
    }
    return;
  }

  // The GnuPG engine does not report which subkey was used, so we cannot show
  // the real encryption-subkey algorithm. Fall back to the primary key and mark
  // the algorithm so the UI does not present it as the subkey actually used.
  for (const auto& key : keys) {
    if (key == nullptr) continue;
    GpgRecipientInfo ri;
    ri.uid = key->UID();
    ri.fingerprint = key->Fingerprint();
    ri.keyId = key->ID();
    ri.pubkeyAlgo = key->PublicKeyAlgo();
    ri.keyFound = true;
    ri.algoIsPrimaryKey = qSharedPointerDynamicCast<GpgKey>(key) != nullptr;
    info.recipients.append(ri);
  }
}

}  // namespace GpgFrontend