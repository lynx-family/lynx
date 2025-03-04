// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxFontFaceManager.h"
#import <CommonCrypto/CommonDigest.h>
#include <CoreText/CTFontManager.h>
#import "LUIBodyView.h"
#import "LynxContext.h"
#import "LynxEnv.h"
#import "LynxLog.h"
#import "LynxResourceResponseDataInfoProtocol.h"
#import "LynxServiceResourceRequestParameters.h"
#import "LynxSubErrorCode.h"
#import "LynxTemplateRender+Internal.h"
#import "LynxTraceEvent.h"
#import "LynxTraceEventWrapper.h"
#import "LynxView+Internal.h"

typedef struct _LynxInnerFontInfo {
  BOOL isSystemFont, isBold, isItalic, isFontWeightBoldMatched, isItalicMatched;
} LynxInnerFontInfo;

@implementation LynxFontSrcItem
@end

@implementation LynxFontFace {
  NSString *_familyName;
  NSArray *_srcArray;
}

- (NSString *)getKey {
  return _familyName;
}

- (instancetype)initWithFamilyName:(NSString *)familyName
                            andSrc:(NSString *)src
                   withLynxContext:(LynxContext *)context {
  if (self = [super init]) {
    _familyName = familyName;
    _srcArray = [LynxFontFace parseSrc:src withLynxContext:context];
    if ([_familyName length] && [_srcArray count]) {
      return self;
    }
  }
  return nil;
}

+ (void)reportSrcFormatError:(NSString *)errorMessage withLynxContext:(LynxContext *)context {
  LynxError *error = [LynxError lynxErrorWithCode:ECLynxResourceFontSrcFormatError
                                          message:errorMessage];
  [context.getLynxView reportError:error];
}

+ (NSArray *)parseSrc:(NSString *)src withLynxContext:(LynxContext *)context {
  if (src == nil) {
    [self reportSrcFormatError:@"Fontface src is empty" withLynxContext:context];
    return nil;
  }

  static NSString *const srcFormatError = @"Src format error";
  NSArray *srcArr = [src componentsSeparatedByString:@","];
  if (![srcArr count]) {
    [self reportSrcFormatError:[NSString stringWithFormat:@"%@:src is %@", srcFormatError, src]
               withLynxContext:context];
    return nil;
  }

  NSMutableCharacterSet *trimmingSet = [[NSCharacterSet whitespaceCharacterSet] mutableCopy];
  [trimmingSet addCharactersInString:@"\"'"];

  NSMutableArray *retArr = nil;

  NSString *curStr = nil;
  for (NSString *cur in srcArr) {
    if (cur == nil) {
      continue;
    }
    if (curStr == nil) {
      curStr = cur;
    } else {
      curStr = [curStr stringByAppendingFormat:@",%@", cur];
    }
    NSRange rangeEnd = [curStr rangeOfString:@")"];
    if (rangeEnd.location == NSNotFound) {
      continue;
    }

    BOOL isLocal = false;
    NSRange rangeStart = [curStr rangeOfString:@"url("];
    if (rangeStart.location == NSNotFound || rangeStart.location > rangeEnd.location) {
      rangeStart = [curStr rangeOfString:@"local("];
      if (rangeStart.location == NSNotFound || rangeStart.location > rangeEnd.location) {
        curStr = nil;
        [self reportSrcFormatError:[NSString stringWithFormat:@"%@,src:%@", srcFormatError, src]
                   withLynxContext:context];
        continue;
      } else {
        isLocal = true;
      }
    }

    NSRange valRange;
    valRange.location = rangeStart.location + rangeStart.length;
    valRange.length = rangeEnd.location - valRange.location;

    curStr = [[curStr substringWithRange:valRange] stringByTrimmingCharactersInSet:trimmingSet];
    if (![curStr length]) {
      curStr = nil;
      [self reportSrcFormatError:[NSString stringWithFormat:@"%@,src:%@", srcFormatError, src]
                 withLynxContext:context];
      continue;
    }

    if (retArr == nil) {
      retArr = [NSMutableArray new];
    }

    LynxFontSrcItem *item = [LynxFontSrcItem new];
    item.src = curStr;
    curStr = nil;
    if (isLocal) {
      item.type = LynxFontSrcLocal;
    } else {
      item.type = LynxFontSrcUrl;
    }
    [retArr addObject:item];
  }

  return retArr;
}

- (NSUInteger)srcCount {
  return [_srcArray count];
}

- (LynxFontSrcItem *)srcAtIndex:(NSUInteger)index {
  return [_srcArray objectAtIndex:index];
}

@end

