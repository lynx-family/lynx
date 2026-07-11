// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#![forbid(unsafe_code)]

use std::collections::BTreeSet;
use std::fs;
use std::path::Path;

use starlight_layout::{
    AlignContent, AlignItems, BaseLength, BoxSizing, Constraints, Direction, Display,
    FlexDirection, FlexWrap, GridAutoFlow, JustifyContent, JustifyItems, Length,
    LinearCrossGravity, LinearGravity, LinearLayoutGravity, LinearOrientation, ListComponentType,
    MeasureMode, PositionType, Rect, RelativeCenter, SideConstraint, Size, Style,
};
use starlight_standalone::{
    standalone_default_style, StandaloneConfig, StandaloneEdge, StandaloneGap, StandaloneTree,
    TreeError,
};

fn assert_close(actual: f32, expected: f32) {
    assert!(
        (actual - expected).abs() < 0.01,
        "expected {expected}, got {actual}"
    );
}

#[test]
fn standalone_tree_safe_style_api_covers_public_standalone_header_families() {
    let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
    let standalone_header = fs::read_to_string(
        manifest_dir.join("../../../../../include/starlight_standalone/starlight.h"),
    )
    .expect("standalone public header should be readable");
    let rust_source =
        fs::read_to_string(manifest_dir.join("src/lib.rs")).expect("standalone Rust source");

    let rust_methods = rust_standalone_tree_public_methods(&rust_source);

    let setter_families = standalone_header_style_setter_families(&standalone_header);
    let missing_setters = setter_families
        .difference(&rust_methods)
        .cloned()
        .collect::<Vec<_>>();
    assert!(
        missing_setters.is_empty(),
        "Rust standalone tree must expose safe setters for every public standalone style setter family: {}",
        missing_setters.join(", ")
    );

    let getter_families = standalone_header_style_getter_families(&standalone_header);
    let missing_getters = getter_families
        .difference(&rust_methods)
        .cloned()
        .collect::<Vec<_>>();
    assert!(
        missing_getters.is_empty(),
        "Rust standalone tree must expose safe getters for every public standalone style getter family: {}",
        missing_getters.join(", ")
    );
}

#[test]
fn standalone_tree_safe_non_style_api_covers_public_standalone_header_families() {
    let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
    let standalone_header = fs::read_to_string(
        manifest_dir.join("../../../../../include/starlight_standalone/starlight.h"),
    )
    .expect("standalone public header should be readable");
    let config_header = fs::read_to_string(
        manifest_dir.join("../../../../../include/starlight_standalone/starlight_config.h"),
    )
    .expect("standalone public config header should be readable");
    let rust_source =
        fs::read_to_string(manifest_dir.join("src/lib.rs")).expect("standalone Rust source");

    let public_non_style_functions = standalone_header_function_names(&standalone_header)
        .into_iter()
        .chain(standalone_header_function_names(&config_header))
        .filter(|name| !name.starts_with("SLNodeStyle"))
        .collect::<BTreeSet<_>>();
    let mapped_functions = PUBLIC_NON_STYLE_API_METHOD_MAPPINGS
        .iter()
        .map(|(function, _)| (*function).to_owned())
        .collect::<BTreeSet<_>>();
    let exempted_functions = EXEMPTED_PUBLIC_NON_STYLE_API_FUNCTIONS
        .iter()
        .map(|(function, _)| (*function).to_owned())
        .collect::<BTreeSet<_>>();
    let covered_functions = mapped_functions
        .union(&exempted_functions)
        .cloned()
        .collect::<BTreeSet<_>>();

    let missing_functions = public_non_style_functions
        .difference(&covered_functions)
        .cloned()
        .collect::<Vec<_>>();
    assert!(
        missing_functions.is_empty(),
        "Rust standalone tree must cover every public standalone non-style API; missing mappings or exemptions: {}",
        missing_functions.join(", ")
    );

    let stale_mappings = mapped_functions
        .difference(&public_non_style_functions)
        .cloned()
        .collect::<Vec<_>>();
    assert!(
        stale_mappings.is_empty(),
        "Rust standalone non-style API mappings reference removed public C functions: {}",
        stale_mappings.join(", ")
    );

    let stale_exemptions = exempted_functions
        .difference(&public_non_style_functions)
        .cloned()
        .collect::<Vec<_>>();
    assert!(
        stale_exemptions.is_empty(),
        "Rust standalone non-style API exemptions reference removed public C functions: {}",
        stale_exemptions.join(", ")
    );

    for (function, reason) in EXEMPTED_PUBLIC_NON_STYLE_API_FUNCTIONS {
        assert!(
            !reason.trim().is_empty() && !reason.contains("TODO"),
            "{function} exemption must document the Rust ownership reason"
        );
    }

    let mut rust_methods = rust_standalone_tree_public_methods(&rust_source);
    rust_methods.extend(rust_standalone_config_public_methods(&rust_source));
    let mut missing_methods = Vec::new();
    for (function, methods) in PUBLIC_NON_STYLE_API_METHOD_MAPPINGS {
        for method in *methods {
            if !rust_methods.contains(*method) {
                missing_methods.push(format!("{function} -> {method}"));
            }
        }
    }
    assert!(
        missing_methods.is_empty(),
        "Rust standalone safe API is missing methods required by public C standalone non-style functions:\n{}",
        missing_methods.join("\n")
    );
}

#[test]
fn standalone_tree_layouts_owned_nodes_with_owner_constraints() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(40.0),
        height: Length::points(60.0),
        padding: Rect::all(Length::points(2.0)),
        ..standalone_default_style()
    });
    let child = tree.create_default_measured_node(Size::new(12.0, 8.0));
    tree.append_child(root, child).expect("append child");

    let size = tree
        .calculate_layout(
            root,
            Size {
                width: 40.0,
                height: 60.0,
            },
            Direction::Ltr,
        )
        .expect("layout root");

    assert_close(size.width, 44.0);
    assert_close(size.height, 64.0);
    let child_layout = tree.layout(child).expect("child layout");
    assert_close(child_layout.offset.x, 2.0);
    assert_close(child_layout.offset.y, 2.0);
    assert_close(child_layout.size.width, 12.0);
    assert_close(child_layout.size.height, 8.0);
    assert!(!tree.is_dirty(root).expect("root dirty state"));
    assert!(!tree.is_dirty(child).expect("child dirty state"));
}

#[test]
fn standalone_default_style_matches_public_standalone_defaults() {
    let style = standalone_default_style();

    assert_eq!(style.display, Display::Flex);
    assert_eq!(style.position, PositionType::Relative);
    assert_eq!(style.box_sizing, BoxSizing::ContentBox);
}

#[test]
fn standalone_tree_edge_style_setters_match_public_standalone_edges() {
    let mut tree = StandaloneTree::new();
    let node = tree.create_node(Style {
        direction: Direction::Rtl,
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..standalone_default_style()
    });
    tree.calculate_layout(
        node,
        Size {
            width: 20.0,
            height: 10.0,
        },
        Direction::Ltr,
    )
    .expect("layout node");
    assert!(!tree.is_dirty(node).expect("clean after layout"));

    tree.set_position(node, StandaloneEdge::Start, Length::points(6.0))
        .expect("set start position");
    tree.set_position(node, StandaloneEdge::End, Length::points(7.0))
        .expect("set end position");
    tree.set_margin(node, StandaloneEdge::Horizontal, Length::points(1.0))
        .expect("set horizontal margin");
    tree.set_margin(node, StandaloneEdge::Start, Length::points(2.0))
        .expect("set start margin");
    tree.set_padding(node, StandaloneEdge::Vertical, Length::percent(10.0))
        .expect("set vertical padding");
    tree.set_border(node, StandaloneEdge::All, 3.0)
        .expect("set all border");
    tree.set_border(node, StandaloneEdge::End, 4.0)
        .expect("set end border");

    let style = tree.style(node).expect("node style");
    assert_eq!(style.right, Length::points(6.0));
    assert_eq!(style.left, Length::points(7.0));
    assert_eq!(style.margin.left, Length::points(1.0));
    assert_eq!(style.margin.right, Length::points(2.0));
    assert_eq!(style.padding.top, Length::percent(10.0));
    assert_eq!(style.padding.bottom, Length::percent(10.0));
    assert_eq!(style.border.left, 4.0);
    assert_eq!(style.border.right, 3.0);
    assert_eq!(style.border.top, 3.0);
    assert_eq!(style.border.bottom, 3.0);
    assert_eq!(
        tree.style_position(node, StandaloneEdge::Start)
            .expect("start position"),
        Length::points(6.0)
    );
    assert_eq!(
        tree.style_position(node, StandaloneEdge::End)
            .expect("end position"),
        Length::points(7.0)
    );
    assert_eq!(
        tree.style_margin(node, StandaloneEdge::Start)
            .expect("start margin"),
        Length::points(2.0)
    );
    assert_eq!(
        tree.style_padding(node, StandaloneEdge::Top)
            .expect("top padding"),
        Length::percent(10.0)
    );
    assert_eq!(
        tree.style_border(node, StandaloneEdge::End)
            .expect("end border"),
        4.0
    );
    assert!(tree.is_dirty(node).expect("dirty after style setters"));
}

