// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/style/data_ref.h"

#include "core/renderer/starlight/style/borders_data.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/starlight/style/layout_computed_style.h"
#include "core/renderer/starlight/style/layout_style_utils.h"
#include "core/renderer/starlight/types/layout_constraints.h"
#include "core/renderer/starlight/types/layout_unit.h"
#include "core/style/color.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace starlight {

class StyleData : public fml::RefCountedThreadSafeStorage {
 public:
  void ReleaseSelf() const override { delete this; }
  static fml::RefPtr<StyleData> Create() {
    return fml::AdoptRef(new StyleData());
  }
  fml::RefPtr<StyleData> Copy() const {
    return fml::AdoptRef(new StyleData(*this));
  }

  StyleData() : value(1) {}

  StyleData(const StyleData& data) : value(data.value) {}

  ~StyleData() override { destroyed_count_++; };

  bool operator==(const StyleData& rhs) const { return value == rhs.value; }
  bool operator!=(const StyleData& rhs) const { return !(*this == rhs); }

  int value = 0;
  // Debug lifecycle
  static int destroyed_count() { return destroyed_count_; }
  static void reset_destroyed() { destroyed_count_ = 0; }

 private:
  static int destroyed_count_;
};

int StyleData::destroyed_count_ = 0;

TEST(DataRefTest, CreateAndDestroy) {
  StyleData::reset_destroyed();
  {
    DataRef<StyleData> data;
    data.Init();
    EXPECT_EQ(StyleData::destroyed_count(), 0);
    EXPECT_TRUE(data.Get()->HasOneRef());
  }
  EXPECT_EQ(StyleData::destroyed_count(), 1);
}

TEST(DataRefTest, CopyOnWrite) {
  DataRef<StyleData> init_data;
  init_data.Init();
  EXPECT_TRUE(init_data->HasOneRef());
  // No copy constructor is called
  DataRef<StyleData> new_data = init_data;
  EXPECT_EQ(new_data->value, init_data->value);
  // Now has two ref count
  EXPECT_FALSE(new_data->HasOneRef());
  init_data.Access()->value = 100;
  // Does not affect the new data
  EXPECT_NE(new_data->value, init_data->value);
  new_data.Access()->value = 2;
  EXPECT_TRUE(new_data->HasOneRef());
  EXPECT_EQ(new_data->value, 2);
}

TEST(DataRefTest, RefCount) {
  StyleData::reset_destroyed();
  {
    DataRef<StyleData> init_data;
    init_data.Init();
    EXPECT_TRUE(init_data->HasOneRef());
    // Create the new ref locally
    {
      DataRef<StyleData> new_data = init_data;
      EXPECT_EQ(new_data->value, init_data->value);
      EXPECT_FALSE(new_data->HasOneRef());
      // The ref count should be 2
      EXPECT_EQ(init_data->SubtleRefCountForDebug(), 2);
    }
    EXPECT_EQ(StyleData::destroyed_count(), 0);
    EXPECT_TRUE(init_data->HasOneRef());
    // Create the new ref locally and write
    {
      DataRef<StyleData> new_data = init_data;
      EXPECT_EQ(new_data->value, init_data->value);
      EXPECT_FALSE(new_data->HasOneRef());
      // The ref count should equal 2
      EXPECT_EQ(init_data->SubtleRefCountForDebug(), 2);
      // Copy the data
      new_data.Access()->value = 2;
      EXPECT_TRUE(new_data->HasOneRef());
      EXPECT_EQ(new_data->value, 2);
    }
    EXPECT_EQ(StyleData::destroyed_count(), 1);
    // Still has one ref count
    EXPECT_TRUE(init_data->HasOneRef());
  }
  // The init data should be destroyed
  EXPECT_EQ(StyleData::destroyed_count(), 2);
}

