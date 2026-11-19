// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/capi_module/lynx_capi_module.h"

#include <atomic>
#include <limits>
#include <utility>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "core/value_wrapper/value_wrapper_utils.h"
#include "platform/embedder/lynx_view_builder_priv.h"

namespace lynx {
namespace embedder {
class LynxCapiModule;
}  // namespace embedder
}  // namespace lynx

struct lynx_native_module_callback_t {
  std::atomic<uint32_t> reference_count{1};
  std::atomic<bool> invoked{false};
  std::shared_ptr<lynx::runtime::LynxModuleCallback> callback;
  std::weak_ptr<lynx::embedder::LynxCapiModule> module;
};

namespace lynx {
namespace embedder {
namespace {

// A C consumer has no runtime environment with which to read extended values.
bool IsPortableValue(const lepus::Value& value) {
  switch (value.value().type) {
    case lynx_value_null:
    case lynx_value_undefined:
    case lynx_value_bool:
    case lynx_value_double:
    case lynx_value_int32:
    case lynx_value_uint32:
    case lynx_value_int64:
    case lynx_value_uint64:
    case lynx_value_nan:
    case lynx_value_external:
      return true;
    case lynx_value_string:
    case lynx_value_arraybuffer:
      return value.Ptr() != nullptr;
    case lynx_value_array:
    case lynx_value_map: {
      if (value.Ptr() == nullptr) {
        return false;
      }
      bool valid = true;
      tasm::ForEachLepusValue(
          value, [&valid](const lepus::Value&, const lepus::Value& item) {
            if (valid) {
              valid = IsPortableValue(item);
            }
          });
      return valid;
    }
    default:
      return false;
  }
}

}  // namespace

class LynxCapiModule final
    : public runtime::LynxNativeModule,
      public std::enable_shared_from_this<LynxCapiModule> {
 public:
  LynxCapiModule(std::string name,
                 std::shared_ptr<LynxCapiModuleRegistry> registry,
                 runtime::NativeModuleMethods methods)
      : name_(std::move(name)), registry_(std::move(registry)) {
    methods_ = std::move(methods);
  }

  void Destroy() override { active_.store(false, std::memory_order_release); }

  bool IsActive() const {
    return active_.load(std::memory_order_acquire) && registry_->IsActive();
  }

  bool InvokeCallback(
      const std::shared_ptr<runtime::LynxModuleCallback>& callback,
      lepus::Value args) {
    auto delegate = delegate_.lock();
    if (!delegate || !IsActive()) {
      return false;
    }
    delegate->InvokeCallback(callback, [module = weak_from_this(), callback,
                                        args = std::move(args)]() mutable {
      auto owner = module.lock();
      if (!owner || !owner->IsActive()) {
        return false;
      }
      callback->SetArgs(std::make_unique<pub::ValueImplLepus>(std::move(args)));
      return true;
    });
    return true;
  }

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string& method_name, std::unique_ptr<pub::Value> args,
      size_t count, const runtime::CallbackMap& callbacks) override {
    const std::string invocation = name_ + "." + method_name;
    auto registration = registry_->GetModule(name_);
    if (!active_.load(std::memory_order_acquire) || !registration) {
      return base::unexpected(invocation + ": module has been detached.");
    }
    auto method = methods_.find(method_name);
    if (method == methods_.end()) {
      return base::unexpected(invocation + ": method is not registered.");
    }
    if (!args || !args->IsArray() ||
        static_cast<size_t>(args->Length()) != count ||
        count != method->second.args_count) {
      return base::unexpected(invocation + ": invalid argument count.");
    }
    // Both runtime adapters build this array with PubValueFactoryDefault.
    auto source = pub::ValueUtils::ConvertValueToLepusValue(*args);
    source.ToLepusValue();
    // source owns the converted values while C borrows their representations.
    std::vector<lynx_value> arguments;
    arguments.reserve(count);
    for (size_t index = 0; index < count; ++index) {
      const auto& argument = source.Array()->get(index);
      if (!IsPortableValue(argument)) {
        return base::unexpected(invocation + ": unsupported argument " +
                                std::to_string(index) + ".");
      }
      arguments.push_back(argument.value());
    }

    using CallbackHandle =
        std::unique_ptr<lynx_native_module_callback_t,
                        decltype(&lynx_native_module_callback_release)>;
    std::vector<CallbackHandle> handles;
    std::vector<lynx_capi_module_callback_t> descriptors;
    for (const auto& [index, callback] : callbacks) {
      if (static_cast<size_t>(index) >= count || !callback) {
        return base::unexpected(invocation + ": invalid callback descriptor.");
      }
      handles.emplace_back(
          new lynx_native_module_callback_t{
              .callback = callback,
              .module = weak_from_this(),
          },
          lynx_native_module_callback_release);
      descriptors.push_back({.argument_index = static_cast<size_t>(index),
                             .callback = handles.back().get()});
    }

    lynx_value raw_result{};
    raw_result.type = lynx_value_undefined;
    // Holding registration keeps opaque alive through concurrent Detach().
    const int32_t status = registration->invoke(
        registration->opaque, method_name.c_str(), arguments.data(), count,
        descriptors.data(), descriptors.size(), &raw_result);
    if (raw_result.type == lynx_value_extended) {
      return base::unexpected(invocation +
                              ": extended result has no environment.");
    }
    // Adopt the returned reference on both success and failure.
    lepus::Value result(nullptr, std::move(raw_result));
    if (status != LYNX_CAPI_MODULE_SUCCESS) {
      return base::unexpected(invocation + ": native invocation failed (" +
                              std::to_string(status) + ").");
    }
    return std::make_unique<pub::ValueImplLepus>(std::move(result));
  }

