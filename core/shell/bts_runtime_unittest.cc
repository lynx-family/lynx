// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/shell/runtime/bts/bts_runtime.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/fml/message_loop.h"
#include "core/runtime/js/bindings/mock_module_delegate.h"
#include "core/runtime/js/bindings/modules/lynx_module_manager.h"
#include "core/runtime/js/jsi/jsi_unittest.h"
#include "core/runtime/js/mock_template_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace {

class RecordingTemplateDelegate : public runtime::test::MockTemplateDelegate {
 public:
  void OnEvaluateJavaScriptEnd(const std::string& url) override {
    last_evaluated_url_ = url;
  }

  std::string last_evaluated_url_;
};

class RecordingLoadCardApp : public piper::HostObject {
 public:
  piper::Value get(piper::Runtime* rt, const piper::PropNameID& name) override {
    if (rt == nullptr) {
      return piper::Value::undefined();
    }
    auto method_name = name.utf8(*rt);
    if (method_name != "loadCard") {
      return piper::Value::undefined();
    }

    return piper::Function::createFromHostFunction(
        *rt, piper::PropNameID::forAscii(*rt, "loadCard"), 1,
        [this](piper::Runtime& rt, const piper::Value& this_val,
               const piper::Value* args, size_t count) {
          (void)this_val;
          ++call_count_;
          last_arg_count_ = count;
          last_global_props_source_.clear();
          if (count >= 3 && args[2].isObject()) {
            auto lynx = args[2].getObject(rt);
            auto global_props = lynx.getProperty(rt, "__globalProps");
            if (global_props && global_props->isObject()) {
              auto global_props_object = global_props->getObject(rt);
              auto source = global_props_object.getProperty(rt, "source");
              if (source && source->isString()) {
                last_global_props_source_ = source->getString(rt).utf8(rt);
              }
            }
          }
          return piper::Value(true);
        });
  }

  void set(piper::Runtime*, const piper::PropNameID&,
           const piper::Value&) override {}

  std::vector<piper::PropNameID> getPropertyNames(piper::Runtime& rt) override {
    return {piper::PropNameID::forUtf8(rt, "loadCard")};
  }

  size_t call_count_{0};
  size_t last_arg_count_{0};
  std::string last_global_props_source_;
};

lepus::Value MakeGlobalProps(const char* source) {
  lepus::Value props(lepus::Dictionary::Create());
  props.SetProperty("source", lepus::Value(source));
  return props;
}

class BTSRuntimeTest : public piper::test::JSITestBase {
 public:
  BTSRuntimeTest() = default;

 protected:
  void SetUp() override {
    fml::MessageLoop::EnsureInitializedForCurrentThread();

    module_manager_ = std::make_shared<piper::LynxModuleManager>();
    module_delegate_ = std::make_shared<piper::MockModuleDelegate>(&rt);
    module_manager_->initBindingPtr(module_manager_, module_delegate_);

    auto native_module_proxy = piper::Object::createFromHostObject(
        *runtime, module_manager_->bindingPtr);

    auto delegate = std::make_unique<RecordingTemplateDelegate>();
    delegate_ = delegate.get();

    app_ = piper::App::Create(0, runtime, delegate_, exception_handler_,
                              std::move(native_module_proxy), nullptr, "-1",
                              tasm::PageOptions());

    js_app_ = std::make_shared<RecordingLoadCardApp>();
    app_->setJsAppObj(piper::Object::createFromHostObject(*runtime, js_app_));

    auto load_card_func = piper::Value(
        *runtime, js_app_
                      ->get(runtime.get(),
                            piper::PropNameID::forAscii(*runtime, "loadCard"))
                      .getObject(*runtime));
    runtime->global().setProperty(*runtime, "loadCard",
                                  std::move(load_card_func));

    runtime_under_test_ = std::make_unique<BTSRuntime>(
        "-1", 1, std::move(delegate), "", LynxRuntimeFlags::INIT,
        tasm::PageOptions());
    runtime_under_test_->app_ = app_;
    runtime_under_test_->state_ = BTSRuntime::State::kJsCoreLoaded;
  }

