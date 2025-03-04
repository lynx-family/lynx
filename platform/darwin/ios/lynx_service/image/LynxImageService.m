// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxImageService.h"
#import <Lynx/LynxLog.h>
#import <Lynx/LynxNinePatchImageProcessor.h>
#import <Lynx/LynxResourceFetcher.h>
#import <Lynx/LynxSubErrorCode.h>
#import <Lynx/LynxTraceEvent.h>
#import <SDWebImage/SDWebImage.h>
@interface LynxImageService ()

@property(nonatomic, weak) LynxUIImage* imageUI;
@end

@LynxServiceRegister(LynxImageService) @implementation LynxImageService

+ (NSString*)serviceBizID {
  return DEFAULT_LYNX_SERVICE;
}

+ (LynxServiceScope)serviceScope {
  return LynxServiceScopeDefault;
}

+ (NSUInteger)serviceType {
  return kLynxServiceImage;
}

- (void)addAnimatedImageCallBack:(nonnull id)view UI:(nonnull LynxUIImage*)ui {
  if (![self checkImageType:view]) {
    return;
  }
  // anim played callback,it's not supported yet.We will support it later.
}

- (void)appendExtraImageLoadDetailForEvent:(nonnull UIImage*)image
                            originalDetail:(nonnull NSMutableDictionary*)detail {
  // append custom info when enable extra-load-info
}

- (BOOL)checkImageType:(nonnull id)view {
  if (![view isKindOfClass:[SDAnimatedImageView class]]) {
    return NO;
  }
  return YES;
}

- (nonnull NSNumber*)getMappedCategorizedPicErrorCode:(nonnull NSNumber*)errorCode {
  // Custom error code mapping
  NSInteger code = [errorCode intValue];
  if (code == SDWebImageErrorCancelled || code == SDWebImageErrorInvalidURL ||
      code == SDWebImageErrorBlackListed) {
    return [NSNumber numberWithInteger:ECLynxResourceImageFromUserOrDesign];
  } else if (code == SDWebImageErrorBadImageData || code == SDWebImageErrorCacheNotModified) {
    return [NSNumber numberWithInteger:ECLynxResourceImagePicSource];
  } else if (code == SDWebImageErrorInvalidDownloadContentType ||
             code == SDWebImageErrorInvalidDownloadResponse ||
             code == SDWebImageErrorInvalidDownloadStatusCode ||
             code == SDWebImageErrorInvalidDownloadOperation) {
    return [NSNumber numberWithInteger:ECLynxResourceImageFromNetworkOrOthers];
  }
  return [NSNumber numberWithInteger:ECLynxResourceException];
}

- (nonnull UIImageView*)imageView {
  SDAnimatedImageView* imageView = [[SDAnimatedImageView alloc] init];
  imageView.shouldCustomLoopCount = YES;
  imageView.animationRepeatCount = 0;
  return imageView;
}

- (BOOL)isAnimatedImage:(nonnull UIImage*)image {
  if ([image isKindOfClass:[SDAnimatedImage class]]) {
    return ((SDAnimatedImage*)image).animatedImageFrameCount >= 1;
  }
  return NO;
}

- (void)handleAnimatedImage:(UIImage*)image
                       view:(UIImageView*)imageView
                  loopCount:(NSInteger)loopCount {
  if ([image isKindOfClass:[SDAnimatedImage class]]) {
    SDAnimatedImage* currentImage = (SDAnimatedImage*)image;
    SDAnimatedImageView* currentImageView = (SDAnimatedImageView*)imageView;
    currentImageView.animationRepeatCount = loopCount;
    currentImageView.image = currentImage;
    return;
  }
  imageView.image = image;
  imageView.animationImages = image.images;
  if (image.images != nil && [image.images count] > 1) {
    imageView.animationDuration = image.duration;
    imageView.animationRepeatCount = loopCount;
    imageView.image = [image.images objectAtIndex:[image.images count] - 1];
    [imageView startAnimating];
  }
}

