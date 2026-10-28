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

#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QFile>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QStackedWidget>
#include <QTableView>
#include <QTimer>
#include <QToolButton>

#include "screenshot/ScreenshotEnv.h"
#include "screenshot/ScreenshotScenario.h"
#include "screenshot/ScreenshotSeed.h"
#include "screenshot/scenarios/ScenarioHelpers.h"
#include "ui/main_window/MainWindow.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::Screenshot {

namespace {

using UI::InfoBoardWidget;
using UI::MainWindow;
using UI::TextEdit;

auto MainWindowScenario(ScenarioContext& ctx) -> bool {
  auto* window =
      OpenMainWindow(ctx, ctx.GetSize(), u"contract.txt"_s, ContractText());
  if (window == nullptr) return false;

  auto* table = KeyTable(window);
  const auto alice = RowOf(table, kAliceEmail);
  const auto bob = RowOf(table, kBobEmail);
  if (alice < 0 || bob < 0) {
    ctx.Fail(kEXIT_USAGE, u"demo-key-missing"_s);
    return false;
  }

  auto* model = table->model();
  table->setFocus();
  table->selectRow(alice);
  model->setData(model->index(bob, 0), Qt::Checked, Qt::CheckStateRole);

  ctx.SetTarget(window, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"keyListDock"_s,
                   window->findChild<QWidget*>(u"keyListDock"_s));
  ctx.AddHighlight(u"cryptToolBar"_s,
                   window->findChild<QWidget*>(u"cryptToolBar"_s));
  ctx.AddHighlight(u"PlainTextEditor"_s, CurrentEditor(window));
  ctx.AddHighlight(u"importKeyButton"_s,
                   window->findChild<QWidget*>(u"importKeyButton"_s));

  return ctx.WaitSettled(
      window,
      [&]() {
        return table->selectionModel()->isRowSelected(alice, {}) &&
               model->index(bob, 0).data(Qt::CheckStateRole).toInt() ==
                   Qt::Checked;
      },
      u"main-window-selection"_s);
}

auto KeyGenerateDialogScenario(ScenarioContext& ctx) -> bool {
  auto* window =
      OpenMainWindow(ctx, kBackdropSize, u"contract.txt"_s, ContractText());
  if (window == nullptr) return false;

  auto* action = window->findChild<QAction*>(u"generate_key_pair"_s);
  if (action == nullptr || !action->isEnabled()) {
    ctx.Fail(kEXIT_USAGE, u"generate-key-unavailable"_s);
    return false;
  }

  QWidget* dialog = nullptr;
  const auto opened =
      ctx.WaitUntil([&]() { action->trigger(); },
                    [&]() {
                      for (auto* w : QApplication::topLevelWidgets()) {
                        if (w->isVisible() &&
                            w->inherits("GpgFrontend::UI::KeyGenerateWizard")) {
                          dialog = w;
                          return true;
                        }
                      }
                      return false;
                    },
                    {}, u"key-generate-dialog-shown"_s);
  if (!opened) return false;

  if (ctx.GetSize().isValid()) {
    dialog->resize(ctx.GetSize());
  }

  auto* name = dialog->findChild<QLineEdit*>(u"nameEdit"_s);
  auto* email = dialog->findChild<QLineEdit*>(u"emailEdit"_s);
  auto* comment = dialog->findChild<QLineEdit*>(u"commentEdit"_s);
  // The wizard's own Continue button: the first step only asks who the key
  // is for, and this is what moves on from it.
  auto* wizard = qobject_cast<QWizard*>(dialog);
  auto* generate =
      wizard != nullptr ? wizard->button(QWizard::NextButton) : nullptr;
  if (name == nullptr || email == nullptr || comment == nullptr ||
      generate == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"key-generate-fields-missing"_s);
    return false;
  }

  name->setText(u"Alice Example"_s);
  email->setText(u"alice@example.com"_s);
  comment->setText(u"Work"_s);
  generate->setFocus();

  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  ctx.AddHighlight(u"nameEdit"_s, name);
  ctx.AddHighlight(u"emailEdit"_s, email);
  ctx.AddHighlight(u"commentEdit"_s, comment);
  ctx.AddHighlight(u"continueButton"_s, generate);

