// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTextRenderer.h"
#import <CoreText/CoreText.h>
#import "LynxBaseTextShadowNode.h"
#import "LynxBaselineShiftLayoutManager.h"
#import "LynxEnv.h"
#import "LynxLog.h"
#import "LynxTextLayoutManager.h"
#import "LynxTextRendererCache.h"
#import "LynxTextShadowNode.h"
#import "LynxTextUtils.h"
#import "LynxTraceEvent.h"
#import "LynxTraceEventWrapper.h"
#import "base/include/compiler_specific.h"

@implementation LynxTextAttachmentInfo

- (instancetype)initWithSign:(NSInteger)sign andFrame:(CGRect)frame {
  self = [super init];

  if (self) {
    _sign = sign;
    _frame = frame;
    _nativeAttachment = NO;
  }

  return self;
}

@end

@implementation LynxTextRenderer {
  CGSize _calculatedSize;
  CGSize _textSize;
  CGFloat _offsetX;
  CGFloat _maxFontSize;
  NSLayoutManager *_layoutManager;
  NSTextStorage *_textStorage;
  NSTextContainer *_textContainer;
  CGSize _truncatedSize;
}

- (instancetype)initWithAttributedString:(NSAttributedString *)attrStr
                              layoutSpec:(LynxLayoutSpec *)spec {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, @"LynxTextRenderer.init");
  if (self = [super init]) {
    _attrStr = attrStr;
    if (spec.textStyle.truncationAttributedStr.length > 0 && _attrStr.length > 0) {
      NSMutableAttributedString *truncationString =
          [spec.textStyle.truncationAttributedStr mutableCopy];
      [truncationString addAttribute:NSParagraphStyleAttributeName
                               value:[_attrStr attribute:NSParagraphStyleAttributeName
                                                 atIndex:0
                                          effectiveRange:NULL]
                               range:NSMakeRange(0, truncationString.length)];
      _truncationToken = truncationString;
    }
    _layoutSpec = spec;
    if (spec.verticalAlign != LynxVerticalAlignDefault) {
      _layoutManager =
          [[LynxBaselineShiftLayoutManager alloc] initWithVerticalAlign:spec.verticalAlign];
    } else {
      _layoutManager = [[LynxTextLayoutManager alloc] init];
    }
    _layoutManager.usesFontLeading = NO;
    _layoutManager.delegate = spec.layoutManagerDelegate;
    _textStorage = [[NSTextStorage alloc] initWithAttributedString:_attrStr];
    [_textStorage addLayoutManager:_layoutManager];
    _offsetX = 0.f;
    _maxFontSize = 0.f;
  }
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
  return self;
}

