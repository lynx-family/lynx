// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContext.h>
#import <Lynx/LynxUIOwner.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxAccessibilityModule.h"
@interface LynxContext (Testing)
- (LynxUIOwner *)uiOwner;

@end

@interface LynxAccessibilityModule (Testing)
- (void)registerMutationStyle:(NSDictionary *)prefetchData callback:(LynxCallbackBlock)callback;
@end

@interface LynxAccessibilityModuleUnitTest : XCTestCase
@property(nonatomic, strong) LynxAccessibilityModule *module;
@property(nonatomic, strong) LynxUIOwner *uiOwner;

@end

@implementation LynxAccessibilityModuleUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
  _uiOwner = [[LynxUIOwner alloc] initWithContainerView:nil
                                         templateRender:nil
                                      componentRegistry:nil
                                          screenMetrics:nil];
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testInvoke {
  LynxContext *context = OCMClassMock([LynxContext class]);
  _module = [[LynxAccessibilityModule alloc] initWithLynxContext:context];

  [_module registerMutationStyle:@{}
                        callback:^(NSDictionary *_Nonnull result) {
                          XCTAssert([result[@"msg"]
                              isEqualToString:@"Init accessibility env error: uiOwner is null"]);
                        }];

  OCMStub([context uiOwner]).andReturn(_uiOwner);
  [_module registerMutationStyle:@{}
                        callback:^(NSDictionary *_Nonnull result) {
                          XCTAssert([result[@"msg"]
                              isEqualToString:@"Params error with mutation_styles"]);
                        }];

  [_module registerMutationStyle:@{@"mutation_styles" : @{}}
                        callback:^(NSDictionary *_Nonnull result) {
                          XCTAssert([result[@"msg"]
                              isEqualToString:@"Params error with mutation_styles"]);
                        }];

  [_module
      registerMutationStyle:@{@"mutation_styles" : @[ @1 ]}
                   callback:^(NSDictionary *_Nonnull result) {
                     XCTAssert([result[@"msg"]
                         isEqualToString:
                             @"Params error with mutation_styles: params must be string values"]);
                   }];

  [_module registerMutationStyle:@{@"mutation_styles" : @[ @"background" ]}
                        callback:^(NSDictionary *_Nonnull result) {
                          XCTAssert([result[@"msg"] isEqualToString:@"Success"]);
                        }];
}

@end
