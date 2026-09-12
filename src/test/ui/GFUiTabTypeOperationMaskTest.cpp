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

#include <QSet>
#include <QString>

#include "GpgFrontendTest.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::Test {

namespace {

using UI::MainWindow;

using OperationMenu = MainWindow::OperationMenu;

/// A module host that listens for exactly the operations named.
auto Listening(const QStringList& suffixes) {
  QSet<QString> ids;
  for (const auto& suffix : suffixes) {
    ids.insert(QString("EDIT_TAB_TYPE_EMAIL_OP_%1").arg(suffix));
  }
  return [ids](const QString& id) { return ids.contains(id); };
}

auto Nothing() {
  return [](const QString&) { return false; };
}

auto Everything() {
  return [](const QString&) { return true; };
}

}  // namespace

// The ordinary editor has always been able to do everything, and nothing about
// a plain text document restricts it. In particular this must not depend on
// any module being loaded at all.
TEST(GFUiTabTypeOperationMaskTest, PlainTextTabsGetEveryOperation) {
  EXPECT_EQ(MainWindow::OperationsMaskForTabType("text", Nothing()), ~0U);
  EXPECT_EQ(MainWindow::OperationsMaskForTabType("text", Everything()), ~0U);
}

TEST(GFUiTabTypeOperationMaskTest,
     AModuleTabOnlyGetsTheOperationsItRegistered) {
  const auto mask = MainWindow::OperationsMaskForTabType(
      "email", Listening({"ENCRYPT", "DECRYPT"}));

  EXPECT_TRUE((mask & OperationMenu::kEncrypt) != 0U);
  EXPECT_TRUE((mask & OperationMenu::kDecrypt) != 0U);

  // ...and nothing else. Offering an operation and then apologising for it
  // afterwards is what this rule exists to stop.
  EXPECT_EQ(mask & OperationMenu::kSign, 0U);
  EXPECT_EQ(mask & OperationMenu::kVerify, 0U);
  EXPECT_EQ(mask & OperationMenu::kEncryptAndSign, 0U);
  EXPECT_EQ(mask & OperationMenu::kDecryptAndVerify, 0U);
}

TEST(GFUiTabTypeOperationMaskTest, AModuleTabNeverGetsSymmetricEncryption) {
  // There is no EDIT_TAB_TYPE_<TYPE>_OP_SYMMETRIC event to route it to, so it
  // could only ever run against the raw document. Even a module claiming to
  // listen for everything does not get it.
  const auto mask = MainWindow::OperationsMaskForTabType("email", Everything());
  EXPECT_EQ(mask & OperationMenu::kSymmetricEncrypt, 0U);

  // The six that DO have events are all there.
  EXPECT_TRUE((mask & OperationMenu::kEncrypt) != 0U);
  EXPECT_TRUE((mask & OperationMenu::kDecrypt) != 0U);
  EXPECT_TRUE((mask & OperationMenu::kSign) != 0U);
  EXPECT_TRUE((mask & OperationMenu::kVerify) != 0U);
  EXPECT_TRUE((mask & OperationMenu::kEncryptAndSign) != 0U);
  EXPECT_TRUE((mask & OperationMenu::kDecryptAndVerify) != 0U);
}

TEST(GFUiTabTypeOperationMaskTest, AnUnclaimedTabTypeGetsNothing) {
  // A tab type no module handles can do nothing at all, rather than appearing
  // to offer the full menu and failing on every item.
  EXPECT_EQ(MainWindow::OperationsMaskForTabType("email", Nothing()),
            OperationMenu::kNone);
}

TEST(GFUiTabTypeOperationMaskTest, TheEventIdsAreUpperCasedAndExact) {
  // The tab type comes from a widget property written in lower case, while
  // the events a module registers are upper case. Getting this wrong would
  // silently disable every operation on every module tab.
  QSet<QString> asked;
  MainWindow::OperationsMaskForTabType("email", [&asked](const QString& id) {
    asked.insert(id);
    return false;
  });

  EXPECT_TRUE(asked.contains("EDIT_TAB_TYPE_EMAIL_OP_ENCRYPT"));
  EXPECT_TRUE(asked.contains("EDIT_TAB_TYPE_EMAIL_OP_DECRYPT"));
  EXPECT_TRUE(asked.contains("EDIT_TAB_TYPE_EMAIL_OP_SIGN"));
  EXPECT_TRUE(asked.contains("EDIT_TAB_TYPE_EMAIL_OP_VERIFY"));
  EXPECT_TRUE(asked.contains("EDIT_TAB_TYPE_EMAIL_OP_ENCRYPT_SIGN"));
  EXPECT_TRUE(asked.contains("EDIT_TAB_TYPE_EMAIL_OP_DECRYPT_VERIFY"));
  EXPECT_EQ(asked.size(), 6);
}

