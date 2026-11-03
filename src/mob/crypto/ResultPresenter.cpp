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
  };
}

}  // namespace GpgFrontend::Mobile
