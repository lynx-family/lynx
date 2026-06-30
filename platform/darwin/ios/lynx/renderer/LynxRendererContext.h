// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxImageManager.h>
#import <Lynx/LynxTextRenderManager.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxURL.h>

@interface LynxRendererContext : NSObject

@property(nonatomic, weak) UIView<LUIBodyView> *bodyView;
@property(nonatomic, strong) LynxUIContext *uiContext;
@property(nonatomic, strong) LynxTextRenderManager *textRenderManager;

- (void)createImageManager:(int32_t)imageManagerID
             withSourceURL:(LynxURL *)sourceURL
         andPlaceholderURL:(LynxURL *)placeholderURL
                 eventMask:(int32_t)eventMask;

- (LynxImageManager *)imageManagerForID:(int32_t)imageManagerID;

- (void)updateTextBundle:(int32_t)textID withBundle:(void *)bundle;

- (void)destroyTextBundle:(int32_t)textID;

- (void *)getTextBundle:(int32_t)textID;

@end
