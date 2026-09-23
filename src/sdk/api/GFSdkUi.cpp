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

#include "GFSDKUI.h"

#include <QByteArray>
#include <QLocale>
#include <QString>
#include <cstring>

#include "GFSdkInternal.h"

/**
 * @file GFSdkUi.cpp
 * @brief Widgets, dialogs and extension points, module-side.
 */

auto GFUICreateGUIObject(GFSDKContext* ctx, QObjectFactory factory, void* data)
    -> void* {
  GF_SDK_REQUIRE(ctx, ui, "GFUICreateGUIObject", nullptr);
  return g->create_object(hctx, factory, data);
}

auto GFUIGetGUIObject(GFSDKContext* ctx, const char* id) -> void* {
  GF_SDK_REQUIRE(ctx, ui, "GFUIGetGUIObject", nullptr);
  return g->get_object(hctx, id);
}

auto GFUIShowDialog(GFSDKContext* ctx, void* dialog, void* parent) -> int {
  GF_SDK_REQUIRE(ctx, ui, "GFUIShowDialog", -1);
  return g->show_dialog(hctx, dialog, parent);
}

auto GFUIThemeColor(GFSDKContext* ctx, int role, void* widget) -> uint32_t {
  GF_SDK_REQUIRE(ctx, ui, "GFUIThemeColor", 0U);
  return g->theme_color(hctx, role, widget);
}

auto GFUILoadScript(GFSDKContext* ctx, const char* chunk_name,
                    GFBufferView source) -> int {
  GF_SDK_REQUIRE(ctx, script, "GFUILoadScript", -1);
  return g->load(hctx, chunk_name, source);
}

auto GFUIThemeColorForRole(GFSDKContext* ctx, int role) -> uint32_t {
  GF_SDK_REQUIRE(ctx, ui, "GFUIThemeColorForRole", 0U);
  if (!GF_SDK_GROUP_HAS(g, theme_color_role)) return 0U;
  return g->theme_color_role(hctx, role);
}

auto GFUIDefaultUserFilePath(GFSDKContext* ctx) -> GFBufferRef {
  GF_SDK_REQUIRE(ctx, ui, "GFUIDefaultUserFilePath", nullptr);
  return g->user_file_path(hctx);
}

auto GFUIRegisterSettingsPage(GFSDKContext* ctx,
                              const GFUISettingsPageSpec* spec) -> int {
  GF_SDK_REQUIRE(ctx, ui, "GFUIRegisterSettingsPage", -1);
  return g->register_settings_page(hctx, spec);
}

auto GFUIUnregisterSettingsPage(GFSDKContext* ctx, const char* page_id) -> int {
  GF_SDK_REQUIRE(ctx, ui, "GFUIUnregisterSettingsPage", -1);
  return g->unregister_settings_page(hctx, page_id);
}

auto GFUIRegisterTabPageView(GFSDKContext* ctx, const GFUITabViewSpec* spec)
    -> int {
  GF_SDK_REQUIRE(ctx, ui, "GFUIRegisterTabPageView", -1);
  return g->register_tab_view(hctx, spec);
}

auto GFUIUnregisterTabPageView(GFSDKContext* ctx, const char* tab_type) -> int {
  GF_SDK_REQUIRE(ctx, ui, "GFUIUnregisterTabPageView", -1);
  return g->unregister_tab_view(hctx, tab_type);
}

auto GFUIRegisterFileExtension(GFSDKContext* ctx, const char* extension,
                               const char* event_prefix) -> int {
  GF_SDK_REQUIRE(ctx, ui, "GFUIRegisterFileExtension", -1);
  return g->register_file_extension(hctx, extension, event_prefix);
}

/**
 * @brief PURE. No context, no host.
 *
 * Formatting a byte count needs a locale, which the module already has, and
 * nothing else. It was an exported host symbol purely so that every panel
 * would spell "50.2 kB" the same way, which this achieves equally well
 * without costing an ABI entry point.
 *
 * Writes into the caller's buffer rather than allocating, because allocating
 * would mean needing an arena, which would mean needing a context, which
 * would make a pure function impure for no reason.
 */
auto GFUIHumanSize(int64_t bytes, char* out, size_t cap) -> int {
  if (out == nullptr || cap == 0) return -1;

  const auto text =
      QLocale()
          .formattedDataSize(bytes, 1, QLocale::DataSizeTraditionalFormat)
          .toUtf8();
  if (static_cast<size_t>(text.size()) + 1 > cap) return -1;

  memcpy(out, text.constData(), text.size());
  out[text.size()] = '\0';
  return static_cast<int>(text.size());
}

auto GFNativeWidgetRegister(GFSDKContext* ctx, const GFNativeWidgetSpec* spec)
    -> int {
  GF_SDK_REQUIRE(ctx, native, "GFNativeWidgetRegister", -1);
  return g->register_widget(hctx, spec);
}

auto GFNativeWidgetUnregister(GFSDKContext* ctx, const char* name) -> int {
  GF_SDK_REQUIRE(ctx, native, "GFNativeWidgetUnregister", -1);
  return g->unregister_widget(hctx, name);
}

auto GFNativeDocumentModified(GFSDKContext* ctx, uint64_t instance) -> int {
  GF_SDK_REQUIRE(ctx, native, "GFNativeDocumentModified", -1);
  return g->document_modified(hctx, instance);
}

auto GFNativeDocumentShowSource(GFSDKContext* ctx, uint64_t instance,
                                int source) -> int {
  GF_SDK_REQUIRE(ctx, native, "GFNativeDocumentShowSource", -1);
  return g->document_show_source(hctx, instance, source);
}

auto GFNativeDocumentRequestCrypto(GFSDKContext* ctx, uint64_t instance,
                                   uint32_t op) -> int {
  GF_SDK_REQUIRE(ctx, native, "GFNativeDocumentRequestCrypto", -1);
  return g->document_request_crypto(hctx, instance, op);
}

auto GFNativeDocumentOpsChanged(GFSDKContext* ctx, uint64_t instance) -> int {
  GF_SDK_REQUIRE(ctx, native, "GFNativeDocumentOpsChanged", -1);
  return g->document_ops_changed(hctx, instance);
}

auto GFNativeSettingsRestartNeeded(GFSDKContext* ctx, uint64_t instance,
                                   int level) -> int {
  GF_SDK_REQUIRE(ctx, native, "GFNativeSettingsRestartNeeded", -1);
  return g->settings_restart_needed(hctx, instance, level);
}

auto GFNativeDialogClose(GFSDKContext* ctx, uint64_t instance) -> int {
  GF_SDK_REQUIRE(ctx, native, "GFNativeDialogClose", -1);
  return g->dialog_close(hctx, instance);
}
