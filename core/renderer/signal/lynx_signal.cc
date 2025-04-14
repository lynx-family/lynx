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

#include "core/renderer/signal/lynx_signal.h"

#include "core/renderer/signal/computation.h"
#include "core/renderer/signal/memo.h"
#include "core/renderer/signal/signal_context.h"
#include "core/runtime/vm/lepus/context.h"

namespace lynx {
namespace tasm {

Signal::Signal(SignalContext* context, lepus::Context* vm_context,
               const lepus::Value& init_value)
    : signal_context_(context),
      vm_context_(vm_context),
      value_(init_value),
      computation_list_() {}

Signal::~Signal() {
  for (auto computation : computation_list_) {
    if (computation == nullptr) {
      continue;
    }
    computation->RemoveSignal(this);
  }
  computation_list_.clear();
}

void Signal::SetValue(const lepus::Value& value) {
  if (computation_list_.empty()) {
    value_ = value;
    MarkSkipCompare(false);
    return;
  }

  if (!skip_compare_ && CheckEqual(value)) {
    value_ = value;
    return;
  }

  value_ = value;
  if (signal_context_ == nullptr) {
    LOGE("Signal trigger computation failed since signal_context_ is nullptr.");
    MarkSkipCompare(false);
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
      if (computation->memo() != nullptr) {
        computation->memo()->MarkSkipCompare(skip_compare_);
      }
    }
  });

  MarkSkipCompare(false);
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

bool Signal::CheckEqual(const lepus::Value& new_value) {
  switch (check_equal_type_) {
    case CheckEqualType::kDeepCheck:
      return value_.IsEqual(new_value);
    case CheckEqualType::kStrictCheck:
      // TODO(songshourui.null): use strict equal later.
      return value_.IsEqual(new_value);
    case CheckEqualType::kCustomCheck:
      return vm_context_->CallClosure(check_equal_function_, value_, new_value)
          .Bool();
  }
  return false;
}

}  // namespace tasm
}  // namespace lynx