@implementation LynxFontFaceContext {
  NSMutableDictionary *_dic;
}

- (void)addFontFace:(LynxFontFace *)fontFace {
  if (fontFace == nil) return;

  if (_dic == nil) {
    _dic = [NSMutableDictionary new];
  }
  [_dic setObject:fontFace forKey:[fontFace getKey]];
}

- (LynxFontFace *)getFontFaceWithFamilyName:(NSString *)familyName {
  if (familyName == nil || _dic == nil) return nil;

  return [_dic valueForKey:familyName];
}

@end

@implementation LynxAliasFontInfo
- (bool)isEmpty {
  return self.font == nil && self.name == nil;
}
@end

@implementation LynxFontFaceManager {
  NSMutableDictionary<NSString *, id> *_registedFontMap;
  NSMutableDictionary<NSString *, LynxAliasFontInfo *> *_registedAliasFontMap;
}

+ (LynxFontFaceManager *)sharedManager {
  static LynxFontFaceManager *sharedManager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    sharedManager = [[LynxFontFaceManager alloc] init];
  });

  return sharedManager;
}

- (instancetype)init {
  if (self = [super init]) {
    _registedFontMap = [NSMutableDictionary new];
    _registedAliasFontMap = [NSMutableDictionary new];
  }
  return self;
}

- (void)registerFont:(UIFont *)font forName:(NSString *)name {
  if ([name length] == 0) {
    return;
  }

  @synchronized(_registedAliasFontMap) {
    LynxAliasFontInfo *info = [_registedAliasFontMap objectForKey:name];
    if (info == nil) {
      if (font != nil) {
        info = [LynxAliasFontInfo new];
        info.font = font;
        [_registedAliasFontMap setObject:info forKey:name];
      }
    } else {
      info.font = font;
      if ([info isEmpty]) {
        [_registedAliasFontMap removeObjectForKey:name];
      }
    }
  }
}

- (void)registerFamilyName:(NSString *)fontFamilyName withAliasName:(NSString *)aliasName {
  if ([aliasName length] == 0) {
    return;
  }

  @synchronized(_registedAliasFontMap) {
    LynxAliasFontInfo *info = [_registedAliasFontMap objectForKey:aliasName];
    if (info == nil) {
      if (fontFamilyName != nil) {
        info = [LynxAliasFontInfo new];
        info.name = fontFamilyName;
        [_registedAliasFontMap setObject:info forKey:aliasName];
      }
    } else {
      info.name = fontFamilyName;
      if ([info isEmpty]) {
        [_registedAliasFontMap removeObjectForKey:aliasName];
      }
    }
  }
}

- (BOOL)isFont:(UIFont *)font matchFontStyle:(LynxFontStyleType)fontStyle {
  NSDictionary *traits = [font.fontDescriptor objectForKey:UIFontDescriptorTraitsAttribute];
  UIFontDescriptorSymbolicTraits symbolicTraits = [traits[UIFontSymbolicTrait] unsignedIntValue];
  int isItalic = fontStyle != LynxFontStyleNormal;
  return (symbolicTraits & UIFontDescriptorTraitItalic) == isItalic;
}

- (CGFloat)fontWeightOfFont:(UIFont *)font {
  static NSArray *fontNames;
  static NSArray *fontWeights;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    fontNames = @[
      @"normal", @"ultralight", @"thin", @"light", @"regular", @"medium", @"semibold", @"demibold",
      @"extrabold", @"bold", @"heavy", @"black"
    ];
    fontWeights = @[
      @(UIFontWeightRegular), @(UIFontWeightUltraLight), @(UIFontWeightThin), @(UIFontWeightLight),
      @(UIFontWeightRegular), @(UIFontWeightMedium), @(UIFontWeightSemibold),
      @(UIFontWeightSemibold), @(UIFontWeightHeavy), @(UIFontWeightBold), @(UIFontWeightHeavy),
      @(UIFontWeightBlack)
    ];
  });

  for (NSUInteger i = 0; i < fontNames.count; i++) {
    if ([font.fontName.lowercaseString hasSuffix:fontNames[i]]) {
      return [fontWeights[i] doubleValue];
    }
  }

  NSDictionary *traits = [font.fontDescriptor objectForKey:UIFontDescriptorTraitsAttribute];
  return [traits[UIFontWeightTrait] doubleValue];
}

- (BOOL)isFont:(UIFont *)font matchFontWeight:(CGFloat)targetFontWeight {
  return [self fontWeightOfFont:font] == targetFontWeight;
}

