// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/lynx_module_impl.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/bindings/jsi/interceptor/network_monitor.h"
#include "core/runtime/bindings/jsi/modules/lynx_jsi_module_callback.h"
#include "core/runtime/bindings/jsi/modules/module_interceptor.h"
#include "core/runtime/jsi/jsi-inl.h"
#include "core/value_wrapper/value_impl_lepus.h"
#if ENABLE_TESTBENCH_RECORDER
#include "core/services/recorder/native_module_recorder.h"
#endif

namespace lynx {
namespace piper {
struct InvokeInfo {
  std::string method_name;
  piper::NativeModuleInfoCollectorPtr timing_collector;
  bool has_error;
};

class InvokeScope {
 public:
  InvokeScope(std::vector<InvokeInfo*>& infos, InvokeInfo* currentInfo)
      : infos_(infos) {
    infos.push_back(currentInfo);
  }
  ~InvokeScope() { infos_.pop_back(); }

 private:
  std::vector<InvokeInfo*>& infos_;
};

void LynxModuleImpl::Destroy() {
  LOGI("LynxModuleImpl Destroy " << name_);
  native_module_->Destroy();
  native_module_ = nullptr;
}

base::expected<piper::Value, piper::JSINativeException>
LynxModuleImpl::invokeMethod(const MethodMetadata& method, Runtime* rt,
                             const piper::Value* args, size_t count) {
  // TODO(liyanbo.monster): let interceptor use pub::Value args, and reuse those
  // trace and monitor.
  if (group_interceptor_) {
    auto interceptor_result = group_interceptor_->InterceptModuleMethod(
        shared_from_this(), method, rt, delegate_, args, count);
    if (interceptor_result.handled) {
      return std::move(interceptor_result.result);
    }
  }

  uint64_t call_func_start = lynx::base::CurrentSystemTimeMilliseconds();
  piper::Scope scope(*rt);
#if ENABLE_TESTBENCH_RECORDER
  std::vector<int64_t> callback_ids;
#endif  // ENABLE_TESTBENCH_RECORDER

  // timing
  std::string first_arg_str;
  if (count > 0 && args && args[0].isString()) {
    first_arg_str = args[0].getString(*rt).utf8(*rt);
  }
  piper::NativeModuleInfoCollectorPtr timing_collector =
      std::make_shared<piper::NativeModuleInfoCollector>(
          delegate_, name_, method.name, first_arg_str);
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, "JSValueToPubValue");
  uint64_t convert_params_start = base::CurrentSystemTimeMilliseconds();
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY_JSB,
                      "JSBTiming::jsb_func_convert_params_start",
                      [&timing_collector,
                       convert_params_start](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_debug_annotations(
                            "first_arg", timing_collector->GetFirstArg());
                        ctx.event()->add_debug_annotations(
                            "timestamp", std::to_string(convert_params_start));
                      });

