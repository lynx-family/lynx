// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_IOS_NATIVE_FACADE_REPORTER_DARWIN_H_
#define CORE_SHELL_IOS_NATIVE_FACADE_REPORTER_DARWIN_H_

#import <Foundation/Foundation.h>

#include <memory>

#include "core/shell/native_facade_reporter.h"
#import "darwin/common/lynx/TemplateRenderCallbackProtocol.h"

namespace lynx {
namespace shell {

class NativeFacadeReporterDarwin : public NativeFacadeReporter {
 public:
  // TODO(heshan): will use adapter instead after ios platfrom ready
  NativeFacadeReporterDarwin(id<TemplateRenderCallbackProtocol> render)
      : _render(render) {}
  ~NativeFacadeReporterDarwin() override = default;

  NativeFacadeReporterDarwin(const NativeFacadeReporterDarwin& facade) = delete;
  NativeFacadeReporterDarwin& operator=(const NativeFacadeReporterDarwin&) =
      delete;

  NativeFacadeReporterDarwin(NativeFacadeReporterDarwin&& facade) = default;
  NativeFacadeReporterDarwin& operator=(NativeFacadeReporterDarwin&&) = default;

  void OnPerformanceEvent(const lepus::Value& entry) override;

 private:
  __weak id<TemplateRenderCallbackProtocol> _render;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_IOS_NATIVE_FACADE_REPORTER_DARWIN_H_
