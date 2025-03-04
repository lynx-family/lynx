// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#import <objc/runtime.h>

#include <memory>

#import "LynxErrorBehavior.h"
#import "LynxTemplateRender+Protected.h"
#import "LynxTemplateRender.h"
#import "LynxUnitTestUtils.h"
#import "LynxView+Internal.h"
#import "LynxView.h"

#include "core/shell/lynx_shell.h"
#include "core/shell/runtime_mediator.h"
#include "core/shell/runtime_standalone_helper.h"

@interface MockLynxViewClient : NSObject <LynxViewLifecycle, LynxBackgroundRuntimeLifecycle>

- (instancetype)init:(XCTestExpectation*)expectation;

@end

@implementation MockLynxViewClient {
  XCTestExpectation* _expectation;
}

- (instancetype)init:(XCTestExpectation*)expectation {
  if (self = [super init]) {
    _expectation = expectation;
  }
  return self;
}

- (void)lynxView:(LynxView*)view didRecieveError:(NSError*)error {
  [self checkLynxError:error];
}

- (void)runtime:(LynxBackgroundRuntime*)runtime didRecieveError:(NSError*)error {
  [self checkLynxError:error];
}

- (void)checkLynxError:(NSError*)error {
  if ([error code] == EBLynxResourceExternalResource) {
    [_expectation fulfill];
  }
}

@end

@interface LynxTemplateRender (UnitTest)

- (lynx::shell::LynxShell*)getShell;

@end

@implementation LynxTemplateRender (UnitTest)

- (lynx::shell::LynxShell*)getShell {
  return shell_.get();
}

@end

@interface LynxResourceLoaderDarwinUnitTest : LynxUnitTest
@property XCTestExpectation* onErrorExpectation;
@end

@implementation LynxResourceLoaderDarwinUnitTest

- (void)testReportErrorToLynxView {
  // create a LynxView and LynxViewClient
  _onErrorExpectation = [self expectationWithDescription:@"onReceiveError"];
  LynxView* lynxView = [[LynxView alloc] init];
  MockLynxViewClient* client = [[MockLynxViewClient alloc] init:_onErrorExpectation];
  [lynxView addLifecycleClient:client];

  // invoke LynxResourceLoader.reportError
  lynx::shell::LynxShell* shell = [[lynxView templateRender] getShell];
  [self invokeReportError:shell->runtime_actor_];

  // wait to receive error
  [self await];
}

- (void)testReportErrorToBackground {
  // create a BackgroundRuntime and BackgroundRuntimeClient
  _onErrorExpectation = [self expectationWithDescription:@"onReceiveError"];
  LynxBackgroundRuntime* backgroundRuntime =
      [[LynxBackgroundRuntime alloc] initWithOptions:[[LynxBackgroundRuntimeOptions alloc] init]];
  MockLynxViewClient* client = [[MockLynxViewClient alloc] init:_onErrorExpectation];
  [backgroundRuntime addLifecycleClient:client];

  // invoke LynxResourceLoader.reportError
  Ivar privateVar =
      class_getInstanceVariable([LynxBackgroundRuntime class], "_runtime_standalone_bundle");
  auto* bundle = (__bridge lynx::shell::InitRuntimeStandaloneResult*)object_getIvar(
      backgroundRuntime, privateVar);
  [self invokeReportError:bundle->runtime_actor_];

  // wait to receive error
  [self await];
}

- (void)await {
  [self waitForExpectationsWithTimeout:5
                               handler:^(NSError* error) {
                                 if (error) {
                                   NSLog(@"Timeout Error: %@", error);
                                 }
                               }];
}

- (void)invokeReportError:
    (const std::shared_ptr<lynx::shell::LynxActor<lynx::runtime::LynxRuntime>>&)runtime {
  lynx::runtime::TemplateDelegate* delegate = runtime->impl_->delegate_.get();
  lynx::shell::RuntimeMediator* runtime_mediator =
      static_cast<lynx::shell::RuntimeMediator*>(delegate);
  runtime_mediator->external_resource_loader_->LoadJSSource("test");
}

@end