- (nonnull dispatch_block_t)loadNewImageFromURL:(nonnull LynxURL*)requestUrl
                                           size:(CGSize)targetSize
                           enableGenericFetcher:(BOOL)enableGenericFetcher
                                    contextInfo:(nonnull NSDictionary*)contextInfo
                                     processors:(nonnull NSArray*)processors
                                      completed:(nonnull LynxImageLoadCompletionBlock)completed
                                    LynxUIImage:(nonnull LynxUIImage*)lynxImage {
  _imageUI = lynxImage;
  NSString* urlStr = requestUrl.url.absoluteString;
  BOOL isBase64 = [urlStr hasPrefix:@"data:image"];
  __weak typeof(self) weakSelf = self;
  BOOL shouldSkipRedirection = NO;
  if ([contextInfo objectForKey:LynxImageSkipRedirection]) {
    shouldSkipRedirection = [[contextInfo objectForKey:LynxImageSkipRedirection] boolValue];
  }

  // skip resolve if image is base64 format
  if (!isBase64 && !shouldSkipRedirection && enableGenericFetcher) {
    LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, @"MediaFetcher.shouldRedirectImageUrl")
    LynxResourceOptionalBool isLocalResource = LynxResourceOptionalBoolUndefined;
    if ([_imageUI.context.mediaResourceFetcher respondsToSelector:@selector(isLocalResource:)]) {
      isLocalResource = [_imageUI.context.mediaResourceFetcher isLocalResource:requestUrl.url];
    }
    if (isLocalResource != LynxResourceOptionalBoolFalse) {
      urlStr = [_imageUI.context.mediaResourceFetcher shouldRedirectUrl:requestUrl.request];
    }
    LLog(@"[lynx]originalURL %@, resolvedURL %@", requestUrl.url.absoluteURL, urlStr);
    LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
  }
  if (isBase64 || [urlStr isEqualToString:requestUrl.url.absoluteString]) {
    requestUrl.redirectedURL = requestUrl.url;
  } else {
    requestUrl.redirectedURL = [NSURL URLWithString:urlStr];
  }
  SDWebImageOptions requestOptions = 0;
  if ([contextInfo objectForKey:LynxImageRequestPriority]) {
    NSString* priority = [contextInfo objectForKey:LynxImageRequestPriority];
    if (priority && [priority isEqualToString:@"low"]) {
      requestOptions = SDWebImageLowPriority;
    } else if (priority && [priority isEqualToString:@"high"]) {
      requestOptions = SDWebImageHighPriority;
    }
  }

  NSMutableDictionary* context = [NSMutableDictionary dictionary];

  if ([[contextInfo objectForKey:LynxImageFetcherContextKeyDownsampling] boolValue]) {
    CGSize thumbnailSize = CGSizeMake(targetSize.width * [UIScreen mainScreen].scale,
                                      targetSize.height * [UIScreen mainScreen].scale);
    context[SDWebImageContextImageThumbnailPixelSize] = [NSValue valueWithCGSize:thumbnailSize];
  }
  if (_imageUI != nil) {
    context[SDWebImageContextAnimatedImageClass] = [SDAnimatedImage class];
  }
  SDWebImageManager* manager = [SDWebImageManager sharedManager];
  [manager
      loadImageWithURL:requestUrl.redirectedURL
               options:requestOptions
               context:context
              progress:nil
             completed:^(UIImage* _Nullable image, NSData* _Nullable data, NSError* _Nullable error,
                         SDImageCacheType cacheType, BOOL finished, NSURL* _Nullable imageURL) {
               typeof(weakSelf) strongSelf = weakSelf;
               if (strongSelf) {
                 LynxImageLoadCompletionBlock recordBlock =
                     ^(UIImage* _Nullable image, NSError* _Nullable error,
                       NSURL* _Nullable imageURL) {
                       completed(image, error, requestUrl.url);
                     };
                 NSMutableArray* syncProcessors = [NSMutableArray array];
                 NSMutableArray* asyncProcessors = [NSMutableArray array];
                 for (id<LynxImageProcessor> processor in processors) {
                   if ([processor isKindOfClass:[LynxNinePatchImageProcessor class]]) {
                     [syncProcessors addObject:processor];
                   } else {
                     [asyncProcessors addObject:processor];
                   }
                 }
                 UIImage* syncProcessedImage = [self syncProcessorsHandler:syncProcessors
                                                                 withImage:image];
                 if (asyncProcessors.count == 0) {
                   recordBlock(syncProcessedImage, error, requestUrl.url);
                 } else {
                   dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                     UIImage* asyncProcessedImage = syncProcessedImage;
                     for (id<LynxImageProcessor> processor in asyncProcessors) {
                       asyncProcessedImage = [processor processImage:asyncProcessedImage];
                     }
                     dispatch_async(dispatch_get_main_queue(), ^{
                       recordBlock(asyncProcessedImage, error, requestUrl.url);
                     });
                   });
                 }
               }
             }];
  return ^{
    typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf) {
      if ([strongSelf.imageUI.customImageRequest
              isKindOfClass:[SDWebImageCombinedOperation class]]) {
        [(SDWebImageCombinedOperation*)strongSelf.imageUI.customImageRequest cancel];
      }
      strongSelf.imageUI.customImageRequest = nil;
    }
  };
}