- (void)ensureTextRenderLayout {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, @"LynxTextRenderer.layout");

  _ellipsisCount = 0;
  // Adjust the overall line height spec.heightMode when the height of the text component is less
  // than the sum of the line heights.
  CGFloat modified_spec_height = _layoutSpec.height;
  if (!isnan(_layoutSpec.textStyle.lineHeight) &&
      _layoutSpec.heightMode != LynxMeasureModeIndefinite &&
      (_layoutSpec.textOverflow == LynxTextOverflowClip || !_layoutSpec.enableTextRefactor)) {
    // add 0.5 to spec.textStyle.lineHeight to ensure modified_spec_height is high enough for
    // system to draw all lines.
    // add more 0.5 to spec.textStyle.lineHeight for iOS9 to ensure modified_spec_height is high
    // enough for system to draw all lines.
    if (@available(iOS 10.0, *)) {
      modified_spec_height = ceil(_layoutSpec.height / _layoutSpec.textStyle.lineHeight) *
                             ceil(_layoutSpec.textStyle.lineHeight + 0.5);
    } else {
      modified_spec_height = ceil(_layoutSpec.height / _layoutSpec.textStyle.lineHeight) *
                             ceil(_layoutSpec.textStyle.lineHeight + 1);
    }
  }

  CGFloat w = _layoutSpec.widthMode == LynxMeasureModeIndefinite ? CGFLOAT_MAX : _layoutSpec.width;
  CGFloat h =
      _layoutSpec.heightMode == LynxMeasureModeIndefinite ? CGFLOAT_MAX : modified_spec_height;

  // text overflow for Y axis
  if (_layoutSpec.overflow == LynxOverflowY || _layoutSpec.overflow == LynxOverflowXY) {
    h = CGFLOAT_MAX;
  }
  // give no width limit when whiteSpace is LynxWhiteSpaceNowrap
  if (_layoutSpec.whiteSpace == LynxWhiteSpaceNowrap &&
      _layoutSpec.textOverflow != LynxTextOverflowEllipsis) {
    w = CGFLOAT_MAX;
  }
  CGSize inputSize = (CGSize){
      w,
      h,
  };

  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, @"LynxTextRenderer.ensureLayout");
  if (!_textContainer) {
    _textContainer = [self createTextContainerWithSize:inputSize spec:_layoutSpec];
    [_layoutManager addTextContainer:_textContainer];
  }
  _textContainer.size = inputSize;

  if (_layoutSpec.enableTextNonContiguousLayout) {
    _layoutManager.allowsNonContiguousLayout = YES;
    if (_layoutSpec.maxTextLength != LynxNumberNotSet) {
      [_layoutManager ensureLayoutForGlyphRange:NSMakeRange(0, _layoutSpec.maxTextLength)];
    } else {
      if ([LynxEnv.sharedInstance enableTextContainerOpt]) {
        LLogInfo(@"LynxTextRender enableTextContainerOpt textLength:%lu, w:%f, "
                 @"h:%f,maxLineNum:%ld,lineHeight:%f,direction:%ld,fontWeight:%f,fontStyle:%ld,"
                 @"fontFamily:%@,letterSpacing:%f",
                 (unsigned long)(_textStorage.length), w, h, (long)_layoutSpec.maxLineNum,
                 _layoutSpec.textStyle.lineHeight, (long)(_layoutSpec.textStyle.direction),
                 _layoutSpec.textStyle.fontWeight, (long)(_layoutSpec.textStyle.fontStyle),
                 _layoutSpec.textStyle.fontFamilyName, _layoutSpec.textStyle.letterSpacing);
        [_layoutManager ensureLayoutForTextContainer:_textContainer];
      } else {
        // FIXME(linxs:) We will delete these when enableTextContainerOpt is enabled by default.
        if (_layoutSpec.maxLineNum > 0) {
          h = _layoutSpec.maxLineNum *
              (MAX(_layoutSpec.textStyle.lineHeight, [self maxFontSize] * 1.5));
        }
        if (_layoutSpec.widthMode != LynxMeasureModeDefinite) {
          w = MAXFLOAT;
        }
        if (_layoutSpec.heightMode != LynxMeasureModeDefinite) {
          h = MAXFLOAT;
        }
        LLogInfo(@"LynxTextRender textLength:%lu, w:%f, "
                 @"h:%f,maxLineNum:%ld,lineHeight:%f,direction:%ld,fontWeight:%f,fontStyle:%ld,"
                 @"fontFamily:%@,letterSpacing:%f",
                 (unsigned long)(_textStorage.length), w, h, (long)_layoutSpec.maxLineNum,
                 _layoutSpec.textStyle.lineHeight, (long)(_layoutSpec.textStyle.direction),
                 _layoutSpec.textStyle.fontWeight, (long)(_layoutSpec.textStyle.fontStyle),
                 _layoutSpec.textStyle.fontFamilyName, _layoutSpec.textStyle.letterSpacing);

        [_layoutManager ensureLayoutForBoundingRect:CGRectMake(0, 0, w, h)
                                    inTextContainer:_textContainer];
      }
    }
  } else {
    [_layoutManager ensureLayoutForTextContainer:_textContainer];
  }
  [self workaroundEllipsisError];
  _calculatedSize = [_layoutManager usedRectForTextContainer:_textContainer].size;

  [self handleInlineTruncation];

  [self handleAutoSize];
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)

  // if text size is smaller than width, need layout once again for text-align
  NSTextAlignment physicalAlignment = _layoutSpec.textStyle.usedParagraphTextAlignment;
  if (_layoutSpec.whiteSpace == LynxWhiteSpaceNowrap &&
      _layoutSpec.widthMode == LynxMeasureModeDefinite &&
      physicalAlignment != NSTextAlignmentLeft && _calculatedSize.width < _layoutSpec.width) {
    _textContainer.size = CGSizeMake(_layoutSpec.width, _calculatedSize.height);
    [_layoutManager ensureLayoutForTextContainer:_textContainer];
    _calculatedSize = [_layoutManager usedRectForTextContainer:_textContainer].size;
  }

  // TODO(yxping): When handling non-natural text positions, the TextContainer needs to be
  // reallocated space to ensure text alignment works within the given space. This logic can be
  // placed in the layout phase's layoutDidFinish phase for re-measurement, rather than re-measuring
  // during the measure process. Currently, it is temporarily placed here mainly because the custom
  // layout's measure and layout phase logic need optimization. This needs to be moved to the layout
  // phase in the future.
  if (physicalAlignment != NSTextAlignmentLeft &&
      ((_layoutSpec.widthMode == LynxMeasureModeAtMost &&
        _calculatedSize.width < _layoutSpec.width) ||
       (_layoutSpec.width == LynxMeasureModeIndefinite))) {
    _textContainer.size = CGSizeMake(_calculatedSize.width, _calculatedSize.height);
    [_layoutManager ensureLayoutForTextContainer:_textContainer];
    _calculatedSize = [_layoutManager usedRectForTextContainer:_textContainer].size;
  } else if (_layoutSpec.widthMode == LynxMeasureModeAtMost ||
             _layoutSpec.widthMode == LynxMeasureModeIndefinite) {
    CGPoint p = [_layoutManager usedRectForTextContainer:_textContainer].origin;
    if (p.x != 0.0) {
      _calculatedSize.width += p.x;
    }
  }
  if (_layoutSpec.enableTailColorConvert) {
    [self overrideTruncatedAttrIfNeed];
  }
  _textSize = CGSizeMake(_calculatedSize.width, _calculatedSize.height);
  // recalucate height according to mode in case of overflow
  if (_layoutSpec.overflow != LynxNoOverflow) {
    if (_calculatedSize.height != modified_spec_height) {
      if (_layoutSpec.heightMode == LynxMeasureModeDefinite ||
          (_layoutSpec.heightMode == LynxMeasureModeAtMost &&
           _calculatedSize.height > modified_spec_height)) {
        _calculatedSize.height = modified_spec_height;
      }
    }
  }

  if (_layoutSpec.widthMode == LynxMeasureModeAtMost && layoutManagerIsTruncated(_layoutManager) &&
      physicalAlignment == NSTextAlignmentCenter) {
    // issue: #2129
    // if textline is truncated by layout manager, the width of final boundary is a litter smaller
    // than parent's width. This will cause text in render result not center in textview boundary.
    // to make the render right, need to do translate on x-axis
    if (_calculatedSize.width < _layoutSpec.width) {
      // only center alignment need to do offset
      _offsetX = (_layoutSpec.width - _calculatedSize.width) / 2.0f;
    }
    _calculatedSize.width = MAX(_calculatedSize.width, _layoutSpec.width);
  }

  [self handleEllipsisDirectionAndNewline:_layoutSpec];

  [_attrStr enumerateAttribute:NSFontAttributeName
                       inRange:NSMakeRange(0, _attrStr.length)
                       options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                    usingBlock:^(UIFont *font, NSRange range, BOOL *_Nonnull stop) {
                      // A hack way to detect if this font is fake apply italic with skew
                      // transform
                      if ([font.fontDescriptor.fontAttributes
                              objectForKey:@"NSCTFontMatrixAttribute"] == nil) {
                        return;
                      }

                      // fake italic use -0.25 skew
                      _calculatedSize.width += font.xHeight * 0.25;
                    }];

  self.baseline = ((LineSpacingAdaptation *)_layoutManager.delegate).baseline;
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
}

