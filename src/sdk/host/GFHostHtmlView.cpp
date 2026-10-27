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

#include <QWidget>
#include <cstring>

#include "GFHostImpl.h"
#include "GFSDKHostApi.h"
#include "core/model/GFBuffer.h"
#include "private/GFHostContext.h"
#include "private/GFHostEnter.h"
#include "private/GFHostGate.h"
#include "private/GFHostTransfer.h"
#include "ui/html/HtmlViewService.h"

/**
 * @file GFHostHtmlView.cpp
 * @brief HTML views: the consumer's calls and the renderer's, checked, and
 *        a module's renderer adapted for the service.
 *
 * Two pointers cross, both from a module to the Host: a consumer's slot,
 * which the service finds in the consumer's own tree before it follows it,
 * and a renderer's widget, from `create`. Everything else is a handle that
 * answers to the module it was issued to and to nobody else.
 */

namespace {

using gf_sdk_internal::EnterModule;
using GpgFrontend::GFBuffer;
using GpgFrontend::UI::HtmlViewPart;
using GpgFrontend::UI::HtmlViewProviderInfo;
using GpgFrontend::UI::HtmlViewRenderer;
using GpgFrontend::UI::HtmlViewService;
namespace HtmlView = GpgFrontend::UI::HtmlView;

// gf_ui spells every SDK constant again so as not to include the SDK; this
// is the one translation unit that sees both, so this is where they meet.
static_assert(GF_HTML_POLICY_NO_NETWORK == HtmlView::kNO_NETWORK);
static_assert(GF_HTML_POLICY_PARTS_ONLY == HtmlView::kPARTS_ONLY);
static_assert(GF_HTML_POLICY_NO_SCRIPT == HtmlView::kNO_SCRIPT);
static_assert(GF_HTML_POLICY_NO_NAVIGATION == HtmlView::kNO_NAVIGATION);
static_assert(GF_HTML_POLICY_NO_POPUPS == HtmlView::kNO_POPUPS);
static_assert(GF_HTML_POLICY_NO_DOWNLOADS == HtmlView::kNO_DOWNLOADS);
static_assert(GF_HTML_POLICY_NO_PERSISTENCE == HtmlView::kNO_PERSISTENCE);
static_assert(GF_HTML_POLICY_NO_PERMISSIONS == HtmlView::kNO_PERMISSIONS);
static_assert(GF_HTML_POLICY_NO_DNS_PREFETCH == HtmlView::kNO_DNS_PREFETCH);
static_assert(GF_HTML_POLICY_NO_PLUGINS == HtmlView::kNO_PLUGINS);
static_assert(GF_HTML_VIEW_PRESET_MAIL == HtmlView::kPresetMail);
static_assert(GF_HTML_VIEW_OK == HtmlViewService::kOK);
static_assert(GF_HTML_VIEW_E_UNAVAILABLE == HtmlViewService::kUNAVAILABLE);
static_assert(GF_HTML_VIEW_E_RESTART_REQUIRED ==
              HtmlViewService::kRESTART_REQUIRED);
static_assert(GF_HTML_VIEW_E_POLICY == HtmlViewService::kPOLICY);
static_assert(GF_HTML_VIEW_E_DENIED == HtmlViewService::kDENIED);
static_assert(GF_HTML_VIEW_E_INVALID == HtmlViewService::kINVALID);
static_assert(GF_HTML_VIEW_E_TOO_LARGE == HtmlViewService::kTOO_LARGE);
static_assert(GF_HTML_VIEW_E_FAILED == HtmlViewService::kFAILED);
static_assert(GF_HTML_PART_OK == HtmlViewService::kPART_OK);
static_assert(GF_HTML_PART_NOT_FOUND == HtmlViewService::kPART_NOT_FOUND);
static_assert(GF_HTML_PART_TOO_LARGE == HtmlViewService::kPART_TOO_LARGE);
static_assert(GF_HTML_PART_STALE == HtmlViewService::kPART_STALE);
static_assert(GF_HTML_VIEW_EVENT_LOADED == HtmlViewService::kEVENT_LOADED);
static_assert(GF_HTML_VIEW_EVENT_LINK == HtmlViewService::kEVENT_LINK);
static_assert(GF_HTML_VIEW_EVENT_LOST == HtmlViewService::kEVENT_LOST);

constexpr uint32_t kViewCaps =
    GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM | GF_HOST_CAP_HTML_VIEW;
constexpr uint32_t kProviderCaps =
    GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM | GF_HOST_CAP_HTML_PROVIDER;

auto Text(const char* s) -> QString {
  return s == nullptr ? QString() : QString::fromUtf8(s);
}

/// Take a buffer the caller handed over; empty for none.
auto Take(GFBufferRef ref, const char* what) -> std::optional<GFBuffer> {
  if (ref == nullptr) return std::nullopt;
  return gf_sdk_internal::TakeBufferForTransfer(ref, what);
}

/// Borrowed bytes, as a handle the module may read for one call.
struct Lent {
  Lent(const GFBuffer& bytes, const QString& module)
      : ref(gf_sdk_internal::NewBufferFor(bytes, module, "html.lent")) {}
  ~Lent() {
    if (ref != nullptr) gf_host::GFBufferRelease(ref);
  }
  Lent(const Lent&) = delete;
  auto operator=(const Lent&) -> Lent& = delete;
  GFBufferRef ref;
};

/**
 * @brief A module's renderer, for the service.
 *
 * Every call enters the module through its gate; once the module is
 * withdrawn none does, and each answers as a renderer that refused.
 */
class ModuleRenderer : public HtmlViewRenderer {
 public:
  ModuleRenderer(QByteArray module, const GFHtmlViewProviderOps& ops,
                 void* user)
      : module_(std::move(module)), ops_(ops), user_(user) {}

