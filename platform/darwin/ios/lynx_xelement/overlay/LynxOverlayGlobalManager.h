// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <XElement/LynxOverlayContainer.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * `<overlay>` introduces the conception of `level`, which rearrange all the Overlays from the small
 * level to the large level. LynxOverlayGlobalManager is designed to make it works.
 */
@interface LynxOverlayGlobalManager : NSObject

+ (instancetype)sharedInstance;
+ (NSMutableArray *)getAllVisibleOverlay;

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
- (void)destoryOverlayView:(UIView *)overlay
                   atLevel:(NSInteger)level
                  withMode:(LynxOverlayMode)mode
      customViewController:(UIViewController *)customViewController;

/**
 * Destory unattached overlay
 */
- (void)destoryUnattachedOverlay:(UIView *)overlay;

@end

NS_ASSUME_NONNULL_END
