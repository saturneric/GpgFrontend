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

#include <QThread>

#include "GpgFrontendTest.h"
#include "core/SdkTestContext.h"
#include "core/module/ModuleCapability.h"
#include "core/module/ModuleSdkBridge.h"
#include "sdk/GFSDKGpgResult.h"
#include "sdk/GFSDKHostApi.h"
#include "sdk/GFSDKStorage.h"

/**
 * @file GpgCoreTestModuleCapability.cpp
 * @brief What a module may do, and the two ways of asking.
 *
 * The property under test is not "a null pointer is null". It is that the
 * answer to "may this module decrypt" is the SAME on every thread, because a
 * module does most of its work on threads it started itself and the host has
 * never been anywhere near them.
 *
 * That is why authorization travels in the context argument rather than in
 * the host's thread-local record of whose code is running. The two
 * cross-thread tests below are written so that a thread-local implementation
 * fails one of them: the grant case would be refused, having no host frame to
 * read, and the denial case would be ALLOWED, for exactly the same reason.
 */

namespace GpgFrontend::Test {

namespace {

/// Mint a table and release it again, whatever the test does in between.
class Granted {
 public:
  Granted(const char* id, uint32_t mask) : id_(id) {
    host_ =
        static_cast<const GFHostApi*>(Module::ModuleSdkMintHostApi(id, mask));
  }
  ~Granted() { Module::ModuleSdkReleaseHostApi(id_); }

  Granted(const Granted&) = delete;
  auto operator=(const Granted&) -> Granted& = delete;

  [[nodiscard]] auto host() const -> const GFHostApi* { return host_; }
  [[nodiscard]] auto id() const -> const char* { return id_; }

 private:
  const char* id_;
  const GFHostApi* host_ = nullptr;
};

/// Run @p work on a thread of its own and wait for it. Nothing the host did
/// is on that thread's stack, which is the whole point.
template <typename F>
void OnItsOwnThread(F work) {
  class Worker : public QThread {
   public:
    explicit Worker(F w) : work_(std::move(w)) {}
    void run() override { work_(); }

   private:
    F work_;
  };

  Worker worker(std::move(work));
  worker.start();
  ASSERT_TRUE(worker.wait(30000));
}

}  // namespace

// ------------------------------------------------------------- vocabulary

TEST(ModuleCapabilityTest, TheVocabularyIsSplitByWhatCanBeEnforced) {
  // Enforceable: there is a host api group behind it that can be withheld.
  for (const auto& name : Module::EnforceableCapabilityNames()) {
    EXPECT_EQ(Module::ModuleCapabilityKindOf(name),
              Module::ModuleCapabilityKind::kENFORCEABLE)
        << name.toStdString();
    EXPECT_NE(Module::ModuleCapabilityMask({name}), 0U)
        << name.toStdString() << " is enforceable but contributes no bit";
  }

  // Advisory: recorded, signed, shown -- and contributing NOTHING to the
  // grant. A mask that claimed a capability the host cannot withhold would
  // be a permission that does not exist.
  for (const auto& name : Module::AdvisoryCapabilityNames()) {
    EXPECT_EQ(Module::ModuleCapabilityKindOf(name),
              Module::ModuleCapabilityKind::kADVISORY)
        << name.toStdString();
    EXPECT_EQ(Module::ModuleCapabilityMask({name}), 0U)
        << name.toStdString() << " is advisory but contributes a bit";
  }

  EXPECT_TRUE(Module::AdvisoryCapabilityNames().contains("network"))
      << "network is not host-mediated: a module opens a socket through Qt, "
         "so there is nothing for the host to withhold";
}

TEST(ModuleCapabilityTest, AnUnknownNameIsNeitherKindAndGrantsNothing) {
  EXPECT_EQ(Module::ModuleCapabilityKindOf("telepathy"),
            Module::ModuleCapabilityKind::kUNKNOWN);
  // It contributes nothing -- which is exactly why the manifest parser
  // REFUSES it rather than letting it through: a typo would otherwise
  // silently narrow a module's access instead of failing.
  EXPECT_EQ(Module::ModuleCapabilityMask({"telepathy"}), 0U);
  EXPECT_TRUE(Module::AdvisoryDeclarationsOf({"telepathy"}).isEmpty());
}

TEST(ModuleCapabilityTest, AMixedDeclarationIsSplitNotFlattened) {
  const QStringList declared{"gpg", "network", "ui"};

  EXPECT_EQ(Module::ModuleCapabilityMask(declared),
            static_cast<uint32_t>(Module::ModuleCapability::kGPG) |
                static_cast<uint32_t>(Module::ModuleCapability::kUI));
  EXPECT_EQ(Module::AdvisoryDeclarationsOf(declared), QStringList{"network"});
}

TEST(ModuleCapabilityTest, AMaskReadsBackAsTheNamesItCameFrom) {
  EXPECT_EQ(Module::ModuleCapabilityMaskToString(0), "none");
  EXPECT_EQ(Module::ModuleCapabilityMaskToString(
                Module::ModuleCapabilityMask({"ui", "gpg"})),
            "gpg, ui");
}

// The C++ vocabulary and the CMake one are two copies of the same list, and
// a module.json is checked against the CMake copy at configure time and the
// C++ copy at load time. They disagreeing is a package that builds and will
// not load.
TEST(ModuleCapabilityTest, TheCmakeVocabularyMatchesTheHostsOwn) {
  const auto registry =
      QString(GF_TEST_SOURCE_DIR) + "/cmake/ModuleRegistry.cmake";
  QFile file(registry);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text))
      << "cannot read " << registry.toStdString();
  const auto text = QString::fromUtf8(file.readAll());

  const auto names_after = [&text](const QString& variable) -> QStringList {
    const QRegularExpression re(
        QString(R"(set\(%1\s+([^)]*)\))").arg(variable));
    const auto match = re.match(text);
    if (!match.hasMatch()) return {};
    auto names = match.captured(1).split(QRegularExpression(R"(\s+)"),
                                         Qt::SkipEmptyParts);
    names.sort();
    return names;
  };

  EXPECT_EQ(names_after("GF_MODULE_ENFORCEABLE_CAPABILITIES"),
            Module::EnforceableCapabilityNames());
  EXPECT_EQ(names_after("GF_MODULE_ADVISORY_CAPABILITIES"),
            Module::AdvisoryCapabilityNames());
}

