// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"

namespace lynx {
namespace lepus {
namespace ir {

TEST(LEPUSIRRegisterAllocatorTest, RegisterFileTest) {
  RegisterFile file;

  // We are starting with an empty register file.
  EXPECT_EQ(file.GetNumLiveRegisters(), 0u);
  EXPECT_EQ(file.GetMaxRegisterUsage(), 0u);

  // Allocate a few registers.
  Register r1 = file.AllocateRegister();
  Register r2 = file.AllocateRegister();
  Register r3 = file.AllocateRegister();
  // Make sure we know which registers are alive.
  EXPECT_EQ(file.GetMaxRegisterUsage(), 3u);
  EXPECT_TRUE(file.IsUsed(r1));
  EXPECT_TRUE(file.IsUsed(r2));
  EXPECT_TRUE(file.IsUsed(r3));

  EXPECT_FALSE(file.IsFree(r1));
  EXPECT_FALSE(file.IsFree(r2));
  EXPECT_FALSE(file.IsFree(r3));

  EXPECT_EQ(file.GetMaxRegisterUsage(), 3u);

  // Make sure we can kill registers and things keep working.
  file.KillRegister(r2);
  EXPECT_TRUE(file.IsUsed(r1));
  EXPECT_FALSE(file.IsUsed(r2));
  EXPECT_TRUE(file.IsUsed(r3));

  EXPECT_TRUE(file.IsFree(r2));

  // Make sure we can reuse the freed register.
  Register r4 = file.AllocateRegister();
  EXPECT_EQ(file.GetMaxRegisterUsage(), 3u);

  file.KillRegister(r1);
  file.KillRegister(r3);
  file.KillRegister(r4);

  // Make sure that all registers have been freed.
  EXPECT_EQ(file.GetNumLiveRegisters(), 0u);

  // Make sure we can allocate lots of registers and free them in some order.
  std::vector<Register> regs;
  for (int i = 0; i < 1000; i++) {
    regs.push_back(file.AllocateRegister());
  }
  for (auto& r : regs) {
    file.KillRegister(r);
  }
  // Make sure that all registers have been freed again.
  EXPECT_EQ(file.GetNumLiveRegisters(), 0u);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
