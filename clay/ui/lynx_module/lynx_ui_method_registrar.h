// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_LYNX_MODULE_LYNX_UI_METHOD_REGISTRAR_H_
#define CLAY_UI_LYNX_MODULE_LYNX_UI_METHOD_REGISTRAR_H_

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

#include "clay/ui/common/type_info.h"
#include "clay/ui/lynx_module/lynx_ui_method_types.h"
#include "clay/ui/lynx_module/types.h"

namespace clay {

class BaseView;

using LynxUIMethodCallback =
    std::function<void(LynxUIMethodResult code, clay::Value data)>;

using LynxUIMethod =
    std::function<void(BaseView* view, const LynxModuleValues& args,
                       const LynxUIMethodCallback& callback)>;

class LynxUIMethodRegistrar {
 public:
  static LynxUIMethodRegistrar& Instance();

  void Register(std::string&& method_name, TypeId view_type,
                LynxUIMethod&& method);

  bool Invoke(const std::string& method_name, BaseView* view,
              const LynxModuleValues& args,
              const LynxUIMethodCallback& callback) const;

 private:
  struct Method {
    // Validation can be null.
    TypeId view_type;
    LynxUIMethod method;

    Method(Method&& other)
        : view_type(other.view_type), method(std::move(other.method)) {}

    Method(TypeId v, LynxUIMethod&& m) : view_type(v), method(std::move(m)) {}
  };

  std::unordered_multimap<std::string, Method> methods_;
};

// Overloading `GenerateLynxUIMethod()` so that we can accept UI methods with
// different args.
template <typename ViewClass>
LynxUIMethod GenerateLynxUIMethod(void (ViewClass::*ui_method)(
    const LynxModuleValues& args, const LynxUIMethodCallback& callback)) {
  return [ui_method](BaseView* view, const LynxModuleValues& args,
                     const LynxUIMethodCallback& callback) {
    (static_cast<ViewClass*>(view)->*ui_method)(args, callback);
  };
}

template <typename ViewClass>
LynxUIMethod GenerateLynxUIMethod(
    void (ViewClass::*ui_method)(const LynxModuleValues& args)) {
  return [ui_method](BaseView* view, const LynxModuleValues& args,
                     const LynxUIMethodCallback& callback) {
    (static_cast<ViewClass*>(view)->*ui_method)(args);
    callback(LynxUIMethodResult::kSuccess, clay::Value());
  };
}

template <typename ViewClass>
LynxUIMethod GenerateLynxUIMethod(
    void (ViewClass::*ui_method)(const LynxUIMethodCallback& callback)) {
  return [ui_method](BaseView* view, const LynxModuleValues& args,
                     const LynxUIMethodCallback& callback) {
    (static_cast<ViewClass*>(view)->*ui_method)(callback);
  };
}

template <typename ViewClass>
LynxUIMethod GenerateLynxUIMethod(void (ViewClass::*ui_method)()) {
  return [ui_method](BaseView* view, const LynxModuleValues& args,
                     const LynxUIMethodCallback& callback) {
    (static_cast<ViewClass*>(view)->*ui_method)();
    callback(LynxUIMethodResult::kSuccess, clay::Value());
  };
}

#define LYNX_UI_METHOD_BEGIN(__view__)   \
  struct __view__##Registrar {           \
    static __view__##Registrar registar; \
    __view__##Registrar()

#define LYNX_UI_METHOD_END(__view__) \
  }                                  \
  ;                                  \
  __view__##Registrar __view__##Registrar::registar

#define LYNX_UI_METHOD(__view__, __method__)  \
  LynxUIMethodRegistrar::Instance().Register( \
      "" #__method__, __view__::StaticType(), \
      GenerateLynxUIMethod(&__view__::__method__))

}  // namespace clay

#endif  // CLAY_UI_LYNX_MODULE_LYNX_UI_METHOD_REGISTRAR_H_
