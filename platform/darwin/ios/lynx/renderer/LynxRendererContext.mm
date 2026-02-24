// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRendererContext.h>

@implementation LynxRendererContext {
  NSMutableDictionary<NSNumber *, LynxImageManager *> *_imageManagers;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _imageManagers = [NSMutableDictionary new];
  }
  return self;
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

@end
