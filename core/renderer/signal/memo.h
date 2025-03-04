// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_SIGNAL_MEMO_H_
#define CORE_RENDERER_SIGNAL_MEMO_H_

#include "core/renderer/signal/lynx_signal.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/ref_counted_class.h"
#include "core/runtime/vm/lepus/ref_type.h"

namespace lynx {

namespace lepus {
class Context;
}

namespace tasm {

class SignalContext;
class Computation;

class Memo : public Signal {
 public:
  Memo(SignalContext* signal_context, lepus::Context* vm_context,
       const lepus::Value& closure, const lepus::Value& value);

  virtual ~Memo();

  lepus::RefType GetRefType() const override { return lepus::RefType::kMemo; }

  void CleanUp();

  void OnCleanUp(const lepus::Value& block);

  void OnInvoked(const lepus::Value& value);

  void MarkDownStream();

  void LookUpstream(Computation* ignore) override;

 private:
  fml::RefPtr<Computation> computation_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_SIGNAL_MEMO_H_