- (UIFont *)findFontWithSize:(CGFloat)fontSize
                      weight:(CGFloat)fontWeight
                       style:(LynxFontStyleType)fontStyle
              fontFamilyName:(NSString *)fontFamilyName
                    fontInfo:(LynxInnerFontInfo *)info {
  UIFont *font = nil;
  if ([UIFont fontNamesForFamilyName:fontFamilyName].count == 0) {
    // Find with given font name when there no font family for it
    font = [UIFont fontWithName:fontFamilyName size:fontSize];
    if (font) {
      info->isFontWeightBoldMatched = [self isFont:font matchFontWeight:fontWeight];
      info->isItalicMatched = [self isFont:font matchFontStyle:fontStyle];
    }
  } else {
    // Get all font names with the same prefix font family name
    NSMutableArray *fontNames = [NSMutableArray new];
    [fontNames addObjectsFromArray:[UIFont fontNamesForFamilyName:fontFamilyName]];
    if (![fontFamilyName containsString:@" light"]) {
      [fontNames
          addObjectsFromArray:[UIFont
                                  fontNamesForFamilyName:[fontFamilyName
                                                             stringByAppendingString:@" light"]]];
    }
    if (![fontFamilyName containsString:@" medium"]) {
      [fontNames
          addObjectsFromArray:[UIFont
                                  fontNamesForFamilyName:[fontFamilyName
                                                             stringByAppendingString:@" medium"]]];
    }

    // Find appropriate font in font family with same font style and closet font weight
    CGFloat closestWeight = INFINITY;
    for (NSString *name in fontNames) {
      UIFont *maybeTargetFont = [UIFont fontWithName:name size:fontSize];
      CGFloat maybeTargetFontWeight = [self fontWeightOfFont:maybeTargetFont];
      if ([self isFont:maybeTargetFont matchFontStyle:fontStyle] &&
          ABS(maybeTargetFontWeight - fontWeight) < ABS(closestWeight - fontWeight)) {
        font = maybeTargetFont;
        closestWeight = maybeTargetFontWeight;
        info->isItalicMatched = YES;
        if (closestWeight > UIFontWeightMedium && info->isBold) {
          info->isFontWeightBoldMatched = YES;
        }
      }
    }

    // If still not find appropriate font, we should return the first font in family
    if (!font) {
      font = [UIFont fontWithName:fontNames[0] size:fontSize];
    }
  }
  return font;
}

- (nullable NSString *)cachedKey:(nullable NSString *)key {
  if (key == nil) {
    key = @"";
  } else if (![key hasPrefix:@"data:"]) {
    return key;
  }
  const char *str = key.UTF8String;
  unsigned char r[CC_MD5_DIGEST_LENGTH];
  CC_MD5(str, (CC_LONG)strlen(str), r);

  return [NSString
      stringWithFormat:@"MD5_%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                       r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9], r[10], r[11],
                       r[12], r[13], r[14], r[15]];
}

- (void)onRequestErrorClearCache:(NSString *)cacheKey {
  @synchronized(_registedFontMap) {
    id cached = [_registedFontMap objectForKey:cacheKey];
    if (cached != nil && ![cached isKindOfClass:[NSString class]]) {
      [_registedFontMap removeObjectForKey:cacheKey];
    }
  }
}

- (bool)onRequestStartForItem:(LynxFontSrcItem *)item
                        cache:(NSString *)cacheKey
             fontFaceObserver:(id<LynxFontFaceObserver>)fontFaceObserver {
  @synchronized(_registedFontMap) {
    id cached = [_registedFontMap objectForKey:cacheKey];
    if (cached != nil) {
      if ([cached isKindOfClass:[NSString class]]) {
        // has name cached
        return false;
      }
      if ([cached isKindOfClass:[NSMutableSet class]]) {
        // is requesting for items
        NSMutableSet *itemSet = cached;
        if (fontFaceObserver != nil) {
          // only add into the existing notify array
          for (LynxFontSrcItem *cur in itemSet) {
            [cur.notifierArray addPointer:(__bridge void *_Nullable)(fontFaceObserver)];
          }
        }
        [itemSet addObject:item];
        // do not request again
        return false;
      }
    }
    NSMutableSet *newSet = [NSMutableSet new];
    if (item.notifierArray == nil) {
      item.notifierArray = [NSPointerArray weakObjectsPointerArray];
    }
    if (fontFaceObserver != nil) {
      [item.notifierArray addPointer:(__bridge void *_Nullable)(fontFaceObserver)];
    }
    [newSet addObject:item];
    [_registedFontMap setValue:newSet forKey:cacheKey];
    return true;
  }
}

