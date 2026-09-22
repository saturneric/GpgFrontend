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

#include <core/utils/RustUtils.h>

#include "GFHostImpl.h"
#include "private/GFHostContext.h"
#include "private/GFSDKPrivate.h"

namespace gf_host {

auto GFPgpInspectData(GFBufferView in, char** out_json) -> int {
  if (out_json == nullptr) {
    LOG_W() << "pgp.inspect: out_json must not be null";
    return -1;
  }

  // An empty input is not an error here either: it inspects to an empty
  // document, which is what the dialog shows before anything is loaded.
  const auto* data = static_cast<const char*>(GFBufferData(in));
  const auto size = GFBufferSize(in);

  auto json = GpgFrontend::InspectOpenPGPData(GpgFrontend::GFBuffer(
      QByteArray(data == nullptr ? "" : data, static_cast<qsizetype>(size))));
  if (json.isEmpty()) {
    LOG_E() << "openpgp inspection produced no document";
    return -1;
  }

  *out_json = GFBytesDup(json, nullptr);
  return 0;
}

}  // namespace gf_host