TEST(DataRefTest, AssignmentMoveAndEqualityBranches) {
  DataRef<StyleData> first;
  first.Init();
  DataRef<StyleData> second;
  second.Init();

  EXPECT_EQ(first, second);
  second.Access()->value = 7;
  EXPECT_NE(first, second);

  first = second;
  EXPECT_EQ(first, second);
  EXPECT_FALSE(first->HasOneRef());

  DataRef<StyleData> moved(std::move(first));
  EXPECT_EQ(moved->value, 7);
  EXPECT_FALSE(moved->HasOneRef());

  DataRef<StyleData> assigned;
  assigned = StyleData::Create();
  assigned.Access()->value = 9;
  EXPECT_EQ(assigned->value, 9);
  assigned = nullptr;
  EXPECT_EQ(nullptr, assigned.Get());
}

TEST(DataRefTest, SharedAccessAndInequalityFalseBranches) {
  DataRef<StyleData> original;
  original.Init();
  DataRef<StyleData> shared = original;

  EXPECT_FALSE(shared->HasOneRef());
  EXPECT_EQ(original, shared);
  EXPECT_FALSE(original != shared);

  shared.Access()->value = 11;
  EXPECT_TRUE(shared->HasOneRef());
  EXPECT_TRUE(original->HasOneRef());
  EXPECT_NE(original, shared);
  EXPECT_FALSE(original == shared);

  DataRef<StyleData> same_value;
  same_value.Init();
  same_value.Access()->value = shared->value;
  EXPECT_NE(same_value.Get(), shared.Get());
  EXPECT_EQ(same_value, shared);
  EXPECT_FALSE(same_value != shared);
}

TEST(StyleDataCoverageTest, BordersDataResetAndEquality) {
  BordersData defaults;
  BordersData legacy_defaults(true);
  EXPECT_TRUE(defaults == BordersData());
  EXPECT_TRUE(legacy_defaults == BordersData(true));

  defaults.width_top = 12.f;
  defaults.width_right = 13.f;
  defaults.width_bottom = 14.f;
  defaults.width_left = 15.f;
  defaults.radius_x_top_left = NLength::MakeUnitNLength(1.f);
  defaults.radius_y_bottom_right = NLength::MakeUnitNLength(2.f);
  defaults.color_top = 0xff00ff00;
  defaults.color_left = 0xff0000ff;
  defaults.style_top = BorderStyleType::kDashed;
  defaults.style_left = BorderStyleType::kDotted;
  EXPECT_FALSE(defaults == BordersData());

  defaults.Reset();
  EXPECT_EQ(DefaultLayoutStyle::SL_DEFAULT_BORDER, defaults.width_top);
  EXPECT_EQ(DefaultLayoutStyle::SL_DEFAULT_BORDER_STYLE, defaults.style_top);
  EXPECT_EQ(DefaultColor::DEFAULT_COLOR, defaults.color_top);
  EXPECT_EQ(DefaultColor::DEFAULT_COLOR, defaults.color_left);
  EXPECT_FALSE(defaults == BordersData());

  legacy_defaults.width_top = 9.f;
  legacy_defaults.style_bottom = BorderStyleType::kDouble;
  legacy_defaults.Reset();
  const BordersData reset_legacy_reference(true);
  EXPECT_EQ(reset_legacy_reference.width_top, legacy_defaults.width_top);
  EXPECT_EQ(reset_legacy_reference.style_bottom, legacy_defaults.style_bottom);
  EXPECT_EQ(DefaultColor::DEFAULT_COLOR, legacy_defaults.color_bottom);
  EXPECT_FALSE(legacy_defaults == BordersData(true));
}

