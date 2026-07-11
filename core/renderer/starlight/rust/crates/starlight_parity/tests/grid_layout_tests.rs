// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use starlight_layout::{
    AlignContent, AlignItems, BaseLength, Constraints, Direction, Display, GridAutoFlow,
    JustifyContent, JustifyItems, LayoutEngine, LayoutResult, LayoutTree, Length, MeasureMode,
    PositionType, Rect, SideConstraint, SimpleNode, SimpleTree, Size, Style, Visibility,
};
use starlight_parity::run_rust_layout;

fn assert_close(actual: f32, expected: f32) {
    assert!(
        (actual - expected).abs() < 0.01,
        "expected {expected}, got {actual}"
    );
}

#[test]
fn grid_placement_properties_do_not_affect_flex_items() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(100.0),
        height: Length::points(20.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let placed_like_grid_item = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(10.0),
        height: Length::points(8.0),
        grid_column_start: Some(99),
        grid_column_end: Some(100),
        grid_row_start: Some(99),
        grid_row_end: Some(100),
        grid_column_span: 3,
        grid_row_span: 3,
        ..Style::default()
    }));
    let following = tree.push(SimpleNode::new(Style {
        flex_basis: Length::points(20.0),
        height: Length::points(8.0),
        ..Style::default()
    }));
    tree.append_child(root, placed_like_grid_item);
    tree.append_child(root, following);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[placed_like_grid_item].layout.offset.x, 0.0);
    assert_close(tree.nodes[placed_like_grid_item].layout.offset.y, 0.0);
    assert_close(tree.nodes[placed_like_grid_item].layout.size.width, 10.0);
    assert_close(tree.nodes[following].layout.offset.x, 10.0);
    assert_close(tree.nodes[following].layout.offset.y, 0.0);
}

#[derive(Clone, Debug)]
struct MeasuringNode {
    style: Style,
    layout: LayoutResult,
    children: Vec<usize>,
    measure: Option<MeasureBehavior>,
    last_constraints: Option<Constraints>,
    min_content: Option<IntrinsicBehavior>,
    max_content: Option<IntrinsicBehavior>,
    last_min_content_constraints: Option<Constraints>,
    last_max_content_constraints: Option<Constraints>,
    min_content_constraints: Vec<Constraints>,
    max_content_constraints: Vec<Constraints>,
}

#[derive(Clone, Copy, Debug)]
enum MeasureBehavior {
    Fixed(Size),
    HeightFromWidth {
        intrinsic_width: f32,
        fallback_height: f32,
        height_ratio: f32,
    },
}

#[derive(Clone, Copy, Debug)]
enum IntrinsicBehavior {
    Fixed(Size),
    WidthFromHeight {
        fallback_width: f32,
        width_ratio: f32,
        height: f32,
    },
    CrossAxis {
        fallback_width: f32,
        width_from_height_ratio: f32,
        fallback_height: f32,
        height_from_width_ratio: f32,
    },
}

impl IntrinsicBehavior {
    fn measure(self, constraints: Constraints) -> Size {
        match self {
            Self::Fixed(size) => size,
            Self::WidthFromHeight {
                fallback_width,
                width_ratio,
                height,
            } => Size::new(
                constraints
                    .height
                    .bounded_size()
                    .map(|height| height * width_ratio)
                    .unwrap_or(fallback_width),
                height,
            ),
            Self::CrossAxis {
                fallback_width,
                width_from_height_ratio,
                fallback_height,
                height_from_width_ratio,
            } => Size::new(
                constraints
                    .height
                    .bounded_size()
                    .map(|height| height * width_from_height_ratio)
                    .unwrap_or(fallback_width),
                constraints
                    .width
                    .bounded_size()
                    .map(|width| width * height_from_width_ratio)
                    .unwrap_or(fallback_height),
            ),
        }
    }
}

impl MeasuringNode {
    fn new(style: Style) -> Self {
        Self {
            style,
            layout: LayoutResult::default(),
            children: Vec::new(),
            measure: None,
            last_constraints: None,
            min_content: None,
            max_content: None,
            last_min_content_constraints: None,
            last_max_content_constraints: None,
            min_content_constraints: Vec::new(),
            max_content_constraints: Vec::new(),
        }
    }

    fn measured(style: Style, measured_size: Size) -> Self {
        Self {
            measure: Some(MeasureBehavior::Fixed(measured_size)),
            ..Self::new(style)
        }
    }

    fn measured_with_intrinsic(
        style: Style,
        measured_size: Size,
        min_content: Size,
        max_content: Size,
    ) -> Self {
        Self {
            measure: Some(MeasureBehavior::Fixed(measured_size)),
            min_content: Some(IntrinsicBehavior::Fixed(min_content)),
            max_content: Some(IntrinsicBehavior::Fixed(max_content)),
            ..Self::new(style)
        }
    }

    fn measured_with_height_dependent_intrinsic(
        style: Style,
        measured_size: Size,
        fallback_width: f32,
        width_ratio: f32,
        intrinsic_height: f32,
    ) -> Self {
        let intrinsic = IntrinsicBehavior::WidthFromHeight {
            fallback_width,
            width_ratio,
            height: intrinsic_height,
        };
        Self {
            measure: Some(MeasureBehavior::Fixed(measured_size)),
            min_content: Some(intrinsic),
            max_content: Some(intrinsic),
            ..Self::new(style)
        }
    }

    fn measured_with_cross_axis_intrinsic(
        style: Style,
        measured_size: Size,
        fallback_width: f32,
        width_from_height_ratio: f32,
        fallback_height: f32,
        height_from_width_ratio: f32,
    ) -> Self {
        let intrinsic = IntrinsicBehavior::CrossAxis {
            fallback_width,
            width_from_height_ratio,
            fallback_height,
            height_from_width_ratio,
        };
        Self {
            measure: Some(MeasureBehavior::Fixed(measured_size)),
            min_content: Some(intrinsic),
            max_content: Some(intrinsic),
            ..Self::new(style)
        }
    }

    fn height_from_width(
        style: Style,
        intrinsic_width: f32,
        fallback_height: f32,
        height_ratio: f32,
    ) -> Self {
        Self {
            measure: Some(MeasureBehavior::HeightFromWidth {
                intrinsic_width,
                fallback_height,
                height_ratio,
            }),
            ..Self::new(style)
        }
    }
}

#[derive(Clone, Debug, Default)]
struct MeasuringTree {
    nodes: Vec<MeasuringNode>,
}

impl MeasuringTree {
    fn push(&mut self, node: MeasuringNode) -> usize {
        let id = self.nodes.len();
        self.nodes.push(node);
        id
    }

    fn append_child(&mut self, parent: usize, child: usize) {
        self.nodes[parent].children.push(child);
    }
}

impl LayoutTree for MeasuringTree {
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

    fn measure(&mut self, node: Self::NodeId, constraints: Constraints) -> Option<Size> {
        let node = &mut self.nodes[node];
        node.last_constraints = Some(constraints);
        node.measure.map(|behavior| match behavior {
            MeasureBehavior::Fixed(size) => Size::new(
                constraints.width.clamp(size.width),
                constraints.height.clamp(size.height),
            ),
            MeasureBehavior::HeightFromWidth {
                intrinsic_width,
                fallback_height,
                height_ratio,
            } => {
                let resolved_width = constraints.width.bounded_size().unwrap_or(intrinsic_width);
                let resolved_height = if constraints.width.bounded_size().is_some() {
                    resolved_width * height_ratio
                } else {
                    fallback_height
                };
                Size::new(
                    constraints.width.clamp(resolved_width),
                    constraints.height.clamp(resolved_height),
                )
            }
        })
    }

    fn measure_min_content(
        &mut self,
        node: Self::NodeId,
        constraints: Constraints,
    ) -> Option<Size> {
        let node = &mut self.nodes[node];
        node.last_min_content_constraints = Some(constraints);
        node.min_content_constraints.push(constraints);
        node.min_content
            .map(|intrinsic| intrinsic.measure(constraints))
    }

    fn measure_max_content(
        &mut self,
        node: Self::NodeId,
        constraints: Constraints,
    ) -> Option<Size> {
        let node = &mut self.nodes[node];
        node.last_max_content_constraints = Some(constraints);
        node.max_content_constraints.push(constraints);
        node.max_content
            .map(|intrinsic| intrinsic.measure(constraints))
    }

    fn has_measure(&self, node: Self::NodeId) -> bool {
        self.nodes[node].measure.is_some()
    }
}

#[test]
fn explicit_grid_tracks_place_children_row_major() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(130.0),
        height: Length::points(70.0),
        grid_template_columns: vec![Length::points(50.0), Length::points(70.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(30.0)],
        column_gap: Length::points(10.0),
        row_gap: Length::points(5.0),
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    let third = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(130.0, 70.0));

    assert_close(size.width, 130.0);
    assert_close(size.height, 70.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[first].layout.size.width, 50.0);
    assert_close(tree.nodes[first].layout.size.height, 20.0);
    assert_close(tree.nodes[second].layout.offset.x, 60.0);
    assert_close(tree.nodes[second].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.size.width, 70.0);
    assert_close(tree.nodes[second].layout.size.height, 20.0);
    assert_close(tree.nodes[third].layout.offset.x, 0.0);
    assert_close(tree.nodes[third].layout.offset.y, 25.0);
    assert_close(tree.nodes[third].layout.size.width, 50.0);
    assert_close(tree.nodes[third].layout.size.height, 30.0);
}

#[test]
fn grid_measured_item_percent_calc_min_max_uses_grid_area_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(80.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(50.0)],
        grid_template_rows: vec![Length::points(30.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::Start,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            min_width: Length::percent(25.0),
            max_width: Length::calc(8.0, 35.0),
            min_height: Length::calc(3.0, 18.0),
            max_height: Length::percent(70.0),
            padding: Rect::all(Length::points(1.0)),
            border: Rect::all(1.0),
            ..Style::default()
        },
        Size::new(80.0, 9.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(80.0, 40.0));

    assert_close(tree.nodes[child].layout.size.width, 30.0);
    assert_close(tree.nodes[child].layout.size.height, 13.0);
}

#[test]
fn display_none_child_is_laid_out_as_zero_and_skipped_by_grid_auto_placement() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let hidden = tree.push(SimpleNode::new(Style {
        display: Display::None,
        width: Length::points(50.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, hidden);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_eq!(tree.nodes[hidden].layout.size, Size::ZERO);
    assert_close(tree.nodes[hidden].layout.offset.x, 0.0);
    assert_close(tree.nodes[hidden].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 50.0);
    assert_close(tree.nodes[second].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.size.width, 50.0);
    assert_close(tree.nodes[second].layout.size.height, 20.0);
}

#[test]
fn hidden_and_collapse_grid_children_participate_in_auto_placement() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(150.0),
        height: Length::points(20.0),
        grid_template_columns: vec![
            Length::points(50.0),
            Length::points(50.0),
            Length::points(50.0),
        ],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let hidden = tree.push(SimpleNode::new(Style {
        visibility: Visibility::Hidden,
        width: Length::points(50.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let collapsed = tree.push(SimpleNode::new(Style {
        visibility: Visibility::Collapse,
        width: Length::points(50.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    let visible = grid_child(&mut tree);
    tree.append_child(root, hidden);
    tree.append_child(root, collapsed);
    tree.append_child(root, visible);

    run_rust_layout(&mut tree, root, Constraints::definite(150.0, 20.0));

    assert_close(tree.nodes[hidden].layout.offset.x, 0.0);
    assert_close(tree.nodes[hidden].layout.offset.y, 0.0);
    assert_close(tree.nodes[hidden].layout.size.width, 50.0);
    assert_close(tree.nodes[hidden].layout.size.height, 20.0);
    assert_close(tree.nodes[collapsed].layout.offset.x, 50.0);
    assert_close(tree.nodes[collapsed].layout.offset.y, 0.0);
    assert_close(tree.nodes[collapsed].layout.size.width, 50.0);
    assert_close(tree.nodes[collapsed].layout.size.height, 20.0);
    assert_close(tree.nodes[visible].layout.offset.x, 100.0);
    assert_close(tree.nodes[visible].layout.offset.y, 0.0);
    assert_close(tree.nodes[visible].layout.size.width, 50.0);
    assert_close(tree.nodes[visible].layout.size.height, 20.0);
}

#[test]
fn column_auto_flow_places_children_down_each_column() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(130.0),
        height: Length::points(70.0),
        grid_auto_flow: GridAutoFlow::Column,
        grid_template_columns: vec![Length::points(50.0), Length::points(70.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(30.0)],
        column_gap: Length::points(10.0),
        row_gap: Length::points(5.0),
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    let third = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);

    run_rust_layout(&mut tree, root, Constraints::definite(130.0, 70.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[first].layout.size.width, 50.0);
    assert_close(tree.nodes[first].layout.size.height, 20.0);
    assert_close(tree.nodes[second].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 25.0);
    assert_close(tree.nodes[second].layout.size.width, 50.0);
    assert_close(tree.nodes[second].layout.size.height, 30.0);
    assert_close(tree.nodes[third].layout.offset.x, 60.0);
    assert_close(tree.nodes[third].layout.offset.y, 0.0);
    assert_close(tree.nodes[third].layout.size.width, 70.0);
    assert_close(tree.nodes[third].layout.size.height, 20.0);
}

#[test]
fn grid_sparse_column_auto_flow_definite_rows_keep_cursor_and_advance_on_backward_row() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(30.0),
        height: Length::points(30.0),
        grid_auto_flow: GridAutoFlow::Column,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_rows: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        ..Style::default()
    }));
    let blocker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let first_row = tree.push(SimpleNode::new(Style {
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let later_row = tree.push(SimpleNode::new(Style {
        grid_row_start: Some(3),
        ..Style::default()
    }));
    let backward_row = tree.push(SimpleNode::new(Style {
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, blocker);
    tree.append_child(root, first_row);
    tree.append_child(root, later_row);
    tree.append_child(root, backward_row);

    run_rust_layout(&mut tree, root, Constraints::definite(30.0, 30.0));

    assert_close(tree.nodes[blocker].layout.offset.x, 0.0);
    assert_close(tree.nodes[blocker].layout.offset.y, 0.0);
    assert_close(tree.nodes[first_row].layout.offset.x, 10.0);
    assert_close(tree.nodes[first_row].layout.offset.y, 0.0);
    assert_close(tree.nodes[later_row].layout.offset.x, 10.0);
    assert_close(tree.nodes[later_row].layout.offset.y, 20.0);
    assert_close(tree.nodes[backward_row].layout.offset.x, 20.0);
    assert_close(tree.nodes[backward_row].layout.offset.y, 0.0);
}

#[test]
fn grid_dense_column_auto_flow_definite_rows_restart_column_search() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(30.0),
        height: Length::points(30.0),
        grid_auto_flow: GridAutoFlow::ColumnDense,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_rows: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        ..Style::default()
    }));
    let blocker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let first_row = tree.push(SimpleNode::new(Style {
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let later_row = tree.push(SimpleNode::new(Style {
        grid_row_start: Some(3),
        ..Style::default()
    }));
    tree.append_child(root, blocker);
    tree.append_child(root, first_row);
    tree.append_child(root, later_row);

    run_rust_layout(&mut tree, root, Constraints::definite(30.0, 30.0));

    assert_close(tree.nodes[blocker].layout.offset.x, 0.0);
    assert_close(tree.nodes[blocker].layout.offset.y, 0.0);
    assert_close(tree.nodes[first_row].layout.offset.x, 10.0);
    assert_close(tree.nodes[first_row].layout.offset.y, 0.0);
    assert_close(tree.nodes[later_row].layout.offset.x, 0.0);
    assert_close(tree.nodes[later_row].layout.offset.y, 20.0);
}

#[test]
fn grid_sparse_row_auto_flow_definite_columns_keep_cursor_and_advance_on_backward_column() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(30.0),
        height: Length::points(30.0),
        grid_auto_flow: GridAutoFlow::Row,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_rows: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        ..Style::default()
    }));
    let blocker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let first_column = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        ..Style::default()
    }));
    let later_column = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(3),
        ..Style::default()
    }));
    let backward_column = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, blocker);
    tree.append_child(root, first_column);
    tree.append_child(root, later_column);
    tree.append_child(root, backward_column);

    run_rust_layout(&mut tree, root, Constraints::definite(30.0, 30.0));

    assert_close(tree.nodes[blocker].layout.offset.x, 0.0);
    assert_close(tree.nodes[blocker].layout.offset.y, 0.0);
    assert_close(tree.nodes[first_column].layout.offset.x, 0.0);
    assert_close(tree.nodes[first_column].layout.offset.y, 10.0);
    assert_close(tree.nodes[later_column].layout.offset.x, 20.0);
    assert_close(tree.nodes[later_column].layout.offset.y, 10.0);
    assert_close(tree.nodes[backward_column].layout.offset.x, 0.0);
    assert_close(tree.nodes[backward_column].layout.offset.y, 20.0);
}

#[test]
fn grid_dense_row_auto_flow_definite_columns_restart_row_search() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(30.0),
        height: Length::points(30.0),
        grid_auto_flow: GridAutoFlow::Dense,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_rows: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        ..Style::default()
    }));
    let blocker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let first_column = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        ..Style::default()
    }));
    let later_column = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(3),
        ..Style::default()
    }));
    tree.append_child(root, blocker);
    tree.append_child(root, first_column);
    tree.append_child(root, later_column);

    run_rust_layout(&mut tree, root, Constraints::definite(30.0, 30.0));

    assert_close(tree.nodes[blocker].layout.offset.x, 0.0);
    assert_close(tree.nodes[blocker].layout.offset.y, 0.0);
    assert_close(tree.nodes[first_column].layout.offset.x, 0.0);
    assert_close(tree.nodes[first_column].layout.offset.y, 10.0);
    assert_close(tree.nodes[later_column].layout.offset.x, 20.0);
    assert_close(tree.nodes[later_column].layout.offset.y, 0.0);
}

#[test]
fn column_auto_flow_keeps_cursor_at_item_start_for_following_search() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(140.0),
        height: Length::points(140.0),
        grid_auto_flow: GridAutoFlow::Column,
        grid_template_columns: vec![
            Length::points(24.0),
            Length::points(24.0),
            Length::points(24.0),
        ],
        grid_template_rows: vec![
            Length::points(18.0),
            Length::points(18.0),
            Length::points(18.0),
        ],
        grid_auto_columns: vec![Length::points(20.0), Length::points(26.0)],
        grid_auto_rows: vec![Length::points(16.0), Length::points(22.0)],
        column_gap: Length::points(4.0),
        row_gap: Length::points(3.0),
        ..Style::default()
    }));
    let blocking = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(8.0, 8.0),
    ));
    let first_wide = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_span: 2,
            ..Style::default()
        },
        Size::new(9.0, 7.0),
    ));
    let tall = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_row_span: 2,
            ..Style::default()
        },
        Size::new(9.0, 7.0),
    ));
    let second_wide = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_span: 2,
            ..Style::default()
        },
        Size::new(9.0, 7.0),
    ));
    tree.append_child(root, blocking);
    tree.append_child(root, first_wide);
    tree.append_child(root, tall);
    tree.append_child(root, second_wide);

    run_rust_layout(&mut tree, root, Constraints::definite(160.0, 160.0));

    assert_close(tree.nodes[blocking].layout.offset.x, 0.0);
    assert_close(tree.nodes[blocking].layout.offset.y, 0.0);
    assert_close(tree.nodes[first_wide].layout.offset.x, 0.0);
    assert_close(tree.nodes[first_wide].layout.offset.y, 21.0);
    assert_close(tree.nodes[first_wide].layout.size.width, 52.0);
    assert_close(tree.nodes[tall].layout.offset.x, 56.0);
    assert_close(tree.nodes[tall].layout.offset.y, 0.0);
    assert_close(tree.nodes[tall].layout.size.height, 39.0);
    assert_close(tree.nodes[second_wide].layout.offset.x, 56.0);
    assert_close(tree.nodes[second_wide].layout.offset.y, 42.0);
    assert_close(tree.nodes[second_wide].layout.size.width, 48.0);
}

#[test]
fn auto_grid_item_skips_cell_occupied_by_explicitly_placed_item() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let explicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let auto = grid_child(&mut tree);
    tree.append_child(root, explicit);
    tree.append_child(root, auto);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[explicit].layout.offset.x, 0.0);
    assert_close(tree.nodes[auto].layout.offset.x, 50.0);
    assert_close(tree.nodes[auto].layout.offset.y, 0.0);
}

#[test]
fn auto_grid_item_skips_later_explicitly_placed_item() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let auto = grid_child(&mut tree);
    let explicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, auto);
    tree.append_child(root, explicit);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[explicit].layout.offset.x, 0.0);
    assert_close(tree.nodes[auto].layout.offset.x, 50.0);
    assert_close(tree.nodes[auto].layout.offset.y, 0.0);
}