- (void)handleAutoSize {
  if (!self.layoutSpec.textStyle.isAutoFontSize ||
      self.layoutSpec.widthMode == LynxMeasureModeIndefinite || _attrStr.length == 0) {
    return;
  }

  BOOL isShrink = [self isTextContentOverflow];
  NSRange textRange = NSMakeRange(0, _textStorage.length);
  NSDictionary<NSAttributedStringKey, id> *attr = [_textStorage attributesAtIndex:0
                                                                   effectiveRange:nil];
  UIFont *font = attr[NSFontAttributeName];
  CGFloat currentFontSize = [font pointSize];

  if (isShrink) {
    if (!isnan(self.layoutSpec.textStyle.lineHeight) &&
        self.layoutSpec.heightMode != LynxMeasureModeIndefinite &&
        self.layoutSpec.textStyle.lineHeight > self.layoutSpec.height &&
        !layoutManagerIsTruncated(_layoutManager)) {
      return;
    }
    // shrink
    do {
      CGFloat smallerFontSize = [self findSmallerFontSize:currentFontSize];
      if (smallerFontSize < 0) {
        break;
      }
      [self ensureLayoutWithFontSize:smallerFontSize textRange:textRange];
      currentFontSize = smallerFontSize;
    } while ([self isTextContentOverflow]);
  } else {
    // grow
    while (true) {
      CGFloat largerFontSize = [self findLargerFontSize:currentFontSize];
      if (largerFontSize < 0) {
        break;
      }
      [self ensureLayoutWithFontSize:largerFontSize textRange:textRange];
      if ([self isTextContentOverflow]) {
        [self ensureLayoutWithFontSize:currentFontSize textRange:textRange];
        break;
      }
      currentFontSize = largerFontSize;
    }
  }
}

- (void)ensureLayoutWithFontSize:(CGFloat)fontSize textRange:(NSRange)textRange {
  [_textStorage enumerateAttribute:NSFontAttributeName
                           inRange:textRange
                           options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                        usingBlock:^(UIFont *font, NSRange range, BOOL *_Nonnull stop) {
                          UIFont *anotherFont = [font fontWithSize:fontSize];
                          [_textStorage addAttribute:NSFontAttributeName
                                               value:anotherFont
                                               range:range];
                        }];
  [_layoutManager ensureLayoutForTextContainer:_textContainer];
  _calculatedSize = [_layoutManager usedRectForTextContainer:_textContainer].size;
}

- (CGFloat)findLargerFontSize:(CGFloat)currentFontSize {
  NSArray *fontSizes = self.layoutSpec.textStyle.autoFontSizePresetSizes;
  if (fontSizes != nil) {
    for (NSUInteger i = 0; i < fontSizes.count; i++) {
      CGFloat fontSize = [fontSizes[i] floatValue];
      if (fontSize > currentFontSize) {
        return fontSize;
      }
    }
  } else {
    CGFloat largerFontSize =
        currentFontSize + self.layoutSpec.textStyle.autoFontSizeStepGranularity;
    if (largerFontSize <= self.layoutSpec.textStyle.autoFontSizeMaxSize) {
      return largerFontSize;
    }
  }

  return -1.f;
}

