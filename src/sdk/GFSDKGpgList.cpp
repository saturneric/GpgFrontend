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

#include <QByteArray>
#include <QList>
#include <QString>
#include <cstdint>
#include <new>

#include "GFSDKBasic.h"
#include "GFSDKGpg.h"
#include "core/model/GFBuffer.h"
#include "core/utils/MemoryUtils.h"
#include "private/GFSDKHandleRegistry.h"
#include "private/GFSDKPrivat.h"

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
};

struct GFGpgRecipientListImpl {
  uint32_t magic = kGFListMagic;
  QList<RecipientRow> rows;
};

struct GFStringListImpl {
  uint32_t magic = kGFListMagic;
  QList<QByteArray> rows;
};

namespace {

template <typename T>
using Reg = GFHandleRegistry<T>;

void ReportStaleList(const void* handle, const char* what) {
  LOG_W() << "GFSDKGpgList:" << what
          << "called on a handle that is not live (already released, or never "
             "issued by this SDK):"
          << handle;
#ifdef DEBUG
  qFatal("GFSDKGpgList: %s on stale handle %p", what, handle);
#endif
}

/// Registry first, dereference second -- never the other way round.
template <typename T>
auto ResolveLive(T* l, const char* what) -> T* {
  if (l == nullptr) return nullptr;
  if (!Reg<T>::Instance().IsLive(l)) {
    ReportStaleList(l, what);
    return nullptr;
  }
  Q_ASSERT(l->magic == kGFListMagic);
  return l;
}

template <typename T>
auto NewList() -> T* {
  auto* mem = GpgFrontend::SMAMalloc(sizeof(T));
  if (mem == nullptr) return nullptr;

  auto* impl = new (mem) T{};
  Reg<T>::Instance().Register(impl, QString());
  return impl;
}

template <typename T>
void ReleaseList(T* l, const char* what) {
  if (l == nullptr) return;
  if (!Reg<T>::Instance().Take(l)) {
    ReportStaleList(l, what);
    return;
  }
  l->magic = 0;
  l->~T();
  GpgFrontend::SMAFree(l);
}

/// Bounds-checked row access. An out-of-range index is a caller bug, but it
/// yields an empty string rather than undefined behaviour.
template <typename L>
auto RowAt(L* l, size_t i) -> decltype(&l->rows[0]) {
  if (l == nullptr) return nullptr;
  if (i >= static_cast<size_t>(l->rows.size())) return nullptr;
  return &l->rows[static_cast<qsizetype>(i)];
}

const char* const kEmpty = "";

}  // namespace

/* --- key briefs ---------------------------------------------------------- */

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

    auto* impl = NewList<GFGpgKeyBriefListImpl>();
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

#define GF_BRIEF_STR(fn, member)                              \
  auto fn(GFGpgKeyBriefListRef l, size_t i) -> const char* {  \
    const auto* row = RowAt(ResolveLive(l, #fn), i);          \
    return row == nullptr ? kEmpty : row->member.constData(); \
  }

GF_BRIEF_STR(GFGpgKeyBriefFingerprint, fingerprint)
GF_BRIEF_STR(GFGpgKeyBriefKeyId, key_id)
GF_BRIEF_STR(GFGpgKeyBriefUid, uid)
GF_BRIEF_STR(GFGpgKeyBriefMatchedEmail, matched_email)
#undef GF_BRIEF_STR

#define GF_BRIEF_NUM(fn, member, type)                \
  auto fn(GFGpgKeyBriefListRef l, size_t i) -> type { \
    const auto* row = RowAt(ResolveLive(l, #fn), i);  \
    return row == nullptr ? 0 : row->member;          \
  }

GF_BRIEF_NUM(GFGpgKeyBriefExpiresAt, expires_at, int64_t)
GF_BRIEF_NUM(GFGpgKeyBriefUsability, usability, int)
GF_BRIEF_NUM(GFGpgKeyBriefCanEncrypt, can_encrypt, int)
GF_BRIEF_NUM(GFGpgKeyBriefCanSign, can_sign, int)
GF_BRIEF_NUM(GFGpgKeyBriefMatchedUidIsPrimary, matched_uid_is_primary, int)
GF_BRIEF_NUM(GFGpgKeyBriefMatchedUidRevoked, matched_uid_revoked, int)
#undef GF_BRIEF_NUM

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

    auto* impl = NewList<GFGpgRecipientListImpl>();
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

#define GF_RCPT_STR(fn, member)                               \
  auto fn(GFGpgRecipientListRef l, size_t i) -> const char* { \
    const auto* row = RowAt(ResolveLive(l, #fn), i);          \
    return row == nullptr ? kEmpty : row->member.constData(); \
  }

GF_RCPT_STR(GFGpgRecipientKeyId, key_id)
GF_RCPT_STR(GFGpgRecipientPubAlgo, pub_algo)
GF_RCPT_STR(GFGpgRecipientFingerprint, fingerprint)
GF_RCPT_STR(GFGpgRecipientUid, uid)
#undef GF_RCPT_STR

#define GF_RCPT_NUM(fn, member)                       \
  auto fn(GFGpgRecipientListRef l, size_t i) -> int { \
    const auto* row = RowAt(ResolveLive(l, #fn), i);  \
    return row == nullptr ? 0 : row->member;          \
  }

GF_RCPT_NUM(GFGpgRecipientKeyFound, key_found)
GF_RCPT_NUM(GFGpgRecipientHasSecret, has_secret)
GF_RCPT_NUM(GFGpgRecipientHidden, hidden)
#undef GF_RCPT_NUM

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

    auto* impl = NewList<GFStringListImpl>();
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

auto GFGpgListOutstandingCount(const char* module_id) -> size_t {
  const auto id =
      module_id == nullptr ? QString() : QString::fromUtf8(module_id);
  return Reg<GFGpgKeyBriefListImpl>::Instance().Count(id) +
         Reg<GFGpgRecipientListImpl>::Instance().Count(id) +
         Reg<GFStringListImpl>::Instance().Count(id);
}
