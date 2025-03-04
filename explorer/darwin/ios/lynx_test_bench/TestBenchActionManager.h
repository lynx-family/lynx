// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxGroup.h>
#import <Lynx/LynxView.h>
#import <UIKit/UIKit.h>
#import "TestBenchReplayConfig.h"
#import "TestBenchStateReplayView.h"

NS_ASSUME_NONNULL_BEGIN

@class TestBenchActionManager;
@protocol TestBenchActionCallback <NSObject>
@required
- (void)onLynxViewWillBuild:(TestBenchActionManager *)manager builder:(LynxViewBuilder *)builder;
- (void)onLynxViewDidBuild:(LynxView *)lynxView;

@end

@interface TestBenchActionManager : NSObject
@property(nonatomic, copy) void (^endTestBenchBlock)(void);
@property(nonatomic, copy) void (^onTestBenchComplete)(void);
@property(nonatomic, copy) void (^firstScreenBlock)(void);
@property(nonatomic) LynxGroup *lynxGroup;
@property(nonatomic, readonly) NSArray<NSDictionary *> *componentList;

- (void)registerTestBenchActionCallback:(id<TestBenchActionCallback>)callback;
- (void)startWithUrl:(NSString *)url
              inView:(UIView *)parentView
          withOrigin:(CGPoint)point
           stateView:(TestBenchStateReplayView *)stateView
        replayConfig:(TestBenchReplayConfig *)replayConfig;
- (void)reload;
- (void)reloadAction;
- (void)endTestBench;
- (void)dispatchAction:(NSString *)functionName
                params:(NSDictionary *)params
              interval:(NSInteger)interval;
@end

NS_ASSUME_NONNULL_END
