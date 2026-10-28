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

#include "screenshot/scenarios/ScenarioHelpers.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QGridLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QScreen>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QWizard>

#include "core/model/SettingsObject.h"
#include "screenshot/ScreenshotEnv.h"
#include "ui/main_window/MainWindow.h"
#include "ui/struct/settings_object/AppearanceSO.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::Screenshot {

namespace {

/// The split the application's own default layout uses: the Key ToolBox
/// takes this share of the window's width, the Status Panel this share of
/// its height.
constexpr double kKeyDockShare = 0.35;
constexpr double kStatusPanelShare = 0.30;

}  // namespace

auto ContractText() -> QString {
  return u"Hello Bob,\n\nplease find the signed contract below.\n\nAlice\n"_s;
}

auto KeyTable(UI::MainWindow* window) -> QTableView* {
  return window->findChild<QTableView*>(u"default"_s);
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

auto CurrentEditor(UI::MainWindow* window) -> QPlainTextEdit* {
  for (auto* editor :
       window->findChildren<QPlainTextEdit*>(u"PlainTextEditor"_s)) {
    if (editor->isVisible()) return editor;
  }
  return nullptr;
}

auto OpenMainWindow(ScenarioContext& ctx, QSize size, const QString& tab_title,
                    const QString& tab_content) -> UI::MainWindow* {
  // Owned as a QObject: deleting through the virtual destructor keeps this
  // file from emitting MainWindow's, whose base class gf_ui does not export.
  auto* window_ptr = new UI::MainWindow();
  ctx.Keep(QSharedPointer<QObject>(window_ptr));
  const QPointer<UI::MainWindow> window(window_ptr);
  window->Init();
  window->resize(size);
  // Init() queues its default layout as a zero timer, which resizes and
  // re-centres the window. Zero timers fire in the order they were started,
  // so once this one has fired, that layout has run too.
  const auto init_layout_done = QSharedPointer<bool>::create(false);
  QTimer::singleShot(0, window_ptr,
                     [init_layout_done]() { *init_layout_done = true; });

  auto* table = KeyTable(window_ptr);
  if (table == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"no-key-table"_s);
    return nullptr;
  }

  const auto ready = [&]() {
    return *init_layout_done && window->isVisible() &&
           window->isActiveWindow() && RowOf(table, kAliceEmail) >= 0 &&
           RowOf(table, kBobEmail) >= 0;
  };
  auto hooks = ModelHooks(table->model());
  hooks.append(ScenarioContext::On(window_ptr, &UI::MainWindow::SignalLoaded));
  const auto shown = ctx.WaitUntil(
      [&]() {
        window->show();
        window->raise();
        window->activateWindow();
      },
      ready, hooks, u"main-window-keys-listed-and-active"_s,
      [&]() {
        return u"visible=%1 active=%2 rows=%3"_s.arg(window->isVisible())
            .arg(window->isActiveWindow())
            .arg(table->model()->rowCount());
      });
  if (!shown) return nullptr;

  // Size, place and dock width are applied only now that the default layout
  // has run, and always: the window may still read the requested size from
  // before that layout, which would make a wait on the size alone skip them.
  // A size the platform still refuses is the size gate's to report.
  auto* key_dock = window->findChild<QDockWidget*>(u"keyListDock"_s);
  auto* status_dock = window->findChild<QDockWidget*>(u"infoBoardDock"_s);
  const auto top_left =
      QApplication::primaryScreen() != nullptr
          ? QApplication::primaryScreen()->availableGeometry().topLeft()
          : QPoint(0, 0);
  window->resize(size);
  window->move(top_left);
  if (key_dock != nullptr) {
    window->resizeDocks({key_dock},
                        {static_cast<int>(size.width() * kKeyDockShare)},
                        Qt::Horizontal);
  }
  if (status_dock != nullptr) {
    window->resizeDocks({status_dock},
                        {static_cast<int>(size.height() * kStatusPanelShare)},
                        Qt::Vertical);
  }
  ctx.WaitUntil(
      {}, [&]() { return window->size() == size && window->pos() == top_left; },
      {}, u"main-window-resized"_s,
      [&]() {
        return u"size=%1x%2 pos=%3,%4"_s.arg(window->width())
            .arg(window->height())
            .arg(window->x())
            .arg(window->y());
      });
  if (ctx.GetFailure()) return nullptr;
  if (const auto bad = SizeGateFailure(size, window->size())) {
    ctx.Fail(kEXIT_ENVIRONMENT, u"gate"_s, *bad);
    return nullptr;
  }
  // A text tab in front: a fresh profile starts on the file browser, whose
  // free-space line describes the machine rendering rather than the demo.
  window_ptr->findChild<UI::TextEdit*>()->SlotNewTabWithContent(tab_title,
                                                                tab_content);
  return window_ptr;
}

