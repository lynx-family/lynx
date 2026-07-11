// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#![allow(clippy::field_reassign_with_default)]

use starlight_layout::{
    list_component_type_is_row, nlength_to_layout_unit, BaseLength, BorderStyle, BordersData,
    CompactConstraints, Constraints, LayoutComputedStyle, LayoutUnit, LinearGravity,
    ListComponentType, MeasureMode, NLength, SideConstraint, DEFAULT_BORDER, DEFAULT_COLOR,
    DEFAULT_FLEX_GROW, DEFAULT_GRID_GAP, DEFAULT_RELATIVE_ID,
};

fn assert_float_eq(actual: f32, expected: f32) {
    assert!(
        (actual - expected).abs() < 0.0001,
        "expected {expected}, got {actual}"
    );
}

#[test]
fn borders_data_reset_and_equality() {
    let mut defaults = BordersData::default();
    assert_eq!(defaults, BordersData::default());

    defaults.width_top = 12.0;
    defaults.width_right = 13.0;
    defaults.width_bottom = 14.0;
    defaults.width_left = 15.0;
    defaults.radius_x_top_left = NLength::unit(1.0);
    defaults.radius_y_bottom_right = NLength::unit(2.0);
    defaults.color_top = 0xff00_ff00;
    defaults.color_left = 0xff00_00ff;
    defaults.style_top = BorderStyle::Dashed;
    defaults.style_left = BorderStyle::Dotted;
    assert_ne!(defaults, BordersData::default());

    defaults.reset();
    assert_float_eq(defaults.width_top, DEFAULT_BORDER);
    assert_eq!(defaults.style_top, BorderStyle::Solid);
    assert_eq!(defaults.color_top, DEFAULT_COLOR);
    assert_eq!(defaults.color_left, DEFAULT_COLOR);
    assert_ne!(defaults, BordersData::default());
}

#[test]
fn borders_data_field_inequality_branches() {
    let reference = BordersData::default();
    let mut changed = BordersData::default();

    changed.width_top = 1.0;
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.width_right = 1.0;
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.width_bottom = 1.0;
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.width_left = 1.0;
    assert_ne!(reference, changed);

    changed = BordersData::default();
    changed.radius_x_top_left = NLength::unit(1.0);
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.radius_x_top_right = NLength::unit(1.0);
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.radius_x_bottom_right = NLength::unit(1.0);
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.radius_x_bottom_left = NLength::unit(1.0);
    assert_ne!(reference, changed);

    changed = BordersData::default();
    changed.radius_y_top_left = NLength::unit(1.0);
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.radius_y_top_right = NLength::unit(1.0);
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.radius_y_bottom_right = NLength::unit(1.0);
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.radius_y_bottom_left = NLength::unit(1.0);
    assert_ne!(reference, changed);

    changed = BordersData::default();
    changed.color_top = 0xff01_0203;
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.color_right = 0xff01_0203;
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.color_bottom = 0xff01_0203;
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.color_left = 0xff01_0203;
    assert_ne!(reference, changed);

    changed = BordersData::default();
    changed.style_top = BorderStyle::Dashed;
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.style_right = BorderStyle::Dashed;
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.style_bottom = BorderStyle::Dashed;
    assert_ne!(reference, changed);
    changed = BordersData::default();
    changed.style_left = BorderStyle::Dashed;
    assert_ne!(reference, changed);
}

#[test]
fn layout_style_utils_and_layout_unit_branches() {
    assert!(list_component_type_is_row(ListComponentType::Header));
    assert!(list_component_type_is_row(ListComponentType::Footer));
    assert!(list_component_type_is_row(ListComponentType::ListRow));
    assert!(!list_component_type_is_row(ListComponentType::Default));

    let indefinite = LayoutUnit::default();
    let also_indefinite = LayoutUnit::indefinite();
    assert_eq!(indefinite, also_indefinite);
    assert!((indefinite + LayoutUnit::new(3.0)).is_indefinite());
    assert!((LayoutUnit::new(3.0) + indefinite).is_indefinite());
    assert!((indefinite - LayoutUnit::new(3.0)).is_indefinite());
    assert!((LayoutUnit::new(3.0) - indefinite).is_indefinite());
    assert_float_eq(
        (LayoutUnit::new(5.0) - LayoutUnit::new(3.0)).to_float(),
        2.0,
    );
    assert!((indefinite * 2.0).is_indefinite());
    assert!((2.0 * indefinite).is_indefinite());
    assert!((LayoutUnit::new(4.0) / 0.0).is_indefinite());
    assert_float_eq((LayoutUnit::new(4.0) / 2.0).to_float(), 2.0);

    let mut assigned = LayoutUnit::default();
    assigned.assign_if_indefinite(LayoutUnit::new(5.0));
    assert_float_eq(assigned.to_float(), 5.0);
    assigned.assign_if_indefinite(LayoutUnit::new(7.0));
    assert_float_eq(assigned.to_float(), 5.0);

    assigned.override_with(LayoutUnit::indefinite());
    assert_float_eq(assigned.to_float(), 5.0);
    assigned.override_with(LayoutUnit::new(7.0));
    assert_float_eq(assigned.to_float(), 7.0);

    assert_float_eq(
        LayoutUnit::lesser(LayoutUnit::new(3.0), LayoutUnit::new(9.0)).to_float(),
        3.0,
    );
    assert_float_eq(
        LayoutUnit::larger(LayoutUnit::new(3.0), LayoutUnit::new(9.0)).to_float(),
        9.0,
    );
    assert_float_eq(
        LayoutUnit::lesser(LayoutUnit::new(9.0), LayoutUnit::new(3.0)).to_float(),
        3.0,
    );
    assert_float_eq(
        LayoutUnit::larger(LayoutUnit::new(9.0), LayoutUnit::new(3.0)).to_float(),
        9.0,
    );
    assert_float_eq(
        LayoutUnit::clamp_with_min_max(
            LayoutUnit::new(8.0),
            LayoutUnit::new(2.0),
            LayoutUnit::new(6.0),
        )
        .to_float(),
        6.0,
    );
    assert!(LayoutUnit::clamp_with_min_max(
        LayoutUnit::indefinite(),
        LayoutUnit::new(2.0),
        LayoutUnit::new(6.0),
    )
    .is_indefinite());
}

