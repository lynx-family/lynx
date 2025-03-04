// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxDevtool.h"
#import "LynxTemplateData.h"
#import "LynxView.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxDevtool ()

- (void)onTemplateLoadSuccess:(nullable NSData *)tem;

- (void)onGlobalPropsUpdated:(LynxTemplateData *)props;

@end

NS_ASSUME_NONNULL_END
