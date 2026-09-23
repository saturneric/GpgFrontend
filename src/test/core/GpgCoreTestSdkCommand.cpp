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

#include <gtest/gtest.h>

#include <QCborValue>
#include <QSemaphore>
#include <QThread>
#include <atomic>

#include "GpgFrontendTest.h"
#include "SdkTestContext.h"
#include "sdk/GFSDKBuffer.h"
#include "sdk/GFSDKCommand.h"
#include "ui/command/CommandRegistry.h"

/**
 * @file GpgCoreTestSdkCommand.cpp
 * @brief Commands across the real C ABI, between two real minted modules.
 *
 * What only this level can show: that a payload changes owner when it
 * crosses from one module to another -- the receiver can read it, the sender
 * no longer holds it, nobody leaks it -- and that a refused call still takes
 * what it was given.
 */

namespace GpgFrontend::Test {

namespace {

constexpr auto kProvider = "com.example.sdkcmd.provider";
constexpr auto kCaller = "com.example.sdkcmd.caller";
constexpr auto kEcho = "com.example.sdkcmd.provider.echo";

auto Bytes(GFSDKContext* ctx, GFBufferView b) -> QByteArray {
  return QByteArray(static_cast<const char*>(GFBufferData(ctx, b)),
                    static_cast<int>(GFBufferSize(ctx, b)));
}

auto Buffer(GFSDKContext* ctx, const QByteArray& b) -> GFBufferRef {
  return GFBufferNewFromBytes(ctx, b.constData(), static_cast<size_t>(b.size()));
}

struct ProviderState {
  GFSDKContext* ctx = nullptr;
  std::atomic<bool> read_blob{false};
  std::atomic<bool> context_named_caller{false};
};

/// Echo the blob back as the result's blob, and say what the text was.
void EchoHandler(void* user, uint64_t call_id, GFBufferView context_cbor,
                 GFBufferRef args_cbor, GFBufferRef* blobs, size_t n) {
  auto* st = static_cast<ProviderState*>(user);
  const auto ctx_map =
      QCborValue::fromCbor(Bytes(st->ctx, context_cbor)).toMap();
  st->context_named_caller =
      ctx_map.value(QStringLiteral("caller")).toString() == kCaller;

  const auto args = QCborValue::fromCbor(Bytes(st->ctx, args_cbor)).toMap();
  GFBufferRelease(st->ctx, args_cbor);

  GFBufferRef echo = nullptr;
  if (n == 1) {
    st->read_blob = Bytes(st->ctx, blobs[0]) == "s3cret";
    echo = blobs[0];  // forwarded as-is: the handle goes back to the Host
  }
  const auto result =
      QCborValue(QCborMap{{QStringLiteral("text"),
                           args.value(QStringLiteral("text"))},
                          {QStringLiteral("blob"),
                           QCborMap{{QStringLiteral("$blob"), 0}}}})
          .toCbor();
  GFBufferRef blob_out[1] = {echo};
  GFCommandComplete(st->ctx, call_id, GF_CMD_OK, Buffer(st->ctx, result),
                    blob_out, echo == nullptr ? 0 : 1, nullptr);
}

struct CallerState {
  GFSDKContext* ctx = nullptr;
  QSemaphore done;
  std::atomic<int> status{1};
  QByteArray text;
  QByteArray blob;
};

void DoneHandler(void* user, uint64_t /*call_id*/, int status,
                 GFBufferRef result_cbor, GFBufferRef* blobs, size_t n,
                 const char* /*error*/) {
  auto* st = static_cast<CallerState*>(user);
  st->status = status;
  if (result_cbor != nullptr) {
    st->text = QCborValue::fromCbor(Bytes(st->ctx, result_cbor))
                   .toMap()
                   .value(QStringLiteral("text"))
                   .toString()
                   .toUtf8();
    GFBufferRelease(st->ctx, result_cbor);
  }
  for (size_t i = 0; i < n; ++i) {
    st->blob = Bytes(st->ctx, blobs[i]);
    GFBufferRelease(st->ctx, blobs[i]);
  }
  st->done.release();
}

auto Descriptor(uint32_t required_caps) -> QByteArray {
  return QCborValue(QCborMap{{QStringLiteral("id"), kEcho},
                             {QStringLiteral("required_caps"),
                              static_cast<qint64>(required_caps)},
                             {QStringLiteral("flags"), 0}})
      .toCbor();
}

}  // namespace

TEST(SdkCommandTest, APayloadChangesOwnerAcrossModules) {
  SdkTestContext provider(kProvider);
  SdkTestContext caller(kCaller);
  ASSERT_TRUE(provider.live());
  ASSERT_TRUE(caller.live());

  ProviderState pst;
  pst.ctx = provider();
  auto* descriptor = Buffer(provider(), Descriptor(0));
  GFCommandSpec spec{};
  spec.struct_size = sizeof(spec);
  spec.id = kEcho;
  spec.descriptor_cbor = descriptor;
  spec.handler = &EchoHandler;
  spec.user = &pst;
  ASSERT_EQ(GFCommandRegister(provider(), &spec), GF_CMD_OK);
  GFBufferRelease(provider(), descriptor);

  const auto caller_before = GFBufferOutstandingCount(caller());
  const auto provider_before = GFBufferOutstandingCount(provider());

  CallerState cst;
  cst.ctx = caller();
  auto* args = Buffer(caller(), QCborValue(QCborMap{{QStringLiteral("text"),
                                                     "hello"}})
                                    .toCbor());
  GFBufferRef blobs[1] = {Buffer(caller(), "s3cret")};
  uint64_t call_id = 0;
  ASSERT_EQ(GFCommandInvoke(caller(), kEcho, 0, args, blobs, 1, &DoneHandler,
                            &cst, &call_id),
            GF_CMD_OK);
  EXPECT_NE(call_id, 0U);
  // Handed over: the caller holds neither handle any more.
  EXPECT_EQ(GFBufferOutstandingCount(caller()), caller_before);

  ASSERT_TRUE(cst.done.tryAcquire(1, 10000));
  EXPECT_EQ(cst.status.load(), GF_CMD_OK);
  EXPECT_TRUE(pst.read_blob.load()) << "the provider could read what it got";
  EXPECT_TRUE(pst.context_named_caller.load())
      << "the Host, not the caller, says who called";
  EXPECT_EQ(cst.text, "hello");
  EXPECT_EQ(cst.blob, "s3cret") << "and the bytes made the round trip";

  EXPECT_EQ(GFBufferOutstandingCount(caller()), caller_before);
  // The Host lends the provider its context for the length of the handler
  // and takes it back when the handler returns -- which is after the result
  // has already been delivered above, on the provider's thread.
  for (int i = 0; i < 100 && GFBufferOutstandingCount(provider()) !=
                                 provider_before;
       ++i) {
    QThread::msleep(10);
  }
  EXPECT_EQ(GFBufferOutstandingCount(provider()), provider_before);

  EXPECT_EQ(GFCommandUnregister(caller(), kEcho), GF_CMD_E_DENIED);
  EXPECT_EQ(GFCommandUnregister(provider(), kEcho), GF_CMD_OK);
}

// Refused calls still take ownership: the caller cannot tell a refusal from
// a success by whether its buffers came back, so they never do.
TEST(SdkCommandTest, ARefusedCallStillTakesItsBuffers) {
  SdkTestContext provider(kProvider);
  SdkTestContext caller(kCaller, GF_HOST_CAP_UI);

  ProviderState pst;
  pst.ctx = provider();
  auto* descriptor = Buffer(provider(), Descriptor(GF_HOST_CAP_GPG));
  GFCommandSpec spec{};
  spec.struct_size = sizeof(spec);
  spec.id = kEcho;
  spec.descriptor_cbor = descriptor;
  spec.handler = &EchoHandler;
  spec.user = &pst;
  ASSERT_EQ(GFCommandRegister(provider(), &spec), GF_CMD_OK);
  GFBufferRelease(provider(), descriptor);

  const auto before = GFBufferOutstandingCount(caller());
  auto* args = Buffer(caller(), QCborValue(QCborMap{}).toCbor());
  GFBufferRef blobs[1] = {Buffer(caller(), "x")};
  CallerState cst;
  cst.ctx = caller();
  EXPECT_EQ(GFCommandInvoke(caller(), kEcho, 0, args, blobs, 1, &DoneHandler,
                            &cst, nullptr),
            GF_CMD_E_DENIED)
      << "the caller lacks gpg, which the command requires";
  EXPECT_EQ(GFBufferOutstandingCount(caller()), before);
  EXPECT_FALSE(cst.done.tryAcquire(1, 200)) << "and is never called back";

  EXPECT_EQ(GFCommandInvoke(caller(), "com.example.sdkcmd.none", 0, nullptr,
                            nullptr, 0, nullptr, nullptr, nullptr),
            GF_CMD_E_UNKNOWN);

  // A module may not register outside its own namespace.
  auto* stolen = Buffer(provider(), Descriptor(0));
  spec.id = "com.example.sdkcmd.caller.stolen";
  spec.descriptor_cbor = stolen;
  EXPECT_EQ(GFCommandRegister(provider(), &spec), GF_CMD_E_DENIED);
  GFBufferRelease(provider(), stolen);

  UI::CommandRegistry::Instance().RemoveAllFor(kProvider);
}

}  // namespace GpgFrontend::Test
