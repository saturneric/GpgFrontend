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
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTableView>
#include <QTimeZone>
#include <QToolButton>
#include <QTreeView>

#include "core/profile/ProfileSession.h"
#include "core/typedef/GpgTypedef.h"
#include "screenshot/ScreenshotEnv.h"
#include "screenshot/ScreenshotScenario.h"
#include "screenshot/scenarios/ScenarioHelpers.h"
#include "ui/main_window/MainWindow.h"
#include "ui/widgets/InfoBoardWidget.h"

namespace GpgFrontend::Screenshot {

namespace {

using UI::InfoBoardWidget;
using UI::MainWindow;

const auto kMeetingNote =
    u"Hello Bob,\n\nthe meeting moved to Thursday at 10:00.\n\nAlice\n"_s;

/// When the demo files were last written: shown in the file panel.
auto DemoFileTime() -> QDateTime {
  return {QDate(2025, 2, 1), QTime(12, 0), QTimeZone::UTC};
}

/**
 * @brief Text in a fixture file.
 */
auto Fixture(ScenarioContext& ctx, const QString& name) -> QString {
  QFile file(ctx.FixturePath(name));
  if (!file.open(QIODevice::ReadOnly)) {
    ctx.Fail(kEXIT_USAGE, u"fixture-missing"_s, file.fileName());
    return {};
  }
  return QString::fromUtf8(file.readAll());
}

/**
 * @brief Run @p trigger and wait for the Status Panel's result and for the
 *        editor to hold @p expected.
 */
auto WaitForResult(ScenarioContext& ctx, MainWindow* window,
                   const std::function<void()>& trigger,
                   const QString& expected, const QString& label) -> bool {
  auto* board = window->findChild<InfoBoardWidget*>();
  bool posted = false;
  const auto done = ctx.WaitUntil(
      trigger,
      [&]() {
        auto* editor = CurrentEditor(window);
        return posted && (expected.isEmpty() ||
                          (editor != nullptr &&
                           editor->toPlainText().contains(expected)));
      },
      {[&](const std::function<void()>& check) {
        return QObject::connect(board, &InfoBoardWidget::SignalResultPosted,
                                board, [&posted, check]() {
                                  posted = true;
                                  check();
                                });
      }},
      label);
  // The result is what the reader looks at; no caret in the editor.
  if (done) board->setFocus();
  return done;
}

/// A text tab, an operation run on it, and its result.
auto TextResult(ScenarioContext& ctx, const QString& tab_title,
                const QString& text, const char* signer,
                void (MainWindow::*slot)(), const QString& expected) -> bool {
  if (ctx.GetFailure()) return false;
  auto* window = OpenMainWindow(ctx, ctx.GetSize(), tab_title, text);
  if (window == nullptr) return false;
  if (signer != nullptr && !CheckKeyRow(ctx, KeyTable(window), signer)) {
    return false;
  }
  if (!WaitForResult(
          ctx, window, [&]() { (window->*slot)(); }, expected,
          u"text-result-posted"_s)) {
    return false;
  }
  ctx.SetTarget(window, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"InfoBoardWidget"_s, window->findChild<InfoBoardWidget*>());
  ctx.AddHighlight(u"PlainTextEditor"_s, CurrentEditor(window));
  return ctx.WaitSettled(window, {}, u"text-result-settled"_s);
}

auto TextDecryptScenario(ScenarioContext& ctx) -> bool {
  return TextResult(ctx, u"message.asc"_s, Fixture(ctx, u"decrypt.asc"_s),
                    nullptr, &MainWindow::SlotDecrypt, u"the meeting moved"_s);
}

auto TextSignScenario(ScenarioContext& ctx) -> bool {
  return TextResult(ctx, u"note.txt"_s, kMeetingNote, kAliceEmail,
                    &MainWindow::SlotSign, u"BEGIN PGP SIGNATURE"_s);
}

auto TextVerifyScenario(ScenarioContext& ctx) -> bool {
  return TextResult(ctx, u"release-notes.asc"_s, Fixture(ctx, u"verify.asc"_s),
                    nullptr, &MainWindow::SlotVerify, {});
}

/**
 * @brief Ready to encrypt: the text, Bob ticked, Encrypt in reach. The
 *        result is not shown: its ciphertext differs on every run.
 */
auto TextEncryptScenario(ScenarioContext& ctx) -> bool {
  auto* window =
      OpenMainWindow(ctx, ctx.GetSize(), u"note.txt"_s, kMeetingNote);
  if (window == nullptr || !CheckKeyRow(ctx, KeyTable(window), kBobEmail)) {
    return false;
  }
  auto* table = KeyTable(window);
  ctx.SetTarget(window, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"encryptButton"_s, ToolBarButtonFor(window, u"encrypt"_s));
  ctx.AddHighlight(u"checkColumn"_s, table, ColumnRect(table, 0));
  return ctx.WaitSettled(
      window, [&]() { return window->isActiveWindow(); },
      u"text-encrypt-settled"_s);
}

/**
 * @brief The main window with Sym. Encrypt offered and nothing ticked, so
 *        the encrypt button runs it.
 */
auto SymEncryptButtonScenario(ScenarioContext& ctx) -> bool {
  OfferToolBarOperation(kSYMMETRIC_ENCRYPT);
  auto* window =
      OpenMainWindow(ctx, ctx.GetSize(), u"note.txt"_s, kMeetingNote);
  if (window == nullptr) return false;
  ctx.SetTarget(window, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"symEncryptButton"_s,
                   ToolBarButtonFor(window, u"symmetric_encryption"_s));
  return ctx.WaitSettled(
      window, [&]() { return window->isActiveWindow(); },
      u"sym-encrypt-button-settled"_s);
}

/// Settings on @p page_class with the check box @p box_name ticked.
auto SettingsCheckBox(ScenarioContext& ctx, const char* page_class,
                      const QString& box_name) -> bool {
  auto* window =
      OpenMainWindow(ctx, kBackdropSize, u"contract.txt"_s, ContractText());
  if (window == nullptr) return false;
  const auto [dialog, page] = OpenSettingsPage(ctx, window, page_class);
  if (page == nullptr) return false;

  auto* box = page->findChild<QCheckBox*>(box_name);
  if (box == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"settings-option-missing"_s, box_name);
    return false;
  }
  box->setChecked(true);

  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  ctx.AddHighlight(box_name, box);
  return ctx.WaitSettled(
      dialog, [&]() { return dialog->isActiveWindow() && box->isChecked(); },
      u"settings-option-settled"_s);
}

auto SettingsSymEncryptScenario(ScenarioContext& ctx) -> bool {
  return SettingsCheckBox(ctx, "GpgFrontend::UI::AppearanceTab",
                          u"symmetricEncrCheckBox"_s);
}

auto SettingsBinaryModeScenario(ScenarioContext& ctx) -> bool {
  return SettingsCheckBox(ctx, "GpgFrontend::UI::GeneralTab",
                          u"asciiModeCheckBox"_s);
}

/**
 * @brief Put the demo files in the profile's workspace, dated the same on
 *        every run.
 */
auto SeedWorkspace(ScenarioContext& ctx) -> bool {
  const QDir workspace(ProfileSession::Instance().EnsureWorkspace());
  if (!workspace.exists()) {
    ctx.Fail(kEXIT_USAGE, u"workspace-missing"_s);
    return false;
  }
  const QList<std::pair<QString, QString>> files{
      {u"quarterly-report.txt"_s,
       u"Quarterly report\n\nRevenue grew by 12 percent.\n"_s},
      {u"budget-2025.csv"_s, u"item,amount\nhardware,4200\ntravel,1800\n"_s},
      {u"meeting-notes.md"_s, u"# Meeting notes\n\n- Release on Thursday\n"_s},
      {u"message.txt.asc"_s, Fixture(ctx, u"decrypt-verify.asc"_s)},
  };
  for (const auto& [name, text] : files) {
    QFile file(workspace.filePath(name));
    if (!file.open(QIODevice::WriteOnly) || file.write(text.toUtf8()) < 0) {
      ctx.Fail(kEXIT_USAGE, u"workspace-seed-failed"_s, file.fileName());
      return false;
    }
    // Written out first: a flush after setting the time would move it again.
    file.flush();
    file.setFileTime(DemoFileTime(), QFileDevice::FileModificationTime);
  }
  return !ctx.GetFailure();
}

/// The main window on its file panel, the demo files listed.
struct FilePanel {
  MainWindow* window = nullptr;
  QWidget* page = nullptr;
  QTreeView* tree = nullptr;
};

auto OpenFilePanel(ScenarioContext& ctx, QSize size) -> FilePanel {
  if (!SeedWorkspace(ctx)) return {};
  auto* window = OpenMainWindow(ctx, size, u"note.txt"_s, kMeetingNote);
  if (window == nullptr) return {};

  auto* page = window->findChild<QWidget*>(u"FilePage"_s);
  auto* tree =
      page != nullptr ? page->findChild<QTreeView*>(u"treeView"_s) : nullptr;
  if (tree == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"file-panel-missing"_s);
    return {};
  }
  for (auto* tabs : window->findChildren<QTabWidget*>()) {
    if (tabs->indexOf(page) >= 0) tabs->setCurrentWidget(page);
  }
  // The free space of the disk below the workspace describes the machine
  // rendering, not the demo.
  if (auto* capacity = page->findChild<QLabel*>(u"capacityLabel"_s)) {
    capacity->hide();
  }

