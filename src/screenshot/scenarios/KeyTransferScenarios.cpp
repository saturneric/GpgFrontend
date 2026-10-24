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
#include <QComboBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QNetworkProxy>
#include <QPushButton>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QToolButton>

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleSettingsPolicy.h"
#include "screenshot/ScreenshotEnv.h"
#include "screenshot/ScreenshotScenario.h"
#include "screenshot/scenarios/ScenarioHelpers.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::Screenshot {

namespace {

using UI::MainWindow;

/// The key database holding Alice's subkeys without her primary key.
const auto kWorkDatabase = u"Work"_s;
/// Listed in the Work database only: a sign the switch has happened.
const auto kWorkOnlyEmail = u"grace@example.org"_s;
/// Where render.py's mock key server listens, as host:port.
constexpr auto kKeyServerProxyEnv = "GF_SCREENSHOT_KEYSERVER_PROXY";
/// The key server the demo is shown using; only the mock answers for it.
const auto kDemoKeyServer = u"http://keys.example.org"_s;
constexpr auto kKeyServerModule =
    "com.bktus.gpgfrontend.module.key_server_sync";
constexpr auto kNativeDialogClass = "GpgFrontend::UI::Lua::NativeDialog";

/**
 * @brief Send key server traffic to render.py's mock and make its demo
 *        server the only one the key server module knows.
 */
auto UseDemoKeyServer(ScenarioContext& ctx) -> bool {
  const auto proxy = qEnvironmentVariable(kKeyServerProxyEnv).split(u':');
  bool port_ok = false;
  const auto port = proxy.size() == 2 ? proxy[1].toUShort(&port_ok) : 0;
  if (!port_ok) {
    ctx.Fail(kEXIT_USAGE, u"keyserver-proxy-missing"_s,
             QString::fromLatin1(kKeyServerProxyEnv));
    return false;
  }
  QNetworkProxy::setApplicationProxy(
      QNetworkProxy(QNetworkProxy::HttpProxy, proxy[0], port));

  const auto group =
      Module::ModuleSettingsGroup(QString::fromLatin1(kKeyServerModule));
  const QJsonArray servers{QJsonObject{
      {"url", kDemoKeyServer},
      {"hkp", true},
      {"vks", false},
      {"state", u"verified"_s},
      {"last_tested", ctx.GetOptions().clock.toString(Qt::ISODate)},
      {"detail", QString()},
  }};
  auto settings = GetSettings();
  settings.setValue(group + u"/schema_version"_s, 1);
  settings.setValue(
      group + u"/servers"_s,
      QString::fromUtf8(QJsonDocument(servers).toJson(QJsonDocument::Compact)));
  settings.setValue(group + u"/default_server"_s, kDemoKeyServer);
  settings.sync();
  return true;
}

/// The visible module dialog frame holding a widget named @p name. Only the
/// frame: the widget can sit in another window before it is moved there.
auto FrameHolding(const QString& name) -> QWidget* {
  for (auto* w : QApplication::topLevelWidgets()) {
    if (w->isVisible() && w->inherits(kNativeDialogClass) &&
        w->findChild<QWidget*>(name) != nullptr) {
      return w;
    }
  }
  return nullptr;
}

/**
 * @brief Open a tool button's menu and make @p entry the active one.
 */
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

// --- Subkeys ---------------------------------------------------------------

/**
 * @brief Generate New Subkey, from Alice's Keychain tab.
 */
auto SubkeyGenerateScenario(ScenarioContext& ctx) -> bool {
  auto* details = OpenKeyDetails(ctx, kAliceEmail);
  auto* page =
      details != nullptr ? ShowTab(ctx, details, kKeychainTab) : nullptr;
  auto* button = page != nullptr
                     ? page->findChild<QPushButton*>(u"newSubkeyButton"_s)
                     : nullptr;
  if (button == nullptr) {
    if (page != nullptr) ctx.Fail(kEXIT_USAGE, u"new-subkey-missing"_s);
    return false;
  }
  auto* dialog = WaitForTopLevel(
      ctx, [&]() { button->click(); }, "GpgFrontend::UI::SubkeyGenerateDialog",
      u"subkey-generate-shown"_s);
  if (dialog == nullptr) return false;
  // An algorithm picked, as the reader is about to: the dialog opens on none.
  auto* algo = dialog->findChild<QComboBox*>(u"algoComboBox"_s);
  const auto picked =
      algo != nullptr ? algo->findText(u"25519"_s, Qt::MatchContains) : -1;
  if (picked < 0) {
    ctx.Fail(kEXIT_USAGE, u"subkey-algorithm-missing"_s);
    return false;
  }
  algo->setCurrentIndex(picked);
  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  return SettleActive(ctx, dialog, u"subkey-generate-settled"_s);
}

/// The Keychain tab with Alice's first subkey selected.
auto KeychainSubkey(ScenarioContext& ctx, const QString& database) -> bool {
  auto* details = OpenKeyDetails(ctx, kAliceEmail, database);
  auto* page =
      details != nullptr ? ShowTab(ctx, details, kKeychainTab) : nullptr;
  auto* list = page != nullptr ? page->findChild<QTableWidget*>() : nullptr;
  if (list == nullptr || list->rowCount() < 2) {
    if (page != nullptr) ctx.Fail(kEXIT_USAGE, u"subkey-list-missing"_s);
    return false;
  }
  list->setFocus();
  list->selectRow(1);

  ctx.SetTarget(details, CaptureMode::kDIALOG);
  ctx.AddHighlight(u"exportSubkeyButton"_s,
                   page->findChild<QPushButton*>(u"exportSubkeyButton"_s));
  return ctx.WaitSettled(
      details,
      [&]() {
        return details->isActiveWindow() &&
               list->selectionModel()->isRowSelected(1, {});
      },
      u"keychain-subkey-settled"_s);
}

auto KeychainSubkeyScenario(ScenarioContext& ctx) -> bool {
  return KeychainSubkey(ctx, {});
}

auto WorkKeychainSubkeyScenario(ScenarioContext& ctx) -> bool {
  return KeychainSubkey(ctx, kWorkDatabase);
}

/**
 * @brief Alice's KeyPair tab in the Work database: no primary key there.
 */
auto WorkKeyPairScenario(ScenarioContext& ctx) -> bool {
  auto* details = OpenKeyDetails(ctx, kAliceEmail, kWorkDatabase);
  auto* page =
      details != nullptr ? ShowTab(ctx, details, kKeyPairTab) : nullptr;
  if (page == nullptr) return false;
  auto* key_box = page->findChild<QWidget*>(u"primaryKeyBox"_s);
  auto* existence = page->findChild<QWidget*>(u"primaryKeyExistenceValue"_s);
  ctx.SetTarget(details, CaptureMode::kDIALOG);
  if (key_box != nullptr && existence != nullptr) {
    ctx.AddHighlight(u"primaryKeyExistence"_s, key_box,
                     GridRowRect(key_box, existence));
  }
  return SettleActive(ctx, details, u"work-keypair-settled"_s);
}

/// The main window on the Work database.
auto OpenWorkWindow(ScenarioContext& ctx) -> MainWindow* {
  auto* window =
      OpenMainWindow(ctx, ctx.GetSize(), u"note.txt"_s, ContractText());
  if (window == nullptr ||
      !SwitchKeyDatabase(ctx, window, kWorkDatabase, kWorkOnlyEmail)) {
    return nullptr;
  }
  return window;
}

/**
 * @brief The Work database's key list, Alice's "pub/sec#" pointed at.
 */
auto WorkKeyListScenario(ScenarioContext& ctx) -> bool {
  auto* window = OpenWorkWindow(ctx);
  if (window == nullptr) return false;
  auto* table = KeyTable(window);
  const auto row = RowOf(table, QString::fromLatin1(kAliceEmail));
  ctx.SetTarget(window, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"aliceType"_s, table->viewport(),
                   table->visualRect(table->model()->index(row, 1)));
  return SettleActive(ctx, window, u"work-key-list-settled"_s);
}

/**
 * @brief Ready to encrypt with the subkeys alone: Alice ticked in Work.
 */
auto WorkEncryptScenario(ScenarioContext& ctx) -> bool {
  auto* window = OpenWorkWindow(ctx);
  if (window == nullptr || !CheckKeyRow(ctx, KeyTable(window), kAliceEmail)) {
    return false;
  }
  ctx.SetTarget(window, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"encryptButton"_s, ToolBarButtonFor(window, u"encrypt"_s));
  return SettleActive(ctx, window, u"work-encrypt-settled"_s);
}

/**
 * @brief The toolbar's Import menu at File.
 */
auto ImportMenuScenario(ScenarioContext& ctx) -> bool {
  auto* window =
      OpenMainWindow(ctx, ctx.GetSize(), u"note.txt"_s, ContractText());
  if (window == nullptr) return false;
  auto* button = window->findChild<QToolButton*>(u"importKeyButton"_s);
  auto* entry = window->findChild<QAction*>(u"import_key_from_file"_s);
  if (!ShowButtonMenu(ctx, button, button != nullptr ? button->menu() : nullptr,
                      entry)) {
    return false;
  }
  ctx.SetTarget(window, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(button->menu());
  ctx.AddHighlight(u"importFromFile"_s, button->menu(),
                   button->menu()->actionGeometry(entry));
  return ctx.WaitSettled(
      window, [&]() { return button->menu()->activeAction() == entry; },
      u"import-menu-settled"_s);
}

// --- Key packages ----------------------------------------------------------

/// KeyPair Management, a tool button's menu open at its entry @p index.
auto KeyMgmtMenu(ScenarioContext& ctx, const QString& button_name, int index,
                 const QString& highlight) -> bool {
  auto* mgmt = OpenKeyMgmt(ctx, ctx.GetSize());
  auto* table = mgmt != nullptr ? ShowCategory(ctx, mgmt, u"all"_s) : nullptr;
  if (table == nullptr || !CheckKeyRow(ctx, table, kAliceEmail)) return false;
  auto* button = mgmt->findChild<QToolButton*>(button_name);
  auto* menu = button != nullptr ? button->menu() : nullptr;
  auto* entry = menu != nullptr && menu->actions().size() > index
                    ? menu->actions().at(index)
                    : nullptr;
  if (!ShowButtonMenu(ctx, button, menu, entry)) return false;
  ctx.SetTarget(mgmt, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(menu);
  ctx.AddHighlight(button_name, button);
  ctx.AddHighlight(highlight, menu, menu->actionGeometry(entry));
  return ctx.WaitSettled(
      mgmt, [&]() { return menu->activeAction() == entry; },
      u"key-management-menu-settled"_s);
}

/// Export Key > Export As Key Package, the menu's first entry.
auto KeyPackageExportMenuScenario(ScenarioContext& ctx) -> bool {
  return KeyMgmtMenu(ctx, u"exportKeyButton"_s, 0, u"exportKeyPackage"_s);
}

/// Import Key > Key Package, the menu's third entry.
auto KeyPackageImportMenuScenario(ScenarioContext& ctx) -> bool {
  return KeyMgmtMenu(ctx, u"importKeyButton"_s, 2, u"importKeyPackage"_s);
}

/**
 * @brief Export As Key Package, for Alice.
 */
auto KeyPackageExportScenario(ScenarioContext& ctx) -> bool {
  auto* mgmt = OpenKeyMgmt(ctx, kBackdropSize);
  auto* table = mgmt != nullptr ? ShowCategory(ctx, mgmt, u"all"_s) : nullptr;
  if (table == nullptr || !CheckKeyRow(ctx, table, kAliceEmail)) return false;
  auto* button = mgmt->findChild<QToolButton*>(u"exportKeyButton"_s);
  auto* entry = button != nullptr && button->menu() != nullptr &&
                        !button->menu()->actions().isEmpty()
                    ? button->menu()->actions().front()
                    : nullptr;
  if (entry == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"export-key-package-missing"_s);
    return false;
  }
  auto* dialog = WaitForTopLevel(
      ctx, [&]() { entry->trigger(); },
      "GpgFrontend::UI::ExportKeyPackageDialog", u"key-package-shown"_s);
  if (dialog == nullptr) return false;
  // The dialog proposes a random name; one of the same shape stands in for
  // it, so the image does not change on every run.
  if (auto* name = dialog->findChild<QLabel*>(u"nameValueLabel"_s)) {
    name->setText(u"KeyPackage_q7Hm2Rk9"_s);
  }
  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  return SettleActive(ctx, dialog, u"key-package-settled"_s);
}

// --- Key server ------------------------------------------------------------

/**
 * @brief Keys > Import Key > Key Server, searched for the demo domain.
 * @return the search dialog's window, with results listed
 */
auto SearchDemoKeyServer(ScenarioContext& ctx, QSize backdrop) -> QWidget* {
  if (!UseDemoKeyServer(ctx)) return nullptr;
  auto* window = OpenMainWindow(ctx, backdrop, u"note.txt"_s, ContractText());
  if (window == nullptr) return nullptr;

  // Key Server is the Import Key entry the Host did not add itself.
  auto* button = window->findChild<QToolButton*>(u"importKeyButton"_s);
  const QList<QAction*> host_imports{
      window->findChild<QAction*>(u"import_key_from_file"_s),
      window->findChild<QAction*>(u"import_key_from_edit"_s),
      window->findChild<QAction*>(u"import_key_from_clipboard"_s)};
  QAction* key_server = nullptr;
  for (auto* action : button != nullptr && button->menu() != nullptr
                          ? button->menu()->actions()
                          : QList<QAction*>{}) {
    if (!action->isSeparator() && action->isVisible() &&
        !host_imports.contains(action)) {
      key_server = action;
    }
  }
  if (key_server == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"key-server-action-missing"_s);
    return nullptr;
  }

  QWidget* frame = nullptr;
  if (!ctx.WaitUntil([&]() { key_server->trigger(); },
                     [&]() {
                       frame = FrameHolding(u"SearchKeyDialog"_s);
                       return frame != nullptr;
                     },
                     {}, u"key-server-search-shown"_s)) {
    return nullptr;
  }
  auto* type = frame->findChild<QComboBox*>(u"searchTypeComboBox"_s);
  auto* edit = frame->findChild<QLineEdit*>(u"searchEdit"_s);
  auto* search = frame->findChild<QPushButton*>(u"searchButton"_s);
  auto* results = frame->findChild<QTableWidget*>(u"tableWidget"_s);
  if (type == nullptr || edit == nullptr || search == nullptr ||
      results == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"key-server-search-fields-missing"_s);
    return nullptr;
  }
  type->setCurrentIndex(type->findData(u"email"_s));
  edit->setText(u"grace@example.org"_s);
  if (!ctx.WaitUntil(
          [&]() { search->click(); },
          [&]() { return results->rowCount() >= 1; },
          {ScenarioContext::On(results->model(),
                               &QAbstractItemModel::rowsInserted)},
          u"key-server-results-listed"_s,
          [&]() {
            auto* error = frame->findChild<QLabel*>(u"errorLabel"_s);
            auto* server = frame->findChild<QComboBox*>(u"keyServerComboBox"_s);
            return u"server=%1 error=%2 rows=%3"_s.arg(
                server != nullptr ? server->currentText() : QString(),
                error != nullptr ? error->text() : QString(),
                QString::number(results->rowCount()));
          })) {
    return nullptr;
  }
  return frame;
}

