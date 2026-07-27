// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTemplateRender+StaticPage.h"

#import <Lynx/LynxLog.h>
#import <Lynx/LynxTemplateData.h>

#import <Lynx/LynxDevtool.h>
#import "LynxEngineProxy+Native.h"
#import "LynxStaticPageTemplateData.h"
#import "LynxTemplateRender+Protected.h"
#import "StaticPageHost+Internal.h"

static BOOL IsStaticPageData(LynxTemplateData* data) {
  return data && LynxTemplateDataIsForStaticPage(data);
}

static NSDictionary<NSString*, id>* StaticPageData(LynxTemplateData* data) {
  return data ? data.dictionary : nil;
}

@implementation LynxTemplateRender (StaticPage)

- (BOOL)prepareStaticPageLoadWithInitialData:(LynxTemplateData*)initialData
                                 globalProps:(LynxTemplateData*)loadGlobalProps
                                    loadMode:(LynxLoadMode)loadMode {
  if (!_staticPageHost && !IsStaticPageData(initialData) && !IsStaticPageData(loadGlobalProps) &&
      !IsStaticPageData(_globalProps)) {
    return YES;
  }

  @synchronized(self) {
    if (_isDestroyed || !shell_ || shell_->IsDestroyed()) {
      return NO;
    }
    if (_staticPageHost.isRegistered) {
      _LogE(@"Repeated loadTemplate is not supported for a static page direct load");
      return NO;
    }
    if ((initialData && !IsStaticPageData(initialData)) ||
        (_globalProps && !IsStaticPageData(_globalProps)) ||
        (loadGlobalProps && !IsStaticPageData(loadGlobalProps))) {
      _LogE(@"Static page direct data cannot be mixed with standard LynxTemplateData");
      return NO;
    }
    if (_threadStrategyForRendering == LynxThreadStrategyForRenderMostOnTASM ||
        _enableReuseEngine || loadMode == LynxLoadModeRenderSSR ||
        loadMode == LynxLoadModeHydrateSSR) {
      _LogE(@"Static page direct data does not support the current rendering configuration");
      return NO;
    }
    int32_t instanceId = shell_->GetInstanceId();
    if (instanceId < 0) {
      _LogE(@"Static page direct data requires a valid Lynx instance id");
      return NO;
    }

    if (loadGlobalProps && ![self mergeStaticPageGlobalProps:loadGlobalProps]) {
      return NO;
    }
    if (!_staticPageHost) {
      _staticPageHost = [self createStaticPageHost];
    }
    NSDictionary<NSString*, id>* registeredGlobalProps =
        [_staticPageHost registerInstanceId:instanceId
                                       data:StaticPageData(initialData)
                                globalProps:StaticPageData(_globalProps)];
    _globalProps =
        registeredGlobalProps ? [LynxTemplateData createForStaticPage:registeredGlobalProps] : nil;
    if (_globalProps) {
      [_devTool onGlobalPropsUpdated:_globalProps];
    }
    return YES;
  }
}

- (void)handleStaticPageMetaData:(LynxTemplateData*)data
                     globalProps:(LynxTemplateData*)globalProps {
  @synchronized(self) {
    if (_isDestroyed || !shell_ || shell_->IsDestroyed()) {
      return;
    }
    if ((data && !IsStaticPageData(data)) || (globalProps && !IsStaticPageData(globalProps))) {
      _LogE(@"Static page direct data cannot be mixed with standard LynxTemplateData");
      return;
    }
    if (!_staticPageHost) {
      if (_hasStartedLoad) {
        _LogE(@"Static page direct metadata must be set before loadTemplate");
        return;
      }
      _staticPageHost = [self createStaticPageHost];
    }

    NSDictionary<NSString*, id>* mergedGlobalProps = nil;
    if (globalProps) {
      if (![self mergeStaticPageGlobalProps:globalProps]) {
        return;
      }
      mergedGlobalProps = StaticPageData(_globalProps);
      [_devTool onGlobalPropsUpdated:_globalProps];
    }
    if (data) {
      [_devTool onUpdateDataWithTemplateData:data];
    }
    [_staticPageHost updateMetaData:StaticPageData(data) globalProps:mergedGlobalProps];
  }
}

- (BOOL)shouldHandleStaticPageMetaData:(LynxTemplateData*)data
                           globalProps:(LynxTemplateData*)globalProps {
  return _staticPageHost || IsStaticPageData(data) || IsStaticPageData(globalProps);
}

- (BOOL)isStaticPageHostRegistered {
  return _staticPageHost.isRegistered;
}

- (BOOL)mergeStaticPageGlobalProps:(LynxTemplateData*)globalProps {
  if (!_globalProps) {
    _globalProps = globalProps;
    return YES;
  }
  if (!IsStaticPageData(_globalProps)) {
    _LogE(@"Static page direct globalProps cannot be mixed with standard LynxTemplateData");
    return NO;
  }
  NSMutableDictionary<NSString*, id>* merged = [StaticPageData(_globalProps) mutableCopy];
  [merged addEntriesFromDictionary:StaticPageData(globalProps)];
  _globalProps = [LynxTemplateData createForStaticPage:[merged copy]];
  return YES;
}

- (StaticPageHost*)createStaticPageHost {
  LynxEngineProxy* engineProxy = _lynxEngineProxy;
  return [[StaticPageHost alloc] initWithTaskDispatcher:^(dispatch_block_t task) {
    [engineProxy dispatchTaskToLynxEngine:task];
  }];
}

- (void)destroyStaticPageHost {
  StaticPageHost* host;
  @synchronized(self) {
    host = _staticPageHost;
    _staticPageHost = nil;
  }
  [host clear];
}

@end
