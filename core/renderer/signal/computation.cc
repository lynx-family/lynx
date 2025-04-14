// Inspired by S.js by Adam Haile, https://github.com/adamhaile/S
/**
The MIT License (MIT)

Copyright (c) 2017 Adam Haile

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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

  if (memo_ != nullptr) {
    SetState(ScopeState::kStateNone);
  } else {
    SetState(ScopeState::kStateStale);
  }

  scope->AdoptComputation(fml::RefPtr<Computation>(this));

  signal_context()->UpdateComputation(this);
}

Computation::~Computation() {
  for (auto signal : signal_list_) {
    if (signal == nullptr) {
      continue;
    }
    signal->CleanComputation(this);
  }
  signal_list_.clear();
}

void Computation::CleanUp() {
  for (auto signal : signal_list_) {
    if (signal == nullptr) {
      continue;
    }
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
    if (source->GetRefType() == lepus::RefType::kMemo) {
      auto* memo = static_cast<Memo*>(source);
      auto* computation = memo->GetComputation();
      if (computation == nullptr) {
        continue;
      }
      if (computation->GetState() == ScopeState::kStateStale) {
        if (computation != ignore) {
          signal_context_->RunComputation(computation);
        }
      } else {
        memo->LookUpstream(ignore);
      }
    }
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

void Computation::RemoveSignal(Signal* signal) { signal_list_.remove(signal); }

}  // namespace tasm
}  // namespace lynx
