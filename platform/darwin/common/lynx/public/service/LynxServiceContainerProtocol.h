// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxView.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxServiceContainerProtocol <NSObject>

- (__kindof LynxView *)getLynxView;

@end

NS_ASSUME_NONNULL_END
