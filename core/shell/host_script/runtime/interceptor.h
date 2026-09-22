// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_INTERCEPTOR_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_INTERCEPTOR_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/include/log/log_context.h"
#include "base/include/value/base_value.h"

namespace lynx::shell {

enum class InterceptKind {
  kCreate,
  kCreated,
  kLoadTemplate,
  kUpdateMetaData,
  kCall,
  kResult,
  kCallback,
  kCount
};

struct InterceptResult {
  // Only explicitly changed fields are returned. The caller commits them once.
  lepus::Value patch;
  lepus::Value mock;
  bool failed = false;
};

// Optional synchronous, thread-affine extension. No JS engine dependency.
class Interceptor {
 public:
  enum class Domain { kUI, kBTS, kMTS };
  virtual ~Interceptor() = default;
  virtual void Uninstall() { Detach(this); }
  virtual bool HasHandlers(InterceptKind kind) const = 0;
  virtual InterceptResult Dispatch(InterceptKind kind,
                                   const lepus::Value& event) = 0;
  virtual void ReportError(const std::string& message) = 0;

  // One provider per domain on its owning thread. Domains may share a thread.
  // Call sites must name the execution domain; platform entries default to UI.
  static bool Attach(const std::shared_ptr<Interceptor>& provider,
                     Domain domain = Domain::kUI);
  virtual bool IsAttached() const { return attached_; }
  static void Detach(const Interceptor* provider);
  static std::shared_ptr<Interceptor> Current(InterceptKind kind,
                                              Domain domain = Domain::kUI);
  static void HandlerAdded(InterceptKind kind, Domain domain = Domain::kUI);
  static void HandlerRemoved(InterceptKind kind, Domain domain = Domain::kUI);
  static bool IsEnabled();
  static void ReportCoverageGap(InterceptKind kind, const char* reason);
  static uint64_t FailureCount();

  // Track liveness without owning a View or allocating another identity.
  static void RegisterView(base::LynxEntityId view);
  static void DestroyView(base::LynxEntityId view);
  static bool IsViewAlive(base::LynxEntityId view);
  static const char* Name(InterceptKind kind);

 private:
  bool attached_ = false;
};

}  // namespace lynx::shell
#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_INTERCEPTOR_H_
