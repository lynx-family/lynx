// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <LynxDevtool/LynxRecorderReplayConfig.h>
#import <LynxDevtool/LynxRecorderURLAnalyzer.h>

@implementation LynxRecorderReplayConfig

- (id)initWithProductUrl:(NSString*)url {
  if (self = [super init]) {
    NSURL* baseURL = [NSURL URLWithString:url];

    _replayGesture = [LynxRecorderURLAnalyzer getQueryBooleanParameter:baseURL
                                                                forKey:@"gesture"
                                                          defaultValue:NO];

    _heightLimit = [LynxRecorderURLAnalyzer getQueryBooleanParameter:baseURL
                                                              forKey:@"heightLimit"
                                                        defaultValue:NO];

    _enablePreDecode = [LynxRecorderURLAnalyzer getQueryBooleanParameter:baseURL
                                                                  forKey:@"enablePreDecode"
                                                            defaultValue:NO];
    _enableAirStrictMode = [LynxRecorderURLAnalyzer getQueryBooleanParameter:baseURL
                                                                      forKey:@"enableAirStrict"
                                                                defaultValue:NO];

    _enableSizeOptimization =
        [LynxRecorderURLAnalyzer getQueryBooleanParameter:baseURL
                                                   forKey:@"enableSizeOptimization"
                                             defaultValue:NO];

    _forbidTimeFreeze = [LynxRecorderURLAnalyzer getQueryBooleanParameter:baseURL
                                                                   forKey:@"forbidTimeFreeze"
                                                             defaultValue:NO];

    _createWhenReload = [LynxRecorderURLAnalyzer getQueryBooleanParameter:baseURL
                                                                   forKey:@"createWhenReload"
                                                             defaultValue:NO];

    _disableOptPushStyleToBundle =
        [LynxRecorderURLAnalyzer getQueryBooleanParameter:baseURL
                                                   forKey:@"disable_opt_push_style_to_bundle"
                                             defaultValue:NO];

    _enableTextGradientOpt =
        [LynxRecorderURLAnalyzer getQueryBooleanParameter:baseURL
                                                   forKey:@"lynx_text_gradient_opt"
                                             defaultValue:NO];

    _enableUnifyFixedBehavior =
        [LynxRecorderURLAnalyzer getQueryBooleanParameter:baseURL
                                                   forKey:@"enable_unify_fixed_behavior"
                                             defaultValue:NO];

    _enableBatchLayoutTaskWithSyncLayout = [LynxRecorderURLAnalyzer
        getQueryBooleanParameter:baseURL
                          forKey:@"enable_batch_layout_task_with_sync_layout"
                    defaultValue:NO];

    NSString* threadModeStr = [LynxRecorderURLAnalyzer getQueryStringParameter:baseURL
                                                                        forKey:@"thread_mode"];
    if (threadModeStr != nil) {
      _thread_mode = [threadModeStr integerValue];
    } else {
      _thread_mode = -1;
    }

    _url = [LynxRecorderURLAnalyzer getQueryStringParameter:baseURL forKey:@"url"];
    _sourceUrl = [LynxRecorderURLAnalyzer getQueryStringParameter:baseURL forKey:@"source"];
    _embeddedMode =
        [[LynxRecorderURLAnalyzer getQueryStringParameter:baseURL
                                                   forKey:@"embedded_mode"] integerValue];
    if (_url == nil || [_url isEqualToString:@""]) {
      [NSException raise:@"Invalid url" format:@"%@ is invalid", url];
    }

    NSString* delayEndIntervalStr =
        [LynxRecorderURLAnalyzer getQueryStringParameter:baseURL forKey:@"delayEndInterval"];
    if (delayEndIntervalStr != nil) {
      _delayEndInterval = [delayEndIntervalStr integerValue];
    } else {
      _delayEndInterval = 3500;
    }

    CGFloat colorValues[4] = {255, 255, 255, 255};

    NSString* rgba = [LynxRecorderURLAnalyzer getQueryStringParameter:baseURL
                                                               forKey:@"backgroundColor"];

    if (rgba != nil) {
      NSArray* values = [rgba componentsSeparatedByString:@"_"];
      for (int i = 0; i < values.count && i < 4; i++) {
        colorValues[i] = [values[i] doubleValue];
      }
    }

    _backgroundColor = [UIColor colorWithRed:colorValues[0] / 255
                                       green:colorValues[1] / 255
                                        blue:colorValues[2] / 255
                                       alpha:colorValues[3] / 255];

    _canMockFuncName = [[NSSet alloc]
        initWithObjects:@"setGlobalProps", @"initialLynxView", @"loadTemplate", @"sendEventDarwin",
                        @"updateDataByPreParsedData", @"sendGlobalEvent", @"reloadTemplate",
                        @"updateConfig", @"loadTemplateBundle", @"updateMetaData",
                        @"switchEngineFromUIThread", @"updateFontScale", nil];
    _reloadFuncName = [[NSSet alloc]
        initWithObjects:@"sendGlobalEvent", @"updateDataByPreParsedData", @"sendEventDarwin", nil];

    _requestCachePolicy = NSURLRequestReloadIgnoringLocalCacheData;
  }
  return self;
}

@end
