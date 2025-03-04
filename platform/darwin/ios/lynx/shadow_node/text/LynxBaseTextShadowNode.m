// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBaseTextShadowNode.h"
#import "LynxBoxShadowManager.h"
#import "LynxConverter+NSShadow.h"
#import "LynxConverter+UI.h"
#import "LynxFontFaceManager.h"
#import "LynxGradient.h"
#import "LynxHtmlEscape.h"
#import "LynxInlineTruncationShadowNode.h"
#import "LynxLog.h"
#import "LynxNativeLayoutNode.h"
#import "LynxPropsProcessor.h"
#import "LynxRawTextShadowNode.h"
#import "LynxTextLayoutSpec.h"
#import "LynxTextSelectionShadowNode.h"
#import "LynxTextShadowNode.h"
#import "LynxTextUtils.h"

NSAttributedStringKey const LynxInlineViewAttributedStringKey =
    @"LynxInlineViewAttributedStringKey";
NSAttributedStringKey const LynxInlineTextShadowNodeSignKey = @"LynxInlineTextShadowNodeSignKey";
NSAttributedStringKey const LynxUsedFontMetricKey = @"LynxUsedFontMetricKey";
NSAttributedStringKey const LynxVerticalAlignKey = @"LynxVerticalAlignKey";

@interface LynxTextAttachment : NSTextAttachment

@end

@implementation LynxTextAttachment

- (nullable UIImage*)imageForBounds:(CGRect)imageBounds
                      textContainer:(nullable NSTextContainer*)textContainer
                     characterIndex:(NSUInteger)charIndex {
  return nil;
}

@end

@interface LynxBaseTextShadowNode () <LynxFontFaceObserver>

@end

@implementation LynxBaseTextShadowNode {
  BOOL _styleDirty;
  NSDictionary<NSAttributedStringKey, id>* _attributes;
}

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString*)tagName {
  self = [super initWithSign:sign tagName:tagName];
  if (self) {
    _textStyle = [LynxTextStyle new];
    _styleDirty = YES;
    _attributes = nil;  // create on first layoutDidStart callback
    _enableTextRefactor = false;
  }
  return self;
}

- (void)setUIOperation:(LynxUIOwner*)owner {
  [super setUIOperation:owner];
  if (self.uiOwner != nil) {
    _enableTextRefactor = self.uiOwner.uiContext.enableTextRefactor;
    _enableNewClipMode = self.uiOwner.uiContext.enableNewClipMode;
  }
}

// only use when enableTextRefactor is false
- (CGFloat)calcVerticalPosition:(LynxShadowNode*)node
                     nodeHeight:(CGFloat)nodeHeight
                           font:(UIFont*)font {
  LynxShadowNodeStyle* shadowNodeStyle = [node shadowNodeStyle];
  if (shadowNodeStyle != nil) {
    switch (shadowNodeStyle.valign) {
      case LynxVerticalAlignBaseline:
        return 0;
      case LynxVerticalAlignTop:
      case LynxVerticalAlignTextTop:
        return font.ascender - nodeHeight;
      case LynxVerticalAlignBottom:
      case LynxVerticalAlignTextBottom:
        return font.descender;
      case LynxVerticalAlignPercent:
        return font.lineHeight * shadowNodeStyle.valignLength / 100.f;
      case LynxVerticalAlignSub:
        return font.descender + font.lineHeight * 0.1f;
      case LynxVerticalAlignSuper:
        return font.ascender - nodeHeight - font.lineHeight * 0.1f;
      case LynxVerticalAlignMiddle:
        break;
      case LynxVerticalAlignLength:
        return shadowNodeStyle.valignLength;
      default:
        break;
    }
  }
  return 0.5f * (font.ascender + font.descender - nodeHeight);
}

- (NSAttributedString*)generateAttributedString:
                           (nullable NSDictionary<NSAttributedStringKey, id>*)baseTextAttribute
                              withTextMaxLength:(NSInteger)textMaxLength {
  return [self generateAttributedString:baseTextAttribute
                      withTextMaxLength:textMaxLength
                          withDirection:NSWritingDirectionNatural];
}

