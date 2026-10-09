// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/lynx_module_manager_testbench.h"

#include <utility>

#include "core/services/replay/lynx_module_fixture_replay.h"

namespace lynx {
namespace runtime {
namespace js {
ModuleManagerTestBench::ModuleManagerTestBench() {
  moduleMap = std::unordered_map<std::string, ModuleTestBenchPtr>();
}

void ModuleManagerTestBench::Destroy() {
  destroyed_ = true;
  for (auto &entry : moduleMap) entry.second->Destroy();
  moduleMap.clear();
  if (fixture_context_) fixture_context_->Destroy();
  fixture_context_.reset();
  if (release_fixture_) {
    Scope scope(*fixture_runtime_);
    release_fixture_->call(*fixture_runtime_);
    release_fixture_.reset();
  }
  fixture_runtime_ = nullptr;
}

void ModuleManagerTestBench::InitFixture(Runtime *rt) {
  // Platforms opt in through the replay data module's getFixtureDirectory().
  // Keep its assets alive until the optional releaseFixture() call in Destroy,
  // after all module handlers and their Fixture context have been released.
  auto module = bindingPtr->getLynxModuleManagerPtr()->get(
      rt, PropNameID::forAscii(*rt, "LynxRecorderReplayDataModule"));
  if (!module.isObject()) return;
  auto getter = module.getObject(*rt).getProperty(*rt, "getFixtureDirectory");
  if (!getter || !getter->isObject() || !getter->getObject(*rt).isFunction(*rt))
    return;
  auto directory = getter->getObject(*rt).getFunction(*rt).call(*rt);
  if (!directory || !directory->isString()) return;
  auto path = directory->getString(*rt).utf8(*rt);
  if (path.empty()) return;
  auto release = module.getObject(*rt).getProperty(*rt, "releaseFixture");
  if (release && release->isObject() &&
      release->getObject(*rt).isFunction(*rt)) {
    release_fixture_ =
        std::make_unique<Function>(release->getObject(*rt).getFunction(*rt));
    fixture_runtime_ = rt;
  }
  fixture_context_ = std::make_shared<tasm::replay::FixtureContext>();
  if (!fixture_context_->Initialize(path)) {
    LOGE("Failed to initialize fixture replay context");
  }
}

void ModuleManagerTestBench::initRecordModuleData(
    Runtime *rt, InitRecordModuleDataCallback callback) {
  InitFixture(rt);
  if (fixture_context_) return;
  PropNameID module_name =
      PropNameID::forAscii(*rt, "LynxRecorderReplayDataModule");
  Value module = bindingPtr->getLynxModuleManagerPtr()->get(rt, module_name);
  if (module.isNull()) {
    return;
  }
  auto getRecordData = module.getObject(*rt).getProperty(*rt, "getData");
  if (!getRecordData) {
    return;
  }

  Value inlineCallback = Function::createFromHostFunction(
      *rt, PropNameID::forAscii(*rt, "getData"), 1,
      [weak = weak_self_, callback](
          Runtime &rt, const Value &thisVal, const Value *args,
          size_t count) -> base::expected<Value, JSINativeException> {
        auto self = weak.lock();
        if (!self || self->destroyed_) return Value::undefined();
        if (count < 1 || !args[0].isString()) {
          return base::unexpected(
              BUILD_JSI_NATIVE_EXCEPTION("loadScript arg count must > 0"));
        }
        std::string data_str = args[0].getString(rt).utf8(rt);
        rapidjson::Document data;
        data.Parse(data_str.c_str());
        if (!data.IsObject() || !data.HasMember("RecordData") ||
            !data["RecordData"].IsString() || !data.HasMember("JsbSettings") ||
            !data["JsbSettings"].IsString() ||
            !data.HasMember("JsbIgnoredInfo") ||
            !data["JsbIgnoredInfo"].IsString())
          return Value::undefined();
        self->recordData.Parse(data["RecordData"].GetString());
        self->jsb_settings_.Parse(data["JsbSettings"].GetString());
        self->jsb_ignored_info_.Parse(data["JsbIgnoredInfo"].GetString());
        if (callback) {
          callback();
        }
        return Value::undefined();
      });
  getRecordData->getObject(*rt).getFunction(*rt).call(*rt, inlineCallback);
}

// init bindingptr, at the same time, get the bindingPtr(lynxPtr) from class
// ModuleManagerDarwin.
void ModuleManagerTestBench::initBindingPtr(
    std::weak_ptr<ModuleManagerTestBench> weak_manager,
    const std::shared_ptr<ModuleDelegate> &delegate,
    LynxJSIModuleBindingPtr lynxPtr) {
  weak_self_ = weak_manager;
  bindingPtr = std::make_shared<LynxJSIModuleBindingTestBench>(
      BindingFunc(weak_manager, delegate));
  // be used to call modules from Lynx SDK.
  bindingPtr->setLynxModuleManagerPtr(lynxPtr);
}

LynxModuleProviderFunction ModuleManagerTestBench::BindingFunc(
    std::weak_ptr<ModuleManagerTestBench> weak_manager,
    const std::shared_ptr<ModuleDelegate> &delegate) {
  return [weak_manager, delegate](const std::string &name) {
    auto manager = weak_manager.lock();
    if (manager && !manager->destroyed_) {
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
               const Value *args, size_t count) {
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
                                            Runtime *rt, const Value *args,
                                            size_t count) {
  PropNameID module_name =
      PropNameID::forAscii(*rt, sync_attrs["platformModule"].GetString());
  Value module = bindingPtr->getLynxModuleManagerPtr()->get(rt, module_name);
  if (module.isNull()) {
    return;
  }
  auto method = module.getObject(*rt).getProperty(
      *rt, sync_attrs["platformMethod"].GetString());
  if (!method->isObject()) {
    return;
  }

  auto args_moved_function = Array::createWithLength(*rt, count);

  for (size_t index = 0; index < count; index++) {
    if ((args + index)->kind() == Value::ValueKind::ObjectKind &&
        (args + index)->getObject(*rt).isFunction(*rt)) {
      args_moved_function->setValueAtIndex(
          *rt, index, String::createFromUtf8(*rt, "Function"));
    } else {
      args_moved_function->setValueAtIndex(*rt, index,
                                           Value(*rt, *(args + index)));
    }
  }

  Object params(*rt);

  params.setProperty(*rt, "args", Value(*args_moved_function));
  if (sync_attrs.HasMember("label")) {
    params.setProperty(
        *rt, "label",
        String::createFromUtf8(*rt, sync_attrs["label"].GetString()));
  } else {
    params.setProperty(*rt, "label", String::createFromUtf8(*rt, "default"));
  }

  method->getObject(*rt).getFunction(*rt).call(*rt, Value(params));
}

ModuleTestBenchPtr ModuleManagerTestBench::getModule(
    const std::string &name, const std::shared_ptr<ModuleDelegate> &delegate) {
  // step 1. try to get module from moduleMap
  auto p = moduleMap.find(name);
  if (p != moduleMap.end()) {
    return p->second;
  }
  if (fixture_context_) {
    auto module =
        std::make_shared<ModuleFixtureReplay>(name, delegate, fixture_context_);
    moduleMap.emplace(name, module);
    return module;
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
               const Value *args, size_t count) {
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

}  // namespace js

}  // namespace runtime
}  // namespace lynx
