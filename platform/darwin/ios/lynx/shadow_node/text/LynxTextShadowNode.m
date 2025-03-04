// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTextShadowNode.h"
#import <CoreText/CoreText.h>
#import "LynxComponentRegistry.h"
#import "LynxLog.h"
#import "LynxNativeLayoutNode.h"
#import "LynxPropsProcessor.h"
#import "LynxRootUI.h"
#import "LynxTemplateRender.h"
#import "LynxTextRendererCache.h"
#import "LynxTextUtils.h"
#import "LynxTraceEvent.h"
#import "LynxTraceEventWrapper.h"

// This is an adaptaion for one of the bug of line spacing in TextKit that
// the last line will disappeare when both maxNumberOfLines and lineSpacing are set.
//
// FIXME(yxping): there still another bug that both height and lineSpacing are set,
// the last line in the visible area will disappeare.
@implementation LineSpacingAdaptation

- (BOOL)layoutManager:(NSLayoutManager *)layoutManager
    shouldBreakLineByWordBeforeCharacterAtIndex:(NSUInteger)charIndex {
  LynxWordBreakType wordBreak = [self getWordBreakStyleAtIndex:charIndex];
  if (wordBreak == LynxWordBreakBreakAll) {
    return NO;
  } else if (wordBreak == LynxWordBreakKeepAll && charIndex > 0) {
    LynxWordBreakType wordBreakBefore = [self getWordBreakStyleAtIndex:charIndex - 1];
    if (wordBreakBefore != LynxWordBreakKeepAll) {
      return YES;
    }
    NSString *str = layoutManager.textStorage.string;
    uint32_t current = [str characterAtIndex:charIndex];
    uint32_t before = [str characterAtIndex:charIndex - 1];
    if (charIndex < str.length - 1 && [self isHighSurrogate:current]) {
      current = [self surrogateToU32WithHigh:current Low:[str characterAtIndex:charIndex + 1]];
    }
    if (charIndex > 1 && [self isLowSurrogate:before]) {
      before = [self surrogateToU32WithHigh:[str characterAtIndex:charIndex - 2] Low:before];
    }
    if ([self isCJKChar:current] && [self isCJKChar:before]) {
      return NO;
    }
  }
  return YES;
}

- (LynxWordBreakType)getWordBreakStyleAtIndex:(NSUInteger)charIndex {
  NSRange range = NSMakeRange(0, _attributedString.length);
  id wordBreak = [_attributedString attribute:LynxWordBreakKey
                                      atIndex:charIndex
                               effectiveRange:&range];
  if (wordBreak == NULL || ![wordBreak isKindOfClass:NSNumber.class]) {
    return LynxWordBreakNormal;
  }
  NSUInteger wordBreakStyle = ((NSNumber *)wordBreak).unsignedIntegerValue;
  return wordBreakStyle;
}

- (int32_t)surrogateToU32WithHigh:(unichar)high Low:(unichar)low {
  return ((high - 0xD800) << 10) + (low - 0xDC00) + 0x10000;
}

- (BOOL)isHighSurrogate:(unichar)character {
  return character >= 0xD800 && character <= 0xDBFF;
}

- (BOOL)isLowSurrogate:(unichar)character {
  return character >= 0xDC00 && character <= 0xDFFF;
}

- (BOOL)isCJKChar:(int32_t)character {
  return (character >= 0x4e00 && character <= 0x9fff) ||   /* CJK Unified Ideographs */
         (character >= 0x3400 && character <= 0x4dbf) ||   /* CJK Unified Ideographs Extention A */
         (character >= 0x20000 && character <= 0x2a6df) || /* CJK Unified Ideographs Extention B */
         (character >= 0x2e80 && character <= 0x2eff) ||   /* CJK Radicals Supplement */
         (character >= 0xf900 && character <= 0xfaff) ||   /* CJK Compatibility Ideographs */
         (character >= 0x3040 && character <= 0x30ff) ||   /* Hiragana and Katakana*/
         (character >= 0x31f0 && character <= 0x31ff) ||   /* Katakana Phonetic Extensions */
         (character >= 0x1100 && character <= 0x11ff) ||   /* Hangul */
         (character > +0xac00 && character <= 0xd7a3) /* Hangul Syllables */;
}

- (BOOL)layoutManager:(NSLayoutManager *)layoutManager
    shouldSetLineFragmentRect:(inout CGRect *)lineFragmentRect
         lineFragmentUsedRect:(inout CGRect *)lineFragmentUsedRect
               baselineOffset:(inout CGFloat *)baselineOffset
              inTextContainer:(NSTextContainer *)textContainer
                forGlyphRange:(NSRange)glyphRange {
  NSTextStorage *textStorage = layoutManager.textStorage;
  // When it returns YES, the layout manager uses the modified rects. Otherwise, it ignores the
  // rects returned from this method.
  BOOL isValid = YES;
  if (_enableLayoutRefactor) {
    __block CGFloat usedBaselinePosition = *baselineOffset;
    __block CGFloat usedLineRectHeight = lineFragmentRect->size.height;
    NSRange character_range = [layoutManager characterRangeForGlyphRange:glyphRange
                                                        actualGlyphRange:nil];

    __block bool hasInlineElement = NO;

    // if set line-height, align the content in the center of line
    // if baseline is out of visual line rect, move the baseline
    if (_lineHeight != 0) {
      // center text in line
      usedBaselinePosition = _maxLineAscender + _halfLeading;
      if (_halfLeading < 0) {
        // baseline will be up if descender > 0
        if (_lineHeight - usedBaselinePosition < 0) {
          usedBaselinePosition = _lineHeight;
        }
        // baseline will be down if ascender < 0
        if (usedBaselinePosition < 0) {
          usedBaselinePosition = 0;
        }
      }

      // if inline-view or inline-image is lager than line-height, line-height will increase.
      [textStorage
          enumerateAttribute:NSAttachmentAttributeName
                     inRange:character_range
                     options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                  usingBlock:^(NSTextAttachment *attachment, NSRange range, __unused BOOL *stop) {
                    if (attachment == nil) {
                      return;
                    }
                    hasInlineElement = YES;

                    CGRect bounds = attachment.bounds;
                    // bounds.origin.y is distance from attachment bottom to baseline
                    CGFloat attachmentTopToBaseline = bounds.size.height + bounds.origin.y;

                    NSRange attachmentRange;
                    id node = [textStorage attribute:LynxInlineViewAttributedStringKey
                                             atIndex:range.location
                                      effectiveRange:&attachmentRange];

                    if (node != nil) {
                      // modify line ascender
                      if (attachmentTopToBaseline > 0 &&
                          attachmentTopToBaseline > usedBaselinePosition) {
                        usedLineRectHeight += attachmentTopToBaseline - usedBaselinePosition;
                        usedBaselinePosition = attachmentTopToBaseline;
                      }
                      // modify line descender
                      if (bounds.origin.y < 0 &&
                          (-bounds.origin.y > usedLineRectHeight - usedBaselinePosition)) {
                        usedLineRectHeight +=
                            -usedLineRectHeight + usedBaselinePosition - bounds.origin.y;
                      }
                    }
                  }];
    }

    if (!hasInlineElement && self.textSingleLineVerticalAlign != LynxVerticalAlignDefault) {
      NSRange characterRange = [layoutManager characterRangeForGlyphRange:glyphRange
                                                         actualGlyphRange:nil];
      if (characterRange.length > 0 && characterRange.length == self.attributedString.length) {
        [self calcTextSingleLineVerticalAlignBaseline:lineFragmentRect->size.height
                                       baselineOffset:baselineOffset];
      }
    } else {
      lineFragmentRect->size.height = usedLineRectHeight + _calculatedLineSpacing;
      lineFragmentUsedRect->size.height = usedLineRectHeight;
      *baselineOffset = usedBaselinePosition;
    }
  } else {
    __block CGFloat maximumLineHeight = 0;
    [textStorage enumerateAttribute:NSFontAttributeName
                            inRange:NSMakeRange(0, textStorage.length)
                            options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                         usingBlock:^(UIFont *font, NSRange range, __unused BOOL *stop) {
                           if (font) {
                             maximumLineHeight = MAX(maximumLineHeight, font.lineHeight);
                             // We are not traversing the paragraph attribute here because the
                             // maximumLineHeight are set to the same value if paragraph attribute
                             // exists.
                             NSParagraphStyle *paragraphStyle =
                                 [textStorage attribute:NSParagraphStyleAttributeName
                                                atIndex:range.location
                                         effectiveRange:nil];
                             if (paragraphStyle && paragraphStyle.maximumLineHeight > 0) {
                               maximumLineHeight =
                                   MIN(maximumLineHeight, paragraphStyle.maximumLineHeight);
                             }
                           }
                         }];
    CGRect usedRect = *lineFragmentUsedRect;
    usedRect.size.height = MAX(maximumLineHeight, usedRect.size.height);
    *lineFragmentUsedRect = usedRect;
    if (_adjustBaseLineOffsetForVerticalAlignCenter) {
      *baselineOffset = (*lineFragmentRect).size.height;
    }
    if (ABS(_calculatedLineSpacing) < FLT_EPSILON) {
      isValid = NO;
    } else {
      CGRect rect = *lineFragmentRect;
      // We add lineSpacing to lineFragmentRect instead of adding to lineFragmentUsedRect
      // to avoid last sentance have a extra lineSpacing pading.
      rect.size.height += _calculatedLineSpacing;
      *lineFragmentRect = rect;
    }
  }
  if (glyphRange.location == 0) {
    if (!_enableLayoutRefactor && _adjustBaseLineOffsetForVerticalAlignCenter) {
      _baseline = *baselineOffset - _baseLineOffsetForVerticalAlignCenter;
    } else {
      _baseline = *baselineOffset;
    }
  }
  return isValid;
}

