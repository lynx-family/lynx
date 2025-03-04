// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIImage.h"
#import "LynxBlurImageProcessor.h"
#import "LynxColorUtils.h"
#import "LynxComponentRegistry.h"
#import "LynxConvertUtils.h"
#import "LynxEnv.h"
#import "LynxImageBlurUtils.h"
#import "LynxImageLoader.h"
#import "LynxImageProcessor.h"
#import "LynxMeasureDelegate.h"
#import "LynxMemoryListener.h"
#import "LynxNinePatchImageProcessor.h"
#import "LynxPropsProcessor.h"
#import "LynxService.h"
#import "LynxServiceTrailProtocol.h"
#import "LynxShadowNodeOwner.h"
#import "LynxUI+Internal.h"
#import "LynxUI+Private.h"
#import "LynxUIUnitUtils.h"
#import "LynxUnitUtils.h"

#import "LynxBackgroundUtils.h"
#import "LynxView+Internal.h"

#import "LynxEventReporter.h"
#import "LynxLog.h"
#import "LynxService.h"
#import "LynxServiceImageProtocol.h"
#import "LynxSubErrorCode.h"
#import "LynxUIContext+Internal.h"
#import "LynxVersion.h"

#import "LynxContext+Internal.h"

typedef NS_ENUM(NSInteger, LynxResizeMode) {
  LynxResizeModeCover = UIViewContentModeScaleAspectFill,
  LynxResizeModeContain = UIViewContentModeScaleAspectFit,
  LynxResizeModeScaleToFill = UIViewContentModeScaleToFill,
  LynxResizeModeCenter = UIViewContentModeCenter
};

#pragma mark LynxURL

@interface LynxUIImageDrawParameter : NSObject

@property(nonatomic) UIImage* image;
@property(nonatomic) UIEdgeInsets borderWidth;
@property(nonatomic) LynxBorderRadii borderRadius;
@property(nonatomic) UIEdgeInsets padding;
@property(nonatomic) CGRect frame;
@property(nonatomic, assign) UIViewContentMode resizeMode;

@end

@implementation LynxUIImageDrawParameter

@end

/**
 Use to process image into image with border radius.
 */
@interface LynxBorderRadiusImageProcessor : NSObject <LynxImageProcessor>

- (instancetype)initWithDrawParameter:(LynxUIImageDrawParameter*)param;

@property(nonatomic) LynxUIImageDrawParameter* param;

@end

@implementation LynxBorderRadiusImageProcessor

- (instancetype)initWithDrawParameter:(LynxUIImageDrawParameter*)param {
  if (self = [super init]) {
    self.param = param;
  }
  return self;
}

- (UIImage*)processImage:(UIImage*)image {
  // If the image is a gif, we don't use drawRect to process it, but simply return.
  if ([LynxUIImage isAnimatedImage:image] || self.param.frame.size.width == 0 ||
      self.param.frame.size.height == 0) {
    return image;
  }

  CGSize size = self.param.frame.size;
  CGRect rect = CGRectMake(0, 0, size.width, size.height);
  self.param.image = image;
  UIImage* output = [LynxUI
      imageWithActionBlock:^(CGContextRef _Nonnull context) {
        UIGraphicsPushContext(context);
        [LynxUIImage drawRect:rect withParameters:self.param];
        UIGraphicsPopContext();
      }
                    opaque:NO
                     scale:[LynxUIUnitUtils screenScale]
                      size:size];
  return output;
}

- (NSString*)cacheKey {
  return
      [NSString stringWithFormat:@"_LynxBorderRadiusImageProcessor_%@_%@_%ld_%@",
                                 NSStringFromUIEdgeInsets(self.param.borderWidth),
                                 NSStringFromUIEdgeInsets(self.param.padding),
                                 (long)self.param.resizeMode, NSStringFromCGRect(self.param.frame)];
}

@end

@implementation LynxImageShadowNode

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_SHADOW_NODE("image")
#else
LYNX_REGISTER_SHADOW_NODE("image")
#endif

@end

@interface LynxUIImage () <LynxMeasureDelegate>
@property(nonatomic, assign) UIViewContentMode resizeMode;
@property(nonatomic, assign) BOOL coverStart;
@property(nonatomic, assign) BOOL freed;
@property(nonatomic) LynxURL* src;
@property(nonatomic) LynxURL* placeholder;
@property(nonatomic) CGFloat blurRadius;
@property(nonatomic, assign) UIEdgeInsets capInsets;
@property(nonatomic, assign) CGFloat capInsetsScale;
@property(nonatomic) UIImage* image;
@property(nonatomic, strong) NSMutableDictionary<id, dispatch_block_t>* cancelBlocks;
@property(nonatomic, readwrite) NSInteger loopCount;
@property(nonatomic, readwrite) CGFloat preFetchWidth;
@property(nonatomic, readwrite) CGFloat preFetchHeight;
@property(nonatomic, assign) BOOL downsampling;
@property(nonatomic, assign) BOOL autoSize;
@property(nonatomic, assign) BOOL isOffScreen;
@property(nonatomic) BOOL deferSrcInvalidation;
@property(nonatomic, assign) NSInteger logBoxSizeWarningThreshold;
@property(nonatomic) CGSize prevSize;
@property(nonatomic) NSDate* startRequestTime;
@property(nonatomic) BOOL useNewImage;
@property(nonatomic) NSDate* finishRequestTime;
@property(nonatomic) BOOL autoPlay;
@property(nonatomic) UIColor* tintColor;
@property(nonatomic) BOOL enableExtraLoadInfo;
@property(nonatomic, assign) BOOL skipRedirection;
@property(nonatomic, assign) BOOL enableImageSR;
@property(nonatomic, assign) BOOL enableFadeIn;
@property(nonatomic, assign) BOOL isDirty;
@property(nonatomic) CGSize lastFrameSize;
@property(nonatomic, assign) BOOL enableImageEventReport;
@property(nonatomic, assign) BOOL enableGenericFetcher;
@property(nonatomic, strong) NSDictionary* additional_custom_info;
@property(nonatomic, strong) NSString* request_priority;
@property(nonatomic, strong) NSString* cache_choice;
@property(nonatomic, strong) NSDictionary* placeholder_hash_config;
@end

@implementation LynxUIImage {
  NSString* _preSrc;
}

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("image")
#else
LYNX_REGISTER_UI("image")
#endif

- (void)initProperties {
  [super initProperties];
  _resizeMode = UIViewContentModeScaleToFill;
  _coverStart = false;
  _cancelBlocks = [NSMutableDictionary new];
  self.loopCount = 0;
  _preSrc = nil;
  _capInsetsScale = 1.0;
  _useNewImage = YES;
  _deferSrcInvalidation = false;
  _requestOptions = LynxImageDefaultOptions;
  _autoPlay = YES;
  _tintColor = nil;
  _lastFrameSize = CGSizeZero;
  _isDirty = YES;
  _enableImageEventReport = [LynxEnv.sharedInstance enableImageEventReport];
}

- (void)freeMemoryCache {
  if (self.isOffScreen && [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvFreeImageMemory
                                                           defaultValue:NO]) {
    self.image = nil;
    [self freeImageCache];
  }
}

- (void)targetOnScreen {
  if (self.freed && self.view.image == nil &&
      [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvFreeImageMemory defaultValue:NO]) {
    self.isOffScreen = NO;
    self.freed = NO;
    [self requestImage];
  }
}

- (void)setContext:(LynxUIContext*)context {
  [super setContext:context];
  _enableGenericFetcher = self.context.mediaResourceFetcher != nil;
}

