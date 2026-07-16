// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#import <Lynx/LynxURL.h>
#import <UIKit/UIKit.h>

@class LynxUIContext;

#if defined(__cplusplus)
namespace lynx::tasm {
struct ImagePaintInfo;
}
#endif

@interface LynxImageManager : NSObject

- (instancetype _Nonnull)initWithContext:(LynxUIContext* _Nullable)context;

- (void)requestImage:(LynxURL* _Nullable)imageURL withType:(LynxImageRequestType)type;

- (void)setTarget:(UIImageView* _Nullable)view;

- (void)setSign:(NSInteger)sign;

- (void)setEventMask:(NSInteger)eventMask;

#if defined(__cplusplus)
- (void)updatePaintInfo:(const lynx::tasm::ImagePaintInfo&)paintInfo;
#endif

- (void)reset;

@end