- (void)calcTextSingleLineVerticalAlignBaseline:(CGFloat)lineRectHeight
                                 baselineOffset:(inout CGFloat *)baselineOffset {
  CTLineRef line =
      CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)self.attributedString);
  CFArrayRef runs = CTLineGetGlyphRuns(line);
  CGFloat maxOffsetFromTopToBaseline = 0.f, minOffsetFromBottomToBaseline = CGFLOAT_MAX;
  for (int i = 0; i < CFArrayGetCount(runs); i++) {
    CTRunRef run = CFArrayGetValueAtIndex(runs, i);

    CGContextRef context = UIGraphicsGetCurrentContext();
    CGRect bounds = CTRunGetImageBounds(run, context, CFRangeMake(0, 0));
    maxOffsetFromTopToBaseline =
        MAX(maxOffsetFromTopToBaseline, bounds.size.height + bounds.origin.y);
    minOffsetFromBottomToBaseline = MIN(minOffsetFromBottomToBaseline, bounds.origin.y);
  }
  CFRelease(line);

  CGFloat baseline = 0.f;
  CGFloat roundLineHeight = [LynxUIUnitUtils roundPtToPhysicalPixel:lineRectHeight];
  CGFloat roundMaxHeight = [LynxUIUnitUtils
      roundPtToPhysicalPixel:maxOffsetFromTopToBaseline - minOffsetFromBottomToBaseline];
  CGFloat roundMinOffsetFromBottomToBaseline =
      [LynxUIUnitUtils roundPtToPhysicalPixel:minOffsetFromBottomToBaseline];
  if (self.textSingleLineVerticalAlign == LynxVerticalAlignBottom) {
    baseline = roundLineHeight + roundMinOffsetFromBottomToBaseline;
  } else if (self.textSingleLineVerticalAlign == LynxVerticalAlignTop) {
    baseline = roundMaxHeight + roundMinOffsetFromBottomToBaseline;
  } else {
    baseline = (roundLineHeight + roundMaxHeight + 2 * roundMinOffsetFromBottomToBaseline) / 2;
  }

  *baselineOffset = baseline;
}

- (CGFloat)layoutManager:(NSLayoutManager *)layoutManager
    lineSpacingAfterGlyphAtIndex:(NSUInteger)glyphIndex
    withProposedLineFragmentRect:(CGRect)rect {
  // Do not include lineSpacing in lineFragmentUsedRect to avoid last line disappearing
  return 0;
}

@end

// LynxTextIndent
@interface LynxTextIndent : NSObject

@property(nonatomic, assign) NSInteger type;
@property(nonatomic, assign) CGFloat value;

- (instancetype _Nullable)initWithValue:(NSArray *)value;

- (CGFloat)applyValue:(CGFloat)widthValue;

@end

@implementation LynxTextIndent

- (instancetype)initWithValue:(NSArray *)value {
  self = [super init];
  if (self) {
    self.value = [value[0] floatValue];
    self.type = [value[1] integerValue];
  }
  return self;
}

- (CGFloat)applyValue:(CGFloat)widthValue {
  return self.type == LynxPlatformLengthUnitNumber ? self.value : self.value * widthValue;
}

@end

// LynxTextShadowNode
@interface LynxTextShadowNode ()

@property(nonatomic) LynxTextRenderer *textRenderer;
@property(nonatomic) LineSpacingAdaptation *lineSpacingAdaptation;

@property(readwrite, nonatomic, assign) LynxTextOverflowType textOverflow;
@property(readwrite, nonatomic, assign) LynxOverflow overflow;
@property(readwrite, nonatomic, assign) LynxWhiteSpaceType whiteSpace;
@property(readwrite, nonatomic, assign) NSInteger maxLineNum;
@property(readwrite, nonatomic, assign) NSInteger maxTextLength;
@property(readwrite, nonatomic, assign) LynxVerticalAlign textVerticalAlign;
@property(readwrite, nonatomic) NSMutableAttributedString *attrString;
@property(readwrite, nonatomic, assign) BOOL enableTailColorConvert;
@property(readwrite, nonatomic, assign) BOOL hyphen;
// text
@property(readwrite, nonatomic, assign) CGFloat maxAscender;
@property(readwrite, nonatomic, assign) CGFloat maxDescender;
// line, include inline-image and inline-view
@property(readwrite, nonatomic, assign) CGFloat maxLineAscender;
@property(readwrite, nonatomic, assign) CGFloat maxLineDescender;

@property(readwrite, nonatomic, assign) CGFloat maxTruncationLineAscender;
@property(readwrite, nonatomic, assign) CGFloat maxTruncationLineDescender;

@property(nonatomic, nonatomic, assign) CGFloat maxXHeight;
@property(nonatomic, nonatomic, assign) BOOL isCalcVerticalAlignValue;
@property(readwrite, nonatomic, strong, nullable) LynxTextIndent *textIndent;
@property(readwrite, nonatomic, assign) LynxVerticalAlign textSingleLineVerticalAlign;

@end

@implementation LynxTextShadowNode

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_SHADOW_NODE("text")
#else
LYNX_REGISTER_SHADOW_NODE("text")
#endif

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString *)tagName {
  self = [super initWithSign:sign tagName:tagName];
  if (self) {
    _lineSpacingAdaptation = [LineSpacingAdaptation new];
    _maxTextLength = LynxNumberNotSet;
    _maxLineNum = LynxNumberNotSet;
    _textVerticalAlign = LynxVerticalAlignDefault;
    _textIndent = nil;
  }
  return self;
}

- (void)adoptNativeLayoutNode:(int64_t)ptr {
  [self setCustomMeasureDelegate:self];
  [super adoptNativeLayoutNode:ptr];
}

- (BOOL)enableTextNonContiguousLayout {
  if (self.uiOwner.uiContext.rootView) {
    // This option can be set by LynxView builder and PageConfig, and either one of them is true,
    // text will enable non contiguous layout.
    return self.uiOwner.uiContext.rootView.enableTextNonContiguousLayout;
  } else {
    // From PageConfig
    return self.uiOwner.uiContext.enableTextNonContiguousLayout;
  }
}

