// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use starlight_layout::{
    AlignContent, AlignItems, BaseLength, BoxSizing, Constraints, Direction, Display, Edges,
    FlexDirection, FlexWrap, JustifyContent, LayoutEngine, LayoutResult, LayoutTree, Length,
    LinearOrientation, MeasureMode, Rect, SideConstraint, SimpleNode, SimpleTree, Size, Style,
    Visibility,
};
use starlight_parity::run_rust_layout;

fn assert_close(actual: f32, expected: f32) {
    assert!(
        (actual - expected).abs() < 0.01,
        "expected {expected}, got {actual}"
    );
}

fn assert_close_named(name: &str, actual: f32, expected: f32) {
    assert!(
        (actual - expected).abs() < 0.01,
        "{name}: expected {expected}, got {actual}"
    );
}

fn measure_available_width(constraints: Constraints) -> Size {
    let width = match constraints.width.mode {
        MeasureMode::Indefinite => 7.0,
        MeasureMode::Definite | MeasureMode::AtMost => constraints.width.size,
    };
    Size::new(width, 10.0)
}

#[test]
fn flex_visibility_collapse_is_trait_facing_style_input() {
    let collapsed = Style {
        display: Display::Flex,
        visibility: Visibility::Collapse,
        ..Style::default()
    };

    assert_eq!(collapsed.visibility, Visibility::Collapse);
    assert_ne!(collapsed.display, Display::None);
}

fn measure_height_from_width(constraints: Constraints) -> Size {
    let width = match constraints.width.mode {
        MeasureMode::Indefinite => 20.0,
        MeasureMode::Definite | MeasureMode::AtMost => constraints.width.size,
    };
    Size::new(width, width / 4.0)
}

#[test]
fn flex_order_reorders_visual_layout_without_reordering_tree() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(90.0),
        height: Length::points(10.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first_in_tree = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(10.0),
        order: 2,
        ..Style::default()
    }));
    let second_in_tree = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(10.0),
        order: 1,
        ..Style::default()
    }));
    let third_in_tree = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(10.0),
        order: 3,
        ..Style::default()
    }));
    tree.append_child(root, first_in_tree);
    tree.append_child(root, second_in_tree);
    tree.append_child(root, third_in_tree);

    run_rust_layout(&mut tree, root, Constraints::definite(90.0, 10.0));

    assert_eq!(
        tree.nodes[root].children,
        vec![first_in_tree, second_in_tree, third_in_tree]
    );
    assert_close(tree.nodes[second_in_tree].layout.offset.x, 0.0);
    assert_close(tree.nodes[first_in_tree].layout.offset.x, 10.0);
    assert_close(tree.nodes[third_in_tree].layout.offset.x, 20.0);
}

#[test]
fn flex_wrap_collects_items_into_multiple_lines() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        width: Length::points(50.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 30.0, 10.0);
    let third = fixed_flex_child(&mut tree, 30.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::definite(50.0), SideConstraint::indefinite()),
    );

    assert_close(size.width, 50.0);
    assert_close(size.height, 30.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 10.0);
    assert_close(tree.nodes[third].layout.offset.x, 0.0);
    assert_close(tree.nodes[third].layout.offset.y, 20.0);
}

#[test]
fn flex_wrap_collects_zero_sized_item_after_exact_fit_on_same_line() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        width: Length::points(50.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 50.0, 10.0);
    let zero = fixed_flex_child(&mut tree, 0.0, 6.0);
    let next_line = fixed_flex_child(&mut tree, 10.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, zero);
    tree.append_child(root, next_line);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::definite(50.0), SideConstraint::indefinite()),
    );

    assert_close(size.height, 20.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[zero].layout.size.width, 0.0);
    assert_close(tree.nodes[zero].layout.offset.x, 50.0);
    assert_close(tree.nodes[zero].layout.offset.y, 0.0);
    assert_close(tree.nodes[next_line].layout.offset.x, 0.0);
    assert_close(tree.nodes[next_line].layout.offset.y, 10.0);
}

#[test]
fn flex_main_size_nowrap_collects_all_items_into_single_line_even_when_overflowing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::NoWrap,
        width: Length::points(50.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(40.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(40.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::definite(50.0), SideConstraint::indefinite()),
    );

    assert_close(size.height, 10.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 40.0);
    assert_close(tree.nodes[second].layout.offset.y, 0.0);
}

#[test]
fn flex_main_size_wrap_collects_oversized_first_item_alone() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        width: Length::points(30.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let oversized = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(50.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let next = fixed_flex_child(&mut tree, 10.0, 10.0);
    tree.append_child(root, oversized);
    tree.append_child(root, next);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::definite(30.0), SideConstraint::indefinite()),
    );

    assert_close(size.height, 20.0);
    assert_close(tree.nodes[oversized].layout.offset.x, 0.0);
    assert_close(tree.nodes[oversized].layout.size.width, 50.0);
    assert_close(tree.nodes[next].layout.offset.x, 0.0);
    assert_close(tree.nodes[next].layout.offset.y, 10.0);
}

#[test]
fn flex_main_size_line_collection_uses_outer_hypothetical_main_with_negative_margin() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        width: Length::points(50.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(40.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::ZERO,
            Length::points(-30.0),
            Length::ZERO,
            Length::ZERO,
        ),
        ..Style::default()
    }));
    let second = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::definite(50.0), SideConstraint::indefinite()),
    );

    assert_close(size.height, 10.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 10.0);
    assert_close(tree.nodes[second].layout.offset.y, 0.0);
}

#[test]
fn flex_main_size_resolves_flexible_lengths_per_line_independently() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        width: Length::points(100.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = growing_flex_child(&mut tree, 40.0, 10.0);
    let second = growing_flex_child(&mut tree, 40.0, 10.0);
    let third = growing_flex_child(&mut tree, 40.0, 10.0);
    let fourth = growing_flex_child(&mut tree, 20.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);
    tree.append_child(root, fourth);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.height, 20.0);
    assert_close(tree.nodes[first].layout.size.width, 50.0);
    assert_close(tree.nodes[second].layout.size.width, 50.0);
    assert_close(tree.nodes[third].layout.size.width, 60.0);
    assert_close(tree.nodes[fourth].layout.size.width, 40.0);
    assert_close(tree.nodes[third].layout.offset.y, 10.0);
    assert_close(tree.nodes[fourth].layout.offset.x, 60.0);
}

#[test]
fn column_flex_item_percent_cross_size_and_aspect_ratio_define_main_basis() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        width: Length::points(126.0),
        height: Length::points(92.0),
        align_items: AlignItems::FlexEnd,
        justify_content: JustifyContent::Center,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            box_sizing: BoxSizing::BorderBox,
            width: Length::percent(38.0),
            height: Length::Auto,
            aspect_ratio: Some(1.5),
            padding: Rect::all(Length::points(1.0)),
            border: Rect::all(1.0),
            ..Style::default()
        },
        Size::new(44.0, 18.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(126.0, 92.0));

    assert_close(tree.nodes[child].layout.size.width, 48.0);
    assert_close(tree.nodes[child].layout.size.height, 32.0);
}

#[test]
fn flex_nowrap_at_most_main_axis_shrinks_to_items() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 20.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::at_most(100.0), SideConstraint::indefinite()),
    );

    assert_close(size.width, 50.0);
    assert_close(size.height, 10.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 30.0);
}

#[test]
fn flex_line_length_available_main_space_uses_inner_content_box_for_auto_basis() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        height: Length::points(20.0),
        padding: Rect::new(
            Length::points(5.0),
            Length::points(5.0),
            Length::points(0.0),
            Length::points(0.0),
        ),
        border: Rect::new(2.0, 2.0, 0.0, 0.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measure_func(
        Style {
            height: Length::points(10.0),
            ..Style::default()
        },
        measure_available_width,
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[child].layout.size.width, 86.0);
    assert_close(tree.nodes[child].layout.offset.x, 7.0);
}

#[test]
fn flex_line_length_definite_flex_basis_overrides_main_size_property() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(30.0),
        width: Length::points(80.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 30.0);
    assert_close(tree.nodes[child].layout.size.width, 30.0);
}

#[test]
fn flex_line_length_aspect_ratio_uses_definite_cross_size_for_content_basis() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(100.0),
        height: Length::points(40.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        height: Length::points(30.0),
        aspect_ratio: Some(2.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[child].layout.size.width, 60.0);
    assert_close(tree.nodes[child].layout.size.height, 30.0);
}

#[test]
fn flex_line_length_hypothetical_main_size_clamps_min_before_wrapping() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        width: Length::points(80.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let clamped = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        min_width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let sibling = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, clamped);
    tree.append_child(root, sibling);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::definite(80.0), SideConstraint::indefinite()),
    );

    assert_close(size.height, 20.0);
    assert_close(tree.nodes[clamped].layout.size.width, 50.0);
    assert_close(tree.nodes[sibling].layout.offset.x, 0.0);
    assert_close(tree.nodes[sibling].layout.offset.y, 10.0);
}

#[test]
fn flex_line_length_hypothetical_main_size_clamps_max_before_wrapping() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        width: Length::points(70.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let clamped = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(80.0),
        max_width: Length::points(30.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let sibling = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, clamped);
    tree.append_child(root, sibling);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::definite(70.0), SideConstraint::indefinite()),
    );

    assert_close(size.height, 10.0);
    assert_close(tree.nodes[clamped].layout.size.width, 30.0);
    assert_close(tree.nodes[sibling].layout.offset.x, 30.0);
    assert_close(tree.nodes[sibling].layout.offset.y, 0.0);
}

#[test]
fn flex_line_length_auto_container_main_size_uses_max_content_sum() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 20.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 50.0);
    assert_close(size.height, 10.0);
}

#[test]
fn flex_stretch_column_rtl_offsets_using_final_stretched_cross_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::ColumnReverse,
        flex_wrap: FlexWrap::NoWrap,
        direction: Direction::Rtl,
        width: Length::points(20.0),
        height: Length::points(24.0),
        padding: Rect::new(
            Length::points(0.0),
            Length::points(5.0),
            Length::points(0.0),
            Length::points(0.0),
        ),
        align_items: AlignItems::Stretch,
        align_content: AlignContent::FlexEnd,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            width: Length::Auto,
            height: Length::points(10.0),
            min_width: Length::points(17.0),
            margin: Rect::new(
                Length::points(0.0),
                Length::points(7.0),
                Length::points(0.0),
                Length::points(0.0),
            ),
            ..Style::default()
        },
        Size::new(26.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 24.0));

    assert_close(tree.nodes[child].layout.offset.x, -4.0);
}

#[test]
fn display_none_child_is_laid_out_as_zero_and_skipped_by_flex() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(100.0),
        height: Length::points(10.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 20.0, 10.0);
    let hidden = tree.push(SimpleNode::new(Style {
        display: Display::None,
        flex_basis: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = fixed_flex_child(&mut tree, 20.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, hidden);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 20.0);
    assert_eq!(tree.nodes[hidden].layout.size, Size::ZERO);
    assert_close(tree.nodes[hidden].layout.offset.x, 0.0);
    assert_close(tree.nodes[hidden].layout.offset.y, 0.0);
}

#[test]
fn flex_wrap_at_most_main_axis_shrinks_to_largest_line() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 40.0, 10.0);
    let second = fixed_flex_child(&mut tree, 40.0, 10.0);
    let third = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::at_most(100.0), SideConstraint::indefinite()),
    );

    assert_close(size.width, 80.0);
    assert_close(size.height, 20.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 40.0);
    assert_close(tree.nodes[third].layout.offset.x, 0.0);
    assert_close(tree.nodes[third].layout.offset.y, 10.0);
}

#[test]
fn flex_nowrap_cross_axis_at_most_does_not_clamp_latest_mode() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let child = fixed_flex_child(&mut tree, 30.0, 20.0);
    tree.append_child(root, child);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(50.0),
            SideConstraint::at_most(10.0),
        ),
    );

    assert_close(size.width, 50.0);
    assert_close(size.height, 20.0);
    assert_close(tree.nodes[child].layout.size.height, 20.0);
}

