// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEnv.h>
#import <Lynx/LynxFontFaceManager.h>
#import <Lynx/LynxLazyRegister.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxTraceEvent.h>
#import <Lynx/LynxTraceEventDef.h>
#import <Lynx/LynxViewBuilder.h>
#import <Lynx/LynxViewGroup.h>
#import "LynxUIRenderer.h"
#import "LynxUIRendererCreator.h"

@implementation LynxViewBuilder

- (LynxConfig*)config {
  if (_lynxViewGroup) {
    return _lynxViewGroup.config;
  }
  return [super config];
}

- (LynxGroup*)group {
  if (_lynxViewGroup) {
    return _lynxViewGroup.group;
  }
  return [super group];
}

- (BOOL)enableLayoutSafepoint {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableLayoutSafepoint;
  }
  return [super enableLayoutSafepoint];
}

- (BOOL)enableAutoExpose {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableAutoExpose;
  }
  return [super enableAutoExpose];
}

- (BOOL)enableTextNonContiguousLayout {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableTextNonContiguousLayout;
  }
  return [super enableTextNonContiguousLayout];
}

- (BOOL)enableLayoutOnly {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableLayoutOnly;
  }
  return [super enableLayoutOnly];
}

- (BOOL)enableUIOperationQueue {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableUIOperationQueue;
  }
  return [super enableUIOperationQueue];
}

- (BOOL)enablePendingJSTaskOnLayout {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enablePendingJSTaskOnLayout;
  }
  return [super enablePendingJSTaskOnLayout];
}

- (BOOL)enableJSRuntime {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableJSRuntime;
  }

  return [super enableJSRuntime];
}

- (BOOL)enableAirStrictMode {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableAirStrictMode;
  }
  return [super enableAirStrictMode];
}

- (BOOL)enableAsyncCreateRender {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableAsyncCreateRender;
  }
  return [super enableAsyncCreateRender];
}

- (BOOL)enableSyncFlush {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableSyncFlush;
  }
  return [super enableSyncFlush];
}

- (BOOL)enableMultiAsyncThread {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableMultiAsyncThread;
  }
  return [super enableMultiAsyncThread];
}

- (BOOL)enableVSyncAlignedMessageLoop {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableVSyncAlignedMessageLoop;
  }
  return [super enableVSyncAlignedMessageLoop];
}

- (BOOL)enableAsyncHydration {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableAsyncHydration;
  }
  return [super enableAsyncHydration];
}

- (BOOL)enableMTSModule {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableMTSModule;
  }
  return [super enableMTSModule];
}

- (CGFloat)fontScale {
  if (_lynxViewGroup) {
    return _lynxViewGroup.fontScale;
  }
  return [super fontScale];
}

- (LynxBooleanOption)enableGenericResourceFetcher {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableGenericResourceFetcher;
  }
  return [super enableGenericResourceFetcher];
}

- (id<LynxGenericResourceFetcher>)genericResourceFetcher {
  if (_lynxViewGroup) {
    return _lynxViewGroup.genericResourceFetcher;
  }
  return [super genericResourceFetcher];
}

- (id<LynxMediaResourceFetcher>)mediaResourceFetcher {
  if (_lynxViewGroup) {
    return _lynxViewGroup.mediaResourceFetcher;
  }
  return [super mediaResourceFetcher];
}

- (id<LynxTemplateResourceFetcher>)templateResourceFetcher {
  if (_lynxViewGroup) {
    return _lynxViewGroup.templateResourceFetcher;
  }
  return [super templateResourceFetcher];
}

- (CGSize)screenSize {
  if (_lynxViewGroup) {
    return _lynxViewGroup.screenSize;
  }
  return [super screenSize];
}

- (BOOL)debuggable {
  if (_lynxViewGroup) {
    return _lynxViewGroup.debuggable;
  }
  return [super debuggable];
}

- (BOOL)enablePreUpdateData {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enablePreUpdateData;
  }
  return [super enablePreUpdateData];
}

- (LynxBackgroundJsRuntimeType)backgroundJsRuntimeType {
  if (_lynxViewGroup) {
    return _lynxViewGroup.backgroundJsRuntimeType;
  }
  return [super backgroundJsRuntimeType];
}

- (BOOL)enableBytecode {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableBytecode;
  }
  return [super enableBytecode];
}

- (BOOL)enableUnifiedPipeline {
  if (_lynxViewGroup) {
    return _lynxViewGroup.enableUnifiedPipeline;
  }
  return [super enableUnifiedPipeline];
}

- (NSString*)bytecodeUrl {
  if (_lynxViewGroup) {
    return _lynxViewGroup.bytecodeUrl;
  }
  return [super bytecodeUrl];
}

- (id<LynxUIRendererCreatorProtocol>)uiRendererCreator {
  if (_lynxViewGroup) {
    return _lynxViewGroup.uiRendererCreator;
  }
  return [super uiRendererCreator];
}

- (LynxThreadStrategyForRender)getThreadStrategyForRender {
  if (_lynxViewGroup) {
    return [_lynxViewGroup getThreadStrategyForRender];
  }
  return [super getThreadStrategyForRender];
}

- (LynxEmbeddedMode)getEmbeddedMode {
  if (_lynxViewGroup) {
    return [_lynxViewGroup getEmbeddedMode];
  }
  return [super getEmbeddedMode];
}

- (void)insertLynxViewConfig:(id)config forKey:(NSString*)key {
  if (!key || !config) {
    return;
  }
  if (!self.lynxViewConfig) {
    self.lynxViewConfig = [[NSMutableDictionary alloc] initWithObjectsAndKeys:config, key, nil];
  } else {
    if (![self.lynxViewConfig objectForKey:key]) {
      [self.lynxViewConfig setObject:config forKey:key];
    }
  }
}

@end
