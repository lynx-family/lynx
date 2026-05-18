// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/layout_object.h"

#include "core/renderer/starlight/style/layout_computed_style.h"
#include "core/renderer/starlight/types/nlength.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace starlight {
namespace {

void SetBoxSize(LayoutComputedStyle& style, float width, float height) {
  style.SetDisplay(DisplayType::kBox);
  style.SetWidth(NLength::MakeUnitNLength(width));
  style.SetHeight(NLength::MakeUnitNLength(height));
}

}  // namespace

TEST(LayoutObjectTests, BoxDoesNotPropagateMinConstraintsByDefault) {
  LayoutConfigs configs;
  LayoutComputedStyle root_style(1.0);
  LayoutComputedStyle child_style(1.0);
  SetBoxSize(root_style, 100.f, 80.f);

  LayoutObject child(configs, &child_style);
  LayoutObject root(configs, &root_style);
  root.AppendChild(&child);

  root.ReLayout();

  EXPECT_FLOAT_EQ(child.GetBorderBoundWidth(), 0.f);
  EXPECT_FLOAT_EQ(child.GetBorderBoundHeight(), 0.f);
}

TEST(LayoutObjectTests, BoxPropagatesOwnMinConstraintsToInFlowChildren) {
  LayoutConfigs configs;
  LayoutComputedStyle root_style(1.0);
  LayoutComputedStyle child_style(1.0);
  SetBoxSize(root_style, 100.f, 80.f);
  root_style.SetXPropagateMinConstraints(XPropagateMinConstraintsType::kSelf);

  LayoutObject child(configs, &child_style);
  LayoutObject root(configs, &root_style);
  root.AppendChild(&child);

  root.ReLayout();

  EXPECT_FLOAT_EQ(child.GetBorderBoundWidth(), 100.f);
  EXPECT_FLOAT_EQ(child.GetBorderBoundHeight(), 80.f);
}

}  // namespace starlight
}  // namespace lynx
