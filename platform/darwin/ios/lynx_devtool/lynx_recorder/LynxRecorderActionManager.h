// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxGroup.h>
#import <Lynx/LynxView.h>
#import <LynxDevtool/LynxRecorderReplayConfig.h>
#import <LynxDevtool/LynxRecorderReplayStateView.h>
#import <LynxDevtool/LynxRecorderTouchHelper.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxRecorderActionManager;
@protocol LynxRecorderActionCallback <NSObject>
@required
- (void)onLynxViewWillBuild:(LynxRecorderActionManager *)manager builder:(LynxViewBuilder *)builder;
- (void)onLynxViewDidBuild:(LynxView *)lynxView;

@end

@interface LynxRecorderActionManager : NSObject
@property(nonatomic, copy) void (^endTestBenchBlock)(void);
@property(nonatomic, copy) void (^onTestBenchComplete)(void);
@property(nonatomic, copy) void (^firstScreenBlock)(void);
@property(nonatomic) LynxGroup *lynxGroup;
@property(nonatomic, readonly) NSArray<NSDictionary *> *componentList;
@property(nonatomic, copy) NSString *url;

- (void)registerLynxRecorderActionCallback:(id<LynxRecorderActionCallback>)callback;
- (void)startWithUrl:(NSString *)url
              inView:(UIView *)parentView
          withOrigin:(CGPoint)point
        replayConfig:(LynxRecorderReplayConfig *)replayConfig
              NavBar:(CGSize)navBarSize;

- (void)startWithUrl:(NSString *)url
              inView:(UIView *)parentView
          withOrigin:(CGPoint)point
        replayConfig:(LynxRecorderReplayConfig *)replayConfig;

- (void)reload;
- (void)reloadAction;
- (void)endTestBench;
- (void)dispatchAction:(NSString *)functionName
                params:(NSDictionary *)params
              interval:(NSInteger)interval;
- (void)registerTouchHelper:(id<LynxRecorderTouchHelper>)touchHelper;
@end

NS_ASSUME_NONNULL_END
