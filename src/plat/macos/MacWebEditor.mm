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

#include <QVBoxLayout>
#include <QWindow>
#include <map>
#include <memory>
#include <utility>

#include "plat/common/PlatformWebEditor.h"
#include "ui/html/WebEditorDocument.h"
#include "ui/html/WebEditorService.h"

#import <WebKit/WebKit.h>

namespace WebEditor = GpgFrontend::UI::WebEditor;
using GpgFrontend::UI::WebEditorService;

namespace {

/// The Host's own engine, as the service knows it: the empty owner.
const QString kOwner;

/// Every load is refused inside WebKit, before any delegate is asked and
/// main frame included; the editor's two schemes are the exceptions, and
/// both are answered by the Host alone.
NSString* const kRuleListId = @"gf-web-editor";
NSString* const kRules =
    @"[{\"trigger\":{\"url-filter\":\".*\"},\"action\":{\"type\":\"block\"}},"
    @"{\"trigger\":{\"url-filter\":\"^gfasset:\"},"
    @"\"action\":{\"type\":\"ignore-previous-rules\"}},"
    @"{\"trigger\":{\"url-filter\":\"^gfres:\"},"
    @"\"action\":{\"type\":\"ignore-previous-rules\"}}]";

/// The page's one way to the Host.
NSString* const kBridgeName = @"gfHost";

/// `window.gfBridge` ({post(obj), onMessage(fn)}), as on every engine, and
/// the one function the Host calls to deliver. Injected by the engine, not
/// served: the page's policy does not apply to it, and the page cannot
/// replace it.
constexpr auto kBridgeScript = R"JS(
(function () {
  "use strict";
  var host = window.webkit.messageHandlers.gfHost;
  var send = host.postMessage.bind(host);
  var listeners = [];
  function fixed(name, value) {
    Object.defineProperty(window, name, {
      configurable: false, enumerable: false, writable: false, value: value
    });
  }
  fixed("gfBridge", Object.freeze({
    post: function (obj) { send(JSON.stringify(obj)); },
    onMessage: function (fn) { if (typeof fn === "function") listeners.push(fn); }
  }));
  fixed("__gfBridgeDeliver", function (json) {
    var obj;
    try { obj = JSON.parse(json); } catch (e) { return; }
    listeners.forEach(function (fn) { fn(obj); });
  });
})();
)JS";

/// What the Host says when the page's process went away.
constexpr auto kCrashed = R"({"type":"engine.crashed"})";

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
 * @brief Holds its handler weakly: a content controller keeps every message
 *        handler it is given alive, and the handler owns nothing of it.
 */
@interface GFWebEditorBridgeProxy : NSObject <WKScriptMessageHandler>
@property(nonatomic, weak) id<WKScriptMessageHandler> target;
@end

@implementation GFWebEditorBridgeProxy
- (void)userContentController:(WKUserContentController*)controller
      didReceiveScriptMessage:(WKScriptMessage*)message {
  [self.target userContentController:controller didReceiveScriptMessage:message];
}
@end

/**
 * @brief One editor's engine side: the two schemes, the bridge, and every
 *        question WebKit asks it.
 *
 * Everything here runs on the main thread, which is the GUI thread.
 */
API_AVAILABLE(macos(12.0))
@interface GFWebEditorHandler : NSObject <WKNavigationDelegate, WKUIDelegate,
                                          WKURLSchemeHandler,
                                          WKScriptMessageHandler>
- (instancetype)initWithEditor:(quint64)editor session:(const QString&)session;
/// (Re)load the editor's page, its one allowed main-frame load.
- (void)loadInto:(WKWebView*)web_view;
/// Deliver one JSON object, or keep it until the page is ready for it.
- (void)post:(const QByteArray&)json to:(WKWebView*)web_view;
/// The editor is gone: nothing it still hears reaches the service.
- (void)detach;
@end

@implementation GFWebEditorHandler {
  quint64 editor_;
  QString session_;
  NSURL* index_;
  /// The one main-frame load allowed next, taken when it is asked about.
  NSURL* expected_main_;
  /// The page's own navigation, the only one Ready() is about.
  WKNavigation* expected_navigation_;
  bool awaiting_load_;
  bool ready_;
  bool detached_;
  /// What the consumer posted before the page could hear it.
  QList<QByteArray> outbox_;
}

