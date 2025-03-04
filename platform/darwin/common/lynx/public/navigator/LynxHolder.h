// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_NAVIGATOR_LYNXHOLDER_H_
#define DARWIN_COMMON_LYNX_NAVIGATOR_LYNXHOLDER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxView;
@class LynxRoute;

@protocol LynxHolder <NSObject>

- (LynxView *)createLynxView:(LynxRoute *)route;
- (void)showLynxView:(nonnull LynxView *)lynxView name:(nonnull NSString *)name;
- (void)hideLynxView:(nonnull LynxView *)lynxView;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_NAVIGATOR_LYNXHOLDER_H_
