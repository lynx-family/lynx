// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public

#import <XCTest/XCTWaiter.h>
#import <XCTest/XCTest.h>
#import <objc/runtime.h>

#import <Lynx/LynxEnv.h>
#import <Lynx/LynxView.h>
#import "LynxBackgroundRuntime+Internal.h"
#import "LynxUnitTestUtils.h"

#include "core/runtime/js/bindings/modules/ios/lynx_module_darwin.h"

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

class MockDelegate : public lynx::runtime::js::ModuleDelegate {
 public:
  int64_t RegisterJSCallbackFunction(lynx::runtime::js::Function func) override { return 1; }
  void CallJSCallback(const std::shared_ptr<lynx::runtime::js::ModuleCallback> &callback,
                      lynx::base::MoveOnlyClosure<bool> invoke_pre_func,
                      int64_t id_to_delete) override {}
  void OnErrorOccurred(lynx::base::LynxError error) override {}
  void OnMethodInvoked(const std::string &module_name, const std::string &method_name,
                       int32_t code) override {}
  void FlushJSBTiming(lynx::runtime::js::NativeModuleInfo timing) override {}
  void RunOnJSThread(lynx::base::closure func) override {}
  void RunOnPlatformThread(lynx::base::closure func) override {}
};

class ModuleFactoryDarwinTester : public lynx::runtime::js::ModuleFactoryDarwin {};
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
  auto factory = [runtime moduleFactoryPtr].lock();
  XCTAssertNotEqual(factory, nullptr);
  std::shared_ptr<lynx::pub::LynxNativeModuleManager> native_module_manager =
      std::make_shared<lynx::pub::LynxNativeModuleManager>();
  native_module_manager->SetPlatformModuleFactory(factory);
  native_module_manager->SetModuleDelegate(_mockDelegate);
  auto platform_module = native_module_manager->GetModule("LynxModuleMockInstance");
  XCTAssertNotEqual(platform_module, nullptr);

  // TODO(huzhanbo.luc) Test ModuleManager overwriting behavior here later
}

@end
