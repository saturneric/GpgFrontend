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

#include "GpgFrontend.h"

#ifdef Q_OS_MACOS

// Built with the PCH skipped (see MacRelaunch.mm), so what it would have
// force-included comes in by hand.
#include "GFCoreExport.h"
#include "GFUiExport.h"
#include "core/GpgFrontendCore.h"
#include "ui/GpgFrontendUI.h"

#include <QPointer>
#include <QVBoxLayout>
#include <QWindow>
#include <map>
#include <memory>

#include "platform/common/PlatformHtmlView.h"
#include "ui/html/HtmlViewService.h"

#import <WebKit/WebKit.h>

namespace HtmlView = GpgFrontend::UI::HtmlView;
using GpgFrontend::UI::HtmlViewService;

namespace {

/// The Host's own renderer, as the service knows it: the empty owner.
const QString kOwner;

/// Every bit, each enforced by WebKit itself; how, is next to where.
constexpr uint32_t kEnforceable =
    HtmlView::kNO_NETWORK | HtmlView::kPARTS_ONLY | HtmlView::kNO_SCRIPT |
    HtmlView::kNO_NAVIGATION | HtmlView::kNO_POPUPS | HtmlView::kNO_DOWNLOADS |
    HtmlView::kNO_PERSISTENCE | HtmlView::kNO_PERMISSIONS |
    HtmlView::kNO_DNS_PREFETCH | HtmlView::kNO_PLUGINS;

/// Every load is refused inside WebKit, before any delegate is asked and
/// main frame included; the part scheme is the one exception. Without that
/// exception the gfpart document itself is blocked, and an http main frame
/// never leaves.
NSString* const kRuleListId = @"gf-html-view-mail";
NSString* const kRules =
    @"[{\"trigger\":{\"url-filter\":\".*\"},\"action\":{\"type\":\"block\"}},"
    @"{\"trigger\":{\"url-filter\":\"^gfpart:\"},"
    @"\"action\":{\"type\":\"ignore-previous-rules\"}}]";

WKContentRuleList* g_rules = nil;  ///< once compiled
bool g_compiling = false;

auto Fail() -> NSError* {
  return [NSError errorWithDomain:NSURLErrorDomain
                             code:NSURLErrorResourceUnavailable
                         userInfo:nil];
}

void Wipe(QByteArray& bytes) {
  if (!bytes.isEmpty()) bytes.fill('\0');
  bytes.clear();
}

}  // namespace

/**
 * @brief One view's engine side: the part scheme, and every question WebKit
 *        asks it.
 *
 * Everything here runs on the main thread, which is the GUI thread.
 */
API_AVAILABLE(macos(12.0))
@interface GFHtmlViewHandler
    : NSObject <WKNavigationDelegate, WKUIDelegate, WKURLSchemeHandler>
- (instancetype)initWithView:(quint64)view;
- (void)load:(const QString&)session
    document:(const QByteArray&)document
        into:(WKWebView*)web_view;
- (void)clear:(WKWebView*)web_view;
/// The view is gone: whatever it asked for is cancelled, and nothing it
/// still hears reaches the service.
- (void)detach;
@end

@implementation GFHtmlViewHandler {
  quint64 view_;
  QString session_;
  QByteArray document_;
  /// The one main-frame load allowed next, taken when it is asked about.
  NSURL* expected_main_;
  /// The document's own navigation, the only one Loaded() is about.
  WKNavigation* expected_navigation_;
  bool awaiting_load_;
  bool detached_;
  /// Part requests in flight, by the service's request id.
  std::map<quint64, id<WKURLSchemeTask>> tasks_;
}

- (instancetype)initWithView:(quint64)view {
  if ((self = [super init])) {
    view_ = view;
    awaiting_load_ = false;
    detached_ = false;
  }
  return self;
}

- (void)dealloc {
  Wipe(document_);
}

- (void)load:(const QString&)session
    document:(const QByteArray&)document
        into:(WKWebView*)web_view {
  Wipe(document_);
  session_ = session;
  document_ = document;
  NSURL* url = [NSURL
      URLWithString:QString("%1://%2/")
                        .arg(QLatin1StringView(HtmlView::kPartScheme), session)
                        .toNSString()];
  expected_main_ = url;
  awaiting_load_ = true;
  // The document comes from the scheme handler, as on every engine: its URL
  // is the session, so its parts resolve under it and nothing else does.
  expected_navigation_ = [web_view loadRequest:[NSURLRequest requestWithURL:url]];
}

