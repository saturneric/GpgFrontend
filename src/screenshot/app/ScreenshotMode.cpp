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

#include "screenshot/app/ScreenshotMode.h"

#include <QApplication>
#include <QCursor>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFontDatabase>
#include <QFontInfo>
#include <QImageWriter>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QProcess>
#include <QSaveFile>
#include <QScreen>
#include <QStyle>
#include <QStyleHints>
#include <QSysInfo>
#include <QTextStream>
#include <QTimeZone>
#include <QTimer>
#include <QWindow>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <tuple>

#include "Initialize.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/model/GpgAbstractKey.h"
#include "core/module/ModuleManager.h"
#include "core/profile/Profile.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/CommonUtils.h"
#include "screenshot/ScreenshotCapture.h"
#include "screenshot/ScreenshotEnv.h"
#include "screenshot/ScreenshotOptions.h"
#include "screenshot/ScreenshotScenario.h"
#include "screenshot/ScreenshotSeed.h"
#include "ui/GpgFrontendUIInit.h"
#include "ui/function/UIStyle.h"

namespace GpgFrontend::Screenshot {

namespace {

constexpr auto kOptScreenshot = "screenshot";
constexpr auto kOptList = "screenshot-list";
constexpr auto kOptEnv = "screenshot-env";

/// Comma separated faults to inject, for testing the failure paths:
/// "gate", "gpg-reject", "agent-reject".
constexpr auto kInjectEnv = "GF_SCREENSHOT_INJECT";

constexpr int kGpgconfTimeoutMs = 15000;

/// The temporary profile's anchor in every render: 16 lowercase hex.
constexpr auto kScreenshotProfileAnchor = "5c3ee45a0b5c5e55";

/// When the demo key group and category were made: before every other
/// demo timestamp the key list could show next to them.
auto DemoSeedTime() -> QDateTime {
  return {QDate(2025, 3, 15), QTime(12, 0), QTimeZone::UTC};
}
/// Past the scenario deadline, a run stuck in a nested loop is ended hard.
constexpr int kWatchdogGraceMs = 5000;
/// 96 dpi, so the PNG pHYs chunk does not depend on the screen.
constexpr int kDotsPerMeter = 3780;

auto Injected(const QString& fault) -> bool {
  return qEnvironmentVariable(kInjectEnv)
      .split(QLatin1Char(','), Qt::SkipEmptyParts)
      .contains(fault);
}

auto HasArg(int argc, char** argv, const char* flag) -> bool {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], flag) == 0) return true;
  }
  return false;
}

auto Err() -> QTextStream& {
  static QTextStream stream(stderr);
  return stream;
}

auto WriteFile(const QString& path, const QByteArray& data) -> bool {
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) return false;
  file.write(data);
  return file.commit();
}

auto ErrorJson(const Failure& f) -> QJsonObject {
  return {{"code", f.exit_code}, {"reason", f.reason}, {"detail", f.detail}};
}

/**
 * @brief Everything one run knows, gathered as it goes.
 */
struct Run {
  GFCxtSPtr ctx;
  Options options;
  const Scenario* scenario = nullptr;
  QSize size;
  QString db_path;
  DemoData demo;
  /// The extra databases the scenario asked for, and where each lives.
  QList<DemoDatabase> databases;
  QMap<QString, QString> database_paths;
  QJsonObject gpg_clock{{"gpg_conf", "unchecked"}, {"agent_conf", "unchecked"}};
  QString ui_clock_probe = u"unchecked"_s;
  QString gpg_clock_probe = u"unchecked"_s;
  QString color_scheme_method;
  QStringList gate_failures;
  QJsonObject env;
};

auto JsonPathFor(const Options& o) -> QString {
  return o.env_probe ? o.out : GeometryJsonPath(o.out);
}

/**
 * @brief Report @p f on stderr and in the JSON the pipeline reads.
 */
auto Report(const Run& run, const Failure& f) -> int {
  Err() << "screenshot: " << f.reason;
  if (!f.detail.isEmpty()) Err() << ": " << f.detail;
  Err() << Qt::endl;

  if (!run.options.out.isEmpty()) {
    QJsonObject root{{"schema", 1},
                     {"scenario", run.options.scenario},
                     {"env", run.env},
                     {"error", ErrorJson(f)}};
    WriteFile(JsonPathFor(run.options), QJsonDocument(root).toJson());
  }
  return f.exit_code;
}

