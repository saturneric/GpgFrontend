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
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>

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
/// Name, Engine, Size, Folder: the Recycle Bin's columns.
constexpr int kRecycleBinSizeColumn = 2;

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

/**
 * @brief KeyPair Management with @p email selected and its context menu
 *        open, as a right-click on the row opens it.
 */
auto OpenKeyPopup(ScenarioContext& ctx, const char* email)
    -> std::pair<QWidget*, QMenu*> {
  auto* mgmt = OpenKeyMgmt(ctx, ctx.GetSize());
  auto* table = mgmt != nullptr ? ShowCategory(ctx, mgmt, u"all"_s) : nullptr;
  auto* menu =
      mgmt != nullptr ? mgmt->findChild<QMenu*>(u"keyPopupMenu"_s) : nullptr;
  if (table == nullptr || !SelectKeyRow(ctx, table, email)) return {};
  if (menu == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"key-popup-menu-missing"_s);
    return {};
  }
  const auto row = RowOf(table, QString::fromLatin1(email));
  const auto cell = table->visualRect(table->model()->index(row, 2));
  // popup() rather than the window's exec(): that runs a nested loop.
  if (!ctx.WaitUntil(
          [&]() { menu->popup(table->viewport()->mapToGlobal(cell.center())); },
          [&]() { return menu->isVisible(); },
          {ScenarioContext::On(menu, &QMenu::aboutToShow)},
          u"key-popup-menu-shown"_s)) {
    return {};
  }
  return {mgmt, menu};
}

/**
 * @brief Alice's context menu, its Key Operations open at Certify Key.
 */