- (instancetype)initWithEditor:(quint64)editor session:(const QString&)session {
  if ((self = [super init])) {
    editor_ = editor;
    session_ = session;
    index_ = [NSURL
        URLWithString:QString("%1://%2/index.html")
                          .arg(QLatin1StringView(WebEditor::kAssetScheme),
                               session)
                          .toNSString()];
    awaiting_load_ = false;
    ready_ = false;
    detached_ = false;
  }
  return self;
}

- (void)dealloc {
  for (auto& json : outbox_) Wipe(json);
}

- (void)loadInto:(WKWebView*)web_view {
  if (detached_) return;
  ready_ = false;
  expected_main_ = index_;
  awaiting_load_ = true;
  expected_navigation_ =
      [web_view loadRequest:[NSURLRequest requestWithURL:index_]];
}

- (void)post:(const QByteArray&)json to:(WKWebView*)web_view {
  if (detached_) return;
  if (!ready_) {
    outbox_.append(json);
    return;
  }
  // An argument, not source: nothing in it is ever parsed as script.
  [web_view callAsyncJavaScript:@"window.__gfBridgeDeliver(json);"
                      arguments:@{@"json" : QString::fromUtf8(json).toNSString()}
                        inFrame:nil
                 inContentWorld:WKContentWorld.pageWorld
              completionHandler:nil];
}

- (void)detach {
  detached_ = true;
  ready_ = false;
  awaiting_load_ = false;
  expected_navigation_ = nil;
  expected_main_ = nil;
  for (auto& json : outbox_) Wipe(json);
  outbox_.clear();
}

- (bool)takeExpectedMainLoad:(NSURL*)url {
  if (expected_main_ == nil || ![url isEqual:expected_main_]) return false;
  expected_main_ = nil;
  return true;
}

- (void)reportLink:(NSURL*)url {
  if (detached_ || url == nil) return;
  WebEditorService::Instance().LinkActivated(
      kOwner, editor_, QString::fromNSString(url.absoluteString));
}

// --- gfasset:// and gfres:// -- whatever the Host serves, nothing else -------

- (void)webView:(WKWebView*)webView
    startURLSchemeTask:(id<WKURLSchemeTask>)task {
  NSURL* url = task.request.URL;
  if (detached_ || url == nil ||
      ![task.request.HTTPMethod isEqualToString:@"GET"]) {
    [task didFailWithError:Fail()];
    return;
  }
  GpgFrontend::GFBuffer data;
  QString mime;
  const auto status = WebEditorService::Instance().Resolve(
      kOwner, editor_, QString::fromNSString(url.absoluteString), &data, &mime);
  if (status != WebEditorService::kOK) {
    [task didFailWithError:Fail()];
    return;
  }

  NSMutableDictionary<NSString*, NSString*>* headers = [@{
    @"Content-Type" : (mime.startsWith(u"text/"_s) ? mime + u"; charset=utf-8"_s
                                                   : mime)
                          .toNSString(),
    @"Content-Length" : [NSString stringWithFormat:@"%zu", data.Size()],
    @"Cache-Control" : @"no-store",
  } mutableCopy];
  // The page carries the policy already; a header says it again, where the
  // page has no say at all.
  if (mime == u"text/html"_s) {
    headers[@"Content-Security-Policy"] =
        WebEditor::ContentSecurityPolicy().toNSString();
  }
  [task didReceiveResponse:[[NSHTTPURLResponse alloc] initWithURL:url
                                                       statusCode:200
                                                      HTTPVersion:@"HTTP/1.1"
                                                     headerFields:headers]];
  [task didReceiveData:[NSData dataWithBytes:data.Data() length:data.Size()]];
  [task didFinish];
}

- (void)webView:(WKWebView*)webView
    stopURLSchemeTask:(id<WKURLSchemeTask>)task {
  // Every task is answered before it is returned from; none is left to stop.
}

// --- the bridge: one string from the page's own document ---------------------

