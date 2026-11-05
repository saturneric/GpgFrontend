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
#include <array>

#include "GpgFrontendTest.h"
#include "core/GpgCoreEngineTest.h"
#include "core/gpg/GpgCoreTest.h"
#include "core/model/SettingsObject.h"
#include "host/SdkTestContext.h"
#include "sdk/GFSDKBuffer.h"
#include "sdk/GFSDKBuffer.hpp"
#include "sdk/GFSDKGpg.h"
#include "sdk/GFSDKGpgResult.hpp"
#include "sdk/GFSDKUI.h"
#include "ui/command/ModuleUiTeardown.h"
#include "ui/lua/LuaMounts.h"
#include "ui/lua/NativeInstances.h"
#include "ui/lua/NativeWidgetRegistry.h"
#include "ui/struct/settings_object/WindowStateSO.h"
#include "ui/widgets/PlainTextEditorPage.h"

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

namespace {

/// What a module-persisted document view was asked.
struct PersistedLog {
  QString opened;
  int saves = 0;
  int save_as = 0;
  int closes = 0;
  int finds = 0;
  int bytes_asked = 0;
  int encrypts = 0;
  bool allow_close = true;
  bool can_encrypt = false;
};
PersistedLog g_persisted;

auto PersistedOpen(void*, uint64_t, const char* path) -> int {
  g_persisted.opened = QString::fromUtf8(path);
  return 0;
}
auto PersistedSave(void*, uint64_t, int save_as) -> int {
  (save_as != 0 ? g_persisted.save_as : g_persisted.saves)++;
  return 0;
}
auto PersistedClose(void*, uint64_t) -> int {
  ++g_persisted.closes;
  return g_persisted.allow_close ? 0 : 1;
}
void PersistedFind(void*, uint64_t) { ++g_persisted.finds; }
auto PersistedEncrypt(void*, uint64_t) -> int {
  ++g_persisted.encrypts;
  return 0;
}
auto PersistedCryptoOps(void*, uint64_t, uint32_t* out) -> int {
  *out = g_persisted.can_encrypt ? GF_CRYPTO_OP_ENCRYPT : 0U;
  return 0;
}
auto PersistedBytes(void*, uint64_t) -> GFBufferRef {
  ++g_persisted.bytes_asked;
  return nullptr;
}
auto PersistedDirty(void*, uint64_t) -> int { return 1; }

GFDocumentWidgetOps PersistedOps() {
  GFDocumentWidgetOps ops{};
  ops.struct_size = sizeof(ops);
  ops.load = &DocLoad;
  ops.save = &PersistedBytes;
  ops.is_dirty = &PersistedDirty;
  ops.open_path = &PersistedOpen;
  ops.save_requested = &PersistedSave;
  ops.close_requested = &PersistedClose;
  ops.find_requested = &PersistedFind;
  ops.encrypt_requested = &PersistedEncrypt;
  ops.crypto_ops = &PersistedCryptoOps;
  return ops;
}

}  // namespace

