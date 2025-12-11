// Inspired by S.js by Adam Haile, https://github.com/adamhaile/S
/**
The MIT License (MIT)

Copyright (c) 2017 Adam Haile

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/signal/signal_context_unittest.h"

#include "core/renderer/signal/computation.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/runtime/vm/lepus/bytecode_generator.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/vm_context.h"

namespace lynx {
namespace tasm {
namespace test {

const std::tuple<bool> test_params[] = {
    std::make_tuple(true),   // ues lepus ng
    std::make_tuple(false),  // use lepus
};

BaseSignalTest::BaseSignalTest() {
  current_parameter_ = GetParam();
  enable_ng_ = std::get<0>(current_parameter_);

  static constexpr int32_t kWidth = 1024;
  static constexpr int32_t kHeight = 768;
  static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
  static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

  auto lynx_env_config =
      LynxEnvConfig(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                    kDefaultPhysicalPixelsPerLayoutUnit);
  delegate_ = std::make_unique<BaseSignalTestMockTasmDelegate>();

  auto manager =
      std::make_unique<ElementManager>(std::make_unique<MockPaintingContext>(),
                                       delegate_.get(), lynx_env_config);

  tasm_ = std::make_unique<lynx::tasm::TemplateAssembler>(
      *delegate_.get(), std::move(manager), delegate_.get(), 0);

  CompileOptions options;
  options.enable_fiber_arch_ = true;

  auto default_entry = std::make_shared<TemplateEntry>();

  default_entry->template_bundle_.compile_options_ = options;

  tasm_->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto test_entry = std::make_shared<TemplateEntry>();

  test_entry->template_bundle_.compile_options_ = options;

  tasm_->template_entries_.insert({"test_entry", test_entry});
}

void BaseSignalTest::SetUp() {
  if (enable_ng_) {
    ctx_ = std::make_shared<lepus::QuickContext>();
    ctx_->Initialize();
  } else {
    ctx_ = std::make_shared<lepus::VMContext>();
    ctx_->Initialize();
  }

  tasm_->template_entries_[DEFAULT_ENTRY_NAME]->SetVm(ctx_);
  tasm_->page_config_ = std::make_shared<PageConfig>();
  tasm_->SetPageConfigClient();

  if (enable_ng_) {
    auto qctx = static_cast<lepus::QuickContext*>(ctx_.get());
    Renderer::RegisterNGBuiltin(qctx, ArchOption::FIBER_ARCH);
  } else {
    auto vm_ctx = static_cast<lepus::VMContext*>(ctx_.get());
    Renderer::RegisterBuiltin(vm_ctx, ArchOption::FIBER_ARCH);
  }

  ctx_->SetGlobalData(
      "$kTemplateAssembler",
      lepus::Value(static_cast<lepus::Context::Delegate*>(tasm_.get())));
  tasm_->page_config_->enable_fiber_arch_ = true;
}

void BaseSignalTest::Compile(const std::string& source, lepus::Context* ctx) {
  if (ctx == nullptr) {
    ctx = ctx_.get();
  }

  lepus::BytecodeGenerator::GenerateBytecode(ctx, source, ctx->GetSdkVersion());
}

bool BaseSignalTest::Execute(lepus::Context* ctx) {
  if (ctx == nullptr) {
    ctx = ctx_.get();
  }

  if (ctx->IsLepusNGContext()) {
    auto qctx = static_cast<lepus::QuickContext*>(ctx);
    return qctx->Execute();
  } else {
    auto vm_ctx = static_cast<lepus::VMContext*>(ctx);
    vm_ctx->heap_ = lepus::Heap();
    return vm_ctx->Execute();
  }
  return false;
}

lepus::Value BaseSignalTest::GetTopLevelVariableByName(
    const std::string& name) {
  lepus::Value result;
  ctx_->GetTopLevelVariableByName(name, &result);
  return result;
}

SignalContextTest::SignalContextTest() : BaseSignalTest() {}

TEST_P(SignalContextTest, CreateAndReadASignal) {
  std::string js_source = R"(
       let signal = __CreateSignal(1);
       let value = __ReadSignal(signal);
  )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value value = GetTopLevelVariableByName("value");
  EXPECT_EQ(value.Number(), 1);
}

TEST_P(SignalContextTest, CreateAndReadAMemo0) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let value = "world";
    __CreateScope(()=>{
         let memo = __CreateMemo(() => "hello", "none");
         value = __ReadSignal(memo);
    });
  )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value value = GetTopLevelVariableByName("value");
  EXPECT_EQ(value.StdString(), "hello");
}

TEST_P(SignalContextTest, CreateAndReadAMemo1) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let value = "world";
    __CreateScope(()=>{
         let memo = __CreateMemo(i => `${i} lynx`, "hello");
         value = __ReadSignal(memo);
    });
  )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value value = GetTopLevelVariableByName("value");
  EXPECT_EQ(value.StdString(), "hello lynx");
}

TEST_P(SignalContextTest, CreateAndReadAMemo2) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let scope = null;
    let value0 = "world";
    let value1 = "world";
    __CreateScope((s)=>{
        scope = s;
        let memo = __CreateMemo(i => `${i} lynx`, "hello");
        value0 = __ReadSignal(memo);
  
        let signal = __CreateSignal("thoughts");
        const fn = (pre) => {
            let str = __ReadSignal(signal);
            value1 = `impure ${str}`
            return value1;
        };
        __CreateComputation(fn, "init", true)
    });
  )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value value0 = GetTopLevelVariableByName("value0");
  EXPECT_EQ(value0.StdString(), "hello lynx");

  lepus::Value value1 = GetTopLevelVariableByName("value1");
  EXPECT_EQ(value1.StdString(), "impure thoughts");

  auto scope_value = GetTopLevelVariableByName("scope");
  EXPECT_TRUE(scope_value.IsRefCounted());
  auto ref =
      fml::static_ref_ptr_cast<lepus::RefCounted>(scope_value.RefCounted());
  EXPECT_EQ(ref->GetRefType(), lepus::RefType::kScope);
  auto scope = fml::static_ref_ptr_cast<Scope>(scope_value.RefCounted());

  EXPECT_EQ(scope->owner_, nullptr);
  EXPECT_EQ(scope->owned_computation_.size(), 2);
  EXPECT_NE((*scope->owned_computation_.begin())->memo(), nullptr);
  EXPECT_EQ((*scope->owned_computation_.begin())->GetState(),
            ScopeState::kStateNone);

  EXPECT_EQ((*(++scope->owned_computation_.begin()))->memo(), nullptr);
  EXPECT_EQ((*(++scope->owned_computation_.begin()))->GetState(),
            ScopeState::kStateNone);

  tasm_->Destroy();
}

TEST_P(SignalContextTest, CreateAEffectWithExplicitDeps0) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let value = "world";
    __CreateScope(()=>{
        let signal = __CreateSignal("thoughts");
        const fn = (pre) => {
            let str = __ReadSignal(signal);
            value = `impure ${str}`
            return value;
        };
        __CreateComputation(fn, "init", true)
    });
  )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value value = GetTopLevelVariableByName("value");
  EXPECT_EQ(value.StdString(), "impure thoughts");
}

TEST_P(SignalContextTest, CreateAEffectWithExplicitDeps1) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let value = "world";
    __CreateScope(()=>{
        let signal = __CreateSignal("thoughts");
        const fn = (pre) => {
            let str = __ReadSignal(signal);
            value = `impure ${str}`
            return value;
        };
        __CreateComputation(fn, "init", false)
    });
  )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value value = GetTopLevelVariableByName("value");
  EXPECT_EQ(value.StdString(), "impure thoughts");
}

TEST_P(SignalContextTest, CreateAEffectWithExplicitDeps2) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let value = "world";
    __CreateScope(()=>{
        let signal = __CreateSignal("thoughts");
        const fn = (pre) => {
            let str = __ReadSignal(signal);
            value = `impure ${str}`
            return value;
        };
        __CreateComputation(fn, "init", false)
        __WriteSignal(signal, "lynx")
    });
  )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value value = GetTopLevelVariableByName("value");
  EXPECT_EQ(value.StdString(), "impure lynx");
}

TEST_P(SignalContextTest, CreateAEffectWithExplicitDeps3) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let value = "world";
    __CreateScope(()=>{
        __UnTrack(()=>{
            let signal = __CreateSignal("thoughts");
            const fn = (pre) => {
                 let str = __ReadSignal(signal);
                 value = `impure ${str}`
                 return value;
            };
            __CreateComputation(fn, "init", false);
            __WriteSignal(signal, "lynx");
        }
        );
    });
  )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value value = GetTopLevelVariableByName("value");
  EXPECT_EQ(value.StdString(), "impure lynx");
}

TEST_P(SignalContextTest, CreateAEffectWithExplicitDeps4) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let value = "world";
    __CreateScope(()=>{
        let signal = __CreateSignal("thoughts");
        const fn = (pre) => {
            return __UnTrack(()=>{
                let str = __ReadSignal(signal);
                value = `impure ${str}`
                return value;
            });
        };
        __CreateComputation(fn, "init", false)
        __WriteSignal(signal, "lynx")
    });
  )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value value = GetTopLevelVariableByName("value");
  EXPECT_EQ(value.StdString(), "impure thoughts");
}

TEST_P(SignalContextTest, TestRunUpdates0) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let scope = null;
    let count = 0;
    let value = "world";
    let signal_0 = __CreateSignal("thoughts");
    let signal_1 = __CreateSignal("thoughts");
    const fn = (pre) => {
        count = count + 1;
        let str_0 = __ReadSignal(signal_0);
        let str_1 = __ReadSignal(signal_1);
        value = `impure ${str_0} ${str_1}`
        return value;
    };
    __CreateScope((s)=>{
        scope = s;
        __CreateComputation(fn, "init", false);
    });
    __WriteSignal(signal_0, "lynx0");
    __WriteSignal(signal_1, "lynx1");
  )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value value = GetTopLevelVariableByName("value");
  EXPECT_EQ(value.StdString(), "impure lynx0 lynx1");

  lepus::Value count = GetTopLevelVariableByName("count");
  EXPECT_EQ(count.Number(), 3);

  tasm_->Destroy();
}

TEST_P(SignalContextTest, TestRunUpdates1) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let scope = null;
    let count = 0;
    let value = "world";
    let signal_0 = __CreateSignal("thoughts");
    let signal_1 = __CreateSignal("thoughts");
    const fn = (pre) => {
        count = count + 1;
        let str_0 = __ReadSignal(signal_0);
        let str_1 = __ReadSignal(signal_1);
        value = `impure ${str_0} ${str_1}`
        return value;
    };
    __CreateScope((s)=>{
      scope = s;
      __CreateComputation(fn, "init", false);
    })
    __RunUpdates(()=>{
        __WriteSignal(signal_0, "lynx0");
        __WriteSignal(signal_1, "lynx1");
    });
  )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value value = GetTopLevelVariableByName("value");
  EXPECT_EQ(value.StdString(), "impure lynx0 lynx1");

  lepus::Value count = GetTopLevelVariableByName("count");
  EXPECT_EQ(count.Number(), 2);

  tasm_->Destroy();
}

TEST_P(SignalContextTest, TestRunUpdates2) {
  if (!enable_ng_) {
    GTEST_SKIP();
  }

  std::string js_source = R"(
    let scope = null;
    let count = 0;
    let value = "world";
    let signal_0 = __CreateSignal("thoughts");
    let signal_1 = __CreateSignal("thoughts");
    const fn = (pre) => {
        count = count + 1;
        let str_0 = __ReadSignal(signal_0);
        let str_1 = __ReadSignal(signal_1);
        value = `impure ${str_0} ${str_1}`
        return value;
    };
    __CreateScope((s)=>{
      scope = s;
      __CreateComputation(fn, "init", false);
    })
    let str = __RunUpdates(()=>{
        __WriteSignal(signal_0, "lynx0");
        __WriteSignal(signal_1, "lynx1");
        return 'test';
    });
  )";

  Compile(js_source);
  EXPECT_TRUE(Execute());

  lepus::Value value = GetTopLevelVariableByName("value");
  EXPECT_EQ(value.StdString(), "impure lynx0 lynx1");

  lepus::Value count = GetTopLevelVariableByName("count");
  EXPECT_EQ(count.Number(), 2);

  lepus::Value str = GetTopLevelVariableByName("str");
  EXPECT_EQ(str.StdString(), "test");

  tasm_->Destroy();
}

INSTANTIATE_TEST_SUITE_P(SignalContextTestModule, SignalContextTest,
                         ::testing::ValuesIn(test_params));

}  // namespace test
}  // namespace tasm
}  // namespace lynx
