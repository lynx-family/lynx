// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBlurImageProcessor.h>
#import <Lynx/LynxColorUtils.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxEventDetail.h>
#import <Lynx/LynxImageLoader.h>
#import <Lynx/LynxImageManager.h>
#import <Lynx/LynxNinePatchImageProcessor.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUIImage.h>
#import <Lynx/LynxUnitUtils.h>
#include "core/renderer/ui_wrapper/painting/paint_image.h"

static const NSInteger kFlagImageLoadEvent = 1 << 0;
static const NSInteger kFlagImageErrorEvent = 1 << 1;
static NSString* const kLynxImageEventLoad = @"load";
static NSString* const kLynxImageEventError = @"error";

using lynx::tasm::ImageFitMode;
using lynx::tasm::ImagePaintInfo;

namespace {

NSString* NSStringFromBaseString(const lynx::base::String& value) {
  return [NSString stringWithUTF8String:value.c_str()];
}

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

CGFloat LynxImageLengthFromProp(NSString* value, LynxUIContext* context) {
  if (value.length == 0 || context == nil) {
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

UIEdgeInsets LynxImageCapInsetsFromProp(NSString* value, LynxUIContext* context) {
  if (value.length == 0) {
    return UIEdgeInsetsZero;
  }
  NSArray<NSString*>* values = [value componentsSeparatedByString:@" "];
  NSInteger count = values.count;
  CGFloat top = LynxImageLengthFromProp(count > 0 ? values[0] : nil, context);
  CGFloat right = count > 1 ? LynxImageLengthFromProp(values[1], context) : top;
  CGFloat bottom = count > 2 ? LynxImageLengthFromProp(values[2], context) : top;
  CGFloat left = count > 3 ? LynxImageLengthFromProp(values[3], context) : right;
  return UIEdgeInsetsMake(top, left, bottom, right);
}

constexpr CGFloat kImageAspectRatioTolerance = 0.05;

bool ShouldUpdateAutoSizeLayout(CGSize image_size, CGSize layout_size) {
  if (image_size.width <= 0 || image_size.height <= 0) {
    return false;
  }
  if (layout_size.width <= 0 || layout_size.height <= 0) {
    return true;
  }

  const CGFloat image_ratio = image_size.width / image_size.height;
  const CGFloat layout_ratio = layout_size.width / layout_size.height;
  return std::abs(image_ratio - layout_ratio) > kImageAspectRatioTolerance;
}
}  // namespace

@implementation LynxImageManager {
  NSMutableDictionary<id, dispatch_block_t>* _cancelBlocks;
  NSMutableDictionary<id, UIImage*>* _images;

  __weak UIImageView* _imageView;
  __weak LynxUIContext* _context;
  NSInteger _sign;
  NSInteger _eventMask;
  UIViewContentMode _contentMode;
  CGFloat _blurRadius;
  BOOL _autoSize;
  UIColor* _tintColor;
  UIEdgeInsets _capInsets;
  CGFloat _capInsetsScale;
  BOOL _skipRedirection;
  BOOL _autoPlay;
  NSInteger _loopCount;
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
    _capInsetsScale = 1;
    _autoPlay = YES;
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
  _blurRadius = LynxImageLengthFromProp(NSStringFromBaseString(paintInfo.blur_radius), _context);
  _autoSize = paintInfo.auto_size;
  _tintColor =
      [LynxColorUtils convertNSStringToUIColor:NSStringFromBaseString(paintInfo.tint_color)];
  _capInsets = LynxImageCapInsetsFromProp(NSStringFromBaseString(paintInfo.cap_insets), _context);
  _capInsetsScale = paintInfo.cap_insets_scale;
  _skipRedirection = paintInfo.skip_redirection;
  _autoPlay = paintInfo.autoplay;
  _loopCount = MAX(paintInfo.loop_count, 0);
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

  LynxImageLoadOptions* options = [[LynxImageLoadOptions alloc] init];
  CGSize targetSize = imageURL.imageSize;
  options.imageURL = imageURL;
  options.targetSize = targetSize;
  options.fontSize = 0;
  options.context = _context;

  NSMutableDictionary* contextInfo = [NSMutableDictionary new];
  contextInfo[LynxEnableGenericFetcher] = @YES;
  contextInfo[LynxShouldUseImageService] = @YES;
  contextInfo[LynxImageSkipRedirection] = @(_skipRedirection);
  options.contextInfo = contextInfo;

  NSMutableArray* processors = [NSMutableArray new];
  if (!UIEdgeInsetsEqualToEdgeInsets(_capInsets, UIEdgeInsetsZero)) {
    [processors addObject:[[LynxNinePatchImageProcessor alloc] initWithCapInsets:_capInsets
                                                                  capInsetsScale:_capInsetsScale]];
  }
  if (_blurRadius > 0) {
    LynxBlurImageProcessor* blurProcessor =
        [[LynxBlurImageProcessor alloc] initWithBlurRadius:_blurRadius];
    [processors addObject:blurProcessor];
  }
  options.processors = processors;
  options.completed = ^(UIImage* image, NSError* error, NSURL* imageURL) {
    self->_images[@(type)] = image;

    if (image != nil && type == LynxImageRequestSrc && self->_autoSize &&
        UIEdgeInsetsEqualToEdgeInsets(self->_capInsets, UIEdgeInsetsZero)) {
      CGSize imageSize = image.size;
      if (ShouldUpdateAutoSizeLayout(imageSize, targetSize)) {
        [self->_context findShadowNodeAndRunTask:self->_sign
                                            task:^(LynxShadowNode* node) {
                                              if ([node isKindOfClass:LynxImageShadowNode.class]) {
                                                [(LynxImageShadowNode*)node setImageSize:imageSize];
                                              }
                                            }];
      }
    }

    // If source image is loaded, placeholder image is not needed to be displayed.
    if (self->_images[@(LynxImageRequestSrc)] != nil && type == LynxImageRequestPlaceholder) {
      return;
    }

    if (self->_imageView != nil) {
      [self applyImage:image withType:type];
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

- (void)applyImage:(UIImage*)image withType:(LynxImageRequestType)type {
  if (_imageView == nil) {
    return;
  }
  id<LynxServiceImageProtocol> imageService = [LynxImageLoader imageService];
  BOOL isAnimatedImage =
      image != nil && ([imageService isAnimatedImage:image] || image.images != nil);

  // Configure custom animated image views before assigning the image.
  if (isAnimatedImage) {
    [imageService setAutoPlay:_imageView value:_autoPlay];
  }
  _imageView.image = image;
  if (image != nil && _tintColor != nil && _loopCount == 0) {
    _imageView.image = [_imageView.image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  }
  // Clear stale animation frames when switching to a static image.
  if (![imageService checkImageType:_imageView]) {
    _imageView.animationImages = image.images;
  }
  if (!isAnimatedImage || type != LynxImageRequestSrc) {
    return;
  }
  if (_loopCount > 0) {
    [imageService handleAnimatedImage:image view:_imageView loopCount:_loopCount];
    return;
  }
  if (image.images.count > 1) {
    _imageView.animationDuration = image.duration;
    _imageView.animationRepeatCount = 0;
    if (_autoPlay) {
      [_imageView startAnimating];
    }
  }
}

- (void)setTarget:(UIImageView*)view {
  _imageView = view;
  _imageView.contentMode = _contentMode;
  _imageView.tintColor = _tintColor;

  // Try set source image first.
  if (_images[@(LynxImageRequestSrc)] != nil) {
    [self applyImage:_images[@(LynxImageRequestSrc)] withType:LynxImageRequestSrc];
  } else if (_images[@(LynxImageRequestPlaceholder)] != nil) {
    [self applyImage:_images[@(LynxImageRequestPlaceholder)] withType:LynxImageRequestPlaceholder];
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
