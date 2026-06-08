// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <LynxDevtool/GlobalDevToolPlatformDarwinDelegate.h>

#import <Lynx/LynxPageReloadHelper+Internal.h>
#import <mach/mach.h>
#import <sys/utsname.h>

#import <Lynx/LynxTraceController.h>
#import <LynxDevtool/LynxDeviceInfoHelper.h>
#import <LynxDevtool/LynxFPSTrace.h>
#import <LynxDevtool/LynxFrameViewTrace.h>
#import <LynxDevtool/LynxInstanceTrace.h>
#import <LynxDevtool/LynxMemoryController.h>

#include <utility>

#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"

#pragma mark - GlobalDevToolPlatformDarwin
namespace lynx {
namespace devtool {

constexpr char kDarwinMemoryUsageBridgeUnavailable[] =
    "Memory.getAllMemoryUsage is not available in this Darwin Lynx runtime. "
    "Upgrade the Lynx runtime and DevTool platform integration to a version "
    "with the global memory bridge.";

class GlobalDevToolPlatformDarwin : public GlobalDevToolPlatformFacade {
 public:
  void StartMemoryTracing() override { [GlobalDevToolPlatformDarwinDelegate startMemoryTracing]; }

  void StopMemoryTracing() override { [GlobalDevToolPlatformDarwinDelegate stopMemoryTracing]; }

  void GetAllMemoryUsage(int64_t timeout_ms, MemoryUsageCallback callback) override {
    (void)timeout_ms;
    // This CDP method needs the Darwin-side memory bridge that lands in the
    // platform integration. Reaching this fallback means the current runtime
    // only contains the global CDP plumbing, so clients should upgrade the Lynx
    // runtime and DevTool platform integration to a version with that bridge.
    if (callback) {
      std::move(callback)("{}", kDarwinMemoryUsageBridgeUnavailable);
    }
  }

  lynx::trace::TraceController* GetTraceController() override {
    intptr_t res = [GlobalDevToolPlatformDarwinDelegate getTraceController];
    if (res) {
      return reinterpret_cast<lynx::trace::TraceController*>(res);
    } else {
      return nullptr;
    }
  }

  lynx::trace::TracePlugin* GetFPSTracePlugin() override {
    intptr_t res = [GlobalDevToolPlatformDarwinDelegate getFPSTracePlugin];
    if (res) {
      return reinterpret_cast<lynx::trace::TracePlugin*>(res);
    } else {
      return nullptr;
    }
  }

  lynx::trace::TracePlugin* GetFrameViewTracePlugin() override {
    intptr_t res = [GlobalDevToolPlatformDarwinDelegate getFrameViewTracePlugin];
    if (res) {
      return reinterpret_cast<lynx::trace::TracePlugin*>(res);
    } else {
      return nullptr;
    }
  }

  lynx::trace::TracePlugin* GetInstanceTracePlugin() override {
    intptr_t res = [GlobalDevToolPlatformDarwinDelegate getInstanceTracePlugin];
    if (res) {
      return reinterpret_cast<lynx::trace::TracePlugin*>(res);
    } else {
      return nullptr;
    }
  }

  std::string GetLynxVersion() override {
    return [[LynxDeviceInfoHelper getLynxVersion] UTF8String];
  }

  std::string GetSystemModelName() override {
    return [GlobalDevToolPlatformDarwinDelegate getSystemModelName];
  }
};

GlobalDevToolPlatformFacade& GlobalDevToolPlatformFacade::GetInstance() {
  static GlobalDevToolPlatformDarwin instance;
  return instance;
}
}  // namespace devtool
}  // namespace lynx

@implementation GlobalDevToolPlatformDarwinDelegate {
}

+ (void)startMemoryTracing {
  [[LynxMemoryController shareInstance] startMemoryTracing];
}

+ (void)stopMemoryTracing {
  [[LynxMemoryController shareInstance] stopMemoryTracing];
}

+ (intptr_t)getTraceController {
  return [[LynxTraceController sharedInstance] getTraceController];
}

+ (intptr_t)getFPSTracePlugin {
  return [[LynxFPSTrace shareInstance] getFPSTracePlugin];
}

+ (intptr_t)getFrameViewTracePlugin {
  return [[LynxFrameViewTrace shareInstance] getFrameViewTracePlugin];
}

+ (intptr_t)getInstanceTracePlugin {
  return [[LynxInstanceTrace shareInstance] getInstanceTracePlugin];
}

+ (std::string)getSystemModelName {
  struct utsname systemInfo;
  uname(&systemInfo);
  NSString* deviceModel = [NSString stringWithCString:systemInfo.machine
                                             encoding:NSUTF8StringEncoding];

  return [deviceModel UTF8String];
}

@end