auto RunGpgconf(const QStringList& args, QString* output) -> bool {
  const auto gpgconf = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});
  if (gpgconf.isEmpty()) return false;

  QProcess process;
  process.start(gpgconf, args);
  if (!process.waitForFinished(kGpgconfTimeoutMs)) {
    process.kill();
    return false;
  }
  if (output != nullptr) {
    *output = QString::fromUtf8(process.readAllStandardOutput()) +
              QString::fromUtf8(process.readAllStandardError());
  }
  return process.exitStatus() == QProcess::NormalExit;
}

void KillAgents(const Run& run) {
  if (run.db_path.isEmpty()) return;
  RunGpgconf({"--homedir", run.db_path, "--kill", "all"}, nullptr);
}

/**
 * @brief For a run that cannot unwind: a modal blocking in its own loop, or
 *        a wait that never returned.
 */
[[noreturn]] void HardExit(const Run& run, const Failure& f) {
  Report(run, f);
  KillAgents(run);
  // What the shutdown watchdog does too: the temporary profile's tree is
  // deleted, or it would outlive the process in memory-backed storage and
  // stand in the way of the next run, which uses the same name.
  if (ProfileSession::Loaded()) {
    ProfileSession::Instance().Unload(ProfileUnmountMode::kFORCED);
  }
  std::_Exit(f.exit_code);
}

auto ParseOptions(const QCommandLineParser& parser, Options* out) -> Failure {
  Options o;
  o.env_probe = parser.isSet(kOptEnv);
  o.scenario = parser.value(kOptScreenshot);
  o.out = parser.value("out");
  o.locale = parser.isSet("locale") ? parser.value("locale") : o.locale;
  o.demo_gnupghome = parser.value("demo-gnupghome");
  o.fixtures = parser.value("fixtures");
  *out = o;

  if (o.out.isEmpty()) return {kEXIT_USAGE, "missing-option", "--out"};
  if (!QFileInfo(QFileInfo(o.out).absolutePath()).isWritable()) {
    return {kEXIT_OUTPUT, "output-not-writable", o.out};
  }
  if (o.demo_gnupghome.isEmpty() || !QDir(o.demo_gnupghome).exists()) {
    return {kEXIT_USAGE, "missing-option", "--demo-gnupghome"};
  }

  if (parser.isSet("size")) {
    o.size = ParseSize(parser.value("size"));
    if (!o.size) return {kEXIT_USAGE, "invalid-size", parser.value("size")};
  }
  if (parser.isSet("theme")) {
    const auto theme = ParseTheme(parser.value("theme"));
    if (!theme) return {kEXIT_USAGE, "invalid-theme", parser.value("theme")};
    o.theme = *theme;
  }

  const auto clock_text =
      parser.isSet("clock") ? parser.value("clock") : QString(kDefaultClock);
  const auto clock = ParseClock(clock_text);
  if (!clock) return {kEXIT_USAGE, "invalid-clock", clock_text};
  o.clock = *clock;

  o.font_families = ParseFontFamilies(parser.isSet("font-family")
                                          ? parser.value("font-family")
                                          : QString(kDefaultFontFamilies));

  if (parser.isSet("timeout")) {
    bool ok = false;
    o.timeout_ms = parser.value("timeout").toInt(&ok);
    if (!ok || o.timeout_ms <= 0) {
      return {kEXIT_USAGE, "invalid-timeout", parser.value("timeout")};
    }
  }

  *out = o;
  return {kEXIT_OK, {}, {}};
}

auto CopyTree(const QString& from, const QString& to) -> bool {
  if (!QDir().mkpath(to)) return false;
  QDirIterator it(
      from, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden,
      QDirIterator::Subdirectories);
  const QDir source(from);
  while (it.hasNext()) {
    const auto path = it.next();
    const auto target = QDir(to).filePath(source.relativeFilePath(path));
    if (it.fileInfo().isDir()) {
      if (!QDir().mkpath(target)) return false;
    } else if (!QFile::copy(path, target)) {
      return false;
    }
  }
  QFile::setPermissions(to, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                QFileDevice::ExeOwner);
  return true;
}