#[test]
fn standalone_tree_dimension_style_setters_match_public_standalone_lengths() {
    let mut tree = StandaloneTree::new();
    let node = tree.create_node(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..standalone_default_style()
    });
    tree.calculate_layout(
        node,
        Size {
            width: 20.0,
            height: 10.0,
        },
        Direction::Ltr,
    )
    .expect("layout node");
    assert!(!tree.is_dirty(node).expect("clean after layout"));

    tree.set_width(node, Length::points(24.0))
        .expect("set width");
    tree.set_height(node, Length::percent(50.0))
        .expect("set height");
    tree.set_min_width(node, Length::calc(3.0, 10.0))
        .expect("set min width");
    tree.set_min_height(node, Length::max_content())
        .expect("set min height");
    tree.set_max_width(node, Length::fit_content(None))
        .expect("set max width");
    tree.set_max_height(node, Length::points(80.0))
        .expect("set max height");
    tree.set_flex_basis(node, Length::fr(1.0))
        .expect("set flex basis");

    let style = tree.style(node).expect("node style");
    assert_eq!(style.width, Length::points(24.0));
    assert_eq!(style.height, Length::percent(50.0));
    assert_eq!(style.min_width, Length::calc(3.0, 10.0));
    assert_eq!(style.min_height, Length::max_content());
    assert_eq!(style.max_width, Length::fit_content(None));
    assert_eq!(style.max_height, Length::points(80.0));
    assert_eq!(style.flex_basis, Length::fr(1.0));
    assert_eq!(tree.style_width(node).expect("style width"), style.width);
    assert_eq!(tree.style_height(node).expect("style height"), style.height);
    assert_eq!(
        tree.style_min_width(node).expect("style min width"),
        style.min_width
    );
    assert_eq!(
        tree.style_min_height(node).expect("style min height"),
        style.min_height
    );
    assert_eq!(
        tree.style_max_width(node).expect("style max width"),
        style.max_width
    );
    assert_eq!(
        tree.style_max_height(node).expect("style max height"),
        style.max_height
    );
    assert_eq!(
        tree.style_flex_basis(node).expect("style flex basis"),
        style.flex_basis
    );
    assert!(tree.is_dirty(node).expect("dirty after dimension setters"));
}

#[test]
fn standalone_tree_gap_setter_and_getter_match_public_gap_edges() {
    let mut tree = StandaloneTree::new();
    let node = tree.create_node(standalone_default_style());

    tree.set_gap(node, StandaloneGap::All, Length::points(6.0))
        .expect("set all gap");
    assert_eq!(
        tree.style_row_gap(node).expect("row gap"),
        Length::points(6.0)
    );
    assert_eq!(
        tree.style_column_gap(node).expect("column gap"),
        Length::points(6.0)
    );
    assert_eq!(
        tree.style_gap(node, StandaloneGap::All)
            .expect("all gap returns row gap"),
        Length::points(6.0)
    );

    tree.set_gap(node, StandaloneGap::Column, Length::percent(12.0))
        .expect("set column gap");
    tree.set_gap(node, StandaloneGap::Row, Length::calc(1.0, 8.0))
        .expect("set row gap");
    assert_eq!(
        tree.style_gap(node, StandaloneGap::Column)
            .expect("column gap"),
        Length::percent(12.0)
    );
    assert_eq!(
        tree.style_gap(node, StandaloneGap::Row).expect("row gap"),
        Length::calc(1.0, 8.0)
    );
}

