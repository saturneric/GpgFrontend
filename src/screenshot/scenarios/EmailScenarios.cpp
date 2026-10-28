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
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QToolButton>

#include "core/function/GlobalSettingStation.h"
#include "core/model/GFBuffer.h"
#include "core/module/ModuleSettingsPolicy.h"
#include "screenshot/ScreenshotEnv.h"
#include "screenshot/ScreenshotScenario.h"
#include "screenshot/scenarios/ScenarioHelpers.h"
#include "ui/main_window/MainWindow.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::Screenshot {

namespace {

using UI::MainWindow;

constexpr auto kEmailModule = "com.bktus.gpgfrontend.module.email";
/// The email module's views: not exported, so only reached by class name.
constexpr auto kPageViewClass = "EMailPageView";
/// What the module's Message Details window is, by class.
constexpr auto kDetailsDialogClass = "QDialog";

auto Account(const QString& id, bool enabled, const QString& name,
             const QString& address, const QString& domain) -> QJsonObject {
  const auto transport = [&](const QString& host) {
    return QJsonObject{{"enabled", true},
                       {"host", host + u"."_s + domain},
                       {"tls", u"implicit"_s},
                       {"username", address},
                       {"pinned_cert_sha256", QString()}};
  };
  return QJsonObject{{"id", id},
                     {"enabled", enabled},
                     {"display_name", name},
                     {"address", address},
                     {"reply_to", QString()},
                     {"default_key_fpr", QString()},
                     {"imap", transport(u"imap"_s)},
                     {"smtp", transport(u"smtp"_s)},
                     {"sent_folder_override", QString()}};
}

/**
 * @brief Give the email module Alice's account and a disabled old one, as
 *        Add Account would have stored them. Fixed ids, so nothing in the
 *        image changes between runs; no password, since nothing connects.
 */
void SeedMailAccounts() {
  const auto group =
      Module::ModuleSettingsGroup(QString::fromLatin1(kEmailModule));
  const QJsonArray accounts{
      Account(u"6f1c2a52-0b5e-4c2e-9d6a-1a2b3c4d5e01"_s, true,
              u"Alice Example"_s, u"alice@example.com"_s, u"example.com"_s),
      Account(u"6f1c2a52-0b5e-4c2e-9d6a-1a2b3c4d5e02"_s, false,
              u"Alice Example"_s, u"alice@old.example.net"_s,
              u"old.example.net"_s),
  };
  auto settings = GetSettings();
  settings.setValue(group + u"/schema_version"_s, 1);
  settings.setValue(group + u"/accounts"_s,
                    QString::fromUtf8(QJsonDocument(accounts).toJson(
                        QJsonDocument::Compact)));
  settings.setValue(group + u"/default_account"_s,
                    accounts.first().toObject().value("id").toString());
  settings.sync();
}

/**
 * @brief The main window with the fixture @p name open in an email tab.
 * @return the window and the tab's view, or nulls after the failure
 */
auto OpenEmail(ScenarioContext& ctx, const QString& name, bool saved)
    -> std::pair<MainWindow*, QWidget*> {
  QFile file(ctx.FixturePath(name));
  if (!file.open(QIODevice::ReadOnly)) {
    ctx.Fail(kEXIT_USAGE, u"fixture-missing"_s, file.fileName());
    return {};
  }
  // Behind a dialog-only capture, the backdrop size.
  const auto size = ctx.GetSize().isValid() ? ctx.GetSize() : kBackdropSize;
  auto* window = OpenMainWindow(ctx, size, u"note.txt"_s, ContractText());
  auto* edit = window != nullptr ? window->findChild<UI::TextEdit*>() : nullptr;
  if (edit == nullptr) return {};
  edit->OpenDocument(u"email"_s, name, saved ? file.fileName() : QString(),
                     GFBuffer(file.readAll()), saved);

  QWidget* view = nullptr;
  if (!ctx.WaitUntil(
          {},
          [&]() {
            for (auto* w : window->findChildren<QWidget*>()) {
              if (w->isVisible() && w->inherits(kPageViewClass)) view = w;
            }
            return view != nullptr;
          },
          {}, u"email-view-shown"_s)) {
    return {};
  }
  return {window, view};
}

/// The security badge of @p view's message, and the menu behind it.
auto SecurityBadge(ScenarioContext& ctx, QWidget* view) -> QToolButton* {
  auto* badge = view->findChild<QToolButton*>(u"emailSecurityButton"_s);
  if (badge == nullptr || badge->menu() == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"email-security-badge-missing"_s);
    return nullptr;
  }
  if (!ctx.WaitUntil(
          {}, [&]() { return badge->isVisible(); }, {},
          u"email-security-badge-shown"_s)) {
    return nullptr;
  }
  return badge;
}

/// The badge menu's entry at @p index, built fresh for the current state.
auto BadgeEntry(QToolButton* badge, int index) -> QAction* {
  QList<QAction*> entries;
  for (auto* action : badge->menu()->actions()) {
    if (!action->isSeparator()) entries.append(action);
  }
  return index < entries.size() ? entries.at(index) : nullptr;
}

/**
 * @brief The signed message opened and its signature checked.
 */