#[test]
fn layout_constraint_branches() {
    assert_eq!(SideConstraint::indefinite(), SideConstraint::indefinite());
    assert!(SideConstraint::indefinite().near(SideConstraint::indefinite()));
    assert_ne!(
        SideConstraint::definite(10.0),
        SideConstraint::at_most(10.0)
    );
    assert!(SideConstraint::definite(10.0).near(SideConstraint::definite(10.000_001)));
    assert!(!SideConstraint::definite(10.0).near(SideConstraint::at_most(10.0)));
    assert!(!SideConstraint::indefinite().near(SideConstraint::definite(10.0)));
    assert!(!SideConstraint::definite(10.0).near(SideConstraint::definite(11.0)));

    assert!(SideConstraint::indefinite()
        .to_percent_base()
        .is_indefinite());
    assert_float_eq(
        SideConstraint::definite(10.0).to_percent_base().to_float(),
        10.0,
    );

    let mut constraint = SideConstraint::at_most(20.0);
    constraint.apply_size(LayoutUnit::indefinite());
    assert_eq!(MeasureMode::AtMost, constraint.mode);
    constraint.apply_size(LayoutUnit::new(12.0));
    assert_eq!(MeasureMode::Definite, constraint.mode);
    assert_float_eq(12.0, constraint.size);

    let constraints = Constraints::new(
        SideConstraint::definite(30.0),
        SideConstraint::at_most(40.0),
    );
    let compact = CompactConstraints::from(constraints);
    assert_eq!(compact.width(), constraints.width);
    assert_eq!(compact.height(), constraints.height);
    assert!(compact.equals_constraints(constraints));

    let changed = Constraints::new(
        SideConstraint::definite(31.0),
        SideConstraint::at_most(40.0),
    );
    assert!(!compact.equals_constraints(changed));

    let changed_vertical = Constraints::new(
        SideConstraint::definite(30.0),
        SideConstraint::at_most(41.0),
    );
    assert!(!compact.equals_constraints(changed_vertical));

    let default_compact = CompactConstraints::default();
    let default_constraints = Constraints::default();
    assert!(default_compact.equals_constraints(default_constraints));
}

#[test]
fn layout_unit_comparison_and_min_max_branches() {
    assert_ne!(LayoutUnit::indefinite(), LayoutUnit::new(0.0));
    assert_ne!(LayoutUnit::new(4.0), LayoutUnit::new(5.0));
    assert_eq!(LayoutUnit::new(4.0), LayoutUnit::new(4.0));
    assert_eq!(LayoutUnit::indefinite(), LayoutUnit::indefinite());
    assert_ne!(LayoutUnit::new(1.0), LayoutUnit::indefinite());
    assert_ne!(LayoutUnit::indefinite(), LayoutUnit::new(1.0));

    assert!((LayoutUnit::indefinite() / 2.0).is_indefinite());
    assert_float_eq((LayoutUnit::new(5.0) - 3.0).to_float(), 2.0);
    assert!(!LayoutUnit::new(1.0)
        .clamp_indefinite_to_zero()
        .is_indefinite());

    assert_float_eq(
        LayoutUnit::lesser(LayoutUnit::indefinite(), LayoutUnit::new(3.0)).to_float(),
        3.0,
    );
    assert_float_eq(
        LayoutUnit::lesser(LayoutUnit::new(4.0), LayoutUnit::indefinite()).to_float(),
        4.0,
    );
    assert!(LayoutUnit::lesser(LayoutUnit::indefinite(), LayoutUnit::indefinite()).is_indefinite());

    assert_float_eq(
        LayoutUnit::larger(LayoutUnit::indefinite(), LayoutUnit::new(9.0)).to_float(),
        9.0,
    );
    assert_float_eq(
        LayoutUnit::larger(LayoutUnit::new(8.0), LayoutUnit::indefinite()).to_float(),
        8.0,
    );
    assert!(LayoutUnit::larger(LayoutUnit::indefinite(), LayoutUnit::indefinite()).is_indefinite());
}

