// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Foundation/Foundation.h>
#import "LynxConverter.h"
#import "LynxError.h"
#import "LynxShadowNode.h"
#import "LynxUI.h"
#import "LynxURL.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_OPTIONS(NSInteger, LynxRequestOptions) {
  LynxImageDefaultOptions = 0,
  LynxImageIgnoreMemoryCache = 1 << 1,  // don't search memory cache
  LynxImageIgnoreDiskCache = 1 << 2,    // don't search disk cache
  LynxImageNotCacheToMemory = 1 << 3,   // don't store to memory cache
  LynxImageNotCacheToDisk = 1 << 4,     // don't store to disk cache
  LynxImageIgnoreCDNDowngradeCachePolicy =
      1 << 5,  // if not set, the CDN downgraded image will not store to disk cache
};

@interface LynxUIImage : LynxUI <UIImageView*>

@property(nonatomic, readonly, getter=isAnimated) BOOL animated;
@property(nonatomic, strong, nullable) id customImageRequest;
@property(nonatomic) NSMutableDictionary* resLoaderInfo;
@property(nonatomic, readonly) LynxRequestOptions requestOptions;

- (void)startAnimating;
- (bool)getEnableImageDownsampling;

// (TODO fangzhou) Add control options for containers. Remove it when all containers support this
// feature.
- (BOOL)shouldUseNewImage;

- (void)requestImage;

+ (BOOL)isAnimatedImage:(UIImage*)image;

@end

@interface LynxConverter (UIViewContentMode)

@end

@interface LynxImageShadowNode : LynxShadowNode

@end

NS_ASSUME_NONNULL_END
