// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_TESTING_MOCK_NATIVE_FACADE_H_
#define CORE_SHELL_TESTING_MOCK_NATIVE_FACADE_H_

#include <any>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/synchronization/waitable_event.h"
#include "core/shell/native_facade_empty_implementation.h"

namespace lynx {

namespace base {
struct LynxError;
}  // namespace base

namespace shell {

struct MockNativeFacade : public NativeFacadeEmptyImpl {
  MockNativeFacade() = default;
  ~MockNativeFacade() override;

  MockNativeFacade(const MockNativeFacade& facade) = delete;
  MockNativeFacade& operator=(const MockNativeFacade&) = delete;

  MockNativeFacade(MockNativeFacade&& facade) = default;
  MockNativeFacade& operator=(MockNativeFacade&&) = default;

  void OnDataUpdated() override;

  void OnTemplateLoaded(const std::string& url) override;

  void OnSSRHydrateFinished(const std::string& url) override;

  void OnRuntimeReady() override;

  void OnTasmFinishByNative() override;

  void ReportError(const base::LynxError& error) override;

  void OnModuleMethodInvoked(const std::string& module,
                             const std::string& method, int32_t code) override;

  void OnConfigUpdated(const lepus::Value& data) override;

  void OnUpdateDataWithoutChange() override;

  std::shared_ptr<fml::AutoResetWaitableEvent> arwe =
      std::make_shared<fml::AutoResetWaitableEvent>();

  struct Result {
    bool on_correct_thread = false;
    std::unordered_map<std::string, std::any> bundle;

    std::any& operator[](const char* key) { return bundle[key]; }

    std::any& operator[](const std::string& key) { return bundle[key]; }

    operator bool() const { return on_correct_thread; }
  };

  std::any& operator[](const std::string& key) { return result[key]; }

  std::any& operator[](const char* key) { return result[key]; }

  operator bool() const { return result; }

  Result result;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_TESTING_MOCK_NATIVE_FACADE_H_
