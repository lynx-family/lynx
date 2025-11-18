// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxResourceListener.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxImageLoadSuccessInfo : LynxResourceLoadSuccessInfo

@property(nonatomic, assign) CGFloat width;
@property(nonatomic, assign) CGFloat height;
@property(nonatomic, assign) CGFloat viewWidth;
@property(nonatomic, assign) CGFloat viewHeight;
@property(nonatomic, assign) CGFloat memoryCost;
@property(nonatomic, assign) NSInteger origin;
@property(nonatomic, assign) NSInteger scale;
@property(nonatomic, assign) double loadStart;
@property(nonatomic, assign) double loadFinish;
@property(nonatomic, assign) double loadCost;
@property(nonatomic, strong) NSDictionary *customInfo;
@property(nonatomic, assign) BOOL downsampling;
@property(nonatomic, assign) double downloadDuration;
@property(nonatomic, assign) double decodeDuration;
@property(nonatomic, copy, nullable) NSString *mimeType;
@property(nonatomic, copy, nullable) NSString *isHitCDNCache;

- (instancetype)initWithURL:(NSString *)url;

@end

NS_ASSUME_NONNULL_END
