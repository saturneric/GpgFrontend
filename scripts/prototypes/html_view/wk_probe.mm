// SPDX-License-Identifier: GPL-3.0-or-later
//
// Stage 0 of the HTML View plan: does WKWebView, configured only through its
// public API, actually enforce the mail ViewPolicy? Nothing here is linked
// into GpgFrontend. It builds the same controlled outer document the host
// will build (sandboxed srcdoc iframe, CSP first in <head>), serves cid parts
// through a gfpart:// scheme handler, and logs every policy decision WebKit
// asks it for. The probe logger (probe_logger.py) records whatever got past.
//
//   probe <probe_mail.html> [seconds]     seconds = 0: stay open, click links

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

#include <string>

static void Log(NSString* what) {
  printf("POLICY %s\n", what.UTF8String);
  fflush(stdout);
}

// The host's escaping for the srcdoc attribute value. The body is never
// parsed, only made inert as an attribute. The document around it mirrors
// src/ui/html/HtmlViewDocument.cpp: keep the two in step.
static NSString* EscapeAttribute(NSString* s) {
  NSMutableString* out = [NSMutableString stringWithCapacity:s.length];
  for (NSUInteger i = 0; i < s.length; ++i) {
    unichar c = [s characterAtIndex:i];
    switch (c) {
      case '&': [out appendString:@"&amp;"]; break;
      case '"': [out appendString:@"&quot;"]; break;
      case '<': [out appendString:@"&lt;"]; break;
      case '>': [out appendString:@"&gt;"]; break;
      case 0: [out appendString:@"�"]; break;
      default: [out appendFormat:@"%C", c];
    }
  }
  return out;
}

static NSString* BuildDocument(NSString* body) {
  NSString* csp =
      @"default-src 'none'; img-src gfpart:; style-src 'unsafe-inline'; "
      @"font-src 'none'; frame-src 'none'; form-action 'none'; "
      @"base-uri 'none'";
  return [NSString
      stringWithFormat:
          @"<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
          @"<meta http-equiv=\"Content-Security-Policy\" content=\"%@\">"
          @"<meta name=\"referrer\" content=\"no-referrer\">"
          @"<meta http-equiv=\"x-dns-prefetch-control\" content=\"off\">"
          @"<style>html,body,iframe{margin:0;border:0;width:100%%;"
          @"height:100%%}</style></head><body><iframe sandbox=\"allow-popups "
          @"allow-top-navigation-by-user-activation\" "
          @"referrerpolicy=\"no-referrer\" srcdoc=\"%@\"></iframe>"
          @"</body></html>",
          csp,
          EscapeAttribute([@"<base target=\"_top\">"
              stringByAppendingString:body])];
}

