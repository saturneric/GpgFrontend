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
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTimer>

#include "screenshot/ScreenshotEnv.h"
#include "screenshot/ScreenshotScenario.h"
#include "screenshot/scenarios/ScenarioHelpers.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::Screenshot {

namespace {

using UI::MainWindow;

/// The key table model's Trust column; its column order is fixed.
constexpr int kTrustColumn = 5;

auto KeyPairTabScenario(ScenarioContext& ctx) -> bool {
  auto* dialog = OpenKeyDetails(ctx, kAliceEmail);
  if (dialog == nullptr || ShowTab(ctx, dialog, kKeyPairTab) == nullptr) {
    return false;
  }

  auto* key_box = dialog->findChild<QGroupBox*>(u"primaryKeyBox"_s);
  auto* algorithm_detail =
      dialog->findChild<QWidget*>(u"algorithmDetailValue"_s);
  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  for (const auto* name : {"ownerBox", "primaryKeyBox", "fingerprintBox"}) {
    ctx.AddHighlight(QString::fromLatin1(name),
                     dialog->findChild<QGroupBox*>(QString::fromLatin1(name)));
  }
  if (key_box != nullptr && algorithm_detail != nullptr) {
    ctx.AddHighlight(u"algorithmDetail"_s, key_box,
                     GridRowRect(key_box, algorithm_detail));
  }
  return ctx.WaitSettled(
      dialog, [&]() { return dialog->isActiveWindow(); },
      u"key-details-keypair-settled"_s);
}

auto UIDTabScenario(ScenarioContext& ctx) -> bool {
  auto* dialog = OpenKeyDetails(ctx, kAliceEmail);
  auto* page = dialog != nullptr ? ShowTab(ctx, dialog, kUIDTab) : nullptr;
  if (page == nullptr) return false;

  auto* signatures = page->findChild<QGroupBox*>(u"uidSignatureBox"_s);
  auto* signature_list =
      signatures != nullptr ? signatures->findChild<QTableWidget*>() : nullptr;
  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  ctx.AddHighlight(u"uidSignatureBox"_s, signatures);
  return ctx.WaitSettled(
      dialog,
      [&]() {
        return dialog->isActiveWindow() && signature_list != nullptr &&
               signature_list->rowCount() > 1;
      },
      u"key-details-uids-settled"_s);
}

auto KeychainTabScenario(ScenarioContext& ctx) -> bool {
  auto* dialog = OpenKeyDetails(ctx, kAliceEmail);
  if (dialog == nullptr || ShowTab(ctx, dialog, kKeychainTab) == nullptr) {
    return false;
  }
  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  ctx.AddHighlight(u"newSubkeyButton"_s,
                   dialog->findChild<QWidget*>(u"newSubkeyButton"_s));
  return ctx.WaitSettled(
      dialog, [&]() { return dialog->isActiveWindow(); },
      u"key-details-keychain-settled"_s);
}

auto OperationsTab(ScenarioContext& ctx, const char* email) -> bool {
  auto* dialog = OpenKeyDetails(ctx, email);
  auto* page =
      dialog != nullptr ? ShowTab(ctx, dialog, kOperationsTab) : nullptr;
  if (page == nullptr) return false;

  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  for (const auto* name : {"revokeCertificateButton", "exportPublicKeyButton",
                           "exportPrivateKeyButton"}) {
    ctx.AddHighlight(QString::fromLatin1(name),
                     page->findChild<QPushButton*>(QString::fromLatin1(name)));
  }
  return ctx.WaitSettled(
      dialog, [&]() { return dialog->isActiveWindow(); },
      u"key-details-operations-settled"_s);
}

auto OperationsTabScenario(ScenarioContext& ctx) -> bool {
  return OperationsTab(ctx, kAliceEmail);
}

auto PublicOperationsTabScenario(ScenarioContext& ctx) -> bool {
  return OperationsTab(ctx, kBobEmail);
}

/**
 * @brief Revocation Options, reached from Alice's Operations tab.
 */
auto RevocationOptionsScenario(ScenarioContext& ctx) -> bool {
  auto* details = OpenKeyDetails(ctx, kAliceEmail);
  auto* page =
      details != nullptr ? ShowTab(ctx, details, kOperationsTab) : nullptr;
  if (page == nullptr) return false;

  auto* button = page->findChild<QPushButton*>(u"revokeCertificateButton"_s);
  auto* menu = button != nullptr ? button->menu() : nullptr;
  // Import comes first, Generate second; the entries carry no names.
  if (menu == nullptr || menu->actions().size() < 2) {
    ctx.Fail(kEXIT_USAGE, u"revoke-menu-missing"_s);
    return false;
  }
  auto* generate = menu->actions().at(1);

  auto* dialog = WaitForTopLevel(
      ctx, [&]() { generate->trigger(); },
      "GpgFrontend::UI::RevocationOptionsDialog",
      u"revocation-options-shown"_s);
  if (dialog == nullptr) return false;

  auto* code = dialog->findChild<QComboBox*>(u"rRcodeComboBox"_s);
  auto* text = dialog->findChild<QPlainTextEdit*>(u"rRPlainTextEdit"_s);
  if (code == nullptr || text == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"revocation-options-fields-missing"_s);
    return false;
  }
  if (ctx.GetSize().isValid()) dialog->resize(ctx.GetSize());
  dialog->raise();
  code->setCurrentIndex(1);
  text->setPlainText(u"Stop using this key."_s);

  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  ctx.AddHighlight(u"rRcodeComboBox"_s, code);
  // Asked again until it holds: a request made before the window is mapped
  // is lost where no window manager activates new windows itself.
  if (!ctx.WaitUntil(
          {},
          [&]() {
            if (!dialog->isActiveWindow()) dialog->activateWindow();
            return dialog->isActiveWindow();
          },
          {}, u"revocation-options-active"_s)) {
    return false;
  }
  return ctx.WaitSettled(
      dialog, [&]() { return code->currentIndex() == 1; },
      u"revocation-options-settled"_s);
}

