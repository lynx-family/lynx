// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#import "LynxCSSType.h"
#import "LynxMeasureDelegate.h"
#import "LynxShadowNodeStyle.h"
#import "LynxTextStyle.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxOverflow) {
  LynxNoOverflow = 0,
  LynxOverflowX = 1,
  LynxOverflowY = 2,
  LynxOverflowXY = 3,
};

static const NSInteger LynxNumberNotSet = -1;

BOOL LynxSameMeasureMode(LynxMeasureMode left, LynxMeasureMode right);

@interface LynxLayoutSpec : NSObject

@property(readonly, nonatomic, assign) CGFloat width;
@property(readonly, nonatomic, assign) CGFloat height;
@property(readonly, nonatomic, assign) LynxMeasureMode widthMode;
@property(readonly, nonatomic, assign) LynxMeasureMode heightMode;
@property(readonly, nonatomic, assign) LynxTextOverflowType textOverflow;
@property(readonly, nonatomic, assign) LynxOverflow overflow;
@property(readonly, nonatomic, assign) LynxWhiteSpaceType whiteSpace;
@property(readonly, nonatomic, assign) NSInteger maxLineNum;
@property(readonly, nonatomic, assign) NSInteger maxTextLength;
@property(readonly, nonatomic) LynxTextStyle *textStyle;
@property(nullable, nonatomic) id<NSLayoutManagerDelegate> layoutManagerDelegate;
@property(nonatomic, assign) LynxVerticalAlign verticalAlign;
@property(readonly, nonatomic, assign) BOOL enableTailColorConvert;
@property(nonatomic, assign) BOOL enableTextNonContiguousLayout;
@property(nonatomic, assign) BOOL enableTextRefactor;
@property(nonatomic, assign) BOOL enableNewClipMode;
@property(nonatomic, assign) LynxVerticalAlign textSingleLineVerticalAlign;

@property(readonly, nonatomic) BOOL widthUndifined;
@property(readonly, nonatomic) BOOL heightUndifined;

- (instancetype)initWithWidth:(CGFloat)width
                       height:(CGFloat)height
                    widthMode:(LynxMeasureMode)widthMode
                   heightMode:(LynxMeasureMode)heightMode
                 textOverflow:(LynxTextOverflowType)textOverflow
                     overflow:(LynxOverflow)overflow
                   whiteSpace:(LynxWhiteSpaceType)whiteSpace
                   maxLineNum:(NSInteger)maxLineNum
                maxTextLength:(NSInteger)maxTextLength
                    textStyle:(LynxTextStyle *)textStyle
       enableTailColorConvert:(BOOL)enableTailColorConvert;

- (BOOL)isEqualToSpec:(LynxLayoutSpec *)spec;

@end

NS_ASSUME_NONNULL_END