  auto Create(quint64 view, const HtmlView::Policy& policy)
      -> QWidget* override {
    GFHtmlViewPolicy c{};
    c.struct_size = sizeof(c);
    c.preset = policy.preset;
    c.required = policy.required;
    c.max_document_bytes = static_cast<uint64_t>(policy.max_document_bytes);
    c.max_part_bytes = static_cast<uint64_t>(policy.max_part_bytes);
    c.max_total_part_bytes = static_cast<uint64_t>(policy.max_total_part_bytes);

    QWidget* widget = nullptr;
    EnterModule(module_, [&] {
      auto* object = static_cast<QObject*>(ops_.create(user_, view, &c));
      widget = qobject_cast<QWidget*>(object);
      if (widget == nullptr && object != nullptr) delete object;
    });
    return widget;
  }

  auto Load(quint64 view, const QString& session, const QByteArray& document)
      -> bool override {
    bool shown = false;
    EnterModule(module_, [&] {
      const Lent lent(GFBuffer(document), QString::fromUtf8(module_));
      const auto name = session.toUtf8();
      shown = ops_.load(user_, view, name.constData(), lent.ref) == 0;
    });
    return shown;
  }

  void Clear(quint64 view) override {
    if (ops_.clear == nullptr) return;
    EnterModule(module_, [&] { ops_.clear(user_, view); });
  }

  void Destroyed(quint64 view) override {
    if (ops_.destroyed == nullptr) return;
    EnterModule(module_, [&] { ops_.destroyed(user_, view); });
  }

