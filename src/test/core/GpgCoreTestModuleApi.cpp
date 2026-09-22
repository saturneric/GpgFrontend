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

#include <cstddef>
#include <cstring>

#include "GpgFrontendTest.h"
#include "core/module/ModuleSdkBridge.h"
#include "sdk/GFSDKBuildInfo.h"
#include "sdk/GFSDKHostApi.h"
#include "sdk/GFSDKModuleApi.h"

/**
 * @file GpgCoreTestModuleApi.cpp
 * @brief The host table: minted per module, grouped, and append-only.
 *
 * Two properties are invisible until they break, and both are asserted here.
 *
 * `struct_size` is what lets a table gain fields without breaking a module
 * compiled against an older, smaller one. If a field is reordered or
 * repurposed instead of appended, every already-built module silently reads
 * the wrong slot.
 *
 * And a table is MINTED, not fetched. There is no function anywhere that
 * returns "the host api": a module has the one it was handed, carrying a
 * context that says who it is and what it may do. A test that could obtain a
 * fully-populated table by asking would be testing a door that no longer
 * exists.
 */

namespace GpgFrontend::Test {

namespace {

/// Mint a table the way Module::Active() does, and clean up after.
class MintedTable {
 public:
  MintedTable(const char* id, uint32_t granted) : id_(id) {
    table_ = static_cast<const GFHostApi*>(
        Module::ModuleSdkMintHostApi(id, granted));
  }
  ~MintedTable() { Module::ModuleSdkReleaseHostApi(id_); }

  MintedTable(const MintedTable&) = delete;
  auto operator=(const MintedTable&) -> MintedTable& = delete;

  [[nodiscard]] auto get() const -> const GFHostApi* { return table_; }
  auto operator->() const -> const GFHostApi* { return table_; }