  // The model lists the folder in the background.
  if (!ctx.WaitUntil(
          {},
          [&]() {
            return page->isVisible() &&
                   tree->model()->rowCount(tree->rootIndex()) >= 4;
          },
          {}, u"file-panel-listed"_s)) {
    return {};
  }
  return {window, page, tree};
}

/// Select the files named @p names in the panel.
auto SelectFiles(ScenarioContext& ctx, const FilePanel& panel,
                 const QStringList& names) -> bool {
  auto* model = panel.tree->model();
  auto* selection = panel.tree->selectionModel();
  selection->clearSelection();
  int found = 0;
  for (int row = 0; row < model->rowCount(panel.tree->rootIndex()); ++row) {
    const auto index = model->index(row, 0, panel.tree->rootIndex());
    if (!names.contains(index.data().toString())) continue;
    selection->select(index,
                      QItemSelectionModel::Select | QItemSelectionModel::Rows);
    selection->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
    ++found;
  }
  if (found != names.size()) {
    ctx.Fail(kEXIT_USAGE, u"demo-file-missing"_s, names.join(u','));
    return false;
  }
  panel.tree->setFocus();
  return true;
}

/// The paths of @p names in the workspace.
auto WorkspacePaths(const QStringList& names) -> QStringList {
  const QDir workspace(ProfileSession::Instance().EnsureWorkspace());
  QStringList paths;
  for (const auto& name : names) paths.append(workspace.filePath(name));
  return paths;
}