  return ctx.WaitSettled(
      dialog,
      [&]() {
        return dialog->isActiveWindow() &&
               (!ctx.GetSize().isValid() || dialog->size() == ctx.GetSize()) &&
               name->text() == "Alice Example"_L1;
      },
      u"key-generate-dialog-filled"_s);
}

auto DecryptVerifyResultScenario(ScenarioContext& ctx) -> bool {
  QFile fixture(ctx.FixturePath(u"decrypt-verify.asc"_s));
  if (!fixture.open(QIODevice::ReadOnly)) {
    ctx.Fail(kEXIT_USAGE, u"fixture-missing"_s, fixture.fileName());
    return false;
  }
  const auto armored = QString::fromUtf8(fixture.readAll());

  auto* window = OpenMainWindow(ctx, ctx.GetSize(), u"message.asc"_s, armored);
  if (window == nullptr) return false;

  auto* board = window->findChild<InfoBoardWidget*>();

  bool posted = false;
  const auto done = ctx.WaitUntil(
      [&]() { window->SlotDecryptVerify(); },
      [&]() {
        auto* editor = CurrentEditor(window);
        return posted && editor != nullptr &&
               editor->toPlainText().contains(
                   "the quarterly report is attached"_L1);
      },
      {[&](const std::function<void()>& check) {
        return QObject::connect(board, &InfoBoardWidget::SignalResultPosted,
                                board, [&posted, check]() {
                                  posted = true;
                                  check();
                                });
      }},
      u"decrypt-verify-result-posted"_s);
  if (!done) return false;

  // The result is what the reader looks at; no caret in the editor.
  board->setFocus();
  ctx.SetTarget(window, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"InfoBoardWidget"_s, board);
  ctx.AddHighlight(u"PlainTextEditor"_s, CurrentEditor(window));

  return ctx.WaitSettled(window, {}, u"decrypt-verify-settled"_s);
}

/**
 * @brief Keys > Import Key > Key Server, as the manual walks through it.
 *
 * "Key Server" is not the Host's: the key server module places it in the
 * Import Key menu, so it is whatever entry there the Host did not add.
 */
