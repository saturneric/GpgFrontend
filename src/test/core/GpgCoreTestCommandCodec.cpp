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

#include "GpgFrontendTest.h"
#include "sdk/GFSDKCommand.hpp"
#include "sdk/GFSDKHostApi.h"

/**
 * @file GpgCoreTestCommandCodec.cpp
 * @brief The typed command layer, on its own: no Host, no module.
 *
 * Everything the registry and the marshalling know about a command is
 * derived from its C++ type here, so this is where "derived correctly" is
 * pinned down -- round trips, strictness, and that a Blob's bytes never end
 * up inside the CBOR.
 */

namespace GpgFrontend::Test {

namespace {

class BytesStorage : public gf::cmd::BlobStorage {
 public:
  explicit BytesStorage(QByteArray b) : bytes_(std::move(b)) {}
  [[nodiscard]] auto Data() const -> const char* override {
    return bytes_.constData();
  }
  [[nodiscard]] auto Size() const -> size_t override {
    return static_cast<size_t>(bytes_.size());
  }

 private:
  QByteArray bytes_;
};

auto BlobOf(const char* text) -> gf::cmd::Blob {
  return gf::cmd::Blob(std::make_shared<BytesStorage>(QByteArray(text)));
}

enum class Mode : int { kPlain = 1, kArmor = 2 };

struct Inner {
  QString name;
  static constexpr auto Fields() {
    return std::make_tuple(gf::cmd::F("name", &Inner::name));
  }
};

struct Everything {
  bool flag = false;
  qint64 count = 0;
  double ratio = 0;
  Mode mode = Mode::kPlain;
  QString text;
  QStringList tags;
  QByteArray octets;
  std::optional<QString> maybe;
  QList<Inner> inners;
  gf::cmd::KeyRef key;
  gf::cmd::Blob secret;

