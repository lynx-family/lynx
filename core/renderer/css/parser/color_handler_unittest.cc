// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/color_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(ColorHandler, Process) {
  auto impl = lepus::Value("not color");
  CSSPropertyID id = CSSPropertyID::kPropertyIDBackgroundColor;
  StyleMap output;
  CSSParserConfigs configs;
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));

  // string color
  impl = lepus::Value("red");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output[id].IsNumber());
  EXPECT_TRUE(output[id].GetValue().IsUInt32());
  EXPECT_EQ(output[id].GetValue().UInt32(), 0xffff0000);

  output.clear();
  impl = lepus::Value("#00ff00");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_EQ(output[id].GetValue().UInt32(), 0xff00ff00);

  output.clear();
  impl = lepus::Value("#056b");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_EQ(output[id].GetValue().UInt32(), 0xbb005566);

  output.clear();
  impl = lepus::Value("#090a");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_EQ(output[id].GetValue().UInt32(), 0xaa009900);

  output.clear();
  impl = lepus::Value("#ghff00");
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));

  output.clear();
  impl = lepus::Value("#00ff00ee");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_EQ(output[id].GetValue().UInt32(), 0xee00ff00);

  output.clear();
  impl = lepus::Value("rgb(0,0,255)");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_EQ(output[id].GetValue().UInt32(), 0xff0000ff);

  output.clear();
  impl = lepus::Value("rgb(2000,-1,255)");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_EQ(output[id].GetValue().UInt32(), 0xffff00ff);

  output.clear();
  impl = lepus::Value("rgb(0, 0, 255)");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_EQ(output[id].GetValue().UInt32(), 0xff0000ff);

  output.clear();
  impl = lepus::Value("rgba(0, 0, 255, 100)");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_EQ(output[id].GetValue().UInt32(), 0xff0000ff);

  output.clear();
  impl = lepus::Value("hsl(240, 100%, 50%)");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_EQ(output[id].GetValue().UInt32(), 0xff0000ff);

  output.clear();
  impl = lepus::Value("hsla(240, 100%, 50%, 0.3)");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_EQ(output[id].GetValue().UInt32(), 0x4c0000ff);

  output.clear();
  impl = lepus::Value("#unknow");
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("Red");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_EQ(output[id].GetValue().UInt32(), 0xffff0000);

  output.clear();
  impl = lepus::Value("rgb(50%, 0%, 100%)");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_EQ(output[id].GetValue().UInt32(), 0xff8000ff);

  output.clear();
  impl = lepus::Value("rgb(100%, 0%, 105%)");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_EQ(output[id].GetValue().UInt32(), 0xffff00ff);
}
}  // namespace test

}  // namespace tasm
}  // namespace lynx