#[test]
fn flex_column_cross_axis_at_most_does_not_clamp_latest_mode() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        height: Length::points(50.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        width: Length::points(30.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::at_most(10.0),
            SideConstraint::definite(50.0),
        ),
    );

    assert_close(size.width, 30.0);
    assert_close(size.height, 50.0);
    assert_close(tree.nodes[child].layout.size.width, 30.0);
}

#[test]
fn flex_wrap_cross_axis_at_most_does_not_clamp_line_sum_latest_mode() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 30.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(50.0),
            SideConstraint::at_most(15.0),
        ),
    );

    assert_close(size.width, 50.0);
    assert_close(size.height, 20.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 10.0);
}

#[test]
fn align_self_overrides_container_align_items() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(50.0),
        height: Length::points(30.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let centered = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::Center),
        ..Style::default()
    }));
    let start = fixed_flex_child(&mut tree, 10.0, 10.0);
    tree.append_child(root, centered);
    tree.append_child(root, start);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 30.0));

    assert_close(tree.nodes[centered].layout.offset.y, 10.0);
    assert_close(tree.nodes[start].layout.offset.y, 0.0);
}

#[test]
fn align_items_start_end_alias_flex_edges_for_items() {
    for (align_items, expected_y) in [
        (AlignItems::Start, 0.0),
        (AlignItems::FlexStart, 0.0),
        (AlignItems::End, 20.0),
        (AlignItems::FlexEnd, 20.0),
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            width: Length::points(50.0),
            height: Length::points(30.0),
            align_items,
            ..Style::default()
        }));
        let child = fixed_flex_child(&mut tree, 10.0, 10.0);
        tree.append_child(root, child);

        run_rust_layout(&mut tree, root, Constraints::definite(50.0, 30.0));

        assert_close(tree.nodes[child].layout.offset.y, expected_y);
    }
}

#[test]
fn single_line_min_cross_size_clamps_line_before_cross_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(50.0),
        min_height: Length::points(30.0),
        align_items: AlignItems::Center,
        ..Style::default()
    }));
    let child = fixed_flex_child(&mut tree, 10.0, 10.0);
    tree.append_child(root, child);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::definite(50.0), SideConstraint::indefinite()),
    );

    assert_close(size.height, 30.0);
    assert_close(tree.nodes[child].layout.offset.y, 10.0);
}

#[test]
fn align_items_center_uses_negative_cross_space_when_item_overflows() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(50.0),
        height: Length::points(30.0),
        align_items: AlignItems::Center,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(50.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 30.0));

    assert_close(tree.nodes[child].layout.offset.y, -10.0);
}

#[test]
fn align_items_flex_end_uses_negative_cross_space_when_item_overflows() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(50.0),
        height: Length::points(30.0),
        align_items: AlignItems::FlexEnd,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(50.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 30.0));

    assert_close(tree.nodes[child].layout.offset.y, -20.0);
}

#[test]
fn flex_row_baseline_aligns_items_by_fallback_border_box_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(110.0),
        height: Length::points(30.0),
        align_items: AlignItems::Baseline,
        ..Style::default()
    }));
    let short = fixed_flex_child(&mut tree, 10.0, 10.0);
    let tall = fixed_flex_child(&mut tree, 10.0, 20.0);
    tree.append_child(root, short);
    tree.append_child(root, tall);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 30.0));

    assert_close(tree.nodes[short].layout.offset.y, 10.0);
    assert_close(tree.nodes[tall].layout.offset.y, 0.0);
}

#[test]
fn flex_row_baseline_uses_measured_content_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::Baseline,
        ..Style::default()
    }));
    let early_baseline = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(10.0, 20.0),
        5.0,
    ));
    let late_baseline = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(10.0, 30.0),
        25.0,
    ));
    tree.append_child(root, early_baseline);
    tree.append_child(root, late_baseline);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 20.0);
    assert_close(size.height, 40.0);
    assert_close(tree.nodes[early_baseline].layout.offset.y, 20.0);
    assert_close(tree.nodes[late_baseline].layout.offset.y, 0.0);
}

#[test]
fn flex_row_align_self_baseline_triggers_baseline_line_sizing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let early_baseline = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style {
            align_self: Some(AlignItems::Baseline),
            ..Style::default()
        },
        Size::new(10.0, 20.0),
        5.0,
    ));
    let late_baseline = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style {
            align_self: Some(AlignItems::Baseline),
            ..Style::default()
        },
        Size::new(10.0, 30.0),
        25.0,
    ));
    tree.append_child(root, early_baseline);
    tree.append_child(root, late_baseline);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.height, 40.0);
    assert_close(tree.nodes[early_baseline].layout.offset.y, 20.0);
    assert_close(tree.nodes[late_baseline].layout.offset.y, 0.0);
}

#[test]
fn flex_row_baseline_uses_nested_flex_container_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::Baseline,
        ..Style::default()
    }));
    let reference = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(10.0, 30.0),
        25.0,
    ));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::Baseline,
        ..Style::default()
    }));
    let nested_early = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(10.0, 20.0),
        5.0,
    ));
    let nested_late = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(10.0, 30.0),
        15.0,
    ));
    tree.append_child(nested, nested_early);
    tree.append_child(nested, nested_late);
    tree.append_child(root, reference);
    tree.append_child(root, nested);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[nested].layout.baseline.unwrap(), 15.0);
    assert_close(size.height, 40.0);
    assert_close(tree.nodes[reference].layout.offset.y, 0.0);
    assert_close(tree.nodes[nested].layout.offset.y, 10.0);
}

#[test]
fn flex_row_baseline_uses_nested_linear_container_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::Baseline,
        ..Style::default()
    }));
    let reference = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(10.0, 40.0),
        35.0,
    ));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Horizontal,
        ..Style::default()
    }));
    let nested_early = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(10.0, 20.0),
        5.0,
    ));
    let nested_late = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(10.0, 30.0),
        25.0,
    ));
    tree.append_child(nested, nested_early);
    tree.append_child(nested, nested_late);
    tree.append_child(root, reference);
    tree.append_child(root, nested);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[nested].layout.baseline.unwrap(), 25.0);
    assert_close(size.height, 40.0);
    assert_close(tree.nodes[reference].layout.offset.y, 0.0);
    assert_close(tree.nodes[nested].layout.offset.y, 10.0);
}

#[test]
fn flex_row_baseline_uses_nested_grid_container_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::Baseline,
        ..Style::default()
    }));
    let reference = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(10.0, 30.0),
        25.0,
    ));
    let nested = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(20.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        align_items: AlignItems::Baseline,
        ..Style::default()
    }));
    let nested_child = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style {
            width: Length::MaxContent,
            height: Length::MaxContent,
            ..Style::default()
        },
        Size::new(8.0, 6.0),
        4.0,
    ));
    tree.append_child(nested, nested_child);
    tree.append_child(root, reference);
    tree.append_child(root, nested);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[nested].layout.baseline.unwrap(), 4.0);
    assert_close(size.height, 31.0);
    assert_close(tree.nodes[reference].layout.offset.y, 0.0);
    assert_close(tree.nodes[nested].layout.offset.y, 21.0);
    assert_close(tree.nodes[nested_child].layout.offset.y, 0.0);
}

#[test]
fn flex_row_baseline_can_expand_auto_cross_size_for_bottom_margin() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(100.0),
        align_items: AlignItems::Baseline,
        ..Style::default()
    }));
    let tall = fixed_flex_child(&mut tree, 10.0, 20.0);
    let bottom_heavy = tree.push(SimpleNode::new(Style {
        width: Length::points(10.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::ZERO,
            Length::ZERO,
            Length::ZERO,
            Length::points(100.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, tall);
    tree.append_child(root, bottom_heavy);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.height, 120.0);
    assert_close(tree.nodes[tall].layout.offset.y, 0.0);
    assert_close(tree.nodes[bottom_heavy].layout.offset.y, 10.0);
}

#[test]
fn flex_cross_size_hypothetical_cross_layout_uses_used_main_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(100.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let measured = tree.push(SimpleNode::with_measure_func(
        Style {
            flex_basis: Length::points(20.0),
            flex_grow: 1.0,
            ..Style::default()
        },
        measure_height_from_width,
    ));
    tree.append_child(root, measured);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.height, 25.0);
    assert_close(tree.nodes[measured].layout.size.width, 100.0);
    assert_close(tree.nodes[measured].layout.size.height, 25.0);
}

#[test]
fn flex_cross_size_baseline_line_size_uses_largest_baseline_distances() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::Baseline,
        width: Length::points(60.0),
        ..Style::default()
    }));
    let high_baseline = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(20.0, 10.0),
        8.0,
    ));
    let deep_descent = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(20.0, 20.0),
        4.0,
    ));
    tree.append_child(root, high_baseline);
    tree.append_child(root, deep_descent);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::definite(60.0), SideConstraint::indefinite()),
    );

    assert_close(size.height, 24.0);
    assert_close(tree.nodes[high_baseline].layout.offset.y, 0.0);
    assert_close(tree.nodes[deep_descent].layout.offset.y, 4.0);
}

#[test]
fn align_content_centers_wrapped_lines_in_cross_axis() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(70.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 30.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 70.0));

    assert_close(tree.nodes[first].layout.offset.y, 25.0);
    assert_close(tree.nodes[second].layout.offset.y, 35.0);
}

#[test]
fn align_content_stretch_expands_wrapped_line_cross_sizes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::Stretch,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(50.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 30.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 50.0));

    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 25.0);
}

#[test]
fn stretched_flex_item_relayouts_percent_height_child_with_definite_cross_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::Stretch,
        width: Length::points(80.0),
        height: Length::points(40.0),
        ..Style::default()
    }));
    let stretched = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        flex_basis: Length::points(20.0),
        width: Length::points(20.0),
        height: Length::Auto,
        ..Style::default()
    }));
    let percent_child = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(10.0),
        height: Length::percent(50.0),
        ..Style::default()
    }));
    tree.append_child(root, stretched);
    tree.append_child(stretched, percent_child);

    run_rust_layout(&mut tree, root, Constraints::definite(80.0, 40.0));

    assert_close(tree.nodes[stretched].layout.size.height, 40.0);
    assert_close(tree.nodes[percent_child].layout.size.height, 20.0);
}

#[test]
fn definite_flex_basis_post_flexing_main_size_defines_descendant_percent_flex_basis_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        height: Length::points(22.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_basis: Length::points(10.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(16.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let percent = tree.push(SimpleNode::new(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(6.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    }));
    let sibling = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(12.0),
        flex_shrink: 0.0,
        width: Length::points(12.0),
        height: Length::points(9.0),
        ..Style::default()
    }));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[root].layout.size.width, 22.0);
    assert_close(tree.nodes[child].layout.size.width, 10.0);
    assert_close(tree.nodes[percent].layout.size.width, 5.0);
    assert_close(tree.nodes[fixed].layout.size.width, 5.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 5.0);
    assert_close(tree.nodes[sibling].layout.offset.x, 10.0);
}

#[test]
fn definite_container_main_size_defines_auto_basis_item_descendant_percent_flex_basis_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(20.0),
        height: Length::points(22.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_grow: 1.0,
        flex_shrink: 0.0,
        height: Length::points(16.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let percent = tree.push(SimpleNode::new(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        height: Length::points(6.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    }));
    let sibling = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(5.0),
        flex_shrink: 0.0,
        width: Length::points(5.0),
        height: Length::points(9.0),
        ..Style::default()
    }));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 22.0));

    assert_close(tree.nodes[root].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.size.width, 15.0);
    assert_close(tree.nodes[percent].layout.size.width, 10.0);
    assert_close(tree.nodes[fixed].layout.size.width, 5.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 10.0);
    assert_close(tree.nodes[sibling].layout.offset.x, 15.0);
}