- (void)userContentController:(WKUserContentController*)controller
      didReceiveScriptMessage:(WKScriptMessage*)message {
  if (detached_ || !message.frameInfo.isMainFrame) return;
  WKSecurityOrigin* origin = message.frameInfo.securityOrigin;
  if (![origin.protocol
          isEqualToString:QString(WebEditor::kAssetScheme).toNSString()] ||
      QString::fromNSString(origin.host) != session_) {
    return;
  }
  if (![message.body isKindOfClass:[NSString class]]) return;
  // Checked by the service; its content is never logged.
  WebEditorService::Instance().Message(
      kOwner, editor_, QString::fromNSString(message.body).toUtf8());
}

// --- navigation: the page itself, once; nothing else -------------------------

- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationAction:(WKNavigationAction*)action
                        preferences:(WKWebpagePreferences*)preferences
                    decisionHandler:(void (^)(WKNavigationActionPolicy,
                                              WKWebpagePreferences*))handler {
  NSURL* url = action.request.URL;
  WKFrameInfo* target = action.targetFrame;
  const bool main = target != nil && target.isMainFrame;

  // Script runs in the module's own page, under the Host's policy, and in
  // nothing else.
  preferences.allowsContentJavaScript = NO;

  if (detached_ || action.shouldPerformDownload) {
    handler(WKNavigationActionPolicyCancel, preferences);
    return;
  }
  if (main && [self takeExpectedMainLoad:url]) {
    preferences.allowsContentJavaScript = YES;
    handler(WKNavigationActionPolicyAllow, preferences);
    return;
  }
  // A link the user activated is the Host's to open, outside the editor --
  // target=_blank included, which arrives here with no target frame and so
  // never reaches createWebView. Everything else is simply refused.
  if (action.navigationType == WKNavigationTypeLinkActivated) {
    [self reportLink:url];
  } else {
    LOG_D() << "web editor" << editor_ << "refused navigation type"
            << static_cast<int>(action.navigationType)
            << (main ? "main" : "sub") << "frame";
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

- (void)webView:(WKWebView*)webView
    didFinishNavigation:(WKNavigation*)navigation {
  if (detached_ || !awaiting_load_ || navigation != expected_navigation_) {
    return;
  }
  awaiting_load_ = false;
  expected_navigation_ = nil;
  ready_ = true;
  // What was posted early, in order, before anything the consumer posts on
  // hearing that the page is ready.
  const auto early = std::exchange(outbox_, {});
  for (auto json : early) {
    [self post:json to:webView];
    Wipe(json);
  }
  WebEditorService::Instance().Ready(kOwner, editor_);
}

- (void)webView:(WKWebView*)webView
    didFailProvisionalNavigation:(WKNavigation*)navigation
                       withError:(NSError*)error {
  [self loadFailed:navigation];
}

- (void)webView:(WKWebView*)webView
    didFailNavigation:(WKNavigation*)navigation
            withError:(NSError*)error {
  [self loadFailed:navigation];
}

// Every refused navigation ends in a failure too; only the page's own is the
// editor failing.
- (void)loadFailed:(WKNavigation*)navigation {
  if (detached_ || !awaiting_load_ || navigation != expected_navigation_) {
    return;
  }
  awaiting_load_ = false;
  expected_navigation_ = nil;
  LOG_W() << "web editor" << editor_ << "could not load its page";
}

// A crashed process took the document with it: say so, then start again.
- (void)webViewWebContentProcessDidTerminate:(WKWebView*)webView {
  LOG_W() << "web editor" << editor_ << "lost its web content process";
  if (detached_) return;
  ready_ = false;
  WebEditorService::Instance().Message(kOwner, editor_, kCrashed);
  // Hearing it, the consumer may have destroyed the editor.
  [self loadInto:webView];
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

class API_AVAILABLE(macos(12.0)) MacWebEditorEngine : public WebEditorEngine {
 public:
  auto Create(quint64 editor, const QString& session) -> QWidget* override {
    if (g_rules == nil) return nullptr;

    GFWebEditorHandler* handler =
        [[GFWebEditorHandler alloc] initWithEditor:editor session:session];
    GFWebEditorBridgeProxy* proxy = [[GFWebEditorBridgeProxy alloc] init];
    proxy.target = handler;

    WKWebViewConfiguration* config = [[WKWebViewConfiguration alloc] init];
    // Nothing written to disk, nothing shared with any other editor.
    config.websiteDataStore = [WKWebsiteDataStore nonPersistentDataStore];
    config.defaultWebpagePreferences.allowsContentJavaScript = NO;
    config.preferences.javaScriptCanOpenWindowsAutomatically = NO;
    // Safe Browsing looks URLs up over the network.
    config.preferences.fraudulentWebsiteWarningEnabled = NO;
    config.mediaTypesRequiringUserActionForPlayback = WKAudiovisualMediaTypeAll;
    WKUserContentController* content = config.userContentController;
    [content addContentRuleList:g_rules];
    [content addUserScript:[[WKUserScript alloc]
                                 initWithSource:@(kBridgeScript)
                                  injectionTime:
                                      WKUserScriptInjectionTimeAtDocumentStart
                               forMainFrameOnly:YES
                                 inContentWorld:WKContentWorld.pageWorld]];
    [content addScriptMessageHandler:proxy
                        contentWorld:WKContentWorld.pageWorld
                                name:kBridgeName];
    for (const auto* scheme :
         {WebEditor::kAssetScheme, WebEditor::kResourceScheme}) {
      [config setURLSchemeHandler:handler
                     forURLScheme:QString(scheme).toNSString()];
    }

    WKWebView* web_view = [[WKWebView alloc] initWithFrame:NSZeroRect
                                             configuration:config];
    web_view.navigationDelegate = handler;
    web_view.UIDelegate = handler;
    web_view.allowsBackForwardNavigationGestures = NO;
    web_view.allowsLinkPreview = NO;

    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    // The offscreen platform the tests run under has no foreign windows; the
    // page still loads and talks there, unseen.
    if (auto* window = QWindow::fromWinId(
            reinterpret_cast<WId>((__bridge void*)web_view))) {
      layout->addWidget(QWidget::createWindowContainer(window, widget));
    }

    editors_[editor] = {handler, web_view};
    [handler loadInto:web_view];
    return widget;
  }

  auto Post(quint64 editor, const QByteArray& json) -> bool override {
    const auto it = editors_.find(editor);
    if (it == editors_.end()) return false;
    [it->second.handler post:json to:it->second.web];
    return true;
  }

  void Destroyed(quint64 editor) override {
    const auto it = editors_.find(editor);
    if (it == editors_.end()) return;
    WKWebView* web_view = it->second.web;
    [it->second.handler detach];
    [web_view stopLoading];
    web_view.navigationDelegate = nil;
    web_view.UIDelegate = nil;
    WKUserContentController* content = web_view.configuration.userContentController;
    [content removeScriptMessageHandlerForName:kBridgeName
                                  contentWorld:WKContentWorld.pageWorld];
    [content removeAllUserScripts];
    editors_.erase(it);
  }

 private:
  struct Entry {
    GFWebEditorHandler* handler;
    WKWebView* web;
  };
  std::map<quint64, Entry> editors_;
};

void RegisterCompiled() API_AVAILABLE(macos(12.0)) {
  // One engine for the process: re-registering after a test detached it
  // gives back the same one, whose editors the drain has already destroyed.
  static const auto engine = std::make_shared<MacWebEditorEngine>();
  WebEditorProviderInfo info;
  info.owner = kOwner;
  info.name = QString::fromLatin1(kPlatformWebEditorEngineName);
  info.priority = WebEditorService::kBuiltInPriority;
  info.restart_required = false;
  info.engine = engine;
  WebEditorService::Instance().RegisterProvider(std::move(info));
}

}  // namespace

void RegisterPlatformWebEditorEngine() {
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
                              // A policy that cannot be applied: no engine,
                              // and editors stay unavailable.
                              LOG_W() << "web editor content rules failed to "
                                         "compile:"
                                      << QString::fromNSString(
                                             error.localizedDescription);
                              return;
                            }
                            g_rules = list;
                            RegisterCompiled();
                          }];
  } else {
    LOG_I() << "web editors need macOS 12 or later; no native engine";
  }
}

}  // namespace GpgFrontend::UI

#endif