- (CGFloat)findSmallerFontSize:(CGFloat)currentFontSize {
  NSArray *fontSizes = self.layoutSpec.textStyle.autoFontSizePresetSizes;
  if (fontSizes != nil) {
    for (NSInteger i = fontSizes.count - 1; i >= 0; i--) {
      CGFloat fontSize = [fontSizes[i] floatValue];
      if (fontSize < currentFontSize) {
        return fontSize;
      }
    }
  } else {
    CGFloat smallerFontSize =
        currentFontSize - self.layoutSpec.textStyle.autoFontSizeStepGranularity;
    if (smallerFontSize >= self.layoutSpec.textStyle.autoFontSizeMinSize) {
      return smallerFontSize;
    }
  }

  return -1.f;
}

- (BOOL)isTextContentOverflow {
  return _calculatedSize.width > self.layoutSpec.width ||
         (self.layoutSpec.heightMode != LynxMeasureModeIndefinite &&
          _calculatedSize.height > self.layoutSpec.height) ||
         layoutManagerIsTruncated(_layoutManager);
}

- (void)genSubSpan {
  __block NSMutableArray<LynxEventTargetSpan *> *subSpan = [NSMutableArray new];
  NSAttributedString *attrStr = [self.textStorage copy];

  [attrStr
      enumerateAttribute:LynxInlineTextShadowNodeSignKey
                 inRange:NSMakeRange(0, attrStr.length)
                 options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
              usingBlock:^(LynxShadowNode *node, NSRange range, BOOL *_Nonnull stop) {
                if (!node) {
                  return;
                }
                // split range by newline, don't need the rect of newline
                NSString *str = [attrStr.string substringWithRange:range];
                NSMutableArray *ranges = [NSMutableArray array];
                NSRange newlineRange =
                    [str rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]];
                if (newlineRange.location == NSNotFound) {
                  [ranges addObject:[NSValue valueWithRange:range]];
                } else {
                  NSRange lastNewlineRange = NSMakeRange(0, 0);
                  while (newlineRange.location != NSNotFound) {
                    NSRange remainRange =
                        NSMakeRange(newlineRange.location + newlineRange.length,
                                    range.length - newlineRange.location - newlineRange.length);
                    [ranges addObject:[NSValue
                                          valueWithRange:NSMakeRange(range.location +
                                                                         lastNewlineRange.location +
                                                                         lastNewlineRange.length,
                                                                     newlineRange.location -
                                                                         lastNewlineRange.location -
                                                                         lastNewlineRange.length)]];
                    lastNewlineRange = newlineRange;
                    newlineRange = [str rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]
                                                        options:NSRegularExpressionSearch
                                                          range:remainRange];
                  }
                  [ranges
                      addObject:[NSValue
                                    valueWithRange:NSMakeRange(
                                                       range.location + lastNewlineRange.location +
                                                           lastNewlineRange.length,
                                                       range.length - lastNewlineRange.location -
                                                           lastNewlineRange.length)]];
                }

                for (NSUInteger i = 0; i < [ranges count]; i++) {
                  NSRange characterRange = [ranges[i] rangeValue];
                  if (characterRange.length == 0) {
                    continue;
                  }
                  // If noncontiguous layout is not enabled, this method forces the generation of
                  // glyphs for all characters up to and including the end of the specified range.
                  NSRange glyphRange = [_layoutManager glyphRangeForCharacterRange:characterRange
                                                              actualCharacterRange:NULL];
                  // since text can be breaked by NSLayoutManager for newline
                  // here we need to fetch precise sub range rect
                  [_layoutManager
                      enumerateEnclosingRectsForGlyphRange:glyphRange
                                  withinSelectedGlyphRange:NSMakeRange(NSNotFound, 0)
                                           inTextContainer:_textContainer
                                                usingBlock:^(CGRect rect, BOOL *_Nonnull stop) {
                                                  [subSpan addObject:[[LynxEventTargetSpan alloc]
                                                                         initWithShadowNode:node
                                                                                      frame:rect]];
                                                }];
                }
              }];

  if ([subSpan count] > 0) {
    _subSpan = subSpan;
  }
}

- (BOOL)shouldAppendTruncatedToken {
  return (self.textStorage.length > 0 && self.truncationToken.length > 0);
}

- (NSInteger)numberOfVisibleLines {
  __block NSInteger lineCount = 0;
  __block CGFloat sumOfLineHeight = 0.f;
  NSRange glyphRange = [self visibleGlyphRange];
  [self.layoutManager
      enumerateLineFragmentsForGlyphRange:glyphRange
                               usingBlock:^(CGRect rect, CGRect usedRect,
                                            NSTextContainer *_Nonnull textContainer,
                                            NSRange glyphRange, BOOL *_Nonnull stop) {
                                 sumOfLineHeight += rect.size.height;
                                 if (self->_layoutSpec.heightMode != LynxMeasureModeIndefinite &&
                                     sumOfLineHeight > self->_layoutSpec.height) {
                                   *stop = true;
                                   if (lineCount == 0) {
                                     // display one line at least
                                     lineCount = 1;
                                   }
                                   return;
                                 }
                                 ++lineCount;
                               }];
  return lineCount;
}