- (void)targetOffScreen {
  self.isOffScreen = YES;
  // release image memory caches when current lynxview entering into
  // background stack, trail for libra abtest, default close
  if ([[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvFreeImageMemoryForce defaultValue:NO]) {
    [self freeMemoryCache];
  }
}

- (UIView*)createView {
  return [self createImageViewForUIImage:self];
}

- (UIImageView*)createImageViewForUIImage:(LynxUIImage*)ui {
  UIImageView* imageView = [[LynxImageLoader imageService] imageView];
  if (imageView) {
    [[LynxImageLoader imageService] addAnimatedImageCallBack:imageView UI:ui];
  } else {
    // fallback to create UIImageView if no imageService
    imageView = [UIImageView new];
  }
  imageView.clipsToBounds = YES;
  // Default contentMode UIViewContentModeScaleToFill
  imageView.contentMode = UIViewContentModeScaleToFill;
  imageView.userInteractionEnabled = YES;
  return imageView;
}

- (void)addExposure {
  BOOL enableExposure = [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableImageExposure
                                                         defaultValue:NO];
  if (enableExposure) {
    self.internalSignature = [NSString stringWithFormat:@"lynx_image_%@", self];
  }
}

- (void)setImageToView:(UIImage*)image {
  self.view.image = image;

  if (image != nil && self.loopCount <= 0 && self.tintColor != nil) {
    // This is a workaround to address an issue with some UIImage subclass. In iOS 16.*, the
    // image rendering mode may not function correctly unless we reset the image rendering mode
    // when view.image is not nil.
    self.view.image = [self.view.image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  }

  if (![[LynxImageLoader imageService] checkImageType:self.view]) {
    // If the imageView is not returned by imageService, we need to manually reset the
    // animationImages.
    self.view.animationImages = image ? image.images : nil;
  }
}

- (void)onImageReady:(UIImage*)image withRequest:(LynxURL*)requestURL {
  __weak typeof(self) weakSelf = self;
  __block void (^ready)(UIImage*, LynxURL*) = ^(UIImage* image, LynxURL* requestURL) {
    typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf) {
      return;
    }
    if (image == nil) {
      [strongSelf setImageToView:image];
      return;
    }
    if (strongSelf.enableFadeIn && [strongSelf shouldUseNewImage] && !requestURL.fromMemoryCache) {
      [strongSelf.view.layer removeAnimationForKey:@"LynxImageFadeAnimation"];
      CATransition* transition = [CATransition animation];
      transition.duration = 0.3;
      transition.timingFunction =
          [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
      transition.type = kCATransitionFade;
      [strongSelf.view.layer addAnimation:transition forKey:@"LynxImageFadeAnimation"];
    }
    if (strongSelf.autoSize &&
        UIEdgeInsetsEqualToEdgeInsets(strongSelf.capInsets, UIEdgeInsetsZero)) {
      LynxShadowNodeOwner* owner = strongSelf.context.nodeOwner;
      if (!owner) {
        return;
      }
      LynxShadowNode* node = [owner nodeWithSign:strongSelf.sign];
      if (!node) {
        return;
      }
      // If the frameDidChange changes is just a flick, ignore it to avoid dead loop as the flick
      // will constantly exists and keeps calling frameDidChange.
      if (![strongSelf isLayoutFlick:strongSelf.prevSize withAnotherSize:strongSelf.frame.size]) {
        [node setMeasureDelegate:strongSelf];
        [node internalSetNeedsLayoutForce];
        strongSelf.prevSize = strongSelf.frame.size;
      }
    }

    if (strongSelf.loopCount <= 0) {
      [strongSelf setImageToView:image];
      if (image.images != nil && [image.images count] > 1) {
        strongSelf.view.animationDuration = image.duration;
        [strongSelf.view startAnimating];
      }
      if (requestURL) {
        [requestURL updatePreviousUrl];
      }
    } else {
      if ([requestURL isPreviousUrl]) {
        return;
      }
      if (requestURL) {
        [requestURL updatePreviousUrl];
      }
      [[LynxImageLoader imageService] handleAnimatedImage:image
                                                     view:strongSelf.view
                                                loopCount:strongSelf.loopCount];
    }
  };
  if ([NSThread isMainThread]) {
    ready(image, requestURL);
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      ready(image, requestURL);
    });
  }
}

- (bool)updateLayerMaskOnFrameChangedInner:(BOOL)needAsyncDisplay URL:(LynxURL*)requestUrl {
  // we do not need to run super, as overflow is not used for image,
  // border-radius will be processed by itself
  if (CGSizeEqualToSize(self.frame.size, CGSizeZero)) {
    return false;
  }
  if (needAsyncDisplay) {
    if (self.image != nil) {
      BOOL isAnimatedImage = [LynxUIImage isAnimatedImage:self.image];
      if (isAnimatedImage ||
          (UIEdgeInsetsEqualToEdgeInsets(self.backgroundManager.borderWidth, UIEdgeInsetsZero) &&
           UIEdgeInsetsEqualToEdgeInsets(self.padding, UIEdgeInsetsZero) &&
           ![self.backgroundManager hasDifferentBorderRadius])) {
        [self onImageReady:_image withRequest:requestUrl];
      } else {
        __weak typeof(self) weakSelf = self;
        NSURL* lastNSUrl = requestUrl.url;
        [self displayAsyncWithCompletionBlock:^(UIImage* _Nonnull image) {
          if (![weakSelf.src.url.absoluteString isEqualToString:lastNSUrl.absoluteString] &&
              ![weakSelf.placeholder.url.absoluteString isEqualToString:lastNSUrl.absoluteString]) {
            return;
          }
          [weakSelf onImageReady:image withRequest:requestUrl];
        }];
      }
    }
  } else {
    [self onImageReady:_image withRequest:requestUrl];
  }
  if (_resizeMode == UIViewContentModeScaleAspectFill && _coverStart) {
    CGFloat availableWidth = self.frame.size.width - self.padding.left - self.padding.right -
                             self.border.left - self.border.right;
    CGFloat availableHeight = self.frame.size.height - self.padding.top - self.padding.bottom -
                              self.border.top - self.border.bottom;
    CGFloat sourceWidth = _image.size.width;
    CGFloat sourceHeight = _image.size.height;
    float w_rate = sourceWidth / availableWidth;
    float h_rate = sourceHeight / availableHeight;
    if (h_rate > w_rate) {
      CGFloat h = w_rate * availableHeight;
      self.view.layer.contentsRect = CGRectMake(0, 0, 1, h / sourceHeight);
    } else {
      CGFloat w = h_rate * availableWidth;
      self.view.layer.contentsRect = CGRectMake(0, 0, w / sourceWidth, 1);
    }
  }
  return true;
}

- (bool)superUpdateLayerMaskOnFrameChanged {
  return [super updateLayerMaskOnFrameChanged];
}

// There will be a rounding accuracy error of 1 physical pixel in layout. Avoid relayout when it is
// less than or equal to 1 physical pixel.
- (BOOL)isLayoutFlick:(CGSize)size withAnotherSize:(CGSize)anotherSize {
  return abs([LynxUnitUtils toPhysicalPixelFromPt:size.width] -
             [LynxUnitUtils toPhysicalPixelFromPt:anotherSize.width]) <= 1.f &&
         abs([LynxUnitUtils toPhysicalPixelFromPt:size.height] -
             [LynxUnitUtils toPhysicalPixelFromPt:anotherSize.height]) <= 1.f;
}

- (bool)updateLayerMaskOnFrameChanged {
  self.view.clipsToBounds = YES;  // image'clipsToBounds should always be YES
  BOOL supportsProcessor = LynxImageFetchherSupportsProcessor(self.context.imageFetcher);
  return [self updateLayerMaskOnFrameChangedInner:!supportsProcessor URL:_src];
}

