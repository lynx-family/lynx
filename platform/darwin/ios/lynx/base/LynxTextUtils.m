// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxFontFaceManager.h>
#import <Lynx/LynxTextRenderer.h>
#import <Lynx/LynxTextUtils.h>
#import <NaturalLanguage/NaturalLanguage.h>

NSString *const ELLIPSIS = @"\u2026";
// Strong direction unicodes to control the direction of ellipsis
NSString *const LTR_MARK = @"\u200E";
NSString *const RTL_MARK = @"\u200F";

@implementation LynxTextUtils

+ (NSTextAlignment)applyNaturalAlignmentAccordingToTextLanguage:
                       (nonnull NSMutableAttributedString *)attrString
                                                       refactor:(BOOL)enableRefactor {
  if (attrString == nil) {
    return NSTextAlignmentNatural;
  }
  NSRange range = NSMakeRange(0, attrString.length);
  if (range.length == 0) {
    return NSTextAlignmentNatural;
  }
  // Fetch the outer paragraph style from the attributed string
  NSMutableParagraphStyle *paraStyle = [[attrString attribute:NSParagraphStyleAttributeName
                                                      atIndex:0
                                        longestEffectiveRange:nil
                                                      inRange:range] mutableCopy];
  if (paraStyle == nil) {
    // If the paragraph style is not set, use the default one.
    paraStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
  }

  NSTextAlignment paraAlignment = paraStyle.alignment;
  NSTextAlignment physicalAlignment = paraAlignment;

  // Only run language detection for first 20 utf-16 codes, that would work for direction detection
  // in most cases.
  const int LANGUAGE_DETECT_MAX_LENGTH = 20;
  // If the paragraph alignment is natural, decide the alignment according to the locale of content.
  if (physicalAlignment == NSTextAlignmentNatural) {
    NSString *text = [attrString string];

    if (text.length) {
      NSString *language = nil;
      // Guess best matched langauge basing on the content.
      if (enableRefactor) {
        if (@available(iOS 12.0, *)) {
          language = [NLLanguageRecognizer dominantLanguageForString:text];
        } else {
          NSLinguisticTagger *tagger =
              [[NSLinguisticTagger alloc] initWithTagSchemes:@[ NSLinguisticTagSchemeLanguage ]
                                                     options:0];
          [tagger setString:text];
          language = [tagger tagAtIndex:0
                                 scheme:NSLinguisticTagSchemeLanguage
                             tokenRange:NULL
                          sentenceRange:NULL];
        }
      } else {
        language = CFBridgingRelease(CFStringTokenizerCopyBestStringLanguage(
            (CFStringRef)text, CFRangeMake(0, MIN([text length], LANGUAGE_DETECT_MAX_LENGTH))));
      }

      if (language) {
        // Get the direction of the guessed locale
        NSLocaleLanguageDirection direction = [NSLocale characterDirectionForLanguage:language];
        physicalAlignment = (direction == NSLocaleLanguageDirectionRightToLeft)
                                ? NSTextAlignmentRight
                                : NSTextAlignmentLeft;
      }
    }
  }
  // If there is a inferred alignment, apply the inferred alignment to the paragraph.
  if (physicalAlignment != paraAlignment) {
    paraStyle.alignment = physicalAlignment;
    [attrString addAttribute:NSParagraphStyleAttributeName value:paraStyle range:range];
  }
  return physicalAlignment;
}

+ (nonnull NSString *)getEllpsisStringAccordingToWritingDirection:(NSWritingDirection)direction {
  if (direction == NSWritingDirectionNatural) {
    return ELLIPSIS;
  }
  return [NSString
      stringWithFormat:@"%@%@", (direction == NSWritingDirectionLeftToRight ? LTR_MARK : RTL_MARK),
                       ELLIPSIS];
}

