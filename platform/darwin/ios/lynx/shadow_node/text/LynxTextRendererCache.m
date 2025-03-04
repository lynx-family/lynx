// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTextRendererCache.h"
#import "LynxEnv.h"
#import "LynxEventReporter.h"

#pragma mark - Helper Functino

#define LYNX_EPSILON 0.0001
#define TOTAL_COST_LIMIT (10 * 1024)  // 10K charaters for global Text Render
NSString *const kLynxSDKTextRenderCacheEvent = @"lynxsdk_text_render_cache_hit";

static bool compareNearlyEqual(CGFloat a, CGFloat b) {
  float epsilon;
  if (a == b) return true;
  if (a > b) {
    epsilon = a * LYNX_EPSILON;
  } else {
    epsilon = b * LYNX_EPSILON;
  }
  return fabs(a - b) < epsilon;
}

extern BOOL layoutManagerIsTruncated(NSLayoutManager *layoutManager) {
  NSTextContainer *container = layoutManager.textContainers.firstObject;
  NSUInteger numberOfGlyphs = [layoutManager numberOfGlyphs];
  __block NSRange truncatedRange;
  __block NSUInteger maxRange = NSMaxRange([layoutManager glyphRangeForTextContainer:container]);
  [layoutManager enumerateLineFragmentsForGlyphRange:NSMakeRange(0, numberOfGlyphs)
                                          usingBlock:^(CGRect rect, CGRect usedRect,
                                                       NSTextContainer *_Nonnull textContainer,
                                                       NSRange glyphRange, BOOL *_Nonnull stop) {
                                            truncatedRange = [layoutManager
                                                truncatedGlyphRangeInLineFragmentForGlyphAtIndex:
                                                    glyphRange.location];
                                            if (truncatedRange.location != NSNotFound) {
                                              maxRange = truncatedRange.location;
                                              *stop = YES;
                                            }
                                          }];
  return numberOfGlyphs > maxRange;
}

static BOOL layoutManagerIsSingleLine(NSLayoutManager *layoutManager) {
  NSUInteger index = 0;
  NSUInteger numberOfGlyphs = [layoutManager numberOfGlyphs];
  NSRange lineRange;
  [layoutManager lineFragmentRectForGlyphAtIndex:index effectiveRange:&lineRange];
  return NSMaxRange(lineRange) == numberOfGlyphs;
}

static NSUInteger layoutManagerLineCount(NSLayoutManager *layoutManager) {
  NSUInteger numberOfLines, index, numberOfGlyphs = [layoutManager numberOfGlyphs];
  NSRange lineRange;
  for (numberOfLines = 0, index = 0; index < numberOfGlyphs; numberOfLines++) {
    (void)[layoutManager lineFragmentRectForGlyphAtIndex:index effectiveRange:&lineRange];
    index = NSMaxRange(lineRange);
  }
  return numberOfLines;
}

#pragma mark - LynxTextRendererKey

@interface LynxTextRendererKey : NSObject

- (instancetype)initWithAttributedString:(NSAttributedString *)attrStr
                              layoutSpec:(LynxLayoutSpec *)spec;

@end

@implementation LynxTextRendererKey {
  NSAttributedString *_attrStr;
  LynxLayoutSpec *_layoutSpec;
  NSUInteger _hashValue;
}

- (instancetype)initWithAttributedString:(NSAttributedString *)attrStr
                              layoutSpec:(LynxLayoutSpec *)spec {
  if (self = [super init]) {
    _attrStr = attrStr;
    _layoutSpec = spec;
    _hashValue = [attrStr hash] ^ [_layoutSpec hash];
  }
  return self;
}

- (BOOL)isEqual:(LynxTextRendererKey *)object {
  if (self == object) {
    return YES;
  }
  if (object == nil) {
    return NO;
  }
  return _hashValue == object->_hashValue &&
         [_attrStr isEqualToAttributedString:object->_attrStr] &&
         [_layoutSpec isEqualToSpec:object->_layoutSpec];
}

- (NSUInteger)hash {
  return _hashValue;
}

@end

#pragma mark - Cache
@interface LynxTextRendererCache ()
@property(nonatomic) NSInteger hitCount;
@property(nonatomic) NSInteger missCount;
@property(nonatomic, strong) dispatch_source_t timer;
@end

@implementation LynxTextRendererCache {
  NSCache<LynxTextRendererKey *, LynxTextRenderer *> *_cache;
  NSMutableDictionary<NSAttributedString *, NSMutableArray<LynxTextRenderer *> *>
      *_attrStringRenderers;
}

+ (instancetype)cache {
  static dispatch_once_t onceToken;
  static LynxTextRendererCache *cache = nil;
  dispatch_once(&onceToken, ^{
    cache = [[LynxTextRendererCache alloc] init];
  });
  return cache;
}

- (instancetype)init {
  if (self = [super init]) {
    _cache = [[NSCache alloc] init];
    _cache.totalCostLimit = [self totalCostLimitFromSettings];
    _cache.delegate = self;
    _attrStringRenderers = [[NSMutableDictionary alloc] init];
    [self startTimeCounter];
  }
  return self;
}

- (void)cache:(NSCache *)cache willEvictObject:(id)obj {
}

- (void)clearCache {
  [_attrStringRenderers removeAllObjects];
  [_cache removeAllObjects];
}

