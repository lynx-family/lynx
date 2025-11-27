// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_DARWIN_MACOS_COMMON_VSYNC_MONITOR_MACOS_H_
#define PLATFORM_DARWIN_MACOS_COMMON_VSYNC_MONITOR_MACOS_H_

#import <Foundation/Foundation.h>
#import <QuartzCore/CADisplayLink.h>

#include "base/include/fml/time/time_point.h"
#include "core/base/threading/vsync_monitor.h"

API_AVAILABLE(macos(14.0))
@interface DisplayLinkImpl : NSObject
@property(nonatomic, strong) CADisplayLink* displayLink;
@end

namespace lynx {
namespace base {

class API_AVAILABLE(macos(14.0)) VSyncMonitorMacOS : public VSyncMonitor {
 public:
  ~VSyncMonitorMacOS() override;

  void Init() override;
  void RequestVSync() override;

 private:
  template <typename Block>
  void ExecuteOnMainThread(Block block);
  std::atomic<bool> destroying_{false};

  DisplayLinkImpl* impl_ = nil;
};

}  // namespace base
}  // namespace lynx

#endif  // PLATFORM_DARWIN_MACOS_COMMON_VSYNC_MONITOR_MACOS_H_
