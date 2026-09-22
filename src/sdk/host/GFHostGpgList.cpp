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

#include <QByteArray>
#include <QList>
#include <QString>
#include <cstdint>
#include <new>

#include "GFHostImpl.h"
#include "core/model/GFBuffer.h"
#include "core/utils/MemoryUtils.h"
#include "private/GFHostContext.h"
#include "private/GFSDKGpgInternal.h"
#include "private/GFSDKHandleRegistry.h"
#include "private/GFSDKHandleSweep.h"
#include "private/GFSDKPrivate.h"

namespace {

constexpr uint32_t kGFListMagic = 0x47464C53U;  // 'GFLS'

/// One key brief, held as real C++ members.
///
/// The whole point of the rework: nothing in here is separately allocated, so
/// there is nothing for a caller to free field by field and therefore nothing
/// to forget. The accessors hand out pointers INTO these members.
struct KeyBriefRow {
  QByteArray fingerprint;
  QByteArray key_id;
  QByteArray uid;
  QByteArray matched_email;
  int64_t expires_at = 0;
  int usability = 0;
  int can_encrypt = 0;
  int can_sign = 0;
  int matched_uid_is_primary = 0;
  int matched_uid_revoked = 0;
};

struct RecipientRow {
  QByteArray key_id;
  QByteArray pub_algo;
  QByteArray fingerprint;
  QByteArray uid;
  int key_found = 0;
  int has_secret = 0;
  int hidden = 0;
};

}  // namespace

struct GFGpgKeyBriefListImpl {
  uint32_t magic = kGFListMagic;
  QList<KeyBriefRow> rows;
  /// The same rows as borrowed C structs, built once after @ref rows is
  /// complete. The host api hands a module one of these instead of eleven
  /// accessors; it points into @ref rows and dies with the list, which is the
  /// ownership rule the accessors already had.
  QList<GFGpgKeyBriefRow> views;
};

struct GFGpgRecipientListImpl {
  uint32_t magic = kGFListMagic;
  QList<RecipientRow> rows;
  QList<GFGpgRecipientRow> views;
};

struct GFStringListImpl {
  uint32_t magic = kGFListMagic;
  QList<QByteArray> rows;
};