auto MenuHolding(const QList<QAction*>& actions, const QAction* action)
    -> std::pair<QMenu*, QAction*> {
  for (auto* entry : actions) {
    if (entry->menu() != nullptr && entry->menu()->actions().contains(action)) {
      return {entry->menu(), entry};
    }
  }
  return {nullptr, nullptr};
}

auto VisibleTopLevel(const char* class_name) -> QWidget* {
  for (auto* w : QApplication::topLevelWidgets()) {
    if (w->isVisible() && w->inherits(class_name)) return w;
  }
  return nullptr;
}

auto WaitForTopLevel(ScenarioContext& ctx, const std::function<void()>& trigger,
                     const char* class_name, const QString& label) -> QWidget* {
  QWidget* found = nullptr;
  ctx.WaitUntil(
      trigger,
      [&]() {
        found = VisibleTopLevel(class_name);
        return found != nullptr;
      },
      {}, label);
  return found;
}

auto SelectKeyRow(ScenarioContext& ctx, QTableView* table, const char* email)
    -> bool {
  const auto row = RowOf(table, QString::fromLatin1(email));
  if (row < 0) {
    ctx.Fail(kEXIT_USAGE, u"demo-key-missing"_s, QString::fromLatin1(email));
    return false;
  }
  table->setFocus();
  table->selectRow(row);
  return true;
}

auto OpenKeyMgmt(ScenarioContext& ctx, QSize size) -> QWidget* {
  auto* window =
      OpenMainWindow(ctx, kBackdropSize, u"contract.txt"_s, ContractText());
  if (window == nullptr) return nullptr;

  auto* action = window->findChild<QAction*>(u"open_key_management"_s);
  if (action == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"key-management-unavailable"_s);
    return nullptr;
  }
  auto* mgmt = WaitForTopLevel(
      ctx, [&]() { action->trigger(); }, kKeyMgmtClass,
      u"key-management-shown"_s);
  if (mgmt == nullptr) return nullptr;

  // Like the main window, it queues its default layout as a zero timer;
  // one queued after it has fired once that layout has run.
  bool layout_done = false;
  QTimer::singleShot(0, mgmt, [&layout_done]() { layout_done = true; });
  if (!ctx.WaitUntil(
          {}, [&]() { return layout_done; }, {},
          u"key-management-laid-out"_s)) {
    return nullptr;
  }
  mgmt->resize(size);
  mgmt->raise();
  mgmt->activateWindow();
  if (!ctx.WaitUntil(
          {}, [&]() { return mgmt->size() == size && mgmt->isActiveWindow(); },
          {}, u"key-management-resized"_s)) {
    return nullptr;
  }
  return mgmt;
}

auto ShowCategory(ScenarioContext& ctx, QWidget* mgmt,
                  const QString& category_id) -> QTableView* {
  auto* categories = mgmt->findChild<QListWidget*>(u"KeyCategoryList"_s);
  auto* table = mgmt->findChild<QTableView*>(category_id);
  int row = -1;
  for (int i = 0; categories != nullptr && i < categories->count(); ++i) {
    if (categories->item(i)->data(Qt::UserRole).toString() == category_id) {
      row = i;
    }
  }
  if (table == nullptr || row < 0) {
    ctx.Fail(kEXIT_USAGE, u"key-category-missing"_s, category_id);
    return nullptr;
  }
  if (!ctx.WaitUntil([&]() { categories->setCurrentRow(row); },
                     [&]() {
                       return table->isVisible() &&
                              table->model()->rowCount() > 0;
                     },
                     ModelHooks(table->model()), u"key-category-listed"_s)) {
    return nullptr;
  }
  return table;
}

auto CheckKeyRow(ScenarioContext& ctx, QTableView* table, const char* email)
    -> bool {
  const auto row = RowOf(table, QString::fromLatin1(email));
  if (row < 0) {
    ctx.Fail(kEXIT_USAGE, u"demo-key-missing"_s, QString::fromLatin1(email));
    return false;
  }
  auto* model = table->model();
  model->setData(model->index(row, 0), Qt::Checked, Qt::CheckStateRole);
  return true;
}