auto KeyServerSearchScenario(ScenarioContext& ctx) -> bool {
  auto* frame = SearchDemoKeyServer(ctx, kBackdropSize);
  if (frame == nullptr) return false;
  if (ctx.GetSize().isValid()) frame->resize(ctx.GetSize());
  auto* results = frame->findChild<QTableWidget*>(u"tableWidget"_s);
  results->setFocus();
  ctx.SetTarget(frame, CaptureMode::kDIALOG);
  ctx.AddHighlight(u"searchResults"_s, results);
  ctx.AddHighlight(u"searchEdit"_s,
                   frame->findChild<QWidget*>(u"searchEdit"_s));
  return SettleActive(ctx, frame, u"key-server-search-settled"_s);
}

/**
 * @brief Key Import Details after a double-click on Grace's result.
 */
auto KeyServerImportScenario(ScenarioContext& ctx) -> bool {
  auto* frame = SearchDemoKeyServer(ctx, kBackdropSize);
  if (frame == nullptr) return false;
  auto* results = frame->findChild<QTableWidget*>(u"tableWidget"_s);
  int grace = -1;
  for (int row = 0; row < results->rowCount() && grace < 0; ++row) {
    for (int col = 0; col < results->columnCount(); ++col) {
      if (auto* item = results->item(row, col);
          item != nullptr && item->text().contains("grace@"_L1)) {
        grace = row;
      }
    }
  }
  if (grace < 0) {
    ctx.Fail(kEXIT_USAGE, u"key-server-result-missing"_s);
    return false;
  }
  auto* dialog = WaitForTopLevel(
      ctx, [&]() { emit results->cellActivated(grace, 0); },
      "GpgFrontend::UI::KeyImportDetailDialog", u"key-import-shown"_s);
  if (dialog == nullptr) return false;
  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  return SettleActive(ctx, dialog, u"key-import-settled"_s);
}

