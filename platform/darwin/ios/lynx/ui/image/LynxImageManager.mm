// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxImageLoader.h>
#import <Lynx/LynxImageManager.h>

@implementation LynxImageManager {
  NSMutableDictionary<id, dispatch_block_t>* _cancelBlocks;
  NSMutableDictionary<id, UIImage*>* _images;

  __weak UIImageView* _imageView;
  __weak LynxUIContext* _context;
}

- (instancetype)initWithContext:(LynxUIContext*)context {
  self = [super init];
  if (self) {
    _context = context;
    _cancelBlocks = [NSMutableDictionary new];
    _images = [NSMutableDictionary new];
  }
  return self;
}

- (void)requestImage:(LynxURL*)imageURL withType:(LynxImageRequestType)type {
  if (_cancelBlocks[@(type)]) {
    _cancelBlocks[@(type)]();
    _cancelBlocks[@(type)] = nil;
  }

  LynxImageLoadOptions* options = [[LynxImageLoadOptions alloc] init];
  options.imageURL = imageURL;
  options.targetSize = imageURL.imageSize;
  options.fontSize = 0;
  options.context = _context;

  NSMutableDictionary* contextInfo = [NSMutableDictionary new];
  contextInfo[LynxEnableGenericFetcher] = @YES;
  contextInfo[LynxShouldUseImageService] = @YES;
  options.contextInfo = contextInfo;

  options.processors = [NSArray new];
  options.completed = ^(UIImage* image, NSError* error, NSURL* imageURL) {
    self->_images[@(type)] = image;

    // If source image is loaded, placeholder image is not needed to be displayed.
    if (self->_images[@(LynxImageRequestSrc)] != nil && type == LynxImageRequestPlaceholder) {
      return;
    }

    if (self->_imageView != nil) {
      self->_imageView.image = image;
    }
  };

  _cancelBlocks[@(type)] = [[LynxImageLoader sharedInstance] loadImageWithOptions:options];
}

- (void)setTarget:(UIImageView*)view {
  _imageView = view;

  // Try set source image first.
  if (_images[@(LynxImageRequestSrc)] != nil) {
    _imageView.image = _images[@(LynxImageRequestSrc)];
  } else if (_images[@(LynxImageRequestPlaceholder)] != nil) {
    _imageView.image = _images[@(LynxImageRequestPlaceholder)];
  }
}

- (void)reset {
  _imageView = nil;
  [_cancelBlocks enumerateKeysAndObjectsUsingBlock:^(id key, dispatch_block_t block, BOOL* stop) {
    if (block) {
      block();
    }
  }];
  [_cancelBlocks removeAllObjects];
}

@end