// ------------------------------------------------------- minted grants

TEST(ModuleCapabilityTest, AWithheldGroupIsAbsentFromTheMintedTable) {
  const Granted ui_only("com.example.cap.uionly",
                        Module::ModuleCapabilityMask({"ui"}));
  ASSERT_NE(ui_only.host(), nullptr);

  EXPECT_NE(ui_only.host()->ui, nullptr);
  EXPECT_EQ(ui_only.host()->gpg, nullptr);
  EXPECT_EQ(ui_only.host()->storage, nullptr);
  EXPECT_EQ(ui_only.host()->process, nullptr);
}

// Absence from the table is the visible half. This is the other half: the
// primitives themselves refuse a context that does not hold the capability,
// so obtaining a group pointer some other way buys nothing.
TEST(ModuleCapabilityTest, APrimitiveRefusesAContextWithoutTheCapability) {
  const Granted with_gpg("com.example.cap.withgpg",
                         Module::ModuleCapabilityMask({"gpg"}));
  const Granted without("com.example.cap.without", 0);
  ASSERT_NE(with_gpg.host(), nullptr);
  ASSERT_NE(without.host(), nullptr);
  ASSERT_NE(with_gpg.host()->gpg, nullptr);

  // The gpg group, reached with a context that was not granted gpg. This is
  // the shape of every bypass: a pointer from somewhere, used with the wrong
  // authority.
  GFGpgResultRef result = nullptr;
  EXPECT_NE(with_gpg.host()->gpg->decrypt(without.host()->context, 0, nullptr,
                                          &result),
            0);
  EXPECT_EQ(result, nullptr);
}

// -------------------------------------------------- the cross-thread pair
//
// Read these two together. A thread-local implementation of authorization
// fails one of them whichever way it errs, which is what makes them a test
// of WHERE the answer comes from rather than of what the answer is.

TEST(ModuleCapabilityTest, AGrantHoldsOnTheModulesOwnThread) {
  const Granted granted("com.example.cap.thread.yes",
                        Module::ModuleCapabilityMask({"storage"}));
  ASSERT_NE(granted.host(), nullptr);
  ASSERT_NE(granted.host()->buffer, nullptr);

  const auto* host = granted.host();
  bool served = false;

  OnItsOwnThread([host, &served]() {
    // No host frame anywhere on this stack: the thread was started by the
    // test, exactly as a module starts its own workers. A buffer allocated
    // here must still be served, and must still be attributed.
    auto* buf = host->buffer->new_from_bytes(host->context, "x", 1);
    served = buf != nullptr;
    if (buf != nullptr) host->buffer->release(host->context, buf);
  });

  EXPECT_TRUE(served)
      << "authorization must come from the context, not from the call stack";
}

TEST(ModuleCapabilityTest, ADenialHoldsOnTheModulesOwnThread) {
  const Granted with_gpg("com.example.cap.thread.src",
                         Module::ModuleCapabilityMask({"gpg"}));
  const Granted without("com.example.cap.thread.no", 0);
  ASSERT_NE(with_gpg.host(), nullptr);
  ASSERT_NE(without.host(), nullptr);

  const auto* gpg = with_gpg.host()->gpg;
  auto* ctx = without.host()->context;
  ASSERT_NE(gpg, nullptr);

  int rc = 0;
  OnItsOwnThread([gpg, ctx, &rc]() {
    GFGpgResultRef result = nullptr;
    rc = gpg->decrypt(ctx, 0, nullptr, &result);
  });

  EXPECT_NE(rc, 0) << "a module without the capability must be refused on "
                      "its own threads too, where no host frame exists to "
                      "consult";
}

// ------------------------------------------------------------ lifetime

TEST(ModuleCapabilityTest, ReleasingTheGrantRefusesLaterCalls) {
  const auto* host = static_cast<const GFHostApi*>(Module::ModuleSdkMintHostApi(
      "com.example.cap.released", Module::ModuleCapabilityMask({"gpg"})));
  ASSERT_NE(host, nullptr);
  ASSERT_NE(host->gpg, nullptr);

  Module::ModuleSdkReleaseHostApi("com.example.cap.released");

  // A thread the module failed to stop, arriving after unload. The table is
  // still mapped on purpose -- freeing it would turn a refusable call into a
  // read of freed memory -- but the grant behind it is gone.
  GFGpgResultRef result = nullptr;
  EXPECT_NE(host->gpg->decrypt(host->context, 0, nullptr, &result), 0);
  EXPECT_EQ(host->buffer->new_from_bytes(host->context, "x", 1), nullptr);
}

// ----------------------------------------------- the one host path
//
// There is no second path to remove a test for. The exported GF* host
// implementations are gone, along with the thread-local fallback that used to
// guard them: authorization is the context, everywhere, and a module that
// reaches a primitive some other way still presents a context the host either
// recognises or does not.

}  // namespace GpgFrontend::Test