#[test]
fn later_locked_main_axis_item_expands_auto_placement_limit_before_auto_items() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(30.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        grid_auto_columns: vec![Length::points(10.0)],
        grid_auto_columns_max: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let first_auto = grid_child(&mut tree);
    let second_auto = grid_child(&mut tree);
    let locked_column = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(3),
        ..Style::default()
    }));
    tree.append_child(root, first_auto);
    tree.append_child(root, second_auto);
    tree.append_child(root, locked_column);

    run_rust_layout(&mut tree, root, Constraints::definite(30.0, 10.0));

    assert_close(tree.nodes[first_auto].layout.offset.x, 0.0);
    assert_close(tree.nodes[first_auto].layout.offset.y, 0.0);
    assert_close(tree.nodes[second_auto].layout.offset.x, 10.0);
    assert_close(tree.nodes[second_auto].layout.offset.y, 0.0);
    assert_close(tree.nodes[locked_column].layout.offset.x, 20.0);
    assert_close(tree.nodes[locked_column].layout.offset.y, 0.0);
}

#[test]
fn grid_auto_placement_orders_in_flow_children_by_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(20.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(10.0), Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let later = tree.push(SimpleNode::new(Style {
        order: 1,
        ..Style::default()
    }));
    let earlier = tree.push(SimpleNode::new(Style {
        order: -1,
        ..Style::default()
    }));
    tree.append_child(root, later);
    tree.append_child(root, earlier);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 10.0));

    assert_close(tree.nodes[earlier].layout.offset.x, 0.0);
    assert_close(tree.nodes[earlier].layout.offset.y, 0.0);
    assert_close(tree.nodes[later].layout.offset.x, 10.0);
    assert_close(tree.nodes[later].layout.offset.y, 0.0);
}

#[test]
fn dense_row_auto_flow_backfills_earlier_holes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(30.0),
        height: Length::points(30.0),
        grid_auto_flow: GridAutoFlow::Dense,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_rows: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        grid_column_span: 2,
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        grid_column_span: 2,
        ..Style::default()
    }));
    let third = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);

    run_rust_layout(&mut tree, root, Constraints::definite(30.0, 30.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 10.0);
    assert_close(tree.nodes[third].layout.offset.x, 20.0);
    assert_close(tree.nodes[third].layout.offset.y, 0.0);
}

#[test]
fn column_dense_auto_flow_backfills_earlier_holes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(30.0),
        height: Length::points(30.0),
        grid_auto_flow: GridAutoFlow::ColumnDense,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_rows: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        grid_row_span: 2,
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        grid_row_span: 2,
        ..Style::default()
    }));
    let third = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);

    run_rust_layout(&mut tree, root, Constraints::definite(30.0, 30.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 10.0);
    assert_close(tree.nodes[second].layout.offset.y, 0.0);
    assert_close(tree.nodes[third].layout.offset.x, 0.0);
    assert_close(tree.nodes[third].layout.offset.y, 20.0);
}

#[test]
fn sparse_locked_row_auto_flow_keeps_later_items_past_previous_locked_item() {
    let (mut tree, root, blocker, wide, small) = locked_row_auto_flow_case(GridAutoFlow::Row);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 10.0));

    assert_close(tree.nodes[blocker].layout.offset.x, 10.0);
    assert_close(tree.nodes[wide].layout.offset.x, 20.0);
    assert_close(tree.nodes[small].layout.offset.x, 40.0);
}

#[test]
fn dense_locked_row_auto_flow_backfills_hole_before_previous_locked_item() {
    let (mut tree, root, blocker, wide, small) = locked_row_auto_flow_case(GridAutoFlow::Dense);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 10.0));

    assert_close(tree.nodes[blocker].layout.offset.x, 10.0);
    assert_close(tree.nodes[wide].layout.offset.x, 20.0);
    assert_close(tree.nodes[small].layout.offset.x, 0.0);
}

#[test]
fn sparse_locked_column_auto_flow_keeps_later_items_past_previous_locked_item() {
    let (mut tree, root, blocker, tall, small) = locked_column_auto_flow_case(GridAutoFlow::Column);

    run_rust_layout(&mut tree, root, Constraints::definite(10.0, 50.0));

    assert_close(tree.nodes[blocker].layout.offset.y, 10.0);
    assert_close(tree.nodes[tall].layout.offset.y, 20.0);
    assert_close(tree.nodes[small].layout.offset.y, 40.0);
}

#[test]
fn dense_locked_column_auto_flow_backfills_hole_before_previous_locked_item() {
    let (mut tree, root, blocker, tall, small) =
        locked_column_auto_flow_case(GridAutoFlow::ColumnDense);

    run_rust_layout(&mut tree, root, Constraints::definite(10.0, 50.0));

    assert_close(tree.nodes[blocker].layout.offset.y, 10.0);
    assert_close(tree.nodes[tall].layout.offset.y, 20.0);
    assert_close(tree.nodes[small].layout.offset.y, 0.0);
}

fn locked_row_auto_flow_case(flow: GridAutoFlow) -> (SimpleTree, usize, usize, usize, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(50.0),
        height: Length::points(10.0),
        grid_auto_flow: flow,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_rows: vec![Length::points(10.0)],
        grid_auto_columns: vec![Length::points(10.0)],
        grid_auto_columns_max: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let blocker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let wide = tree.push(SimpleNode::new(Style {
        grid_row_start: Some(1),
        grid_column_span: 2,
        ..Style::default()
    }));
    let small = tree.push(SimpleNode::new(Style {
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, blocker);
    tree.append_child(root, wide);
    tree.append_child(root, small);
    (tree, root, blocker, wide, small)
}

fn locked_column_auto_flow_case(flow: GridAutoFlow) -> (SimpleTree, usize, usize, usize, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(10.0),
        height: Length::points(50.0),
        grid_auto_flow: flow,
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_auto_rows: vec![Length::points(10.0)],
        grid_auto_rows_max: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let blocker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    let tall = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_span: 2,
        ..Style::default()
    }));
    let small = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, blocker);
    tree.append_child(root, tall);
    tree.append_child(root, small);
    (tree, root, blocker, tall, small)
}

#[test]
fn explicit_grid_line_position_places_child_in_target_cell() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(130.0),
        height: Length::points(70.0),
        grid_template_columns: vec![Length::points(50.0), Length::points(70.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(30.0)],
        column_gap: Length::points(10.0),
        row_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(130.0, 70.0));

    assert_close(tree.nodes[child].layout.offset.x, 60.0);
    assert_close(tree.nodes[child].layout.offset.y, 25.0);
    assert_close(tree.nodes[child].layout.size.width, 70.0);
    assert_close(tree.nodes[child].layout.size.height, 30.0);
}

#[test]
fn explicit_grid_line_end_derives_span_from_start_and_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![
            Length::points(20.0),
            Length::points(30.0),
            Length::points(40.0),
        ],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_column_end: Some(3),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 55.0);
}

#[test]
fn explicit_grid_line_end_without_start_uses_span_to_compute_start() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(70.0),
        height: Length::points(20.0),
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(20.0),
            Length::points(30.0),
        ],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        grid_column_end: Some(4),
        grid_column_span: 2,
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(70.0, 20.0));

    assert_close(tree.nodes[child].layout.offset.x, 15.0);
    assert_close(tree.nodes[child].layout.size.width, 55.0);
}

#[test]
fn reversed_grid_lines_swap_start_and_end_on_both_axes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(60.0),
        height: Length::points(36.0),
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(20.0),
            Length::points(30.0),
        ],
        grid_template_rows: vec![
            Length::points(8.0),
            Length::points(12.0),
            Length::points(16.0),
        ],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(4),
        grid_column_end: Some(2),
        grid_row_start: Some(3),
        grid_row_end: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(60.0, 36.0));

    assert_close(tree.nodes[child].layout.offset.x, 10.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 50.0);
    assert_close(tree.nodes[child].layout.size.height, 20.0);
}

#[test]
fn equal_grid_lines_drop_end_and_use_default_span_on_both_axes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(60.0),
        height: Length::points(36.0),
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(20.0),
            Length::points(30.0),
        ],
        grid_template_rows: vec![
            Length::points(8.0),
            Length::points(12.0),
            Length::points(16.0),
        ],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_column_end: Some(2),
        grid_row_start: Some(2),
        grid_row_end: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(60.0, 36.0));

    assert_close(tree.nodes[child].layout.offset.x, 10.0);
    assert_close(tree.nodes[child].layout.offset.y, 8.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.size.height, 12.0);
}

#[test]
fn negative_grid_lines_resolve_from_explicit_grid_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(70.0),
        height: Length::points(20.0),
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(20.0),
            Length::points(30.0),
        ],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(-3),
        grid_column_end: Some(-1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(70.0, 20.0));

    assert_close(tree.nodes[child].layout.offset.x, 15.0);
    assert_close(tree.nodes[child].layout.size.width, 55.0);
}

#[test]
fn negative_grid_end_line_with_span_computes_start_on_both_axes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(60.0),
        height: Length::points(36.0),
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(20.0),
            Length::points(30.0),
        ],
        grid_template_rows: vec![
            Length::points(8.0),
            Length::points(12.0),
            Length::points(16.0),
        ],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        grid_column_end: Some(-1),
        grid_column_span: 2,
        grid_row_end: Some(-1),
        grid_row_span: 2,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(60.0, 36.0));

    assert_close(tree.nodes[child].layout.offset.x, 10.0);
    assert_close(tree.nodes[child].layout.offset.y, 8.0);
    assert_close(tree.nodes[child].layout.size.width, 50.0);
    assert_close(tree.nodes[child].layout.size.height, 28.0);
}

#[test]
fn negative_grid_start_line_with_span_creates_trailing_implicit_tracks() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(54.0),
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(20.0),
            Length::points(30.0),
        ],
        grid_template_rows: vec![
            Length::points(8.0),
            Length::points(12.0),
            Length::points(16.0),
        ],
        grid_auto_columns: vec![Length::points(40.0)],
        grid_auto_columns_max: vec![Length::points(40.0)],
        grid_auto_rows: vec![Length::points(18.0)],
        grid_auto_rows_max: vec![Length::points(18.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(-2),
        grid_column_span: 2,
        grid_row_start: Some(-2),
        grid_row_span: 2,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 54.0));

    assert_close(tree.nodes[child].layout.offset.x, 30.0);
    assert_close(tree.nodes[child].layout.offset.y, 20.0);
    assert_close(tree.nodes[child].layout.size.width, 70.0);
    assert_close(tree.nodes[child].layout.size.height, 34.0);
}

#[test]
fn mixed_positive_and_negative_grid_lines_resolve_against_explicit_grid_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(60.0),
        height: Length::points(36.0),
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(20.0),
            Length::points(30.0),
        ],
        grid_template_rows: vec![
            Length::points(8.0),
            Length::points(12.0),
            Length::points(16.0),
        ],
        ..Style::default()
    }));
    let positive_start_negative_end = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_column_end: Some(-1),
        grid_row_start: Some(2),
        grid_row_end: Some(-1),
        ..Style::default()
    }));
    let negative_start_positive_end = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(-1),
        grid_column_end: Some(2),
        grid_row_start: Some(-1),
        grid_row_end: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, positive_start_negative_end);
    tree.append_child(root, negative_start_positive_end);

    run_rust_layout(&mut tree, root, Constraints::definite(60.0, 36.0));

    for child in [positive_start_negative_end, negative_start_positive_end] {
        assert_close(tree.nodes[child].layout.offset.x, 10.0);
        assert_close(tree.nodes[child].layout.offset.y, 8.0);
        assert_close(tree.nodes[child].layout.size.width, 50.0);
        assert_close(tree.nodes[child].layout.size.height, 28.0);
    }
}

#[test]
fn positive_grid_end_line_with_span_w3c_expected_leading_implicit_tracks() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(20.0),
            Length::points(30.0),
        ],
        grid_template_rows: vec![
            Length::points(8.0),
            Length::points(12.0),
            Length::points(16.0),
        ],
        grid_auto_columns: vec![Length::points(6.0)],
        grid_auto_columns_max: vec![Length::points(6.0)],
        grid_auto_rows: vec![Length::points(4.0)],
        grid_auto_rows_max: vec![Length::points(4.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        grid_column_end: Some(1),
        grid_column_span: 2,
        grid_row_end: Some(1),
        grid_row_span: 2,
        ..Style::default()
    }));
    let explicit_marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, child);
    tree.append_child(root, explicit_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 72.0);
    assert_close(size.height, 44.0);
    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 12.0);
    assert_close(tree.nodes[child].layout.size.height, 8.0);
    assert_close(tree.nodes[explicit_marker].layout.offset.x, 12.0);
    assert_close(tree.nodes[explicit_marker].layout.offset.y, 8.0);
}

#[test]
fn negative_grid_line_before_explicit_grid_creates_leading_implicit_column() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(20.0),
            Length::points(30.0),
        ],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let leading = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(-5),
            grid_column_end: Some(-4),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(12.0, 10.0),
    ));
    let explicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, leading);
    tree.append_child(root, explicit);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 72.0);
    assert_close(tree.nodes[leading].layout.offset.x, 0.0);
    assert_close(tree.nodes[leading].layout.size.width, 12.0);
    assert_close(tree.nodes[explicit].layout.offset.x, 12.0);
    assert_close(tree.nodes[explicit].layout.size.width, 10.0);
}

#[test]
fn negative_grid_line_before_explicit_grid_creates_leading_implicit_row() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(20.0)],
        ..Style::default()
    }));
    let leading = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(-4),
            grid_row_end: Some(-3),
            ..Style::default()
        },
        Size::new(10.0, 8.0),
    ));
    let explicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, leading);
    tree.append_child(root, explicit);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.height, 38.0);
    assert_close(tree.nodes[leading].layout.offset.y, 0.0);
    assert_close(tree.nodes[leading].layout.size.height, 8.0);
    assert_close(tree.nodes[explicit].layout.offset.y, 8.0);
    assert_close(tree.nodes[explicit].layout.size.height, 10.0);
}

#[test]
fn sparse_leading_implicit_column_auto_flow_keeps_cursor_after_span() {
    let (mut tree, root, blocker, first, second, small) =
        leading_implicit_column_auto_flow_case(GridAutoFlow::Row);

    run_rust_layout(&mut tree, root, Constraints::definite(40.0, 20.0));

    assert_close(tree.nodes[blocker].layout.offset.x, 0.0);
    assert_close(tree.nodes[blocker].layout.offset.y, 0.0);
    assert_close(tree.nodes[first].layout.offset.x, 10.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 10.0);
    assert_close(tree.nodes[small].layout.offset.x, 20.0);
    assert_close(tree.nodes[small].layout.offset.y, 10.0);
}

#[test]
fn dense_leading_implicit_column_auto_flow_backfills_start_side_hole() {
    let (mut tree, root, blocker, first, second, small) =
        leading_implicit_column_auto_flow_case(GridAutoFlow::Dense);

    run_rust_layout(&mut tree, root, Constraints::definite(40.0, 20.0));

    assert_close(tree.nodes[blocker].layout.offset.x, 0.0);
    assert_close(tree.nodes[blocker].layout.offset.y, 0.0);
    assert_close(tree.nodes[first].layout.offset.x, 10.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 10.0);
    assert_close(tree.nodes[small].layout.offset.x, 30.0);
    assert_close(tree.nodes[small].layout.offset.y, 0.0);
}

#[test]
fn sparse_leading_implicit_row_column_auto_flow_keeps_cursor_after_span() {
    let (mut tree, root, blocker, first, second, small) =
        leading_implicit_row_auto_flow_case(GridAutoFlow::Column);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 40.0));

    assert_close(tree.nodes[blocker].layout.offset.x, 0.0);
    assert_close(tree.nodes[blocker].layout.offset.y, 0.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.offset.y, 10.0);
    assert_close(tree.nodes[second].layout.offset.x, 10.0);
    assert_close(tree.nodes[second].layout.offset.y, 0.0);
    assert_close(tree.nodes[small].layout.offset.x, 10.0);
    assert_close(tree.nodes[small].layout.offset.y, 20.0);
}

#[test]
fn dense_leading_implicit_row_column_auto_flow_backfills_start_side_hole() {
    let (mut tree, root, blocker, first, second, small) =
        leading_implicit_row_auto_flow_case(GridAutoFlow::ColumnDense);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 40.0));

    assert_close(tree.nodes[blocker].layout.offset.x, 0.0);
    assert_close(tree.nodes[blocker].layout.offset.y, 0.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.offset.y, 10.0);
    assert_close(tree.nodes[second].layout.offset.x, 10.0);
    assert_close(tree.nodes[second].layout.offset.y, 0.0);
    assert_close(tree.nodes[small].layout.offset.x, 0.0);
    assert_close(tree.nodes[small].layout.offset.y, 30.0);
}

fn leading_implicit_column_auto_flow_case(
    flow: GridAutoFlow,
) -> (SimpleTree, usize, usize, usize, usize, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(40.0),
        height: Length::points(20.0),
        grid_auto_flow: flow,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        grid_auto_columns: vec![Length::points(10.0)],
        grid_auto_columns_max: vec![Length::points(10.0)],
        grid_auto_rows: vec![Length::points(10.0)],
        grid_auto_rows_max: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let blocker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(-5),
        grid_column_end: Some(-4),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        grid_column_span: 2,
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        grid_column_span: 2,
        ..Style::default()
    }));
    let small = grid_child(&mut tree);
    tree.append_child(root, blocker);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, small);
    (tree, root, blocker, first, second, small)
}

fn leading_implicit_row_auto_flow_case(
    flow: GridAutoFlow,
) -> (SimpleTree, usize, usize, usize, usize, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(20.0),
        height: Length::points(40.0),
        grid_auto_flow: flow,
        grid_template_columns: vec![Length::points(10.0), Length::points(10.0)],
        grid_template_rows: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_auto_columns: vec![Length::points(10.0)],
        grid_auto_columns_max: vec![Length::points(10.0)],
        grid_auto_rows: vec![Length::points(10.0)],
        grid_auto_rows_max: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let blocker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(-5),
        grid_row_end: Some(-4),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        grid_row_span: 2,
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        grid_row_span: 2,
        ..Style::default()
    }));
    let small = grid_child(&mut tree);
    tree.append_child(root, blocker);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, small);
    (tree, root, blocker, first, second, small)
}

#[test]
fn positive_implicit_grid_columns_repeat_auto_track_pattern() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        grid_auto_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_auto_columns_max: vec![Length::points(20.0), Length::points(30.0)],
        ..Style::default()
    }));
    let explicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let first_implicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let second_implicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, explicit);
    tree.append_child(root, first_implicit);
    tree.append_child(root, second_implicit);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 60.0);
    assert_close(tree.nodes[explicit].layout.size.width, 10.0);
    assert_close(tree.nodes[first_implicit].layout.offset.x, 10.0);
    assert_close(tree.nodes[first_implicit].layout.size.width, 20.0);
    assert_close(tree.nodes[second_implicit].layout.offset.x, 30.0);
    assert_close(tree.nodes[second_implicit].layout.size.width, 30.0);
}

#[test]
fn leading_implicit_grid_columns_align_auto_track_pattern_before_explicit_grid() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        grid_auto_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_auto_columns_max: vec![Length::points(20.0), Length::points(30.0)],
        ..Style::default()
    }));
    let leading_span = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(-4),
        grid_column_end: Some(-2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let explicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, leading_span);
    tree.append_child(root, explicit);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 60.0);
    assert_close(tree.nodes[leading_span].layout.offset.x, 0.0);
    assert_close(tree.nodes[leading_span].layout.size.width, 50.0);
    assert_close(tree.nodes[explicit].layout.offset.x, 50.0);
    assert_close(tree.nodes[explicit].layout.size.width, 10.0);
}

#[test]
fn positive_implicit_grid_rows_repeat_auto_track_pattern() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        grid_auto_rows: vec![Length::points(20.0), Length::points(30.0)],
        grid_auto_rows_max: vec![Length::points(20.0), Length::points(30.0)],
        ..Style::default()
    }));
    let explicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let first_implicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    let second_implicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(3),
        ..Style::default()
    }));
    tree.append_child(root, explicit);
    tree.append_child(root, first_implicit);
    tree.append_child(root, second_implicit);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.height, 60.0);
    assert_close(tree.nodes[explicit].layout.size.height, 10.0);
    assert_close(tree.nodes[first_implicit].layout.offset.y, 10.0);
    assert_close(tree.nodes[first_implicit].layout.size.height, 20.0);
    assert_close(tree.nodes[second_implicit].layout.offset.y, 30.0);
    assert_close(tree.nodes[second_implicit].layout.size.height, 30.0);
}