static NSData* PartImage() {
  NSImage* image = [[NSImage alloc] initWithSize:NSMakeSize(16, 16)];
  [image lockFocus];
  [[NSColor systemGreenColor] setFill];
  NSRectFill(NSMakeRect(0, 0, 16, 16));
  [image unlockFocus];
  NSBitmapImageRep* rep =
      [[NSBitmapImageRep alloc] initWithData:[image TIFFRepresentation]];
  return [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
}

@interface Probe : NSObject <WKNavigationDelegate, WKUIDelegate,
                             WKURLSchemeHandler, WKDownloadDelegate>
@property(copy) NSString* session;
@property BOOL documentLoaded;
@end

@implementation Probe

// --- gfpart:// -- the only scheme anything may load from ----------------

- (void)webView:(WKWebView*)webView
    startURLSchemeTask:(id<WKURLSchemeTask>)task {
  NSURL* url = task.request.URL;
  NSString* cid = [url.path substringFromIndex:url.path.length > 0 ? 1 : 0];
  cid = cid.stringByRemovingPercentEncoding;
  BOOL ours = [url.host isEqualToString:self.session] &&
              [cid isEqualToString:@"probe-part@gpgfrontend"];
  Log([NSString stringWithFormat:@"part %@ %@", ours ? @"serve" : @"refuse",
                                 url.absoluteString]);
  if (!ours) {
    [task didFailWithError:[NSError errorWithDomain:NSURLErrorDomain
                                               code:NSURLErrorFileDoesNotExist
                                           userInfo:nil]];
    return;
  }
  NSData* data = PartImage();
  [task didReceiveResponse:[[NSURLResponse alloc]
                                         initWithURL:url
                                            MIMEType:@"image/png"
                               expectedContentLength:(NSInteger)data.length
                                    textEncodingName:nil]];
  [task didReceiveData:data];
  [task didFinish];
}

- (void)webView:(WKWebView*)webView
    stopURLSchemeTask:(id<WKURLSchemeTask>)task {
  Log([NSString stringWithFormat:@"part cancel %@", task.request.URL]);
}

// --- navigation: the first document and its srcdoc frame, nothing else ---

- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationAction:(WKNavigationAction*)action
                        preferences:(WKWebpagePreferences*)preferences
                    decisionHandler:(void (^)(WKNavigationActionPolicy,
                                              WKWebpagePreferences*))handler {
  NSURL* url = action.request.URL;
  BOOL main = action.targetFrame.isMainFrame;
  BOOL initial = main && !self.documentLoaded &&
                 [url.scheme isEqualToString:@"gfpart"] &&
                 [url.host isEqualToString:self.session];
  BOOL srcdoc = !main && [url.absoluteString isEqualToString:@"about:srcdoc"];
  preferences.allowsContentJavaScript = NO;

  if (action.shouldPerformDownload) {
    Log([NSString stringWithFormat:@"download refused %@", url]);
    handler(WKNavigationActionPolicyCancel, preferences);
    return;
  }
  if (initial || srcdoc) {
    Log([NSString stringWithFormat:@"navigate allow %@ (%@)", url,
                                   initial ? @"document" : @"srcdoc"]);
    handler(WKNavigationActionPolicyAllow, preferences);
    return;
  }
  if (action.navigationType == WKNavigationTypeLinkActivated ||
      action.navigationType == WKNavigationTypeFormSubmitted) {
    Log([NSString stringWithFormat:@"link_activated %@ (target %@)", url,
                                   action.targetFrame ? @"frame" : @"new"]);
  } else {
    Log([NSString stringWithFormat:@"navigate refuse type=%ld main=%d %@",
                                   (long)action.navigationType, main, url]);
  }
  handler(WKNavigationActionPolicyCancel, preferences);
}

- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationResponse:(WKNavigationResponse*)response
                      decisionHandler:
                          (void (^)(WKNavigationResponsePolicy))handler {
  if (!response.canShowMIMEType) {
    Log([NSString stringWithFormat:@"response refused %@",
                                   response.response.URL]);
    handler(WKNavigationResponsePolicyCancel);
    return;
  }
  handler(WKNavigationResponsePolicyAllow);
}

- (void)webView:(WKWebView*)webView
    navigationAction:(WKNavigationAction*)action
    didBecomeDownload:(WKDownload*)download {
  Log(@"download cancelled (navigation action)");
  [download cancel:nil];
}

- (void)webView:(WKWebView*)webView
    navigationResponse:(WKNavigationResponse*)response
     didBecomeDownload:(WKDownload*)download {
  Log(@"download cancelled (navigation response)");
  [download cancel:nil];
}

- (void)download:(WKDownload*)download
    decideDestinationUsingResponse:(NSURLResponse*)response
                 suggestedFilename:(NSString*)name
                 completionHandler:(void (^)(NSURL*))handler {
  handler(nil);
}

- (void)webView:(WKWebView*)webView
    didFinishNavigation:(WKNavigation*)navigation {
  self.documentLoaded = YES;
  Log(@"document finished");
}

- (void)webView:(WKWebView*)webView
    didFailProvisionalNavigation:(WKNavigation*)navigation
                       withError:(NSError*)error {
  Log([NSString stringWithFormat:@"document failed %@", error]);
}

// --- UI: no windows, no permissions, no file pickers -----------------------

- (WKWebView*)webView:(WKWebView*)webView
    createWebViewWithConfiguration:(WKWebViewConfiguration*)configuration
               forNavigationAction:(WKNavigationAction*)action
                    windowFeatures:(WKWindowFeatures*)features {
  // A target=_blank link the user clicked: the click is the Host's, the
  // window is never made. Anything else asking for a window is refused.
  if (action.navigationType == WKNavigationTypeLinkActivated) {
    Log([NSString stringWithFormat:@"link_activated %@ (popup)",
                                   action.request.URL]);
  } else {
    Log([NSString stringWithFormat:@"popup refused %@", action.request.URL]);
  }
  return nil;
}

- (void)webView:(WKWebView*)webView
    requestMediaCapturePermissionForOrigin:(WKSecurityOrigin*)origin
                          initiatedByFrame:(WKFrameInfo*)frame
                                      type:(WKMediaCaptureType)type
                           decisionHandler:
                               (void (^)(WKPermissionDecision))handler
    API_AVAILABLE(macos(12.0)) {
  Log(@"media capture refused");
  handler(WKPermissionDecisionDeny);
}

- (void)webView:(WKWebView*)webView
    runOpenPanelWithParameters:(WKOpenPanelParameters*)parameters
              initiatedByFrame:(WKFrameInfo*)frame
             completionHandler:(void (^)(NSArray<NSURL*>*))handler {
  Log(@"open panel refused");
  handler(nil);
}

@end

// Every load is blocked inside WebKit, before any of the delegates above;
// gfpart: is the one exception. Whether the rule list sees custom schemes and
// the main document at all is one of the things this prototype is for.
static NSString* const kRules =
    @"[{\"trigger\":{\"url-filter\":\".*\"},\"action\":{\"type\":\"block\"}},"
    @"{\"trigger\":{\"url-filter\":\"^gfpart:\"},"
    @"\"action\":{\"type\":\"ignore-previous-rules\"}}]";

int main(int argc, const char* argv[]) {
  @autoreleasepool {
    if (argc < 2) {
      fprintf(stderr, "usage: %s <probe_mail.html> [seconds]\n", argv[0]);
      return 2;
    }
    NSString* body = [NSString
        stringWithContentsOfFile:[NSString stringWithUTF8String:argv[1]]
                        encoding:NSUTF8StringEncoding
                           error:nil];
    if (body == nil) {
      fprintf(stderr, "cannot read %s\n", argv[1]);
      return 2;
    }
    const double seconds = argc > 2 ? atof(argv[2]) : 8.0;

    NSApplication* app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];

    Probe* probe = [Probe new];
    // 128 random bits, as the host mints them: the URL host of this session.
    probe.session =
        [[NSUUID UUID].UUIDString.lowercaseString
            stringByReplacingOccurrencesOfString:@"-"
                                      withString:@""];
    body = [body
        stringByReplacingOccurrencesOfString:@"cid:"
                                  withString:[NSString
                                                 stringWithFormat:
                                                     @"gfpart://%@/",
                                                     probe.session]];

    WKWebViewConfiguration* config = [WKWebViewConfiguration new];
    config.websiteDataStore = [WKWebsiteDataStore nonPersistentDataStore];
    config.defaultWebpagePreferences.allowsContentJavaScript = NO;
    config.preferences.javaScriptCanOpenWindowsAutomatically = NO;
    // Safe Browsing looks URLs up over the network.
    config.preferences.fraudulentWebsiteWarningEnabled = NO;
    config.mediaTypesRequiringUserActionForPlayback =
        WKAudiovisualMediaTypeAll;
    [config setURLSchemeHandler:probe forURLScheme:@"gfpart"];

    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, 900, 800)
                  styleMask:NSWindowStyleMaskTitled |
                            NSWindowStyleMaskClosable |
                            NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:NO];
    window.title = @"HTML View policy probe";

    [[WKContentRuleListStore defaultStore]
        compileContentRuleListForIdentifier:@"gf-html-view-mail"
                     encodedContentRuleList:kRules
                          completionHandler:^(WKContentRuleList* list,
                                              NSError* error) {
                            if (list == nil) {
                              Log([NSString
                                  stringWithFormat:@"rule list FAILED %@",
                                                   error]);
                              exit(3);  // a policy that cannot be applied
                            }
                            [config.userContentController
                                addContentRuleList:list];
                            WKWebView* view = [[WKWebView alloc]
                                initWithFrame:window.contentView.bounds
                                configuration:config];
                            view.autoresizingMask =
                                NSViewWidthSizable | NSViewHeightSizable;
                            view.navigationDelegate = probe;
                            view.UIDelegate = probe;
                            [window.contentView addSubview:view];
                            NSURL* base = [NSURL
                                URLWithString:[NSString
                                                  stringWithFormat:
                                                      @"gfpart://%@/",
                                                      probe.session]];
                            [view loadHTMLString:BuildDocument(body)
                                         baseURL:base];
                          }];

    [window center];
    [window makeKeyAndOrderFront:nil];
    [app activateIgnoringOtherApps:YES];
    if (seconds > 0) {
      dispatch_after(
          dispatch_time(DISPATCH_TIME_NOW, (int64_t)(seconds * NSEC_PER_SEC)),
          dispatch_get_main_queue(), ^{
            Log(@"done");
            [app terminate:nil];
          });
    }
    [app run];
  }
  return 0;
}
