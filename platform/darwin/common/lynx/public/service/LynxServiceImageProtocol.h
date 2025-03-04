// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEIMAGEPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEIMAGEPROTOCOL_H_
#import <Foundation/Foundation.h>
#if TARGET_OS_IOS
#import <UIKit/UIKit.h>
#import "LynxServiceProtocol.h"
#import "LynxUIImage.h"

NS_ASSUME_NONNULL_BEGIN
typedef void (^LynxImageLoadCompletionBlock)(UIImage *_Nullable image, NSError *_Nullable error,
                                             NSURL *_Nullable imageURL);

@protocol LynxServiceImageProtocol <LynxServiceProtocol>

/**
 * Initializes and returns a specific UIImageView implementation required by the image library.
 * If no specific implementation is needed, it simply returns a standard UIImageView.
 */
- (UIImageView *)imageView;

/**
 * Pauses the image playback and invokes the corresponding callback to return the result.
 *
 * @param view The view containing the image to be paused.
 * @param callback The callback block to handle the result (optional).
 */
- (void)pauseImage:(id)view callback:(LynxUIMethodCallbackBlock _Nullable)callback;

/**
 * Stops the image playback and invokes the corresponding callback to return the result.
 *
 * @param view The view containing the image to be stopped.
 * @param callback The callback block to handle the result (optional).
 */
- (void)stopImage:(id)view callback:(LynxUIMethodCallbackBlock _Nullable)callback;

/**
 * Resumes the image playback and invokes the corresponding callback to return the result.
 *
 * @param view The view containing the image to be resumed.
 * @param callback The callback block to handle the result (optional).
 */
- (void)resumeImage:(id)view callback:(LynxUIMethodCallbackBlock _Nullable)callback;

/**
 * Restarts the image playback if possible and invokes the corresponding callback to return the
 * result.
 *
 * @param view The view containing the image to be restarted.
 * @param callback The callback block to handle the result (optional).
 * @return YES if the image was successfully restarted, NO otherwise.
 */

- (BOOL)restartImageIfPossible:(id)view callback:(LynxUIMethodCallbackBlock _Nullable)callback;

/**
 * Sets whether the animated image should automatically play after loading is complete.
 *
 * @param view The view containing the animated image.
 * @param autoPlay A boolean value indicating whether to enable auto-play.
 */

- (void)setAutoPlay:(id)view value:(BOOL)autoPlay;

/**
 * Adds callbacks for animated image events, including when the animation starts, completes a single
 * loop, and finishes all configured loops.
 *
 * @param view The view containing the animated image.
 * @param ui The LynxUIImage instance associated with the image.
 */
- (void)addAnimatedImageCallBack:(id)view UI:(LynxUIImage *)ui;

/**
 * Determines whether the given image is an animated graphic.
 *
 * @param image The UIImage to check.
 * @return YES if the image is animated, NO otherwise.
 */
- (BOOL)isAnimatedImage:(UIImage *)image;

/**
 * Checks the type of the provided view.
 *
 * @param view The view to check.
 * @return YES if the view is a CustomImageView, NO if it is a standard UIImageView or the
 * LynxService is not initialized.
 */
- (BOOL)checkImageType:(id)view;

/**
 * Loads a new image from the specified URL with additional options.
 *
 * @param url The LynxURL of the image to load.
 * @param targetSize The desired size of the ui.
 * @param enableGenericFetcher A flag indicating whether to enable the generic fetcher.
 * @param contextInfo Additional context information.
 * @param processors An array of image processors to apply.
 * @param completed The completion block to execute when the image load is finished.
 * @param lynxUIImage The LynxUIImage instance .
 * @return A block that can be used to cancel the operation.
 */
- (dispatch_block_t)loadNewImageFromURL:(LynxURL *)url
                                   size:(CGSize)targetSize
                   enableGenericFetcher:(BOOL)enableGenericFetcher
                            contextInfo:(NSDictionary *)contextInfo
                             processors:(NSArray *)processors
                              completed:(LynxImageLoadCompletionBlock)completed
                            LynxUIImage:(LynxUIImage *)lynxUIImage;

/**
 * Appends extra details about the image load event, including any additional information
 * for CustomImage instances.
 *
 * @param image The image for which details are appended.
 * @param detail The original detail dictionary to append information to.
 */
- (void)appendExtraImageLoadDetailForEvent:(UIImage *)image
                            originalDetail:(NSMutableDictionary *)detail;

/**
 * Handles image loading information; implementation is optional.
 *
 * @param LynxView The LynxView instance associated with the image.
 * @param data A dictionary containing image data.
 * @param extra Additional optional information.
 */
- (void)reportResourceStatus:(LynxView *)LynxView
                        data:(NSMutableDictionary *)data
                       extra:(NSDictionary *__nullable)extra;

/**
 * Maps the error codes from the image library to the internal Lynx image error codes.
 *
 * @param errorCode The error code from the image library.
 * @return The corresponding Lynx image error code.
 */
- (NSNumber *)getMappedCategorizedPicErrorCode:(NSNumber *)errorCode;

/**
 * Prefetches an image from a URI for faster subsequent loading.
 *
 * @param url The URI of the image to prefetch.
 * @param params Additional parameters for prefetching (optional).
 */
- (void)prefetchImage:(LynxURL *)url params:(nullable NSDictionary *)params;

/**
 * Decodes the image data into a UIImage.
 *
 * @param data The image data to decode.
 * @return The decoded UIImage.
 */
@optional
- (UIImage *)decodeImage:(NSData *)data;

/**
 * Sets the loop count of the animated image and assigns the image to the specified imageView.
 *
 * @param image The animated image to handle.
 * @param imageView The UIImageView to display the image.
 * @param loopCount The number of loops for the animation.
 */
- (void)handleAnimatedImage:(UIImage *)image
                       view:(UIImageView *)imageView
                  loopCount:(NSInteger)loopCount;

@end

NS_ASSUME_NONNULL_END
#endif  // TARGET_OS_IOS
#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEIMAGEPROTOCOL_H_
