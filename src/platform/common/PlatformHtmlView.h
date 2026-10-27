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

namespace GpgFrontend::UI {

/// The name the platform's own HTML renderer registers under; its owner is
/// the empty one, the Host's.
constexpr auto kPlatformHtmlViewRendererName = "platform";

/**
 * @brief Register the platform's own HTML view renderer, where there is one.
 *
 * On macOS that is WKWebView. The engine's content rules compile
 * asynchronously, so the renderer is registered when they have compiled, and
 * never if they fail to: HTML mail then stays source, which is what a Host
 * with no renderer shows. Elsewhere this does nothing.
 *
 * GUI thread, after the application exists. Calling it again once the rules
 * have compiled registers the same renderer again at once -- a test that
 * detached it puts it back this way.
 */
void GF_UI_EXPORT RegisterPlatformHtmlViewRenderer();

}  // namespace GpgFrontend::UI
