// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_SIGNAL_LYNX_SIGNAL_H_
#define CORE_RENDERER_SIGNAL_LYNX_SIGNAL_H_

#include <list>

#include "base/include/vector.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/ref_counted_class.h"
#include "core/runtime/vm/lepus/ref_type.h"

namespace lynx {
namespace tasm {

class SignalContext;
class Computation;

class Signal : public lepus::RefCounted {
 public:
  Signal(SignalContext* context, const lepus::Value& init_value);
  virtual ~Signal();

  SignalContext* signal_context() { return signal_context_; }

  lepus::RefType GetRefType() const { return lepus::RefType::kSignal; }

  void SetValue(const lepus::Value& value);
  lepus::Value GetValue();

  void CleanComputation(Computation* computation);

  virtual void LookUpstream(Computation* ignore) {}

 protected:
  SignalContext* signal_context_;
  lepus::Value value_;
  std::list<Computation*> computation_list_;
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_SIGNAL_LYNX_SIGNAL_H_