- (NSRange)visibleGlyphRange {
  return [self.layoutManager glyphRangeForTextContainer:_textContainer];
}

- (void)updateTruncatedSize {
  if (_truncationToken.length > 0) {
    NSTextStorage *truncationTokeStorage =
        [[NSTextStorage alloc] initWithAttributedString:_truncationToken];
    LynxTextLayoutManager *layoutManager = [[LynxTextLayoutManager alloc] init];
    layoutManager.usesFontLeading = NO;
    [truncationTokeStorage addLayoutManager:layoutManager];
    NSTextContainer *textContainer =
        [self createTextContainerWithSize:CGSizeMake(CGFLOAT_MAX, CGFLOAT_MAX) spec:_layoutSpec];
    [layoutManager addTextContainer:textContainer];
    [layoutManager ensureLayoutForTextContainer:textContainer];
    _truncatedSize = [layoutManager usedRectForTextContainer:textContainer].size;
  }
}

- (CGFloat)layoutMaxWidth {
  return _layoutSpec.width;
}

- (void)handleInlineTruncation {
  if (![self shouldAppendTruncatedToken]) {
    return;
  }

  // must should set, self.textStorage.string.length == NSMaxRange(lineCharacterRange)
  _textContainer.lineBreakMode = NSLineBreakByWordWrapping;
  NSInteger approximateNumberOfLines = [self numberOfVisibleLines];
  if (_layoutSpec.maxLineNum > 0 && approximateNumberOfLines > _layoutSpec.maxLineNum) {
    approximateNumberOfLines = _layoutSpec.maxLineNum;
  }

  NSInteger numberOfLines, index, numberOfGlyphs = [self.layoutManager numberOfGlyphs];
  NSRange lineGlyphRange = NSMakeRange(NSNotFound, 0);
  NSRange lastLineCharacterRange = lineGlyphRange;
  NSUInteger originTextStorageLength = _textStorage.length;
  for (numberOfLines = 0, index = 0; index < numberOfGlyphs && approximateNumberOfLines > 0;
       numberOfLines++) {
    [self.layoutManager lineFragmentRectForGlyphAtIndex:index effectiveRange:&lineGlyphRange];
    if (numberOfLines == approximateNumberOfLines - 1) {
      lastLineCharacterRange = [self.layoutManager characterRangeForGlyphRange:lineGlyphRange
                                                              actualGlyphRange:NULL];
      break;
    }
    index = NSMaxRange(lineGlyphRange);
  }
  if ((lastLineCharacterRange.location == NSNotFound ||
       originTextStorageLength <= NSMaxRange(lastLineCharacterRange)) &&
      _calculatedSize.width <= [self layoutMaxWidth]) {
    return;
  }

  [self updateTruncatedSize];
  if (_truncatedSize.width > [self layoutMaxWidth]) {
    return;
  }

  NSRange lastLineGlyphRange = [_layoutManager glyphRangeForCharacterRange:lastLineCharacterRange
                                                      actualCharacterRange:NULL];
  NSUInteger truncationPositionIndex = NSMaxRange(lastLineCharacterRange);

  NSDictionary *lastLineGlyphWidthDic = [self calculateLastLineGlyphWidth:lastLineGlyphRange];
  CGFloat sumOfGlyphWidth = 0.f;
  CGFloat remainLastLineWidth = [self layoutMaxWidth] - _truncatedSize.width;
  NSUInteger truncatedGlyphIndex = NSMaxRange(lastLineGlyphRange);
  for (NSUInteger i = lastLineGlyphRange.location; i < NSMaxRange(lastLineGlyphRange); i++) {
    id glyphWidth = [lastLineGlyphWidthDic objectForKey:@(i)];
    if (glyphWidth) {
      sumOfGlyphWidth += [glyphWidth floatValue];
      if (sumOfGlyphWidth > remainLastLineWidth) {
        truncatedGlyphIndex = i;
        break;
      }
    }
  }
  truncationPositionIndex = [_layoutManager characterIndexForGlyphAtIndex:truncatedGlyphIndex];

  while (truncationPositionIndex >= lastLineCharacterRange.location) {
    if (truncationPositionIndex > lastLineCharacterRange.location) {
      NSString *truncatedLineStr = [[self.textStorage string]
          substringWithRange:NSMakeRange(
                                 lastLineCharacterRange.location,
                                 truncationPositionIndex - lastLineCharacterRange.location)];
      // remove newline character
      while (truncationPositionIndex > lastLineCharacterRange.location &&
             [truncatedLineStr
                 rangeOfCharacterFromSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]
                                 options:NSBackwardsSearch]
                     .location == truncationPositionIndex - 1) {
        truncatedLineStr = [truncatedLineStr substringToIndex:truncatedLineStr.length - 1];
        truncationPositionIndex--;
      }
    }

    [self ensureTextRenderLayoutAfterTruncated:truncationPositionIndex];
    if ([self isTextOverflowAfterTruncated:lastLineGlyphRange.location] &&
        truncationPositionIndex > lastLineCharacterRange.location) {
      truncationPositionIndex--;
    } else {
      break;
    }
  };

  _ellipsisCount = originTextStorageLength - truncationPositionIndex;
}

