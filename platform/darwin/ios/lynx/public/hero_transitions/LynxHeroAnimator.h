// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxHeroAnimatorDelegate <NSObject>

- (void)updateProgress:(double)progress;
- (void)complete:(BOOL)finished;

@end

@interface LynxHeroAnimator : NSObject
@property(nonatomic, nullable, weak) id<LynxHeroAnimatorDelegate> delegate;
@property(nonatomic, assign) NSTimeInterval timePassed;
@property(nonatomic, assign) NSTimeInterval totalTime;
@property(nonatomic, assign) BOOL isReversed;

- (void)startWithTimePassed:(NSTimeInterval)timePassed
                  totalTime:(NSTimeInterval)totalTime
                 isReversed:(BOOL)isReversed;
- (void)stop;
@end

NS_ASSUME_NONNULL_END
