// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxCustomMeasureDelegate.h>
#import <Lynx/LynxFontFaceManager.h>
#import <Lynx/LynxShadowNode.h>
#import <Lynx/LynxUIOwner.h>
#import <ServalMarkdown/IMarkdownResourceDelegate.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxMarkdownResourceLoaderHost <NSObject>

- (BOOL)markdownHostDestroyed;
- (NSArray<LynxShadowNode *> *)markdownHostChildren;
- (nullable LynxUIOwner *)markdownHostUIOwner;
- (nullable MeasureContext *)markdownHostMeasureContext;
- (nullable AlignContext *)markdownHostAlignContext;
- (void)onImageLoaded:(NSString *)url;
- (void)onFontLoaded:(NSString *)family Weight:(int)weight Style:(int)style;
@end

@interface LynxMarkdownResourceLoader : NSObject <IMarkdownResourceDelegate>

@property(nonatomic, assign) BOOL enableImageDownSampling;

- (instancetype)initWithHost:(id<LynxMarkdownResourceLoaderHost>)host;
- (void)releaseResources;

@end

NS_ASSUME_NONNULL_END
