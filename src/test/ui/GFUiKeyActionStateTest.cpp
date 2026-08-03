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

#include <gtest/gtest.h>

#include "GpgFrontendTest.h"
#include "ui/main_window/GeneralMainWindow.h"
#include "ui/main_window/KeyActionState.h"

namespace GpgFrontend::Test {

namespace {

using UI::EffectiveTargetCount;
using UI::EvaluateKeyAction;
using UI::KeyAction;
using UI::KeyActionContext;

/**
 * @brief A context where everything the engine and the modules could offer is
 * available, so a test only has to state the part it cares about.
 */
auto FullySupported() -> KeyActionContext {
  KeyActionContext ctx;
  ctx.sign_supported = true;
  ctx.owner_trust_supported = true;
  ctx.subkey_generation_supported = true;
  ctx.ssh_export_supported = true;
  ctx.keyserver_search_available = true;
  ctx.keyserver_upload_available = true;
  ctx.keyserver_fetch_available = true;
  ctx.any_private_key_in_keyring = true;
  return ctx;
}

/**
 * @brief Exactly one key of our own selected.
 */
auto OneOwnKey() -> KeyActionContext {
  auto ctx = FullySupported();
  ctx.selected_count = 1;
  ctx.selection_is_gpg_key = true;
  ctx.selection_is_private = true;
  ctx.selection_has_email = true;
  ctx.any_target_private_key = true;
  return ctx;
}

/**
 * @brief Exactly one key belonging to someone else.
 */
auto OneForeignKey() -> KeyActionContext {
  auto ctx = OneOwnKey();
  ctx.selection_is_private = false;
  ctx.any_target_private_key = false;
  return ctx;
}

}  // namespace

TEST(KeyActionStateTest, NothingSelectedDisablesEveryPerKeyAction) {
  const auto ctx = FullySupported();

  for (const auto action :
       {KeyAction::kShowDetails, KeyAction::kCopyFingerprint,
        KeyAction::kCopyKeyId, KeyAction::kCopyEmail, KeyAction::kCopyPublicKey,
        KeyAction::kCertify, KeyAction::kSetExpiry, KeyAction::kGenerateSubkey,
        KeyAction::kSetOwnerTrust, KeyAction::kGenerateRevokeCert,
        KeyAction::kDeleteSelected, KeyAction::kDeleteChecked,
        KeyAction::kExportPackage, KeyAction::kExportClipboard,
        KeyAction::kExportOpenSsh}) {
    const auto state = EvaluateKeyAction(action, ctx);
    EXPECT_FALSE(state.enabled);
    // A greyed entry with no explanation is worse than no entry.
    EXPECT_FALSE(state.reason.isEmpty());
  }
}

TEST(KeyActionStateTest, SearchingAKeyserverNeedsNoSelection) {
  // The point of searching is to find a key you do not have yet.
  EXPECT_TRUE(
      EvaluateKeyAction(KeyAction::kKeyserverSearch, FullySupported()).enabled);
}

TEST(KeyActionStateTest, BackupAllPrivateFollowsTheKeyringNotTheSelection) {
  auto ctx = FullySupported();
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kBackupAllPrivate, ctx).enabled);

  ctx.any_private_key_in_keyring = false;
  const auto state = EvaluateKeyAction(KeyAction::kBackupAllPrivate, ctx);
  EXPECT_FALSE(state.enabled);
  EXPECT_FALSE(state.reason.isEmpty());
}

TEST(KeyActionStateTest, OwnKeyEnablesTheLifecycleOperations) {
  const auto ctx = OneOwnKey();

  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kSetExpiry, ctx).enabled);
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kGenerateSubkey, ctx).enabled);
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kGenerateRevokeCert, ctx).enabled);
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kSetOwnerTrust, ctx).enabled);
}

TEST(KeyActionStateTest, CertifyingYourOwnKeyIsDisabled) {
  // gpg already trusts it ultimately by virtue of you holding the private half.
  const auto state = EvaluateKeyAction(KeyAction::kCertify, OneOwnKey());
  EXPECT_FALSE(state.enabled);
  EXPECT_TRUE(state.supported);
  EXPECT_FALSE(state.reason.isEmpty());
}

TEST(KeyActionStateTest, CertifyingSomeoneElsesKeyIsEnabled) {
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kCertify, OneForeignKey()).enabled);
}

TEST(KeyActionStateTest, ForeignKeyCannotHaveItsExpiryOrSubkeysChanged) {
  const auto ctx = OneForeignKey();

  EXPECT_FALSE(EvaluateKeyAction(KeyAction::kSetExpiry, ctx).enabled);
  EXPECT_FALSE(EvaluateKeyAction(KeyAction::kGenerateSubkey, ctx).enabled);
  EXPECT_FALSE(EvaluateKeyAction(KeyAction::kGenerateRevokeCert, ctx).enabled);
}