#[test]
fn unresolved_percent_flex_basis_does_not_define_descendant_percent_flex_basis_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        height: Length::points(22.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        height: Length::points(16.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let percent = tree.push(SimpleNode::new(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 1.0,
        flex_shrink: 0.0,
        height: Length::points(6.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(5.0),
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    }));
    let sibling = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(12.0),
        flex_shrink: 0.0,
        width: Length::points(12.0),
        height: Length::points(9.0),
        ..Style::default()
    }));
    tree.append_child(root, child);
    tree.append_child(root, sibling);
    tree.append_child(child, percent);
    tree.append_child(child, fixed);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[root].layout.size.width, 17.0);
    assert_close(tree.nodes[child].layout.size.width, 5.0);
    assert_close(tree.nodes[percent].layout.size.width, 0.0);
    assert_close(tree.nodes[fixed].layout.size.width, 5.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 0.0);
    assert_close(tree.nodes[sibling].layout.offset.x, 5.0);
}

#[test]
fn resolved_flex_line_cross_size_defines_nested_flex_percent_basis_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::Stretch,
        width: Length::points(40.0),
        ..Style::default()
    }));
    let row_flex = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Row,
        flex_basis: Length::points(12.0),
        height: Length::points(12.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let percent_basis = tree.push(SimpleNode::new(Style {
        flex_basis: Length::percent(50.0),
        flex_grow: 0.0,
        flex_shrink: 0.0,
        width: Length::points(8.0),
        height: Length::points(6.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(5.0),
        height: Length::points(6.0),
        ..Style::default()
    }));
    tree.append_child(root, row_flex);
    tree.append_child(row_flex, percent_basis);
    tree.append_child(row_flex, fixed);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[root].layout.size.width, 40.0);
    assert_close(tree.nodes[row_flex].layout.size.width, 40.0);
    assert_close(tree.nodes[percent_basis].layout.size.width, 20.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 20.0);
}

#[test]
fn stretched_flex_item_cross_size_respects_min_max_constraints() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::Stretch,
        width: Length::points(120.0),
        height: Length::points(60.0),
        ..Style::default()
    }));
    let capped = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        flex_basis: Length::points(20.0),
        width: Length::points(20.0),
        height: Length::Auto,
        max_height: Length::points(35.0),
        ..Style::default()
    }));
    let floored = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        flex_basis: Length::points(20.0),
        width: Length::points(20.0),
        height: Length::Auto,
        min_height: Length::points(75.0),
        ..Style::default()
    }));
    tree.append_child(root, capped);
    tree.append_child(root, floored);

    run_rust_layout(&mut tree, root, Constraints::definite(120.0, 60.0));

    assert_close(tree.nodes[capped].layout.size.height, 35.0);
    assert_close(tree.nodes[floored].layout.size.height, 75.0);
    assert_close(tree.nodes[capped].layout.offset.y, 0.0);
    assert_close(tree.nodes[floored].layout.offset.y, 0.0);
}

#[test]
fn flex_visibility_collapse_preserves_line_cross_strut_and_removes_main_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(200.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let collapsed = tree.push(SimpleNode::new(Style {
        width: Length::points(100.0),
        height: Length::points(80.0),
        visibility: Visibility::Collapse,
        ..Style::default()
    }));
    let visible = tree.push(SimpleNode::new(Style {
        width: Length::points(40.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    tree.append_child(root, collapsed);
    tree.append_child(root, visible);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(200.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.height, 80.0);
    assert_close(tree.nodes[visible].layout.offset.x, 0.0);
    assert_close(tree.nodes[visible].layout.size.width, 40.0);
    assert_close(tree.nodes[collapsed].layout.size.width, 0.0);
    assert_close(tree.nodes[collapsed].layout.size.height, 0.0);
}

#[test]
fn flex_visibility_collapse_restarts_line_collection_with_zero_main_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(100.0),
        flex_wrap: FlexWrap::Wrap,
        align_items: AlignItems::FlexStart,
        align_content: AlignContent::FlexStart,
        ..Style::default()
    }));
    let collapsed = tree.push(SimpleNode::new(Style {
        width: Length::points(80.0),
        height: Length::points(50.0),
        visibility: Visibility::Collapse,
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        width: Length::points(60.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        width: Length::points(60.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, collapsed);
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::indefinite(),
        ),
    );

    assert_close(size.height, 60.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 50.0);
    assert_close(tree.nodes[collapsed].layout.size.width, 0.0);
}

#[test]
fn stretched_flex_item_with_aspect_ratio_keeps_flexed_main_size_and_uses_line_cross_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::Stretch,
        width: Length::points(100.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let stretched = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        flex_basis: Length::points(40.0),
        height: Length::Auto,
        aspect_ratio: Some(2.0),
        ..Style::default()
    }));
    tree.append_child(root, stretched);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 80.0));

    assert_close(tree.nodes[stretched].layout.size.width, 40.0);
    assert_close(tree.nodes[stretched].layout.size.height, 80.0);
}

#[test]
fn align_items_stretch_only_stretches_auto_cross_size_items_without_auto_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::Stretch,
        width: Length::points(120.0),
        height: Length::points(40.0),
        ..Style::default()
    }));
    let auto_cross = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        width: Length::points(20.0),
        height: Length::Auto,
        ..Style::default()
    }));
    let definite_cross = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        width: Length::points(20.0),
        height: Length::points(12.0),
        ..Style::default()
    }));
    let auto_margin_cross = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        width: Length::points(20.0),
        height: Length::Auto,
        margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::ZERO),
        ..Style::default()
    }));
    tree.append_child(root, auto_cross);
    tree.append_child(root, definite_cross);
    tree.append_child(root, auto_margin_cross);

    run_rust_layout(&mut tree, root, Constraints::definite(120.0, 40.0));

    assert_close(tree.nodes[auto_cross].layout.size.height, 40.0);
    assert_close(tree.nodes[definite_cross].layout.size.height, 12.0);
    assert_close(tree.nodes[auto_margin_cross].layout.size.height, 0.0);
    assert_close(tree.nodes[auto_margin_cross].layout.offset.y, 40.0);
}

#[test]
fn align_content_start_end_alias_flex_edges_for_wrapped_lines() {
    for (align_content, first_y, second_y) in [
        (AlignContent::Start, 0.0, 10.0),
        (AlignContent::End, 50.0, 60.0),
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            flex_wrap: FlexWrap::Wrap,
            align_content,
            align_items: AlignItems::FlexStart,
            width: Length::points(50.0),
            height: Length::points(70.0),
            ..Style::default()
        }));
        let first = fixed_flex_child(&mut tree, 30.0, 10.0);
        let second = fixed_flex_child(&mut tree, 30.0, 10.0);
        tree.append_child(root, first);
        tree.append_child(root, second);

        run_rust_layout(&mut tree, root, Constraints::definite(50.0, 70.0));

        assert_close(tree.nodes[first].layout.offset.y, first_y);
        assert_close(tree.nodes[second].layout.offset.y, second_y);
    }
}

#[test]
fn flex_wrap_reverse_places_first_line_at_cross_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::WrapReverse,
        align_content: AlignContent::FlexStart,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(70.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 30.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 70.0));

    assert_close(tree.nodes[first].layout.offset.y, 60.0);
    assert_close(tree.nodes[second].layout.offset.y, 50.0);
}

#[test]
fn flex_wrap_reverse_reverses_space_between_line_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::WrapReverse,
        align_content: AlignContent::SpaceBetween,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(70.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 30.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 70.0));

    assert_close(tree.nodes[first].layout.offset.y, 60.0);
    assert_close(tree.nodes[second].layout.offset.y, 0.0);
}

#[test]
fn flex_wrap_reverse_stretched_line_uses_reversed_cross_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::RowReverse,
        flex_wrap: FlexWrap::WrapReverse,
        justify_content: JustifyContent::FlexEnd,
        align_items: AlignItems::FlexEnd,
        width: Length::points(120.0),
        height: Length::points(82.0),
        padding: Rect::new(
            Length::points(5.0),
            Length::points(7.0),
            Length::points(3.0),
            Length::points(4.0),
        ),
        border: Rect::new(2.0, 1.0, 3.0, 1.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            width: Length::points(18.0),
            height: Length::points(11.0),
            ..Style::default()
        },
        Size::new(18.0, 11.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(180.0, 120.0));

    assert_close(tree.nodes[child].layout.offset.x, 7.0);
    assert_close(tree.nodes[child].layout.offset.y, 6.0);
}

#[test]
fn single_line_align_content_does_not_pack_the_line() {
    for align_content in [
        AlignContent::FlexEnd,
        AlignContent::End,
        AlignContent::Center,
        AlignContent::SpaceBetween,
        AlignContent::SpaceAround,
        AlignContent::SpaceEvenly,
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            flex_wrap: FlexWrap::NoWrap,
            align_content,
            align_items: AlignItems::FlexStart,
            width: Length::points(50.0),
            height: Length::points(70.0),
            ..Style::default()
        }));
        let child = fixed_flex_child(&mut tree, 10.0, 10.0);
        tree.append_child(root, child);

        run_rust_layout(&mut tree, root, Constraints::definite(50.0, 70.0));

        assert_close(tree.nodes[child].layout.offset.y, 0.0);
    }
}

#[test]
fn main_axis_auto_margin_consumes_remaining_space_before_justify_content() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: starlight_layout::JustifyContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let leading = fixed_flex_child(&mut tree, 10.0, 10.0);
    let trailing = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(10.0),
        margin: Rect::new(Length::Auto, Length::ZERO, Length::ZERO, Length::ZERO),
        ..Style::default()
    }));
    tree.append_child(root, leading);
    tree.append_child(root, trailing);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[leading].layout.offset.x, 0.0);
    assert_close(tree.nodes[trailing].layout.offset.x, 90.0);
}

#[test]
fn paired_main_axis_auto_margins_center_item() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(Length::Auto, Length::Auto, Length::ZERO, Length::ZERO),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[child].layout.offset.x, 40.0);
}

#[test]
fn multiple_main_axis_auto_margins_share_positive_free_space_before_justify_content() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(Length::Auto, Length::Auto, Length::ZERO, Length::ZERO),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        height: Length::points(10.0),
        margin: Rect::new(Length::Auto, Length::ZERO, Length::ZERO, Length::ZERO),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 20.0);
    assert_close(tree.nodes[second].layout.offset.x, 80.0);
}

#[test]
fn main_axis_auto_margin_without_positive_free_space_zeroes_margins_then_justify_content() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        margin: Rect::new(Length::Auto, Length::ZERO, Length::ZERO, Length::ZERO),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 10.0));

    assert_close(tree.nodes[first].layout.margin.left, 0.0);
    assert_close(tree.nodes[first].layout.offset.x, -5.0);
    assert_close(tree.nodes[second].layout.offset.x, 25.0);
}

#[derive(Clone, Copy, Debug)]
struct MainAxisDirectionCase {
    flex_direction: FlexDirection,
    direction: Direction,
}

impl MainAxisDirectionCase {
    fn is_row(self) -> bool {
        self.flex_direction.is_row()
    }

    fn reverse_main(self) -> bool {
        if self.is_row() {
            self.flex_direction.is_reverse() ^ self.direction.is_rtl()
        } else {
            self.flex_direction.is_reverse()
        }
    }

    fn name(self) -> String {
        format!("{:?}/{:?}", self.flex_direction, self.direction)
    }
}

#[derive(Clone, Copy, Debug)]
struct JustifyMatrixCase {
    justify_content: JustifyContent,
    logical_offsets: [f32; 2],
}

const MAIN_AXIS_MATRIX: [MainAxisDirectionCase; 8] = [
    MainAxisDirectionCase {
        flex_direction: FlexDirection::Row,
        direction: Direction::Ltr,
    },
    MainAxisDirectionCase {
        flex_direction: FlexDirection::Row,
        direction: Direction::Rtl,
    },
    MainAxisDirectionCase {
        flex_direction: FlexDirection::RowReverse,
        direction: Direction::Ltr,
    },
    MainAxisDirectionCase {
        flex_direction: FlexDirection::RowReverse,
        direction: Direction::Rtl,
    },
    MainAxisDirectionCase {
        flex_direction: FlexDirection::Column,
        direction: Direction::Ltr,
    },
    MainAxisDirectionCase {
        flex_direction: FlexDirection::Column,
        direction: Direction::Rtl,
    },
    MainAxisDirectionCase {
        flex_direction: FlexDirection::ColumnReverse,
        direction: Direction::Ltr,
    },
    MainAxisDirectionCase {
        flex_direction: FlexDirection::ColumnReverse,
        direction: Direction::Rtl,
    },
];