- (MeasureResult)measureWithMeasureParam:(MeasureParam *)param
                          MeasureContext:(MeasureContext *)ctx {
#if ENABLE_TRACE_PERFETTO
  NSString *textValue = [self.attrString string];
  // The average word in the English language is approximately 5 characters.
  // 10 words may be enough to distinguish text.
  LYNX_TRACE_SECTION_WITH_INFO(
      LYNX_TRACE_CATEGORY_WRAPPER, @"text.TextShadowNode.measure",
      textValue.length > 50
          ? @{@"first_fifty_characters" : [textValue substringWithRange:NSMakeRange(0, 50)]}
          : @{@"characters" : textValue == nil ? @"" : textValue})
#endif
  LynxLayoutSpec *spec = [[LynxLayoutSpec alloc] initWithWidth:param.width
                                                        height:param.height
                                                     widthMode:param.widthMode
                                                    heightMode:param.heightMode
                                                  textOverflow:self.textOverflow
                                                      overflow:self.overflow
                                                    whiteSpace:self.whiteSpace
                                                    maxLineNum:self.maxLineNum
                                                 maxTextLength:self.maxTextLength
                                                     textStyle:self.textStyle
                                        enableTailColorConvert:self.enableTailColorConvert];
  spec.enableTextRefactor = self.enableTextRefactor;
  spec.enableTextNonContiguousLayout = [self enableTextNonContiguousLayout];
  spec.enableNewClipMode = self.enableNewClipMode;
  spec.layoutManagerDelegate = _lineSpacingAdaptation;
  spec.verticalAlign = self.textVerticalAlign;
  spec.textSingleLineVerticalAlign = self.textSingleLineVerticalAlign;

  [self calculateLineAscenderAndDescenderForAttributeStr:self.attrString
                                            MeasureParam:param
                                          MeasureContext:ctx];

  self.lineSpacingAdaptation.textSingleLineVerticalAlign = self.textSingleLineVerticalAlign;
  if (self.textStyle.truncationAttributedStr.length > 0) {
    self.textRenderer = [[LynxTextRenderer alloc] initWithAttributedString:[self.attrString copy]
                                                                layoutSpec:spec];
    __weak typeof(self) weak_self = self;
    self.textRenderer.layoutTruncationBlock = ^(NSMutableAttributedString *_Nonnull attributedStr) {
      __strong typeof(weak_self) _self = weak_self;
      [_self calculateLineAscenderAndDescenderForAttributeStr:attributedStr
                                                 MeasureParam:param
                                               MeasureContext:ctx];
      [_self updateLineHeightWithMaxLineAscender:_self.maxTruncationLineAscender
                                maxLineDescender:_self.maxTruncationLineDescender];
      [_self calculateLineSpaceForAttributedStr:attributedStr];
    };
    [self.textRenderer ensureTextRenderLayout];
  } else {
    self.textRenderer = [[LynxTextRendererCache cache] rendererWithString:[self.attrString copy]
                                                               layoutSpec:spec];
  }

  self.textRenderer.selectionColor = self.textStyle.selectionColor;
  CGSize size = self.textRenderer.size;
  CGFloat letterSpacing = self.textStyle.letterSpacing;
  if (!isnan(letterSpacing) && letterSpacing < 0) {
    size.width -= letterSpacing;
  }

  MeasureResult result;
  result.size = size;
  result.baseline = self.textRenderer.baseline;
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)

  return result;
}

- (void)calculateLineAscenderAndDescenderForAttributeStr:(NSMutableAttributedString *)attrString
                                            MeasureParam:(MeasureParam *)param
                                          MeasureContext:(MeasureContext *)ctx {
  __block float maxLineAscender = 0, maxLineDescender = 0;
  // layout native node.
  if (ctx != nil) {
    [attrString
        enumerateAttribute:LynxInlineViewAttributedStringKey
                   inRange:NSMakeRange(0, attrString.length)
                   options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                usingBlock:^(LynxShadowNode *node, NSRange range, BOOL *_Nonnull stop) {
                  if (node == nil || ![node isKindOfClass:[LynxNativeLayoutNode class]]) {
                    return;
                  }

                  LynxNativeLayoutNode *child = (LynxNativeLayoutNode *)node;
                  MeasureParam *nParam = [[MeasureParam alloc] initWithWidth:param.width
                                                                   WidthMode:param.widthMode
                                                                      Height:param.height
                                                                  HeightMode:param.heightMode];
                  MeasureResult result = [child measureWithMeasureParam:nParam MeasureContext:ctx];

                  NSTextAttachment *attachment = [attrString attribute:NSAttachmentAttributeName
                                                               atIndex:range.location
                                                        effectiveRange:nil];
                  CGFloat baselineOffset = 0.f;

                  if (self.enableTextRefactor) {
                    if (node.shadowNodeStyle.valign == LynxVerticalAlignBaseline) {
                      baselineOffset = result.baseline - result.size.height;
                    } else {
                      baselineOffset =
                          [self calcBaselineShiftOffset:node.shadowNodeStyle.valign
                                     verticalAlignValue:node.shadowNodeStyle.valignLength
                                           withAscender:result.size.height
                                          withDescender:0.f];
                    }
                    maxLineAscender = MAX(maxLineAscender, baselineOffset + result.size.height);
                    maxLineDescender = MIN(maxLineDescender, baselineOffset);
                  } else {
                    baselineOffset = attachment.bounds.origin.y;
                  }
                  [attachment setBounds:CGRectMake(attachment.bounds.origin.x, baselineOffset,
                                                   result.size.width, result.size.height)];
                }];
    [self calculateTruncationLineAscenderAndDescenderMeasureParam:param MeasureContext:ctx];
  }
  [self updateLineHeightWithMaxLineAscender:maxLineAscender maxLineDescender:maxLineDescender];
}

- (void)calculateTruncationLineAscenderAndDescenderMeasureParam:(MeasureParam *)param
                                                 MeasureContext:(MeasureContext *)ctx {
  if (ctx) {
    if (self.textStyle.truncationAttributedStr.length > 0) {
      [self.textStyle.truncationAttributedStr
          enumerateAttribute:LynxInlineViewAttributedStringKey
                     inRange:NSMakeRange(0, self.textStyle.truncationAttributedStr.length)
                     options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                  usingBlock:^(LynxShadowNode *node, NSRange range, BOOL *_Nonnull stop) {
                    if (node == nil || ![node isKindOfClass:[LynxNativeLayoutNode class]]) {
                      return;
                    }
                    LynxNativeLayoutNode *child = (LynxNativeLayoutNode *)node;
                    MeasureResult result = [child measureWithMeasureParam:param MeasureContext:ctx];

                    NSTextAttachment *attachment =
                        [self.textStyle.truncationAttributedStr attribute:NSAttachmentAttributeName
                                                                  atIndex:range.location
                                                           effectiveRange:nil];
                    CGFloat baselineOffset = 0.f;

                    if (self.enableTextRefactor) {
                      if (node.shadowNodeStyle.valign == LynxVerticalAlignBaseline) {
                        baselineOffset = result.baseline - result.size.height;
                      } else {
                        baselineOffset =
                            [self calcBaselineShiftOffset:node.shadowNodeStyle.valign
                                       verticalAlignValue:node.shadowNodeStyle.valignLength
                                             withAscender:result.size.height
                                            withDescender:0.f];
                      }
                      self.maxTruncationLineAscender =
                          MAX(self.maxLineAscender, baselineOffset + result.size.height);
                      self.maxTruncationLineDescender = MIN(self.maxLineDescender, baselineOffset);
                    } else {
                      baselineOffset = attachment.bounds.origin.y;
                    }
                    [attachment setBounds:CGRectMake(attachment.bounds.origin.x, baselineOffset,
                                                     result.size.width, result.size.height)];
                  }];
    }
  }
}

