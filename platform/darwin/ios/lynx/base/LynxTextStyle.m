// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTextStyle.h"
#import "LynxBackgroundImageLayerInfo.h"
#import "LynxFontFaceManager.h"

static CGFloat kLynxDefaultFontSize = 14;

NSAttributedStringKey const LynxTextColorGradientKey = @"LynxTextColorGradient";
NSAttributedStringKey const LynxInlineBackgroundKey = @"LynxInlineBackgroundKey";
NSAttributedStringKey const LynxWordBreakKey = @"LynxWordBreakKey";

@implementation LynxTextStyle

- (instancetype)init {
  if (self = [super init]) {
    // NAN means undefined
    _fontSize = NAN;
    _letterSpacing = NAN;
    _textAlignment = NSTextAlignmentNatural;
    _usedParagraphTextAlignment = NSTextAlignmentNatural;
    _direction = NSWritingDirectionNatural;
    _fontWeight = NAN;
    _fontStyle = LynxFontStyleNormal;
    _lineHeight = NAN;
    _lineSpacing = NAN;
    _fontFamilyName = nil;
    _underLine = nil;
    _lineThrough = nil;
    _textDecorationStyle = NSUnderlineStyleSingle;
    _textDecorationColor = nil;
    _textShadow = nil;
    _textGradient = nil;
    _enableFontScaling = NO;
    _textFakeBold = NO;
    _textIndent = 0;
    _textStrokeWidth = NAN;
    _isAutoFontSize = NO;
    _autoFontSizeMinSize = 0.f;
    _autoFontSizeMaxSize = 0.f;
    _autoFontSizeStepGranularity = 1.f;
    _autoFontSizePresetSizes = nil;
    _wordBreak = LynxWordBreakNormal;
  }

  return self;
}

- (void)applyTextStyle:(LynxTextStyle *)textStyle {
  _foregroundColor = textStyle->_foregroundColor ?: _foregroundColor;
  _backgroundColor = textStyle->_backgroundColor ?: _backgroundColor;
  _fontSize = !isnan(textStyle->_fontSize) ? textStyle->_fontSize : _fontSize;
  _fontWeight = !isnan(textStyle->_fontWeight) ? textStyle->_fontWeight : _fontWeight;
  _fontStyle = !isnan(textStyle->_fontStyle) ? textStyle->_fontStyle : _fontStyle;
  _letterSpacing = !isnan(textStyle->_letterSpacing) ? textStyle->_letterSpacing : _letterSpacing;
  _lineHeight = !isnan(textStyle->_lineHeight) ? textStyle->_lineHeight : _lineHeight;
  _lineSpacing = !isnan(textStyle->_lineSpacing) ? textStyle->_lineSpacing : _lineSpacing;
  _textAlignment = textStyle->_textAlignment;
  _usedParagraphTextAlignment = textStyle->_usedParagraphTextAlignment;
  _direction = textStyle->_direction;
  _fontFamilyName = textStyle->_fontFamilyName ? textStyle->_fontFamilyName : _fontFamilyName;
  _underLine = textStyle->_underLine ? textStyle->_underLine : _underLine;
  _lineThrough = textStyle->_lineThrough ? textStyle->_lineThrough : _lineThrough;
  _textDecorationStyle =
      textStyle->_textDecorationStyle ? textStyle->_textDecorationStyle : _textDecorationStyle;
  _textDecorationColor =
      textStyle->_textDecorationColor ? textStyle->_textDecorationColor : _textDecorationColor;
  _textShadow = textStyle->_textShadow ? textStyle->_textShadow : _textShadow;
  _textGradient = textStyle->_textGradient ? textStyle->_textGradient : _textGradient;
  _enableFontScaling = textStyle->_enableFontScaling;
  _textFakeBold = textStyle.textFakeBold;
  _textIndent = textStyle->_textIndent;
  _textStrokeWidth = textStyle.textStrokeWidth;
  _textStrokeColor = textStyle.textStrokeColor;
  _enableLanguageAlignment = textStyle.enableLanguageAlignment;
  _isAutoFontSize = textStyle.isAutoFontSize;
  _autoFontSizeMinSize = textStyle.autoFontSizeMinSize;
  _autoFontSizeMaxSize = textStyle.autoFontSizeMaxSize;
  _autoFontSizeStepGranularity = textStyle.autoFontSizeStepGranularity;
  _autoFontSizePresetSizes = textStyle.autoFontSizePresetSizes;
  _wordBreak = !isnan(textStyle.wordBreak) ? textStyle.wordBreak : _wordBreak;
}

- (LynxTextStyle *)copyWithZone:(NSZone *)zone {
  LynxTextStyle *textStyle = [LynxTextStyle new];
  [textStyle applyTextStyle:self];
  return textStyle;
}

