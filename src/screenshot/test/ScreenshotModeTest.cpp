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

// The screenshot mode's rules, each on its own: option parsing, the
// geometry the pipeline draws highlights from, the environment hash it
// keys invalidation on, the gates, and the readiness wait. Widget work runs
// on the GUI thread; gtest bodies do not.

#include <gtest/gtest.h>

#include <QDialog>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardItemModel>
#include <QTimeZone>
#include <QTimer>
#include <QUuid>
#include <QWidget>

#include "GpgFrontendTest.h"
#include "screenshot/ScreenshotCapture.h"
#include "screenshot/ScreenshotEnv.h"
#include "screenshot/ScreenshotOptions.h"
#include "screenshot/ScreenshotScenario.h"
#include "screenshot/ScreenshotSeed.h"

namespace GpgFrontend::Test {

namespace {

using namespace GpgFrontend::Screenshot;  // NOLINT

auto ResourcePath(const QString& name) -> QString {
  return QString(GF_TEST_SOURCE_DIR) + "/resource/screenshot/" + name;
}

auto ReadResource(const QString& name) -> QByteArray {
  QFile file(ResourcePath(name));
  if (!file.open(QIODevice::ReadOnly)) return {};
  return file.readAll();
}

auto Utc(const QString& text) -> QDateTime { return *ParseClock(text); }

}  // namespace

TEST(ScreenshotModeTest, ParseSizeAcceptsOnlyPositiveWxH) {
  EXPECT_EQ(ParseSize("1100x680"), QSize(1100, 680));
  EXPECT_EQ(ParseSize(" 320x200 "), QSize(320, 200));
  EXPECT_FALSE(ParseSize("0x680"));
  EXPECT_FALSE(ParseSize("1100X680"));
  EXPECT_FALSE(ParseSize("1100x"));
  EXPECT_FALSE(ParseSize("auto"));
}

TEST(ScreenshotModeTest, ParseThemeIsExact) {
  EXPECT_EQ(ParseTheme("light"), Theme::kLIGHT);
  EXPECT_EQ(ParseTheme("dark"), Theme::kDARK);
  EXPECT_FALSE(ParseTheme("Dark"));
  EXPECT_EQ(ThemeName(Theme::kDARK), "dark");
}

TEST(ScreenshotModeTest, ParseClockRequiresUtcDesignator) {
  const auto clock = ParseClock("2026-01-15T10:00:00Z");
  ASSERT_TRUE(clock);
  EXPECT_EQ(clock->toSecsSinceEpoch(), 1768471200);
  EXPECT_EQ(clock->offsetFromUtc(), 0);

  EXPECT_FALSE(ParseClock("2026-01-15T10:00:00"));
  EXPECT_FALSE(ParseClock("2026-01-15T10:00:00+01:00"));
  EXPECT_FALSE(ParseClock("2026-01-15 10:00:00Z"));
  EXPECT_FALSE(ParseClock("2026-02-30T10:00:00Z"));
  EXPECT_FALSE(ParseClock("2026-01-15T24:00:00Z"));
}

TEST(ScreenshotModeTest, GpgFakedTimeFollowsTheClock) {
  EXPECT_EQ(GpgFakedSystemTime(Utc("2026-01-15T10:00:00Z")),
            "20260115T100000!");
  EXPECT_EQ(GpgFakedSystemTime(Utc("2025-04-15T23:59:59Z")),
            "20250415T235959!");
}

TEST(ScreenshotModeTest, FontFamiliesDropBlanks) {
  EXPECT_EQ(ParseFontFamilies(" Noto Sans, ,Noto Sans CJK SC,"),
            QStringList({"Noto Sans", "Noto Sans CJK SC"}));
}

TEST(ScreenshotModeTest, GeometryJsonSitsBesideThePng) {
  EXPECT_EQ(GeometryJsonPath("/tmp/out/main-window.png"),
            "/tmp/out/main-window.json");
}

TEST(ScreenshotModeTest, RegistryHasThePhaseOneScenarios) {
  for (const auto* name :
       {"main-window", "key-generate-dialog", "text-decrypt-verify-result",
        "main-window-menu-popup"}) {
    const auto* s = FindScenario(name);
    ASSERT_NE(s, nullptr) << name;
    EXPECT_TRUE(s->listed) << name;
    EXPECT_FALSE(s->needs_agent_clock) << name;
  }
  EXPECT_EQ(FindScenario("main-window-menu-popup")->mode,
            CaptureMode::kCOMPOSITE);
  EXPECT_EQ(FindScenario("key-generate-dialog")->mode, CaptureMode::kDIALOG);
  EXPECT_FALSE(FindScenario("key-generate-dialog")->default_size.isValid());
  EXPECT_EQ(FindScenario("no-such-scenario"), nullptr);

  // Only the scenarios about key databases ask for the extra ones; every
  // other render keeps the database list startup derives.
  for (const auto& s : Scenarios()) {
    const bool about_databases = s.name == "key-database-menu"_L1 ||
                                 s.name == "settings-key-databases"_L1;
    EXPECT_EQ(s.key_databases.isEmpty(), !about_databases)
        << s.name.toStdString();
  }
  EXPECT_EQ(FindScenario("key-database-menu")->key_databases,
            QStringList({"work", "lab"}));
  EXPECT_EQ(FindScenario("main-window-key-groups")->mode, CaptureMode::kWINDOW);
  EXPECT_EQ(FindScenario("settings-key-databases")->mode, CaptureMode::kDIALOG);
  EXPECT_EQ(FindScenario("settings-key-databases")->default_size,
            QSize(900, 660));
}

TEST(ScreenshotModeTest, SelfTestsAreRegisteredButUnlisted) {
  for (const auto* name : {"selftest-timeout", "selftest-modal",
                           "selftest-gate", "selftest-agent-clock"}) {
    const auto* s = FindScenario(name);
    ASSERT_NE(s, nullptr) << name;
    EXPECT_FALSE(s->listed) << name;
  }
  EXPECT_TRUE(FindScenario("selftest-agent-clock")->needs_agent_clock);
}

TEST(ScreenshotModeTest, ScenarioNamesAreUnique) {
  QSet<QString> names;
  for (const auto& s : Scenarios()) {
    EXPECT_FALSE(names.contains(s.name)) << s.name.toStdString();
    names.insert(s.name);
  }
}

TEST(ScreenshotModeTest, MapToImageScalesByDpr) {
  const auto placed = MapToImage(QRect(10, 20, 30, 40), 2.0, QSize(400, 300));
  ASSERT_TRUE(placed);
  EXPECT_EQ(placed->rect, QRect(20, 40, 60, 80));
  EXPECT_FALSE(placed->clipped);
}

TEST(ScreenshotModeTest, MapToImageClipsToTheImage) {
  const auto placed = MapToImage(QRect(180, 140, 50, 50), 2.0, QSize(400, 300));
  ASSERT_TRUE(placed);
  EXPECT_EQ(placed->rect, QRect(360, 280, 40, 20));
  EXPECT_TRUE(placed->clipped);

  const auto negative = MapToImage(QRect(-5, -5, 10, 10), 2.0, QSize(400, 300));
  ASSERT_TRUE(negative);
  EXPECT_EQ(negative->rect, QRect(0, 0, 10, 10));
  EXPECT_TRUE(negative->clipped);
}

TEST(ScreenshotModeTest, MapToImageDropsWhatIsOutside) {
  EXPECT_FALSE(MapToImage(QRect(500, 10, 20, 20), 2.0, QSize(400, 300)));
  EXPECT_FALSE(MapToImage(QRect(10, 10, 0, 20), 2.0, QSize(400, 300)));
}

TEST(ScreenshotModeTest, CompositeBoundsGrowForAPopupPastTheWindow) {
  const QRect window(100, 100, 400, 300);
  const QRect popup(450, 150, 120, 200);
  EXPECT_EQ(CompositeBounds(window, {popup}), QRect(100, 100, 470, 300));
  EXPECT_EQ(CompositeBounds(window, {QRect(150, 150, 50, 50)}), window);
  // A submenu beside its menu: both widen the canvas.
  EXPECT_EQ(CompositeBounds(window, {popup, QRect(560, 200, 100, 250)}),
            QRect(100, 100, 560, 350));
  EXPECT_EQ(CompositeBounds(window, {}), window);
}

TEST(ScreenshotModeTest, CaptureWidgetPlacesChildrenAndReportsHidden) {
  RunOnMainThread([]() {
    QWidget window;
    window.resize(200, 100);
    auto* child = new QWidget(&window);
    child->setGeometry(10, 20, 30, 40);
    auto* hidden = new QWidget(&window);
    hidden->setGeometry(0, 0, 5, 5);
    hidden->hide();
    window.show();

    const auto capture = CaptureWidget(&window, CaptureMode::kWINDOW,
                                       {{"child", child, std::nullopt},
                                        {"part", child, QRect(5, 5, 10, 10)},
                                        {"hidden", hidden, std::nullopt},
                                        {"gone", nullptr, std::nullopt}});

    // The drawn title bar sits above the client area and shifts it down.
    const auto dpr = capture.dpr;
    EXPECT_EQ(capture.image.size(), QSize(200, 100 + kCaptionHeight) * dpr);
    ASSERT_EQ(capture.layers.size(), 2);
    EXPECT_EQ(capture.layers[0].name, "caption");
    EXPECT_EQ(capture.layers[0].rect,
              QRect(QPoint(0, 0), QSize(200, kCaptionHeight) * dpr));
    EXPECT_EQ(capture.layers[1].name, "window");
    EXPECT_EQ(capture.layers[1].rect,
              QRect(QPoint(0, kCaptionHeight) * dpr, QSize(200, 100) * dpr));
    ASSERT_TRUE(capture.widgets.contains("child"));
    EXPECT_EQ(
        capture.widgets["child"].rect,
        QRect(QPoint(10, 20 + kCaptionHeight) * dpr, QSize(30, 40) * dpr));
    EXPECT_EQ(
        capture.widgets["part"].rect,
        QRect(QPoint(15, 25 + kCaptionHeight) * dpr, QSize(10, 10) * dpr));
    EXPECT_EQ(capture.missing, QStringList({"hidden", "gone"}));

    const auto json = GeometryJson(capture, "unit");
    EXPECT_EQ(json["capture_mode"].toString(), "window");
    EXPECT_EQ(json["missing"].toArray().size(), 2);
  });
}

TEST(ScreenshotModeTest, CaptionIsDeterministicAndSizedToTheWindow) {
  RunOnMainThread([]() {
    QWidget window;
    window.setWindowTitle("Unit");
    const auto a = RenderCaption(&window, 300, 2.0);
    EXPECT_EQ(a.size(), QSize(300, kCaptionHeight) * 2);
    EXPECT_EQ(a.devicePixelRatio(), 2.0);
    EXPECT_TRUE(SameImage(a, RenderCaption(&window, 300, 2.0)));

    // A dialog has a close button only, so the area left of it stays empty
    // where a window draws its minimise and maximise glyphs.
    QDialog dialog;
    dialog.setWindowTitle("Unit");
    EXPECT_FALSE(SameImage(a, RenderCaption(&dialog, 300, 2.0)));
  });
}

TEST(ScreenshotModeTest, SameImageComparesPixels) {
  QImage a(4, 4, QImage::Format_ARGB32);
  a.fill(Qt::red);
  auto b = a.copy();
  EXPECT_TRUE(SameImage(a, b));
  b.setPixel(3, 3, qRgb(0, 0, 0));
  EXPECT_FALSE(SameImage(a, b));
  EXPECT_FALSE(SameImage(a, a.scaled(2, 2)));
}

TEST(ScreenshotModeTest, EnvHashMatchesThePythonFixture) {
  const auto doc =
      QJsonDocument::fromJson(ReadResource("env-hash-fixture.json"));
  ASSERT_TRUE(doc.isObject());
  const auto fixture = doc.object();
  const auto env = fixture["env"].toObject();

  EXPECT_EQ(QString::fromUtf8(CanonicalJson(EnvHashSubset(env))),
            fixture["canonical"].toString());
  EXPECT_EQ(EnvHashCore(env), fixture["env_hash_core"].toString());
}

TEST(ScreenshotModeTest, EnvHashIgnoresVolatileFields) {
  const auto env =
      QJsonDocument::fromJson(ReadResource("env-hash-fixture.json"))
          .object()["env"]
          .toObject();
  const auto base = EnvHashCore(env);

  auto volatile_env = env;
  volatile_env["display"] = ":99";
  volatile_env["profile_path"] = "/tmp/elsewhere";
  volatile_env["build_commit"] = "deadbeef";
  volatile_env["color_scheme_method"] = "palette";
  auto os = volatile_env["os"].toObject();
  os["kernel"] = "6.12.0";
  volatile_env["os"] = os;
  auto screen = volatile_env["screen"].toObject();
  screen["name"] = "other";
  screen["geometry"] = QJsonArray{1920, 0, 1920, 1080};
  volatile_env["screen"] = screen;
  EXPECT_EQ(EnvHashCore(volatile_env), base);

  for (const auto& [key, value] :
       {std::pair{"style", QJsonValue("fusion")},
        std::pair{"color_scheme", QJsonValue("light")},
        std::pair{"locale", QJsonValue("en_US")},
        std::pair{"clock", QJsonValue("2026-02-01T10:00:00Z")},
        std::pair{"font_resolved", QJsonValue(QJsonArray{"Noto Sans"})}}) {
    auto changed = env;
    changed[key] = value;
    EXPECT_NE(EnvHashCore(changed), base) << key;
  }
}

TEST(ScreenshotModeTest, CheckOptionsReadsTheOkayField) {
  EXPECT_EQ(ParseCheckOptions("gpg:OpenPGP:/usr/bin/gpg:1:1:\n", "gpg"), true);
  EXPECT_EQ(ParseCheckOptions("gpg:OpenPGP:/usr/bin/gpg:1:0:\n", "gpg"), false);
  EXPECT_EQ(
      ParseCheckOptions("gpg-agent:Private Keys:C%3a\\gpg-agent.exe:1:1:\r\n",
                        "gpg-agent"),
      true);
  EXPECT_FALSE(ParseCheckOptions("gpg-agent:Private Keys:/x:1:1:\n", "gpg"));
  EXPECT_FALSE(ParseCheckOptions("", "gpg"));
  EXPECT_FALSE(ParseCheckOptions("gpg:OpenPGP:/x:1:\n", "gpg"));
}

TEST(ScreenshotModeTest, DemoDataPredictsKeyStatesAtAClock) {
  QString error;
  const auto demo = ParseDemoData(ReadResource("demo-keys.json"), &error);
  ASSERT_TRUE(demo) << error.toStdString();
  ASSERT_EQ(demo->keys.size(), 5);

  const auto by_name = [](const QList<ExpectedKeyState>& states,
                          const QString& name) {
    for (const auto& s : states) {
      if (s.name == name) return s;
    }
    return ExpectedKeyState{};
  };

  const auto later = ExpectedKeyStates(demo->keys, Utc("2026-01-15T10:00:00Z"));
  EXPECT_TRUE(by_name(later, "Dave").expired);
  EXPECT_TRUE(by_name(later, "Carol").revoked);
  EXPECT_FALSE(by_name(later, "Alice").expired);
  EXPECT_FALSE(by_name(later, "Bob").revoked);

  const auto earlier =
      ExpectedKeyStates(demo->keys, Utc("2025-04-15T10:00:00Z"));
  EXPECT_FALSE(by_name(earlier, "Dave").expired);

  EXPECT_GT(demo->newest_timestamp,
            Utc("2025-03-31T12:00:00Z").toSecsSinceEpoch());

  // Erin is the "expiring soon" key: valid at the default clock, inside the
  // default 30-day window, expired once it passes.
  EXPECT_FALSE(by_name(later, "Erin").expired);
  EXPECT_TRUE(
      by_name(ExpectedKeyStates(demo->keys, Utc("2026-02-02T00:00:00Z")),
              "Erin")
          .expired);
  for (const auto& key : demo->keys) {
    if (key.name != "Erin"_L1) continue;
    ASSERT_TRUE(key.expires);
    const auto days = Utc(kDefaultClock)
                          .daysTo(QDateTime::fromSecsSinceEpoch(
                              *key.expires, QTimeZone::UTC));
    EXPECT_GT(days, 0);
    EXPECT_LE(days, 30);
  }
}

TEST(ScreenshotModeTest, DemoDataDescribesTheExtraDatabases) {
  QString error;
  const auto demo = ParseDemoData(ReadResource("demo-keys.json"), &error);
  ASSERT_TRUE(demo) << error.toStdString();

  const auto* work = demo->Database("work");
  const auto* lab = demo->Database("lab");
  ASSERT_NE(work, nullptr);
  ASSERT_NE(lab, nullptr);
  EXPECT_EQ(demo->Database("nope"), nullptr);

  EXPECT_EQ(work->backend, "gnupg");
  EXPECT_EQ(work->keys.size(), 2);
  EXPECT_TRUE(work->imports.isEmpty());

  EXPECT_EQ(lab->backend, "rpgp");
  EXPECT_EQ(lab->imports, QStringList({"ivan.key", "judy.cert"}));
  for (const auto& key : lab->keys) {
    EXPECT_EQ(key.fingerprint.size(), 64) << "an OpenPGP v6 fingerprint";
    EXPECT_TRUE(QFile::exists(ResourcePath("demo-dbs/lab/") +
                              lab->imports.value(&key - lab->keys.data())));
  }
}

TEST(ScreenshotModeTest, KeyDatabaseListPutsTheBuiltInFirst) {
  QString error;
  const auto demo = ParseDemoData(ReadResource("demo-keys.json"), &error);
  ASSERT_TRUE(demo);

  const auto list =
      BuildKeyDatabaseList({*demo->Database("work"), *demo->Database("lab")});
  EXPECT_EQ(list["default_database_id"].toString(), "default");

  const auto dbs = list["key_databases"].toArray();
  ASSERT_EQ(dbs.size(), 3);
  EXPECT_EQ(dbs[0]["id"].toString(), "default");
  EXPECT_EQ(dbs[0]["kind"].toString(), "default");
  EXPECT_EQ(dbs[0]["channel"].toInt(), 0);

  // Shaped like every other database id, and the same on every run.
  EXPECT_EQ(dbs[1]["id"].toString(), ScreenshotDatabaseId("work"));
  EXPECT_FALSE(QUuid(dbs[1]["id"].toString()).isNull());
  EXPECT_EQ(QUuid(dbs[1]["id"].toString()).version(), QUuid::Sha1);
  EXPECT_NE(ScreenshotDatabaseId("work"), ScreenshotDatabaseId("lab"));
  EXPECT_EQ(dbs[1]["name"].toString(), "Work");
  EXPECT_EQ(dbs[1]["path"].toString(), "@profile/dbs/Work");
  EXPECT_EQ(dbs[1]["backend_type"].toString(), "gnupg");
  EXPECT_EQ(dbs[1]["kind"].toString(), "managed");

  EXPECT_EQ(dbs[2]["backend_type"].toString(), "rpgp");
  EXPECT_EQ(dbs[2]["channel"].toInt(), 2);
}

TEST(ScreenshotModeTest, DemoDataRejectsMalformedInput) {
  QString error;
  EXPECT_FALSE(ParseDemoData("not json", &error));
  EXPECT_FALSE(ParseDemoData(R"({"schema":2,"keys":[]})", &error));
  EXPECT_FALSE(
      ParseDemoData(R"({"schema":1,"keys":[],"newest_timestamp":1})", &error));
}

TEST(ScreenshotModeTest, GatesAcceptTheReferenceEnvironment) {
  GateInputs inputs{2.0, 96, 0, {"Noto Sans"}, {"Noto Sans"}};
  EXPECT_TRUE(EvaluateGates(inputs).isEmpty());
}

TEST(ScreenshotModeTest, GatesNameEachFailure) {
  GateInputs inputs{3.0,
                    144,
                    3600,
                    {"Noto Sans", "Noto Sans CJK SC"},
                    {"Noto Sans", "DejaVu Sans"}};
  EXPECT_EQ(
      EvaluateGates(inputs),
      QStringList({"dpr", "logical-dpi", "timezone", "font:Noto Sans CJK SC"}));

  GateInputs none{2.0, 96, 0, {}, {}};
  EXPECT_EQ(EvaluateGates(none), QStringList({"font:none-requested"}));
}

TEST(ScreenshotModeTest, SizeGateReportsAClampedWindow) {
  EXPECT_FALSE(SizeGateFailure(QSize(1100, 680), QSize(1100, 680)));
  EXPECT_EQ(SizeGateFailure(QSize(1100, 680), QSize(960, 520)),
            "size:960x520!=1100x680");
}

TEST(ScreenshotModeTest, WaitUntilSeesAStateReachedBeforeItStarts) {
  RunOnMainThread([]() {
    ScenarioContext ctx({}, {}, 2000);
    bool triggered = false;
    EXPECT_TRUE(ctx.WaitUntil([&]() { triggered = true; }, [] { return true; },
                              {}, "already"));
    EXPECT_FALSE(triggered);
    EXPECT_FALSE(ctx.GetFailure());
  });
}

TEST(ScreenshotModeTest, WaitUntilSeesASignalFromInsideTheTrigger) {
  RunOnMainThread([]() {
    ScenarioContext ctx({}, {}, 5000);
    QStandardItemModel model;
    bool reset_seen = false;
    QElapsedTimer timer;
    timer.start();
    // The trigger spins its own loop and the signal fires inside it: the
    // wait must not then sit in exec() until the deadline.
    EXPECT_TRUE(ctx.WaitUntil(
        [&]() {
          QEventLoop inner;
          QTimer::singleShot(10, &inner, [&]() {
            model.clear();
            inner.quit();
          });
          inner.exec();
        },
        [&] { return reset_seen; }, {[&](const std::function<void()>& check) {
          return QObject::connect(&model, &QAbstractItemModel::modelReset,
                                  &model, [&reset_seen, check]() {
                                    reset_seen = true;
                                    check();
                                  });
        }},
        "inside-trigger"));
    EXPECT_LT(timer.elapsed(), 2000);
  });
}

TEST(ScreenshotModeTest, WaitUntilPollsStateWithoutASignal) {
  RunOnMainThread([]() {
    ScenarioContext ctx({}, {}, 5000);
    bool ready = false;
    EXPECT_TRUE(ctx.WaitUntil(
        [&]() { QTimer::singleShot(50, [&ready]() { ready = true; }); },
        [&] { return ready; }, {}, "polled"));
  });
}

TEST(ScreenshotModeTest, WaitUntilTimesOutWithItsLabel) {
  RunOnMainThread([]() {
    ScenarioContext ctx({}, {}, 100);
    EXPECT_FALSE(ctx.WaitUntil({}, [] { return false; }, {}, "never"));
    ASSERT_TRUE(ctx.GetFailure());
    EXPECT_EQ(ctx.GetFailure()->exit_code, kEXIT_TIMEOUT);
    EXPECT_EQ(ctx.GetFailure()->reason, "timeout");
    EXPECT_EQ(ctx.GetFailure()->detail, "never");

    // The first failure wins, and a failed context waits no more.
    ctx.Fail(kEXIT_USAGE, "later");
    EXPECT_EQ(ctx.GetFailure()->reason, "timeout");
    EXPECT_FALSE(ctx.WaitUntil({}, [] { return true; }, {}, "after"));
  });
}

}  // namespace GpgFrontend::Test
