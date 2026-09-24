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

#include "GFModuleRuntimeNative.h"

#include <QCborValue>
#include <QCoreApplication>
#include <QFont>
#include <QHash>
#include <QPointer>
#include <QThread>
#include <QWidget>
#include <list>

#include "GFModuleRuntimeBoot.h"
#include "include/GFModule.h"
#include "include/GFModuleNativeWidget.h"

/**
 * @file GFModuleRuntimeNative.cpp
 * @brief The typed native widget interfaces, as C ops tables.
 *
 * A module writes a class with named virtual methods; this file is the C
 * table the Host calls, which finds the instance and calls the method. The
 * Host never learns the class, and the module never learns the container.
 */

namespace gf::ui {

struct NativeWidgetAccess {
  static void SetInstance(NativeWidgetBase* base, quint64 instance) {
    base->instance_ = instance;
  }
};

}  // namespace gf::ui

namespace {

using gf::ui::DialogWidget;
using gf::ui::DocumentWidget;
using gf::ui::NativeWidgetBase;
using gf::ui::SettingsWidget;

struct Registration {
  QByteArray name, title, keywords, suffix, filter, icon;
  gf::ui::detail::MakeFn make;
  GFNativeWidgetSpec spec{};
};

struct Live {
  QPointer<QWidget> widget;
  NativeWidgetBase* base = nullptr;
};

/// Stable addresses: the Host holds `user` pointers into this list.
auto Registrations() -> std::list<Registration>& {
  static std::list<Registration> r;
  return r;
}

auto Instances() -> QHash<quint64, Live>& {
  static QHash<quint64, Live> i;
  return i;
}

template <typename T>
auto As(uint64_t instance) -> T* {
  const auto it = Instances().constFind(instance);
  if (it == Instances().constEnd() || it->widget.isNull()) return nullptr;
  return dynamic_cast<T*>(it->base);
}

auto Bytes(GFBufferView view) -> QByteArray {
  auto* ctx = gf::runtime::SdkContext();
  const auto* data = static_cast<const char*>(GFBufferData(ctx, view));
  return data == nullptr
             ? QByteArray()
             : QByteArray(data, static_cast<int>(GFBufferSize(ctx, view)));
}

auto Buffer(const QByteArray& bytes) -> GFBufferRef {
  return GFBufferNewFromBytes(gf::runtime::SdkContext(), bytes.constData(),
                              static_cast<size_t>(bytes.size()));
}

// --- lifecycle

auto Create(void* user, uint64_t instance, GFBufferView args) -> void* {
  auto* reg = static_cast<Registration*>(user);
  NativeWidgetBase* base = nullptr;
  auto* widget = reg->make(QCborValue::fromCbor(Bytes(args)).toMap(), &base);
  if (widget == nullptr || base == nullptr) return nullptr;
  gf::ui::NativeWidgetAccess::SetInstance(base, instance);
  Instances().insert(instance, Live{widget, base});
  return widget;
}

void Destroyed(void* /*user*/, uint64_t instance) {
  Instances().remove(instance);
}

// --- documents

void DocLoad(void*, uint64_t i, GFBufferView bytes) {
  if (auto* d = As<DocumentWidget>(i)) d->Load(Bytes(bytes));
}
auto DocSave(void*, uint64_t i) -> GFBufferRef {
  auto* d = As<DocumentWidget>(i);
  if (d == nullptr) return nullptr;
  const auto saved = d->Save();
  return saved.has_value() ? Buffer(*saved) : nullptr;
}
auto DocIsDirty(void*, uint64_t i) -> int {
  auto* d = As<DocumentWidget>(i);
  return d != nullptr && d->IsDirty() ? 1 : 0;
}
auto DocCryptoOps(void*, uint64_t i, uint32_t* out) -> int {
  auto* d = As<DocumentWidget>(i);
  if (d == nullptr || out == nullptr) return -1;
  const auto ops = d->CryptoOperations();
  if (!ops.has_value()) return -1;
  *out = *ops;
  return 0;
}
auto DocSuggestedFileName(void*, uint64_t i) -> GFBufferRef {
  auto* d = As<DocumentWidget>(i);
  if (d == nullptr) return nullptr;
  const auto name = d->SuggestedFileName();
  return name.isEmpty() ? nullptr : Buffer(name.toUtf8());
}
void DocApplyVerification(void*, uint64_t i, GFBufferView json) {
  if (auto* d = As<DocumentWidget>(i)) d->ApplyVerification(Bytes(json));
}
auto DocAppendText(void*, uint64_t i, GFBufferView utf8) -> int {
  auto* d = As<DocumentWidget>(i);
  return d != nullptr && d->AppendText(QString::fromUtf8(Bytes(utf8))) ? 1 : 0;
}
auto DocAttachPublicKey(void*, uint64_t i, GFBufferView key, const char* name)
    -> int {
  auto* d = As<DocumentWidget>(i);
  return d != nullptr && d->AttachPublicKey(Bytes(key), QString::fromUtf8(name))
             ? 1
             : 0;
}
void DocApplyFont(void*, uint64_t i, const char* family, int size) {
  if (auto* d = As<DocumentWidget>(i)) {
    d->ApplyFont(QFont(QString::fromUtf8(family), size));
  }
}
void DocWipe(void*, uint64_t i) {
  if (auto* d = As<DocumentWidget>(i)) d->WipeContent();
}
auto DocPrepareSave(void*, uint64_t i, GFBufferView bytes, GFBufferRef* out)
    -> int {
  if (out != nullptr) *out = nullptr;
  auto* d = As<DocumentWidget>(i);
  if (d == nullptr) return 0;
  const auto decision = d->PrepareSave(Bytes(bytes));
  if (decision.cancelled) return 1;
  if (decision.bytes.has_value() && out != nullptr)
    *out = Buffer(*decision.bytes);
  return 0;
}

auto DocSourcePolicy(void*, uint64_t i, GFBufferRef* reason) -> int {
  if (reason != nullptr) *reason = nullptr;
  auto* d = As<DocumentWidget>(i);
  if (d == nullptr) return 1;
  const auto why = d->SourceLockReason();
  if (!why.has_value()) return 1;
  if (reason != nullptr) *reason = Buffer(why->toUtf8());
  return 0;
}

const GFDocumentWidgetOps kDocumentOps = {
    sizeof(GFDocumentWidgetOps),
    &DocLoad,
    &DocSave,
    &DocIsDirty,
    &DocCryptoOps,
    &DocSuggestedFileName,
    &DocApplyVerification,
    &DocAppendText,
    &DocAttachPublicKey,
    &DocApplyFont,
    &DocWipe,
    &DocPrepareSave,
    &DocSourcePolicy,
};

// --- settings

void SetLoad(void*, uint64_t i) {
  if (auto* s = As<SettingsWidget>(i)) s->LoadSettings();
}
auto SetApply(void*, uint64_t i) -> int {
  auto* s = As<SettingsWidget>(i);
  return s == nullptr || s->ApplySettings() ? 0 : -1;
}

const GFSettingsWidgetOps kSettingsOps = {sizeof(GFSettingsWidgetOps), &SetLoad,
                                          &SetApply};

// --- dialogs

void DlgOpened(void*, uint64_t i, GFBufferView args) {
  if (auto* d = As<DialogWidget>(i)) {
    d->Opened(QCborValue::fromCbor(Bytes(args)).toMap());
  }
}
auto DlgCloseRequested(void*, uint64_t i) -> int {
  auto* d = As<DialogWidget>(i);
  return d == nullptr || d->CloseRequested() ? 1 : 0;
}

const GFDialogWidgetOps kDialogOps = {sizeof(GFDialogWidgetOps), &DlgOpened,
                                      &DlgCloseRequested};

}  // namespace

