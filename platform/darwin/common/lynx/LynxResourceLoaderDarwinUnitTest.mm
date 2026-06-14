// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#import <objc/runtime.h>

#include <memory>

#import <Lynx/LynxBackgroundRuntime+Internal.h>
#import <Lynx/LynxErrorBehavior.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxView.h>
#import "LynxTemplateRender+Protected.h"
#import "LynxUnitTestUtils.h"

#include "core/resource/lynx_resource_loader_darwin.h"
#include "core/shell/lynx_shell.h"
#include "core/shell/runtime/bts/bts_runtime_mediator.h"
#include "core/shell/runtime/bts/bts_runtime_standalone_helper.h"

@interface Mock2GenericResourceFetcher : NSObject <LynxGenericResourceFetcher>

@end

@implementation Mock2GenericResourceFetcher

- (dispatch_block_t)fetchResource:(LynxResourceRequest*)request
                       onComplete:(LynxGenericResourceCompletionBlock)callback {
  return nil;
}

- (dispatch_block_t)fetchResourcePath:(LynxResourceRequest*)request
                           onComplete:(LynxGenericResourcePathCompletionBlock)callback {
  return nil;
}

@end

@interface MockBytecodeErrorFetcher : Mock2GenericResourceFetcher

@end

@implementation MockBytecodeErrorFetcher

- (dispatch_block_t)fetchBytecode:(LynxResourceRequest*)request
                       onComplete:(LynxGenericResourceCompletionBlock)callback {
  NSError* error = [NSError errorWithDomain:@"LynxResourceLoaderDarwinUnitTest"
                                       code:-1
                                   userInfo:nil];
  callback(nil, error);
  return nil;
}

@end

@interface MockDynamicComponentFetcher : NSObject <LynxDynamicComponentFetcher>

@property(nonatomic, assign) BOOL fetchedOnMainThread;
@property(nonatomic, assign) BOOL waitForContinueSignal;
@property(nonatomic, strong) dispatch_semaphore_t fetchStartedSemaphore;
@property(nonatomic, strong) dispatch_semaphore_t continueSemaphore;

@end

@implementation MockDynamicComponentFetcher

- (instancetype)init {
  if (self = [super init]) {
    _fetchStartedSemaphore = dispatch_semaphore_create(0);
    _continueSemaphore = dispatch_semaphore_create(0);
  }
  return self;
}

- (void)loadDynamicComponent:(NSString*)url withLoadedBlock:(onComponentLoaded)block {
  self.fetchedOnMainThread = [NSThread isMainThread];
  dispatch_semaphore_signal(self.fetchStartedSemaphore);
  if (self.waitForContinueSignal) {
    dispatch_semaphore_wait(self.continueSemaphore, DISPATCH_TIME_FOREVER);
  }
  NSData* data = [@"unit_test_bundle" dataUsingEncoding:NSUTF8StringEncoding];
  block(data, nil);
}

@end

@interface MockLynxErrorReceiver : NSObject <LynxErrorReceiverProtocol>

- (instancetype)initWithExpectation:(XCTestExpectation*)expectation;

@end

@implementation MockLynxErrorReceiver {
  XCTestExpectation* _expectation;
}

- (instancetype)initWithExpectation:(XCTestExpectation*)expectation {
  if (self = [super init]) {
    _expectation = expectation;
  }
  return self;
}

- (void)onErrorOccurred:(LynxError*)error {
  [_expectation fulfill];
}

@end

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

