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

#include "crypto/ResultPresenter.h"

#include "core/function/result_analyse/InfoBoardCard.h"

namespace GpgFrontend::Mobile {

auto ValidityName(GpgSigValidity validity) -> QString {
  switch (validity) {
    case GpgSigValidity::kFULLY_VALID:
      return QStringLiteral("fully_valid");
    case GpgSigValidity::kVALID_WITH_ISSUES:
      return QStringLiteral("valid_with_issues");
    case GpgSigValidity::kVALID_NOT_FULLY_TRUSTED:
      return QStringLiteral("valid_not_fully_trusted");
    case GpgSigValidity::kINVALID:
      return QStringLiteral("invalid");
    case GpgSigValidity::kKEY_MISSING:
      return QStringLiteral("key_missing");
    case GpgSigValidity::kKEY_REVOKED:
      return QStringLiteral("key_revoked");
    case GpgSigValidity::kSIG_EXPIRED:
      return QStringLiteral("sig_expired");
    case GpgSigValidity::kKEY_EXPIRED:
      return QStringLiteral("key_expired");
    case GpgSigValidity::kKEY_NOT_AUTHORIZED:
      return QStringLiteral("key_not_authorized");
    case GpgSigValidity::kERROR:
    case GpgSigValidity::kUNKNOWN:
      return QStringLiteral("error");
  }
  return QStringLiteral("error");
}

namespace {

/// A card's colour, as QML names it.
auto CardStatusName(InfoBoardStatus status) -> QString {
  switch (status) {
    case kINFO_ERROR_OK:
      return QStringLiteral("ok");
    case kINFO_ERROR_WARN:
      return QStringLiteral("warn");
    case kINFO_ERROR_CRITICAL:
      return QStringLiteral("critical");
    default:
      return QStringLiteral("neutral");
  }
}

auto VerdictName(GpgSigVerdict verdict) -> QString {
  switch (verdict) {
    case GpgSigVerdict::kGOOD:
      return QStringLiteral("good");
    case GpgSigVerdict::kWARNING:
      return QStringLiteral("warning");
    case GpgSigVerdict::kBAD:
      break;
  }
  return QStringLiteral("bad");
}

/// Whether the signature itself holds: made by that key over this data.
auto SignatureFact(const GpgVerifySigInfo& sig) -> QString {
  if (sig.rejected) return QStringLiteral("rejected");
  switch (sig.validity) {
    case GpgSigValidity::kINVALID:
      return QStringLiteral("invalid");
    case GpgSigValidity::kKEY_MISSING:
    case GpgSigValidity::kERROR:
    case GpgSigValidity::kUNKNOWN:
      return QStringLiteral("unchecked");
    case GpgSigValidity::kSIG_EXPIRED:
      return QStringLiteral("expired");
    default:
      return QStringLiteral("valid");
  }
}

/// What state the signing key is in, apart from the signature.
auto KeyStatusFact(const GpgVerifySigInfo& sig) -> QString {
  switch (sig.validity) {
    case GpgSigValidity::kKEY_MISSING:
      return QStringLiteral("missing");
    case GpgSigValidity::kKEY_EXPIRED:
      return QStringLiteral("expired");
    case GpgSigValidity::kKEY_REVOKED:
      return QStringLiteral("revoked");
    case GpgSigValidity::kKEY_NOT_AUTHORIZED:
      return QStringLiteral("not_authorized");
    case GpgSigValidity::kVALID_WITH_ISSUES:
      return QStringLiteral("issues");
    default:
      return sig.rejected ? QStringLiteral("expired") : QStringLiteral("ok");
  }
}

/// Whether anything vouched for who signed: only an engine with a trust
/// model (GnuPG) says; rPGP never does.
auto IdentityTrustFact(const GpgVerifySigInfo& sig) -> QString {
  if (!sig.trustModel) return QStringLiteral("not_assessed");
  return sig.validity == GpgSigValidity::kFULLY_VALID
             ? QStringLiteral("trusted")
             : QStringLiteral("not_trusted");
}

}  // namespace

auto PresentResult(const GpgOpResultInfo& info) -> QVariantMap {
  QVariantList signatures;
  for (const auto& sig : info.signatures) {
    // Graded as the desktop grades it; one the engine rejected is bad
    // whatever its validity says.
    const auto verdict =
        sig.rejected ? GpgSigVerdict::kBAD : GradeSignature(sig.validity);
    signatures.append(QVariantMap{
        {QStringLiteral("uid"), sig.signer.uid},
        {QStringLiteral("fingerprint"), sig.signer.fingerprint},
        {QStringLiteral("keyId"), sig.signer.keyId},
        {QStringLiteral("validity"), ValidityName(sig.validity)},
        {QStringLiteral("good"), verdict == GpgSigVerdict::kGOOD},
        {QStringLiteral("verdict"), VerdictName(verdict)},
        {QStringLiteral("signature"), SignatureFact(sig)},
        {QStringLiteral("keyStatus"), KeyStatusFact(sig)},
        {QStringLiteral("identityTrust"), IdentityTrustFact(sig)},
        {QStringLiteral("signTime"), sig.signer.signTime},
        {QStringLiteral("warnings"), sig.warnings},
    });
  }

  QVariantList recipients;
  for (const auto& recipient : info.recipients) {
    recipients.append(QVariantMap{
        {QStringLiteral("uid"), recipient.uid},
        {QStringLiteral("keyId"), recipient.keyId},
        {QStringLiteral("keyFound"), recipient.keyFound},
    });
  }

  QStringList errors;
  bool cancelled = false;
  for (const auto& error : info.stageErrors) {
    if (error.error.Canceled()) cancelled = true;
    if (!error.codeText.isEmpty()) errors << error.codeText;
    for (const auto& detail : error.engineDetails) {
      if (!detail.second.isEmpty()) errors << detail.second;
    }
  }
  errors.removeDuplicates();

  // The cards the desktop's Status Panel shows for the same result, made the
  // same way (ConvertOpInfoToCards), so both say the same things.
  QVariantList cards;
  for (const auto& card : ConvertOpInfoToCards(info)) {
    QVariantList fields;
    for (qsizetype i = 0; i < card.fields.size(); ++i) {
      const auto& [label, value] = card.fields.at(i);
      const auto meta = card.MetaAt(i);
      // As the desktop's screen: no empty value, and nothing the headline
      // already says (the engine's report keeps all of it).
      if (value.isEmpty() || meta.summarized) continue;
      fields.append(QVariantMap{
          {QStringLiteral("label"), label},
          {QStringLiteral("value"), value},
          {QStringLiteral("technical"), meta.technical},
          {QStringLiteral("note"), meta.note},
          // Runs of fields (what happened, why, what next) are set apart.
          {QStringLiteral("group"), static_cast<int>(meta.group)},
      });
    }
    cards.append(QVariantMap{
        {QStringLiteral("title"), card.title},
        {QStringLiteral("status"), CardStatusName(card.status)},
        {QStringLiteral("fields"), fields},
    });
  }

  return {
      {QStringLiteral("status"), info.status},
      {QStringLiteral("cancelled"), cancelled},
      {QStringLiteral("operation"), info.operation},
      {QStringLiteral("description"), info.description},
      {QStringLiteral("engine"), info.engine},
      {QStringLiteral("report"), info.report},
      {QStringLiteral("signatures"), signatures},
      {QStringLiteral("recipients"), recipients},
      {QStringLiteral("errors"), errors},
      {QStringLiteral("cards"), cards},
  };
}

}  // namespace GpgFrontend::Mobile
