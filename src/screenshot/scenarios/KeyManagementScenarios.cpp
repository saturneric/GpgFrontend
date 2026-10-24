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

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTableView>

#include "screenshot/ScreenshotEnv.h"
#include "screenshot/ScreenshotScenario.h"
#include "screenshot/ScreenshotSeed.h"
#include "screenshot/scenarios/ScenarioHelpers.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::Screenshot {

namespace {

using UI::MainWindow;

/// The check box column of every key table.
constexpr int kCheckColumn = 0;

/**
 * @brief KeyPair Management on its All tab with Alice and Bob ticked.
 */
auto KeyMgmtCheckedScenario(ScenarioContext& ctx) -> bool {
  auto* mgmt = OpenKeyMgmt(ctx, ctx.GetSize());
  auto* table = mgmt != nullptr ? ShowCategory(ctx, mgmt, u"all"_s) : nullptr;
  if (table == nullptr || !CheckKeyRow(ctx, table, kAliceEmail) ||
      !CheckKeyRow(ctx, table, kBobEmail)) {
    return false;
  }

  ctx.SetTarget(mgmt, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"checkColumn"_s, table, ColumnRect(table, kCheckColumn));
  ctx.AddHighlight(u"exportKeyButton"_s,
                   mgmt->findChild<QWidget*>(u"exportKeyButton"_s));
  return ctx.WaitSettled(
      mgmt, [&]() { return mgmt->isActiveWindow(); },
      u"key-management-checked-settled"_s);
}

/**
 * @brief The main window with Alice and Bob ticked, ready for a new group.
 */
auto OpenWithCheckedKeys(ScenarioContext& ctx, QSize size) -> MainWindow* {
  auto* window = OpenMainWindow(ctx, size, u"contract.txt"_s, ContractText());
  if (window == nullptr) return nullptr;
  auto* table = KeyTable(window);
  if (!CheckKeyRow(ctx, table, kAliceEmail) ||
      !CheckKeyRow(ctx, table, kBobEmail)) {
    return nullptr;
  }
  return window;
}

auto CheckedKeysScenario(ScenarioContext& ctx) -> bool {
  auto* window = OpenWithCheckedKeys(ctx, ctx.GetSize());
  if (window == nullptr) return false;

  auto* table = KeyTable(window);
  ctx.SetTarget(window, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"checkColumn"_s, table, ColumnRect(table, kCheckColumn));
  ctx.AddHighlight(u"keyGroupButton"_s,
                   window->findChild<QWidget*>(u"keyGroupButton"_s));
  return ctx.WaitSettled(
      window, [&]() { return window->isActiveWindow(); },
      u"checked-keys-settled"_s);
}

/**
 * @brief New Key Group, opened for the ticked keys and filled in.
 */
auto KeyGroupCreateScenario(ScenarioContext& ctx) -> bool {
  auto* window = OpenWithCheckedKeys(ctx, kBackdropSize);
  if (window == nullptr) return false;

  auto* button = window->findChild<QAbstractButton*>(u"keyGroupButton"_s);
  if (button == nullptr || !button->isEnabled()) {
    ctx.Fail(kEXIT_USAGE, u"key-group-button-unavailable"_s);
    return false;
  }
  auto* dialog = WaitForTopLevel(
      ctx, [&]() { button->click(); },
      "GpgFrontend::UI::KeyGroupCreationDialog", u"key-group-creation-shown"_s);
  if (dialog == nullptr) return false;

  // Name, email and comment, in the order the dialog creates them.
  const auto edits = dialog->findChildren<QLineEdit*>();
  if (edits.size() < 3) {
    ctx.Fail(kEXIT_USAGE, u"key-group-fields-missing"_s);
    return false;
  }
  edits.at(0)->setText(u"Partners"_s);
  edits.at(1)->setText(u"partners@example.com"_s);
  edits.at(2)->setText(u"Alice and Bob"_s);
  // Focus off the fields once the dialog is active, so the field gets its
  // focus-out: with blinking off, a focused one keeps drawing its cursor.
  auto* create = dialog->findChild<QPushButton*>();
  if (!ctx.WaitUntil(
          {},
          [&]() {
            if (!dialog->isActiveWindow()) dialog->activateWindow();
            return dialog->isActiveWindow();
          },
          {}, u"key-group-creation-active"_s)) {
    return false;
  }
  if (create != nullptr) create->setFocus();

  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  return ctx.WaitSettled(
      dialog,
      [&]() {
        if (!dialog->isActiveWindow()) dialog->activateWindow();
        return dialog->isActiveWindow();
      },
      u"key-group-creation-settled"_s);
}

/**
 * @brief Key Group Management for the demo group, from the Key Groups tab.
 */
auto KeyGroupManageScenario(ScenarioContext& ctx) -> bool {
  auto* window =
      OpenMainWindow(ctx, kBackdropSize, u"contract.txt"_s, ContractText());
  if (window == nullptr) return false;

  auto* categories = window->findChild<QListWidget*>(u"KeyCategoryList"_s);
  auto* groups = window->findChild<QTableView*>(u"key_group"_s);
  auto* details = window->findChild<QAction*>(u"show_key_details"_s);
  int row = -1;
  for (int i = 0; categories != nullptr && i < categories->count(); ++i) {
    if (categories->item(i)->data(Qt::UserRole).toString() == "key_group"_L1) {
      row = i;
    }
  }
  if (row < 0 || groups == nullptr || details == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"key-group-tab-missing"_s);
    return false;
  }
  if (!ctx.WaitUntil([&]() { categories->setCurrentRow(row); },
                     [&]() {
                       return groups->isVisible() &&
                              RowOf(groups,
                                    QLatin1StringView(kDemoGroupName)) >= 0;
                     },
                     ModelHooks(groups->model()), u"key-group-listed"_s)) {
    return false;
  }
  if (!SelectKeyRow(ctx, groups, kDemoGroupName)) return false;

