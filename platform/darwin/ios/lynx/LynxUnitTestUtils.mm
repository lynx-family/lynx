// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUnitTestUtils.h"

#import <objc/runtime.h>

#include "core/renderer/tasm/config.h"

@implementation NSBundle (MainTestBundle)

+ (void)swizzle {
  SEL originalSelector = @selector(mainBundle);
  SEL swizzledSelector = @selector(testViewBundle);
  Method originalMethod = class_getClassMethod(self, originalSelector);
  Method swizzledMethod = class_getClassMethod(self, swizzledSelector);
  method_exchangeImplementations(originalMethod, swizzledMethod);
}

+ (NSBundle*)testViewBundle {
  return [NSBundle bundleForClass:[LynxUnitTest class]];
}

@end

@implementation LynxUnitTest

+ (void)setUp {
  // to load core.js
  [NSBundle swizzle];
}

- (void)setUp {
}

+ (NSData*)getNSData:(NSString*)fileName {
  NSBundle* bundle = [NSBundle bundleForClass:[self class]];
  NSString* str = [NSString stringWithFormat:@"TestResource.bundle/%@", fileName];
  NSString* url = [bundle pathForResource:[str stringByDeletingPathExtension] ofType:@"js"];
  return [NSData dataWithContentsOfFile:url];
}

@end
