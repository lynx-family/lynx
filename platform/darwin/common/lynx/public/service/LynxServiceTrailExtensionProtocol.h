// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICETRAILEXTENSIONPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICETRAILEXTENSIONPROTOCOL_H_

#import <Foundation/Foundation.h>
#import <LynxServiceAPI/LynxServiceTrailProtocol.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxViewBuilder;

/**
 * This protocol is used to extend the default LynxServiceTrailProtocol, which does not have some
 * lynx-specific methods.
 */
@protocol LynxServiceTrailExtensionProtocol

/**
 * parse configs of LynxViewBuilder
 */
- (void)parseLynxViewBuilder:(LynxViewBuilder *)builder;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICETRAILEXTENSIONPROTOCOL_H_
