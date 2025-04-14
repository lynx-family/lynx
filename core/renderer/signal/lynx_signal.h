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

#ifndef CORE_RENDERER_SIGNAL_LYNX_SIGNAL_H_
#define CORE_RENDERER_SIGNAL_LYNX_SIGNAL_H_

#include <list>

#include "base/include/vector.h"
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

enum class CheckEqualType : int32_t {
  kDeepCheck = 0,
  kStrictCheck,
  kCustomCheck,
};

class Signal : public lepus::RefCounted {
 public:
  Signal(SignalContext* context, lepus::Context* vm_context,
         const lepus::Value& init_value);
  virtual ~Signal();

  SignalContext* signal_context() { return signal_context_; }

  lepus::RefType GetRefType() const { return lepus::RefType::kSignal; }

  void SetValue(const lepus::Value& value);
  lepus::Value GetValue();

  void CleanComputation(Computation* computation);

  virtual void LookUpstream(Computation* ignore) {}

  void SetCheckEqualType(CheckEqualType type) { check_equal_type_ = type; }

  void SetCustomEqualFunction(const lepus::Value& function) {
    SetCheckEqualType(CheckEqualType::kCustomCheck);
    check_equal_function_ = function;
  }

  void MarkSkipCompare(bool skip_compare) { skip_compare_ = skip_compare; }

 protected:
  bool CheckEqual(const lepus::Value& new_value);

  bool skip_compare_{false};
  CheckEqualType check_equal_type_{CheckEqualType::kDeepCheck};

  SignalContext* signal_context_;
  lepus::Context* vm_context_;

  lepus::Value value_;
  std::list<Computation*> computation_list_;

  lepus::Value check_equal_function_;
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_SIGNAL_LYNX_SIGNAL_H_