 private:
  const std::string name_;
  const std::shared_ptr<LynxCapiModuleRegistry> registry_;
  std::atomic<bool> active_{true};
};

LynxCapiModuleRegistration::~LynxCapiModuleRegistration() {
  if (finalizer) {
    finalizer(opaque);
  }
}

LynxCapiModuleRegistry::LynxCapiModuleRegistry(LynxCapiModuleMap modules)
    : modules_(std::move(modules)) {}

LynxCapiModuleRegistry::~LynxCapiModuleRegistry() { Detach(); }

std::shared_ptr<LynxCapiModuleRegistration> LynxCapiModuleRegistry::GetModule(
    const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto module = modules_.find(name);
  return detached_ || module == modules_.end() ? nullptr : module->second;
}

bool LynxCapiModuleRegistry::IsActive() {
  std::lock_guard<std::mutex> lock(mutex_);
  return !detached_;
}

void LynxCapiModuleRegistry::Detach() {
  LynxCapiModuleMap modules;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    detached_ = true;
    modules.swap(modules_);
  }
  // Registrations and their user finalizers are released outside the lock.
}

std::shared_ptr<runtime::LynxNativeModule> LynxCapiModuleFactory::CreateModule(
    const std::string& name) {
  auto registration = registry_->GetModule(name);
  if (!registration) {
    return nullptr;
  }
  return std::make_shared<LynxCapiModule>(name, registry_,
                                          registration->methods);
}

}  // namespace embedder
}  // namespace lynx

LYNX_EXTERN_C int32_t lynx_view_builder_register_capi_module(
    lynx_view_builder_t* builder, const char* name,
    const lynx_capi_module_method_t* methods, size_t method_count,
    lynx_capi_module_invoke invoke, void* opaque,
    lynx_capi_module_finalizer finalizer) {
  if (!builder || !name || !name[0] || !methods || !method_count || !invoke) {
    return LYNX_CAPI_MODULE_ERROR;
  }
  auto registration =
      std::make_shared<lynx::embedder::LynxCapiModuleRegistration>();
  for (size_t index = 0; index < method_count; ++index) {
    const auto& method = methods[index];
    if (!method.name || !method.name[0] ||
        method.argument_count >
            static_cast<size_t>(std::numeric_limits<int>::max()) ||
        !registration->methods
             .emplace(method.name, lynx::runtime::NativeModuleMethod(
                                       method.name, method.argument_count))
             .second) {
      return LYNX_CAPI_MODULE_ERROR;
    }
  }
  registration->invoke = invoke;
  registration->opaque = opaque;
  registration->finalizer = finalizer;
  builder->capi_modules.insert_or_assign(name, std::move(registration));
  return LYNX_CAPI_MODULE_SUCCESS;
}

LYNX_EXTERN_C void lynx_native_module_callback_retain(
    lynx_native_module_callback_t* callback) {
  if (callback) {
    callback->reference_count.fetch_add(1, std::memory_order_relaxed);
  }
}

LYNX_EXTERN_C void lynx_native_module_callback_release(
    lynx_native_module_callback_t* callback) {
  if (callback &&
      callback->reference_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
    delete callback;
  }
}

LYNX_EXTERN_C int lynx_native_module_callback_invoke(
    lynx_native_module_callback_t* callback, const lynx_value* arguments,
    size_t argument_count) {
  if (!callback || (!arguments && argument_count) ||
      argument_count > static_cast<size_t>(std::numeric_limits<int>::max())) {
    return 0;
  }
  auto module = callback->module.lock();
  if (!module || !module->IsActive()) {
    return 0;
  }
  auto array = lynx::lepus::CArray::Create();
  array->reserve(argument_count);
  for (size_t index = 0; index < argument_count; ++index) {
    // An extended value cannot be retained or accessed without its environment.
    if (arguments[index].type == lynx_value_extended) {
      return 0;
    }
    array->emplace_back(nullptr, arguments[index]);
  }
  if (callback->invoked.exchange(true, std::memory_order_acq_rel)) {
    return 0;
  }
  return module->InvokeCallback(callback->callback,
                                lynx::lepus::Value(std::move(array)));
}