- (void)clear:(WKWebView*)web_view {
  session_.clear();
  Wipe(document_);
  awaiting_load_ = false;
  expected_navigation_ = nil;
  expected_main_ = [NSURL URLWithString:@"about:blank"];
  [web_view loadRequest:[NSURLRequest requestWithURL:expected_main_]];
}

- (void)detach {
  detached_ = true;
  // Never answered now, and never touched again: WebKit stops each one as
  // the view goes, and a stopped task is not ours to finish.
  for (const auto& [request, task] : tasks_) {
    HtmlViewService::Instance().CancelPart(kOwner, request);
  }
  tasks_.clear();
  session_.clear();
  Wipe(document_);
  awaiting_load_ = false;
  expected_navigation_ = nil;
  expected_main_ = nil;
}

- (bool)takeExpectedMainLoad:(NSURL*)url {
  if (expected_main_ == nil || ![url isEqual:expected_main_]) return false;
  expected_main_ = nil;
  return true;
}

- (void)reportLoaded:(WKNavigation*)navigation ok:(bool)ok {
  if (detached_ || !awaiting_load_ || navigation != expected_navigation_) {
    return;
  }
  awaiting_load_ = false;
  expected_navigation_ = nil;
  if (!session_.isEmpty()) HtmlViewService::Instance().Loaded(kOwner, view_, ok);
}

- (void)reportLink:(NSURL*)url {
  if (detached_ || url == nil) return;
  HtmlViewService::Instance().LinkActivated(
      kOwner, view_, QString::fromNSString(url.absoluteString));
}

// --- gfpart:// -- the document at /, a part anywhere else -------------------

- (void)webView:(WKWebView*)webView
    startURLSchemeTask:(id<WKURLSchemeTask>)task {
  NSURL* url = task.request.URL;
  NSURLComponents* parts = [NSURLComponents componentsWithURL:url
                                      resolvingAgainstBaseURL:NO];
  if (detached_ || parts == nil ||
      ![task.request.HTTPMethod isEqualToString:@"GET"]) {
    [task didFailWithError:Fail()];
    return;
  }
  const auto session = QString::fromNSString(parts.host ?: @"");
  const auto path = QString::fromNSString(parts.percentEncodedPath ?: @"");

  if (path.isEmpty() || path == u"/"_s) {
    if (session_.isEmpty() || session != session_) {
      [task didFailWithError:Fail()];
      return;
    }
    [task didReceiveResponse:[[NSURLResponse alloc]
                                         initWithURL:url
                                            MIMEType:@"text/html"
                               expectedContentLength:document_.size()
                                    textEncodingName:@"utf-8"]];
    [task didReceiveData:[NSData dataWithBytes:document_.constData()
                                        length:document_.size()]];
    [task didFinish];
    return;
  }

  const auto cid = QUrl::fromPercentEncoding(path.mid(1).toUtf8());
  __weak GFHtmlViewHandler* weak = self;
  quint64 request = 0;
  const auto status = HtmlViewService::Instance().RequestPart(
      kOwner, view_, session, cid,
      [weak](quint64 request, int status, const QString& mime,
             const GpgFrontend::GFBuffer& data) {
        GFHtmlViewHandler* handler = weak;
        if (handler != nil) {
          [handler finishPart:request status:status mime:mime data:data];
        }
      },
      &request);
  if (status != HtmlViewService::kOK) {
    [task didFailWithError:Fail()];
    return;
  }
  tasks_[request] = task;
}

- (void)finishPart:(quint64)request
            status:(int)status
              mime:(const QString&)mime
              data:(const GpgFrontend::GFBuffer&)data {
  const auto it = tasks_.find(request);
  if (it == tasks_.end()) return;  // stopped, or the view is gone
  id<WKURLSchemeTask> task = it->second;
  tasks_.erase(it);
  if (status != HtmlViewService::kPART_OK) {
    [task didFailWithError:Fail()];
    return;
  }
  [task didReceiveResponse:[[NSURLResponse alloc]
                                       initWithURL:task.request.URL
                                          MIMEType:mime.toNSString()
                             expectedContentLength:data.Size()
                                  textEncodingName:nil]];
  [task didReceiveData:[NSData dataWithBytes:data.Data() length:data.Size()]];
  [task didFinish];
}

- (void)webView:(WKWebView*)webView
    stopURLSchemeTask:(id<WKURLSchemeTask>)task {
  for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
    if (it->second != task) continue;
    HtmlViewService::Instance().CancelPart(kOwner, it->first);
    tasks_.erase(it);
    return;
  }
}

// --- navigation: the document and its srcdoc frame, nothing else ------------

- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationAction:(WKNavigationAction*)action
                        preferences:(WKWebpagePreferences*)preferences
                    decisionHandler:(void (^)(WKNavigationActionPolicy,
                                              WKWebpagePreferences*))handler {
  NSURL* url = action.request.URL;
  WKFrameInfo* target = action.targetFrame;
  const bool main = target != nil && target.isMainFrame;

  // No script, in anything the sender wrote. WebKit parses a document whose
  // script is off with its scripting content stripped -- on* handlers,
  // javascript: URLs and srcdoc alike -- so the Host's own document, which
  // carries the body as a srcdoc and has no script of its own (and a CSP
  // under which it could run none), is the one place it is on. The body's
  // frame then loses those same attributes before anything could run them.
  preferences.allowsContentJavaScript = NO;

  if (detached_ || action.shouldPerformDownload) {
    handler(WKNavigationActionPolicyCancel, preferences);
    return;
  }
  if (main && [self takeExpectedMainLoad:url]) {
    preferences.allowsContentJavaScript =
        [url.scheme isEqualToString:QString(HtmlView::kPartScheme).toNSString()];
    handler(WKNavigationActionPolicyAllow, preferences);
    return;
  }
  // The sandboxed frame the body lives in: about:srcdoc, never a request.
  if (target != nil && !main &&
      [url.absoluteString isEqualToString:@"about:srcdoc"]) {
    handler(WKNavigationActionPolicyAllow, preferences);
    return;
  }
  // A link the user activated is the Host's to open, outside this view --
  // target=_blank included, which arrives here with no target frame and so
  // never reaches createWebView. Everything else is simply refused.
  if (action.navigationType == WKNavigationTypeLinkActivated) {
    [self reportLink:url];
  } else {
    LOG_D() << "HTML view" << view_ << "refused navigation type"
            << static_cast<int>(action.navigationType)
            << (main ? "main" : "sub") << "frame:"
            << QString::fromNSString(url.absoluteString);
  }
  handler(WKNavigationActionPolicyCancel, preferences);
}

- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationResponse:(WKNavigationResponse*)response
                      decisionHandler:
                          (void (^)(WKNavigationResponsePolicy))handler {
  // What WebKit cannot show it would offer to save.
  handler(response.canShowMIMEType ? WKNavigationResponsePolicyAllow
                                   : WKNavigationResponsePolicyCancel);
}

- (void)webView:(WKWebView*)webView
     navigationAction:(WKNavigationAction*)action
    didBecomeDownload:(WKDownload*)download {
  [download cancel:nil];
}

- (void)webView:(WKWebView*)webView
    navigationResponse:(WKNavigationResponse*)response
     didBecomeDownload:(WKDownload*)download {
  [download cancel:nil];
}

// Every refused navigation ends in a failure too; only the document's own
// is the document failing.
- (void)webView:(WKWebView*)webView
    didFinishNavigation:(WKNavigation*)navigation {
  [self reportLoaded:navigation ok:true];
}

- (void)webView:(WKWebView*)webView
    didFailProvisionalNavigation:(WKNavigation*)navigation
                       withError:(NSError*)error {
  [self reportLoaded:navigation ok:false];
}

- (void)webView:(WKWebView*)webView
    didFailNavigation:(WKNavigation*)navigation
            withError:(NSError*)error {
  [self reportLoaded:navigation ok:false];
}

- (void)webViewWebContentProcessDidTerminate:(WKWebView*)webView {
  LOG_W() << "HTML view" << view_ << "lost its web content process";
  if (detached_ || !awaiting_load_) return;
  awaiting_load_ = false;
  expected_navigation_ = nil;
  if (!session_.isEmpty()) {
    HtmlViewService::Instance().Loaded(kOwner, view_, false);
  }
}

// --- UI: no windows, no permissions, no file pickers -------------------------

- (WKWebView*)webView:(WKWebView*)webView
    createWebViewWithConfiguration:(WKWebViewConfiguration*)configuration
               forNavigationAction:(WKNavigationAction*)action
                    windowFeatures:(WKWindowFeatures*)features {
  // A popup is only ever a way for a click to reach the Host; no window is
  // made.
  if (action.navigationType == WKNavigationTypeLinkActivated) {
    [self reportLink:action.request.URL];
  }
  return nil;
}

- (void)webView:(WKWebView*)webView
    requestMediaCapturePermissionForOrigin:(WKSecurityOrigin*)origin
                          initiatedByFrame:(WKFrameInfo*)frame
                                      type:(WKMediaCaptureType)type
                           decisionHandler:
                               (void (^)(WKPermissionDecision))handler {
  handler(WKPermissionDecisionDeny);
}

