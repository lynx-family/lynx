// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxUIBaseInputShadowNode.h>
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxNativeLayoutNode.h>
#import <Lynx/LynxFontFaceManager.h>
#import <Lynx/LynxConverter+UI.h>
#import <Lynx/LynxColorUtils.h>
#import <Lynx/LynxUnitUtils.h>

#define UNDEFINED_INT NSUIntegerMax
#define UNDEFINED_FLOAT CGFLOAT_MIN

@interface LynxUIBaseInputShadowNode () <LynxFontFaceObserver>

@property (nonatomic, strong) NSString *value;
@property (nonatomic, strong) NSString *placeholder;
@property (nonatomic, assign) CGFloat fontSize;
@property (nonatomic, assign) CGFloat placeholderFontSize;
@property (nonatomic, strong) NSString *fontFamily;
@property (nonatomic, strong) NSString *placeholderFontFamily;
@property (nonatomic, assign) LynxFontStyleType fontStyle;
@property (nonatomic, assign) LynxFontStyleType placeholderFontStyle;
@property (nonatomic, assign) CGFloat lineHeight;
@property (nonatomic, assign) CGFloat lineSpacing;
@property (nonatomic, assign) UIFontWeight fontWeight;
@property (nonatomic, assign) UIFontWeight placeholderFontWeight;
@property (nonatomic, strong) NSMutableDictionary *inputAttrs;
@property (nonatomic, strong) NSMutableDictionary *placeholderAttrs;
@property (nonatomic, strong) NSMutableParagraphStyle *inputParagraphStyle;
@property (nonatomic, assign) BOOL defaultValueConsumed;

@end

@implementation LynxUIBaseInputShadowNode

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString *)tagName {
  if (self = [super initWithSign:sign tagName:tagName]) {
    _widthForMeasure = UNDEFINED_FLOAT;
    _fontSize = 14;
    _placeholderFontSize = UNDEFINED_FLOAT;
    _inputAttrs = [NSMutableDictionary dictionary];
    _placeholderAttrs = [NSMutableDictionary dictionary];
    _inputParagraphStyle = [[NSMutableParagraphStyle alloc] init];
    _inputAttrs[NSParagraphStyleAttributeName] = _inputParagraphStyle;
    _placeholderAttrs[NSParagraphStyleAttributeName] = _inputParagraphStyle;
    _fontWeight = UIFontWeightRegular;
    _placeholderFontWeight = UNDEFINED_FLOAT;
    _fontStyle = LynxFontStyleNormal;
    _placeholderFontStyle = UNDEFINED_INT;
    _fontFamily = nil;
    _placeholderFontFamily = nil;
  }
  return self;
}


- (void)adoptNativeLayoutNode:(int64_t)ptr {
  [self setMeasureDelegate:self];
  [super adoptNativeLayoutNode:ptr];
}





- (CGSize)measureNode:(LynxLayoutNode*)node
            withWidth:(CGFloat)width
            widthMode:(LynxMeasureMode)widthMode
               height:(CGFloat)height
           heightMode:(LynxMeasureMode)heightMode {

  LynxFontFaceContext* fontFaceContext = [self.uiOwner fontFaceContext];
  
  UIFont *font = [[LynxFontFaceManager sharedManager]
                  generateFontWithSize:self.fontSize
                  weight:self.fontWeight
                  style:self.fontStyle
                  fontFamilyName:self.fontFamily
                  fontFaceContext:fontFaceContext
                  fontFaceObserver:self];
  
  CGFloat placeholderFontSize = self.placeholderFontSize != UNDEFINED_FLOAT ? self.placeholderFontSize : self.fontSize;
  CGFloat placeholderFontWeight = self.placeholderFontWeight != UNDEFINED_FLOAT ? self.placeholderFontWeight : self.fontWeight;
  LynxFontStyleType placeholderFontStyle = self.placeholderFontStyle != UNDEFINED_INT ? self.placeholderFontStyle : self.fontStyle;
  NSString * placeholderFontFamily = self.placeholderFontFamily ? : self.fontFamily;
  
  
  UIFont *placeholderFont = [[LynxFontFaceManager sharedManager]
                  generateFontWithSize:placeholderFontSize
                  weight:placeholderFontWeight
                  style:placeholderFontStyle
                  fontFamilyName:placeholderFontFamily
                  fontFaceContext:fontFaceContext
                  fontFaceObserver:self];;
  
  self.inputAttrs[NSFontAttributeName] = font;
  self.placeholderAttrs[NSFontAttributeName] = placeholderFont;
  
  
  
  NSAttributedString * valueMeasureUnit = [[NSAttributedString alloc] initWithString:self.value.length ? self.value : @" " attributes:self.inputAttrs];
  
  NSAttributedString * placeholderMeasureUnit = self.placeholder.length ? [[NSAttributedString alloc] initWithString:self.placeholder attributes:self.placeholderAttrs] : nil;
  
  if (self.widthForMeasure != UNDEFINED_FLOAT) {
    width = self.widthForMeasure;
  }
  
  CGSize size = [valueMeasureUnit boundingRectWithSize:CGSizeMake(width, height) options:NSStringDrawingUsesLineFragmentOrigin | NSStringDrawingUsesFontLeading context:nil].size;
  
  if (placeholderMeasureUnit) {
    CGSize placeholderSize = [placeholderMeasureUnit boundingRectWithSize:CGSizeMake(width, height) options:NSStringDrawingUsesLineFragmentOrigin | NSStringDrawingUsesFontLeading context:nil].size;
    size.width = MAX(size.width, placeholderSize.width);
    size.height = MAX(size.height, placeholderSize.height);
  }
  
  if (!CGSizeEqualToSize(self.uiSize, CGSizeZero)) {
    size = self.uiSize;
  }
  
  if (widthMode == LynxMeasureModeDefinite) {
    size.width = width;
  } else if (widthMode == LynxMeasureModeAtMost) {
    size.width = MIN(size.width, width);
  }
  
  if (heightMode == LynxMeasureModeDefinite) {
    size.height = height;
  } else if (heightMode == LynxMeasureModeAtMost) {
    size.height = MIN(size.height, height);
  }
  
  return CGSizeMake(ceil(size.width), ceil(size.height));;
}



