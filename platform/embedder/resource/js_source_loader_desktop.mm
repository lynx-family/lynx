// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/resource/js_source_loader_desktop.h"

#import <Foundation/Foundation.h>
#include "core/renderer/utils/lynx_env.h"

@interface JSSourceLoaderMac : NSObject
+ (instancetype)sharedInstance;
@end

@implementation JSSourceLoaderMac
+ (instancetype)sharedInstance {
  static JSSourceLoaderMac* instance;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[JSSourceLoaderMac alloc] init];
  });
  return instance;
}
@end

namespace lynx {
namespace runtime {
namespace js {
static NSString* FILE_SCHEME = @"file://";
static NSString* CORE_DEBUG_JS = @"lynx_core_dev";
static NSString* ASSETS_SCHEME = @"assets://";
static NSString* ASSETS_CORE_SCHEME = @"assets://lynx_core.js";

std::string JSSourceLoaderDesktop::LoadJSSource(const std::string& name) {
  [JSSourceLoaderMac sharedInstance];
  NSString* str = [NSString stringWithUTF8String:name.c_str()];

  NSString* path = nil;

  if ([ASSETS_CORE_SCHEME isEqualToString:str]) {
    str = [str componentsSeparatedByString:@"."][0];
    NSURL* debugBundleUrl = [[NSBundle mainBundle] URLForResource:@"LynxDebugResources"
                                                    withExtension:@"bundle"];

    if (path == nil && debugBundleUrl && lynx::tasm::LynxEnv::GetInstance().IsDevToolEnabled()) {
      NSBundle* bundle = [NSBundle bundleWithURL:debugBundleUrl];
      path = [bundle pathForResource:CORE_DEBUG_JS ofType:@"js"];
    }

    if (path == nil) {
      NSURL* bundleUrl = [[NSBundle mainBundle] URLForResource:@"LynxResources"
                                                 withExtension:@"bundle"];
      if (bundleUrl) {
        NSBundle* bundle = [NSBundle bundleWithURL:bundleUrl];
        path = [bundle pathForResource:[str substringFromIndex:[ASSETS_SCHEME length]]
                                ofType:@"js"];
      }
    }

    if (path == nil) {
      NSBundle* frameworkBundle = [NSBundle bundleForClass:[JSSourceLoaderMac class]];
      NSURL* bundleUrl = [frameworkBundle URLForResource:@"LynxResources" withExtension:@"bundle"];
      if (bundleUrl) {
        NSBundle* bundle = [NSBundle bundleWithURL:bundleUrl];
        path = [bundle pathForResource:[str substringFromIndex:[ASSETS_SCHEME length]]
                                ofType:@"js"];
      }
    }
  } else if ([str length] > [FILE_SCHEME length] && [str hasPrefix:FILE_SCHEME]) {
    NSString* filePath = [str substringFromIndex:[FILE_SCHEME length]];
    if ([filePath hasPrefix:@"/"]) {
      // URL parsing removes the query/fragment and decodes the path once.
      path = [NSURL URLWithString:str].path;
    } else {
      // Preserve the legacy file://<cache-relative-path> convention.
      NSRange suffix = [filePath
          rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"?#"]];
      if (suffix.location != NSNotFound) {
        filePath = [filePath substringToIndex:suffix.location];
      }
      filePath = filePath.stringByRemovingPercentEncoding;
      if (filePath.length == 0) {
        return "";
      }
      NSString* cachePath = [NSSearchPathForDirectoriesInDomains(
          NSCachesDirectory, NSUserDomainMask, YES) firstObject];
      path = [cachePath stringByAppendingPathComponent:filePath];
    }
  } else if ([str length] > [ASSETS_SCHEME length] && [str hasPrefix:ASSETS_SCHEME]) {
    NSRange suffix =
        [str rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"?#"]];
    if (suffix.location != NSNotFound) {
      str = [str substringToIndex:suffix.location];
    }
    NSRange range = [str rangeOfString:@"." options:NSBackwardsSearch];
    if (range.location == NSNotFound) {
      return "";
    }
    str = [str substringToIndex:range.location];
    path = [[NSBundle mainBundle]
        pathForResource:[@"Resource/"
                            stringByAppendingString:[str substringFromIndex:[ASSETS_SCHEME length]]]
                 ofType:@"js"];
  }
  if (path) {
    NSData* data = [NSData dataWithContentsOfFile:path];
    return data.length ? std::string(static_cast<const char*>(data.bytes), data.length) : "";
  }
  return "";
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