- (BOOL)isTextOverflowAfterTruncated:(NSInteger)lastLineStartIndex {
  NSRange lastLineGlyphRange;
  [_layoutManager lineFragmentRectForGlyphAtIndex:lastLineStartIndex
                                   effectiveRange:&lastLineGlyphRange];
  NSRange characterRange = [_layoutManager characterRangeForGlyphRange:lastLineGlyphRange
                                                      actualGlyphRange:NULL];
  return NSMaxRange(characterRange) < _textStorage.length;
}

- (void)ensureTextRenderLayoutAfterTruncated:(NSUInteger)truncationPositionIndex {
  NSRange tokenRange =
      NSMakeRange(truncationPositionIndex, self.textStorage.length - truncationPositionIndex);
  if (tokenRange.length > 0) {
    [self.layoutManager invalidateDisplayForCharacterRange:tokenRange];
    [self.textStorage beginEditing];
    [self.textStorage replaceCharactersInRange:tokenRange
                          withAttributedString:self.truncationToken];
    if (self.layoutTruncationBlock) {
      self.layoutTruncationBlock(self.textStorage);
    }
    [self.textStorage endEditing];

    [_layoutManager ensureLayoutForTextContainer:_textContainer];
    _calculatedSize = [_layoutManager usedRectForTextContainer:_textContainer].size;
  }
}

- (NSDictionary *)calculateLastLineGlyphWidth:(NSRange)lastLineGlyphRange {
  NSMutableDictionary *glyphWidthDic = [[NSMutableDictionary alloc] init];
  NSMutableSet *locationSet = [[NSMutableSet alloc] init];
  NSMutableArray *glyphLocationArray = [[NSMutableArray alloc] init];
  NSUInteger glyphIndex = lastLineGlyphRange.location;
  while (glyphIndex < NSMaxRange(lastLineGlyphRange)) {
    NSRange actualGlyphRange;
    [_layoutManager characterRangeForGlyphRange:NSMakeRange(glyphIndex, 1)
                               actualGlyphRange:&actualGlyphRange];
    CGPoint location = [_layoutManager locationForGlyphAtIndex:actualGlyphRange.location];
    if (![locationSet containsObject:@(location.x)]) {
      [glyphWidthDic setObject:@(location.x) forKey:@(glyphIndex)];
      [locationSet addObject:@(location.x)];
      [glyphLocationArray addObject:@(location.x)];
    }
    glyphIndex = NSMaxRange(actualGlyphRange);
  }
  CGRect lastLineRect = [_layoutManager lineFragmentRectForGlyphAtIndex:lastLineGlyphRange.location
                                                         effectiveRange:NULL];
  [glyphLocationArray addObject:@(lastLineRect.origin.x)];
  [glyphLocationArray addObject:@(lastLineRect.origin.x + lastLineRect.size.width)];
  [glyphLocationArray sortUsingSelector:@selector(compare:)];
  NSRange arrayRange = NSMakeRange(0, glyphLocationArray.count);
  for (NSNumber *key in glyphWidthDic.allKeys) {
    id location = glyphWidthDic[key];
    NSUInteger lastEqualIndex =
        [glyphLocationArray indexOfObject:location
                            inSortedRange:arrayRange
                                  options:NSBinarySearchingLastEqual
                          usingComparator:^NSComparisonResult(id _Nonnull obj1, id _Nonnull obj2) {
                            return [obj1 compare:obj2];
                          }];
    if (lastEqualIndex < glyphLocationArray.count - 1) {
      glyphWidthDic[key] = @([[glyphLocationArray objectAtIndex:(lastEqualIndex + 1)] floatValue] -
                             [location floatValue]);
    } else {
      glyphWidthDic[key] = @0;
    }
  }

  return glyphWidthDic;
}

/**
 * This method is designed to work around an ellipsis error in text layout.
 * In versions above iOS 18, text omission may cause an exception, which is manifested as only one
 * character and ellipsis displayed in the last line of text. This issue is related to the text
 * content and text width limit, and it is likely to be triggered when the omission position is a
 * full-width symbol. The method only executes its logic on iOS 18.0 and above.
 */
