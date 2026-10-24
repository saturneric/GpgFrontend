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

namespace GpgFrontend {

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

void GpgOpResultInfo::CollapseSharedCallError(unsigned int call_error_code,
                                              GpgOperation combined) {
  if (call_error_code == 0) return;

  GpgOpStageError shared;
  shared.scope = combined;
  shared.origin = GpgErrorOrigin::kENGINE;
  shared.code = call_error_code;

  QList<GpgOpStageError> kept;
  qsizetype first_shared = -1;
  for (const auto& e : stageErrors) {
    if (e.origin != GpgErrorOrigin::kENGINE || e.code != call_error_code) {
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

}  // namespace GpgFrontend
