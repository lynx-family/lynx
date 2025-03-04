// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/lynx_module_manager_testbench.h"

#include <utility>

namespace lynx {
namespace piper {

ModuleManagerTestBench::ModuleManagerTestBench() {
  moduleMap = std::unordered_map<std::string, ModuleTestBenchPtr>();
}

void ModuleManagerTestBench::Destroy() {}

void ModuleManagerTestBench::initRecordModuleData(
    Runtime *rt, InitRecordModuleDataCallback callback) {
  PropNameID module_name =
      PropNameID::forAscii(*rt, "TestBenchReplayDataModule");
  piper::Value module =
      bindingPtr->getLynxModuleManagerPtr()->get(rt, module_name);
  if (module.isNull()) {
    return;
  }
  auto getRecordData = module.getObject(*rt).getProperty(*rt, "getData");
  if (!getRecordData) {
    return;
  }

  piper::Value inlineCallback = piper::Function::createFromHostFunction(
      *rt, PropNameID::forAscii(*rt, "getData"), 1,
      [this, callback](
          Runtime &rt, const Value &thisVal, const Value *args,
          size_t count) -> base::expected<Value, JSINativeException> {
        if (count < 1) {
          return base::unexpected(
              BUILD_JSI_NATIVE_EXCEPTION("loadScript arg count must > 0"));
        }
        std::string data_str = args[0].getString(rt).utf8(rt);
        rapidjson::Document data;
        data.Parse(data_str.c_str());
        recordData.Parse(data["RecordData"].GetString());
        jsb_settings_.Parse(data["JsbSettings"].GetString());
        jsb_ignored_info_.Parse(data["JsbIgnoredInfo"].GetString());
        if (callback) {
          callback();
        }
        return piper::Value::undefined();
      });
  getRecordData->getObject(*rt).getFunction(*rt).call(*rt, inlineCallback);
}

// init bindingptr, at the same time, get the bindingPtr(lynxPtr) from class
// ModuleManagerDarwin.
void ModuleManagerTestBench::initBindingPtr(
    std::weak_ptr<ModuleManagerTestBench> weak_manager,
    const std::shared_ptr<ModuleDelegate> &delegate,
    LynxModuleBindingPtr lynxPtr) {
  bindingPtr = std::make_shared<lynx::piper::LynxModuleBindingTestBench>(
      BindingFunc(weak_manager, delegate));
  // be used to call modules from Lynx SDK.
  bindingPtr->setLynxModuleManagerPtr(lynxPtr);
}

LynxModuleProviderFunction ModuleManagerTestBench::BindingFunc(
    std::weak_ptr<ModuleManagerTestBench> weak_manager,
    const std::shared_ptr<ModuleDelegate> &delegate) {
  return [weak_manager, &delegate](const std::string &name) {
    auto manager = weak_manager.lock();
    if (manager) {
      auto ptr = manager->getModule(name, delegate);
      if (ptr.get() != nullptr) {
        return ptr;
      }
    }
    return ModuleTestBenchPtr(nullptr);
  };
}

void ModuleManagerTestBench::resetModuleRecordData(
    const std::string &module_name, InvokeMethodCallback callback) {
  auto p = moduleMap.find(module_name);
  if (p == moduleMap.end()) {
    return;
  }
  if (p->second->moduleData.IsNull()) {
    p->second->initModuleData(
        recordData[module_name.c_str()], &jsb_ignored_info_, &jsb_settings_,
        [&doc{recordData}](rapidjson::Value &dst, const rapidjson::Value &src) {
          dst = rapidjson::Value(src, doc.GetAllocator());
        },
        [this](const rapidjson::Value &sync_attrs, Runtime *rt,
               const piper::Value *args, size_t count) {
          this->syncToPlatform(sync_attrs, rt, args, count);
        });
  }

  callback();
}

void ModuleManagerTestBench::fetchRecordData(const std::string &module_name,
                                             Runtime &runtime,
                                             InvokeMethodCallback callback) {
  if (this->recordData.IsNull()) {
    initRecordModuleData(&runtime, [this, module_name, callback]() {
      this->resetModuleRecordData(module_name, callback);
    });

  } else {
    resetModuleRecordData(module_name, callback);
  }
}

void ModuleManagerTestBench::syncToPlatform(const rapidjson::Value &sync_attrs,
                                            Runtime *rt,
                                            const piper::Value *args,
                                            size_t count) {
  PropNameID module_name =
      PropNameID::forAscii(*rt, sync_attrs["platformModule"].GetString());
  piper::Value module =
      bindingPtr->getLynxModuleManagerPtr()->get(rt, module_name);
  if (module.isNull()) {
    return;
  }
  auto method = module.getObject(*rt).getProperty(
      *rt, sync_attrs["platformMethod"].GetString());
  if (!method->isObject()) {
    return;
  }

  auto args_moved_function = piper::Array::createWithLength(*rt, count);

  for (size_t index = 0; index < count; index++) {
    if ((args + index)->kind() == piper::Value::ValueKind::ObjectKind &&
        (args + index)->getObject(*rt).isFunction(*rt)) {
      args_moved_function->setValueAtIndex(
          *rt, index, piper::String::createFromUtf8(*rt, "Function"));
    } else {
      args_moved_function->setValueAtIndex(*rt, index,
                                           piper::Value(*rt, *(args + index)));
    }
  }

  piper::Object params(*rt);

  params.setProperty(*rt, "args", piper::Value(*args_moved_function));
  if (sync_attrs.HasMember("label")) {
    params.setProperty(
        *rt, "label",
        piper::String::createFromUtf8(*rt, sync_attrs["label"].GetString()));
  } else {
    params.setProperty(*rt, "label",
                       piper::String::createFromUtf8(*rt, "default"));
  }

  method->getObject(*rt).getFunction(*rt).call(*rt, piper::Value(params));
}

ModuleTestBenchPtr ModuleManagerTestBench::getModule(
    const std::string &name, const std::shared_ptr<ModuleDelegate> &delegate) {
  // step 1. try to get module from moduleMap
  auto p = moduleMap.find(name);
  if (p != moduleMap.end()) {
    return p->second;
  }
  // step 2. try to find correct module from recordData
  ModuleTestBenchPtr module = std::make_shared<ModuleTestBench>(name, delegate);
  if (!recordData.IsNull() && recordData.HasMember(name.c_str())) {
    module.get()->initModuleData(
        recordData[name.c_str()], &jsb_ignored_info_, &jsb_settings_,
        [&doc{recordData}](rapidjson::Value &dst, const rapidjson::Value &src) {
          dst = rapidjson::Value(src, doc.GetAllocator());
        },
        [this](const rapidjson::Value &sync_attrs, Runtime *rt,
               const piper::Value *args, size_t count) {
          this->syncToPlatform(sync_attrs, rt, args, count);
        });
  }
  module.get()->SetFetchDataHandler([this](const std::string &module_name,
                                           Runtime &runtime,
                                           InvokeMethodCallback callback) {
    this->fetchRecordData(module_name, runtime, callback);
  });
  module->SetModuleInterceptor(group_interceptor_);
  moduleMap.insert(std::pair<std::string, ModuleTestBenchPtr>(name, module));
  return module;
}

}  // namespace piper
}  // namespace lynx
