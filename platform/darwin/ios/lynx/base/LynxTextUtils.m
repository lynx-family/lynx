// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTextUtils.h"
#import <NaturalLanguage/NaturalLanguage.h>
#import "LynxFontFaceManager.h"

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

@end