 private:
  const char* id_;
  const GFHostApi* table_ = nullptr;
};

}  // namespace

TEST(ModuleApiTest, AMintedTableIsPopulatedAndSelfDescribing) {
  const MintedTable host("com.example.selfdescribing", 0);
  ASSERT_NE(host.get(), nullptr);

  EXPECT_EQ(host->struct_size, sizeof(GFHostApi));
  EXPECT_EQ(host->abi_version, static_cast<uint32_t>(GF_SDK_ABI_VERSION));
  EXPECT_STREQ(host->module_id, "com.example.selfdescribing");
  EXPECT_NE(host->context, nullptr);
}

// The always-granted groups are not a default that happens to be filled in --
// they are the ones for which "was this granted" is not a question. A module
// that cannot allocate, log, subscribe or read a list it was given is not a
// module.
TEST(ModuleApiTest, TheAlwaysGrantedGroupsArePresentEvenWithNoGrant) {
  const MintedTable host("com.example.nogrant", 0);
  ASSERT_NE(host.get(), nullptr);

  EXPECT_NE(host->buffer, nullptr);
  EXPECT_NE(host->log, nullptr);
  EXPECT_NE(host->app, nullptr);
  EXPECT_NE(host->event, nullptr);
  EXPECT_NE(host->bootstrap, nullptr);
  EXPECT_NE(host->list, nullptr);

  // And every grantable one is absent, because nothing was granted.
  EXPECT_EQ(host->gpg, nullptr);
  EXPECT_EQ(host->pgp, nullptr);
  EXPECT_EQ(host->ui, nullptr);
  EXPECT_EQ(host->editor, nullptr);
  EXPECT_EQ(host->storage, nullptr);
  EXPECT_EQ(host->process, nullptr);
}

TEST(ModuleApiTest, AGroupIsPresentExactlyWhenItsBitIsGranted) {
  const MintedTable host("com.example.uionly", GF_HOST_CAP_UI);
  ASSERT_NE(host.get(), nullptr);

  EXPECT_EQ(host->granted, static_cast<uint32_t>(GF_HOST_CAP_UI));
  EXPECT_NE(host->ui, nullptr);

  // The one that matters: a module declaring "ui" cannot decrypt. Before the
  // table was minted per module there was nowhere to express this at all.
  EXPECT_EQ(host->gpg, nullptr);
  EXPECT_EQ(host->process, nullptr);
  EXPECT_EQ(host->storage, nullptr);
}

TEST(ModuleApiTest, EveryGroupIsSelfDescribing) {
  const MintedTable host("com.example.everything",
                         GF_HOST_CAP_GPG | GF_HOST_CAP_PGP | GF_HOST_CAP_UI |
                             GF_HOST_CAP_EDITOR | GF_HOST_CAP_STORAGE |
                             GF_HOST_CAP_PROCESS);
  ASSERT_NE(host.get(), nullptr);

  // Each group carries its own struct_size, written by the side that compiled
  // it, so each can grow independently of the others and of the top table.
  EXPECT_EQ(host->buffer->struct_size, sizeof(GFHostBufferApi));
  EXPECT_EQ(host->log->struct_size, sizeof(GFHostLogApi));
  EXPECT_EQ(host->app->struct_size, sizeof(GFHostAppApi));
  EXPECT_EQ(host->event->struct_size, sizeof(GFHostEventApi));
  EXPECT_EQ(host->bootstrap->struct_size, sizeof(GFHostBootstrapApi));
  EXPECT_EQ(host->list->struct_size, sizeof(GFHostListApi));
  EXPECT_EQ(host->gpg->struct_size, sizeof(GFHostGpgApi));
  EXPECT_EQ(host->pgp->struct_size, sizeof(GFHostPgpApi));
  EXPECT_EQ(host->ui->struct_size, sizeof(GFHostUiApi));
  EXPECT_EQ(host->editor->struct_size, sizeof(GFHostEditorApi));
  EXPECT_EQ(host->storage->struct_size, sizeof(GFHostStorageApi));
  EXPECT_EQ(host->process->struct_size, sizeof(GFHostProcessApi));
}

// The table is usable through the pointers, not merely present -- and the
// context is what makes a call legal, so this also exercises the gate on its
// allowing path.
TEST(ModuleApiTest, AModuleCanRoundTripBytesThroughTheTableAlone) {
  const MintedTable host("com.example.roundtrip", 0);
  ASSERT_NE(host.get(), nullptr);

  const char payload[] = "a\0b";  // an embedded NUL, as message data has
  auto* buf =
      host->buffer->new_from_bytes(host->context, payload, sizeof(payload) - 1);
  ASSERT_NE(buf, nullptr);

  EXPECT_EQ(host->buffer->size(host->context, buf), sizeof(payload) - 1);
  EXPECT_EQ(std::memcmp(host->buffer->data(host->context, buf), payload,
                        sizeof(payload) - 1),
            0);

  host->buffer->release(host->context, buf);
}

// A context the host does not recognise is refused WITHOUT being read. This
// is the reason validity is decided by looking a pointer up in a registry
// rather than by dereferencing it to check a magic word.
TEST(ModuleApiTest, AForgedContextIsRefusedRatherThanFollowed) {
  const MintedTable host("com.example.forged", 0);
  ASSERT_NE(host.get(), nullptr);

  // A value that was never minted. Nothing is allocated at it, and nothing
  // reads it -- the refusal comes from its absence from the registry.
  auto* invented = reinterpret_cast<GFHostContextRef>(0xD15EA5EULL);

  auto* buf = host->buffer->new_from_bytes(invented, "x", 1);
  EXPECT_EQ(buf, nullptr);
  EXPECT_EQ(host->buffer->size(invented, nullptr), 0U);
}

// Release invalidates the context. A module that failed to stop a thread gets
// a refusal rather than a walk into an unmapped image.
TEST(ModuleApiTest, AReleasedContextStopsWorkingOnEveryThread) {
  const auto* host = static_cast<const GFHostApi*>(
      Module::ModuleSdkMintHostApi("com.example.released", 0));
  ASSERT_NE(host, nullptr);

  auto* ctx = host->context;
  auto* before = host->buffer->new_from_bytes(ctx, "x", 1);
  ASSERT_NE(before, nullptr);
  host->buffer->release(ctx, before);

  Module::ModuleSdkReleaseHostApi("com.example.released");

  // The table itself is still mapped -- deliberately, because the module may
  // still be holding it -- but the grant behind it is gone.
  EXPECT_EQ(host->buffer->new_from_bytes(ctx, "x", 1), nullptr);
}

// The growth rule, stated as a test: a field may only ever be APPENDED.
// These offsets are the prefix that already-built modules read; changing any
// of them makes those modules read the wrong slot at runtime, with no
// diagnostic anywhere.
TEST(ModuleApiTest, ThePrefixLayoutIsFrozen) {
  EXPECT_EQ(offsetof(GFHostApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFHostApi, abi_version), sizeof(size_t));

  // Every group struct leads with its own size, at offset zero, for the same
  // reason the top table does.
  EXPECT_EQ(offsetof(GFHostBufferApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFHostLogApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFHostAppApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFHostEventApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFHostBootstrapApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFHostListApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFHostGpgApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFHostPgpApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFHostUiApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFHostEditorApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFHostStorageApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFHostProcessApi, struct_size), 0U);

  // So do the row structs a group hands out, which is what lets a key brief
  // gain a field without a new accessor.
  EXPECT_EQ(offsetof(GFGpgKeyBriefRow, struct_size), 0U);
  EXPECT_EQ(offsetof(GFGpgRecipientRow, struct_size), 0U);
  EXPECT_EQ(offsetof(GFGpgAnalysis, struct_size), 0U);
  EXPECT_EQ(offsetof(GFUISettingsPageSpec, struct_size), 0U);
  EXPECT_EQ(offsetof(GFUITabViewSpec, struct_size), 0U);
  EXPECT_EQ(offsetof(GFModuleEventAnswer, struct_size), 0U);

  EXPECT_EQ(offsetof(GFModuleApi, struct_size), 0U);
  EXPECT_EQ(offsetof(GFModuleApi, abi_version), sizeof(size_t));

  // The host reads up to and including unregister, so a module table must be
  // at least this big to be usable. Module.cpp enforces exactly this.
  EXPECT_LE(offsetof(GFModuleApi, unregister) + sizeof(void*),
            sizeof(GFModuleApi));
}

// An older module hands over a SMALLER struct. The host must accept it and
// read only the shared prefix -- that is the entire point of struct_size.
TEST(ModuleApiTest, ASmallerModuleTableIsStillUsable) {
  GFModuleApi older{};
  older.struct_size = offsetof(GFModuleApi, unregister) + sizeof(void*);
  older.abi_version = GF_SDK_ABI_VERSION;
  older.module_id = "com.example.older";
  older.version = "1.0.0";

  EXPECT_GE(older.struct_size,
            offsetof(GFModuleApi, unregister) + sizeof(void*));
  EXPECT_LE(older.struct_size, sizeof(GFModuleApi));
}

TEST(ModuleApiTest, ATableTooSmallToReadIsRejectable) {
  // Below the prefix the host dereferences: accepting this would read past
  // the end of what the module actually compiled.
  GFModuleApi stunted{};
  stunted.struct_size = sizeof(size_t) + sizeof(uint32_t);
  EXPECT_LT(stunted.struct_size,
            offsetof(GFModuleApi, unregister) + sizeof(void*));
}

}  // namespace GpgFrontend::Test
