// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>
#import <XElement/ILynxMapView.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxMapProvider <NSObject>
- (UIView<ILynxMapView> *)createMapView;
- (UIView<ILynxMapMarkerView> *)createMapMarkerView;
@end

NS_ASSUME_NONNULL_END