// persistence = "module": the page holds nothing of the document and hands
// everything about its file to the view.
TEST(NativeWidgetTest, AModulePersistedDocumentIsTheViewsToOpenSaveAndClose) {
  g_persisted = {};
  SdkTestContext owner(kOwner, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
  static const auto kOps = PersistedOps();
  auto spec = Spec("persisted", GF_NATIVE_DOCUMENT);
  spec.document = &kOps;
  ASSERT_EQ(GFNativeWidgetRegister(owner(), &spec), 0);

  UI::PlainTextEditorPage* page = nullptr;
  OnGui([&] {
    page = new UI::PlainTextEditorPage();
    ASSERT_TRUE(page->MountNativeView(QString(kOwner) + ".persisted"));
    page->SetModulePersisted(true);
  });
  ASSERT_NE(page, nullptr);

  QStringList saved;
  OnGui([&] {
    QObject::connect(page, &UI::PlainTextEditorPage::SignalModuleSaved, page,
                     [&saved](const QString& path, const QString& title) {
                       saved << path << title;
                     });
    EXPECT_TRUE(page->ModuleOpenPath(u"/tmp/a.textpack.gpg"_s));
    EXPECT_EQ(page->GetFilePath(), u"/tmp/a.textpack.gpg"_s);

    EXPECT_TRUE(page->ModuleSaveRequested(false));
    EXPECT_TRUE(page->ModuleSaveRequested(true));
    page->ModuleFindRequested();

    // Asked like any view, for what the recovery cache keeps of unsaved
    // work; the file itself is still the view's own business.
    page->FlushPrimaryView();

    // No Host crypto on a document it does not hold: Encrypt alone, handed
    // to the view, and only while the view says it can.
    bool has_opinion = false;
    g_persisted.can_encrypt = false;
    EXPECT_TRUE(page->PrimaryViewCryptoOperations(has_opinion).isEmpty());
    EXPECT_TRUE(has_opinion);
    g_persisted.can_encrypt = true;
    EXPECT_EQ(page->PrimaryViewCryptoOperations(has_opinion),
              QStringList{u"encrypt"_s});
    EXPECT_TRUE(page->ModuleEncryptRequested());

    // A table without the newer hooks: Decrypt is not the view's, the
    // actions keep their tooltips, and Save is offered.
    EXPECT_FALSE(page->ModuleDecryptRequested());
    EXPECT_FALSE(page->ModuleCryptoTip(GF_CRYPTO_OP_DECRYPT).has_value());
    EXPECT_TRUE(page->ModuleSaveAllowed());

    g_persisted.allow_close = false;
    EXPECT_FALSE(page->ModuleCloseRequested());
    g_persisted.allow_close = true;
    EXPECT_TRUE(page->ModuleCloseRequested());

    // Every edit the view reports reaches the recovery cache, not only the
    // first one that made the tab dirty.
    int edits = 0;
    QObject::connect(page, &UI::PlainTextEditorPage::SignalPrimaryViewEdited,
                     page, [&edits] { ++edits; });
    EXPECT_EQ(GFNativeDocumentModified(owner(), page->NativeInstanceId()), 0);
    EXPECT_EQ(GFNativeDocumentModified(owner(), page->NativeInstanceId()), 0);
    EXPECT_EQ(edits, 2);

    EXPECT_EQ(GFNativeDocumentSaved(owner(), page->NativeInstanceId(),
                                    "/tmp/b.textpack.gpg", "b"),
              0);
  });
  EXPECT_EQ(g_persisted.opened, u"/tmp/a.textpack.gpg"_s);
  EXPECT_EQ(g_persisted.saves, 1);
  EXPECT_EQ(g_persisted.save_as, 1);
  EXPECT_EQ(g_persisted.finds, 1);
  EXPECT_EQ(g_persisted.encrypts, 1);
  EXPECT_EQ(g_persisted.closes, 2);
  EXPECT_EQ(g_persisted.bytes_asked, 1);
  EXPECT_EQ(saved, (QStringList{u"/tmp/b.textpack.gpg"_s, u"b"_s}));

  OnGui([&] {
    EXPECT_EQ(page->GetFilePath(), u"/tmp/b.textpack.gpg"_s);
    delete page;
  });
  GFNativeWidgetUnregister(owner(), "persisted");
}

// A view's own crypto operation reaches the Status Panel as the Host's own
// Encrypt does: analysed by the Host, with what became of the output.
TEST_F(GpgCoreTest, NativeViewCryptoResultIsReportedAsTheHostsOwn) {
  if (!ChannelIsUsable(kGpgFrontendDefaultChannel)) {
    GTEST_SKIP() << "no live default channel in this profile";
  }
  SdkTestContext owner(
      kOwner, GF_HOST_CAP_GPG | GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
  SdkTestContext other(kOther, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
  static const auto kOps = PersistedOps();
  auto spec = Spec("reporting", GF_NATIVE_DOCUMENT);
  spec.document = &kOps;
  ASSERT_EQ(GFNativeWidgetRegister(owner(), &spec), 0);

  UI::PlainTextEditorPage* page = nullptr;
  OnGui([&] {
    page = new UI::PlainTextEditorPage();
    ASSERT_TRUE(page->MountNativeView(QString(kOwner) + ".reporting"));
    page->SetModulePersisted(true);
  });
  ASSERT_NE(page, nullptr);

  QList<std::pair<QList<GpgFrontend::GpgOperaResult>,
                  GpgFrontend::InfoBoardBatchContext>>
      posted;
  OnGui([&] {
    QObject::connect(
        page, &UI::PlainTextEditorPage::SignalModuleResult, page,
        [&posted](const QList<GpgFrontend::GpgOperaResult>& results,
                  const GpgFrontend::InfoBoardBatchContext& context) {
          posted.append({results, context});
        });
    const auto id = page->NativeInstanceId();

    // Refused before OpenPGP ran: no capsule, the view's reason.
    GFDocumentCryptoResult refused{};
    refused.struct_size = sizeof(refused);
    refused.channel = kGpgFrontendDefaultChannel;
    refused.operation = GF_GPG_ANALYSE_ENCRYPT;
    refused.input_hash = "abc123";
    refused.output = GF_DOCUMENT_OUTPUT_NOT_PRODUCED;
    refused.reason = "a reader's key is missing";

    EXPECT_NE(GFNativeDocumentResult(other(), id, &refused), 0)
        << "another module's view";
    EXPECT_NE(GFNativeDocumentResult(owner(), id, nullptr), 0);
    auto bad_channel = refused;
    bad_channel.channel = 987654;
    EXPECT_NE(GFNativeDocumentResult(owner(), id, &bad_channel), 0)
        << "a channel that is not live";
    auto bad_op = refused;
    bad_op.operation = 42;
    EXPECT_NE(GFNativeDocumentResult(owner(), id, &bad_op), 0);

    EXPECT_EQ(GFNativeDocumentResult(owner(), id, &refused), 0);

    // Produced, but the view could not use it.
    auto unused = refused;
    unused.operation = GF_GPG_ANALYSE_DECRYPT;
    unused.capsule_id = "no-such-capsule";
    unused.output = GF_DOCUMENT_OUTPUT_APPLY_FAILED;
    unused.reason = "not a note";
    EXPECT_EQ(GFNativeDocumentResult(owner(), id, &unused), 0);
  });

  ASSERT_EQ(posted.size(), 2);
  const auto& [first, first_context] = posted[0];
  ASSERT_EQ(first.size(), 1);
  EXPECT_EQ(first[0].op_info.op, GpgOperation::kENCRYPT);
  EXPECT_LT(first[0].op_info.status, 0);
  EXPECT_EQ(first[0].op_info.inputHash, u"abc123"_s);
  EXPECT_EQ(first[0].input_index, 0);
  EXPECT_TRUE(first_context.document_target);
  EXPECT_EQ(first_context.fates.value(0),
            GpgFrontend::OutputFate::kNOT_PRODUCED);
  bool reason_kept = false;
  for (const auto& e : first[0].op_info.stageErrors) {
    for (const auto& d : e.engineDetails) {
      reason_kept |= d.second == u"a reader's key is missing"_s;
    }
  }
  EXPECT_TRUE(reason_kept);

  const auto& [second, second_context] = posted[1];
  ASSERT_EQ(second.size(), 1);
  EXPECT_EQ(second[0].op_info.op, GpgOperation::kDECRYPT);
  EXPECT_EQ(second_context.fates.value(0),
            GpgFrontend::OutputFate::kAPPLY_FAILED);
  ASSERT_EQ(second_context.output_errors.value(0).size(), 1);
  EXPECT_EQ(second_context.output_errors.value(0).first().message,
            u"not a note"_s);

  OnGui([&] { delete page; });
  GFNativeWidgetUnregister(owner(), "reporting");
}

// A real operation's capsule: the Host analyses it, on either engine, exactly
// as it analyses its own Encrypt -- and only for the module it was made for.
TEST_P(GpgCoreEngineTest, NativeViewEncryptResultIsAnalysedByTheHost) {
  SdkTestContext owner(
      kOwner, GF_HOST_CAP_GPG | GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
  SdkTestContext other(kOther, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
  static const auto kOps = PersistedOps();
  auto spec = Spec("engine_reporting", GF_NATIVE_DOCUMENT);
  spec.document = &kOps;
  ASSERT_EQ(GFNativeWidgetRegister(owner(), &spec), 0);

  auto key = GenerateFullKey("native_view_report");
  ASSERT_TRUE(key != nullptr);
  const auto key_id = key->ID().toUtf8();
  const std::array<const char*, 1> ids{key_id.constData()};
  QString capsule;
  {
    auto in = GFBuf::Copy(owner(), QByteArray("note"));
    GFGpgResult r(owner());
    ASSERT_EQ(GFGpgEncrypt(owner(), Channel(), ids.data(), ids.size(),
                           in.View(), 0, r.Out()),
              GF_GPG_OK);
    capsule = r.CapsuleId();
  }  // released: the capsule outlives it, as the SDK promises
  ASSERT_FALSE(capsule.isEmpty());

  UI::PlainTextEditorPage* page = nullptr;
  QList<GpgFrontend::GpgOperaResult> posted;
  GpgFrontend::InfoBoardBatchContext context;
  OnGui([&] {
    page = new UI::PlainTextEditorPage();
    ASSERT_TRUE(page->MountNativeView(QString(kOwner) + ".engine_reporting"));
    page->SetModulePersisted(true);
    QObject::connect(page, &UI::PlainTextEditorPage::SignalModuleResult, page,
                     [&](const QList<GpgFrontend::GpgOperaResult>& results,
                         const GpgFrontend::InfoBoardBatchContext& c) {
                       posted += results;
                       context = c;
                     });

    const auto capsule_id = capsule.toUtf8();
    GFDocumentCryptoResult done{};
    done.struct_size = sizeof(done);
    done.channel = Channel();
    done.operation = GF_GPG_ANALYSE_ENCRYPT;
    done.capsule_id = capsule_id.constData();
    done.input_hash = "feed";
    done.output = GF_DOCUMENT_OUTPUT_APPLIED;
    EXPECT_NE(GFNativeDocumentResult(other(), page->NativeInstanceId(), &done),
              0);
    EXPECT_EQ(GFNativeDocumentResult(owner(), page->NativeInstanceId(), &done),
              0);
  });
  ASSERT_NE(page, nullptr);

  ASSERT_EQ(posted.size(), 1);
  const auto& info = posted.first().op_info;
  EXPECT_EQ(info.op, GpgOperation::kENCRYPT);
  EXPECT_GT(info.status, 0) << "analysed, not reported as stopped";
  EXPECT_FALSE(info.engine.isEmpty());
  EXPECT_TRUE(info.stageErrors.isEmpty()) << "a stopped one records why";
  EXPECT_EQ(info.inputHash, u"feed"_s);
  EXPECT_EQ(context.fates.value(0), GpgFrontend::OutputFate::kAPPLIED);

  OnGui([&] { delete page; });
  GFNativeWidgetUnregister(owner(), "engine_reporting");
  DeleteKey(key);
}

namespace {

/// A note-like view: encrypted (Decrypt opens it, nothing to save) or open
/// with an encrypted snapshot (Decrypt takes the encryption off).
struct CryptoViewLog {
  GFSDKContext* ctx = nullptr;
  bool encrypted = true;
  int decrypts = 0;
};
CryptoViewLog g_crypto_view;

auto CryptoViewOps(void*, uint64_t, uint32_t* out) -> int {
  *out = g_crypto_view.encrypted ? GF_CRYPTO_OP_DECRYPT
                                 : GF_CRYPTO_OP_ENCRYPT | GF_CRYPTO_OP_DECRYPT;
  return 0;
}
auto CryptoViewDecrypt(void*, uint64_t) -> int {
  ++g_crypto_view.decrypts;
  return 0;
}
auto CryptoViewTip(void*, uint64_t, uint32_t op, GFBufferRef* out) -> int {
  if (g_crypto_view.encrypted || op != GF_CRYPTO_OP_DECRYPT) return 1;
  static constexpr std::string_view kTip = "Remove encryption";
  *out = GFBufferNewFromBytes(g_crypto_view.ctx, kTip.data(), kTip.size());
  return 0;
}
auto CryptoViewSaveAllowed(void*, uint64_t) -> int {
  return g_crypto_view.encrypted ? 0 : 1;
}

GFDocumentWidgetOps CryptoViewTable() {
  auto ops = PersistedOps();
  ops.crypto_ops = &CryptoViewOps;
  ops.decrypt_requested = &CryptoViewDecrypt;
  ops.crypto_op_tip = &CryptoViewTip;
  ops.save_allowed = &CryptoViewSaveAllowed;
  return ops;
}

}  // namespace

// A view that persists its document may give Decrypt its own meaning, say
// what the toolbar action does there, and grey out Save while there is
// nothing to save.
TEST(NativeWidgetTest, AModulePersistedViewOwnsDecryptItsTipAndSave) {
  g_crypto_view = {};
  SdkTestContext owner(kOwner, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
  g_crypto_view.ctx = owner();
  static const auto kOps = CryptoViewTable();
  auto spec = Spec("crypto_view", GF_NATIVE_DOCUMENT);
  spec.document = &kOps;
  ASSERT_EQ(GFNativeWidgetRegister(owner(), &spec), 0);

  OnGui([&] {
    auto* page = new UI::PlainTextEditorPage();
    ASSERT_TRUE(page->MountNativeView(QString(kOwner) + ".crypto_view"));
    page->SetModulePersisted(true);

    bool has_opinion = false;
    EXPECT_EQ(page->PrimaryViewCryptoOperations(has_opinion),
              QStringList{u"decrypt"_s});
    EXPECT_FALSE(page->ModuleSaveAllowed());
    EXPECT_FALSE(page->ModuleCryptoTip(GF_CRYPTO_OP_DECRYPT).has_value());
    EXPECT_TRUE(page->ModuleDecryptRequested());

    g_crypto_view.encrypted = false;
    EXPECT_EQ(page->PrimaryViewCryptoOperations(has_opinion),
              (QStringList{u"encrypt"_s, u"decrypt"_s}));
    EXPECT_TRUE(page->ModuleSaveAllowed());
    EXPECT_EQ(page->ModuleCryptoTip(GF_CRYPTO_OP_DECRYPT),
              std::optional<QString>(u"Remove encryption"_s));
    EXPECT_FALSE(page->ModuleCryptoTip(GF_CRYPTO_OP_ENCRYPT).has_value());
    delete page;
  });
  EXPECT_EQ(g_crypto_view.decrypts, 1);
  GFNativeWidgetUnregister(owner(), "crypto_view");
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
  const auto state_name =
      UI::Lua::NativeDialogStateName(widget_id) + u"_dialog_state"_s;
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