auto MenuPopupScenario(ScenarioContext& ctx) -> bool {
  auto* window =
      OpenMainWindow(ctx, ctx.GetSize(), u"contract.txt"_s, ContractText());
  if (window == nullptr) return false;

  auto* bar = window->menuBar();
  const auto* generate = window->findChild<QAction*>(u"generate_key_pair"_s);
  const QList<QAction*> host_imports{
      window->findChild<QAction*>(u"import_key_from_file"_s),
      window->findChild<QAction*>(u"import_key_from_edit"_s),
      window->findChild<QAction*>(u"import_key_from_clipboard"_s)};

  auto [keys_menu, keys_entry] = MenuHolding(bar->actions(), generate);
  if (keys_menu == nullptr || host_imports.contains(nullptr)) {
    ctx.Fail(kEXIT_USAGE, u"keys-menu-missing"_s);
    return false;
  }
  auto [import_menu, import_entry] =
      MenuHolding(keys_menu->actions(), host_imports.front());
  if (import_menu == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"import-key-menu-missing"_s);
    return false;
  }

  QAction* key_server = nullptr;
  for (auto* action : import_menu->actions()) {
    if (!action->isSeparator() && action->isVisible() &&
        !host_imports.contains(action)) {
      key_server = action;
      break;
    }
  }
  if (key_server == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"key-server-action-missing"_s);
    return false;
  }

  // Through the menu bar, so "Keys" is drawn as the open item.
  if (!ctx.WaitUntil([&]() { bar->setActiveAction(keys_entry); },
                     [&]() { return keys_menu->isVisible(); },
                     {ScenarioContext::On(keys_menu, &QMenu::aboutToShow)},
                     u"keys-menu-shown"_s)) {
    return false;
  }

  // Opened the way the keyboard opens it, so Qt treats it as the Keys
  // menu's own submenu; a second independent popup would close the first.
  keys_menu->setActiveAction(import_entry);
  if (!ctx.WaitUntil(
          [&]() {
            QKeyEvent right(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
            QApplication::sendEvent(keys_menu, &right);
          },
          [&]() { return import_menu->isVisible(); },
          {ScenarioContext::On(import_menu, &QMenu::aboutToShow)},
          u"import-key-menu-shown"_s)) {
    return false;
  }
  import_menu->setActiveAction(key_server);

  ctx.SetTarget(window, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(keys_menu);
  ctx.AddPopup(import_menu);
  ctx.AddHighlight(u"keysMenu"_s, keys_menu);
  ctx.AddHighlight(u"importKeyMenu"_s, import_menu);
  ctx.AddHighlight(u"importKeyFromKeyServer"_s, import_menu,
                   import_menu->actionGeometry(key_server));

  return ctx.WaitSettled(
      window,
      [&]() {
        return bar->activeAction() == keys_entry &&
               keys_menu->activeAction() == import_entry &&
               import_menu->activeAction() == key_server;
      },
      u"import-key-menu-settled"_s);
}

/**
 * @brief The key list's Key Groups tab, with the demo group selected.
 */
auto KeyGroupsScenario(ScenarioContext& ctx) -> bool {
  auto* window =
      OpenMainWindow(ctx, ctx.GetSize(), u"contract.txt"_s, ContractText());
  if (window == nullptr) return false;

  auto* categories = window->findChild<QListWidget*>(u"KeyCategoryList"_s);
  auto* groups = window->findChild<QTableView*>(u"key_group"_s);
  if (categories == nullptr || groups == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"key-group-tab-missing"_s);
    return false;
  }

  int row = -1;
  for (int i = 0; i < categories->count(); ++i) {
    if (categories->item(i)->data(Qt::UserRole).toString() == "key_group"_L1) {
      row = i;
    }
  }
  if (row < 0) {
    ctx.Fail(kEXIT_USAGE, u"key-group-tab-missing"_s);
    return false;
  }

  int group_row = -1;
  if (!ctx.WaitUntil([&]() { categories->setCurrentRow(row); },
                     [&]() {
                       group_row =
                           RowOf(groups, QLatin1StringView(kDemoGroupName));
                       return groups->isVisible() && group_row >= 0;
                     },
                     ModelHooks(groups->model()), u"key-group-listed"_s)) {
    return false;
  }
  groups->setFocus();
  groups->selectRow(group_row);

  ctx.SetTarget(window, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"KeyCategoryList"_s, categories);
  ctx.AddHighlight(u"key_group"_s, groups);

  return ctx.WaitSettled(
      window,
      [&]() { return groups->selectionModel()->isRowSelected(group_row, {}); },
      u"key-group-selected"_s);
}

/**
 * @brief The Key ToolBox's database menu, open, with the Work database
 *        under the pointer's place.
 */
auto KeyDatabaseMenuScenario(ScenarioContext& ctx) -> bool {
  auto* window =
      OpenMainWindow(ctx, ctx.GetSize(), u"contract.txt"_s, ContractText());
  if (window == nullptr) return false;

  auto* button = window->findChild<QToolButton*>(u"switchContextButton"_s);
  auto* menu = button != nullptr ? button->menu() : nullptr;
  if (menu == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"key-database-menu-missing"_s);
    return false;
  }

  QAction* work = nullptr;
  for (auto* action : menu->actions()) {
    if (action->text().contains("Work"_L1)) work = action;
  }
  if (work == nullptr || menu->actions().size() < 3) {
    ctx.Fail(kEXIT_USAGE, u"key-databases-not-listed"_s,
             QString::number(menu->actions().size()));
    return false;
  }

  // QToolButton::showMenu() runs the menu's own loop; the popup is opened
  // under the button instead, with the button drawn pressed as it would be.
  const auto at = button->mapToGlobal(button->rect().bottomLeft());
  if (!ctx.WaitUntil(
          [&]() {
            button->setDown(true);
            menu->popup(at);
          },
          [&]() { return menu->isVisible(); },
          {ScenarioContext::On(menu, &QMenu::aboutToShow)},
          u"key-database-menu-shown"_s)) {
    return false;
  }
  menu->setActiveAction(work);

  ctx.SetTarget(window, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(menu);
  ctx.AddHighlight(u"switchContextButton"_s, button);
  ctx.AddHighlight(u"keyDatabaseWork"_s, menu, menu->actionGeometry(work));

  return ctx.WaitSettled(
      window, [&]() { return menu->activeAction() == work; },
      u"key-database-menu-settled"_s);
}

