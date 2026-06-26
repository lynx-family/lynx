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
  NSMutableDictionary<NSNumber *, NSNumber *> *_ownerImageResources;
  NSMutableDictionary<NSNumber *, NSNumber *> *_imageResourceOwners;
  NSMutableDictionary<NSNumber *, NSValue *> *_textBundles;
  NSMutableDictionary<NSNumber *, NSNumber *> *_ownerTextResources;
  NSMutableDictionary<NSNumber *, NSNumber *> *_textResourceOwners;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _imageManagers = [NSMutableDictionary new];
    _ownerImageResources = [NSMutableDictionary new];
    _imageResourceOwners = [NSMutableDictionary new];
    _textBundles = [NSMutableDictionary new];
    _ownerTextResources = [NSMutableDictionary new];
    _textResourceOwners = [NSMutableDictionary new];
  }
  return self;
}

- (void)dealloc {
  for (LynxImageManager *imageManager in _imageManagers.objectEnumerator) {
    [imageManager reset];
  }
  for (NSValue *value in _textBundles.objectEnumerator) {
    DestroyTextBundlePointer([value pointerValue]);
  }
}

- (void)createImageManager:(int32_t)imageManagerID
                   ownerID:(int32_t)ownerID
             withSourceURL:(LynxURL *)sourceURL
         andPlaceholderURL:(LynxURL *)placeholderURL
                 eventMask:(int32_t)eventMask {
  LynxImageManager *imageManager = [[LynxImageManager alloc] initWithContext:_uiContext];
  [imageManager setSign:ownerID];
  [imageManager setEventMask:eventMask];
  [imageManager requestImage:sourceURL withType:LynxImageRequestSrc];
  [imageManager requestImage:sourceURL withType:LynxImageRequestPlaceholder];
  @synchronized(self) {
    _imageManagers[@(imageManagerID)] = imageManager;
    _imageResourceOwners[@(imageManagerID)] = @(ownerID);
    _ownerImageResources[@(ownerID)] = @(imageManagerID);
  }
}

- (LynxImageManager *)getImageManager:(int32_t)imageManagerID {
  LynxImageManager *imageManager = nil;
  @synchronized(self) {
    imageManager = _imageManagers[@(imageManagerID)];
  }
  return imageManager;
}

- (void)releaseImageManager:(int32_t)imageManagerID {
  @synchronized(self) {
    NSNumber *key = @(imageManagerID);
    NSNumber *ownerID = _imageResourceOwners[key];
    if (ownerID != nil && [_ownerImageResources[ownerID] isEqualToNumber:key]) {
      [_ownerImageResources removeObjectForKey:ownerID];
    }
    [_imageManagers[key] reset];
    [_imageResourceOwners removeObjectForKey:key];
    [_imageManagers removeObjectForKey:key];
  }
}

- (void)createTextResource:(int32_t)textID ownerID:(int32_t)ownerID withBundle:(void *)bundle {
  @synchronized(self) {
    _textResourceOwners[@(textID)] = @(ownerID);
    _ownerTextResources[@(ownerID)] = @(textID);
    _textBundles[@(textID)] = [NSValue valueWithPointer:bundle];
  }
}

- (void)releaseTextResource:(int32_t)textID {
  void *bundle = nullptr;
  @synchronized(self) {
    NSNumber *key = @(textID);
    NSNumber *ownerID = _textResourceOwners[key];
    if (ownerID != nil && [_ownerTextResources[ownerID] isEqualToNumber:key]) {
      [_ownerTextResources removeObjectForKey:ownerID];
    }
    [_textResourceOwners removeObjectForKey:key];
    NSValue *value = _textBundles[key];
    if (value != nil) {
      bundle = [value pointerValue];
      [_textBundles removeObjectForKey:key];
    }
  }
  DestroyTextBundlePointer(bundle);
}

- (void *)getTextBundle:(int32_t)textID {
  void *bundle = nullptr;
  @synchronized(self) {
    NSNumber *key = @(textID);
    NSValue *value = _textBundles[key];
    if (value == nil) {
      NSNumber *resourceID = _ownerTextResources[key];
      value = resourceID != nil ? _textBundles[resourceID] : nil;
    }
    if (value) {
      bundle = [value pointerValue];
    }
  }
  return bundle;
}

@end