auto WriteConf(const QString& path, const QString& body) -> bool {
  return WriteFile(path, body.toUtf8());
}

/**
 * @brief Copy the demo keyring into the temporary profile's own key
 *        database, and set up the extra databases the scenario asks for.
 *        Conf files are written later, by ValidateClock().
 */
auto Seed(Run& run) -> Failure {
  const auto demo_keys =
      QDir(run.options.demo_gnupghome).filePath("../demo-keys.json");
  QFile file(demo_keys);
  if (!file.open(QIODevice::ReadOnly)) {
    return {kEXIT_USAGE, "demo-keys-missing", demo_keys};
  }
  QString error;
  auto demo = ParseDemoData(file.readAll(), &error);
  if (!demo) return {kEXIT_USAGE, "demo-keys-invalid", error};
  run.demo = *demo;

  if (run.options.clock.toSecsSinceEpoch() < run.demo.newest_timestamp) {
    return {
        kEXIT_USAGE, "clock-before-demo-data",
        QDateTime::fromSecsSinceEpoch(run.demo.newest_timestamp, QTimeZone::UTC)
            .toString(Qt::ISODate)};
  }

  run.db_path =
      QDir(GlobalSettingStation::GetInstance().GetAppDataPath()).filePath("db");
  // The temporary profile has the same name on every run. A keyring already
  // here is a session a crash left behind, and its settings and caches would
  // leak into this render; it is not reused.
  if (QFileInfo::exists(run.db_path)) {
    return {kEXIT_USAGE, "stale-session",
            u"remove %1 and try again"_s.arg(
                GlobalSettingStation::GetInstance().GetAppDataPath())};
  }
  if (!CopyTree(run.options.demo_gnupghome, run.db_path)) {
    return {kEXIT_USAGE, "seed-failed", run.db_path};
  }

  const auto app_data =
      QDir(GlobalSettingStation::GetInstance().GetAppDataPath());
  const auto demo_root =
      QDir(run.options.demo_gnupghome).filePath("../demo-dbs");
  for (const auto& id :
       run.scenario != nullptr ? run.scenario->key_databases : QStringList{}) {
    const auto* db = run.demo.Database(id);
    if (db == nullptr) return {kEXIT_USAGE, "unknown-demo-database", id};

    const auto path = app_data.filePath(ScreenshotDatabaseDir(*db));
    const auto seeded = db->backend == "gnupg"_L1
                            ? CopyTree(QDir(demo_root).filePath(id), path)
                            : QDir().mkpath(path);
    if (!seeded) return {kEXIT_USAGE, "seed-failed", path};
    run.databases.append(*db);
    run.database_paths.insert(id, path);
  }
  // Only when asked for: every other render keeps the list startup derives.
  if (!run.databases.isEmpty() && !StoreKeyDatabaseList(run.databases)) {
    return {kEXIT_USAGE, "seed-failed", "key_database_list"};
  }

  auto settings = GetSettings();
  SettingItems::kLanguage.Write(settings, run.options.locale);
  SettingItems::kShowWizard.Write(settings, false);
  settings.sync();
  return {kEXIT_OK, {}, {}};
}

auto IsDark(const QPalette& palette) -> bool {
  return palette.color(QPalette::WindowText).lightness() >
         palette.color(QPalette::Window).lightness();
}

void RequestColorScheme(Theme theme) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
  QGuiApplication::styleHints()->setColorScheme(
      theme == Theme::kDARK ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
#else
  Q_UNUSED(theme);
#endif
}

/**
 * @brief After the style is in place: keep the hint if it worked, otherwise
 *        set the palette by hand.
 */
auto EnforceColorScheme(Theme theme) -> QString {
  const bool want_dark = theme == Theme::kDARK;
  if (IsDark(QApplication::palette()) == want_dark) return u"hint"_s;
  QApplication::setPalette(want_dark
                               ? UI::DarkFusionPalette()
                               : QApplication::style()->standardPalette());
  return u"palette"_s;
}

void FixUiEnvironment(const Options& o) {
  for (const auto effect :
       {Qt::UI_AnimateMenu, Qt::UI_FadeMenu, Qt::UI_AnimateCombo,
        Qt::UI_AnimateTooltip, Qt::UI_FadeTooltip, Qt::UI_AnimateToolBox}) {
    QApplication::setEffectEnabled(effect, false);
  }
  QApplication::setCursorFlashTime(0);

  auto font = QApplication::font();
  font.setFamilies(o.font_families);
  QApplication::setFont(font);
}

