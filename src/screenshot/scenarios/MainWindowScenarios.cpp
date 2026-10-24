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
#include <QToolButton>

#include "screenshot/ScreenshotEnv.h"
#include "screenshot/ScreenshotScenario.h"
#include "screenshot/ScreenshotSeed.h"
#include "ui/main_window/MainWindow.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::Screenshot {

namespace {

using UI::InfoBoardWidget;
using UI::MainWindow;
using UI::TextEdit;

/// The keys the scenarios work with. The main window's default tab hides
/// expired and revoked keys, so Carol and Dave are never listed there.
constexpr auto kAliceEmail = "alice@example.com";
constexpr auto kBobEmail = "bob@example.com";
/// Window size behind dialog-only captures, which never shows.
const QSize kBackdropSize(1100, 680);
/// Logical width of the Key ToolBox dock.
constexpr int kKeyDockWidth = 480;
const auto kContractText = QStringLiteral(
    "Hello Bob,\n\nplease find the signed contract below.\n\nAlice\n");

auto KeyTable(MainWindow* window) -> QTableView* {
  return window->findChild<QTableView*>(QStringLiteral("default"));
}

auto RowOf(QTableView* table, const QString& needle) -> int {
  auto* model = table->model();
  for (int row = 0; row < model->rowCount(); ++row) {
    for (int col = 0; col < model->columnCount(); ++col) {
      if (model->index(row, col).data().toString().contains(needle)) return row;
    }
  }
  return -1;
}

auto ModelHooks(QAbstractItemModel* model) -> QList<ScenarioContext::Hook> {
  return {ScenarioContext::On(model, &QAbstractItemModel::modelReset),
          ScenarioContext::On(model, &QAbstractItemModel::rowsInserted),
          ScenarioContext::On(model, &QAbstractItemModel::layoutChanged)};
}

auto CurrentEditor(MainWindow* window) -> QPlainTextEdit* {
  for (auto* editor : window->findChildren<QPlainTextEdit*>(
           QStringLiteral("PlainTextEditor"))) {
    if (editor->isVisible()) return editor;
  }
  return nullptr;
}

/**
 * @brief Create, show and activate the main window, wait for the demo keys
 *        to be listed, and open a text tab holding @p tab_content.
 */
auto OpenMainWindow(ScenarioContext& ctx, QSize size, const QString& tab_title,
                    const QString& tab_content) -> MainWindow* {
  // Owned as a QObject: deleting through the virtual destructor keeps this
  // file from emitting MainWindow's, whose base class gf_ui does not export.
  auto* window_ptr = new MainWindow();
  ctx.Keep(QSharedPointer<QObject>(window_ptr));
  const QPointer<MainWindow> window(window_ptr);
  window->Init();
  window->resize(size);

  auto* table = KeyTable(window_ptr);
  if (table == nullptr) {
    ctx.Fail(kEXIT_USAGE, QStringLiteral("no-key-table"));
    return nullptr;
  }

  const auto ready = [&]() {
    return window->isVisible() && window->isActiveWindow() &&
           RowOf(table, kAliceEmail) >= 0 && RowOf(table, kBobEmail) >= 0;
  };
  auto hooks = ModelHooks(table->model());
  hooks.append(ScenarioContext::On(window_ptr, &MainWindow::SignalLoaded));
  const auto shown = ctx.WaitUntil(
      [&]() {
        window->show();
        window->raise();
        window->activateWindow();
      },
      ready, hooks, QStringLiteral("main-window-keys-listed-and-active"),
      [&]() {
        return QStringLiteral("visible=%1 active=%2 rows=%3")
            .arg(window->isVisible())
            .arg(window->isActiveWindow())
            .arg(table->model()->rowCount());
      });
  if (!shown) return nullptr;

  // Init() defers its default layout to the event loop, and that resizes and
  // re-centres the window; size, place and dock width are applied once it
  // has run. A size the platform still refuses is the size gate's to report.
  auto* key_dock =
      window->findChild<QDockWidget*>(QStringLiteral("keyListDock"));
  const auto top_left =
      QApplication::primaryScreen() != nullptr
          ? QApplication::primaryScreen()->availableGeometry().topLeft()
          : QPoint(0, 0);
  ctx.WaitUntil(
      [&]() {
        window->resize(size);
        window->move(top_left);
        // Wide enough that a name and an email address are read whole.
        if (key_dock != nullptr) {
          window->resizeDocks({key_dock}, {kKeyDockWidth}, Qt::Horizontal);
        }
      },
      [&]() { return window->size() == size; }, {},
      QStringLiteral("main-window-resized"));
  if (ctx.GetFailure()) return nullptr;
  if (const auto bad = SizeGateFailure(size, window->size())) {
    ctx.Fail(kEXIT_ENVIRONMENT, QStringLiteral("gate"), *bad);
    return nullptr;
  }
  // A text tab in front: a fresh profile starts on the file browser, whose
  // free-space line describes the machine rendering rather than the demo.
  window_ptr->findChild<TextEdit*>()->SlotNewTabWithContent(tab_title,
                                                            tab_content);
  return window_ptr;
}

auto MainWindowScenario(ScenarioContext& ctx) -> bool {
  auto* window = OpenMainWindow(ctx, ctx.GetSize(),
                                QStringLiteral("contract.txt"), kContractText);
  if (window == nullptr) return false;

  auto* table = KeyTable(window);
  const auto alice = RowOf(table, kAliceEmail);
  const auto bob = RowOf(table, kBobEmail);
  if (alice < 0 || bob < 0) {
    ctx.Fail(kEXIT_USAGE, QStringLiteral("demo-key-missing"));
    return false;
  }

  auto* model = table->model();
  table->setFocus();
  table->selectRow(alice);
  model->setData(model->index(bob, 0), Qt::Checked, Qt::CheckStateRole);

  ctx.SetTarget(window, CaptureMode::kWINDOW);
  ctx.AddHighlight(QStringLiteral("keyListDock"),
                   window->findChild<QWidget*>(QStringLiteral("keyListDock")));
  ctx.AddHighlight(QStringLiteral("cryptToolBar"),
                   window->findChild<QWidget*>(QStringLiteral("cryptToolBar")));
  ctx.AddHighlight(QStringLiteral("PlainTextEditor"), CurrentEditor(window));

  return ctx.WaitSettled(
      window,
      [&]() {
        return table->selectionModel()->isRowSelected(alice, {}) &&
               model->index(bob, 0).data(Qt::CheckStateRole).toInt() ==
                   Qt::Checked;
      },
      QStringLiteral("main-window-selection"));
}

auto KeyGenerateDialogScenario(ScenarioContext& ctx) -> bool {
  auto* window = OpenMainWindow(ctx, kBackdropSize,
                                QStringLiteral("contract.txt"), kContractText);
  if (window == nullptr) return false;

  auto* action =
      window->findChild<QAction*>(QStringLiteral("generate_key_pair"));
  if (action == nullptr || !action->isEnabled()) {
    ctx.Fail(kEXIT_USAGE, QStringLiteral("generate-key-unavailable"));
    return false;
  }

  QWidget* dialog = nullptr;
  const auto opened =
      ctx.WaitUntil([&]() { action->trigger(); },
                    [&]() {
                      for (auto* w : QApplication::topLevelWidgets()) {
                        if (w->isVisible() &&
                            w->inherits("GpgFrontend::UI::KeyGenerateDialog")) {
                          dialog = w;
                          return true;
                        }
                      }
                      return false;
                    },
                    {}, QStringLiteral("key-generate-dialog-shown"));
  if (!opened) return false;

  if (ctx.GetSize().isValid()) {
    dialog->resize(ctx.GetSize());
  }

  auto* name = dialog->findChild<QLineEdit*>(QStringLiteral("nameEdit"));
  auto* email = dialog->findChild<QLineEdit*>(QStringLiteral("emailEdit"));
  auto* comment = dialog->findChild<QLineEdit*>(QStringLiteral("commentEdit"));
  auto* generate =
      dialog->findChild<QPushButton*>(QStringLiteral("generateButton"));
  if (name == nullptr || email == nullptr || comment == nullptr ||
      generate == nullptr) {
    ctx.Fail(kEXIT_USAGE, QStringLiteral("key-generate-fields-missing"));
    return false;
  }

  name->setText(QStringLiteral("Alice Example"));
  email->setText(QStringLiteral("alice@example.com"));
  comment->setText(QStringLiteral("Work"));
  generate->setFocus();

  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  ctx.AddHighlight(QStringLiteral("nameEdit"), name);
  ctx.AddHighlight(QStringLiteral("emailEdit"), email);
  ctx.AddHighlight(QStringLiteral("commentEdit"), comment);
  ctx.AddHighlight(QStringLiteral("generateButton"), generate);

  return ctx.WaitSettled(
      dialog,
      [&]() {
        return dialog->isActiveWindow() &&
               (!ctx.GetSize().isValid() || dialog->size() == ctx.GetSize()) &&
               name->text() == QLatin1String("Alice Example");
      },
      QStringLiteral("key-generate-dialog-filled"));
}

auto DecryptVerifyResultScenario(ScenarioContext& ctx) -> bool {
  QFile fixture(ctx.FixturePath(QStringLiteral("decrypt-verify.asc")));
  if (!fixture.open(QIODevice::ReadOnly)) {
    ctx.Fail(kEXIT_USAGE, QStringLiteral("fixture-missing"),
             fixture.fileName());
    return false;
  }
  const auto armored = QString::fromUtf8(fixture.readAll());

  auto* window = OpenMainWindow(ctx, ctx.GetSize(),
                                QStringLiteral("message.asc"), armored);
  if (window == nullptr) return false;

  auto* board = window->findChild<InfoBoardWidget*>();

  bool posted = false;
  const auto done = ctx.WaitUntil(
      [&]() { window->SlotDecryptVerify(); },
      [&]() {
        auto* editor = CurrentEditor(window);
        return posted && editor != nullptr &&
               editor->toPlainText().contains(
                   QLatin1String("the quarterly report is attached"));
      },
      {[&](const std::function<void()>& check) {
        return QObject::connect(board, &InfoBoardWidget::SignalResultPosted,
                                board, [&posted, check]() {
                                  posted = true;
                                  check();
                                });
      }},
      QStringLiteral("decrypt-verify-result-posted"));
  if (!done) return false;

  // The result is what the reader looks at; no caret in the editor.
  board->setFocus();
  ctx.SetTarget(window, CaptureMode::kWINDOW);
  ctx.AddHighlight(QStringLiteral("InfoBoardWidget"), board);
  ctx.AddHighlight(QStringLiteral("PlainTextEditor"), CurrentEditor(window));

  return ctx.WaitSettled(window, {}, QStringLiteral("decrypt-verify-settled"));
}

/**
 * @brief The menu that holds @p action, among @p menus' own actions.
 */
auto MenuHolding(const QList<QAction*>& actions, const QAction* action)
    -> std::pair<QMenu*, QAction*> {
  for (auto* entry : actions) {
    if (entry->menu() != nullptr && entry->menu()->actions().contains(action)) {
      return {entry->menu(), entry};
    }
  }
  return {nullptr, nullptr};
}

/**
 * @brief Keys > Import Key > Key Server, as the manual walks through it.
 *
 * "Key Server" is not the Host's: the key server module places it in the
 * Import Key menu, so it is whatever entry there the Host did not add.
 */
auto MenuPopupScenario(ScenarioContext& ctx) -> bool {
  auto* window = OpenMainWindow(ctx, ctx.GetSize(),
                                QStringLiteral("contract.txt"), kContractText);
  if (window == nullptr) return false;

  auto* bar = window->menuBar();
  const auto* generate =
      window->findChild<QAction*>(QStringLiteral("generate_key_pair"));
  const QList<QAction*> host_imports{
      window->findChild<QAction*>(QStringLiteral("import_key_from_file")),
      window->findChild<QAction*>(QStringLiteral("import_key_from_edit")),
      window->findChild<QAction*>(QStringLiteral("import_key_from_clipboard"))};

  auto [keys_menu, keys_entry] = MenuHolding(bar->actions(), generate);
  if (keys_menu == nullptr || host_imports.contains(nullptr)) {
    ctx.Fail(kEXIT_USAGE, QStringLiteral("keys-menu-missing"));
    return false;
  }
  auto [import_menu, import_entry] =
      MenuHolding(keys_menu->actions(), host_imports.front());
  if (import_menu == nullptr) {
    ctx.Fail(kEXIT_USAGE, QStringLiteral("import-key-menu-missing"));
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
    ctx.Fail(kEXIT_USAGE, QStringLiteral("key-server-action-missing"));
    return false;
  }

  // Through the menu bar, so "Keys" is drawn as the open item.
  if (!ctx.WaitUntil([&]() { bar->setActiveAction(keys_entry); },
                     [&]() { return keys_menu->isVisible(); },
                     {ScenarioContext::On(keys_menu, &QMenu::aboutToShow)},
                     QStringLiteral("keys-menu-shown"))) {
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
          QStringLiteral("import-key-menu-shown"))) {
    return false;
  }
  import_menu->setActiveAction(key_server);

