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

void GpgOpResultInfo::Merge(const GpgOpResultInfo& other) {
  // Status: take the minimum (worst) status
  status = std::min(status, other.status);

  // Report: concatenate
  report += other.report;

  // Operation: concatenate with " + " separator, skip empty
  if (operation.isEmpty()) {
    operation = other.operation;
  } else if (!other.operation.isEmpty()) {
    operation += QStringLiteral(" + ") + other.operation;
  }

  // Description: concatenate with newline, skip empty
  if (description.isEmpty()) {
    description = other.description;
  } else if (!other.description.isEmpty()) {
    description += QStringLiteral("\n") + other.description;
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

}  // namespace GpgFrontend
