// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBlurImageProcessor.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxEventDetail.h>
#import <Lynx/LynxImageLoader.h>
#import <Lynx/LynxImageManager.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUnitUtils.h>
#include "core/renderer/ui_wrapper/painting/paint_image.h"

static const NSInteger kFlagImageLoadEvent = 1 << 0;
static const NSInteger kFlagImageErrorEvent = 1 << 1;
static NSString* const kLynxImageEventLoad = @"load";
static NSString* const kLynxImageEventError = @"error";

using lynx::tasm::ImageFitMode;
using lynx::tasm::ImagePaintInfo;

namespace {

UIViewContentMode LynxImageContentModeFromMode(int32_t mode) {
  switch (static_cast<ImageFitMode>(mode)) {
    case ImageFitMode::kAspectFit:
      return UIViewContentModeScaleAspectFit;
    case ImageFitMode::kAspectFill:
      return UIViewContentModeScaleAspectFill;
    case ImageFitMode::kCenter:
      return UIViewContentModeCenter;
    case ImageFitMode::kScaleToFill:
    default:
      return UIViewContentModeScaleToFill;
  }
}

CGFloat LynxImageBlurRadiusFromProp(NSString* value, LynxUIContext* context) {
  if (![value isKindOfClass:NSString.class] || value.length == 0) {
    return 0;
  }
  return [LynxUnitUtils toPtWithScreenMetrics:context.screenMetrics
                                    unitValue:value
                                 rootFontSize:0
                                  curFontSize:0
                                    rootWidth:0
                                   rootHeight:0
                                withDefaultPt:0];
}
}  // namespace

@implementation LynxImageManager {
  NSMutableDictionary<id, dispatch_block_t>* _cancelBlocks;
  NSMutableDictionary<id, UIImage*>* _images;

  __weak UIImageView* _imageView;
  __weak LynxUIContext* _context;
  float _viewWidth;
  float _viewHeight;
  NSInteger _sign;
  NSInteger _eventMask;
  UIViewContentMode _contentMode;
  CGFloat _blurRadius;
}

- (instancetype)initWithContext:(LynxUIContext*)context {
  self = [super init];
  if (self) {
    _context = context;
    _sign = -1;
    _contentMode = UIViewContentModeScaleToFill;
    _cancelBlocks = [NSMutableDictionary new];
    _images = [NSMutableDictionary new];
    _blurRadius = 0;
  }
  return self;
}

- (void)setSign:(NSInteger)sign {
  _sign = sign;
}

- (void)setEventMask:(NSInteger)eventMask {
  _eventMask = eventMask;
}

- (void)updatePaintInfo:(const ImagePaintInfo&)paintInfo {
  _contentMode = LynxImageContentModeFromMode(static_cast<int32_t>(paintInfo.mode));
  NSString* blurRadius = paintInfo.blur_radius.empty()
                             ? nil
                             : [[NSString alloc] initWithUTF8String:paintInfo.blur_radius.c_str()];
  _blurRadius = LynxImageBlurRadiusFromProp(blurRadius, _context);
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

  NSMutableArray* processors = [NSMutableArray new];
  if (_blurRadius > 0) {
    LynxBlurImageProcessor* blurProcessor =
        [[LynxBlurImageProcessor alloc] initWithBlurRadius:_blurRadius];
    if ([LynxEnv.sharedInstance enableImageCIGaussianBlur]) {
      [blurProcessor setUseCIGaussianBlur:YES];
    }
    [processors addObject:blurProcessor];
  }
  options.processors = processors;
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
  _imageView.contentMode = _contentMode;

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
