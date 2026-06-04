// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import <Lynx/LynxGenericResourceFetcher.h>
#import <Lynx/LynxMediaResourceFetcher.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxError;
@class SrSVG;
@class LynxThreadSafeDictionary;

typedef UIImage *_Nullable (^LynxSVGRenderBlock)(void);
typedef void (^LynxSVGRenderCompletion)(UIImage *_Nullable image);
typedef void (^LynxSVGRenderAsyncExecutor)(LynxSVGRenderBlock renderBlock,
                                           LynxSVGRenderCompletion completion);
typedef void (^LynxSVGRenderErrorHandler)(LynxError *error);

@interface LynxSVGRenderEngine : NSObject

- (instancetype)initWithSrSVG:(SrSVG *)srSvg
                        color:(nullable NSString *)color
               genericFetcher:(id<LynxGenericResourceFetcher>)genericFetcher
                 mediaFetcher:(id<LynxMediaResourceFetcher>)mediaFetcher
                asyncExecutor:(LynxSVGRenderAsyncExecutor)asyncExecutor
                  reloadBlock:(dispatch_block_t)reloadBlock;

@property(nonatomic, copy, nullable) NSString *src;
@property(nonatomic, copy, nullable) NSString *content;
@property(nonatomic, copy, nullable) NSString *color;

- (void)renderWithSize:(CGSize)devSize
            completion:(nullable LynxSVGRenderCompletion)completion
          errorHandler:(nullable LynxSVGRenderErrorHandler)errorHandler;

- (void)cancel;

// Internal methods exposed for subclass override support
- (nullable UIImage *)renderSVGData:(NSData *)data withSize:(CGSize)devSize;
- (nullable UIImage *)loadImageFromHref:(NSString *)href withSize:(CGSize)devSize;

@end

NS_ASSUME_NONNULL_END