- (NSString *)registerFontWithData:(NSData *)data
                      withCacheKey:(NSString *)cacheKey
                          lynxView:(UIView<LUIBodyView> *)lynxView {
  [UIFont familyNames];  // This prevents a known crash in CGDataProviderCreateWithCFData 😓
  CGDataProviderRef fontDataProvider = CGDataProviderCreateWithCFData((CFDataRef)data);
  CGFontRef newFont = CGFontCreateWithDataProvider(fontDataProvider);
  NSString *newFontName = (__bridge_transfer NSString *)CGFontCopyPostScriptName(newFont);
  CGDataProviderRelease(fontDataProvider);
  CFErrorRef error;
  bool regResult = CTFontManagerRegisterGraphicsFont(newFont, &error);
  LLogInfo(@"font Name: %@ register lynx: %@, font src: %@", newFontName, lynxView.url, cacheKey);
  if (!regResult) {
    // Fix __bridge retain error and cause memory leak
    // https://stackoverflow.com/questions/14352494/bridged-cast-bridge-transfer-vs-bridge-with-synthesized-setter/28495303
    NSError *nsError = (__bridge_transfer NSError *)error;
    if (nsError.code == kCTFontManagerErrorAlreadyRegistered) {
      regResult = true;
      LLogError(
          @"font %@, register error: kCTFontManagerErrorAlreadyRegistered lynx: %@, font src: %@",
          newFontName, lynxView.url, cacheKey);

      LynxError *lynxError = [LynxError lynxErrorWithCode:ECLynxResourceFontRegisterFailed
                                                  message:@"Font already registered"
                                            fixSuggestion:LynxErrorSuggestionRefOfficialSite
                                                    level:LynxErrorLevelWarn
                                               customInfo:nil
                                             isLogBoxOnly:YES];

      [lynxError addCustomInfo:newFontName forKey:@"font_name"];
      [self reportResourceError:lynxError withLynxView:lynxView resourceUrl:cacheKey];
    } else {
      if (@available(iOS 14.0, *)) {
        regResult = nsError.code == kCTFontManagerErrorDuplicatedName;
      }
      CFStringRef errorDescription = CFErrorCopyDescription(error);
      LLogError(@"font register error: %@ lynxview: %@, font src: %@", errorDescription,
                lynxView.url, cacheKey);

      LynxError *lynxError =
          [LynxError lynxErrorWithCode:ECLynxResourceFontRegisterFailed
                               message:[NSString stringWithFormat:@"Font registration failed: %@",
                                                                  errorDescription]
                         fixSuggestion:LynxErrorSuggestionRefOfficialSite
                                 level:LynxErrorLevelWarn];
      [lynxError addCustomInfo:newFontName forKey:@"font_name"];
      [self reportResourceError:lynxError withLynxView:lynxView resourceUrl:cacheKey];
      CFRelease(errorDescription);
    }
  }
  CGFontRelease(newFont);
  if (regResult) {
    @synchronized(_registedFontMap) {
      id cachedVal = [_registedFontMap valueForKey:cacheKey];
      if ([cachedVal isKindOfClass:[NSMutableSet class]]) {
        for (LynxFontSrcItem *item in cachedVal) {
          item.dataFontName = newFontName;
          // do not used again
          item.src = nil;
        }
      }
      [_registedFontMap setValue:newFontName forKey:cacheKey];
      return newFontName;
    }
  } else {
    [self onRequestErrorClearCache:cacheKey];
    return nil;
  }
}

- (void)handleFontData:(NSData *)data
              withItem:(LynxFontSrcItem *)item
              lynxView:(UIView<LUIBodyView> *)rootView
          withCacheKey:(NSString *)cacheKey {
  if ([data length] > 0) {
    item.dataFontName = [self registerFontWithData:data withCacheKey:cacheKey lynxView:rootView];
  } else {
    [self onRequestErrorClearCache:cacheKey];
  }
}

- (void)handleGenericResourceFetcherFontData:(NSData *)data
                                   withError:(NSError *)error
                                    withItem:(LynxFontSrcItem *)item
                                    lynxView:(UIView<LUIBodyView> *)rootView
                                withCacheKey:(NSString *)cacheKey {
  if ([data length] > 0) {
    item.dataFontName = [self registerFontWithData:data withCacheKey:cacheKey lynxView:rootView];
  } else {
    [self onRequestErrorClearCache:cacheKey];
  }
}