  CallbackMap callback_map;
  auto args_array = value_factory_->CreateArray();
  // args is a pointer array. can not use getArray to read value.
  for (size_t i = 0; i < count; i++) {
    const lynx::piper::Value* arg = &args[i];
    if (arg->isBool()) {
      args_array->PushBoolToArray(arg->getBool());
    } else if (arg->isNumber()) {
      args_array->PushDoubleToArray(arg->getNumber());
    } else if (arg->isNull() || arg->isUndefined()) {
      args_array->PushNullToArray();
    } else if (arg->isString()) {
      args_array->PushStringToArray(arg->getString(*rt).utf8(*rt));
    } else if (arg->isObject()) {
      lynx::piper::Object o = arg->getObject(*rt);
      if (o.isArray(*rt)) {
        auto sub_arr = o.getArray(*rt);
        auto sub_arr_result = pub::ValueUtils::ConvertPiperArrayToPubValue(
            *rt, sub_arr, value_factory_);
        args_array->PushValueToArray(std::move(sub_arr_result));
      } else if (o.isArrayBuffer(*rt)) {
        size_t length;
        args_array->PushArrayBufferToArray(
            pub::ValueUtils::ConvertPiperToArrayBuffer(*rt, o, length), length);
      } else if (o.isFunction(*rt)) {
        uint64_t callback_flow_id = TRACE_FLOW_ID();
        TRACE_EVENT_INSTANT(
            LYNX_TRACE_CATEGORY_JSB, "CreateJSB Callback",
            [=](lynx::perfetto::EventContext ctx) {
              ctx.event()->add_flow_ids(callback_flow_id);
              auto* debug = ctx.event()->add_debug_annotations();
              debug->set_name("startTimestamp");
              debug->set_string_value(std::to_string(call_func_start));
            });

        auto function = o.getFunction(*rt);
        auto callback_id =
            delegate_->RegisterJSCallbackFunction(std::move(function));
        auto callback =
            std::make_shared<lynx::piper::ModuleCallback>(callback_id);
        callback->SetModuleName(name_);
        callback->SetMethodName(method.name);
        callback->timing_collector_ = timing_collector;
        callback->SetCallbackFlowId(callback_flow_id);
        callback->SetFirstArg(first_arg_str);
#if ENABLE_TESTBENCH_RECORDER
        callback->SetRecordID(record_id_);
        callback_ids.push_back(callback_id);
#endif  // ENABLE_TESTBENCH_RECORDER
        callback_map.emplace(static_cast<int>(i), std::move(callback));
        args_array->PushInt64ToArray(callback_id);
      } else {
        std::string r;
        auto ret = pub::ValueUtils::ConvertBigIntToStringIfNecessary(*rt, o, r);
        if (ret) {
          args_array->PushBigIntToArray(r);
          continue;
        }
        auto dict = pub::ValueUtils::ConvertPiperObjectToPubValue(
            *rt, o, value_factory_);
        args_array->PushValueToArray(std::move(dict));
      }
    }
  }

  timing_collector->EndFuncParamsConvert(convert_params_start);
  // issue: #1510
  uint64_t invoke_facade_method_start = base::CurrentSystemTimeMilliseconds();
  if (timing_collector != nullptr) {
    TRACE_EVENT_INSTANT(
        LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_func_platform_method_start",
        [first_arg_str = timing_collector->GetFirstArg(),
         invoke_facade_method_start](lynx::perfetto::EventContext ctx) {
          ctx.event()->add_debug_annotations("first_arg", first_arg_str);
          ctx.event()->add_debug_annotations(
              "timestamp", std::to_string(invoke_facade_method_start));
        });
  }
  InvokeInfo invoke_info{.method_name = method.name,
                         .timing_collector = timing_collector,
                         .has_error = false};
  InvokeScope invoke_scope(invoke_scopes_, &invoke_info);
#if (OS_IOS || OS_TVOS || OS_OSX) && \
    (!defined(LYNX_UNIT_TEST) || !LYNX_UNIT_TEST)
  // TODO(liyanbo.monster): when android refact finished, move this out of
  // macro.
  //  We need these information to monitor network request information,
  //  the rate of success and the proportion of requests accomplished by
  //  Lynx. After fully switch to Lynx Network, we can remove these logics.
  network::SetNetworkCallbackInfo(method.name, args_array, count,
                                  timing_collector);
  // TODO(liyanbo.monster): after remove native promise, delete this.
  native_module_->EnterInvokeScope(rt, delegate_);
#endif
  // call method by native module
  auto ret = native_module_->InvokeMethod(method.name, std::move(args_array),
                                          count, callback_map);
  // TODO(liyanbo.monster): after remove native promise, delete this.
  std::optional<piper::Value> promise_res;
#if OS_IOS || OS_TVOS || OS_OSX
  native_module_->ExitInvokeScope();
  // hack here, this will be deleted later.
  if (!ret.has_value() && ret.error() == "__IS_NATIVE_PROMISE__") {
    auto promise_res = native_module_->TryGetPromiseRet();
  }
#endif
  base::expected<piper::Value, piper::JSINativeException> response;
  if (promise_res) {
    response = std::move(*promise_res);
  } else {
    if (!ret.has_value()) {
      timing_collector->OnErrorOccurred(NativeModuleStatusCode::FAILURE);
      response = base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(ret.error()));
    } else if (!ret.value()) {
      response = Value::undefined();
    } else {
      response =
          pub::ValueUtils::ConvertValueToPiperValue(*rt, *(ret.value().get()));
    }
  }

