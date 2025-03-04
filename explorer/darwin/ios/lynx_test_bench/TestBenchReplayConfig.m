// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestBenchReplayConfig.h"
#import <Foundation/Foundation.h>
#import "TestBenchURLAnalyzer.h"

@implementation TestBenchReplayConfig

- (id)initWithProductUrl:(NSString*)url {
  if (self = [super init]) {
    NSURL* baseURL = [NSURL URLWithString:url];

    _replayGesture = [TestBenchURLAnalyzer getQueryBooleanParameter:baseURL
                                                             forKey:@"gesture"
                                                       defaultValue:NO];

    _heightLimit = [TestBenchURLAnalyzer getQueryBooleanParameter:baseURL
                                                           forKey:@"heightLimit"
                                                     defaultValue:NO];

    _enablePreDecode = [TestBenchURLAnalyzer getQueryBooleanParameter:baseURL
                                                               forKey:@"enablePreDecode"
                                                         defaultValue:NO];
    _enableAirStrictMode = [TestBenchURLAnalyzer getQueryBooleanParameter:baseURL
                                                                   forKey:@"enableAirStrict"
                                                             defaultValue:NO];

    NSString* threadModeStr = [TestBenchURLAnalyzer getQueryStringParameter:baseURL
                                                                     forKey:@"thread_mode"];
    if (threadModeStr != nil) {
      _thread_mode = [threadModeStr integerValue];
    } else {
      _thread_mode = -1;
    }

    _url = [TestBenchURLAnalyzer getQueryStringParameter:baseURL forKey:@"url"];
    _sourceUrl = [TestBenchURLAnalyzer getQueryStringParameter:baseURL forKey:@"source"];
    if (_url == nil || [_url isEqualToString:@""]) {
      [NSException raise:@"Invalid url" format:@"%@ is invalid", url];
    }

    NSString* delayEndIntervalStr =
        [TestBenchURLAnalyzer getQueryStringParameter:baseURL forKey:@"delayEndInterval"];
    if (delayEndIntervalStr != nil) {
      _delayEndInterval = [delayEndIntervalStr integerValue];
    } else {
      _delayEndInterval = 3500;
    }

    _canMockFuncName = [[NSSet alloc]
        initWithObjects:@"setGlobalProps", @"initialLynxView", @"loadTemplate", @"sendEventDarwin",
                        @"updateDataByPreParsedData", @"sendGlobalEvent", @"reloadTemplate",
                        @"updateConfig", @"loadTemplateBundle", @"updateMetaData",
                        @"updateFontScale", nil];
    _reloadFuncName = [[NSSet alloc]
        initWithObjects:@"sendGlobalEvent", @"updateDataByPreParsedData", @"sendEventDarwin", nil];

    _requestCachePolicy = NSURLRequestReloadIgnoringLocalCacheData;
  }
  return self;
}

@end
