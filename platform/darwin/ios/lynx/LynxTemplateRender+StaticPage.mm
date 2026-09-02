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

static NSDictionary<NSString*, id>* TemplateDataDictionary(LynxTemplateData* data) {
  // Static-page data returns its retained platform dictionary. Standard TemplateData is accepted
  // only for global props and is materialized here at the direct-load boundary.
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
    // Page data has no standard-data fallback. Global props may use standard TemplateData and are
    // materialized once when the direct load is registered.
    if (initialData && !IsStaticPageData(initialData)) {
      _LogE(@"Static page direct load requires static page initial data");
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

    if (loadGlobalProps) {
      [self mergeStaticPageGlobalProps:loadGlobalProps];
    }
    if (!_staticPageHost) {
      _staticPageHost = [self createStaticPageHost];
    }
    NSDictionary<NSString*, id>* registeredGlobalProps =
        [_staticPageHost registerInstanceId:instanceId
                                       data:TemplateDataDictionary(initialData)
                                globalProps:TemplateDataDictionary(_globalProps)];
    _globalProps =
        registeredGlobalProps ? [LynxTemplateData createForStaticPage:registeredGlobalProps] : nil;
    if (_globalProps) {
      // Static-page loading skips the standard updateGlobalPropsWithTemplateData path. Report the
      // final merged value so DevTool observes the same load-time global props.
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
      [self mergeStaticPageGlobalProps:globalProps];
      mergedGlobalProps = TemplateDataDictionary(_globalProps);
      [_devTool onGlobalPropsUpdated:_globalProps];
    }
    [_staticPageHost updateMetaData:TemplateDataDictionary(data) globalProps:mergedGlobalProps];
  }
}

- (BOOL)shouldHandleStaticPageMetaData:(LynxTemplateData*)data
                           globalProps:(LynxTemplateData*)globalProps {
  return _staticPageHost || IsStaticPageData(data) || IsStaticPageData(globalProps);
}

- (BOOL)isStaticPageHostRegistered {
  return _staticPageHost.isRegistered;
}

- (void)mergeStaticPageGlobalProps:(LynxTemplateData*)globalProps {
  if (!_globalProps) {
    _globalProps = globalProps;
    return;
  }
  NSMutableDictionary<NSString*, id>* merged = [TemplateDataDictionary(_globalProps) mutableCopy];
  [merged addEntriesFromDictionary:TemplateDataDictionary(globalProps)];
  _globalProps = [LynxTemplateData createForStaticPage:[merged copy]];
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