#[test]
fn standalone_tree_enum_scalar_and_vector_style_setters_update_style() {
    let mut tree = StandaloneTree::new();
    let node = tree.create_node(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..standalone_default_style()
    });
    tree.calculate_layout(
        node,
        Size {
            width: 20.0,
            height: 10.0,
        },
        Direction::Ltr,
    )
    .expect("layout node");
    assert!(!tree.is_dirty(node).expect("clean after layout"));

    tree.set_display(node, Display::Grid).expect("set display");
    tree.set_direction(node, Direction::Rtl)
        .expect("set direction");
    tree.set_position_type(node, PositionType::Absolute)
        .expect("set position type");
    tree.set_box_sizing(node, BoxSizing::ContentBox)
        .expect("set box sizing");
    tree.set_flex_direction(node, FlexDirection::ColumnReverse)
        .expect("set flex direction");
    tree.set_flex_wrap(node, FlexWrap::WrapReverse)
        .expect("set flex wrap");
    tree.set_justify_content(node, JustifyContent::SpaceBetween)
        .expect("set justify content");
    tree.set_align_content(node, AlignContent::SpaceAround)
        .expect("set align content");
    tree.set_align_items(node, AlignItems::Baseline)
        .expect("set align items");
    tree.set_align_self(node, Some(AlignItems::End))
        .expect("set align self");
    tree.set_justify_items(node, JustifyItems::Center)
        .expect("set justify items");
    tree.set_justify_self(node, JustifyItems::End)
        .expect("set justify self");
    tree.set_aspect_ratio(node, Some(1.5))
        .expect("set aspect ratio");
    tree.set_order(node, -2).expect("set order");
    tree.set_flex_grow(node, 1.25).expect("set flex grow");
    tree.set_flex_shrink(node, 0.5).expect("set flex shrink");
    tree.set_flex(node, 2.0).expect("set flex shorthand");
    tree.set_row_gap(node, Length::percent(10.0))
        .expect("set row gap");
    tree.set_column_gap(node, Length::calc(2.0, 5.0))
        .expect("set column gap");
    tree.set_linear_orientation(node, LinearOrientation::HorizontalReverse)
        .expect("set linear orientation");
    tree.set_linear_gravity(node, LinearGravity::SpaceBetween)
        .expect("set linear gravity");
    tree.set_linear_layout_gravity(node, LinearLayoutGravity::FillHorizontal)
        .expect("set linear layout gravity");
    tree.set_linear_cross_gravity(node, LinearCrossGravity::Stretch)
        .expect("set linear cross gravity");
    tree.set_linear_weight(node, 2.0)
        .expect("set linear weight");
    tree.set_linear_weight_sum(node, 5.0)
        .expect("set linear weight sum");
    tree.set_linear_column_count(node, Some(3))
        .expect("set linear column count");
    tree.set_list_main_axis_gap(node, Length::fr(1.0))
        .expect("set list main gap");
    tree.set_list_cross_axis_gap(node, Length::max_content())
        .expect("set list cross gap");
    tree.set_list_component_type(node, Some(ListComponentType::ListRow))
        .expect("set list component type");
    tree.set_grid_template_columns(node, vec![Length::points(8.0), Length::fr(1.0)])
        .expect("set grid template columns");
    tree.set_grid_template_rows(node, vec![Length::percent(50.0)])
        .expect("set grid template rows");
    tree.set_grid_template_columns_max(
        node,
        vec![Length::fit_content(Some(BaseLength::fixed(16.0)))],
    )
    .expect("set grid template columns max");
    tree.set_grid_template_rows_max(node, vec![Length::max_content()])
        .expect("set grid template rows max");
    tree.set_grid_auto_columns(node, vec![Length::points(12.0)])
        .expect("set grid auto columns");
    tree.set_grid_auto_rows(node, vec![Length::calc(1.0, 20.0)])
        .expect("set grid auto rows");
    tree.set_grid_auto_columns_max(node, vec![Length::fr(2.0)])
        .expect("set grid auto columns max");
    tree.set_grid_auto_rows_max(node, vec![Length::percent(25.0)])
        .expect("set grid auto rows max");
    tree.set_grid_auto_flow(node, GridAutoFlow::ColumnDense)
        .expect("set grid auto flow");
    tree.set_grid_column_start(node, Some(2))
        .expect("set grid column start");
    tree.set_grid_column_end(node, Some(4))
        .expect("set grid column end");
    tree.set_grid_row_start(node, Some(1))
        .expect("set grid row start");
    tree.set_grid_row_end(node, Some(3))
        .expect("set grid row end");
    tree.set_grid_column_span(node, 2)
        .expect("set grid column span");
    tree.set_grid_row_span(node, 3).expect("set grid row span");
    tree.set_relative_id(node, 7).expect("set relative id");
    tree.set_relative_align_top(node, 1)
        .expect("set relative align top");
    tree.set_relative_align_right(node, 2)
        .expect("set relative align right");
    tree.set_relative_align_bottom(node, 3)
        .expect("set relative align bottom");
    tree.set_relative_align_left(node, 4)
        .expect("set relative align left");
    tree.set_relative_top_of(node, 5)
        .expect("set relative top of");
    tree.set_relative_right_of(node, 6)
        .expect("set relative right of");
    tree.set_relative_bottom_of(node, 8)
        .expect("set relative bottom of");
    tree.set_relative_left_of(node, 9)
        .expect("set relative left of");
    tree.set_relative_layout_once(node, true)
        .expect("set relative layout once");
    tree.set_relative_center(node, RelativeCenter::Both)
        .expect("set relative center");

    let style = tree.style(node).expect("node style");
    assert_eq!(style.display, Display::Grid);
    assert_eq!(style.direction, Direction::Rtl);
    assert_eq!(style.position, PositionType::Absolute);
    assert_eq!(style.box_sizing, BoxSizing::ContentBox);
    assert_eq!(style.flex_direction, FlexDirection::ColumnReverse);
    assert_eq!(style.flex_wrap, FlexWrap::WrapReverse);
    assert_eq!(style.justify_content, JustifyContent::SpaceBetween);
    assert_eq!(style.align_content, AlignContent::SpaceAround);
    assert_eq!(style.align_items, AlignItems::Baseline);
    assert_eq!(style.align_self, Some(AlignItems::End));
    assert_eq!(style.justify_items, JustifyItems::Center);
    assert_eq!(style.justify_self, JustifyItems::End);
    assert_eq!(style.aspect_ratio, Some(1.5));
    assert_eq!(style.order, -2);
    assert_eq!(style.flex_grow, 2.0);
    assert_eq!(style.flex_shrink, 1.0);
    assert_eq!(style.flex_basis, Length::ZERO);
    assert_eq!(style.row_gap, Length::percent(10.0));
    assert_eq!(style.column_gap, Length::calc(2.0, 5.0));
    assert_eq!(
        style.linear_orientation,
        LinearOrientation::HorizontalReverse
    );
    assert_eq!(style.linear_gravity, LinearGravity::SpaceBetween);
    assert_eq!(
        style.linear_layout_gravity,
        LinearLayoutGravity::FillHorizontal
    );
    assert_eq!(style.linear_cross_gravity, LinearCrossGravity::Stretch);
    assert_eq!(style.linear_weight, 2.0);
    assert_eq!(style.linear_weight_sum, 5.0);
    assert_eq!(style.linear_column_count, Some(3));
    assert_eq!(style.list_main_axis_gap, Length::fr(1.0));
    assert_eq!(style.list_cross_axis_gap, Length::max_content());
    assert_eq!(style.list_component_type, Some(ListComponentType::ListRow));
    assert_eq!(
        style.grid_template_columns,
        vec![Length::points(8.0), Length::fr(1.0)]
    );
    assert_eq!(style.grid_template_rows, vec![Length::percent(50.0)]);
    assert_eq!(
        style.grid_template_columns_max,
        vec![Length::fit_content(Some(BaseLength::fixed(16.0)))]
    );
    assert_eq!(style.grid_template_rows_max, vec![Length::max_content()]);
    assert_eq!(style.grid_auto_columns, vec![Length::points(12.0)]);
    assert_eq!(style.grid_auto_rows, vec![Length::calc(1.0, 20.0)]);
    assert_eq!(style.grid_auto_columns_max, vec![Length::fr(2.0)]);
    assert_eq!(style.grid_auto_rows_max, vec![Length::percent(25.0)]);
    assert_eq!(style.grid_auto_flow, GridAutoFlow::ColumnDense);
    assert_eq!(style.grid_column_start, Some(2));
    assert_eq!(style.grid_column_end, Some(4));
    assert_eq!(style.grid_row_start, Some(1));
    assert_eq!(style.grid_row_end, Some(3));
    assert_eq!(style.grid_column_span, 2);
    assert_eq!(style.grid_row_span, 3);
    assert_eq!(style.relative_id, 7);
    assert_eq!(style.relative_align_top, 1);
    assert_eq!(style.relative_align_right, 2);
    assert_eq!(style.relative_align_bottom, 3);
    assert_eq!(style.relative_align_left, 4);
    assert_eq!(style.relative_top_of, 5);
    assert_eq!(style.relative_right_of, 6);
    assert_eq!(style.relative_bottom_of, 8);
    assert_eq!(style.relative_left_of, 9);
    assert!(style.relative_layout_once);
    assert_eq!(style.relative_center, RelativeCenter::Both);
    assert_eq!(tree.style_display(node).expect("display"), style.display);
    assert_eq!(
        tree.style_direction(node).expect("direction"),
        style.direction
    );
    assert_eq!(
        tree.style_position_type(node).expect("position type"),
        style.position
    );
    assert_eq!(
        tree.style_box_sizing(node).expect("box sizing"),
        style.box_sizing
    );
    assert_eq!(
        tree.style_flex_direction(node).expect("flex direction"),
        style.flex_direction
    );
    assert_eq!(
        tree.style_flex_wrap(node).expect("flex wrap"),
        style.flex_wrap
    );
    assert_eq!(
        tree.style_justify_content(node).expect("justify content"),
        style.justify_content
    );
    assert_eq!(
        tree.style_align_content(node).expect("align content"),
        style.align_content
    );
    assert_eq!(
        tree.style_align_items(node).expect("align items"),
        style.align_items
    );
    assert_eq!(
        tree.style_align_self(node).expect("align self"),
        style.align_self
    );
    assert_eq!(
        tree.style_justify_items(node).expect("justify items"),
        style.justify_items
    );
    assert_eq!(
        tree.style_justify_self(node).expect("justify self"),
        style.justify_self
    );
    assert_eq!(
        tree.style_aspect_ratio(node).expect("aspect ratio"),
        style.aspect_ratio
    );
    assert_eq!(tree.style_order(node).expect("order"), style.order);
    assert_eq!(
        tree.style_flex_grow(node).expect("flex grow"),
        style.flex_grow
    );
    assert_eq!(
        tree.style_flex_shrink(node).expect("flex shrink"),
        style.flex_shrink
    );
    assert_eq!(
        tree.style_flex_basis(node).expect("flex basis"),
        style.flex_basis
    );
    assert_eq!(tree.style_row_gap(node).expect("row gap"), style.row_gap);
    assert_eq!(
        tree.style_column_gap(node).expect("column gap"),
        style.column_gap
    );
    assert_eq!(
        tree.style_linear_orientation(node)
            .expect("linear orientation"),
        style.linear_orientation
    );
    assert_eq!(
        tree.style_linear_gravity(node).expect("linear gravity"),
        style.linear_gravity
    );
    assert_eq!(
        tree.style_linear_layout_gravity(node)
            .expect("linear layout gravity"),
        style.linear_layout_gravity
    );
    assert_eq!(
        tree.style_linear_cross_gravity(node)
            .expect("linear cross gravity"),
        style.linear_cross_gravity
    );
    assert_eq!(
        tree.style_linear_weight(node).expect("linear weight"),
        style.linear_weight
    );
    assert_eq!(
        tree.style_linear_weight_sum(node)
            .expect("linear weight sum"),
        style.linear_weight_sum
    );
    assert_eq!(
        tree.style_linear_column_count(node)
            .expect("linear column count"),
        style.linear_column_count
    );
    assert_eq!(
        tree.style_list_main_axis_gap(node).expect("list main gap"),
        style.list_main_axis_gap
    );
    assert_eq!(
        tree.style_list_cross_axis_gap(node)
            .expect("list cross gap"),
        style.list_cross_axis_gap
    );
    assert_eq!(
        tree.style_list_component_type(node)
            .expect("list component type"),
        style.list_component_type
    );
    assert_eq!(
        tree.style_grid_template_columns(node)
            .expect("grid template columns"),
        style.grid_template_columns.as_slice()
    );
    assert_eq!(
        tree.style_grid_template_rows(node)
            .expect("grid template rows"),
        style.grid_template_rows.as_slice()
    );
    assert_eq!(
        tree.style_grid_template_columns_max(node)
            .expect("grid template columns max"),
        style.grid_template_columns_max.as_slice()
    );
    assert_eq!(
        tree.style_grid_template_rows_max(node)
            .expect("grid template rows max"),
        style.grid_template_rows_max.as_slice()
    );
    assert_eq!(
        tree.style_grid_auto_columns(node)
            .expect("grid auto columns"),
        style.grid_auto_columns.as_slice()
    );
    assert_eq!(
        tree.style_grid_auto_rows(node).expect("grid auto rows"),
        style.grid_auto_rows.as_slice()
    );
    assert_eq!(
        tree.style_grid_auto_columns_max(node)
            .expect("grid auto columns max"),
        style.grid_auto_columns_max.as_slice()
    );
    assert_eq!(
        tree.style_grid_auto_rows_max(node)
            .expect("grid auto rows max"),
        style.grid_auto_rows_max.as_slice()
    );
    assert_eq!(
        tree.style_grid_auto_flow(node).expect("grid auto flow"),
        style.grid_auto_flow
    );
    assert_eq!(
        tree.style_grid_column_start(node)
            .expect("grid column start"),
        style.grid_column_start
    );
    assert_eq!(
        tree.style_grid_column_end(node).expect("grid column end"),
        style.grid_column_end
    );
    assert_eq!(
        tree.style_grid_row_start(node).expect("grid row start"),
        style.grid_row_start
    );
    assert_eq!(
        tree.style_grid_row_end(node).expect("grid row end"),
        style.grid_row_end
    );
    assert_eq!(
        tree.style_grid_column_span(node).expect("grid column span"),
        style.grid_column_span
    );
    assert_eq!(
        tree.style_grid_row_span(node).expect("grid row span"),
        style.grid_row_span
    );
    assert_eq!(
        tree.style_relative_id(node).expect("relative id"),
        style.relative_id
    );
    assert_eq!(
        tree.style_relative_align_top(node)
            .expect("relative align top"),
        style.relative_align_top
    );
    assert_eq!(
        tree.style_relative_align_right(node)
            .expect("relative align right"),
        style.relative_align_right
    );
    assert_eq!(
        tree.style_relative_align_bottom(node)
            .expect("relative align bottom"),
        style.relative_align_bottom
    );
    assert_eq!(
        tree.style_relative_align_left(node)
            .expect("relative align left"),
        style.relative_align_left
    );
    assert_eq!(
        tree.style_relative_top_of(node).expect("relative top of"),
        style.relative_top_of
    );
    assert_eq!(
        tree.style_relative_right_of(node)
            .expect("relative right of"),
        style.relative_right_of
    );
    assert_eq!(
        tree.style_relative_bottom_of(node)
            .expect("relative bottom of"),
        style.relative_bottom_of
    );
    assert_eq!(
        tree.style_relative_left_of(node).expect("relative left of"),
        style.relative_left_of
    );
    assert_eq!(
        tree.style_relative_layout_once(node)
            .expect("relative layout once"),
        style.relative_layout_once
    );
    assert_eq!(
        tree.style_relative_center(node).expect("relative center"),
        style.relative_center
    );
    assert!(tree.is_dirty(node).expect("dirty after style setters"));
}