namespace gf_host {

namespace {

template <typename T>
using Reg = GFHandleRegistry<T>;

void ReportStaleList(const void* handle, const char* what) {
  LOG_W().nospace()
      << what
      << ": stale handle (already released, or never issued by the host): "
      << handle;
#ifdef DEBUG
  qFatal("%s: stale handle %p", what, handle);
#endif
}

/// Registry first, dereference second -- never the other way round.
template <typename T>
auto ResolveLive(T* l, const char* what) -> T* {
  if (l == nullptr) return nullptr;
  const auto state = Reg<T>::Instance().Check(l, what);
  if (state != GFHandleState::kLive) {
    if (state == GFHandleState::kStale) ReportStaleList(l, what);
    return nullptr;
  }
  Q_ASSERT(l->magic == kGFListMagic);
  return l;
}

template <typename T>
auto NewList(const char* origin) -> T* {
  auto* mem = GpgFrontend::SMAMalloc(sizeof(T));
  if (mem == nullptr) return nullptr;

  auto* impl = new (mem) T{};
  Reg<T>::Instance().Register(impl, gf_sdk_internal::CurrentModuleId(), origin);
  return impl;
}

/// Free one list handle. The single place that knows how, so release and the
/// unload sweep cannot diverge about what destruction means.
template <typename T>
void DestroyList(T* l) {
  l->magic = 0;
  l->~T();
  GpgFrontend::SMAFree(l);
}

template <typename T>
void ReleaseList(T* l, const char* what) {
  if (l == nullptr) return;
  const auto state = Reg<T>::Instance().Take(l, what);
  if (state != GFHandleState::kLive) {
    if (state == GFHandleState::kStale) ReportStaleList(l, what);
    return;
  }
  DestroyList(l);
}

/// Reclaim one list type's outstanding handles, appending what issued each.
template <typename T>
void SweepOneListType(const QString& module_id, QList<const char*>& origins) {
  for (const auto& entry : Reg<T>::Instance().TakeAllFor(module_id)) {
    origins.append(entry.second);
    DestroyList(entry.first);
  }
}

/// Bounds-checked row access. An out-of-range index is a caller bug, but it
/// yields an empty string rather than undefined behavior.
template <typename L>
auto RowAt(L* l, size_t i) -> decltype(&l->rows[0]) {
  if (l == nullptr) return nullptr;
  if (i >= static_cast<size_t>(l->rows.size())) return nullptr;
  return &l->rows[static_cast<qsizetype>(i)];
}

const char* const kEmpty = "";

}  // namespace

/* --- key briefs ---------------------------------------------------------- */

namespace {

/// Build the borrowed row views. Called once, AFTER rows is final: a later
/// append would reallocate the QList and leave every view pointing at a moved
/// element.
void BuildViews(GFGpgKeyBriefListImpl* impl) {
  impl->views.reserve(impl->rows.size());
  for (const auto& r : impl->rows) {
    GFGpgKeyBriefRow view{};
    view.struct_size = sizeof(GFGpgKeyBriefRow);
    view.fingerprint = r.fingerprint.constData();
    view.key_id = r.key_id.constData();
    view.uid = r.uid.constData();
    view.matched_email = r.matched_email.constData();
    view.expires_at = r.expires_at;
    view.usability = r.usability;
    view.can_encrypt = r.can_encrypt;
    view.can_sign = r.can_sign;
    view.matched_uid_is_primary = r.matched_uid_is_primary;
    view.matched_uid_revoked = r.matched_uid_revoked;
    impl->views.append(view);
  }
}

void BuildViews(GFGpgRecipientListImpl* impl) {
  impl->views.reserve(impl->rows.size());
  for (const auto& r : impl->rows) {
    GFGpgRecipientRow view{};
    view.struct_size = sizeof(GFGpgRecipientRow);
    view.key_id = r.key_id.constData();
    view.pub_algo = r.pub_algo.constData();
    view.fingerprint = r.fingerprint.constData();
    view.uid = r.uid.constData();
    view.key_found = r.key_found;
    view.has_secret = r.has_secret;
    view.hidden = r.hidden;
    impl->views.append(view);
  }
}

}  // namespace

auto GFGpgFindKeys(int channel, const char* email, GFGpgKeyBriefListRef* out)
    -> int {
  if (out == nullptr) return -1;
  *out = nullptr;
  if (email == nullptr) return -1;

  try {
    // Reuses the existing gatherer rather than duplicating the UID-matching
    // rules, which are subtle (every UID, not just the primary) and already
    // covered by tests.
    GFGpgKeyBrief* briefs = nullptr;
    int count = 0;
    if (GFGpgFindKeysByEmail(channel, email, &briefs, &count) != 0) return -1;

    auto* impl = NewList<GFGpgKeyBriefListImpl>("GFGpgFindKeys");
    if (impl == nullptr) {
      GFGpgFreeKeyBriefs(briefs, count);
      return -1;
    }

    for (int i = 0; i < count; ++i) {
      const auto& b = briefs[i];
      impl->rows.append(KeyBriefRow{
          b.fingerprint == nullptr ? QByteArray() : QByteArray(b.fingerprint),
          b.key_id == nullptr ? QByteArray() : QByteArray(b.key_id),
          b.uid == nullptr ? QByteArray() : QByteArray(b.uid),
          b.matched_email == nullptr ? QByteArray()
                                     : QByteArray(b.matched_email),
          b.expires_at, b.usability, b.can_encrypt, b.can_sign,
          b.matched_uid_is_primary, b.matched_uid_revoked});
    }

    GFGpgFreeKeyBriefs(briefs, count);
    BuildViews(impl);
    *out = impl;
    return 0;
  } catch (...) {
    LOG_E() << "GFGpgFindKeys: unexpected exception";
    return -1;
  }
}

auto GFGpgKeyBriefListCount(GFGpgKeyBriefListRef l) -> size_t {
  auto* impl = ResolveLive(l, "GFGpgKeyBriefListCount");
  return impl == nullptr ? 0 : static_cast<size_t>(impl->rows.size());
}

void GFGpgKeyBriefListRelease(GFGpgKeyBriefListRef l) {
  ReleaseList(l, "GFGpgKeyBriefListRelease");
}

/* --- recipients ---------------------------------------------------------- */

auto GFGpgSniffRecipients(int channel, GFBufferView in,
                          GFGpgRecipientListRef* out) -> int {
  if (out == nullptr) return -1;
  *out = nullptr;

  const auto* data = static_cast<const char*>(GFBufferData(in));
  const auto size = GFBufferSize(in);
  if (data == nullptr || size == 0) return -1;

  try {
    GFGpgEncRecipient* raw = nullptr;
    int count = 0;
    if (GFGpgSniffEncryptedRecipients(channel, data, static_cast<int>(size),
                                      &raw, &count) != 0) {
      return -1;
    }

    auto* impl = NewList<GFGpgRecipientListImpl>("GFGpgSniffRecipients");
    if (impl == nullptr) {
      GFGpgFreeEncRecipients(raw, count);
      return -1;
    }

    for (int i = 0; i < count; ++i) {
      const auto& r = raw[i];
      impl->rows.append(RecipientRow{
          r.key_id == nullptr ? QByteArray() : QByteArray(r.key_id),
          r.pub_algo == nullptr ? QByteArray() : QByteArray(r.pub_algo),
          r.fingerprint == nullptr ? QByteArray() : QByteArray(r.fingerprint),
          r.uid == nullptr ? QByteArray() : QByteArray(r.uid), r.key_found,
          r.has_secret, r.hidden});
    }

    GFGpgFreeEncRecipients(raw, count);
    BuildViews(impl);
    *out = impl;
    return 0;
  } catch (...) {
    LOG_E() << "GFGpgSniffRecipients: unexpected exception";
    return -1;
  }
}

auto GFGpgRecipientListCount(GFGpgRecipientListRef l) -> size_t {
  auto* impl = ResolveLive(l, "GFGpgRecipientListCount");
  return impl == nullptr ? 0 : static_cast<size_t>(impl->rows.size());
}

void GFGpgRecipientListRelease(GFGpgRecipientListRef l) {
  ReleaseList(l, "GFGpgRecipientListRelease");
}

/* --- string lists --------------------------------------------------------- */

auto GFGpgListAddresses(int channel, int secret_only, GFStringListRef* out)
    -> int {
  if (out == nullptr) return -1;
  *out = nullptr;

  try {
    char** raw = nullptr;
    int count = 0;
    if (GFGpgListKeyAddresses(channel, secret_only, &raw, &count) != 0) {
      return -1;
    }

    auto* impl = NewList<GFStringListImpl>("GFGpgListAddresses");
    if (impl == nullptr) {
      GFGpgFreeStringArray(raw, count);
      return -1;
    }

    for (int i = 0; i < count; ++i) {
      impl->rows.append(raw[i] == nullptr ? QByteArray() : QByteArray(raw[i]));
    }

    GFGpgFreeStringArray(raw, count);
    *out = impl;
    return 0;
  } catch (...) {
    LOG_E() << "GFGpgListAddresses: unexpected exception";
    return -1;
  }
}

auto GFStringListCount(GFStringListRef l) -> size_t {
  auto* impl = ResolveLive(l, "GFStringListCount");
  return impl == nullptr ? 0 : static_cast<size_t>(impl->rows.size());
}

auto GFStringListAt(GFStringListRef l, size_t i) -> const char* {
  const auto* row = RowAt(ResolveLive(l, "GFStringListAt"), i);
  return row == nullptr ? kEmpty : row->constData();
}

void GFStringListRelease(GFStringListRef l) {
  ReleaseList(l, "GFStringListRelease");
}

}  // namespace gf_host