#[test]
fn leading_implicit_grid_rows_align_auto_track_pattern_before_explicit_grid() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        grid_auto_rows: vec![Length::points(20.0), Length::points(30.0)],
        grid_auto_rows_max: vec![Length::points(20.0), Length::points(30.0)],
        ..Style::default()
    }));
    let leading_span = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(-4),
        grid_row_end: Some(-2),
        ..Style::default()
    }));
    let explicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, leading_span);
    tree.append_child(root, explicit);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.height, 60.0);
    assert_close(tree.nodes[leading_span].layout.offset.y, 0.0);
    assert_close(tree.nodes[leading_span].layout.size.height, 50.0);
    assert_close(tree.nodes[explicit].layout.offset.y, 50.0);
    assert_close(tree.nodes[explicit].layout.size.height, 10.0);
}

#[test]
fn definite_grid_auto_fit_content_percent_column_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(120.0),
        height: Length::points(10.0),
        grid_auto_columns: vec![Length::points(20.0), Length::points(10.0)],
        grid_auto_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
            Length::points(60.0),
        ],
        grid_auto_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(90.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(120.0, 10.0));

    assert_close(size.width, 120.0);
    assert_close(tree.nodes[following].layout.offset.x, 60.0);
}

#[test]
fn definite_grid_auto_fit_content_calc_column_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(120.0),
        height: Length::points(10.0),
        grid_auto_columns: vec![Length::points(20.0), Length::points(10.0)],
        grid_auto_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
            Length::points(50.0),
        ],
        grid_auto_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(90.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(120.0, 10.0));

    assert_close(size.width, 120.0);
    assert_close(tree.nodes[following].layout.offset.x, 70.0);
}

#[test]
fn definite_grid_auto_fit_content_percent_row_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(10.0),
        height: Length::points(120.0),
        grid_auto_columns: vec![Length::points(10.0)],
        grid_auto_rows: vec![Length::points(20.0), Length::points(10.0)],
        grid_auto_rows_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
            Length::points(60.0),
        ],
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(10.0, 90.0),
    ));
    let following = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(10.0, 120.0));

    assert_close(size.height, 120.0);
    assert_close(tree.nodes[following].layout.offset.y, 60.0);
}

#[test]
fn definite_grid_auto_fit_content_calc_row_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(10.0),
        height: Length::points(120.0),
        grid_auto_columns: vec![Length::points(10.0)],
        grid_auto_rows: vec![Length::points(20.0), Length::points(10.0)],
        grid_auto_rows_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
            Length::points(50.0),
        ],
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(10.0, 90.0),
    ));
    let following = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(10.0, 120.0));

    assert_close(size.height, 120.0);
    assert_close(tree.nodes[following].layout.offset.y, 70.0);
}

#[test]
fn indefinite_grid_auto_fit_content_column_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_auto_columns: vec![Length::points(20.0), Length::points(10.0)],
        grid_auto_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed(40.0))),
            Length::points(10.0),
        ],
        grid_auto_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(70.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 50.0);
    assert_close(tree.nodes[following].layout.offset.x, 40.0);
}

#[test]
fn indefinite_grid_auto_fit_content_row_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_auto_columns: vec![Length::points(10.0)],
        grid_auto_rows: vec![Length::points(20.0), Length::points(10.0)],
        grid_auto_rows_max: vec![
            Length::fit_content(Some(BaseLength::fixed(40.0))),
            Length::points(10.0),
        ],
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(10.0, 70.0),
    ));
    let following = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.height, 50.0);
    assert_close(tree.nodes[following].layout.offset.y, 40.0);
}

#[test]
fn grid_items_center_child_with_justify_and_align_self() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(100.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        justify_self: JustifyItems::Center,
        align_self: Some(AlignItems::Center),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.x, 40.0);
    assert_close(tree.nodes[child].layout.offset.y, 45.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn grid_align_items_end_and_align_self_start_variants() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(50.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(30.0)],
        align_items: AlignItems::End,
        ..Style::default()
    }));
    let end_aligned = tree.push(SimpleNode::new(Style {
        width: Length::points(10.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let start_aligned = tree.push(SimpleNode::new(Style {
        width: Length::points(10.0),
        height: Length::points(10.0),
        align_self: Some(AlignItems::Start),
        ..Style::default()
    }));
    tree.append_child(root, end_aligned);
    tree.append_child(root, start_aligned);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 40.0));

    assert_close(tree.nodes[end_aligned].layout.offset.x, 0.0);
    assert_close(tree.nodes[end_aligned].layout.offset.y, 20.0);
    assert_close(tree.nodes[start_aligned].layout.offset.x, 20.0);
    assert_close(tree.nodes[start_aligned].layout.offset.y, 0.0);
}

#[test]
fn grid_align_items_baseline_uses_start_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(80.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(80.0)],
        align_items: AlignItems::Baseline,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style {
            width: Length::MaxContent,
            height: Length::MaxContent,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
        6.0,
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 80.0));

    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn grid_align_self_baseline_overrides_container_end_alignment_to_start() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(80.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(80.0)],
        align_items: AlignItems::End,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style {
            width: Length::MaxContent,
            height: Length::MaxContent,
            align_self: Some(AlignItems::Baseline),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
        6.0,
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 80.0));

    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn grid_container_baseline_uses_first_row_item_baseline() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(40.0),
        height: Length::points(50.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        row_gap: Length::points(5.0),
        padding: Rect::new(
            Length::points(0.0),
            Length::points(3.0),
            Length::points(0.0),
            Length::points(0.0),
        ),
        border: Rect::new(0.0, 2.0, 0.0, 0.0),
        align_items: AlignItems::Start,
        ..Style::default()
    }));
    let later_row = tree.push(SimpleNode::with_measured_size_and_baseline(
        grid_start_item(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(2),
            width: Length::MaxContent,
            height: Length::MaxContent,
            ..Style::default()
        }),
        Size::new(8.0, 10.0),
        7.0,
    ));
    let first_row = tree.push(SimpleNode::with_measured_size_and_baseline(
        grid_start_item(Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            width: Length::MaxContent,
            height: Length::MaxContent,
            ..Style::default()
        }),
        Size::new(8.0, 10.0),
        4.0,
    ));
    tree.append_child(root, later_row);
    tree.append_child(root, first_row);

    run_rust_layout(&mut tree, root, Constraints::definite(40.0, 50.0));

    assert_close(tree.nodes[root].layout.baseline.unwrap(), 4.0);
    assert_close(tree.nodes[later_row].layout.offset.y, 25.0);
    assert_close(tree.nodes[first_row].layout.offset.y, 0.0);
}

#[test]
fn grid_container_baseline_uses_row_major_item_before_source_order() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(40.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        align_items: AlignItems::Start,
        ..Style::default()
    }));
    let second_column = tree.push(SimpleNode::with_measured_size_and_baseline(
        grid_start_item(Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            width: Length::MaxContent,
            height: Length::MaxContent,
            ..Style::default()
        }),
        Size::new(8.0, 10.0),
        12.0,
    ));
    let first_column = tree.push(SimpleNode::with_measured_size_and_baseline(
        grid_start_item(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            width: Length::MaxContent,
            height: Length::MaxContent,
            ..Style::default()
        }),
        Size::new(8.0, 10.0),
        5.0,
    ));
    tree.append_child(root, second_column);
    tree.append_child(root, first_column);

    run_rust_layout(&mut tree, root, Constraints::definite(40.0, 20.0));

    assert_close(tree.nodes[root].layout.baseline.unwrap(), 5.0);
    assert_close(tree.nodes[first_column].layout.offset.x, 0.0);
    assert_close(tree.nodes[second_column].layout.offset.x, 20.0);
}

#[test]
fn non_stretch_grid_item_receives_intrinsic_auto_axis_constraints() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    }));
    let child = tree.push(MeasuringNode::measured(
        Style {
            justify_self: JustifyItems::Center,
            align_self: Some(AlignItems::Center),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    let constraints = tree.nodes[child]
        .last_constraints
        .expect("child should have been measured");
    assert!(constraints.width.near(SideConstraint::indefinite()));
    assert!(constraints.height.near(SideConstraint::indefinite()));
    assert_close(tree.nodes[child].layout.offset.x, 40.0);
    assert_close(tree.nodes[child].layout.offset.y, 15.0);
}

#[test]
fn stretch_grid_item_receives_definite_cell_constraints_without_auto_margins() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    }));
    let child = tree.push(MeasuringNode::measured(
        Style::default(),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    let constraints = tree.nodes[child]
        .last_constraints
        .expect("child should have been measured");
    assert!(constraints.width.near(SideConstraint::definite(100.0)));
    assert!(constraints.height.near(SideConstraint::definite(40.0)));
}

#[test]
fn grid_stretch_does_not_override_explicit_child_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn grid_stretch_does_not_override_max_content_child_size() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    }));
    let child = tree.push(MeasuringNode::measured(
        Style {
            width: Length::MaxContent,
            height: Length::MaxContent,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    let constraints = tree.nodes[child]
        .last_constraints
        .expect("child should have been measured");
    assert!(constraints.width.near(SideConstraint::indefinite()));
    assert!(constraints.height.near(SideConstraint::indefinite()));
    assert_close(tree.nodes[child].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn grid_stretch_does_not_expand_max_content_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::MaxContent, Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, marker);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[intrinsic].layout.size.width, 20.0);
    assert_close(tree.nodes[marker].layout.offset.x, 20.0);
}

#[test]
fn grid_fit_content_child_receives_at_most_measure_constraints() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    }));
    let child = tree.push(MeasuringNode::measured(
        Style {
            width: Length::fit_content(Some(BaseLength::fixed(60.0))),
            height: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 50.0))),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(90.0, 30.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 40.0));

    let constraints = tree.nodes[child]
        .last_constraints
        .expect("child should have been measured");
    assert!(constraints.width.near(SideConstraint::at_most(60.0)));
    assert!(constraints.height.near(SideConstraint::at_most(25.0)));
    assert_close(tree.nodes[child].layout.size.width, 60.0);
    assert_close(tree.nodes[child].layout.size.height, 25.0);
}

#[test]
fn grid_fit_content_keyword_child_falls_back_to_available_space() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Grid,
        width: Length::points(80.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::points(80.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    }));
    let child = tree.push(MeasuringNode::measured(
        Style {
            width: Length::fit_content(None),
            height: Length::fit_content(None),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(120.0, 60.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(80.0, 40.0));

    let constraints = tree.nodes[child]
        .last_constraints
        .expect("child should have been measured");
    assert!(constraints.width.near(SideConstraint::at_most(80.0)));
    assert!(constraints.height.near(SideConstraint::at_most(40.0)));
    assert_close(tree.nodes[child].layout.size.width, 80.0);
    assert_close(tree.nodes[child].layout.size.height, 40.0);
}

#[test]
fn grid_item_horizontal_auto_margins_override_justify_self() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            margin: Rect::new(Length::Auto, Length::Auto, Length::ZERO, Length::ZERO),
            justify_self: JustifyItems::End,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[child].layout.offset.x, 40.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.margin.left, 40.0);
    assert_close(tree.nodes[child].layout.margin.right, 40.0);
}

#[test]
fn rtl_grid_places_inline_tracks_from_right_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(5.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(7.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 95.0);
    assert_close(tree.nodes[second].layout.offset.x, 63.0);
}

#[test]
fn rtl_grid_justify_self_end_uses_left_edge_of_cell() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::End,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(5.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[child].layout.offset.x, 80.0);
}

#[test]
fn rtl_grid_justify_self_center_centers_item_in_cell() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::End,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            justify_self: JustifyItems::Center,
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[child].layout.offset.x, 40.0);
}

#[test]
fn rtl_grid_auto_inline_margins_center_item_in_cell() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::End,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            margin: Rect::new(Length::Auto, Length::Auto, Length::ZERO, Length::ZERO),
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(6.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[child].layout.offset.x, 87.0);
    assert_close(tree.nodes[child].layout.margin.left, 7.0);
    assert_close(tree.nodes[child].layout.margin.right, 7.0);
}

#[test]
fn rtl_grid_justify_content_center_offsets_track_group_from_right_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(5.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(7.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 75.0);
    assert_close(tree.nodes[second].layout.offset.x, 43.0);
}

#[test]
fn rtl_grid_justify_content_start_variants_align_track_group_to_right_edge() {
    for justify_content in [JustifyContent::FlexStart, JustifyContent::Start] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Grid,
            direction: Direction::Rtl,
            justify_content,
            width: Length::points(100.0),
            height: Length::points(10.0),
            grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
            grid_template_rows: vec![Length::points(10.0)],
            column_gap: Length::points(10.0),
            justify_items: JustifyItems::Start,
            align_items: AlignItems::FlexStart,
            ..Style::default()
        }));
        let first = tree.push(SimpleNode::with_measured_size(
            Style {
                grid_column_start: Some(1),
                grid_row_start: Some(1),
                ..Style::default()
            },
            Size::new(5.0, 10.0),
        ));
        let second = tree.push(SimpleNode::with_measured_size(
            Style {
                grid_column_start: Some(2),
                grid_row_start: Some(1),
                ..Style::default()
            },
            Size::new(7.0, 10.0),
        ));
        tree.append_child(root, first);
        tree.append_child(root, second);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

        assert_close(tree.nodes[first].layout.offset.x, 95.0);
        assert_close(tree.nodes[second].layout.offset.x, 63.0);
    }
}

#[test]
fn rtl_grid_justify_content_end_variants_align_track_group_to_left_edge() {
    for justify_content in [JustifyContent::FlexEnd, JustifyContent::End] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Grid,
            direction: Direction::Rtl,
            justify_content,
            width: Length::points(100.0),
            height: Length::points(10.0),
            grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
            grid_template_rows: vec![Length::points(10.0)],
            column_gap: Length::points(10.0),
            justify_items: JustifyItems::Start,
            align_items: AlignItems::FlexStart,
            ..Style::default()
        }));
        let first = tree.push(SimpleNode::with_measured_size(
            Style {
                grid_column_start: Some(1),
                grid_row_start: Some(1),
                ..Style::default()
            },
            Size::new(5.0, 10.0),
        ));
        let second = tree.push(SimpleNode::with_measured_size(
            Style {
                grid_column_start: Some(2),
                grid_row_start: Some(1),
                ..Style::default()
            },
            Size::new(7.0, 10.0),
        ));
        tree.append_child(root, first);
        tree.append_child(root, second);

        run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

        assert_close(tree.nodes[first].layout.offset.x, 55.0);
        assert_close(tree.nodes[second].layout.offset.x, 23.0);
    }
}

#[test]
fn rtl_grid_justify_content_space_between_keeps_right_origin_lines() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        justify_content: JustifyContent::SpaceBetween,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(5.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(7.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 95.0);
    assert_close(tree.nodes[second].layout.offset.x, 23.0);
}

#[test]
fn rtl_grid_justify_content_space_evenly_offsets_track_group_from_right_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        justify_content: JustifyContent::SpaceEvenly,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(5.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(7.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 82.0);
    assert_close(tree.nodes[second].layout.offset.x, 36.0);
}

#[test]
fn rtl_grid_justify_content_space_around_offsets_track_group_from_right_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        justify_content: JustifyContent::SpaceAround,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(5.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(7.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 85.0);
    assert_close(tree.nodes[second].layout.offset.x, 33.0);
}

#[test]
fn rtl_grid_justify_content_space_evenly_falls_back_to_right_edge_when_tracks_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        justify_content: JustifyContent::SpaceEvenly,
        width: Length::points(50.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(40.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(5.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(7.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 45.0);
    assert_close(tree.nodes[second].layout.offset.x, -7.0);
}

#[test]
fn rtl_grid_justify_content_space_around_falls_back_to_right_edge_when_tracks_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        justify_content: JustifyContent::SpaceAround,
        width: Length::points(50.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(40.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(5.0, 10.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(7.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(50.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 45.0);
    assert_close(tree.nodes[second].layout.offset.x, -7.0);
}

#[test]
fn grid_item_vertical_auto_margins_override_align_self() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(20.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(100.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            margin: Rect::new(Length::ZERO, Length::ZERO, Length::Auto, Length::Auto),
            align_self: Some(AlignItems::FlexEnd),
            ..Style::default()
        },
        Size::new(10.0, 20.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.y, 40.0);
    assert_close(tree.nodes[child].layout.size.height, 20.0);
    assert_close(tree.nodes[child].layout.margin.top, 40.0);
    assert_close(tree.nodes[child].layout.margin.bottom, 40.0);
}

#[test]
fn grid_item_single_start_auto_margin_pushes_item_to_end() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            margin: Rect::new(Length::Auto, Length::ZERO, Length::ZERO, Length::ZERO),
            justify_self: JustifyItems::Start,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[child].layout.offset.x, 80.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.margin.left, 80.0);
    assert_close(tree.nodes[child].layout.margin.right, 0.0);
}

#[test]
fn grid_item_single_end_auto_margin_keeps_item_at_start() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            margin: Rect::new(Length::ZERO, Length::Auto, Length::ZERO, Length::ZERO),
            justify_self: JustifyItems::End,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.margin.left, 0.0);
    assert_close(tree.nodes[child].layout.margin.right, 80.0);
}

#[test]
fn absolute_grid_item_uses_grid_area_as_containing_block_for_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(20.0)],
        column_gap: Length::points(5.0),
        row_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(2),
        grid_column_end: Some(3),
        grid_row_start: Some(1),
        grid_row_end: Some(3),
        left: Length::points(2.0),
        right: Length::points(3.0),
        top: Length::points(1.0),
        bottom: Length::points(4.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 27.0);
    assert_close(tree.nodes[child].layout.offset.y, 1.0);
    assert_close(tree.nodes[child].layout.size.width, 25.0);
    assert_close(tree.nodes[child].layout.size.height, 30.0);
}

#[test]
fn absolute_grid_item_excludes_trailing_gutter_from_internal_end_line() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(1),
        grid_column_end: Some(2),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        left: Length::ZERO,
        right: Length::ZERO,
        top: Length::ZERO,
        bottom: Length::ZERO,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn rtl_absolute_grid_item_uses_right_origin_grid_area() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(5.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[child].layout.offset.x, 95.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
}

#[test]
fn rtl_absolute_grid_item_tracks_justify_content_end_area_shift() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        justify_content: JustifyContent::End,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(5.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[child].layout.offset.x, 55.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
}

#[test]
fn rtl_absolute_grid_item_left_inset_remains_physical_left_offset() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_column_end: Some(2),
            grid_row_start: Some(1),
            left: Length::points(2.0),
            ..Style::default()
        },
        Size::new(5.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[child].layout.offset.x, 82.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
}

#[test]
fn rtl_absolute_grid_item_right_inset_uses_physical_right_offset() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_column_end: Some(2),
            grid_row_start: Some(1),
            right: Length::points(2.0),
            ..Style::default()
        },
        Size::new(5.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[child].layout.offset.x, 93.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
}

#[test]
fn fixed_grid_item_uses_grid_area_without_root_fixed_pass_overwrite() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        grid_column_start: Some(2),
        grid_column_end: Some(3),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        left: Length::points(2.0),
        right: Length::points(3.0),
        top: Length::points(1.0),
        bottom: Length::points(4.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 27.0);
    assert_close(tree.nodes[child].layout.offset.y, 1.0);
    assert_close(tree.nodes[child].layout.size.width, 25.0);
    assert_close(tree.nodes[child].layout.size.height, 5.0);
}

#[test]
fn absolute_grid_item_max_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(1),
        grid_column_end: Some(2),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        width: Length::MaxContent,
        height: Length::MaxContent,
        left: Length::points(3.0),
        top: Length::points(4.0),
        ..Style::default()
    }));
    let grandchild = tree.push(SimpleNode::new(Style {
        width: Length::points(70.0),
        height: Length::points(25.0),
        ..Style::default()
    }));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 3.0);
    assert_close(tree.nodes[child].layout.offset.y, 4.0);
    assert_close(tree.nodes[child].layout.size.width, 70.0);
    assert_close(tree.nodes[child].layout.size.height, 25.0);
}

#[test]
fn fixed_grid_item_max_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        grid_column_start: Some(2),
        grid_column_end: Some(3),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        width: Length::MaxContent,
        height: Length::MaxContent,
        left: Length::points(3.0),
        top: Length::points(4.0),
        ..Style::default()
    }));
    let grandchild = tree.push(SimpleNode::new(Style {
        width: Length::points(70.0),
        height: Length::points(25.0),
        ..Style::default()
    }));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 58.0);
    assert_close(tree.nodes[child].layout.offset.y, 4.0);
    assert_close(tree.nodes[child].layout.size.width, 70.0);
    assert_close(tree.nodes[child].layout.size.height, 25.0);
}