#[test]
fn standalone_tree_measurement_api_tracks_dirty_state_and_baseline() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::Baseline,
        width: Length::points(80.0),
        height: Length::points(40.0),
        ..standalone_default_style()
    });
    let child = tree.create_default_node();
    tree.append_child(root, child).expect("append child");

    assert!(!tree.has_measure_func(child).expect("initial measure flag"));
    assert_eq!(
        tree.measured_size(child).expect("initial measured size"),
        None
    );
    assert_eq!(tree.baseline(child).expect("initial baseline"), None);

    tree.set_measured_size(child, Some(Size::new(10.0, 6.0)))
        .expect("set measured size");
    tree.set_baseline(child, Some(4.0)).expect("set baseline");
    assert!(tree.has_measure_func(child).expect("measure flag"));
    assert_eq!(
        tree.measured_size(child).expect("measured size"),
        Some(Size::new(10.0, 6.0))
    );
    assert_eq!(tree.baseline(child).expect("baseline"), Some(4.0));
    assert!(tree.is_dirty(child).expect("child dirty"));
    assert!(tree.is_dirty(root).expect("parent dirty"));

    tree.calculate_layout(root, Size::new(80.0, 40.0), Direction::Ltr)
        .expect("layout root");
    assert_close(tree.layout_width(child).expect("child width"), 10.0);
    assert_close(tree.layout_height(child).expect("child height"), 6.0);
    assert_close(tree.layout_baseline(child).expect("child baseline"), 4.0);
    assert!(!tree.is_dirty(root).expect("root clean"));
    assert!(!tree.is_dirty(child).expect("child clean"));

    tree.set_measured_size(child, None)
        .expect("clear measured size");
    tree.set_baseline(child, None).expect("clear baseline");
    assert!(!tree.has_measure_func(child).expect("cleared measure flag"));
    assert_eq!(
        tree.measured_size(child).expect("cleared measured size"),
        None
    );
    assert_eq!(tree.baseline(child).expect("cleared baseline"), None);
    assert!(tree.is_dirty(root).expect("parent dirty after clear"));
}

fn standalone_constraint_sensitive_measure(constraints: Constraints) -> Size {
    let width = match constraints.width.mode {
        MeasureMode::AtMost => constraints.width.size - 3.0,
        MeasureMode::Definite => constraints.width.size + 20.0,
        MeasureMode::Indefinite => 13.0,
    };
    let height = match constraints.height.mode {
        MeasureMode::AtMost => constraints.height.size - 5.0,
        MeasureMode::Definite => constraints.height.size + 30.0,
        MeasureMode::Indefinite => 11.0,
    };
    Size::new(width, height)
}

fn standalone_baseline_from_content_size(content_size: Size) -> f32 {
    content_size.height - 2.0
}

