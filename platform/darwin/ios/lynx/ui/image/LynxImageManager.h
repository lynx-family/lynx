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

// Custom fetchers and failed or invalidated requests retain resize reloads.
- (BOOL)canReuseOnResize;

- (void)setTarget:(UIImageView* _Nullable)view;

// Sets the target layer for displaying background images.
- (void)setLayerTarget:(CALayer* _Nullable)layer;

// Updates background geometry for cached images and asynchronous loads.
- (void)setLayerTarget:(CALayer* _Nullable)layer
    imageSizeDidChange:(void (^_Nullable)(CGSize size))imageSizeDidChange;

- (void)setSign:(NSInteger)sign;

- (void)setEventMask:(NSInteger)eventMask;

#if defined(__cplusplus)
- (void)updatePaintInfo:(const lynx::tasm::ImagePaintInfo&)paintInfo;
- (void)updateSourceURL:(LynxURL* _Nullable)sourceURL
         placeholderURL:(LynxURL* _Nullable)placeholderURL
              paintInfo:(const lynx::tasm::ImagePaintInfo&)paintInfo
                changes:(uint32_t)changes;
#endif

- (void)reset;

@end
