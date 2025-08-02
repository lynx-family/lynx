// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <LynxDevtool/GlobalDevToolPlatformDarwinDelegate.h>

#import <Lynx/LynxPageReloadHelper+Internal.h>
#import <mach/mach.h>
#import <sys/utsname.h>

#import <Lynx/LynxFSPTracer+Internal.h>
#import <Lynx/LynxTraceController.h>
#import <LynxDevtool/LynxDeviceInfoHelper.h>
#import <LynxDevtool/LynxFPSTrace.h>
#import <LynxDevtool/LynxFrameViewTrace.h>
#import <LynxDevtool/LynxInstanceTrace.h>
#import <LynxDevtool/LynxMemoryController.h>
#import <LynxDevtool/LynxTimingOverlayHelper.h>

#include "core/services/fsp_tracing/fsp_config.h"
#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"
#include "third_party/jsoncpp/include/json/json.h"

#pragma mark - GlobalDevToolPlatformDarwin
namespace lynx {
namespace devtool {
class GlobalDevToolPlatformDarwin : public GlobalDevToolPlatformFacade {
 public:
  void StartMemoryTracing() override { [GlobalDevToolPlatformDarwinDelegate startMemoryTracing]; }

  void StopMemoryTracing() override { [GlobalDevToolPlatformDarwinDelegate stopMemoryTracing]; }

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

  void SetTimingOverlay(bool enabled, const Json::Value& params) override {
    [LynxTimingOverlayHelper setEnabled:enabled];

    if (!enabled) {
      [LynxFSPTracer setEnabled:NO config:NULL];
      return;
    }

    LynxFSPConfig config = kLynxFSPConfigDefault;
    if (params.isMember("fsp_interval")) {
      config.snapshotInterval = params["fsp_interval"].asDouble();
    }
    if (params.isMember("fsp_hard_timeout")) {
      config.hardTimeout = params["fsp_hard_timeout"].asDouble();
    }
    if (params.isMember("fsp_graceful_timeout")) {
      config.gracefulTimeout = params["fsp_graceful_timeout"].asDouble();
    }
    if (params.isMember("fsp_extended_graceful_timeout")) {
      config.extendedGracefulTimeout = params["fsp_extended_graceful_timeout"].asDouble();
    }
    if (params.isMember("fsp_mode")) {
      config.mode = (LynxFSPMode)params["fsp_mode"].asInt();
    }

    static lynx::tasm::timing::FSPConfig tracerCfg;
    if (config.mode == LynxFSPModeAxial) {
      lynx::tasm::timing::FSPAxialConfig cfg = {};

      if (params.isMember("fsp_acceptable_difference")) {
        cfg.acceptable_pixel_difference_per_snapshot =
            params["fsp_acceptable_difference"].asDouble();
      }
      if (params.isMember("fsp_acceptable_difference_per_ms")) {
        cfg.acceptable_pixel_difference_per_ms =
            params["fsp_acceptable_difference_per_ms"].asDouble();
      }
      if (params.isMember("fsp_min_completion_rate_1")) {
        cfg.minimum_completion_rate_x = params["fsp_min_completion_rate_1"].asDouble();
      }
      if (params.isMember("fsp_min_completion_rate_2")) {
        cfg.minimum_completion_rate_y = params["fsp_min_completion_rate_2"].asDouble();
      }
      tracerCfg = cfg;
    } else if (config.mode == LynxFSPModeAxial) {
      lynx::tasm::timing::FSPAreaConfig cfg = {};

      if (params.isMember("fsp_acceptable_difference")) {
        cfg.acceptable_difference_per_snapshot = params["fsp_acceptable_difference"].asDouble();
      }
      if (params.isMember("fsp_acceptable_difference_per_ms")) {
        cfg.acceptable_difference_per_ms = params["fsp_acceptable_difference_per_ms"].asDouble();
      }
      if (params.isMember("fsp_min_completion_rate_1")) {
        cfg.minimum_completion_rate = params["fsp_min_completion_rate_1"].asDouble();
      }

      tracerCfg = cfg;
    }

    config.tracerConfig = reinterpret_cast<intptr_t>(&tracerCfg);
    [LynxFSPTracer setEnabled:enabled config:&config];
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