#[test]
fn standalone_tree_measure_func_receives_constraints_and_can_be_replaced() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();

    tree.set_measure_func(root, Some(standalone_constraint_sensitive_measure))
        .expect("set measure func");
    assert!(tree.has_measure_func(root).expect("measure flag"));
    assert!(tree.measure_func(root).expect("measure func").is_some());
    assert_eq!(tree.measured_size(root).expect("measured size"), None);

    let size = tree
        .calculate_layout_with_mode(
            root,
            Constraints::new(SideConstraint::at_most(30.0), SideConstraint::indefinite()),
            Direction::Ltr,
        )
        .expect("layout with callback measure");
    assert_close(size.width, 27.0);
    assert_close(size.height, 11.0);
    assert_close(tree.layout_width(root).expect("callback width"), 27.0);
    assert_close(tree.layout_height(root).expect("callback height"), 11.0);

    tree.set_measured_size(root, Some(Size::new(5.0, 4.0)))
        .expect("replace callback with static measured size");
    assert!(tree.measure_func(root).expect("measure func").is_none());
    assert_eq!(
        tree.measured_size(root).expect("static measured size"),
        Some(Size::new(5.0, 4.0))
    );
    tree.calculate_layout_with_mode(root, Constraints::indefinite(), Direction::Ltr)
        .expect("layout static measured size");
    assert_close(tree.layout_width(root).expect("static width"), 5.0);
    assert_close(tree.layout_height(root).expect("static height"), 4.0);

    tree.set_measure_func(root, None)
        .expect("clear measurement callback");
    assert!(!tree.has_measure_func(root).expect("measure flag"));
    assert!(tree.measure_func(root).expect("measure func").is_none());
    assert_eq!(tree.measured_size(root).expect("measured size"), None);
}

#[test]
fn standalone_tree_baseline_func_receives_content_size_and_can_be_replaced() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    tree.set_measure_func(root, Some(standalone_constraint_sensitive_measure))
        .expect("set measure func");

    tree.set_baseline_func(root, Some(standalone_baseline_from_content_size))
        .expect("set baseline func");
    assert_eq!(tree.baseline(root).expect("static baseline"), None);
    assert!(tree.baseline_func(root).expect("baseline func").is_some());

    tree.calculate_layout_with_mode(
        root,
        Constraints::new(SideConstraint::at_most(30.0), SideConstraint::indefinite()),
        Direction::Ltr,
    )
    .expect("layout with callback baseline");
    assert_close(tree.layout_width(root).expect("callback width"), 27.0);
    assert_close(tree.layout_height(root).expect("callback height"), 11.0);
    assert_close(tree.layout_baseline(root).expect("callback baseline"), 9.0);

    tree.set_baseline(root, Some(4.0))
        .expect("replace callback with static baseline");
    assert_eq!(tree.baseline(root).expect("static baseline"), Some(4.0));
    assert!(tree.baseline_func(root).expect("baseline func").is_none());
    tree.calculate_layout_with_mode(root, Constraints::indefinite(), Direction::Ltr)
        .expect("layout static baseline");
    assert_close(tree.layout_baseline(root).expect("static baseline"), 4.0);

    tree.set_baseline_func(root, None)
        .expect("clear baseline callback");
    assert_eq!(tree.baseline(root).expect("static baseline"), None);
    assert!(tree.baseline_func(root).expect("baseline func").is_none());
}

#[test]
fn standalone_tree_config_tracks_physical_pixels_per_layout_unit_and_survives_reset() {
    let mut tree = StandaloneTree::new();
    let mut config = StandaloneConfig::new();
    assert_close(config.physical_pixels_per_layout_unit(), 1.0);
    config.set_physical_pixels_per_layout_unit(2.0);

    let root = tree.create_default_node_with_config(config);
    assert_eq!(tree.node_config(root).expect("node config"), config);
    assert_close(
        tree.physical_pixels_per_layout_unit(root)
            .expect("physical pixels"),
        2.0,
    );

    tree.reset_node(root).expect("reset root");
    assert_eq!(tree.node_config(root).expect("node config"), config);
    assert_close(
        tree.physical_pixels_per_layout_unit(root)
            .expect("physical pixels after reset"),
        2.0,
    );
}

#[test]
fn standalone_tree_measured_layout_ceil_uses_node_physical_pixels_per_layout_unit() {
    let mut default_tree = StandaloneTree::new();
    let default_root = default_tree.create_default_node();
    default_tree
        .set_measured_size(default_root, Some(Size::new(10.2, 4.2)))
        .expect("set default measured size");
    default_tree
        .calculate_layout_with_mode(default_root, Constraints::indefinite(), Direction::Ltr)
        .expect("layout default root");
    assert_close(
        default_tree.layout_width(default_root).expect("width"),
        11.0,
    );
    assert_close(
        default_tree.layout_height(default_root).expect("height"),
        5.0,
    );

    let mut configured_tree = StandaloneTree::new();
    let configured_root = configured_tree.create_default_node_with_config(
        StandaloneConfig::with_physical_pixels_per_layout_unit(2.0),
    );
    configured_tree
        .set_measured_size(configured_root, Some(Size::new(10.2, 4.2)))
        .expect("set configured measured size");
    configured_tree
        .calculate_layout_with_mode(configured_root, Constraints::indefinite(), Direction::Ltr)
        .expect("layout configured root");
    assert_close(
        configured_tree
            .layout_width(configured_root)
            .expect("configured width"),
        10.5,
    );
    assert_close(
        configured_tree
            .layout_height(configured_root)
            .expect("configured height"),
        4.5,
    );
}

#[test]
fn standalone_tree_exposes_layout_getters_with_edge_resolution() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(80.0),
        height: Length::points(50.0),
        ..standalone_default_style()
    });
    let child = tree.create_measured_node(
        Style {
            direction: Direction::Rtl,
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::points(3.0),
                Length::points(4.0),
            ),
            padding: Rect::new(
                Length::points(5.0),
                Length::points(6.0),
                Length::points(7.0),
                Length::points(8.0),
            ),
            border: Rect::new(9.0, 10.0, 11.0, 12.0),
            ..standalone_default_style()
        },
        Size::new(10.0, 5.0),
    );
    tree.append_child(root, child).expect("append child");
    tree.calculate_layout(
        root,
        Size {
            width: 80.0,
            height: 50.0,
        },
        Direction::Ltr,
    )
    .expect("layout root");

    let child_layout = tree.layout(child).expect("child layout");
    assert_close(
        tree.layout_left(child).expect("layout left"),
        child_layout.offset.x,
    );
    assert_close(
        tree.layout_top(child).expect("layout top"),
        child_layout.offset.y,
    );
    assert_close(
        tree.layout_width(child).expect("layout width"),
        child_layout.size.width,
    );
    assert_close(
        tree.layout_height(child).expect("layout height"),
        child_layout.size.height,
    );
    assert_close(
        tree.layout_baseline(child).expect("baseline fallback"),
        child_layout.baseline.unwrap_or(child_layout.size.height),
    );
    assert_close(
        tree.layout_margin(child, StandaloneEdge::Left)
            .expect("left margin"),
        child_layout.margin.left,
    );
    assert_close(
        tree.layout_margin(child, StandaloneEdge::Start)
            .expect("start margin"),
        child_layout.margin.right,
    );
    assert_close(
        tree.layout_margin(child, StandaloneEdge::End)
            .expect("end margin"),
        child_layout.margin.left,
    );
    assert_close(
        tree.layout_padding(child, StandaloneEdge::Bottom)
            .expect("bottom padding"),
        child_layout.padding.bottom,
    );
    assert_close(
        tree.layout_border(child, StandaloneEdge::Right)
            .expect("right border"),
        child_layout.border.right,
    );
    assert_close(
        tree.layout_sticky_position(child, StandaloneEdge::Start)
            .expect("start sticky"),
        child_layout.sticky_pos.right,
    );
}

#[test]
fn standalone_tree_preserves_child_order_and_parent_links() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style::default());
    let first = tree.create_node(Style::default());
    let second = tree.create_node(Style::default());
    let inserted = tree.create_node(Style::default());

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.insert_child_before(root, inserted, second)
        .expect("insert before second");

    assert_eq!(
        tree.children(root).expect("root children"),
        [first, inserted, second]
    );
    assert_eq!(tree.parent(inserted).expect("inserted parent"), Some(root));

    tree.remove_child(root, inserted).expect("remove inserted");
    assert_eq!(tree.children(root).expect("root children"), [first, second]);
    assert_eq!(tree.parent(inserted).expect("inserted parent"), None);

    tree.remove_all_children(root).expect("remove all children");
    assert!(tree.children(root).expect("root children").is_empty());
    assert_eq!(tree.parent(first).expect("first parent"), None);
    assert_eq!(tree.parent(second).expect("second parent"), None);
}