- (void)updateLineHeightWithMaxLineAscender:(float)maxLineAscender
                           maxLineDescender:(float)maxLineDescender {
  _maxLineAscender = MAX(_maxLineAscender, maxLineAscender);
  _maxLineDescender = MIN(_maxLineDescender, maxLineDescender);
  self.lineSpacingAdaptation.maxLineAscender = _maxLineAscender;
  self.lineSpacingAdaptation.maxLineDescender = _maxLineDescender;
  if (!isnan(self.textStyle.lineHeight)) {
    self.lineSpacingAdaptation.lineHeight = self.textStyle.lineHeight;
    self.lineSpacingAdaptation.halfLeading =
        (self.textStyle.lineHeight - _maxLineAscender + _maxLineDescender) * 0.5f;
  } else {
    self.lineSpacingAdaptation.lineHeight = self.lineSpacingAdaptation.halfLeading = 0.f;
  }
}

- (BOOL)needsEventSet {
  return YES;
}

/*
 {
   lineCount: number;    //line count of display text
   lines: LineInfo[];    //contain line layout info
   size: {width: number, height: number}; //text layout rect
 }
 class LineInfo {
   start: number;        //the line start offset for text
   end: number;          //the line end offset for text
   ellipsisCount: number;//ellipsis count of the line. If larger than 0, truncate text in this line.
 }
 */
- (void)dispatchLayoutEvent {
  if ([self.eventSet objectForKey:@"layout"] == nil || self.textRenderer.layoutManager == nil) {
    return;
  }
  NSLayoutManager *layoutManager = self.textRenderer.layoutManager;
  NSMutableDictionary *layoutInfo = [NSMutableDictionary new];
  NSMutableArray *lineInfo = [NSMutableArray new];

  __block NSInteger lineCount = 0;
  [layoutManager enumerateLineFragmentsForGlyphRange:NSMakeRange(0, self.attrString.length)
                                          usingBlock:^(CGRect rect, CGRect usedRect,
                                                       NSTextContainer *_Nonnull textContainer,
                                                       NSRange glyphRange, BOOL *_Nonnull stop) {
                                            lineCount++;
                                          }];
  NSInteger actualLineCount = lineCount;
  if (self.maxLineNum != LynxNumberNotSet && self.maxLineNum < lineCount) {
    actualLineCount = self.maxLineNum;
  }

  __block NSInteger index = 0;
  [layoutManager
      enumerateLineFragmentsForGlyphRange:NSMakeRange(0, self.attrString.length)
                               usingBlock:^(CGRect rect, CGRect usedRect,
                                            NSTextContainer *_Nonnull textContainer,
                                            NSRange glyphRange, BOOL *_Nonnull stop) {
                                 NSRange characterRange =
                                     [layoutManager characterRangeForGlyphRange:glyphRange
                                                               actualGlyphRange:nil];
                                 NSMutableDictionary *info = [NSMutableDictionary new];
                                 [info setObject:@(characterRange.location) forKey:@"start"];
                                 NSInteger end = characterRange.location + characterRange.length;
                                 NSInteger ellipsisCount = 0;
                                 if (index == lineCount - 1) {
                                   if (self.textRenderer.ellipsisCount != 0) {
                                     ellipsisCount = self.textRenderer.ellipsisCount;
                                     end = self.attrString.length;
                                   } else {
                                     NSRange truncatedRange = [layoutManager
                                         truncatedGlyphRangeInLineFragmentForGlyphAtIndex:
                                             glyphRange.location];
                                     ellipsisCount =
                                         [layoutManager characterRangeForGlyphRange:truncatedRange
                                                                   actualGlyphRange:nil]
                                             .length;
                                   }
                                 }

                                 if (index == actualLineCount - 1 && actualLineCount < lineCount) {
                                   ellipsisCount = self.attrString.length -
                                                   characterRange.location - characterRange.length;
                                   end = self.attrString.length;
                                   *stop = YES;
                                 }
                                 [info setObject:@(end) forKey:@"end"];
                                 [info setObject:@(ellipsisCount) forKey:@"ellipsisCount"];
                                 [lineInfo addObject:info];

                                 index++;
                               }];

  [layoutInfo setObject:@(actualLineCount) forKey:@"lineCount"];
  [layoutInfo setObject:lineInfo forKey:@"lines"];

  CGSize layoutSize = [self.textRenderer size];
  NSMutableDictionary *rect = [[NSMutableDictionary alloc] init];
  [rect setObject:@(layoutSize.width) forKey:@"width"];
  [rect setObject:@(layoutSize.height) forKey:@"height"];
  [layoutInfo setObject:rect forKey:@"size"];

  LynxDetailEvent *event = [[LynxDetailEvent alloc] initWithName:@"layout"
                                                      targetSign:[self sign]
                                                          detail:layoutInfo];
  dispatch_async(dispatch_get_main_queue(), ^{
    [self.uiOwner.uiContext.eventEmitter dispatchCustomEvent:event];
  });
}

- (void)alignOneLine:(NSRange)characterRange
            lineRect:(CGRect)rect
       layoutManager:(NSLayoutManager *)layoutManager
       textContainer:(NSTextContainer *)textContainer
        AlignContext:(AlignContext *)ctx {
  NSTextStorage *textStorage = self.textRenderer.textStorage;
  if (characterRange.location + characterRange.length > textStorage.length) {
    // Here the layout manager append ellipsis to text storage string.
    // Ignore those appended string when aligning inline view.
    if (textStorage.length >= characterRange.location) {
      return;
    }
    characterRange.length = textStorage.length - characterRange.location;
  }

  [textStorage
      enumerateAttribute:LynxInlineViewAttributedStringKey
                 inRange:characterRange
                 options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
              usingBlock:^(LynxShadowNode *value, NSRange range, BOOL *_Nonnull stop) {
                if (value == nil || ![value isKindOfClass:[LynxNativeLayoutNode class]]) {
                  return;
                }

                NSTextAttachment *attachment = [textStorage attribute:NSAttachmentAttributeName
                                                              atIndex:range.location
                                                       effectiveRange:nil];
                if (attachment) {
                  CGFloat yOffsetToTop = 0;
                  NSRange glyphRange = [layoutManager glyphRangeForCharacterRange:range
                                                             actualCharacterRange:nil];
                  CGRect glyphRect = [layoutManager boundingRectForGlyphRange:glyphRange
                                                              inTextContainer:textContainer];

                  LynxNativeLayoutNode *child = (LynxNativeLayoutNode *)value;
                  CGFloat yPosition = [layoutManager locationForGlyphAtIndex:glyphRange.location].y;
                  yOffsetToTop = [self alignInlineNodeInVertical:child.shadowNodeStyle.valign
                                                  withLineHeight:rect.size.height
                                            withAttachmentHeight:attachment.bounds.size.height
                                         withAttachmentYPosition:yPosition];

                  AlignParam *alignParam = [[AlignParam alloc] init];
                  CGFloat leftOffset = glyphRect.origin.x + attachment.bounds.origin.x +
                                       self.textRenderer.textContentOffsetX;
                  CGFloat topOffset = self.enableTextRefactor
                                          ? rect.origin.y + yOffsetToTop
                                          : rect.origin.y + attachment.bounds.origin.y;
                  [alignParam SetAlignOffsetWithLeft:leftOffset Top:topOffset];

                  [child alignWithAlignParam:alignParam AlignContext:ctx];
                }
              }];
  if (_isCalcVerticalAlignValue) {
    [textStorage
        enumerateAttribute:LynxInlineTextShadowNodeSignKey
                   inRange:characterRange
                   options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                usingBlock:^(LynxShadowNode *value, NSRange range, BOOL *_Nonnull stop) {
                  if (value == nil) {
                    return;
                  }
                  CGFloat baselinePosition = 0.f;
                  // ascender,descender
                  NSArray *fontMetric = [textStorage attribute:LynxUsedFontMetricKey
                                                       atIndex:range.location
                                                effectiveRange:nil];
                  bool isResetLocation = true;
                  switch (value.shadowNodeStyle.valign) {
                    case LynxVerticalAlignCenter:
                      baselinePosition =
                          (rect.size.height - ([[fontMetric objectAtIndex:0] floatValue] -
                                               [[fontMetric objectAtIndex:1] floatValue])) *
                              0.5f +
                          [[fontMetric objectAtIndex:0] floatValue];
                      break;
                    case LynxVerticalAlignBottom:
                      baselinePosition =
                          rect.size.height + [[fontMetric objectAtIndex:1] floatValue];
                      break;
                    case LynxVerticalAlignTop:
                      baselinePosition = [[fontMetric objectAtIndex:0] floatValue];
                      break;
                    default:
                      isResetLocation = false;
                      break;
                  }
                  if (isResetLocation) {
                    NSRange glyphRange = [layoutManager glyphRangeForCharacterRange:range
                                                               actualCharacterRange:nil];
                    CGPoint point = {[layoutManager locationForGlyphAtIndex:glyphRange.location].x,
                                     baselinePosition};
                    [layoutManager setLocation:point forStartOfGlyphRange:glyphRange];
                  }
                }];
  }
}

