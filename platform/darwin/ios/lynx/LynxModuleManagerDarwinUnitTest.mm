// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public

#import <XCTest/XCTWaiter.h>
#import <XCTest/XCTest.h>
#import <objc/runtime.h>

#import "LynxBackgroundRuntime+Internal.h"
#import "LynxEnv.h"
#import "LynxUnitTestUtils.h"
#import "LynxView.h"

#include "core/runtime/bindings/jsi/modules/ios/lynx_module_darwin.h"

@interface LynxModuleMockGlobal : NSObject <LynxModule>
@end

@interface LynxModuleMockInstance : NSObject <LynxModule>
@end

@implementation LynxModuleMockGlobal

+ (NSString *)name {
  return @"LynxModuleMockGlobal";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{};
}

- (instancetype)initWithParam:(id)param {
  self = [super init];
  if (self) {
  }
  return self;
}
@end

@implementation LynxModuleMockInstance

+ (NSString *)name {
  return @"LynxModuleMockInstance";
}

- (instancetype)initWithParam:(id)param {
  self = [super init];
  if (self) {
  }
  return self;
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{};
}
@end

class MockDelegate : public lynx::piper::ModuleDelegate {
 public:
  int64_t RegisterJSCallbackFunction(lynx::piper::Function func) override { return 1; }
  void CallJSCallback(const std::shared_ptr<lynx::piper::ModuleCallback> &callback,
                      int64_t id_to_delete) override {}
  void OnErrorOccurred(lynx::base::LynxError error) override {}
  void OnMethodInvoked(const std::string &module_name, const std::string &method_name,
                       int32_t code) override {}
  void FlushJSBTiming(lynx::piper::NativeModuleInfo timing) override {}
  void RunOnJSThread(lynx::base::closure func) override {}
  void RunOnPlatformThread(lynx::base::closure func) override {}
};

class ModuleFactoryDarwinTester : public lynx::piper::ModuleFactoryDarwin {};
@interface LynxModuleManagerDarwinUnitTest : LynxUnitTest
@end

@implementation LynxModuleManagerDarwinUnitTest {
  std::shared_ptr<MockDelegate> _mockDelegate;
}
- (void)setUp {
  _mockDelegate = std::make_shared<MockDelegate>();
}
- (void)testModuleManager {
  LynxBackgroundRuntimeOptions *options = [[LynxBackgroundRuntimeOptions alloc] init];
  [options registerModule:LynxModuleMockInstance.class param:[[NSObject new] init]];
  LynxBackgroundRuntime *runtime = [[LynxBackgroundRuntime alloc] initWithOptions:options];
  auto manager = [runtime moduleManagerPtr].lock();
  XCTAssertNotEqual(manager, nullptr);
  auto module_host_object = manager->GetModule("LynxModuleMockInstance", _mockDelegate);
  XCTAssertNotEqual(module_host_object, nullptr);

  // TODO(huzhanbo.luc) Test ModuleManager overwriting behavior here later
}

@end