#[test]
fn standalone_tree_remove_child_noops_for_unattached_child_like_public_standalone() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style::default());
    let child = tree.create_node(Style::default());
    let wrong_parent = tree.create_node(Style::default());
    let outsider = tree.create_node(Style::default());

    tree.append_child(root, child).expect("append child");
    tree.calculate_layout(root, Size::new(20.0, 10.0), Direction::Ltr)
        .expect("layout root");
    assert!(!tree.is_dirty(root).expect("root clean after layout"));

    tree.remove_child(root, outsider)
        .expect("removing an unattached child is a no-op");
    assert_eq!(tree.children(root).expect("root children"), [child]);
    assert_eq!(tree.parent(outsider).expect("outsider parent"), None);
    assert!(!tree.is_dirty(root).expect("no-op keeps root clean"));

    tree.remove_child(wrong_parent, child)
        .expect("removing from the wrong parent is a no-op");
    assert_eq!(tree.children(root).expect("root children"), [child]);
    assert_eq!(tree.parent(child).expect("child parent"), Some(root));
    assert!(tree
        .children(wrong_parent)
        .expect("wrong parent children")
        .is_empty());
    assert!(!tree
        .is_dirty(root)
        .expect("wrong-parent no-op keeps root clean"));
}

#[test]
fn standalone_tree_insert_child_or_append_matches_public_standalone_index_semantics() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style::default());
    let first = tree.create_node(Style::default());
    let second = tree.create_node(Style::default());
    let inserted = tree.create_node(Style::default());
    let clamped = tree.create_node(Style::default());
    let negative = tree.create_node(Style::default());
    let zero = tree.create_node(Style::default());

    tree.insert_child_or_append(root, first, None)
        .expect("none appends first child");
    tree.insert_child_or_append(root, second, None)
        .expect("none appends second child");
    tree.insert_child_or_append(root, inserted, Some(1))
        .expect("some inserts at index");
    tree.insert_child_or_append(root, clamped, Some(99))
        .expect("oversized index appends");
    tree.insert_child_at_standalone_index(root, negative, -2)
        .expect("any negative index appends");
    tree.insert_child_at_standalone_index(root, zero, 0)
        .expect("zero index inserts before first child");

    assert_eq!(
        tree.children(root).expect("root children"),
        [zero, first, inserted, second, clamped, negative]
    );
    assert_eq!(tree.child_at(root, 6).expect("out of range child"), None);
    assert_eq!(tree.parent(zero).expect("zero parent"), Some(root));
    assert_eq!(tree.parent(first).expect("first parent"), Some(root));
    assert_eq!(tree.parent(inserted).expect("inserted parent"), Some(root));
    assert_eq!(tree.parent(clamped).expect("clamped parent"), Some(root));
    assert_eq!(tree.parent(negative).expect("negative parent"), Some(root));
}

#[test]
fn standalone_tree_insert_child_before_or_append_matches_public_standalone_reference_semantics() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style::default());
    let first = tree.create_node(Style::default());
    let second = tree.create_node(Style::default());
    let inserted = tree.create_node(Style::default());

    tree.insert_child_before_or_append(root, first, None)
        .expect("none reference appends first child");
    tree.insert_child_before_or_append(root, second, None)
        .expect("none reference appends second child");
    tree.insert_child_before_or_append(root, inserted, Some(second))
        .expect("some reference inserts before child");
    assert_eq!(
        tree.children(root).expect("root children"),
        [first, inserted, second]
    );

    tree.insert_child_before_or_append(root, first, Some(second))
        .expect("same-parent reinsert before reference");
    assert_eq!(
        tree.children(root).expect("root children after reorder"),
        [inserted, first, second]
    );
    assert_eq!(tree.parent(first).expect("first parent"), Some(root));
    assert_eq!(tree.parent(inserted).expect("inserted parent"), Some(root));
    assert_eq!(tree.parent(second).expect("second parent"), Some(root));
}

#[test]
fn standalone_tree_exposes_child_parent_and_rtl_queries() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style {
        direction: Direction::Rtl,
        ..Style::default()
    });
    let first = tree.create_node(Style::default());
    let second = tree.create_node(Style::default());
    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");

    assert_eq!(tree.child_count(root).expect("root child count"), 2);
    assert_eq!(tree.child_at(root, 0).expect("first child"), Some(first));
    assert_eq!(tree.child_at(root, 1).expect("second child"), Some(second));
    assert_eq!(tree.child_at(root, 2).expect("out of range child"), None);
    assert_eq!(
        tree.child_at_standalone_index(root, -1)
            .expect("negative child index"),
        None
    );
    assert_eq!(
        tree.child_at_standalone_index(root, 0)
            .expect("signed first child"),
        Some(first)
    );
    assert_eq!(
        tree.child_at_standalone_index(root, 2)
            .expect("signed out of range child"),
        None
    );
    assert_eq!(tree.parent(first).expect("first parent"), Some(root));
    assert!(tree.is_rtl(root).expect("root rtl"));
    assert!(!tree.is_rtl(first).expect("first rtl"));
    assert!(!tree.is_rtl(second).expect("second rtl"));
}

#[test]
fn standalone_tree_reparents_existing_child_on_insert() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style::default());
    let child = tree.create_node(Style::default());
    let other = tree.create_node(Style::default());
    let first = tree.create_node(Style::default());
    let second = tree.create_node(Style::default());

    assert_eq!(
        tree.append_child(root, root)
            .expect_err("self parenting fails"),
        TreeError::CannotParentNodeToItself(root)
    );

    tree.append_child(root, child).expect("append child");
    tree.append_child(other, child)
        .expect("append reparents existing child");
    assert!(tree.children(root).expect("root children").is_empty());
    assert_eq!(tree.children(other).expect("other children"), [child]);
    assert_eq!(tree.parent(child).expect("child parent"), Some(other));

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.append_child(root, child)
        .expect("append moves child back to root");
    tree.insert_child_before(root, child, second)
        .expect("reorder child before second");
    assert_eq!(
        tree.children(root).expect("root children"),
        [first, child, second]
    );
    assert!(tree.children(other).expect("other children").is_empty());

    assert_eq!(
        tree.insert_child_before(root, other, other)
            .expect_err("foreign reference fails"),
        TreeError::ReferenceNotChild {
            parent: root,
            reference: other,
        }
    );
}

#[test]
fn standalone_tree_dirty_state_tracks_mutations_and_layout() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(40.0),
        height: Length::points(20.0),
        ..standalone_default_style()
    });
    let child = tree.create_default_measured_node(Size::new(10.0, 5.0));
    tree.append_child(root, child).expect("append child");

    assert!(tree.is_dirty(root).expect("parent dirty after append"));
    assert!(!tree.is_dirty(child).expect("child clean after append"));

    tree.calculate_layout(
        root,
        Size {
            width: 40.0,
            height: 20.0,
        },
        Direction::Ltr,
    )
    .expect("layout root");

    assert!(!tree.is_dirty(root).expect("root clean after layout"));
    assert!(!tree.is_dirty(child).expect("child clean after layout"));

    tree.node_mut(child)
        .expect("child node")
        .set_measured_size(Some(Size::new(12.0, 6.0)));
    assert!(tree.is_dirty(child).expect("child dirty after measure"));
    assert!(tree
        .is_dirty(root)
        .expect("parent dirty after child mutation"));

    tree.calculate_layout(
        root,
        Size {
            width: 40.0,
            height: 20.0,
        },
        Direction::Ltr,
    )
    .expect("layout root");

    tree.style_mut(child).expect("child style").margin.left = Length::points(3.0);
    assert!(tree.is_dirty(child).expect("child dirty after style"));
    assert!(tree.is_dirty(root).expect("parent dirty after style"));
}

