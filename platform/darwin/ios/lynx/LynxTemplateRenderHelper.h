// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

@class LynxTemplateRender;
@class LynxViewBuilder;
@class LynxFrameRender;

@interface LynxTemplateRenderHelper : NSObject

/**
 *  Help to setup UIRender, LynxShell and Event for LynxTemplateRender
 */
+ (void)setUpTemplateRender:(LynxTemplateRender*)render
                    builder:(LynxViewBuilder*)builder
                 screenSize:(CGSize)screenSize;

/**
 *  Help to reset ShadowNodeOwner, UIDelegate, LynxShell and EventHandler for LynxTemplateRender
 */
+ (void)resetTemplateRender:(LynxTemplateRender*)render lastInstanceId:(int32_t)lastInstanceId;

/**
 *  Help to setup UIRender, LynxShell and Event for LynxFrameRender
 */
+ (void)setUpFrameRender:(LynxFrameRender*)render
                 builder:(LynxViewBuilder*)builder
              screenSize:(CGSize)screenSize;

@end
