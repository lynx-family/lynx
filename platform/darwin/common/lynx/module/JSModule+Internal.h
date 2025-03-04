// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_MODULE_JSMODULE_INTERNAL_H_
#define DARWIN_COMMON_LYNX_MODULE_JSMODULE_INTERNAL_H_

#import "JSModule.h"
#import "core/shell/ios/js_proxy_darwin.h"

@interface JSModule ()

- (void)setJSProxy:(const std::shared_ptr<lynx::shell::JSProxyDarwin>&)proxy;

@end

#endif  // DARWIN_COMMON_LYNX_MODULE_JSMODULE_INTERNAL_H_