const JUSTIFY_MATRIX: [JustifyMatrixCase; 9] = [
    JustifyMatrixCase {
        justify_content: JustifyContent::Stretch,
        logical_offsets: [0.0, 10.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::FlexStart,
        logical_offsets: [0.0, 10.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::Start,
        logical_offsets: [0.0, 10.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::Center,
        logical_offsets: [40.0, 50.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::FlexEnd,
        logical_offsets: [80.0, 90.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::End,
        logical_offsets: [80.0, 90.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::SpaceBetween,
        logical_offsets: [0.0, 90.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::SpaceAround,
        logical_offsets: [20.0, 70.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::SpaceEvenly,
        logical_offsets: [27.0, 63.0],
    },
];

fn expected_main_offsets(case: MainAxisDirectionCase, logical_offsets: [f32; 2]) -> [f32; 2] {
    expected_sized_main_offsets(case, 100.0, 10.0, logical_offsets)
}

fn expected_sized_main_offsets(
    case: MainAxisDirectionCase,
    container_main_size: f32,
    item_main_size: f32,
    logical_offsets: [f32; 2],
) -> [f32; 2] {
    if case.reverse_main() {
        [
            container_main_size - item_main_size - logical_offsets[0],
            container_main_size - item_main_size - logical_offsets[1],
        ]
    } else {
        logical_offsets
    }
}

fn expected_variable_main_offsets(
    case: MainAxisDirectionCase,
    container_main_size: f32,
    item_main_sizes: [f32; 2],
    logical_offsets: [f32; 2],
) -> [f32; 2] {
    if case.reverse_main() {
        [
            container_main_size - item_main_sizes[0] - logical_offsets[0],
            container_main_size - item_main_sizes[1] - logical_offsets[1],
        ]
    } else {
        logical_offsets
    }
}

fn main_axis_offset(layout: LayoutResult, is_row: bool) -> f32 {
    if is_row {
        layout.offset.x
    } else {
        layout.offset.y
    }
}

fn main_axis_size(layout: LayoutResult, is_row: bool) -> f32 {
    if is_row {
        layout.size.width
    } else {
        layout.size.height
    }
}

fn fixed_matrix_flex_child(tree: &mut SimpleTree) -> usize {
    tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        width: Length::points(10.0),
        height: Length::points(10.0),
        ..Style::default()
    }))
}

fn fixed_main_axis_matrix_child(
    tree: &mut SimpleTree,
    case: MainAxisDirectionCase,
    main_size: f32,
    cross_size: f32,
) -> usize {
    tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(main_size),
        flex_shrink: 0.0,
        width: Length::points(if case.is_row() { main_size } else { cross_size }),
        height: Length::points(if case.is_row() { cross_size } else { main_size }),
        ..Style::default()
    }))
}

#[derive(Clone, Copy, Debug)]
struct AlignCrossMatrixCase {
    align_items: AlignItems,
    logical_border_offset: f32,
}

const CROSS_WRAP_MATRIX: [FlexWrap; 2] = [FlexWrap::NoWrap, FlexWrap::WrapReverse];

const ALIGN_CROSS_MATRIX: [AlignCrossMatrixCase; 6] = [
    AlignCrossMatrixCase {
        align_items: AlignItems::Stretch,
        logical_border_offset: 0.0,
    },
    AlignCrossMatrixCase {
        align_items: AlignItems::FlexStart,
        logical_border_offset: 0.0,
    },
    AlignCrossMatrixCase {
        align_items: AlignItems::Start,
        logical_border_offset: 0.0,
    },
    AlignCrossMatrixCase {
        align_items: AlignItems::Center,
        logical_border_offset: 40.0,
    },
    AlignCrossMatrixCase {
        align_items: AlignItems::FlexEnd,
        logical_border_offset: 80.0,
    },
    AlignCrossMatrixCase {
        align_items: AlignItems::End,
        logical_border_offset: 80.0,
    },
];

impl MainAxisDirectionCase {
    fn reverse_cross(self, flex_wrap: FlexWrap) -> bool {
        (!self.is_row() && self.direction.is_rtl()) ^ (flex_wrap == FlexWrap::WrapReverse)
    }
}

fn cross_axis_offset(layout: LayoutResult, case: MainAxisDirectionCase) -> f32 {
    if case.is_row() {
        layout.offset.y
    } else {
        layout.offset.x
    }
}

fn logical_cross_margin(margin: Edges, case: MainAxisDirectionCase, flex_wrap: FlexWrap) -> f32 {
    if case.is_row() {
        if case.reverse_cross(flex_wrap) {
            margin.bottom
        } else {
            margin.top
        }
    } else if case.reverse_cross(flex_wrap) {
        margin.right
    } else {
        margin.left
    }
}

fn logical_cross_end_margin(
    margin: Edges,
    case: MainAxisDirectionCase,
    flex_wrap: FlexWrap,
) -> f32 {
    if case.is_row() {
        if case.reverse_cross(flex_wrap) {
            margin.top
        } else {
            margin.bottom
        }
    } else if case.reverse_cross(flex_wrap) {
        margin.left
    } else {
        margin.right
    }
}

fn cross_axis_auto_margin(
    case: MainAxisDirectionCase,
    flex_wrap: FlexWrap,
    start_auto: bool,
    end_auto: bool,
) -> Rect<Length> {
    let mut margin = Rect::new(Length::ZERO, Length::ZERO, Length::ZERO, Length::ZERO);
    let start = if start_auto {
        Length::Auto
    } else {
        Length::ZERO
    };
    let end = if end_auto { Length::Auto } else { Length::ZERO };
    if case.is_row() {
        if case.reverse_cross(flex_wrap) {
            margin.bottom = start;
            margin.top = end;
        } else {
            margin.top = start;
            margin.bottom = end;
        }
    } else if case.reverse_cross(flex_wrap) {
        margin.right = start;
        margin.left = end;
    } else {
        margin.left = start;
        margin.right = end;
    }
    margin
}

fn fixed_cross_axis_matrix_child(tree: &mut SimpleTree, case: MainAxisDirectionCase) -> usize {
    fixed_cross_axis_sized_matrix_child(tree, case, 20.0)
}

fn fixed_cross_axis_sized_matrix_child(
    tree: &mut SimpleTree,
    case: MainAxisDirectionCase,
    cross_size: f32,
) -> usize {
    tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        flex_shrink: 0.0,
        width: Length::points(if case.is_row() { 10.0 } else { cross_size }),
        height: Length::points(if case.is_row() { cross_size } else { 10.0 }),
        ..Style::default()
    }))
}

fn expected_cross_offset(
    case: MainAxisDirectionCase,
    flex_wrap: FlexWrap,
    logical_border_offset: f32,
    child_cross_size: f32,
) -> f32 {
    if case.reverse_cross(flex_wrap) {
        100.0 - child_cross_size - logical_border_offset
    } else {
        logical_border_offset
    }
}

#[test]
fn align_items_cross_axis_direction_and_wrap_reverse_matrix_places_items() {
    for direction_case in MAIN_AXIS_MATRIX {
        for flex_wrap in CROSS_WRAP_MATRIX {
            for align_case in ALIGN_CROSS_MATRIX {
                let mut tree = SimpleTree::default();
                let root = tree.push(SimpleNode::new(Style {
                    display: Display::Flex,
                    flex_direction: direction_case.flex_direction,
                    direction: direction_case.direction,
                    flex_wrap,
                    align_items: align_case.align_items,
                    width: Length::points(100.0),
                    height: Length::points(100.0),
                    ..Style::default()
                }));
                let child = fixed_cross_axis_matrix_child(&mut tree, direction_case);
                tree.append_child(root, child);

                run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

                let expected = expected_cross_offset(
                    direction_case,
                    flex_wrap,
                    align_case.logical_border_offset,
                    20.0,
                );
                let name = format!(
                    "{} {:?} {:?}",
                    direction_case.name(),
                    flex_wrap,
                    align_case.align_items
                );
                assert_close_named(
                    &name,
                    cross_axis_offset(tree.nodes[child].layout, direction_case),
                    expected,
                );
            }
        }
    }
}

#[test]
fn cross_axis_auto_margin_direction_and_wrap_reverse_matrix_resolves_margins() {
    for direction_case in MAIN_AXIS_MATRIX {
        for flex_wrap in CROSS_WRAP_MATRIX {
            for (start_auto, end_auto, logical_offset, start_margin, end_margin) in [
                (true, false, 80.0, 80.0, 0.0),
                (false, true, 0.0, 0.0, 80.0),
                (true, true, 40.0, 40.0, 40.0),
            ] {
                let mut tree = SimpleTree::default();
                let root = tree.push(SimpleNode::new(Style {
                    display: Display::Flex,
                    flex_direction: direction_case.flex_direction,
                    direction: direction_case.direction,
                    flex_wrap,
                    align_items: AlignItems::FlexEnd,
                    width: Length::points(100.0),
                    height: Length::points(100.0),
                    ..Style::default()
                }));
                let child = tree.push(SimpleNode::new(Style {
                    flex_basis: Length::points(10.0),
                    flex_shrink: 0.0,
                    width: Length::points(if direction_case.is_row() { 10.0 } else { 20.0 }),
                    height: Length::points(if direction_case.is_row() { 20.0 } else { 10.0 }),
                    margin: cross_axis_auto_margin(direction_case, flex_wrap, start_auto, end_auto),
                    ..Style::default()
                }));
                tree.append_child(root, child);

                run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

                let expected =
                    expected_cross_offset(direction_case, flex_wrap, logical_offset, 20.0);
                let name = format!(
                    "{} {:?} auto-start={} auto-end={}",
                    direction_case.name(),
                    flex_wrap,
                    start_auto,
                    end_auto
                );
                assert_close_named(
                    &format!("{name} offset"),
                    cross_axis_offset(tree.nodes[child].layout, direction_case),
                    expected,
                );
                assert_close_named(
                    &format!("{name} start margin"),
                    logical_cross_margin(
                        tree.nodes[child].layout.margin,
                        direction_case,
                        flex_wrap,
                    ),
                    start_margin,
                );
                assert_close_named(
                    &format!("{name} end margin"),
                    logical_cross_end_margin(
                        tree.nodes[child].layout.margin,
                        direction_case,
                        flex_wrap,
                    ),
                    end_margin,
                );
            }
        }
    }
}

#[test]
fn overflowing_cross_axis_auto_margin_direction_matrix_overflows_cross_end() {
    for direction_case in MAIN_AXIS_MATRIX {
        let flex_wrap = FlexWrap::NoWrap;
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            flex_direction: direction_case.flex_direction,
            direction: direction_case.direction,
            flex_wrap,
            align_items: AlignItems::FlexEnd,
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        }));
        let child = tree.push(SimpleNode::new(Style {
            flex_basis: Length::points(10.0),
            flex_shrink: 0.0,
            width: Length::points(if direction_case.is_row() { 10.0 } else { 120.0 }),
            height: Length::points(if direction_case.is_row() { 120.0 } else { 10.0 }),
            margin: cross_axis_auto_margin(direction_case, flex_wrap, true, true),
            ..Style::default()
        }));
        tree.append_child(root, child);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

        let expected = expected_cross_offset(direction_case, flex_wrap, 0.0, 120.0);
        let name = direction_case.name();
        assert_close_named(
            &format!("{name} overflow offset"),
            cross_axis_offset(tree.nodes[child].layout, direction_case),
            expected,
        );
        assert_close_named(
            &format!("{name} overflow start margin"),
            logical_cross_margin(tree.nodes[child].layout.margin, direction_case, flex_wrap),
            0.0,
        );
        assert_close_named(
            &format!("{name} overflow end margin"),
            logical_cross_end_margin(tree.nodes[child].layout.margin, direction_case, flex_wrap),
            -20.0,
        );
    }
}