#[test]
fn absolute_grid_item_fit_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(1),
        grid_column_end: Some(2),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        width: Length::fit_content(Some(BaseLength::fixed(40.0))),
        height: Length::fit_content(Some(BaseLength::fixed(15.0))),
        left: Length::points(3.0),
        top: Length::points(4.0),
        ..Style::default()
    }));
    let grandchild = tree.push(SimpleNode::new(Style {
        width: Length::points(70.0),
        height: Length::points(25.0),
        ..Style::default()
    }));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 3.0);
    assert_close(tree.nodes[child].layout.offset.y, 4.0);
    assert_close(tree.nodes[child].layout.size.width, 70.0);
    assert_close(tree.nodes[child].layout.size.height, 25.0);
}

#[test]
fn fixed_grid_item_fit_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        grid_column_start: Some(2),
        grid_column_end: Some(3),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        width: Length::fit_content(Some(BaseLength::fixed(40.0))),
        height: Length::fit_content(Some(BaseLength::fixed(15.0))),
        left: Length::points(3.0),
        top: Length::points(4.0),
        ..Style::default()
    }));
    let grandchild = tree.push(SimpleNode::new(Style {
        width: Length::points(70.0),
        height: Length::points(25.0),
        ..Style::default()
    }));
    tree.append_child(root, child);
    tree.append_child(child, grandchild);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 58.0);
    assert_close(tree.nodes[child].layout.offset.y, 4.0);
    assert_close(tree.nodes[child].layout.size.width, 70.0);
    assert_close(tree.nodes[child].layout.size.height, 25.0);
}

#[test]
fn absolute_measured_grid_item_max_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_column_end: Some(2),
            grid_row_start: Some(1),
            grid_row_end: Some(2),
            width: Length::MaxContent,
            height: Length::MaxContent,
            left: Length::points(3.0),
            top: Length::points(4.0),
            ..Style::default()
        },
        Size::new(70.0, 25.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 3.0);
    assert_close(tree.nodes[child].layout.offset.y, 4.0);
    assert_close(tree.nodes[child].layout.size.width, 70.0);
    assert_close(tree.nodes[child].layout.size.height, 25.0);
}

#[test]
fn fixed_measured_grid_item_max_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Fixed,
            grid_column_start: Some(2),
            grid_column_end: Some(3),
            grid_row_start: Some(1),
            grid_row_end: Some(2),
            width: Length::MaxContent,
            height: Length::MaxContent,
            left: Length::points(3.0),
            top: Length::points(4.0),
            ..Style::default()
        },
        Size::new(70.0, 25.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 58.0);
    assert_close(tree.nodes[child].layout.offset.y, 4.0);
    assert_close(tree.nodes[child].layout.size.width, 70.0);
    assert_close(tree.nodes[child].layout.size.height, 25.0);
}

#[test]
fn absolute_measured_grid_item_fit_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_column_end: Some(2),
            grid_row_start: Some(1),
            grid_row_end: Some(2),
            width: Length::fit_content(Some(BaseLength::fixed(40.0))),
            height: Length::fit_content(Some(BaseLength::fixed(15.0))),
            left: Length::points(3.0),
            top: Length::points(4.0),
            ..Style::default()
        },
        Size::new(70.0, 25.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 3.0);
    assert_close(tree.nodes[child].layout.offset.y, 4.0);
    assert_close(tree.nodes[child].layout.size.width, 70.0);
    assert_close(tree.nodes[child].layout.size.height, 25.0);
}

#[test]
fn fixed_measured_grid_item_fit_content_uses_grid_area_and_natural_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Fixed,
            grid_column_start: Some(2),
            grid_column_end: Some(3),
            grid_row_start: Some(1),
            grid_row_end: Some(2),
            width: Length::fit_content(Some(BaseLength::fixed(40.0))),
            height: Length::fit_content(Some(BaseLength::fixed(15.0))),
            left: Length::points(3.0),
            top: Length::points(4.0),
            ..Style::default()
        },
        Size::new(70.0, 25.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 58.0);
    assert_close(tree.nodes[child].layout.offset.y, 4.0);
    assert_close(tree.nodes[child].layout.size.width, 70.0);
    assert_close(tree.nodes[child].layout.size.height, 25.0);
}

#[test]
fn absolute_grid_item_uses_grid_alignment_when_insets_are_auto() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_column_end: Some(2),
            grid_row_start: Some(1),
            grid_row_end: Some(2),
            justify_self: JustifyItems::End,
            align_self: Some(AlignItems::Center),
            ..Style::default()
        },
        Size::new(10.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[child].layout.offset.x, 40.0);
    assert_close(tree.nodes[child].layout.offset.y, 15.0);
    assert_close(tree.nodes[child].layout.size.width, 10.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn absolute_grid_items_use_bottom_inset_and_block_end_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(50.0)],
        grid_template_rows: vec![Length::points(40.0)],
        ..Style::default()
    }));
    let bottom_inset = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_column_end: Some(2),
            grid_row_start: Some(1),
            grid_row_end: Some(2),
            left: Length::ZERO,
            bottom: Length::points(5.0),
            ..Style::default()
        },
        Size::new(10.0, 10.0),
    ));
    let block_end_aligned = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            grid_column_start: Some(1),
            grid_column_end: Some(2),
            grid_row_start: Some(1),
            grid_row_end: Some(2),
            left: Length::ZERO,
            align_self: Some(AlignItems::End),
            ..Style::default()
        },
        Size::new(10.0, 10.0),
    ));
    tree.append_child(root, bottom_inset);
    tree.append_child(root, block_end_aligned);

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(tree.nodes[bottom_inset].layout.offset.x, 0.0);
    assert_close(tree.nodes[bottom_inset].layout.offset.y, 25.0);
    assert_close(tree.nodes[block_end_aligned].layout.offset.x, 0.0);
    assert_close(tree.nodes[block_end_aligned].layout.offset.y, 30.0);
}

#[test]
fn absolute_grid_item_static_position_uses_content_edges_with_padding_and_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(60.0),
        padding: Rect::new(
            Length::points(10.0),
            Length::points(5.0),
            Length::points(4.0),
            Length::points(6.0),
        ),
        border: Rect::new(2.0, 0.0, 3.0, 0.0),
        grid_template_columns: vec![Length::points(30.0)],
        grid_template_rows: vec![Length::points(20.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::Start,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            justify_self: JustifyItems::End,
            align_self: Some(AlignItems::Center),
            margin: Rect::new(
                Length::points(3.0),
                Length::points(4.0),
                Length::points(2.0),
                Length::points(6.0),
            ),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 60.0));

    assert_close(tree.nodes[child].layout.offset.x, 88.0);
    assert_close(tree.nodes[child].layout.offset.y, 30.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn fixed_grid_item_static_position_uses_content_edges_with_padding_and_margins() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(60.0),
        padding: Rect::new(
            Length::points(10.0),
            Length::points(5.0),
            Length::points(4.0),
            Length::points(6.0),
        ),
        border: Rect::new(2.0, 0.0, 3.0, 0.0),
        grid_template_columns: vec![Length::points(30.0)],
        grid_template_rows: vec![Length::points(20.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::Start,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Fixed,
            justify_self: JustifyItems::Center,
            align_self: Some(AlignItems::End),
            margin: Rect::new(
                Length::points(3.0),
                Length::points(4.0),
                Length::points(2.0),
                Length::points(6.0),
            ),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 60.0));

    assert_close(tree.nodes[child].layout.offset.x, 52.0);
    assert_close(tree.nodes[child].layout.offset.y, 51.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn absolute_grid_item_static_position_auto_margins_follow_grid_item_rules() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(60.0),
        justify_items: JustifyItems::End,
        align_items: AlignItems::End,
        ..Style::default()
    }));
    let both_auto = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            margin: Rect::new(Length::Auto, Length::Auto, Length::Auto, Length::Auto),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let start_auto = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::Start),
            margin: Rect::new(Length::Auto, Length::ZERO, Length::Auto, Length::ZERO),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    let end_auto = tree.push(SimpleNode::with_measured_size(
        Style {
            position: PositionType::Absolute,
            justify_self: JustifyItems::End,
            align_self: Some(AlignItems::End),
            margin: Rect::new(Length::ZERO, Length::Auto, Length::ZERO, Length::Auto),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, both_auto);
    tree.append_child(root, start_auto);
    tree.append_child(root, end_auto);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 60.0));

    assert_close(tree.nodes[both_auto].layout.offset.x, 40.0);
    assert_close(tree.nodes[both_auto].layout.offset.y, 25.0);
    assert_close(tree.nodes[start_auto].layout.offset.x, 80.0);
    assert_close(tree.nodes[start_auto].layout.offset.y, 50.0);
    assert_close(tree.nodes[end_auto].layout.offset.x, 0.0);
    assert_close(tree.nodes[end_auto].layout.offset.y, 0.0);
}

#[test]
fn absolute_and_fixed_grid_static_position_self_alignment_matrix() {
    let justify_cases = [
        (JustifyItems::Auto, 74.0),
        (JustifyItems::Stretch, 2.0),
        (JustifyItems::Start, 2.0),
        (JustifyItems::Center, 38.0),
        (JustifyItems::End, 74.0),
    ];
    let align_cases = [
        (None, 45.0),
        (Some(AlignItems::Stretch), 3.0),
        (Some(AlignItems::FlexStart), 3.0),
        (Some(AlignItems::Start), 3.0),
        (Some(AlignItems::Center), 24.0),
        (Some(AlignItems::FlexEnd), 45.0),
        (Some(AlignItems::End), 45.0),
        (Some(AlignItems::Baseline), 3.0),
    ];

    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(60.0),
        justify_items: JustifyItems::End,
        align_items: AlignItems::End,
        ..Style::default()
    }));
    let mut expected = Vec::new();
    for position in [PositionType::Absolute, PositionType::Fixed] {
        for (justify_self, expected_x) in justify_cases {
            for (align_self, expected_y) in align_cases {
                let child = tree.push(SimpleNode::with_measured_size(
                    Style {
                        position,
                        justify_self,
                        align_self,
                        margin: Rect::new(
                            Length::points(2.0),
                            Length::points(6.0),
                            Length::points(3.0),
                            Length::points(5.0),
                        ),
                        ..Style::default()
                    },
                    Size::new(20.0, 10.0),
                ));
                tree.append_child(root, child);
                expected.push((child, expected_x, expected_y));
            }
        }
    }

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 60.0));

    for (child, expected_x, expected_y) in expected {
        assert_close(tree.nodes[child].layout.offset.x, expected_x);
        assert_close(tree.nodes[child].layout.offset.y, expected_y);
        assert_close(tree.nodes[child].layout.size.width, 20.0);
        assert_close(tree.nodes[child].layout.size.height, 10.0);
    }
}

#[derive(Clone, Copy)]
struct GridInsetAxisCase {
    start: Length,
    end: Length,
    offset: f32,
    size: f32,
}

#[test]
fn absolute_and_fixed_grid_inset_pair_matrix_uses_grid_area_containing_block() {
    let inline_cases = [
        GridInsetAxisCase {
            start: Length::Auto,
            end: Length::Auto,
            offset: 66.0,
            size: 20.0,
        },
        GridInsetAxisCase {
            start: Length::points(7.0),
            end: Length::Auto,
            offset: 19.0,
            size: 20.0,
        },
        GridInsetAxisCase {
            start: Length::Auto,
            end: Length::points(11.0),
            offset: 55.0,
            size: 20.0,
        },
        GridInsetAxisCase {
            start: Length::points(7.0),
            end: Length::points(11.0),
            offset: 19.0,
            size: 56.0,
        },
    ];
    let block_cases = [
        GridInsetAxisCase {
            start: Length::Auto,
            end: Length::Auto,
            offset: 30.0,
            size: 10.0,
        },
        GridInsetAxisCase {
            start: Length::points(6.0),
            end: Length::Auto,
            offset: 14.0,
            size: 10.0,
        },
        GridInsetAxisCase {
            start: Length::Auto,
            end: Length::points(8.0),
            offset: 22.0,
            size: 10.0,
        },
        GridInsetAxisCase {
            start: Length::points(6.0),
            end: Length::points(8.0),
            offset: 14.0,
            size: 18.0,
        },
    ];

    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(80.0),
            Length::points(10.0),
        ],
        grid_template_rows: vec![
            Length::points(5.0),
            Length::points(40.0),
            Length::points(5.0),
        ],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::Start,
        ..Style::default()
    }));
    let mut expected = Vec::new();
    for position in [PositionType::Absolute, PositionType::Fixed] {
        for inline in inline_cases {
            for block in block_cases {
                let child = tree.push(SimpleNode::with_measured_size(
                    Style {
                        position,
                        grid_column_start: Some(2),
                        grid_column_end: Some(3),
                        grid_row_start: Some(2),
                        grid_row_end: Some(3),
                        justify_self: JustifyItems::End,
                        align_self: Some(AlignItems::End),
                        left: inline.start,
                        right: inline.end,
                        top: block.start,
                        bottom: block.end,
                        margin: Rect::new(
                            Length::points(2.0),
                            Length::points(4.0),
                            Length::points(3.0),
                            Length::points(5.0),
                        ),
                        ..Style::default()
                    },
                    Size::new(20.0, 10.0),
                ));
                tree.append_child(root, child);
                expected.push((child, inline.offset, block.offset, inline.size, block.size));
            }
        }
    }

    run_rust_layout(&mut tree, root, Constraints::indefinite());

    for (child, expected_x, expected_y, expected_width, expected_height) in expected {
        assert_close(tree.nodes[child].layout.offset.x, expected_x);
        assert_close(tree.nodes[child].layout.offset.y, expected_y);
        assert_close(tree.nodes[child].layout.size.width, expected_width);
        assert_close(tree.nodes[child].layout.size.height, expected_height);
    }
}

#[test]
fn grid_absolute_child_does_not_affect_sizing_or_auto_placement() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        width: Length::points(500.0),
        height: Length::points(500.0),
        ..Style::default()
    }));
    let first_in_flow = tree.push(SimpleNode::new(Style {
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    }));
    let second_in_flow = tree.push(SimpleNode::new(Style {
        width: Length::points(5.0),
        height: Length::points(5.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);
    tree.append_child(root, first_in_flow);
    tree.append_child(root, second_in_flow);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 40.0);
    assert_close(size.height, 10.0);
    assert_close(tree.nodes[first_in_flow].layout.offset.x, 0.0);
    assert_close(tree.nodes[first_in_flow].layout.offset.y, 0.0);
    assert_close(tree.nodes[second_in_flow].layout.offset.x, 20.0);
    assert_close(tree.nodes[second_in_flow].layout.offset.y, 0.0);
    assert_close(tree.nodes[absolute].layout.size.width, 500.0);
    assert_close(tree.nodes[absolute].layout.size.height, 500.0);
}

#[test]
fn absolute_grid_item_without_grid_lines_uses_content_box_area() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(80.0),
        height: Length::points(50.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        left: Length::ZERO,
        right: Length::ZERO,
        top: Length::ZERO,
        bottom: Length::ZERO,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(80.0, 50.0));

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 80.0);
    assert_close(tree.nodes[child].layout.size.height, 50.0);
}

#[test]
fn absolute_grid_item_out_of_range_lines_fall_back_to_padding_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(80.0),
        height: Length::points(50.0),
        padding: Rect::new(
            Length::points(6.0),
            Length::points(4.0),
            Length::points(3.0),
            Length::points(5.0),
        ),
        border: Rect::new(2.0, 0.0, 1.0, 0.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(99),
        grid_column_end: Some(100),
        grid_row_start: Some(99),
        grid_row_end: Some(100),
        left: Length::ZERO,
        right: Length::ZERO,
        top: Length::ZERO,
        bottom: Length::ZERO,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(80.0, 50.0));

    assert_close(tree.nodes[child].layout.offset.x, 2.0);
    assert_close(tree.nodes[child].layout.offset.y, 1.0);
    assert_close(tree.nodes[child].layout.size.width, 90.0);
    assert_close(tree.nodes[child].layout.size.height, 58.0);
}

#[test]
fn fixed_grid_item_span_only_placement_uses_padding_edges_for_inset_containing_block() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(80.0),
        height: Length::points(50.0),
        padding: Rect::new(
            Length::points(6.0),
            Length::points(4.0),
            Length::points(3.0),
            Length::points(5.0),
        ),
        border: Rect::new(2.0, 0.0, 1.0, 0.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Fixed,
        grid_column_span: 2,
        grid_row_span: 2,
        left: Length::points(5.0),
        right: Length::points(7.0),
        top: Length::points(3.0),
        bottom: Length::points(4.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(3.0),
            Length::points(1.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(80.0, 50.0));

    assert_close(tree.nodes[child].layout.offset.x, 8.0);
    assert_close(tree.nodes[child].layout.offset.y, 7.0);
    assert_close(tree.nodes[child].layout.size.width, 75.0);
    assert_close(tree.nodes[child].layout.size.height, 47.0);
}

#[test]
fn absolute_grid_item_reversed_lines_create_zero_sized_area() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(40.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(3),
        grid_column_end: Some(2),
        grid_row_start: Some(2),
        grid_row_end: Some(1),
        left: Length::ZERO,
        right: Length::ZERO,
        top: Length::ZERO,
        bottom: Length::ZERO,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(40.0, 20.0));

    assert_close(tree.nodes[child].layout.offset.x, 40.0);
    assert_close(tree.nodes[child].layout.offset.y, 10.0);
    assert_close(tree.nodes[child].layout.size.width, 0.0);
    assert_close(tree.nodes[child].layout.size.height, 0.0);
}

#[test]
fn absolute_grid_item_auto_grid_lines_use_container_padding_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        left: Length::ZERO,
        right: Length::ZERO,
        top: Length::ZERO,
        bottom: Length::ZERO,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 100.0);
    assert_close(tree.nodes[child].layout.size.height, 20.0);
}

#[test]
fn absolute_grid_item_auto_lines_use_scrollable_overflow_padding_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(80.0),
        height: Length::points(30.0),
        padding: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_template_columns: vec![Length::points(60.0), Length::points(60.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        column_gap: Length::points(10.0),
        row_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        left: Length::ZERO,
        right: Length::ZERO,
        top: Length::ZERO,
        bottom: Length::ZERO,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(80.0, 30.0));

    assert_close(tree.nodes[root].layout.size.width, 88.0);
    assert_close(tree.nodes[root].layout.size.height, 36.0);
    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 138.0);
    assert_close(tree.nodes[child].layout.size.height, 51.0);
}

#[test]
fn rtl_absolute_grid_item_auto_lines_use_scrollable_overflow_padding_edges() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        width: Length::points(80.0),
        height: Length::points(30.0),
        padding: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_template_columns: vec![Length::points(60.0), Length::points(60.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        column_gap: Length::points(10.0),
        row_gap: Length::points(5.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        left: Length::ZERO,
        right: Length::ZERO,
        top: Length::ZERO,
        bottom: Length::ZERO,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(80.0, 30.0));

    assert_close(tree.nodes[root].layout.size.width, 88.0);
    assert_close(tree.nodes[root].layout.size.height, 36.0);
    assert_close(tree.nodes[child].layout.offset.x, -50.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 138.0);
    assert_close(tree.nodes[child].layout.size.height, 51.0);
}

#[test]
fn rtl_absolute_grid_item_auto_lines_use_inline_start_padding_for_fill_available_insets() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        direction: Direction::Rtl,
        width: Length::points(132.0),
        height: Length::points(88.0),
        padding: Rect::new(
            Length::points(4.0),
            Length::points(5.0),
            Length::points(6.0),
            Length::points(7.0),
        ),
        border: Rect::new(1.0, 2.0, 3.0, 4.0),
        grid_template_columns: vec![Length::points(24.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(18.0), Length::points(20.0)],
        column_gap: Length::points(5.0),
        row_gap: Length::points(3.0),
        justify_content: JustifyContent::Center,
        align_content: AlignContent::Center,
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        width: Length::Auto,
        height: Length::Auto,
        left: Length::percent(10.0),
        right: Length::calc(2.0, 15.0),
        top: Length::calc(1.0, 20.0),
        bottom: Length::percent(12.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(2.0),
            Length::points(1.0),
        ),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(1.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(132.0, 88.0));

    assert_close(tree.nodes[child].layout.offset.x, 16.0);
}

#[test]
fn absolute_grid_item_last_real_start_line_can_span_to_auto_end_edge() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        grid_row_end: Some(2),
        left: Length::ZERO,
        right: Length::ZERO,
        top: Length::ZERO,
        bottom: Length::ZERO,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[child].layout.offset.x, 60.0);
    assert_close(tree.nodes[child].layout.size.width, 40.0);
}

