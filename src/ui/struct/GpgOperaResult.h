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

#include "core/function/result_analyse/GpgOpResultInfo.h"
#include "core/model/GFBuffer.h"

namespace GpgFrontend::UI {

struct GpgOperaResult {
  QString tag;        ///< Display tag (e.g., filename for file operations)
  GFBuffer o_buffer;  ///< Output buffer (for message operations)
  GpgFrontend::GpgOpResultInfo op_info;  ///< Structured operation result info

  /// Construct from GpgOpResultInfo and optional display tag
  explicit GpgOperaResult(GpgFrontend::GpgOpResultInfo info, QString tag = {});

  /// Merge another GpgOperaResult into this one (delegates to op_info.merge)
  void Merge(const GpgOperaResult& other);
};

}  // namespace GpgFrontend::UI