#[test]
fn justify_content_main_axis_direction_matrix_places_items() {
    for direction_case in MAIN_AXIS_MATRIX {
        for justify_case in JUSTIFY_MATRIX {
            let mut tree = SimpleTree::default();
            let root = tree.push(SimpleNode::new(Style {
                display: Display::Flex,
                flex_direction: direction_case.flex_direction,
                direction: direction_case.direction,
                justify_content: justify_case.justify_content,
                align_items: AlignItems::FlexStart,
                width: Length::points(100.0),
                height: Length::points(100.0),
                ..Style::default()
            }));
            let first = fixed_matrix_flex_child(&mut tree);
            let second = fixed_matrix_flex_child(&mut tree);
            tree.append_child(root, first);
            tree.append_child(root, second);

            run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

            let expected = expected_main_offsets(direction_case, justify_case.logical_offsets);
            let name = format!(
                "{} {:?}",
                direction_case.name(),
                justify_case.justify_content
            );
            assert_close_named(
                &format!("{name} first"),
                main_axis_offset(tree.nodes[first].layout, direction_case.is_row()),
                expected[0],
            );
            assert_close_named(
                &format!("{name} second"),
                main_axis_offset(tree.nodes[second].layout, direction_case.is_row()),
                expected[1],
            );
        }
    }
}

const NEGATIVE_FREE_SPACE_JUSTIFY_MATRIX: [JustifyMatrixCase; 8] = [
    JustifyMatrixCase {
        justify_content: JustifyContent::Stretch,
        logical_offsets: [0.0, 30.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::FlexStart,
        logical_offsets: [0.0, 30.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::Start,
        logical_offsets: [0.0, 30.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::Center,
        logical_offsets: [-10.0, 20.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::FlexEnd,
        logical_offsets: [-20.0, 10.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::End,
        logical_offsets: [-20.0, 10.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::SpaceBetween,
        logical_offsets: [0.0, 30.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::SpaceAround,
        logical_offsets: [-10.0, 20.0],
    },
];

#[test]
fn justify_content_negative_free_space_direction_matrix_uses_w3c_fallbacks() {
    for direction_case in MAIN_AXIS_MATRIX {
        for justify_case in NEGATIVE_FREE_SPACE_JUSTIFY_MATRIX {
            let mut tree = SimpleTree::default();
            let root = tree.push(SimpleNode::new(Style {
                display: Display::Flex,
                flex_direction: direction_case.flex_direction,
                direction: direction_case.direction,
                justify_content: justify_case.justify_content,
                align_items: AlignItems::FlexStart,
                width: Length::points(40.0),
                height: Length::points(40.0),
                ..Style::default()
            }));
            let first = fixed_main_axis_matrix_child(&mut tree, direction_case, 30.0, 10.0);
            let second = fixed_main_axis_matrix_child(&mut tree, direction_case, 30.0, 10.0);
            tree.append_child(root, first);
            tree.append_child(root, second);

            run_rust_layout(&mut tree, root, Constraints::definite(40.0, 40.0));

            let expected = expected_sized_main_offsets(
                direction_case,
                40.0,
                30.0,
                justify_case.logical_offsets,
            );
            let name = format!(
                "{} {:?} negative free-space",
                direction_case.name(),
                justify_case.justify_content
            );
            assert_close_named(
                &format!("{name} first"),
                main_axis_offset(tree.nodes[first].layout, direction_case.is_row()),
                expected[0],
            );
            assert_close_named(
                &format!("{name} second"),
                main_axis_offset(tree.nodes[second].layout, direction_case.is_row()),
                expected[1],
            );
        }
    }
}

const GAP_OVERFLOW_JUSTIFY_MATRIX: [JustifyMatrixCase; 3] = [
    JustifyMatrixCase {
        justify_content: JustifyContent::Center,
        logical_offsets: [-10.0, 30.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::SpaceBetween,
        logical_offsets: [0.0, 40.0],
    },
    JustifyMatrixCase {
        justify_content: JustifyContent::SpaceAround,
        logical_offsets: [-10.0, 30.0],
    },
];

#[test]
fn justify_content_gap_overflow_direction_matrix_preserves_gap_after_fallback() {
    for direction_case in MAIN_AXIS_MATRIX {
        for justify_case in GAP_OVERFLOW_JUSTIFY_MATRIX {
            let mut tree = SimpleTree::default();
            let root = tree.push(SimpleNode::new(Style {
                display: Display::Flex,
                flex_direction: direction_case.flex_direction,
                direction: direction_case.direction,
                justify_content: justify_case.justify_content,
                align_items: AlignItems::FlexStart,
                width: Length::points(50.0),
                height: Length::points(50.0),
                row_gap: Length::points(10.0),
                column_gap: Length::points(10.0),
                ..Style::default()
            }));
            let first = fixed_main_axis_matrix_child(&mut tree, direction_case, 30.0, 10.0);
            let second = fixed_main_axis_matrix_child(&mut tree, direction_case, 30.0, 10.0);
            tree.append_child(root, first);
            tree.append_child(root, second);

            run_rust_layout(&mut tree, root, Constraints::definite(50.0, 50.0));

            let expected = expected_sized_main_offsets(
                direction_case,
                50.0,
                30.0,
                justify_case.logical_offsets,
            );
            let name = format!(
                "{} {:?} gap overflow",
                direction_case.name(),
                justify_case.justify_content
            );
            assert_close_named(
                &format!("{name} first"),
                main_axis_offset(tree.nodes[first].layout, direction_case.is_row()),
                expected[0],
            );
            assert_close_named(
                &format!("{name} second"),
                main_axis_offset(tree.nodes[second].layout, direction_case.is_row()),
                expected[1],
            );
        }
    }
}

fn main_start_auto_margin(case: MainAxisDirectionCase) -> Rect<Length> {
    match case.flex_direction {
        FlexDirection::Row => {
            if case.direction.is_rtl() {
                Rect::new(Length::ZERO, Length::Auto, Length::ZERO, Length::ZERO)
            } else {
                Rect::new(Length::Auto, Length::ZERO, Length::ZERO, Length::ZERO)
            }
        }
        FlexDirection::RowReverse => {
            if case.direction.is_rtl() {
                Rect::new(Length::Auto, Length::ZERO, Length::ZERO, Length::ZERO)
            } else {
                Rect::new(Length::ZERO, Length::Auto, Length::ZERO, Length::ZERO)
            }
        }
        FlexDirection::Column => Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::ZERO),
        FlexDirection::ColumnReverse => {
            Rect::new(Length::ZERO, Length::ZERO, Length::ZERO, Length::Auto)
        }
    }
}

#[test]
fn main_axis_auto_margin_direction_matrix_consumes_free_space() {
    for direction_case in MAIN_AXIS_MATRIX {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            flex_direction: direction_case.flex_direction,
            direction: direction_case.direction,
            justify_content: JustifyContent::Center,
            align_items: AlignItems::FlexStart,
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        }));
        let first = fixed_matrix_flex_child(&mut tree);
        let second = tree.push(SimpleNode::new(Style {
            flex_basis: Length::points(10.0),
            width: Length::points(10.0),
            height: Length::points(10.0),
            margin: main_start_auto_margin(direction_case),
            ..Style::default()
        }));
        tree.append_child(root, first);
        tree.append_child(root, second);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

        let expected = expected_main_offsets(direction_case, [0.0, 90.0]);
        let name = direction_case.name();
        assert_close_named(
            &format!("{name} auto first"),
            main_axis_offset(tree.nodes[first].layout, direction_case.is_row()),
            expected[0],
        );
        assert_close_named(
            &format!("{name} auto second"),
            main_axis_offset(tree.nodes[second].layout, direction_case.is_row()),
            expected[1],
        );
    }
}

#[test]
fn cross_axis_auto_margin_overrides_stretch_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::Stretch,
        width: Length::points(50.0),
        height: Length::points(30.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            flex_basis: Length::points(10.0),
            margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::ZERO),
            ..Style::default()
        },
        Size::new(10.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 30.0));

    assert_close(tree.nodes[child].layout.offset.y, 20.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn paired_cross_axis_auto_margins_center_item() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(20.0),
        margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::Auto),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.y, 40.0);
    assert_close(tree.nodes[child].layout.margin.top, 40.0);
    assert_close(tree.nodes[child].layout.margin.bottom, 40.0);
}

#[test]
fn single_cross_axis_auto_margins_absorb_positive_free_space() {
    for (margin, expected_y, expected_top, expected_bottom) in [
        (
            Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::ZERO),
            30.0,
            30.0,
            0.0,
        ),
        (
            Rect::new(Length::ZERO, Length::ZERO, Length::ZERO, Length::Auto),
            0.0,
            0.0,
            30.0,
        ),
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            align_items: AlignItems::Center,
            width: Length::points(50.0),
            height: Length::points(40.0),
            ..Style::default()
        }));
        let child = tree.push(SimpleNode::new(Style {
            flex_basis: Length::points(10.0),
            height: Length::points(10.0),
            margin,
            ..Style::default()
        }));
        tree.append_child(root, child);

        run_rust_layout(&mut tree, root, Constraints::definite(50.0, 40.0));

        assert_close(tree.nodes[child].layout.offset.y, expected_y);
        assert_close(tree.nodes[child].layout.margin.top, expected_top);
        assert_close(tree.nodes[child].layout.margin.bottom, expected_bottom);
    }
}

#[test]
fn overflowing_cross_axis_auto_margins_place_overflow_at_cross_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(50.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(70.0),
        margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::Auto),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 50.0));

    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.margin.top, 0.0);
    assert_close(tree.nodes[child].layout.margin.bottom, -20.0);
}

#[test]
fn border_box_min_width_freezes_flex_item_without_adding_padding_border() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        box_sizing: BoxSizing::BorderBox,
        flex_basis: Length::points(40.0),
        min_width: Length::points(30.0),
        height: Length::points(10.0),
        padding: Rect::new(
            Length::points(5.0),
            Length::points(5.0),
            Length::ZERO,
            Length::ZERO,
        ),
        border: Rect::new(1.0, 1.0, 0.0, 0.0),
        ..Style::default()
    }));
    let second = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 20.0));

    assert_close(tree.nodes[first].layout.size.width, 30.0);
    assert_close(tree.nodes[second].layout.offset.x, 30.0);
    assert_close(tree.nodes[second].layout.size.width, 20.0);
}

#[test]
fn border_box_max_width_caps_flex_grow_without_adding_padding_border() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let capped = tree.push(SimpleNode::new(Style {
        box_sizing: BoxSizing::BorderBox,
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::points(30.0),
        height: Length::points(10.0),
        padding: Rect::new(
            Length::points(5.0),
            Length::points(5.0),
            Length::ZERO,
            Length::ZERO,
        ),
        border: Rect::new(1.0, 1.0, 0.0, 0.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[capped].layout.size.width, 30.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 30.0);
    assert_close(tree.nodes[flexible].layout.size.width, 70.0);
}

#[test]
fn flex_item_fit_content_width_uses_natural_main_axis_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(200.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::fit_content(Some(BaseLength::fixed(80.0))),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let grandchild = tree.push(SimpleNode::new(Style {
        width: Length::points(120.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 20.0));

    assert_close(tree.nodes[child].layout.size.width, 120.0);
    assert_close(tree.nodes[grandchild].layout.size.width, 120.0);
}

#[test]
fn column_flex_item_fit_content_height_uses_natural_main_axis_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(20.0),
        height: Length::points(200.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(10.0),
        height: Length::fit_content(Some(BaseLength::fixed(80.0))),
        ..Style::default()
    }));
    let grandchild = tree.push(SimpleNode::new(Style {
        width: Length::points(10.0),
        height: Length::points(120.0),
        ..Style::default()
    }));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 200.0));

    assert_close(tree.nodes[child].layout.size.height, 120.0);
    assert_close(tree.nodes[grandchild].layout.size.height, 120.0);
}

