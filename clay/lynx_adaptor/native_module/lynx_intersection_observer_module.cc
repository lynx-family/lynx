// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/native_module/lynx_intersection_observer_module.h"

#include <memory>
#include <string>
#include <utility>

#include "clay/lynx_adaptor/clay_value.h"
#include "clay/lynx_adaptor/value_converter.h"
#include "clay/ui/common/value_utils.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"

namespace lynx {

const std::string LynxIntersectionObserverModule::name_ =
    "IntersectionObserverModule";

LynxIntersectionObserverModule::LynxIntersectionObserverModule(
    uint32_t view_context_id, fml::RefPtr<fml::TaskRunner> task_runner)
    : LynxModuleBase(view_context_id, task_runner) {
  lynx::runtime::NativeModuleMethod create_intersection_observer(
      "createIntersectionObserver", 3);
  RegisterMethod(create_intersection_observer,
                 &LynxIntersectionObserverModule::CreateIntersectionObserver);
  lynx::runtime::NativeModuleMethod relative_to("relativeTo", 3);
  RegisterMethod(relative_to, &LynxIntersectionObserverModule::RelativeTo);
  lynx::runtime::NativeModuleMethod observe("observe", 3);
  RegisterMethod(observe, &LynxIntersectionObserverModule::Observe);
  lynx::runtime::NativeModuleMethod disconnect("disconnect", 1);
  RegisterMethod(disconnect, &LynxIntersectionObserverModule::Disconnect);
  lynx::runtime::NativeModuleMethod relative_to_screen("relativeToScreen", 2);
  RegisterMethod(relative_to_screen,
                 &LynxIntersectionObserverModule::RelativeToScreen);
  lynx::runtime::NativeModuleMethod relative_to_viewport("relativeToViewport",
                                                         2);
  RegisterMethod(relative_to_viewport,
                 &LynxIntersectionObserverModule::RelativeToViewport);
}

LynxIntersectionObserverModule::~LynxIntersectionObserverModule() = default;

std::unique_ptr<lynx::pub::Value>
LynxIntersectionObserverModule::CreateIntersectionObserver(
    std::unique_ptr<lynx::pub::Value> args_array,
    const lynx::runtime::CallbackMap& callback_map) {
  fml::TaskRunner::RunNowOrPostTask(
      task_runner_, [week_this = weak_from_this(),
                     args_array = std::move(args_array), callback_map]() {
        auto strong_this = week_this.lock();
        if (strong_this) {
          auto view_context = clay::Isolate::Instance().GetViewContextById(
              strong_this->view_context_id_);
          if (!view_context) {
            FML_LOG(ERROR) << "setTriggerInterval failed, view context has "
                              "been destroyed.";
            return;
          }
          int id = args_array->GetValueAtIndex(0)->Int32();
          int component = args_array->GetValueAtIndex(1)->Int32();
          auto options = args_array->GetValueAtIndex(2);
          clay::Value clay_option_value =
              ValueConverter::CreateClayValue(*options);
          clay_option_value.GetMap()["componentId"] = clay::Value(component);
          clay_option_value.GetMap()["customObserverId"] = clay::Value(id);
          strong_this->observer_configs_[id] =
              std::move(clay_option_value.GetMap());
        }
      });
  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

std::unique_ptr<lynx::pub::Value> LynxIntersectionObserverModule::RelativeTo(
    std::unique_ptr<lynx::pub::Value> args_array,
    const lynx::runtime::CallbackMap& callback_map) {
  fml::TaskRunner::RunNowOrPostTask(task_runner_, [week_this = weak_from_this(),
                                                   args_array =
                                                       std::move(args_array),
                                                   callback_map]() {
    auto strong_this = week_this.lock();
    if (strong_this) {
      auto view_context = clay::Isolate::Instance().GetViewContextById(
          strong_this->view_context_id_);
      if (!view_context) {
        FML_DLOG(ERROR) << "setTriggerInterval failed, view context has "
                           "been destroyed.";
        return;
      }
      if (args_array->Length() != 3) {
        FML_LOG(ERROR) << "relativeTo failed, args length is not 2.";
        return;
      }
      int id = args_array->GetValueAtIndex(0)->Int32();
      std::string relativeToIdSelector = args_array->GetValueAtIndex(1)->str();
      clay::Value::Map& option = strong_this->observer_configs_[id];
      option["relativeToIdSelector"] = clay::Value(relativeToIdSelector);
      args_array->GetValueAtIndex(2)->ForeachMap(
          [&option](const pub::Value& key, const pub::Value& value) {
            if (!key.IsString()) {
              return;
            }
            if (key.str() == "left") {
              option["marginLeft"] = ValueConverter::CreateClayValue(value);
            }
            if (key.str() == "right") {
              option["marginRight"] = ValueConverter::CreateClayValue(value);
            }
            if (key.str() == "top") {
              option["marginTop"] = ValueConverter::CreateClayValue(value);
            }
            if (key.str() == "bottom") {
              option["marginBottom"] = ValueConverter::CreateClayValue(value);
            }
          });
    }
  });
  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

std::unique_ptr<lynx::pub::Value> LynxIntersectionObserverModule::Observe(
    std::unique_ptr<lynx::pub::Value> args_array,
    const lynx::runtime::CallbackMap& callback_map) {
  fml::TaskRunner::RunNowOrPostTask(
      task_runner_, [week_this = weak_from_this(),
                     args_array_ = std::move(args_array), callback_map]() {
        auto strong_this = week_this.lock();
        if (strong_this) {
          auto view_context = clay::Isolate::Instance().GetViewContextById(
              strong_this->view_context_id_);
          if (!view_context) {
            FML_DLOG(ERROR) << "setTriggerInterval failed, view context has "
                               "been destroyed.";
            return;
          }
          int id = args_array_->GetValueAtIndex(0)->Int32();
          std::string target_id_selector =
              args_array_->GetValueAtIndex(1)->str().substr(1);
          int callback_id = args_array_->GetValueAtIndex(2)->Int32();
          clay::BaseView* target = view_context->FindViewByIdSelector(
              target_id_selector, view_context->GetPageView());
          if (target == nullptr) {
            FML_LOG(ERROR) << "relativeTo failed, target is null.";
            return;
          }
          auto it = strong_this->observer_configs_.find(id);
          if (it == strong_this->observer_configs_.end()) {
            FML_LOG(ERROR) << "Observe failed, no observer config for id: "
                           << id;
            return;
          }
          clay::Value::Map& option_map = it->second;
          clay::Value::Map option_cp;
          for (const auto& [key, value] : option_map) {
            option_cp.emplace(key, CloneClayValue(value));
          }
          option_cp.emplace("customCallbackId", clay::Value(callback_id));
          clay::Value option_value = clay::Value(std::move(option_cp));
          clay::Value::Array wrapper_array = {};
          wrapper_array.emplace_back(std::move(option_value));
          clay::Value wrapper = clay::Value(std::move(wrapper_array));
          target->SetAttribute("intersection-observers", std::move(wrapper));
          auto it2 = strong_this->observers_.find(id);
          if (it2 == strong_this->observers_.end()) {
            strong_this->observers_[id] = {};
          }
          strong_this->observers_[id].push_back(target);
        }
      });
  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

std::unique_ptr<lynx::pub::Value> LynxIntersectionObserverModule::Disconnect(
    std::unique_ptr<lynx::pub::Value> args_array,
    const lynx::runtime::CallbackMap& callback_map) {
  fml::TaskRunner::RunNowOrPostTask(
      task_runner_, [week_this = weak_from_this(),
                     args_array = std::move(args_array), callback_map]() {
        auto strong_this = week_this.lock();
        if (strong_this) {
          auto view_context = clay::Isolate::Instance().GetViewContextById(
              strong_this->view_context_id_);
          if (!view_context) {
            FML_DLOG(ERROR) << "setTriggerInterval failed, view context has "
                               "been destroyed.";
            return;
          }
          if (args_array->Length() != 1) {
            FML_LOG(ERROR) << "relativeTo failed, args length is not 3.";
            return;
          }
          int id = args_array->GetValueAtIndex(0)->Int32();
          auto observer_manager =
              view_context->GetPageView()->intersection_observer_manager();
          for (auto observer : strong_this->observers_[id]) {
            observer_manager->RemoveIntersectionObserver(observer);
          }
          strong_this->observers_.erase(id);
          strong_this->observer_configs_.erase(id);
        }
      });
  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

// in clay, root view will always be a sub view of screen, thus relative to
// screen is equal to relative to viewport
// the additional situation is x-overlay-ng, will be supported in the future.
std::unique_ptr<lynx::pub::Value>
LynxIntersectionObserverModule::RelativeToScreen(
    std::unique_ptr<lynx::pub::Value> args_array,
    const lynx::runtime::CallbackMap& callback_map) {
  return RelativeToViewport(std::move(args_array), callback_map);
}

std::unique_ptr<lynx::pub::Value>
LynxIntersectionObserverModule::RelativeToViewport(
    std::unique_ptr<lynx::pub::Value> args_array,
    const lynx::runtime::CallbackMap& callback_map) {
  fml::TaskRunner::RunNowOrPostTask(
      task_runner_, [week_this = weak_from_this(),
                     args_array = std::move(args_array), callback_map]() {
        auto strong_this = week_this.lock();
        if (strong_this) {
          auto view_context = clay::Isolate::Instance().GetViewContextById(
              strong_this->view_context_id_);
          if (!view_context) {
            FML_DLOG(ERROR) << "setTriggerInterval failed, view context has "
                               "been destroyed.";
            return;
          }
          if (args_array->Length() != 3) {
            FML_LOG(ERROR) << "relativeTo failed, args length is not 2.";
            return;
          }
          int id = args_array->GetValueAtIndex(0)->Int32();
          clay::Value::Map& option = strong_this->observer_configs_[id];
          std::string target_id_selector =
              option["componentId"].GetString().substr(1);
          clay::BaseView* target = view_context->FindViewByIdSelector(
              target_id_selector, view_context->GetPageView());
          if (target == nullptr) {
            FML_LOG(ERROR) << "relativeTo failed, target is null.";
            return;
          }

          args_array->GetValueAtIndex(1)->ForeachMap(
              [&option](const pub::Value& key, const pub::Value& value) {
                if (!key.IsString()) {
                  return;
                }
                if (key.str() == "left") {
                  option["marginLeft"] = ValueConverter::CreateClayValue(value);
                }
                if (key.str() == "right") {
                  option["marginRight"] =
                      ValueConverter::CreateClayValue(value);
                }
                if (key.str() == "top") {
                  option["marginTop"] = ValueConverter::CreateClayValue(value);
                }
                if (key.str() == "bottom") {
                  option["marginBottom"] =
                      ValueConverter::CreateClayValue(value);
                }
              });
        }
      });
  return std::make_unique<lynx::ClayValue>(clay::Value(true));
}

}  // namespace lynx
