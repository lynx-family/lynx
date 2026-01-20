// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/native_module/lynx_ui_method_module.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/lynx_adaptor/clay_value.h"
#include "clay/lynx_adaptor/value_converter.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/lynx_module/lynx_ui_method_registrar.h"
#include "clay/ui/lynx_module/type_utils.h"
#include "core/public/jsb/lynx_native_module.h"

namespace lynx {

namespace {

constexpr char kStrIsCalledByRefId[] = "_isCallByRefId";

struct CallbackData {
  std::shared_ptr<lynx::runtime::LynxModuleCallback> callback;
  std::weak_ptr<lynx::runtime::LynxNativeModule::Delegate> delegate;
  clay::LynxUIMethodCallback ui_callback;
};

}  // namespace

const std::string LynxUIMethodModule::name_ = "LynxUIMethodModule";

LynxUIMethodModule::LynxUIMethodModule(uint32_t view_context_id,
                                       fml::RefPtr<fml::TaskRunner> task_runner)
    : LynxModuleBase(view_context_id, task_runner) {
  // argmentlist: "view_id", "nodes", "method_name", "args", "callback"
  lynx::runtime::NativeModuleMethod invoke_ui_method("invokeUIMethod", 5);
  RegisterMethod(invoke_ui_method,
                 &LynxUIMethodModule::InvokeUIMethodCompatibility);
}

LynxUIMethodModule::~LynxUIMethodModule() = default;

std::unique_ptr<lynx::pub::Value>
LynxUIMethodModule::InvokeUIMethodCompatibility(
    std::unique_ptr<lynx::pub::Value> args_array,
    const lynx::runtime::CallbackMap& callbacks) {
  auto module_values = std::make_shared<clay::LynxModuleValues>();
  CallbackData* callback_data = nullptr;

  args_array->ForeachArray([&](int64_t key, const pub::Value& value) {
    runtime::CallbackMap::const_iterator it;
    if (value.IsInt64() && (it = callbacks.find(key)) != callbacks.end()) {
      callback_data =
          new CallbackData{.callback = it->second, .delegate = delegate_};
      callback_data->ui_callback =
          [callback_data](clay::LynxUIMethodResult code, clay::Value data) {
            auto delegate = callback_data->delegate.lock();
            if (!delegate || !callback_data->callback) {
              delete callback_data;
              return;
            }

            clay::Value::Map map;
            map.emplace("code", clay::Value(static_cast<int>(code)));
            map.emplace("data", std::move(data));

            clay::Value::Array array_wrapper(1);
            array_wrapper[0] = clay::Value(std::move(map));

            callback_data->callback->SetArgs(std::make_unique<lynx::ClayValue>(
                clay::Value(std::move(array_wrapper))));
            delegate->InvokeCallback(callback_data->callback);
            delete callback_data;
          };
    } else {
      module_values->values.push_back(
          lynx::ValueConverter::CreateClayValue(value));
    }
  });
  if (!callback_data) {
    FML_DLOG(ERROR) << "callback is empty!";
    return std::make_unique<lynx::ClayValue>(clay::Value());
  }

  fml::TaskRunner::RunNowOrPostTask(
      task_runner_,
      [weak_this = weak_from_this(), callback_data, module_values]() {
        auto strong_this = weak_this.lock();
        if (!strong_this) {
          delete callback_data;
          return;
        }

        strong_this->EnsureInvokeAfterLayout(
            [weak_this, callback_data, module_values] {
              auto strong_this = weak_this.lock();
              if (!strong_this) {
                delete callback_data;
                return;
              }
              std::string view_id;
              std::vector<std::string> nodes;
              std::string ui_method_name;
              clay::LynxModuleValues ui_args;
              CastLynxModuleArgs(*module_values, view_id, nodes, ui_method_name,
                                 ui_args);
              strong_this->InvokeUIMethod(view_id, nodes, ui_method_name,
                                          ui_args, callback_data->ui_callback);
            });
      });
  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

void LynxUIMethodModule::InvokeUIMethod(
    const std::string& component_id, const std::vector<std::string>& nodes,
    const std::string& method_name, const clay::LynxModuleValues& args,
    const clay::LynxUIMethodCallback& callback) {
  auto view_context =
      clay::Isolate::Instance().GetViewContextById(view_context_id_);
  if (!view_context) {
    FML_DLOG(ERROR) << "ViewContext has been released.";
    return;
  }

  clay::BaseView* root = view_context->FindViewByComponentId(component_id);
  if (root == nullptr || nodes.size() != 1) {
    FML_DLOG(ERROR) << "Cannot find view with component id:" << component_id;
    callback(clay::LynxUIMethodResult::kNodeNotFound,
             clay::Value("component not found"));
    return;
  }

  bool is_called_by_ref =
      clay::attribute_utils::GetBool(args.Get(kStrIsCalledByRefId), false);
  if (is_called_by_ref) {
    callback(clay::LynxUIMethodResult::kParamInvalid,
             clay::Value("called by ref is not supported"));
    return;
  }

  const std::string& node_str = nodes[0];
  if (node_str.length() > 0 && node_str[0] == '#') {
    auto node =
        clay::ViewContext::FindViewByIdSelector(node_str.substr(1), root);
    if (node != nullptr) {
      clay::LynxUIMethodRegistrar::Instance().Invoke(method_name, node, args,
                                                     callback);
    } else {
      callback(clay::LynxUIMethodResult::kNodeNotFound,
               clay::Value("node not found"));
    }
  } else {
    callback(clay::LynxUIMethodResult::kSelectorNotSupported,
             clay::Value(
                 "selector not support, only support id selector currently"));
  }
}

void LynxUIMethodModule::EnsureInvokeAfterLayout(
    std::function<void()> invocation) {
  auto view_context =
      clay::Isolate::Instance().GetViewContextById(view_context_id_);
  if (!view_context) {
    FML_DLOG(ERROR)
        << "invokeUIMethod failed, view context has been destroyed.";
    return;
  }
  auto page_view = static_cast<clay::PageView*>(view_context->GetPageView());
  if (!page_view->GetLayoutController()->HasDirtyNodes()) {
    invocation();
  } else {
    // If there is a pending layout, we make the call in the next frame to
    // ensure that the layout is updated. This is a workaround for the case
    // of invoking UI methods on a list item in the callback of `setData`.
    page_view->PostUIMethodTask(invocation);
  }
}

}  // namespace lynx
