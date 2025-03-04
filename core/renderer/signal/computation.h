// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_SIGNAL_COMPUTATION_H_
#define CORE_RENDERER_SIGNAL_COMPUTATION_H_

#include <list>

#include "core/renderer/signal/scope.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/ref_counted_class.h"
#include "core/runtime/vm/lepus/ref_type.h"

namespace lynx {

namespace lepus {
class Context;
}

namespace tasm {

class SignalContext;
class Signal;
class Memo;

class Computation : public BaseScope {
 public:
  Computation(SignalContext* signal_context, lepus::Context* vm_context,
              const lepus::Value& closure, const lepus::Value& value,
              bool pure_computation, Memo* memo);
  virtual ~Computation();

  lepus::RefType GetRefType() const override {
    return lepus::RefType::kComputation;
  }

  void CleanUp() override;

  void MarkDownStream();

  void LookUpstream(Computation* ignore);

  void PushSignal(Signal* signal);

  void Invoke(int32_t time);

  const lepus::Value& GetValue() { return value_; }

 private:
  lepus::Value closure_;
  lepus::Value value_;

  Memo* memo_;

  std::list<Signal*> signal_list_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_SIGNAL_COMPUTATION_H_
