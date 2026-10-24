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

#include <QLabel>
#include <QMessageBox>

#include "screenshot/ScreenshotScenario.h"

/**
 * @file SelfTestScenarios.cpp
 * @brief Scenarios that exist to fail, one per failure the pipeline must
 *        tell apart. Not listed by --screenshot-list.
 */

namespace GpgFrontend::Screenshot {

namespace {

auto ShowPlainWindow(ScenarioContext& ctx) -> QWidget* {
  auto label = QSharedPointer<QLabel>::create(u"self-test"_s);
  ctx.Keep(label);
  label->setObjectName(u"SelfTestWindow"_s);
  label->setAlignment(Qt::AlignCenter);
  label->resize(ctx.GetSize());
  label->show();
  return label.data();
}

auto SettledPlainWindow(ScenarioContext& ctx) -> bool {
  auto* window = ShowPlainWindow(ctx);
  ctx.SetTarget(window, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"SelfTestWindow"_s, window);
  return ctx.WaitSettled(window, {}, u"self-test-window"_s);
}

}  // namespace

void RegisterSelfTestScenarios(QList<Scenario>& scenarios) {
  const QStringList plain{u"QLabel"_s};

  // Waits on something that never happens: exit 3, naming the wait.
  scenarios.append({u"selftest-timeout"_s, QSize(320, 200),
                    CaptureMode::kWINDOW, plain, false, false,
                    [](ScenarioContext& ctx) {
                      ShowPlainWindow(ctx);
                      return ctx.WaitUntil(
                          {}, [] { return false; }, {}, u"selftest-never"_s);
                    }});

  // A message box nobody asked for: exit 5, naming it.
  scenarios.append({u"selftest-modal"_s, QSize(320, 200), CaptureMode::kWINDOW,
                    plain, false, false, [](ScenarioContext& ctx) {
                      auto box = QSharedPointer<QMessageBox>::create(
                          QMessageBox::Warning, u"Unexpected"_s,
                          u"self-test"_s);
                      ctx.Keep(box);
                      box->show();
                      return SettledPlainWindow(ctx);
                    }});

  // Fails the environment gates on purpose: exit 6 "selftest-injected".
  scenarios.append({u"selftest-gate"_s, QSize(320, 200), CaptureMode::kWINDOW,
                    plain, false, false, SettledPlainWindow});

  // Depends on the agent clock: exit 6 when gpg-agent.conf was refused.
  scenarios.append({u"selftest-agent-clock"_s, QSize(320, 200),
                    CaptureMode::kWINDOW, plain, true, false,
                    SettledPlainWindow});
}

}  // namespace GpgFrontend::Screenshot
