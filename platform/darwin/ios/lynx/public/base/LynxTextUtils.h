// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface LynxTextUtils : NSObject
/*
 * Resolve NSTextAlignNatural to physical alignment according to the inferred languange of given
 * attributed string. The resolved alignment will be applied to the paragraph style of the given
 * attirbuted string. If attributed string has a explictly assigned physical alignment, this
 * function won't do anything
 *
 * @param attrString the attributed string to infer from and apply physical alignment.
 * @return the applied alignment.
 */
+ (NSTextAlignment)applyNaturalAlignmentAccordingToTextLanguage:
                       (nonnull NSMutableAttributedString *)attrString
                                                       refactor:(BOOL)enableRefactor;

/*
 * Get ellipsis string with unicode direction marker to controll the direction of ellipsis
 *
 * @param direction of the ellipsis
 * @return the ellipsis string.
 */
+ (nonnull NSString *)getEllpsisStringAccordingToWritingDirection:(NSWritingDirection)direction;

+ (NSDictionary *_Nonnull)measureText:(NSString *_Nullable)text
                             fontSize:(CGFloat)fontSize
                           fontFamily:(NSString *_Nullable)fontFamily
                             maxWidth:(CGFloat)maxWidth
                              maxLine:(NSInteger)maxLine;

+ (UIFontWeight)convertLynxFontWeight:(NSUInteger)fontWeight;
+ (NSString *_Nonnull)ConvertRawText:(id _Nullable)rawText;
@end
