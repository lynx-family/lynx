// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUI.h>

NS_ASSUME_NONNULL_BEGIN

@protocol ILynxMapView;

@interface LynxUIMapMarker : LynxUI <UIView *>

- (void)bindMapView:(id<ILynxMapView>)mapView;
- (void)sendEvent:(NSString *)name detail:(nullable NSDictionary<NSString *, id> *)detail;

@end

NS_ASSUME_NONNULL_END