TEST(KeyActionStateTest, UnsupportedOperationsAreHiddenNotGreyed) {
  // Offering something this engine cannot do at all is worse than not
  // mentioning it, so these vanish rather than sitting there greyed forever.
  auto ctx = OneForeignKey();
  ctx.sign_supported = false;
  ctx.owner_trust_supported = false;
  ctx.subkey_generation_supported = false;
  ctx.ssh_export_supported = false;

  for (const auto action :
       {KeyAction::kCertify, KeyAction::kSetOwnerTrust,
        KeyAction::kGenerateSubkey, KeyAction::kExportOpenSsh}) {
    EXPECT_FALSE(EvaluateKeyAction(action, ctx).supported);
  }
}

TEST(KeyActionStateTest, KeyGroupDisablesTheKeySpecificOperations) {
  auto ctx = OneOwnKey();
  ctx.selection_is_gpg_key = false;

  for (const auto action :
       {KeyAction::kCertify, KeyAction::kSetExpiry, KeyAction::kGenerateSubkey,
        KeyAction::kSetOwnerTrust, KeyAction::kGenerateRevokeCert}) {
    const auto state = EvaluateKeyAction(action, ctx);
    EXPECT_FALSE(state.enabled);
    EXPECT_FALSE(state.reason.isEmpty());
  }
}

TEST(KeyActionStateTest, KeyGroupStillSupportsDetailsAndCopy) {
  auto ctx = OneOwnKey();
  ctx.selection_is_gpg_key = false;

  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kShowDetails, ctx).enabled);
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kCopyKeyId, ctx).enabled);
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kDeleteSelected, ctx).enabled);
}

TEST(KeyActionStateTest, MultiSelectionKeepsWhatWorksOnAListAndDropsTheRest) {
  auto ctx = OneOwnKey();
  ctx.selected_count = 3;

  // Deletion takes a list and armored blocks concatenate.
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kDeleteSelected, ctx).enabled);
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kCopyPublicKey, ctx).enabled);

  // These each open a dialog about one key, or write one value to the
  // clipboard; three at once is not a feature.
  for (const auto action :
       {KeyAction::kShowDetails, KeyAction::kCopyFingerprint,
        KeyAction::kCopyKeyId, KeyAction::kCopyEmail, KeyAction::kCertify,
        KeyAction::kSetExpiry, KeyAction::kGenerateSubkey,
        KeyAction::kSetOwnerTrust, KeyAction::kGenerateRevokeCert,
        KeyAction::kExportOpenSsh}) {
    EXPECT_FALSE(EvaluateKeyAction(action, ctx).enabled);
  }
}

TEST(KeyActionStateTest, CopyEmailNeedsTheKeyToHaveOne) {
  auto ctx = OneOwnKey();
  ctx.selection_has_email = false;

  const auto state = EvaluateKeyAction(KeyAction::kCopyEmail, ctx);
  EXPECT_FALSE(state.enabled);
  EXPECT_FALSE(state.reason.isEmpty());
}

// The checked-else-selected fallback: selecting a row and hitting Export used
// to raise a modal "Forbidden — please check some keys" instead of working.

TEST(KeyActionStateTest, SelectionAloneEnablesTheMultiKeyOperations) {
  auto ctx = FullySupported();
  ctx.selected_count = 2;
  ctx.checked_count = 0;

  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kExportPackage, ctx).enabled);
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kExportClipboard, ctx).enabled);
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kKeyserverPublish, ctx).enabled);
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kKeyserverRefresh, ctx).enabled);
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kCategory, ctx).enabled);
}

TEST(KeyActionStateTest, CheckedSetWinsOverTheSelection) {
  auto ctx = FullySupported();
  ctx.selected_count = 1;
  ctx.checked_count = 5;

  EXPECT_EQ(EffectiveTargetCount(ctx), 5);
}

TEST(KeyActionStateTest, SelectionIsUsedWhenNothingIsChecked) {
  auto ctx = FullySupported();
  ctx.selected_count = 2;
  ctx.checked_count = 0;

  EXPECT_EQ(EffectiveTargetCount(ctx), 2);
}

TEST(KeyActionStateTest, DeleteCheckedMeansCheckedAndNothingElse) {
  // Its counterpart for the selection sits right beside it in the menu, so
  // this one must not quietly fall back or the two become the same command.
  auto ctx = FullySupported();
  ctx.selected_count = 3;
  ctx.checked_count = 0;

  const auto state = EvaluateKeyAction(KeyAction::kDeleteChecked, ctx);
  EXPECT_FALSE(state.enabled);
  EXPECT_FALSE(state.reason.isEmpty());

  ctx.checked_count = 1;
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kDeleteChecked, ctx).enabled);
}