auto ResolvedFamilies(const QStringList& requested) -> QStringList {
  QStringList resolved;
  for (const auto& family : requested) {
    resolved.append(QFontDatabase::hasFamily(family)
                        ? QFontInfo(QFont(family)).family()
                        : QString());
  }
  return resolved;
}

auto ScreenJson() -> QJsonObject {
  auto* screen = QGuiApplication::primaryScreen();
  if (screen == nullptr) return {};
  const auto g = screen->geometry();
  return {{"name", screen->name()},
          {"size", QJsonArray{g.width(), g.height()}},
          {"geometry", QJsonArray{g.x(), g.y(), g.width(), g.height()}},
          {"dpr", static_cast<int>(std::lround(screen->devicePixelRatio()))},
          {"logical_dpi",
           static_cast<int>(std::lround(screen->logicalDotsPerInch()))},
          {"physical_dpi",
           static_cast<int>(std::lround(screen->physicalDotsPerInch()))}};
}

auto BuildEnv(const Run& run) -> QJsonObject {
  const auto& o = run.options;
#if defined(Q_OS_WINDOWS)
  const auto os_build = QSysInfo::kernelVersion();
#else
  const auto os_build = QSysInfo::productVersion();
#endif

  QJsonObject env{
      {"schema", 1},
      {"os", QJsonObject{{"product", QSysInfo::prettyProductName()},
                         {"kernel", QSysInfo::kernelVersion()},
                         {"build", os_build}}},
      {"qt_version", QString::fromLatin1(qVersion())},
      {"platform_name", QGuiApplication::platformName()},
      {"screen", ScreenJson()},
      {"style", QApplication::style()->name()},
      {"color_scheme", ThemeName(o.theme)},
      {"color_scheme_method", run.color_scheme_method},
      {"locale", o.locale},
      {"clock", o.clock.toString(Qt::ISODate)},
      {"font_requested", QJsonArray::fromStringList(o.font_families)},
      {"font_resolved",
       QJsonArray::fromStringList(ResolvedFamilies(o.font_families))},
      {"system_font",
       QFontDatabase::systemFont(QFontDatabase::GeneralFont).family()},
      {"gnupg_version", Module::RetrieveRTValueTypedOrDefault<>(
                            "core", "gpgme.ctx.gnupg_version", QString{})},
      {"gpgme_version", Module::RetrieveRTValueTypedOrDefault<>(
                            "core", "gpgme.version", QString{})},
      {"gpg_clock", run.gpg_clock},
      {"ui_clock_probe", run.ui_clock_probe},
      {"gpg_clock_probe", run.gpg_clock_probe},
      {"tz_offset_check",
       QDateTime::fromSecsSinceEpoch(o.clock.toSecsSinceEpoch())
           .offsetFromUtc()},
      {"build_commit", GetProjectBuildGitCommitHash()},
      {"display", qEnvironmentVariable("DISPLAY")},
      {"profile_path", GlobalSettingStation::GetInstance().GetAppDataPath()},
      {"gate_failures", QJsonArray::fromStringList(run.gate_failures)},
  };
  env.insert("env_hash_core", EnvHashCore(env));
  return env;
}

/**
 * @brief Pin one GnuPG homedir's clock and have gpgconf vet both conf files.
 *
 * @return whether gpg.conf and gpg-agent.conf were accepted, and gpgconf's
 *         word on the agent's when it was not
 */
