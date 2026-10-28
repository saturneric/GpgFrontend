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

#include "screenshot/ScreenshotScenario.h"

#include <QAbstractAnimation>
#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QTimer>
#include <QWindow>
#include <algorithm>

namespace GpgFrontend::Screenshot {

namespace {

constexpr int kPollIntervalMs = 20;
constexpr int kMaxStabilityGrabs = 20;

auto IsLaidOut(QWidget* w) -> bool {
  return !w->testAttribute(Qt::WA_PendingResizeEvent) &&
         !w->testAttribute(Qt::WA_PendingMoveEvent);
}

/// Whether an animation inside @p w is still running: a line edit's clear
/// button fading in, a switch sliding. Two grabs in a row can catch the
/// same in-between frame, so a settled frame waits for them to finish.
auto IsAnimating(QWidget* w) -> bool {
  const auto animations = w->findChildren<QAbstractAnimation*>();
  return std::any_of(animations.cbegin(), animations.cend(),
                     [](const QAbstractAnimation* a) {
                       return a->state() == QAbstractAnimation::Running;
                     });
}

auto IsExposed(QWidget* w) -> bool {
  auto* handle = w->window()->windowHandle();
  return handle != nullptr && handle->isExposed();
}

}  // namespace

ScenarioContext::ScenarioContext(Options options, QSize size, int timeout_ms)
    : options_(std::move(options)), size_(size), deadline_(timeout_ms) {}

auto ScenarioContext::WaitUntil(const std::function<void()>& trigger,
                                const std::function<bool()>& predicate,
                                const QList<Hook>& hooks, const QString& label,
                                const std::function<QString()>& describe)
    -> bool {
  if (failure_) return false;

  QEventLoop loop;
  bool done = false;
  const auto check = [&]() {
    if (done || !predicate()) return;
    done = true;
    loop.quit();
  };

  QList<QMetaObject::Connection> connections;
  for (const auto& hook : hooks) connections.append(hook(check));

  check();
  if (!done && trigger) {
    trigger();
    check();
  }

  // A quit() issued while trigger() spun its own loop is lost on a loop that
  // was not running yet, so a satisfied wait must never reach exec().
  if (!done && !deadline_.hasExpired()) {
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, &loop, check);
    poll.start(kPollIntervalMs);

    QTimer limit;
    limit.setSingleShot(true);
    QObject::connect(&limit, &QTimer::timeout, &loop, &QEventLoop::quit);
    limit.start(static_cast<int>(deadline_.remainingTime()));

    loop.exec();
  }

  for (const auto& c : connections) QObject::disconnect(c);

  if (!done) {
    Fail(kEXIT_TIMEOUT, u"timeout"_s,
         describe ? label + u" ("_s + describe() + QLatin1Char(')') : label);
  }
  return done;
}

auto ScenarioContext::WaitSettled(QWidget* target,
                                  const std::function<bool()>& predicate,
                                  const QString& label) -> bool {
  const auto settled = [&]() {
    return target->isVisible() && IsExposed(target) && IsLaidOut(target) &&
           !IsAnimating(target) &&
           std::all_of(popups_.cbegin(), popups_.cend(),
                       [](const QPointer<QWidget>& p) {
                         return !p.isNull() && p->isVisible() && IsExposed(p) &&
                                !IsAnimating(p);
                       }) &&
           !WaitingDialogVisible() && (!predicate || predicate());
  };
  if (!WaitUntil({}, settled, {}, label)) return false;

  // Layout, deferred updates and icon loading can still change pixels after
  // every flag above reads ready. The frame is done when it stops changing.
  const auto grab_all = [&]() {
    QList<QImage> images{target->grab().toImage()};
    for (const auto& p : popups_) images.append(p->grab().toImage());
    return images;
  };
  const auto same = [](const QList<QImage>& a, const QList<QImage>& b) {
    if (a.size() != b.size()) return false;
    for (qsizetype i = 0; i < a.size(); ++i) {
      if (!SameImage(a.at(i), b.at(i))) return false;
    }
    return true;
  };

  auto previous = grab_all();
  for (int i = 0; i < kMaxStabilityGrabs; ++i) {
    QApplication::processEvents();
    auto current = grab_all();
    if (same(current, previous)) return true;
    previous = std::move(current);
  }

  Fail(kEXIT_TIMEOUT, u"frame-unstable"_s, label);
  return false;
}

void ScenarioContext::SetTarget(QWidget* target, CaptureMode mode) {
  target_ = target;
  mode_ = mode;
}

void ScenarioContext::AddPopup(QWidget* popup) { popups_.append(popup); }

void ScenarioContext::AddHighlight(const QString& name, QWidget* widget,
                                   std::optional<QRect> local_rect) {
  highlights_.append({name, widget, local_rect});
}

void ScenarioContext::Keep(const QSharedPointer<QObject>& object) {
  kept_.append(object);
}

void ScenarioContext::Fail(int exit_code, const QString& reason,
                           const QString& detail) {
  if (failure_) return;
  failure_ = Failure{exit_code, reason, detail};
}

auto ScenarioContext::GetOptions() const -> const Options& { return options_; }

auto ScenarioContext::GetSize() const -> QSize { return size_; }

auto ScenarioContext::FixturePath(const QString& name) const -> QString {
  return QDir(options_.fixtures).filePath(name);
}

auto ScenarioContext::Target() const -> QWidget* { return target_; }

auto ScenarioContext::Popups() const -> QList<QWidget*> {
  QList<QWidget*> popups;
  for (const auto& p : popups_) popups.append(p.data());
  return popups;
}

auto ScenarioContext::Mode() const -> CaptureMode { return mode_; }

auto ScenarioContext::Highlights() const -> const QList<Highlight>& {
  return highlights_;
}

auto ScenarioContext::GetFailure() const -> const std::optional<Failure>& {
  return failure_;
}

auto Scenarios() -> const QList<Scenario>& {
  static const QList<Scenario> kScenarios = [] {
    QList<Scenario> scenarios;
    RegisterMainWindowScenarios(scenarios);
    RegisterKeyDetailsScenarios(scenarios);
    RegisterKeyManagementScenarios(scenarios);
    RegisterOperationScenarios(scenarios);
    RegisterKeyTransferScenarios(scenarios);
    RegisterKeyGenerationScenarios(scenarios);
    RegisterProfileScenarios(scenarios);
    RegisterEmailScenarios(scenarios);
    RegisterSelfTestScenarios(scenarios);
    return scenarios;
  }();
  return kScenarios;
}

auto FindScenario(const QString& name) -> const Scenario* {
  for (const auto& s : Scenarios()) {
    if (s.name == name) return &s;
  }
  return nullptr;
}

auto WaitingDialogVisible() -> bool {
  for (auto* w : QApplication::topLevelWidgets()) {
    if (w->isVisible() && w->inherits("GpgFrontend::UI::WaitingDialog")) {
      return true;
    }
  }
  return false;
}

}  // namespace GpgFrontend::Screenshot