/**
 * @brief The file panel with a report selected and Bob ticked: what one
 *        sees before encrypting a file.
 */
auto FilePanelScenario(ScenarioContext& ctx) -> bool {
  const auto panel = OpenFilePanel(ctx, ctx.GetSize());
  if (panel.window == nullptr ||
      !SelectFiles(ctx, panel, {u"quarterly-report.txt"_s}) ||
      !CheckKeyRow(ctx, KeyTable(panel.window), kBobEmail)) {
    return false;
  }
  auto* table = KeyTable(panel.window);
  ctx.SetTarget(panel.window, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"fileTree"_s, panel.tree);
  ctx.AddHighlight(u"checkColumn"_s, table, ColumnRect(table, 0));
  ctx.AddHighlight(u"encryptButton"_s,
                   ToolBarButtonFor(panel.window, u"encrypt"_s));
  ctx.AddHighlight(u"openButton"_s,
                   panel.window->findChild<QWidget*>(u"openButton"_s));
  return ctx.WaitSettled(
      panel.window, [&]() { return panel.window->isActiveWindow(); },
      u"file-panel-settled"_s);
}

/**
 * @brief Batch mode: several files selected, Bob ticked.
 */
auto FileBatchScenario(ScenarioContext& ctx) -> bool {
  const auto panel = OpenFilePanel(ctx, ctx.GetSize());
  if (panel.window == nullptr) return false;
  auto* batch = panel.page->findChild<QAbstractButton*>(u"batchModeButton"_s);
  if (batch == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"batch-mode-missing"_s);
    return false;
  }
  if (!batch->isChecked()) batch->click();
  if (!SelectFiles(ctx, panel,
                   {u"quarterly-report.txt"_s, u"budget-2025.csv"_s,
                    u"meeting-notes.md"_s}) ||
      !CheckKeyRow(ctx, KeyTable(panel.window), kBobEmail)) {
    return false;
  }
  ctx.SetTarget(panel.window, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"batchModeButton"_s, batch);
  ctx.AddHighlight(u"fileTree"_s, panel.tree);
  ctx.AddHighlight(u"encryptButton"_s,
                   ToolBarButtonFor(panel.window, u"encrypt"_s));
  // The file panel's own tool row, for a close-up of it.
  QRect row;
  for (const auto* name : {"hardDiskButton", "upPathButton", "pathBar",
                           "filterEdit", "batchModeButton", "optionsButton"}) {
    if (auto* w = panel.page->findChild<QWidget*>(QString::fromLatin1(name))) {
      row = row.united(QRect(w->mapTo(panel.page, QPoint(0, 0)), w->size()));
    }
  }
  ctx.AddHighlight(u"fileToolRow"_s, panel.page, row.adjusted(-4, -4, 4, 4));
  return ctx.WaitSettled(
      panel.window,
      [&]() { return panel.window->isActiveWindow() && batch->isChecked(); },
      u"file-batch-settled"_s);
}

