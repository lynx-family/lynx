// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseConfigurator.h>
#import <Lynx/LynxFontFaceManager.h>
#import <Lynx/LynxLazyRegister.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxTraceEvent.h>
#import "LynxBaseConfigurator+Internal.h"
#import "LynxTraceEventDef.h"
#import "LynxUIRenderer.h"
#import "LynxUIRendererCreator.h"

@implementation LynxBaseConfigurator {
  LynxEmbeddedMode _embeddedMode;
  NSMutableDictionary<NSString*, LynxAliasFontInfo*>* _builderRegisteredAliasFontMap;
}

- (id)init {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_VIEW_BUILDER_INIT);
  self = [super init];
  if (self) {
    [LynxLazyRegister loadLynxInitTask];
    self.enableAutoExpose = YES;
    self.fontScale = 1.0;
    self.enableTextNonContiguousLayout = NO;
    self.enableLayoutOnly = YES;
    self.debuggable = false;
    self.enableGenericResourceFetcher = LynxBooleanOptionUnset;
    self.enableSharedModule = NO;
    self.uiRendererCreator = [[LynxUIRendererCreator alloc] init];
    self.lynxBackgroundRuntimeOptions = [[LynxBackgroundRuntimeOptions alloc] init];
    _builderRegisteredAliasFontMap = [NSMutableDictionary dictionary];
    _threadStrategy = LynxThreadStrategyForRenderAllOnUI;
    _hasThreadStrategySet = NO;
    _hasPendingJsTaskSet = NO;
  }
  return self;
}

- (LynxEmbeddedMode)getEmbeddedMode {
  return _embeddedMode;
}

- (void)setEmbeddedMode:(LynxEmbeddedMode)embeddedMode {
  _embeddedMode = embeddedMode;
  if ((embeddedMode & LynxEmbeddedModeUnset) > 0) {
    [LynxComponentScopeRegistry tryRegisterBuiltInClasses];
  }
}

- (LynxThreadStrategyForRender)getThreadStrategyForRender {
  return _threadStrategy;
}

- (void)setThreadStrategyForRender:(LynxThreadStrategyForRender)threadStrategy {
  _hasThreadStrategySet = YES;
  switch (threadStrategy) {
    case LynxThreadStrategyForRenderAllOnUI:
    case LynxThreadStrategyForRenderPartOnLayout:
    case LynxThreadStrategyForRenderMostOnTASM:
    case LynxThreadStrategyForRenderMultiThreads:
      _threadStrategy = threadStrategy;
      break;
    default:
      _LogE(@"invalid value used for thread rendering strategy, please use enum "
            @"'LynxThreadStrategyForRender' defined in LynxView.mm");
      _threadStrategy = LynxThreadStrategyForRenderAllOnUI;
      break;
  }
}

- (void)setEnablePendingJSTaskOnLayout:(BOOL)enablePendingJSTaskOnLayout {
  _hasPendingJsTaskSet = YES;
  _enablePendingJSTaskOnLayout = enablePendingJSTaskOnLayout;
}

- (void)addLynxResourceProvider:(NSString*)resType provider:(id<LynxResourceProvider>)provider {
  [self.lynxBackgroundRuntimeOptions addLynxResourceProvider:resType provider:provider];
}

- (NSDictionary*)getLynxResourceProviders {
  return [self.lynxBackgroundRuntimeOptions providers];
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
  return self.lynxBackgroundRuntimeOptions.group;
}
- (void)setGroup:(LynxGroup*)group {
  [self.lynxBackgroundRuntimeOptions setGroup:group];
}

@dynamic backgroundJsRuntimeType;
- (LynxBackgroundJsRuntimeType)backgroundJsRuntimeType {
  return self.lynxBackgroundRuntimeOptions.backgroundJsRuntimeType;
}

- (void)setBackgroundJsRuntimeType:(LynxBackgroundJsRuntimeType)type {
  [self.lynxBackgroundRuntimeOptions setBackgroundJsRuntimeType:type];
}

@dynamic enableBytecode;
- (BOOL)enableBytecode {
  return self.lynxBackgroundRuntimeOptions.enableBytecode;
}

- (void)setEnableBytecode:(BOOL)enableBytecode {
  [self.lynxBackgroundRuntimeOptions setEnableBytecode:enableBytecode];
}

@dynamic bytecodeUrl;
- (NSString*)bytecodeUrl {
  return self.lynxBackgroundRuntimeOptions.bytecodeUrl;
}
- (void)setBytecodeUrl:(NSString*)bytecodeUrl {
  [self.lynxBackgroundRuntimeOptions setBytecodeUrl:bytecodeUrl];
}

- (id<LynxGenericResourceFetcher>)genericResourceFetcher {
  return self.lynxBackgroundRuntimeOptions.genericResourceFetcher;
}

- (void)setGenericResourceFetcher:(id<LynxGenericResourceFetcher>)genericResourceFetcher {
  _genericResourceFetcher = genericResourceFetcher;
  self.lynxBackgroundRuntimeOptions.genericResourceFetcher = genericResourceFetcher;
}

- (id<LynxMediaResourceFetcher>)mediaResourceFetcher {
  return self.lynxBackgroundRuntimeOptions.mediaResourceFetcher;
}

- (void)setMediaResourceFetcher:(id<LynxMediaResourceFetcher>)mediaResourceFetcher {
  _mediaResourceFetcher = mediaResourceFetcher;
  self.lynxBackgroundRuntimeOptions.mediaResourceFetcher = mediaResourceFetcher;
}

- (id<LynxTemplateResourceFetcher>)templateResourceFetcher {
  return self.lynxBackgroundRuntimeOptions.templateResourceFetcher;
}

- (void)setTemplateResourceFetcher:(id<LynxTemplateResourceFetcher>)templateResourceFetcher {
  _templateResourceFetcher = templateResourceFetcher;
  self.lynxBackgroundRuntimeOptions.templateResourceFetcher = templateResourceFetcher;
}

@end