#[test]
fn nlength_string_and_conversion_branches() {
    let empty_base = BaseLength::empty();
    assert!(!empty_base.has_value());
    assert!(!empty_base.contains_fixed_value());
    assert!(!empty_base.contains_percentage());
    assert_eq!(empty_base, BaseLength::empty());
    assert_ne!(empty_base, BaseLength::fixed(0.0));
    assert_ne!(
        BaseLength::fixed(1.0),
        BaseLength::fixed_and_percent(1.0, 0.0)
    );
    assert_ne!(
        BaseLength::fixed_and_percent(1.0, 10.0),
        BaseLength::fixed_and_percent(1.0, 20.0)
    );

    let zero_fixed = BaseLength::fixed(0.0);
    assert!(zero_fixed.has_value());
    assert!(zero_fixed.contains_fixed_value());
    assert!(!zero_fixed.contains_percentage());

    let percent_only = BaseLength::fixed_and_percent(0.0, 25.0);
    assert!(percent_only.has_value());
    assert!(!percent_only.contains_fixed_value());
    assert!(percent_only.contains_percentage());

    let fixed_and_percent = BaseLength::fixed_and_percent(4.0, 25.0);
    assert!(fixed_and_percent.contains_fixed_value());
    assert!(fixed_and_percent.contains_percentage());
    assert_ne!(fixed_and_percent, BaseLength::fixed_and_percent(4.0, 30.0));

    assert_eq!(
        "fit-content;",
        NLength::fit_content_with(BaseLength::empty()).to_css_string()
    );
    assert_eq!(
        "fit-content(25.000000%);",
        NLength::fit_content_with(BaseLength::fixed_and_percent(0.0, 25.0)).to_css_string()
    );
    assert_eq!(
        "calc(25.000000%);",
        NLength::calc(0.0, 25.0).to_css_string()
    );
    assert_eq!(
        "calc(12.000000unit);",
        NLength::calc_fixed(12.0).to_css_string()
    );
    assert_eq!(
        "calc(4.000000unit+50.000000%);",
        NLength::calc(4.0, 50.0).to_css_string()
    );

    assert!(nlength_to_layout_unit(NLength::fit_content(), LayoutUnit::new(100.0)).is_indefinite());
    assert!(
        nlength_to_layout_unit(NLength::percentage(50.0), LayoutUnit::indefinite()).is_indefinite()
    );
    assert_float_eq(
        nlength_to_layout_unit(NLength::calc(4.0, 50.0), LayoutUnit::new(100.0)).to_float(),
        54.0,
    );
    assert!(!NLength::fit_content_with(BaseLength::fixed(0.0)).contains_percentage());
    assert_ne!(NLength::calc(4.0, 50.0), NLength::calc_fixed(4.0));
}

#[test]
fn layout_computed_style_copy_on_write_data_refs() {
    let original = LayoutComputedStyle::new(1.0);
    let mut shared = original.clone();

    shared.box_data.access().unwrap().width = NLength::unit(12.0);
    shared.flex_data.access().unwrap().flex_grow = 2.0;
    shared.grid_data.access().unwrap().grid_column_gap = NLength::unit(3.0);
    shared.linear_data.access().unwrap().linear_gravity = LinearGravity::Center;
    shared.relative_data.access().unwrap().relative_align_top = 7;

    assert_ne!(original.width(), shared.width());
    assert_ne!(original.flex_grow(), shared.flex_grow());
    assert_ne!(original.grid_column_gap(), shared.grid_column_gap());
    assert_ne!(original.linear_gravity(), shared.linear_gravity());
    assert_ne!(original.relative_align_top(), shared.relative_align_top());

    assert_eq!(NLength::auto(), original.width());
    assert_float_eq(DEFAULT_FLEX_GROW, original.flex_grow());
    assert_eq!(DEFAULT_GRID_GAP, original.grid_column_gap());
    assert_eq!(LinearGravity::None, original.linear_gravity());
    assert_eq!(DEFAULT_RELATIVE_ID, original.relative_align_top());
}

#[test]
fn layout_computed_style_copy_from_null_data_refs() {
    let mut source = LayoutComputedStyle::new(1.0);
    source.box_data.clear();
    source.flex_data.clear();
    source.grid_data.clear();
    source.linear_data.clear();
    source.relative_data.clear();

    let mut target = LayoutComputedStyle::new(2.0);
    target.copy_from(&source);

    assert!(target.width().is_auto());
    assert_float_eq(DEFAULT_FLEX_GROW, target.flex_grow());
    assert_eq!(DEFAULT_GRID_GAP, target.grid_column_gap());
    assert_eq!(LinearGravity::None, target.linear_gravity());
    assert_eq!(DEFAULT_RELATIVE_ID, target.relative_id());
    assert_float_eq(1.0, target.physical_pixels_per_layout_unit());
}
