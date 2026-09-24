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

#include "GFModuleRuntimeBoot.h"

#include <QByteArray>
#include <atomic>
#include <cstddef>

#include "GFSDKBuildInfo.h"

namespace gf::runtime {

namespace {

/// Copy a borrowed C string. Null and empty both become an empty QString,
/// which is what every caller here wants to test for anyway.
auto Own(const char* s) -> QString {
  return s == nullptr ? QString() : QString::fromUtf8(s);
}

/// Copy a borrowed, counted array of C strings.
auto OwnList(const char* const* items, size_t size) -> QStringList {
  QStringList out;
  if (items == nullptr) return out;
  out.reserve(static_cast<qsizetype>(size));
  for (size_t i = 0; i < size; ++i) {
    if (items[i] != nullptr) out.append(QString::fromUtf8(items[i]));
  }
  return out;
}

/// Whether a payload written by the host is long enough to contain a field.
///
/// The growth rule for these tables is append-only, so "the host is older than
/// this field" and "this field is present" are the same question asked of
/// struct_size. Asking it is what makes appending safe later.
template <typename T, typename M>
auto Covers(const T* t, M T::* member) -> bool {
  const auto* base = reinterpret_cast<const char*>(t);
  const auto* field = reinterpret_cast<const char*>(&(t->*member));
  return t->struct_size >= static_cast<size_t>(field - base) + sizeof(M);
}

}  // namespace

namespace {

/// The current facts and context. Each activation publishes new ones and the
/// previous ones are deliberately never freed: a thread the module started
/// may still be reading them, and replacing a field in place under it -- a
/// QByteArray reassigned while another thread holds its constData() -- was a
/// use-after-free. A few dozen bytes per activation. Per module, because this
/// archive is linked statically into each module with hidden visibility.
std::atomic<const RuntimeFacts*> g_facts{nullptr};
std::atomic<GFSDKContext*> g_context{nullptr};

}  // namespace

auto Facts() -> const RuntimeFacts& {
  static const RuntimeFacts kNone;
  const auto* facts = g_facts.load();
  return facts == nullptr ? kNone : *facts;
}

void PublishFacts(RuntimeFacts facts) {
  facts.id_utf8 = facts.id.toUtf8();
  g_facts.store(new RuntimeFacts(std::move(facts)));
}

void AdoptHostApi(const GFHostApi* host, const char* module_id) {
  // The same table again -- a reactivation with an unchanged grant -- needs
  // no new context: every thread already holds the right one.
  if (const auto* current = g_context.load();
      current != nullptr && current->host == host) {
    return;
  }

  auto* context = new GFSDKContext{};
  context->struct_size = sizeof(GFSDKContext);
  context->abi_version = GF_SDK_ABI_VERSION;
  context->reserved = 0;
  context->host = host;
  // Diagnostic only. Nothing on either side of the boundary decides anything
  // from it: the host identifies the caller from the token it validates.
  // Owned by the context, and like it, never freed.
  context->module_id =
      (new QByteArray(module_id == nullptr ? "" : module_id))->constData();
  g_context.store(context);
}

auto SdkContext() -> GFSDKContext* { return g_context.load(); }

auto HostApiIsUsable(const GFHostApi* host) -> bool {
  if (host == nullptr) return false;

  // The always-granted groups are what the runtime itself needs, and `list`
  // is the last of them. Checking to the end of it therefore checks that
  // every group the runtime may reach is present in the table as the host
  // compiled it; a module reaching a grantable group asks GFHost() and checks
  // that pointer for itself, because NULL there means "withheld" rather than
  // "too old".
  if (host->struct_size < offsetof(GFHostApi, list) + sizeof(void*)) {
    return false;
  }

  // A table with no context cannot authorize anything, so every call through
  // it would be refused. Better to decline activation than to load a module
  // that can do nothing and cannot say why.
  return host->context != nullptr && host->buffer != nullptr &&
         host->log != nullptr;
}

auto AdoptBootstrapInfo(const GFModuleBootstrapInfo* info,
                        const char* fallback_id, const char* fallback_version,
                        const char* fallback_context) -> RuntimeFacts {
  RuntimeFacts facts;

  // No payload at all: an older host. Everything the module knows then comes
  // from constants compiled into it, which is exactly what `verified` being
  // false records -- and the runtime refuses to activate on that.
  if (info == nullptr) {
    facts.id = Own(fallback_id);
    facts.version = Own(fallback_version);
    facts.translation_context = Own(fallback_context);
    return facts;
  }

  if (Covers(info, &GFModuleBootstrapInfo::flags)) {
    facts.verified = (info->flags & GF_MODULE_BOOT_VERIFIED) != 0U;
  }

  if (Covers(info, &GFModuleBootstrapInfo::module_id)) {
    facts.id = Own(info->module_id);
  }
  if (Covers(info, &GFModuleBootstrapInfo::module_version)) {
    facts.version = Own(info->module_version);
  }
  if (Covers(info, &GFModuleBootstrapInfo::translation_context)) {
    facts.translation_context = Own(info->translation_context);
  }
  if (Covers(info, &GFModuleBootstrapInfo::locale)) {
    facts.locale = Own(info->locale);
  }
  if (Covers(info, &GFModuleBootstrapInfo::capabilities_size)) {
    facts.capabilities = OwnList(info->capabilities, info->capabilities_size);
  }
  if (Covers(info, &GFModuleBootstrapInfo::events_size)) {
    facts.events = OwnList(info->events, info->events_size);
  }
  if (Covers(info, &GFModuleBootstrapInfo::commands_size)) {
    facts.commands = OwnList(info->commands, info->commands_size);
  }

  // A verified payload is the authority; anything it did not carry falls back
  // to the compiled-in constant.
  if (facts.id.isEmpty()) facts.id = Own(fallback_id);
  if (facts.version.isEmpty()) facts.version = Own(fallback_version);
  if (facts.translation_context.isEmpty()) {
    facts.translation_context = Own(fallback_context);
  }

  return facts;
}

}  // namespace gf::runtime