- (void)propsDidUpdate {
  [self addExposure];
  // Treate border-radius specially because border-radius processor is in requestImage.
  if (self.backgroundManager.backgroundInfo.borderChanged) {
    [self markAsDirty];
  }
  [super propsDidUpdate];
}

- (void)onNodeReady {
  [super onNodeReady];
  if (_isDirty) {
    [self requestImage];
    _isDirty = NO;
  }
}

- (void)frameDidChange {
  [super frameDidChange];
  if (![self isLayoutFlick:_lastFrameSize withAnotherSize:self.frame.size]) {
    _lastFrameSize = self.frame.size;
    [self markAsDirty];
  }
}

- (void)markAsDirty {
  LYNX_MAYBE_ON_ASYNC_THREAD;

  _isDirty = YES;
}

- (void)onLayoutAnimationEnd:(CGRect)frame {
  [super onLayoutAnimationEnd:frame];
  [self requestImage];
}

UIEdgeInsets LynxRoundInsetsToPixel(UIEdgeInsets edgeInsets) {
  edgeInsets.top = round(edgeInsets.top);
  edgeInsets.bottom = round(edgeInsets.bottom);
  edgeInsets.left = round(edgeInsets.left);
  edgeInsets.right = round(edgeInsets.right);

  return edgeInsets;
}

- (BOOL)enableAsyncDisplay {
  // Images may or may not be displayed asynchronously; the default is AsyncDisplay.
  return _asyncDisplayFromTTML;
}

- (void)requestImage {
  self.image = nil;
  [self requestImage:_src];
  [self requestImage:_placeholder];
}

- (void)freeImageCache {
  self.freed = YES;
  [self resetImage];
}

- (bool)getEnableImageDownsampling {
  return self.context.enableImageDownsampling;
}

- (BOOL)getTrailUseNewImage {
  return self.context.trailUseNewImage;
}

- (bool)getPageConfigEnableNewImage {
  return self.context.enableNewImage;
}

- (NSInteger)getLogBoxSizeWarningThreshold {
  return self.context.logBoxImageSizeWarningThreshold;
}

- (BOOL)getSetUseNewImage {
  return [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvUseNewImage defaultValue:NO];
}

- (BOOL)shouldUseNewImage {
  if ([self getTrailUseNewImage]) {
    return true;
  }
  if ([self getSetUseNewImage] && [self getPageConfigEnableNewImage] && _useNewImage) {
    return true;
  }
  return false;
}

- (void)reportBigImageWarningIfNeed:(UIImage*)image {
  if (!image) {
    return;
  }
  self.logBoxSizeWarningThreshold = [self getLogBoxSizeWarningThreshold];
  bool overThresholdJudge = (self.updatedFrame.size.width * self.updatedFrame.size.height) > 0 &&
                            (image.size.width * image.size.height) /
                                    (self.updatedFrame.size.width * self.updatedFrame.size.height) >
                                self.logBoxSizeWarningThreshold;
  if (overThresholdJudge) {
    CGFloat memoryUse = image.size.width * image.size.height * 4;
    NSString* errorInfo = [NSString
        stringWithFormat:
            @"Image size is %d times bigger than your UI. Please consider downsampling it to "
            @"avoid protential OOM.\n"
            @"Red Box Warning Threshod: %d. Can be adjusted in pageConfig.\n"
            @"url:%@\n"
            @"image size: %f * %f\n"
            @"memory use: %f\n"
            @"component size: %f * %f\n",
            (int)self.logBoxSizeWarningThreshold, (int)self.logBoxSizeWarningThreshold,
            self.src.url.absoluteString, image.size.height, image.size.width, memoryUse,
            self.view.frame.size.height, self.view.frame.size.width];
    LynxError* error = [LynxError lynxErrorWithCode:ECLynxResourceImageBigImage message:errorInfo];
    [self.context reportLynxError:error];
  }
}

