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
#include <memory>
#include <mutex>
#include <utility>
#import "LynxSettingPlatformDarwin.h"

#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"

#pragma mark - GlobalDevToolPlatformDarwin
namespace lynx {
namespace devtool {
namespace {

constexpr char kEmptyMemoryUsageResultJson[] = "{}";
constexpr char kDarwinMemoryUsageInvalidResult[] =
    "Memory.getAllMemoryUsage returned empty or invalid result JSON on Darwin";

bool IsJsonWhitespace(unichar value) {
  return value == ' ' || value == '\n' || value == '\r' || value == '\t';
}

bool LooksLikeJsonObjectString(NSString* _Nullable value) {
  if (!value) {
    return false;
  }
  NSUInteger start = 0;
  NSUInteger end = [value length];
  while (start < end && IsJsonWhitespace([value characterAtIndex:start])) {
    ++start;
  }
  while (end > start && IsJsonWhitespace([value characterAtIndex:end - 1])) {
    --end;
  }
  if (start == end) {
    return false;
  }
  return [value characterAtIndex:start] == '{' && [value characterAtIndex:end - 1] == '}';
}

struct MemoryUsageCallbackState {
  explicit MemoryUsageCallbackState(GlobalDevToolPlatformFacade::MemoryUsageCallback&& callback)
      : callback(std::move(callback)) {}

  std::mutex mutex;
  GlobalDevToolPlatformFacade::MemoryUsageCallback callback;
};

struct LynxSettingCallbackState {
  explicit LynxSettingCallbackState(GlobalDevToolPlatformFacade::LynxSettingCallback&& callback)
      : callback(std::move(callback)) {}

  std::mutex mutex;
  GlobalDevToolPlatformFacade::LynxSettingCallback callback;
};

}  // namespace

class GlobalDevToolPlatformDarwin : public GlobalDevToolPlatformFacade {
 public:
  void StartMemoryTracing() override { [GlobalDevToolPlatformDarwinDelegate startMemoryTracing]; }

  void StopMemoryTracing() override { [GlobalDevToolPlatformDarwinDelegate stopMemoryTracing]; }

  void GetAllMemoryUsage(int64_t timeout_ms, MemoryUsageCallback callback) override {
    if (!callback) {
      return;
    }
    auto callbackState = std::make_shared<MemoryUsageCallbackState>(std::move(callback));
    [GlobalDevToolPlatformDarwinDelegate
        queryAllMemoryUsageWithTimeoutMs:timeout_ms
                                callback:^(NSString* _Nullable resultJson,
                                           NSString* _Nullable errorMessage) {
                                  MemoryUsageCallback callbackToRun;
                                  {
                                    std::lock_guard<std::mutex> lock(callbackState->mutex);
                                    if (!callbackState->callback) {
                                      return;
                                    }
                                    callbackToRun = std::move(callbackState->callback);
                                  }
                                  const char* resultCStr =
                                      resultJson ? [resultJson UTF8String] : nullptr;
                                  const char* errorCStr =
                                      errorMessage ? [errorMessage UTF8String] : nullptr;
                                  const bool hasValidResult =
                                      resultCStr && LooksLikeJsonObjectString(resultJson);
                                  std::string result =
                                      hasValidResult ? resultCStr : kEmptyMemoryUsageResultJson;
                                  std::string error = errorCStr ? errorCStr : "";
                                  if (!hasValidResult && error.empty()) {
                                    error = kDarwinMemoryUsageInvalidResult;
                                  }
                                  std::move(callbackToRun)(result, error);
                                }];
  }

  void HandleLynxSetting(LynxSettingRequest request, LynxSettingCallback callback) override {
    if (!callback) {
      return;
    }
    NSString* method = [[NSString alloc] initWithBytes:request.method.data()
                                                length:request.method.size()
                                              encoding:NSUTF8StringEncoding];
    NSString* key = [[NSString alloc] initWithBytes:request.key.data()
                                             length:request.key.size()
                                           encoding:NSUTF8StringEncoding];
    NSString* value = [[NSString alloc] initWithBytes:request.value.data()
                                               length:request.value.size()
                                             encoding:NSUTF8StringEncoding];
    if (!method || !key || !value) {
      std::move(callback)("{}", "Invalid UTF-8 LynxSetting request");
      return;
    }
    auto callbackState = std::make_shared<LynxSettingCallbackState>(std::move(callback));
    [GlobalDevToolPlatformDarwinDelegate
        handleLynxSettingMethod:method
                            key:key
                          value:value
                       callback:^(NSString* resultJson, NSString* errorMessage) {
                         LynxSettingCallback callbackToRun;
                         {
                           std::lock_guard<std::mutex> lock(callbackState->mutex);
                           if (!callbackState->callback) {
                             return;
                           }
                           callbackToRun = std::move(callbackState->callback);
                         }
                         const char* result = resultJson.UTF8String;
                         const char* error = errorMessage.UTF8String;
                         std::move(callbackToRun)(result ?: "{}", error ?: "");
                       }];
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

  lynx::trace::TracePlugin* GetMemoryTracePlugin() override {
    intptr_t res = [GlobalDevToolPlatformDarwinDelegate getMemoryTracePlugin];
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

+ (void)queryAllMemoryUsageWithTimeoutMs:(int64_t)timeoutMs
                                callback:(GlobalDevToolMemoryUsageCallback)callback {
  [[LynxMemoryController shareInstance] queryAllMemoryUsageWithTimeoutMs:timeoutMs
                                                                callback:callback];
}

+ (void)handleLynxSettingMethod:(NSString*)method
                            key:(NSString*)key
                          value:(NSString*)value
                       callback:(GlobalDevToolLynxSettingCallback)callback {
  [LynxSettingPlatformDarwin handleMethod:method key:key value:value callback:callback];
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

+ (intptr_t)getMemoryTracePlugin {
  return [[LynxMemoryController shareInstance] getMemoryTracePlugin];
}

+ (std::string)getSystemModelName {
  struct utsname systemInfo;
  uname(&systemInfo);
  NSString* deviceModel = [NSString stringWithCString:systemInfo.machine
                                             encoding:NSUTF8StringEncoding];

  return [deviceModel UTF8String];
}

@end