- (void)webView:(WKWebView*)webView
    runOpenPanelWithParameters:(WKOpenPanelParameters*)parameters
              initiatedByFrame:(WKFrameInfo*)frame
             completionHandler:(void (^)(NSArray<NSURL*>*))handler {
  handler(nil);
}

@end

namespace GpgFrontend::UI {

namespace {

class API_AVAILABLE(macos(12.0)) MacHtmlViewRenderer : public HtmlViewRenderer {
 public:
  auto Create(quint64 view, const HtmlView::Policy& policy)
      -> QWidget* override {
    if ((policy.required & ~kEnforceable) != 0 || g_rules == nil) {
      return nullptr;
    }

    GFHtmlViewHandler* handler = [[GFHtmlViewHandler alloc] initWithView:view];
    WKWebViewConfiguration* config = [[WKWebViewConfiguration alloc] init];
    // Nothing written to disk, nothing shared with any other view.
    config.websiteDataStore = [WKWebsiteDataStore nonPersistentDataStore];
    config.defaultWebpagePreferences.allowsContentJavaScript = NO;
    config.preferences.javaScriptCanOpenWindowsAutomatically = NO;
    // Safe Browsing looks URLs up over the network.
    config.preferences.fraudulentWebsiteWarningEnabled = NO;
    config.mediaTypesRequiringUserActionForPlayback = WKAudiovisualMediaTypeAll;
    [config.userContentController addContentRuleList:g_rules];
    [config setURLSchemeHandler:handler
                   forURLScheme:QString(HtmlView::kPartScheme).toNSString()];

    WKWebView* web_view = [[WKWebView alloc] initWithFrame:NSZeroRect
                                             configuration:config];
    web_view.navigationDelegate = handler;
    web_view.UIDelegate = handler;
    web_view.allowsBackForwardNavigationGestures = NO;
    web_view.allowsLinkPreview = NO;

    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(QWidget::createWindowContainer(
        QWindow::fromWinId(reinterpret_cast<WId>((__bridge void*)web_view)),
        widget));

    views_[view] = {handler, web_view};
    return widget;
  }

  auto Load(quint64 view, const QString& session, const QByteArray& document)
      -> bool override {
    const auto it = views_.find(view);
    if (it == views_.end()) return false;
    [it->second.handler load:session document:document into:it->second.web];
    return true;
  }

  void Clear(quint64 view) override {
    const auto it = views_.find(view);
    if (it == views_.end()) return;
    [it->second.handler clear:it->second.web];
  }

  void Destroyed(quint64 view) override {
    const auto it = views_.find(view);
    if (it == views_.end()) return;
    WKWebView* web_view = it->second.web;
    [it->second.handler detach];
    [web_view stopLoading];
    web_view.navigationDelegate = nil;
    web_view.UIDelegate = nil;
    views_.erase(it);
  }

 private:
  struct Entry {
    GFHtmlViewHandler* handler;
    WKWebView* web;
  };
  std::map<quint64, Entry> views_;
};

void RegisterCompiled() API_AVAILABLE(macos(12.0)) {
  // One renderer for the process: re-registering after a test detached it
  // gives back the same one, whose views the drain has already destroyed.
  static const auto renderer = std::make_shared<MacHtmlViewRenderer>();
  HtmlViewProviderInfo info;
  info.owner = kOwner;
  info.name = QString::fromLatin1(kPlatformHtmlViewRendererName);
  info.priority = HtmlViewService::kBuiltInPriority;
  info.enforceable = kEnforceable;
  info.restart_required = false;
  info.renderer = renderer;
  HtmlViewService::Instance().RegisterProvider(std::move(info));
}

}  // namespace

void RegisterPlatformHtmlViewRenderer() {
  // allowsContentJavaScript and the media-capture delegate are macOS 12.
  if (@available(macOS 12.0, *)) {
    if (g_rules != nil) {
      RegisterCompiled();
      return;
    }
    if (g_compiling) return;
    g_compiling = true;
    [[WKContentRuleListStore defaultStore]
        compileContentRuleListForIdentifier:kRuleListId
                     encodedContentRuleList:kRules
                          completionHandler:^(WKContentRuleList* list,
                                              NSError* error) {
                            g_compiling = false;
                            if (list == nil) {
                              // A policy that cannot be applied: no renderer,
                              // and mail stays source.
                              LOG_W() << "HTML view content rules failed to "
                                         "compile:"
                                      << QString::fromNSString(
                                             error.localizedDescription);
                              return;
                            }
                            g_rules = list;
                            RegisterCompiled();
                          }];
  } else {
    LOG_I() << "HTML views need macOS 12 or later; no native renderer";
  }
}

}  // namespace GpgFrontend::UI

#endif