- (CGFloat)fontScaleWithSizeCategory:(UIContentSizeCategory)category {
  // https://github.com/adamyanalunas/Font/blob/master/Pod/Classes/Font.swift#L37

  if ([UIContentSizeCategoryAccessibilityExtraExtraExtraLarge isEqual:category]) {
    return 2.0;
  } else if ([UIContentSizeCategoryAccessibilityExtraExtraLarge isEqual:category]) {
    return 1.9;
  } else if ([UIContentSizeCategoryAccessibilityExtraLarge isEqual:category]) {
    return 1.75;
  } else if ([UIContentSizeCategoryAccessibilityLarge isEqual:category]) {
    return 1.65;
  } else if ([UIContentSizeCategoryAccessibilityMedium isEqual:category]) {
    return 1.5;
  } else if ([UIContentSizeCategoryExtraExtraExtraLarge isEqual:category]) {
    return 1.4;
  } else if ([UIContentSizeCategoryExtraExtraLarge isEqual:category]) {
    return 1.25;
  } else if ([UIContentSizeCategoryExtraLarge isEqual:category]) {
    return 1.15;
  } else if ([UIContentSizeCategoryLarge isEqual:category]) {
    return 1.0;
  } else if ([UIContentSizeCategoryMedium isEqual:category]) {
    return 1.0 / 1.2;
  } else if ([UIContentSizeCategorySmall isEqual:category]) {
    return 1.0 / 1.4;
  } else if ([UIContentSizeCategoryExtraSmall isEqual:category]) {
    return 1.0 / 1.6;
  } else {
    return 1.0;
  }
}

- (UIFont *)applyFontScaling:(UIFont *)font {
  if (@available(iOS 11.0, *)) {
    font = [[UIFontMetrics defaultMetrics] scaledFontForFont:font];
  } else {
    // Fallback on earlier versions
    CGFloat scale = [self
        fontScaleWithSizeCategory:[[UIApplication sharedApplication] preferredContentSizeCategory]];
    if (scale != 1.0) {
      font = [font fontWithSize:_fontSize * scale];
    }
  }
  return font;
}

- (CGFloat)autoFontSizeStepGranularity {
  return _autoFontSizeStepGranularity > 0 ? _autoFontSizeStepGranularity : 1.f;
}

- (void)setTextAlignment:(NSTextAlignment)textAlignment {
  _textAlignment = textAlignment;
  _usedParagraphTextAlignment = textAlignment;
}

- (NSMutableArray *)backgroundImageSize {
  if (!_backgroundImageSize) {
    _backgroundImageSize = [[NSMutableArray alloc] init];
  }
  return _backgroundImageSize;
}

- (NSMutableArray *)backgroundPosition {
  if (!_backgroundPosition) {
    _backgroundPosition = [[NSMutableArray alloc] init];
  }
  return _backgroundPosition;
}

- (NSMutableArray *)backgroundRepeat {
  if (!_backgroundRepeat) {
    _backgroundRepeat = [[NSMutableArray alloc] init];
  }
  return _backgroundRepeat;
}

- (void)updateBackgroundDrawableRepeat {
  NSArray *repeat = self.backgroundRepeat;
  // set background repeatType
  LynxBackgroundRepeatType repeatXType = LynxBackgroundRepeatRepeat;
  LynxBackgroundRepeatType repeatYType = LynxBackgroundRepeatRepeat;
  if ([repeat count] >= 2) {
    const int usedRepeatIndex = 0;  // i % ([repeat count] / 2);
    repeatXType = [((NSNumber *)repeat[usedRepeatIndex * 2]) integerValue];
    repeatYType = [((NSNumber *)repeat[usedRepeatIndex * 2 + 1]) integerValue];
  }
  LynxBackgroundDrawable *drawable = self.backgroundDrawable.firstObject;
  drawable.repeatX = repeatXType;
  drawable.repeatY = repeatYType;
}

- (void)updateBackgroundDrawablePosition {
  LynxBackgroundDrawable *drawable = self.backgroundDrawable.firstObject;
  NSArray *pos = self.backgroundPosition;
  // set background image size
  if ([pos count] >= 2) {
    int usedPosIndex = 0;  // i % ([pos count] / 2);
    drawable.posX = pos[usedPosIndex * 2];
    drawable.posY = pos[usedPosIndex * 2 + 1];
  }
}

- (void)updateBackgroundDrawableSize {
  LynxBackgroundDrawable *drawable = self.backgroundDrawable.firstObject;
  NSArray *size = self.backgroundImageSize;
  // set background image size
  if ([size count] >= 2) {
    int i = 0;
    drawable.sizeX = size[i * 2];
    drawable.sizeY = size[i * 2 + 1];
  }
}

- (void)updateBackgroundRadius {
  LynxBackgroundDrawable *drawable = self.backgroundDrawable.firstObject;
  drawable.borderRadius = self.borderRadius;
}

