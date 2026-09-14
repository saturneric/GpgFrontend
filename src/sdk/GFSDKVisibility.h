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

/**
 * @file GFSDKVisibility.h
 * @brief Fallback definition of GF_SDK_EXPORT for consumers of these headers.
 *
 * GF_SDK_EXPORT is normally supplied by the generated GFSdkExport.h, which
 * CMake force-includes through gf_sdk's precompiled header. That works while
 * gf_sdk is building itself, and it works for modules, which inherit that PCH.
 * It does NOT work for any other consumer -- gf_core including a public SDK
 * header, or a plain C translation unit -- which would otherwise fail to
 * compile on a macro it has no reason to know about.
 *
 * A public header should stand on its own, so define the macro to nothing
 * when nobody has defined it. An empty expansion is correct for a CONSUMER:
 * importing a symbol on ELF and Mach-O needs no attribute, and on Windows the
 * import side is handled by the same generated header when it is present.
 *
 * CAREFUL: this must never win while gf_sdk is compiling its OWN sources, or
 * the definitions would lose visibility("default") and the symbols would
 * silently stop being exported -- modules would then fail at load time rather
 * than at build time. That does not happen because the PCH is force-included
 * ahead of everything else, so GF_SDK_EXPORT is already defined by the time
 * any of this is seen. The export audit in the test suite exists to catch it
 * if that ever stops being true.
 */

#ifndef GF_SDK_EXPORT
#define GF_SDK_EXPORT
#endif