- (NSAttributedString*)generateAttributedString:
                           (nullable NSDictionary<NSAttributedStringKey, id>*)baseTextAttribute
                              withTextMaxLength:(NSInteger)textMaxLength
                                  withDirection:(NSWritingDirection)direction {
  NSMutableDictionary<NSAttributedStringKey, id>* textAttributes =
      [NSMutableDictionary dictionaryWithDictionary:baseTextAttribute];
  // override style value by self attributes
  [textAttributes addEntriesFromDictionary:_attributes];

  _hasNonVirtualOffspring = NO;

  NSMutableAttributedString* attributedString = [[NSMutableAttributedString alloc] init];
  [attributedString beginEditing];

  if (self.children.count == 0 && self.text) {
    [self handleRawTextValue:self.text
        withAttributedString:attributedString
           withTextMaxLength:&textMaxLength
           withTextAttribute:textAttributes
              withIsRootText:baseTextAttribute == nil
               withDirection:direction];
  }

  for (LynxShadowNode* child in self.children) {
    if ([child isKindOfClass:[LynxRawTextShadowNode class]]) {
      LynxRawTextShadowNode* rawText = (LynxRawTextShadowNode*)child;
      if (rawText.text) {
        [self handleRawTextValue:rawText.text
            withAttributedString:attributedString
               withTextMaxLength:&textMaxLength
               withTextAttribute:textAttributes
                  withIsRootText:baseTextAttribute == nil
                   withDirection:direction];
      }
    } else if ([child isKindOfClass:[LynxBaseTextShadowNode class]]) {
      LynxBaseTextShadowNode* baseText = (LynxBaseTextShadowNode*)child;
      NSAttributedString* baseTextAttributedString = nil;
      if (textMaxLength == LynxNumberNotSet) {
        baseTextAttributedString = [baseText generateAttributedString:textAttributes
                                                    withTextMaxLength:LynxNumberNotSet
                                                        withDirection:direction];
      } else if (textMaxLength > 0) {
        baseTextAttributedString = [baseText generateAttributedString:textAttributes
                                                    withTextMaxLength:textMaxLength
                                                        withDirection:direction];
        if (baseTextAttributedString != nil) {
          textMaxLength = baseTextAttributedString.length >= (NSUInteger)textMaxLength
                              ? 0
                              : textMaxLength - baseTextAttributedString.length;
        }
      }
      if (baseTextAttributedString != nil) {
        if ([child isKindOfClass:[LynxInlineTruncationShadowNode class]]) {
          self.textStyle.truncationAttributedStr = baseTextAttributedString;
        } else {
          [attributedString appendAttributedString:baseTextAttributedString];
        }
        if (baseText.textStyle.backgroundDrawable.count > 0) {
          [attributedString
              addAttribute:LynxInlineBackgroundKey
                     value:baseText.textStyle.backgroundDrawable
                     range:NSMakeRange(attributedString.length - baseTextAttributedString.length,
                                       baseTextAttributedString.length)];
        }

        _hasNonVirtualOffspring |= baseText.hasNonVirtualOffspring;
      }
    } else if ([child isKindOfClass:[LynxTextSelectionShadowNode class]]) {
      LynxTextSelectionShadowNode* selectionNode = (LynxTextSelectionShadowNode*)child;
      self.textStyle.selectionColor = selectionNode.backgroundColor;
    } else {
      // process inline-view and inline-image
      if (textMaxLength != LynxNumberNotSet) {
        if (textMaxLength <= 0) {
          continue;
        } else {
          textMaxLength--;
        }
      }
      if (![child isKindOfClass:[LynxNativeLayoutNode class]]) {
        // Now only support inline effect for leaf node!
        NSAssert(child.children != 0, @"only support inline effect for leaf node");
      }
      // Add other node as attachment
      NSTextAttachment* textAttachment = [LynxTextAttachment new];
      NSMutableAttributedString* inlineElementAttributedString = [NSMutableAttributedString new];
      const CGFloat width = child.style.computedWidth + child.style.computedMarginLeft +
                            child.style.computedMarginRight;
      CGFloat height = child.style.computedHeight + child.style.computedMarginTop +
                       child.style.computedMarginBottom;
      CGFloat y = 0.f;
      if (!self.enableTextRefactor && ![child isKindOfClass:[LynxNativeLayoutNode class]]) {
        UIFont* font = textAttributes[NSFontAttributeName];
        y = [self calcVerticalPosition:child nodeHeight:height font:font];
      }
      // inline-view's bounds will be reset after measuring inline-view
      textAttachment.bounds = CGRectMake(0, y, width, height);
      [inlineElementAttributedString beginEditing];
      [inlineElementAttributedString
          appendAttributedString:[NSAttributedString
                                     attributedStringWithAttachment:textAttachment]];
      [inlineElementAttributedString
          addAttribute:LynxInlineViewAttributedStringKey
                 value:child
                 range:(NSRange){0, inlineElementAttributedString.length}];
      [inlineElementAttributedString
          addAttribute:LynxVerticalAlignKey
                 value:[NSNumber numberWithInteger:child.shadowNodeStyle.valign]
                 range:(NSRange){0, inlineElementAttributedString.length}];

      [inlineElementAttributedString
          addAttributes:textAttributes
                  range:NSMakeRange(0, inlineElementAttributedString.length)];
      [inlineElementAttributedString endEditing];

      [attributedString appendAttributedString:inlineElementAttributedString];
      _hasNonVirtualOffspring = YES;
    }
  }

  [attributedString endEditing];

  return attributedString;
}

