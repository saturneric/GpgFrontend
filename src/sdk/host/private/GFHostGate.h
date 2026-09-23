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

#include "GFSDKHostApi.h"
#include "private/GFHostAttribution.h"
#include "private/GFHostContext.h"

namespace gf_sdk_internal {

/**
 * @brief The gate every thunk opens with: authorize, count, attribute.
 *
 * One object rather than separate statements so they cannot be separated: a
 * thunk that checked the capability and forgot the attribution would work
 * perfectly and quietly stop recording who owns the handles it creates. The
 * call stays counted until the thunk returns, which is what lets shutdown
 * wait for it before freeing the module's handles.
 *
 * `capability` 0 means "a live context is enough", which is the whole question
 * for the always-granted groups.
 */
class Gate {
 public:
  Gate(GFHostContextRef ctx, uint32_t capability, const char* entry_point)
      : ticket_(BeginCall(ctx, capability, entry_point)),
        attribution_(ticket_.attribution) {}

  ~Gate() { EndCall(ticket_); }

  Gate(const Gate&) = delete;
  auto operator=(const Gate&) -> Gate& = delete;

  explicit operator bool() const { return ticket_.record != nullptr; }

 private:
  CallTicket ticket_;
  ScopedContextAttribution attribution_;
};

/// Shorthand: `GATE(ctx, GF_HOST_CAP_GPG, "gpg.sign", -1);`
#define GATE(ctx, cap, name, failure)    \
  const ::gf_sdk_internal::Gate gate((ctx), (cap), (name)); \
  if (!gate) return failure;

/// The void-returning form, which cannot use `return failure`.
#define GATE_VOID(ctx, cap, name)        \
  const ::gf_sdk_internal::Gate gate((ctx), (cap), (name)); \
  if (!gate) return;

/// The command group, defined in GFHostCommand.cpp.
extern const GFHostCommandApi kCommandApi;

/// The script group, defined in GFHostScript.cpp.
extern const GFHostScriptApi kScriptApi;

/// The native widget group, defined in GFHostNative.cpp.
extern const GFHostNativeWidgetApi kNativeApi;

}  // namespace gf_sdk_internal
