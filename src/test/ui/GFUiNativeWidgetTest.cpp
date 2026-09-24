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

#include <QCoreApplication>
#include <QDialog>
#include <QPointer>
#include <QWidget>

#include "GpgFrontendTest.h"
#include "core/SdkTestContext.h"
#include "core/model/SettingsObject.h"
#include "sdk/GFSDKUI.h"
#include "ui/command/ModuleUiTeardown.h"
#include "ui/lua/LuaMounts.h"
#include "ui/lua/NativeInstances.h"
#include "ui/lua/NativeWidgetRegistry.h"
#include "ui/struct/settings_object/WindowStateSO.h"

/**
 * @file GFUiNativeWidgetTest.cpp
 * @brief Native widgets: typed interfaces only, and each instance answers
 *        to the module that owns it and to nobody else.
 *
 * A QWidget may only be built on the GUI thread, and these tests do not run
 * there; where an instance is needed it is built and destroyed there with a
 * blocking call, which a test thread -- unlike a module's -- may make.
 */

namespace GpgFrontend::Test {

namespace {

constexpr auto kOwner = "com.example.native.owner";
constexpr auto kOther = "com.example.native.other";

auto OnGui(const std::function<void()>& fn) {
  QMetaObject::invokeMethod(QCoreApplication::instance(), fn,
                            Qt::BlockingQueuedConnection);
}

auto CreateWidget(void*, uint64_t, GFBufferView) -> void* {
  return new QWidget();  // only ever called on the GUI thread here
}

/// What a module that builds its widget as a QDialog hands over.
auto CreateDialogWidget(void*, uint64_t, GFBufferView) -> void* {
  return new QDialog();
}

void DocLoad(void*, uint64_t, GFBufferView) {}
void DlgOpened(void*, uint64_t, GFBufferView) {}

const GFDocumentWidgetOps kDoc = {sizeof(GFDocumentWidgetOps), &DocLoad};
const GFDialogWidgetOps kDlg = {sizeof(GFDialogWidgetOps), &DlgOpened};

auto Spec(const char* name, int kind) -> GFNativeWidgetSpec {
  GFNativeWidgetSpec s{};
  s.struct_size = sizeof(s);
  s.name = name;
  s.kind = kind;
  s.create = &CreateWidget;
  s.document = kind == GF_NATIVE_DOCUMENT ? &kDoc : nullptr;
  s.dialog = kind == GF_NATIVE_DIALOG ? &kDlg : nullptr;
  return s;
}

/// Records what reached it.
class Recorder : public UI::NativeContainer {
 public:
  int modified = 0;
  int closed = 0;
  int withdrawn = 0;
  void OnModified() override { ++modified; }
  void OnClose() override { ++closed; }
  void OnWithdrawn() override { ++withdrawn; }
};

}  // namespace

TEST(NativeWidgetTest, OnlyTheOpsTheKindNeedsAreAccepted) {
  SdkTestContext owner(kOwner, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);

  auto wrong = Spec("mixed", GF_NATIVE_DOCUMENT);
  wrong.dialog = &kDlg;  // a document that also claims to be a dialog
  EXPECT_NE(GFNativeWidgetRegister(owner(), &wrong), 0);

  auto missing = Spec("bare", GF_NATIVE_DIALOG);
  missing.dialog = nullptr;
  EXPECT_NE(GFNativeWidgetRegister(owner(), &missing), 0);

  auto unknown = Spec("odd", 99);
  EXPECT_NE(GFNativeWidgetRegister(owner(), &unknown), 0);

  auto good = Spec("viewer", GF_NATIVE_DOCUMENT);
  ASSERT_EQ(GFNativeWidgetRegister(owner(), &good), 0);
  const auto entry =
      UI::NativeWidgetRegistry::Instance().Find(QString(kOwner) + ".viewer");
  ASSERT_TRUE(entry.has_value()) << "named inside the module's namespace";
  EXPECT_EQ(entry->owner, kOwner);
  EXPECT_EQ(entry->kind, UI::NativeWidgetKind::kDOCUMENT);
  EXPECT_TRUE(static_cast<bool>(entry->document.load));
  EXPECT_FALSE(static_cast<bool>(entry->document.save))
      << "an op the table leaves NULL is simply never asked";

  EXPECT_NE(GFNativeWidgetRegister(owner(), &good), 0) << "registered once";

  SdkTestContext other(kOther, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
  EXPECT_NE(GFNativeWidgetUnregister(other(), "viewer"), 0);
  EXPECT_EQ(GFNativeWidgetUnregister(owner(), "viewer"), 0);
}

TEST(NativeWidgetTest, NativeWidgetsNeedUiCustomAsWellAsUi) {
  SdkTestContext ui_only(kOwner, GF_HOST_CAP_UI);
  ASSERT_TRUE(ui_only.live());
  EXPECT_EQ(ui_only()->host->native, nullptr);
  auto spec = Spec("viewer", GF_NATIVE_DIALOG);
  EXPECT_NE(GFNativeWidgetRegister(ui_only(), &spec), 0);
}

TEST(NativeWidgetTest, AnInstanceAnswersOnlyToItsOwnerAndItsKind) {
  SdkTestContext owner(kOwner, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
  SdkTestContext other(kOther, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
  auto spec = Spec("dialog", GF_NATIVE_DIALOG);
  ASSERT_EQ(GFNativeWidgetRegister(owner(), &spec), 0);

  Recorder recorder;
  quint64 id = 0;
  OnGui([&] {
    const auto instance = UI::NativeInstances::Instance().Create(
        QString(kOwner) + ".dialog", {}, &recorder);
    ASSERT_TRUE(instance.has_value());
    id = instance->id;
  });
  ASSERT_NE(id, 0U);

  // Wrong module: refused. Wrong kind: refused. Owner and kind: delivered.
  OnGui([&] {
    EXPECT_NE(GFNativeDialogClose(other(), id), 0);
    EXPECT_NE(GFNativeDocumentModified(owner(), id), 0);
    EXPECT_EQ(GFNativeDialogClose(owner(), id), 0);
  });
  EXPECT_EQ(recorder.closed, 1);
  EXPECT_EQ(recorder.modified, 0);
  EXPECT_EQ(UI::NativeInstances::Instance().CountFor(kOwner), 1);

  OnGui([&] {
    auto instance = UI::NativeInstances::Instance().Find(id);
    ASSERT_TRUE(instance.has_value());
    delete instance->widget.data();
    UI::NativeInstances::Instance().Destroy(id);
  });
  EXPECT_EQ(UI::NativeInstances::Instance().CountFor(kOwner), 0);
  OnGui([&] {
    EXPECT_NE(GFNativeDialogClose(owner(), id), 0);
  });  // gone: nothing to address

  GFNativeWidgetUnregister(owner(), "dialog");
}

TEST(NativeWidgetTest, AWithdrawnModulesInstancesAreDroppedByTheirContainers) {
  // A module's widgets must not outlive it inside the Host's window, running
  // module code against state the module has dropped.
  constexpr auto kModule = "com.example.native.withdrawn";
  SdkTestContext owner(kModule, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
  auto spec = Spec("dialog", GF_NATIVE_DIALOG);
  ASSERT_EQ(GFNativeWidgetRegister(owner(), &spec), 0);

  Recorder first;
  Recorder second;
  QPointer<QWidget> widget;
  QPointer<QWidget> widget_b;
  OnGui([&] {
    auto a = UI::NativeInstances::Instance().Create(
        QString(kModule) + ".dialog", {}, &first);
    auto b = UI::NativeInstances::Instance().Create(
        QString(kModule) + ".dialog", {}, &second);
    ASSERT_TRUE(a.has_value() && b.has_value());
    widget = a->widget;
    widget_b = b->widget;
  });
  ASSERT_EQ(UI::NativeInstances::Instance().CountFor(kModule), 2);

  OnGui([&] { UI::NativeInstances::Instance().WithdrawAll(kModule); });
  EXPECT_EQ(first.withdrawn, 1);
  EXPECT_EQ(second.withdrawn, 1);
  EXPECT_EQ(UI::NativeInstances::Instance().CountFor(kModule), 0);

  // A container destroyed afterwards has nothing left to tell the module.
  OnGui([&] {
    delete widget.data();
    delete widget_b.data();
    UI::NativeInstances::Instance().WithdrawAll(kModule);  // idempotent
  });
  EXPECT_EQ(first.withdrawn, 1);
  GFNativeWidgetUnregister(owner(), "dialog");
}

TEST(NativeWidgetTest,
     AModuleDialogOpensAtItsDeclaredSizeAndThenWhereItWasLeft) {
  // The module's widget is a child of the Host's frame, not a window, so it
  // cannot remember its own geometry: the frame has to.
  constexpr auto kModule = "com.example.native.geometry";
  const auto widget_id = QString(kModule) + ".dialog";
  const auto state_name = UI::Lua::NativeDialogStateName(widget_id) +
                          QStringLiteral("_dialog_state");
  SdkTestContext owner(kModule, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
  auto spec = Spec("dialog", GF_NATIVE_DIALOG);
  spec.create = &CreateDialogWidget;
  spec.width = 900;
  spec.height = 650;
  ASSERT_EQ(GFNativeWidgetRegister(owner(), &spec), 0);
  OnGui([&] { SettingsObject(state_name).Store(QJsonObject{}); });

  const QRect left_at(40, 30, 700, 520);  // fits the offscreen screen
  OnGui([&] {
    auto* dialog = new UI::Lua::NativeDialog("main", widget_id, {}, nullptr);
    ASSERT_TRUE(dialog->Ok());
    EXPECT_GE(dialog->width(), 900);
    EXPECT_GE(dialog->height(), 650);
    dialog->show();
    dialog->setGeometry(left_at);

    // Escape and a Close button reach the module's QDialog, not the frame.
    // Hiding only itself would leave the frame open around nothing.
    auto* inner = dialog->findChild<QDialog*>();
    ASSERT_NE(inner, nullptr);
    inner->reject();
    EXPECT_FALSE(dialog->isVisible());
    delete dialog;
  });

  OnGui([&] {
    auto* dialog = new UI::Lua::NativeDialog("main", widget_id, {}, nullptr);
    ASSERT_TRUE(dialog->Ok());
    dialog->show();
    EXPECT_EQ(dialog->size(), left_at.size());
    delete dialog;
    SettingsObject(state_name).Store(QJsonObject{});
  });

  GFNativeWidgetUnregister(owner(), "dialog");
}

TEST(NativeWidgetTest, AWidgetNameIsOneDotlessPart) {
  // `owner.a.b` would sit in the namespace of a module called `owner.a`.
  SdkTestContext owner(kOwner, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
  auto spec = Spec("nested.name", GF_NATIVE_DIALOG);
  EXPECT_NE(GFNativeWidgetRegister(owner(), &spec), 0);
}

TEST(NativeWidgetTest, ModuleUiTeardownWithdrawsAtOnceFromAModuleThread) {
  // Deactivation runs on the module runner, not the GUI thread. What must
  // not wait for the GUI's queue -- the commands, the widget registrations --
  // is gone when the call returns, and the module is a closed caller until
  // it is reopened; a reactivation right behind it must not lose what it
  // registers to a removal still sitting in that queue.
  constexpr auto kModule = "com.example.native.teardown";
  SdkTestContext owner(kModule, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
  auto spec = Spec("dialog", GF_NATIVE_DIALOG);
  ASSERT_EQ(GFNativeWidgetRegister(owner(), &spec), 0);
  ASSERT_TRUE(UI::NativeWidgetRegistry::Instance()
                  .Find(QString(kModule) + ".dialog")
                  .has_value());

  UI::ModuleUiTeardown(kModule);
  EXPECT_FALSE(UI::NativeWidgetRegistry::Instance()
                   .Find(QString(kModule) + ".dialog")
                   .has_value());
  UI::ModuleUiTeardown(kModule);  // idempotent

  UI::ModuleUiReopen(kModule);
  ASSERT_EQ(GFNativeWidgetRegister(owner(), &spec), 0)
      << "a reopened module registers again";
  // Let the GUI half of the two teardowns run: it must not take the new
  // registration with it.
  OnGui([] {});
  EXPECT_TRUE(UI::NativeWidgetRegistry::Instance()
                  .Find(QString(kModule) + ".dialog")
                  .has_value());
  GFNativeWidgetUnregister(owner(), "dialog");
}

}  // namespace GpgFrontend::Test
