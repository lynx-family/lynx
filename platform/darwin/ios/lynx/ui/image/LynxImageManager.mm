// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBlurImageProcessor.h>
#import <Lynx/LynxColorUtils.h>
#import <Lynx/LynxContext.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxEventDetail.h>
#import <Lynx/LynxImageLoader.h>
#import <Lynx/LynxImageManager.h>
#import <Lynx/LynxNinePatchImageProcessor.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIContext+Internal.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUIImage.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxUnitUtils.h>
#import <QuartzCore/QuartzCore.h>
#include "core/renderer/ui_wrapper/painting/paint_image.h"

static const NSInteger kFlagImageLoadEvent = 1 << 0;
static const NSInteger kFlagImageErrorEvent = 1 << 1;
static const NSInteger kFlagImageStartPlayEvent = 1 << 2;
static const NSInteger kFlagImageCurrentLoopEvent = 1 << 3;
static const NSInteger kFlagImageFinalLoopEvent = 1 << 4;
static const NSInteger kFlagImageLoadStartEvent = 1 << 5;
static NSString* const kBackgroundImageAnimationKey = @"lynx.backgroundImage";
static NSString* const kLynxImageEventLoad = @"load";
static NSString* const kLynxImageEventError = @"error";
static NSString* const kLynxImageEventStartPlay = @"startplay";
static NSString* const kLynxImageEventCurrentLoop = @"currentloopcomplete";
static NSString* const kLynxImageEventFinalLoop = @"finalloopcomplete";

using lynx::tasm::ImageFitMode;
using lynx::tasm::ImagePaintInfo;

