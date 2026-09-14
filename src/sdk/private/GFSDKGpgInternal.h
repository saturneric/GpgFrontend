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

#include <gpgme.h>
#include <stddef.h>

#include "GFSDKGpg.h"

/**
 * @file GFSDKGpgInternal.h
 * @brief Gatherers that are no longer part of the module-facing SDK.
 *
 * These used to be public entry points. They are kept because the key-matching
 * and address-deduplication rules inside them are subtle -- every UID rather
 * than only the primary, revoked UIDs excluded, deduplication on the address
 * alone -- and are covered by existing tests. Moving that logic would have
 * risked those behaviours for no gain.
 *
 * What DID change is that they are no longer exported: modules reach this
 * functionality through the opaque list handles in GFSDKGpgList.h, which have
 * one release per list and borrowed element accessors, rather than through a
 * caller-allocated array plus a matching free-the-array function plus a count
 * the caller has to carry around.
 *
 * Internal to gf_sdk. Not installed, not for modules.
 */

auto GFGpgFindKeysByEmail(int channel, const char* email, GFGpgKeyBrief** keys,
                          int* count) -> int;
void GFGpgFreeKeyBriefs(GFGpgKeyBrief* keys, int count);

auto GFGpgSniffEncryptedRecipients(int channel, const char* data, int size,
                                   GFGpgEncRecipient** out, int* count) -> int;
void GFGpgFreeEncRecipients(GFGpgEncRecipient* out, int count);

auto GFGpgListKeyAddresses(int channel, int secret_only, char*** addresses,
                           int* count) -> int;
void GFGpgFreeStringArray(char** strings, int count);
