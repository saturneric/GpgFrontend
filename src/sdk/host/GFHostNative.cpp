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

#include <QCborValue>
#include <QCoreApplication>
#include <QThread>
#include <QWidget>

#include "GFHostImpl.h"
#include "GFSDKHostApi.h"
#include "private/GFHostContext.h"
#include "private/GFHostEnter.h"
#include "private/GFHostGate.h"
#include "private/GFHostTransfer.h"
#include "private/GFSDKPrivate.h"
#include "ui/lua/NativeInstances.h"
#include "ui/lua/NativeWidgetRegistry.h"

/**
 * @file GFHostNative.cpp
 * @brief Native widgets: the module's typed ops, adapted for the Host's
 *        containers, and the module's notifications, checked.
 *
 * Only one pointer ever crosses: the module's widget, from the module to the
 * Host, from `create`. Everything else is an instance number, and every
 * notification a module sends about one is refused unless the instance is
 * the module's own and of the kind the notification is for.
 */

namespace {

using GpgFrontend::GFBuffer;
using GpgFrontend::UI::NativeInstances;
using GpgFrontend::UI::NativeWidgetEntry;
using GpgFrontend::UI::NativeWidgetKind;
using GpgFrontend::UI::NativeWidgetRegistry;
using gf_sdk_internal::EnterModule;

/// Borrowed bytes, as a handle the module may read for one call.
struct Lent {
  Lent(const QByteArray& bytes, const QString& module)
      : ref(gf_sdk_internal::NewBufferFor(GFBuffer(bytes), module,
                                          "native.lent")) {}
  ~Lent() {
    if (ref != nullptr) gf_host::GFBufferRelease(ref);
  }
  Lent(const Lent&) = delete;
  auto operator=(const Lent&) -> Lent& = delete;
  GFBufferRef ref;
};

/// A handle the module returned, taken over by the Host.
auto Take(GFBufferRef ref) -> std::optional<QByteArray> {
  if (ref == nullptr) return std::nullopt;
  const auto b = gf_sdk_internal::TakeBufferForTransfer(ref, "native.return");
  if (!b.has_value()) return std::nullopt;
  return b->ConvertToQByteArray();
}

auto Kind(int k) -> std::optional<NativeWidgetKind> {
  switch (k) {
    case GF_NATIVE_DOCUMENT:
      return NativeWidgetKind::kDOCUMENT;
    case GF_NATIVE_SETTINGS:
      return NativeWidgetKind::kSETTINGS;
    case GF_NATIVE_DIALOG:
      return NativeWidgetKind::kDIALOG;
    default:
      return std::nullopt;
  }
}

auto Text(const char* s) -> QString {
  return s == nullptr ? QString() : QString::fromUtf8(s);
}

void AdaptDocument(NativeWidgetEntry& e, const GFDocumentWidgetOps ops,
                   void* user, const QByteArray& m) {
  const auto module = QString::fromUtf8(m);
  auto& d = e.document;
  // Each member is optional in the table; an absent one is simply not asked.
  if (ops.load != nullptr) {
    d.load = [=](quint64 i, const QByteArray& bytes) {
      EnterModule(m, [&] {
        const Lent lent(bytes, module);
        ops.load(user, i, lent.ref);
      });
    };
  }
  if (ops.save != nullptr) {
    d.save = [=](quint64 i) -> std::optional<QByteArray> {
      std::optional<QByteArray> out;
      EnterModule(m, [&] { out = Take(ops.save(user, i)); });
      return out;
    };
  }
  if (ops.is_dirty != nullptr) {
    d.is_dirty = [=](quint64 i) {
      bool dirty = false;
      EnterModule(m, [&] { dirty = ops.is_dirty(user, i) != 0; });
      return dirty;
    };
  }
  if (ops.crypto_ops != nullptr) {
    d.crypto_ops = [=](quint64 i) -> std::optional<uint32_t> {
      std::optional<uint32_t> out;
      EnterModule(m, [&] {
        uint32_t bits = 0;
        if (ops.crypto_ops(user, i, &bits) == 0) out = bits;
      });
      return out;
    };
  }
  if (ops.suggested_file_name != nullptr) {
    d.suggested_file_name = [=](quint64 i) {
      QString name;
      EnterModule(m, [&] {
        const auto b = Take(ops.suggested_file_name(user, i));
        if (b.has_value()) name = QString::fromUtf8(*b);
      });
      return name;
    };
  }
  if (ops.apply_verification != nullptr) {
    d.apply_verification = [=](quint64 i, const QByteArray& json) {
      EnterModule(m, [&] {
        const Lent lent(json, module);
        ops.apply_verification(user, i, lent.ref);
      });
    };
  }
  if (ops.append_text != nullptr) {
    d.append_text = [=](quint64 i, const QString& text) {
      bool taken = false;
      EnterModule(m, [&] {
        const Lent lent(text.toUtf8(), module);
        taken = ops.append_text(user, i, lent.ref) != 0;
      });
      return taken;
    };
  }
  if (ops.attach_public_key != nullptr) {
    d.attach_public_key = [=](quint64 i, const QByteArray& key,
                              const QString& name) {
      bool taken = false;
      EnterModule(m, [&] {
        const Lent lent(key, module);
        const auto n = name.toUtf8();
        taken = ops.attach_public_key(user, i, lent.ref, n.constData()) != 0;
      });
      return taken;
    };
  }
  if (ops.apply_font != nullptr) {
    d.apply_font = [=](quint64 i, const QString& family, int size) {
      EnterModule(m, [&] {
        const auto f = family.toUtf8();
        ops.apply_font(user, i, f.constData(), size);
      });
    };
  }
  if (ops.wipe_content != nullptr) {
    d.wipe_content = [=](quint64 i) {
      EnterModule(m, [&] { ops.wipe_content(user, i); });
    };
  }
  if (ops.prepare_save != nullptr) {
    d.prepare_save = [=](quint64 i, const QByteArray& bytes)
        -> std::optional<std::optional<QByteArray>> {
      std::optional<std::optional<QByteArray>> out;
      const bool entered = EnterModule(m, [&] {
        const Lent lent(bytes, module);
        GFBufferRef replaced = nullptr;
        if (ops.prepare_save(user, i, lent.ref, &replaced) != 0) {
          if (replaced != nullptr) Take(replaced);
          return;  // cancelled
        }
        out = Take(replaced);
      });
      if (!entered) return std::optional<QByteArray>{};  // module gone
      return out;
    };
  }
  if (ops.source_policy != nullptr) {
    d.source_lock = [=](quint64 i) -> std::optional<QString> {
      std::optional<QString> reason;
      EnterModule(m, [&] {
        GFBufferRef text = nullptr;
        if (ops.source_policy(user, i, &text) == 0) {
          const auto b = Take(text);
          reason = b.has_value() ? QString::fromUtf8(*b) : QString();
        } else if (text != nullptr) {
          Take(text);
        }
      });
      return reason;
    };
  }
}

// ------------------------------------------------------------------ thunks

auto RegisterWidget(GFHostContextRef ctx, const GFNativeWidgetSpec* spec)
    -> int {
  GATE(ctx, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM, "native.register_widget",
       -1);
  if (spec == nullptr || spec->struct_size < sizeof(GFNativeWidgetSpec) ||
      spec->name == nullptr || spec->create == nullptr) {
    return -1;
  }
  const auto kind = Kind(spec->kind);
  if (!kind.has_value()) return -1;
  // Exactly the ops table the kind says.
  const bool fits =
      (*kind == NativeWidgetKind::kDOCUMENT) == (spec->document != nullptr) &&
      (*kind == NativeWidgetKind::kSETTINGS) == (spec->settings != nullptr) &&
      (*kind == NativeWidgetKind::kDIALOG) == (spec->dialog != nullptr);
  if (!fits) {
    LOG_W() << "native widget" << spec->name
            << "does not carry exactly the ops its kind needs";
    return -1;
  }

  const auto module = gf_sdk_internal::ContextModuleId(ctx);
  const auto m = module.toUtf8();
  NativeWidgetEntry e;
  e.owner = module;
  e.id = module + "." + Text(spec->name);
  e.kind = *kind;
  e.multi_instance = spec->multi_instance != 0;
  e.title = Text(spec->title);
  e.keywords = Text(spec->keywords);
  e.suffix = Text(spec->suffix);
  e.filter = Text(spec->filter);
  e.icon = Text(spec->icon);
  e.width = spec->width;
  e.height = spec->height;

  auto* const user = spec->user;
  const auto create = spec->create;
  const auto destroyed = spec->destroyed;
  e.create = [=](quint64 instance, const QCborMap& args) -> QWidget* {
    QWidget* widget = nullptr;
    EnterModule(m, [&] {
      const Lent lent(QCborValue(args).toCbor(), module);
      auto* object = static_cast<QObject*>(create(user, instance, lent.ref));
      widget = qobject_cast<QWidget*>(object);
      if (widget == nullptr && object != nullptr) delete object;
    });
    return widget;
  };
  if (destroyed != nullptr) {
    e.destroyed = [=](quint64 instance) {
      EnterModule(m, [&] { destroyed(user, instance); });
    };
  }

  if (spec->document != nullptr) {
    GFDocumentWidgetOps ops{};
    std::memcpy(&ops, spec->document,
                std::min(sizeof(ops), spec->document->struct_size));
    AdaptDocument(e, ops, user, m);
  }
  if (spec->settings != nullptr) {
    GFSettingsWidgetOps ops{};
    std::memcpy(&ops, spec->settings,
                std::min(sizeof(ops), spec->settings->struct_size));
    if (ops.load != nullptr) {
      e.settings.load = [=](quint64 i) {
        EnterModule(m, [&] { ops.load(user, i); });
      };
    }
    if (ops.apply != nullptr) {
      e.settings.apply = [=](quint64 i) {
        bool ok = false;
        EnterModule(m, [&] { ok = ops.apply(user, i) == 0; });
        return ok;
      };
    }
  }
  if (spec->dialog != nullptr) {
    GFDialogWidgetOps ops{};
    std::memcpy(&ops, spec->dialog,
                std::min(sizeof(ops), spec->dialog->struct_size));
    if (ops.opened != nullptr) {
      e.dialog.opened = [=](quint64 i, const QCborMap& args) {
        EnterModule(m, [&] {
          const Lent lent(QCborValue(args).toCbor(), module);
          ops.opened(user, i, lent.ref);
        });
      };
    }
    if (ops.close_requested != nullptr) {
      e.dialog.close_requested = [=](quint64 i) {
        bool allow = true;
        EnterModule(m, [&] { allow = ops.close_requested(user, i) != 0; });
        return allow;
      };
    }
  }

  return NativeWidgetRegistry::Instance().Register(std::move(e)) ? 0 : -1;
}

auto UnregisterWidget(GFHostContextRef ctx, const char* name) -> int {
  GATE(ctx, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM,
       "native.unregister_widget", -1);
  if (name == nullptr) return -1;
  const auto module = gf_sdk_internal::ContextModuleId(ctx);
  return NativeWidgetRegistry::Instance().Unregister(
             module, module + "." + QString::fromUtf8(name))
             ? 0
             : -1;
}

/// Deliver a notification to the instance's container, on the GUI thread,
/// if -- and only if -- the instance is the caller's own and of @p kind.
template <typename Fn>
auto Notify(GFHostContextRef ctx, uint64_t instance, NativeWidgetKind kind,
            Fn fn) -> int {
  const auto module = gf_sdk_internal::ContextModuleId(ctx);
  auto deliver = [=]() -> int {
    auto* c = NativeInstances::Instance().ContainerFor(instance, module, kind);
    if (c == nullptr) return -1;
    fn(c);
    return 0;
  };
  auto* app = QCoreApplication::instance();
  if (app == nullptr || QThread::currentThread() == app->thread()) {
    return deliver();
  }
  QMetaObject::invokeMethod(app, [deliver]() { deliver(); },
                            Qt::QueuedConnection);
  return 0;
}

#define NATIVE_GATE(name)                                                   \
  GATE(ctx, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM, name, -1)

auto DocumentModified(GFHostContextRef ctx, uint64_t instance) -> int {
  NATIVE_GATE("native.document_modified");
  return Notify(ctx, instance, NativeWidgetKind::kDOCUMENT,
                [](auto* c) { c->OnModified(); });
}

auto DocumentShowSource(GFHostContextRef ctx, uint64_t instance, int source)
    -> int {
  NATIVE_GATE("native.document_show_source");
  return Notify(ctx, instance, NativeWidgetKind::kDOCUMENT,
                [source](auto* c) { c->OnShowSource(source != 0); });
}

auto DocumentRequestCrypto(GFHostContextRef ctx, uint64_t instance,
                           uint32_t op) -> int {
  NATIVE_GATE("native.document_request_crypto");
  return Notify(ctx, instance, NativeWidgetKind::kDOCUMENT,
                [op](auto* c) { c->OnRequestCrypto(op); });
}

auto DocumentOpsChanged(GFHostContextRef ctx, uint64_t instance) -> int {
  NATIVE_GATE("native.document_ops_changed");
  return Notify(ctx, instance, NativeWidgetKind::kDOCUMENT,
                [](auto* c) { c->OnOpsChanged(); });
}

auto SettingsRestartNeeded(GFHostContextRef ctx, uint64_t instance, int level)
    -> int {
  NATIVE_GATE("native.settings_restart_needed");
  return Notify(ctx, instance, NativeWidgetKind::kSETTINGS,
                [level](auto* c) { c->OnRestartNeeded(level); });
}

auto DialogClose(GFHostContextRef ctx, uint64_t instance) -> int {
  NATIVE_GATE("native.dialog_close");
  return Notify(ctx, instance, NativeWidgetKind::kDIALOG,
                [](auto* c) { c->OnClose(); });
}

}  // namespace

namespace gf_sdk_internal {

const GFHostNativeWidgetApi kNativeApi = {
    sizeof(GFHostNativeWidgetApi), &RegisterWidget,        &UnregisterWidget,
    &DocumentModified,             &DocumentShowSource,    &DocumentRequestCrypto,
    &DocumentOpsChanged,           &SettingsRestartNeeded, &DialogClose,
};

}  // namespace gf_sdk_internal