- (void)requestImage:(LynxURL*)requestUrl {
  if (_cancelBlocks[@(requestUrl.type)]) {
    _cancelBlocks[@(requestUrl.type)]();
    _cancelBlocks[@(requestUrl.type)] = nil;
  }
  NSURL* url = requestUrl.url;
  if (!url || [url.absoluteString isEqualToString:@""]) {
    return;
  }
  if (self.frame.size.width <= 0 && self.frame.size.height <= 0 && _preFetchWidth <= 0 &&
      _preFetchHeight <= 0) {
    return;
  }
  if (_autoSize && self.frame.size.width <= 0 && self.frame.size.height <= 0) {
    return;
  }
  NSMutableArray* processors = [NSMutableArray new];
  if (!UIEdgeInsetsEqualToEdgeInsets(_capInsets, UIEdgeInsetsZero)) {
    [processors addObject:[[LynxNinePatchImageProcessor alloc] initWithCapInsets:_capInsets
                                                                  capInsetsScale:_capInsetsScale]];
  }
  if (_blurRadius > 0) {
    [processors addObject:[[LynxBlurImageProcessor alloc] initWithBlurRadius:_blurRadius]];
  }
  BOOL supportsProcessor = LynxImageFetchherSupportsProcessor(self.context.imageFetcher);
  if (supportsProcessor) {
    BOOL hasNoBorderRadii =
        UIEdgeInsetsEqualToEdgeInsets(self.backgroundManager.borderWidth, UIEdgeInsetsZero) &&
        UIEdgeInsetsEqualToEdgeInsets(self.padding, UIEdgeInsetsZero) &&
        !LynxHasBorderRadii(self.backgroundManager.borderRadius);
    if (!hasNoBorderRadii) {
      // FIXME(linxs): is it necessary to process radius like this?
      [processors addObject:[[LynxBorderRadiusImageProcessor alloc]
                                initWithDrawParameter:self.drawParameter]];
    }
  }
  __weak typeof(self) weakSelf = self;
  static NSString* LynxImageEventLoad = @"load";
  CGSize size = CGSizeZero;
  if ((_preFetchWidth > 0 && _preFetchWidth > 0) &&
      (self.frame.size.width <= 0 || self.frame.size.height <= 0)) {
    size = CGSizeMake(_preFetchWidth, _preFetchHeight);
  } else {
    size = self.view.bounds.size;
  }
  LynxImageLoadCompletionBlock requestBlock = ^(UIImage* _Nullable image, NSError* _Nullable error,
                                                NSURL* _Nullable imageURL) {
    typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf) {
      return;
    }
    if (requestUrl.lastRequestUrl &&
        ![requestUrl.lastRequestUrl.absoluteString isEqualToString:url.absoluteString]) {
      _LogD(@"the image block ignored, due to the new src is loading!");
      return;
    }

    [strongSelf reportImageEvent:requestUrl
                       startTime:[strongSelf.startRequestTime timeIntervalSince1970] * 1000
                      finishTime:[[NSDate date] timeIntervalSince1970] * 1000
                      isNewImage:[strongSelf shouldUseNewImage]
                           error:error
                           width:image.size.width
                          height:image.size.height
                      customInfo:strongSelf.additional_custom_info];

    NSDate* getImageTime = [NSDate date];
    [strongSelf reportBigImageWarningIfNeed:image];
    strongSelf.cancelBlocks[@(requestUrl.type)] = nil;
    // If enable NewImage, remember to free newImageRequest after cancel and complete
    if (strongSelf.useNewImage) {
      strongSelf.customImageRequest = nil;
    }
    NSString* errorDetail;
    BOOL isLatestImageURL = [requestUrl.url.absoluteString isEqualToString:imageURL.absoluteString];
    if (!isLatestImageURL) {
      return;
    }
    requestUrl.memoryCost = image.size.width * image.size.height * 4;
    if (error) {
      errorDetail = [NSString stringWithFormat:@"url:%@,%@", url, [error description]];
    }
    if (!errorDetail) {
      [[LynxImageLoader imageService] setAutoPlay:strongSelf.view value:strongSelf.autoPlay];
      requestUrl.imageSize = image.size;
      requestUrl.isSuccess = 1;
      if (requestUrl.type == LynxImageRequestPlaceholder && strongSelf.image) {
        return;
      }
      requestUrl.memoryCost =
          (image.size.height * image.scale) * (image.size.width * image.scale) * 4;
      strongSelf.image = image;
      // To enable gifs with corner-radius.
      // If the image is a gif, we call LynxUI::updateLayerMaskOnFrameChanged.
      // Using gifs with corner-radius in animation is highly unrecommended.
      BOOL isAnimatedImage = [LynxUIImage isAnimatedImage:strongSelf.image];
      if (isAnimatedImage) {
        [strongSelf onImageReady:image withRequest:requestUrl];
        if ([NSThread isMainThread]) {
          [strongSelf superUpdateLayerMaskOnFrameChanged];
        } else {
          dispatch_async(dispatch_get_main_queue(), ^{
            typeof(weakSelf) strongSelf = weakSelf;
            if (strongSelf) {
              [strongSelf superUpdateLayerMaskOnFrameChanged];
            }
          });
        }
      }
      // If not a gif, we call LynxUIImage::updateLayerMaskOnFrameChanged.
      else {
        [strongSelf updateLayerMaskOnFrameChangedInner:!supportsProcessor URL:requestUrl];
      }
      [requestUrl updateTimeStamp:getImageTime startRequestTime:strongSelf.startRequestTime];
      if (requestUrl.type == LynxImageRequestSrc &&
          [strongSelf.eventSet valueForKey:LynxImageEventLoad]) {
        [strongSelf addReportInfo:requestUrl];
        [[LynxImageLoader imageService] appendExtraImageLoadDetailForEvent:image
                                                            originalDetail:requestUrl.reportInfo];
        if (strongSelf.enableExtraLoadInfo) {
          [strongSelf.context.eventEmitter
              dispatchCustomEvent:[[LynxDetailEvent alloc] initWithName:LynxImageEventLoad
                                                             targetSign:strongSelf.sign
                                                                 detail:requestUrl.reportInfo]];
        } else {
          [strongSelf.context.eventEmitter
              dispatchCustomEvent:[[LynxDetailEvent alloc]
                                      initWithName:LynxImageEventLoad
                                        targetSign:strongSelf.sign
                                            detail:@{
                                              @"height" : [NSNumber
                                                  numberWithFloat:roundf(image.size.height)],
                                              @"width" : [NSNumber
                                                  numberWithFloat:roundf(image.size.width)]
                                            }]];
        }
      } else {
        if (strongSelf.context.devtoolEnabled || strongSelf.context.imageMonitorEnabled) {
          [strongSelf addReportInfo:requestUrl];
        }
      }
    } else {
      [strongSelf addErrorInfo:requestUrl];
      [requestUrl updateTimeStamp:getImageTime startRequestTime:strongSelf.startRequestTime];
      [strongSelf reportURLSrcError:error type:requestUrl.type source:url];
    }

    [strongSelf monitorReporter:requestUrl];
  };
  _startRequestTime = [NSDate date];
  BOOL downsampling = (_downsampling || self.getEnableImageDownsampling) && !_autoSize;
  requestUrl.lastRequestUrl = url;
  [self initResourceLoaderInformation];
  [requestUrl initResourceInformation];
  _cancelBlocks[@(requestUrl.type)] = [[LynxImageLoader sharedInstance]
      loadImageFromLynxURL:requestUrl
                      size:size
               contextInfo:@{
                 LynxImageFetcherContextKeyUI : self,
                 LynxImageFetcherContextKeyDownsampling : @(downsampling),
                 LynxImageRequestOptions : [NSNumber numberWithLong:_requestOptions],
                 LynxImageRequestContextModuleExtraData : self.context.lynxModuleExtraData ?: @"",
                 LynxImageSkipRedirection : @(_skipRedirection),
                 LynxImageFixNewImageDownsampling : @(self.context.fixNewImageDownSampling),
                 LynxImageAdditionalCustomInfo : self.additional_custom_info ?: [NSNull null],
                 LynxImagePlaceholderHashConfig : self.placeholder_hash_config ?: [NSNull null],
                 LynxImageEnableSR : @(_enableImageSR),
                 LynxImageCacheChoice : self.cache_choice ?: @"",
                 LynxImageRequestPriority : self.request_priority ?: @"",
               }
                processors:processors
              imageFetcher:[self shouldUseNewImage] ? nil : self.context.imageFetcher
               LynxUIImage:self
      enableGenericFetcher:self.enableGenericFetcher
                 completed:(LynxImageLoadCompletionBlock)requestBlock];
}

- (void)initResourceLoaderInformation {
  if (!self.resLoaderInfo) {
    self.resLoaderInfo = [NSMutableDictionary dictionary];
  }
  self.resLoaderInfo[@"res_loader_name"] = @"Lynx";
  self.resLoaderInfo[@"res_loader_version"] = [LynxVersion versionString] ?: @"";
}

#pragma mark monitor

- (void)reportImageEvent:(LynxURL*)url
               startTime:(double)loadStartTime
              finishTime:(double)loadFinishTime
              isNewImage:(BOOL)newImage
                   error:(NSError*)error
                   width:(CGFloat)imageWidth
                  height:(CGFloat)imageHeight
              customInfo:(NSDictionary*)customInfo {
  if (!_enableImageEventReport || ![[url.url scheme] hasPrefix:@"http"]) {
    return;
  }

  NSInteger origin = -1;  // -1:UNKNOWN
  if (newImage) {
    if ([url.resourceInfo[@"is_network"] boolValue]) {
      origin = 0;  // 0: NETWORK
    } else if ([url.resourceInfo[@"is_memory"] boolValue]) {
      origin = 1;  // 1: MEMORY
    } else if ([url.resourceInfo[@"is_disk"] boolValue]) {
      origin = 2;  // 2: DISK
    }
  }

  int32_t instanceId = self.context.instanceId;

  NSString* reportUrl = url.url.absoluteString;
  [LynxEventReporter onEvent:@"lynxsdk_image_event"
                  instanceId:instanceId
                propsBuilder:^NSDictionary* {
                  NSDictionary* baseProps = @{
                    @"load_start" : [NSNumber numberWithDouble:loadStartTime],
                    @"load_finish" : [NSNumber numberWithDouble:loadFinishTime],
                    @"cost" : [NSNumber numberWithDouble:(loadFinishTime - loadStartTime)],
                    @"src" : reportUrl,
                    @"new_image" : [NSNumber numberWithBool:newImage],
                    @"error_code" : error ? [NSNumber numberWithInteger:error.code] : @0,
                    @"origin" : [NSNumber numberWithInteger:origin],
                    @"width" : [NSNumber numberWithFloat:roundf(imageWidth)],
                    @"height" : [NSNumber numberWithFloat:roundf(imageHeight)]
                  };
                  if (customInfo != nil) {
                    NSMutableDictionary* mergedProps =
                        [NSMutableDictionary dictionaryWithDictionary:baseProps];
                    [mergedProps addEntriesFromDictionary:customInfo];
                    return [mergedProps copy];
                  } else {
                    return baseProps;
                  }
                }];
}