/**
 * @brief Alice's Operations tab, its Key Server Operations menu open.
 */
auto KeyServerOperationsScenario(ScenarioContext& ctx) -> bool {
  auto* details = OpenKeyDetails(ctx, kAliceEmail);
  auto* page =
      details != nullptr ? ShowTab(ctx, details, kOperationsTab) : nullptr;
  if (page == nullptr) return false;
  // The key server module adds its button last, a menu of its operations.
  QPushButton* button = nullptr;
  for (auto* candidate : page->findChildren<QPushButton*>()) {
    if (candidate->menu() != nullptr &&
        candidate->objectName() != "revokeCertificateButton"_L1 &&
        candidate->objectName() != "exportPrivateKeyButton"_L1) {
      button = candidate;
    }
  }
  auto* menu = button != nullptr ? button->menu() : nullptr;
  QAction* publish = nullptr;
  for (auto* action : menu != nullptr ? menu->actions() : QList<QAction*>{}) {
    if (action->property("gf_lua_action").toString().contains("publish"_L1)) {
      publish = action;
    }
  }
  if (!ShowButtonMenu(ctx, button, menu, publish)) return false;
  ctx.SetTarget(details, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(menu);
  ctx.AddHighlight(u"keyServerOperations"_s, button);
  ctx.AddHighlight(u"publishKey"_s, menu, menu->actionGeometry(publish));
  return ctx.WaitSettled(
      details, [&]() { return menu->activeAction() == publish; },
      u"key-server-operations-settled"_s);
}

// --- About -----------------------------------------------------------------

auto AboutBuildInfoScenario(ScenarioContext& ctx) -> bool {
  auto* window =
      OpenMainWindow(ctx, kBackdropSize, u"note.txt"_s, ContractText());
  if (window == nullptr) return false;
  auto* action = window->findChild<QAction*>(u"about"_s);
  if (action == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"about-missing"_s);
    return false;
  }
  auto* dialog = WaitForTopLevel(
      ctx, [&]() { action->trigger(); }, "GpgFrontend::UI::AboutDialog",
      u"about-shown"_s);
  if (dialog == nullptr) return false;
  auto* tabs = dialog->findChild<QTabWidget*>();
  QWidget* build = nullptr;
  for (int i = 0; tabs != nullptr && i < tabs->count(); ++i) {
    if (tabs->widget(i)->inherits("GpgFrontend::UI::BuildInfoTab")) {
      build = tabs->widget(i);
      tabs->setCurrentIndex(i);
    }
  }
  if (build == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"build-info-tab-missing"_s);
    return false;
  }
  if (ctx.GetSize().isValid()) dialog->resize(ctx.GetSize());
  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  return SettleActive(ctx, dialog, u"about-settled"_s);
}

}  // namespace