- (void)handleRawTextValue:(NSString*)text
      withAttributedString:(NSMutableAttributedString*)attributedString
         withTextMaxLength:(NSInteger*)textMaxLength
         withTextAttribute:(nullable NSDictionary<NSAttributedStringKey, id>*)textAttributes
            withIsRootText:(Boolean)isRootText
             withDirection:(NSWritingDirection)direction {
  NSInteger preLength = attributedString.length;
  NSString* newText = nil;
  if ([text rangeOfString:@"&"].location != NSNotFound) {
    newText = [text stringByUnescapingFromHtml];
  }
  NSAttributedString* rawTextAttributedString =
      [[NSAttributedString alloc] initWithString:(newText != nil ? newText : text)
                                      attributes:textAttributes];
  if (*textMaxLength == LynxNumberNotSet) {
    [attributedString appendAttributedString:rawTextAttributedString];
  } else if (*textMaxLength > 0) {
    if ((NSUInteger)*textMaxLength >= rawTextAttributedString.length) {
      [attributedString appendAttributedString:rawTextAttributedString];
      *textMaxLength -= rawTextAttributedString.length;
    } else {
      NSAttributedString* ellipsisAttributedString = [[NSAttributedString alloc]
          initWithString:[LynxTextUtils getEllpsisStringAccordingToWritingDirection:direction]
              attributes:textAttributes];
      rawTextAttributedString =
          [rawTextAttributedString attributedSubstringFromRange:NSMakeRange(0, *textMaxLength)];
      [attributedString appendAttributedString:rawTextAttributedString];
      [attributedString appendAttributedString:ellipsisAttributedString];
      *textMaxLength = 0;
    }
  }
  if (!isRootText) {
    // if contains baseTextAttribute means this is not root TextShadowNode
    // need to mark ShadowNode sign on this range for future use
    [attributedString addAttribute:LynxInlineTextShadowNodeSignKey
                             value:self
                             range:NSMakeRange(preLength, attributedString.length - preLength)];
    [attributedString addAttribute:LynxVerticalAlignKey
                             value:[NSNumber numberWithInteger:self.shadowNodeStyle.valign]
                             range:NSMakeRange(preLength, attributedString.length - preLength)];
  }
}

- (void)markStyleDirty {
  _styleDirty = YES;
}