- (void)reportImageInfo:(LynxURL*)currentUrl {
  if (![self shouldUseNewImage]) {
    return;
  }

  if (!currentUrl.resourceInfo) {
    return;
  }

  if (![self.context.rootView isKindOfClass:[LynxView class]]) {
    return;
  }

  NSMutableDictionary* data = [NSMutableDictionary dictionary];
  if (currentUrl.isSuccess == 1) {
    currentUrl.resourceInfo[@"res_state"] = @"success";
    currentUrl.resourceInfo[@"res_size"] = [NSNumber numberWithFloat:currentUrl.memoryCost];

    NSMutableDictionary* resLoadPerf = [NSMutableDictionary dictionary];
    resLoadPerf[@"res_load_start"] =
        [NSNumber numberWithDouble:[_startRequestTime timeIntervalSince1970] * 1000];
    resLoadPerf[@"res_load_finish"] =
        [NSNumber numberWithDouble:[_finishRequestTime timeIntervalSince1970] * 1000];
    data[@"res_load_perf"] = resLoadPerf;
  } else {
    currentUrl.resourceInfo[@"res_state"] = @"failed";

    NSMutableDictionary* resLoadError = [NSMutableDictionary dictionary];
    resLoadError[@"res_error_msg"] =
        [NSString stringWithFormat:@"url:%@,%@", currentUrl.url, [currentUrl.error description]];
    NSNumber* originalErrorCode = [currentUrl.error.userInfo valueForKey:@"error_num"]
                                      ?: [NSNumber numberWithInteger:currentUrl.error.code];
    NSNumber* categorizedErrorCode =
        [[LynxImageLoader imageService] getMappedCategorizedPicErrorCode:originalErrorCode];
    resLoadError[@"net_library_error_code"] = originalErrorCode ?: [NSNumber numberWithInt:-1];
    resLoadError[@"res_loader_error_code"] = categorizedErrorCode ?: [NSNumber numberWithInt:-1];
    data[@"res_load_error"] = resLoadError;
  }

  data[@"res_info"] = currentUrl.resourceInfo;
  data[@"res_loader_info"] = _resLoaderInfo;

  [LynxService(LynxServiceMonitorProtocol) reportResourceStatus:(LynxView*)self.context.rootView
                                                           data:data
                                                          extra:NULL];
}

- (void)reportURLSrcError:(NSError*)error
                     type:(LynxImageRequestType)requestType
                   source:(NSURL*)url {
  static NSString* LynxImageEventError = @"error";
  NSString* errorDetail = [NSString stringWithFormat:@"url:%@,%@", url, [error description]];
  NSNumber* errorCode = [self shouldUseNewImage] ? [NSNumber numberWithInteger:error.code]
                                                 : ([error.userInfo valueForKey:@"error_num"]
                                                        ?: [NSNumber numberWithInteger:error.code]);
  NSNumber* categorizedErrorCode =
      [[LynxImageLoader imageService] getMappedCategorizedPicErrorCode:errorCode];
  if (requestType == LynxImageRequestSrc && [self.eventSet valueForKey:LynxImageEventError]) {
    NSDictionary* detail = @{
      @"errMsg" : errorDetail ?: @"",
      @"error_code" : errorCode,
      @"lynx_categorized_code" : categorizedErrorCode ?: @(-1)
    };
    [self.context.eventEmitter
        dispatchCustomEvent:[[LynxDetailEvent alloc] initWithName:LynxImageEventError
                                                       targetSign:self.sign
                                                           detail:detail]];
  }

  NSInteger subErrCode =
      categorizedErrorCode ? [categorizedErrorCode integerValue] : ECLynxResourceImageException;
  NSString* errMsg = @"Error when loading image";
  LynxError* err = [LynxError lynxErrorWithCode:subErrCode message:errMsg];
  [err setRootCause:errorDetail];
  [self.context didReceiveResourceError:err
                             withSource:url ? url.absoluteString : @""
                                   type:@"image"];
}

- (void)monitorReporter:(LynxURL*)reportUrl {
  if (!reportUrl) {
    return;
  }
  // The reason for using sampling instead of spawning a separate thread is to avoid the performance
  // overhead caused by the addReportInfo function.
  if (self.context.imageMonitorEnabled) {
    [LynxService(LynxServiceMonitorProtocol) reportImageStatus:@"lynx_image_status"
                                                          data:reportUrl.reportInfo];
  }
  [self reportImageInfo:reportUrl];
  // upload image memory info for devtool
  if (self.context.devtoolEnabled) {
    [[LynxMemoryListener shareInstance] uploadImageInfo:reportUrl.reportInfo];
  }
}

- (void)addReportInfo:(LynxURL*)reportUrl {
  NSDictionary* timeMetrics = @{
    @"fetchTime" : [NSNumber numberWithDouble:reportUrl.fetchTime],
    @"completeTime" : [NSNumber numberWithDouble:reportUrl.completeTime],
    @"fetchTimeStamp" : [NSString
        stringWithFormat:@"%lld", (uint64_t)(self.startRequestTime.timeIntervalSince1970 * 1000)],
    @"finishTimeStamp" :
        [NSString stringWithFormat:@"%lld", (uint64_t)((CFAbsoluteTimeGetCurrent() +
                                                        kCFAbsoluteTimeIntervalSince1970) *
                                                       1000)],
  };
  NSDictionary* templateMetric = @{
    @"url" : self.context.rootView.url ?: @"",
    @"width" : @(self.image.size.width),
    @"height" : @(self.image.size.height),
    @"viewWidth" : @(self.view.frame.size.width),
    @"viewHeight" : @(self.view.frame.size.height),
  };
  NSMutableDictionary* reportData = reportUrl.reportInfo;
  // Basic info
  reportData[@"type"] = @"image";
  reportData[@"metric"] = templateMetric ?: @"";
  reportData[@"image_url"] = reportUrl.url.absoluteString ?: @"";
  // Image info
  reportData[@"width"] = @(self.image.size.width);
  reportData[@"height"] = @(self.image.size.height);
  reportData[@"timeMetrics"] = timeMetrics ?: @"";
  reportData[@"successRate"] = [NSNumber numberWithInt:(int)reportUrl.isSuccess] ?: @"";
  reportData[@"memoryCost"] = @(reportUrl.memoryCost) ?: @"";
  reportData[@"resourceFromDiskCache"] = reportUrl.resourceInfo[@"is_disk"] ?: @"";
  reportData[@"resourceFromMemoryCache"] = reportUrl.resourceInfo[@"is_memory"] ?: @"";
  if (((NSNumber*)reportUrl.resourceInfo[@"isBase64"]).boolValue) {
    reportData[@"resourceFrom"] = @"base64";
  } else if ([reportUrl.resourceInfo[@"res_from"] isEqualToString:@"cdn"]) {
    reportData[@"resourceFrom"] = @"cdn";
  } else {
    reportData[@"resourceFrom"] = @"local resource";
  }
  // Settings info
  reportData[@"globalDownsampleSet"] = @(self.getEnableImageDownsampling);
  reportData[@"singleDownsampleSet"] = @(self.downsampling);
  reportData[@"newImage"] = @([self shouldUseNewImage]);
  reportData[@"preloaded"] = reportUrl.resourceInfo[@"is_preloaded"] ?: @"";
}

- (void)addErrorInfo:(LynxURL*)reportUrl {
  NSDictionary* templateMetric = @{
    @"url" : self.context.rootView.url ?: @"",
  };
  NSMutableDictionary* reportData = reportUrl.reportInfo;
  reportData[@"type"] = @"image";
  reportData[@"metric"] = templateMetric ?: @"";
  reportData[@"image_url"] = reportUrl.url.absoluteString ?: @"";
  reportData[@"successRate"] = [NSNumber numberWithInt:(int)reportUrl.isSuccess] ?: @"";
  reportData[@"newImage"] = @([self shouldUseNewImage]);
}

