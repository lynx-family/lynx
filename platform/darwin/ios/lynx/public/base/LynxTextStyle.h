// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxBackgroundManager.h"
#import "LynxCSSType.h"

NS_ASSUME_NONNULL_BEGIN

#if !defined(__IPHONE_8_2) || __IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_8_2

#define UIFontWeightUltraLight -0.8
#define UIFontWeightThin -0.6
#define UIFontWeightLight -0.4
#define UIFontWeightRegular 0
#define UIFontWeightMedium 0.23
#define UIFontWeightSemibold 0.3
#define UIFontWeightBold 0.4
#define UIFontWeightHeavy 0.56
#define UIFontWeightBlack 0.62

#endif
@class LynxFontFaceContext;
@protocol LynxFontFaceObserver;

extern NSAttributedStringKey const LynxTextColorGradientKey;
extern NSAttributedStringKey const LynxInlineBackgroundKey;
extern NSAttributedStringKey const LynxWordBreakKey;
@interface LynxTextStyle : NSObject <NSCopying>

@property(nonatomic, assign) CGFloat fontSize;
@property(nonatomic, assign) CGFloat lineHeight;
@property(nonatomic, assign) CGFloat lineSpacing;
@property(nonatomic, assign) CGFloat letterSpacing;
@property(nonatomic, assign) NSTextAlignment textAlignment;
@property(nonatomic, assign) NSTextAlignment usedParagraphTextAlignment;
@property(nonatomic, assign) NSWritingDirection direction;
@property(nonatomic, assign) CGFloat fontWeight;
@property(nonatomic, assign) LynxFontStyleType fontStyle;
@property(nonatomic, strong, nullable) UIColor* foregroundColor;
@property(nonatomic, strong, nullable) UIColor* backgroundColor;
@property(nonatomic, nullable) NSString* fontFamilyName;
@property(nonatomic, nullable) NSString* underLine;
@property(nonatomic, nullable) NSString* lineThrough;
@property(nonatomic, assign) NSInteger textDecorationStyle;
@property(nonatomic, nullable) UIColor* textDecorationColor;
@property(nonatomic, strong, nullable) NSShadow* textShadow;
@property(nonatomic, strong, nullable) LynxGradient* textGradient;
@property(nonatomic, assign) LynxWordBreakType wordBreak;

@property(nonatomic, strong) NSMutableArray* backgroundDrawable;
@property(nonatomic, nullable) NSMutableArray* backgroundPosition;
@property(nonatomic, nullable) NSMutableArray* backgroundRepeat;
@property(nonatomic, nullable) NSMutableArray* backgroundImageSize;
@property(nonatomic, assign) LynxBorderRadii borderRadius;

- (void)updateBackgroundDrawableRepeat;
- (void)updateBackgroundDrawablePosition;
- (void)updateBackgroundDrawableSize;
- (void)updateBackgroundRadius;

@property(nonatomic, assign) BOOL enableFontScaling;
@property(nonatomic, assign) BOOL textFakeBold;
@property(nonatomic, assign) BOOL enableLanguageAlignment;
@property(nonatomic, assign) CGFloat textIndent;
@property(nonatomic, assign) CGFloat textStrokeWidth;
@property(nonatomic, strong) UIColor* textStrokeColor;
@property(nonatomic, strong) UIColor* selectionColor;
@property(nonatomic, assign) BOOL isAutoFontSize;
@property(nonatomic, assign) CGFloat autoFontSizeMaxSize;
@property(nonatomic, assign) CGFloat autoFontSizeMinSize;
@property(nonatomic, assign) CGFloat autoFontSizeStepGranularity;
@property(nonatomic, strong, nullable) NSArray* autoFontSizePresetSizes;
@property(nonatomic, assign) BOOL hyphen;

@property(nonatomic, strong, nullable) NSAttributedString* truncationAttributedStr;

- (NSDictionary<NSAttributedStringKey, id>*)
    toAttributesWithFontFaceContext:(LynxFontFaceContext*)fontFaceContext
               withFontFaceObserver:(id<LynxFontFaceObserver> _Nullable)observer;

- (NSParagraphStyle*)genParagraphStyle;

- (UIFont*)fontWithFontFaceContext:(LynxFontFaceContext*)fontFaceContext
                  fontFaceObserver:(id<LynxFontFaceObserver>)observer;

- (void)applyTextStyle:(LynxTextStyle*)textStyle;
- (void)setTextAlignment:(NSTextAlignment)textAlignment;

@end

NS_ASSUME_NONNULL_END