- (void)layoutDidStart {
  [super layoutDidStart];

  self.textStyle.truncationAttributedStr = nil;
  if (self.parent != nil && [self.parent isKindOfClass:[LynxTextShadowNode class]]) {
    CGFloat parentIndent = ((LynxBaseTextShadowNode*)self.parent).textStyle.textIndent;
    if (parentIndent != _textStyle.textIndent) {
      _textStyle.textIndent = parentIndent;
      [self markStyleDirty];
    }
  }

  if (_styleDirty) {
    LynxFontFaceContext* fontFaceContext = [self.uiOwner fontFaceContext];

    // Text related properties is inheritable with "text" tag even if css inheritance is disabled
    bool fontFamilyInherited = NO;
    if (_enableTextRefactor && _textStyle.fontFamilyName == nil && self.parent != nil &&
        [self.parent isKindOfClass:[LynxBaseTextShadowNode class]]) {
      // Font family should be inherited from parent if it is not set.
      fontFamilyInherited = YES;
      _textStyle.fontFamilyName = ((LynxBaseTextShadowNode*)self.parent).textStyle.fontFamilyName;
    }

    _attributes = [self.textStyle toAttributesWithFontFaceContext:fontFaceContext
                                             withFontFaceObserver:self];

    if (fontFamilyInherited) {
      // Remove the inherited font family
      _textStyle.fontFamilyName = nil;
    }

    _styleDirty = NO;
  }
  for (LynxShadowNode* child in self.children) {
    if ([child isVirtual]) {
      // only virtual child need parent to trigger this callback
      [child layoutDidStart];
    }
  }
}

- (BOOL)hasCustomLayout {
  return YES;
}

- (void)onFontFaceLoad {
  _styleDirty = YES;
  [self setNeedsLayout];
}

- (BOOL)isUndefinedValue:(CGFloat)value {
  return value >= 10E8 || value <= -10E8;
}

LYNX_PROP_SETTER("font-size", setFontSize, CGFloat) {
  if (requestReset) {
    value = NAN;
  }
  if (_textStyle.fontSize != value) {
    _textStyle.fontSize = value;
    [self markStyleDirty];
    [self setNeedsLayout];
  }
}

LYNX_PROP_SETTER("background-color", setBackgroundColor, UIColor*) {
  if (requestReset) {
    value = nil;
  }
  if (_textStyle.backgroundColor != value) {
    _textStyle.backgroundColor = value;
    [self markStyleDirty];
    [self setNeedsLayout];
  }
}

LYNX_PROP_SETTER("color", setColor, id) {
  if (requestReset) {
    value = nil;
  }
  if (value == nil || [value isKindOfClass:NSNumber.class]) {
    _textStyle.foregroundColor = [LynxConverter toUIColor:value];
    _textStyle.textGradient = nil;
  } else {
    _textStyle.foregroundColor = [UIColor blackColor];
    if ([value isKindOfClass:NSArray.class]) {
      [self setTextGradient:(NSArray*)value];
    } else {
      [self setTextGradient:nil];
    }
  }
  [self markStyleDirty];
  [self setNeedsLayout];
}

- (void)setTextGradient:(NSArray*)value {
  if (value == nil || [value count] < 2 || ![value[1] isKindOfClass:[NSArray class]]) {
    _textStyle.textGradient = nil;
  } else {
    NSUInteger type = [LynxConverter toNSUInteger:value[0]];
    NSArray* args = (NSArray*)value[1];
    if (type == LynxBackgroundImageLinearGradient) {
      _textStyle.textGradient = [[LynxLinearGradient alloc] initWithArray:args];
    } else if (type == LynxBackgroundImageRadialGradient) {
      _textStyle.textGradient = [[LynxRadialGradient alloc] initWithArray:args];
    }
  }
}

LYNX_PROP_SETTER("font-weight", setFontWeight, LynxFontWeightType) {
  if (requestReset) {
    value = LynxFontWeightNormal;
  }
  _textStyle.fontWeight = [LynxTextUtils convertLynxFontWeight:value];
  [self markStyleDirty];
  [self setNeedsLayout];
}