auto PinGnuPGClock(const QString& home, const QDateTime& clock)
    -> std::tuple<bool, bool, QString> {
  // Written only now, after core startup has listed the keys: a conf gpg
  // refuses makes that listing fail. gpg reads gpg.conf on every invocation,
  // so the cache startup filled is listed again before anything reads it.
  const auto line = u"faked-system-time %1\n"_s.arg(GpgFakedSystemTime(clock));
  auto conf = line;
  if (Injected("gpg-reject")) {
    conf += u"no-such-option-injected\n"_s;
  }
  WriteConf(QDir(home).filePath("gpg.conf"), conf);

  QString output;
  const auto gpg_ok =
      RunGpgconf({"--homedir", home, "--check-options", "gpg"}, &output)
          ? ParseCheckOptions(output, "gpg").value_or(false)
          : false;

  const auto agent_conf = QDir(home).filePath("gpg-agent.conf");
  WriteConf(agent_conf, line);
  const auto agent_ok =
      RunGpgconf({"--homedir", home, "--check-options", "gpg-agent"}, &output)
          ? ParseCheckOptions(output, "gpg-agent").value_or(false)
          : false;
  if (agent_ok && !Injected("agent-reject")) {
    // The agent reads its clock at startup; one started during core init
    // without it is ended so the next use starts on the vetted config.
    RunGpgconf({"--homedir", home, "--kill", "gpg-agent"}, nullptr);
    return {gpg_ok, true, {}};
  }
  // An agent that refuses its config does not start, and decryption with it;
  // the file goes unless a scenario needs the agent's clock.
  QFile::remove(agent_conf);
  return {gpg_ok, false,
          Injected("agent-reject") ? u"injected"_s : output.trimmed()};
}

/**
 * @brief What GnuPG says of @p keys on @p channel, against what it should.
 */
auto ProbeKeyStates(int channel, const QList<DemoKey>& keys,
                    const QDateTime& clock, const QString& prefix)
    -> QStringList {
  QStringList mismatches;
  for (const auto& expected : ExpectedKeyStates(keys, clock)) {
    const auto key = AbstractKeyRepository::GetInstance(channel).GetKey(
        expected.fingerprint);
    const auto name = prefix + expected.name;
    if (key == nullptr) {
      mismatches.append(name + u":missing"_s);
      continue;
    }
    if (key->IsExpired() != expected.expired) {
      mismatches.append(u"%1:expired=%2,want=%3"_s.arg(name)
                            .arg(key->IsExpired())
                            .arg(expected.expired));
    }
    if (key->IsRevoked() != expected.revoked) {
      mismatches.append(u"%1:revoked=%2,want=%3"_s.arg(name)
                            .arg(key->IsRevoked())
                            .arg(expected.revoked));
    }
  }
  return mismatches;
}

/**
 * @brief Prove that GnuPG and the UI both run on the requested clock, in
 *        every database the run opened.
 */
auto ValidateClock(Run& run) -> QStringList {
  QStringList failures;

  // Every GnuPG homedir of the run: the built-in one and any extra.
  QList<std::pair<QString, QString>> homes{{QString(), run.db_path}};
  for (const auto& db : run.databases) {
    if (db.backend == "gnupg"_L1) {
      homes.append({db.id, run.database_paths.value(db.id)});
    }
  }

  bool all_gpg = true;
  bool all_agent = true;
  QString agent_detail;
  for (const auto& [id, home] : homes) {
    const auto [gpg_ok, agent_ok, detail] =
        PinGnuPGClock(home, run.options.clock);
    all_gpg = all_gpg && gpg_ok;
    if (!agent_ok) {
      all_agent = false;
      if (agent_detail.isEmpty()) agent_detail = detail;
    }
  }
  run.gpg_clock.insert("gpg_conf", all_gpg ? "applied" : "rejected");
  run.gpg_clock.insert(
      "agent_conf", all_agent ? u"applied"_s : u"rejected:"_s + agent_detail);
  if (!all_gpg) failures.append(u"gpg-clock-option-rejected:gpg"_s);
  if (!all_agent && run.scenario != nullptr &&
      run.scenario->needs_agent_clock) {
    failures.append(u"gpg-clock-option-rejected:gpg-agent"_s);
  }
  if (!failures.isEmpty()) return failures;

  const auto builtin = UsableDefaultChannelHandle();
  AbstractKeyRepository::GetInstance(builtin).FlushCache();
  auto mismatches =
      ProbeKeyStates(builtin, run.demo.keys, run.options.clock, QString());

  for (const auto& db : run.databases) {
    const auto channel = ResolveDatabaseHandle(ScreenshotDatabaseId(db.id));
    if (channel < 0) {
      mismatches.append(db.id + u":not-open"_s);
      continue;
    }
    if (db.backend == "rpgp"_L1) {
      const auto demo_root =
          QDir(run.options.demo_gnupghome).filePath("../demo-dbs/" + db.id);
      if (const auto err = ImportDemoKeys(channel, demo_root, db);
          !err.isEmpty()) {
        mismatches.append(db.id + QLatin1Char(':') + err);
        continue;
      }
    } else {
      AbstractKeyRepository::GetInstance(channel).FlushCache();
    }
    mismatches += ProbeKeyStates(channel, db.keys, run.options.clock,
                                 db.id + QLatin1Char('/'));
  }

  run.gpg_clock_probe = mismatches.isEmpty() ? u"ok"_s : mismatches.join(';');
  if (!mismatches.isEmpty()) {
    failures.append(u"gpg-clock-not-effective"_s);
  }

  const auto ui_now = CurrentDateTime().toSecsSinceEpoch();
  run.ui_clock_probe = ui_now == run.options.clock.toSecsSinceEpoch()
                           ? u"ok"_s
                           : QString::number(ui_now);
  if (run.ui_clock_probe != "ok"_L1) {
    failures.append(u"ui-clock-mismatch"_s);
  }
  return failures;
}

