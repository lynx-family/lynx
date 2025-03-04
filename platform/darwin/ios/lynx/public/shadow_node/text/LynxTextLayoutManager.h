// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

@interface LynxTextLayoutManager : NSLayoutManager
@property(nonatomic, assign) CGPoint overflowOffset;
@property(nonatomic, assign) CGSize textBoundingRectSize;
@property(nonatomic, assign) NSInteger glyphCount;
@property(nonatomic, assign) CGPoint preEndPosition;
@property(nonatomic, assign) NSRange preDrawableRange;
@end