/**
 * @brief Carol's revoked key, opened from KeyPair Management's Revoked tab:
 *        the main window hides revoked keys.
 */
auto RevokedKeyScenario(ScenarioContext& ctx) -> bool {
  auto* mgmt = OpenKeyMgmt(ctx, kBackdropSize);
  auto* table =
      mgmt != nullptr ? ShowCategory(ctx, mgmt, u"revoked"_s) : nullptr;
  if (table == nullptr || !SelectKeyRow(ctx, table, kCarolEmail)) return false;

  auto* dialog = WaitForTopLevel(
      ctx, [&]() { QMetaObject::invokeMethod(mgmt, "SlotShowKeyDetails"); },
      kKeyDetailsClass, u"key-details-shown"_s);
  if (dialog == nullptr) return false;

  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  return ctx.WaitSettled(
      dialog, [&]() { return dialog->isActiveWindow(); },
      u"key-details-revoked-settled"_s);
}

/**
 * @brief KeyPair Management on its All tab, where the Trust column shows.
 */
auto KeyMgmtScenario(ScenarioContext& ctx) -> bool {
  auto* mgmt = OpenKeyMgmt(ctx, ctx.GetSize());
  auto* table = mgmt != nullptr ? ShowCategory(ctx, mgmt, u"all"_s) : nullptr;
  if (table == nullptr) return false;

  ctx.SetTarget(mgmt, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"trustColumn"_s, table, ColumnRect(table, kTrustColumn));
  for (const auto* name :
       {"KeyCategoryList", "importKeyButton", "exportKeyButton"}) {
    ctx.AddHighlight(QString::fromLatin1(name),
                     mgmt->findChild<QWidget*>(QString::fromLatin1(name)));
  }
  return ctx.WaitSettled(
      mgmt, [&]() { return mgmt->isActiveWindow(); },
      u"key-management-settled"_s);
}

/**
 * @brief The key list's context menu on Alice, at Set Owner Trust Level.
 */
