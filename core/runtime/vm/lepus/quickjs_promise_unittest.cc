// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>
#include <vector>

#include "core/runtime/vm/lepus/bytecode_generator.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "quickjs/include/quickjs.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

class LepusPromiseMethods : public ::testing::Test {
 protected:
  LepusPromiseMethods() = default;
  ~LepusPromiseMethods() override = default;
};

TEST_F(LepusPromiseMethods, TestPromiseReject) {
  lepus::QuickContext qctx;
  std::string src = R"--(
    let res = 0;
    bar = ()=>{
      return new Promise((_, reject) => reject(res = 1)).
      then(() => { console.log('fulfill'); });
    }
    
    bar();
    
    function result() {
      return res;
    }
  )--";
  lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.0");
  qctx.Execute();
  LEPUSValue res = qctx.GetAndCall("result", nullptr, 0);
  LEPUSValue expected_res = LEPUS_MKVAL(LEPUS_TAG_INT, 1);
  EXPECT_EQ(LEPUS_VALUE_GET_TAG(res), LEPUS_VALUE_GET_TAG(expected_res));
  EXPECT_EQ(LEPUS_VALUE_GET_INT(res), LEPUS_VALUE_GET_INT(expected_res));
}

}  // namespace base
}  // namespace lynx
