// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Foundation/Foundation.h>
#include "core/shell/list_container_proxy.h"
#include "core/shell/list_engine_proxy.h"

@interface ListContainerProxyWrapper : NSObject

- (instancetype)initWithListEngineProxy:
    (lynx::shell::ListEngineProxy *)listEngineProxy;
- (lynx::shell::ListContainerProxy *)getListContainerProxy;

@end