  ctx.SetTarget(window, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(keys_menu);
  ctx.AddPopup(import_menu);
  ctx.AddHighlight(QStringLiteral("keysMenu"), keys_menu);
  ctx.AddHighlight(QStringLiteral("importKeyMenu"), import_menu);
  ctx.AddHighlight(QStringLiteral("importKeyFromKeyServer"), import_menu,
                   import_menu->actionGeometry(key_server));

  return ctx.WaitSettled(
      window,
      [&]() {
        return bar->activeAction() == keys_entry &&
               keys_menu->activeAction() == import_entry &&
               import_menu->activeAction() == key_server;
      },
      QStringLiteral("import-key-menu-settled"));
}

/**
 * @brief The key list's Key Groups tab, with the demo group selected.
 */
auto KeyGroupsScenario(ScenarioContext& ctx) -> bool {
  auto* window = OpenMainWindow(ctx, ctx.GetSize(),
                                QStringLiteral("contract.txt"), kContractText);
  if (window == nullptr) return false;

  auto* categories =
      window->findChild<QListWidget*>(QStringLiteral("KeyCategoryList"));
  auto* groups = window->findChild<QTableView*>(QStringLiteral("key_group"));
  if (categories == nullptr || groups == nullptr) {
    ctx.Fail(kEXIT_USAGE, QStringLiteral("key-group-tab-missing"));
    return false;
  }

  int row = -1;
  for (int i = 0; i < categories->count(); ++i) {
    if (categories->item(i)->data(Qt::UserRole).toString() ==
        QLatin1String("key_group")) {
      row = i;
    }
  }
  if (row < 0) {
    ctx.Fail(kEXIT_USAGE, QStringLiteral("key-group-tab-missing"));
    return false;
  }

  int group_row = -1;
  if (!ctx.WaitUntil([&]() { categories->setCurrentRow(row); },
                     [&]() {
                       group_row = RowOf(groups, QLatin1String(kDemoGroupName));
                       return groups->isVisible() && group_row >= 0;
                     },
                     ModelHooks(groups->model()),
                     QStringLiteral("key-group-listed"))) {
    return false;
  }
  groups->setFocus();
  groups->selectRow(group_row);

  ctx.SetTarget(window, CaptureMode::kWINDOW);
  ctx.AddHighlight(QStringLiteral("KeyCategoryList"), categories);
  ctx.AddHighlight(QStringLiteral("key_group"), groups);

  return ctx.WaitSettled(
      window,
      [&]() { return groups->selectionModel()->isRowSelected(group_row, {}); },
      QStringLiteral("key-group-selected"));
}

/**
 * @brief The Key ToolBox's database menu, open, with the Work database
 *        under the pointer's place.
 */
auto KeyDatabaseMenuScenario(ScenarioContext& ctx) -> bool {
  auto* window = OpenMainWindow(ctx, ctx.GetSize(),
                                QStringLiteral("contract.txt"), kContractText);
  if (window == nullptr) return false;

  auto* button =
      window->findChild<QToolButton*>(QStringLiteral("switchContextButton"));
  auto* menu = button != nullptr ? button->menu() : nullptr;
  if (menu == nullptr) {
    ctx.Fail(kEXIT_USAGE, QStringLiteral("key-database-menu-missing"));
    return false;
  }

  QAction* work = nullptr;
  for (auto* action : menu->actions()) {
    if (action->text().contains(QLatin1String("Work"))) work = action;
  }
  if (work == nullptr || menu->actions().size() < 3) {
    ctx.Fail(kEXIT_USAGE, QStringLiteral("key-databases-not-listed"),
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
          QStringLiteral("key-database-menu-shown"))) {
    return false;
  }
  menu->setActiveAction(work);

  ctx.SetTarget(window, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(menu);
  ctx.AddHighlight(QStringLiteral("switchContextButton"), button);
  ctx.AddHighlight(QStringLiteral("keyDatabaseWork"), menu,
                   menu->actionGeometry(work));

  return ctx.WaitSettled(
      window, [&]() { return menu->activeAction() == work; },
      QStringLiteral("key-database-menu-settled"));
}

/**
 * @brief Settings, on the Key Databases page, with three databases.
 */
auto SettingsKeyDatabasesScenario(ScenarioContext& ctx) -> bool {
  auto* window = OpenMainWindow(ctx, kBackdropSize,
                                QStringLiteral("contract.txt"), kContractText);
  if (window == nullptr) return false;

  auto* action = window->findChild<QAction*>(QStringLiteral("settings"));
  if (action == nullptr) {
    ctx.Fail(kEXIT_USAGE, QStringLiteral("settings-unavailable"));
    return false;
  }

  QWidget* dialog = nullptr;
  if (!ctx.WaitUntil([&]() { action->trigger(); },
                     [&]() {
                       for (auto* w : QApplication::topLevelWidgets()) {
                         if (w->isVisible() &&
                             w->inherits("GpgFrontend::UI::SettingsDialog")) {
                           dialog = w;
                           return true;
                         }
                       }
                       return false;
                     },
                     {}, QStringLiteral("settings-dialog-shown"))) {
    return false;
  }

  // The page is selected the way a click does: through the navigation row
  // whose page index is the Key Databases page's place in the stack.
  QWidget* page = nullptr;
  for (auto* w : dialog->findChildren<QWidget*>()) {
    if (w->inherits("GpgFrontend::UI::KeyDatabasesTab")) page = w;
  }
  // Each page sits in the stack inside a scroll area, so the stack entry is
  // the one that holds the page.
  QStackedWidget* stack = nullptr;
  int page_index = -1;
  for (auto* candidate : dialog->findChildren<QStackedWidget*>()) {
    for (int i = 0; page != nullptr && i < candidate->count(); ++i) {
      auto* entry = candidate->widget(i);
      if (entry == page || entry->isAncestorOf(page)) {
        stack = candidate;
        page_index = i;
      }
    }
  }
  QListWidget* nav = nullptr;
  int nav_row = -1;
  for (auto* candidate : dialog->findChildren<QListWidget*>()) {
    for (int i = 0; stack != nullptr && i < candidate->count(); ++i) {
      const auto data = candidate->item(i)->data(Qt::UserRole);
      if (data.isValid() && data.toInt() == page_index) {
        nav = candidate;
        nav_row = i;
      }
    }
  }
  if (nav == nullptr) {
    ctx.Fail(kEXIT_USAGE, QStringLiteral("key-databases-page-missing"));
    return false;
  }

  if (ctx.GetSize().isValid()) dialog->resize(ctx.GetSize());
  if (!ctx.WaitUntil(
          [&]() { nav->setCurrentRow(nav_row); },
          [&]() { return stack->currentIndex() == page_index; },
          {ScenarioContext::On(stack, &QStackedWidget::currentChanged)},
          QStringLiteral("key-databases-page-shown"))) {
    return false;
  }
  nav->setFocus();

  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  ctx.AddHighlight(QStringLiteral("KeyDatabasesTab"), page);

  return ctx.WaitSettled(
      dialog,
      [&]() {
        return dialog->isActiveWindow() && page->isVisible() &&
               (!ctx.GetSize().isValid() || dialog->size() == ctx.GetSize());
      },
      QStringLiteral("key-databases-page-settled"));
}

}  // namespace

void RegisterMainWindowScenarios(QList<Scenario>& scenarios) {
  const QStringList main_window{QStringLiteral("GpgFrontend::UI::MainWindow")};
  const QStringList with_waiting{
      QStringLiteral("GpgFrontend::UI::MainWindow"),
      QStringLiteral("GpgFrontend::UI::WaitingDialog")};
  const QStringList with_keygen{
      QStringLiteral("GpgFrontend::UI::MainWindow"),
      QStringLiteral("GpgFrontend::UI::KeyGenerateDialog")};

  scenarios.append({QStringLiteral("main-window"), QSize(1100, 680),
                    CaptureMode::kWINDOW, main_window, false, true,
                    MainWindowScenario});
  scenarios.append({QStringLiteral("key-generate-dialog"), QSize(),
                    CaptureMode::kDIALOG, with_keygen, false, true,
                    KeyGenerateDialogScenario});
  scenarios.append({QStringLiteral("text-decrypt-verify-result"),
                    QSize(1100, 680), CaptureMode::kWINDOW, with_waiting, false,
                    true, DecryptVerifyResultScenario});
  scenarios.append({QStringLiteral("main-window-menu-popup"), QSize(1100, 680),
                    CaptureMode::kCOMPOSITE, main_window, false, true,
                    MenuPopupScenario});
  scenarios.append({QStringLiteral("main-window-key-groups"), QSize(1100, 680),
                    CaptureMode::kWINDOW, main_window, false, true,
                    KeyGroupsScenario});

  // The two below ask for the Work (GnuPG) and Lab (rPGP) databases.
  const QStringList extra_databases{QStringLiteral("work"),
                                    QStringLiteral("lab")};
  Scenario menu{QStringLiteral("key-database-menu"),
                QSize(1100, 680),
                CaptureMode::kCOMPOSITE,
                main_window,
                false,
                true,
                KeyDatabaseMenuScenario};
  menu.key_databases = extra_databases;
  scenarios.append(menu);

  // Wide enough for the database table's columns, which the dialog's own
  // size cuts off.
  Scenario settings{QStringLiteral("settings-key-databases"),
                    QSize(900, 660),
                    CaptureMode::kDIALOG,
                    {QStringLiteral("GpgFrontend::UI::MainWindow"),
                     QStringLiteral("GpgFrontend::UI::SettingsDialog")},
                    false,
                    true,
                    SettingsKeyDatabasesScenario};
  settings.key_databases = extra_databases;
  scenarios.append(settings);
}

}  // namespace GpgFrontend::Screenshot