auto EvaluateEnvironment(Run& run) -> QStringList {
  auto* screen = QGuiApplication::primaryScreen();
  GateInputs inputs;
  inputs.dpr = screen != nullptr ? screen->devicePixelRatio() : 0;
  inputs.logical_dpi =
      screen != nullptr
          ? static_cast<int>(std::lround(screen->logicalDotsPerInch()))
          : 0;
  inputs.tz_offset_seconds =
      QDateTime::fromSecsSinceEpoch(run.options.clock.toSecsSinceEpoch())
          .offsetFromUtc();
  inputs.requested_families = run.options.font_families;
  inputs.resolved_families = ResolvedFamilies(run.options.font_families);

  auto failures = EvaluateGates(inputs);
  failures += ValidateClock(run);
  if (Injected("gate") ||
      (run.scenario != nullptr && run.scenario->name == "selftest-gate")) {
    failures.append(u"selftest-injected"_s);
  }
  return failures;
}

/**
 * @brief Ends the run on any real window the scenario did not declare.
 *        Popups and tooltips never reach a grab, so they are left alone.
 */
class ModalGuard : public QObject {
 public:
  ModalGuard(QStringList allowed, std::function<void(QWidget*)> on_unexpected)
      : allowed_(std::move(allowed)),
        on_unexpected_(std::move(on_unexpected)) {}

  auto eventFilter(QObject* watched, QEvent* event) -> bool override {
    if (event->type() != QEvent::Show || !watched->isWidgetType()) return false;
    auto* w = static_cast<QWidget*>(watched);
    if (!w->isWindow()) return false;

    const auto type = w->windowType();
    if (type != Qt::Window && type != Qt::Dialog && type != Qt::Sheet) {
      return false;
    }
    for (const auto& name : allowed_) {
      if (w->inherits(name.toLatin1().constData())) return false;
    }
    on_unexpected_(w);
    return false;
  }

 private:
  QStringList allowed_;
  std::function<void(QWidget*)> on_unexpected_;
};

auto SaveCapture(const Run& run, const Capture& capture) -> Failure {
  auto image = capture.image;
  image.setDotsPerMeterX(kDotsPerMeter);
  image.setDotsPerMeterY(kDotsPerMeter);

  QImageWriter writer(run.options.out, "png");
  if (!writer.write(image)) {
    return {kEXIT_OUTPUT, "png-write-failed", writer.errorString()};
  }

  auto root = GeometryJson(capture, run.options.scenario);
  root.insert("env", run.env);
  root.insert("error", QJsonValue::Null);
  if (!WriteFile(GeometryJsonPath(run.options.out),
                 QJsonDocument(root).toJson())) {
    return {kEXIT_OUTPUT, "json-write-failed",
            GeometryJsonPath(run.options.out)};
  }
  return {kEXIT_OK, {}, {}};
}

