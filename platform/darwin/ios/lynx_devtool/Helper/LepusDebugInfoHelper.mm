// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LepusDebugInfoHelper.h"
#import "LynxDevToolDownloader.h"
#import "LynxLog.h"

@implementation LepusDebugInfoHelper {
  BOOL _waitFlag;
  NSString* _debugInfo;
}

- (instancetype)init {
  if (self = [super init]) {
    _waitFlag = NO;
    _debugInfo = @"";
  }
  return self;
}

- (std::string)getDebugInfo:(const std::string&)url {
  LLogInfo(@"lepus debug: debug info url: %s", url.c_str());
  [self setDebugInfoUrl:[NSString stringWithUTF8String:url.c_str()]];
  _waitFlag = YES;
  [self downloadDebugInfo];

  // Wait for downloading
  while (_waitFlag) {
    [NSThread sleepForTimeInterval:0.01];
  }

  return [_debugInfo UTF8String];
}

- (void)downloadDebugInfo {
  __weak typeof(self) weakSelf = self;
  [LynxDevToolDownloader download:_debugInfoUrl
                     withCallback:^(NSData* _Nullable data, NSError* _Nullable error) {
                       __strong typeof(weakSelf) strongSelf = weakSelf;
                       if (strongSelf == nil) {
                         LLogError(@"lepus debug: LepusDebugInfoHelper has been destroyed!");
                         return;
                       }
                       if (error != nil) {
                         strongSelf->_debugInfo = @"";
                         LLogError(@"lepus debug: download debug info failed, the reason is: %@",
                                   [error localizedFailureReason]);
                       } else {
                         strongSelf->_debugInfo =
                             [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
                       }
                       strongSelf->_waitFlag = NO;
                     }];
}

@end
