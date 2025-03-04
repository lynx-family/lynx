// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/signal/computation.h"

#include "core/renderer/signal/memo.h"
#include "core/renderer/signal/signal_context.h"
#include "core/runtime/vm/lepus/context.h"

namespace lynx {
namespace tasm {

Computation::Computation(SignalContext* signal_context_ptr,
                         lepus::Context* vm_context_ptr,
                         const lepus::Value& closure, const lepus::Value& value,
                         bool pure_computation, Memo* memo)
    : BaseScope(signal_context_ptr, vm_context_ptr),
      closure_(closure),
      value_(value),
      memo_(memo) {
  auto scope = signal_context()->GetTopScope();

  if (scope == nullptr) {
    LOGE("Init Computation error, the scope is nullptr.");
    return;
  }

  if (!pure_computation || memo != nullptr) {
    SetScopeType(ScopeType::kMemoComputation);
  } else {
    SetScopeType(ScopeType::kPureComputation);
  }

  scope->AdoptComputation(fml::RefPtr<Computation>(this));

  signal_context()->UpdateComputation(this);

  SetState(ScopeState::kStateNone);
}

Computation::~Computation() {}

void Computation::CleanUp() {
  for (auto signal : signal_list_) {
    signal->CleanComputation(this);
  }
  signal_list_.clear();

  BaseScope::CleanUp();

  SetState(ScopeState::kStateNone);
}

void Computation::MarkDownStream() {
  if (memo_ == nullptr) {
    return;
  }
  memo_->MarkDownStream();
}

void Computation::LookUpstream(Computation* ignore) {
  SetState(ScopeState::kStateNone);

  if (signal_list_.empty()) {
    return;
  }

  for (auto source : signal_list_) {
    source->LookUpstream(ignore);
  }
}

void Computation::PushSignal(Signal* signal) {
  signal_list_.emplace_back(signal);
}

void Computation::Invoke(int32_t time) {
  if (vm_context() == nullptr) {
    LOGE("Computation Invoke failed since vm_context_ is nullptr.");
    return;
  }

  value_ = vm_context()->CallClosure(closure_, value_);

  if (GetUpdatedTime() <= time &&
      GetScopeType() == ScopeType::kMemoComputation) {
    if (memo_ != nullptr) {
      memo_->OnInvoked(value_);
    }
    SetUpdatedTime(time);
  }
}

}  // namespace tasm
}  // namespace lynx