auto RenderScenario(const Run& run) -> int {
  ScenarioContext ctx(run.options, run.size, run.options.timeout_ms);
  const auto ok = run.scenario->setup(ctx);

  if (const auto& failure = ctx.GetFailure()) return Report(run, *failure);
  if (!ok || ctx.Target() == nullptr) {
    return Report(run, {kEXIT_USAGE, "scenario-failed", run.scenario->name});
  }

  const auto capture =
      ctx.Mode() == CaptureMode::kCOMPOSITE && !ctx.Popups().isEmpty()
          ? CaptureComposite(ctx.Target(), ctx.Popups(), ctx.Highlights())
          : CaptureWidget(ctx.Target(), ctx.Mode(), ctx.Highlights());
  if (std::lround(capture.dpr) != 2) {
    return Report(run, {kEXIT_ENVIRONMENT, "gate", "capture-dpr"});
  }

  const auto saved = SaveCapture(run, capture);
  if (saved.exit_code != kEXIT_OK) return Report(run, saved);
  return kEXIT_OK;
}

auto ProbeEnvironment(Run& run) -> int {
  QWidget probe;
  if (run.options.size) probe.resize(*run.options.size);
  probe.show();

  ScenarioContext ctx(run.options, run.size, run.options.timeout_ms);
  const auto shown = ctx.WaitUntil(
      {},
      [&]() {
        return probe.windowHandle() != nullptr &&
               probe.windowHandle()->isExposed();
      },
      {}, u"probe-window-exposed"_s);
  if (!shown) return Report(run, *ctx.GetFailure());

  if (run.options.size) {
    if (const auto bad = SizeGateFailure(*run.options.size, probe.size())) {
      run.gate_failures.append(*bad);
      run.env = BuildEnv(run);
      return Report(run, {kEXIT_ENVIRONMENT, "gate", *bad});
    }
  }

  QJsonObject record = run.env;
  record.insert("error", QJsonValue::Null);
  if (!WriteFile(run.options.out, QJsonDocument(record).toJson())) {
    return Report(run, {kEXIT_OUTPUT, "json-write-failed", run.options.out});
  }
  return kEXIT_OK;
}

}  // namespace

auto PrepareScreenshotEnvironment(int argc, char** argv) -> std::optional<int> {
  if (HasArg(argc, argv, "--screenshot-list")) {
    QTextStream out(stdout);
    for (const auto& s : Scenarios()) {
      if (!s.listed) continue;
      const auto size = s.default_size.isValid()
                            ? u"%1x%2"_s.arg(s.default_size.width())
                                  .arg(s.default_size.height())
                            : u"auto"_s;
      out << s.name << '\t' << size << '\t' << CaptureModeName(s.mode) << '\n';
    }
    return kEXIT_OK;
  }

  if (!HasArg(argc, argv, "--screenshot") &&
      !HasArg(argc, argv, "--screenshot-env")) {
    return std::nullopt;
  }

  // Exactly 2x and 96 dpi whatever the desktop scaling is; the gates prove
  // it took effect. UTC so absolute dates render the same everywhere.
  qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
  qputenv("QT_SCALE_FACTOR", "2");
  qputenv("QT_FONT_DPI", "96");
  qunsetenv("QT_SCREEN_SCALE_FACTORS");
  qunsetenv("QT_AUTO_SCREEN_SCALE_FACTOR");
  qunsetenv("QT_SCALE_FACTOR_ROUNDING_POLICY");
  qputenv("TZ", "UTC");
  return std::nullopt;
}

void AddScreenshotOptions(QCommandLineParser& parser) {
  const auto hidden = [&parser](QCommandLineOption option) {
    option.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(option);
  };
  hidden({kOptScreenshot, "Render a scenario.", "scenario"});
  hidden({kOptList, "List the screenshot scenarios."});
  hidden({kOptEnv, "Write the rendering environment record."});
  hidden({"locale", "Locale of the render.", "locale"});
  hidden({"out", "Output file.", "file"});
  hidden({"size", "Logical size, WxH.", "size"});
  hidden({"theme", "light or dark.", "theme"});
  hidden({"demo-gnupghome", "Demo keyring to copy.", "dir"});
  hidden({"fixtures", "Fixture directory.", "dir"});
  hidden({"font-family", "Comma separated font families.", "families"});
  hidden({"clock", "Fixed UTC clock, yyyy-MM-ddTHH:mm:ssZ.", "time"});
  hidden({"timeout", "Readiness deadline in milliseconds.", "ms"});
}

