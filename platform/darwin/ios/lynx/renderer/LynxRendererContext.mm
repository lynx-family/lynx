// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceTextProtocol.h>

namespace {

void DestroyTextBundlePointer(void *bundle) {
  if (bundle == nullptr) {
    return;
  }
  id<LynxServiceTextProtocol> textService = LynxService(LynxServiceTextProtocol);
  if (textService != nil) {
    [textService destroyPage:bundle];
  }
}

}  // namespace

@implementation LynxRendererContext {
  NSMutableDictionary<NSNumber *, LynxImageManager *> *_imageManagers;
  NSMutableDictionary<NSNumber *, NSValue *> *_textBundles;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _imageManagers = [NSMutableDictionary new];
    _textBundles = [NSMutableDictionary new];
  }
  return self;
}

- (void)dealloc {
  for (NSValue *value in _textBundles.objectEnumerator) {
    DestroyTextBundlePointer([value pointerValue]);
  }
}

- (void)createImageManager:(int32_t)imageManagerID
             withSourceURL:(LynxURL *)sourceURL
         andPlaceholderURL:(LynxURL *)placeholderURL {
  LynxImageManager *imageManager = [[LynxImageManager alloc] initWithContext:_uiContext];
  [imageManager requestImage:sourceURL withType:LynxImageRequestSrc];
  [imageManager requestImage:sourceURL withType:LynxImageRequestPlaceholder];
  @synchronized(self) {
    _imageManagers[@(imageManagerID)] = imageManager;
  }
}

- (LynxImageManager *)takeImageManager:(int32_t)imageManagerID {
  LynxImageManager *imageManager = nil;
  @synchronized(self) {
    imageManager = _imageManagers[@(imageManagerID)];
    [_imageManagers removeObjectForKey:@(imageManagerID)];
  }
  return imageManager;
}

- (void)updateTextBundle:(int32_t)textID withBundle:(void *)bundle {
  void *previousBundle = nullptr;
  @synchronized(self) {
    NSValue *previousValue = _textBundles[@(textID)];
    if (previousValue != nil) {
      previousBundle = [previousValue pointerValue];
    }
    _textBundles[@(textID)] = [NSValue valueWithPointer:bundle];
  }
  DestroyTextBundlePointer(previousBundle);
}

- (void)destroyTextBundle:(int32_t)textID {
  void *bundle = nullptr;
  @synchronized(self) {
    NSValue *value = _textBundles[@(textID)];
    if (value) {
      bundle = [value pointerValue];
      [_textBundles removeObjectForKey:@(textID)];
    }
  }
  DestroyTextBundlePointer(bundle);
}

- (void *)takeTextBundle:(int32_t)textID {
  void *bundle = nullptr;
  @synchronized(self) {
    NSValue *value = _textBundles[@(textID)];
    if (value) {
      bundle = [value pointerValue];
      [_textBundles removeObjectForKey:@(textID)];
    }
  }
  return bundle;
}

@end