+ (NSDictionary *)measureText:(NSString *_Nullable)text
                     fontSize:(CGFloat)fontSize
                   fontFamily:(NSString *_Nullable)fontFamily
                     maxWidth:(CGFloat)maxWidth
                      maxLine:(NSInteger)maxLine {
  CGFloat width = 0.f;
  if (text == nil || text.length == 0 || fontSize == 0 || (maxLine > 1 && maxWidth < 1)) {
    return @{@"width" : @(width)};
  }
  UIFont *font = [UIFont systemFontOfSize:fontSize];
  if (fontFamily.length > 0) {
    UIFont *maybeTargetFont = [[LynxFontFaceManager sharedManager] getRegisteredUIFont:fontFamily
                                                                              fontSize:fontSize];
    if (!maybeTargetFont) {
      maybeTargetFont = [UIFont fontWithName:fontFamily size:fontSize];
    }
    font = maybeTargetFont ?: font;
  }
  NSDictionary *textAttributes = @{NSFontAttributeName : font};
  // if maxWidth is not set, the text only layout for one line
  if (maxWidth < 1) {
    CGRect rect = [text boundingRectWithSize:CGSizeMake(CGFLOAT_MAX, 100)
                                     options:NSStringDrawingUsesLineFragmentOrigin
                                  attributes:textAttributes
                                     context:nil];
    width = rect.size.width;
    return @{@"width" : @(width)};
  } else {
    NSAttributedString *measureText = [[NSAttributedString alloc] initWithString:text
                                                                      attributes:textAttributes];
    NSTextStorage *textStorage = [[NSTextStorage alloc] initWithAttributedString:measureText];
    NSLayoutManager *layoutManager = [[NSLayoutManager alloc] init];
    layoutManager.usesFontLeading = NO;
    layoutManager.allowsNonContiguousLayout = YES;
    [textStorage addLayoutManager:layoutManager];
    NSTextContainer *textContainer =
        [[NSTextContainer alloc] initWithSize:CGSizeMake(maxWidth, 1000)];
    textContainer.maximumNumberOfLines = maxLine;
    textContainer.lineFragmentPadding = 0;
    [layoutManager addTextContainer:textContainer];
    [layoutManager ensureLayoutForTextContainer:textContainer];
    width = [layoutManager usedRectForTextContainer:textContainer].size.width;
    NSRange glyphsRange = [layoutManager glyphRangeForTextContainer:textContainer];
    NSMutableArray *lineStringArr = [NSMutableArray new];
    [layoutManager enumerateLineFragmentsForGlyphRange:glyphsRange
                                            usingBlock:^(CGRect rect, CGRect usedRect,
                                                         NSTextContainer *_Nonnull textContainer,
                                                         NSRange glyphRange, BOOL *_Nonnull stop) {
                                              NSRange characterRange = [layoutManager
                                                  characterRangeForGlyphRange:glyphRange
                                                             actualGlyphRange:nil];
                                              NSString *lineStr =
                                                  [text substringWithRange:characterRange];
                                              [lineStringArr addObject:lineStr ?: @""];
                                            }];

    return @{@"width" : @(width), @"content" : lineStringArr};
  }
}

+ (UIFontWeight)convertLynxFontWeight:(NSUInteger)fontWeight {
  if (fontWeight == LynxFontWeightNormal) {
    return UIFontWeightRegular;
  } else if (fontWeight == LynxFontWeightBold) {
    return UIFontWeightBold;
  } else if (fontWeight == LynxFontWeight100) {
    return UIFontWeightUltraLight;
  } else if (fontWeight == LynxFontWeight200) {
    return UIFontWeightThin;
  } else if (fontWeight == LynxFontWeight300) {
    return UIFontWeightLight;
  } else if (fontWeight == LynxFontWeight400) {
    return UIFontWeightRegular;
  } else if (fontWeight == LynxFontWeight500) {
    return UIFontWeightMedium;
  } else if (fontWeight == LynxFontWeight600) {
    return UIFontWeightSemibold;
  } else if (fontWeight == LynxFontWeight700) {
    return UIFontWeightBold;
  } else if (fontWeight == LynxFontWeight800) {
    return UIFontWeightHeavy;
  } else if (fontWeight == LynxFontWeight900) {
    return UIFontWeightBlack;
  } else {
    return UIFontWeightRegular;
  }
}

+ (NSString *)ConvertRawText:(id)rawText {
  NSString *text = @"";
  if ([rawText isKindOfClass:[NSString class]]) {
    text = rawText;
  } else if ([rawText isKindOfClass:[@(NO) class]]) {
    // __NSCFBoolean is subclass of NSNumber, so this need to check first
    BOOL boolValue = [rawText boolValue];
    text = boolValue ? @"true" : @"false";
  } else if ([rawText isKindOfClass:[NSNumber class]]) {
    double conversionValue = [rawText doubleValue];
    NSString *doubleString = [NSString stringWithFormat:@"%lf", conversionValue];
    NSDecimalNumber *decNumber = [NSDecimalNumber decimalNumberWithString:doubleString];
    text = [decNumber stringValue];
    // remove scientific notation when display big num, such as "1.23456789012E11" to "123456789012"
    // NSNumberFormatter* formatter = [[NSNumberFormatter alloc] init];
    // formatter.numberStyle = kCFNumberFormatterNoStyle;
    //  text = [formatter stringFromNumber:[NSNumber numberWithDouble:[value doubleValue]]];
    //  text = [[NSDecimalNumber decimalNumberWithString:text] stringValue];
  }
  return text;
}

