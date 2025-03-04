// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "DevToolPlatformDarwinDelegate.h"

NS_ASSUME_NONNULL_BEGIN

extern NSString* const ScreenshotModeLynxView;
extern NSString* const ScreenshotModeFullScreen;

extern int const ScreenshotPreviewDelayTime;

typedef NSString* ScreenshotMode;

@interface LynxScreenCastHelper : NSObject

- (nonnull instancetype)initWithLynxView:(LynxView*)view
                    withPlatformDelegate:(DevToolPlatformDarwinDelegate*)platformDelegate;

- (void)startCasting:(int)quality
               width:(int)max_width
              height:(int)max_height
                mode:(NSString*)screenshot_mode;

- (void)stopCasting;
- (void)continueCasting;
- (void)pauseCasting;
- (void)attachLynxView:(nonnull LynxView*)lynxView;
- (void)onAckReceived;

- (void)sendCardPreview;

@end

NS_ASSUME_NONNULL_END