TEST(StyleDataCoverageTest, BordersDataFieldInequalityBranches) {
  const BordersData reference;
  BordersData changed;

  changed.width_top = 1.f;
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.width_right = 1.f;
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.width_bottom = 1.f;
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.width_left = 1.f;
  EXPECT_FALSE(reference == changed);

  changed = BordersData();
  changed.radius_x_top_left = NLength::MakeUnitNLength(1.f);
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.radius_x_top_right = NLength::MakeUnitNLength(1.f);
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.radius_x_bottom_right = NLength::MakeUnitNLength(1.f);
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.radius_x_bottom_left = NLength::MakeUnitNLength(1.f);
  EXPECT_FALSE(reference == changed);

  changed = BordersData();
  changed.radius_y_top_left = NLength::MakeUnitNLength(1.f);
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.radius_y_top_right = NLength::MakeUnitNLength(1.f);
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.radius_y_bottom_right = NLength::MakeUnitNLength(1.f);
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.radius_y_bottom_left = NLength::MakeUnitNLength(1.f);
  EXPECT_FALSE(reference == changed);

  changed = BordersData();
  changed.color_top = 0xff010203;
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.color_right = 0xff010203;
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.color_bottom = 0xff010203;
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.color_left = 0xff010203;
  EXPECT_FALSE(reference == changed);

  changed = BordersData();
  changed.style_top = BorderStyleType::kDashed;
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.style_right = BorderStyleType::kDashed;
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.style_bottom = BorderStyleType::kDashed;
  EXPECT_FALSE(reference == changed);
  changed = BordersData();
  changed.style_left = BorderStyleType::kDashed;
  EXPECT_FALSE(reference == changed);
}

TEST(StyleDataCoverageTest, LayoutStyleUtilsAndLayoutUnitBranches) {
  EXPECT_TRUE(
      LayoutStyleUtils::ListComponentTypeIsRow(ListComponentType::HEADER));
  EXPECT_TRUE(
      LayoutStyleUtils::ListComponentTypeIsRow(ListComponentType::FOOTER));
  EXPECT_TRUE(
      LayoutStyleUtils::ListComponentTypeIsRow(ListComponentType::LIST_ROW));
  EXPECT_FALSE(
      LayoutStyleUtils::ListComponentTypeIsRow(ListComponentType::DEFAULT));

  LayoutUnit indefinite;
  LayoutUnit also_indefinite = LayoutUnit::Indefinite();
  EXPECT_EQ(indefinite, also_indefinite);
  EXPECT_TRUE((indefinite + LayoutUnit(3.f)).IsIndefinite());
  EXPECT_TRUE((LayoutUnit(3.f) + indefinite).IsIndefinite());
  EXPECT_TRUE((indefinite - LayoutUnit(3.f)).IsIndefinite());
  EXPECT_TRUE((LayoutUnit(3.f) - indefinite).IsIndefinite());
  EXPECT_FLOAT_EQ(2.f, (LayoutUnit(5.f) - LayoutUnit(3.f)).ToFloat());
  EXPECT_TRUE((indefinite * 2.f).IsIndefinite());
  EXPECT_TRUE((2.f * indefinite).IsIndefinite());
  EXPECT_TRUE((LayoutUnit(4.f) / 0.f).IsIndefinite());
  EXPECT_FLOAT_EQ(2.f, (LayoutUnit(4.f) / 2.f).ToFloat());

  LayoutUnit assigned;
  assigned.AssignIfIndefinite(LayoutUnit(5.f));
  EXPECT_FLOAT_EQ(5.f, assigned.ToFloat());
  assigned.AssignIfIndefinite(LayoutUnit(7.f));
  EXPECT_FLOAT_EQ(5.f, assigned.ToFloat());

  assigned.Override(LayoutUnit::Indefinite());
  EXPECT_FLOAT_EQ(5.f, assigned.ToFloat());
  assigned.Override(LayoutUnit(7.f));
  EXPECT_FLOAT_EQ(7.f, assigned.ToFloat());

  EXPECT_FLOAT_EQ(
      3.f,
      LayoutUnit::LesserLayoutUnit(LayoutUnit(3.f), LayoutUnit(9.f)).ToFloat());
  EXPECT_FLOAT_EQ(
      9.f,
      LayoutUnit::LargerLayoutUnit(LayoutUnit(3.f), LayoutUnit(9.f)).ToFloat());
  EXPECT_FLOAT_EQ(
      3.f,
      LayoutUnit::LesserLayoutUnit(LayoutUnit(9.f), LayoutUnit(3.f)).ToFloat());
  EXPECT_FLOAT_EQ(
      9.f,
      LayoutUnit::LargerLayoutUnit(LayoutUnit(9.f), LayoutUnit(3.f)).ToFloat());
  EXPECT_FLOAT_EQ(6.f, LayoutUnit::ClampLayoutUnitWithMinMax(
                           LayoutUnit(8.f), LayoutUnit(2.f), LayoutUnit(6.f))
                           .ToFloat());
  EXPECT_TRUE(LayoutUnit::ClampLayoutUnitWithMinMax(
                  LayoutUnit::Indefinite(), LayoutUnit(2.f), LayoutUnit(6.f))
                  .IsIndefinite());
}