- (void)prefetchImage:(nonnull LynxURL*)url params:(nullable NSDictionary*)params {
  NSString* priority = [params objectForKey:@"priority"];
  NSString* cacheTarget = [params objectForKey:@"cacheTarget"];
  SDWebImageOptions options = SDWebImageContinueInBackground;
  if (priority && [priority isEqualToString:@"low"]) {
    options |= SDWebImageLowPriority;
  } else if (priority && [priority isEqualToString:@"high"]) {
    options |= SDWebImageHighPriority;
  }
  if (cacheTarget && [cacheTarget isEqualToString:@"disk"]) {
    options |= SDWebImageAvoidDecodeImage;
  }
  SDWebImageManager* manager = [SDWebImageManager sharedManager];
  [manager
      loadImageWithURL:url.url
               options:options
              progress:nil
             completed:^(UIImage* _Nullable image, NSData* _Nullable data, NSError* _Nullable error,
                         SDImageCacheType cacheType, BOOL finished, NSURL* _Nullable imageURL){
                 // no need to impl

             }];
}

- (void)reportResourceStatus:(nonnull LynxView*)LynxView
                        data:(nonnull NSMutableDictionary*)data
                       extra:(NSDictionary* _Nullable)extra {
  // can report image event here
}

#pragma mark processors

- (UIImage*)syncProcessorsHandler:(NSArray*)processors withImage:(UIImage*)image {
  UIImage* syncFilterImage = image;
  for (id<LynxImageProcessor> processor in processors) {
    if ([processor isKindOfClass:[LynxNinePatchImageProcessor class]]) {
      syncFilterImage = [processor processImage:syncFilterImage];
    }
  }
  return syncFilterImage;
}

#pragma mark callback

- (void)successCallback:(LynxUIMethodCallbackBlock _Nullable)callback message:(NSString*)msg {
  if (nil != callback) {
    callback(kUIMethodSuccess, msg);
  }
}

- (void)failCallback:(LynxUIMethodCallbackBlock _Nullable)callback message:(NSString*)msg {
  if (nil != callback) {
    callback(kUIMethodParamInvalid, msg);
  }
}

- (BOOL)restartImageIfPossible:(nonnull id)view
                      callback:(LynxUIMethodCallbackBlock _Nullable)callback {
  if ([view isKindOfClass:[SDAnimatedImageView class]]) {
    SDAnimatedImageView* customizedView = (SDAnimatedImageView*)view;
    customizedView.resetFrameIndexWhenStopped = YES;
    [customizedView stopAnimating];
    [customizedView startAnimating];
    [self successCallback:callback message:@"Animation restarted."];
    return YES;
  }
  return NO;
}

- (void)resumeImage:(nonnull id)view callback:(LynxUIMethodCallbackBlock _Nullable)callback {
  if (![self checkImageType:view]) {
    return;
  }
  SDAnimatedImageView* customizedView = (SDAnimatedImageView*)view;
  [customizedView startAnimating];
  [self successCallback:callback message:@"Animation resumed."];
}

- (void)setAutoPlay:(nonnull id)view value:(BOOL)autoPlay {
  if ([view isKindOfClass:SDAnimatedImageView.class]) {
    SDAnimatedImageView* customView = (SDAnimatedImageView*)view;
    customView.autoPlayAnimatedImage = autoPlay;
  }
}

- (void)stopImage:(nonnull id)view callback:(LynxUIMethodCallbackBlock _Nullable)callback {
  if (![self checkImageType:view]) {
    return;
  }
  SDAnimatedImageView* customizedView = (SDAnimatedImageView*)view;
  customizedView.resetFrameIndexWhenStopped = YES;
  [customizedView stopAnimating];
  [self successCallback:callback message:@"Animation stopped."];
}

- (void)pauseImage:(nonnull id)view callback:(LynxUIMethodCallbackBlock _Nullable)callback {
  if (![self checkImageType:view]) {
    return;
  }
  SDAnimatedImageView* customizedView = (SDAnimatedImageView*)view;
  customizedView.resetFrameIndexWhenStopped = NO;
  [customizedView stopAnimating];
  [self successCallback:callback message:@"Animation paused."];
}

@end
