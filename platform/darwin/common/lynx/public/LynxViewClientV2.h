// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxPerformanceObserverProtocol.h>

@class LynxView;

/**
 * The cause that the lynx pixel pipeline is activated
 */
typedef NS_ENUM(NSInteger, LynxPipelineOrigin) {
  LynxFirstScreen = 1,
  LynxReload = 1 << 1,
};

/**
 * Provide information about the lynx pixel pipeline
 * @property url url of LynxView
 * @property stage stage of pixel pipeline in Lynx lifecycle
 */
@interface LynxPipelineInfo : NSObject

@property(nonatomic, readonly, nullable, copy) NSString *url;
@property(nonatomic, readonly) NSInteger pipelineOrigin;

- (nonnull instancetype)initWithUrl:(nullable NSString *)url;

- (void)addPipelineOrigin:(NSInteger)pipelineOrigin;

@end

/**
 * Base protocol of LynxViewLifecycle and LynxViewLifecycleV2 that helps to keep compatible with
 * previous interfaces.
 */
@protocol LynxViewBaseLifecycle <NSObject>
@end

/**
 * Give the host application a chance to take control when a lynx template is about to be loaded in
 * the current LynxView.
 */
@protocol LynxViewLifecycleV2 <LynxViewBaseLifecycle, LynxPerformanceObserverProtocol>

/**
 * Notify that a lynx template has started loading. It will be call at both `loadTemplate` and
 * `reloadTemplate`.
 *
 * Note: this method will be executed before the main process of lynx so do not execute overly
 * complex logic in this method.
 *
 * @param lynxView the LynxView which has started loading
 * @param info the information about the pixel pipeline
 *
 */
@optional
- (void)onPageStartedWithLynxView:(nonnull LynxView *)lynxView
                 withPipelineInfo:(nonnull LynxPipelineInfo *)info;

@end