+ (CGFloat)calcBaselineShiftOffset:(LynxVerticalAlign)verticalAlign
                verticalAlignValue:(CGFloat)verticalAlignValue
                      withAscender:(CGFloat)ascender
                     withDescender:(CGFloat)descender
                    withLineHeight:(CGFloat)lineHeight
                   withMaxAscender:(CGFloat)maxAscender
                  withMaxDescender:(CGFloat)maxDescender
                    withMaxXHeight:(CGFloat)maxXHeight {
  switch (verticalAlign) {
    case LynxVerticalAlignLength:
      return verticalAlignValue;
    case LynxVerticalAlignPercent:
      // if set vertical-align:50%, baselineShift = 50 * lineHeight /100.f, the lineHeight is 0 if
      // lineHeight not set.
      return lineHeight * verticalAlignValue / 100.f;
    case LynxVerticalAlignMiddle:
      // the middle of element will be align to the middle of max x-height
      return (-descender - ascender + maxXHeight) * 0.5f;
    case LynxVerticalAlignTextTop:
    case LynxVerticalAlignTop:
      // the ascender of element will be align to text max ascender
      return maxAscender - ascender;
    case LynxVerticalAlignTextBottom:
    case LynxVerticalAlignBottom:
      // the descender of element will be align to text max descender
      return maxDescender - descender;
    case LynxVerticalAlignSub:
      //-height * 0.1
      return -(ascender - descender) * 0.1f;
    case LynxVerticalAlignSuper:
      // height * 0.1
      return (ascender - descender) * 0.1f;
    case LynxVerticalAlignCenter:
      // the middle of element will be align to the middle of line
      return (maxAscender + maxDescender - ascender - descender) * 0.5f;
    default:
      // baseline,center,top,bottom
      return 0.f;
  }
}

+ (CGFloat)alignInlineNodeInVertical:(LynxVerticalAlign)verticalAlign
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

+ (NSInteger)convertLynxTextDecorationStyle:(NSInteger)decorationStyle {
  NSInteger textDecorationStyle;
  switch (decorationStyle) {
    case LynxTextDecorationSolid:
      textDecorationStyle = NSUnderlineStylePatternSolid;
      break;
    case LynxTextDecorationDouble:
      textDecorationStyle = NSUnderlineStyleDouble;
      break;
    case LynxTextDecorationDotted:
      textDecorationStyle = NSUnderlineStylePatternDot;
      break;
    case LynxTextDecorationDashed:
      textDecorationStyle = NSUnderlineStylePatternDash;
      break;
    // todo wavy
    default:
      textDecorationStyle = NSUnderlineStyleSingle;
  }
  return textDecorationStyle;
}

+ (void)setLynxTextGradient:(LynxTextStyle *_Nonnull)textStyle
               withGradient:(NSArray *_Nullable)value {
  if (value == nil || [value count] < 2 || ![value[1] isKindOfClass:[NSArray class]]) {
    textStyle.textGradient = nil;
  } else {
    NSUInteger type = [LynxConverter toNSUInteger:value[0]];
    NSArray *args = (NSArray *)value[1];
    if (type == LynxBackgroundImageLinearGradient) {
      textStyle.textGradient = [[LynxLinearGradient alloc] initWithArray:args];
    } else if (type == LynxBackgroundImageRadialGradient) {
      textStyle.textGradient = [[LynxRadialGradient alloc] initWithArray:args];
    } else if (type == LynxBackgroundImageConicGradient) {
      textStyle.textGradient = [[LynxConicGradient alloc] initWithArray:args];
    }
  }
}

