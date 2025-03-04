// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <objc/runtime.h>
#import "LynxView+Identify.h"

@implementation LynxView (Identify)

- (NSString *)containerID {
  NSString *containerID = objc_getAssociatedObject(self, @"containerID");
  if (!containerID) {
    containerID = [NSUUID UUID].UUIDString;
    objc_setAssociatedObject(self, @"containerID", containerID, OBJC_ASSOCIATION_COPY_NONATOMIC);
  }
  return containerID;
}

- (void)setContainerID:(NSString *)containerID {
  objc_setAssociatedObject(self, @"containerID", containerID, OBJC_ASSOCIATION_COPY_NONATOMIC);
}

- (NSString *)namescope {
  return objc_getAssociatedObject(self, _cmd);
}

- (void)setNamescope:(NSString *)namescope {
  objc_setAssociatedObject(self, @selector(namescope), namescope, OBJC_ASSOCIATION_COPY_NONATOMIC);
}

@end
