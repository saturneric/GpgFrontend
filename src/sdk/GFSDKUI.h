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

#include "GFSDKUIModel.h"

extern "C" {

/**
 * @brief Creates a QObject-derived GUI object on the main (UI) thread.
 *
 * If called from a non-UI thread, the factory is dispatched to the main thread
 * via a blocking queued connection before returning.
 *
 * @param factory Callback that constructs the GUI object.
 * @param data    User data forwarded to @p factory.
 * @return Opaque pointer to the created QObject, or nullptr on failure.
 */
auto GF_SDK_EXPORT GFUICreateGUIObject(QObjectFactory factory, void* data)
    -> void*;

/**
 * @brief Retrieves a registered GUI object by its string identifier.
 *
 * @param id Null-terminated identifier of the GUI object.
 * @return Opaque QObject pointer, or nullptr if not found or @p id is nullptr.
 */
auto GF_SDK_EXPORT GFUIGetGUIObject(const char* id) -> void*;

/**
 * @brief Shows a QDialog on the main thread.
 *
 * The dialog must have been created on the main thread. The call returns
 * immediately after scheduling the show; it does not wait for the dialog to
 * close.
 *
 * @param dialog Opaque pointer to a QDialog instance.
 * @param parent Opaque pointer to a QWidget parent, or nullptr for no parent.
 * @return true if the dialog was shown successfully, false on error.
 */
auto GF_SDK_EXPORT GFUIShowDialog(void* dialog, void* parent) -> bool;

/**
 * @brief Returns a pointer to the application-wide QSettings object.
 *
 * The returned pointer is valid for the lifetime of the application and must
 * not be deleted by the caller.
 *
 * @return Opaque pointer to the global QSettings instance.
 */
auto GF_SDK_EXPORT GFUIGlobalSettings() -> void*;

/**
 * @brief Associates a file extension with an event prefix for open-file
 *        handling.
 *
 * When the user opens a file with the given extension, the UI emits an event
 * named @p event_prefix + "." + extension.
 *
 * @param extension    File extension string without the leading dot (e.g.
 *                     "gpg").
 * @param event_prefix Prefix used to construct the event identifier.
 * @return 0 on success, -1 if either argument is nullptr.
 */
auto GF_SDK_EXPORT GFUIRegisterFileExtensionHandleEvent(
    const char* extension, const char* event_prefix) -> int;

/**
 * @brief Registers a settings page owned by a module.
 *
 * The page appears in the application's Settings dialog, grouped under
 * @p section_id. The dialog is built and destroyed every time the user opens
 * it, so @p factory is invoked once per dialog instance, on the main (UI)
 * thread, and must return a *fresh*, unparented QWidget each time. Ownership
 * of the returned widget passes to the dialog. Registering while a dialog is
 * already open is harmless: that dialog snapshots the registry at
 * construction, so the page shows up the next time it is opened.
 *
 * The widget should expose `void SetSettings()` and `void ApplySettings()` as
 * slots (or Q_INVOKABLE); the dialog calls them by name to load, revert and
 * apply. A widget may also declare a `void SignalRestartNeeded(int)` signal to
 * take part in the dialog's restart confirmation.
 *
 * @p title and @p keywords must be untranslated source strings, marked with
 * GC_TR(). The host translates them in the "GTrC" context while the dialog is
 * built, so a language change is picked up without re-registering.
 *
 * @param page_id    Unique, module-namespaced identifier.
 * @param section_id Canonical section key: "application", "keys_engines",
 *                   "features" or "system". An unknown key creates its own
 *                   section, placed after the built-in ones.
 * @param title      Sidebar row and page heading.
 * @param keywords   Extra search terms, '\n'-separated. May be nullptr.
 * @param factory    Widget factory, invoked on the main thread.
 * @param data       Opaque pointer handed unchanged to @p factory on *every*
 *                   invocation. Unlike GFUICreateGUIObject this is not a
 *                   one-shot capsule: it must stay valid while the module is
 *                   loaded.
 * @return 0 on success, -1 on a missing argument or a duplicate @p page_id.
 */
auto GF_SDK_EXPORT GFUIRegisterSettingsPage(
    const char* page_id, const char* section_id, const char* title,
    const char* keywords, QObjectFactory factory, void* data) -> int;

/**
 * @brief Removes a settings page registration.
 *
 * Call this from GFDeactivateModule(): a factory pointing into an unloaded
 * shared object would crash the next time the Settings dialog is opened.
 * Dialogs already on screen keep the widget they built.
 *
 * @param page_id Identifier passed to GFUIRegisterSettingsPage.
 * @return 0 on success, -1 if @p page_id is nullptr or was never registered.
 */
auto GF_SDK_EXPORT GFUIUnregisterSettingsPage(const char* page_id) -> int;

/**
 * @brief Registers a module-owned primary view for a tab type.
 *
 * The host still creates and owns the editor page for the tab, and that page's
 * text document stays the canonical content -- so saving, crash recovery and
 * the unsaved-changes prompt keep working exactly as they do for a plain text
 * tab. This factory only supplies the widget shown *on top of* that document,
 * with the plain editor remaining reachable as the tab's "Raw Source" view.
 *
 * @p factory is invoked once per tab, on the main (UI) thread, and must return
 * a *fresh*, unparented QWidget each time. Ownership passes to the page.
 *
 * The widget may expose any of the following as slots or Q_INVOKABLE members;
 * the host probes for each by name and simply skips the ones that are absent,
 * so the contract is additive:
 *   - `void LoadFromSource(const QByteArray&)` -- the document changed from
 *     the outside (file opened, operation result applied). The widget must not
 *     write the document back from inside this call.
 *   - `QByteArray SaveToSource()` -- return the view reserialized. The host
 *     writes the result into the document; the view never touches it. Called
 *     only when IsDirty() says so, and always before a save, before every
 *     crypto operation, on a view switch and before the tab closes.
 *   - `bool IsDirty()` -- the view holds edits not yet written back.
 *   - `void WipeContent()` -- zero any decrypted plaintext the view holds,
 *     including attachment buffers. Called when the tab closes and at exit.
 *
 * A view may also declare a `void SignalContentModified()` signal. Emitting it
 * marks the tab modified straight away, without reserializing anything -- so a
 * tab closed immediately after an edit still prompts to save. Serialization
 * stays lazy; only the flag is eager.
 *
 * A tab type with no registered view behaves exactly as before: the host opens
 * an ordinary plain text tab.
 *
 * @param tab_type Tab type as used in `EDIT_TAB_TYPE_<TYPE>_OP_*`, matched
 *                 case-insensitively (e.g. "email").
 * @param factory  Widget factory, invoked on the main thread.
 * @param data     Opaque pointer handed unchanged to @p factory on *every*
 *                 invocation; it must stay valid while the module is loaded.
 * @return 0 on success, -1 on a missing argument or a duplicate @p tab_type.
 */
auto GF_SDK_EXPORT GFUIRegisterTabPageView(const char* tab_type,
                                           QObjectFactory factory, void* data)
    -> int;

/**
 * @brief Removes a tab page view registration.
 *
 * Call this from GFDeactivateModule(): a factory pointing into an unloaded
 * shared object would crash the next time a tab of this type is opened. Tabs
 * already on screen keep the widget they built.
 *
 * @param tab_type Type passed to GFUIRegisterTabPageView.
 * @return 0 on success, -1 if @p tab_type is nullptr or was never registered.
 */
auto GF_SDK_EXPORT GFUIUnregisterTabPageView(const char* tab_type) -> int;

/**
 * @brief The directory a file dialog for *user files* should open in.
 *
 * The same answer the application's own file dialogs use, so a module's dialog
 * lands where the user expects rather than in the process working directory.
 *
 * Only for user files. A dialog picking a system location -- a GnuPG
 * installation, a key database -- is asking a different question.
 *
 * @return Newly allocated absolute path; free it with GFFreeMemory.
 */
auto GF_SDK_EXPORT GFUIDefaultUserFilePath() -> char*;

/**
 * @brief Colours of the application's own visual language, for module widgets.
 *
 * A module cannot link the UI library, so without these it has to invent its
 * own palette -- and a panel that picks its own greys and greens stops looking
 * like part of the application, and stops following the user's theme.
 *
 * Every colour is derived from the widget's palette rather than fixed, so it
 * stays legible under both light and dark themes. Two conventions are worth
 * knowing before using them: a negative state is *not* painted red, it is
 * de-emphasised (GFUIAccentColor with @p positive false); and danger red is
 * reserved for what cannot be undone, or for secrets about to travel in the
 * clear.
 *
 * @param widget Opaque pointer to the QWidget whose palette to derive from.
 * @return Colour as 0xAARRGGBB, or 0 if @p widget is not a QWidget.
 */
auto GF_SDK_EXPORT GFUIMutedTextColor(void* widget) -> uint32_t;
auto GF_SDK_EXPORT GFUIBorderColor(void* widget) -> uint32_t;
auto GF_SDK_EXPORT GFUIWarningColor(void* widget) -> uint32_t;
auto GF_SDK_EXPORT GFUIDangerColor(void* widget) -> uint32_t;

/**
 * @brief Accent colour for a status chip.
 *
 * @param widget Opaque pointer to the QWidget whose palette to derive from.
 * @param positive Non-zero when the chip reports a good state.
 * @return Colour as 0xAARRGGBB, or 0 if @p widget is not a QWidget.
 */
auto GF_SDK_EXPORT GFUIAccentColor(void* widget, int positive) -> uint32_t;

/**
 * @brief A file size as the rest of the application writes it.
 *
 * Traditional units, one decimal. Worth going through rather than reaching for
 * QLocale directly: a panel printing "50.2 kB" beside one printing "50.2 KiB"
 * makes the reader wonder which of the two numbers is the real one.
 *
 * @param bytes The size.
 * @return Newly allocated string; free it with GFFreeMemory.
 */
auto GF_SDK_EXPORT GFUIHumanSize(int64_t bytes) -> char*;
}