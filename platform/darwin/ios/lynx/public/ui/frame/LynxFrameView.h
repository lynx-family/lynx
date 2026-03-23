// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxTemplateBundle.h>
#import <Lynx/LynxUpdateMeta.h>
#import <Lynx/LynxViewEnum.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxUIContext;

@interface LynxFrameView : UIView <LUIBodyView>

@property(nonatomic, assign) NSInteger sign;
@property(nonatomic, weak) LynxUIContext *context;

- (void)initWithRootView:(UIView<LUIBodyView> *)rootView;

- (BOOL)setAppBundle:(LynxTemplateBundle *)bundle;

- (void)setInitData:(nullable LynxTemplateData *)initData;

- (void)setGlobalProps:(nullable LynxTemplateData *)globalProps;

- (void)propsDidUpdate;

- (void)setUrl:(NSString *)url;

- (void)setAutoWidth:(BOOL)autoWidth;

- (void)setAutoHeight:(BOOL)autoHeight;

- (UIView<LUIBodyView> *_Nullable)getRootView;

- (void)updateFrame:(CGRect)frame contentFrame:(CGRect)contentFrame;

- (void)setEmbeddedMode:(LynxEmbeddedMode)embeddedMode;

@end

NS_ASSUME_NONNULL_END