#[test]
fn absolute_grid_item_auto_end_line_uses_padding_edge_for_fit_content_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(120.0),
        height: Length::points(80.0),
        padding: Rect::all(Length::points(4.0)),
        grid_template_columns: vec![Length::points(30.0), Length::points(40.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(30.0)],
        column_gap: Length::points(5.0),
        row_gap: Length::points(3.0),
        justify_content: JustifyContent::Center,
        align_content: AlignContent::Center,
        ..Style::default()
    }));
    let absolute = tree.push(SimpleNode::new(Style {
        position: PositionType::Absolute,
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        width: Length::fit_content(Some(BaseLength::fixed(35.0))),
        height: Length::fit_content(Some(BaseLength::fixed(22.0))),
        justify_self: JustifyItems::Center,
        align_self: Some(AlignItems::Center),
        ..Style::default()
    }));
    let content = tree.push(SimpleNode::new(Style {
        width: Length::points(50.0),
        height: Length::points(20.0),
        ..Style::default()
    }));
    tree.append_child(root, absolute);
    tree.append_child(absolute, content);

    run_rust_layout(&mut tree, root, Constraints::definite(120.0, 80.0));

    assert_close(tree.nodes[root].layout.size.width, 128.0);
    assert_close(tree.nodes[root].layout.size.height, 88.0);
    assert_close(tree.nodes[absolute].layout.offset.x, 70.0);
    assert_close(tree.nodes[absolute].layout.offset.y, 54.0);
    assert_close(tree.nodes[absolute].layout.size.width, 50.0);
    assert_close(tree.nodes[absolute].layout.size.height, 20.0);
}

#[test]
fn grid_container_alignment_applies_to_auto_self_children() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_items: JustifyItems::End,
        align_items: AlignItems::FlexEnd,
        width: Length::points(100.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(100.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.x, 80.0);
    assert_close(tree.nodes[child].layout.offset.y, 90.0);
}

#[test]
fn grid_justify_items_auto_and_stretch_map_to_stretch() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_items: JustifyItems::Stretch,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let stretch_from_auto = tree.push(SimpleNode::new(Style {
        height: Length::points(10.0),
        justify_self: JustifyItems::Auto,
        ..Style::default()
    }));
    let explicit_stretch = tree.push(SimpleNode::new(Style {
        height: Length::points(10.0),
        justify_self: JustifyItems::Stretch,
        ..Style::default()
    }));
    tree.append_child(root, stretch_from_auto);
    tree.append_child(root, explicit_stretch);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[stretch_from_auto].layout.offset.x, 0.0);
    assert_close(tree.nodes[stretch_from_auto].layout.size.width, 50.0);
    assert_close(tree.nodes[explicit_stretch].layout.offset.x, 50.0);
    assert_close(tree.nodes[explicit_stretch].layout.size.width, 50.0);
}

#[test]
fn grid_justify_self_overrides_container_justify_items() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_items: JustifyItems::End,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(100.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(20.0),
        height: Length::points(20.0),
        justify_self: JustifyItems::Start,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
}

#[test]
fn grid_justify_content_offsets_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 30.0);
    assert_close(tree.nodes[second].layout.offset.x, 50.0);
}

#[test]
fn grid_justify_content_space_evenly_offsets_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::SpaceEvenly,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 20.0);
    assert_close(tree.nodes[second].layout.offset.x, 60.0);
}

#[test]
fn grid_justify_content_space_between_offsets_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::SpaceBetween,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 80.0);
}

#[test]
fn grid_justify_content_space_around_offsets_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::SpaceAround,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 15.0);
    assert_close(tree.nodes[second].layout.offset.x, 65.0);
}

#[test]
fn grid_justify_content_space_between_keeps_column_gap_when_tracks_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::SpaceBetween,
        width: Length::points(30.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(30.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 30.0);
}

#[test]
fn grid_justify_content_space_evenly_falls_back_to_start_when_tracks_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::SpaceEvenly,
        width: Length::points(30.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(30.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 30.0);
}

#[test]
fn grid_justify_content_space_around_falls_back_to_start_when_tracks_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::SpaceAround,
        width: Length::points(30.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(30.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 30.0);
}

#[test]
fn grid_auto_column_justify_center_offsets_intrinsic_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::Auto],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[child].layout.offset.x, 40.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
}

#[test]
fn definite_grid_auto_track_caps_intrinsic_growth_to_available_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::Auto, Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(120.0, 10.0),
    ));
    let marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    }));
    tree.append_child(root, child);
    tree.append_child(root, marker);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 120.0);
    assert_close(tree.nodes[marker].layout.offset.x, 100.0);
}

#[test]
fn definite_grid_auto_track_uses_definite_child_minimum_contribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(30.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::Auto, Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let definite = tree.push(SimpleNode::new(Style {
        width: Length::points(50.0),
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    }));
    let marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    }));
    tree.append_child(root, definite);
    tree.append_child(root, marker);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(30.0, 10.0));

    assert_close(size.width, 30.0);
    assert_close(tree.nodes[definite].layout.size.width, 50.0);
    assert_close(tree.nodes[marker].layout.offset.x, 30.0);
}

#[test]
fn definite_grid_fixed_min_intrinsic_max_updates_growth_limit_without_base_growth() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(20.0),
        height: Length::points(10.0),
        justify_content: JustifyContent::Start,
        grid_template_columns: vec![Length::points(10.0), Length::points(0.0)],
        grid_template_columns_max: vec![Length::MaxContent, Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let intrinsic_max = tree.push(SimpleNode::new(Style {
        width: Length::points(40.0),
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let marker = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic_max);
    tree.append_child(root, marker);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(20.0, 10.0));

    assert_close(size.width, 20.0);
    assert_close(tree.nodes[intrinsic_max].layout.size.width, 40.0);
    assert_close(tree.nodes[marker].layout.offset.x, 20.0);
}

#[test]
fn definite_grid_calc_percent_preferred_size_uses_minimum_contribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(10.0),
        justify_content: JustifyContent::Start,
        grid_template_columns: vec![Length::Auto, Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let calc_percent = tree.push(SimpleNode::new(Style {
        width: Length::calc(0.0, 10.0),
        min_width: Length::points(40.0),
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    }));
    let marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    }));
    tree.append_child(root, calc_percent);
    tree.append_child(root, marker);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[calc_percent].layout.size.width, 40.0);
    assert_close(tree.nodes[marker].layout.offset.x, 40.0);
}

#[test]
fn fixed_preferred_size_grid_item_uses_external_min_content_contribution() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Grid,
        width: Length::points(20.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::Auto, Length::points(0.0)],
        grid_template_columns_max: vec![Length::points(35.0), Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let fixed_preferred = tree.push(MeasuringNode::measured_with_intrinsic(
        Style {
            width: Length::points(80.0),
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(80.0, 10.0),
        Size::new(30.0, 10.0),
        Size::new(80.0, 10.0),
    ));
    let marker = tree.push(MeasuringNode::new(grid_start_item(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, fixed_preferred);
    tree.append_child(root, marker);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(20.0, 10.0));

    assert_close(size.width, 20.0);
    assert_close(tree.nodes[fixed_preferred].layout.size.width, 80.0);
    assert_close(tree.nodes[marker].layout.offset.x, 30.0);
    assert!(
        tree.nodes[fixed_preferred]
            .min_content_constraints
            .iter()
            .any(|constraints| constraints.width.mode == MeasureMode::Indefinite),
        "fixed preferred size should still use the W3C min-content contribution branch"
    );
}

#[test]
fn definite_grid_minmax_auto_fixed_track_uses_intrinsic_minimum() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(30.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::Auto, Length::points(0.0)],
        grid_template_columns_max: vec![Length::points(50.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let definite = tree.push(SimpleNode::new(Style {
        width: Length::points(45.0),
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    }));
    let marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    }));
    tree.append_child(root, definite);
    tree.append_child(root, marker);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(30.0, 10.0));

    assert_close(size.width, 30.0);
    assert_close(tree.nodes[definite].layout.size.width, 45.0);
    assert_close(tree.nodes[marker].layout.offset.x, 30.0);
}

#[test]
fn grid_root_at_most_does_not_cap_intrinsic_fixed_max_track_growth() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::Start,
        align_content: AlignContent::FlexStart,
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Start,
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_columns_max: vec![Length::points(70.0), Length::points(90.0), Length::Auto],
        grid_template_rows: vec![Length::Auto, Length::points(18.0)],
        grid_template_rows_max: vec![Length::Auto, Length::points(18.0)],
        column_gap: Length::points(7.0),
        row_gap: Length::points(5.0),
        padding: Rect::new(
            Length::points(4.0),
            Length::points(6.0),
            Length::points(3.0),
            Length::points(2.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        ..Style::default()
    }));
    let first_span = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(108.0, 46.0),
    ));
    let second_span = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(2),
            grid_column_span: 2,
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(96.0, 54.0),
    ));
    let full_span = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(1),
            grid_column_span: 3,
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(176.0, 72.0),
    ));
    let definite = tree.push(SimpleNode::new(grid_start_item(Style {
        width: Length::points(64.0),
        height: Length::points(28.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, first_span);
    tree.append_child(root, second_span);
    tree.append_child(root, full_span);
    tree.append_child(root, definite);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::at_most(190.0),
            SideConstraint::at_most(110.0),
        ),
    );

    assert_close(size.width, 259.0);
    assert_close(size.height, 103.0);
    assert_close(tree.nodes[second_span].layout.offset.x, 82.0);
    assert_close(tree.nodes[definite].layout.offset.x, 182.0);
}

#[test]
fn definite_grid_auto_row_caps_intrinsic_growth_to_available_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        align_content: AlignContent::Center,
        width: Length::points(10.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::Auto, Length::points(0.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(10.0, 120.0),
    ));
    let marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..Style::default()
    }));
    tree.append_child(root, child);
    tree.append_child(root, marker);

    run_rust_layout(&mut tree, root, Constraints::definite(10.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.size.height, 120.0);
    assert_close(tree.nodes[marker].layout.offset.y, 100.0);
}

#[test]
fn grid_intrinsic_growth_processes_shorter_spans_before_longer_spans() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let wide_span = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_column_span: 3,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(90.0, 10.0),
    ));
    let single_span = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(50.0, 10.0),
    ));
    tree.append_child(root, wide_span);
    tree.append_child(root, single_span);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::indefinite(), SideConstraint::indefinite()),
    );

    assert_close(size.width, 90.0);
    assert_close(size.height, 10.0);
    assert_close(tree.nodes[wide_span].layout.size.width, 90.0);
    assert_close(tree.nodes[single_span].layout.size.width, 50.0);
}

#[test]
fn grid_intrinsic_growth_batches_equal_span_planned_increases() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let first_span = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(100.0, 10.0),
    ));
    let second_span = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(2),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(100.0, 10.0),
    ));
    let marker = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(3),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(0.0, 0.0),
    ));
    tree.append_child(root, first_span);
    tree.append_child(root, second_span);
    tree.append_child(root, marker);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::indefinite(), SideConstraint::indefinite()),
    );

    assert_close(size.width, 150.0);
    assert_close(tree.nodes[marker].layout.offset.x, 150.0);
    assert_close(tree.nodes[first_span].layout.size.width, 100.0);
    assert_close(tree.nodes[second_span].layout.size.width, 100.0);
}

#[test]
fn grid_intrinsic_growth_planned_increases_are_source_order_independent() {
    fn layout_width(first_then_second: bool) -> f32 {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Grid,
            grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
            grid_template_rows: vec![Length::points(10.0)],
            ..Style::default()
        }));
        let first_span = tree.push(SimpleNode::with_measured_size(
            Style {
                grid_column_start: Some(1),
                grid_column_span: 2,
                grid_row_start: Some(1),
                justify_self: JustifyItems::Start,
                align_self: Some(AlignItems::FlexStart),
                ..Style::default()
            },
            Size::new(80.0, 10.0),
        ));
        let second_span = tree.push(SimpleNode::with_measured_size(
            Style {
                grid_column_start: Some(2),
                grid_column_span: 2,
                grid_row_start: Some(1),
                justify_self: JustifyItems::Start,
                align_self: Some(AlignItems::FlexStart),
                ..Style::default()
            },
            Size::new(100.0, 10.0),
        ));
        if first_then_second {
            tree.append_child(root, first_span);
            tree.append_child(root, second_span);
        } else {
            tree.append_child(root, second_span);
            tree.append_child(root, first_span);
        }

        run_rust_layout(&mut tree, root, Constraints::indefinite()).width
    }

    assert_close(layout_width(true), 140.0);
    assert_close(layout_width(false), 140.0);
}

#[test]
fn grid_row_intrinsic_growth_planned_increases_are_source_order_independent() {
    fn layout_height(first_then_second: bool) -> f32 {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Grid,
            grid_template_columns: vec![Length::points(10.0)],
            grid_template_rows: vec![Length::Auto, Length::Auto, Length::Auto],
            ..Style::default()
        }));
        let first_span = tree.push(SimpleNode::with_measured_size(
            Style {
                grid_column_start: Some(1),
                grid_row_start: Some(1),
                grid_row_span: 2,
                justify_self: JustifyItems::Start,
                align_self: Some(AlignItems::FlexStart),
                ..Style::default()
            },
            Size::new(10.0, 80.0),
        ));
        let second_span = tree.push(SimpleNode::with_measured_size(
            Style {
                grid_column_start: Some(1),
                grid_row_start: Some(2),
                grid_row_span: 2,
                justify_self: JustifyItems::Start,
                align_self: Some(AlignItems::FlexStart),
                ..Style::default()
            },
            Size::new(10.0, 100.0),
        ));
        if first_then_second {
            tree.append_child(root, first_span);
            tree.append_child(root, second_span);
        } else {
            tree.append_child(root, second_span);
            tree.append_child(root, first_span);
        }

        run_rust_layout(&mut tree, root, Constraints::indefinite()).height
    }

    assert_close(layout_height(true), 140.0);
    assert_close(layout_height(false), 140.0);
}

#[test]
fn grid_spanning_auto_minimum_redistributes_after_fixed_growth_limit() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::Auto, Length::Auto],
        grid_template_columns_max: vec![Length::points(40.0), Length::Auto],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let spanning = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            min_width: Length::points(100.0),
            ..Style::default()
        },
        Size::new(100.0, 10.0),
    ));
    let capped_marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        width: Length::percent(100.0),
        ..Style::default()
    }));
    let uncapped_marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        width: Length::percent(100.0),
        ..Style::default()
    }));
    tree.append_child(root, spanning);
    tree.append_child(root, capped_marker);
    tree.append_child(root, uncapped_marker);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[spanning].layout.size.width, 100.0);
    assert_close(tree.nodes[capped_marker].layout.size.width, 40.0);
    assert_close(tree.nodes[uncapped_marker].layout.offset.x, 40.0);
    assert_close(tree.nodes[uncapped_marker].layout.size.width, 60.0);
}

#[test]
fn grid_spanning_minimum_uses_non_affected_track_before_exceeding_limits() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(40.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::Auto, Length::points(0.0), Length::Auto],
        grid_template_columns_max: vec![
            Length::points(20.0),
            Length::points(50.0),
            Length::points(20.0),
        ],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let spanning = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_column_span: 3,
            grid_row_start: Some(1),
            min_width: Length::points(70.0),
            ..Style::default()
        },
        Size::new(70.0, 10.0),
    ));
    let middle_marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        width: Length::percent(100.0),
        ..Style::default()
    }));
    let trailing_marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(2),
        width: Length::percent(100.0),
        ..Style::default()
    }));
    tree.append_child(root, spanning);
    tree.append_child(root, middle_marker);
    tree.append_child(root, trailing_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(40.0, 20.0));

    assert_close(size.width, 40.0);
    assert_close(tree.nodes[spanning].layout.size.width, 70.0);
    assert_close(tree.nodes[middle_marker].layout.offset.x, 20.0);
    assert_close(tree.nodes[middle_marker].layout.size.width, 30.0);
    assert_close(tree.nodes[trailing_marker].layout.offset.x, 50.0);
    assert_close(tree.nodes[trailing_marker].layout.size.width, 20.0);
}

#[test]
fn grid_spanning_minimum_continues_beyond_fixed_growth_limits() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(40.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::Auto, Length::Auto],
        grid_template_columns_max: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let spanning = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            min_width: Length::points(70.0),
            ..Style::default()
        },
        Size::new(70.0, 10.0),
    ));
    let first_marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        width: Length::percent(100.0),
        ..Style::default()
    }));
    let second_marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        width: Length::percent(100.0),
        ..Style::default()
    }));
    tree.append_child(root, spanning);
    tree.append_child(root, first_marker);
    tree.append_child(root, second_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(40.0, 20.0));

    assert_close(size.width, 40.0);
    assert_close(tree.nodes[spanning].layout.size.width, 70.0);
    assert_close(tree.nodes[first_marker].layout.size.width, 35.0);
    assert_close(tree.nodes[second_marker].layout.offset.x, 35.0);
    assert_close(tree.nodes[second_marker].layout.size.width, 35.0);
}

#[test]
fn grid_spanning_growth_limit_uses_newly_finite_limit_as_infinitely_growable() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::Auto, Length::Auto],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        justify_items: JustifyItems::Stretch,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = tree.push(MeasuringNode::measured_with_intrinsic(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(10.0, 10.0),
        Size::new(10.0, 10.0),
        Size::new(10.0, 10.0),
    ));
    let spanning = tree.push(MeasuringNode::measured_with_intrinsic(
        Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(2),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(100.0, 10.0),
        Size::new(30.0, 10.0),
        Size::new(100.0, 10.0),
    ));
    tree.append_child(root, first);
    tree.append_child(root, spanning);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(size.width, 100.0);
    assert_close(tree.nodes[first].layout.size.width, 10.0);
    assert_close(tree.nodes[spanning].layout.size.width, 100.0);
}

#[test]
fn indefinite_grid_justify_content_uses_container_min_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::Center,
        min_width: Length::points(100.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = grid_child(&mut tree);
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 100.0);
    assert_close(tree.nodes[child].layout.offset.x, 40.0);
    assert_close(tree.nodes[child].layout.size.width, 20.0);
}

#[test]
fn indefinite_grid_stretch_auto_track_uses_container_min_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        min_width: Length::points(100.0),
        grid_template_columns: vec![Length::Auto],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = grid_child(&mut tree);
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 100.0);
    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 100.0);
}

#[test]
fn definite_grid_stretch_distributes_free_space_to_auto_max_tracks_only() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(60.0),
        justify_content: JustifyContent::Stretch,
        align_content: AlignContent::Stretch,
        grid_template_columns: vec![
            Length::points(20.0),
            Length::points(20.0),
            Length::points(20.0),
        ],
        grid_template_columns_max: vec![Length::Auto, Length::points(20.0), Length::Auto],
        grid_template_rows: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_rows_max: vec![Length::Auto, Length::points(10.0), Length::Auto],
        ..Style::default()
    }));
    let first_auto = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    let second_auto = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(3),
        ..Style::default()
    }));
    tree.append_child(root, first_auto);
    tree.append_child(root, fixed);
    tree.append_child(root, second_auto);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(100.0, 60.0));

    assert_close(size.width, 100.0);
    assert_close(size.height, 60.0);
    assert_close(tree.nodes[first_auto].layout.offset.x, 0.0);
    assert_close(tree.nodes[first_auto].layout.offset.y, 0.0);
    assert_close(tree.nodes[first_auto].layout.size.width, 40.0);
    assert_close(tree.nodes[first_auto].layout.size.height, 25.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 40.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 25.0);
    assert_close(tree.nodes[fixed].layout.size.width, 20.0);
    assert_close(tree.nodes[fixed].layout.size.height, 10.0);
    assert_close(tree.nodes[second_auto].layout.offset.x, 60.0);
    assert_close(tree.nodes[second_auto].layout.offset.y, 35.0);
    assert_close(tree.nodes[second_auto].layout.size.width, 40.0);
    assert_close(tree.nodes[second_auto].layout.size.height, 25.0);
}

#[test]
fn indefinite_grid_percentage_column_gap_resolves_after_container_min_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        min_width: Length::points(100.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::percent(10.0),
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 100.0);
    assert_close(tree.nodes[first].layout.size.width, 20.0);
    assert_close(tree.nodes[second].layout.offset.x, 30.0);
    assert_close(tree.nodes[second].layout.size.width, 20.0);
}

#[test]
fn indefinite_grid_percentage_row_gap_resolves_after_container_min_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        min_height: Length::points(100.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        row_gap: Length::percent(10.0),
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.height, 100.0);
    assert_close(tree.nodes[first].layout.size.height, 20.0);
    assert_close(tree.nodes[second].layout.offset.y, 30.0);
    assert_close(tree.nodes[second].layout.size.height, 20.0);
}

#[test]
fn grid_align_content_distributes_extra_row_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        align_content: AlignContent::SpaceBetween,
        width: Length::points(20.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 100.0));

    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 80.0);
}