/**
 * @brief Decrypt & Verify run on a file, with its result.
 */
auto FileDecryptVerifyScenario(ScenarioContext& ctx) -> bool {
  const auto panel = OpenFilePanel(ctx, ctx.GetSize());
  if (panel.window == nullptr ||
      !SelectFiles(ctx, panel, {u"message.txt.asc"_s})) {
    return false;
  }
  if (!WaitForResult(
          ctx, panel.window,
          [&]() {
            panel.window->SlotFileDecryptVerify(
                WorkspacePaths({u"message.txt.asc"_s}));
          },
          {}, u"file-decrypt-verify-posted"_s)) {
    return false;
  }

  // The output is new, so it carries today's date; it gets the demo's.
  QFile output(WorkspacePaths({u"message.txt"_s}).front());
  if (!output.exists() ||
      !output.open(QIODevice::ReadWrite | QIODevice::ExistingOnly) ||
      !output.setFileTime(DemoFileTime(), QFileDevice::FileModificationTime)) {
    ctx.Fail(kEXIT_USAGE, u"file-decrypt-output-missing"_s, output.fileName());
    return false;
  }
  output.close();
  QMetaObject::invokeMethod(panel.page, "SlotRefreshState");
  if (!ctx.WaitUntil(
          {},
          [&]() {
            return panel.tree->model()->rowCount(panel.tree->rootIndex()) >= 5;
          },
          {}, u"file-decrypt-output-listed"_s) ||
      !SelectFiles(ctx, panel, {u"message.txt.asc"_s})) {
    return false;
  }
  panel.window->findChild<InfoBoardWidget*>()->setFocus();

  ctx.SetTarget(panel.window, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"InfoBoardWidget"_s,
                   panel.window->findChild<InfoBoardWidget*>());
  ctx.AddHighlight(u"fileTree"_s, panel.tree);
  return ctx.WaitSettled(panel.window, {}, u"file-decrypt-verify-settled"_s);
}

/**
 * @brief A tool button's menu, open, with @p entry the active one.
 */
auto OpenButtonMenu(ScenarioContext& ctx, QToolButton* button, QAction* entry)
    -> QMenu* {
  auto* menu = button != nullptr ? button->menu() : nullptr;
  if (menu == nullptr || entry == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"tool-button-menu-missing"_s);
    return nullptr;
  }
  // popup() rather than showMenu(): the latter runs a nested event loop.
  if (!ctx.WaitUntil(
          [&]() {
            menu->popup(button->mapToGlobal(QPoint(0, button->height())));
          },
          [&]() { return menu->isVisible(); },
          {ScenarioContext::On(menu, &QMenu::aboutToShow)},
          u"tool-button-menu-shown"_s)) {
    return nullptr;
  }
  menu->setActiveAction(entry);
  return menu;
}

/**
 * @brief The encrypt button's menu at Sym. Encrypt, nothing ticked.
 */