namespace gf::ui {

void DocumentWidget::NotifyModified() {
  GFNativeDocumentModified(gf::runtime::SdkContext(), Instance());
}
void DocumentWidget::ShowSource(bool source) {
  GFNativeDocumentShowSource(gf::runtime::SdkContext(), Instance(),
                             source ? 1 : 0);
}
void DocumentWidget::RequestCrypto(uint32_t op) {
  GFNativeDocumentRequestCrypto(gf::runtime::SdkContext(), Instance(), op);
}
void DocumentWidget::NotifyOpsChanged() {
  GFNativeDocumentOpsChanged(gf::runtime::SdkContext(), Instance());
}
void SettingsWidget::NotifyRestartNeeded(int level) {
  GFNativeSettingsRestartNeeded(gf::runtime::SdkContext(), Instance(), level);
}
void DialogWidget::CloseDialog() {
  GFNativeDialogClose(gf::runtime::SdkContext(), Instance());
}

namespace detail {

auto RegisterNativeWidget(const char* name, int kind, bool multi,
                          const NativeWidgetMeta& meta, MakeFn make) -> bool {
  auto& reg = Registrations().emplace_back();
  reg.name = name;
  reg.title = meta.title;
  reg.keywords = meta.keywords;
  reg.suffix = meta.suffix;
  reg.filter = meta.filter;
  reg.icon = meta.icon;
  reg.make = std::move(make);

  auto& s = reg.spec;
  s.struct_size = sizeof(GFNativeWidgetSpec);
  s.name = reg.name.constData();
  s.kind = kind;
  s.multi_instance = multi ? 1 : 0;
  s.title = reg.title.constData();
  s.keywords = reg.keywords.constData();
  s.suffix = reg.suffix.constData();
  s.filter = reg.filter.constData();
  s.icon = reg.icon.constData();
  s.width = meta.width;
  s.height = meta.height;
  s.user = &reg;
  s.create = &Create;
  s.destroyed = &Destroyed;
  s.document = kind == GF_NATIVE_DOCUMENT ? &kDocumentOps : nullptr;
  s.settings = kind == GF_NATIVE_SETTINGS ? &kSettingsOps : nullptr;
  s.dialog = kind == GF_NATIVE_DIALOG ? &kDialogOps : nullptr;

  if (GFNativeWidgetRegister(gf::runtime::SdkContext(), &s) != 0) {
    LOG_ERROR(QString("the host refused native widget %1")
                  .arg(QString::fromUtf8(name)));
    Registrations().pop_back();
    return false;
  }
  return true;
}

}  // namespace detail

}  // namespace gf::ui

namespace gf::runtime {

void ForgetNativeWidgets() {
  // Nothing to withdraw from the Host: it withdrew every registration and
  // closed this module's gate before this runs, so nothing calls in. The
  // instance map belongs to the GUI thread, where every op touching it runs,
  // so it is cleared there -- this runs on the module runner.
  auto* app = QCoreApplication::instance();
  if (app == nullptr || QThread::currentThread() == app->thread()) {
    Instances().clear();
  } else {
    QMetaObject::invokeMethod(
        app, []() { Instances().clear(); }, Qt::QueuedConnection);
  }
}

void ResetNativeRegistrations() {
  // The Host dropped every `user` pointer into this list when the module was
  // last withdrawn; each activation registers afresh, and the list used to
  // grow by one set per activation.
  Registrations().clear();
}

}  // namespace gf::runtime