- (void)notifyFontFaceObserverWithItem:(LynxFontSrcItem *)item
                   lynxFontFaceContext:(LynxFontFaceContext *)fontFaceContext {
  __weak typeof(fontFaceContext) weakFontFaceContext = fontFaceContext;
  dispatch_async(dispatch_get_main_queue(), ^{
    if (item.notifierArray != nil && item.dataFontName != nil && weakFontFaceContext) {
      [item.notifierArray compact];
      NSArray<id<LynxFontFaceObserver>> *allObjects = item.notifierArray.allObjects;
      for (id<LynxFontFaceObserver> cur in allObjects) {
        [cur onFontFaceLoad];
      }
    }
    item.notifierArray = nil;
  });
}

- (void)requestFontfaceItem:(LynxFontSrcItem *)item
            fontFaceContext:(LynxFontFaceContext *)fontFaceContext
           fontFaceObserver:(id<LynxFontFaceObserver>)fontFaceObserver
                   cacheKey:(NSString *)cacheKey {
  __weak typeof(self) weakSelf = self;
  __weak typeof(fontFaceContext) weakFontFaceContext = fontFaceContext;
  __weak typeof(fontFaceObserver) weakFontFaceObserver = fontFaceObserver;
  NSString *urlStr = item.src;
  if ([urlStr hasPrefix:@"data:"]) {
    NSRange range = [urlStr rangeOfString:@";base64," options:0 range:NSMakeRange(0, 100)];
    if (range.location != NSNotFound) {
      if ([self onRequestStartForItem:item cache:cacheKey fontFaceObserver:fontFaceObserver]) {
        // decode base64
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
          NSData *decodedData = [[NSData alloc]
              initWithBase64EncodedString:[urlStr substringFromIndex:range.location + range.length]
                                  options:0];
          [weakSelf handleFontData:decodedData
                          withItem:item
                          lynxView:weakFontFaceContext.rootView
                      withCacheKey:cacheKey];
          if (decodedData == nil || decodedData.length == 0) {
            LynxError *error = [LynxError lynxErrorWithCode:ECLynxResourceFontBase64ParsingError
                                                    message:@"Error when parsing base64 resource"];
            [self reportResourceError:error
                         withLynxView:weakFontFaceContext.rootView
                          resourceUrl:urlStr];
          }
          // TODO(zhouzhuangzhuang):only notify observer when register success.
          [weakSelf notifyFontFaceObserverWithItem:item lynxFontFaceContext:weakFontFaceContext];
        });
      }
      return;
    }
  }
  // try fetch resource by genericResourceFetcher, if failed, try provider and fetcher.
  id<LynxGenericResourceFetcher> genericResourceFetcher =
      fontFaceContext.genericResourceServiceFetcher;
  if (urlStr != nil && genericResourceFetcher != nil &&
      [genericResourceFetcher respondsToSelector:@selector(fetchResource:onComplete:)]) {
    if ([self onRequestStartForItem:item cache:cacheKey fontFaceObserver:fontFaceObserver]) {
      LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                         @"LynxFontFaceManager requestFontfaceItemWithGenericResourceFetcher")
      LynxResourceRequest *request = [[LynxResourceRequest alloc] initWithUrl:urlStr
                                                                         type:LynxResourceTypeFont];
      [genericResourceFetcher
          fetchResource:request
             onComplete:^(NSData *_Nullable data, NSError *_Nullable error) {
               [weakSelf handleGenericResourceFetcherFontData:data
                                                    withError:error
                                                     withItem:item
                                                     lynxView:weakFontFaceContext.rootView
                                                 withCacheKey:cacheKey];
               if (error) {
                 NSString *errMessage = [NSString
                     stringWithFormat:@"Load font with genericResourceFetcher Failed:%@", error];
                 LynxError *error = [LynxError lynxErrorWithCode:ECLynxResourceFontResourceLoadError
                                                         message:errMessage];
                 [self reportResourceError:error
                              withLynxView:weakFontFaceContext.rootView
                               resourceUrl:urlStr];

                 // TODO(zhouzhuangzhuang):there is no need to continue requesting resources.
                 LLogWarn(@"request Fontface with generic Lynx resource fetcher failed, try "
                          @"request Fontface with font provider and fetcher, font src: %@",
                          urlStr);
                 [weakSelf requestFontfaceByFontProviderWithItem:item
                                                 fontFaceContext:weakFontFaceContext
                                                fontFaceObserver:weakFontFaceObserver
                                                        cacheKey:cacheKey];
               } else {
                 [weakSelf notifyFontFaceObserverWithItem:item
                                      lynxFontFaceContext:weakFontFaceContext];
               }
             }];
      LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
      return;
    } else {
      return;
    }
  }

  LLogWarn(
      @"generic Lynx resource fetcher is not available, try provider and fetcher, font src: %@",
      urlStr);
  [self requestFontfaceByFontProviderWithItem:item
                              fontFaceContext:fontFaceContext
                             fontFaceObserver:fontFaceObserver
                                     cacheKey:cacheKey];
}