  static constexpr auto Fields() {
    return std::make_tuple(
        gf::cmd::F("flag", &Everything::flag),
        gf::cmd::F("count", &Everything::count),
        gf::cmd::F("ratio", &Everything::ratio),
        gf::cmd::F("mode", &Everything::mode),
        gf::cmd::F("text", &Everything::text),
        gf::cmd::F("tags", &Everything::tags),
        gf::cmd::F("octets", &Everything::octets),
        gf::cmd::F("maybe", &Everything::maybe),
        gf::cmd::F("inners", &Everything::inners),
        gf::cmd::F("key", &Everything::key),
        gf::cmd::F("secret", &Everything::secret));
  }
};

struct Echo {
  static constexpr gf::cmd::Meta kMeta{
      "com.example.codec.echo", "Echo", "Echo the text", "Tests",
      GF_HOST_CAP_GPG, gf::cmd::kCheckable};
  struct Args {
    QString text;
    static constexpr auto Fields() {
      return std::make_tuple(gf::cmd::F("text", &Args::text));
    }
  };
  struct Result {
    QString text;
    static constexpr auto Fields() {
      return std::make_tuple(gf::cmd::F("text", &Result::text));
    }
  };
  static auto State(const gf::cmd::CommandContext& ctx) -> uint32_t {
    return ctx.has_selection ? GF_CMD_STATE_ENABLED : 0U;
  }
};

auto EchoSync(const gf::cmd::CommandContext& /*ctx*/, const Echo::Args& a)
    -> gf::cmd::Outcome<Echo::Result> {
  if (a.text == "fail") {
    return gf::cmd::Outcome<Echo::Result>::Failure(GF_CMD_E_FAILED, "asked to");
  }
  return gf::cmd::Outcome<Echo::Result>::Success({a.text.toUpper()});
}

gf::cmd::Reply<Echo::Result>* g_parked = nullptr;

void EchoDeferred(const gf::cmd::CommandContext& /*ctx*/, Echo::Args a,
                  gf::cmd::Reply<Echo::Result> reply) {
  if (a.text == "park") {
    g_parked = new gf::cmd::Reply<Echo::Result>(reply);
    return;
  }
  if (a.text == "drop") return;  // the Reply dies unanswered
  reply.Ok({a.text + "!"});
}

auto RunBinding(const gf::cmd::Binding& b, const QCborMap& args)
    -> gf::cmd::RawResult {
  gf::cmd::RawResult out;
  bool called = false;
  b.run({}, args, {}, [&](gf::cmd::RawResult r) {
    called = true;
    out = std::move(r);
  });
  EXPECT_TRUE(called);
  return out;
}

}  // namespace

TEST(CommandCodecTest, EveryFieldKindRoundTrips) {
  Everything in;
  in.flag = true;
  in.count = -42;
  in.ratio = 0.5;
  in.mode = Mode::kArmor;
  in.text = QStringLiteral("héllo");
  in.tags = QStringList{"a", "b"};
  in.octets = QByteArray("\x00\x01\x02", 3);
  in.maybe = QStringLiteral("there");
  in.inners = {Inner{"x"}, Inner{"y"}};
  in.key.channel = 3;
  in.key.fingerprint = "ABCD";
  in.key.has_secret = true;
  in.secret = BlobOf("top secret");

  gf::cmd::EncodeState st;
  const auto map = gf::cmd::EncodeMap(in, st);
  ASSERT_EQ(st.blobs.size(), 1U);

  Everything out;
  QString error;
  ASSERT_TRUE(gf::cmd::DecodeMap(map, st.blobs, out, &error))
      << error.toStdString();
  EXPECT_TRUE(out.flag);
  EXPECT_EQ(out.count, -42);
  EXPECT_DOUBLE_EQ(out.ratio, 0.5);
  EXPECT_EQ(out.mode, Mode::kArmor);
  EXPECT_EQ(out.text, in.text);
  EXPECT_EQ(out.tags, in.tags);
  EXPECT_EQ(out.octets, in.octets);
  ASSERT_TRUE(out.maybe.has_value());
  EXPECT_EQ(*out.maybe, "there");
  ASSERT_EQ(out.inners.size(), 2);
  EXPECT_EQ(out.inners[1].name, "y");
  EXPECT_EQ(out.key.channel, 3);
  EXPECT_EQ(out.key.fingerprint, "ABCD");
  EXPECT_TRUE(out.key.has_secret);
  EXPECT_EQ(QByteArray(out.secret.Data(), static_cast<int>(out.secret.Size())),
            "top secret");
}

// The point of the side channel: the bytes of a Blob are never serialized.
TEST(CommandCodecTest, ABlobsBytesNeverEnterTheCbor) {
  Everything in;
  in.secret = BlobOf("correct horse battery staple");
  gf::cmd::EncodeState st;
  const auto cbor = QCborValue(gf::cmd::EncodeMap(in, st)).toCbor();
  EXPECT_FALSE(cbor.contains("correct horse"));
  EXPECT_EQ(st.blobs.size(), 1U);
}

TEST(CommandCodecTest, DecodingIsStrict) {
  Echo::Args out;
  QString error;

  EXPECT_FALSE(gf::cmd::DecodeMap(QCborMap{}, {}, out, &error));
  EXPECT_TRUE(error.contains("missing")) << error.toStdString();

  EXPECT_FALSE(gf::cmd::DecodeMap(
      QCborMap{{QStringLiteral("text"), 7}}, {}, out, &error))
      << "a number is not a string";

  EXPECT_FALSE(gf::cmd::DecodeMap(QCborMap{{QStringLiteral("text"), "a"},
                                           {QStringLiteral("extra"), 1}},
                                  {}, out, &error))
      << "an undeclared field is refused, not dropped";

  EXPECT_TRUE(gf::cmd::DecodeMap(QCborMap{{QStringLiteral("text"), "a"}}, {},
                                 out, &error));

  // An optional field may be absent; a blob reference must be in range.
  Everything e;
  gf::cmd::EncodeState st;
  auto m = gf::cmd::EncodeMap(e, st);
  m.remove(QStringLiteral("maybe"));
  m.insert(QStringLiteral("secret"),
           QCborMap{{QStringLiteral("$blob"), 5}});
  EXPECT_FALSE(gf::cmd::DecodeMap(m, st.blobs, e, &error));
  m.insert(QStringLiteral("secret"), QCborValue(QCborValue::Null));
  EXPECT_TRUE(gf::cmd::DecodeMap(m, st.blobs, e, &error))
      << error.toStdString();
}

TEST(CommandCodecTest, TheDescriptorIsDerivedFromTheType) {
  const auto d = gf::cmd::Describe<Echo>();
  EXPECT_EQ(d.value(QStringLiteral("id")).toString(), "com.example.codec.echo");
  EXPECT_EQ(d.value(QStringLiteral("required_caps")).toInteger(),
            GF_HOST_CAP_GPG);
  EXPECT_EQ(d.value(QStringLiteral("flags")).toInteger(), gf::cmd::kCheckable);

  const auto args = d.value(QStringLiteral("args")).toMap();
  EXPECT_EQ(args.value(QStringLiteral("type")).toString(), "object");
  const auto fields = args.value(QStringLiteral("fields")).toArray();
  ASSERT_EQ(fields.size(), 1);
  EXPECT_EQ(fields[0].toMap().value(QStringLiteral("name")).toString(),
            "text");
}

TEST(CommandCodecTest, ASynchronousHandlerIsBoundByOneLine) {
  const auto b = gf::cmd::Bind<Echo, &EchoSync>();
  EXPECT_STREQ(b.id, "com.example.codec.echo");
  ASSERT_NE(b.state, nullptr) << "a State() on the type is picked up";

  gf::cmd::CommandContext ctx;
  EXPECT_EQ(b.state(ctx), 0U);
  ctx.has_selection = true;
  EXPECT_EQ(b.state(ctx), GF_CMD_STATE_ENABLED);

  auto r = RunBinding(b, QCborMap{{QStringLiteral("text"), "abc"}});
  EXPECT_EQ(r.status, GF_CMD_OK);
  EXPECT_EQ(r.result.value(QStringLiteral("text")).toString(), "ABC");

  r = RunBinding(b, QCborMap{{QStringLiteral("text"), "fail"}});
  EXPECT_EQ(r.status, GF_CMD_E_FAILED);
  EXPECT_EQ(r.error, "asked to");

  r = RunBinding(b, QCborMap{{QStringLiteral("wrong"), "abc"}});
  EXPECT_EQ(r.status, GF_CMD_E_BAD_ARGS) << "decoded before the handler runs";
}

TEST(CommandCodecTest, ADeferredReplyAnswersExactlyOnce) {
  const auto b = gf::cmd::Bind<Echo, &EchoDeferred>();

  auto r = RunBinding(b, QCborMap{{QStringLiteral("text"), "now"}});
  EXPECT_EQ(r.status, GF_CMD_OK);
  EXPECT_EQ(r.result.value(QStringLiteral("text")).toString(), "now!");

  // A reply dropped on the floor still answers -- as a failure -- rather
  // than leaving the caller waiting for ever.
  r = RunBinding(b, QCborMap{{QStringLiteral("text"), "drop"}});
  EXPECT_EQ(r.status, GF_CMD_E_FAILED);

  // A parked reply answers when it is used, and only the first use counts.
  int answers = 0;
  b.run({}, QCborMap{{QStringLiteral("text"), "park"}}, {},
        [&](gf::cmd::RawResult) { ++answers; });
  EXPECT_EQ(answers, 0);
  ASSERT_NE(g_parked, nullptr);
  g_parked->Ok({"late"});
  g_parked->Fail(GF_CMD_E_FAILED, "too late");
  delete g_parked;
  g_parked = nullptr;
  EXPECT_EQ(answers, 1);
}

}  // namespace GpgFrontend::Test