- (void)workaroundEllipsisError {
  if (@available(iOS 18.0, *)) {
    if (_layoutSpec.textOverflow != LynxTextOverflowEllipsis ||
        _layoutSpec.widthMode == LynxMeasureModeIndefinite) {
      return;
    }
    NSRange truncatedRange = [_layoutManager
        truncatedGlyphRangeInLineFragmentForGlyphAtIndex:_layoutManager.numberOfGlyphs - 1];
    if (truncatedRange.location == NSNotFound) {
      return;
    }

    __block NSRange lastLineGlyphRange = NSMakeRange(0, 0);
    [_layoutManager
        enumerateLineFragmentsForGlyphRange:NSMakeRange(0, _layoutManager.numberOfGlyphs)
                                 usingBlock:^(CGRect rect, CGRect usedRect,
                                              NSTextContainer *_Nonnull textContainer,
                                              NSRange glyphRange, BOOL *_Nonnull stop) {
                                   lastLineGlyphRange = glyphRange;
                                 }];
    // Check if the truncated glyph is at the expected position in the last line.
    // In case of an exception, only one glyph and ellipsis will be displayed on the last line.
    if (truncatedRange.location != lastLineGlyphRange.location + 1) {
      return;
    }
    CGPoint truncatedGlyphPoint = [_layoutManager locationForGlyphAtIndex:truncatedRange.location];
    CGPoint lastLineStartPoint =
        [_layoutManager locationForGlyphAtIndex:lastLineGlyphRange.location];
    if (fabs(truncatedGlyphPoint.x - lastLineStartPoint.x) * 3 > _layoutSpec.width) {
      return;
    }
    // Append the space attributed string to the text storage.
    // This may change the text layout and potentially fix the ellipsis issue.
    NSAttributedString *spaceString = [[NSAttributedString alloc] initWithString:@" "];
    [_textStorage appendAttributedString:spaceString];
    [_layoutManager ensureLayoutForTextContainer:_textContainer];
  }
}

- (void)handleEllipsisDirectionAndNewline:(LynxLayoutSpec *)spec {
  if (spec.textOverflow != LynxTextOverflowEllipsis || _truncationToken.length > 0) {
    return;
  }

  __block NSUInteger truncatedLocation = NSNotFound;
  if (spec.textStyle.direction != NSWritingDirectionNatural) {
    [_layoutManager
        enumerateLineFragmentsForGlyphRange:NSMakeRange(0, _layoutManager.textStorage.length)
                                 usingBlock:^(CGRect rect, CGRect usedRect,
                                              NSTextContainer *_Nonnull textContainer,
                                              NSRange glyphRange, BOOL *_Nonnull stop) {
                                   NSRange truncatedRange = [self->_layoutManager
                                       truncatedGlyphRangeInLineFragmentForGlyphAtIndex:
                                           glyphRange.location];
                                   if (truncatedRange.location != NSNotFound) {
                                     truncatedLocation = truncatedRange.location;
                                     *stop = YES;
                                   }
                                 }];
  }
  NSRange truncatedGlyphRange;
  if (truncatedLocation != NSNotFound) {
    truncatedGlyphRange =
        [_layoutManager truncatedGlyphRangeInLineFragmentForGlyphAtIndex:(truncatedLocation)];
  } else {
    NSRange visibleRange = [self visibleGlyphRange];
    NSUInteger glyphLength = [_layoutManager numberOfGlyphs];
    truncatedGlyphRange =
        NSMakeRange(visibleRange.location + visibleRange.length, glyphLength - visibleRange.length);
  }

  if (truncatedGlyphRange.location != NSNotFound && truncatedGlyphRange.length != 0) {
    NSRange truncatedCharacterRange =
        [_layoutManager characterRangeForGlyphRange:truncatedGlyphRange actualGlyphRange:NULL];
    // handle newline at the end of last line.
    if (truncatedCharacterRange.location > 0) {
      NSRange newlineRange = [[[_textStorage
          attributedSubstringFromRange:NSMakeRange(truncatedCharacterRange.location - 1, 1)] string]
          rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]];
      if (newlineRange.location != NSNotFound) {
        truncatedCharacterRange.location--;
        truncatedCharacterRange.length++;
      }
    }
    _ellipsisCount = truncatedCharacterRange.length;
    [_textStorage
        replaceCharactersInRange:truncatedCharacterRange
                      withString:[LynxTextUtils
                                     getEllpsisStringAccordingToWritingDirection:spec.textStyle
                                                                                     .direction]];
    [_layoutManager ensureLayoutForTextContainer:_textContainer];
  }
}

- (void)overrideTruncatedAttrIfNeed {
  [_layoutManager
      enumerateLineFragmentsForGlyphRange:NSMakeRange(0, _layoutManager.textStorage.length)
                               usingBlock:^(CGRect rect, CGRect usedRect,
                                            NSTextContainer *_Nonnull textContainer,
                                            NSRange glyphRange, BOOL *_Nonnull stop) {
                                 NSRange truncatedRange = [self->_layoutManager
                                     truncatedGlyphRangeInLineFragmentForGlyphAtIndex:
                                         glyphRange.location];
                                 if (truncatedRange.length == 0) {
                                   // no truncated on this line
                                   return;
                                 }
                                 truncatedRange = [self->_layoutManager
                                     characterRangeForGlyphRange:truncatedRange
                                                actualGlyphRange:NULL];

                                 NSDictionary<NSAttributedStringKey, id> *attr =
                                     [self->_layoutManager.textStorage
                                         attributesAtIndex:truncatedRange.location
                                            effectiveRange:nil];
                                 NSDictionary *baseAttr =
                                     [self->_layoutManager.textStorage attributesAtIndex:0
                                                                          effectiveRange:nil];
                                 NSMutableDictionary *overrideAttr =
                                     [NSMutableDictionary dictionaryWithDictionary:attr];
                                 overrideAttr[NSForegroundColorAttributeName] =
                                     baseAttr[NSForegroundColorAttributeName];

                                 [self->_layoutManager.textStorage
                                     setAttributes:overrideAttr
                                             range:NSMakeRange(
                                                       truncatedRange.location,
                                                       self->_layoutManager.textStorage.length -
                                                           truncatedRange.location)];

                                 *stop = YES;
                               }];
}