TEST(KeyActionStateTest, OpenSshExportTakesExactlyOneKey) {
  auto ctx = FullySupported();

  ctx.selected_count = 1;
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kExportOpenSsh, ctx).enabled);

  ctx.checked_count = 4;
  const auto state = EvaluateKeyAction(KeyAction::kExportOpenSsh, ctx);
  EXPECT_FALSE(state.enabled);
  EXPECT_FALSE(state.reason.isEmpty());
}

TEST(KeyActionStateTest, KeyserverActionsTrackTheirOwnModuleEvents) {
  // Search available but upload not is a real configuration, and it must not
  // take the whole submenu down with it.
  auto ctx = FullySupported();
  ctx.selected_count = 1;
  ctx.keyserver_upload_available = false;

  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kKeyserverSearch, ctx).supported);
  EXPECT_FALSE(EvaluateKeyAction(KeyAction::kKeyserverPublish, ctx).supported);
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kKeyserverRefresh, ctx).supported);
}

TEST(KeyActionStateTest, NoKeyserverModuleHidesAllThreeKeyserverActions) {
  auto ctx = FullySupported();
  ctx.selected_count = 1;
  ctx.keyserver_search_available = false;
  ctx.keyserver_upload_available = false;
  ctx.keyserver_fetch_available = false;

  for (const auto action :
       {KeyAction::kKeyserverSearch, KeyAction::kKeyserverPublish,
        KeyAction::kKeyserverRefresh}) {
    EXPECT_FALSE(EvaluateKeyAction(action, ctx).supported);
  }
}

TEST(KeyActionStateTest, BulkExtendExpiryNeedsAPrivateKeyAmongTheTargets) {
  auto ctx = FullySupported();
  ctx.checked_count = 3;
  ctx.any_target_private_key = false;

  const auto state = EvaluateKeyAction(KeyAction::kBulkExtendExpiry, ctx);
  EXPECT_FALSE(state.enabled);
  EXPECT_FALSE(state.reason.isEmpty());

  ctx.any_target_private_key = true;
  EXPECT_TRUE(EvaluateKeyAction(KeyAction::kBulkExtendExpiry, ctx).enabled);
}

TEST(KeyActionStateTest, EveryDisabledActionExplainsItself) {
  // Guards against a new enumerator picking up a default case that greys the
  // entry with no tooltip, which reads to the user as the app being broken.
  const auto empty = KeyActionContext{};

  for (const auto action : UI::AllKeyActions()) {
    const auto state = EvaluateKeyAction(action, empty);
    if (state.supported && !state.enabled) {
      EXPECT_FALSE(state.reason.isEmpty())
          << "action " << static_cast<int>(action) << " gives no reason";
    }
  }
}

TEST(KeyActionStateTest, EnabledActionsCarryNoReason) {
  for (const auto action : UI::AllKeyActions()) {
    const auto state = EvaluateKeyAction(action, OneOwnKey());
    if (state.enabled) {
      EXPECT_TRUE(state.reason.isEmpty())
          << "action " << static_cast<int>(action) << " is enabled but excused";
    }
  }
}

// The chrome style sheet is shared by every main window and is the reason the
// key management window stopped depending on being parented to the main window
// to look styled at all. It is a plain string factory precisely so it can be
// checked here: these cases run off the GUI thread and cannot build a widget.

TEST(MainWindowChromeStyleTest, StylesTheChromeItClaimsTo) {
  const auto qss = UI::MainWindowChromeStyleSheet();

  EXPECT_TRUE(qss.contains("QToolBar {"));
  EXPECT_TRUE(qss.contains("QToolBar::separator"));
  EXPECT_TRUE(qss.contains("QStatusBar"));
}

TEST(MainWindowChromeStyleTest, NamesOnlyPaletteRolesSoDarkModeSurvives) {
  const auto qss = UI::MainWindowChromeStyleSheet();

  EXPECT_TRUE(qss.contains("palette(window)"));
  EXPECT_TRUE(qss.contains("palette(mid)"));

  // There is no theme engine here, only the palette swap in
  // GpgFrontendUIInit.cpp, so a literal colour would survive the swap and
  // stand out against every other surface once the user goes dark.
  EXPECT_FALSE(qss.contains(QChar('#')))
      << "literal colour in the shared chrome sheet: " << qss.toStdString();
}

}  // namespace GpgFrontend::Test
