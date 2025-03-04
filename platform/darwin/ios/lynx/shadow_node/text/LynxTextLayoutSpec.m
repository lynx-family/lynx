// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTextLayoutSpec.h"
#import <objc/runtime.h>

BOOL LynxSameMeasureMode(LynxMeasureMode left, LynxMeasureMode right) {
  if (left == right) {
    return YES;
  }
  if ((left == LynxMeasureModeDefinite || left == LynxMeasureModeAtMost) &&
      (right == LynxMeasureModeDefinite || right == LynxMeasureModeAtMost)) {
    return YES;
  }
  return NO;
}

@implementation LynxLayoutSpec

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
       enableTailColorConvert:(BOOL)enableTailColorConvert {
  if (self = [super init]) {
    _width = width;
    _height = height;
    _widthMode = widthMode;
    _heightMode = heightMode;
    _textOverflow = textOverflow;
    _overflow = overflow;
    _whiteSpace = whiteSpace;
    _maxLineNum = maxLineNum;
    _maxTextLength = maxTextLength;
    _textStyle = textStyle;
    _verticalAlign = LynxVerticalAlignDefault;
    _enableTailColorConvert = enableTailColorConvert;
    _enableTextRefactor = NO;
    _enableNewClipMode = NO;
    _textSingleLineVerticalAlign = LynxVerticalAlignDefault;
  }
  return self;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"LayoutSpec: width %f, height %f", _width, _height];
}

- (BOOL)widthUndifined {
  return _widthMode == LynxMeasureModeIndefinite;
}

- (BOOL)heightUndifined {
  return _heightMode == LynxMeasureModeIndefinite;
}

- (BOOL)isEqualToSpec:(LynxLayoutSpec *)spec {
  return LynxSameMeasureMode(_widthMode, spec.widthMode) && _width == spec.width &&
         LynxSameMeasureMode(_heightMode, spec.heightMode) && _height == spec.height &&
         _textOverflow == spec.textOverflow && _whiteSpace == spec.whiteSpace &&
         _maxLineNum == spec.maxLineNum && _maxTextLength == spec.maxTextLength &&
         LynxSameLynxGradient(_textStyle.textGradient, spec.textStyle.textGradient) &&
         _enableTailColorConvert == spec.enableTailColorConvert &&
         _enableTextRefactor == spec.enableTextRefactor &&
         _enableNewClipMode == spec.enableNewClipMode &&
         _textSingleLineVerticalAlign == spec.textSingleLineVerticalAlign;
}

- (NSUInteger)hash {
  return (*(NSUInteger *)&_width) ^ (*(NSUInteger *)&_height) ^ _widthMode ^ _heightMode ^
         _textOverflow ^ _whiteSpace ^ _maxLineNum ^ (_enableTailColorConvert ? 1 : 0) ^
         (_enableTextRefactor ? 1 : 0) ^ (_enableNewClipMode ? 1 : 0) ^
         _textSingleLineVerticalAlign;
}

@end
