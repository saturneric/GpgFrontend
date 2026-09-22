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

#include "GFSDKGpgList.h"

#include "GFSdkInternal.h"

/**
 * @file GFSdkGpgList.cpp
 * @brief Collections, module-side.
 *
 * Nineteen per-field accessors used to live here. They are gone: each list
 * hands back a borrowed row, and reading a field is reading a field.
 *
 * The string-list accessors go through the `list` group rather than `gpg`,
 * because a string list is a CONTAINER: `gpg` produces one and `storage`
 * produces another, and a module able to obtain a list but not release it
 * would be leaking because of a permission boundary.
 */

auto GFGpgFindKeys(GFSDKContext* ctx, int channel, const char* email,
                   GFGpgKeyBriefListRef* out) -> int {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgFindKeys", -1);
  return g->find_keys(hctx, channel, email, out);
}

auto GFGpgKeyBriefCount(GFSDKContext* ctx, GFGpgKeyBriefListRef l) -> size_t {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgKeyBriefCount", 0);
  return g->key_brief_count(hctx, l);
}

auto GFGpgKeyBriefAt(GFSDKContext* ctx, GFGpgKeyBriefListRef l, size_t i)
    -> const GFGpgKeyBriefRow* {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgKeyBriefAt", nullptr);
  return g->key_brief_at(hctx, l, i);
}

void GFGpgKeyBriefRelease(GFSDKContext* ctx, GFGpgKeyBriefListRef l) {
  GF_SDK_REQUIRE_VOID(ctx, gpg, "GFGpgKeyBriefRelease");
  g->key_brief_release(hctx, l);
}

auto GFGpgSniffRecipients(GFSDKContext* ctx, int channel, GFBufferView in,
                          GFGpgRecipientListRef* out) -> int {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgSniffRecipients", -1);
  return g->sniff_recipients(hctx, channel, in, out);
}

auto GFGpgRecipientCount(GFSDKContext* ctx, GFGpgRecipientListRef l) -> size_t {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgRecipientCount", 0);
  return g->recipient_count(hctx, l);
}

auto GFGpgRecipientAt(GFSDKContext* ctx, GFGpgRecipientListRef l, size_t i)
    -> const GFGpgRecipientRow* {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgRecipientAt", nullptr);
  return g->recipient_at(hctx, l, i);
}

void GFGpgRecipientRelease(GFSDKContext* ctx, GFGpgRecipientListRef l) {
  GF_SDK_REQUIRE_VOID(ctx, gpg, "GFGpgRecipientRelease");
  g->recipient_release(hctx, l);
}

auto GFGpgListAddresses(GFSDKContext* ctx, int channel, int secret_only,
                        GFStringListRef* out) -> int {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgListAddresses", -1);
  return g->list_addresses(hctx, channel, secret_only, out);
}

auto GFStringListCount(GFSDKContext* ctx, GFStringListRef l) -> size_t {
  GF_SDK_REQUIRE(ctx, list, "GFStringListCount", 0);
  return g->string_count(hctx, l);
}

auto GFStringListAt(GFSDKContext* ctx, GFStringListRef l, size_t i) -> const
    char* {
  GF_SDK_REQUIRE(ctx, list, "GFStringListAt", nullptr);
  return g->string_at(hctx, l, i);
}

void GFStringListRelease(GFSDKContext* ctx, GFStringListRef l) {
  GF_SDK_REQUIRE_VOID(ctx, list, "GFStringListRelease");
  g->string_release(hctx, l);
}

auto GFListOutstandingCount(GFSDKContext* ctx) -> size_t {
  GF_SDK_REQUIRE(ctx, list, "GFListOutstandingCount", 0);
  return g->outstanding_count(hctx);
}