TEST(StyleDataCoverageTest, LayoutConstraintBranches) {
  EXPECT_EQ(OneSideConstraint::Indefinite(), OneSideConstraint::Indefinite());
  EXPECT_TRUE(
      OneSideConstraint::Indefinite().Near(OneSideConstraint::Indefinite()));
  EXPECT_FALSE(OneSideConstraint::Definite(10.f) ==
               OneSideConstraint::AtMost(10.f));
  EXPECT_TRUE(OneSideConstraint::Definite(10.f).Near(
      OneSideConstraint::Definite(10.f + 0.000001f)));
  EXPECT_FALSE(
      OneSideConstraint::Definite(10.f).Near(OneSideConstraint::AtMost(10.f)));
  EXPECT_FALSE(
      OneSideConstraint::Indefinite().Near(OneSideConstraint::Definite(10.f)));
  EXPECT_FALSE(OneSideConstraint::Definite(10.f).Near(
      OneSideConstraint::Definite(11.f)));

  EXPECT_TRUE(OneSideConstraint::Indefinite().ToPercentBase().IsIndefinite());
  EXPECT_FLOAT_EQ(10.f,
                  OneSideConstraint::Definite(10.f).ToPercentBase().ToFloat());

  OneSideConstraint constraint = OneSideConstraint::AtMost(20.f);
  constraint.ApplySize(LayoutUnit::Indefinite());
  EXPECT_EQ(SLMeasureModeAtMost, constraint.Mode());
  constraint.ApplySize(LayoutUnit(12.f));
  EXPECT_EQ(SLMeasureModeDefinite, constraint.Mode());
  EXPECT_FLOAT_EQ(12.f, constraint.Size());

  Constraints constraints{OneSideConstraint::Definite(30.f),
                          OneSideConstraint::AtMost(40.f)};
  CompactConstraints compact;
  compact = constraints;
  EXPECT_EQ(compact[kHorizontal], constraints[kHorizontal]);
  EXPECT_EQ(compact[kVertical], constraints[kVertical]);
  EXPECT_TRUE(compact == constraints);

  Constraints changed{OneSideConstraint::Definite(31.f),
                      OneSideConstraint::AtMost(40.f)};
  EXPECT_FALSE(compact == changed);

  Constraints changed_vertical{OneSideConstraint::Definite(30.f),
                               OneSideConstraint::AtMost(41.f)};
  EXPECT_FALSE(compact == changed_vertical);

  CompactConstraints default_compact;
  Constraints default_constraints;
  EXPECT_TRUE(default_compact == default_constraints);
}

