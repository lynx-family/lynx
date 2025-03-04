// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEventReporterUtils.h"
#import "LynxLog.h"

// GeneralInfo props name:
// The last loaded URL in this lynxView, will be updated when lynxView render new template.
NSString *const kPropURL = @"url";
// the relative_path would be equivalent to the url to remove applicationExternalCacheDir,
// applicationFilesDir and LocalDir.
// It can be more accurate to filter info by relative_path than by url on the tea platform.
NSString *const kPropRelativePath = @"relative_path";
// The last thread strategy this lynxView is using, will be updated when the lynxView is init.
NSString *const kPropThreadMode = @"thread_mode";
// Enable SSR.
NSString *const kPropEnableSSR = @"enable_ssr";

@interface LynxEventReporterUtils ()

@end

@implementation LynxEventReporterUtils

+ (NSString *)relativePathForURL:(NSString *)urlStr {
  if (!urlStr) {
    LLogInfo(@"LynxEventReporterUtils.relativePathForURL with nil");
    return nil;
  }
  NSString *appSupportDir = [self appSupportDir];
  if (!appSupportDir) {
    LLogInfo(@"LynxEventReporterUtils.relativePathForURL appSupportDir is nil");
    return nil;
  }
  // 0. try removing unencoded AppSupportDir
  NSString *relative_path = [urlStr stringByReplacingOccurrencesOfString:appSupportDir
                                                              withString:@""];
  // 1. try removing encoded AppSupportDir
  NSString *encodeAppSupportDir = [appSupportDir
      stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet
                                                             URLQueryAllowedCharacterSet]];
  return [relative_path stringByReplacingOccurrencesOfString:encodeAppSupportDir withString:@""];
}

#pragma mark - Private
+ (NSString *)appSupportDir {
  static NSString *kApplicationSupportPath;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    NSString *path = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,
                                                          NSUserDomainMask, YES) firstObject];
    kApplicationSupportPath = [NSString stringWithFormat:@"file://%@", path ?: @""];
  });
  return kApplicationSupportPath;
}

@end