- (void)requestFontfaceByFontProviderWithItem:(LynxFontSrcItem *)item
                              fontFaceContext:(LynxFontFaceContext *)fontFaceContext
                             fontFaceObserver:(id<LynxFontFaceObserver>)fontFaceObserver
                                     cacheKey:(NSString *)cacheKey {
  __weak typeof(self) weakSelf = self;
  __weak typeof(fontFaceContext) weakFontFaceContext = fontFaceContext;
  id<LynxResourceProvider> fontResProvider = fontFaceContext.resourceProvider;
  NSString *urlStr = item.src;
  if (fontResProvider != nil && urlStr != nil) {
    if (![self onRequestStartForItem:item cache:cacheKey fontFaceObserver:fontFaceObserver]) {
      return;
    }
    LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                       @"LynxFontFaceManager requestFontfaceByFontProvider")
    LynxResourceRequest *request = [[LynxResourceRequest alloc] initWithUrl:urlStr];
    [fontResProvider
           request:request
        onComplete:^(LynxResourceResponse *_Nonnull response) {
          NSData *data = response.data;
          [weakSelf handleFontData:data
                          withItem:item
                          lynxView:weakFontFaceContext.rootView
                      withCacheKey:cacheKey];
          if (data == nil || data.length == 0) {
            NSString *reasonPrefix = @"Load font with resourceProvider Failed";
            NSString *errMessage = reasonPrefix;
            if (response.error != nil) {
              errMessage = [NSString stringWithFormat:@"%@: %@", reasonPrefix, response.error];
            }
            LynxError *error = [LynxError lynxErrorWithCode:ECLynxResourceFontResourceLoadError
                                                    message:errMessage];
            [self reportResourceError:error
                         withLynxView:weakFontFaceContext.rootView
                          resourceUrl:item.src];
          }

          [weakSelf notifyFontFaceObserverWithItem:item lynxFontFaceContext:weakFontFaceContext];
        }];
    LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
    return;
  }

  id<LynxResourceFetcher> fetcher = fontFaceContext.resourceFetcher;
  if (fetcher != nil &&
      [fetcher respondsToSelector:@selector(loadResourceWithURL:type:completion:)] &&
      urlStr != nil) {
    NSURL *url = [[NSURL alloc] initWithString:urlStr];
    if ([self onRequestStartForItem:item cache:cacheKey fontFaceObserver:fontFaceObserver]) {
      LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                         @"LynxFontFaceManager requestFontfaceByResourceFetcher")
      [fetcher loadResourceWithURL:url
                              type:LynxFetchResFontFace
                        completion:^(BOOL isSyncCallback, NSData *_Nullable data,
                                     NSError *_Nullable error, NSURL *_Nullable resURL) {
                          [weakSelf handleFontData:data
                                          withItem:item
                                          lynxView:weakFontFaceContext.rootView
                                      withCacheKey:cacheKey];
                          if (data == nil || data.length == 0) {
                            NSString *reasonPrefix = @"Load font with resourceFetcher Failed";
                            NSString *errMessage = reasonPrefix;
                            if (error != nil) {
                              errMessage =
                                  [NSString stringWithFormat:@"%@: %@", reasonPrefix, error];
                            }
                            LynxError *error =
                                [LynxError lynxErrorWithCode:ECLynxResourceFontResourceLoadError
                                                     message:errMessage];
                            [self reportResourceError:error
                                         withLynxView:weakFontFaceContext.rootView
                                          resourceUrl:item.src];
                          }
                          [weakSelf notifyFontFaceObserverWithItem:item
                                               lynxFontFaceContext:weakFontFaceContext];
                        }];
      LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
    }
  }
}

