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

#include <QAbstractItemModel>
#include <QList>
#include <QRect>
#include <QSize>
#include <QString>
#include <QTableView>
#include <functional>
#include <optional>
#include <utility>

#include "screenshot/ScreenshotScenario.h"

class QMenu;
class QPlainTextEdit;

namespace GpgFrontend::UI {
class MainWindow;
}  // namespace GpgFrontend::UI

/**
 * @file ScenarioHelpers.h
 * @brief What several scenario files share: the demo keys they look for and
 *        the main window every scenario starts from.
 */

namespace GpgFrontend::Screenshot {

/// The keys the scenarios work with. The main window's default tab hides
/// expired and revoked keys, so Carol and Dave are never listed there.
inline constexpr auto kAliceEmail = "alice@example.com";
inline constexpr auto kBobEmail = "bob@example.com";
inline constexpr auto kCarolEmail = "carol@example.com";
/// Window size behind dialog-only captures, which never shows.
inline constexpr QSize kBackdropSize(1100, 680);
inline constexpr auto kKeyMgmtClass = "GpgFrontend::UI::KeyMgmt";
inline constexpr auto kKeyDetailsClass = "GpgFrontend::UI::KeyDetailsDialog";

/// The Key Details tabs, by the class of the page each shows.
inline constexpr auto kKeyPairTab = "GpgFrontend::UI::KeyPairDetailTab";
inline constexpr auto kUIDTab = "GpgFrontend::UI::KeyPairUIDTab";
inline constexpr auto kKeychainTab = "GpgFrontend::UI::KeyPairSubkeyTab";
inline constexpr auto kOperationsTab = "GpgFrontend::UI::KeyPairOperaTab";

auto ContractText() -> QString;

/// The main window's key table, on its default tab.
auto KeyTable(UI::MainWindow* window) -> QTableView*;

/// The first row any of whose cells contains @p needle, or -1.
auto RowOf(QTableView* table, const QString& needle) -> int;

/// Hooks that fire whenever @p model's rows change.
auto ModelHooks(QAbstractItemModel* model) -> QList<ScenarioContext::Hook>;

auto CurrentEditor(UI::MainWindow* window) -> QPlainTextEdit*;

/**
 * @brief Create, show and activate the main window, wait for the demo keys
 *        to be listed, and open a text tab holding @p tab_content.
 */
auto OpenMainWindow(ScenarioContext& ctx, QSize size, const QString& tab_title,
                    const QString& tab_content) -> UI::MainWindow*;

/**
 * @brief The menu that holds @p action, among @p actions' own menus, and
 *        the entry that opens it.
 */
auto MenuHolding(const QList<QAction*>& actions, const QAction* action)
    -> std::pair<QMenu*, QAction*>;

/**
 * @brief The visible top-level widget of class @p class_name, or null.
 */
auto VisibleTopLevel(const char* class_name) -> QWidget*;

/**
 * @brief Run @p trigger and wait for a top-level @p class_name to show.
 * @return it, or null after recording @p label as the failure
 */
auto WaitForTopLevel(ScenarioContext& ctx, const std::function<void()>& trigger,
                     const char* class_name, const QString& label) -> QWidget*;

/**
 * @brief Select the row of @p email in @p table, as a click would.
 */
auto SelectKeyRow(ScenarioContext& ctx, QTableView* table, const char* email)
    -> bool;

/**
 * @brief Tick the check box of @p email's row in @p table.
 */
auto CheckKeyRow(ScenarioContext& ctx, QTableView* table, const char* email)
    -> bool;

/**
 * @brief @p column of @p table, header and rows, in the table's coordinates.
 */
auto ColumnRect(QTableView* table, int column) -> QRect;

/**
 * @brief The KeyPair Management window at @p size, laid out, in front of
 *        the main window.
 */
auto OpenKeyMgmt(ScenarioContext& ctx, QSize size) -> QWidget*;

/**
 * @brief Show the KeyPair Management tab @p category_id.
 */
auto ShowCategory(ScenarioContext& ctx, QWidget* mgmt,
                  const QString& category_id) -> QTableView*;

/**
 * @brief Open Settings from @p window and show the page of class
 *        @p page_class, as a click on its navigation row does. The dialog is
 *        resized to the scenario's size when it has one.
 * @return the dialog and the page, or nulls after recording the failure
 */
auto OpenSettingsPage(ScenarioContext& ctx, UI::MainWindow* window,
                      const char* page_class) -> std::pair<QWidget*, QWidget*>;

/**
 * @brief The toolbar button that runs @p action_name: its own, or the
 *        button of the family it was grouped into.
 */
auto ToolBarButtonFor(UI::MainWindow* window, const QString& action_name)
    -> QWidget*;

/**
 * @brief Offer @p opera on the main window's toolbar, before the window
 *        reads its settings.
 */
void OfferToolBarOperation(int opera);

/**
 * @brief Show @p tab_class's page of the Key Details @p dialog.
 */
auto ShowTab(ScenarioContext& ctx, QWidget* dialog, const char* tab_class)
    -> QWidget*;

/**
 * @brief Switch the Key ToolBox to the key database named @p name and wait
 *        for @p listed_email to be listed in it.
 */
auto SwitchKeyDatabase(ScenarioContext& ctx, UI::MainWindow* window,
                       const QString& name, const QString& listed_email)
    -> bool;

/**
 * @brief The main window, behind the Key Details of @p email's key, in
 *        @p database (by name) or the one the window starts on.
 */
auto OpenKeyDetails(ScenarioContext& ctx, const char* email,
                    const QString& database = {}) -> QWidget*;

/**
 * @brief The whole row of @p value in @p box's grid: its title and itself,
 *        in @p box's coordinates.
 */
auto GridRowRect(QWidget* box, QWidget* value) -> std::optional<QRect>;

}  // namespace GpgFrontend::Screenshot
