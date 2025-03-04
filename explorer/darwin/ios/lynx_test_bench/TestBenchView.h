// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@protocol TestBenchActionCallback;
@interface TestBenchView : UIView
- (void)registerTestBenchActionCallback:(id<TestBenchActionCallback>)callback;
- (void)loadPageWithPoint:(NSString *)url
                    point:(CGPoint)point
          scrollContainer:(UIScrollView *)scrollContainer;
- (void)reload;
- (void)updateScrollContainerSize;
@end

NS_ASSUME_NONNULL_END