#[test]
fn grid_align_content_start_end_alias_flex_edges() {
    for (align_content, first_y, second_y) in [
        (AlignContent::Start, 0.0, 20.0),
        (AlignContent::End, 60.0, 80.0),
    ] {
        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style {
            display: Display::Grid,
            align_content,
            width: Length::points(20.0),
            height: Length::points(100.0),
            grid_template_columns: vec![Length::points(20.0)],
            grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
            ..Style::default()
        }));
        let first = grid_child(&mut tree);
        let second = grid_child(&mut tree);
        tree.append_child(root, first);
        tree.append_child(root, second);

        run_rust_layout(&mut tree, root, Constraints::definite(20.0, 100.0));

        assert_close(tree.nodes[first].layout.offset.y, first_y);
        assert_close(tree.nodes[second].layout.offset.y, second_y);
    }
}

#[test]
fn grid_align_content_space_around_offsets_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        align_content: AlignContent::SpaceAround,
        width: Length::points(20.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 100.0));

    assert_close(tree.nodes[first].layout.offset.y, 15.0);
    assert_close(tree.nodes[second].layout.offset.y, 65.0);
}

#[test]
fn grid_align_content_space_evenly_offsets_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        align_content: AlignContent::SpaceEvenly,
        width: Length::points(20.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 100.0));

    assert_close(tree.nodes[first].layout.offset.y, 20.0);
    assert_close(tree.nodes[second].layout.offset.y, 60.0);
}

#[test]
fn grid_align_content_space_between_keeps_row_gap_when_tracks_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        align_content: AlignContent::SpaceBetween,
        width: Length::points(20.0),
        height: Length::points(30.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        row_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 30.0));

    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 30.0);
}

#[test]
fn grid_align_content_space_evenly_falls_back_to_start_when_tracks_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        align_content: AlignContent::SpaceEvenly,
        width: Length::points(20.0),
        height: Length::points(30.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        row_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 30.0));

    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 30.0);
}

#[test]
fn grid_align_content_space_around_falls_back_to_start_when_tracks_overflow() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        align_content: AlignContent::SpaceAround,
        width: Length::points(20.0),
        height: Length::points(30.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(20.0)],
        row_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 30.0));

    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.offset.y, 30.0);
}

#[test]
fn grid_auto_row_align_center_offsets_intrinsic_track_group() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        align_content: AlignContent::Center,
        width: Length::points(20.0),
        height: Length::points(100.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::Auto],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(20.0, 100.0));

    assert_close(tree.nodes[child].layout.offset.y, 45.0);
    assert_close(tree.nodes[child].layout.size.height, 10.0);
}

#[test]
fn grid_span_uses_combined_tracks_and_gap() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(120.0),
        height: Length::points(40.0),
        grid_template_columns: vec![Length::percent(50.0), Length::Auto],
        grid_template_rows: vec![Length::points(40.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        grid_column_span: 2,
        ..Style::default()
    }));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(120.0, 40.0));

    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
    assert_close(tree.nodes[child].layout.size.width, 120.0);
    assert_close(tree.nodes[child].layout.size.height, 40.0);
}

#[test]
fn auto_grid_rows_grow_from_measured_children_when_container_height_is_indefinite() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        grid_template_columns: vec![Length::points(50.0), Length::points(50.0)],
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(30.0, 12.0),
    ));
    let second = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(20.0, 25.0),
    ));
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

    assert_close(size.width, 100.0);
    assert_close(size.height, 25.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.offset.y, 0.0);
    assert_close(tree.nodes[first].layout.size.width, 50.0);
    assert_close(tree.nodes[first].layout.size.height, 25.0);
    assert_close(tree.nodes[second].layout.offset.x, 50.0);
    assert_close(tree.nodes[second].layout.offset.y, 0.0);
    assert_close(tree.nodes[second].layout.size.width, 50.0);
    assert_close(tree.nodes[second].layout.size.height, 25.0);
}

#[test]
fn auto_grid_row_grows_from_child_aspect_ratio() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(80.0),
        grid_template_columns: vec![Length::points(80.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(80.0),
        aspect_ratio: Some(2.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    let size = run_rust_layout(
        &mut tree,
        root,
        Constraints::new(SideConstraint::definite(80.0), SideConstraint::indefinite()),
    );

    assert_close(size.width, 80.0);
    assert_close(size.height, 40.0);
    assert_close(tree.nodes[child].layout.size.width, 80.0);
    assert_close(tree.nodes[child].layout.size.height, 40.0);
    assert_close(tree.nodes[child].layout.offset.x, 0.0);
    assert_close(tree.nodes[child].layout.offset.y, 0.0);
}

#[test]
fn grid_auto_rows_use_column_sized_measured_block_contribution() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::Auto],
        grid_template_rows: vec![Length::Auto],
        ..Style::default()
    }));
    let child = tree.push(MeasuringNode::height_from_width(
        Style::default(),
        80.0,
        10.0,
        0.5,
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 80.0);
    assert_close(size.height, 40.0);
    assert_close(tree.nodes[child].layout.size.width, 80.0);
    assert_close(tree.nodes[child].layout.size.height, 40.0);
}

#[test]
fn grid_columns_re_resolve_when_row_sizing_changes_inline_min_content_contribution() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::MinContent, Length::points(0.0)],
        grid_template_rows: vec![Length::points(40.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::Stretch,
        ..Style::default()
    }));
    let intrinsic = tree.push(MeasuringNode::measured_with_height_dependent_intrinsic(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
        20.0,
        2.0,
        10.0,
    ));
    let trailing_marker = tree.push(MeasuringNode::new(grid_start_item(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, trailing_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 80.0);
    assert_close(size.height, 40.0);
    assert_close(tree.nodes[trailing_marker].layout.offset.x, 80.0);
    assert!(
        tree.nodes[intrinsic]
            .min_content_constraints
            .iter()
            .any(
                |constraints| constraints.width.mode == MeasureMode::Indefinite
                    && constraints.height.mode == MeasureMode::Definite
                    && (constraints.height.size - 40.0).abs() < 0.01
            ),
        "inline min-content contribution should be requeried with the resolved row height"
    );
}

#[test]
fn grid_columns_re_resolve_with_updated_inline_max_content_contribution() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::MaxContent, Length::points(0.0)],
        grid_template_rows: vec![Length::points(40.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::Stretch,
        ..Style::default()
    }));
    let intrinsic = tree.push(MeasuringNode::measured_with_height_dependent_intrinsic(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
        20.0,
        3.0,
        10.0,
    ));
    let trailing_marker = tree.push(MeasuringNode::new(grid_start_item(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, trailing_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 120.0);
    assert_close(size.height, 40.0);
    assert_close(tree.nodes[trailing_marker].layout.offset.x, 120.0);
    assert!(
        tree.nodes[intrinsic]
            .max_content_constraints
            .iter()
            .any(
                |constraints| constraints.width.mode == MeasureMode::Indefinite
                    && constraints.height.mode == MeasureMode::Definite
                    && (constraints.height.size - 40.0).abs() < 0.01
            ),
        "inline max-content contribution should be requeried with the resolved row height"
    );
}

#[test]
fn grid_rows_re_resolve_after_column_re_resolution_changes_block_min_content_contribution() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::MinContent, Length::points(0.0)],
        grid_template_rows: vec![Length::Auto],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::Stretch,
        ..Style::default()
    }));
    let intrinsic = tree.push(MeasuringNode::measured_with_cross_axis_intrinsic(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            ..Style::default()
        },
        Size::new(20.0, 10.0),
        20.0,
        8.0,
        10.0,
        0.5,
    ));
    let trailing_marker = tree.push(MeasuringNode::new(grid_start_item(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, trailing_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 80.0);
    assert_close(size.height, 40.0);
    assert_close(tree.nodes[trailing_marker].layout.offset.x, 80.0);
    assert!(
        tree.nodes[intrinsic]
            .min_content_constraints
            .iter()
            .any(|constraints| constraints
                .width
                .bounded_size()
                .is_some_and(|width| { (width - 80.0).abs() < 0.01 })
                && constraints.height.mode == MeasureMode::Indefinite),
        "block min-content contribution should be requeried after the column re-resolution"
    );
}

#[test]
fn grid_fr_tracks_share_remaining_content_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(110.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::fr(1.0), Length::fr(2.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(110.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.size.width, 33.0);
    assert_close(tree.nodes[second].layout.offset.x, 43.0);
    assert_close(tree.nodes[second].layout.size.width, 67.0);
}

#[test]
fn grid_fr_tracks_reserve_fixed_tracks_and_gaps() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(120.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::fr(1.0), Length::fr(2.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let fixed = grid_child(&mut tree);
    let one_fr = grid_child(&mut tree);
    let two_fr = grid_child(&mut tree);
    tree.append_child(root, fixed);
    tree.append_child(root, one_fr);
    tree.append_child(root, two_fr);

    run_rust_layout(&mut tree, root, Constraints::definite(120.0, 20.0));

    assert_close(tree.nodes[fixed].layout.offset.x, 0.0);
    assert_close(tree.nodes[fixed].layout.size.width, 20.0);
    assert_close(tree.nodes[one_fr].layout.offset.x, 30.0);
    assert_close(tree.nodes[one_fr].layout.size.width, 27.0);
    assert_close(tree.nodes[two_fr].layout.offset.x, 67.0);
    assert_close(tree.nodes[two_fr].layout.size.width, 53.0);
}

#[test]
fn grid_fr_tracks_do_not_expand_when_available_space_is_exhausted() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(10.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::fr(1.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(20.0),
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(10.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.size.width, 0.0);
    assert_close(tree.nodes[second].layout.offset.x, 20.0);
    assert_close(tree.nodes[second].layout.size.width, 0.0);
}

#[test]
fn grid_fr_tracks_flex_factor_sum_below_one_leaves_remaining_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::fr(0.25), Length::fr(0.25)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.size.width, 25.0);
    assert_close(tree.nodes[second].layout.offset.x, 25.0);
    assert_close(tree.nodes[second].layout.size.width, 25.0);
}

#[test]
fn grid_fr_tracks_freeze_large_base_sizes_when_finding_fr_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(80.0), Length::fr(1.0)],
        grid_template_columns_max: vec![Length::fr(1.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let wide_base = grid_child(&mut tree);
    let flexible = grid_child(&mut tree);
    tree.append_child(root, wide_base);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 20.0));

    assert_close(tree.nodes[wide_base].layout.offset.x, 0.0);
    assert_close(tree.nodes[wide_base].layout.size.width, 80.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 80.0);
    assert_close(tree.nodes[flexible].layout.size.width, 20.0);
}

#[test]
fn grid_fr_size_restarts_after_each_large_base_freeze() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(180.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(80.0), Length::points(70.0), Length::Auto],
        grid_template_columns_max: vec![Length::fr(1.0), Length::fr(1.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(20.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    let third = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);

    run_rust_layout(&mut tree, root, Constraints::definite(180.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.size.width, 80.0);
    assert_close(tree.nodes[second].layout.offset.x, 80.0);
    assert_close(tree.nodes[second].layout.size.width, 70.0);
    assert_close(tree.nodes[third].layout.offset.x, 150.0);
    assert_close(tree.nodes[third].layout.size.width, 30.0);
}

#[test]
fn grid_fr_size_uses_spanning_intrinsic_growth_as_base_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(180.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_columns_max: vec![Length::fr(1.0), Length::fr(1.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        ..Style::default()
    }));
    let spanning = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_column_end: Some(3),
            grid_row_start: Some(1),
            min_width: Length::points(160.0),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(160.0, 10.0),
    ));
    let first = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    let third = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, spanning);
    tree.append_child(root, first);
    tree.append_child(root, second);
    tree.append_child(root, third);

    run_rust_layout(&mut tree, root, Constraints::definite(180.0, 20.0));

    assert_close(tree.nodes[spanning].layout.size.width, 160.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.size.width, 80.0);
    assert_close(tree.nodes[second].layout.offset.x, 80.0);
    assert_close(tree.nodes[second].layout.size.width, 80.0);
    assert_close(tree.nodes[third].layout.offset.x, 160.0);
    assert_close(tree.nodes[third].layout.size.width, 20.0);
}

#[test]
fn indefinite_grid_fr_tracks_expand_from_existing_flex_base() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::fr(2.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(100.0, 10.0),
    ));
    let sibling_fr = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, sibling_fr);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 150.0);
    assert_close(tree.nodes[intrinsic].layout.size.width, 100.0);
    assert_close(tree.nodes[sibling_fr].layout.offset.x, 100.0);
    assert_close(tree.nodes[sibling_fr].layout.size.width, 50.0);
}

#[test]
fn min_content_grid_width_uses_zero_flex_fraction_for_fr_tracks() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::MinContent,
        grid_template_columns: vec![Length::fr(1.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            min_width: Length::points(30.0),
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(80.0, 10.0),
    ));
    let second_track_marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, second_track_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 30.0);
    assert_close(tree.nodes[intrinsic].layout.size.width, 80.0);
    assert_close(tree.nodes[second_track_marker].layout.offset.x, 30.0);
    assert_close(tree.nodes[second_track_marker].layout.size.width, 0.0);
}

#[test]
fn max_content_grid_width_uses_item_contribution_to_expand_fr_tracks() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::MaxContent,
        grid_template_columns: vec![Length::fr(1.0), Length::fr(1.0), Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            min_width: Length::points(30.0),
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(80.0, 10.0),
    ));
    let trailing_marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, trailing_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 160.0);
    assert_close(tree.nodes[intrinsic].layout.size.width, 80.0);
    assert_close(tree.nodes[trailing_marker].layout.offset.x, 160.0);
}

#[test]
fn min_content_grid_height_uses_zero_flex_fraction_for_fr_tracks() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(10.0),
        height: Length::MinContent,
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::fr(1.0), Length::fr(1.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            min_height: Length::points(20.0),
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(10.0, 60.0),
    ));
    let second_track_marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, second_track_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.height, 20.0);
    assert_close(tree.nodes[intrinsic].layout.size.height, 60.0);
    assert_close(tree.nodes[second_track_marker].layout.offset.y, 20.0);
    assert_close(tree.nodes[second_track_marker].layout.size.height, 0.0);
}

#[test]
fn indefinite_grid_without_flexible_tracks_skips_fr_expansion() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 50.0);
    assert_close(tree.nodes[first].layout.size.width, 20.0);
    assert_close(tree.nodes[second].layout.offset.x, 20.0);
    assert_close(tree.nodes[second].layout.size.width, 30.0);
}

#[test]
fn indefinite_grid_fixed_track_items_do_not_seed_fr_fraction() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(40.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let fixed_track_item = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(30.0, 10.0),
    ));
    let flexible_track_marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, fixed_track_item);
    tree.append_child(root, flexible_track_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 40.0);
    assert_close(tree.nodes[fixed_track_item].layout.size.width, 30.0);
    assert_close(tree.nodes[flexible_track_marker].layout.offset.x, 40.0);
    assert_close(tree.nodes[flexible_track_marker].layout.size.width, 0.0);
}

#[test]
fn indefinite_grid_max_width_below_fixed_tracks_suppresses_fr_expansion() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        max_width: Length::points(30.0),
        grid_template_columns: vec![Length::points(40.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let fixed_track_item = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(30.0, 10.0),
    ));
    let flexible_track_marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, fixed_track_item);
    tree.append_child(root, flexible_track_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 30.0);
    assert_close(tree.nodes[fixed_track_item].layout.size.width, 30.0);
    assert_close(tree.nodes[flexible_track_marker].layout.offset.x, 40.0);
    assert_close(tree.nodes[flexible_track_marker].layout.size.width, 0.0);
}

#[test]
fn indefinite_grid_fr_columns_expand_to_container_min_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        min_width: Length::points(120.0),
        grid_template_columns: vec![Length::fr(1.0), Length::fr(2.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let one_fr = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let two_fr = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, one_fr);
    tree.append_child(root, two_fr);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 120.0);
    assert_close(tree.nodes[one_fr].layout.size.width, 40.0);
    assert_close(tree.nodes[two_fr].layout.offset.x, 40.0);
    assert_close(tree.nodes[two_fr].layout.size.width, 80.0);
}

#[test]
fn indefinite_grid_fr_columns_redo_flex_fraction_with_container_max_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        max_width: Length::points(100.0),
        grid_template_columns: vec![Length::fr(1.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let spanning = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(200.0, 10.0),
    ));
    let first_marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let second_marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, spanning);
    tree.append_child(root, first_marker);
    tree.append_child(root, second_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 100.0);
    assert_close(tree.nodes[first_marker].layout.size.width, 50.0);
    assert_close(tree.nodes[second_marker].layout.offset.x, 50.0);
    assert_close(tree.nodes[second_marker].layout.size.width, 50.0);
}

#[test]
fn indefinite_grid_fr_rows_expand_to_container_min_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        min_height: Length::points(90.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::fr(1.0), Length::fr(2.0)],
        ..Style::default()
    }));
    let one_fr = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let two_fr = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, one_fr);
    tree.append_child(root, two_fr);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.height, 90.0);
    assert_close(tree.nodes[one_fr].layout.size.height, 30.0);
    assert_close(tree.nodes[two_fr].layout.offset.y, 30.0);
    assert_close(tree.nodes[two_fr].layout.size.height, 60.0);
}

#[test]
fn indefinite_grid_spanning_fr_item_distributes_growth_by_flex_factor() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::fr(1.0), Length::fr(2.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        ..Style::default()
    }));
    let spanning = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_column_end: Some(3),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(90.0, 10.0),
    ));
    let one_fr = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    let two_fr = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, spanning);
    tree.append_child(root, one_fr);
    tree.append_child(root, two_fr);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 90.0);
    assert_close(tree.nodes[spanning].layout.size.width, 90.0);
    assert_close(tree.nodes[one_fr].layout.offset.x, 0.0);
    assert_close(tree.nodes[one_fr].layout.size.width, 30.0);
    assert_close(tree.nodes[two_fr].layout.offset.x, 30.0);
    assert_close(tree.nodes[two_fr].layout.size.width, 60.0);
}

#[test]
fn indefinite_grid_spanning_fr_item_with_flex_sum_below_one_distributes_remainder_equally() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::fr(0.1), Length::fr(0.3)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        ..Style::default()
    }));
    let spanning = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_column_end: Some(3),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(100.0, 10.0),
    ));
    let one_fr = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    let three_fr = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, spanning);
    tree.append_child(root, one_fr);
    tree.append_child(root, three_fr);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 100.0);
    assert_close(tree.nodes[spanning].layout.size.width, 100.0);
    assert_close(tree.nodes[one_fr].layout.offset.x, 0.0);
    assert_close(tree.nodes[one_fr].layout.size.width, 40.0);
    assert_close(tree.nodes[three_fr].layout.offset.x, 40.0);
    assert_close(tree.nodes[three_fr].layout.size.width, 60.0);
}

#[test]
fn indefinite_grid_spanning_zero_fr_item_distributes_remainder_equally() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::fr(0.0), Length::fr(0.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        ..Style::default()
    }));
    let spanning = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_column_end: Some(3),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(100.0, 10.0),
    ));
    let first = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, spanning);
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 100.0);
    assert_close(tree.nodes[spanning].layout.size.width, 100.0);
    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.size.width, 50.0);
    assert_close(tree.nodes[second].layout.offset.x, 50.0);
    assert_close(tree.nodes[second].layout.size.width, 50.0);
}

#[test]
fn calc_grid_tracks_resolve_against_definite_content_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(200.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::calc(10.0, 25.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let calc = grid_child(&mut tree);
    let flexible = grid_child(&mut tree);
    tree.append_child(root, calc);
    tree.append_child(root, flexible);

    run_rust_layout(&mut tree, root, Constraints::definite(200.0, 20.0));

    assert_close(tree.nodes[calc].layout.offset.x, 0.0);
    assert_close(tree.nodes[calc].layout.size.width, 60.0);
    assert_close(tree.nodes[flexible].layout.offset.x, 70.0);
    assert_close(tree.nodes[flexible].layout.size.width, 130.0);
}

#[test]
fn grid_minmax_tracks_use_fr_size_from_available_track_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(140.0),
        height: Length::points(20.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_columns_max: vec![Length::fr(1.0), Length::fr(2.0)],
        grid_template_rows: vec![Length::points(20.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(140.0, 20.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.size.width, 43.0);
    assert_close(tree.nodes[second].layout.offset.x, 53.0);
    assert_close(tree.nodes[second].layout.size.width, 87.0);
}

#[test]
fn grid_minmax_fixed_max_tracks_share_definite_free_space_up_to_limits() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_columns_max: vec![Length::points(50.0), Length::points(60.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 50.0);
    assert_close(tree.nodes[second].layout.offset.x, 50.0);
    assert_close(tree.nodes[second].layout.size.width, 50.0);
}

#[test]
fn grid_maximize_tracks_resolves_percent_and_calc_growth_limits_before_redistribution() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(150.0),
        height: Length::points(10.0),
        grid_template_columns: vec![
            Length::points(20.0),
            Length::points(20.0),
            Length::points(20.0),
        ],
        grid_template_columns_max: vec![
            Length::calc(10.0, 20.0),
            Length::percent(50.0),
            Length::points(100.0),
        ],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let calc_capped = grid_child(&mut tree);
    let percent_capped = grid_child(&mut tree);
    let fixed_capped = grid_child(&mut tree);
    tree.append_child(root, calc_capped);
    tree.append_child(root, percent_capped);
    tree.append_child(root, fixed_capped);

    run_rust_layout(&mut tree, root, Constraints::definite(150.0, 10.0));

    assert_close(tree.nodes[calc_capped].layout.size.width, 40.0);
    assert_close(tree.nodes[percent_capped].layout.offset.x, 40.0);
    assert_close(tree.nodes[percent_capped].layout.size.width, 55.0);
    assert_close(tree.nodes[fixed_capped].layout.offset.x, 95.0);
    assert_close(tree.nodes[fixed_capped].layout.size.width, 55.0);
}

#[test]
fn grid_maximize_tracks_does_not_grow_indefinite_growth_limits() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(10.0),
        justify_content: JustifyContent::Start,
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_columns_max: vec![Length::Auto, Length::Auto],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        width: Length::percent(100.0),
        ..Style::default()
    }));
    let second = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        width: Length::percent(100.0),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.offset.x, 0.0);
    assert_close(tree.nodes[first].layout.size.width, 20.0);
    assert_close(tree.nodes[second].layout.offset.x, 20.0);
    assert_close(tree.nodes[second].layout.size.width, 20.0);
}

