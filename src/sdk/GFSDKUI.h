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
 * Widgets may only be created and shown on the main thread; the two calls
 * that need it marshal there for you.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Construct a QObject-derived GUI object on the main thread.
 *
 * If called from another thread the factory is dispatched to the main thread
 * via a blocking queued connection before returning.
 *
 * @return opaque QObject pointer, or NULL on failure
 */
void* GFUICreateGUIObject(GFSDKContext* ctx, QObjectFactory factory,
                          void* data);

/**
 * @brief Resolve a GUI handle the host passed in an event parameter.
 *
 * Returns NULL once the object has been destroyed, so a stale handle fails
 * closed. Most handlers should use GFEvent::RequireGui<T>() instead, which
 * does this resolution and produces the failure to return in one line.
 */
void* GFUIGetGUIObject(GFSDKContext* ctx, const char* id);

/**
 * @brief Show a QDialog on the main thread.
 *
 * The dialog must have been created on the main thread. Ownership transfers
 * to @p parent. Returns immediately after scheduling the show.
 *
 * @param dialog a QDialog; @param parent a QWidget or NULL. Any other
 *        non-QObject pointer is undefined behavior.
 * @return 0 when the show was scheduled; negative otherwise
 */
int GFUIShowDialog(GFSDKContext* ctx, void* dialog, void* parent);

/**
 * @brief A color of the application's own visual language.
 *
 * Five functions became one taking a @ref GFUIColorRole, because they
 * differed by which role they named and nothing else. Every color is derived
 * from @p widget's palette rather than fixed, so it stays legible under both
 * themes.
 *
 * @return 0xAARRGGBB, or 0 if @p widget is not a QWidget
 */
uint32_t GFUIThemeColor(GFSDKContext* ctx, int role, void* widget);

/**
 * @brief A colour of the application's visual language, from its palette.
 *
 * The form for everything except a widget with a palette of its own: no
 * widget is involved, so no object crosses the boundary.
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

/* --- extension points ----------------------------------------------------
 *
 * Each takes an append-only spec struct rather than positional arguments, so
 * a later field costs an append rather than a new entry point.
 *
 * Every registration MUST be undone from the module's on_deactivate hook: a
 * factory pointing into an unloaded shared object crashes the next time the
 * host needs it. Widgets already on screen keep working.
 */

int GFUIRegisterSettingsPage(GFSDKContext* ctx,
                             const GFUISettingsPageSpec* spec);
int GFUIUnregisterSettingsPage(GFSDKContext* ctx, const char* page_id);

int GFUIRegisterTabPageView(GFSDKContext* ctx, const GFUITabViewSpec* spec);
int GFUIUnregisterTabPageView(GFSDKContext* ctx, const char* tab_type);

/**
 * @brief Take over opening files with @p extension.
 *
 * When the user opens such a file the host fires
 * `FILE_EXT_<PREFIX>_OP_OPEN_FILE` and does nothing else, so the module owns
 * the operation from there.
 *
 * @param extension without the leading dot, e.g. "eml"
 */
int GFUIRegisterFileExtension(GFSDKContext* ctx, const char* extension,
                              const char* event_prefix);

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
