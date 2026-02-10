// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxURL.h>

typedef void (^LynxImageLoadCompletionBlock)(UIImage *_Nullable image, NSError *_Nullable error,
                                             NSURL *_Nullable imageURL);

/**
 * Options for loading images.
 */
@interface LynxImageLoadOptions : NSObject

/**
 * The URL of the image to be loaded.
 */
@property(nonatomic, assign, nullable) LynxURL *imageURL;

/**
 * The font size to be used for loading the image.
 */
@property(nonatomic) CGFloat fontSize;

/**
 * The target size for the loaded image.
 */
@property(nonatomic, assign) CGSize targetSize;

/**
 * Image processors to be applied to the loaded image.
 */
@property(nonatomic, strong, nullable) NSArray *processors;

/**
 * The completion block to be invoked when the image load operation completes.
 */
@property(nonatomic, copy, nullable) LynxImageLoadCompletionBlock completed;

/**
 * The context associated with the image load operation.
 */
@property(nonatomic, weak, nullable) LynxUIContext *context;

/**
 * Additional context information for the image load operation.
 */
@property(nonatomic, strong, nullable) NSDictionary *contextInfo;

@end