auto SymEncryptMenuScenario(ScenarioContext& ctx) -> bool {
  OfferToolBarOperation(kSYMMETRIC_ENCRYPT);
  auto* window =
      OpenMainWindow(ctx, ctx.GetSize(), u"note.txt"_s, kMeetingNote);
  if (window == nullptr) return false;
  auto* entry = window->findChild<QAction*>(u"symmetric_encryption"_s);
  auto* button = qobject_cast<QToolButton*>(
      ToolBarButtonFor(window, u"symmetric_encryption"_s));
  auto* menu = OpenButtonMenu(ctx, button, entry);
  if (menu == nullptr) return false;

  ctx.SetTarget(window, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(menu);
  ctx.AddHighlight(u"symEncrypt"_s, menu, menu->actionGeometry(entry));
  return ctx.WaitSettled(
      window, [&]() { return menu->activeAction() == entry; },
      u"sym-encrypt-menu-settled"_s);
}

/**
 * @brief Workspace > New File Panel.
 */
auto WorkspaceMenuScenario(ScenarioContext& ctx) -> bool {
  auto* window =
      OpenMainWindow(ctx, ctx.GetSize(), u"note.txt"_s, kMeetingNote);
  if (window == nullptr) return false;
  auto* button = window->findChild<QToolButton*>(u"workspaceButton"_s);
  auto* entry = window->findChild<QAction*>(u"file_browser_dir"_s);
  auto* menu = OpenButtonMenu(ctx, button, entry);
  if (menu == nullptr) return false;

  ctx.SetTarget(window, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(menu);
  ctx.AddHighlight(u"newFilePanel"_s, menu, menu->actionGeometry(entry));
  return ctx.WaitSettled(
      window, [&]() { return menu->activeAction() == entry; },
      u"workspace-menu-settled"_s);
}

/**
 * @brief The file panel's options menu at Use ASCII Armor.
 */
auto FileOptionsMenuScenario(ScenarioContext& ctx) -> bool {
  const auto panel = OpenFilePanel(ctx, ctx.GetSize());
  if (panel.window == nullptr) return false;
  auto* button = panel.page->findChild<QToolButton*>(u"optionsButton"_s);
  QAction* armor = nullptr;
  for (auto* action : button != nullptr && button->menu() != nullptr
                          ? button->menu()->actions()
                          : QList<QAction*>{}) {
    if (action->isCheckable() && action->menu() == nullptr) armor = action;
  }
  auto* menu = OpenButtonMenu(ctx, button, armor);
  if (menu == nullptr) return false;

  ctx.SetTarget(panel.window, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(menu);
  ctx.AddHighlight(u"useAsciiArmor"_s, menu, menu->actionGeometry(armor));
  // The panel's tool row and the menu below it, for a close-up of both.
  const auto origin = panel.window->mapToGlobal(QPoint(0, 0));
  const QRect button_rect(button->mapToGlobal(QPoint(0, 0)) - origin,
                          button->size());
  const QRect menu_rect(menu->mapToGlobal(QPoint(0, 0)) - origin, menu->size());
  const auto around = button_rect.united(menu_rect).adjusted(-360, -12, 16, 16);
  ctx.AddHighlight(u"optionsArea"_s, panel.window, around);
  return ctx.WaitSettled(
      panel.window, [&]() { return menu->activeAction() == armor; },
      u"file-options-menu-settled"_s);
}

}  // namespace

void RegisterOperationScenarios(QList<Scenario>& scenarios) {
  const QStringList main_window{u"GpgFrontend::UI::MainWindow"_s};
  const QStringList with_waiting{u"GpgFrontend::UI::MainWindow"_s,
                                 u"GpgFrontend::UI::WaitingDialog"_s};
  const QStringList settings{u"GpgFrontend::UI::MainWindow"_s,
                             u"GpgFrontend::UI::SettingsDialog"_s};
  const QSize window(1280, 960);

  scenarios.append({u"text-encrypt-ready"_s, window, CaptureMode::kWINDOW,
                    main_window, false, true, TextEncryptScenario});
  scenarios.append({u"text-decrypt-result"_s, window, CaptureMode::kWINDOW,
                    with_waiting, false, true, TextDecryptScenario});
  scenarios.append({u"text-sign-result"_s, window, CaptureMode::kWINDOW,
                    with_waiting, false, true, TextSignScenario});
  scenarios.append({u"text-verify-result"_s, window, CaptureMode::kWINDOW,
                    with_waiting, false, true, TextVerifyScenario});
  scenarios.append({u"sym-encrypt-button"_s, window, CaptureMode::kWINDOW,
                    main_window, false, true, SymEncryptButtonScenario});
  scenarios.append({u"settings-sym-encrypt"_s, QSize(900, 660),
                    CaptureMode::kDIALOG, settings, false, true,
                    SettingsSymEncryptScenario});
  scenarios.append({u"settings-binary-mode"_s, QSize(900, 660),
                    CaptureMode::kDIALOG, settings, false, true,
                    SettingsBinaryModeScenario});
  scenarios.append({u"file-panel"_s, window, CaptureMode::kWINDOW, main_window,
                    false, true, FilePanelScenario});
  scenarios.append({u"file-batch"_s, window, CaptureMode::kWINDOW, main_window,
                    false, true, FileBatchScenario});
  scenarios.append({u"file-decrypt-verify-result"_s, window,
                    CaptureMode::kWINDOW, with_waiting, false, true,
                    FileDecryptVerifyScenario});
  scenarios.append({u"sym-encrypt-menu"_s, window, CaptureMode::kCOMPOSITE,
                    main_window, false, true, SymEncryptMenuScenario});
  scenarios.append({u"workspace-menu"_s, window, CaptureMode::kCOMPOSITE,
                    main_window, false, true, WorkspaceMenuScenario});
  scenarios.append({u"file-options-menu"_s, window, CaptureMode::kCOMPOSITE,
                    main_window, false, true, FileOptionsMenuScenario});
}

}  // namespace GpgFrontend::Screenshot
