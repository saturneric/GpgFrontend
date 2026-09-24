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
#include <QStringList>
#include <cstdint>
#include <new>

#include "GFHostImpl.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/model/DataObject.h"
#include "core/model/GFBuffer.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"
#include "private/GFHostContext.h"
#include "private/GFSDKHandleRegistry.h"
#include "private/GFSDKHandleSweep.h"
#include "private/GFSDKPrivate.h"

namespace {

constexpr uint32_t kGFGpgResultMagic = 0x47465250U;  // 'GFRP'

}  // namespace

/**
 * @brief What a GFGpgResultRef points at.
 *
 * Every field is a real C++ member, not a char* the caller has to reclaim.
 * That is the whole point: there is nothing here to free individually, so
 * there is no per-field free to forget. Accessors hand out borrowed pointers
 * into these members, which stay valid exactly as long as the result does.
 */
struct GFGpgResultImpl {
  uint32_t magic = kGFGpgResultMagic;

  GFGpgResultStatus status = GF_GPG_OK;
  uint32_t gpgme_error = 0;

  /// Owned. Released with the result unless GFGpgResultTakeData took it.
  GFBufferRef data = nullptr;

  // QByteArray keeps its own NUL terminator, so constData() is a valid C
  // string and the accessors never need to allocate.
  QByteArray capsule_id;
  QByteArray error_string;
  QByteArray hash_algo;
};