- (void)resetImage {
  LYNX_MAYBE_ON_ASYNC_THREAD;

  [self.propsDidUpdateBlockArray addObject:^(LynxUI* ui) {
    if (ui.view == nil) {
      return;
    }

    LynxUIImage* image = (LynxUIImage*)ui;
    image.view.image = nil;
    if (![[LynxImageLoader imageService] checkImageType:image.view]) {
      image.view.animationImages = nil;
    }
  }];
}

- (NSURL*)illegalUrlHandler:(NSString*)value {
  LYNX_MAYBE_ON_ASYNC_THREAD;

  NSURL* urlValue = [[NSURL alloc] initWithString:value];
  // To handle some illegal symbols, such as chinese characters and [], etc
  // Query + Path characterset will cover all other urlcharacterset
  if (!urlValue) {
    NSMutableCharacterSet* characterSetForEncode = [[NSMutableCharacterSet alloc] init];
    [characterSetForEncode formUnionWithCharacterSet:[NSCharacterSet URLQueryAllowedCharacterSet]];
    [characterSetForEncode formUnionWithCharacterSet:[NSCharacterSet URLPathAllowedCharacterSet]];
    value = [value stringByAddingPercentEncodingWithAllowedCharacters:characterSetForEncode];
    urlValue = [[NSURL alloc] initWithString:value];
  }
  return urlValue;
}

#pragma mark prop setter

LYNX_PROP_SETTER("src", setSrc, NSString*) {
  [self markAsDirty];
  if (requestReset || value == nil) {
    self.image = nil;
    [self resetImage];
    _src = nil;
    return;
  }

  NSURL* newURL = [self illegalUrlHandler:value];

  if (!_src) {
    _src = [[LynxURL alloc] init];
    _src.type = LynxImageRequestSrc;
  }
  if (!newURL || ![newURL.absoluteString isEqualToString:_src.url.absoluteString]) {
    self.image = nil;
    if (!_deferSrcInvalidation) {
      [self resetImage];
    }
    _src.url = newURL;
    if (_enableGenericFetcher) {
      _src.request = [[LynxResourceRequest alloc] initWithUrl:newURL.absoluteString
                                                         type:LynxResourceTypeImage];
    }
    _src.imageSize = CGSizeZero;
  }
  _prevSize = CGSizeZero;
}

LYNX_PROP_SETTER("placeholder", setPlaceholder, NSString*) {
  [self markAsDirty];
  if (requestReset || value == nil) {
    self.image = nil;
    [self resetImage];
    _placeholder = nil;
    return;
  }

  NSURL* newURL = [self illegalUrlHandler:value];
  if (!_placeholder) {
    _placeholder = [[LynxURL alloc] init];
    _placeholder.type = LynxImageRequestPlaceholder;
  }
  if (!newURL || ![newURL.absoluteString isEqualToString:_placeholder.url.absoluteString]) {
    self.image = nil;
    if (!_deferSrcInvalidation) {
      [self resetImage];
    }
    _placeholder.imageSize = CGSizeZero;
    _placeholder.url = newURL;
    if (_enableGenericFetcher) {
      _placeholder.request = [[LynxResourceRequest alloc] initWithUrl:newURL.absoluteString
                                                                 type:LynxResourceTypeImage];
    }
  }
}

LYNX_PROP_SETTER("defer-src-invalidation", setDeferSrcInvalidation, BOOL) {
  if (requestReset) {
    value = false;
  }
  _deferSrcInvalidation = value;
}

LYNX_PROP_SETTER("mode", setMode, UIViewContentMode) {
  [self markAsDirty];
  if (requestReset) {
    value = UIViewContentModeScaleToFill;
  }
  if (_resizeMode != value || self.view.contentMode != value) {
    _resizeMode = value;
    [self.propsDidUpdateBlockArray addObject:^(LynxUI* ui) {
      ((LynxUIImage*)ui).view.contentMode = value;
    }];
  }
}

LYNX_PROP_SETTER("tint-color", setTintColor, NSString*) {
  [self markAsDirty];
  if (requestReset) {
    value = nil;
  }
  _tintColor = [LynxColorUtils convertNSStringToUIColor:value];
  [self.propsDidUpdateBlockArray addObject:^(LynxUI* ui) {
    if (ui.view == nil) {
      return;
    }
    LynxUIImage* image = (LynxUIImage*)ui;
    [image.view setTintColor:image.tintColor];
  }];
}

LYNX_PROP_SETTER("autoplay", setAutoPlay, BOOL) {
  [self markAsDirty];
  if (requestReset) {
    value = YES;
  }

  _autoPlay = value;
}

LYNX_PROP_SETTER("cover-start", setCoverStart, BOOL) {
  if (requestReset) {
    value = false;
  }
  if (_coverStart != value) {
    _coverStart = value;
  }
}

LYNX_PROP_SETTER("blur-radius", setBlurRadius, NSString*) {
  [self markAsDirty];
  if (requestReset) {
    _blurRadius = 0;
    return;
  }

  [self.propsDidUpdateBlockArray addObject:^(LynxUI* ui) {
    LynxUIImage* image = (LynxUIImage*)ui;

    LynxUI* rootUI = (LynxUI*)image.context.rootUI;
    UIView* rootView = image.context.rootView;
    LynxScreenMetrics* screenMetrics = image.context.screenMetrics;
    image.blurRadius = [LynxUnitUtils toPtWithScreenMetrics:screenMetrics
                                                  unitValue:value
                                               rootFontSize:rootUI.fontSize
                                                curFontSize:image.fontSize
                                                  rootWidth:rootView.frame.size.width
                                                 rootHeight:rootView.frame.size.height
                                              withDefaultPt:0];
  }];
}

LYNX_PROP_SETTER("capInsets", setInnerCapInsets, NSString*) {
  [self markAsDirty];
  if (requestReset) {
    _capInsets = UIEdgeInsetsZero;
  }
  UIEdgeInsets capInsets = _capInsets;
  NSArray* capInsetsProps = [value componentsSeparatedByString:@" "];
  const NSInteger count = [capInsetsProps count];

  capInsets.top = [self toCapInsetValue:count > 0 ? capInsetsProps[0] : nil];
  capInsets.right = count > 1 ? [self toCapInsetValue:capInsetsProps[1]] : capInsets.top;
  capInsets.bottom = count > 2 ? [self toCapInsetValue:capInsetsProps[2]] : capInsets.top;
  capInsets.left = count > 3 ? [self toCapInsetValue:capInsetsProps[3]] : capInsets.right;
  if (!UIEdgeInsetsEqualToEdgeInsets(_capInsets, capInsets)) {
    _capInsets = capInsets;
  }
}

LYNX_PROP_SETTER("cap-insets", setCapInsets, NSString*) {
  [self markAsDirty];
  [self setInnerCapInsets:value requestReset:requestReset];
}

LYNX_PROP_SETTER("cap-insets-scale", setCapInsetsScale, NSString*) {
  [self markAsDirty];
  if (requestReset) {
    _capInsetsScale = 1.0;
    return;
  }
  _capInsetsScale = [value floatValue];
}

LYNX_PROP_SETTER("loop-count", setLoopCount, NSInteger) {
  [self markAsDirty];
  if (requestReset) {
    value = 0;
  }
  self.loopCount = value;
}

LYNX_PROP_SETTER("prefetch-width", setPreFetchWidth, NSString*) {
  [self markAsDirty];
  if (requestReset) {
    value = @"-1px";
  }
  LynxScreenMetrics* screenMetrics = self.context.screenMetrics;

  _preFetchWidth = [LynxUnitUtils toPtWithScreenMetrics:screenMetrics
                                              unitValue:value
                                           rootFontSize:0
                                            curFontSize:0
                                              rootWidth:0
                                             rootHeight:0
                                          withDefaultPt:-1];
}