TEST(StyleDataCoverageTest, LayoutUnitComparisonAndMinMaxBranches) {
  EXPECT_NE(LayoutUnit::Indefinite(), LayoutUnit(0.f));
  EXPECT_NE(LayoutUnit(4.f), LayoutUnit(5.f));
  EXPECT_EQ(LayoutUnit(4.f), LayoutUnit(4.f));
  EXPECT_TRUE(LayoutUnit::Indefinite() == LayoutUnit::Indefinite());
  EXPECT_FALSE(LayoutUnit(1.f) == LayoutUnit::Indefinite());
  EXPECT_FALSE(LayoutUnit::Indefinite() == LayoutUnit(1.f));

  EXPECT_TRUE((LayoutUnit::Indefinite() / 2.f).IsIndefinite());
  EXPECT_FLOAT_EQ(2.f, (LayoutUnit(5.f) - 3.f).ToFloat());
  EXPECT_FALSE(LayoutUnit(1.f).ClampIndefiniteToZero().IsIndefinite());

  EXPECT_FLOAT_EQ(3.f, LayoutUnit::LesserLayoutUnit(LayoutUnit::Indefinite(),
                                                    LayoutUnit(3.f))
                           .ToFloat());
  EXPECT_FLOAT_EQ(4.f, LayoutUnit::LesserLayoutUnit(LayoutUnit(4.f),
                                                    LayoutUnit::Indefinite())
                           .ToFloat());
  EXPECT_TRUE(LayoutUnit::LesserLayoutUnit(LayoutUnit::Indefinite(),
                                           LayoutUnit::Indefinite())
                  .IsIndefinite());

  EXPECT_FLOAT_EQ(9.f, LayoutUnit::LargerLayoutUnit(LayoutUnit::Indefinite(),
                                                    LayoutUnit(9.f))
                           .ToFloat());
  EXPECT_FLOAT_EQ(8.f, LayoutUnit::LargerLayoutUnit(LayoutUnit(8.f),
                                                    LayoutUnit::Indefinite())
                           .ToFloat());
  EXPECT_TRUE(LayoutUnit::LargerLayoutUnit(LayoutUnit::Indefinite(),
                                           LayoutUnit::Indefinite())
                  .IsIndefinite());
}

TEST(StyleDataCoverageTest, NLengthStringAndConversionBranches) {
  const NLength::BaseLength empty_base;
  EXPECT_FALSE(empty_base.HasValue());
  EXPECT_FALSE(empty_base.ContainsFixedValue());
  EXPECT_FALSE(empty_base.ContainsPercentage());
  EXPECT_EQ(empty_base, NLength::BaseLength());
  EXPECT_NE(empty_base, NLength::BaseLength(0.f));
  EXPECT_NE(NLength::BaseLength(1.f), NLength::BaseLength(1.f, 0.f));
  EXPECT_NE(NLength::BaseLength(1.f, 10.f), NLength::BaseLength(1.f, 20.f));

  const NLength::BaseLength zero_fixed(0.f);
  EXPECT_TRUE(zero_fixed.HasValue());
  EXPECT_TRUE(zero_fixed.ContainsFixedValue());
  EXPECT_FALSE(zero_fixed.ContainsPercentage());

  const NLength::BaseLength percent_only(0.f, 25.f);
  EXPECT_TRUE(percent_only.HasValue());
  EXPECT_FALSE(percent_only.ContainsFixedValue());
  EXPECT_TRUE(percent_only.ContainsPercentage());

  const NLength::BaseLength fixed_and_percent(4.f, 25.f);
  EXPECT_TRUE(fixed_and_percent.ContainsFixedValue());
  EXPECT_TRUE(fixed_and_percent.ContainsPercentage());
  EXPECT_NE(fixed_and_percent, NLength::BaseLength(4.f, 30.f));

  EXPECT_EQ("fit-content;",
            NLength::MakeFitContentNLength(NLength::BaseLength()).ToString());
  EXPECT_EQ("fit-content(25.000000%);",
            NLength::MakeFitContentNLength(NLength::BaseLength(0.f, 25.f))
                .ToString());
  EXPECT_EQ("calc(25.000000%);",
            NLength::MakeCalcNLength(0.f, 25.f).ToString());
  EXPECT_EQ("calc(12.000000unit);", NLength::MakeCalcNLength(12.f).ToString());
  EXPECT_EQ("calc(4.000000unit+50.000000%);",
            NLength::MakeCalcNLength(4.f, 50.f).ToString());

  EXPECT_TRUE(
      NLengthToLayoutUnit(NLength::MakeFitContentNLength(), LayoutUnit(100.f))
          .IsIndefinite());
  EXPECT_TRUE(NLengthToLayoutUnit(NLength::MakePercentageNLength(50.f),
                                  LayoutUnit::Indefinite())
                  .IsIndefinite());
  EXPECT_FLOAT_EQ(54.f, NLengthToLayoutUnit(NLength::MakeCalcNLength(4.f, 50.f),
                                            LayoutUnit(100.f))
                            .ToFloat());
  EXPECT_FALSE(NLength::MakeFitContentNLength(NLength::BaseLength(0.f))
                   .ContainsPercentage());
  EXPECT_NE(NLength::MakeCalcNLength(4.f, 50.f), NLength::MakeCalcNLength(4.f));
}