namespace gf_host {

namespace {

using ResultRegistry = GFHandleRegistry<GFGpgResultImpl>;

/// The engine's full result model behind one capsule id, for GFGpgAnalyse().
///
/// Deliberately NOT part of the result handle: the SDK contract is that a
/// capsule id stays analysable after its result is released -- callers copy
/// the id, let the result go, and analyse later -- and it is consumed by the
/// first analysis. It belongs to the module the operation ran for, is refused
/// to any other, is capped per module, and goes with the module's handles in
/// the shutdown sweep.
struct Capsule {
  QString owner;
  std::any model;
  quint64 serial = 0;  ///< insertion order, for the per-module cap
};

/// Capsules one module may hold unanalysed. A module that never analyses what
/// it asks for loses its oldest, rather than growing the table for ever.
constexpr int kMaxCapsulesPerModule = 64;

struct CapsuleIndex {
  QMutex mutex;
  QHash<QByteArray, Capsule> by_id;
  quint64 next_serial = 0;
};

auto Capsules() -> CapsuleIndex& {
  static CapsuleIndex index;
  return index;
}

void ReportStaleResult(const void* handle, const char* what) {
  ResultRegistry::ReportStale(handle, what);
}

auto ResolveLive(GFGpgResultRef r, const char* what) -> GFGpgResultImpl* {
  auto* impl = ResultRegistry::Instance().ResolveLive(r, what);
  Q_ASSERT(impl == nullptr || impl->magic == kGFGpgResultMagic);
  return impl;
}

/// Wipe and free one result handle. The single place that knows how, so
/// release and the unload sweep cannot diverge about what destruction means.
void DestroyResult(GFGpgResultImpl* impl) {
  // The nested buffer goes through its own release, so it leaves the buffer
  // registry rather than being destroyed behind its back -- which is also
  // what stops the buffer sweep finding it again afterwards.
  GFBufferRelease(impl->data);
  impl->magic = 0;
  impl->~GFGpgResultImpl();
  GpgFrontend::SMAFree(impl);
}

/// Allocate a live result. Returns nullptr only when memory is exhausted.
auto NewResult(const char* origin) -> GFGpgResultImpl* {
  auto* mem = GpgFrontend::SMAMalloc(sizeof(GFGpgResultImpl));
  if (mem == nullptr) return nullptr;

  auto* impl = new (mem) GFGpgResultImpl{};
  ResultRegistry::Instance().Register(impl, gf_sdk_internal::CurrentModuleId(),
                                      origin);
  return impl;
}

/// The arguments were unusable, so nothing was attempted. The caller still
/// gets an owned result carrying the reason -- the contract says a
/// non-negative return always means there is something to release.
auto FailRequest(GFGpgResultRef* out, const char* why, const char* origin)
    -> int {
  auto* impl = NewResult(origin);
  if (impl == nullptr) {
    *out = nullptr;
    return -1;
  }
  impl->status = GF_GPG_BAD_REQUEST;
  impl->error_string = why;
  *out = impl;
  return GF_GPG_BAD_REQUEST;
}

/// A gpg-level failure. Same rule: an owned result comes back either way.
auto FailOperation(GFGpgResultImpl* impl, GpgFrontend::GFError err,
                   GFGpgResultRef* out) -> int {
  impl->status = GF_GPG_OP_FAILED;
  impl->gpgme_error = static_cast<uint32_t>(err);
  impl->error_string = GpgFrontend::DescribeGpgErrCode(err).second.toUtf8();
  *out = impl;
  return GF_GPG_OP_FAILED;
}

/// Borrowed view -> a core GFBuffer, copying the octets verbatim.
auto ViewToBuffer(GFBufferView v) -> GpgFrontend::GFBuffer {
  const auto* data = static_cast<const char*>(GFBufferData(v));
  const auto size = GFBufferSize(v);
  if (data == nullptr || size == 0) return {};
  return GpgFrontend::GFBuffer{data, size};
}

auto KeysOf(int channel, const char* const* key_ids, size_t key_ids_size)
    -> GpgFrontend::GpgAbstractKeyPtrList {
  GpgFrontend::GpgAbstractKeyPtrList keys;
  if (key_ids == nullptr) return keys;

  for (size_t i = 0; i < key_ids_size; ++i) {
    if (key_ids[i] == nullptr) continue;
    auto key = GpgFrontend::GpgKeyRepository::GetInstance(channel).GetKeyPtr(
        QString::fromUtf8(key_ids[i]));
    if (key != nullptr) keys.push_back(key);
  }
  return keys;
}

/// Fill the success fields shared by every operation.
template <typename ResultT>
void Finish(GFGpgResultImpl* impl, const ResultT& result,
            const GpgFrontend::GFBuffer& out_buffer, GpgFrontend::GFError err) {
  impl->status = GF_GPG_OK;
  impl->gpgme_error = static_cast<uint32_t>(err);
  impl->capsule_id = QUuid::createUuid().toString().toUtf8();
  {
    const auto owner = gf_sdk_internal::CurrentModuleId();
    auto& index = Capsules();
    const QMutexLocker lock(&index.mutex);
    index.by_id.insert(impl->capsule_id,
                       Capsule{owner, std::any(result), index.next_serial++});

    // The cap: this module's oldest unanalysed capsule goes first.
    QByteArray oldest;
    quint64 oldest_serial = 0;
    int held = 0;
    for (auto it = index.by_id.cbegin(); it != index.by_id.cend(); ++it) {
      if (it->owner != owner) continue;
      ++held;
      if (oldest.isEmpty() || it->serial < oldest_serial) {
        oldest = it.key();
        oldest_serial = it->serial;
      }
    }
    if (held > kMaxCapsulesPerModule) index.by_id.remove(oldest);
  }
  impl->error_string = GpgFrontend::DescribeGpgErrCode(err).second.toUtf8();
  impl->data = GFBufferNewFromBytes(out_buffer.Data(), out_buffer.Size());
}

}  // namespace

auto TakeResultModel(const char* capsule_id) -> std::any {
  if (capsule_id == nullptr) return {};
  const auto caller = gf_sdk_internal::CurrentModuleId();
  auto& index = Capsules();
  const QMutexLocker lock(&index.mutex);
  const auto it = index.by_id.find(QByteArray(capsule_id));
  if (it == index.by_id.end()) return {};
  // Only the module the operation ran for. An id is unguessable, but it is
  // also handed around -- in event answers, in logs -- and knowing one must
  // not be enough to read another module's result.
  if (!caller.isEmpty() && !it->owner.isEmpty() && it->owner != caller) {
    LOG_W() << "gpg.analyse_result: module" << caller
            << "presented a capsule issued to module" << it->owner
            << "; refused";
    return {};
  }
  auto model = std::move(it->model);
  index.by_id.erase(it);
  return model;
}

auto GFGpgSign(int channel, const char* const* key_ids, size_t key_ids_size,
               GFBufferView in, int sign_mode, int ascii, GFGpgResultRef* out)
    -> int {
  if (out == nullptr) return -1;
  *out = nullptr;

  // Declared outside the try so an exception cannot orphan a registered
  // result: the catch releases it.
  GFGpgResultImpl* impl = nullptr;
  try {
    auto signer_keys = KeysOf(channel, key_ids, key_ids_size);
    if (signer_keys.empty()) {
      return FailRequest(out, "no usable signing key was given", "GFGpgSign");
    }

    impl = NewResult("GFGpgSign");
    if (impl == nullptr) return -1;

    auto [err, data_object] =
        GpgFrontend::MessageCryptoOperation::GetInstance(channel).SignSync(
            signer_keys, ViewToBuffer(in),
            sign_mode == 0 ? GPGME_SIG_MODE_NORMAL : GPGME_SIG_MODE_DETACH,
            ascii != 0);

    if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) {
      return FailOperation(impl, err, out);
    }

    auto result =
        GpgFrontend::ExtractParams<GpgFrontend::GpgSignResult>(data_object, 0);
    auto out_buffer =
        GpgFrontend::ExtractParams<GpgFrontend::GFBuffer>(data_object, 1);

    Finish(impl, result, out_buffer, err);
    impl->hash_algo = result.HashAlgo().toUtf8();
    *out = impl;
    return GF_GPG_OK;
  } catch (...) {
    // No C++ exception may cross the C ABI.
    LOG_E() << "GFGpgSign: unexpected exception";
    if (impl != nullptr) {
      GFGpgResultRelease(impl);
      *out = nullptr;
    }
    return -1;
  }
}