namespace {

void CancelImageRequest(dispatch_block_t cancel) {
  if (!cancel) {
    return;
  }
  if ([NSThread isMainThread]) {
    cancel();
  } else {
    dispatch_async(dispatch_get_main_queue(), cancel);
  }
}

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

@interface LynxImageManager ()

- (void)registerAnimatedImageCallbacks:(id<LynxServiceImageProtocol>)imageService;

- (BOOL)invokeUIMethod:(NSString*)method
                params:(NSDictionary*)params
              callback:(LynxUIMethodCallbackBlock)callback;

@end

@implementation LynxImageManager {
  NSMutableDictionary<id, dispatch_block_t>* _cancelBlocks;
  NSUInteger _requestGenerations[LynxImageRequestPlaceholder + 1];
  NSMutableDictionary<id, UIImage*>* _images;
  CGSize _prefetchSize;
  BOOL _downsampling;
  BOOL _isBackground;
  BOOL _resizeReuseDisabled;
  NSString* _requestPriority;

  __weak UIImageView* _imageView;
  __weak CALayer* _imageLayer;
  void (^_imageSizeDidChange)(CGSize);
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

- (void)dealloc {
  if (_cancelBlocks.count == 0) {
    return;
  }
  NSArray<dispatch_block_t>* cancelBlocks = _cancelBlocks.allValues;
  CancelImageRequest(^{
    for (dispatch_block_t cancel in cancelBlocks) {
      cancel();
    }
  });
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
  _isBackground = paintInfo.is_background;
  _downsampling = paintInfo.downsampling;
  _requestPriority =
      paintInfo.request_priority.empty() ? nil : NSStringFromBaseString(paintInfo.request_priority);
  _prefetchSize = CGSizeMake(
      paintInfo.prefetch_width.empty()
          ? 0
          : LynxImageLengthFromProp(NSStringFromBaseString(paintInfo.prefetch_width), _context),
      paintInfo.prefetch_height.empty()
          ? 0
          : LynxImageLengthFromProp(NSStringFromBaseString(paintInfo.prefetch_height), _context));
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

- (BOOL)canReuseOnResize {
  LynxUIContext* context = _context;
  if (context.imageFetcher != nil || context.mediaResourceFetcher != nil ||
      context.lynxContext.hasCustomMediaFetcher) {
    return NO;
  }
  @synchronized(self) {
    return !_resizeReuseDisabled;
  }
}

- (void)requestImage:(LynxURL*)imageURL withType:(LynxImageRequestType)type {
  // Start loading on the caller's thread. Only request ownership is shared;
  // never invoke the loader or cancellation while holding this lock.
  NSUInteger requestGeneration;
  dispatch_block_t cancel;
  @synchronized(self) {
    // Invalidate before cancelling: cancellation may complete synchronously.
    requestGeneration = ++_requestGenerations[type];
    cancel = _cancelBlocks[@(type)];
    _cancelBlocks[@(type)] = nil;
  }
  CancelImageRequest(cancel);
  if (imageURL.url == nil || imageURL.url.absoluteString.length == 0) {
    return;
  }

  const CGSize layoutSize = imageURL.imageSize;
  CGSize targetSize = layoutSize;
  if (!_isBackground) {
    // A single resolved dimension is enough for auto-size. Prefetch may start
    // ordinary requests before layout, but cannot resolve a 0x0 auto-size image.
    if (targetSize.width <= 0 && targetSize.height <= 0 &&
        (_autoSize || (_prefetchSize.width <= 0 && _prefetchSize.height <= 0))) {
      return;
    }
    if ((targetSize.width <= 0 || targetSize.height <= 0) && _prefetchSize.width > 0 &&
        _prefetchSize.height > 0) {
      targetSize = _prefetchSize;
    }
  }
  LynxImageLoadOptions* options = [[LynxImageLoadOptions alloc] init];
  options.imageURL = imageURL;
  options.targetSize = targetSize;
  options.fontSize = 0;
  options.context = _context;

  NSMutableDictionary* contextInfo = [NSMutableDictionary new];
  BOOL enableGenericFetcher = _context.mediaResourceFetcher != nil;
  BOOL shouldUseImageService =
      enableGenericFetcher || _context.trailUseNewImage || _context.imageFetcher == nil;
  contextInfo[LynxEnableGenericFetcher] = @(enableGenericFetcher);
  contextInfo[LynxShouldUseImageService] = @(shouldUseImageService);
  // Preserve direct loading when no media fetcher is available, including when
  // the context hides an injected fetcher. Keep legacy fetcher options unchanged.
  contextInfo[LynxImageSkipRedirection] =
      @(_skipRedirection || (shouldUseImageService && !enableGenericFetcher));
  contextInfo[LynxImageCancelRequest] = @YES;
  // CALayer background animations need decoded frames; image views stream them.
  contextInfo[LynxImagePreloadAllFrames] = @(_isBackground);
  if (!_isBackground) {
    if (_downsampling && !_autoSize) {
      contextInfo[LynxImageFetcherContextKeyDownsampling] = @YES;
      contextInfo[LynxImageFixNewImageDownsampling] = @(_context.fixNewImageDownSampling);
    }
    if (_requestPriority != nil) {
      contextInfo[LynxImageRequestPriority] = _requestPriority;
    }
  }
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
  __weak typeof(self) weakSelf = self;
  __block BOOL completed = NO;
  options.completed = ^(UIImage* image, NSError* error, NSURL* imageURL) {
    __strong typeof(weakSelf) self = weakSelf;
    if (self == nil) {
      return;
    }
    @synchronized(self) {
      completed = YES;
      if (self->_requestGenerations[type] != requestGeneration) {
        return;
      }
      if (image == nil || error != nil) {
        self->_resizeReuseDisabled = YES;
      }
      self->_cancelBlocks[@(type)] = nil;
    }
    dispatch_block_t applyResult = ^{
      __strong typeof(weakSelf) self = weakSelf;
      if (self == nil) {
        return;
      }
      // Once accepted on main, this result finishes before queued target cleanup.
      // Do not hold the request lock across image service or event callbacks.
      @synchronized(self) {
        if (self->_requestGenerations[type] != requestGeneration) {
          return;
        }
      }
      self->_images[@(type)] = image;

      if (image != nil && type == LynxImageRequestSrc && self->_autoSize &&
          UIEdgeInsetsEqualToEdgeInsets(self->_capInsets, UIEdgeInsetsZero)) {
        CGSize imageSize = image.size;
        if (ShouldUpdateAutoSizeLayout(imageSize, layoutSize)) {
          [self->_context
              findShadowNodeAndRunTask:self->_sign
                                  task:^(LynxShadowNode* node) {
                                    __strong typeof(weakSelf) self = weakSelf;
                                    if (self == nil) {
                                      return;
                                    }
                                    // Serialize the layout mutation with request invalidation.
                                    @synchronized(self) {
                                      if (self->_requestGenerations[type] == requestGeneration &&
                                          [node isKindOfClass:LynxImageShadowNode.class]) {
                                        [(LynxImageShadowNode*)node setImageSize:imageSize];
                                      }
                                    }
                                  }];
        }
      }

      // If source image is loaded, placeholder image is not needed to be displayed.
      if (self->_images[@(LynxImageRequestSrc)] != nil && type == LynxImageRequestPlaceholder) {
        return;
      }

      if (self->_imageView != nil || self->_imageLayer != nil) {
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
    if ([NSThread isMainThread]) {
      applyResult();
    } else {
      dispatch_async(dispatch_get_main_queue(), applyResult);
    }
  };

  if (type == LynxImageRequestSrc && (_eventMask & kFlagImageLoadStartEvent) != 0) {
    [self sendCustomEvent:@"loadstart" withParams:@{@"url" : imageURL.url.absoluteString}];
  }
  cancel = [[LynxImageLoader sharedInstance] loadImageWithOptions:options];
  @synchronized(self) {
    if (completed) {
      return;
    }
    if (_requestGenerations[type] == requestGeneration) {
      _cancelBlocks[@(type)] = cancel;
      return;
    }
  }
  CancelImageRequest(cancel);
}

- (void)applyBackgroundImage:(UIImage*)image {
  CALayer* layer = _imageLayer;
  NSArray<UIImage*>* frames = image.images ?: (image != nil ? @[ image ] : @[]);
  NSMutableArray* contents = [NSMutableArray arrayWithCapacity:frames.count];
  for (UIImage* sourceFrame in frames) {
    UIImage* frame = sourceFrame;
    if (frame.imageOrientation != UIImageOrientationUp || frame.CGImage == nil) {
      // CALayer does not interpret UIImage orientation metadata.
      UIGraphicsBeginImageContextWithOptions(frame.size, NO, frame.scale);
      [frame drawInRect:(CGRect){CGPointZero, frame.size}];
      frame = UIGraphicsGetImageFromCurrentImageContext();
      UIGraphicsEndImageContext();
    }
    if (frame.CGImage == nil) {
      return;
    }
    [contents addObject:(__bridge id)frame.CGImage];
  }

  BOOL animated = contents.count > 1 && image.duration > 0;
  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  if (_imageSizeDidChange) {
    _imageSizeDidChange(image != nil ? image.size : CGSizeZero);
  }
  layer.contents = contents.firstObject;
  layer.contentsScale = image != nil ? image.scale : 1;
  layer.contentsGravity = kCAGravityResize;
  if (animated) {
    NSUInteger frameCount = contents.count;
    NSMutableArray<NSNumber*>* keyTimes = [NSMutableArray arrayWithCapacity:frameCount + 1];
    for (NSUInteger i = 0; i <= frameCount; ++i) {
      [keyTimes addObject:@((double)i / frameCount)];
    }
    CAKeyframeAnimation* animation = [CAKeyframeAnimation animationWithKeyPath:@"contents"];
    animation.values = contents;
    animation.keyTimes = keyTimes;
    animation.calculationMode = kCAAnimationDiscrete;
    animation.duration = image.duration;
    animation.repeatCount = HUGE_VALF;
    [layer addAnimation:animation forKey:kBackgroundImageAnimationKey];
  }
  [CATransaction commit];
}

- (void)applyImage:(UIImage*)image withType:(LynxImageRequestType)type {
  if (_imageLayer != nil) {
    [self applyBackgroundImage:image];
    return;
  }
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
  BOOL registerCallbacksBeforeSettingImage =
      isAnimatedImage && type == LynxImageRequestSrc &&
      [imageService
          respondsToSelector:@selector(requiresAnimatedImageCallbackBeforeSettingImage)] &&
      [imageService requiresAnimatedImageCallbackBeforeSettingImage];
  if (registerCallbacksBeforeSettingImage) {
    [self registerAnimatedImageCallbacks:imageService];
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
  } else if (image.images.count > 1) {
    _imageView.animationDuration = image.duration;
    _imageView.animationRepeatCount = 0;
    if (_autoPlay) {
      [_imageView startAnimating];
    }
  }
  if (!registerCallbacksBeforeSettingImage) {
    [self registerAnimatedImageCallbacks:imageService];
  }
}

- (void)registerAnimatedImageCallbacks:(id<LynxServiceImageProtocol>)imageService {
  const NSInteger animationEventMask =
      kFlagImageStartPlayEvent | kFlagImageCurrentLoopEvent | kFlagImageFinalLoopEvent;
  if ((_eventMask & animationEventMask) == 0 ||
      ![imageService respondsToSelector:@selector
                     (registerAnimatedImageCallback:
                                      startCallback:currentLoopCallback:finalLoopCallback:)]) {
    return;
  }

  __weak typeof(self) weakSelf = self;
  dispatch_block_t startCallback = nil;
  dispatch_block_t currentLoopCallback = nil;
  dispatch_block_t finalLoopCallback = nil;
  if ((_eventMask & kFlagImageStartPlayEvent) != 0) {
    startCallback = ^{
      [weakSelf sendCustomEvent:kLynxImageEventStartPlay withParams:@{}];
    };
  }
  if ((_eventMask & kFlagImageCurrentLoopEvent) != 0) {
    currentLoopCallback = ^{
      [weakSelf sendCustomEvent:kLynxImageEventCurrentLoop withParams:@{}];
    };
  }
  if ((_eventMask & kFlagImageFinalLoopEvent) != 0) {
    finalLoopCallback = ^{
      [weakSelf sendCustomEvent:kLynxImageEventFinalLoop withParams:@{}];
    };
  }
  [imageService registerAnimatedImageCallback:_imageView
                                startCallback:startCallback
                          currentLoopCallback:currentLoopCallback
                            finalLoopCallback:finalLoopCallback];
}

- (void)setLayerTarget:(CALayer*)layer {
  [self setLayerTarget:layer imageSizeDidChange:nil];
}

- (void)setLayerTarget:(CALayer*)layer imageSizeDidChange:(void (^)(CGSize))imageSizeDidChange {
  _imageSizeDidChange = [imageSizeDidChange copy];
  _imageLayer = layer;
  if (_images[@(LynxImageRequestSrc)] != nil) {
    [self applyImage:_images[@(LynxImageRequestSrc)] withType:LynxImageRequestSrc];
  } else {
    [self applyImage:_images[@(LynxImageRequestPlaceholder)] withType:LynxImageRequestPlaceholder];
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

- (BOOL)invokeUIMethod:(NSString*)method
                params:(NSDictionary*)params
              callback:(LynxUIMethodCallbackBlock)callback {
  (void)params;
  id<LynxServiceImageProtocol> imageService = [LynxImageLoader imageService];
  if ([method isEqualToString:@"pauseAnimation"]) {
    [imageService pauseImage:_imageView callback:callback];
    return YES;
  }
  if ([method isEqualToString:@"resumeAnimation"]) {
    [imageService resumeImage:_imageView callback:callback];
    return YES;
  }
  if ([method isEqualToString:@"stopAnimation"]) {
    [imageService stopImage:_imageView callback:callback];
    return YES;
  }
  if ([method isEqualToString:@"startAnimate"]) {
    if (![imageService restartImageIfPossible:_imageView callback:callback]) {
      [_imageView stopAnimating];
      [_imageView startAnimating];
      if (callback != nil) {
        callback(kUIMethodSuccess, @"Animation started.");
      }
    }
    return YES;
  }
  return NO;
}

- (void)reset {
  NSArray<dispatch_block_t>* cancelBlocks;
  @synchronized(self) {
    _resizeReuseDisabled = YES;
    for (NSUInteger& generation : _requestGenerations) {
      ++generation;
    }
    cancelBlocks = _cancelBlocks.allValues;
    [_cancelBlocks removeAllObjects];
  }
  CancelImageRequest(^{
    [self->_imageLayer removeAnimationForKey:kBackgroundImageAnimationKey];
    self->_imageLayer = nil;
    self->_imageSizeDidChange = nil;
    self->_imageView = nil;
    for (dispatch_block_t cancel in cancelBlocks) {
      cancel();
    }
  });
}

@end