- (void)alignWithAlignParam:(AlignParam *)param AlignContext:(AlignContext *)ctx {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, @"LynxTextShadowNode.align");

  NSTextStorage *textStorage = self.textRenderer.textStorage;
  NSLayoutManager *layoutManager = textStorage.layoutManagers.firstObject;

  [layoutManager enumerateLineFragmentsForGlyphRange:(NSRange){0, textStorage.length}
                                          usingBlock:^(CGRect rect, CGRect usedRect,
                                                       NSTextContainer *_Nonnull textContainer,
                                                       NSRange glyphRange, BOOL *_Nonnull stop) {
                                            NSRange character_range = [layoutManager
                                                characterRangeForGlyphRange:glyphRange
                                                           actualGlyphRange:nil];
                                            [self alignOneLine:character_range
                                                      lineRect:rect
                                                 layoutManager:layoutManager
                                                 textContainer:textContainer
                                                  AlignContext:ctx];
                                          }];
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
}

- (CGSize)measureNode:(LynxLayoutNode *)node
            withWidth:(CGFloat)width
            widthMode:(LynxMeasureMode)widthMode
               height:(CGFloat)height
           heightMode:(LynxMeasureMode)heightMode {
  MeasureParam *param = [[MeasureParam alloc] initWithWidth:width
                                                  WidthMode:widthMode
                                                     Height:height
                                                 HeightMode:heightMode];
  return [self measureWithMeasureParam:param MeasureContext:NULL].size;
}

- (void)determineLineSpacing:(NSMutableAttributedString *)attributedString {
  __block CGFloat calculatedLineSpacing = 0;
  [attributedString enumerateAttribute:NSParagraphStyleAttributeName
                               inRange:NSMakeRange(0, attributedString.length)
                               options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                            usingBlock:^(NSParagraphStyle *paragraphStyle, __unused NSRange range,
                                         __unused BOOL *stop) {
                              if (paragraphStyle) {
                                calculatedLineSpacing =
                                    MAX(paragraphStyle.lineSpacing, calculatedLineSpacing);
                              }
                            }];
  _lineSpacingAdaptation.calculatedLineSpacing = calculatedLineSpacing;
}

- (void)modifyLineHeightForStorage:(NSMutableAttributedString *)storage {
  if (storage.length == 0) {
    return;
  }
  __block CGFloat minimumLineHeight = 0;
  __block CGFloat maximumLineHeight = 0;

  // Check max line-height
  [storage enumerateAttribute:NSParagraphStyleAttributeName
                      inRange:NSMakeRange(0, storage.length)
                      options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                   usingBlock:^(NSParagraphStyle *paragraphStyle, __unused NSRange range,
                                __unused BOOL *stop) {
                     if (paragraphStyle) {
                       minimumLineHeight = MAX(paragraphStyle.minimumLineHeight, minimumLineHeight);
                       maximumLineHeight = MAX(paragraphStyle.maximumLineHeight, maximumLineHeight);
                     }
                   }];

  //  I don't think the process below is necessary
  if (minimumLineHeight == 0 && maximumLineHeight == 0) {
    __block CGFloat lineHeight = 0;

    [storage enumerateAttribute:NSFontAttributeName
                        inRange:NSMakeRange(0, storage.length)
                        options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                     usingBlock:^(UIFont *font, NSRange range, __unused BOOL *stop) {
                       if (font) {
                         lineHeight = MAX(lineHeight, font.lineHeight);
                       }
                     }];
    minimumLineHeight = lineHeight;
    maximumLineHeight = lineHeight;
  }

  if (minimumLineHeight == 0 && maximumLineHeight == 0) {
    return;
  }

  if ([storage attribute:NSParagraphStyleAttributeName atIndex:0 effectiveRange:nil] == nil) {
    NSMutableParagraphStyle *newStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
    newStyle.minimumLineHeight = minimumLineHeight;
    newStyle.maximumLineHeight = maximumLineHeight;
    [storage addAttribute:NSParagraphStyleAttributeName value:newStyle range:NSMakeRange(0, 1)];
  }

  [storage enumerateAttribute:NSParagraphStyleAttributeName
                      inRange:NSMakeRange(0, storage.length)
                      options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                   usingBlock:^(NSParagraphStyle *paragraphStyle, __unused NSRange range,
                                __unused BOOL *stop) {
                     if (paragraphStyle) {
                       NSMutableParagraphStyle *style = [paragraphStyle mutableCopy];
                       style.minimumLineHeight = minimumLineHeight;
                       style.maximumLineHeight = maximumLineHeight;
                       [storage addAttribute:NSParagraphStyleAttributeName value:style range:range];
                     }
                   }];
}

/**
 * Vertical align center in line
 */
- (void)addVerticalAlignCenterInline:(NSMutableAttributedString *)attributedString {
  __block CGFloat maxiumCapheight = 0;

  [attributedString enumerateAttribute:NSFontAttributeName
                               inRange:NSMakeRange(0, attributedString.length)
                               options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                            usingBlock:^(UIFont *font, NSRange range, __unused BOOL *stop) {
                              if (font) {
                                maxiumCapheight = MAX(maxiumCapheight, font.capHeight);
                              }
                            }];

  __block CGFloat maximumLineHeight = 0;

  // Check max line-height
  [attributedString enumerateAttribute:NSParagraphStyleAttributeName
                               inRange:NSMakeRange(0, attributedString.length)
                               options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                            usingBlock:^(NSParagraphStyle *paragraphStyle, __unused NSRange range,
                                         __unused BOOL *stop) {
                              if (paragraphStyle) {
                                maximumLineHeight =
                                    MAX(paragraphStyle.maximumLineHeight, maximumLineHeight);
                              }
                            }];

  if (maximumLineHeight == 0) {
    return;
  }

  if (self.textVerticalAlign != LynxVerticalAlignDefault) {
    // baseLine offset will calculate during LayoutManager draw
    return;
  }
  _lineSpacingAdaptation.adjustBaseLineOffsetForVerticalAlignCenter = YES;

  CGFloat baseLineOffset = 0;
  baseLineOffset = maximumLineHeight * 1 / 2 - maxiumCapheight * 1 / 2;
  [attributedString addAttribute:NSBaselineOffsetAttributeName
                           value:@(baseLineOffset)
                           range:NSMakeRange(0, attributedString.length)];
  _lineSpacingAdaptation.baseLineOffsetForVerticalAlignCenter = baseLineOffset;
}

