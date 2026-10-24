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

#pragma once

#include <QDeadlineTimer>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <functional>
#include <optional>

#include "screenshot/ScreenshotCapture.h"
#include "screenshot/ScreenshotOptions.h"

/**
 * @file ScreenshotScenario.h
 * @brief A scenario puts the application into one state worth showing.
 *
 * Scenarios reach the UI through exported classes and plain Qt only (object
 * names, QObject::inherits, item models). The UI library hides every symbol
 * it does not export, so anything else would link on no platform.
 */

namespace GpgFrontend::Screenshot {

class ScenarioContext {
 public:
  /// Connects a callback to one signal and returns the connection.
  using Hook =
      std::function<QMetaObject::Connection(const std::function<void()>&)>;

  ScenarioContext(Options options, QSize size, int timeout_ms);

  /**
   * @brief A hook for @p signal of @p sender.
   */
  template <typename Sender, typename Signal>
  static auto On(Sender* sender, Signal signal) -> Hook {
    QPointer<Sender> guard(sender);
    return [guard, signal](const std::function<void()>& callback) {
      if (guard.isNull()) return QMetaObject::Connection();
      return QObject::connect(guard.data(), signal, guard.data(),
                              [callback]() { callback(); });
    };
  }

  /**
   * @brief Run @p trigger and wait until @p predicate holds.
   *
   * The hooks are connected before @p trigger runs and @p predicate is
   * checked once before as well, so neither a state reached early nor a
   * signal emitted from inside the trigger is missed. It is re-checked on
   * every hooked signal and on a short timer for state that announces
   * itself with no signal at all. Bounded by the run's deadline.
   *
   * @return false on timeout, after recording @p label, and what
   *         @p describe says of the state then, as the failure
   */
  auto WaitUntil(const std::function<void()>& trigger,
                 const std::function<bool()>& predicate,
                 const QList<Hook>& hooks, const QString& label,
                 const std::function<QString()>& describe = {}) -> bool;

  /**
   * @brief Wait until @p target is shown, exposed, laid out, no waiting
   *        dialog is up and @p predicate holds; then until two consecutive
   *        grabs are identical.
   */
  auto WaitSettled(QWidget* target, const std::function<bool()>& predicate,
                   const QString& label) -> bool;

  void SetTarget(QWidget* target, CaptureMode mode);
  /// A popup drawn over the target, in the order they stack.
  void AddPopup(QWidget* popup);
  void AddHighlight(const QString& name, QWidget* widget,
                    std::optional<QRect> local_rect = std::nullopt);

  /// Keeps an object the scenario created alive until the capture is done.
  void Keep(const QSharedPointer<QObject>& object);

  /**
   * @brief Record why the scenario cannot continue; the first one wins.
   */
  void Fail(int exit_code, const QString& reason, const QString& detail = {});

  [[nodiscard]] auto GetOptions() const -> const Options&;
  [[nodiscard]] auto GetSize() const -> QSize;
  [[nodiscard]] auto FixturePath(const QString& name) const -> QString;
  [[nodiscard]] auto Target() const -> QWidget*;
  [[nodiscard]] auto Popups() const -> QList<QWidget*>;
  [[nodiscard]] auto Mode() const -> CaptureMode;
  [[nodiscard]] auto Highlights() const -> const QList<Highlight>&;
  [[nodiscard]] auto GetFailure() const -> const std::optional<Failure>&;

 private:
  Options options_;
  QSize size_;
  QDeadlineTimer deadline_;
  QPointer<QWidget> target_;
  QList<QPointer<QWidget>> popups_;
  CaptureMode mode_ = CaptureMode::kWINDOW;
  QList<Highlight> highlights_;
  QList<QSharedPointer<QObject>> kept_;
  std::optional<Failure> failure_;
};

struct Scenario {
  QString name;
  QSize default_size;
  CaptureMode mode = CaptureMode::kWINDOW;
  /// Class names of top-level windows the scenario expects to see.
  QStringList allowed_windows;
  /// The scenario depends on gpg-agent honouring the faked clock.
  bool needs_agent_clock = false;
  /// Shown by --screenshot-list. Self-tests are not.
  bool listed = true;
  std::function<bool(ScenarioContext&)> setup;
  /// Extra demo key databases to set up before startup ("work", "lab"); the
  /// built-in one is always there. Empty for most scenarios, which then
  /// render exactly as they would without the extra databases.
  QStringList key_databases;
};

/**
 * @brief Every registered scenario, in listing order.
 */
auto Scenarios() -> const QList<Scenario>&;

auto FindScenario(const QString& name) -> const Scenario*;

/**
 * @brief Whether any visible top-level widget is one of the application's
 *        waiting dialogs.
 */
auto WaitingDialogVisible() -> bool;

// Registration, one function per scenarios/ file.
void RegisterMainWindowScenarios(QList<Scenario>& scenarios);
void RegisterSelfTestScenarios(QList<Scenario>& scenarios);

}  // namespace GpgFrontend::Screenshot