- (UIFont *)generateUIFontWithSize:(CGFloat)fontSize
                            weight:(CGFloat)fontWeight
                             style:(LynxFontStyleType)fontStyle
                    fontFamilyName:(NSString *)fontFamilyName
                   fontFaceContext:(LynxFontFaceContext *)fontFaceContext
                  fontFaceObserver:(id<LynxFontFaceObserver>)fontFaceObserver
                          fontInfo:(LynxInnerFontInfo *)info {
  UIFont *font = nil;
  if (fontFaceContext != nil) {
    LynxFontFace *fontFace = [fontFaceContext getFontFaceWithFamilyName:fontFamilyName];
    NSUInteger count = [fontFace srcCount];
    for (NSUInteger i = 0; i < count; ++i) {
      LynxFontSrcItem *item = [fontFace srcAtIndex:i];
      NSString *urlStr = item.src;
      if (item.type == LynxFontSrcLocal) {
        // replace with local font family names
        font = [self findFontWithSize:fontSize
                               weight:fontWeight
                                style:fontStyle
                       fontFamilyName:urlStr
                             fontInfo:info];
      } else if (item.type == LynxFontSrcUrl) {
        if (item.dataFontName == nil) {
          NSString *cacheKey = [self cachedKey:urlStr];
          @synchronized(_registedFontMap) {
            id cacheVal = [_registedFontMap objectForKey:cacheKey];
            if (cacheVal != nil && [cacheVal isKindOfClass:[NSString class]]) {
              item.dataFontName = cacheVal;
            }
          }

          if (item.dataFontName == nil) {
            // request by url, return will be sync or ansync
            [self requestFontfaceItem:item
                      fontFaceContext:fontFaceContext
                     fontFaceObserver:fontFaceObserver
                             cacheKey:cacheKey];
          }
        }
        if (item.dataFontName != nil) {
          // match better than [UIFont fontWithName:]
          font =
              [UIFont fontWithDescriptor:[UIFontDescriptor fontDescriptorWithName:item.dataFontName
                                                                             size:fontSize]
                                    size:fontSize];
          info->isFontWeightBoldMatched = [self isFont:font matchFontWeight:fontWeight];
          info->isItalicMatched = [self isFont:font matchFontStyle:fontStyle];
        }
      }
    }
  }

  if (font != nil) {
    return font;
  }

  // find alias settings
  UIFont *fontByAlias = nil;
  NSString *fontFamilyNameByAlias = nil;
  if (fontFaceContext.builderRegistedAliasFontMap &&
      [fontFaceContext.builderRegistedAliasFontMap objectForKey:fontFamilyName]) {
    LynxAliasFontInfo *aliasInfo =
        [fontFaceContext.builderRegistedAliasFontMap objectForKey:fontFamilyName];
    if (aliasInfo != nil) {
      fontByAlias = aliasInfo.font;
      fontFamilyNameByAlias = aliasInfo.name;
    }
  } else {
    @synchronized(_registedAliasFontMap) {
      LynxAliasFontInfo *aliasInfo = [_registedAliasFontMap objectForKey:fontFamilyName];
      if (aliasInfo != nil) {
        fontByAlias = aliasInfo.font;
        fontFamilyNameByAlias = aliasInfo.name;
      }
    }
  }

  if (fontByAlias != nil) {
    // get alias font
    font = [fontByAlias fontWithSize:fontSize];
    if (font != nil) {
      info->isFontWeightBoldMatched = [self isFont:font matchFontWeight:fontWeight];
      info->isItalicMatched = [self isFont:font matchFontStyle:fontStyle];
    }
  }

  if (font == nil) {
    // find font by fontFamilyName
    font = [self findFontWithSize:fontSize
                           weight:fontWeight
                            style:fontStyle
                   fontFamilyName:fontFamilyName
                         fontInfo:info];
  }

  if (font == nil && fontFamilyNameByAlias != nil) {
    // find font by aliasName if exists
    font = [self findFontWithSize:fontSize
                           weight:fontWeight
                            style:fontStyle
                   fontFamilyName:fontFamilyNameByAlias
                         fontInfo:info];
  }
  return font;
}

