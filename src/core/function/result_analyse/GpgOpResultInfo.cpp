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

#include "GpgOpResultInfo.h"

#include "core/function/result_analyse/InfoBoardCard.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

auto GradeSignature(GpgSigValidity validity) -> GpgSigVerdict {
  switch (validity) {
    case GpgSigValidity::kFULLY_VALID:
      return GpgSigVerdict::kGOOD;
    case GpgSigValidity::kVALID_NOT_FULLY_TRUSTED:
    case GpgSigValidity::kVALID_WITH_ISSUES:
    case GpgSigValidity::kSIG_EXPIRED:
    case GpgSigValidity::kKEY_EXPIRED:
      return GpgSigVerdict::kWARNING;
    default:
      return GpgSigVerdict::kBAD;
  }
}

auto CombineGpgOperations(GpgOperation a, GpgOperation b) -> GpgOperation {
  if (a == b || b == GpgOperation::kNONE) return a;
  if (a == GpgOperation::kNONE) return b;

  const auto is_pair = [&](GpgOperation x, GpgOperation y) -> bool {
    return (a == x && b == y) || (a == y && b == x);
  };
  if (is_pair(GpgOperation::kENCRYPT, GpgOperation::kSIGN)) {
    return GpgOperation::kENCRYPT_SIGN;
  }
  if (is_pair(GpgOperation::kDECRYPT, GpgOperation::kVERIFY)) {
    return GpgOperation::kDECRYPT_VERIFY;
  }
  return GpgOperation::kNONE;
}

auto GpgOperationName(GpgOperation op) -> QString {
  // The contexts are the analysers' own: these names were always translated
  // there, and looking them up in place keeps every existing translation.
  const auto encrypt = [] {
    return QCoreApplication::translate("GpgFrontend::GpgEncryptResultAnalyse",
                                       "Encrypt");
  };
  const auto decrypt = [] {
    return QCoreApplication::translate("GpgFrontend::GpgDecryptResultAnalyse",
                                       "Decrypt");
  };
  const auto sign = [] {
    return QCoreApplication::translate("GpgFrontend::GpgSignResultAnalyse",
                                       "Sign");
  };
  const auto verify = [] {
    return QCoreApplication::translate("GpgFrontend::GpgVerifyResultAnalyse",
                                       "Verify");
  };

  switch (op) {
    case GpgOperation::kENCRYPT:
      return encrypt();
    case GpgOperation::kDECRYPT:
      return decrypt();
    case GpgOperation::kSIGN:
      return sign();
    case GpgOperation::kVERIFY:
      return verify();
    case GpgOperation::kENCRYPT_SIGN:
      return encrypt() + u" + "_s + sign();
    case GpgOperation::kDECRYPT_VERIFY:
      return decrypt() + u" + "_s + verify();
    case GpgOperation::kNONE:
    default:
      return {};
  }
}

void GpgOpResultInfo::Merge(const GpgOpResultInfo& other) {
  // Status: take the minimum (worst) status
  status = std::min(status, other.status);

  // The output's status: the worst of the stages that produced it.
  if (other.outputStatus.has_value()) {
    outputStatus = outputStatus.has_value()
                       ? std::min(*outputStatus, *other.outputStatus)
                       : other.outputStatus;
  }

  // Report: concatenate
  report += other.report;

  // Operation: the combined identity; the display name follows from it, and
  // falls back to joining the two names only when neither side has one.
  const auto combined_op = CombineGpgOperations(op, other.op);
  if (combined_op != GpgOperation::kNONE) {
    op = combined_op;
    operation = GpgOperationName(op);
  } else {
    op = GpgOperation::kNONE;
    if (operation.isEmpty()) {
      operation = other.operation;
    } else if (!other.operation.isEmpty()) {
      operation += u" + "_s + other.operation;
    }
  }

  // Errors: every one is kept, whichever stage it came from.
  stageErrors += other.stageErrors;

  // Description: concatenate with newline, skip empty
  if (description.isEmpty()) {
    description = other.description;
  } else if (!other.description.isEmpty()) {
    description += u"\n"_s + other.description;
  }

  // Engine: first non-empty wins
  if (engine.isEmpty()) engine = other.engine;

  // List fields: concatenate
  details += other.details;
  signatures += other.signatures;
  newSignatures += other.newSignatures;
  invalidSigners += other.invalidSigners;
  recipients += other.recipients;

  // Scalar fields: last non-empty wins
  if (!other.filename.isEmpty()) filename = other.filename;
  if (!other.symmetricAlgo.isEmpty()) symmetricAlgo = other.symmetricAlgo;

  // Boolean flags: OR (if either says true, result is true)
  mimeEncoded = mimeEncoded || other.mimeEncoded;
  messageIntegrityProtected =
      messageIntegrityProtected || other.messageIntegrityProtected;
}

