// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxThreadSafeDictionary.h>
#import <Lynx/LynxUI.h>
#import <XElement/LynxSVGView.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxUISVG : LynxUI <LynxSVGView *>

@property(nonatomic, copy) NSString *src;
@property(nonatomic, copy) NSString *content;
@property(atomic, strong) LynxThreadSafeDictionary *imageHolder;

- (void)updateLayoutIfNeed;
- (UIImage *)processSVGData:(NSData *)data withSize:(CGSize)devSize;
- (UIImage *)loadImageFromHref:(NSString *)href withSize:(CGSize)devSize;
- (void)fetchSVGResource:(NSString *)src
                complete:(LynxGenericResourceCompletionBlock _Nonnull)callback;
/// devSize must be computed by the caller on the main thread in device pixels.
/// Do not read UIView size or screen scale inside implementations because this
/// method may run on the async SVG rendering queue.
- (void)fetchSVGImage:(NSString *_Nonnull)url
             withSize:(CGSize)devSize
             complete:(LynxMediaResourceCompletionBlock _Nonnull)callback;

@end
NS_ASSUME_NONNULL_END
