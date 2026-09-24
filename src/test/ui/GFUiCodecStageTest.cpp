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

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QSemaphore>
#include <atomic>
#include <functional>
#include <mutex>

#include "GpgFrontendTest.h"
#include "sdk/GFSDKHostApi.h"
#include "sdk/GFSDKHostCommands.hpp"
#include "ui/command/CodecStage.h"
#include "ui/command/CommandRegistry.h"

/**
 * @file GFUiCodecStageTest.cpp
 * @brief The codec stage around the Host's own decrypt and encrypt, through
 *        the real registry: which codec is asked, what ends the stage, and
 *        that a codec that goes away can neither block the Host's decrypt
 *        nor be called back into.
 *
 * Codecs here are plain providers owned by test "modules"; deactivation is
 * what the Host does for a module going away: CommandRegistry::RemoveAllFor.
 * Each test uses ids of its own and removes what it registered. Other
 * decoders may be registered in the process (bundled modules); these inputs
 * are ones no real codec claims.
 */

namespace GpgFrontend::Test {

namespace {

using gf::cmd::host::CodecOutcome;
using gf::cmd::host::CodecResult;
using UI::CodecStage;
using UI::CodecStageResult;
using UI::CommandProvider;
using UI::CommandRegistry;

auto Reg() -> CommandRegistry& { return CommandRegistry::Instance(); }

struct Shape {
  static constexpr gf::cmd::Meta kMeta{"", "", "", "", 0, 0};
  using Args = gf::cmd::host::CodecArgs;
  using Result = gf::cmd::host::CodecResult;
};

/// What a fake codec answers, given the input bytes.
using Answer = std::function<gf::cmd::RawResult(const QByteArray&)>;

auto Result(CodecOutcome outcome, const QByteArray& output = {},
            const QString& error = {}) -> gf::cmd::RawResult {
  CodecResult r;
  r.outcome = outcome;
  if (!output.isNull()) r.output = UI::MakeHostBlob(GFBuffer(output));
  r.error = error;
  gf::cmd::EncodeState st;
  auto map = gf::cmd::EncodeMap(r, st);
  return {GF_CMD_OK, 0, map, std::move(st.blobs), {}};
}

/// A codec of module @p owner, counting how often it is entered.
auto Codec(const QString& id, const QString& owner, uint32_t flags,
           Answer answer, std::atomic<int>* calls = nullptr)
    -> CommandProvider {
  CommandProvider p;
  p.id = id;
  p.owner = owner;
  p.owner_caps = GF_HOST_CAP_EDITOR;
  p.flags = flags;
  p.descriptor = gf::cmd::Describe<Shape>();
  p.run = [answer = std::move(answer), calls](
              const gf::cmd::CommandContext&, QCborMap args,
              std::vector<gf::cmd::Blob> blobs, gf::cmd::Completer done) {
    if (calls != nullptr) ++*calls;
    gf::cmd::host::CodecArgs a;
    const bool ok = gf::cmd::DecodeMap(args, blobs, a);
    done(answer(ok ? QByteArray(a.input.Data(),
                                static_cast<qsizetype>(a.input.Size()))
                   : QByteArray()));
  };
  return p;
}

auto Decoder(const QString& id, const QString& owner, Answer answer,
             std::atomic<int>* calls = nullptr) -> CommandProvider {
  return Codec(id, owner, gf::cmd::kInputDecoder, std::move(answer), calls);
}

auto NotMine() -> Answer {
  return [](const QByteArray&) { return Result(CodecOutcome::kNotHandled); };
}

/// A decoder that parks its completer for the test to fire, or not, later.
struct Parked {
  std::mutex mutex;
  gf::cmd::Completer done;
  QSemaphore arrived;
};

auto ParkingDecoder(const QString& id, const QString& owner, Parked* parked)
    -> CommandProvider {
  auto p = Decoder(id, owner, NotMine());
  p.run = [parked](const gf::cmd::CommandContext&, QCborMap,
                   std::vector<gf::cmd::Blob>, gf::cmd::Completer done) {
    {
      std::lock_guard<std::mutex> lock(parked->mutex);
      parked->done = std::move(done);
    }
    parked->arrived.release();
  };
  return p;
}

/// Everything one stage reported. `done` must run exactly once.
struct Outcome {
  std::atomic<int> done{0};
  CodecStageResult result;
};

/// Run this thread's event loop until @p pred or a timeout. @p pred is asked
/// once per turn and never again after it said yes, so it may consume.
template <typename F>
auto Pump(F pred, int ms = 5000) -> bool {
  QElapsedTimer t;
  t.start();
  while (t.elapsed() < ms) {
    if (pred()) return true;
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  return pred();
}

/// Run the decoder stage on @p input and wait for it to finish.
/// A real decoder loaded in this process may be slow to answer; the stage
/// leaves it behind well within the wait, as it would for the user.
auto RunDecoders(const QByteArray& input, Outcome* out, int timeout_ms = 2000)
    -> bool {
  CodecStage::RunDecoders(
      GFBuffer(input),
      [out](CodecStageResult r) {
        out->result = std::move(r);
        ++out->done;
      },
      timeout_ms);
  return Pump([out] { return out->done.load() > 0; }, 10000);
}

/// A little longer, so a second `done` would have had its chance to arrive.
void Settle() { Pump([] { return false; }, 100); }

void Drop(const QString& owner) {
  Reg().RemoveAllFor(owner);
  Reg().Reopen(owner);
}

// Input no real codec claims: it is not even Base58.
const QByteArray kInput = "an ordinary message, not an encoding!";

}  // namespace

// ------------------------------------------------------------ registration

TEST(CodecStageTest, ACodecMustHaveTheCodecShapeAndRules) {
  const QString owner = "com.example.codec.reg";

  auto gui = Decoder(owner + ".gui", owner, NotMine());
  gui.flags |= gf::cmd::kNeedsGuiThread;
  EXPECT_EQ(Reg().Register(gui), GF_CMD_E_DENIED) << "a codec shows no dialog";

  auto blind = Decoder(owner + ".blind", owner, NotMine());
  blind.owner_caps = 0;
  EXPECT_EQ(Reg().Register(blind), GF_CMD_E_DENIED)
      << "a decoder reads editor text, so it needs the editor capability";

  auto odd = Decoder(owner + ".odd", owner, NotMine());
  odd.descriptor = gf::cmd::Describe<gf::cmd::host::AppMessage>();
  EXPECT_EQ(Reg().Register(odd), GF_CMD_E_DENIED) << "only the codec shape";

  auto encoder =
      Codec(owner + ".enc", owner, gf::cmd::kOutputEncoder, NotMine());
  encoder.owner_caps = 0;
  EXPECT_EQ(Reg().Register(encoder), GF_CMD_E_DENIED);

  EXPECT_EQ(Reg().Register(Decoder(owner + ".ok", owner, NotMine())),
            GF_CMD_OK);
  Drop(owner);
}

TEST(CodecStageTest, DecodersAreListedInIdOrderAndGoWithTheirModule) {
  const QString a = "com.example.codec.lista";
  const QString b = "com.example.codec.listb";
  ASSERT_EQ(Reg().Register(Decoder(b + ".d", b, NotMine())), GF_CMD_OK);
  ASSERT_EQ(Reg().Register(Decoder(a + ".d", a, NotMine())), GF_CMD_OK);

  auto ids = Reg().ProvidersWithFlag(gf::cmd::kInputDecoder);
  const auto ia = ids.indexOf(a + ".d");
  const auto ib = ids.indexOf(b + ".d");
  ASSERT_GE(ia, 0);
  ASSERT_GE(ib, 0);
  EXPECT_LT(ia, ib);
  EXPECT_FALSE(Reg().ProvidersWithFlag(gf::cmd::kOutputEncoder)
                   .contains(a + ".d"));

  Reg().RemoveAllFor(a);
  ids = Reg().ProvidersWithFlag(gf::cmd::kInputDecoder);
  EXPECT_FALSE(ids.contains(a + ".d"));
  EXPECT_TRUE(ids.contains(b + ".d"));
  Reg().Reopen(a);
  Drop(b);
}

// ------------------------------------------------------------ decoding

// The token case: a decoder claims the input and hands back the message the
// Host then decrypts. Later decoders are not asked.
TEST(CodecStageTest, TheFirstDecoderToClaimTheInputWins) {
  const QString m = "com.example.codec.claim";
  std::atomic<int> first{0};
  std::atomic<int> after{0};
  ASSERT_EQ(Reg().Register(Decoder(m + ".a", m, NotMine(), &first)),
            GF_CMD_OK);
  ASSERT_EQ(Reg().Register(Decoder(
                m + ".b", m,
                [](const QByteArray& in) {
                  return in == kInput
                             ? Result(CodecOutcome::kHandled, "inner message")
                             : Result(CodecOutcome::kNotHandled);
                })),
            GF_CMD_OK);
  ASSERT_EQ(Reg().Register(Decoder(
                m + ".c", m,
                [](const QByteArray&) {
                  return Result(CodecOutcome::kHandled, "not me");
                },
                &after)),
            GF_CMD_OK);

  Outcome out;
  ASSERT_TRUE(RunDecoders(kInput, &out));
  EXPECT_EQ(out.result.kind, CodecStageResult::Kind::kHandled);
  EXPECT_EQ(out.result.output.ConvertToQByteArray(), "inner message");
  EXPECT_EQ(out.result.provider, m + ".b");
  EXPECT_EQ(first.load(), 1);
  EXPECT_EQ(after.load(), 0);
  Settle();
  EXPECT_EQ(out.done.load(), 1);
  Drop(m);
}

// Ordinary OpenPGP or plain text: every decoder passes, and the stage says
// so -- the Host's own decrypt then runs on the original text.
TEST(CodecStageTest, UnclaimedInputFallsThroughToTheHostsDecrypt) {
  const QString m = "com.example.codec.pass";
  std::atomic<int> calls{0};
  ASSERT_EQ(Reg().Register(Decoder(m + ".a", m, NotMine(), &calls)),
            GF_CMD_OK);
  ASSERT_EQ(Reg().Register(Decoder(m + ".b", m, NotMine(), &calls)),
            GF_CMD_OK);

  Outcome out;
  ASSERT_TRUE(RunDecoders(kInput, &out));
  EXPECT_EQ(out.result.kind, CodecStageResult::Kind::kNotHandled);
  EXPECT_TRUE(out.result.output.Empty());
  EXPECT_EQ(calls.load(), 2);
  Settle();
  EXPECT_EQ(out.done.load(), 1);
  Drop(m);
}

// A decoder that claims the input and cannot decode it ends the stage with
// its error, once: no later decoder, and no OpenPGP decrypt of the text.
TEST(CodecStageTest, AHandledErrorEndsTheStageOnce) {
  const QString m = "com.example.codec.fail";
  std::atomic<int> after{0};
  ASSERT_EQ(Reg().Register(Decoder(m + ".a", m,
                                   [](const QByteArray&) {
                                     return Result(CodecOutcome::kFailed, {},
                                                   "the token is damaged");
                                   })),
            GF_CMD_OK);
  ASSERT_EQ(Reg().Register(Decoder(
                m + ".b", m,
                [](const QByteArray&) {
                  return Result(CodecOutcome::kHandled, "x");
                },
                &after)),
            GF_CMD_OK);

  Outcome out;
  ASSERT_TRUE(RunDecoders(kInput, &out));
  EXPECT_EQ(out.result.kind, CodecStageResult::Kind::kFailed);
  EXPECT_EQ(out.result.error, "the token is damaged");
  EXPECT_EQ(out.result.provider, m + ".a");
  EXPECT_EQ(after.load(), 0);
  Settle();
  EXPECT_EQ(out.done.load(), 1);
  Drop(m);
}

// A decoder that fails to answer properly is not a claim: it is passed over,
// and the next one -- or the Host's own decrypt -- still gets the input.
TEST(CodecStageTest, ABrokenDecoderIsPassedOver) {
  const QString m = "com.example.codec.broken";
  ASSERT_EQ(Reg().Register(Decoder(m + ".a", m,
                                   [](const QByteArray&) {
                                     return gf::cmd::RawResult{
                                         GF_CMD_E_FAILED, 0, {}, {}, "boom"};
                                   })),
            GF_CMD_OK);
  ASSERT_EQ(Reg().Register(Decoder(m + ".b", m,
                                   [](const QByteArray&) {
                                     return gf::cmd::RawResult{
                                         GF_CMD_OK,
                                         0,
                                         QCborMap{{QStringLiteral("nonsense"),
                                                   1}},
                                         {},
                                         {}};
                                   })),
            GF_CMD_OK);
  ASSERT_EQ(Reg().Register(Decoder(m + ".c", m,
                                   [](const QByteArray&) {
                                     return Result(CodecOutcome::kHandled);
                                   })),
            GF_CMD_OK);

  Outcome out;
  ASSERT_TRUE(RunDecoders(kInput, &out));
  // .c claimed it with no output: a failure, not a silent empty message.
  EXPECT_EQ(out.result.kind, CodecStageResult::Kind::kFailed);
  EXPECT_EQ(out.result.provider, m + ".c");
  Drop(m);
}

// A decoder whose module is deactivated mid-call: the Host's decrypt goes on
// without it, and nothing the module does afterwards reaches the stage.
TEST(CodecStageTest, ADeactivatedDecoderNeitherBlocksNorIsHeardFrom) {
  const QString m = "com.example.codec.gone";
  Parked parked;
  ASSERT_EQ(Reg().Register(ParkingDecoder(m + ".d", m, &parked)), GF_CMD_OK);

  Outcome out;
  CodecStage::RunDecoders(GFBuffer(kInput), [&out](CodecStageResult r) {
    out.result = std::move(r);
    ++out.done;
  });
  ASSERT_TRUE(Pump([&parked] { return parked.arrived.tryAcquire(); }));
  EXPECT_EQ(out.done.load(), 0) << "the stage waits for the decoder";

  Reg().RemoveAllFor(m);  // what deactivation does
  ASSERT_TRUE(Pump([&out] { return out.done.load() > 0; }));
  EXPECT_EQ(out.result.kind, CodecStageResult::Kind::kNotHandled);

  // The module answering late changes nothing.
  {
    std::lock_guard<std::mutex> lock(parked.mutex);
    if (parked.done) {
      parked.done(Result(CodecOutcome::kHandled, "too late"));
    }
  }
  Settle();
  EXPECT_EQ(out.done.load(), 1);
  EXPECT_EQ(out.result.kind, CodecStageResult::Kind::kNotHandled);

  // And a deactivated module is not asked at all.
  Outcome again;
  ASSERT_TRUE(RunDecoders(kInput, &again));
  EXPECT_EQ(again.result.kind, CodecStageResult::Kind::kNotHandled);
  EXPECT_FALSE(parked.arrived.tryAcquire());
  Reg().Reopen(m);
}

// A decoder that never answers is bounded: the stage moves on without it.
TEST(CodecStageTest, ASlowDecoderIsLeftBehind) {
  const QString m = "com.example.codec.slow";
  Parked parked;
  ASSERT_EQ(Reg().Register(ParkingDecoder(m + ".d", m, &parked)), GF_CMD_OK);

  Outcome out;
  ASSERT_TRUE(RunDecoders(kInput, &out, 50));
  EXPECT_EQ(out.result.kind, CodecStageResult::Kind::kNotHandled);
  ASSERT_TRUE(parked.arrived.tryAcquire());

  {
    std::lock_guard<std::mutex> lock(parked.mutex);
    if (parked.done) parked.done(Result(CodecOutcome::kHandled, "late"));
  }
  Settle();
  EXPECT_EQ(out.done.load(), 1);
  Drop(m);
}

// ------------------------------------------------------------ encoding

TEST(CodecStageTest, AnEncoderMustExistAndClaimItsInput) {
  const QString m = "com.example.codec.enc";
  ASSERT_EQ(Reg().Register(Codec(
                m + ".wrap", m, gf::cmd::kOutputEncoder,
                [](const QByteArray& in) {
                  return Result(CodecOutcome::kHandled, "<" + in + ">");
                })),
            GF_CMD_OK);
  ASSERT_EQ(Reg().Register(
                Codec(m + ".shy", m, gf::cmd::kOutputEncoder, NotMine())),
            GF_CMD_OK);
  // A decoder is not an encoder, whatever it would answer.
  ASSERT_EQ(Reg().Register(Decoder(m + ".dec", m, NotMine())), GF_CMD_OK);

  const auto run = [](const QString& id) {
    Outcome out;
    CodecStage::RunEncoder(id, GFBuffer(QByteArray("msg")),
                           [&out](CodecStageResult r) {
                             out.result = std::move(r);
                             ++out.done;
                           });
    EXPECT_TRUE(Pump([&out] { return out.done.load() > 0; }));
    return out.result;
  };

  const auto wrapped = run(m + ".wrap");
  EXPECT_EQ(wrapped.kind, CodecStageResult::Kind::kHandled);
  EXPECT_EQ(wrapped.output.ConvertToQByteArray(), "<msg>");

  EXPECT_EQ(run(m + ".shy").kind, CodecStageResult::Kind::kFailed);
  EXPECT_EQ(run(m + ".dec").kind, CodecStageResult::Kind::kFailed);
  EXPECT_EQ(run(m + ".missing").kind, CodecStageResult::Kind::kFailed);
  Drop(m);
}

}  // namespace GpgFrontend::Test