void GpgOpResultInfo::CollapseSharedCallError(const GFError& call_error,
                                              GpgOperation combined) {
  if (call_error.Ok()) return;

  GpgOpStageError shared;
  shared.scope = combined;
  shared.error = call_error;

  QList<GpgOpStageError> kept;
  qsizetype first_shared = -1;
  for (const auto& e : stageErrors) {
    if (!e.FromEngine() || e.error != call_error) {
      kept.append(e);
      continue;
    }
    if (first_shared < 0) first_shared = kept.size();
    if (shared.codeText.isEmpty()) shared.codeText = e.codeText;
    for (const auto& detail : e.engineDetails) {
      if (!detail.second.isEmpty() && !shared.engineDetails.contains(detail)) {
        shared.engineDetails.append(detail);
      }
    }
  }

  if (first_shared < 0) return;
  kept.insert(first_shared, shared);
  stageErrors = kept;
}

auto MakeAbortedOpInfo(
    GpgOperation scope, const GFError& err, const QString& engine,
    const QList<QPair<GpgOperation, QString>>& engine_details)
    -> GpgOpResultInfo {
  GpgOpResultInfo info;
  info.status = -1;
  info.op = scope;
  info.operation = GpgOperationName(scope);
  info.engine = engine;

  GpgOpStageError e;
  e.scope = scope;
  e.engineDetails = engine_details;

  if (err.Aborted()) {
    // GpgFrontend's own stop of a task that failed before the engine
    // answered: the error is kept, but it is not the engine's message.
    e.error = err;
    e.codeText = QCoreApplication::translate(
        "GpgFrontend::UI::GpgOperaHelper",
        "The operation stopped before the engine returned a result.");
  } else if (!err.Ok()) {
    e.error = err;
    e.codeText = DescribeGFError(err).second;
  } else {
    // No code at all, and no usable result: nothing the engine said explains
    // it, so this is recorded as GpgFrontend's own finding.
    e.error = GFError::Application(GFErrorCode::kGENERAL);
    e.codeText =
        QCoreApplication::translate("GpgFrontend::UI::GpgOperaHelper",
                                    "The engine returned no usable result.");
  }
  info.stageErrors.append(e);

  // The headings keep the context they were always translated in; each is
  // spelled out in full, which is all lupdate can read.
  if (err.code == GFErrorCode::kNOT_SUPPORTED) {
    info.report =
        "# " + QCoreApplication::translate("GpgFrontend::UI::GpgOperaHelper",
                                           "Operation Not Supported");
  } else if (err.Canceled()) {
    info.report =
        "# " + QCoreApplication::translate("GpgFrontend::UI::GpgOperaHelper",
                                           "Operation Cancelled");
  } else {
    info.report =
        "# " + QCoreApplication::translate("GpgFrontend::UI::GpgOperaHelper",
                                           "Critical Error");
  }
  return info;
}

// ---- The cards a result is shown in (InfoBoardCard.h). Moved from the
// desktop's InfoBoardCardConverter, which the phone does not build; the
// strings keep their QObject context.

namespace {

auto sig_validity_to_status(GpgSigValidity validity) -> InfoBoardStatus {
  switch (GradeSignature(validity)) {
    case GpgSigVerdict::kGOOD:
      return kINFO_ERROR_OK;
    case GpgSigVerdict::kWARNING:
      return kINFO_ERROR_WARN;
    case GpgSigVerdict::kBAD:
      break;
  }
  return kINFO_ERROR_CRITICAL;
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
    case GpgSigValidity::kKEY_NOT_AUTHORIZED:
      return QObject::tr("Key Not Authorized");
    default:
      return QObject::tr("Unknown");
  }
}