- (void)runtime:(LynxBackgroundRuntime*)runtime didReceiveError:(NSError*)error {
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

- (void)testLoadBytecode {
  Mock2GenericResourceFetcher* genericFetcher = [[Mock2GenericResourceFetcher alloc] init];
  auto loader =
      std::make_shared<lynx::shell::LynxResourceLoaderDarwin>(nil, nil, nil, nil, genericFetcher);
  auto request = lynx::pub::LynxResourceRequest{"bytecode_url",
                                                lynx::pub::LynxResourceType::kExternalByteCode};
  std::promise<std::string> promise;
  std::future<std::string> future = promise.get_future();
  loader->LoadBytecode(
      request, [promise = std::move(promise)](lynx::pub::LynxResourceResponse& response) mutable {
        promise.set_value(response.err_msg);
      });

  if (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
    XCTAssertTrue(false);
  }
  XCTAssertTrue(future.get().length() > 0);
}

- (void)testLoadBytecodeFailureDoesNotReportError {
  XCTestExpectation* onErrorExpectation =
      [self expectationWithDescription:@"loadBytecode should not report error"];
  onErrorExpectation.inverted = YES;
  XCTestExpectation* callbackExpectation =
      [self expectationWithDescription:@"loadBytecode callback"];
  MockBytecodeErrorFetcher* genericFetcher = [[MockBytecodeErrorFetcher alloc] init];
  MockLynxErrorReceiver* errorReceiver =
      [[MockLynxErrorReceiver alloc] initWithExpectation:onErrorExpectation];
  auto loader = std::make_shared<lynx::shell::LynxResourceLoaderDarwin>(nil, nil, errorReceiver,
                                                                        nil, genericFetcher);
  auto request = lynx::pub::LynxResourceRequest{"bytecode_url",
                                                lynx::pub::LynxResourceType::kExternalByteCode};
  loader->LoadBytecode(request, [callbackExpectation](lynx::pub::LynxResourceResponse& response) {
    XCTAssertFalse(response.Success());
    [callbackExpectation fulfill];
  });

  [self waitForExpectations:@[ callbackExpectation, onErrorExpectation ] timeout:1];
}

- (void)testLoadLazyBundleOffCurrentThreadWhenRequested {
  MockDynamicComponentFetcher* fetcher = [[MockDynamicComponentFetcher alloc] init];
  auto loader =
      std::make_shared<lynx::shell::LynxResourceLoaderDarwin>(nil, fetcher, nil, nil, nil);
  auto request = lynx::pub::LynxResourceRequest{"unit_test_url",
                                                lynx::pub::LynxResourceType::kLazyBundle, false};

  std::promise<std::tuple<bool, bool, size_t>> promise;
  std::future<std::tuple<bool, bool, size_t>> future = promise.get_future();
  loader->LoadResource(request, [fetcher, promise = std::move(promise)](
                                    lynx::pub::LynxResourceResponse& response) mutable {
    promise.set_value(std::make_tuple(fetcher.fetchedOnMainThread, [NSThread isMainThread],
                                      response.data.size()));
  });

  XCTAssertEqual(future.wait_for(std::chrono::seconds(2)), std::future_status::ready);
  auto [fetch_on_main_thread, callback_on_main_thread, response_size] = future.get();
  XCTAssertFalse(fetch_on_main_thread);
  XCTAssertFalse(callback_on_main_thread);
  XCTAssertTrue(response_size > 0);
}

- (void)testOffThreadLoadKeepsLoaderAliveUntilQueuedWorkCompletes {
  MockDynamicComponentFetcher* fetcher = [[MockDynamicComponentFetcher alloc] init];
  fetcher.waitForContinueSignal = YES;
  auto loader =
      std::make_shared<lynx::shell::LynxResourceLoaderDarwin>(nil, fetcher, nil, nil, nil);
  std::weak_ptr<lynx::shell::LynxResourceLoaderDarwin> weak_loader = loader;
  auto request = lynx::pub::LynxResourceRequest{"unit_test_url",
                                                lynx::pub::LynxResourceType::kLazyBundle, false};

  std::promise<size_t> promise;
  std::future<size_t> future = promise.get_future();
  loader->LoadResource(
      request, [promise = std::move(promise)](lynx::pub::LynxResourceResponse& response) mutable {
        promise.set_value(response.data.size());
      });

  XCTAssertEqual(dispatch_semaphore_wait(fetcher.fetchStartedSemaphore,
                                         dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC)),
                 0);

  loader.reset();
  XCTAssertFalse(weak_loader.expired());

  dispatch_semaphore_signal(fetcher.continueSemaphore);

  XCTAssertEqual(future.wait_for(std::chrono::seconds(2)), std::future_status::ready);
  XCTAssertTrue(future.get() > 0);
  XCTAssertTrue(weak_loader.expired());
}

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
  [self invokeReportError:[backgroundRuntime runtimeActor]];

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
    (const std::shared_ptr<lynx::shell::LynxActor<lynx::shell::BTSRuntime>>&)runtime {
  lynx::runtime::TemplateDelegate* delegate = runtime->impl_->delegate_.get();
  lynx::shell::BTSRuntimeMediator* runtime_mediator =
      static_cast<lynx::shell::BTSRuntimeMediator*>(delegate);
  runtime_mediator->external_resource_loader_->LoadJSSource("test");
}

@end
