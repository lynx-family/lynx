// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/lynx_module/lynx_ui_method_registrar.h"

#include <string>
#include <utility>

#include "base/include/compiler_specific.h"
#include "clay/fml/logging.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/native_view.h"

namespace clay {

LynxUIMethodRegistrar& LynxUIMethodRegistrar::Instance() {
  static LynxUIMethodRegistrar instance;
  return instance;
}

void LynxUIMethodRegistrar::Register(std::string&& method_name,
                                     TypeId validation_data,
                                     LynxUIMethod&& method) {
  FML_DCHECK(method);
  methods_.emplace(std::move(method_name),
                   Method(validation_data, std::move(method)));
}

bool LynxUIMethodRegistrar::Invoke(const std::string& method_name,
                                   BaseView* view, const LynxModuleValues& args,
                                   const LynxUIMethodCallback& callback) const {
  auto itr_pair = methods_.equal_range(method_name);
  for (auto itr = itr_pair.first; itr != itr_pair.second; ++itr) {
    const Method& method = itr->second;
    if (view->IsOfType(method.view_type)) {
      method.method(view, args, callback);
      return true;
    }
  }
  // Check if we are trying to call platform ui method in platform view
  if (UNLIKELY(view->Is<NativeView>())) {
    // Platform Method has two ways to notify callback value.
    // 1. Platform search callback id to invoke callback.
    // 2. Platform return result and clay invoke external callback ref
    clay::Value::Map args_map;
    auto& mutable_args = const_cast<LynxModuleValues&>(args);
    for (size_t i = 0; i < mutable_args.names.size(); i++) {
      args_map.insert(
          {mutable_args.names[i], std::move(mutable_args.values[i])});
    }
    static_cast<clay::NativeView*>(view)->InvokePlatformMethod(
        method_name, std::move(args_map), callback);
    return true;
  }

  callback(LynxUIMethodResult::kMethodNotFound,
           clay::Value("method not found: " + method_name));

  FML_LOG(ERROR) << "UI Method (" << method_name << ") can't match view type";
  return false;
}

}  // namespace clay
