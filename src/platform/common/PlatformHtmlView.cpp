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

#include "GpgFrontend.h"

// Platforms with an engine of their own provide the real renderer; this keeps
// gf_ui linkable everywhere else, where HTML views come from modules alone.
// Windows has one only when the build found the WebView2 SDK.
#if !defined(Q_OS_MACOS) && \
    !(defined(Q_OS_WINDOWS) && defined(GF_HAS_WEBVIEW2_SDK))

#include "platform/common/PlatformHtmlView.h"

namespace GpgFrontend::UI {

void RegisterPlatformHtmlViewRenderer() {}

}  // namespace GpgFrontend::UI

#endif
