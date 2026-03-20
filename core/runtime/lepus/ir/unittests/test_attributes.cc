// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "core/runtime/lepus/ir/attribute_kind.h"
#include "core/runtime/lepus/ir/unittests/ir_test_base.h"

namespace lynx {
namespace lepus {
namespace ir {

class LEPUSIRTestAttributes : public IRTestBase {
 public:
  virtual void SetUp(void) {
    IRTestBase::SetUp();
    ASSERT_NE(nullptr, ir_ctx->GetMainMod());
    ASSERT_NE(nullptr, ir_ctx->GetOpBuilder());
  }
  virtual void TearDown(void) {}
};

TEST_F(LEPUSIRTestAttributes, test_inst_attr_default_constructor) {
  ASSERT_EQ(10, static_cast<uint32_t>(SpecificAttr::SA_CNT));

  // 2. check the field
  Attributes attr;

  attr.RegisterAttr(SpecificAttr::SA_SmallJmp,
                    Attributes::GetBoolAttr(BoolAttrEntry, false));
  attr.RegisterAttr(SpecificAttr::SA_FixReg,
                    Attributes::GetBoolAttr(BoolAttrEntry, false));
  attr.RegisterAttr(SpecificAttr::SA_CallFuncMov,
                    Attributes::GetBoolAttr(BoolAttrEntry, false));
  attr.RegisterAttr(SpecificAttr::SA_DeepCloneCall,
                    Attributes::GetBoolAttr(BoolAttrEntry, false));
  attr.RegisterAttr(SpecificAttr::SA_ReadonlyCall,
                    Attributes::GetBoolAttr(BoolAttrEntry, false));

  ASSERT_EQ(false, attr.IsSmallJmp());
  ASSERT_EQ(false, attr.IsFixReg());
  ASSERT_EQ(false, attr.IsCallFuncMov());
  ASSERT_EQ(false, attr.IsDeepCloneCall());
  ASSERT_EQ(false, attr.IsReadonlyCall());

  attr.RegisterAttr(SpecificAttr::SA_ClosureVarReg,
                    Attributes::GetUnsignedAttr(UnsignedAttrEntry, 1));
  attr.RegisterAttr(SpecificAttr::SA_ChildrenIndex,
                    Attributes::GetUnsignedAttr(UnsignedAttrEntry, -1));
  attr.RegisterAttr(SpecificAttr::SA_ToplevelVarReg,
                    Attributes::GetUnsignedAttr(UnsignedAttrEntry, 2));

  ASSERT_EQ(1, attr.GetClosureVarReg());
  ASSERT_EQ(-1, attr.GetChildrenIndex());
  ASSERT_EQ(2, attr.GetToplevelVarReg());
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