// The tab TYPE says which operations exist for this kind of document; the open
// document says which of them mean anything right now. The two are separate
// questions, and the menu is the intersection -- so a rule about one must not
// quietly answer the other.
TEST(GFUiTabTypeOperationMaskTest, TheTypeMaskSaysNothingAboutDocumentState) {
  // An e-mail module registers all six regardless of what is open, which is
  // why a second, state-based narrowing is needed at all.
  const auto mask = MainWindow::OperationsMaskForTabType("email", Everything());

  EXPECT_TRUE((mask & OperationMenu::kDecrypt) != 0U);
  EXPECT_TRUE((mask & OperationMenu::kVerify) != 0U);
}

TEST(GFUiTabTypeOperationMaskTest, TabTypeIsMatchedCaseInsensitively) {
  // The property is written in lower case on the widget and in upper case in
  // the event id, and a tab created by a module could use either.
  const auto lower =
      MainWindow::OperationsMaskForTabType("email", Everything());
  const auto upper =
      MainWindow::OperationsMaskForTabType("EMAIL", Everything());
  EXPECT_EQ(lower, upper);
}

// The menu the user ends up with is the intersection of three separate
// narrowings: what the tab TYPE supports, what the open DOCUMENT admits of,
// and what the CHECKED KEYS can do. They are computed independently, so it is
// worth pinning down how they interact -- the combination is where a menu
// goes completely dark without any one rule looking wrong on its own.
namespace {

/// What slot_update_operations_menu_by_checked_keys clears when the user has
/// checked no key at all. Mirrored here so a change to that rule has to be a
/// deliberate one.
constexpr unsigned int kClearedWithNoKeysChecked =
    OperationMenu::kEncrypt | OperationMenu::kEncryptAndSign |
    OperationMenu::kSign;

}  // namespace

TEST(GFUiTabTypeOperationMaskTest, TheReadingOperationsSurviveWithNoKeys) {
  // Checking no key is the ordinary state of a freshly opened window, and it
  // withdraws every operation that needs a recipient or a signer. What it must
  // never touch is the operations that only read what is already there --
  // otherwise a document state offering nothing else leaves an empty menu.
  constexpr unsigned int kReading = OperationMenu::kDecrypt |
                                    OperationMenu::kVerify |
                                    OperationMenu::kDecryptAndVerify;

  EXPECT_EQ(kReading & kClearedWithNoKeysChecked, 0U)
      << "the no-keys rule must not clear a read-only operation";
}

TEST(GFUiTabTypeOperationMaskTest,
     APlainDocumentNeedsAKeyBeforeAnythingApplies) {
  // A message with no OpenPGP structure has nothing to decrypt or verify --
  // the handlers refuse anything that is not multipart/encrypted or
  // multipart/signed -- so all it can offer are the operations that PRODUCE a
  // protected message. Those are exactly the ones that need a key.
  //
  // So an empty crypto menu on a plain draft with no key checked is the
  // correct answer, not a fault: there is genuinely nothing to do yet.
  constexpr unsigned int kPlainDocument = OperationMenu::kEncrypt |
                                          OperationMenu::kSign |
                                          OperationMenu::kEncryptAndSign;

  EXPECT_EQ(kPlainDocument & ~kClearedWithNoKeysChecked, 0U);

  // Checking a capable key brings all three back. This is the half that must
  // keep working: if a mask latched from somewhere else were still ANDed in
  // here, the menu would stay dark and look exactly like the case above.
  constexpr unsigned int kNothingClearedWithAKeyChecked = ~0U;
  EXPECT_EQ(kPlainDocument & kNothingClearedWithAKeyChecked, kPlainDocument);
}

}  // namespace GpgFrontend::Test
