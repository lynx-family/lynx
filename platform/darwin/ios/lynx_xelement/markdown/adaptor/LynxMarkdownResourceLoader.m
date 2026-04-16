// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxMarkdownResourceLoader.h"

#import <Lynx/LynxCSSType.h>
#import <Lynx/LynxImageFetcher.h>
#import <Lynx/LynxImageLoader.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxNativeLayoutNode.h>
#import <Lynx/LynxURL.h>

#import "LynxMarkdownInlineViewHandle.h"

static UIFontWeight LynxMarkdownToUIFontWeight(NSInteger weight) {
  if (weight >= 900) {
    return UIFontWeightBlack;
  }
  if (weight >= 800) {
    return UIFontWeightHeavy;
  }
  if (weight >= 700) {
    return UIFontWeightBold;
  }
  if (weight >= 600) {
    return UIFontWeightSemibold;
  }
  if (weight >= 500) {
    return UIFontWeightMedium;
  }
  if (weight >= 300) {
    return UIFontWeightLight;
  }
  if (weight >= 200) {
    return UIFontWeightThin;
  }
  if (weight >= 100) {
    return UIFontWeightUltraLight;
  }
  return UIFontWeightRegular;
}

static LynxFontStyleType LynxMarkdownToFontStyle(NSInteger style) {
  if (style == 1) {
    return LynxFontStyleItalic;
  }
  return LynxFontStyleNormal;
}

@interface LynxMarkdownResourceLoader ()
- (void)onFontFaceLoad:(NSString *)family Weight:(int)weight Style:(int)style;
@end

@interface FontFaceListener : NSObject <LynxFontFaceObserver>
@property(nonatomic) NSString *family;
@property(nonatomic) int weight;
@property(nonatomic) int style;
@property(nonatomic, weak) LynxMarkdownResourceLoader *loader;
@end

@implementation FontFaceListener

- (void)onFontFaceLoad {
  __strong LynxMarkdownResourceLoader *loader = self.loader;
  if (loader != nil) {
    [loader onFontFaceLoad:self.family Weight:self.weight Style:self.style];
  }
}

@end

@implementation LynxMarkdownResourceLoader {
  __weak id<LynxMarkdownResourceLoaderHost> _host;
  NSMutableDictionary<NSString *, id> *_imageCache;
  NSMutableDictionary<NSString *, LynxMarkdownInlineViewHandle *> *_inlineViewCache;
  NSMutableDictionary<NSString *, UIFont *> *_fontCache;
}