#[test]
fn flex_basis_fit_content_argument_resolves_before_measuring_item() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            flex_basis: Length::fit_content(Some(BaseLength::fixed(40.0))),
            height: Length::points(10.0),
            ..Style::default()
        },
        Size::new(80.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[child].layout.size.width, 40.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn flex_basis_fit_content_percent_argument_resolves_against_main_axis() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::with_measured_size(
        Style {
            flex_basis: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 50.0))),
            height: Length::points(10.0),
            ..Style::default()
        },
        Size::new(80.0, 10.0),
    ));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 55.0);
    assert_close(tree.nodes[second].layout.offset.x, 55.0);
}

#[test]
fn root_flex_fit_content_percent_argument_caps_final_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 20.0),
    );

    assert_close(size.width, 100.0);
    assert_close(tree.nodes[root].layout.size.width, 100.0);
    assert_close(tree.nodes[first].layout.size.width, 70.0);
    assert_close(tree.nodes[second].layout.size.width, 70.0);
    assert_close(tree.nodes[second].layout.offset.x, 70.0);
}

#[test]
fn root_flex_fit_content_calc_argument_caps_final_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 20.0),
    );

    assert_close(size.width, 110.0);
    assert_close(tree.nodes[root].layout.size.width, 110.0);
    assert_close(tree.nodes[first].layout.size.width, 70.0);
    assert_close(tree.nodes[second].layout.size.width, 70.0);
    assert_close(tree.nodes[second].layout.offset.x, 70.0);
}

#[test]
fn root_column_flex_fit_content_percent_argument_caps_final_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        width: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        width: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(20.0, 200.0),
    );

    assert_close(size.height, 100.0);
    assert_close(tree.nodes[root].layout.size.height, 100.0);
    assert_close(tree.nodes[first].layout.size.height, 70.0);
    assert_close(tree.nodes[second].layout.size.height, 70.0);
    assert_close(tree.nodes[second].layout.offset.y, 70.0);
}

#[test]
fn root_column_flex_fit_content_calc_argument_caps_final_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        width: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(70.0),
        flex_shrink: 0.0,
        width: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(20.0, 200.0),
    );

    assert_close(size.height, 110.0);
    assert_close(tree.nodes[root].layout.size.height, 110.0);
    assert_close(tree.nodes[first].layout.size.height, 70.0);
    assert_close(tree.nodes[second].layout.size.height, 70.0);
    assert_close(tree.nodes[second].layout.offset.y, 70.0);
}

#[test]
fn min_width_freezes_item_during_flex_shrink() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(40.0),
        min_width: Length::points(30.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 30.0);
    assert_close(tree.nodes[second].layout.size.width, 20.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 30.0);
}

#[test]
fn fit_content_min_width_freezes_item_during_flex_shrink() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(40.0),
        min_width: Length::fit_content(Some(BaseLength::fixed(30.0))),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 30.0);
    assert_close(tree.nodes[second].layout.size.width, 20.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 30.0);
}

#[test]
fn max_content_min_width_does_not_freeze_item_during_flex_shrink() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(40.0),
        min_width: Length::MaxContent,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = fixed_flex_child(&mut tree, 40.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 25.0);
    assert_close(tree.nodes[second].layout.size.width, 25.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 25.0);
}

#[test]
fn percent_min_width_freezes_item_during_flex_shrink() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(80.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(60.0),
        min_width: Length::percent(50.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = fixed_flex_child(&mut tree, 60.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(80.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 40.0);
    assert_close(tree.nodes[second].layout.size.width, 40.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 40.0);
}

#[test]
fn min_width_above_flex_basis_freezes_shrinking_item_to_hypothetical_main_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let frozen = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_shrink: 1.0,
        min_width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(80.0),
        flex_shrink: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, frozen);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[frozen].layout.size.width, 50.0);
    assert_close(tree.nodes[flexible].layout.size.width, 50.0);
    assert_close(tree.nodes[frozen].layout.offset.x, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 50.0);
}

#[test]
fn multiple_min_width_violations_freeze_before_redistributing_flex_shrink_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(180.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(100.0),
        min_width: Length::points(80.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(100.0),
        min_width: Length::points(70.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let third = fixed_flex_child(&mut tree, 100.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);

    run_rust_layout(&mut tree, root, Constraints::definite(180.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 80.0);
    assert_close(tree.nodes[second].layout.size.width, 70.0);
    assert_close(tree.nodes[third].layout.size.width, 30.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 80.0);
    assert_close(tree.nodes[third].layout.offset.x, 150.0);
}

#[test]
fn max_width_freezes_item_and_redistributes_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let capped = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::points(30.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[capped].layout.size.width, 30.0);
    assert_close(tree.nodes[flexible].layout.size.width, 70.0);
    assert_close(tree.nodes[capped].layout.offset.x, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 30.0);
}

#[test]
fn max_width_below_flex_basis_freezes_growing_item_to_hypothetical_main_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(140.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let capped = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(80.0),
        flex_grow: 1.0,
        max_width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(140.0, 10.0));

    assert_close(tree.nodes[capped].layout.size.width, 50.0);
    assert_close(tree.nodes[flexible].layout.size.width, 90.0);
    assert_close(tree.nodes[capped].layout.offset.x, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 50.0);
}

#[test]
fn zero_flex_grow_freezes_item_before_distributing_positive_free_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let frozen = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, frozen);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[frozen].layout.size.width, 20.0);
    assert_close(tree.nodes[flexible].layout.size.width, 80.0);
    assert_close(tree.nodes[frozen].layout.offset.x, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 20.0);
}

#[test]
fn min_width_violation_freezes_item_during_flex_grow_and_restarts_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let clamped = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        min_width: Length::points(70.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, clamped);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[clamped].layout.size.width, 70.0);
    assert_close(tree.nodes[flexible].layout.size.width, 30.0);
    assert_close(tree.nodes[clamped].layout.offset.x, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 70.0);
}

#[test]
fn main_axis_gap_reduces_free_space_before_flex_grow_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(110.0),
        height: Length::points(10.0),
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 3.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(110.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 35.0);
    assert_close(tree.nodes[second].layout.size.width, 65.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 45.0);
}

#[test]
fn multiple_max_width_violations_freeze_before_redistributing_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(180.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::points(30.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let third = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);

    run_rust_layout(&mut tree, root, Constraints::definite(180.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 30.0);
    assert_close(tree.nodes[second].layout.size.width, 50.0);
    assert_close(tree.nodes[third].layout.size.width, 100.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 30.0);
    assert_close(tree.nodes[third].layout.offset.x, 80.0);
}

#[test]
fn flex_grow_sum_below_one_leaves_remaining_space_for_justify_content() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        flex_grow: 0.25,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        flex_grow: 0.25,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 30.0);
    assert_close(tree.nodes[second].layout.size.width, 30.0);
    assert_close(tree.nodes[first].layout.offset.x, 20.0);
    assert_close(tree.nodes[second].layout.offset.x, 50.0);
}

#[test]
fn flex_shrink_sum_below_one_leaves_negative_space_for_justify_content() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        justify_content: JustifyContent::Center,
        width: Length::points(80.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(50.0),
        flex_shrink: 0.25,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(50.0),
        flex_shrink: 0.25,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(80.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 45.0);
    assert_close(tree.nodes[second].layout.size.width, 45.0);
    assert_close(tree.nodes[first].layout.offset.x, -5.0);
    assert_close(tree.nodes[second].layout.offset.x, 40.0);
}

#[test]
fn flex_shrink_distribution_is_scaled_by_flex_base_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(120.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let large_base = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(100.0),
        flex_shrink: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let small_base = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(50.0),
        flex_shrink: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, large_base);
    tree.append_child(root, small_base);

    run_rust_layout(&mut tree, root, Constraints::definite(120.0, 10.0));

    assert_close(tree.nodes[large_base].layout.size.width, 80.0);
    assert_close(tree.nodes[small_base].layout.size.width, 40.0);
    assert_close(tree.nodes[large_base].layout.offset.x, 0.0);
    assert_close(tree.nodes[small_base].layout.offset.x, 80.0);
}

#[test]
fn flex_shrink_negative_inner_size_is_floored_after_outer_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(0.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        flex_shrink: 1.0,
        height: Length::points(10.0),
        margin: Rect::new(
            Length::points(10.0),
            Length::points(10.0),
            Length::ZERO,
            Length::ZERO,
        ),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        flex_shrink: 1.0,
        height: Length::points(10.0),
        margin: Rect::new(
            Length::points(10.0),
            Length::points(10.0),
            Length::ZERO,
            Length::ZERO,
        ),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(0.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 0.0);
    assert_close(tree.nodes[second].layout.size.width, 0.0);
    assert_close(tree.nodes[first].layout.offset.x, 10.0);
    assert_close(tree.nodes[second].layout.offset.x, 30.0);
}

#[test]
fn all_zero_flex_grow_items_freeze_and_leave_space_for_justify_content() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(30.0),
        flex_grow: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 20.0);
    assert_close(tree.nodes[second].layout.size.width, 30.0);
    assert_close(tree.nodes[first].layout.offset.x, 25.0);
    assert_close(tree.nodes[second].layout.offset.x, 45.0);
}

#[test]
fn zero_flex_shrink_freezes_item_before_distributing_negative_free_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(80.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let frozen = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(50.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(50.0),
        flex_shrink: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, frozen);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(80.0, 10.0));

    assert_close(tree.nodes[frozen].layout.size.width, 50.0);
    assert_close(tree.nodes[flexible].layout.size.width, 30.0);
    assert_close(tree.nodes[frozen].layout.offset.x, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 50.0);
}

#[test]
fn max_width_violation_freezes_item_during_flex_shrink_and_restarts_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(160.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let capped = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(100.0),
        flex_shrink: 1.0,
        max_width: Length::points(70.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(100.0),
        flex_shrink: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(160.0, 10.0));

    assert_close(tree.nodes[capped].layout.size.width, 70.0);
    assert_close(tree.nodes[flexible].layout.size.width, 90.0);
    assert_close(tree.nodes[capped].layout.offset.x, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 70.0);
}

#[test]
fn fit_content_max_width_freezes_item_and_redistributes_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let capped = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::fit_content(Some(BaseLength::fixed(30.0))),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[capped].layout.size.width, 30.0);
    assert_close(tree.nodes[flexible].layout.size.width, 70.0);
    assert_close(tree.nodes[capped].layout.offset.x, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 30.0);
}

#[test]
fn fit_content_max_width_without_argument_does_not_cap_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::fit_content(None),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 50.0);
    assert_close(tree.nodes[second].layout.size.width, 50.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 50.0);
}

#[test]
fn percent_max_width_freezes_item_and_redistributes_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let capped = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::percent(30.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[capped].layout.size.width, 30.0);
    assert_close(tree.nodes[flexible].layout.size.width, 70.0);
    assert_close(tree.nodes[capped].layout.offset.x, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 30.0);
}

#[test]
fn column_percent_min_height_freezes_item_during_flex_shrink() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(60.0),
        min_height: Length::percent(50.0),
        width: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(60.0),
        width: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(10.0, 80.0));

    assert_close(tree.nodes[first].layout.size.height, 40.0);
    assert_close(tree.nodes[second].layout.size.height, 40.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 40.0);
}

#[test]
fn column_fit_content_min_height_freezes_item_during_flex_shrink() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(60.0),
        min_height: Length::fit_content(Some(BaseLength::fixed(40.0))),
        width: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(60.0),
        width: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(10.0, 80.0));

    assert_close(tree.nodes[first].layout.size.height, 40.0);
    assert_close(tree.nodes[second].layout.size.height, 40.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 40.0);
}