- (void)layoutDidStart {
  if (self.textIndent != nil) {
    self.textStyle.textIndent = [self.textIndent applyValue:[self.style computedWidth]];
  } else {
    self.textStyle.textIndent = 0;
  }
  [super layoutDidStart];
  [self.children enumerateObjectsUsingBlock:^(LynxShadowNode *_Nonnull obj, NSUInteger idx,
                                              BOOL *_Nonnull stop) {
    // if child is not virtual, maybe this is a text insert by slot
    if (![obj isVirtual]) {
      [obj layoutDidStart];
    }
  }];
  self.lineSpacingAdaptation.enableLayoutRefactor = self.enableTextRefactor;
  NSMutableAttributedString *attrString;
  if (self.enableTextRefactor) {
    attrString = [[NSTextStorage alloc]
        initWithAttributedString:[self generateAttributedString:nil
                                              withTextMaxLength:self.maxTextLength
                                                  withDirection:self.textStyle.direction]];
  } else {
    attrString = [[self generateAttributedString:nil
                               withTextMaxLength:self.maxTextLength
                                   withDirection:self.textStyle.direction] mutableCopy];
  }
  BOOL enableTextLanguageAlignment =
      self.textStyle.enableLanguageAlignment ?: self.uiOwner.uiContext.enableTextLanguageAlignment;
  NSTextAlignment inferredAlignment =
      [LynxTextUtils applyNaturalAlignmentAccordingToTextLanguage:attrString
                                                         refactor:enableTextLanguageAlignment];
  if (inferredAlignment != NSTextAlignmentNatural) {
    self.textStyle.usedParagraphTextAlignment = inferredAlignment;
  }
  [self calculateLineSpaceForAttributedStr:attrString];
  self.attrString = attrString;
  _lineSpacingAdaptation.attributedString = attrString;
  self.textRenderer = nil;
}

- (void)calculateLineSpaceForAttributedStr:(NSMutableAttributedString *)attrString {
  [self determineLineSpacing:attrString];
  if (!self.enableTextRefactor) {
    [self modifyLineHeightForStorage:attrString];
    [self addVerticalAlignCenterInline:attrString];
  }
  if (self.enableTextRefactor) {
    [self setVerticalAlign:attrString];
  }
}

- (NSAttributedString *)generateAttributedString:
                            (NSDictionary<NSAttributedStringKey, id> *)baseTextAttribute
                               withTextMaxLength:(NSInteger)textMaxLength
                                   withDirection:(NSWritingDirection)direction {
  NSAttributedString *attrStr = [super generateAttributedString:baseTextAttribute
                                              withTextMaxLength:textMaxLength
                                                  withDirection:direction];

  if (!self.enableTextRefactor) {
    return attrStr;
  }

  // if TextRefactor is enabled, inline-text's line-height is ignored. And may contains the
  // following bad case cause line-height not working:
  //      <text style="line-height: xxx"> <text>aaa</text> bbb </text>
  // So set ParagraphStyle at root text node to make sure it is working
  NSMutableAttributedString *mutableStr = [attrStr mutableCopy];
  NSParagraphStyle *paragraphStyle = [self.textStyle genParagraphStyle];
  [mutableStr addAttribute:NSParagraphStyleAttributeName
                     value:paragraphStyle
                     range:NSMakeRange(0, mutableStr.length)];

  return mutableStr;
}

- (void)setVerticalAlign:(NSMutableAttributedString *)attributedString {
  __block CGFloat maxAscender = 0, maxDescender = 0, maxXHeight = 0;
  [attributedString enumerateAttribute:NSFontAttributeName
                               inRange:NSMakeRange(0, attributedString.length)
                               options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                            usingBlock:^(UIFont *font, NSRange range, BOOL *_Nonnull stop) {
                              if (font) {
                                maxAscender = MAX(maxAscender, font.ascender);
                                maxDescender = MIN(maxDescender, font.descender);
                                maxXHeight = MAX(maxXHeight, font.xHeight);
                              }
                            }];
  _maxLineAscender = _maxAscender = maxAscender;
  _maxLineDescender = _maxDescender = maxDescender;
  _maxXHeight = maxXHeight;

  __block BOOL isCalcVerticalAlignValue = false;
  [attributedString enumerateAttribute:LynxVerticalAlignKey
                               inRange:NSMakeRange(0, attributedString.length)
                               options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                            usingBlock:^(NSNumber *value, NSRange range, BOOL *_Nonnull stop) {
                              isCalcVerticalAlignValue =
                                  isCalcVerticalAlignValue ||
                                  ([value integerValue] != LynxVerticalAlignDefault);
                            }];
  _isCalcVerticalAlignValue = isCalcVerticalAlignValue;
  if (!_isCalcVerticalAlignValue && isnan(self.textStyle.lineHeight)) {
    return;
  }

  __block float maxLineAscender = _maxLineAscender, maxLineDescender = _maxLineDescender;
  [attributedString
      enumerateAttribute:LynxInlineTextShadowNodeSignKey
                 inRange:NSMakeRange(0, attributedString.length)
                 options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
              usingBlock:^(LynxShadowNode *node, NSRange range, BOOL *_Nonnull stop) {
                if (!node) {
                  return;
                }
                if ([node isKindOfClass:[LynxBaseTextShadowNode class]]) {
                  LynxBaseTextShadowNode *textNode = (LynxBaseTextShadowNode *)node;
                  if (textNode.shadowNodeStyle.valign != LynxVerticalAlignDefault) {
                    __block CGFloat fontAscent = 0.f, fontDescent = 0.f;

                    [attributedString
                        enumerateAttribute:NSFontAttributeName
                                   inRange:range
                                   options:
                                       NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                                usingBlock:^(UIFont *font, NSRange fontRange, __unused BOOL *stop) {
                                  if (font) {
                                    fontAscent = MAX(fontAscent, font.ascender);
                                    fontDescent = MIN(fontDescent, font.descender);
                                  }
                                }];
                    [attributedString
                        addAttribute:LynxUsedFontMetricKey
                               value:[NSArray
                                         arrayWithObjects:[NSNumber numberWithFloat:fontAscent],
                                                          [NSNumber numberWithFloat:fontDescent],
                                                          nil]
                               range:range];
                    CGFloat baselineOffset =
                        [self calcBaselineShiftOffset:textNode.shadowNodeStyle.valign
                                   verticalAlignValue:textNode.shadowNodeStyle.valignLength
                                         withAscender:fontAscent
                                        withDescender:fontDescent];
                    [attributedString addAttribute:NSBaselineOffsetAttributeName
                                             value:@(baselineOffset)
                                             range:range];
                    maxLineAscender = MAX(maxLineAscender, fontAscent + baselineOffset);
                    maxLineDescender = MIN(maxLineDescender, fontDescent + baselineOffset);
                  }
                }
              }];
  [attributedString
      enumerateAttribute:LynxInlineViewAttributedStringKey
                 inRange:NSMakeRange(0, attributedString.length)
                 options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
              usingBlock:^(LynxShadowNode *node, NSRange range, BOOL *_Nonnull stop) {
                if (!node || [node isKindOfClass:[LynxNativeLayoutNode class]]) {
                  return;
                }

                LynxBaseTextShadowNode *imageNode = (LynxBaseTextShadowNode *)node;
                NSTextAttachment *attachment = [attributedString attribute:NSAttachmentAttributeName
                                                                   atIndex:range.location
                                                            effectiveRange:nil];
                if (imageNode.shadowNodeStyle.valign != LynxVerticalAlignDefault) {
                  CGFloat baselineOffset =
                      [self calcBaselineShiftOffset:imageNode.shadowNodeStyle.valign
                                 verticalAlignValue:imageNode.shadowNodeStyle.valignLength
                                       withAscender:attachment.bounds.size.height
                                      withDescender:0.f];

                  CGRect rect = attachment.bounds;
                  rect.origin.y = baselineOffset;
                  [attachment setBounds:rect];
                }
                maxLineAscender = MAX(maxLineAscender,
                                      attachment.bounds.size.height + attachment.bounds.origin.y);
                maxLineDescender = MIN(maxLineDescender, attachment.bounds.origin.y);
              }];
  _maxLineAscender = MAX(_maxLineAscender, maxLineAscender);
  _maxLineDescender = MIN(_maxLineDescender, maxLineDescender);
}