  std::shared_ptr<piper::App> app_;
  std::shared_ptr<RecordingLoadCardApp> js_app_;
  RecordingTemplateDelegate* delegate_{nullptr};
  std::shared_ptr<piper::LynxModuleManager> module_manager_;
  std::shared_ptr<piper::MockModuleDelegate> module_delegate_;
  std::unique_ptr<BTSRuntime> runtime_under_test_;
};

TEST_P(BTSRuntimeTest,
       EvaluateScriptStandaloneMarksAppLoadedForStandaloneRuntime) {
  std::unordered_map<std::string, runtime::js::JsContent> js_files;
  js_files.emplace(
      "standalone://entry.js",
      runtime::js::JsContent("globalThis.__lynx_ut = 1;",
                             runtime::js::JsContent::Type::SOURCE));

  EXPECT_FALSE(runtime_under_test_->js_app_loaded_);

  runtime_under_test_->EvaluateScriptStandalone(
      "standalone://entry.js", js_files, tasm::PackageInstanceDSL::STANDALONE,
      0);

  EXPECT_TRUE(runtime_under_test_->js_app_loaded_);
  EXPECT_EQ(js_app_->call_count_, 1u);
  EXPECT_EQ(js_app_->last_arg_count_, 3u);
  EXPECT_EQ(delegate_->last_evaluated_url_, "standalone://entry.js");
}

TEST_P(BTSRuntimeTest, EvaluateScriptStandaloneShouldNotLoadAppTwice) {
  std::unordered_map<std::string, runtime::js::JsContent> js_files;
  js_files.emplace(
      "standalone://entry.js",
      runtime::js::JsContent("globalThis.__lynx_ut = 1;",
                             runtime::js::JsContent::Type::SOURCE));

  runtime_under_test_->EvaluateScriptStandalone(
      "standalone://entry.js", js_files, tasm::PackageInstanceDSL::STANDALONE,
      0);
  EXPECT_EQ(js_app_->call_count_, 1u);

  runtime_under_test_->EvaluateScriptStandalone(
      "standalone://entry.js", js_files, tasm::PackageInstanceDSL::STANDALONE,
      1);

  EXPECT_EQ(js_app_->call_count_, 1u);
}

TEST_P(BTSRuntimeTest,
       OnGlobalPropsUpdatedBeforeStandaloneLoadUsesLatestGlobalProps) {
  std::unordered_map<std::string, runtime::js::JsContent> js_files;
  js_files.emplace(
      "standalone://entry.js",
      runtime::js::JsContent("globalThis.__lynx_ut = 1;",
                             runtime::js::JsContent::Type::SOURCE));

  runtime_under_test_->init_global_props_ = MakeGlobalProps("runtime");
  runtime_under_test_->OnGlobalPropsUpdated(MakeGlobalProps("lynx_view"));

  runtime_under_test_->EvaluateScriptStandalone(
      "standalone://entry.js", js_files, tasm::PackageInstanceDSL::STANDALONE,
      0);

  EXPECT_TRUE(runtime_under_test_->js_app_loaded_);
  EXPECT_EQ(js_app_->last_global_props_source_, "lynx_view");
}

TEST_P(BTSRuntimeTest,
       OnJSSourcePreparedDoesNotOverwriteLoadedStandaloneGlobalProps) {
  runtime_under_test_->js_app_loaded_ = true;
  runtime_under_test_->state_ = BTSRuntime::State::kRuntimeReady;
  runtime_under_test_->init_global_props_ = MakeGlobalProps("runtime");

  runtime_under_test_->OnJSSourcePrepared(
      tasm::TasmRuntimeBundle(), MakeGlobalProps("lynx_view"), "page",
      tasm::PackageInstanceDSL::STANDALONE,
      tasm::PackageInstanceBundleModuleMode::RETURN_BY_FUNCTION_MODE,
      "standalone://entry.js", nullptr, 0);

  auto source = runtime_under_test_->init_global_props_.GetProperty("source");
  EXPECT_TRUE(source.IsString());
  EXPECT_EQ(source.StdString(), "runtime");
}

INSTANTIATE_TEST_SUITE_P(BTSRuntimeTests, BTSRuntimeTest,
                         ::testing::ValuesIn(piper::test::runtimeGenerators()));

}  // namespace
}  // namespace shell
}  // namespace lynx
