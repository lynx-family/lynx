// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxCustomMeasureDelegate.h>
#import <Lynx/LynxTextLayoutSpec.h>
#import <Lynx/LynxTextStyle.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxTextRenderer;
@class LynxEventEmitter;

@interface LynxAttributedTextBundle : NSObject
@property(nonatomic, strong) NSAttributedString* attributedString;
@property(nonatomic, strong) LynxTextStyle* textStyle;
@property(nonatomic, strong) NSSet* inlineElementSigns;

@property(readwrite, nonatomic, assign) LynxTextOverflowType textOverflow;
@property(readwrite, nonatomic, assign) LynxOverflow overflow;
@property(readwrite, nonatomic, assign) LynxWhiteSpaceType whiteSpace;
@property(readwrite, nonatomic, assign) NSInteger maxLineNum;
@end

@interface LynxTextRenderManager : NSObject

- (MeasureResult)measureTextWithSign:(int)sign
                               width:(float)width
                           widthMode:(LynxMeasureMode)widthMode
                              height:(float)height
                          heightMode:(LynxMeasureMode)heightMode
                     childrenSizeDic:(nullable NSDictionary*)childrenSizeDic;

- (id)takeTextRender:(NSInteger)sign;

- (void)putAttributedTextBundle:(NSInteger)sign textBundle:(LynxAttributedTextBundle*)textBundle;

- (NSDictionary*)getInlineElementOffsetDic:(NSInteger)sign;

- (void)releaseTextRender:(NSInteger)sign;

- (void)dispatchLayoutEventWithRenderer:(LynxTextRenderer*)renderer
                                   sign:(NSInteger)sign
                           eventEmitter:(LynxEventEmitter*)eventEmitter;

@end

NS_ASSUME_NONNULL_END