LYNX_PROP_SETTER("font-style", setFontStyle, LynxFontStyleType) {
  if (requestReset) {
    value = LynxFontStyleNormal;
  }
  if (_textStyle.fontStyle != value) {
    _textStyle.fontStyle = value;
    [self markStyleDirty];
    [self setNeedsLayout];
  }
}

LYNX_PROP_SETTER("line-height", setLineHeight, CGFloat) {
  if (_enableTextRefactor) {
    // There is no way to support inline line-height based on pure Android text layout.
    // So let's make it clear, Lynx won't support line-height on inline-text component.
    if (![self isKindOfClass:[LynxTextShadowNode class]]) {
      return;
    }
  }

  if (requestReset || [self isUndefinedValue:value]) {
    value = NAN;
  }
  if (_textStyle.lineHeight != value) {
    _textStyle.lineHeight = value;
    [self markStyleDirty];
    [self setNeedsLayout];
  }
}

LYNX_PROP_SETTER("line-spacing", setLineSpacing, CGFloat) {
  if (requestReset) {
    value = NAN;
  }
  if (_textStyle.lineSpacing != value) {
    _textStyle.lineSpacing = value;
    [self markStyleDirty];
    [self setNeedsLayout];
  }
}

LYNX_PROP_SETTER("letter-spacing", setLetterSpacing, CGFloat) {
  if (requestReset) {
    value = NAN;
  }
  if (_textStyle.letterSpacing != value) {
    _textStyle.letterSpacing = value;
    [self markStyleDirty];
    [self setNeedsLayout];
  }
}

LYNX_PROP_SETTER("font-family", setFontFamily, NSString*) {
  if ((requestReset && _textStyle.fontFamilyName) ||
      (value && ![value isEqualToString:_textStyle.fontFamilyName])) {
    _textStyle.fontFamilyName = value;
    [self markStyleDirty];
    [self setNeedsLayout];
  }
}

LYNX_PROP_SETTER("direction", setDirection, LynxDirectionType) {
  if (requestReset) {
    value = LynxDirectionLtr;
  }
  if (value == LynxDirectionNormal) {
    _textStyle.direction = NSWritingDirectionNatural;
  } else if (value == LynxDirectionLtr) {
    _textStyle.direction = NSWritingDirectionLeftToRight;
  } else {
    _textStyle.direction = NSWritingDirectionRightToLeft;
  }
  [self markStyleDirty];
  [self setNeedsLayout];
}

LYNX_PROP_SETTER("text-align", setTextAlign, LynxTextAlignType) {
  if (requestReset) {
    value = LynxTextAlignStart;
  }
  if (value == LynxTextAlignLeft) {
    _textStyle.textAlignment = NSTextAlignmentLeft;
  } else if (value == LynxTextAlignRight) {
    _textStyle.textAlignment = NSTextAlignmentRight;
  } else if (value == LynxTextAlignCenter) {
    _textStyle.textAlignment = NSTextAlignmentCenter;
  } else if (value == LynxTextAlignStart) {
    _textStyle.textAlignment = NSTextAlignmentNatural;
  } else if (value == LynxTextAlignJustify) {
    _textStyle.textAlignment = NSTextAlignmentJustified;
  } else {
    LLogFatal(@"Unexpected text align type");
  }

  [self markStyleDirty];
  [self setNeedsLayout];
}