#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::NativeModuleRecorder::GetInstance().RecordFunctionCall(
      name_.c_str(), method.name.c_str(), static_cast<uint32_t>(count), args,
      callback_ids.data(), static_cast<uint32_t>(callback_ids.size()),
      response.value(), rt, record_id_);
#endif  // ENABLE_TESTBENCH_RECORDER

  timing_collector->EndPlatformMethodInvoke(invoke_facade_method_start);
  timing_collector->EndCallFunc(call_func_start);
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, "OnMethodInvoked");
  if (!invoke_info.has_error) {
    delegate_->OnMethodInvoked(name_, method.name, error::E_SUCCESS);
  }
  return response;
}

void LynxModuleImpl::InvokeCallback(
    const std::shared_ptr<LynxModuleCallback>& callback) {
  auto module_callback = std::static_pointer_cast<ModuleCallback>(callback);
  if (module_callback->timing_collector_) {
    module_callback->timing_collector_->CallbackThreadSwitchStart();
  }
  delegate_->CallJSCallback(module_callback);
}

void LynxModuleImpl::RunOnJSThread(base::closure func) {
  delegate_->RunOnJSThread(std::move(func));
}

void LynxModuleImpl::RunOnPlatformThread(base::closure func) {
  delegate_->RunOnPlatformThread(std::move(func));
}

const std::shared_ptr<pub::PubValueFactory>& LynxModuleImpl::GetValueFactory() {
  return value_factory_;
}

void LynxModuleImpl::OnErrorOccurred(const std::string& module_name,
                                     const std::string& method_name,
                                     base::LynxError error) {
  int error_code = error.error_code_;
  NativeModuleStatusCode timing_error_code(NativeModuleStatusCode::SUCCESS);
  if (error_code == error::E_NATIVE_MODULES_COMMON_WRONG_PARAM_TYPE ||
      error_code == error::E_NATIVE_MODULES_COMMON_WRONG_PARAM_NUM) {
    timing_error_code = NativeModuleStatusCode::PARAMETER_ERROR;
  } else if (error_code ==
             error::E_NATIVE_MODULES_COMMON_SYSTEM_AUTHORIZATION_ERROR) {
    timing_error_code = NativeModuleStatusCode::UNAUTHORIZED_BY_SYSTEM;
  } else if (error_code == error::E_NATIVE_MODULES_COMMON_AUTHORIZATION_ERROR) {
    timing_error_code = NativeModuleStatusCode::UNAUTHORIZED;
  } else if (error_code == error::E_NATIVE_MODULES_COMMON_RETURN_ERROR) {
    timing_error_code = NativeModuleStatusCode::RETURN_ERROR;
  }

  auto* current_invoke_info = CurrentInvokeInfo();
  if (current_invoke_info) {
    current_invoke_info->has_error = true;
    delegate_->OnErrorOccurred(std::move(error));
    delegate_->OnMethodInvoked(module_name, method_name, error_code);
    if (timing_error_code != NativeModuleStatusCode::SUCCESS) {
      current_invoke_info->timing_collector->OnErrorOccurred(timing_error_code);
    }
  }
}

// private
void LynxModuleImpl::SetMethodMetadata() {
  auto methods = native_module_->GetMethodList();
  for (auto& pair : methods) {
    auto data = std::make_shared<MethodMetadata>(0, pair.first);
    methodMap_[pair.first] = data;
  }
}

InvokeInfo* LynxModuleImpl::CurrentInvokeInfo() {
  if (invoke_scopes_.size() > 0) {
    return invoke_scopes_.back();
  }
  return nullptr;
}

}  // namespace piper
}  // namespace lynx