#[test]
fn grid_maximize_tracks_subtracts_gaps_from_definite_free_space() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_columns_max: vec![Length::points(100.0), Length::points(100.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(10.0),
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[first].layout.size.width, 45.0);
    assert_close(tree.nodes[second].layout.offset.x, 55.0);
    assert_close(tree.nodes[second].layout.size.width, 45.0);
}

#[test]
fn grid_maximize_tracks_redistributes_after_fixed_growth_limit_freezes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_columns_max: vec![Length::points(30.0), Length::points(100.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let capped = grid_child(&mut tree);
    let uncapped = grid_child(&mut tree);
    tree.append_child(root, capped);
    tree.append_child(root, uncapped);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[capped].layout.size.width, 30.0);
    assert_close(tree.nodes[uncapped].layout.offset.x, 30.0);
    assert_close(tree.nodes[uncapped].layout.size.width, 70.0);
}

#[test]
fn indefinite_grid_minmax_fixed_max_tracks_grow_to_limits() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(20.0), Length::points(30.0)],
        grid_template_columns_max: vec![Length::points(50.0), Length::points(40.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 90.0);
    assert_close(tree.nodes[first].layout.size.width, 50.0);
    assert_close(tree.nodes[second].layout.offset.x, 50.0);
    assert_close(tree.nodes[second].layout.size.width, 40.0);
}

#[test]
fn indefinite_grid_minmax_fixed_max_tracks_respect_container_max_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        max_width: Length::points(70.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_columns_max: vec![Length::points(60.0), Length::points(60.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = grid_child(&mut tree);
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 70.0);
    assert_close(tree.nodes[first].layout.size.width, 35.0);
    assert_close(tree.nodes[second].layout.offset.x, 35.0);
    assert_close(tree.nodes[second].layout.size.width, 35.0);
}

#[test]
fn min_content_grid_size_uses_zero_free_space_for_fixed_growth_limits() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::MinContent,
        height: Length::MinContent,
        grid_template_columns: vec![Length::points(20.0), Length::points(20.0)],
        grid_template_columns_max: vec![Length::points(80.0), Length::points(80.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        grid_template_rows_max: vec![Length::points(40.0), Length::points(40.0)],
        ..Style::default()
    }));
    let first = grid_child(&mut tree);
    let second = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, first);
    tree.append_child(root, second);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 40.0);
    assert_close(size.height, 20.0);
    assert_close(tree.nodes[first].layout.size.width, 20.0);
    assert_close(tree.nodes[first].layout.size.height, 10.0);
    assert_close(tree.nodes[second].layout.offset.x, 20.0);
    assert_close(tree.nodes[second].layout.offset.y, 10.0);
    assert_close(tree.nodes[second].layout.size.width, 20.0);
    assert_close(tree.nodes[second].layout.size.height, 10.0);
}

#[test]
fn grid_auto_column_pattern_initializes_leading_and_positive_implicit_tracks() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_rows: vec![Length::points(10.0)],
        grid_auto_columns: vec![Length::points(5.0), Length::points(7.0)],
        grid_auto_columns_max: vec![Length::points(8.0), Length::points(12.0)],
        justify_items: JustifyItems::Stretch,
        align_items: AlignItems::Stretch,
        ..Style::default()
    }));
    let leading_first = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(-4),
        grid_column_end: Some(-3),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let leading_second = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(-3),
        grid_column_end: Some(-2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let explicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let positive_implicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    for child in [leading_first, leading_second, explicit, positive_implicit] {
        tree.append_child(root, child);
    }

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 48.0);
    assert_close(tree.nodes[leading_first].layout.offset.x, 0.0);
    assert_close(tree.nodes[leading_first].layout.size.width, 8.0);
    assert_close(tree.nodes[leading_second].layout.offset.x, 8.0);
    assert_close(tree.nodes[leading_second].layout.size.width, 12.0);
    assert_close(tree.nodes[explicit].layout.offset.x, 20.0);
    assert_close(tree.nodes[explicit].layout.size.width, 20.0);
    assert_close(tree.nodes[positive_implicit].layout.offset.x, 40.0);
    assert_close(tree.nodes[positive_implicit].layout.size.width, 8.0);
}

#[test]
fn grid_auto_row_pattern_initializes_leading_and_positive_implicit_tracks() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(20.0)],
        grid_auto_rows: vec![Length::points(5.0), Length::points(7.0)],
        grid_auto_rows_max: vec![Length::points(8.0), Length::points(12.0)],
        justify_items: JustifyItems::Stretch,
        align_items: AlignItems::Stretch,
        ..Style::default()
    }));
    let leading_first = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(-4),
        grid_row_end: Some(-3),
        ..Style::default()
    }));
    let leading_second = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(-3),
        grid_row_end: Some(-2),
        ..Style::default()
    }));
    let explicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    let positive_implicit = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    for child in [leading_first, leading_second, explicit, positive_implicit] {
        tree.append_child(root, child);
    }

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.height, 48.0);
    assert_close(tree.nodes[leading_first].layout.offset.y, 0.0);
    assert_close(tree.nodes[leading_first].layout.size.height, 8.0);
    assert_close(tree.nodes[leading_second].layout.offset.y, 8.0);
    assert_close(tree.nodes[leading_second].layout.size.height, 12.0);
    assert_close(tree.nodes[explicit].layout.offset.y, 20.0);
    assert_close(tree.nodes[explicit].layout.size.height, 20.0);
    assert_close(tree.nodes[positive_implicit].layout.offset.y, 40.0);
    assert_close(tree.nodes[positive_implicit].layout.size.height, 8.0);
}

#[test]
fn grid_column_track_sizing_function_matrix_initializes_base_sizes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(220.0),
        height: Length::points(10.0),
        justify_content: JustifyContent::FlexStart,
        align_content: AlignContent::FlexStart,
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::percent(25.0),
            Length::calc(5.0, 10.0),
            Length::Auto,
            Length::MaxContent,
            Length::fit_content(Some(BaseLength::fixed(40.0))),
        ],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    let calc = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    let auto = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(4),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(30.0, 10.0),
    ));
    let max_content = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(5),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(35.0, 10.0),
    ));
    let fit_content = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(6),
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(45.0, 10.0),
    ));
    let trailing_marker = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(7),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    for child in [
        fixed,
        percent,
        calc,
        auto,
        max_content,
        fit_content,
        trailing_marker,
    ] {
        tree.append_child(root, child);
    }

    let size = run_rust_layout(&mut tree, root, Constraints::definite(220.0, 10.0));

    assert_close(size.width, 220.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 0.0);
    assert_close(tree.nodes[percent].layout.offset.x, 10.0);
    assert_close(tree.nodes[calc].layout.offset.x, 65.0);
    assert_close(tree.nodes[auto].layout.offset.x, 92.0);
    assert_close(tree.nodes[auto].layout.size.width, 30.0);
    assert_close(tree.nodes[max_content].layout.offset.x, 122.0);
    assert_close(tree.nodes[max_content].layout.size.width, 35.0);
    assert_close(tree.nodes[fit_content].layout.offset.x, 157.0);
    assert_close(tree.nodes[trailing_marker].layout.offset.x, 197.0);
}

#[test]
fn grid_row_track_sizing_function_matrix_initializes_base_sizes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(10.0),
        height: Length::points(160.0),
        justify_content: JustifyContent::FlexStart,
        align_content: AlignContent::FlexStart,
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![
            Length::points(8.0),
            Length::percent(25.0),
            Length::calc(4.0, 10.0),
            Length::Auto,
            Length::MaxContent,
            Length::fit_content(Some(BaseLength::fixed(30.0))),
        ],
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    let percent = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    })));
    let calc = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(3),
        ..Style::default()
    })));
    let auto = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(4),
            ..Style::default()
        }),
        Size::new(10.0, 24.0),
    ));
    let max_content = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(5),
            ..Style::default()
        }),
        Size::new(10.0, 28.0),
    ));
    let fit_content = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(6),
            ..Style::default()
        }),
        Size::new(10.0, 36.0),
    ));
    let trailing_marker = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(7),
        ..Style::default()
    })));
    for child in [
        fixed,
        percent,
        calc,
        auto,
        max_content,
        fit_content,
        trailing_marker,
    ] {
        tree.append_child(root, child);
    }

    let size = run_rust_layout(&mut tree, root, Constraints::definite(10.0, 160.0));

    assert_close(size.height, 160.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 0.0);
    assert_close(tree.nodes[percent].layout.offset.y, 8.0);
    assert_close(tree.nodes[calc].layout.offset.y, 48.0);
    assert_close(tree.nodes[auto].layout.offset.y, 68.0);
    assert_close(tree.nodes[auto].layout.size.height, 24.0);
    assert_close(tree.nodes[max_content].layout.offset.y, 92.0);
    assert_close(tree.nodes[max_content].layout.size.height, 28.0);
    assert_close(tree.nodes[fit_content].layout.offset.y, 120.0);
    assert_close(tree.nodes[trailing_marker].layout.offset.y, 150.0);
}

#[test]
fn indefinite_grid_max_size_track_redistribution_does_not_subtract_gaps() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        max_width: Length::points(210.0),
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_columns_max: vec![Length::points(70.0), Length::points(90.0), Length::Auto],
        grid_template_rows: vec![Length::Auto],
        column_gap: Length::points(7.0),
        ..Style::default()
    }));
    let fixed = tree.push(SimpleNode::new(grid_start_item(Style {
        width: Length::points(64.0),
        height: Length::points(28.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    let first_marker = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    let second_marker = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    let third_marker = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, fixed);
    tree.append_child(root, first_marker);
    tree.append_child(root, second_marker);
    tree.append_child(root, third_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 210.0);
    assert_close(tree.nodes[first_marker].layout.offset.x, 0.0);
    assert_close(tree.nodes[second_marker].layout.offset.x, 76.0);
    assert_close(tree.nodes[third_marker].layout.offset.x, 152.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 155.0);
}

#[test]
fn indefinite_grid_auto_minimum_spanning_items_feed_growth_limits_not_base_sizes() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::Start,
        align_content: AlignContent::FlexStart,
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Start,
        min_width: Length::points(150.0),
        max_width: Length::points(210.0),
        min_height: Length::points(90.0),
        max_height: Length::points(150.0),
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_columns_max: vec![Length::points(70.0), Length::points(90.0), Length::Auto],
        grid_template_rows: vec![Length::Auto, Length::points(18.0)],
        grid_template_rows_max: vec![Length::Auto, Length::points(18.0)],
        column_gap: Length::points(7.0),
        row_gap: Length::points(5.0),
        padding: Rect::new(
            Length::points(4.0),
            Length::points(6.0),
            Length::points(3.0),
            Length::points(2.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        ..Style::default()
    }));
    let first_span = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(108.0, 46.0),
    ));
    let second_span = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(2),
            grid_column_span: 2,
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(96.0, 54.0),
    ));
    let full_span = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(1),
            grid_column_span: 3,
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(176.0, 72.0),
    ));
    let fixed = tree.push(SimpleNode::new(grid_start_item(Style {
        width: Length::points(64.0),
        height: Length::points(28.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    let auto_marker = tree.push(SimpleNode::new(grid_start_item(Style {
        width: Length::points(18.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, first_span);
    tree.append_child(root, second_span);
    tree.append_child(root, full_span);
    tree.append_child(root, fixed);
    tree.append_child(root, auto_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 223.0);
    assert_close(size.height, 103.0);
    assert_close(tree.nodes[second_span].layout.offset.x, 81.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 160.0);
    assert_close(tree.nodes[auto_marker].layout.offset.x, 5.0);
    assert_close(tree.nodes[auto_marker].layout.offset.y, 81.0);
}

#[test]
fn indefinite_grid_row_fixed_max_spanning_items_respect_max_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::Start,
        align_content: AlignContent::FlexStart,
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Start,
        min_width: Length::points(150.0),
        max_width: Length::points(280.0),
        min_height: Length::points(90.0),
        max_height: Length::points(180.0),
        grid_template_columns: vec![Length::Auto, Length::points(18.0)],
        grid_template_columns_max: vec![Length::Auto, Length::points(18.0)],
        grid_template_rows: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_rows_max: vec![Length::points(70.0), Length::points(90.0), Length::Auto],
        column_gap: Length::points(7.0),
        row_gap: Length::points(5.0),
        padding: Rect::new(
            Length::points(4.0),
            Length::points(6.0),
            Length::points(3.0),
            Length::points(2.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        ..Style::default()
    }));
    let first_span = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_row_start: Some(1),
            grid_row_span: 2,
            grid_column_start: Some(1),
            ..Style::default()
        }),
        Size::new(46.0, 108.0),
    ));
    let second_span = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_row_start: Some(2),
            grid_row_span: 2,
            grid_column_start: Some(1),
            ..Style::default()
        }),
        Size::new(54.0, 96.0),
    ));
    let full_span = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_row_start: Some(1),
            grid_row_span: 3,
            grid_column_start: Some(1),
            ..Style::default()
        }),
        Size::new(72.0, 176.0),
    ));
    let fixed = tree.push(SimpleNode::new(grid_start_item(Style {
        width: Length::points(28.0),
        height: Length::points(64.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_row_start: Some(3),
        grid_column_start: Some(1),
        ..Style::default()
    })));
    let auto_marker = tree.push(SimpleNode::new(grid_start_item(Style {
        width: Length::points(9.0),
        height: Length::points(18.0),
        grid_column_start: Some(2),
        ..Style::default()
    })));
    tree.append_child(root, first_span);
    tree.append_child(root, second_span);
    tree.append_child(root, full_span);
    tree.append_child(root, fixed);
    tree.append_child(root, auto_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 163.0);
    assert_close(size.height, 188.0);
    assert_close(tree.nodes[first_span].layout.offset.y, 4.0);
    assert_close(tree.nodes[second_span].layout.offset.y, 67.0);
    assert_close(tree.nodes[full_span].layout.offset.y, 4.0);
    assert_close(tree.nodes[fixed].layout.offset.y, 132.0);
    assert_close(tree.nodes[auto_marker].layout.offset.x, 84.0);
}

#[test]
fn indefinite_grid_fit_content_max_spanning_items_grow_base_tracks() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::Start,
        align_content: AlignContent::FlexStart,
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Start,
        grid_template_columns: vec![Length::Auto, Length::Auto, Length::Auto],
        grid_template_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed(70.0))),
            Length::fit_content(Some(BaseLength::fixed(90.0))),
            Length::Auto,
        ],
        grid_template_rows: vec![Length::Auto, Length::points(18.0)],
        grid_template_rows_max: vec![Length::Auto, Length::points(18.0)],
        column_gap: Length::points(7.0),
        row_gap: Length::points(5.0),
        padding: Rect::new(
            Length::points(4.0),
            Length::points(6.0),
            Length::points(3.0),
            Length::points(2.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        ..Style::default()
    }));
    let first_span = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(108.0, 46.0),
    ));
    let second_span = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(2),
            grid_column_span: 2,
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(96.0, 54.0),
    ));
    let full_span = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(1),
            grid_column_span: 3,
            grid_row_start: Some(1),
            ..Style::default()
        }),
        Size::new(176.0, 72.0),
    ));
    let fixed = tree.push(SimpleNode::new(grid_start_item(Style {
        width: Length::points(64.0),
        height: Length::points(28.0),
        margin: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(2.0),
            Length::points(4.0),
        ),
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    let auto_marker = tree.push(SimpleNode::new(grid_start_item(Style {
        width: Length::points(18.0),
        height: Length::points(9.0),
        ..Style::default()
    })));
    tree.append_child(root, first_span);
    tree.append_child(root, second_span);
    tree.append_child(root, full_span);
    tree.append_child(root, fixed);
    tree.append_child(root, auto_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 206.0);
    assert_close(size.height, 103.0);
    assert_close(tree.nodes[first_span].layout.offset.x, 5.0);
    assert_close(tree.nodes[second_span].layout.offset.x, 30.0);
    assert_close(tree.nodes[full_span].layout.offset.x, 5.0);
    assert_close(tree.nodes[fixed].layout.offset.x, 129.0);
    assert_close(tree.nodes[auto_marker].layout.offset.y, 81.0);
}

#[test]
fn grid_max_content_track_grows_from_measured_child_when_container_is_indefinite() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::max_content()],
        grid_template_rows: vec![Length::max_content()],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(45.0, 18.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 45.0);
    assert_close(size.height, 18.0);
    assert_close(tree.nodes[child].layout.size.width, 45.0);
    assert_close(tree.nodes[child].layout.size.height, 18.0);
}

#[test]
fn grid_min_content_track_uses_external_min_content_contribution() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::MinContent, Length::MaxContent, Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let min_content = tree.push(MeasuringNode::measured_with_intrinsic(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(70.0, 10.0),
        Size::new(30.0, 10.0),
        Size::new(70.0, 10.0),
    ));
    let max_content = tree.push(MeasuringNode::measured_with_intrinsic(
        Style {
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(70.0, 10.0),
        Size::new(20.0, 10.0),
        Size::new(70.0, 10.0),
    ));
    let trailing_marker = tree.push(MeasuringNode::new(grid_start_item(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, min_content);
    tree.append_child(root, max_content);
    tree.append_child(root, trailing_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 100.0);
    assert_close(tree.nodes[min_content].layout.offset.x, 0.0);
    assert_close(tree.nodes[max_content].layout.offset.x, 30.0);
    assert_close(tree.nodes[trailing_marker].layout.offset.x, 100.0);
    assert!(
        tree.nodes[min_content]
            .min_content_constraints
            .iter()
            .any(|constraints| constraints.width.mode == MeasureMode::Indefinite),
        "min-content contribution should be queried with initial indefinite inline constraints"
    );
}

#[test]
fn grid_max_content_track_uses_external_max_content_contribution() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::MaxContent, Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let intrinsic = tree.push(MeasuringNode::measured_with_intrinsic(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(40.0, 10.0),
        Size::new(25.0, 10.0),
        Size::new(90.0, 10.0),
    ));
    let trailing_marker = tree.push(MeasuringNode::new(grid_start_item(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, trailing_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 90.0);
    assert_close(tree.nodes[intrinsic].layout.size.width, 40.0);
    assert_close(tree.nodes[trailing_marker].layout.offset.x, 90.0);
    assert!(
        tree.nodes[intrinsic]
            .max_content_constraints
            .iter()
            .any(|constraints| constraints.width.mode == MeasureMode::Indefinite),
        "max-content contribution should be queried with initial indefinite inline constraints"
    );
}

#[test]
fn grid_ignores_non_finite_external_max_content_contribution() {
    let mut tree = MeasuringTree::default();
    let root = tree.push(MeasuringNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::MaxContent, Length::points(0.0)],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let intrinsic = tree.push(MeasuringNode::measured_with_intrinsic(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(40.0, 10.0),
        Size::new(25.0, 10.0),
        Size::new(f32::INFINITY, 10.0),
    ));
    let trailing_marker = tree.push(MeasuringNode::new(grid_start_item(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, trailing_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 40.0);
    assert_close(tree.nodes[intrinsic].layout.size.width, 40.0);
    assert_close(tree.nodes[trailing_marker].layout.offset.x, 40.0);
    assert!(
        tree.nodes[intrinsic]
            .max_content_constraints
            .iter()
            .any(|constraints| constraints.width.mode == MeasureMode::Indefinite),
        "max-content contribution should still be queried before falling back"
    );
}

#[test]
fn grid_minmax_fixed_auto_track_grows_to_measured_child_when_indefinite() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_columns_max: vec![Length::Auto],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(45.0, 10.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 45.0);
    assert_close(tree.nodes[child].layout.size.width, 45.0);
}

#[test]
fn grid_min_content_track_uses_minimum_contribution_not_measured_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::MinContent, Length::MaxContent],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let min_content = tree.push(SimpleNode::with_measured_size(
        Style {
            min_width: Length::points(30.0),
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(70.0, 10.0),
    ));
    let max_content = tree.push(SimpleNode::with_measured_size(
        Style {
            min_width: Length::points(30.0),
            grid_column_start: Some(2),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(70.0, 10.0),
    ));
    let trailing_marker = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, min_content);
    tree.append_child(root, max_content);
    tree.append_child(root, trailing_marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 100.0);
    assert_close(tree.nodes[min_content].layout.offset.x, 0.0);
    assert_close(tree.nodes[min_content].layout.size.width, 70.0);
    assert_close(tree.nodes[max_content].layout.offset.x, 30.0);
    assert_close(tree.nodes[max_content].layout.size.width, 70.0);
    assert_close(tree.nodes[trailing_marker].layout.offset.x, 100.0);
}

#[test]
fn grid_minmax_min_content_maximum_uses_minimum_contribution_as_growth_limit() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(20.0), Length::points(10.0)],
        grid_template_columns_max: vec![Length::MinContent, Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            min_width: Length::points(45.0),
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(70.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 55.0);
    assert_close(tree.nodes[intrinsic].layout.size.width, 70.0);
    assert_close(tree.nodes[following].layout.offset.x, 45.0);
}

#[test]
fn grid_spanning_min_content_maximum_distributes_minimum_contribution_across_tracks() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![
            Length::points(10.0),
            Length::points(10.0),
            Length::points(10.0),
        ],
        grid_template_columns_max: vec![
            Length::MinContent,
            Length::MinContent,
            Length::points(10.0),
        ],
        grid_template_rows: vec![Length::points(10.0)],
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            min_width: Length::points(50.0),
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(90.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(grid_start_item(Style {
        grid_column_start: Some(3),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 60.0);
    assert_close(tree.nodes[intrinsic].layout.size.width, 90.0);
    assert_close(tree.nodes[following].layout.offset.x, 50.0);
}

#[test]
fn grid_minmax_max_content_minimum_can_exceed_fixed_maximum() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::MaxContent],
        grid_template_columns_max: vec![Length::points(40.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(70.0, 10.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 70.0);
    assert_close(tree.nodes[child].layout.size.width, 70.0);
}

#[test]
fn grid_spanning_max_content_minimum_track_can_exceed_fixed_maximum() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::MaxContent, Length::points(10.0)],
        grid_template_columns_max: vec![Length::points(40.0), Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0), Length::points(10.0)],
        column_gap: Length::points(5.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let spanning = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_span: 2,
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(100.0, 10.0),
    ));
    let marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, spanning);
    tree.append_child(root, marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 100.0);
    assert_close(tree.nodes[marker].layout.offset.x, 90.0);
}

