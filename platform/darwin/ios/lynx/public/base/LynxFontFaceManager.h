// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LUIBodyView.h"
#import "LynxContext.h"
#import "LynxError.h"
#import "LynxGenericResourceFetcher.h"
#import "LynxResourceFetcher.h"
#import "LynxResourceProvider.h"
#import "LynxTextStyle.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, LynxFontSrcType) {
  LynxFontSrcLocal = 0,
  LynxFontSrcUrl,
};

@interface LynxFontSrcItem : NSObject
@property(nonatomic, assign) LynxFontSrcType type;
@property(atomic, strong, nullable) NSString *src;
@property(atomic, strong, nullable) NSString *dataFontName;
@property(nonatomic, strong, nullable) NSPointerArray *notifierArray;
@end

@interface LynxAliasFontInfo : NSObject
@property(nonatomic, strong) UIFont *font;
@property(nonatomic, strong) NSString *name;
- (bool)isEmpty;
@end

@interface LynxFontFace : NSObject
- (instancetype)initWithFamilyName:(NSString *)familyName
                            andSrc:(NSString *)src
                   withLynxContext:(LynxContext *)context;
- (NSUInteger)srcCount;
- (LynxFontSrcItem *)srcAtIndex:(NSUInteger)index;
@end

@protocol LynxFontFaceObserver <NSObject>
@optional
- (void)onFontFaceLoad;

@end
@interface LynxFontFaceContext : NSObject
@property(nonatomic, weak) id<LynxResourceFetcher> resourceFetcher;
@property(nonatomic, weak) id<LynxResourceProvider> resourceProvider;
@property(nonatomic, weak) id<LynxGenericResourceFetcher> genericResourceServiceFetcher;
@property(nonatomic, weak) UIView<LUIBodyView> *rootView;
@property(nonatomic, weak) NSDictionary *builderRegistedAliasFontMap;
- (void)addFontFace:(LynxFontFace *)fontFace;
- (nullable LynxFontFace *)getFontFaceWithFamilyName:(NSString *)familyName;
@end

@interface LynxFontFaceManager : NSObject
+ (LynxFontFaceManager *)sharedManager;
- (UIFont *)generateFontWithSize:(CGFloat)fontSize
                          weight:(CGFloat)fontWeight
                           style:(LynxFontStyleType)fontStyle
                  fontFamilyName:(NSString *)fontFamilyName
                 fontFaceContext:(LynxFontFaceContext *)fontFaceContext
                fontFaceObserver:(id<LynxFontFaceObserver>)observer;
- (void)registerFont:(UIFont *)font forName:(NSString *)name;
- (void)registerFamilyName:(NSString *)fontFamilyName withAliasName:(NSString *)aliasName;
- (UIFont *)getRegisteredUIFont:(NSString *)familyName fontSize:(CGFloat)fontSize;
@end
NS_ASSUME_NONNULL_END
