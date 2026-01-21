// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LUIErrorHandling.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxView.h>

@protocol LynxBaseInspectorController;

@interface LynxView ()

- (LynxTemplateRender* _Nullable)templateRender;
- (void)setLynxViewId:(NSInteger)id;

/**
 * @warning The following APIs are intended for internal use within Lynx components only.
 *
 * External dependencies must not invoke these methods, as they are reserved exclusively
 * for the implementation of Lynx's internal inspector functionality.
 */
- (NSDictionary* _Nullable)getAllJsSource;
- (void)onLongPress;
- (id<LynxBaseInspectorController> _Nullable)baseInspectorController;

@end