- (CGFloat)calcBaselineShiftOffset:(LynxVerticalAlign)verticalAlign
                verticalAlignValue:(CGFloat)verticalAlignValue
                      withAscender:(CGFloat)ascender
                     withDescender:(CGFloat)descender {
  switch (verticalAlign) {
    case LynxVerticalAlignLength:
      return verticalAlignValue;
    case LynxVerticalAlignPercent:
      // if set vertical-align:50%, baselineShift = 50 * lineHeight /100.f, the lineHeight is 0 if
      // lineHeight not set.
      return _lineSpacingAdaptation.lineHeight * verticalAlignValue / 100.f;
    case LynxVerticalAlignMiddle:
      // the middle of element will be align to the middle of max x-height
      return (-descender - ascender + _maxXHeight) * 0.5f;
    case LynxVerticalAlignTextTop:
    case LynxVerticalAlignTop:
      // the ascender of element will be align to text max ascender
      return _maxAscender - ascender;
    case LynxVerticalAlignTextBottom:
    case LynxVerticalAlignBottom:
      // the descender of element will be align to text max descender
      return _maxDescender - descender;
    case LynxVerticalAlignSub:
      //-height * 0.1
      return -(ascender - descender) * 0.1f;
    case LynxVerticalAlignSuper:
      // height * 0.1
      return (ascender - descender) * 0.1f;
    case LynxVerticalAlignCenter:
      // the middle of element will be align to the middle of line
      return (_maxAscender + _maxDescender - ascender - descender) * 0.5f;
    default:
      // baseline,center,top,bottom
      return 0.f;
  }
}

- (CGFloat)alignInlineNodeInVertical:(LynxVerticalAlign)verticalAlign
                      withLineHeight:(CGFloat)lineFragmentHeight
                withAttachmentHeight:(CGFloat)attachmentHeight
             withAttachmentYPosition:(CGFloat)attachmentYPosition {
  CGFloat yOffsetToTop = 0;
  switch (verticalAlign) {
    case LynxVerticalAlignBottom:
      yOffsetToTop = lineFragmentHeight - attachmentHeight;
      break;
    case LynxVerticalAlignTop:
      yOffsetToTop = 0;
      break;
    case LynxVerticalAlignCenter:
      yOffsetToTop = (lineFragmentHeight - attachmentHeight) * 0.5f;
      break;
    default:
      yOffsetToTop = attachmentYPosition - attachmentHeight;
      break;
  }
  return yOffsetToTop;
}

- (void)layoutDidUpdate {
  [super layoutDidUpdate];
  if (self.textRenderer == nil) {
    [self measureNode:self
            withWidth:self.frame.size.width
            widthMode:LynxMeasureModeDefinite
               height:self.frame.size.height
           heightMode:LynxMeasureModeDefinite];
  }
  // As TextShadowNode has custom layout, we have to handle children layout
  // after layout updated.
  [self updateNonVirtualOffspringLayout];
}

- (id)getExtraBundle {
  [self dispatchLayoutEvent];
  return self.textRenderer;
}

/**
 * Update layout info for those non-virtual shadow node which will not layout
 * by native layout system.
 */
- (void)updateNonVirtualOffspringLayout {
  if (!self.hasNonVirtualOffspring) {
    return;
  }
  NSTextStorage *textStorage = self.textRenderer.textStorage;
  NSLayoutManager *layoutManager = textStorage.layoutManagers.firstObject;
  NSTextContainer *textContainer = layoutManager.textContainers.firstObject;
  NSRange glyphRange = [layoutManager glyphRangeForTextContainer:textContainer];
  NSRange characterRange = [layoutManager characterRangeForGlyphRange:glyphRange
                                                     actualGlyphRange:nil];

  NSMutableArray *attachments = [NSMutableArray new];
  NSMutableSet *nodeSignSet = [NSMutableSet new];

  // Update child node layout info
  [textStorage
      enumerateAttribute:LynxInlineViewAttributedStringKey
                 inRange:characterRange
                 options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
              usingBlock:^(LynxShadowNode *node, NSRange range, BOOL *stop) {
                if (!node) {
                  return;
                }
                [nodeSignSet addObject:@(node.sign)];
                if ([node isKindOfClass:[LynxNativeLayoutNode class]]) {
                  // native inline view
                  NSRange inlineViewGlyphRange = [layoutManager glyphRangeForCharacterRange:range
                                                                       actualCharacterRange:nil];
                  NSRange truncatedGlyphRange = [layoutManager
                      truncatedGlyphRangeInLineFragmentForGlyphAtIndex:inlineViewGlyphRange
                                                                           .location];
                  LynxTextAttachmentInfo *inlineViewAttachment = nil;
                  if (truncatedGlyphRange.location != NSNotFound &&
                      truncatedGlyphRange.location <= inlineViewGlyphRange.location) {
                    inlineViewAttachment = [[LynxTextAttachmentInfo alloc] initWithSign:node.sign
                                                                               andFrame:CGRectZero];
                  } else {
                    NSTextAttachment *attachment = [textStorage attribute:NSAttachmentAttributeName
                                                                  atIndex:range.location
                                                           effectiveRange:nil];

                    inlineViewAttachment =
                        [[LynxTextAttachmentInfo alloc] initWithSign:node.sign
                                                            andFrame:attachment.bounds];
                  }

                  inlineViewAttachment.nativeAttachment = YES;

                  [attachments addObject:inlineViewAttachment];

                  return;
                }

                // Get current line rect
                NSRange lineRange = NSMakeRange(0, 0);
                NSRange inlineImageGlyphRange = [layoutManager glyphRangeForCharacterRange:range
                                                                      actualCharacterRange:nil];
                CGRect lineFragment =
                    [layoutManager lineFragmentRectForGlyphAtIndex:inlineImageGlyphRange.location
                                                    effectiveRange:&lineRange];
                CGRect glyphRect = [layoutManager boundingRectForGlyphRange:inlineImageGlyphRange
                                                            inTextContainer:textContainer];

                // Get attachment size
                NSTextAttachment *attachment = [textStorage attribute:NSAttachmentAttributeName
                                                              atIndex:range.location
                                                       effectiveRange:nil];
                CGSize attachmentSize = attachment.bounds.size;
                CGFloat yOffsetToTop = 0;
                CGFloat yPosition =
                    [layoutManager locationForGlyphAtIndex:inlineImageGlyphRange.location].y;
                const LynxVerticalAlign vAlign =
                    ([node shadowNodeStyle] != nil ? [node shadowNodeStyle].valign
                                                   : LynxVerticalAlignDefault);

                if (self.enableTextRefactor) {
                  yOffsetToTop = [self alignInlineNodeInVertical:vAlign
                                                  withLineHeight:lineFragment.size.height
                                            withAttachmentHeight:attachment.bounds.size.height
                                         withAttachmentYPosition:yPosition];
                } else {
                  switch (vAlign) {
                    case LynxVerticalAlignBottom:
                      yOffsetToTop = lineFragment.size.height - attachmentSize.height;
                      break;
                    case LynxVerticalAlignTop:
                      yOffsetToTop = 0;
                      break;
                    case LynxVerticalAlignMiddle:
                    case LynxVerticalAlignDefault:
                      yOffsetToTop = (lineFragment.size.height - attachmentSize.height) * 0.5f;
                      break;
                    default:
                      yOffsetToTop = yPosition - attachmentSize.height;
                      break;
                  }
                }

                // Determin final rect, make attachment center in line
                CGRect frame = {
                    {glyphRect.origin.x + node.style.computedMarginLeft +
                         self.textRenderer.textContentOffsetX,
                     lineFragment.origin.y + node.style.computedMarginTop + yOffsetToTop},
                    {attachmentSize.width - node.style.computedMarginLeft -
                         node.style.computedMarginRight,
                     attachmentSize.height - node.style.computedMarginTop -
                         node.style.computedMarginBottom}};

                NSRange truncatedGlyphRange = [layoutManager
                    truncatedGlyphRangeInLineFragmentForGlyphAtIndex:inlineImageGlyphRange
                                                                         .location];
                if (truncatedGlyphRange.location != NSNotFound &&
                    truncatedGlyphRange.location <= inlineImageGlyphRange.location) {
                  // truncated happen before this inline-image;
                  // no need to show all remined inline-image
                  [node updateLayoutWithFrame:CGRectZero];
                  [attachments addObject:[[LynxTextAttachmentInfo alloc] initWithSign:node.sign
                                                                             andFrame:CGRectZero]];
                } else {
                  [node updateLayoutWithFrame:frame];
                  [attachments addObject:[[LynxTextAttachmentInfo alloc] initWithSign:node.sign
                                                                             andFrame:frame]];
                }
              }];
  [self updateHiddenAttachmentFrame:_attrString signSet:nodeSignSet attachments:attachments];
  [self updateHiddenAttachmentFrame:self.textStyle.truncationAttributedStr
                            signSet:nodeSignSet
                        attachments:attachments];

  self.textRenderer.attachments = attachments;
}