auto OpenVerified(ScenarioContext& ctx)
    -> std::tuple<MainWindow*, QWidget*, QToolButton*> {
  auto [window, view] = OpenEmail(ctx, u"signed-contract.eml"_s, true);
  auto* badge = view != nullptr ? SecurityBadge(ctx, view) : nullptr;
  // Verify Signature leads the menu of a signed message.
  auto* verify = badge != nullptr ? BadgeEntry(badge, 0) : nullptr;
  if (verify == nullptr) {
    if (!ctx.GetFailure()) ctx.Fail(kEXIT_USAGE, u"email-verify-missing"_s);
    return {};
  }
  const auto before = badge->text();
  if (!ctx.WaitUntil(
          [&]() { verify->trigger(); },
          [&]() { return badge->text() != before && !WaitingDialogVisible(); },
          {}, u"email-verified"_s,
          [&]() { return u"badge=%1"_s.arg(badge->text()); })) {
    return {};
  }
  return {window, view, badge};
}

auto VerifiedScenario(ScenarioContext& ctx) -> bool {
  auto [window, view, badge] = OpenVerified(ctx);
  if (badge == nullptr) return false;
  ctx.SetTarget(window, CaptureMode::kWINDOW);
  ctx.AddHighlight(u"securityBadge"_s, badge);
  return SettleActive(ctx, window, u"email-verified-settled"_s);
}

/**
 * @brief Message Details on its Security tab, from the verified badge.
 */
auto SignatureDetailsScenario(ScenarioContext& ctx) -> bool {
  auto [window, view, badge] = OpenVerified(ctx);
  if (badge == nullptr) return false;
  // Signature Details... follows Verify Signature.
  auto* details = BadgeEntry(badge, 1);
  if (details == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"email-details-missing"_s);
    return false;
  }
  QWidget* dialog = nullptr;
  if (!ctx.WaitUntil([&]() { details->trigger(); },
                     [&]() {
                       dialog =
                           view->findChild<QWidget*>(u"emailDetailsDialog"_s);
                       return dialog != nullptr && dialog->isVisible();
                     },
                     {}, u"email-details-shown"_s)) {
    return false;
  }
  if (ctx.GetSize().isValid()) dialog->resize(ctx.GetSize());
  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  return SettleActive(ctx, dialog, u"email-details-settled"_s);
}

/**
 * @brief A draft in the Mail Editor, its security menu open at Sign.
 */
auto EditorSecurityMenuScenario(ScenarioContext& ctx) -> bool {
  auto [window, view] = OpenEmail(ctx, u"draft-contract.eml"_s, false);
  auto* badge = view != nullptr ? SecurityBadge(ctx, view) : nullptr;
  auto* sign = badge != nullptr ? BadgeEntry(badge, 0) : nullptr;
  if (!ShowButtonMenu(ctx, badge, badge != nullptr ? badge->menu() : nullptr,
                      sign)) {
    return false;
  }
  auto* menu = badge->menu();
  ctx.SetTarget(window, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(menu);
  ctx.AddHighlight(u"securityBadge"_s, badge);
  ctx.AddHighlight(u"sign"_s, menu, menu->actionGeometry(sign));
  return ctx.WaitSettled(
      window, [&]() { return menu->activeAction() == sign; },
      u"email-security-menu-settled"_s);
}

/**
 * @brief An HTML-only message, shown as its source with the notice above.
 */
auto HtmlNoticeScenario(ScenarioContext& ctx) -> bool {
  auto [window, view] = OpenEmail(ctx, u"newsletter.eml"_s, true);
  if (view == nullptr) return false;
  ctx.SetTarget(window, CaptureMode::kWINDOW);
  return SettleActive(ctx, window, u"email-html-settled"_s);
}

/**
 * @brief Settings > Mail Accounts with Alice's account and a disabled one.
 */
auto AccountsPageScenario(ScenarioContext& ctx) -> bool {
  SeedMailAccounts();
  auto* window =
      OpenMainWindow(ctx, kBackdropSize, u"note.txt"_s, ContractText());
  if (window == nullptr) return false;
  auto [dialog, page] =
      OpenSettingsPage(ctx, window, "EMailAccountSettingsPage");
  if (page == nullptr) return false;
  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  return SettleActive(ctx, dialog, u"email-accounts-settled"_s);
}

}  // namespace

void RegisterEmailScenarios(QList<Scenario>& scenarios) {
  const QString main_window = u"GpgFrontend::UI::MainWindow"_s;
  const QString waiting = u"GpgFrontend::UI::WaitingDialog"_s;
  const QSize window(1280, 960);

  scenarios.append({u"email-security-menu"_s,
                    window,
                    CaptureMode::kCOMPOSITE,
                    {main_window},
                    false,
                    true,
                    EditorSecurityMenuScenario});
  scenarios.append({u"email-verified"_s,
                    window,
                    CaptureMode::kWINDOW,
                    {main_window, waiting},
                    false,
                    true,
                    VerifiedScenario});
  scenarios.append(
      {u"email-signature-details"_s,
       QSize(),
       CaptureMode::kDIALOG,
       {main_window, waiting, QString::fromLatin1(kDetailsDialogClass)},
       false,
       true,
       SignatureDetailsScenario});
  scenarios.append({u"email-html-notice"_s,
                    window,
                    CaptureMode::kWINDOW,
                    {main_window},
                    false,
                    true,
                    HtmlNoticeScenario});
  scenarios.append({u"email-accounts-page"_s,
                    QSize(900, 660),
                    CaptureMode::kDIALOG,
                    {main_window, u"GpgFrontend::UI::SettingsDialog"_s},
                    false,
                    true,
                    AccountsPageScenario});
}

}  // namespace GpgFrontend::Screenshot