LYNX_PROP_SETTER("prefetch-height", setPreFetchHeight, NSString*) {
  [self markAsDirty];
  if (requestReset) {
    value = @"-1px";
  }

  LynxScreenMetrics* screenMetrics = self.context.screenMetrics;
  _preFetchHeight = [LynxUnitUtils toPtWithScreenMetrics:screenMetrics
                                               unitValue:value
                                            rootFontSize:0
                                             curFontSize:0
                                               rootWidth:0
                                              rootHeight:0
                                           withDefaultPt:-1];
}

LYNX_PROP_SETTER("downsampling", setDownsampling, BOOL) {
  [self markAsDirty];
  if (requestReset) {
    value = NO;
  }
  _downsampling = value;
}

LYNX_PROP_SETTER("use-new-image", setUseNewImage, BOOL) {
  if (requestReset) {
    value = NO;
  }
  _useNewImage = value;
}

LYNX_PROP_SETTER("auto-size", setAutoSize, BOOL) {
  [self markAsDirty];
  if (requestReset) {
    value = NO;
  }
  _autoSize = value;
}

/**
 * @name: ignore-cdn-downgrade-cache-policy
 * @description:  If set, the downgraded CDN image will be store to disk cache.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.9
 **/
LYNX_PROP_SETTER("ignore-cdn-downgrade-cache-policy", setIgnoreCDNDowngradeCachePolicy, BOOL) {
  if (requestReset) {
    value = NO;
  }
  if (value) {
    _requestOptions = _requestOptions | LynxImageIgnoreCDNDowngradeCachePolicy;
  } else if (_requestOptions & LynxImageIgnoreCDNDowngradeCachePolicy) {
    _requestOptions ^= LynxImageIgnoreCDNDowngradeCachePolicy;
  }
}

/**
 * @name: ignore-memory-cache
 * @description:  If set, the request will not search memory cache.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.9
 **/
LYNX_PROP_SETTER("ignore-memory-cache", setIgnoreMemoryCache, BOOL) {
  if (requestReset) {
    value = NO;
  }
  if (value) {
    _requestOptions = _requestOptions | LynxImageIgnoreMemoryCache;
  } else if (_requestOptions & LynxImageIgnoreMemoryCache) {
    _requestOptions ^= LynxImageIgnoreMemoryCache;
  }
}

/**
 * @name: ignore-disk-cache
 * @description:  If set, the request will not search disk cache.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.9
 **/
LYNX_PROP_SETTER("ignore-disk-cache", setIgnoreDiskCache, BOOL) {
  if (requestReset) {
    value = NO;
  }
  if (value) {
    _requestOptions = _requestOptions | LynxImageIgnoreDiskCache;
  } else if (_requestOptions & LynxImageIgnoreDiskCache) {
    _requestOptions ^= LynxImageIgnoreDiskCache;
  }
}

/**
 * @name: not-cache-to-memory
 * @description:  If set, the requested image will not be stored to memory cache.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.9
 **/
LYNX_PROP_SETTER("not-cache-to-memory", setNotCacheToMemory, BOOL) {
  if (requestReset) {
    value = NO;
  }
  if (value) {
    _requestOptions = _requestOptions | LynxImageNotCacheToMemory;
  } else if (_requestOptions & LynxImageNotCacheToMemory) {
    _requestOptions ^= LynxImageNotCacheToMemory;
  }
}

/**
 * @name: not-cache-to-disk
 * @description:  If set, the requested image will not be stored to disk cache.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.9
 **/
LYNX_PROP_SETTER("not-cache-to-disk", setNotCacheToDisk, BOOL) {
  if (requestReset) {
    value = NO;
  }
  if (value) {
    _requestOptions = _requestOptions | LynxImageNotCacheToDisk;
  } else if (_requestOptions & LynxImageNotCacheToDisk) {
    _requestOptions ^= LynxImageNotCacheToDisk;
  }
}

/**
 * @name: extra-load-info
 * @description:  If enabled, the loading callback will include additional information, which may
 *negatively impact performance. It is recommended to use this feature in DEBUG mode.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.12
 **/
LYNX_PROP_SETTER("extra-load-info", setBindLoadExtraInfo, BOOL) {
  if (requestReset) {
    value = NO;
  }
  _enableExtraLoadInfo = value;
}

/**
 * @name: skip-redirection
 * @description: If enabled, can skip fetchLocalResourceSync logic from Forest. Used only in
 *NewImage
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.14
 **/
LYNX_PROP_SETTER("skip-redirection", setSkipRedirection, BOOL) {
  if (requestReset) {
    value = NO;
  }
  _skipRedirection = value;
}

/**
 * @name: image-transition-style
 * @description: If enabled, image will show fade-in animation on first load
 *NewImage
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.14
 **/
LYNX_PROP_SETTER("image-transition-style", setTransitionStyle, NSString*) {
  [self markAsDirty];
  if (requestReset) {
    value = @"none";
  }
  _enableFadeIn = [@"fadeIn" isEqualToString:value];
}

LYNX_PROP_SETTER("additional-custom-info", setAdditionalCustomInfo, NSDictionary*) {
  if (requestReset) {
    value = nil;
  }
  _additional_custom_info = value;
}

LYNX_PROP_SETTER("enable-super-resolution", setEnableSuperResolution, BOOL) {
  if (requestReset) {
    value = NO;
  }
  _enableImageSR = value;
}

LYNX_PROP_SETTER("request-priority", setRequestPriority, NSString*) {
  if (requestReset) {
    value = nil;
  }
  _request_priority = value;
}

LYNX_PROP_SETTER("cache-choice", setCacheChoice, NSString*) {
  if (requestReset) {
    value = nil;
  }
  _cache_choice = value;
}

LYNX_PROP_SETTER("placeholder-hash-config", setPlaceHolderHash, NSDictionary*) {
  if (requestReset) {
    value = nil;
  }
  _placeholder_hash_config = value;
}

#pragma mark UI_Method

LYNX_UI_METHOD(startAnimate) {
  if (![[LynxImageLoader imageService] restartImageIfPossible:self.view callback:callback]) {
    [self.view stopAnimating];
    [self restartAnimation];
  }
}

LYNX_UI_METHOD(pauseAnimation) {
  [[LynxImageLoader imageService] pauseImage:self.view callback:callback];
}

LYNX_UI_METHOD(resumeAnimation) {
  [[LynxImageLoader imageService] resumeImage:self.view callback:callback];
}

LYNX_UI_METHOD(stopAnimation) {
  [[LynxImageLoader imageService] stopImage:self.view callback:callback];
}

- (CGFloat)toCapInsetValue:(NSString*)unitValue {
  const CGSize rootSize = self.context.rootView.frame.size;
  LynxScreenMetrics* screenMetrics = self.context.screenMetrics;
  return [LynxUnitUtils toPtWithScreenMetrics:screenMetrics
                                    unitValue:unitValue
                                 rootFontSize:((LynxUI*)self.context.rootUI).fontSize
                                  curFontSize:self.fontSize
                                    rootWidth:rootSize.width
                                   rootHeight:rootSize.height
                                withDefaultPt:0];
}

- (CGSize)frameSize {
  return CGRectIntegral(self.frame).size;
}

- (id)drawParameter {
  LynxUIImageDrawParameter* param = [[LynxUIImageDrawParameter alloc] init];
  param.image = self.image;
  param.borderWidth = self.backgroundManager.borderWidth;
  param.borderRadius = self.backgroundManager.borderRadius;
  param.padding = self.padding;
  param.frame = CGRectIntegral(self.frame);
  param.resizeMode = self.resizeMode;
  return param;
}

