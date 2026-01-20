// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/native_module/lynx_focus_module.h"

#include <string>
#include <utility>

#include "clay/lynx_adaptor/clay_value.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/lynx_module/lynx_ui_method_registrar.h"

namespace lynx {

const std::string LynxFocusModule::name_ = "LynxFocusModule";

LynxFocusModule::LynxFocusModule(uint32_t view_context_id,
                                 fml::RefPtr<fml::TaskRunner> task_runner)
    : LynxModuleBase(view_context_id, task_runner) {
  lynx::runtime::NativeModuleMethod set_trigger_internal("setTriggerInterval",
                                                         1);
  RegisterMethod(set_trigger_internal, &LynxFocusModule::setTriggerInterval);

  lynx::runtime::NativeModuleMethod get_curent_focus("getCurrentFocus", 1);
  RegisterMethod(get_curent_focus, &LynxFocusModule::getCurrentFocus);

  lynx::runtime::NativeModuleMethod native_by_direction("navigateByDirection",
                                                        1);
  RegisterMethod(native_by_direction, &LynxFocusModule::navigateByDirection);
}

LynxFocusModule::~LynxFocusModule() = default;

std::unique_ptr<lynx::pub::Value> LynxFocusModule::setTriggerInterval(
    std::unique_ptr<lynx::pub::Value> args_array,
    const lynx::runtime::CallbackMap& callback_map) {
  fml::TaskRunner::RunNowOrPostTask(
      task_runner_, [weak_this = weak_from_this(),
                     args_array = std::move(args_array), callback_map]() {
        auto strong_this = weak_this.lock();
        if (!strong_this) {
          return;
        }
        auto view_context = clay::Isolate::Instance().GetViewContextById(
            strong_this->view_context_id_);
        if (!view_context) {
          FML_DLOG(ERROR) << "setTriggerInterval failed, view context has "
                             "been destroyed.";
          return;
        }
        int32_t internal = args_array->GetValueAtIndex(0)->Number();
        view_context->GetPageView()->GetFocusManager()->SetTriggerInterval(
            internal);
      });

  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

std::unique_ptr<lynx::pub::Value> LynxFocusModule::getCurrentFocus(
    std::unique_ptr<lynx::pub::Value> args_array,
    const lynx::runtime::CallbackMap& callback_map) {
  fml::TaskRunner::RunNowOrPostTask(task_runner_, [weak_this = weak_from_this(),
                                                   callback_map]() {
    auto strong_this = weak_this.lock();
    if (!strong_this) {
      return;
    }
    auto view_context = clay::Isolate::Instance().GetViewContextById(
        strong_this->view_context_id_);
    if (!view_context) {
      FML_DLOG(ERROR) << "getCurrentFocus failed, view context has "
                         "been destroyed.";
      return;
    }
    auto delegate = strong_this->delegate_.lock();
    if (!delegate) {
      return;
    }

    auto* view = static_cast<clay::BaseView*>(
        view_context->GetPageView()->GetFocusManager()->GetLeafFocusedNode());
    clay::Value::Map map;
    if (view) {
      std::string focus_index =
          std::to_string(view->GetFocusIndex(clay::Axis::kX)) + "," +
          std::to_string(view->GetFocusIndex(clay::Axis::kY));
      std::string id_selector = view->GetIdSelector();
      map["focusIndex"] = clay::Value(std::move(focus_index));
      map["id"] = clay::Value(std::move(id_selector));
    }
    map["exist"] = clay::Value(!!view);

    clay::Value::Array array_wrapper(1);
    array_wrapper[0] = clay::Value(std::move(map));
    auto args = clay::Value(std::move(array_wrapper));
    auto callback = callback_map.at(0);
    callback->SetArgs(std::make_unique<lynx::ClayValue>(std::move(args)));
    delegate->InvokeCallback(callback);
  });
  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

std::unique_ptr<lynx::pub::Value> LynxFocusModule::navigateByDirection(
    std::unique_ptr<lynx::pub::Value> args_array,
    const lynx::runtime::CallbackMap& callbacks) {
  fml::TaskRunner::RunNowOrPostTask(task_runner_, [weak_this = weak_from_this(),
                                                   args_array =
                                                       std::move(args_array),
                                                   callbacks]() {
    auto strong_this = weak_this.lock();
    if (!strong_this) {
      return;
    }
    auto view_context = clay::Isolate::Instance().GetViewContextById(
        strong_this->view_context_id_);
    if (!view_context) {
      FML_DLOG(ERROR) << "navigateByDirection failed, view context has "
                         "been destroyed.";
      return;
    }

    auto direction_str = args_array->GetValueAtIndex(0)->str();
    clay::FocusManager::Direction direction;
    if (direction_str == "left") {
      direction = clay::FocusManager::Direction::kLeft;
    } else if (direction_str == "right") {
      direction = clay::FocusManager::Direction::kRight;
    } else if (direction_str == "up") {
      direction = clay::FocusManager::Direction::kUp;
    } else if (direction_str == "down") {
      direction = clay::FocusManager::Direction::kDown;
    } else {
      FML_DLOG(ERROR) << "navigateByDirection failed, unrecognized direction: "
                      << direction_str;
      return;
    }
    view_context->GetPageView()->GetFocusManager()->DoTraversal(direction);
  });

  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

}  // namespace lynx
