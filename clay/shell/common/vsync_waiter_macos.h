// Copyright 2025 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_VSYNC_WAITER_MACOS_H_
#define CLAY_SHELL_COMMON_VSYNC_WAITER_MACOS_H_

#import <Foundation/Foundation.h>
#import <QuartzCore/CADisplayLink.h>

#include <memory>
#include <utility>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/fml/time/time_point.h"
#include "clay/shell/common/services/vsync_waiter_service.h"
#include "clay/shell/common/vsync_waiter.h"

API_AVAILABLE(macos(14.0))
@interface DisplayLinkManager : NSObject
@property(nonatomic, strong) CADisplayLink* displayLink;
@end

namespace clay {

class API_AVAILABLE(macos(14.0)) VsyncWaiterMacOS final : public VsyncWaiter {
 public:
  explicit VsyncWaiterMacOS(fml::RefPtr<fml::TaskRunner> task_runner);
  ~VsyncWaiterMacOS() override;

 private:
  // |VsyncWaiter|
  void AwaitVSync() override;

  DisplayLinkManager* manager_ = nil;

  BASE_DISALLOW_COPY_AND_ASSIGN(VsyncWaiterMacOS);
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_VSYNC_WAITER_MACOS_H_
