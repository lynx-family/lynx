// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <XElement/LynxOverlayContainer.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxView;

/**
 * `<overlay>` introduces the conception of `level`, which rearrange all the Overlays from the small
 * level to the large level. LynxOverlayGlobalManager is designed to make it works.
 */
@interface LynxOverlayGlobalManager : NSObject

+ (instancetype)sharedInstance;
+ (NSMutableArray *)getAllVisibleOverlay;

/**
 * Update custom layout bounds for visible overlays belonging to the LynxView that follow their
 * mode's bounds. Call on the main thread.
 */
- (void)layoutIfNeededForLynxView:(LynxView *)lynxView;

/**
 * Update custom layout bounds for visible overlays whose views are descendants of the view
 * controller's loaded view and follow their mode's bounds. Call on the main thread.
 */
- (void)layoutIfNeededForViewController:(UIViewController *)viewController;

/**
 * Display the `<overlay>` according to its level and mode
 * @return the container in the corresponding mode which contains the overlay
 */
- (UIView *)showOverlayView:(UIView *)overlay
                    atLevel:(NSInteger)level
                   withMode:(LynxOverlayMode)mode
       customViewController:(UIViewController *)customViewController;

/**
 * Destory the `<overlay>` according to its level and mode
 */
- (void)destroyOverlayView:(UIView *)overlay
                   atLevel:(NSInteger)level
                  withMode:(LynxOverlayMode)mode
      customViewController:(UIViewController *)customViewController;

/**
 * Destory unattached overlay
 */
- (void)destoryUnattachedOverlay:(UIView *)overlay;

+ (UIView *)getTopViewControllerWithMode:(LynxOverlayMode)mode
                    customViewController:(UIViewController *)customViewController;

@end

NS_ASSUME_NONNULL_END