auto KeyContextMenuScenario(ScenarioContext& ctx) -> bool {
  auto* window =
      OpenMainWindow(ctx, ctx.GetSize(), u"contract.txt"_s, ContractText());
  if (window == nullptr) return false;

  auto* table = KeyTable(window);
  auto* trust = window->findChild<QAction*>(u"set_owner_trust_level"_s);
  QMenu* menu = nullptr;
  for (auto* candidate : window->findChildren<QMenu*>()) {
    if (trust != nullptr && candidate->actions().contains(trust)) {
      menu = candidate;
    }
  }
  if (menu == nullptr || !SelectKeyRow(ctx, table, kAliceEmail)) {
    if (menu == nullptr) ctx.Fail(kEXIT_USAGE, u"key-context-menu-missing"_s);
    return false;
  }

  const auto row = RowOf(table, QString::fromLatin1(kAliceEmail));
  const auto cell = table->visualRect(table->model()->index(row, 2));
  if (!ctx.WaitUntil(
          [&]() { menu->popup(table->viewport()->mapToGlobal(cell.center())); },
          [&]() { return menu->isVisible(); },
          {ScenarioContext::On(menu, &QMenu::aboutToShow)},
          u"key-context-menu-shown"_s)) {
    return false;
  }
  menu->setActiveAction(trust);

  ctx.SetTarget(window, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(menu);
  ctx.AddHighlight(u"setOwnerTrustLevel"_s, menu, menu->actionGeometry(trust));
  return ctx.WaitSettled(
      window, [&]() { return menu->activeAction() == trust; },
      u"key-context-menu-settled"_s);
}

}  // namespace

void RegisterKeyDetailsScenarios(QList<Scenario>& scenarios) {
  const QStringList details{u"GpgFrontend::UI::MainWindow"_s,
                            QString::fromLatin1(kKeyDetailsClass)};
  const QStringList revocation{u"GpgFrontend::UI::MainWindow"_s,
                               QString::fromLatin1(kKeyDetailsClass),
                               u"GpgFrontend::UI::RevocationOptionsDialog"_s};
  const QStringList mgmt{u"GpgFrontend::UI::MainWindow"_s,
                         QString::fromLatin1(kKeyMgmtClass)};
  const QStringList mgmt_details{u"GpgFrontend::UI::MainWindow"_s,
                                 QString::fromLatin1(kKeyMgmtClass),
                                 QString::fromLatin1(kKeyDetailsClass)};

  scenarios.append({u"key-details-keypair"_s, QSize(), CaptureMode::kDIALOG,
                    details, false, true, KeyPairTabScenario});
  scenarios.append({u"key-details-uids"_s, QSize(), CaptureMode::kDIALOG,
                    details, false, true, UIDTabScenario});
  scenarios.append({u"key-details-keychain"_s, QSize(), CaptureMode::kDIALOG,
                    details, false, true, KeychainTabScenario});
  scenarios.append({u"key-details-operations"_s, QSize(), CaptureMode::kDIALOG,
                    details, false, true, OperationsTabScenario});
  scenarios.append({u"key-details-operations-public"_s, QSize(),
                    CaptureMode::kDIALOG, details, false, true,
                    PublicOperationsTabScenario});
  scenarios.append({u"revocation-options-dialog"_s, QSize(),
                    CaptureMode::kDIALOG, revocation, false, true,
                    RevocationOptionsScenario});
  scenarios.append({u"key-details-revoked"_s, QSize(), CaptureMode::kDIALOG,
                    mgmt_details, false, true, RevokedKeyScenario});
  scenarios.append({u"key-management-window"_s, QSize(1200, 760),
                    CaptureMode::kWINDOW, mgmt, false, true, KeyMgmtScenario});
  scenarios.append({u"key-context-menu"_s,
                    QSize(1280, 960),
                    CaptureMode::kCOMPOSITE,
                    {u"GpgFrontend::UI::MainWindow"_s},
                    false,
                    true,
                    KeyContextMenuScenario});
}

}  // namespace GpgFrontend::Screenshot