#[test]
fn standalone_tree_node_mut_style_changes_dirty_ancestors_and_next_layout() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(40.0),
        ..standalone_default_style()
    });
    let child = tree.create_default_measured_node(Size::new(10.0, 6.0));
    tree.append_child(root, child).expect("append child");

    tree.calculate_layout(
        root,
        Size {
            width: 100.0,
            height: 40.0,
        },
        Direction::Ltr,
    )
    .expect("initial layout");
    assert_close(tree.layout_width(child).expect("initial child width"), 10.0);
    assert!(!tree.is_dirty(root).expect("root clean after layout"));
    assert!(!tree.is_dirty(child).expect("child clean after layout"));

    tree.node_mut(child)
        .expect("mutable child")
        .style_mut()
        .min_width = Length::points(24.0);

    assert!(tree.is_dirty(child).expect("child dirty after node_mut"));
    assert!(tree.is_dirty(root).expect("ancestor dirty after node_mut"));

    tree.calculate_layout(
        root,
        Size {
            width: 100.0,
            height: 40.0,
        },
        Direction::Ltr,
    )
    .expect("relayout after node_mut style");
    assert_close(tree.layout_width(child).expect("updated child width"), 24.0);
    assert!(!tree.is_dirty(root).expect("root clean after relayout"));
    assert!(!tree.is_dirty(child).expect("child clean after relayout"));
}

#[test]
fn standalone_tree_reset_node_clears_children_layout_style_and_measurement() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style {
        width: Length::points(30.0),
        height: Length::points(20.0),
        ..standalone_default_style()
    });
    let child = tree.create_default_measured_node(Size::new(10.0, 5.0));
    tree.set_measured_size(root, Some(Size::new(12.0, 6.0)))
        .expect("set root measured size");
    tree.set_baseline(root, Some(7.0))
        .expect("set root baseline");
    tree.append_child(root, child).expect("append child");
    tree.calculate_layout(
        root,
        Size {
            width: 30.0,
            height: 20.0,
        },
        Direction::Ltr,
    )
    .expect("layout root");

    assert!(tree.has_measure_func(root).expect("root measure flag"));
    assert_eq!(tree.baseline(root).expect("root baseline"), Some(7.0));

    tree.reset_node(root).expect("reset root");

    assert!(tree.children(root).expect("root children").is_empty());
    assert_eq!(tree.parent(child).expect("child parent"), None);
    assert_eq!(
        tree.style(root).expect("root style"),
        &standalone_default_style()
    );
    assert_eq!(
        tree.layout(root).expect("root layout"),
        starlight_layout::LayoutResult::default()
    );
    assert_eq!(tree.node(root).expect("root node").measured_size(), None);
    assert!(!tree.has_measure_func(root).expect("root measure flag"));
    assert_eq!(tree.node(root).expect("root node").baseline(), None);
    assert_eq!(tree.baseline(root).expect("root baseline"), None);
    assert!(!tree.is_dirty(root).expect("root clean after reset"));
}

#[test]
fn standalone_tree_reset_attached_child_preserves_clean_parent_behavior() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(80.0),
        height: Length::points(30.0),
        ..standalone_default_style()
    });
    let child = tree.create_measured_node(
        Style {
            width: Length::points(24.0),
            height: Length::points(12.0),
            ..standalone_default_style()
        },
        Size::new(24.0, 12.0),
    );
    let grandchild = tree.create_default_measured_node(Size::new(6.0, 4.0));
    tree.set_baseline(child, Some(7.0))
        .expect("set child baseline");
    tree.append_child(root, child).expect("append child");
    tree.append_child(child, grandchild)
        .expect("append grandchild");

    tree.calculate_layout(
        root,
        Size {
            width: 80.0,
            height: 30.0,
        },
        Direction::Ltr,
    )
    .expect("layout root");
    assert!(!tree.is_dirty(root).expect("root clean after layout"));
    assert!(!tree.is_dirty(child).expect("child clean after layout"));

    tree.reset_node(child).expect("reset child");

    assert_eq!(tree.children(root).expect("root children"), &[child]);
    assert_eq!(tree.parent(child).expect("child parent"), Some(root));
    assert!(tree.children(child).expect("child children").is_empty());
    assert_eq!(tree.parent(grandchild).expect("grandchild parent"), None);
    assert_eq!(
        tree.style(child).expect("child style"),
        &standalone_default_style()
    );
    assert_eq!(
        tree.layout(child).expect("child layout"),
        starlight_layout::LayoutResult::default()
    );
    assert_eq!(tree.measured_size(child).expect("child measurement"), None);
    assert!(!tree.has_measure_func(child).expect("child measure flag"));
    assert_eq!(tree.baseline(child).expect("child baseline"), None);
    assert!(!tree.is_dirty(child).expect("child clean after reset"));
    assert!(!tree.is_dirty(root).expect("root clean after child reset"));
}

#[test]
fn standalone_tree_calculate_layout_with_mode_uses_owner_direction_temporarily() {
    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style {
        width: Length::points(40.0),
        height: Length::points(20.0),
        ..Style::default()
    });

    tree.calculate_layout_with_mode(
        root,
        Constraints::new(
            SideConstraint::at_most(100.0),
            SideConstraint::at_most(100.0),
        ),
        Direction::Rtl,
    )
    .expect("layout root");

    assert_eq!(
        tree.style(root).expect("root style").direction,
        Direction::Ltr
    );
    assert_close(tree.layout(root).expect("root layout").size.width, 40.0);
    assert_close(tree.layout(root).expect("root layout").size.height, 20.0);
}

#[test]
fn standalone_tree_owner_direction_reaches_unset_descendants_only_during_layout() {
    let nested_style = Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(30.0),
        height: Length::points(10.0),
        ..Style::default()
    };
    let leaf_style = Style {
        width: Length::points(10.0),
        height: Length::points(5.0),
        ..Style::default()
    };

    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(30.0),
        height: Length::points(20.0),
        ..Style::default()
    });
    let inherited_direction_container = tree.create_node(nested_style.clone());
    let inherited_direction_leaf = tree.create_node(leaf_style.clone());
    let explicit_ltr_container = tree.create_node(nested_style);
    let explicit_ltr_leaf = tree.create_node(leaf_style);

    tree.append_child(root, inherited_direction_container)
        .expect("append inherited-direction container");
    tree.append_child(inherited_direction_container, inherited_direction_leaf)
        .expect("append inherited-direction leaf");
    tree.append_child(root, explicit_ltr_container)
        .expect("append explicit-ltr container");
    tree.append_child(explicit_ltr_container, explicit_ltr_leaf)
        .expect("append explicit-ltr leaf");
    tree.set_direction(explicit_ltr_container, Direction::Ltr)
        .expect("mark nested container direction explicit");

    tree.calculate_layout_with_mode(
        root,
        Constraints::new(SideConstraint::at_most(30.0), SideConstraint::at_most(20.0)),
        Direction::Rtl,
    )
    .expect("layout root");

    assert_close(
        tree.layout(inherited_direction_leaf)
            .expect("inherited-direction leaf layout")
            .offset
            .x,
        20.0,
    );
    assert_close(
        tree.layout(explicit_ltr_leaf)
            .expect("explicit-ltr leaf layout")
            .offset
            .x,
        0.0,
    );
    assert_eq!(
        tree.style(root).expect("root style").direction,
        Direction::Ltr
    );
    assert_eq!(
        tree.style(inherited_direction_container)
            .expect("inherited-direction container style")
            .direction,
        Direction::Ltr
    );
    assert_eq!(
        tree.style(explicit_ltr_container)
            .expect("explicit-ltr container style")
            .direction,
        Direction::Ltr
    );
}

