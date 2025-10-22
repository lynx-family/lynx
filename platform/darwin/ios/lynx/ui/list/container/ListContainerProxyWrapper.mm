// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/ListContainerProxyWrapper.h>

@interface ListContainerProxyWrapper () {
  lynx::shell::ListContainerProxy *listContainerProxy_;  // 裸指针成员变量
}
@end

@implementation ListContainerProxyWrapper

- (instancetype)initWithListEngineProxy:
    (lynx::shell::ListEngineProxy* )listEngineProxy {
  self = [super init];
  if (self) {
    
    listContainerProxy_ = new lynx::shell::ListContainerProxy(listEngineProxy);
  }
  return self;
}

- (lynx::shell::ListContainerProxy *)getListContainerProxy {
  return listContainerProxy_;
}

@end
