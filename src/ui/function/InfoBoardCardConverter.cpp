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

#include "ui/function/InfoBoardCardConverter.h"

namespace GpgFrontend::UI {

namespace {

auto sig_validity_to_status(GpgSigValidity validity) -> InfoBoardStatus {
  switch (validity) {
    case GpgSigValidity::kFULLY_VALID:
      return kINFO_ERROR_OK;
    case GpgSigValidity::kVALID_NOT_FULLY_TRUSTED:
    case GpgSigValidity::kVALID_WITH_ISSUES:
    case GpgSigValidity::kSIG_EXPIRED:
    case GpgSigValidity::kKEY_EXPIRED:
      return kINFO_ERROR_WARN;
    default:
      return kINFO_ERROR_CRITICAL;
  }
}

auto sig_validity_to_text(GpgSigValidity validity) -> QString {
  switch (validity) {
    case GpgSigValidity::kFULLY_VALID:
      return QObject::tr("Fully Valid");
    case GpgSigValidity::kVALID_WITH_ISSUES:
      return QObject::tr("Valid (with Issues)");
    case GpgSigValidity::kVALID_NOT_FULLY_TRUSTED:
      return QObject::tr("Valid (Not Fully Trusted)");
    case GpgSigValidity::kINVALID:
      return QObject::tr("Invalid");
    case GpgSigValidity::kKEY_MISSING:
      return QObject::tr("Key Missing");
    case GpgSigValidity::kKEY_REVOKED:
      return QObject::tr("Key Revoked");
    case GpgSigValidity::kSIG_EXPIRED:
      return QObject::tr("Signature Expired");
    case GpgSigValidity::kKEY_EXPIRED:
      return QObject::tr("Key Expired");
    case GpgSigValidity::kERROR:
      return QObject::tr("Verification Error");
    default:
      return QObject::tr("Unknown");
  }
}

void add_signer_fields(InfoBoardCard& card, const GpgSignerInfo& signer) {
  if (!signer.uid.isEmpty()) {
    card.fields.append({QObject::tr("Signer"), signer.uid});
  } else if (!signer.fingerprint.isEmpty()) {
    card.fields.append({QObject::tr("Fingerprint"), signer.fingerprint});
  }
  card.fields.append({QObject::tr("Key ID"), signer.keyId});
  if (!signer.pubkeyAlgo.isEmpty() || !signer.hashAlgo.isEmpty()) {
    QString algo = signer.pubkeyAlgo;
    if (!signer.hashAlgo.isEmpty()) {
      algo += QStringLiteral(" / ") + signer.hashAlgo;
    }
    card.fields.append({QObject::tr("Algorithm"), algo});
  }
  if (signer.signTime.isValid()) {
    card.fields.append({QObject::tr("Signed"),
                        QLocale().toString(signer.signTime.toLocalTime(),
                                           QLocale::ShortFormat)});
  }
}

void add_recipient_fields(InfoBoardCard& card, const GpgRecipientInfo& reci) {
  if (!reci.uid.isEmpty()) {
    card.fields.append({QObject::tr("Recipient"), reci.uid});
  } else if (!reci.fingerprint.isEmpty()) {
    card.fields.append({QObject::tr("Fingerprint"), reci.fingerprint});
  }
  card.fields.append({QObject::tr("Key ID"), reci.keyId});
  card.fields.append({reci.algoIsPrimaryKey
                          ? QObject::tr("Primary Key Algorithm")
                          : QObject::tr("Algorithm"),
                      reci.pubkeyAlgo});
}

}  // namespace

auto convert_op_info_to_cards(const GpgOpResultInfo& info)
    -> QContainer<InfoBoardCard> {
  QContainer<InfoBoardCard> cards;

  if (!info.inputHash.isEmpty()) {
    InfoBoardCard hash_card;
    hash_card.title = QObject::tr("Input Material Hash");
    hash_card.status = kINFO_ERROR_NEUTRAL;
    hash_card.fields.append({QObject::tr("SHA-256"), info.inputHash});
    cards.append(hash_card);
  }

  for (const auto& sig : info.signatures) {
    InfoBoardCard sig_card;
    sig_card.title = sig_validity_to_text(sig.validity);
    sig_card.status = sig_validity_to_status(sig.validity);
    add_signer_fields(sig_card, sig.signer);
    for (const auto& w : sig.warnings) {
      sig_card.fields.append({QStringLiteral("⚠"), w});
    }
    cards.append(sig_card);
  }

  for (const auto& ns : info.newSignatures) {
    InfoBoardCard ns_card;
    ns_card.title = QObject::tr("Signature Created");
    ns_card.status = kINFO_ERROR_OK;
    add_signer_fields(ns_card, ns.signer);
    ns_card.fields.append({QObject::tr("Mode"), ns.sigMode});
    cards.append(ns_card);
  }

  for (const auto& inv : info.invalidSigners) {
    InfoBoardCard inv_card;
    inv_card.title = QObject::tr("Invalid Signer");
    inv_card.status = kINFO_ERROR_WARN;
    inv_card.fields.append({QObject::tr("Fingerprint"), inv.first});
    inv_card.fields.append({QObject::tr("Reason"), inv.second});
    cards.append(inv_card);
  }

  const bool is_decrypt_with_data =
      info.newSignatures.isEmpty() && info.invalidSigners.isEmpty() &&
      (!info.recipients.isEmpty() || !info.details.isEmpty()) &&
      info.operation.contains(QObject::tr("Decrypt"));
  if (is_decrypt_with_data) {
    InfoBoardCard meta_card;
    meta_card.title = QObject::tr("Message Metadata");
    meta_card.status =
        info.messageIntegrityProtected ? kINFO_ERROR_OK : kINFO_ERROR_WARN;
    meta_card.fields.append({QObject::tr("File"), info.filename});
    meta_card.fields.append({QObject::tr("Cipher"), info.symmetricAlgo});
    meta_card.fields.append({QObject::tr("MIME"), info.mimeEncoded
                                                      ? QObject::tr("Yes")
                                                      : QObject::tr("No")});
    meta_card.fields.append({QObject::tr("Integrity"),
                             info.messageIntegrityProtected
                                 ? QObject::tr("Protected")
                                 : QObject::tr("Not Protected (unsafe)")});
    cards.append(meta_card);
  }

  if (info.operation.contains(QObject::tr("Encrypt"))) {
    for (const auto& reci : info.recipients) {
      InfoBoardCard reci_card;
      reci_card.title = QObject::tr("Encryption Recipient");
      reci_card.status = kINFO_ERROR_OK;
      add_recipient_fields(reci_card, reci);
      cards.append(reci_card);
    }
  }

  if (info.operation.contains(QObject::tr("Decrypt"))) {
    for (const auto& reci : info.recipients) {
      InfoBoardCard reci_card;
      reci_card.title = QObject::tr("Decryption Recipient");
      reci_card.status = reci.keyFound ? kINFO_ERROR_OK : kINFO_ERROR_WARN;
      add_recipient_fields(reci_card, reci);
      cards.append(reci_card);
    }
  }

  return cards;
}

}  // namespace GpgFrontend::UI
