// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_JAVA_SCRIPT_ELEMENT_H_
#define CORE_RUNTIME_BINDINGS_JSI_JAVA_SCRIPT_ELEMENT_H_

#include <memory>
#include <string>
#include <vector>

#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {

class App;
class Runtime;

class JavaScriptElement : public HostObject {
 public:
  enum AnimationOperation : int32_t { START = 0, PLAY, PAUSE, CANCEL, FINISH };

  JavaScriptElement(std::weak_ptr<Runtime> rt, std::weak_ptr<App> app,
                    const std::string& root_id, const std::string& selector_id)
      : rt_(rt),
        native_app_(app),
        root_id_(root_id),
        selector_id_(selector_id){};
  virtual ~JavaScriptElement() override {
    LOGI("LYNX ~NativeElement destroy");
  };

  virtual Value get(Runtime*, const PropNameID& name) override;
  virtual void set(Runtime*, const PropNameID& name,
                   const Value& value) override;
  virtual std::vector<PropNameID> getPropertyNames(Runtime& rt) override;

 private:
  std::weak_ptr<Runtime> rt_;
  std::weak_ptr<App> native_app_;
  std::string root_id_;
  std::string selector_id_;
};
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_JAVA_SCRIPT_ELEMENT_H_