auto GFGpgEncrypt(int channel, const char* const* key_ids, size_t key_ids_size,
                  GFBufferView in, int ascii, GFGpgResultRef* out) -> int {
  if (out == nullptr) return -1;
  *out = nullptr;

  // Declared outside the try so an exception cannot orphan a registered
  // result: the catch releases it.
  GFGpgResultImpl* impl = nullptr;
  try {
    auto recipients = KeysOf(channel, key_ids, key_ids_size);
    if (recipients.empty()) {
      return FailRequest(out, "no usable recipient key was given",
                         "GFGpgEncrypt");
    }

    impl = NewResult("GFGpgEncrypt");
    if (impl == nullptr) return -1;

    auto [err, data_object] =
        GpgFrontend::MessageCryptoOperation::GetInstance(channel).EncryptSync(
            recipients, ViewToBuffer(in), ascii != 0);

    if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) {
      return FailOperation(impl, err, out);
    }

    auto result = GpgFrontend::ExtractParams<GpgFrontend::GpgEncryptResult>(
        data_object, 0);
    auto out_buffer =
        GpgFrontend::ExtractParams<GpgFrontend::GFBuffer>(data_object, 1);

    Finish(impl, result, out_buffer, err);
    *out = impl;
    return GF_GPG_OK;
  } catch (...) {
    LOG_E() << "GFGpgEncrypt: unexpected exception";
    if (impl != nullptr) {
      GFGpgResultRelease(impl);
      *out = nullptr;
    }
    return -1;
  }
}

auto GFGpgDecrypt(int channel, GFBufferView in, GFGpgResultRef* out) -> int {
  if (out == nullptr) return -1;
  *out = nullptr;

  // Declared outside the try so an exception cannot orphan a registered
  // result: the catch releases it.
  GFGpgResultImpl* impl = nullptr;
  try {
    impl = NewResult("GFGpgDecrypt");
    if (impl == nullptr) return -1;

    auto [err, data_object] =
        GpgFrontend::MessageCryptoOperation::GetInstance(channel).DecryptSync(
            ViewToBuffer(in));

    if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) {
      return FailOperation(impl, err, out);
    }

    auto result = GpgFrontend::ExtractParams<GpgFrontend::GpgDecryptResult>(
        data_object, 0);
    auto out_buffer =
        GpgFrontend::ExtractParams<GpgFrontend::GFBuffer>(data_object, 1);

    Finish(impl, result, out_buffer, err);
    *out = impl;
    return GF_GPG_OK;
  } catch (...) {
    LOG_E() << "GFGpgDecrypt: unexpected exception";
    if (impl != nullptr) {
      GFGpgResultRelease(impl);
      *out = nullptr;
    }
    return -1;
  }
}