- (instancetype)initWithHost:(id<LynxMarkdownResourceLoaderHost>)host {
  self = [super init];
  if (self != nil) {
    _host = host;
    _enableImageDownSampling = NO;
    _imageCache = [NSMutableDictionary dictionary];
    _inlineViewCache = [NSMutableDictionary dictionary];
    _fontCache = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)releaseResources {
  @synchronized(self) {
    [_imageCache removeAllObjects];
    [_inlineViewCache removeAllObjects];
    [_fontCache removeAllObjects];
  }
}

- (UIImage *)loadImageByURL:(NSString *)url {
  if (url == nil || url.length == 0) {
    return nil;
  }

  @synchronized(self) {
    id cached = _imageCache[url];
    if ([cached isKindOfClass:[UIImage class]]) {
      return (UIImage *)cached;
    }
    if (cached != nil) {
      return nil;
    }
    _imageCache[url] = [NSNull null];
  }

  NSURL *nsURL = [NSURL URLWithString:url];
  if (nsURL == nil || nsURL.scheme.length == 0) {
    @synchronized(self) {
      [_imageCache removeObjectForKey:url];
    }
    return nil;
  }

  LynxURL *requestURL = [[LynxURL alloc] init];
  requestURL.url = nsURL;
  NSMutableDictionary *contextInfo = [NSMutableDictionary dictionary];
  if (self.enableImageDownSampling) {
    contextInfo[LynxImageFetcherContextKeyDownsampling] = @(YES);
  }
  contextInfo[LynxImagePreloadAllFrames] = @(NO);

  __weak typeof(self) weakSelf = self;
  [[LynxImageLoader sharedInstance]
      loadImageFromLynxURL:requestURL
                      size:CGSizeZero
               contextInfo:[contextInfo copy]
                processors:nil
              imageFetcher:[self imageFetcher]
               LynxUIImage:nil
      enableGenericFetcher:NO
                 completed:^(UIImage *_Nullable image, NSError *_Nullable error,
                             NSURL *_Nullable imageURL) {
                   __strong typeof(weakSelf) strongSelf = weakSelf;
                   if (strongSelf == nil) {
                     return;
                   }
                   BOOL shouldRequestLayout = NO;
                   @synchronized(strongSelf) {
                     if (image != nil) {
                       strongSelf->_imageCache[url] = image;
                       shouldRequestLayout = YES;
                     } else {
                       [strongSelf->_imageCache removeObjectForKey:url];
                       LLogError(@"LynxMarkdown image load failed: %@, %@", url, error);
                     }
                   }
                   if (shouldRequestLayout) {
                     [strongSelf onImageLoaded:url];
                   }
                 }];

  @synchronized(self) {
    id cached = _imageCache[url];
    if ([cached isKindOfClass:[UIImage class]]) {
      return (UIImage *)cached;
    }
  }
  return nil;
}

- (UIFont *)loadFontByFamilyName:(NSString *)family Weight:(int)weight Style:(int)style {
  NSString *familyName = family == nil ? @"" : family;
  NSString *fontKey = [NSString stringWithFormat:@"%@_%d_%d", familyName, weight, style];
  @synchronized(self) {
    UIFont *cachedFont = _fontCache[fontKey];
    if (cachedFont != nil) {
      return cachedFont;
    }
  }
  FontFaceListener *observer = [[FontFaceListener alloc] init];
  observer.family = family;
  observer.weight = weight;
  observer.style = style;
  observer.loader = self;
  LynxUIOwner *uiOwner = [_host markdownHostUIOwner];
  UIFont *font =
      [[LynxFontFaceManager sharedManager] generateFontWithSize:16.f
                                                         weight:LynxMarkdownToUIFontWeight(weight)
                                                          style:LynxMarkdownToFontStyle(style)
                                                 fontFamilyName:familyName
                                                fontFaceContext:uiOwner.fontFaceContext
                                               fontFaceObserver:observer];

  if (font != nil) {
    @synchronized(self) {
      _fontCache[fontKey] = font;
    }
  }
  return font;
}

- (id<IMarkdownPlatformViewHandle>)loadInlineView:(NSString *)idSelector {
  if (idSelector == nil || idSelector.length == 0) {
    return nil;
  }

  @synchronized(self) {
    LynxMarkdownInlineViewHandle *cachedHandle = _inlineViewCache[idSelector];
    if (cachedHandle != nil) {
      return cachedHandle;
    }
  }

  NSArray<LynxShadowNode *> *children = [_host markdownHostChildren];
  LynxNativeLayoutNode *targetNode = nil;
  for (LynxShadowNode *child in children) {
    if (![child isKindOfClass:[LynxNativeLayoutNode class]]) {
      continue;
    }
    LynxNativeLayoutNode *layoutNode = (LynxNativeLayoutNode *)child;
    if ([layoutNode.idSelector isEqualToString:idSelector]) {
      targetNode = layoutNode;
      break;
    }
  }
  if (targetNode == nil) {
    return nil;
  }

  LynxMarkdownInlineViewHandle *handle =
      [[LynxMarkdownInlineViewHandle alloc] initWithLayoutNode:targetNode host:_host];
  @synchronized(self) {
    _inlineViewCache[idSelector] = handle;
  }
  return handle;
}

- (void)onFontFaceLoad:(NSString *)family Weight:(int)weight Style:(int)style {
  @synchronized(self) {
    [_fontCache removeAllObjects];
  }
  id<LynxMarkdownResourceLoaderHost> host = _host;
  if (host == nil || [host markdownHostDestroyed]) {
    return;
  }
  [host onFontLoaded:family Weight:weight Style:style];
}

- (void)onImageLoaded:(NSString *)url {
  id<LynxMarkdownResourceLoaderHost> host = _host;
  if (host == nil || [host markdownHostDestroyed]) {
    return;
  }
  [host onImageLoaded:url];
}

- (id<LynxImageFetcher>)imageFetcher {
  LynxUIOwner *uiOwner = [_host markdownHostUIOwner];
  return uiOwner.uiContext.imageFetcher;
}

@end