#[test]
fn column_fit_content_min_height_without_argument_does_not_freeze_item() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(50.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(40.0),
        min_height: Length::fit_content(None),
        width: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(40.0),
        width: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(10.0, 50.0));

    assert_close(tree.nodes[first].layout.size.height, 25.0);
    assert_close(tree.nodes[second].layout.size.height, 25.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 25.0);
}

#[test]
fn column_percent_max_height_freezes_item_and_redistributes_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let capped = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_height: Length::percent(30.0),
        width: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        width: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(10.0, 100.0));

    assert_close(tree.nodes[capped].layout.size.height, 30.0);
    assert_close(tree.nodes[flexible].layout.size.height, 70.0);
    assert_close(tree.nodes[capped].layout.offset.y, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.y, 30.0);
}

#[test]
fn column_fit_content_max_height_freezes_item_and_redistributes_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let capped = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_height: Length::fit_content(Some(BaseLength::fixed(30.0))),
        width: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        width: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(10.0, 100.0));

    assert_close(tree.nodes[capped].layout.size.height, 30.0);
    assert_close(tree.nodes[flexible].layout.size.height, 70.0);
    assert_close(tree.nodes[capped].layout.offset.y, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.y, 30.0);
}

#[test]
fn column_max_content_max_height_does_not_cap_flex_grow_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_height: Length::MaxContent,
        width: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        width: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(10.0, 100.0));

    assert_close(tree.nodes[first].layout.size.height, 50.0);
    assert_close(tree.nodes[second].layout.size.height, 50.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 50.0);
}

#[test]
fn row_reverse_flex_grow_freeze_places_flexed_items_from_right_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::RowReverse,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let capped = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        max_width: Length::points(30.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[capped].layout.size.width, 30.0);
    assert_close(tree.nodes[flexible].layout.size.width, 70.0);
    assert_close(tree.nodes[capped].layout.offset.x, 70.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 0.0);
}

#[test]
fn column_reverse_flex_shrink_freeze_places_flexed_items_from_bottom_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::ColumnReverse,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(80.0),
        ..Style::default()
    }));
    let frozen = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(50.0),
        flex_shrink: 0.0,
        width: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(50.0),
        flex_shrink: 1.0,
        width: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, frozen);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(10.0, 80.0));

    assert_close(tree.nodes[frozen].layout.size.height, 50.0);
    assert_close(tree.nodes[flexible].layout.size.height, 30.0);
    assert_close(tree.nodes[frozen].layout.offset.y, 30.0);
    assert_close(tree.nodes[flexible].layout.offset.y, 0.0);
}

#[test]
fn flexible_lengths_resolve_independently_per_wrapped_line() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        align_items: AlignItems::FlexStart,
        align_content: AlignContent::FlexStart,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(50.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(40.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let third = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let fourth = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 3.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);
    tree.append_child(root, fourth);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.size.width, 55.0);
    assert_close(tree.nodes[second].layout.size.width, 45.0);
    assert_close(tree.nodes[third].layout.size.width, 35.0);
    assert_close(tree.nodes[fourth].layout.size.width, 65.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 55.0);
    assert_close(tree.nodes[third].layout.offset.x, 0.0);
    assert_close(tree.nodes[fourth].layout.offset.x, 35.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[third].layout.offset.y, 10.0);
}

#[test]
fn flexible_lengths_direction_matrix_places_resolved_main_sizes() {
    for direction_case in MAIN_AXIS_MATRIX {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            flex_direction: direction_case.flex_direction,
            direction: direction_case.direction,
            align_items: AlignItems::FlexStart,
            width: Length::points(100.0),
            height: Length::points(100.0),
            ..Style::default()
        }));
        let first = tree.push(SimpleNode::new(Style {
            flex_basis: Length::points(20.0),
            flex_grow: 1.0,
            width: Length::points(if direction_case.is_row() { 20.0 } else { 10.0 }),
            height: Length::points(if direction_case.is_row() { 10.0 } else { 20.0 }),
            ..Style::default()
        }));
        let second = tree.push(SimpleNode::new(Style {
            flex_basis: Length::points(20.0),
            flex_grow: 2.0,
            width: Length::points(if direction_case.is_row() { 20.0 } else { 10.0 }),
            height: Length::points(if direction_case.is_row() { 10.0 } else { 20.0 }),
            ..Style::default()
        }));
        tree.append_child(root, first);
        tree.append_child(root, second);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

        let expected_offsets =
            expected_variable_main_offsets(direction_case, 100.0, [40.0, 60.0], [0.0, 40.0]);
        let name = direction_case.name();
        assert_close_named(
            &format!("{name} first main size"),
            main_axis_size(tree.nodes[first].layout, direction_case.is_row()),
            40.0,
        );
        assert_close_named(
            &format!("{name} second main size"),
            main_axis_size(tree.nodes[second].layout, direction_case.is_row()),
            60.0,
        );
        assert_close_named(
            &format!("{name} first offset"),
            main_axis_offset(tree.nodes[first].layout, direction_case.is_row()),
            expected_offsets[0],
        );
        assert_close_named(
            &format!("{name} second offset"),
            main_axis_offset(tree.nodes[second].layout, direction_case.is_row()),
            expected_offsets[1],
        );
    }
}

#[test]
fn measured_flex_basis_grow_max_width_violation_restarts_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(120.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let capped = tree.push(SimpleNode::with_measured_size(
        Style {
            flex_grow: 1.0,
            max_width: Length::points(70.0),
            height: Length::points(10.0),
            ..Style::default()
        },
        Size::new(60.0, 10.0),
    ));
    let flexible = tree.push(SimpleNode::with_measured_size(
        Style {
            flex_grow: 1.0,
            height: Length::points(10.0),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(120.0, 10.0));

    assert_close(tree.nodes[capped].layout.size.width, 70.0);
    assert_close(tree.nodes[flexible].layout.size.width, 50.0);
    assert_close(tree.nodes[capped].layout.offset.x, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 70.0);
}

#[test]
fn measured_flex_basis_shrink_min_width_violation_restarts_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(80.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let floored = tree.push(SimpleNode::with_measured_size(
        Style {
            min_width: Length::points(50.0),
            height: Length::points(10.0),
            ..Style::default()
        },
        Size::new(60.0, 10.0),
    ));
    let flexible = tree.push(SimpleNode::with_measured_size(
        Style {
            height: Length::points(10.0),
            ..Style::default()
        },
        Size::new(60.0, 10.0),
    ));
    tree.append_child(root, floored);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(80.0, 10.0));

    assert_close(tree.nodes[floored].layout.size.width, 50.0);
    assert_close(tree.nodes[flexible].layout.size.width, 30.0);
    assert_close(tree.nodes[floored].layout.offset.x, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 50.0);
}

#[test]
fn nested_intrinsic_flex_basis_grow_max_width_violation_restarts_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(120.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let capped = tree.push(SimpleNode::new(Style {
        max_width: Length::points(70.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let nested_child = tree.push(SimpleNode::new(Style {
        width: Length::points(60.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        flex_grow: 1.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, capped);
    tree.append_child(root, flexible);
    tree.append_child(capped, nested_child);

    run_rust_layout(&mut tree, root, Constraints::definite(120.0, 10.0));

    assert_close(tree.nodes[capped].layout.size.width, 70.0);
    assert_close(tree.nodes[flexible].layout.size.width, 50.0);
    assert_close(tree.nodes[capped].layout.offset.x, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 70.0);
}

#[test]
fn nested_intrinsic_flex_basis_shrink_min_width_violation_restarts_distribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::FlexStart,
        width: Length::points(80.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let floored = tree.push(SimpleNode::new(Style {
        min_width: Length::points(50.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let nested_child = tree.push(SimpleNode::new(Style {
        width: Length::points(60.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let flexible = fixed_flex_child(&mut tree, 60.0, 10.0);
    tree.append_child(root, floored);
    tree.append_child(root, flexible);
    tree.append_child(floored, nested_child);

    run_rust_layout(&mut tree, root, Constraints::definite(80.0, 10.0));

    assert_close(tree.nodes[floored].layout.size.width, 50.0);
    assert_close(tree.nodes[flexible].layout.size.width, 30.0);
    assert_close(tree.nodes[floored].layout.offset.x, 0.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 50.0);
}

#[test]
fn row_reverse_positions_items_from_right_edge_in_tree_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::RowReverse,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 10.0, 10.0);
    let second = fixed_flex_child(&mut tree, 10.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 90.0);
    assert_close(tree.nodes[second].layout.offset.x, 80.0);
}

#[test]
fn rtl_row_positions_items_from_right_edge_in_tree_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        direction: Direction::Rtl,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 10.0, 10.0);
    let second = fixed_flex_child(&mut tree, 10.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 90.0);
    assert_close(tree.nodes[second].layout.offset.x, 80.0);
}

#[test]
fn rtl_row_reverse_positions_items_from_left_edge_in_tree_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        direction: Direction::Rtl,
        flex_direction: FlexDirection::RowReverse,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 10.0, 10.0);
    let second = fixed_flex_child(&mut tree, 10.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 10.0);
}

#[test]
fn rtl_column_uses_right_cross_start_for_flex_start() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        direction: Direction::Rtl,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        width: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.x, 90.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
}

#[test]
fn justify_content_stretch_behaves_like_flex_start_in_flex_layout() {
    assert_eq!(Style::default().justify_content, JustifyContent::Stretch);

    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: JustifyContent::Stretch,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 10.0, 10.0);
    let second = fixed_flex_child(&mut tree, 10.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 10.0);
}

#[test]
fn justify_content_center_uses_negative_free_space_when_items_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(40.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(40.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, -10.0);
    assert_close(tree.nodes[second].layout.offset.x, 20.0);
}

#[test]
fn justify_content_space_evenly_distributes_free_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: JustifyContent::SpaceEvenly,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 27.0);
    assert_close(tree.nodes[second].layout.offset.x, 63.0);
}

#[test]
fn justify_content_space_evenly_single_item_uses_equal_edge_spaces() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: JustifyContent::SpaceEvenly,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let child = fixed_flex_child(&mut tree, 20.0, 10.0);
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[child].layout.offset.x, 40.0);
}

#[test]
fn justify_content_space_between_single_item_falls_back_to_flex_start() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: JustifyContent::SpaceBetween,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let child = fixed_flex_child(&mut tree, 20.0, 10.0);
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
}

#[test]
fn justify_content_space_between_keeps_gap_when_items_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: JustifyContent::SpaceBetween,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(10.0),
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 40.0);
}

#[test]
fn justify_content_space_around_single_item_falls_back_to_center() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: JustifyContent::SpaceAround,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let child = fixed_flex_child(&mut tree, 20.0, 10.0);
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[child].layout.offset.x, 40.0);
}

#[test]
fn justify_content_space_around_centers_overflow_and_keeps_gap() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: JustifyContent::SpaceAround,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(10.0),
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, -10.0);
    assert_close(tree.nodes[second].layout.offset.x, 30.0);
}

#[test]
fn justify_content_space_around_uses_edge_difference_width_rounding_when_overflowing() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: JustifyContent::SpaceAround,
        align_items: AlignItems::FlexStart,
        width: Length::points(55.0),
        height: Length::points(10.0),
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(30.0),
        flex_shrink: 0.0,
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(55.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, -8.0);
    assert_close(tree.nodes[first].layout.size.width, 31.0);
    assert_close(tree.nodes[second].layout.offset.x, 33.0);
}

#[test]
fn align_content_center_uses_negative_free_space_when_lines_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(15.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 30.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 15.0));

    assert_close(tree.nodes[first].layout.offset.y, -3.0);
    assert_close(tree.nodes[second].layout.offset.y, 8.0);
}

#[test]
fn align_content_space_between_negative_free_space_falls_back_to_flex_start() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::SpaceBetween,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(15.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 30.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 15.0));

    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 10.0);
}

#[test]
fn align_content_space_between_keeps_row_gap_when_lines_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::SpaceBetween,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(15.0),
        row_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 30.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 15.0));

    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 20.0);
}

