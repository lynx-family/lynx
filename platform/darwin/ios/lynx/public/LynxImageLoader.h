// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxImageFetcher.h>
#import <Lynx/LynxServiceImageProtocol.h>
#import <Lynx/LynxUIImage.h>
#import <Lynx/LynxURL.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxImageLoader : NSObject

+ (nonnull instancetype)sharedInstance;

+ (id<LynxServiceImageProtocol>)imageService;

/**
 Load image from remote. After image downloaded, processors will be use to preprocess
 image in order on background thread. Completed block will be called after every
 thing prepare on main thread

 @param requestUrl     the image url
 @param processors  processor list to process image
 @param completed   callback when image ready
 */

- (dispatch_block_t)loadImageFromLynxURL:(LynxURL*)requestUrl
                                    size:(CGSize)targetSize
                             contextInfo:(NSDictionary*)contextInfo
                              processors:(nullable NSArray*)processors
                            imageFetcher:(nullable id<LynxImageFetcher>)imageFetcher
                             LynxUIImage:(nullable LynxUIImage*)lynxUIImage
                    enableGenericFetcher:(BOOL)enableGenericFetcher
                               completed:(LynxImageLoadCompletionBlock)completed;

#pragma mark - deprecated API
/**
 Deprecated:
 This method is deprecated.
 For Lynx internal components, please use loadImageFromLynxURL. For external components, use
 LynxImageFetcher to encapsulate the functionality. */
- (dispatch_block_t)loadImageFromURL:(NSURL*)url
                                size:(CGSize)targetSize
                         contextInfo:(NSDictionary*)contextInfo
                          processors:(NSArray*)processors
                        imageFetcher:(id<LynxImageFetcher>)imageFetcher
                           completed:(LynxImageLoadCompletionBlock)completed;

@end
#if defined __cplusplus
extern "C" {
#endif
BOOL LynxImageFetchherSupportsProcessor(id<LynxImageFetcher> fetcher);
#if defined __cplusplus
};
#endif
NS_ASSUME_NONNULL_END