 private:
  QByteArray module_;
  GFHtmlViewProviderOps ops_;
  void* user_;
};

/* --- consumers ----------------------------------------------------------- */

auto Available(GFHostContextRef ctx, int preset) -> int {
  GATE(ctx, kViewCaps, "html_view.available", GF_HTML_VIEW_E_DENIED);
  return HtmlViewService::Instance().Available(preset);
}

auto CreateView(GFHostContextRef ctx, uint64_t instance, void* slot, int preset,
                GFHtmlViewEventFn fn, void* user, uint64_t* out_view) -> int {
  GATE(ctx, kViewCaps, "html_view.create_view", GF_HTML_VIEW_E_DENIED);
  if (out_view != nullptr) *out_view = 0;
  const auto module = gf_sdk_internal::ContextModuleId(ctx);
  HtmlViewService::EventFn events;
  if (fn != nullptr) {
    const auto m = module.toUtf8();
    events = [m, fn, user](quint64 view, int event, int value,
                           const QString& text) {
      EnterModule(m, [&] {
        const auto utf8 = text.toUtf8();
        fn(user, view, event, value,
           text.isNull() ? nullptr : utf8.constData());
      });
    };
  }
  quint64 view = 0;
  const auto rc = HtmlViewService::Instance().CreateView(
      module, instance, slot, preset, std::move(events), &view);
  if (out_view != nullptr) *out_view = view;
  return rc;
}

auto Load(GFHostContextRef ctx, uint64_t view, GFBufferRef html_utf8,
          const GFHtmlPart* parts, size_t part_count, size_t* out_dropped)
    -> int {
  if (out_dropped != nullptr) *out_dropped = 0;
  // Every buffer is the Host's from here, whatever happens next: taken
  // before the gate, so a refusal does not leave them behind.
  const auto html = Take(html_utf8, "html_view.load");
  QList<HtmlViewPart> taken;
  for (size_t i = 0; parts != nullptr && i < part_count; ++i) {
    GFHtmlPart part{};
    std::memcpy(&part, &parts[i], std::min(sizeof(part), parts[i].struct_size));
    auto data = Take(part.data, "html_view.load.part");
    if (!data.has_value()) continue;
    taken.append(
        HtmlViewPart{Text(part.cid), Text(part.mime), std::move(*data)});
  }

  GATE(ctx, kViewCaps, "html_view.load", GF_HTML_VIEW_E_DENIED);
  if (!html.has_value()) return GF_HTML_VIEW_E_INVALID;
  // A part whose buffer the transfer refused counts as dropped too.
  const auto refused = part_count - static_cast<size_t>(taken.size());
  int dropped = 0;
  const auto rc = HtmlViewService::Instance().Load(
      gf_sdk_internal::ContextModuleId(ctx), view, html->ConvertToQByteArray(),
      std::move(taken), &dropped);
  if (out_dropped != nullptr && rc == GF_HTML_VIEW_OK) {
    *out_dropped = refused + static_cast<size_t>(dropped);
  }
  return rc;
}

auto Clear(GFHostContextRef ctx, uint64_t view) -> int {
  GATE(ctx, kViewCaps, "html_view.clear", GF_HTML_VIEW_E_DENIED);
  return HtmlViewService::Instance().Clear(
      gf_sdk_internal::ContextModuleId(ctx), view);
}

auto DestroyView(GFHostContextRef ctx, uint64_t view) -> int {
  GATE(ctx, kViewCaps, "html_view.destroy_view", GF_HTML_VIEW_E_DENIED);
  return HtmlViewService::Instance().DestroyView(
      gf_sdk_internal::ContextModuleId(ctx), view);
}

/* --- renderers ----------------------------------------------------------- */

auto RegisterProvider(GFHostContextRef ctx, const GFHtmlViewProviderSpec* spec)
    -> int {
  GATE(ctx, kProviderCaps, "html_provider.register_provider",
       GF_HTML_VIEW_E_DENIED);
  if (spec == nullptr || spec->struct_size < sizeof(size_t)) {
    return GF_HTML_VIEW_E_INVALID;
  }
  GFHtmlViewProviderSpec s{};
  std::memcpy(&s, spec, std::min(sizeof(s), spec->struct_size));
  // A generation this Host does not know is refused, not guessed at: its
  // ops may mean something else.
  if (s.interface_version != GF_HTML_VIEW_PROVIDER_V1 || s.ops == nullptr ||
      s.name == nullptr || *s.name == '\0') {
    return GF_HTML_VIEW_E_INVALID;
  }
  GFHtmlViewProviderOps ops{};
  std::memcpy(&ops, s.ops, std::min(sizeof(ops), s.ops->struct_size));
  if (ops.create == nullptr || ops.load == nullptr) {
    return GF_HTML_VIEW_E_INVALID;
  }

  const auto module = gf_sdk_internal::ContextModuleId(ctx);
  auto& service = HtmlViewService::Instance();
  HtmlViewProviderInfo info;
  info.owner = module;
  info.name = QString::fromUtf8(s.name);
  info.priority = HtmlViewService::kModulePriority;
  info.enforceable = s.enforceable;
  // Listed, so consumers can say a restart is needed; never selected.
  info.restart_required =
      (s.features & GF_HTML_PROVIDER_FEATURE_RESTART_REQUIRED) != 0;
  info.renderer =
      std::make_shared<ModuleRenderer>(module.toUtf8(), ops, s.user);
  return service.RegisterProvider(std::move(info));
}

auto UnregisterProvider(GFHostContextRef ctx, const char* name) -> int {
  GATE(ctx, kProviderCaps, "html_provider.unregister_provider",
       GF_HTML_VIEW_E_DENIED);
  if (name == nullptr) return GF_HTML_VIEW_E_INVALID;
  return HtmlViewService::Instance().UnregisterProvider(
      gf_sdk_internal::ContextModuleId(ctx), QString::fromUtf8(name));
}

auto RequestPart(GFHostContextRef ctx, uint64_t view, const char* session,
                 const char* cid, GFHtmlPartDoneFn done, void* user,
                 uint64_t* out_request) -> int {
  GATE(ctx, kProviderCaps, "html_provider.request_part", GF_HTML_VIEW_E_DENIED);
  if (out_request != nullptr) *out_request = 0;
  if (done == nullptr || session == nullptr || cid == nullptr) {
    return GF_HTML_VIEW_E_INVALID;
  }
  const auto module = gf_sdk_internal::ContextModuleId(ctx);
  const auto m = module.toUtf8();
  quint64 request = 0;
  const auto rc = HtmlViewService::Instance().RequestPart(
      module, view, QString::fromUtf8(session), QString::fromUtf8(cid),
      [m, module, done, user](quint64 id, int status, const QString& mime,
                              const GFBuffer& data) {
        EnterModule(m, [&] {
          // Issued inside the gate, so a refused entry leaves no handle.
          auto* ref = status == GF_HTML_PART_OK ? gf_sdk_internal::NewBufferFor(
                                                      data, module, "html.part")
                                                : nullptr;
          const auto type = mime.toUtf8();
          done(user, id, status, mime.isEmpty() ? nullptr : type.constData(),
               ref);
        });
      },
      &request);
  if (out_request != nullptr) *out_request = request;
  return rc;
}

auto CancelPart(GFHostContextRef ctx, uint64_t request) -> int {
  GATE(ctx, kProviderCaps, "html_provider.cancel_part", GF_HTML_VIEW_E_DENIED);
  return HtmlViewService::Instance().CancelPart(
      gf_sdk_internal::ContextModuleId(ctx), request);
}

auto LinkActivated(GFHostContextRef ctx, uint64_t view, const char* url)
    -> int {
  GATE(ctx, kProviderCaps, "html_provider.link_activated",
       GF_HTML_VIEW_E_DENIED);
  if (url == nullptr) return GF_HTML_VIEW_E_INVALID;
  return HtmlViewService::Instance().LinkActivated(
      gf_sdk_internal::ContextModuleId(ctx), view, QString::fromUtf8(url));
}

auto Loaded(GFHostContextRef ctx, uint64_t view, int ok) -> int {
  GATE(ctx, kProviderCaps, "html_provider.loaded", GF_HTML_VIEW_E_DENIED);
  return HtmlViewService::Instance().Loaded(
      gf_sdk_internal::ContextModuleId(ctx), view, ok != 0);
}

}  // namespace

namespace gf_sdk_internal {

const GFHostHtmlViewApi kHtmlViewApi = {
    sizeof(GFHostHtmlViewApi),
    &Available,
    &CreateView,
    &Load,
    &Clear,
    &DestroyView,
};

const GFHostHtmlProviderApi kHtmlProviderApi = {
    sizeof(GFHostHtmlProviderApi),
    &RegisterProvider,
    &UnregisterProvider,
    &RequestPart,
    &CancelPart,
    &LinkActivated,
    &Loaded,
};

}  // namespace gf_sdk_internal