- (void)updateHiddenAttachmentFrame:(NSAttributedString *)attributeString
                            signSet:(NSSet *)nodeSignSet
                        attachments:(NSMutableArray *)attachments {
  if (!attributeString || attributeString.length == 0) {
    return;
  }
  [attributeString enumerateAttribute:LynxInlineViewAttributedStringKey
                              inRange:NSMakeRange(0, attributeString.length)
                              options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                           usingBlock:^(LynxShadowNode *node, NSRange range, BOOL *stop) {
                             if (!node) {
                               return;
                             }
                             if (![nodeSignSet containsObject:@(node.sign)]) {
                               [node updateLayoutWithFrame:CGRectZero];
                               LynxTextAttachmentInfo *inlineElementAttachment =
                                   [[LynxTextAttachmentInfo alloc] initWithSign:node.sign
                                                                       andFrame:CGRectZero];
                               if ([node isKindOfClass:[LynxNativeLayoutNode class]]) {
                                 inlineElementAttachment.nativeAttachment = YES;
                               }
                               [attachments addObject:inlineElementAttachment];
                             }
                           }];
}

LYNX_PROP_SETTER("background-color", setBackgroundColor, UIColor *) {
  // Do nothing as background-color will be handle by ui
}

LYNX_PROP_SETTER("text-maxline", setMaxeLine, NSInteger) {
  if (requestReset) {
    value = LynxNumberNotSet;
  }
  if (self.maxLineNum != value) {
    if (value > 0) {
      self.maxLineNum = value;
    } else {
      self.maxLineNum = LynxNumberNotSet;
    }
    [self setNeedsLayout];
  }
}

LYNX_PROP_SETTER("text-maxlength", setTextMaxLength, NSInteger) {
  if (requestReset) {
    value = LynxNumberNotSet;
  }
  if (self.maxTextLength != value) {
    if (value > 0) {
      self.maxTextLength = value;
    } else {
      self.maxTextLength = LynxNumberNotSet;
    }
    [self setNeedsLayout];
  }
}

LYNX_PROP_SETTER("white-space", setWhiteSpace, LynxWhiteSpaceType) {
  if (requestReset) {
    value = LynxWhiteSpaceNormal;
  }
  if (self.whiteSpace != value) {
    self.whiteSpace = value;
    [self setNeedsLayout];
  }
}

LYNX_PROP_SETTER("text-overflow", setTextOverflow, LynxTextOverflowType) {
  if (requestReset) {
    value = LynxTextOverflowClip;
  }
  if (self.textOverflow != value) {
    self.textOverflow = value;
    [self setNeedsLayout];
  }
}

LYNX_PROP_SETTER("overflow-x", setOverflowX, LynxOverflowType) {
  if (requestReset) {
    value = LynxOverflowHidden;
  }
  if (value == LynxOverflowVisible) {
    self.overflow = LynxOverflowX;
  }
}

LYNX_PROP_SETTER("overflow-y", setOverflowY, LynxOverflowType) {
  if (requestReset) {
    value = LynxOverflowHidden;
  }
  if (value == LynxOverflowVisible) {
    self.overflow = LynxOverflowY;
  }
}
LYNX_PROP_SETTER("overflow", setOverflow, LynxOverflowType) {
  if (requestReset) {
    value = LynxOverflowHidden;
  }
  if (value == LynxOverflowVisible) {
    self.overflow = LynxOverflowXY;
  }
}

LYNX_PROP_SETTER("text-vertical-align", setTextVerticalAlign, NSString *) {
  if (requestReset) {
    value = @"center";
  }

  if ([value isEqualToString:@"bottom"]) {
    self.textVerticalAlign = LynxVerticalAlignBottom;
  } else if ([value isEqualToString:@"top"]) {
    self.textVerticalAlign = LynxVerticalAlignTop;
  } else {
    self.textVerticalAlign = LynxVerticalAlignMiddle;
  }
  [self setNeedsLayout];
}

LYNX_PROP_SETTER("text-single-line-vertical-align", setVerticalTextAlign, NSString *) {
  if (requestReset) {
    value = @"normal";
  }

  if ([value isEqualToString:@"center"]) {
    self.textSingleLineVerticalAlign = LynxVerticalAlignCenter;
  } else if ([value isEqualToString:@"top"]) {
    self.textSingleLineVerticalAlign = LynxVerticalAlignTop;
  } else if ([value isEqualToString:@"bottom"]) {
    self.textSingleLineVerticalAlign = LynxVerticalAlignBottom;
  } else {
    self.textSingleLineVerticalAlign = LynxVerticalAlignDefault;
  }
  [self setNeedsLayout];
}

LYNX_PROP_SETTER("tail-color-convert", setEnableTailColorConvert, BOOL) {
  if (requestReset) {
    value = NO;
  }

  self.enableTailColorConvert = value;
  [self setNeedsLayout];
}

LYNX_PROP_SETTER("hyphens", setHyphens, int) {
  if (requestReset) {
    value = LynxHyphensNone;
  }

  self.textStyle.hyphen = (value == LynxHyphensAuto);
  [self setNeedsLayout];
}

LYNX_PROP_SETTER("text-indent", setTextIndent, NSArray *) {
  if (requestReset || value == nil || value.count != 2) {
    self.textIndent = nil;
  } else {
    self.textIndent = [[LynxTextIndent alloc] initWithValue:value];
  }
  [self markStyleDirty];
  [self setNeedsLayout];
}

LYNX_PROP_SETTER("-x-auto-font-size", setXAutoFontSize, NSArray *) {
  if (requestReset || value == nil || value.count != 4) {
    self.textStyle.isAutoFontSize = NO;
  } else {
    self.textStyle.isAutoFontSize = [value[0] boolValue];
    self.textStyle.autoFontSizeMinSize = [value[1] floatValue];
    self.textStyle.autoFontSizeMaxSize = [value[2] floatValue];
    self.textStyle.autoFontSizeStepGranularity = [value[3] floatValue];
  }

  [self markStyleDirty];
  [self setNeedsLayout];
}

LYNX_PROP_SETTER("-x-auto-font-size-preset-sizes", setXAutoFontSizePresetSizes, NSArray *) {
  if (requestReset) {
    self.textStyle.autoFontSizePresetSizes = nil;
  } else {
    self.textStyle.autoFontSizePresetSizes = value;
  }

  [self markStyleDirty];
  [self setNeedsLayout];
}

@end