- (LynxTextRenderer *)_suitableRendererWithString:(NSAttributedString *)str
                                       layoutSpec:(LynxLayoutSpec *)spec {
  BOOL widthUndifined = spec.widthMode == LynxMeasureModeIndefinite;
  BOOL heightUndifined = spec.heightMode == LynxMeasureModeIndefinite;
  for (LynxTextRenderer *renderer in [_attrStringRenderers objectForKey:str]) {
    NSLayoutManager *layoutManager = renderer.layoutManager;
    NSTextContainer *container = layoutManager.textContainers.firstObject;
    CGSize textSize = [layoutManager usedRectForTextContainer:container].size;
    LynxLayoutSpec *storedSpec = renderer.layoutSpec;
    if ([spec isEqualToSpec:storedSpec]) {
      return renderer;
    }
    // If two renderer have different LynxBackgroundGradient, we cannot reuse them.
    if (!LynxSameLynxGradient(storedSpec.textStyle.textGradient, spec.textStyle.textGradient)) {
      continue;
    }
    if (storedSpec.textOverflow != spec.textOverflow) continue;
    if (layoutManagerIsSingleLine(layoutManager)) {
      if (([storedSpec widthUndifined] || !layoutManagerIsTruncated(layoutManager)) &&
          (widthUndifined || textSize.width <= spec.width + LYNX_EPSILON) &&
          (!widthUndifined && ABS(spec.width - storedSpec.width) < LYNX_EPSILON) &&
          ((heightUndifined && storedSpec.heightUndifined) ||
           (spec.height <= storedSpec.height + LYNX_EPSILON))) {
        return renderer;
      }
    } else {
      if (!LynxSameMeasureMode(spec.widthMode, storedSpec.widthMode) ||
          !compareNearlyEqual(spec.width, storedSpec.width) ||
          spec.maxLineNum != storedSpec.maxLineNum) {
        continue;
      }
      if ((NSUInteger)spec.maxLineNum == layoutManagerLineCount(layoutManager)) {
        continue;
      }
      if (((heightUndifined && storedSpec.heightUndifined) ||
           compareNearlyEqual(spec.height, storedSpec.height)) &&
          (storedSpec.whiteSpace == spec.whiteSpace)) {
        return renderer;
      } else {
        continue;
      }
    }
  }
  return nil;
}

- (BOOL)isEnableCache:(LynxLayoutSpec *)spec {
  return !spec.textStyle.isAutoFontSize;
}

- (LynxTextRenderer *)rendererWithString:(NSAttributedString *)str
                              layoutSpec:(LynxLayoutSpec *)spec {
  if (str == nil) return nil;

  LynxTextRendererKey *key = [[LynxTextRendererKey alloc] initWithAttributedString:str
                                                                        layoutSpec:spec];
  LynxTextRenderer *renderer = [_cache objectForKey:key];
  if (renderer == nil) {
    renderer = [[LynxTextRenderer alloc] initWithAttributedString:str layoutSpec:spec];
    [renderer ensureTextRenderLayout];
    // https://developer.apple.com/documentation/foundation/nscache/1407672-totalcostlimit?language=objc
    // to make LRU remove effect, must pass `cost` parameter
    if ([self isEnableCache:spec]) {
      if (![NSThread isMainThread]) {
        // Ensure cache.setObj will be called by the main thread, to avoid render being release by
        // mistake.
        LynxTextRendererCache *__weak weakSelf = self;
        dispatch_async(dispatch_get_main_queue(), ^{
          LynxTextRendererCache *strongSelf = weakSelf;
          if (strongSelf == nil) {
            return;
          }
          strongSelf->_missCount++;
          [strongSelf->_cache
              setObject:renderer
                 forKey:key
                   cost:[[str string] maximumLengthOfBytesUsingEncoding:NSUTF8StringEncoding]];
        });
      } else {
        self.missCount++;
        [_cache setObject:renderer
                   forKey:key
                     cost:[[str string] maximumLengthOfBytesUsingEncoding:NSUTF8StringEncoding]];
      }
    }

  } else {
    if (![NSThread isMainThread]) {
      dispatch_async(dispatch_get_main_queue(), ^{
        self.hitCount++;
      });
    } else {
      self.hitCount++;
    }
  }
  return renderer;
}

#pragma mark - settings & events
- (NSUInteger)totalCostLimitFromSettings {
  NSString *cacheLimitStr =
      [[LynxEnv sharedInstance] stringFromExternalEnv:LynxEnvTextRenderCacheLimit];
  if (cacheLimitStr && cacheLimitStr.length > 0 && !isnan([cacheLimitStr integerValue])) {
    NSInteger cacheLimit = [cacheLimitStr integerValue];
    // 10000 >= cacheLimit >= 10
    cacheLimit = MIN(10000, MAX(10, cacheLimit));
    return cacheLimit * 1024;
  }
  return TOTAL_COST_LIMIT;
}

- (void)startTimeCounter {
  BOOL enableCacheHitRate =
      [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableTextRenderCacheHitRate
                                       defaultValue:NO];
  if (!enableCacheHitRate) {
    return;
  }
  _timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
  dispatch_source_set_timer(_timer, dispatch_walltime(NULL, 0), 20 * NSEC_PER_SEC, 0);
  __weak typeof(self) weakSelf = self;
  dispatch_source_set_event_handler(_timer, ^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf.hitCount > 0 || strongSelf.missCount > 0) {
      [LynxEventReporter onEvent:kLynxSDKTextRenderCacheEvent
                      instanceId:kUnknownInstanceId
                           props:@{
                             @"hitCount" : @(strongSelf.hitCount),
                             @"missCount" : @(strongSelf.missCount)
                           }];
      strongSelf.hitCount = 0;
      strongSelf.missCount = 0;
    }
  });
  dispatch_resume(self.timer);
}
@end
