// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEnv.h"
#import "LynxFontFaceManager.h"
#import "LynxLazyRegister.h"
#import "LynxLog.h"
#import "LynxTraceEvent.h"
#import "LynxUIRenderer.h"
#import "LynxViewBuilder+Internal.h"

@implementation LynxViewBuilder {
  LynxThreadStrategyForRender _threadStrategy;
  NSMutableDictionary<NSString*, LynxAliasFontInfo*>* _builderRegisteredAliasFontMap;
}

- (id)init {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewBuilder::init");
  self = [super init];
  if (self) {
    [LynxLazyRegister loadLynxInitTask];
    self.enableAutoExpose = YES;
    self.fontScale = 1.0;
    self.enableTextNonContiguousLayout = NO;
    self.enableLayoutOnly = [LynxEnv.sharedInstance getEnableLayoutOnly];
    self.debuggable = false;
    self.enableGenericResourceFetcher = LynxBooleanOptionUnset;
    _builderRegisteredAliasFontMap = [NSMutableDictionary dictionary];
    _lynxBackgroundRuntimeOptions = [[LynxBackgroundRuntimeOptions alloc] init];
    _lynxUIRenderer = [[LynxUIRenderer alloc] init];
  }
  return self;
}

- (LynxThreadStrategyForRender)getThreadStrategyForRender {
  return _threadStrategy;
}

- (void)setThreadStrategyForRender:(LynxThreadStrategyForRender)threadStrategy {
  switch (threadStrategy) {
    case LynxThreadStrategyForRenderAllOnUI:
      _threadStrategy = LynxThreadStrategyForRenderAllOnUI;
      break;
    case LynxThreadStrategyForRenderPartOnLayout:
      _threadStrategy = LynxThreadStrategyForRenderPartOnLayout;
      break;
    case LynxThreadStrategyForRenderMostOnTASM:
      _threadStrategy = LynxThreadStrategyForRenderMostOnTASM;
      break;
    case LynxThreadStrategyForRenderMultiThreads:
      _threadStrategy = LynxThreadStrategyForRenderMultiThreads;
      break;
    default:
      _LogE(@"invalid value used for thread rendering strategy, please use enum "
            @"'LynxThreadStrategyForRender' defined in LynxView.mm");
      _threadStrategy = LynxThreadStrategyForRenderAllOnUI;
      break;
  }
}

- (void)addLynxResourceProvider:(NSString*)resType provider:(id<LynxResourceProvider>)provider {
  [_lynxBackgroundRuntimeOptions addLynxResourceProvider:resType provider:provider];
}

- (NSDictionary*)getLynxResourceProviders {
  return [_lynxBackgroundRuntimeOptions providers];
}

- (void)registerFont:(UIFont*)font forName:(NSString*)name {
  if ([name length] == 0) {
    return;
  }

  LynxAliasFontInfo* info = [_builderRegisteredAliasFontMap objectForKey:name];
  if (info == nil) {
    if (font != nil) {
      info = [LynxAliasFontInfo new];
      info.font = font;
      [_builderRegisteredAliasFontMap setObject:info forKey:name];
    }
  } else {
    info.font = font;
    if ([info isEmpty]) {
      [_builderRegisteredAliasFontMap removeObjectForKey:name];
    }
  }
}

- (void)registerFamilyName:(NSString*)fontFamilyName withAliasName:(NSString*)aliasName {
  if ([aliasName length] == 0) {
    return;
  }

  LynxAliasFontInfo* info = [_builderRegisteredAliasFontMap objectForKey:aliasName];
  if (info == nil) {
    if (fontFamilyName != nil) {
      info = [LynxAliasFontInfo new];
      info.name = fontFamilyName;
      [_builderRegisteredAliasFontMap setObject:info forKey:aliasName];
    }
  } else {
    info.name = fontFamilyName;
    if ([info isEmpty]) {
      [_builderRegisteredAliasFontMap removeObjectForKey:aliasName];
    }
  }
}

- (NSDictionary*)getBuilderRegisteredAliasFontMap {
  return _builderRegisteredAliasFontMap;
}

// group had moved to _lynxBackgroundRuntimeOptions
- (LynxGroup*)group {
  return _lynxBackgroundRuntimeOptions.group;
}
- (void)setGroup:(LynxGroup*)group {
  [_lynxBackgroundRuntimeOptions setGroup:group];
}

@dynamic backgroundJsRuntimeType;
- (LynxBackgroundJsRuntimeType)backgroundJsRuntimeType {
  return _lynxBackgroundRuntimeOptions.backgroundJsRuntimeType;
}

- (void)setBackgroundJsRuntimeType:(LynxBackgroundJsRuntimeType)type {
  [_lynxBackgroundRuntimeOptions setBackgroundJsRuntimeType:type];
}

@dynamic enableBytecode;
- (BOOL)enableBytecode {
  return _lynxBackgroundRuntimeOptions.enableBytecode;
}

- (void)setEnableBytecode:(BOOL)enableBytecode {
  [_lynxBackgroundRuntimeOptions setEnableBytecode:enableBytecode];
}

@dynamic bytecodeUrl;
- (NSString*)bytecodeUrl {
  return _lynxBackgroundRuntimeOptions.bytecodeUrl;
}
- (void)setBytecodeUrl:(NSString*)bytecodeUrl {
  [_lynxBackgroundRuntimeOptions setBytecodeUrl:bytecodeUrl];
}

- (id<LynxGenericResourceFetcher>)genericResourceFetcher {
  return _lynxBackgroundRuntimeOptions.genericResourceFetcher;
}

- (void)setGenericResourceFetcher:(id<LynxGenericResourceFetcher>)genericResourceFetcher {
  _lynxBackgroundRuntimeOptions.genericResourceFetcher = genericResourceFetcher;
}

- (id<LynxMediaResourceFetcher>)mediaResourceFetcher {
  return _lynxBackgroundRuntimeOptions.mediaResourceFetcher;
}

- (void)setMediaResourceFetcher:(id<LynxMediaResourceFetcher>)mediaResourceFetcher {
  _lynxBackgroundRuntimeOptions.mediaResourceFetcher = mediaResourceFetcher;
}

- (id<LynxTemplateResourceFetcher>)templateResourceFetcher {
  return _lynxBackgroundRuntimeOptions.templateResourceFetcher;
}

- (void)setTemplateResourceFetcher:(id<LynxTemplateResourceFetcher>)templateResourceFetcher {
  _lynxBackgroundRuntimeOptions.templateResourceFetcher = templateResourceFetcher;
}

- (id<LynxUIRendererProtocol>)lynxUIRenderer {
  return _lynxUIRenderer;
}

@end