LYNX_PROP_SETTER("font-size", setFontSize, CGFloat) {
  self.fontSize = value;
}

LYNX_PROP_SETTER("placeholder-font-size", setPlaceholderFontSize, CGFloat) {
  self.placeholderFontSize = value;
}


- (UIFontWeight)lynxFontWeightToUIFontWeight:(LynxFontWeightType)value {
  if (value == LynxFontWeightNormal) {
    return UIFontWeightRegular;
  } else if (value == LynxFontWeightBold) {
    return UIFontWeightBold;
  } else if (value == LynxFontWeight100) {
    return UIFontWeightUltraLight;
  } else if (value == LynxFontWeight200) {
    return UIFontWeightThin;
  } else if (value == LynxFontWeight300) {
    return UIFontWeightLight;
  } else if (value == LynxFontWeight400) {
    return UIFontWeightRegular;
  } else if (value == LynxFontWeight500) {
    return UIFontWeightMedium;
  } else if (value == LynxFontWeight600) {
    return UIFontWeightSemibold;
  } else if (value == LynxFontWeight700) {
    return UIFontWeightBold;
  } else if (value == LynxFontWeight800) {
    return UIFontWeightHeavy;
  } else if (value == LynxFontWeight900) {
    return UIFontWeightBlack;
  } else {
    return UIFontWeightRegular;
  }
}
LYNX_PROP_SETTER("font-weight", setFontWeight, LynxFontWeightType) {
  self.fontWeight = [self lynxFontWeightToUIFontWeight:value];
}

LYNX_PROP_SETTER("placeholder-font-weight", setPlaceholderFontWeight, LynxFontWeightType) {
  self.placeholderFontWeight = [self lynxFontWeightToUIFontWeight:value];
}

LYNX_PROP_SETTER("font-style", setFontStyle, LynxFontStyleType) {
  self.fontStyle = value;
}

LYNX_PROP_SETTER("placeholder-font-style", setPlaceholderFontStyle, LynxFontStyleType) {
  self.placeholderFontStyle = value;
}

LYNX_PROP_SETTER("letter-spacing", setLetterSpacing, CGFloat) {
  self.inputAttrs[NSKernAttributeName] = @(value);
  self.placeholderAttrs[NSKernAttributeName] = @(value);
}

LYNX_PROP_SETTER("line-height", setLineHeight, CGFloat) {
  self.inputParagraphStyle.minimumLineHeight = value;
  self.inputParagraphStyle.maximumLineHeight = value;
}

LYNX_PROP_SETTER("line-spacing", setLineSpacing, NSString *) {
  CGFloat spacing = [LynxUnitUtils toPtWithScreenMetrics:[LynxScreenMetrics getDefaultLynxScreenMetrics]
                                    unitValue:value
                                 rootFontSize:0
                                  curFontSize:0
                                    rootWidth:0
                                   rootHeight:0
                                withDefaultPt:0];
  self.inputParagraphStyle.lineSpacing = spacing;
}

LYNX_PROP_SETTER("font-family", setFontFamily, NSString*) {
  self.fontFamily = value;
}

LYNX_PROP_SETTER("placeholder-font-family", setPlaceholderFontFamily, NSString*) {
  self.placeholderFontFamily = value;
}

LYNX_PROP_SETTER("text-align", setTextAlign, LynxTextAlignType) {
  if (value == LynxTextAlignCenter) {
    self.inputParagraphStyle.alignment = NSTextAlignmentCenter;
  } else if (value == LynxTextAlignLeft) {
    self.inputParagraphStyle.alignment = NSTextAlignmentLeft;
  } else if (value == LynxTextAlignRight) {
    self.inputParagraphStyle.alignment = NSTextAlignmentRight;
  } else if (value == LynxTextAlignStart) {
    self.inputParagraphStyle.alignment = NSTextAlignmentNatural;
  }
}

LYNX_PROP_SETTER("placeholder", setPlaceholder, NSString *) {
  if (requestReset) {
      value = nil;
  }
  self.placeholder = value;
}

LYNX_PROP_SETTER("default-value", setDefaultValue, NSString *) {
  if (self.defaultValueConsumed) {
    return;
  }
  self.value = (requestReset || value == nil) ? @"" : value;
}

- (void)propsDidUpdate {
  [super propsDidUpdate];
  self.defaultValueConsumed = YES;
  [self setNeedsLayout];
}


- (void)setNeedsLayout {
  @synchronized (self) {
    [super setNeedsLayout];
  }
}

- (void)internalSetNeedsLayoutForce {
  @synchronized (self) {
    [super internalSetNeedsLayoutForce];
  }
}

- (void)onFontFaceLoad {
}
@end