auto ColumnRect(QTableView* table, int column) -> QRect {
  auto* header = table->horizontalHeader();
  const auto rows = table->model()->rowCount();
  const auto bottom = rows > 0 ? table->rowViewportPosition(rows - 1) +
                                     table->rowHeight(rows - 1)
                               : 0;
  return {header->sectionViewportPosition(column) + table->viewport()->x(),
          header->y(), header->sectionSize(column), header->height() + bottom};
}

auto OpenSettingsPage(ScenarioContext& ctx, UI::MainWindow* window,
                      const char* page_class) -> std::pair<QWidget*, QWidget*> {
  auto* action = window->findChild<QAction*>(u"settings"_s);
  if (action == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"settings-unavailable"_s);
    return {nullptr, nullptr};
  }
  auto* dialog = WaitForTopLevel(
      ctx, [&]() { action->trigger(); }, "GpgFrontend::UI::SettingsDialog",
      u"settings-dialog-shown"_s);
  if (dialog == nullptr) return {nullptr, nullptr};

  QWidget* page = nullptr;
  for (auto* w : dialog->findChildren<QWidget*>()) {
    if (w->inherits(page_class)) page = w;
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
    ctx.Fail(kEXIT_USAGE, u"settings-page-missing"_s,
             QString::fromLatin1(page_class));
    return {nullptr, nullptr};
  }

  if (ctx.GetSize().isValid()) dialog->resize(ctx.GetSize());
  if (!ctx.WaitUntil(
          [&]() { nav->setCurrentRow(nav_row); },
          [&]() { return stack->currentIndex() == page_index; },
          {ScenarioContext::On(stack, &QStackedWidget::currentChanged)},
          u"settings-page-shown"_s)) {
    return {nullptr, nullptr};
  }
  nav->setFocus();
  return {dialog, page};
}

auto ToolBarButtonFor(UI::MainWindow* window, const QString& action_name)
    -> QWidget* {
  auto* action = window->findChild<QAction*>(action_name);
  if (action == nullptr) return nullptr;
  for (auto* bar : window->findChildren<QToolBar*>()) {
    if (auto* own = bar->widgetForAction(action); own != nullptr) return own;
    for (auto* button : bar->findChildren<QToolButton*>()) {
      if (button->menu() != nullptr &&
          button->menu()->actions().contains(action)) {
        return button;
      }
    }
  }
  return nullptr;
}

void OfferToolBarOperation(int opera) {
  SettingsObject state(u"general_settings_state"_s);
  UI::AppearanceSO appearance(state);
  appearance.tool_bar_crypto_operas_type |= opera;
  state.Store(appearance.ToJson());
}

auto ShowTab(ScenarioContext& ctx, QWidget* dialog, const char* tab_class)
    -> QWidget* {
  auto* tabs = dialog->findChild<QTabWidget*>();
  for (int i = 0; tabs != nullptr && i < tabs->count(); ++i) {
    if (tabs->widget(i)->inherits(tab_class)) {
      tabs->setCurrentIndex(i);
      return tabs->widget(i);
    }
  }
  ctx.Fail(kEXIT_USAGE, u"key-details-tab-missing"_s,
           QString::fromLatin1(tab_class));
  return nullptr;
}

auto SwitchKeyDatabase(ScenarioContext& ctx, UI::MainWindow* window,
                       const QString& name, const QString& listed_email)
    -> bool {
  auto* button = window->findChild<QToolButton*>(u"switchContextButton"_s);
  QAction* entry = nullptr;
  for (auto* action : button != nullptr && button->menu() != nullptr
                          ? button->menu()->actions()
                          : QList<QAction*>{}) {
    if (action->text().contains(name)) entry = action;
  }
  if (entry == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"key-database-missing"_s, name);
    return false;
  }
  // Alice is in more than one database: her row alone does not show the
  // switch happened. The table must have been refilled since, too.
  auto* table = KeyTable(window);
  auto* const before = table->model();
  bool refilled = false;
  const auto hook = QObject::connect(before, &QAbstractItemModel::modelReset,
                                     [&refilled]() { refilled = true; });
  const auto switched = ctx.WaitUntil(
      [&]() { entry->trigger(); },
      [&]() {
        return (refilled || table->model() != before) &&
               button->text().contains(name) && RowOf(table, listed_email) >= 0;
      },
      {}, u"key-database-listed"_s,
      [&]() {
        return u"button=%1 refilled=%2"_s.arg(button->text())
            .arg(refilled || table->model() != before);
      });
  QObject::disconnect(hook);
  return switched;
}