#[test]
fn definite_grid_max_content_minimum_floors_fixed_maximum_before_alignment() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(120.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::MaxContent, Length::points(10.0)],
        grid_template_columns_max: vec![Length::points(40.0), Length::points(10.0)],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(5.0),
        justify_content: JustifyContent::Center,
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(70.0, 10.0),
    ));
    let marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, marker);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(120.0, 10.0));

    assert_close(size.width, 120.0);
    assert_close(tree.nodes[intrinsic].layout.offset.x, 18.0);
    assert_close(tree.nodes[marker].layout.offset.x, 93.0);
}

#[test]
fn grid_max_content_minimum_floors_fit_content_maximum() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::MaxContent, Length::points(10.0)],
        grid_template_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed(40.0))),
            Length::points(10.0),
        ],
        grid_template_rows: vec![Length::points(10.0)],
        column_gap: Length::points(5.0),
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(70.0, 10.0),
    ));
    let marker = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, marker);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 85.0);
    assert_close(tree.nodes[marker].layout.offset.x, 75.0);
}

#[test]
fn definite_grid_minmax_auto_maximum_updates_growth_limit() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::Center,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0)],
        grid_template_columns_max: vec![Length::Auto],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style {
            justify_self: JustifyItems::Start,
            align_self: Some(AlignItems::FlexStart),
            ..Style::default()
        },
        Size::new(45.0, 10.0),
    ));
    tree.append_child(root, child);

    run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(tree.nodes[child].layout.offset.x, 28.0);
    assert_close(tree.nodes[child].layout.size.width, 45.0);
}

#[test]
fn definite_grid_spanning_fit_content_max_track_contributes_to_content_alignment_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(140.0),
        height: Length::points(90.0),
        grid_template_columns: vec![Length::Auto, Length::points(20.0)],
        grid_template_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed(40.0))),
            Length::points(20.0),
        ],
        grid_template_rows: vec![Length::Auto, Length::Auto],
        grid_template_rows_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(4.0, 50.0))),
            Length::MaxContent,
        ],
        column_gap: Length::points(3.0),
        row_gap: Length::points(2.0),
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Start,
        ..Style::default()
    }));
    let spanning = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_span: 2,
            ..Style::default()
        },
        Size::new(80.0, 12.0),
    ));
    let marker = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(2),
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::points(1.0),
                Length::ZERO,
            ),
            ..Style::default()
        },
        Size::new(30.0, 18.0),
    ));
    tree.append_child(root, spanning);
    tree.append_child(root, marker);

    run_rust_layout(&mut tree, root, Constraints::definite(140.0, 90.0));

    assert_close(tree.nodes[spanning].layout.offset.x, 39.0);
    assert_close(tree.nodes[marker].layout.offset.x, 40.0);
    assert_close(tree.nodes[marker].layout.offset.y, 15.0);
}

#[test]
fn definite_grid_spanning_flexible_max_tracks_expand_before_max_content_sibling() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(150.0),
        height: Length::points(96.0),
        padding: Rect::all(Length::points(2.0)),
        grid_template_columns: vec![Length::Auto, Length::points(20.0), Length::Auto],
        grid_template_columns_max: vec![Length::fr(1.0), Length::fr(1.0), Length::MaxContent],
        grid_template_rows: vec![Length::Auto, Length::points(18.0)],
        grid_template_rows_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(4.0, 40.0))),
            Length::MaxContent,
        ],
        column_gap: Length::points(4.0),
        row_gap: Length::points(3.0),
        justify_content: JustifyContent::FlexStart,
        align_content: AlignContent::FlexStart,
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Start,
        ..Style::default()
    }));
    let spanning = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_column_span: 3,
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(116.0, 14.0),
    ));
    let marker = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(2),
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::points(1.0),
                Length::ZERO,
            ),
            ..Style::default()
        },
        Size::new(38.0, 16.0),
    ));
    let tail = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(3),
            grid_row_start: Some(2),
            ..Style::default()
        },
        Size::new(52.0, 12.0),
    ));
    tree.append_child(root, spanning);
    tree.append_child(root, marker);
    tree.append_child(root, tail);

    run_rust_layout(&mut tree, root, Constraints::definite(160.0, 110.0));

    assert_close(tree.nodes[root].layout.size.width, 154.0);
    assert_close(tree.nodes[spanning].layout.offset.x, 2.0);
    assert_close(tree.nodes[marker].layout.offset.x, 3.0);
    assert_close(tree.nodes[tail].layout.offset.x, 100.0);
    assert_close(tree.nodes[tail].layout.offset.y, 19.0);
}

#[test]
fn dense_grid_spanning_auto_rows_prefer_indefinite_growth_limits() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(140.0),
        height: Length::points(90.0),
        grid_template_columns: vec![Length::points(18.0), Length::Auto],
        grid_template_columns_max: vec![Length::points(18.0), Length::fr(1.0)],
        grid_template_rows: vec![Length::points(12.0), Length::Auto],
        grid_template_rows_max: vec![Length::points(12.0), Length::MaxContent],
        grid_auto_columns: vec![
            Length::Auto,
            Length::fit_content(Some(BaseLength::fixed(22.0))),
        ],
        grid_auto_columns_max: vec![Length::MaxContent, Length::fr(1.0)],
        grid_auto_rows: vec![Length::Auto, Length::points(9.0)],
        grid_auto_rows_max: vec![Length::MaxContent, Length::fr(1.0)],
        grid_auto_flow: GridAutoFlow::Dense,
        column_gap: Length::points(2.0),
        row_gap: Length::points(3.0),
        justify_content: JustifyContent::Center,
        align_content: AlignContent::Center,
        ..Style::default()
    }));
    let explicit_late = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(2),
            grid_row_start: Some(2),
            grid_row_span: 2,
            ..Style::default()
        },
        Size::new(34.0, 40.0),
    ));
    let span = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_span: 2,
            ..Style::default()
        },
        Size::new(60.0, 18.0),
    ));
    let filler = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(20.0, 10.0),
    ));
    tree.append_child(root, explicit_late);
    tree.append_child(root, span);
    tree.append_child(root, filler);

    run_rust_layout(&mut tree, root, Constraints::definite(140.0, 90.0));

    assert_close(tree.nodes[explicit_late].layout.offset.x, 20.0);
    assert_close(tree.nodes[explicit_late].layout.offset.y, 33.0);
    assert_close(tree.nodes[explicit_late].layout.size.width, 120.0);
    assert_close(tree.nodes[span].layout.offset.y, 18.0);
    assert_close(tree.nodes[span].layout.size.width, 140.0);
    assert_close(tree.nodes[span].layout.size.height, 12.0);
    assert_close(tree.nodes[filler].layout.offset.x, 0.0);
    assert_close(tree.nodes[filler].layout.offset.y, 33.0);
    assert_close(tree.nodes[filler].layout.size.width, 18.0);
    assert_close(tree.nodes[filler].layout.size.height, 10.0);
}

#[test]
fn grid_fit_content_track_caps_fixed_item_growth_to_argument() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::fit_content(Some(BaseLength::fixed(40.0)))],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::new(Style {
        width: Length::points(70.0),
        height: Length::points(10.0),
        ..Style::default()
    }));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 40.0);
    assert_close(tree.nodes[child].layout.size.width, 70.0);
    assert_close(tree.nodes[child].layout.offset.x, 0.0);
}

#[test]
fn grid_fit_content_track_clamps_intrinsic_growth_to_argument() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::fit_content(Some(BaseLength::fixed(40.0)))],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(70.0, 10.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 40.0);
}

#[test]
fn grid_minmax_fit_content_max_caps_track_and_preserves_following_offset() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_template_columns: vec![Length::points(20.0), Length::points(10.0)],
        grid_template_columns_max: vec![Length::fit_content(Some(BaseLength::fixed(40.0)))],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(70.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    let size = run_rust_layout(&mut tree, root, Constraints::indefinite());

    assert_close(size.width, 50.0);
    assert_close(tree.nodes[following].layout.offset.x, 40.0);
}

#[test]
fn definite_grid_minmax_fit_content_percent_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(120.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(10.0)],
        grid_template_columns_max: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            0.0, 50.0,
        )))],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(90.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(120.0, 10.0));

    assert_close(size.width, 120.0);
    assert_close(tree.nodes[following].layout.offset.x, 60.0);
}

#[test]
fn definite_grid_minmax_fit_content_calc_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(120.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(20.0), Length::points(10.0)],
        grid_template_columns_max: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            10.0, 50.0,
        )))],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(90.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(120.0, 10.0));

    assert_close(size.width, 120.0);
    assert_close(tree.nodes[following].layout.offset.x, 70.0);
}

#[test]
fn definite_grid_minmax_fit_content_percent_row_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(10.0),
        height: Length::points(120.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(10.0)],
        grid_template_rows_max: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            0.0, 50.0,
        )))],
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(10.0, 90.0),
    ));
    let following = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(10.0, 120.0));

    assert_close(size.height, 120.0);
    assert_close(tree.nodes[following].layout.offset.y, 60.0);
}

#[test]
fn definite_grid_minmax_fit_content_calc_row_max_caps_track() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(10.0),
        height: Length::points(120.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(10.0)],
        grid_template_rows_max: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            10.0, 50.0,
        )))],
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(10.0, 90.0),
    ));
    let following = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(10.0, 120.0));

    assert_close(size.height, 120.0);
    assert_close(tree.nodes[following].layout.offset.y, 70.0);
}

#[test]
fn grid_fit_content_track_caps_intrinsic_growth_in_definite_container() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(100.0),
        height: Length::points(10.0),
        grid_template_columns: vec![
            Length::fit_content(Some(BaseLength::fixed(40.0))),
            Length::points(10.0),
        ],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let intrinsic = tree.push(SimpleNode::with_measured_size(
        Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        },
        Size::new(70.0, 10.0),
    ));
    let following = tree.push(SimpleNode::new(Style {
        grid_column_start: Some(2),
        grid_row_start: Some(1),
        ..Style::default()
    }));
    tree.append_child(root, intrinsic);
    tree.append_child(root, following);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(100.0, 10.0));

    assert_close(size.width, 100.0);
    assert_close(tree.nodes[following].layout.offset.x, 40.0);
}

#[test]
fn grid_fit_content_percent_track_caps_intrinsic_growth_in_definite_container() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(120.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            0.0, 50.0,
        )))],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(90.0, 10.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(120.0, 10.0));

    assert_close(size.width, 120.0);
    assert_close(tree.nodes[child].layout.size.width, 60.0);
}

#[test]
fn grid_fit_content_calc_track_caps_intrinsic_growth_in_definite_container() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(120.0),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            10.0, 50.0,
        )))],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(90.0, 10.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(120.0, 10.0));

    assert_close(size.width, 120.0);
    assert_close(tree.nodes[child].layout.size.width, 70.0);
}

#[test]
fn grid_fit_content_percent_row_track_caps_intrinsic_growth_in_definite_container() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(10.0),
        height: Length::points(120.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            0.0, 50.0,
        )))],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(10.0, 90.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(10.0, 120.0));

    assert_close(size.height, 120.0);
    assert_close(tree.nodes[child].layout.size.height, 60.0);
}

#[test]
fn grid_fit_content_calc_row_track_caps_intrinsic_growth_in_definite_container() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(10.0),
        height: Length::points(120.0),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::fit_content(Some(BaseLength::fixed_and_percent(
            10.0, 50.0,
        )))],
        ..Style::default()
    }));
    let child = tree.push(SimpleNode::with_measured_size(
        Style::default(),
        Size::new(10.0, 90.0),
    ));
    tree.append_child(root, child);

    let size = run_rust_layout(&mut tree, root, Constraints::definite(10.0, 120.0));

    assert_close(size.height, 120.0);
    assert_close(tree.nodes[child].layout.size.height, 70.0);
}

#[test]
fn root_grid_fit_content_percent_argument_preserves_larger_track_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(140.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = grid_child(&mut tree);
    tree.append_child(root, child);

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 20.0),
    );

    assert_close(size.width, 140.0);
    assert_close(tree.nodes[root].layout.size.width, 140.0);
    assert_close(tree.nodes[child].layout.size.width, 140.0);
}

#[test]
fn root_grid_fit_content_calc_argument_preserves_larger_track_width() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        height: Length::points(10.0),
        grid_template_columns: vec![Length::points(140.0)],
        grid_template_rows: vec![Length::points(10.0)],
        ..Style::default()
    }));
    let child = grid_child(&mut tree);
    tree.append_child(root, child);

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 20.0),
    );

    assert_close(size.width, 140.0);
    assert_close(tree.nodes[root].layout.size.width, 140.0);
    assert_close(tree.nodes[child].layout.size.width, 140.0);
}

#[test]
fn root_grid_fit_content_percent_argument_preserves_larger_track_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(10.0),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(140.0)],
        ..Style::default()
    }));
    let child = grid_child(&mut tree);
    tree.append_child(root, child);

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(20.0, 200.0),
    );

    assert_close(size.height, 140.0);
    assert_close(tree.nodes[root].layout.size.height, 140.0);
    assert_close(tree.nodes[child].layout.size.height, 140.0);
}

#[test]
fn root_grid_fit_content_calc_argument_preserves_larger_track_height() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(10.0),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        grid_template_columns: vec![Length::points(10.0)],
        grid_template_rows: vec![Length::points(140.0)],
        ..Style::default()
    }));
    let child = grid_child(&mut tree);
    tree.append_child(root, child);

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(20.0, 200.0),
    );

    assert_close(size.height, 140.0);
    assert_close(tree.nodes[root].layout.size.height, 140.0);
    assert_close(tree.nodes[child].layout.size.height, 140.0);
}

#[test]
fn child_grid_fit_content_percent_argument_preserves_larger_track_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(160.0),
        ..Style::default()
    }));
    let child_grid = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 25.0))),
        grid_template_columns: vec![Length::points(140.0)],
        grid_template_rows: vec![Length::points(60.0)],
        ..Style::default()
    }));
    let item = grid_child(&mut tree);
    tree.append_child(root, child_grid);
    tree.append_child(child_grid, item);

    LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 160.0),
    );

    assert_close(tree.nodes[child_grid].layout.size.width, 140.0);
    assert_close(tree.nodes[child_grid].layout.size.height, 60.0);
    assert_close(tree.nodes[item].layout.size.width, 140.0);
    assert_close(tree.nodes[item].layout.size.height, 60.0);
}

#[test]
fn child_grid_fit_content_calc_argument_preserves_larger_track_size() {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        width: Length::points(200.0),
        height: Length::points(160.0),
        ..Style::default()
    }));
    let child_grid = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 25.0))),
        grid_template_columns: vec![Length::points(140.0)],
        grid_template_rows: vec![Length::points(60.0)],
        ..Style::default()
    }));
    let item = grid_child(&mut tree);
    tree.append_child(root, child_grid);
    tree.append_child(child_grid, item);

    LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::definite(200.0, 160.0),
    );

    assert_close(tree.nodes[child_grid].layout.size.width, 140.0);
    assert_close(tree.nodes[child_grid].layout.size.height, 60.0);
    assert_close(tree.nodes[item].layout.size.width, 140.0);
    assert_close(tree.nodes[item].layout.size.height, 60.0);
}

#[test]
fn indefinite_grid_fixed_min_fit_content_max_spanning_growth_keeps_base_and_limit_separate() {
    let (mut tree, root, marker) = fit_content_max_spanning_grid();

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::indefinite(),
    );

    assert_close(size.width, 223.0);
    assert_close(size.height, 77.0);
    assert_close(tree.nodes[marker].layout.offset.x, 197.0);
}

#[test]
fn at_most_grid_fit_content_max_updates_growth_limits_without_max_content_base_growth() {
    let (mut tree, root, marker) = fit_content_max_spanning_grid();

    let size = LayoutEngine::new().layout_with_owner_constraints(
        &mut tree,
        root,
        Constraints::new(
            SideConstraint::at_most(148.0),
            SideConstraint::at_most(94.0),
        ),
    );

    assert_close(size.width, 144.0);
    assert_close(size.height, 77.0);
    assert_close(tree.nodes[marker].layout.offset.x, 118.0);
}

fn fit_content_max_spanning_grid() -> (SimpleTree, usize, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        justify_content: JustifyContent::FlexStart,
        align_content: AlignContent::FlexStart,
        justify_items: JustifyItems::Start,
        align_items: AlignItems::FlexStart,
        column_gap: Length::points(3.0),
        row_gap: Length::points(2.0),
        padding: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(1.0),
            Length::points(4.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        grid_template_columns: vec![Length::points(16.0), Length::Auto, Length::points(10.0)],
        grid_template_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(6.0, 32.0))),
            Length::fit_content(Some(BaseLength::fixed(46.0))),
            Length::points(22.0),
        ],
        grid_template_rows: vec![Length::Auto, Length::points(12.0)],
        grid_template_rows_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(4.0, 40.0))),
            Length::MaxContent,
        ],
        ..Style::default()
    }));
    let capped = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::points(1.0),
                Length::ZERO,
            ),
            ..Style::default()
        }),
        Size::new(92.0, 54.0),
    ));
    let spanning = tree.push(SimpleNode::with_measured_size(
        grid_start_item(Style {
            grid_column_start: Some(1),
            grid_column_span: 2,
            grid_row_start: Some(1),
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::points(1.0),
                Length::ZERO,
            ),
            ..Style::default()
        }),
        Size::new(108.0, 28.0),
    ));
    let marker = tree.push(SimpleNode::new(grid_start_item(Style {
        width: Length::points(10.0),
        height: Length::points(9.0),
        grid_column_start: Some(3),
        grid_row_start: Some(2),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    })));

    tree.append_child(root, capped);
    tree.append_child(root, spanning);
    tree.append_child(root, marker);

    (tree, root, marker)
}

fn grid_child(tree: &mut SimpleTree) -> usize {
    tree.push(SimpleNode::new(Style::default()))
}

fn grid_start_item(style: Style) -> Style {
    Style {
        display: Display::Block,
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        ..style
    }
}