#[test]
fn standalone_tree_clear_direction_restores_owner_direction_inheritance() {
    let container_style = Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Row,
        align_items: AlignItems::FlexStart,
        width: Length::points(30.0),
        height: Length::points(10.0),
        ..Style::default()
    };
    let leaf_style = Style {
        width: Length::points(10.0),
        height: Length::points(5.0),
        ..Style::default()
    };

    let mut tree = StandaloneTree::new();
    let root = tree.create_node(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(30.0),
        height: Length::points(20.0),
        ..Style::default()
    });
    let container = tree.create_node(container_style);
    let leaf = tree.create_node(leaf_style);

    tree.append_child(root, container)
        .expect("append direction container");
    tree.append_child(container, leaf)
        .expect("append direction leaf");
    tree.set_direction(container, Direction::Ltr)
        .expect("set explicit ltr direction");
    assert!(tree
        .has_explicit_direction_style(container)
        .expect("explicit direction flag"));

    tree.calculate_layout_with_mode(
        root,
        Constraints::new(
            SideConstraint::definite(30.0),
            SideConstraint::definite(20.0),
        ),
        Direction::Rtl,
    )
    .expect("layout with explicit ltr child");
    assert_close(tree.layout(leaf).expect("explicit ltr leaf").offset.x, 0.0);
    assert!(!tree.is_dirty(root).expect("clean after first layout"));

    tree.clear_direction(container)
        .expect("clear explicit direction");
    assert!(!tree
        .has_explicit_direction_style(container)
        .expect("cleared direction flag"));
    assert!(tree.is_dirty(root).expect("dirty root after clear"));

    tree.calculate_layout_with_mode(
        root,
        Constraints::new(
            SideConstraint::definite(30.0),
            SideConstraint::definite(20.0),
        ),
        Direction::Rtl,
    )
    .expect("layout after clearing direction");
    assert_close(
        tree.layout(leaf).expect("inherited rtl leaf").offset.x,
        20.0,
    );
    assert_eq!(
        tree.style(container)
            .expect("container style after restore")
            .direction,
        Direction::Ltr
    );
}

fn standalone_header_style_setter_families(source: &str) -> BTreeSet<String> {
    source
        .lines()
        .map(str::trim)
        .filter_map(|line| {
            let rest = line.strip_prefix("void SLNodeStyleSet")?;
            let (suffix, _) = rest.split_once('(')?;
            Some(format!(
                "set_{}",
                camel_to_snake(normalize_public_style_setter_suffix(suffix.trim()))
            ))
        })
        .collect()
}

fn standalone_header_style_getter_families(source: &str) -> BTreeSet<String> {
    source
        .lines()
        .map(str::trim)
        .filter_map(|line| {
            let name_start = line.find("SLNodeStyleGet")?;
            let rest = &line[name_start + "SLNodeStyleGet".len()..];
            let (suffix, _) = rest.split_once('(')?;
            Some(format!("style_{}", camel_to_snake(suffix.trim())))
        })
        .collect()
}

const PUBLIC_NON_STYLE_API_METHOD_MAPPINGS: &[(&str, &[&str])] = &[
    ("SLConfigCreate", &["new"]),
    (
        "SLConfigSetPhysicalPixelsPerLayoutUnit",
        &["set_physical_pixels_per_layout_unit"],
    ),
    (
        "SLConfigGetPhysicalPixelsPerLayoutUnit",
        &["physical_pixels_per_layout_unit"],
    ),
    ("SLNodeNew", &["create_default_node"]),
    ("SLNodeNewWithConfig", &["create_default_node_with_config"]),
    ("SLNodeInsertChild", &["insert_child_at_standalone_index"]),
    (
        "SLNodeInsertChildBefore",
        &["insert_child_before_or_append"],
    ),
    ("SLNodeRemoveChild", &["remove_child"]),
    ("SLNodeRemoveAllChildren", &["remove_all_children"]),
    ("SLNodeReset", &["reset_node"]),
    ("SLNodeGetChild", &["child_at_standalone_index"]),
    ("SLNodeGetChildCount", &["child_count"]),
    ("SLNodeGetParent", &["parent"]),
    ("SLNodeIsDirty", &["is_dirty"]),
    ("SLNodeMarkDirty", &["mark_dirty"]),
    ("SLNodeIsRTL", &["is_rtl"]),
    ("SLNodeCalculateLayout", &["calculate_layout"]),
    (
        "SLNodeCalculateLayoutWithMode",
        &["calculate_layout_with_mode"],
    ),
    (
        "SLNodeSetMeasureDelegate",
        &["set_measure_func", "set_baseline_func"],
    ),
    (
        "SLNodeGetMeasureDelegate",
        &["measure_func", "baseline_func"],
    ),
    ("SLNodeHasMeasureFunc", &["has_measure_func"]),
    ("SLNodeLayoutGetLeft", &["layout_left"]),
    ("SLNodeLayoutGetTop", &["layout_top"]),
    ("SLNodeLayoutGetWidth", &["layout_width"]),
    ("SLNodeLayoutGetHeight", &["layout_height"]),
    ("SLNodeLayoutGetBaseline", &["layout_baseline"]),
    ("SLNodeLayoutGetMargin", &["layout_margin"]),
    ("SLNodeLayoutGetPadding", &["layout_padding"]),
    ("SLNodeLayoutGetBorder", &["layout_border"]),
    ("SLNodeLayoutGetStickyPosition", &["layout_sticky_position"]),
];

const EXEMPTED_PUBLIC_NON_STYLE_API_FUNCTIONS: &[(&str, &str)] = &[
    (
        "SLConfigFree",
        "StandaloneConfig is a Rust value and follows Rust drop semantics.",
    ),
    (
        "SLNodeFree",
        "StandaloneTree owns node storage and releases it with the tree.",
    ),
    (
        "SLNodeFreeRecursive",
        "StandaloneTree owns child storage and releases subtrees with the tree.",
    ),
];

fn standalone_header_function_names(source: &str) -> BTreeSet<String> {
    source
        .lines()
        .map(str::trim)
        .filter(|line| !line.starts_with("//"))
        .filter_map(|line| {
            let paren = line.find('(')?;
            let before_paren = line[..paren].trim();
            let name = before_paren.split_whitespace().last()?;
            (name.starts_with("SLNode") || name.starts_with("SLConfig")).then(|| name.to_owned())
        })
        .collect()
}

fn rust_standalone_tree_public_methods(source: &str) -> BTreeSet<String> {
    rust_public_methods_in_region(
        source,
        "impl StandaloneTree {",
        "impl LayoutTree for StandaloneTree",
    )
}

fn rust_standalone_config_public_methods(source: &str) -> BTreeSet<String> {
    rust_public_methods_in_region(
        source,
        "impl StandaloneConfig {",
        "impl Default for StandaloneConfig",
    )
}

fn rust_public_methods_in_region(
    source: &str,
    start_marker: &str,
    end_marker: &str,
) -> BTreeSet<String> {
    source_region(source, start_marker, end_marker)
        .lines()
        .map(str::trim)
        .filter_map(|line| {
            let rest = line
                .strip_prefix("pub fn ")
                .or_else(|| line.strip_prefix("pub const fn "))?;
            let (name, _) = rest.split_once('(')?;
            Some(name.trim().to_owned())
        })
        .collect()
}

fn normalize_public_style_setter_suffix(suffix: &str) -> &str {
    for variant_suffix in [
        "FitContentValue",
        "MaxContent",
        "FitContent",
        "Percent",
        "Calc",
        "Value",
        "Auto",
    ] {
        if let Some(base) = suffix.strip_suffix(variant_suffix) {
            return base;
        }
    }
    suffix
}

fn camel_to_snake(value: &str) -> String {
    let mut output = String::new();
    let mut previous_was_lower_or_digit = false;
    for ch in value.chars() {
        if ch.is_ascii_uppercase() {
            if previous_was_lower_or_digit {
                output.push('_');
            }
            output.push(ch.to_ascii_lowercase());
            previous_was_lower_or_digit = false;
        } else {
            output.push(ch);
            previous_was_lower_or_digit = ch.is_ascii_lowercase() || ch.is_ascii_digit();
        }
    }
    output
}

fn source_region<'a>(source: &'a str, start_marker: &str, end_marker: &str) -> &'a str {
    let start = source.find(start_marker).expect("source region starts");
    let end = source[start..]
        .find(end_marker)
        .map(|offset| start + offset)
        .expect("source region ends");
    &source[start..end]
}