auto KeyPopupCertifyScenario(ScenarioContext& ctx) -> bool {
  auto [mgmt, menu] = OpenKeyPopup(ctx, kAliceEmail);
  if (menu == nullptr) return false;
  auto* operations = mgmt->findChild<QMenu*>(u"keyOperationsMenu"_s);
  auto* certify = mgmt->findChild<QAction*>(u"certify_key"_s);
  auto* submenu = operations != nullptr
                      ? OpenSubmenu(ctx, menu, operations->menuAction())
                      : nullptr;
  if (submenu == nullptr || certify == nullptr) {
    if (!ctx.GetFailure()) ctx.Fail(kEXIT_USAGE, u"certify-key-missing"_s);
    return false;
  }
  submenu->setActiveAction(certify);

  ctx.SetTarget(mgmt, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(menu);
  ctx.AddPopup(submenu);
  ctx.AddHighlight(u"keyOperations"_s, menu,
                   menu->actionGeometry(operations->menuAction()));
  ctx.AddHighlight(u"certifyKey"_s, submenu, submenu->actionGeometry(certify));
  return ctx.WaitSettled(
      mgmt, [&]() { return submenu->activeAction() == certify; },
      u"key-popup-certify-settled"_s);
}

/**
 * @brief Alice's context menu, its Category submenu open.
 */
auto KeyPopupCategoryScenario(ScenarioContext& ctx) -> bool {
  auto [mgmt, menu] = OpenKeyPopup(ctx, kAliceEmail);
  if (menu == nullptr) return false;
  auto* category = mgmt->findChild<QMenu*>(u"categoryMenu"_s);
  auto* submenu = category != nullptr
                      ? OpenSubmenu(ctx, menu, category->menuAction())
                      : nullptr;
  if (submenu == nullptr) {
    if (!ctx.GetFailure()) ctx.Fail(kEXIT_USAGE, u"category-menu-missing"_s);
    return false;
  }

  ctx.SetTarget(mgmt, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(menu);
  ctx.AddPopup(submenu);
  ctx.AddHighlight(u"categoryMenu"_s, submenu);
  return ctx.WaitSettled(
      mgmt, [&]() { return submenu->isVisible(); },
      u"key-popup-category-settled"_s);
}

/**
 * @brief Operations > Bulk in KeyPair Management's menu bar.
 */
auto KeyMgmtBulkMenuScenario(ScenarioContext& ctx) -> bool {
  auto* mgmt = OpenKeyMgmt(ctx, ctx.GetSize());
  auto* table = mgmt != nullptr ? ShowCategory(ctx, mgmt, u"all"_s) : nullptr;
  if (table == nullptr || !CheckKeyRow(ctx, table, kAliceEmail) ||
      !CheckKeyRow(ctx, table, kBobEmail)) {
    return false;
  }
  auto* bar = mgmt->findChild<QMenuBar*>();
  auto* bulk = mgmt->findChild<QMenu*>(u"bulkMenu"_s);
  auto [operations, operations_entry] =
      bar != nullptr && bulk != nullptr
          ? MenuHolding(bar->actions(), bulk->menuAction())
          : std::pair<QMenu*, QAction*>{nullptr, nullptr};
  if (operations == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"bulk-menu-missing"_s);
    return false;
  }
  // Through the menu bar, so "Operations" is drawn as the open item.
  if (!ctx.WaitUntil([&]() { bar->setActiveAction(operations_entry); },
                     [&]() { return operations->isVisible(); },
                     {ScenarioContext::On(operations, &QMenu::aboutToShow)},
                     u"operations-menu-shown"_s)) {
    return false;
  }
  if (OpenSubmenu(ctx, operations, bulk->menuAction()) == nullptr) {
    return false;
  }

  ctx.SetTarget(mgmt, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(operations);
  ctx.AddPopup(bulk);
  ctx.AddHighlight(u"bulkMenu"_s, bulk);
  return ctx.WaitSettled(
      mgmt,
      [&]() {
        return bar->activeAction() == operations_entry && bulk->isVisible();
      },
      u"bulk-menu-settled"_s);
}

/**
 * @brief Certify Key for Bob, from KeyPair Management, Alice ticked to
 *        certify with.
 */
auto CertifyDialogScenario(ScenarioContext& ctx) -> bool {
  auto* mgmt = OpenKeyMgmt(ctx, kBackdropSize);
  auto* table = mgmt != nullptr ? ShowCategory(ctx, mgmt, u"all"_s) : nullptr;
  auto* certify =
      mgmt != nullptr ? mgmt->findChild<QAction*>(u"certify_key"_s) : nullptr;
  if (table == nullptr || !SelectKeyRow(ctx, table, kBobEmail)) return false;
  if (certify == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"certify-key-missing"_s);
    return false;
  }
  auto* dialog = WaitForTopLevel(
      ctx, [&]() { certify->trigger(); }, "GpgFrontend::UI::KeyUIDSignDialog",
      u"certify-dialog-shown"_s);
  auto* signers = dialog != nullptr
                      ? dialog->findChild<QTableView*>(u"signers"_s)
                      : nullptr;
  if (signers == nullptr) {
    if (dialog != nullptr) ctx.Fail(kEXIT_USAGE, u"certify-signers-missing"_s);
    return false;
  }
  if (!ctx.WaitUntil(
          {},
          [&]() {
            return RowOf(signers, QString::fromLatin1(kAliceEmail)) >= 0;
          },
          ModelHooks(signers->model()), u"certify-signers-listed"_s) ||
      !CheckKeyRow(ctx, signers, kAliceEmail)) {
    return false;
  }
  if (ctx.GetSize().isValid()) dialog->resize(ctx.GetSize());
  // The search field takes the focus first.
  if (!FocusWhenActive(ctx, dialog, signers)) return false;

  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  ctx.AddHighlight(u"signers"_s, signers);
  return SettleActive(ctx, dialog, u"certify-dialog-settled"_s);
}

/**
 * @brief New Category, from KeyPair Management, named.
 */
auto KeyCategoryNewScenario(ScenarioContext& ctx) -> bool {
  auto* mgmt = OpenKeyMgmt(ctx, kBackdropSize);
  QAbstractButton* add = nullptr;
  for (auto* button : mgmt != nullptr ? mgmt->findChildren<QAbstractButton*>(
                                            u"addCategoryButton"_s)
                                      : QList<QAbstractButton*>{}) {
    if (button->isVisible()) add = button;
  }
  if (add == nullptr) {
    if (mgmt != nullptr) ctx.Fail(kEXIT_USAGE, u"add-category-missing"_s);
    return false;
  }
  auto* dialog = WaitForTopLevel(
      ctx, [&]() { add->click(); }, "GpgFrontend::UI::KeyCategoryEditDialog",
      u"category-dialog-shown"_s);
  auto* name = dialog != nullptr ? dialog->findChild<QLineEdit*>() : nullptr;
  if (name == nullptr) {
    if (dialog != nullptr) ctx.Fail(kEXIT_USAGE, u"category-name-missing"_s);
    return false;
  }
  name->setText(u"Friends"_s);
  if (!FocusWhenActive(ctx, dialog, dialog->findChild<QPushButton*>())) {
    return false;
  }

  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  ctx.AddHighlight(u"preview"_s,
                   dialog->findChild<QWidget*>(u"KeyCategoryPreview"_s));
  return SettleActive(ctx, dialog, u"category-dialog-settled"_s);
}

/**
 * @brief Settings > Key Databases on its Recycle Bin tab, one removed
 *        database listed.
 */
auto RecycleBinScenario(ScenarioContext& ctx) -> bool {
  auto* window =
      OpenMainWindow(ctx, kBackdropSize, u"contract.txt"_s, ContractText());
  if (window == nullptr) return false;
  auto [dialog, page] =
      OpenSettingsPage(ctx, window, "GpgFrontend::UI::KeyDatabasesTab");
  if (page == nullptr) return false;
  auto* tabs = page->findChild<QTabWidget*>(u"keyDatabaseTabs"_s);
  auto* bin = page->findChild<QWidget*>(u"recycleBinTab"_s);
  auto* table = page->findChild<QTableWidget*>(u"recycleBinTable"_s);
  if (tabs == nullptr || bin == nullptr || table == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"recycle-bin-missing"_s);
    return false;
  }
  tabs->setCurrentWidget(bin);
  // Size is measured in the background; until then the cell has no unit.
  const auto measured = [&]() {
    const auto* item =
        table->rowCount() > 0 ? table->item(0, kRecycleBinSizeColumn) : nullptr;
    return item != nullptr && item->text().endsWith(u'B');
  };
  if (!ctx.WaitUntil({}, measured, {}, u"recycle-bin-listed"_s,
                     [&]() { return u"rows=%1"_s.arg(table->rowCount()); })) {
    return false;
  }
  table->setFocus();
  table->selectRow(0);

  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  ctx.AddHighlight(u"recycleBinTable"_s, table);
  ctx.AddHighlight(u"restoreButton"_s,
                   page->findChild<QWidget*>(u"recycleBinRestoreButton"_s));
  return SettleActive(ctx, dialog, u"recycle-bin-settled"_s);
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
  const QSize mgmt_size(1200, 760);
  scenarios.append({u"key-popup-certify"_s, mgmt_size, CaptureMode::kCOMPOSITE,
                    mgmt, false, true, KeyPopupCertifyScenario});
  scenarios.append({u"key-popup-category"_s, mgmt_size, CaptureMode::kCOMPOSITE,
                    mgmt, false, true, KeyPopupCategoryScenario});
  scenarios.append({u"key-management-bulk-menu"_s, mgmt_size,
                    CaptureMode::kCOMPOSITE, mgmt, false, true,
                    KeyMgmtBulkMenuScenario});
  scenarios.append(
      {u"certify-dialog"_s,
       QSize(),
       CaptureMode::kDIALOG,
       {u"GpgFrontend::UI::MainWindow"_s, QString::fromLatin1(kKeyMgmtClass),
        u"GpgFrontend::UI::KeyUIDSignDialog"_s},
       false,
       true,
       CertifyDialogScenario});
  scenarios.append(
      {u"key-category-new"_s,
       QSize(),
       CaptureMode::kDIALOG,
       {u"GpgFrontend::UI::MainWindow"_s, QString::fromLatin1(kKeyMgmtClass),
        u"GpgFrontend::UI::KeyCategoryEditDialog"_s},
       false,
       true,
       KeyCategoryNewScenario});
  Scenario recycle_bin{
      u"settings-key-databases-recycle-bin"_s,
      QSize(900, 660),
      CaptureMode::kDIALOG,
      {u"GpgFrontend::UI::MainWindow"_s, u"GpgFrontend::UI::SettingsDialog"_s},
      false,
      true,
      RecycleBinScenario};
  recycle_bin.recycled_databases = {u"work"_s};
  scenarios.append(recycle_bin);
}

}  // namespace GpgFrontend::Screenshot
