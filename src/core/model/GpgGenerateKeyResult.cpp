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

#include "GpgGenerateKeyResult.h"

namespace GpgFrontend {

auto GpgGenerateKeyResult::IsGood() -> bool { return good_; }

auto GpgGenerateKeyResult::GetFingerprint() -> QString const {
  return fingerprint_;
}

GpgGenerateKeyResult::GpgGenerateKeyResult() = default;

GpgGenerateKeyResult::GpgGenerateKeyResult(const GpgGenerateKeyResult &) =
    default;

auto GpgGenerateKeyResult::operator=(const GpgGenerateKeyResult &)
    -> GpgGenerateKeyResult & = default;

GpgGenerateKeyResult::~GpgGenerateKeyResult() = default;

GpgGenerateKeyResult::GpgGenerateKeyResult(const QString &fingerprint)
    : good_(true), fingerprint_(fingerprint) {}

auto GpgGenerateKeyResult::Failed(const QString &error_detail)
    -> GpgGenerateKeyResult {
  GpgGenerateKeyResult result;
  result.error_detail_ = error_detail;
  return result;
}

auto GpgGenerateKeyResult::ErrorDetail() const -> QString {
  return error_detail_;
}

}  // namespace GpgFrontend