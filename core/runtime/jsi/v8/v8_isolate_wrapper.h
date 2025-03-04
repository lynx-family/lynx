// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_V8_V8_ISOLATE_WRAPPER_H_
#define CORE_RUNTIME_JSI_V8_V8_ISOLATE_WRAPPER_H_

#include <unordered_map>

#include "core/base/observer/observer_list.h"
#include "core/runtime/jsi/jsi.h"
#include "v8.h"

namespace lynx {
namespace piper {

class V8IsolateInstance : public VMInstance {
 public:
  V8IsolateInstance() = default;
  virtual ~V8IsolateInstance() = default;

  virtual void InitIsolate(const char* arg, bool useSnapshot) = 0;

  // void AddObserver(base::Observer* obs) { observers_.AddObserver(obs); }
  // void RemoveObserver(base::Observer* obs) {
  // observers_.RemoveObserver(obs);
  // }
  virtual v8::Isolate* Isolate() const = 0;
  JSRuntimeType GetRuntimeType() { return piper::JSRuntimeType::v8; }
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_V8_V8_ISOLATE_WRAPPER_H_