- (UIFont *)fontWithFontFaceContext:(LynxFontFaceContext *)fontFaceContext
                   fontFaceObserver:(id<LynxFontFaceObserver>)observer {
  UIFont *font = [[LynxFontFaceManager sharedManager]
      generateFontWithSize:isnan(_fontSize) ? kLynxDefaultFontSize : _fontSize
                    weight:isnan(_fontWeight) ? UIFontWeightRegular : _fontWeight
                     style:_fontStyle
            fontFamilyName:_fontFamilyName
           fontFaceContext:fontFaceContext
          fontFaceObserver:observer];

  if (_enableFontScaling) {
    font = [self applyFontScaling:font];
  }
  return font;
}

- (NSDictionary<NSAttributedStringKey, id> *)
    toAttributesWithFontFaceContext:(LynxFontFaceContext *)fontFaceContext
               withFontFaceObserver:(id<LynxFontFaceObserver>)observer {
  NSMutableDictionary<NSAttributedStringKey, id> *attributes =
      [NSMutableDictionary dictionaryWithCapacity:5];

  UIFont *font = [self fontWithFontFaceContext:fontFaceContext fontFaceObserver:observer];
  if (font) {
    attributes[NSFontAttributeName] = font;
  }

  // If the font has no bold feature but the user needs it, use NSAttributedString to stroke text
  // and make it bold
  if (self.textFakeBold && self.fontWeight >= UIFontWeightBold &&
      !([[font fontDescriptor] symbolicTraits] & UIFontDescriptorTraitBold)) {
    attributes[NSStrokeWidthAttributeName] = [NSNumber numberWithFloat:-3];
  }

  UIColor *foregroundColor = self.foregroundColor;

  if (foregroundColor) {
    attributes[NSForegroundColorAttributeName] = foregroundColor;
  }

  UIColor *backgroundColor = self.backgroundColor;

  if (backgroundColor) {
    attributes[NSBackgroundColorAttributeName] = backgroundColor;
  }
  if (!isnan(self.textStrokeWidth) && self.textStrokeColor) {
    attributes[NSStrokeWidthAttributeName] =
        self.fontSize > 0 && !isnan(self.fontSize)
            ? [NSNumber numberWithFloat:-self.textStrokeWidth / self.fontSize * 100]
            : [NSNumber numberWithFloat:-self.textStrokeWidth / kLynxDefaultFontSize * 100];
    attributes[NSStrokeColorAttributeName] = self.textStrokeColor;
  }

  if (!isnan(_letterSpacing)) {
    attributes[NSKernAttributeName] = @(_letterSpacing);
  }

  attributes[NSParagraphStyleAttributeName] = [self genParagraphStyle];

  NSInteger textDecorationStyle = _textDecorationStyle;
  if (!(textDecorationStyle == NSUnderlineStyleDouble ||
        textDecorationStyle == NSUnderlineStyleSingle)) {
    textDecorationStyle |= NSUnderlineStyleSingle;
  }

  if (_underLine) {
    attributes[_underLine] = [NSNumber numberWithInteger:textDecorationStyle];
    if (_textDecorationColor) {
      attributes[NSUnderlineColorAttributeName] = _textDecorationColor;
    } else {
      attributes[NSUnderlineColorAttributeName] = foregroundColor;
    }
  }

  if (_lineThrough) {
    attributes[_lineThrough] = [NSNumber numberWithInteger:textDecorationStyle];
    if (_textDecorationColor) {
      attributes[NSStrikethroughColorAttributeName] = _textDecorationColor;
    } else {
      attributes[NSStrikethroughColorAttributeName] = foregroundColor;
    }
  }

  if (_textShadow) {
    attributes[NSShadowAttributeName] = _textShadow;
  }

  if (_textGradient) {
    attributes[LynxTextColorGradientKey] = _textGradient;
  }

  attributes[LynxWordBreakKey] = [[NSNumber alloc] initWithUnsignedInteger:_wordBreak];

  return attributes;
}

- (NSParagraphStyle *)genParagraphStyle {
  NSMutableParagraphStyle *paragraphStyle = [NSMutableParagraphStyle new];

  paragraphStyle.baseWritingDirection = _direction;

  NSTextAlignment nsTextAlignment = _textAlignment;
  if (nsTextAlignment != NSTextAlignmentNatural) {
    paragraphStyle.alignment = nsTextAlignment;
    if (paragraphStyle.alignment == NSTextAlignmentJustified) {
      // if not set,word will be stretched.
      paragraphStyle.hyphenationFactor = 1;
    }
  }

  if (!isnan(_lineHeight)) {
    paragraphStyle.minimumLineHeight = _lineHeight;
    paragraphStyle.maximumLineHeight = _lineHeight;
  }

  if (!isnan(_lineSpacing)) {
    paragraphStyle.lineSpacing = _lineSpacing;
  }

  if (_textIndent != 0) {
    paragraphStyle.firstLineHeadIndent = _textIndent;
  }

  if (_hyphen) {
    paragraphStyle.hyphenationFactor = 1.f;
  }

  return paragraphStyle;
}

@end