#[test]
fn align_content_space_around_negative_free_space_falls_back_to_center() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::SpaceAround,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(15.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 30.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 15.0));

    assert_close(tree.nodes[first].layout.offset.y, -3.0);
    assert_close(tree.nodes[second].layout.offset.y, 8.0);
}

#[test]
fn align_content_space_around_centers_overflow_and_keeps_row_gap() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::SpaceAround,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(15.0),
        row_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 30.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 15.0));

    assert_close(tree.nodes[first].layout.offset.y, -8.0);
    assert_close(tree.nodes[first].layout.size.height, 11.0);
    assert_close(tree.nodes[first].layout.baseline.unwrap(), 10.0);
    assert_close(tree.nodes[second].layout.offset.y, 13.0);
}

#[test]
fn align_content_space_evenly_distributes_wrapped_lines() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::SpaceEvenly,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 30.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 100.0));

    assert_close(tree.nodes[first].layout.offset.y, 27.0);
    assert_close(tree.nodes[second].layout.offset.y, 63.0);
}

#[test]
fn align_content_space_evenly_uses_negative_space_when_lines_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        align_content: AlignContent::SpaceEvenly,
        align_items: AlignItems::FlexStart,
        width: Length::points(50.0),
        height: Length::points(15.0),
        row_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 30.0, 10.0);
    let second = fixed_flex_child(&mut tree, 30.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 15.0));

    assert_close(tree.nodes[first].layout.offset.y, -5.0);
    assert_close(tree.nodes[second].layout.offset.y, 10.0);
}

#[test]
fn flex_column_container_baseline_uses_first_item_baseline_after_main_axis_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(40.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style::default(),
        Size::new(20.0, 20.0),
        6.0,
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(40.0, 100.0));

    assert_close(tree.nodes[root].layout.baseline.unwrap(), 46.0);
    assert_close(tree.nodes[child].layout.offset.y, 40.0);
}

#[test]
fn flex_layout_uses_external_text_layout_trait_for_content_size_and_baseline() {
    let mut tree = TextTree::default();
    let root = tree.push(TextNode::new(Style {
        display: Display::Flex,
        align_items: AlignItems::Baseline,
        ..Style::default()
    }));
    let early = tree.push(TextNode::text(Style::default(), Size::new(30.0, 12.0), 9.0));
    let late = tree.push(TextNode::text(
        Style::default(),
        Size::new(20.0, 20.0),
        15.0,
    ));
    tree.append_child(root, early);
    tree.append_child(root, late);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 50.0);
    assert_close(size.height, 20.0);
    assert_close(tree.nodes[early].layout.size.width, 30.0);
    assert_close(tree.nodes[late].layout.size.height, 20.0);
    assert_close(tree.nodes[early].layout.offset.y, 6.0);
    assert_close(tree.nodes[late].layout.offset.y, 0.0);
    assert!(tree.text.calls[early] > 0);
    assert!(tree.text.calls[late] > 0);
}

#[test]
fn flex_initial_setup_adapter_generates_anonymous_items_for_contiguous_text_runs() {
    let element = TextNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(10.0),
        ..Style::default()
    });
    let (mut tree, root, generated) = TextTree::from_flex_adapter_inputs(
        Style {
            display: Display::Flex,
            align_items: AlignItems::FlexStart,
            ..Style::default()
        },
        vec![
            TextFlexInput::text("alpha", Size::new(12.0, 10.0), 8.0),
            TextFlexInput::text("beta", Size::new(8.0, 6.0), 5.0),
            TextFlexInput::text("   \n\t", Size::new(100.0, 100.0), 100.0),
            TextFlexInput::element(element),
            TextFlexInput::text(" ", Size::new(100.0, 100.0), 100.0),
            TextFlexInput::text("gamma", Size::new(15.0, 9.0), 7.0),
        ],
    );

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_eq!(tree.nodes[root].children, generated);
    assert_eq!(generated.len(), 3);
    let first_text = generated[0];
    let block = generated[1];
    let second_text = generated[2];
    assert_close(size.width, 45.0);
    assert_close(size.height, 10.0);
    assert_close(tree.nodes[first_text].layout.size.width, 20.0);
    assert_close(tree.nodes[first_text].layout.size.height, 10.0);
    assert_close(tree.nodes[first_text].layout.offset.x, 0.0);
    assert_close(tree.nodes[block].layout.offset.x, 20.0);
    assert_close(tree.nodes[second_text].layout.size.width, 15.0);
    assert_close(tree.nodes[second_text].layout.offset.x, 30.0);
    assert!(tree.text.calls[first_text] > 0);
    assert_eq!(tree.text.calls[block], 0);
    assert!(tree.text.calls[second_text] > 0);
}

#[test]
fn flex_initial_setup_skips_out_of_flow_children_but_positions_static_rect() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 20.0, 10.0);
    let absolute = tree.push(SimpleNode::new(Style {
        position: starlight_layout::PositionType::Absolute,
        width: Length::points(10.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::FlexEnd),
        margin: Rect::all(Length::Auto),
        ..Style::default()
    }));
    let second = fixed_flex_child(&mut tree, 20.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, absolute);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 30.0);
    assert_close(tree.nodes[second].layout.offset.x, 50.0);
    assert_close(tree.nodes[absolute].layout.size.width, 10.0);
    assert_close(tree.nodes[absolute].layout.size.height, 10.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 45.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 10.0);
}

#[test]
fn row_reverse_flex_end_packs_items_at_left_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::RowReverse,
        justify_content: JustifyContent::FlexEnd,
        align_items: AlignItems::FlexStart,
        width: Length::points(100.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 10.0, 10.0);
    let second = fixed_flex_child(&mut tree, 10.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 10.0);
    assert_close(tree.nodes[second].layout.offset.x, 0.0);
}

#[test]
fn column_reverse_positions_items_from_bottom_edge_in_tree_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::ColumnReverse,
        align_items: AlignItems::FlexStart,
        width: Length::points(10.0),
        height: Length::points(100.0),
        ..Style::default()
    }));
    let first = fixed_flex_child(&mut tree, 10.0, 10.0);
    let second = fixed_flex_child(&mut tree, 10.0, 10.0);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(10.0, 100.0));

    assert_close(tree.nodes[first].layout.offset.y, 90.0);
    assert_close(tree.nodes[second].layout.offset.y, 80.0);
}

fn fixed_flex_child(tree: &mut SimpleTree, basis: f32, height: f32) -> usize {
    tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(basis),
        height: Length::points(height),
        ..Style::default()
    }))
}

fn growing_flex_child(tree: &mut SimpleTree, basis: f32, height: f32) -> usize {
    tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(basis),
        flex_grow: 1.0,
        height: Length::points(height),
        ..Style::default()
    }))
}

trait ExternalTextLayout {
    fn measure_text(&mut self, node: usize, constraints: Constraints) -> Size;
    fn text_baseline(&self, node: usize, content_size: Size) -> f32;
}

#[derive(Clone, Debug, Default)]
struct RecordingTextLayout {
    sizes: Vec<Option<Size>>,
    baselines: Vec<Option<f32>>,
    calls: Vec<usize>,
}

impl RecordingTextLayout {
    fn push_box(&mut self, size: Option<Size>, baseline: Option<f32>) {
        self.sizes.push(size);
        self.baselines.push(baseline);
        self.calls.push(0);
    }
}

impl ExternalTextLayout for RecordingTextLayout {
    fn measure_text(&mut self, node: usize, _constraints: Constraints) -> Size {
        self.calls[node] += 1;
        self.sizes[node].expect("text node must have an external measured size")
    }

    fn text_baseline(&self, node: usize, content_size: Size) -> f32 {
        self.baselines[node].unwrap_or(content_size.height)
    }
}

#[derive(Clone, Debug)]
struct TextNode {
    style: Style,
    layout: LayoutResult,
    children: Vec<usize>,
    is_text: bool,
    text_size: Option<Size>,
    text_baseline: Option<f32>,
}

impl TextNode {
    fn new(style: Style) -> Self {
        Self {
            style,
            layout: LayoutResult::default(),
            children: Vec::new(),
            is_text: false,
            text_size: None,
            text_baseline: None,
        }
    }

    fn text(style: Style, size: Size, baseline: f32) -> Self {
        Self {
            is_text: true,
            text_size: Some(size),
            text_baseline: Some(baseline),
            ..Self::new(style)
        }
    }
}

#[derive(Clone, Debug)]
enum TextFlexInput {
    Element(Box<TextNode>),
    TextRun {
        content: &'static str,
        size: Size,
        baseline: f32,
    },
}

impl TextFlexInput {
    fn element(node: TextNode) -> Self {
        Self::Element(Box::new(node))
    }

    fn text(content: &'static str, size: Size, baseline: f32) -> Self {
        Self::TextRun {
            content,
            size,
            baseline,
        }
    }
}

#[derive(Clone, Debug, Default)]
struct TextTree {
    nodes: Vec<TextNode>,
    text: RecordingTextLayout,
}

impl TextTree {
    fn from_flex_adapter_inputs(
        root_style: Style,
        inputs: Vec<TextFlexInput>,
    ) -> (Self, usize, Vec<usize>) {
        let mut tree = Self::default();
        let root = tree.push(TextNode::new(root_style));
        let mut generated = Vec::new();
        let mut pending_text: Option<(Size, f32)> = None;

        for input in inputs {
            match input {
                TextFlexInput::TextRun {
                    content,
                    size,
                    baseline,
                } => {
                    if content.trim().is_empty() {
                        continue;
                    }
                    if let Some((pending_size, pending_baseline)) = &mut pending_text {
                        pending_size.width += size.width;
                        pending_size.height = pending_size.height.max(size.height);
                        *pending_baseline = pending_baseline.max(baseline);
                    } else {
                        pending_text = Some((size, baseline));
                    }
                }
                TextFlexInput::Element(node) => {
                    Self::flush_anonymous_text(&mut tree, root, &mut generated, &mut pending_text);
                    let child = tree.push(*node);
                    tree.append_child(root, child);
                    generated.push(child);
                }
            }
        }
        Self::flush_anonymous_text(&mut tree, root, &mut generated, &mut pending_text);

        (tree, root, generated)
    }

    fn flush_anonymous_text(
        tree: &mut Self,
        root: usize,
        generated: &mut Vec<usize>,
        pending_text: &mut Option<(Size, f32)>,
    ) {
        let Some((size, baseline)) = pending_text.take() else {
            return;
        };
        let child = tree.push(TextNode::text(Style::default(), size, baseline));
        tree.append_child(root, child);
        generated.push(child);
    }

    fn push(&mut self, node: TextNode) -> usize {
        let id = self.nodes.len();
        let size = node.text_size;
        let baseline = node.text_baseline;
        self.nodes.push(node);
        self.text.push_box(size, baseline);
        id
    }

    fn append_child(&mut self, parent: usize, child: usize) {
        self.nodes[parent].children.push(child);
    }
}

impl LayoutTree for TextTree {
    type NodeId = usize;
    type Children<'a> = std::iter::Copied<std::slice::Iter<'a, usize>>;

    fn children(&self, node: Self::NodeId) -> Self::Children<'_> {
        self.nodes[node].children.iter().copied()
    }

    fn style(&self, node: Self::NodeId) -> &Style {
        &self.nodes[node].style
    }

    fn set_layout(&mut self, node: Self::NodeId, layout: LayoutResult) {
        self.nodes[node].layout = layout;
    }

    fn layout(&self, node: Self::NodeId) -> Option<LayoutResult> {
        Some(self.nodes[node].layout)
    }

    fn measure(&mut self, node: Self::NodeId, constraints: Constraints) -> Option<Size> {
        self.nodes[node]
            .is_text
            .then(|| self.text.measure_text(node, constraints))
    }

    fn has_measure(&self, node: Self::NodeId) -> bool {
        self.nodes[node].is_text
    }

    fn baseline(&self, node: Self::NodeId, content_size: Size) -> Option<f32> {
        self.nodes[node]
            .is_text
            .then(|| self.text.text_baseline(node, content_size))
    }
}