+ (NSDictionary *)computeLayoutEventInfoWithRenderer:(LynxTextRenderer *)renderer
                                    attributedString:(NSAttributedString *)attributedString
                                          maxLineNum:(NSInteger)maxLineNum {
  if (!renderer || !attributedString) {
    return nil;
  }

  NSTextStorage *textStorage = renderer.textStorage;
  if (!textStorage) {
    return nil;
  }

  NSLayoutManager *layoutManager = textStorage.layoutManagers.firstObject;
  NSTextContainer *textContainer = layoutManager.textContainers.firstObject;
  if (!layoutManager || !textContainer) {
    return nil;
  }

  NSRange layoutGlyphRange =
      [layoutManager glyphRangeForCharacterRange:NSMakeRange(0, textStorage.length)
                            actualCharacterRange:nil];

  __block NSInteger lineCount = 0;
  __block NSRange lastLineGlyphRange = {0, 0};
  [layoutManager enumerateLineFragmentsForGlyphRange:layoutGlyphRange
                                          usingBlock:^(CGRect rect, CGRect usedRect,
                                                       NSTextContainer *_Nonnull container,
                                                       NSRange glyphRange, BOOL *_Nonnull stop) {
                                            lineCount++;
                                            lastLineGlyphRange = glyphRange;
                                          }];

  NSMutableDictionary *layoutInfo = [NSMutableDictionary new];

  if (lineCount == 0) {
    layoutInfo[@"lineCount"] = @(0);
    layoutInfo[@"lines"] = @[];

    CGSize layoutSize = renderer.size;
    NSMutableDictionary *sizeDict = [NSMutableDictionary new];
    sizeDict[@"width"] = @(layoutSize.width);
    sizeDict[@"height"] = @(layoutSize.height);
    layoutInfo[@"size"] = sizeDict;

    return layoutInfo;
  }

  NSRange truncatedRangeForLastLine =
      [layoutManager truncatedGlyphRangeInLineFragmentForGlyphAtIndex:lastLineGlyphRange.location];
  if (truncatedRangeForLastLine.location == NSNotFound) {
    NSString *string = attributedString.string;
    if (string.length > 0 && [string characterAtIndex:string.length - 1] == '\n') {
      lineCount++;
    }
  }

  NSInteger actualLineCount = lineCount;
  if (maxLineNum > 0 && maxLineNum < lineCount) {
    actualLineCount = maxLineNum;
  }

  NSMutableArray *lineInfo = [NSMutableArray new];
  __block NSInteger index = 0;
  [layoutManager
      enumerateLineFragmentsForGlyphRange:layoutGlyphRange
                               usingBlock:^(CGRect rect, CGRect usedRect,
                                            NSTextContainer *_Nonnull container, NSRange glyphRange,
                                            BOOL *_Nonnull stop) {
                                 NSRange characterRange =
                                     [layoutManager characterRangeForGlyphRange:glyphRange
                                                               actualGlyphRange:nil];
                                 NSMutableDictionary *info = [NSMutableDictionary new];
                                 NSInteger start = characterRange.location;
                                 NSInteger end = characterRange.location + characterRange.length;
                                 NSInteger ellipsisCount = 0;

                                 if (index == lineCount - 1) {
                                   if (renderer.ellipsisCount != 0) {
                                     ellipsisCount = renderer.ellipsisCount;
                                     end = attributedString.length;
                                   } else {
                                     NSRange truncatedRange = [layoutManager
                                         truncatedGlyphRangeInLineFragmentForGlyphAtIndex:
                                             glyphRange.location];
                                     if (truncatedRange.location != NSNotFound) {
                                       ellipsisCount =
                                           [layoutManager characterRangeForGlyphRange:truncatedRange
                                                                     actualGlyphRange:nil]
                                               .length;
                                     }
                                   }
                                 }

                                 if (index == actualLineCount - 1 && actualLineCount < lineCount) {
                                   ellipsisCount = attributedString.length -
                                                   characterRange.location - characterRange.length;
                                   end = attributedString.length;
                                   *stop = YES;
                                 }

                                 info[@"start"] = @(start);
                                 info[@"end"] = @(end);
                                 info[@"ellipsisCount"] = @(ellipsisCount);
                                 [lineInfo addObject:info];

                                 index++;
                               }];

  if ((NSInteger)lineInfo.count < actualLineCount) {
    NSMutableDictionary *lastLineInfo = [NSMutableDictionary new];
    NSInteger length = attributedString.length;
    lastLineInfo[@"start"] = @(length);
    lastLineInfo[@"end"] = @(length);
    lastLineInfo[@"ellipsisCount"] = @(0);
    [lineInfo addObject:lastLineInfo];
  }

  layoutInfo[@"lineCount"] = @(actualLineCount);
  layoutInfo[@"lines"] = lineInfo;

  CGSize layoutSize = renderer.size;
  NSMutableDictionary *sizeDict = [NSMutableDictionary new];
  sizeDict[@"width"] = @(layoutSize.width);
  sizeDict[@"height"] = @(layoutSize.height);
  layoutInfo[@"size"] = sizeDict;

  return layoutInfo;
}

@end