TEST(StyleDataCoverageTest, LayoutComputedStyleCopyOnWriteDataRefs) {
  LayoutComputedStyle original(1.f);
  LayoutComputedStyle shared(original);

  shared.box_data_.Access()->width_ = NLength::MakeUnitNLength(12.f);
  shared.flex_data_.Access()->flex_grow_ = 2.f;
  shared.grid_data_.Access()->grid_column_gap_ = NLength::MakeUnitNLength(3.f);
  shared.linear_data_.Access()->linear_gravity_ = LinearGravityType::kCenter;
  shared.relative_data_.Access()->relative_align_top_ = 7;

  EXPECT_NE(original.GetWidth(), shared.GetWidth());
  EXPECT_NE(original.GetFlexGrow(), shared.GetFlexGrow());
  EXPECT_NE(original.GetGridColumnGap(), shared.GetGridColumnGap());
  EXPECT_NE(original.GetLinearGravity(), shared.GetLinearGravity());
  EXPECT_NE(original.GetRelativeAlignTop(), shared.GetRelativeAlignTop());

  EXPECT_EQ(DefaultLayoutStyle::SL_DEFAULT_WIDTH(), original.GetWidth());
  EXPECT_EQ(DefaultLayoutStyle::SL_DEFAULT_FLEX_GROW, original.GetFlexGrow());
  EXPECT_EQ(DefaultLayoutStyle::SL_DEFAULT_GRID_GAP(),
            original.GetGridColumnGap());
  EXPECT_EQ(DefaultLayoutStyle::SL_DEFAULT_LINEAR_GRAVITY,
            original.GetLinearGravity());
  EXPECT_EQ(DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ID,
            original.GetRelativeAlignTop());
}

TEST(StyleDataCoverageTest, LayoutComputedStyleCopyFromNullDataRefs) {
  LayoutComputedStyle source(1.f);
  source.box_data_ = nullptr;
  source.flex_data_ = nullptr;
  source.grid_data_ = nullptr;
  source.linear_data_ = nullptr;
  source.relative_data_ = nullptr;

  LayoutComputedStyle target(2.f);
  target.CopyFrom(source);

  EXPECT_TRUE(target.GetWidth().IsAuto());
  EXPECT_FLOAT_EQ(DefaultLayoutStyle::SL_DEFAULT_FLEX_GROW,
                  target.GetFlexGrow());
  EXPECT_EQ(DefaultLayoutStyle::SL_DEFAULT_GRID_GAP(),
            target.GetGridColumnGap());
  EXPECT_EQ(DefaultLayoutStyle::SL_DEFAULT_LINEAR_GRAVITY,
            target.GetLinearGravity());
  EXPECT_EQ(DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ID, target.GetRelativeId());
  EXPECT_FLOAT_EQ(1.f, target.PhysicalPixelsPerLayoutUnit());
}

}  // namespace starlight
}  // namespace lynx
