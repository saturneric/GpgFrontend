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
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>
#include <QWizard>

#include "screenshot/ScreenshotEnv.h"
#include "screenshot/ScreenshotScenario.h"
#include "screenshot/scenarios/ScenarioHelpers.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::Screenshot {

namespace {

using UI::MainWindow;

constexpr auto kKeyGenerateWizardClass = "GpgFrontend::UI::KeyGenerateWizard";

/// The wizard's pages, as KeyGenerateStep numbers them.
constexpr int kKeyDetailsPage = 4;

/// The button that moves the wizard on from its current page.
auto ForwardButton(QWizard* wizard) -> QAbstractButton* {
  for (const auto which :
       {QWizard::FinishButton, QWizard::CommitButton, QWizard::NextButton}) {
    auto* button = wizard->button(which);
    if (button != nullptr && button->isVisible()) return button;
  }
  return nullptr;
}

/**
 * @brief Generate Key from the main window, its first step filled in for
 *        Alice.
 */
auto OpenKeyGenerateWizard(ScenarioContext& ctx) -> QWizard* {
  auto* window =
      OpenMainWindow(ctx, kBackdropSize, u"contract.txt"_s, ContractText());
  if (window == nullptr) return nullptr;
  auto* action = window->findChild<QAction*>(u"generate_key_pair"_s);
  if (action == nullptr || !action->isEnabled()) {
    ctx.Fail(kEXIT_USAGE, u"generate-key-unavailable"_s);
    return nullptr;
  }
  auto* wizard = qobject_cast<QWizard*>(WaitForTopLevel(
      ctx, [&]() { action->trigger(); }, kKeyGenerateWizardClass,
      u"key-generate-wizard-shown"_s));
  if (wizard == nullptr) return nullptr;

  auto* name = wizard->findChild<QLineEdit*>(u"nameEdit"_s);
  auto* email = wizard->findChild<QLineEdit*>(u"emailEdit"_s);
  auto* comment = wizard->findChild<QLineEdit*>(u"commentEdit"_s);
  if (name == nullptr || email == nullptr || comment == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"key-generate-fields-missing"_s);
    return nullptr;
  }
  name->setText(u"Alice Example"_s);
  email->setText(u"alice@example.com"_s);
  comment->setText(u"Work"_s);
  return wizard;
}

/// Capture @p wizard as it is, its forward button focused and highlighted.
auto SettleWizard(ScenarioContext& ctx, QWizard* wizard, const QString& label)
    -> bool {
  if (ctx.GetSize().isValid()) {
    // The wizard fits itself to its tallest page from a queued call; one
    // queued after it has run once that fit has, and only then is a size set
    // here kept.
    bool fitted = false;
    QTimer::singleShot(0, wizard, [&fitted]() { fitted = true; });
    if (!ctx.WaitUntil(
            {}, [&]() { return fitted; }, {}, u"wizard-fitted"_s) ||
        !ctx.WaitUntil(
            [&]() { wizard->resize(ctx.GetSize()); },
            [&]() { return wizard->size() == ctx.GetSize(); }, {},
            u"wizard-resized"_s,
            [&]() {
              return u"size=%1x%2"_s.arg(wizard->width()).arg(wizard->height());
            })) {
      return false;
    }
  }
  auto* forward = ForwardButton(wizard);
  ctx.SetTarget(wizard, CaptureMode::kDIALOG);
  if (forward != nullptr) ctx.AddHighlight(u"forwardButton"_s, forward);
  if (auto* expert = wizard->findChild<QWidget*>(u"expertSwitch"_s)) {
    ctx.AddHighlight(u"expertSwitch"_s, expert);
  }
  // Each page moves focus to its first field from a queued call; this one
  // is queued after it, so the focus ends on the button and no field draws
  // its cursor.
  bool focus_moved = false;
  QTimer::singleShot(0, wizard, [&]() {
    if (forward != nullptr) forward->setFocus();
    focus_moved = true;
  });
  return ctx.WaitSettled(
      wizard,
      [&]() {
        if (!wizard->isActiveWindow()) wizard->activateWindow();
        return focus_moved && wizard->isActiveWindow();
      },
      label);
}

/// The wizard @p steps pages after Identity.
auto KeyGenerateStep(ScenarioContext& ctx, int steps, const QString& label)
    -> bool {
  auto* wizard = OpenKeyGenerateWizard(ctx);
  if (wizard == nullptr || !AdvanceWizard(ctx, wizard, steps)) return false;
  return SettleWizard(ctx, wizard, label);
}

auto KeyTypeScenario(ScenarioContext& ctx) -> bool {
  return KeyGenerateStep(ctx, 1, u"key-generate-type-settled"_s);
}

