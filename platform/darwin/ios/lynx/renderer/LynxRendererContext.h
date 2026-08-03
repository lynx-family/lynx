// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

@protocol LUIBodyView;

@class LynxImageManager;
@class LynxTextRenderManager;
@class LynxUIContext;
@class LynxURL;
@class UIView;

#if defined(__cplusplus)
namespace lynx::tasm {
struct ImagePaintInfo;
}
#endif

@interface LynxRendererContext : NSObject

@property(nonatomic, weak) UIView<LUIBodyView> *bodyView;
@property(nonatomic, strong) LynxUIContext *uiContext;
@property(nonatomic, strong) LynxTextRenderManager *textRenderManager;

#if defined(__cplusplus)
- (void)createImageManager:(int32_t)imageManagerID
             withSourceURL:(LynxURL *)sourceURL
         andPlaceholderURL:(LynxURL *)placeholderURL
                 paintInfo:(const lynx::tasm::ImagePaintInfo &)paintInfo
                 eventMask:(int32_t)eventMask
                  imageKey:(int32_t)imageKey;
#endif

- (LynxImageManager *)imageManagerForID:(int32_t)imageManagerID;

- (void)destroyImage:(int32_t)imageKey;

- (void)updateTextBundle:(int32_t)textID withBundle:(void *)bundle;

- (void)destroyTextBundle:(int32_t)textID;

- (void *)getTextBundle:(int32_t)textID;

@end