LYNX_PROP_SETTER("text-decoration", setTextDecoration, NSArray*) {
  if (requestReset) {
    value = nil;
    _textStyle.underLine = nil;
    _textStyle.lineThrough = nil;
    _textStyle.textDecorationStyle = NSUnderlineStyleSingle;
    _textStyle.textDecorationColor = nil;
    [self markStyleDirty];
    [self setNeedsLayout];
    return;
  }
  NSInteger textDecorationLine = [LynxConverter toNSInteger:value[0]];
  NSInteger textDecorationStyle = [LynxConverter toNSInteger:value[1]];
  NSInteger color = [LynxConverter toNSInteger:value[2]];
  UIColor* textDecoationColor = [LynxConverter toUIColor:value[2]];

  if (textDecorationLine & LynxTextDecorationUnderLine) {
    _textStyle.underLine = NSUnderlineStyleAttributeName;
  }
  if (textDecorationLine & LynxTextDecorationLineThrough) {
    _textStyle.lineThrough = NSStrikethroughStyleAttributeName;
  }
  if (0 == textDecorationLine) {
    _textStyle.underLine = nil;
    _textStyle.lineThrough = nil;
  }

  switch (textDecorationStyle) {
    case LynxTextDecorationSolid:
      _textStyle.textDecorationStyle = NSUnderlineStylePatternSolid;
      break;
    case LynxTextDecorationDouble:
      _textStyle.textDecorationStyle = NSUnderlineStyleDouble;
      break;
    case LynxTextDecorationDotted:
      _textStyle.textDecorationStyle = NSUnderlineStylePatternDot;
      break;
    case LynxTextDecorationDashed:
      _textStyle.textDecorationStyle = NSUnderlineStylePatternDash;
      break;
    // todo wavy
    default:
      _textStyle.textDecorationStyle = NSUnderlineStyleSingle;
  }

  if (color != 0) {
    _textStyle.textDecorationColor = textDecoationColor;
  }
  [self markStyleDirty];
  [self setNeedsLayout];
}

LYNX_PROP_SETTER("text-shadow", setTextShadow, NSArray*) {
  if (requestReset) {
    value = nil;
  }
  NSArray<LynxBoxShadow*>* shadowArr = [LynxConverter toLynxBoxShadow:value];
  _textStyle.textShadow = [LynxConverter toNSShadow:shadowArr];
  [self markStyleDirty];
  [self setNeedsLayout];
}

LYNX_PROP_SETTER("text-stroke-width", setTextStrokeWidth, CGFloat) {
  if (requestReset) {
    value = NAN;
  }
  if (_textStyle.textStrokeWidth != value) {
    _textStyle.textStrokeWidth = value;
    [self markStyleDirty];
    [self setNeedsLayout];
  }
}

LYNX_PROP_SETTER("text-stroke-color", setTextStrokeColor, UIColor*) {
  if (requestReset) {
    value = nil;
  }
  if ([value isKindOfClass:UIColor.class]) {
    _textStyle.textStrokeColor = value;
  } else if ([value isKindOfClass:NSNumber.class]) {
    _textStyle.textStrokeColor = [LynxConverter toUIColor:value];
  } else if (!value) {
    _textStyle.textStrokeColor = [UIColor blackColor];
  }
  if (_textStyle.textStrokeWidth != NAN) {
    [self markStyleDirty];
    [self setNeedsLayout];
  }
}

LYNX_PROP_SETTER("enable-font-scaling", setEnableFontScaling, BOOL) {
  if (requestReset) {
    value = NO;
  }
  _textStyle.enableFontScaling = value;
  [self markStyleDirty];
  [self setNeedsLayout];
}

// TODO: remove setter later
LYNX_PROP_SETTER("text-fake-bold", setTextFakeBold, BOOL) {
  if (requestReset) {
    value = NO;
  }
  _textStyle.textFakeBold = value;
  [self markStyleDirty];
  [self setNeedsLayout];
}

// text alignment refactor, Guess best matched language basing on the content.
LYNX_PROP_SETTER("enable-language-alignment", setEnableLanguageAlignment, BOOL) {
  if (requestReset) {
    value = NO;
  }
  _textStyle.enableLanguageAlignment = value;
}

LYNX_PROP_SETTER("word-break", setWordBreakStrategy, LynxWordBreakType) {
  if (requestReset) {
    value = LynxWordBreakNormal;
  }
  // simplest implement for css word-break
  // current only handle `break-all` and `keep-all`, or other mode is fallback to
  // WordBreakStrategyNone
  self.textStyle.wordBreak = value;
  [self setNeedsLayout];
}

LYNX_PROP_SETTER("text", setText, id) {
  if (requestReset) {
    value = nil;
  }
  NSString* text = [LynxTextUtils ConvertRawText:value];
  if (_text != text) {
    _text = text;
    [self setNeedsLayout];
  }
}

@end
