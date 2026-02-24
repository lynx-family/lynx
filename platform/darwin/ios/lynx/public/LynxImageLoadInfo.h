// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxViewClientV2.h>

/// Detailed information about loaded image resources.
@interface LynxImageLoadInfo : LynxResourceLoadInfo

/// The src of the image.
@property(nonatomic, copy) NSString *src;

/// Image width in pt.
@property(nonatomic, assign) CGFloat width;

/// Image height in pt.
@property(nonatomic, assign) CGFloat height;

/// Image view width in pt.
@property(nonatomic, assign) CGFloat viewWidth;

/// Image view height in pt.
@property(nonatomic, assign) CGFloat viewHeight;

/// Image start loading timestamp, in ms.
@property(nonatomic, assign) double loadStart;

/// Image loading completion timestamp, in ms.
@property(nonatomic, assign) double loadFinish;

/// Image resource origin. The value is aligned with LynxImageOrigin.
@property(nonatomic, assign) NSInteger origin;

@end
