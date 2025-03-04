// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTextRenderer.h"

NS_ASSUME_NONNULL_BEGIN

extern BOOL layoutManagerIsTruncated(NSLayoutManager *layoutManager);

@interface LynxTextRendererCache : NSObject <NSCacheDelegate>

+ (instancetype)cache;

- (instancetype)init NS_UNAVAILABLE;

- (LynxTextRenderer *)rendererWithString:(NSAttributedString *)str
                              layoutSpec:(LynxLayoutSpec *)spec;

- (void)clearCache;

@end

NS_ASSUME_NONNULL_END