auto OpenKeyDetails(ScenarioContext& ctx, const char* email,
                    const QString& database) -> QWidget* {
  auto* window =
      OpenMainWindow(ctx, kBackdropSize, u"contract.txt"_s, ContractText());
  if (window == nullptr) return nullptr;
  if (!database.isEmpty() &&
      !SwitchKeyDatabase(ctx, window, database, QString::fromLatin1(email))) {
    return nullptr;
  }

  auto* action = window->findChild<QAction*>(u"show_key_details"_s);
  if (action == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"show-key-details-missing"_s);
    return nullptr;
  }
  if (!SelectKeyRow(ctx, KeyTable(window), email)) return nullptr;
  return WaitForTopLevel(
      ctx, [&]() { action->trigger(); }, kKeyDetailsClass,
      u"key-details-shown"_s);
}

auto ShowButtonMenu(ScenarioContext& ctx, QAbstractButton* button, QMenu* menu,
                    QAction* entry) -> bool {
  if (button == nullptr || menu == nullptr || entry == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"button-menu-missing"_s);
    return false;
  }
  // popup() rather than showMenu(): the latter runs a nested event loop.
  if (!ctx.WaitUntil(
          [&]() {
            menu->popup(button->mapToGlobal(QPoint(0, button->height())));
          },
          [&]() { return menu->isVisible(); },
          {ScenarioContext::On(menu, &QMenu::aboutToShow)},
          u"button-menu-shown"_s)) {
    return false;
  }
  menu->setActiveAction(entry);
  return true;
}

auto OpenSubmenu(ScenarioContext& ctx, QMenu* menu, QAction* entry) -> QMenu* {
  auto* submenu = entry != nullptr ? entry->menu() : nullptr;
  if (submenu == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"submenu-missing"_s);
    return nullptr;
  }
  // A second independent popup would close the first.
  menu->setActiveAction(entry);
  if (!ctx.WaitUntil(
          [&]() {
            QKeyEvent right(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
            QApplication::sendEvent(menu, &right);
          },
          [&]() { return submenu->isVisible(); },
          {ScenarioContext::On(submenu, &QMenu::aboutToShow)},
          u"submenu-shown"_s)) {
    return nullptr;
  }
  return submenu;
}

auto SettleActive(ScenarioContext& ctx, QWidget* window, const QString& label)
    -> bool {
  return ctx.WaitSettled(
      window,
      [&]() {
        if (!window->isActiveWindow()) window->activateWindow();
        return window->isActiveWindow();
      },
      label);
}

auto FocusWhenActive(ScenarioContext& ctx, QWidget* window, QWidget* widget)
    -> bool {
  if (!ctx.WaitUntil(
          {},
          [&]() {
            if (!window->isActiveWindow()) window->activateWindow();
            return window->isActiveWindow();
          },
          {}, u"window-active"_s)) {
    return false;
  }
  if (widget != nullptr) widget->setFocus();
  return true;
}

auto AdvanceWizard(ScenarioContext& ctx, QWizard* wizard, int steps) -> bool {
  for (int i = 0; i < steps; ++i) {
    const auto from = wizard->currentId();
    if (!ctx.WaitUntil(
            [&]() { wizard->next(); },
            [&]() { return wizard->currentId() != from; },
            {ScenarioContext::On(wizard, &QWizard::currentIdChanged)},
            u"wizard-step-%1"_s.arg(i + 1),
            [&]() { return u"at=%1"_s.arg(wizard->currentId()); })) {
      return false;
    }
  }
  return true;
}

auto GridRowRect(QWidget* box, QWidget* value) -> std::optional<QRect> {
  auto* grid = qobject_cast<QGridLayout*>(box->layout());
  const auto index = grid != nullptr ? grid->indexOf(value) : -1;
  if (index < 0) return std::nullopt;
  int row = 0;
  int column = 0;
  int row_span = 0;
  int column_span = 0;
  grid->getItemPosition(index, &row, &column, &row_span, &column_span);
  QRect rect;
  for (int c = 0; c < grid->columnCount(); ++c) {
    if (auto* item = grid->itemAtPosition(row, c); item != nullptr) {
      rect = rect.united(item->geometry());
    }
  }
  return rect;
}

}  // namespace GpgFrontend::Screenshot