void add_signer_fields(InfoBoardCard& card, const GpgSignerInfo& signer) {
  if (!signer.uid.isEmpty()) {
    card.fields.append({QObject::tr("Signer"), signer.uid});
  } else if (!signer.fingerprint.isEmpty()) {
    card.AddField(QObject::tr("Fingerprint"), signer.fingerprint,
                  {{}, true, InfoBoardFieldGroup::kDETAIL});
  }
  card.AddField(QObject::tr("Key ID"), signer.keyId,
                {{}, true, InfoBoardFieldGroup::kDETAIL});
  if (!signer.pubkeyAlgo.isEmpty() || !signer.hashAlgo.isEmpty()) {
    QString algo = signer.pubkeyAlgo;
    if (!signer.hashAlgo.isEmpty()) {
      algo += u" / "_s + signer.hashAlgo;
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
    card.AddField(QObject::tr("Fingerprint"), reci.fingerprint,
                  {{}, true, InfoBoardFieldGroup::kDETAIL});
  }
  card.AddField(QObject::tr("Key ID"), reci.keyId,
                {{}, true, InfoBoardFieldGroup::kDETAIL});
  card.fields.append({reci.algoIsPrimaryKey
                          ? QObject::tr("Primary Key Algorithm")
                          : QObject::tr("Algorithm"),
                      reci.pubkeyAlgo});
}

}  // namespace

auto ConvertOpInfoToCards(const GpgOpResultInfo& info) -> QList<InfoBoardCard> {
  QList<InfoBoardCard> cards;

  for (const auto& sig : info.signatures) {
    InfoBoardCard sig_card;
    sig_card.kind = InfoBoardCardKind::kSIGNATURE;
    // Say what the card is, then its verdict.
    sig_card.title =
        QObject::tr("Signature · %1").arg(sig_validity_to_text(sig.validity));
    sig_card.status = sig.rejected ? kINFO_ERROR_CRITICAL
                                   : sig_validity_to_status(sig.validity);
    add_signer_fields(sig_card, sig.signer);
    for (const auto& w : sig.warnings) {
      sig_card.fields.append({u"⚠"_s, w});
    }
    cards.append(sig_card);
  }

  for (const auto& ns : info.newSignatures) {
    InfoBoardCard ns_card;
    ns_card.kind = InfoBoardCardKind::kNEW_SIGNATURE;
    ns_card.title = QObject::tr("Signature Created");
    ns_card.status = kINFO_ERROR_OK;
    add_signer_fields(ns_card, ns.signer);
    ns_card.fields.append({QObject::tr("Mode"), ns.sigMode});
    cards.append(ns_card);
  }

  for (const auto& inv : info.invalidSigners) {
    InfoBoardCard inv_card;
    inv_card.kind = InfoBoardCardKind::kINVALID_SIGNER;
    inv_card.title = QObject::tr("Invalid Signer");
    inv_card.status = kINFO_ERROR_WARN;
    inv_card.AddField(QObject::tr("Fingerprint"), inv.first,
                      {{}, true, InfoBoardFieldGroup::kDETAIL});
    inv_card.fields.append({QObject::tr("Reason"), inv.second});
    cards.append(inv_card);
  }

  const bool is_decrypt = info.op == GpgOperation::kDECRYPT ||
                          info.op == GpgOperation::kDECRYPT_VERIFY;
  const bool is_encrypt = info.op == GpgOperation::kENCRYPT ||
                          info.op == GpgOperation::kENCRYPT_SIGN;

  const bool is_decrypt_with_data =
      info.newSignatures.isEmpty() && info.invalidSigners.isEmpty() &&
      (!info.recipients.isEmpty() || !info.details.isEmpty()) && is_decrypt;
  if (is_decrypt_with_data) {
    InfoBoardCard meta_card;
    meta_card.kind = InfoBoardCardKind::kMESSAGE_METADATA;
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

  if (is_encrypt) {
    for (const auto& reci : info.recipients) {
      InfoBoardCard reci_card;
      reci_card.kind = InfoBoardCardKind::kRECIPIENT;
      reci_card.title = QObject::tr("Encryption Recipient");
      reci_card.status = kINFO_ERROR_OK;
      add_recipient_fields(reci_card, reci);
      cards.append(reci_card);
    }
  }

  if (is_decrypt) {
    for (const auto& reci : info.recipients) {
      InfoBoardCard reci_card;
      reci_card.kind = InfoBoardCardKind::kRECIPIENT;
      reci_card.title = QObject::tr("Decryption Recipient");
      reci_card.status = reci.keyFound ? kINFO_ERROR_OK : kINFO_ERROR_WARN;
      add_recipient_fields(reci_card, reci);
      cards.append(reci_card);
    }
  }

  return cards;
}

}  // namespace GpgFrontend
