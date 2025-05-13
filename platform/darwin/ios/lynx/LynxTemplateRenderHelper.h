// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

@class LynxFrameRender;
@class LynxTemplateRender;

/**
 * Help to setup native environment for LynxTemplateRender and LynxFrameRender
 */
@interface LynxTemplateRenderHelper : NSObject

+ (void)setUpTemplateRender:(LynxTemplateRender*)render screenSize:(CGSize)screenSize;

+ (void)resetTemplateRender:(LynxTemplateRender*)render lastInstanceId:(int32_t)lastInstanceId;

+ (void)setUpFrameRender:(LynxFrameRender*)render screenSize:(CGSize)screenSize;

@end
