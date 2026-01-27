// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxTemplateBundle.h>
#import <Lynx/LynxUpdateMeta.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxFrameView : UIView <LUIBodyView>

- (void)initWithRootView:(UIView<LUIBodyView> *)rootView;

- (void)setAppBundle:(LynxTemplateBundle *)bundle;

- (void)updateMetaData:(nullable LynxTemplateData *)initData
           globalProps:(nullable LynxTemplateData *)globalProps;

- (void)setUrl:(NSString *)url;

- (UIView<LUIBodyView> *_Nullable)getRootView;

@end

NS_ASSUME_NONNULL_END