+ (void)drawRect:(CGRect)bounds withParameters:(id)drawParameters {
  LynxUIImageDrawParameter* param = drawParameters;

  UIEdgeInsets borderWidth = param.borderWidth;
  UIEdgeInsets padding = param.padding;
  LynxBorderRadii borderRadius = param.borderRadius;

  CGRect borderBounds;
  CGFloat initialBoundsWidth =
      param.frame.size.width - borderWidth.left - borderWidth.right - padding.left - padding.right;
  CGFloat initialBoundsHeight =
      param.frame.size.height - borderWidth.top - borderWidth.bottom - padding.top - padding.bottom;
  if (param.resizeMode == UIViewContentModeScaleAspectFit) {
    CGFloat min_scale = MIN(initialBoundsWidth / param.image.size.width,
                            initialBoundsHeight / param.image.size.height);
    CGFloat borderBoundsWidth = param.image.size.width * min_scale;
    CGFloat borderBoundsHeight = param.image.size.height * min_scale;
    borderBounds =
        CGRectMake(borderWidth.left + padding.left + initialBoundsWidth / 2 - borderBoundsWidth / 2,
                   borderWidth.top + padding.top + initialBoundsHeight / 2 - borderBoundsHeight / 2,
                   borderBoundsWidth, borderBoundsHeight);
  } else if (param.resizeMode == UIViewContentModeScaleAspectFill) {
    CGFloat max_scale = MAX(initialBoundsWidth / param.image.size.width,
                            initialBoundsHeight / param.image.size.height);
    CGFloat borderBoundsWidth = param.image.size.width * max_scale;
    CGFloat borderBoundsHeight = param.image.size.height * max_scale;
    borderBounds =
        CGRectMake(borderWidth.left + padding.left + initialBoundsWidth / 2 - borderBoundsWidth / 2,
                   borderWidth.top + padding.top + initialBoundsHeight / 2 - borderBoundsHeight / 2,
                   borderBoundsWidth, borderBoundsHeight);
  } else if (param.resizeMode == UIViewContentModeScaleToFill) {
    borderBounds = CGRectMake(borderWidth.left + padding.left, borderWidth.top + padding.top,
                              initialBoundsWidth, initialBoundsHeight);
  } else {  // "center"
    borderBounds = CGRectMake(
        borderWidth.left + padding.left + initialBoundsWidth / 2 - param.image.size.width / 2,
        borderWidth.top + padding.top + initialBoundsHeight / 2 - param.image.size.height / 2,
        param.image.size.width, param.image.size.height);
  }

  LynxBorderRadii radius = borderRadius;
  radius.topLeftX.val -= borderWidth.left + padding.left;
  radius.bottomLeftX.val -= borderWidth.left + padding.left;
  radius.topRightX.val -= borderWidth.right + padding.right;
  radius.bottomRightX.val -= borderWidth.right + padding.right;
  radius.topLeftY.val -= borderWidth.top + padding.top;
  radius.topRightY.val -= borderWidth.top + padding.top;
  radius.bottomLeftY.val -= borderWidth.bottom + padding.bottom;
  radius.bottomRightY.val -= borderWidth.bottom + padding.bottom;

  CGRect clipRaddiBounds =
      CGRectMake(borderWidth.left + padding.left, borderWidth.top + padding.top, initialBoundsWidth,
                 initialBoundsHeight);
  CGPathRef pathRef = [LynxBackgroundUtils createBezierPathWithRoundedRect:clipRaddiBounds
                                                               borderRadii:radius];

  CGContextRef ctx = UIGraphicsGetCurrentContext();
  CGContextAddPath(ctx, pathRef);
  CGContextClip(ctx);
  [param.image drawInRect:borderBounds];
  CGContextDrawPath(ctx, kCGPathFillStroke);
  CGPathRelease(pathRef);
}

- (void)restartAnimation {
  [super restartAnimation];
  if ([self isAnimated]) {
    [self startAnimating];
  }
}

- (BOOL)isAnimated {
  // use the image property of UIImageView to check whether this is an animated-image
  return [LynxUIImage isAnimatedImage:self.image];
}

- (BOOL)enableAccessibilityByDefault {
  return YES;
}

- (void)startAnimating {
  [self.view startAnimating];
}

- (CGSize)measureNode:(LynxLayoutNode*)node
            withWidth:(CGFloat)width
            widthMode:(LynxMeasureMode)widthMode
               height:(CGFloat)height
           heightMode:(LynxMeasureMode)heightMode {
  if (!_autoSize) {
    return CGSizeMake((widthMode == LynxMeasureModeDefinite) ? width : 0,
                      (heightMode == LynxMeasureModeDefinite) ? height : 0);
  }

  CGFloat tmpWidth = 0;
  CGFloat tmpHeight = 0;
  CGFloat imgWidth;
  CGFloat imgHeight;
  if (CGSizeEqualToSize(_src.imageSize, CGSizeZero) || _src.error) {
    imgWidth = _placeholder.imageSize.width;
    imgHeight = _placeholder.imageSize.height;
  } else {
    imgWidth = _src.imageSize.width;
    imgHeight = _src.imageSize.height;
  }

  if (widthMode == LynxMeasureModeIndefinite || widthMode == LynxMeasureModeAtMost) {
    tmpWidth = INFINITY;
  } else if (widthMode == LynxMeasureModeDefinite) {
    tmpWidth = width;
  }

  if (heightMode == LynxMeasureModeIndefinite || heightMode == LynxMeasureModeAtMost) {
    tmpHeight = INFINITY;
  } else if (heightMode == LynxMeasureModeDefinite) {
    tmpHeight = height;
  }

  if (tmpWidth == INFINITY && tmpHeight == INFINITY) {
    tmpWidth = imgWidth;
    tmpHeight = imgHeight;
  } else if (tmpWidth == INFINITY && tmpHeight != INFINITY) {
    tmpWidth = imgWidth * (tmpHeight / imgHeight);
  } else if (tmpWidth != INFINITY && tmpHeight == INFINITY) {
    tmpHeight = imgHeight * (tmpWidth / imgWidth);
  }

  if (widthMode == LynxMeasureModeAtMost) {
    tmpWidth = MIN(tmpWidth, width);
  }
  if (heightMode == LynxMeasureModeAtMost) {
    tmpHeight = MIN(tmpHeight, height);
  }

  return CGSizeMake(tmpWidth, tmpHeight);
}

- (UIAccessibilityTraits)accessibilityTraitsByDefault {
  return UIAccessibilityTraitImage;
}

+ (BOOL)isAnimatedImage:(UIImage*)image {
  return [[LynxImageLoader imageService] isAnimatedImage:image] || (image.images != nil);
}

@end

@implementation LynxConverter (UIViewContentMode)

+ (UIViewContentMode)toUIViewContentMode:(id)value {
  if (!value || [value isEqual:[NSNull null]]) {
    return UIViewContentModeScaleAspectFill;
  }
  NSString* valueStr = [self toNSString:value];
  if ([valueStr isEqualToString:@"aspectFit"]) {
    return UIViewContentModeScaleAspectFit;
  } else if ([valueStr isEqualToString:@"aspectFill"]) {
    return UIViewContentModeScaleAspectFill;
  } else if ([valueStr isEqualToString:@"scaleToFill"]) {
    return UIViewContentModeScaleToFill;
  } else if ([valueStr isEqualToString:@"center"]) {
    return UIViewContentModeCenter;
  } else {
    return UIViewContentModeScaleAspectFill;
  }
}

@end
