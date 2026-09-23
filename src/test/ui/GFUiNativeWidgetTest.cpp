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
#include <QWidget>

#include "GpgFrontendTest.h"
#include "core/SdkTestContext.h"
#include "sdk/GFSDKUI.h"
#include "ui/lua/NativeInstances.h"
#include "ui/lua/NativeWidgetRegistry.h"

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
  void OnModified() override { ++modified; }
  void OnClose() override { ++closed; }
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
  OnGui([&] { EXPECT_NE(GFNativeDialogClose(owner(), id), 0); })
      ;  // gone: nothing to address

  GFNativeWidgetUnregister(owner(), "dialog");
}

}  // namespace GpgFrontend::Test
