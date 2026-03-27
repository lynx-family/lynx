// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#import <Lynx/LynxURL.h>
#import <UIKit/UIKit.h>

@class LynxUIContext;

@interface LynxImageManager : NSObject

- (instancetype)initWithContext:(LynxUIContext*)context;

- (void)requestImage:(LynxURL*)imageURL withType:(LynxImageRequestType)type;

- (void)setTarget:(UIImageView*)view;

- (void)setSign:(NSInteger)sign;

- (void)setEventMask:(NSInteger)eventMask;

- (void)reset;

@end
