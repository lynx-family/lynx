// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxSVGRenderEngine.h>

#import <Lynx/LynxError.h>
#import <Lynx/LynxResourceRequest.h>
#import <Lynx/LynxThreadSafeDictionary.h>
#import <ServalSVG/SrSVG.h>

@interface LynxSVGRenderEngine () {
  SrSVG *_srSvg;
  id<LynxGenericResourceFetcher> _genericFetcher;
  id<LynxMediaResourceFetcher> _mediaFetcher;
  LynxSVGRenderAsyncExecutor _asyncExecutor;
  dispatch_block_t _reloadBlock;
  LynxThreadSafeDictionary *_imageHolder;

  BOOL _isRendering;
  BOOL _needRender;
  BOOL _isCancelled;
  CGSize _pendingSize;
}
@end

@implementation LynxSVGRenderEngine

- (instancetype)initWithSrSVG:(SrSVG *)srSvg
                        color:(nullable NSString *)color
               genericFetcher:(id<LynxGenericResourceFetcher>)genericFetcher
                 mediaFetcher:(id<LynxMediaResourceFetcher>)mediaFetcher
                asyncExecutor:(LynxSVGRenderAsyncExecutor)asyncExecutor
                  reloadBlock:(dispatch_block_t)reloadBlock {
  if (self = [super init]) {
    _srSvg = srSvg;
    _color = [color copy];
    _genericFetcher = genericFetcher;
    _mediaFetcher = mediaFetcher;
    _asyncExecutor = [asyncExecutor copy];
    _reloadBlock = [reloadBlock copy];
  }
  return self;
}

- (void)renderWithSize:(CGSize)devSize
            completion:(LynxSVGRenderCompletion)completion
          errorHandler:(LynxSVGRenderErrorHandler)errorHandler {
  if (devSize.width <= 0 || devSize.height <= 0) {
    if (completion) {
      completion(nil);
    }
    return;
  }

  @synchronized(self) {
    if (_isRendering) {
      _needRender = YES;
      _pendingSize = devSize;
      return;
    }

    if ((!_src || _src.length == 0) && (!_content || _content.length == 0)) {
      if (completion) {
        completion(nil);
      }
      return;
    }

    _isRendering = YES;
    _needRender = NO;
    _isCancelled = NO;
  }

  [self startRenderWithSize:devSize completion:completion errorHandler:errorHandler];
}

- (void)startRenderWithSize:(CGSize)devSize
                 completion:(LynxSVGRenderCompletion)completion
               errorHandler:(LynxSVGRenderErrorHandler)errorHandler {
  __weak __typeof(self) weakSelf = self;

  LynxSVGRenderCompletion wrappedCompletion = ^(UIImage *_Nullable image) {
    __strong __typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf) {
      if (completion) {
        completion(image);
      }
      return;
    }

    BOOL wasCancelled = NO;
    BOOL needsFollowUp = NO;
    CGSize followUpSize = CGSizeZero;
    @synchronized(strongSelf) {
      wasCancelled = strongSelf->_isCancelled;
      strongSelf->_isRendering = NO;
      if (strongSelf->_needRender && !strongSelf->_isCancelled) {
        needsFollowUp = YES;
        followUpSize = strongSelf->_pendingSize;
        strongSelf->_needRender = NO;
      }
    }

    if (!wasCancelled) {
      if (completion) {
        completion(image);
      }
    }

    if (needsFollowUp) {
      [strongSelf renderWithSize:followUpSize completion:completion errorHandler:errorHandler];
    }
  };

  if (_src.length > 0) {
    if (!_genericFetcher) {
      if (errorHandler) {
        LynxError *error =
            [LynxError lynxErrorWithCode:LynxErrorCodeForResourceError
                                 message:@"genericFetcher is nil, cannot fetch SVG resource."];
        errorHandler(error);
      }
      wrappedCompletion(nil);
      return;
    }

    LynxResourceRequest *request = [[LynxResourceRequest alloc] initWithUrl:_src
                                                                       type:LynxResourceTypeSVG];
    [_genericFetcher fetchResource:request
                        onComplete:^(NSData *_Nullable data, NSError *_Nullable error) {
                          __strong __typeof(weakSelf) strongSelf = weakSelf;
                          if (!strongSelf) {
                            wrappedCompletion(nil);
                            return;
                          }

                          @synchronized(strongSelf) {
                            if (strongSelf->_isCancelled) {
                              wrappedCompletion(nil);
                              return;
                            }
                          }

                          if (!data) {
                            if (errorHandler) {
                              LynxError *lynxError =
                                  [LynxError lynxErrorWithCode:LynxErrorCodeForResourceError
                                                       message:@"Failed to fetch SVG resource."];
                              errorHandler(lynxError);
                            }
                            wrappedCompletion(nil);
                            return;
                          }

                          strongSelf->_asyncExecutor(
                              ^{
                                return [strongSelf renderSVGData:data withSize:devSize];
                              },
                              wrappedCompletion);
                        }];
  } else if (_content.length > 0) {
    NSData *data = [[_content stringByReplacingOccurrencesOfString:@"&quot;" withString:@"\""]
        dataUsingEncoding:NSUTF8StringEncoding];
    _asyncExecutor(
        ^{
          return [self renderSVGData:data withSize:devSize];
        },
        wrappedCompletion);
  }
}

- (UIImage *)renderSVGData:(NSData *)data withSize:(CGSize)devSize {
  if (data == nil || data.length == 0) {
    return nil;
  }
  if (!_imageHolder) {
    _imageHolder = [LynxThreadSafeDictionary new];
  }
  return [_srSvg getSrSvgDrawImageWithData:data
                                   andSize:devSize
                                  andColor:_color
                               andCallback:^UIImage *_Nullable(NSString *_Nullable href) {
                                 if (!href || href.length == 0) {
                                   return nil;
                                 }
                                 return [self loadImageFromHref:href withSize:devSize];
                               }];
}

- (UIImage *)loadImageFromHref:(NSString *)href withSize:(CGSize)devSize {
  (void)devSize;
  if (!href || href.length == 0) {
    return nil;
  }

  UIImage *image = _imageHolder[href];
  if (!image) {
    if (!_mediaFetcher) {
      return nil;
    }
    [_imageHolder setObject:[NSNull null] forKey:href];

    __weak __typeof(self) weakSelf = self;
    LynxResourceRequest *request = [[LynxResourceRequest alloc] initWithUrl:href];
    [_mediaFetcher fetchUIImage:request
                     onComplete:^(UIImage *_Nullable uiImage, NSError *_Nullable error) {
                       __strong __typeof(weakSelf) strongSelf = weakSelf;
                       if (!strongSelf) {
                         return;
                       }

                       @synchronized(strongSelf) {
                         if (strongSelf->_isCancelled) {
                           [strongSelf->_imageHolder removeObjectForKey:href];
                           return;
                         }
                       }

                       if (uiImage) {
                         [strongSelf->_imageHolder setObject:uiImage forKey:href];
                         if (strongSelf->_reloadBlock) {
                           dispatch_async(dispatch_get_main_queue(), strongSelf->_reloadBlock);
                         }
                       } else {
                         [strongSelf->_imageHolder removeObjectForKey:href];
                       }
                     }];
  }

  if ((NSNull *)image != [NSNull null]) {
    return image;
  }
  return nil;
}

- (void)cancel {
  @synchronized(self) {
    _isCancelled = YES;
    _isRendering = NO;
    _needRender = NO;
  }
}

@end