- (CGSize)size {
  return _calculatedSize;
}

- (CGFloat)textContentOffsetX {
  return _offsetX;
}

- (CGSize)textsize {
  return _textSize;
}

- (CGFloat)maxFontSize {
  // TODO: (linxs)check performance and run times
  if (_maxFontSize == 0) {
    __block CGFloat fontsize = 0;
    [_layoutManager.textStorage
        enumerateAttribute:NSFontAttributeName
                   inRange:NSMakeRange(0, _layoutManager.textStorage.length)
                   options:NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                usingBlock:^(UIFont *font, NSRange range, __unused BOOL *stop) {
                  fontsize = font.pointSize > fontsize ? font.pointSize : fontsize;
                }];
    _maxFontSize = fontsize;
  }
  return _maxFontSize;
}

- (NSLayoutManager *)layoutManager {
  return _layoutManager;
}

- (NSTextStorage *)textStorage {
  return _textStorage;
}

- (void)drawTextRect:(CGRect)bounds padding:(UIEdgeInsets)padding border:(UIEdgeInsets)border {
  NSTextContainer *textContainer = _layoutManager.textContainers.firstObject;
  NSRange glyphRange = [_layoutManager glyphRangeForTextContainer:textContainer];
  CGPoint origin = bounds.origin;
  origin.x += (padding.left + border.left);
  origin.y += (padding.top + border.top);
  // issue: #2129
  // do a little translate on x-axis to make text content render at center
  origin.x += _offsetX;
  if ([_layoutManager isKindOfClass:[LynxTextLayoutManager class]]) {
    LynxTextLayoutManager *temp = (LynxTextLayoutManager *)_layoutManager;
    temp.glyphCount = 0;
    temp.preEndPosition = CGPointZero;
    temp.preDrawableRange = NSMakeRange(0, 0);
  }
  [_layoutManager drawBackgroundForGlyphRange:glyphRange atPoint:origin];
  [_layoutManager drawGlyphsForGlyphRange:glyphRange atPoint:origin];
}

- (void)drawRect:(CGRect)bounds padding:(UIEdgeInsets)padding border:(UIEdgeInsets)border {
  if ([_layoutManager isKindOfClass:[LynxTextLayoutManager class]]) {
    NSRange glyphRange = [_layoutManager glyphRangeForTextContainer:_textContainer];
    CGRect textBoundingRect = [_layoutManager boundingRectForGlyphRange:glyphRange
                                                        inTextContainer:_textContainer];
    [(id)_layoutManager setOverflowOffset:CGPointMake(bounds.origin.x + textBoundingRect.origin.x,
                                                      bounds.origin.y + textBoundingRect.origin.y)];
    // _calculatedSize.width is larger than textBoundingRect.size.width when font style is italic
    [(id)_layoutManager
        setTextBoundingRectSize:CGSizeMake(
                                    MAX(_calculatedSize.width, textBoundingRect.size.width),
                                    MAX(_calculatedSize.height, textBoundingRect.size.height))];
  }
  [self drawTextRect:bounds padding:padding border:border];
}

- (NSTextContainer *)createTextContainerWithSize:(CGSize)inputSize spec:(LynxLayoutSpec *)spec {
  NSTextContainer *textContainer = [[NSTextContainer alloc] initWithSize:inputSize];
  // lineFragmentPadding default is 5.0
  textContainer.lineFragmentPadding = 0;
  // ellipsis mode
  if (spec.textOverflow == LynxTextOverflowEllipsis) {
    textContainer.lineBreakMode = NSLineBreakByTruncatingTail;
  } else {
    if (spec.enableNewClipMode) {
      textContainer.lineBreakMode = NSLineBreakByWordWrapping;
    } else {
      textContainer.lineBreakMode = NSLineBreakByClipping;
    }
  }
  // max-line 0 means no limits
  if (spec.whiteSpace == LynxWhiteSpaceNowrap) {
    textContainer.maximumNumberOfLines = 1;
  } else if (spec.maxLineNum != LynxNumberNotSet) {
    textContainer.maximumNumberOfLines = spec.maxLineNum;
  } else {
    textContainer.maximumNumberOfLines = 0;
  }
  return textContainer;
}

@end
