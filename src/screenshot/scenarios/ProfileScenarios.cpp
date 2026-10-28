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

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QLineEdit>
#include <QWizard>
#include <array>

#include "core/profile/Profile.h"
#include "core/profile/ProfileMarker.h"
#include "core/profile/ProfileRegistry.h"
#include "screenshot/ScreenshotEnv.h"
#include "screenshot/ScreenshotScenario.h"
#include "screenshot/scenarios/ScenarioHelpers.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::Screenshot {

namespace {

using UI::MainWindow;

/// The demo passphrase typed into Export Profile; only its dots show.
const auto kDemoProfilePassphrase = u"Correct-Horse-Battery-42"_s;

struct DemoProfile {
  const char* id;
  const char* name;
  const char* last_opened;
};

/// Saved profiles beside the temporary one this window runs in, opened
/// before the demo's clock.
constexpr std::array kDemoProfiles = {
    DemoProfile{"a1b2c3", "Work", "2026-01-14T16:20:00Z"},
    DemoProfile{"d4e5f6", "Travel", "2025-12-02T09:05:00Z"},
};

/**
 * @brief Write the demo profiles into this installation's profiles folder,
 *        as New Profile would have made them.
 */
auto SeedDemoProfiles(ScenarioContext& ctx) -> bool {
  const auto root = qApp->property("GFProfilesRoot").toString();
  if (root.isEmpty()) {
    ctx.Fail(kEXIT_USAGE, u"profiles-root-missing"_s);
    return false;
  }
  for (const auto& demo : kDemoProfiles) {
    const auto dir = QDir(root).filePath(QString::fromLatin1(demo.id));
    auto marker = NewProfileMarker(QString::fromLatin1(demo.id),
                                   QString::fromLatin1(demo.name),
                                   ProfileKind::kPERSIST, true);
    marker.last_opened = QString::fromLatin1(demo.last_opened);
    if (!QDir().mkpath(dir) ||
        !WriteProfileMarker(ProfileMarkerPathFor(dir), marker)) {
      ctx.Fail(kEXIT_USAGE, u"seed-failed"_s, dir);
      return false;
    }
  }
  return true;
}

/// The main window behind the window @p action_name opens, of @p class_name.
auto OpenFromAction(ScenarioContext& ctx, const QString& action_name,
                    const char* class_name) -> QWidget* {
  auto* window =
      OpenMainWindow(ctx, kBackdropSize, u"contract.txt"_s, ContractText());
  if (window == nullptr) return nullptr;
  auto* action = window->findChild<QAction*>(action_name);
  if (action == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"profile-action-missing"_s, action_name);
    return nullptr;
  }
  return WaitForTopLevel(
      ctx, [&]() { action->trigger(); }, class_name, u"profile-dialog-shown"_s);
}

/**
 * @brief Profiles On This Computer, with two saved profiles listed.
 */
auto ProfileManagerScenario(ScenarioContext& ctx) -> bool {
  if (!SeedDemoProfiles(ctx)) return false;
  auto* dialog = OpenFromAction(ctx, u"profile_manager"_s,
                                "GpgFrontend::UI::ProfileManagerDialog");
  if (dialog == nullptr) return false;
  if (ctx.GetSize().isValid()) dialog->resize(ctx.GetSize());
  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  return SettleActive(ctx, dialog, u"profile-manager-settled"_s);
}

/// New Profile, named, @p steps pages after its first.
auto NewProfile(ScenarioContext& ctx, int steps, const QString& label) -> bool {
  auto* wizard = qobject_cast<QWizard*>(OpenFromAction(
      ctx, u"new_profile"_s, "GpgFrontend::UI::ProfileCreateDialog"));
  auto* name = wizard != nullptr ? wizard->findChild<QLineEdit*>() : nullptr;
  if (name == nullptr) {
    if (wizard != nullptr) ctx.Fail(kEXIT_USAGE, u"profile-name-missing"_s);
    return false;
  }
  name->setText(u"Work"_s);
  if (!AdvanceWizard(ctx, wizard, steps)) return false;
  if (ctx.GetSize().isValid()) wizard->resize(ctx.GetSize());
  auto* forward = wizard->button(QWizard::NextButton);
  if (forward == nullptr || !forward->isVisible()) {
    forward = wizard->button(QWizard::FinishButton);
  }
  if (!FocusWhenActive(ctx, wizard, forward)) return false;
  ctx.SetTarget(wizard, CaptureMode::kDIALOG);
  ctx.AddHighlight(u"forwardButton"_s, forward);
  return SettleActive(ctx, wizard, label);
}

auto NewProfileKeysScenario(ScenarioContext& ctx) -> bool {
  return NewProfile(ctx, 1, u"new-profile-keys-settled"_s);
}

auto NewProfileSettingsScenario(ScenarioContext& ctx) -> bool {
  return NewProfile(ctx, 2, u"new-profile-settings-settled"_s);
}

/**
 * @brief Export Profile on its Protection step, a passphrase typed in.
 */
auto ExportProfileProtectionScenario(ScenarioContext& ctx) -> bool {
  auto* wizard = qobject_cast<QWizard*>(OpenFromAction(
      ctx, u"export_profile"_s, "GpgFrontend::UI::ProfileExportDialog"));
  if (wizard == nullptr || !AdvanceWizard(ctx, wizard, 1)) return false;
  for (auto* edit : wizard->currentPage()->findChildren<QLineEdit*>()) {
    if (edit->echoMode() == QLineEdit::Password) {
      edit->setText(kDemoProfilePassphrase);
    }
  }
  if (ctx.GetSize().isValid()) wizard->resize(ctx.GetSize());
  auto* forward = wizard->button(QWizard::NextButton);
  if (!FocusWhenActive(ctx, wizard, forward)) return false;
  ctx.SetTarget(wizard, CaptureMode::kDIALOG);
  ctx.AddHighlight(u"forwardButton"_s, forward);
  return SettleActive(ctx, wizard, u"export-profile-settled"_s);
}

}  // namespace

void RegisterProfileScenarios(QList<Scenario>& scenarios) {
  const QString main_window = u"GpgFrontend::UI::MainWindow"_s;

  scenarios.append({u"profile-manager"_s,
                    QSize(),
                    CaptureMode::kDIALOG,
                    {main_window, u"GpgFrontend::UI::ProfileManagerDialog"_s},
                    false,
                    true,
                    ProfileManagerScenario});
  const QStringList create{main_window,
                           u"GpgFrontend::UI::ProfileCreateDialog"_s};
  scenarios.append({u"profile-new-keys"_s, QSize(), CaptureMode::kDIALOG,
                    create, false, true, NewProfileKeysScenario});
  scenarios.append({u"profile-new-settings"_s, QSize(), CaptureMode::kDIALOG,
                    create, false, true, NewProfileSettingsScenario});
  scenarios.append({u"profile-export-protection"_s,
                    QSize(),
                    CaptureMode::kDIALOG,
                    {main_window, u"GpgFrontend::UI::ProfileExportDialog"_s},
                    false,
                    true,
                    ExportProfileProtectionScenario});
}

}  // namespace GpgFrontend::Screenshot