auto GFGpgVerify(int channel, GFBufferView in, GFBufferView signature,
                 GFGpgResultRef* out) -> int {
  if (out == nullptr) return -1;
  *out = nullptr;

  // Declared outside the try so an exception cannot orphan a registered
  // result: the catch releases it.
  GFGpgResultImpl* impl = nullptr;
  try {
    impl = NewResult("GFGpgVerify");
    if (impl == nullptr) return -1;

    auto [err, data_object] =
        GpgFrontend::MessageCryptoOperation::GetInstance(channel).VerifySync(
            ViewToBuffer(in), ViewToBuffer(signature));

    if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) {
      return FailOperation(impl, err, out);
    }

    auto result = GpgFrontend::ExtractParams<GpgFrontend::GpgVerifyResult>(
        data_object, 0);

    // Verification produces no payload: the input IS the message.
    Finish(impl, result, GpgFrontend::GFBuffer{}, err);
    *out = impl;
    return GF_GPG_OK;
  } catch (...) {
    LOG_E() << "GFGpgVerify: unexpected exception";
    if (impl != nullptr) {
      GFGpgResultRelease(impl);
      *out = nullptr;
    }
    return -1;
  }
}

/* --- accessors ----------------------------------------------------------- */

auto GFGpgResultStatusOf(GFGpgResultRef r) -> int {
  auto* impl = ResolveLive(r, "GFGpgResultStatusOf");
  return impl == nullptr ? GF_GPG_BAD_REQUEST : impl->status;
}

auto GFGpgResultError(GFGpgResultRef r) -> uint32_t {
  auto* impl = ResolveLive(r, "GFGpgResultError");
  return impl == nullptr ? 0 : impl->gpgme_error;
}

auto GFGpgResultData(GFGpgResultRef r) -> GFBufferView {
  auto* impl = ResolveLive(r, "GFGpgResultData");
  return impl == nullptr ? nullptr : impl->data;
}

auto GFGpgResultCapsuleId(GFGpgResultRef r) -> const char* {
  auto* impl = ResolveLive(r, "GFGpgResultCapsuleId");
  return impl == nullptr ? "" : impl->capsule_id.constData();
}

auto GFGpgResultErrorString(GFGpgResultRef r) -> const char* {
  auto* impl = ResolveLive(r, "GFGpgResultErrorString");
  return impl == nullptr ? "" : impl->error_string.constData();
}

auto GFGpgResultHashAlgo(GFGpgResultRef r) -> const char* {
  auto* impl = ResolveLive(r, "GFGpgResultHashAlgo");
  return impl == nullptr ? "" : impl->hash_algo.constData();
}

auto GFGpgResultTakeData(GFGpgResultRef r) -> GFBufferRef {
  auto* impl = ResolveLive(r, "GFGpgResultTakeData");
  if (impl == nullptr) return nullptr;

  auto* taken = impl->data;
  impl->data = nullptr;
  return taken;
}

void GFGpgResultRelease(GFGpgResultRef r) {
  if (r == nullptr) return;

  // Registry first: never dereference a pointer we may not own.
  const auto state = ResultRegistry::Instance().Take(r, "gpg.result_release");
  if (state != GFHandleState::kLive) {
    if (state == GFHandleState::kStale) {
      ReportStaleResult(r, "gpg.result_release");
    }
    return;
  }

  // The one teardown. Note there is no per-field free here for the strings:
  // they are QByteArray members, so ~GFGpgResultImpl reclaims them, and a
  // caller cannot forget one because a caller never sees them.
  DestroyResult(r);
}

}  // namespace gf_host

// The file-local helpers above are inside gf_host; these definitions are
// not, because their declarations are at global scope.
using namespace gf_host;  // NOLINT(build/namespaces)

namespace gf_sdk_internal {

auto SweepResultHandles(const QString& module_id) -> QList<const char*> {
  QList<const char*> origins;
  for (const auto& entry : ResultRegistry::Instance().TakeAllFor(module_id)) {
    origins.append(entry.second);
    DestroyResult(entry.first);
  }
  // Its unanalysed capsules go with it.
  {
    auto& index = Capsules();
    const QMutexLocker lock(&index.mutex);
    index.by_id.removeIf(
        [&](const auto& entry) { return entry.value().owner == module_id; });
  }
  return origins;
}

}  // namespace gf_sdk_internal

namespace gf_host {

auto GFGpgResultOutstandingCount(const char* module_id) -> size_t {
  return ResultRegistry::Instance().Count(
      module_id == nullptr ? QString() : QString::fromUtf8(module_id));
}

}  // namespace gf_host