auto MakeScreenshotProfile(const ProfileSelection& selection,
                           const QCommandLineParser& parser)
    -> QSharedPointer<Profile> {
  if (!IsScreenshotRun(parser) || selection.kind != ProfileKind::kEPHEMERAL) {
    return MakeProfile(selection);
  }
  // One anchor for every run: two renders never run at the same time, and a
  // session a hard exit left behind is swept like any abandoned one.
  return QSharedPointer<EphemeralProfile>::create(
      selection.profiles_root, selection.inherited_settings,
      QLatin1StringView(kScreenshotProfileAnchor));
}

auto IsScreenshotRun(const QCommandLineParser& parser) -> bool {
  return parser.isSet(kOptScreenshot) || parser.isSet(kOptEnv);
}

auto RunScreenshotMode(const GFCxtSPtr& ctx, const QCommandLineParser& parser)
    -> int {
  Run run;
  run.ctx = ctx;

  if (const auto f = ParseOptions(parser, &run.options);
      f.exit_code != kEXIT_OK) {
    return Report(run, f);
  }

  if (!run.options.env_probe) {
    run.scenario = FindScenario(run.options.scenario);
    if (run.scenario == nullptr) {
      return Report(run,
                    {kEXIT_USAGE, "unknown-scenario", run.options.scenario});
    }
    run.size = run.options.size.value_or(run.scenario->default_size);
  }

  // Never against a real profile: the temporary one is self-contained, has
  // no PIN and is deleted on exit.
  if (ProfileSession::Instance().Profile().Kind() != ProfileKind::kEPHEMERAL) {
    return Report(run,
                  {kEXIT_USAGE, "profile-not-ephemeral", "--temp-profile"});
  }

  if (const auto f = Seed(run); f.exit_code != kEXIT_OK) return Report(run, f);

  ctx->gather_external_gnupg_info = false;
  ctx->unit_test_mode = false;
  InitGlobalBasicEnv(ctx, true);
  Module::UpsertRTValue(
      "core", kFixedClockRTKey,
      static_cast<qint64>(run.options.clock.toSecsSinceEpoch()));

  auto* app = ctx->GetApp();
  InitLocale();
  RequestColorScheme(run.options.theme);
  UI::InitGpgFrontendUI(app);
  UI::WaitingAllInitializationFinished();
  UI::InitModulesTranslations();
  run.color_scheme_method = EnforceColorScheme(run.options.theme);
  FixUiEnvironment(run.options);

  run.gate_failures = EvaluateEnvironment(run);
  run.env = BuildEnv(run);

  int rtn = kEXIT_OK;
  if (!run.gate_failures.isEmpty()) {
    rtn = Report(run, {kEXIT_ENVIRONMENT, "gate", run.gate_failures.join(',')});
  } else if (const auto err = SeedGroupsAndCategories(
                 UsableDefaultChannelHandle(), run.demo, DemoSeedTime());
             !err.isEmpty()) {
    rtn = Report(run, {kEXIT_USAGE, "seed-failed", err});
  } else if (run.options.env_probe) {
    rtn = ProbeEnvironment(run);
  } else {
    // Nothing on screen may be hovered: the cursor waits in a corner the
    // windows, placed top-left, never reach.
    if (auto* screen = QGuiApplication::primaryScreen(); screen != nullptr) {
      QCursor::setPos(screen->geometry().bottomRight() - QPoint(1, 1));
    }

    ModalGuard guard(run.scenario->allowed_windows, [&run](QWidget* w) {
      HardExit(run, {kEXIT_UNEXPECTED_MODAL, "unexpected-window",
                     u"%1 \"%2\""_s.arg(
                         QString::fromLatin1(w->metaObject()->className()),
                         w->windowTitle())});
    });
    app->installEventFilter(&guard);

    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, [&run]() {
      HardExit(run, {kEXIT_TIMEOUT, "watchdog", run.options.scenario});
    });
    watchdog.start(run.options.timeout_ms + kWatchdogGraceMs);

    QTimer::singleShot(0, app, [&]() {
      rtn = RenderScenario(run);
      QCoreApplication::exit(rtn);
    });
    QApplication::exec();
    app->removeEventFilter(&guard);
  }

  // Anything but a deep restart or the crash default, which would make
  // shutdown relaunch this very command line.
  ctx->rtn = rtn;
  ShutdownGlobalBasicEnv(ctx);
  KillAgents(run);
  return rtn;
}

}  // namespace GpgFrontend::Screenshot
