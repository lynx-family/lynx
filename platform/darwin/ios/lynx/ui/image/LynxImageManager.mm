// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEventDetail.h>
#import <Lynx/LynxImageLoader.h>
#import <Lynx/LynxImageManager.h>
#import <Lynx/LynxUIContext.h>

static const NSInteger kFlagImageLoadEvent = 1 << 0;
static const NSInteger kFlagImageErrorEvent = 1 << 1;
static NSString* const kLynxImageEventLoad = @"load";
static NSString* const kLynxImageEventError = @"error";

@implementation LynxImageManager {
  NSMutableDictionary<id, dispatch_block_t>* _cancelBlocks;
  NSMutableDictionary<id, UIImage*>* _images;

  __weak UIImageView* _imageView;
  __weak LynxUIContext* _context;
  float _viewWidth;
  float _viewHeight;
  NSInteger _sign;
  NSInteger _eventMask;
}

- (instancetype)initWithContext:(LynxUIContext*)context {
  self = [super init];
  if (self) {
    _context = context;
    _sign = -1;
    _cancelBlocks = [NSMutableDictionary new];
    _images = [NSMutableDictionary new];
  }
  return self;
}

- (void)setSign:(NSInteger)sign {
  _sign = sign;
}

- (void)setEventMask:(NSInteger)eventMask {
  _eventMask = eventMask;
}

- (void)sendCustomEvent:(NSString*)name withParams:(NSDictionary*)params {
  if (_context != nil && _sign >= 0) {
    LynxDetailEvent* event = [[LynxDetailEvent alloc] initWithName:name
                                                        targetSign:_sign
                                                            detail:params];
    [_context.eventEmitter sendCustomEvent:event];
  }
}

- (void)requestImage:(LynxURL*)imageURL withType:(LynxImageRequestType)type {
  if (_cancelBlocks[@(type)]) {
    _cancelBlocks[@(type)]();
    _cancelBlocks[@(type)] = nil;
  }

  _viewWidth = imageURL.imageSize.width;
  _viewHeight = imageURL.imageSize.height;

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
    if (type != LynxImageRequestSrc) {
      return;
    }

    if (error != nil) {
      BOOL should_emit_error = (self->_eventMask & kFlagImageErrorEvent) != 0;
      if (should_emit_error) {
        [self sendCustomEvent:kLynxImageEventError
                   withParams:@{
                     @"errMsg" : error.localizedDescription,
                     @"error_code" : @(0),
                     @"lynx_categorized_code" : @(0),
                   }];
      }
      return;
    }

    BOOL should_emit_load = (self->_eventMask & kFlagImageLoadEvent) != 0;
    if (image != nil && should_emit_load) {
      [self sendCustomEvent:kLynxImageEventLoad
                 withParams:@{
                   @"width" : @(image.size.width),
                   @"height" : @(image.size.height),
                 }];
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
