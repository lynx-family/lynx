// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxImageLoader.h"
#import "LynxDebugger.h"
#import "LynxEnv.h"
#import "LynxImageProcessor.h"
#import "LynxLog.h"
#import "LynxNinePatchImageProcessor.h"
#import "LynxService.h"
#import "LynxTraceEvent.h"
#import "LynxUI.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

NSString* const LynxImageFetcherContextKeyUI = @"LynxImageFetcherContextKeyUI";
NSString* const LynxImageFetcherContextKeyLynxView = @"LynxImageFetcherContextKeyLynxView";
NSString* const LynxImageFetcherContextKeyDownsampling = @"LynxImageFetcherContextKeyDownsampling";
NSString* const LynxImageRequestOptions = @"LynxImageRequestOptions";
NSString* const LynxImageRequestContextModuleExtraData = @"LynxImageRequestContextModuleExtraData";
NSString* const LynxImageSkipRedirection = @"LynxImageSkipRedirection";
NSString* const LynxImageFixNewImageDownsampling = @"LynxImageFixNewImageDownsampling";
NSString* const LynxImageAdditionalCustomInfo = @"LynxImageAdditionalCustomInfo";
NSString* const LynxImageEnableSR = @"LynxImageEnableSR";
NSString* const LynxImageCacheChoice = @"LynxImageCacheChoice";
NSString* const LynxImageRequestPriority = @"LynxImageRequestPriority";
NSString* const LynxImagePlaceholderHashConfig = @"LynxImagePlaceholderHashConfig";
BOOL LynxImageFetchherSupportsProcessor(id<LynxImageFetcher> fetcher) {
  return [fetcher respondsToSelector:@selector(loadImageWithURL:
                                                     processors:size:contextInfo:completion:)];
}
@implementation LynxImageLoader
+ (nonnull instancetype)sharedInstance {
  static dispatch_once_t once;
  static id instance;
  dispatch_once(&once, ^{
    instance = [self new];
  });
  return instance;
}

- (dispatch_block_t)loadImageFromURL:(NSURL*)url
                                size:(CGSize)targetSize
                         contextInfo:(NSDictionary*)contextInfo
                          processors:(NSArray*)processors
                        imageFetcher:(id<LynxImageFetcher>)imageFetcher
                           completed:(LynxImageLoadCompletionBlock)completed {
  BOOL supportsProcessor = LynxImageFetchherSupportsProcessor(imageFetcher);
  if (supportsProcessor) {
    return [imageFetcher loadImageWithURL:url
                               processors:processors
                                     size:targetSize
                              contextInfo:contextInfo
                               completion:^(UIImage* _Nullable image, NSError* _Nullable error,
                                            NSURL* _Nullable imageURL) {
                                 completed(image, error, url);
                               }];
  }
  LynxImageLoadCompletionBlock completionBlock =
      ^(UIImage* _Nullable image, NSError* _Nullable error, NSURL* _Nullable imageURL) {
        if (error != nil) {
          _LogE(@"loadImageFromURL failed url:%@,%@", url, [error localizedDescription]);
        }
        // If there no processor, return image now
        if (!processors || processors.count == 0) {
          completed(image, error, url);
          return;
        }
        NSMutableArray* syncProcessors = [NSMutableArray array];
        NSMutableArray* asyncProcessors = [NSMutableArray array];
        for (id<LynxImageProcessor> processor in processors) {
          if ([processor isKindOfClass:[LynxNinePatchImageProcessor class]]) {
            [syncProcessors addObject:processor];
          } else {
            [asyncProcessors addObject:processor];
          }
        }
        UIImage* syncFilterImage = image;
        // Deal with sync processors
        for (id<LynxImageProcessor> processor in syncProcessors) {
          syncFilterImage = [processor processImage:syncFilterImage];
        }
        // If there no asyncProcessor, return image now
        if (asyncProcessors.count == 0) {
          completed(syncFilterImage, error, url);
          return;
        }
        // Deal with async processors
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
          UIImage* asyncFilterImage = syncFilterImage;
          for (id<LynxImageProcessor> processor in asyncProcessors) {
            asyncFilterImage = [processor processImage:asyncFilterImage];
          }
          // Return baked image to main thread
          dispatch_async(dispatch_get_main_queue(), ^{
            completed(asyncFilterImage, error, url);
          });
        });
      };
  if ([imageFetcher respondsToSelector:@selector(loadImageWithURL:size:contextInfo:completion:)]) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxImageFetcher::loadImageWithURL", "url",
                [url.absoluteString UTF8String]);
    return [imageFetcher loadImageWithURL:url
                                     size:targetSize
                              contextInfo:contextInfo
                               completion:completionBlock];
  } else if ([imageFetcher respondsToSelector:@selector(cancelableLoadImageWithURL:
                                                                              size:completion:)]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    return [imageFetcher cancelableLoadImageWithURL:url size:targetSize completion:completionBlock];
  } else if ([imageFetcher respondsToSelector:@selector(loadImageWithURL:size:completion:)]) {
    [imageFetcher loadImageWithURL:url size:targetSize completion:completionBlock];
#pragma clang diagnostic pop
  }
  return nil;
}

+ (id<LynxServiceImageProtocol>)imageService {
  static id<LynxServiceImageProtocol> _imageService;
  if (!_imageService) {
    _imageService = LynxService(LynxServiceImageProtocol);
  }
  return _imageService;
}

- (dispatch_block_t)loadImageFromLynxURL:(LynxURL*)requestUrl
                                    size:(CGSize)targetSize
                             contextInfo:(NSDictionary*)contextInfo
                              processors:(NSArray*)processors
                            imageFetcher:(id<LynxImageFetcher>)imageFetcher
                             LynxUIImage:(LynxUIImage*)lynxUIImage
                    enableGenericFetcher:(BOOL)enableGenericFetcher
                               completed:(LynxImageLoadCompletionBlock)completed {
  BOOL shouldUseImageService = !imageFetcher;
  if (shouldUseImageService) {
    return [[LynxImageLoader imageService] loadNewImageFromURL:requestUrl
                                                          size:targetSize
                                          enableGenericFetcher:enableGenericFetcher
                                                   contextInfo:contextInfo
                                                    processors:processors
                                                     completed:completed
                                                   LynxUIImage:lynxUIImage];
  } else {
    // We will deprecate LynxImageLoader once ImageService is ready !!!
    NSURL* url = requestUrl.url;
    return [self loadImageFromURL:url
                             size:targetSize
                      contextInfo:contextInfo
                       processors:processors
                     imageFetcher:imageFetcher
                        completed:completed];
  }
  return nil;
}

@end
