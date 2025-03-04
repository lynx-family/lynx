// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "GlobalDevToolPlatformDarwinDelegate.h"

#import <Lynx/LynxPageReloadHelper+Internal.h>
#import <mach/mach.h>
#import <sys/utsname.h>

#import "LynxDeviceInfoHelper.h"
#import "LynxFPSTrace.h"
#import "LynxFrameViewTrace.h"
#import "LynxInstanceTrace.h"
#import "LynxMemoryController.h"
#import "LynxTraceController.h"

#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"

#pragma mark - GlobalDevToolPlatformDarwin
namespace lynx {
namespace devtool {
class GlobalDevToolPlatformDarwin : public GlobalDevToolPlatformFacade {
 public:
  void StartMemoryTracing() override { [GlobalDevToolPlatformDarwinDelegate startMemoryTracing]; }

  void StopMemoryTracing() override { [GlobalDevToolPlatformDarwinDelegate stopMemoryTracing]; }

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
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
#endif

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

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

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
#endif

+ (std::string)getSystemModelName {
  struct utsname systemInfo;
  uname(&systemInfo);
  NSString* deviceModel = [NSString stringWithCString:systemInfo.machine
                                             encoding:NSUTF8StringEncoding];

  return [deviceModel UTF8String];
}

@end
