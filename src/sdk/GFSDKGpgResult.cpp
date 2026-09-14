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

#include "GFSDKGpgResult.h"

#include <QByteArray>
#include <QStringList>
#include <cstdint>
#include <new>

#include "GFSDKBasic.h"
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
#include "private/GFSDKHandleRegistry.h"
#include "private/GFSDKPrivat.h"
#include "ui/UIModuleManager.h"

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

namespace {

using ResultRegistry = GFHandleRegistry<GFGpgResultImpl>;

void ReportStaleResult(const void* handle, const char* what) {
  LOG_W() << "GFSDKGpgResult:" << what
          << "called on a handle that is not live (already released, or never "
             "issued by this SDK):"
          << handle;
#ifdef DEBUG
  qFatal("GFSDKGpgResult: %s on stale handle %p", what, handle);
#endif
}

/// Validate against the registry BEFORE dereferencing. Never the other way.
auto ResolveLive(GFGpgResultRef r, const char* what) -> GFGpgResultImpl* {
  if (r == nullptr) return nullptr;
  if (!ResultRegistry::Instance().IsLive(r)) {
    ReportStaleResult(r, what);
    return nullptr;
  }
  Q_ASSERT(r->magic == kGFGpgResultMagic);
  return r;
}

/// Allocate a live result. Returns nullptr only when memory is exhausted.
auto NewResult() -> GFGpgResultImpl* {
  auto* mem = GpgFrontend::SMAMalloc(sizeof(GFGpgResultImpl));
  if (mem == nullptr) return nullptr;

  auto* impl = new (mem) GFGpgResultImpl{};
  ResultRegistry::Instance().Register(impl, QString());
  return impl;
}

/// The arguments were unusable, so nothing was attempted. The caller still
/// gets an owned result carrying the reason -- the contract says a
/// non-negative return always means there is something to release.
auto FailRequest(GFGpgResultRef* out, const char* why) -> int {
  auto* impl = NewResult();
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
  impl->capsule_id = GpgFrontend::UI::UIModuleManager::GetInstance()
                         .MakeCapsule(result)
                         .toUtf8();
  impl->error_string = GpgFrontend::DescribeGpgErrCode(err).second.toUtf8();
  impl->data = GFBufferNewFromBytes(out_buffer.Data(), out_buffer.Size());
}

}  // namespace

auto GFGpgSign(int channel, const char* const* key_ids, size_t key_ids_size,
               GFBufferView in, int sign_mode, int ascii, GFGpgResultRef* out)
    -> int {
  if (out == nullptr) return -1;
  *out = nullptr;

  try {
    auto signer_keys = KeysOf(channel, key_ids, key_ids_size);
    if (signer_keys.empty()) {
      return FailRequest(out, "no usable signing key was given");
    }

    auto* impl = NewResult();
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
    return -1;
  }
}

auto GFGpgEncrypt(int channel, const char* const* key_ids, size_t key_ids_size,
                  GFBufferView in, int ascii, GFGpgResultRef* out) -> int {
  if (out == nullptr) return -1;
  *out = nullptr;

  try {
    auto recipients = KeysOf(channel, key_ids, key_ids_size);
    if (recipients.empty()) {
      return FailRequest(out, "no usable recipient key was given");
    }

    auto* impl = NewResult();
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
    return -1;
  }
}

auto GFGpgDecrypt(int channel, GFBufferView in, GFGpgResultRef* out) -> int {
  if (out == nullptr) return -1;
  *out = nullptr;

  try {
    auto* impl = NewResult();
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
    return -1;
  }
}

auto GFGpgVerify(int channel, GFBufferView in, GFBufferView signature,
                 GFGpgResultRef* out) -> int {
  if (out == nullptr) return -1;
  *out = nullptr;

  try {
    auto* impl = NewResult();
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
  if (!ResultRegistry::Instance().Take(r)) {
    ReportStaleResult(r, "GFGpgResultRelease");
    return;
  }

  // The one teardown. Note there is no per-field free here for the strings:
  // they are QByteArray members, so ~GFGpgResultImpl reclaims them, and a
  // caller cannot forget one because a caller never sees them.
  GFBufferRelease(r->data);
  r->magic = 0;
  r->~GFGpgResultImpl();
  GpgFrontend::SMAFree(r);
}

auto GFGpgResultOutstandingCount(const char* module_id) -> size_t {
  return ResultRegistry::Instance().Count(
      module_id == nullptr ? QString() : QString::fromUtf8(module_id));
}