/**
 * @brief Settings, on the Key Databases page, with three databases.
 */
auto SettingsKeyDatabasesScenario(ScenarioContext& ctx) -> bool {
  auto* window =
      OpenMainWindow(ctx, kBackdropSize, u"contract.txt"_s, ContractText());
  if (window == nullptr) return false;

  const auto [dialog, page] =
      OpenSettingsPage(ctx, window, "GpgFrontend::UI::KeyDatabasesTab");
  if (page == nullptr) return false;

  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  ctx.AddHighlight(u"KeyDatabasesTab"_s, page);

  return ctx.WaitSettled(
      dialog,
      [&]() {
        return dialog->isActiveWindow() && page->isVisible() &&
               (!ctx.GetSize().isValid() || dialog->size() == ctx.GetSize());
      },
      u"key-databases-page-settled"_s);
}

}  // namespace

void RegisterMainWindowScenarios(QList<Scenario>& scenarios) {
  const QStringList main_window{u"GpgFrontend::UI::MainWindow"_s};
  const QStringList with_waiting{u"GpgFrontend::UI::MainWindow"_s,
                                 u"GpgFrontend::UI::WaitingDialog"_s};
  const QStringList with_keygen{u"GpgFrontend::UI::MainWindow"_s,
                                u"GpgFrontend::UI::KeyGenerateWizard"_s};

  scenarios.append({u"main-window"_s, QSize(1280, 960), CaptureMode::kWINDOW,
                    main_window, false, true, MainWindowScenario});
  scenarios.append({u"key-generate-dialog"_s, QSize(), CaptureMode::kDIALOG,
                    with_keygen, false, true, KeyGenerateDialogScenario});
  scenarios.append({u"text-decrypt-verify-result"_s, QSize(1280, 960),
                    CaptureMode::kWINDOW, with_waiting, false, true,
                    DecryptVerifyResultScenario});
  scenarios.append({u"main-window-menu-popup"_s, QSize(1280, 960),
                    CaptureMode::kCOMPOSITE, main_window, false, true,
                    MenuPopupScenario});
  scenarios.append({u"main-window-key-groups"_s, QSize(1280, 960),
                    CaptureMode::kWINDOW, main_window, false, true,
                    KeyGroupsScenario});

  // The two below ask for the Work (GnuPG) and Lab (rPGP) databases.
  const QStringList extra_databases{u"work"_s, u"lab"_s};
  Scenario menu{u"key-database-menu"_s,
                QSize(1280, 960),
                CaptureMode::kCOMPOSITE,
                main_window,
                false,
                true,
                KeyDatabaseMenuScenario};
  menu.key_databases = extra_databases;
  scenarios.append(menu);

  // Wide enough for the database table's columns, which the dialog's own
  // size cuts off.
  Scenario settings{
      u"settings-key-databases"_s,
      QSize(900, 660),
      CaptureMode::kDIALOG,
      {u"GpgFrontend::UI::MainWindow"_s, u"GpgFrontend::UI::SettingsDialog"_s},
      false,
      true,
      SettingsKeyDatabasesScenario};
  settings.key_databases = extra_databases;
  scenarios.append(settings);
}

}  // namespace GpgFrontend::Screenshot
