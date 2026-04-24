
#include <QStringList>
#include <QDebug>

namespace GpgFrontend {

#ifdef Q_OS_MACOS

#import <AppKit/AppKit.h>

bool RelaunchApplication(const QStringList& arguments) {
  @autoreleasepool {
    NSURL* bundle_url = [[NSBundle mainBundle] bundleURL];
    if (bundle_url == nil) {
      qWarning() << "failed to get main bundle url";
      return false;
    }

    NSWorkspaceOpenConfiguration* config =
        [NSWorkspaceOpenConfiguration configuration];

    config.activates = YES;

    if ([config respondsToSelector:@selector(setCreatesNewApplicationInstance:)]) {
      config.createsNewApplicationInstance = YES;
    }

    NSMutableArray<NSString*>* ns_args = [NSMutableArray array];

    for (const auto& arg : arguments) {
      [ns_args addObject:arg.toNSString()];
    }

    config.arguments = ns_args;

    [[NSWorkspace sharedWorkspace]
        openApplicationAtURL:bundle_url
               configuration:config
           completionHandler:^(NSRunningApplication* app, NSError* error) {
             if (error != nil) {
               qWarning() << "failed to relaunch application:"
                          << QString::fromNSString([error localizedDescription]);
             }
           }];

    return true;
  }
}



#endif

}  // namespace GpgFrontend