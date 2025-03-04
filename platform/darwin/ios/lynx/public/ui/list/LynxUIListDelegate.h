// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxUIListDelegate <NSObject>

@optional
- (void)listDidScroll:(UIScrollView *)scrollView;
- (void)listWillBeginDragging:(UIScrollView *)scrollView;
- (void)listDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate;
- (void)listDidEndDecelerating:(UIScrollView *)scrollView;
@end

NS_ASSUME_NONNULL_END
