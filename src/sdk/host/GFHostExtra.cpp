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

#include <core/utils/BuildInfoUtils.h>
#include <core/utils/CommonUtils.h>

#include "GFHostImpl.h"
#include "private/GFSDKPrivate.h"

namespace gf_host {

auto GFHttpRequestUserAgent() -> const char* {
  // Borrowed, with process lifetime -- which is what the header has always
  // claimed. It used to hand back a fresh GFStrDup allocation instead, so the
  // five call sites that took the documentation at its word and did not free
  // leaked once per request, while the two that did free were the only ones
  // written against the implementation. Under the SDK's one ownership rule a
  // const char* return is ALWAYS borrowed, so there is no per-function rule
  // left to get wrong.
  static const QByteArray kUserAgent =
      GpgFrontend::GetHttpRequestUserAgent().toUtf8();
  return kUserAgent.constData();
}
}  // namespace gf_host