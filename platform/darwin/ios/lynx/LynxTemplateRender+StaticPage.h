// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxLoadMeta.h>
#import <Lynx/LynxTemplateRender.h>

@class LynxTemplateData;

NS_ASSUME_NONNULL_BEGIN

@interface LynxTemplateRender (StaticPage)

/**
 * Prepares the platform-side host for a static-page direct load. This is a no-op for a standard
 * Lynx load.
 *
 * @return YES when preparation is not needed or succeeds. NO when this is a static-page direct
 * load but its platform host or input cannot be prepared. The caller must stop because direct data
 * intentionally has no standard Lynx data fallback, and continuing would run the load with missing
 * data or an incomplete host binding.
 */
- (BOOL)prepareStaticPageLoadWithInitialData:(nullable LynxTemplateData*)initialData
                                 globalProps:(nullable LynxTemplateData*)loadGlobalProps
                                    loadMode:(LynxLoadMode)loadMode;
- (void)handleStaticPageMetaData:(nullable LynxTemplateData*)data
                     globalProps:(nullable LynxTemplateData*)globalProps;
- (BOOL)shouldHandleStaticPageMetaData:(nullable LynxTemplateData*)data
                           globalProps:(nullable LynxTemplateData*)globalProps;
- (BOOL)isStaticPageHostRegistered;
- (void)destroyStaticPageHost;

@end

NS_ASSUME_NONNULL_END