void RegisterKeyTransferScenarios(QList<Scenario>& scenarios) {
  const QString main_window = u"GpgFrontend::UI::MainWindow"_s;
  const QString details = QString::fromLatin1(kKeyDetailsClass);
  const QString mgmt = QString::fromLatin1(kKeyMgmtClass);
  const QStringList work{u"work"_s};
  const QSize window(1280, 960);

  const auto add = [&scenarios](const QString& name, QSize size,
                                CaptureMode mode, const QStringList& windows,
                                auto setup, const QStringList& databases = {}) {
    Scenario s{name, size, mode, windows, false, true, setup};
    s.key_databases = databases;
    scenarios.append(s);
  };

  add(u"subkey-generate-dialog"_s, QSize(), CaptureMode::kDIALOG,
      {main_window, details, u"GpgFrontend::UI::SubkeyGenerateDialog"_s},
      SubkeyGenerateScenario);
  add(u"keychain-subkey"_s, QSize(), CaptureMode::kDIALOG,
      {main_window, details}, KeychainSubkeyScenario);
  add(u"work-keychain-subkey"_s, QSize(), CaptureMode::kDIALOG,
      {main_window, details}, WorkKeychainSubkeyScenario, work);
  add(u"work-keypair"_s, QSize(), CaptureMode::kDIALOG, {main_window, details},
      WorkKeyPairScenario, work);
  add(u"work-key-list"_s, window, CaptureMode::kWINDOW, {main_window},
      WorkKeyListScenario, work);
  add(u"work-encrypt-ready"_s, window, CaptureMode::kWINDOW, {main_window},
      WorkEncryptScenario, work);
  add(u"import-menu"_s, window, CaptureMode::kCOMPOSITE, {main_window},
      ImportMenuScenario);
  add(u"key-package-export-menu"_s, QSize(1200, 760), CaptureMode::kCOMPOSITE,
      {main_window, mgmt}, KeyPackageExportMenuScenario);
  add(u"key-package-import-menu"_s, QSize(1200, 760), CaptureMode::kCOMPOSITE,
      {main_window, mgmt}, KeyPackageImportMenuScenario);
  add(u"key-package-export-dialog"_s, QSize(), CaptureMode::kDIALOG,
      {main_window, mgmt, u"GpgFrontend::UI::ExportKeyPackageDialog"_s},
      KeyPackageExportScenario);
  add(u"key-server-search"_s, QSize(820, 600), CaptureMode::kDIALOG,
      {main_window, QString::fromLatin1(kNativeDialogClass)},
      KeyServerSearchScenario);
  add(u"key-server-import"_s, QSize(), CaptureMode::kDIALOG,
      {main_window, QString::fromLatin1(kNativeDialogClass),
       u"GpgFrontend::UI::KeyImportDetailDialog"_s,
       u"GpgFrontend::UI::WaitingDialog"_s},
      KeyServerImportScenario);
  add(u"key-server-operations"_s, QSize(), CaptureMode::kCOMPOSITE,
      {main_window, details}, KeyServerOperationsScenario);
  add(u"about-build-info"_s, QSize(), CaptureMode::kDIALOG,
      {main_window, u"GpgFrontend::UI::AboutDialog"_s}, AboutBuildInfoScenario);
}

}  // namespace GpgFrontend::Screenshot