auto ProtectionScenario(ScenarioContext& ctx) -> bool {
  return KeyGenerateStep(ctx, 2, u"key-generate-protection-settled"_s);
}

auto ReviewScenario(ScenarioContext& ctx) -> bool {
  return KeyGenerateStep(ctx, 3, u"key-generate-review-settled"_s);
}

/**
 * @brief Expert Mode turned on at Key Type: Key Details takes its place.
 */
auto ExpertScenario(ScenarioContext& ctx) -> bool {
  auto* wizard = OpenKeyGenerateWizard(ctx);
  if (wizard == nullptr || !AdvanceWizard(ctx, wizard, 1)) return false;
  auto* expert = wizard->findChild<QAbstractButton*>(u"expertSwitch"_s);
  if (expert == nullptr) {
    ctx.Fail(kEXIT_USAGE, u"expert-switch-missing"_s);
    return false;
  }
  if (!ctx.WaitUntil([&]() { expert->setChecked(true); },
                     [&]() { return wizard->currentId() == kKeyDetailsPage; },
                     {ScenarioContext::On(wizard, &QWizard::currentIdChanged)},
                     u"key-generate-expert-shown"_s)) {
    return false;
  }
  return SettleWizard(ctx, wizard, u"key-generate-expert-settled"_s);
}

/**
 * @brief The Keys toolbar's New Keypair menu, at Quick Generate.
 */
auto QuickGenerateMenuScenario(ScenarioContext& ctx) -> bool {
  auto* window =
      OpenMainWindow(ctx, ctx.GetSize(), u"contract.txt"_s, ContractText());
  if (window == nullptr) return false;
  auto* button = window->findChild<QToolButton*>(u"generateKeyButton"_s);
  auto* entry = window->findChild<QAction*>(u"quick_generate_key"_s);
  auto* menu = button != nullptr ? button->menu() : nullptr;
  if (!ShowButtonMenu(ctx, button, menu, entry)) return false;
  ctx.SetTarget(window, CaptureMode::kCOMPOSITE);
  ctx.AddPopup(menu);
  ctx.AddHighlight(u"generateKeyButton"_s, button);
  ctx.AddHighlight(u"quickGenerate"_s, menu, menu->actionGeometry(entry));
  return ctx.WaitSettled(
      window, [&]() { return menu->activeAction() == entry; },
      u"quick-generate-menu-settled"_s);
}

/**
 * @brief Add Subkey from Alice's Keychain tab, with Expert Mode on.
 */
auto SubkeyExpertScenario(ScenarioContext& ctx) -> bool {
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
  auto* expert = dialog != nullptr
                     ? dialog->findChild<QAbstractButton*>(u"expertSwitch"_s)
                     : nullptr;
  if (expert == nullptr) {
    if (dialog != nullptr) ctx.Fail(kEXIT_USAGE, u"expert-switch-missing"_s);
    return false;
  }
  expert->setChecked(true);
  ctx.SetTarget(dialog, CaptureMode::kDIALOG);
  ctx.AddHighlight(u"expertSwitch"_s, expert);
  return SettleActive(ctx, dialog, u"subkey-expert-settled"_s);
}

}  // namespace

void RegisterKeyGenerationScenarios(QList<Scenario>& scenarios) {
  const QString main_window = u"GpgFrontend::UI::MainWindow"_s;
  const QStringList with_wizard{main_window,
                                QString::fromLatin1(kKeyGenerateWizardClass)};

  scenarios.append({u"key-generate-type"_s, QSize(), CaptureMode::kDIALOG,
                    with_wizard, false, true, KeyTypeScenario});
  scenarios.append({u"key-generate-protection"_s, QSize(), CaptureMode::kDIALOG,
                    with_wizard, false, true, ProtectionScenario});
  scenarios.append({u"key-generate-review"_s, QSize(), CaptureMode::kDIALOG,
                    with_wizard, false, true, ReviewScenario});
  scenarios.append({u"key-generate-expert"_s, QSize(), CaptureMode::kDIALOG,
                    with_wizard, false, true, ExpertScenario});
  scenarios.append({u"quick-generate-menu"_s,
                    QSize(1280, 960),
                    CaptureMode::kCOMPOSITE,
                    {main_window},
                    false,
                    true,
                    QuickGenerateMenuScenario});
  scenarios.append({u"subkey-generate-expert"_s,
                    QSize(),
                    CaptureMode::kDIALOG,
                    {main_window, QString::fromLatin1(kKeyDetailsClass),
                     u"GpgFrontend::UI::SubkeyGenerateDialog"_s},
                    false,
                    true,
                    SubkeyExpertScenario});
}

}  // namespace GpgFrontend::Screenshot