// The file-local helpers above are inside gf_host; these definitions are
// not, because their declarations are at global scope.
using namespace gf_host;  // NOLINT(build/namespaces)

namespace gf_sdk_internal {

auto SweepListHandles(const QString& module_id) -> QList<const char*> {
  QList<const char*> origins;
  SweepOneListType<GFGpgKeyBriefListImpl>(module_id, origins);
  SweepOneListType<GFGpgRecipientListImpl>(module_id, origins);
  SweepOneListType<GFStringListImpl>(module_id, origins);
  return origins;
}

}  // namespace gf_sdk_internal

namespace gf_host {

auto GFGpgListOutstandingCount(const char* module_id) -> size_t {
  const auto id =
      module_id == nullptr ? QString() : QString::fromUtf8(module_id);
  return Reg<GFGpgKeyBriefListImpl>::Instance().Count(id) +
         Reg<GFGpgRecipientListImpl>::Instance().Count(id) +
         Reg<GFStringListImpl>::Instance().Count(id);
}

}  // namespace gf_host

namespace gf_sdk_internal {

auto NewStringList(const QList<QString>& values, GFStringListRef* out) -> int {
  if (out == nullptr) return -1;
  try {
    auto* impl = NewList<GFStringListImpl>("NewStringList");
    if (impl == nullptr) return -1;
    impl->rows.reserve(values.size());
    for (const auto& v : values) impl->rows.append(v.toUtf8());
    *out = impl;
    return 0;
  } catch (...) {
    LOG_E() << "NewStringList: unexpected exception";
    return -1;
  }
}

auto KeyBriefRowAt(GFGpgKeyBriefListRef l, size_t i)
    -> const GFGpgKeyBriefRow* {
  auto* impl = ResolveLive(l, "KeyBriefRowAt");
  if (impl == nullptr) return nullptr;
  if (i >= static_cast<size_t>(impl->views.size())) return nullptr;
  return &impl->views[static_cast<qsizetype>(i)];
}

auto RecipientRowAt(GFGpgRecipientListRef l, size_t i)
    -> const GFGpgRecipientRow* {
  auto* impl = ResolveLive(l, "RecipientRowAt");
  if (impl == nullptr) return nullptr;
  if (i >= static_cast<size_t>(impl->views.size())) return nullptr;
  return &impl->views[static_cast<qsizetype>(i)];
}

}  // namespace gf_sdk_internal