  auto* dialog = WaitForTopLevel(
      ctx, [&]() { details->trigger(); },
      "GpgFrontend::UI::KeyGroupManageDialog", u"key-group-manage-shown"_s);
  if (dialog == nullptr) return false;
  if (ctx.GetSize().isValid()) dialog->resize(ctx.GetSize());

  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  return ctx.WaitSettled(
      dialog,
      [&]() {
        if (!dialog->isActiveWindow()) dialog->activateWindow();
        return dialog->isActiveWindow();
      },
      u"key-group-manage-settled"_s);
}

}  // namespace

void RegisterKeyManagementScenarios(QList<Scenario>& scenarios) {
  const QStringList mgmt{u"GpgFrontend::UI::MainWindow"_s,
                         QString::fromLatin1(kKeyMgmtClass)};
  const QStringList main_window{u"GpgFrontend::UI::MainWindow"_s};

  scenarios.append({u"key-management-checked"_s, QSize(1200, 760),
                    CaptureMode::kWINDOW, mgmt, false, true,
                    KeyMgmtCheckedScenario});
  scenarios.append({u"main-window-checked-keys"_s, QSize(1280, 960),
                    CaptureMode::kWINDOW, main_window, false, true,
                    CheckedKeysScenario});
  scenarios.append({u"key-group-create-dialog"_s,
                    QSize(),
                    CaptureMode::kDIALOG,
                    {u"GpgFrontend::UI::MainWindow"_s,
                     u"GpgFrontend::UI::KeyGroupCreationDialog"_s},
                    false,
                    true,
                    KeyGroupCreateScenario});
  scenarios.append({u"key-group-manage-dialog"_s,
                    QSize(),
                    CaptureMode::kDIALOG,
                    {u"GpgFrontend::UI::MainWindow"_s,
                     u"GpgFrontend::UI::KeyGroupManageDialog"_s},
                    false,
                    true,
                    KeyGroupManageScenario});
}

}  // namespace GpgFrontend::Screenshot
