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

#ifndef CORE_RENDERER_SIGNAL_SIGNAL_CONTEXT_UNITTEST_H_
#define CORE_RENDERER_SIGNAL_SIGNAL_CONTEXT_UNITTEST_H_

#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include "core/renderer/signal/signal_context.h"
#include "core/runtime/lepus/context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

class BaseSignalTestMockTasmDelegate
    : public ::testing::NiceMock<MockTasmDelegate> {
 public:
  BaseSignalTestMockTasmDelegate() : current_error_(0, "") {}
  virtual ~BaseSignalTestMockTasmDelegate() override = default;

  std::string DumpRenderFuncResult() { return render_func_ss_.str(); }

  void ClearRenderFuncResult() {
    render_func_ss_.str("");
    render_func_ss_.clear();
  }

  void OnErrorOccurred(base::LynxError error) override {
    current_error_ = std::move(error);
  }

  std::stringstream render_func_ss_;

  base::LynxError current_error_;
};

class BaseSignalTest : public ::testing::TestWithParam<std::tuple<bool>> {
 protected:
  BaseSignalTest();
  ~BaseSignalTest() override{};

  void SetUp() override;
  void TearDown() override {}

  void Compile(const std::string& code, lepus::Context* ctx = nullptr);
  bool Execute(lepus::Context* ctx = nullptr);
  lepus::Value GetTopLevelVariableByName(const std::string& name);

  std::unique_ptr<BaseSignalTestMockTasmDelegate> delegate_;
  std::shared_ptr<lynx::tasm::LayoutContext> layout_context_;
  std::unique_ptr<TemplateAssembler> tasm_;

  std::shared_ptr<lepus::Context> ctx_;

  bool enable_ng_;

  std::tuple<bool> current_parameter_;
};

class SignalContextTest : public BaseSignalTest {
 protected:
  SignalContextTest();
  ~SignalContextTest() override {}
};

}  // namespace test
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_SIGNAL_SIGNAL_CONTEXT_UNITTEST_H_
