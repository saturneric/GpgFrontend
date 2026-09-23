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

#include <stddef.h>
#include <stdint.h>

#include "GFSDKContext.h"

/**
 * @file GFSDKUI.h
 * @brief Putting things on the screen.
 *
 * Reading the application's settings is NOT here, although it used to be
 * spelled `GFUIGlobalSettings`. A module reading configuration is doing
 * storage, and making it ask for the ability to open dialogs in order to read
 * a preference would be a grant that says more than it means. See
 * GFSDKStorage.h.
 *
 * No function here hands a module a Host object, or takes one: a module's
 * UI is its script (anchors, actions, mounts) and its own native widgets,
 * which the Host places in containers of its own.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A colour of the application's visual language, from its palette.
 *
 * Asked for by role alone, from the application palette: no widget is
 * involved, so no object crosses the boundary. A module widget with a palette
 * of its own reads that palette itself.
 *
 * @return 0xAARRGGBB, or 0 when the Host cannot say
 */
uint32_t GFUIThemeColorForRole(GFSDKContext* ctx, int role);

/**
 * @brief The directory a file dialog for USER files should open in.
 *
 * The same answer the application's own dialogs use, so a module's dialog
 * lands where the user expects rather than in the process working directory.
 * Only for user files: a dialog picking a GnuPG installation or a key
 * database is asking a different question.
 *
 * @return owned; release with GFBufferRelease
 */
GFBufferRef GFUIDefaultUserFilePath(GFSDKContext* ctx);

/**
 * @brief Load one of the module's UI scripts, by name.
 *
 * The runtime does this itself for every script embedded with
 * `gf_add_module(... LUA_SCRIPTS ...)`, after on_activate -- so the native
 * widgets on_activate registered already exist when the script mounts them.
 * Loading happens on the GUI thread, later; a script that fails is reported
 * in the log and the Module Controller, and nothing it registered remains.
 *
 * @return 0 when the load was scheduled
 */
int GFUILoadScript(GFSDKContext* ctx, const char* chunk_name,
                   GFBufferView source);

/* --- native widgets (capability "ui.custom") -----------------------------
 *
 * A module's own QWidget, which its UI script mounts into a container the
 * Host owns: a dialog, a settings page, a document view. The widget is the
 * module's in every respect; the Host only ever hands back an instance
 * number, and every call below is refused for an instance the calling
 * module does not own, or of another kind. Most modules use the C++ classes
 * in GFModuleNativeWidget.h instead of these.
 */

int GFNativeWidgetRegister(GFSDKContext* ctx, const GFNativeWidgetSpec* spec);
int GFNativeWidgetUnregister(GFSDKContext* ctx, const char* name);
int GFNativeDocumentModified(GFSDKContext* ctx, uint64_t instance);
int GFNativeDocumentShowSource(GFSDKContext* ctx, uint64_t instance,
                               int source);
int GFNativeDocumentRequestCrypto(GFSDKContext* ctx, uint64_t instance,
                                  uint32_t op);
int GFNativeDocumentOpsChanged(GFSDKContext* ctx, uint64_t instance);
int GFNativeSettingsRestartNeeded(GFSDKContext* ctx, uint64_t instance,
                                  int level);
int GFNativeDialogClose(GFSDKContext* ctx, uint64_t instance);

/**
 * @brief A file size as the rest of the application writes it. PURE.
 *
 * Takes no context: formatting a byte count needs a locale, which the module
 * already has, and nothing else. It was an exported host symbol purely so
 * every panel would spell "50.2 kB" the same way, which this achieves equally
 * well without costing an ABI entry point.
 *
 * Writes at most @p cap bytes including the terminator.
 *
 * @return the length written, or negative if @p out is NULL or too small
 */
int GFUIHumanSize(int64_t bytes, char* out, size_t cap);

#ifdef __cplusplus
}
#endif