- (UIFont *)generateFontWithSize:(CGFloat)fontSize
                          weight:(CGFloat)fontWeight
                           style:(LynxFontStyleType)fontStyle
                  fontFamilyName:(NSString *)fontFamilyName
                 fontFaceContext:(LynxFontFaceContext *)fontFaceContext
                fontFaceObserver:(id<LynxFontFaceObserver>)observer {
  static NSString *defaultFontFamilyName;
  static NSCache *systemFontCache;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    defaultFontFamilyName = [UIFont systemFontOfSize:14].familyName;
    systemFontCache = [NSCache new];
  });

  LynxInnerFontInfo info;
  UIFont *font = nil;
  NSArray *fontFamilyNameArray = [fontFamilyName componentsSeparatedByString:@","];
  info.isBold = fontWeight > UIFontWeightMedium;
  info.isItalic = fontStyle != LynxFontStyleNormal;
  if (![fontFamilyNameArray count]) {
    info.isSystemFont = YES;
    info.isFontWeightBoldMatched = info.isItalicMatched = NO;
  } else {
    NSMutableCharacterSet *trimmingSet = [[NSCharacterSet whitespaceCharacterSet] mutableCopy];
    [trimmingSet addCharactersInString:@"\"'"];
    for (NSString *itName in fontFamilyNameArray) {
      NSString *curFontFamilyName = [itName stringByTrimmingCharactersInSet:trimmingSet];
      info.isSystemFont = [curFontFamilyName isEqualToString:defaultFontFamilyName];
      info.isFontWeightBoldMatched = info.isItalicMatched = NO;
      font = [self generateUIFontWithSize:fontSize
                                   weight:fontWeight
                                    style:fontStyle
                           fontFamilyName:curFontFamilyName
                          fontFaceContext:fontFaceContext
                         fontFaceObserver:observer
                                 fontInfo:&info];
      if (font != nil) break;
    }
  }

  // If font family is default font or font still not find with given name, we should generate a
  // system font
  if (font == nil) {
    // System font
    NSString *cacheKey = [[NSString alloc] initWithFormat:@"%.1f/%.2f", fontSize, fontWeight];
    font = [systemFontCache objectForKey:cacheKey];
    if (!font) {
      if (@available(iOS 8.2, *)) {
        font = [UIFont systemFontOfSize:fontSize weight:fontWeight];
      } else if (fontWeight >= UIFontWeightBold) {
        font = [UIFont boldSystemFontOfSize:fontSize];
      } else if (fontWeight >= UIFontWeightMedium) {
        font = [UIFont fontWithName:@"HelveticaNeue-Medium" size:fontSize];
      } else if (fontWeight <= UIFontWeightLight) {
        font = [UIFont fontWithName:@"HelveticaNeue-Light" size:fontSize];
      } else {
        font = [UIFont systemFontOfSize:fontSize];
      }

      [systemFontCache setObject:font forKey:cacheKey];
    }

    // System font have api to handle bold
    info.isFontWeightBoldMatched = YES;
  }

  // Make sure italic and bold are applied to font
  BOOL forceApplyBold = !info.isFontWeightBoldMatched && info.isBold;
  BOOL forceApplyItalic = !info.isItalicMatched && info.isItalic;
  if (forceApplyItalic || forceApplyBold) {
    UIFontDescriptor *fontDescriptor = [font fontDescriptor];
    NSMutableDictionary<UIFontDescriptorAttributeName, id> *attributes = [NSMutableDictionary new];
    if (forceApplyItalic) {
      // A hack way to set italic, the proper way is to use font with italic (but italic for chinese
      // is not work on most case). Custom font always handle the italic with ttf or otf but if
      // there are no avaiable font for italic, we will still use this hack way to apply italic for
      // font.
      CGAffineTransform matrix =
          CGAffineTransformMake(1, 0, tanf(15 * (CGFloat)M_PI / 180), 1, 0, 0);
      [attributes setObject:[NSValue valueWithCGAffineTransform:matrix]
                     forKey:UIFontDescriptorMatrixAttribute];
    }
    if (forceApplyBold) {
      UIFontDescriptorSymbolicTraits symbolicTraits = fontDescriptor.symbolicTraits;
      symbolicTraits |= UIFontDescriptorTraitBold;
      [attributes setObject:@{UIFontSymbolicTrait : @(symbolicTraits)}
                     forKey:UIFontDescriptorTraitsAttribute];
    }
    if (!info.isSystemFont) {
      // Font name need to reset here or system font will be apply
      [attributes setObject:font.fontName forKey:UIFontDescriptorNameAttribute];
    }
    fontDescriptor = [fontDescriptor fontDescriptorByAddingAttributes:attributes];
    font = [UIFont fontWithDescriptor:fontDescriptor size:fontSize];
  }

  return font;
}

- (bool)addFontAliasForFamilyName:(NSString *)name {
  return false;
}

- (void)reportResourceError:(LynxError *)error
               withLynxView:(UIView<LUIBodyView> *)lynxView
                resourceUrl:(NSString *)resourceUrl {
  [error addCustomInfo:@"font" forKey:LynxErrorKeyResourceType];
  [error addCustomInfo:resourceUrl forKey:LynxErrorKeyResourceUrl];
  [lynxView reportLynxError:error];
}

- (UIFont *)getRegisteredUIFont:(NSString *)familyName fontSize:(CGFloat)fontSize {
  // try to get the registered font-face
  UIFont *fontByAlias = nil;
  /* cSpell:disable */
  @synchronized(_registedAliasFontMap) {
    LynxAliasFontInfo *aliasInfo = [_registedAliasFontMap objectForKey:familyName];
    if (aliasInfo != nil) {
      fontByAlias = aliasInfo.font;
    }
  }
  /* cSpell:enable  */

  if (fontByAlias != nil) {
    // get registered font
    fontByAlias = [fontByAlias fontWithSize:fontSize];
  }

  return fontByAlias;
}

@end
