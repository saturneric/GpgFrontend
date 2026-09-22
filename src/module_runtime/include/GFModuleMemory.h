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

#include <GFSDKBuffer.h>
#include <GFSDKContext.h>

#include <QByteArray>
#include <QSharedPointer>
#include <QString>
#include <cstring>
#include <memory>
#include <new>

/**
 * @file GFModuleMemory.h
 * @brief Who frees what, and the helpers that make it obvious.
 *
 * ## The rule, in one place
 *
 * **SDK arguments are borrowed.** Pass a pointer straight through --
 * `s.toUtf8().constData()` for a QString -- and keep owning it. Do *not*
 * pre-allocate one to hand over: nothing on the other side will free it. That
 * is what the old `DUP()`/`QDUP()` at call sites was for, and why doing it now
 * simply leaks.
 *
 * The one exception is a struct rather than an argument: the few aggregates a
 * module builds and hands over whole -- `GFModuleEvent`, `GFModuleEventParam`,
 * `GFModuleMetaData`, `GFCommandExecuteContext` -- still transfer, so their
 * `char*` members are still allocated with GFModuleStrDup.
 *
 * **SDK return values are owned.** Reclaim them, which is what the `U...`
 * family below is for.
 */

/// This module's SDK context, from the runtime. Declared here because this
/// header is included before GFModule.h declares it.
auto GFModuleSdkContext() -> GFSDKContext*;

/// Allocate a copy of @p v that a transferring struct will own.
///
/// The context comes from the runtime rather than being spelled at every call
/// site: it is one pointer per module, fixed after activation, and threading
/// it through 154 DUP() calls would be noise. The SDK itself still holds no
/// state -- this is the runtime's context, passed explicitly, one level down.
#define DUP(v) GFMemStrDup(GFModuleSdkContext(), GF_ARENA_NORMAL, (v))

/// The same, from the wiping allocator, for a value that is a secret.
#define SECDUP(v) GFMemStrDup(GFModuleSdkContext(), GF_ARENA_SECURE, (v))

/// Take ownership of an SDK string and return it as a QString.
#define UDUP(v) UnStrDup(v)

/// The same, releasing it back to the wiping allocator.
#define USECDUP(v) UnSecStrDup(v)

/// Take ownership of a sized SDK buffer and return its exact octets.
#define UDUPN(v, n) UnBytesDup(v, n)

/// Takes ownership of a sized SDK buffer and returns its octets verbatim.
///
/// The QString forms below decode UTF-8 and stop at the first NUL, which is
/// right for identifiers and error strings and wrong for message data: a MIME
/// entity may be 8bit or binary, and a signature covers exact octets. Anything
/// that crosses the crypto boundary must use this and the matching @c *N SDK
/// entry points instead.
inline auto UnBytesDup(const char* s, size_t size) -> QByteArray {
  if (s == nullptr) return {};
  QByteArray bytes(s, static_cast<qsizetype>(size));
  GFMemFree(GFModuleSdkContext(), GF_ARENA_NORMAL,
            static_cast<void*>(const_cast<char*>(s)));
  return bytes;
}

inline auto UnStrDup(const char* s) -> QString {
  auto q_s = QString::fromUtf8(s == nullptr ? "" : s);
  if (s != nullptr)
    GFMemFree(GFModuleSdkContext(), GF_ARENA_NORMAL,
              static_cast<void*>(const_cast<char*>(s)));
  return q_s;
}

inline auto UnSecStrDup(const char* s) -> QString {
  auto q_s = QString::fromUtf8(s == nullptr ? "" : s);
  if (s != nullptr)
    GFMemFree(GFModuleSdkContext(), GF_ARENA_SECURE,
              static_cast<void*>(const_cast<char*>(s)));
  return q_s;
}

inline auto QStrDup(const QString& str) -> char* { return DUP(str.toUtf8()); }

inline auto QSecStrDup(const QString& str) -> char* {
  return SECDUP(str.toUtf8());
}

/// Copies @p b into an SDK buffer, NUL-terminated one past the end.
///
/// The terminator is not optional politeness: the buffer is handed to the
/// host, and a caller that treats it as a C string would otherwise read past
/// the allocation. @p b .size() stays authoritative for the byte count.
inline auto AllocBufferAndCopy(const QByteArray& b) -> char* {
  auto* p = static_cast<char*>(GFMemAlloc(GFModuleSdkContext(), GF_ARENA_NORMAL,
                                          sizeof(char) * (b.size() + 1)));
  if (p == nullptr) return nullptr;
  memcpy(p, b.constData(), b.size());
  p[b.size()] = '\0';
  return p;
}

template <typename T>
class PointerConverter {
 public:
  explicit PointerConverter(void* ptr) : ptr_(ptr) {}

  auto AsType() const -> T* { return static_cast<T*>(ptr_); }

 private:
  void* ptr_;
};

/**
 * @brief Construct a T in SDK-allocated memory, freed through the SDK.
 *
 * NAMING. These four used to be called `Secure...`, which said the wrong
 * thing: they allocate from the ORDINARY SDK arena (GFAllocateMemory), not the
 * wiping one (GFSecAllocateMemory). The old names had a comment saying so,
 * which is the weakest possible place to put it -- a name is read every time
 * and a comment once. For anything that must actually be erased, use the
 * secure entry points explicitly or a GFBuf handle.
 */
template <typename T, typename... Args>
auto SdkCreateSharedObject(Args&&... args) -> std::shared_ptr<T> {
  void* mem = GFMemAlloc(GFModuleSdkContext(), GF_ARENA_NORMAL, sizeof(T));
  if (!mem) throw std::bad_alloc();

  try {
    T* obj = new (mem) T(std::forward<Args>(args)...);
    return std::shared_ptr<T>(obj, [](T* ptr) {
      ptr->~T();
      GFMemFree(GFModuleSdkContext(), GF_ARENA_NORMAL, ptr);
    });
  } catch (...) {
    GFMemFree(GFModuleSdkContext(), GF_ARENA_NORMAL, mem);
    throw;
  }
}

template <typename T, typename... Args>
auto SdkCreateQSharedObject(Args&&... args) -> QSharedPointer<T> {
  void* mem = GFMemAlloc(GFModuleSdkContext(), GF_ARENA_NORMAL, sizeof(T));
  if (!mem) throw std::bad_alloc();

  try {
    T* obj = new (mem) T(std::forward<Args>(args)...);
    return QSharedPointer<T>(obj, [](T* ptr) {
      ptr->~T();
      GFMemFree(GFModuleSdkContext(), GF_ARENA_NORMAL, ptr);
    });
  } catch (...) {
    GFMemFree(GFModuleSdkContext(), GF_ARENA_NORMAL, mem);
    throw;
  }
}

/// Allocate @p size bytes from the ordinary SDK arena, typed.
template <typename T>
auto SdkMallocAsType(std::size_t size) -> T* {
  return PointerConverter<T>(
             GFMemAlloc(GFModuleSdkContext(), GF_ARENA_NORMAL, size))
      .AsType();
}

/// Resize an SDK allocation, typed.
template <typename T>
auto SdkReallocAsType(T* ptr, std::size_t size) -> T* {
  return PointerConverter<T>(
             GFMemRealloc(GFModuleSdkContext(), GF_ARENA_NORMAL, ptr, size))
      .AsType();
}
