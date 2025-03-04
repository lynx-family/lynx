// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/signal/lynx_signal.h"

#include "core/renderer/signal/computation.h"
#include "core/renderer/signal/signal_context.h"

namespace lynx {
namespace tasm {

Signal::Signal(SignalContext* context, const lepus::Value& init_value)
    : signal_context_(context), value_(init_value), computation_list_() {}

Signal::~Signal() {}

void Signal::SetValue(const lepus::Value& value) {
  if (value_.IsEqual(value)) {
    return;
  }
  value_ = value;
  if (computation_list_.empty()) {
    return;
  }
  if (signal_context_ == nullptr) {
    LOGE("Signal trigger computation failed since signal_context_ is nullptr.");
    return;
  }
  signal_context_->RunUpdates([this]() {
    for (auto computation : computation_list_) {
      if (computation->GetState() == ScopeState::kStateNone) {
        signal_context_->EnqueueComputation(computation);
        if (computation->GetScopeType() == ScopeType::kMemoComputation) {
          computation->MarkDownStream();
        }
      }
      computation->SetState(ScopeState::kStateStale);
    }
  });
}

lepus::Value Signal::GetValue() {
  if (signal_context_ == nullptr) {
    LOGE("Signal GetValue failed since signal_context_ is nullptr.");
    return value_;
  }

  auto computation = signal_context_->GetTopComputation();
  if (computation != nullptr) {
    computation->PushSignal(this);
    computation_list_.emplace_back(computation);
  }
  return value_;
}

void Signal::CleanComputation(Computation* computation) {
  computation_list_.remove(computation);
}

}  // namespace tasm
}  // namespace lynx
