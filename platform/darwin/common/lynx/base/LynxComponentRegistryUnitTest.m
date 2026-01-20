// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <XCTest/XCTest.h>

@interface LynxComponentRegistryUnitTest : XCTestCase
@end

@implementation LynxComponentRegistryUnitTest

- (void)setUp {
}

- (void)tearDown {
}

#ifdef OS_IOS
- (void)testRegisterBuiltInBehaviors {
  [LynxComponentScopeRegistry tryRegisterBuiltInClasses];
  LynxComponentScopeRegistry* registry = [LynxComponentScopeRegistry new];
  [LynxComponentScopeRegistry registerBuiltInBehaviors:registry];

  BOOL supported = YES;
  XCTAssertTrue([registry uiClassWithName:@"image" accessible:&supported] != nil);
  XCTAssertTrue([registry shadowNodeClassWithName:@"image" accessible:&supported] != nil);
}
#endif

@end
