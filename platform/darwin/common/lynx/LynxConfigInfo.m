// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxConfigInfo.h"
#import "LynxView.h"

@implementation LynxConfigInfo

- (instancetype)initWithBuilder:(LynxConfigInfoBuilder*)builder {
  self = [super init];
  if (self) {
    _pageVersion = builder.pageVersion;
    _pageType = builder.pageType;
    _cliVersion = builder.cliVersion;
    _customData = builder.customData;
    _templateUrl = builder.templateUrl;
    _targetSdkVersion = builder.targetSdkVersion;
    _lepusVersion = builder.lepusVersion;
    _threadStrategyForRendering = builder.threadStrategyForRendering;
    _enableLepusNG = builder.enableLepusNG;
    _radonMode = builder.radonMode;
    _reactVersion = builder.reactVersion;
    _registeredComponent = builder.registeredComponent;
    _cssAlignWithLegacyW3c = builder.cssAlignWithLegacyW3c;
    _enableCSSParser = builder.enableCSSParser;
  }
  return self;
}

- (NSString*)lnxThreadStrategyForRenderToString:(LynxThreadStrategyForRender)strategy {
  NSString* result = nil;
  switch (strategy) {
    case LynxThreadStrategyForRenderAllOnUI:
      result = @"LynxThreadStrategyForRenderAllOnUI";
      break;
    case LynxThreadStrategyForRenderMostOnTASM:
      result = @"LynxThreadStrategyForRenderMostOnTASM";
      break;
    case LynxThreadStrategyForRenderPartOnLayout:
      result = @"LynxThreadStrategyForRenderPartOnLayout";
      break;
    case LynxThreadStrategyForRenderMultiThreads:
      result = @"LynxThreadStrategyForRenderMultiThreads";
      break;
    default:
      [NSException raise:NSGenericException format:@"Unexpected FormatType."];
  }
  return result;
}

- (NSData*)json {
  NSError* jsonError;
  NSMutableDictionary* jsonDict = [NSMutableDictionary dictionary];
  [jsonDict setValue:_pageVersion forKey:@"pageVersion"];
  [jsonDict setValue:_pageType forKey:@"pageType"];
  [jsonDict setValue:_cliVersion forKey:@"cliVersion"];
  [jsonDict setValue:_customData forKey:@"customData"];
  [jsonDict setValue:_templateUrl forKey:@"templateUrl"];
  [jsonDict setValue:_targetSdkVersion forKey:@"targetSdkVersion"];
  [jsonDict setValue:_lepusVersion forKey:@"lepusVersion"];
  [jsonDict setValue:[self lnxThreadStrategyForRenderToString:_threadStrategyForRendering]
              forKey:@"threadStrategyForRendering"];
  [jsonDict setValue:_enableLepusNG ? @"True" : @"False" forKey:@"enableLepusNG"];
  [jsonDict setValue:_radonMode forKey:@"radonMode"];
  [jsonDict setValue:_reactVersion forKey:@"reactVersion"];
  [jsonDict setValue:[_registeredComponent allObjects] forKey:@"registeredComponent"];
  [jsonDict setValue:_enableCSSParser ? @"True" : @"False" forKey:@"enableCSSParser"];

  return [NSJSONSerialization dataWithJSONObject:jsonDict options:0 error:&jsonError];
}

@end

@implementation LynxConfigInfoBuilder

- (instancetype)init {
  self = [super init];
  if (self) {
    _pageVersion = @"error";
    _pageType = @"error";
    _cliVersion = @"error";
    _customData = @"{}";
    _templateUrl = @"error";
    _targetSdkVersion = @"error";
    _lepusVersion = @"error";
    _threadStrategyForRendering = 0;
    _enableLepusNG = false;
    _radonMode = @"error";
    _reactVersion = @"error";
    _registeredComponent = NULL;
  }
  return self;
}

- (LynxConfigInfo*)build {
  return [[LynxConfigInfo alloc] initWithBuilder:self];
}

@end
