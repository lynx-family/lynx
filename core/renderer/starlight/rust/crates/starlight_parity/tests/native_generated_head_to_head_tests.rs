// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#![forbid(unsafe_code)]

use std::fmt::Debug;

use starlight_cpp::{BaselineLayoutTree, CppBaselineError, CppStarlightEngine};
use starlight_layout::{
    AlignContent, AlignItems, BaseLength, BoxSizing, Constraints, Direction, Display,
    FlexDirection, FlexWrap, GridAutoFlow, JustifyContent, JustifyItems, LayoutEngine,
    LayoutResult, LayoutTree, Length, LinearCrossGravity, LinearGravity, LinearLayoutGravity,
    LinearOrientation, PositionType, Rect, RelativeCenter, SideConstraint, SimpleNode, SimpleTree,
    Size, Style, RELATIVE_ALIGN_PARENT,
};
use starlight_parity::{collect_layout_snapshots, run_head_to_head, LayoutTolerance, ParityError};

const LAYOUT_DIRECTIONS: [Direction; 2] = [Direction::Ltr, Direction::Rtl];
const DEFAULT_DETERMINISTIC_SUPPORTED_TREE_CASES: usize = 32768;
const ENV_GENERATED_CASE: &str = "STARLIGHT_GENERATED_CASE";
const ENV_GENERATED_CASE_COUNT: &str = "STARLIGHT_GENERATED_CASE_COUNT";

#[derive(Clone, Copy, Debug)]
enum GeneratedContainer {
    Block,
    FlexRow,
    FlexColumnRtl,
    LinearRow,
    LinearColumnRtl,
    Relative,
    Grid,
}

const GENERATED_CONTAINERS: [GeneratedContainer; 7] = [
    GeneratedContainer::Block,
    GeneratedContainer::FlexRow,
    GeneratedContainer::FlexColumnRtl,
    GeneratedContainer::LinearRow,
    GeneratedContainer::LinearColumnRtl,
    GeneratedContainer::Relative,
    GeneratedContainer::Grid,
];

#[derive(Clone, Copy, Debug)]
enum OutOfFlowInset {
    None,
    Start,
    End,
    Both,
}

#[derive(Clone, Copy, Debug)]
enum OutOfFlowSizingVariant {
    PercentCalc,
    FillAvailable,
    OversizedFillAvailableMeasured,
    MinMaxMeasuredClamp,
    FitContentMeasured,
    AspectBorderBoxMeasured,
}

#[derive(Clone, Copy, Debug)]
enum GridOutOfFlowAreaPattern {
    SingleTrack,
    SpanningTracks,
    AutoPaddingEdges,
    LastLineToAutoEnd,
}

#[derive(Clone, Copy, Debug)]
enum GridOutOfFlowSizingVariant {
    FillAvailableInsets,
    MeasuredAlignment,
    FitContentSubtree,
    MeasuredFitContent,
}

#[derive(Clone, Copy, Debug)]
enum FixedDescendantVariant {
    PercentStart,
    CalcEnd,
    FillAvailable,
    MeasuredAspect,
    FitContentSubtree,
}

#[derive(Clone, Copy, Debug)]
enum StickyInsetLength {
    Points,
    Percent,
    Calc,
}

#[derive(Clone, Copy, Debug)]
enum StickySizingVariant {
    PercentCalc,
    AutoMeasured,
    MinMaxMeasuredClamp,
    FitContentMeasured,
    AspectBorderBoxMeasured,
}

#[derive(Clone, Copy, Debug)]
enum MeasuredVariant {
    Plain,
    Baseline,
    MinMax,
    AspectBorderBox,
}

#[derive(Clone, Copy, Debug)]
enum BaselineConstraintMode {
    DefiniteRoot,
    AtMostOwner,
    IndefiniteOwner,
}

#[derive(Clone, Copy, Debug)]
enum BaselineTrigger {
    ContainerAlignItems,
    ChildAlignSelf,
}

#[derive(Clone, Copy, Debug)]
enum BaselineSource {
    MeasuredLeaf,
    NestedFlex,
    NestedFlexColumn,
    NestedFlexColumnReverse,
    NestedLinear,
    NestedLinearVertical,
    NestedLinearVerticalReverse,
    NestedGridFallback,
    NestedRelativeFallback,
}

#[derive(Clone, Copy, Debug)]
enum GridPlacementPattern {
    AutoSpans,
    LockedLines,
    ImplicitLines,
}

#[derive(Clone, Copy, Debug)]
enum SizingVariant {
    PercentCalcRoot,
    FitContentRoot,
    FitContentSubtree,
    PercentMinMaxRoot,
    BorderBoxPercentMinMaxRoot,
    ContentBoxAspectRoot,
    BorderBoxAspectRoot,
    IntrinsicMeasuredChild,
}

#[derive(Clone, Copy, Debug)]
enum LinearConstraintMode {
    DefiniteRoot,
    AtMostOwner,
    IndefiniteOwner,
}

#[derive(Clone, Copy, Debug)]
enum LinearEdgePattern {
    WeightedMinMax,
    WeightSumMainGravity,
    LayoutGravityOverride,
    CrossAutoMarginBaseline,
}

#[derive(Clone, Copy, Debug)]
enum RelativeParentEdge {
    None,
    Start,
    End,
    Both,
}

#[derive(Clone, Copy, Debug)]
enum RelativeSiblingEdge {
    After,
    Before,
    AlignStart,
    AlignEnd,
}

#[derive(Clone, Copy, Debug)]
enum RelativeMeasuredConstraint {
    ParentEnd,
    ParentBoth,
    AfterAnchor,
    BeforeAnchor,
    BetweenAnchors,
}

#[derive(Clone, Copy, Debug)]
enum RelativeConstraintMode {
    DefiniteRoot,
    AtMostOwner,
    IndefiniteOwner,
}

#[derive(Clone, Copy, Debug)]
enum RelativeDependencyPattern {
    DuplicatePosition,
    DisplayNoneDuplicate,
    DuplicateEdgeAlignment,
    ParentEndRecompute,
    CombinedDependencyOrder,
}

#[derive(Clone, Copy, Debug)]
enum GridAutoMarginPattern {
    None,
    Start,
    End,
    Both,
}

#[derive(Clone, Copy, Debug)]
enum GridAutoMarginPlacement {
    ExplicitFirstCell,
    ExplicitSecondCell,
    AutoPlacedSpan,
}

#[derive(Clone, Copy, Debug)]
enum GridContentSizeMode {
    ExtraSpace,
    Overflow,
}

#[derive(Clone, Copy, Debug)]
enum GridTrackConstraintMode {
    DefiniteRoot,
    IndefiniteOwner,
    AtMostOwner,
}

#[derive(Clone, Copy, Debug)]
enum GridTrackSizingVariant {
    FlexibleMinMax,
    FixedMaxGrowthLimit,
    MaxContentMinimum,
    FitContentCaps,
    ImplicitAutoTracks,
    ImplicitFitContentMaxCaps,
}

#[derive(Clone, Copy, Debug)]
enum GeneratedMeasureBehavior {
    Fixed(Size),
    HeightFromWidth {
        intrinsic_width: f32,
        fallback_height: f32,
        height_ratio: f32,
    },
    WidthByHeightMode {
        at_most_width: f32,
        definite_width: f32,
        height: f32,
    },
}

#[test]
fn generated_flex_axis_alignment_matrix_matches_cpp() {
    let flex_directions = [
        FlexDirection::Row,
        FlexDirection::RowReverse,
        FlexDirection::Column,
        FlexDirection::ColumnReverse,
    ];
    let directions = LAYOUT_DIRECTIONS;
    let justify_content_values = [
        JustifyContent::FlexStart,
        JustifyContent::Center,
        JustifyContent::FlexEnd,
        JustifyContent::SpaceBetween,
        JustifyContent::SpaceAround,
        JustifyContent::SpaceEvenly,
        JustifyContent::Stretch,
    ];
    let align_items_values = [
        AlignItems::FlexStart,
        AlignItems::Center,
        AlignItems::FlexEnd,
        AlignItems::Stretch,
    ];

    let mut case_index = 0;
    for flex_direction in flex_directions {
        for direction in directions {
            for justify_content in justify_content_values {
                for align_items in align_items_values {
                    let (tree, root) =
                        flex_matrix_tree(flex_direction, direction, justify_content, align_items);
                    assert_head_to_head_or_skip(
                        case_index,
                        tree,
                        root,
                        Constraints::definite(120.0, 80.0),
                    );
                    case_index += 1;
                }
            }
        }
    }
}

#[test]
fn generated_flex_start_end_alias_matrix_matches_cpp() {
    let flex_directions = [
        FlexDirection::Row,
        FlexDirection::RowReverse,
        FlexDirection::Column,
        FlexDirection::ColumnReverse,
    ];
    let justify_content_values = [JustifyContent::Start, JustifyContent::End];
    let align_items_values = [AlignItems::Start, AlignItems::End];

    let mut case_index = 0;
    for flex_direction in flex_directions {
        for direction in LAYOUT_DIRECTIONS {
            for justify_content in justify_content_values {
                for align_items in align_items_values {
                    let (tree, root) =
                        flex_matrix_tree(flex_direction, direction, justify_content, align_items);
                    assert_head_to_head_or_skip(
                        case_index,
                        tree,
                        root,
                        Constraints::definite(120.0, 80.0),
                    );
                    case_index += 1;
                }
            }
        }
    }
}

#[test]
fn generated_wrapped_flex_gap_matrix_matches_cpp() {
    let justify_content_values = [
        JustifyContent::FlexStart,
        JustifyContent::Center,
        JustifyContent::SpaceBetween,
        JustifyContent::SpaceAround,
        JustifyContent::SpaceEvenly,
    ];
    let align_content_values = [
        AlignContent::FlexStart,
        AlignContent::Center,
        AlignContent::SpaceBetween,
        AlignContent::SpaceAround,
        AlignContent::SpaceEvenly,
    ];

    let mut case_index = 0;
    for justify_content in justify_content_values {
        for align_content in align_content_values {
            let (tree, root) = wrapped_flex_tree(justify_content, align_content);
            assert_head_to_head_or_skip(
                case_index,
                tree,
                root,
                Constraints::new(
                    SideConstraint::definite(72.0),
                    SideConstraint::definite(60.0),
                ),
            );
            case_index += 1;
        }
    }
}

#[test]
fn generated_flex_wrap_direction_alignment_matrix_matches_cpp() {
    let flex_directions = [
        FlexDirection::Row,
        FlexDirection::RowReverse,
        FlexDirection::Column,
        FlexDirection::ColumnReverse,
    ];
    let directions = LAYOUT_DIRECTIONS;
    let flex_wrap_values = [FlexWrap::Wrap, FlexWrap::WrapReverse];
    let align_content_values = [
        AlignContent::FlexStart,
        AlignContent::Start,
        AlignContent::Center,
        AlignContent::FlexEnd,
        AlignContent::End,
        AlignContent::Stretch,
    ];
    let align_items_values = [
        AlignItems::FlexStart,
        AlignItems::Center,
        AlignItems::FlexEnd,
        AlignItems::Stretch,
    ];

    let mut case_index = 0;
    for flex_direction in flex_directions {
        for direction in directions {
            for flex_wrap in flex_wrap_values {
                for align_content in align_content_values {
                    for align_items in align_items_values {
                        let (tree, root) = flex_wrap_direction_alignment_tree(
                            flex_direction,
                            direction,
                            flex_wrap,
                            align_content,
                            align_items,
                        );
                        assert_head_to_head_or_skip(
                            case_index,
                            tree,
                            root,
                            Constraints::definite(76.0, 64.0),
                        );
                        case_index += 1;
                    }
                }
            }
        }
    }
}

#[test]
fn generated_measured_callback_matrix_matches_cpp() {
    let containers = GENERATED_CONTAINERS;
    let variants = [
        MeasuredVariant::Plain,
        MeasuredVariant::Baseline,
        MeasuredVariant::MinMax,
        MeasuredVariant::AspectBorderBox,
    ];

    let mut case_index = 0;
    for container in containers {
        for variant in variants {
            let (tree, root) = measured_callback_tree(container, variant);
            assert_head_to_head_or_skip(
                case_index,
                tree,
                root,
                Constraints::definite(142.0, 104.0),
            );
            case_index += 1;
        }
    }
}

#[test]
fn generated_flex_baseline_propagation_matrix_matches_cpp() {
    let constraint_modes = [
        BaselineConstraintMode::DefiniteRoot,
        BaselineConstraintMode::AtMostOwner,
        BaselineConstraintMode::IndefiniteOwner,
    ];
    let triggers = [
        BaselineTrigger::ContainerAlignItems,
        BaselineTrigger::ChildAlignSelf,
    ];
    let sources = [
        BaselineSource::MeasuredLeaf,
        BaselineSource::NestedFlex,
        BaselineSource::NestedFlexColumn,
        BaselineSource::NestedFlexColumnReverse,
        BaselineSource::NestedLinear,
        BaselineSource::NestedLinearVertical,
        BaselineSource::NestedLinearVerticalReverse,
        BaselineSource::NestedGridFallback,
        BaselineSource::NestedRelativeFallback,
    ];

    let mut case_index = 0;
    for constraint_mode in constraint_modes {
        for trigger in triggers {
            for source in sources {
                let (tree, root, constraints) =
                    flex_baseline_propagation_tree(constraint_mode, trigger, source);
                assert_head_to_head_or_skip(case_index, tree, root, constraints);
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_grid_item_alignment_matrix_matches_cpp() {
    let directions = LAYOUT_DIRECTIONS;
    let justify_items_values = [
        JustifyItems::Auto,
        JustifyItems::Start,
        JustifyItems::Center,
        JustifyItems::End,
        JustifyItems::Stretch,
    ];
    let align_items_values = [
        AlignItems::FlexStart,
        AlignItems::Start,
        AlignItems::Center,
        AlignItems::FlexEnd,
        AlignItems::End,
        AlignItems::Stretch,
        AlignItems::Baseline,
    ];

    let mut case_index = 0;
    for direction in directions {
        for justify_items in justify_items_values {
            for align_items in align_items_values {
                let (tree, root) = grid_item_alignment_tree(direction, justify_items, align_items);
                assert_head_to_head_or_skip(
                    case_index,
                    tree,
                    root,
                    Constraints::definite(80.0, 55.0),
                );
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_grid_auto_margin_alignment_matrix_matches_cpp() {
    let directions = LAYOUT_DIRECTIONS;
    let placements = [
        GridAutoMarginPlacement::ExplicitFirstCell,
        GridAutoMarginPlacement::ExplicitSecondCell,
        GridAutoMarginPlacement::AutoPlacedSpan,
    ];
    let inline_margins = [
        GridAutoMarginPattern::None,
        GridAutoMarginPattern::Start,
        GridAutoMarginPattern::End,
        GridAutoMarginPattern::Both,
    ];
    let block_margins = [
        GridAutoMarginPattern::None,
        GridAutoMarginPattern::Start,
        GridAutoMarginPattern::End,
        GridAutoMarginPattern::Both,
    ];

    let mut case_index = 0;
    for direction in directions {
        for placement in placements {
            for inline_margin in inline_margins {
                for block_margin in block_margins {
                    let (tree, root) = grid_auto_margin_alignment_tree(
                        direction,
                        placement,
                        inline_margin,
                        block_margin,
                    );
                    assert_head_to_head_or_skip(
                        case_index,
                        tree,
                        root,
                        Constraints::definite(112.0, 74.0),
                    );
                    case_index += 1;
                }
            }
        }
    }
}

#[test]
fn generated_grid_track_sizing_matrix_matches_cpp() {
    let constraint_modes = [
        GridTrackConstraintMode::DefiniteRoot,
        GridTrackConstraintMode::IndefiniteOwner,
        GridTrackConstraintMode::AtMostOwner,
    ];
    let variants = [
        GridTrackSizingVariant::FlexibleMinMax,
        GridTrackSizingVariant::FixedMaxGrowthLimit,
        GridTrackSizingVariant::MaxContentMinimum,
        GridTrackSizingVariant::FitContentCaps,
        GridTrackSizingVariant::ImplicitAutoTracks,
        GridTrackSizingVariant::ImplicitFitContentMaxCaps,
    ];

    let mut case_index = 0;
    for constraint_mode in constraint_modes {
        for variant in variants {
            let (tree, root, constraints) = grid_track_sizing_tree(constraint_mode, variant);
            assert_head_to_head_or_skip(case_index, tree, root, constraints);
            case_index += 1;
        }
    }
}

#[test]
fn generated_grid_content_alignment_matrix_matches_cpp() {
    let justify_content_values = [
        JustifyContent::FlexStart,
        JustifyContent::Start,
        JustifyContent::Center,
        JustifyContent::FlexEnd,
        JustifyContent::End,
        JustifyContent::SpaceBetween,
        JustifyContent::SpaceAround,
        JustifyContent::SpaceEvenly,
        JustifyContent::Stretch,
    ];
    let align_content_values = [
        AlignContent::FlexStart,
        AlignContent::Start,
        AlignContent::Center,
        AlignContent::FlexEnd,
        AlignContent::End,
        AlignContent::SpaceBetween,
        AlignContent::SpaceAround,
        AlignContent::SpaceEvenly,
        AlignContent::Stretch,
    ];
    let directions = LAYOUT_DIRECTIONS;
    let size_modes = [
        GridContentSizeMode::ExtraSpace,
        GridContentSizeMode::Overflow,
    ];

    let mut case_index = 0;
    for justify_content in justify_content_values {
        for align_content in align_content_values {
            for direction in directions {
                for size_mode in size_modes {
                    let (tree, root, constraints) = grid_content_alignment_tree(
                        justify_content,
                        align_content,
                        direction,
                        size_mode,
                    );
                    assert_head_to_head_or_skip(case_index, tree, root, constraints);
                    case_index += 1;
                }
            }
        }
    }
}

#[test]
fn generated_grid_auto_flow_placement_matrix_matches_cpp() {
    let auto_flow_values = [
        GridAutoFlow::Row,
        GridAutoFlow::Column,
        GridAutoFlow::Dense,
        GridAutoFlow::RowDense,
        GridAutoFlow::ColumnDense,
    ];
    let directions = LAYOUT_DIRECTIONS;
    let patterns = [
        GridPlacementPattern::AutoSpans,
        GridPlacementPattern::LockedLines,
        GridPlacementPattern::ImplicitLines,
    ];

    let mut case_index = 0;
    for auto_flow in auto_flow_values {
        for direction in directions {
            for pattern in patterns {
                let (tree, root) = grid_auto_flow_placement_tree(auto_flow, direction, pattern);
                assert_head_to_head_or_skip(
                    case_index,
                    tree,
                    root,
                    Constraints::definite(118.0, 86.0),
                );
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_sizing_minmax_aspect_matrix_matches_cpp() {
    let containers = GENERATED_CONTAINERS;
    let variants = [
        SizingVariant::PercentCalcRoot,
        SizingVariant::FitContentRoot,
        SizingVariant::FitContentSubtree,
        SizingVariant::PercentMinMaxRoot,
        SizingVariant::BorderBoxPercentMinMaxRoot,
        SizingVariant::ContentBoxAspectRoot,
        SizingVariant::BorderBoxAspectRoot,
        SizingVariant::IntrinsicMeasuredChild,
    ];

    let mut case_index = 0;
    for container in containers {
        for variant in variants {
            let (tree, root) = sizing_minmax_aspect_tree(container, variant);
            assert_head_to_head_or_skip(
                case_index,
                tree,
                root,
                Constraints::definite(160.0, 120.0),
            );
            case_index += 1;
        }
    }
}

#[test]
fn generated_linear_orientation_justify_direction_matrix_matches_cpp() {
    let orientations = [
        LinearOrientation::Horizontal,
        LinearOrientation::HorizontalReverse,
        LinearOrientation::Vertical,
        LinearOrientation::VerticalReverse,
        LinearOrientation::Row,
        LinearOrientation::RowReverse,
        LinearOrientation::Column,
        LinearOrientation::ColumnReverse,
    ];
    let directions = LAYOUT_DIRECTIONS;
    let justify_content_values = [
        JustifyContent::FlexStart,
        JustifyContent::Center,
        JustifyContent::FlexEnd,
        JustifyContent::SpaceBetween,
        JustifyContent::SpaceAround,
        JustifyContent::SpaceEvenly,
        JustifyContent::Stretch,
    ];

    let mut case_index = 0;
    for linear_orientation in orientations {
        for direction in directions {
            for justify_content in justify_content_values {
                let (tree, root) =
                    linear_orientation_tree(linear_orientation, direction, justify_content);
                assert_head_to_head_or_skip(
                    case_index,
                    tree,
                    root,
                    Constraints::definite(120.0, 90.0),
                );
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_linear_gravity_orientation_direction_matrix_matches_cpp() {
    let orientations = [
        LinearOrientation::Horizontal,
        LinearOrientation::HorizontalReverse,
        LinearOrientation::Vertical,
        LinearOrientation::VerticalReverse,
        LinearOrientation::Row,
        LinearOrientation::RowReverse,
        LinearOrientation::Column,
        LinearOrientation::ColumnReverse,
    ];
    let linear_gravity_values = [
        LinearGravity::None,
        LinearGravity::Top,
        LinearGravity::Bottom,
        LinearGravity::Left,
        LinearGravity::Right,
        LinearGravity::CenterVertical,
        LinearGravity::CenterHorizontal,
        LinearGravity::SpaceBetween,
        LinearGravity::Start,
        LinearGravity::End,
        LinearGravity::Center,
    ];

    let mut case_index = 0;
    for linear_orientation in orientations {
        for direction in LAYOUT_DIRECTIONS {
            for linear_gravity in linear_gravity_values {
                let (tree, root) =
                    linear_gravity_tree(linear_orientation, direction, linear_gravity);
                assert_head_to_head_or_skip(
                    case_index,
                    tree,
                    root,
                    Constraints::definite(120.0, 90.0),
                );
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_linear_layout_gravity_orientation_direction_matrix_matches_cpp() {
    let orientations = [
        LinearOrientation::Horizontal,
        LinearOrientation::HorizontalReverse,
        LinearOrientation::Vertical,
        LinearOrientation::VerticalReverse,
        LinearOrientation::Row,
        LinearOrientation::RowReverse,
        LinearOrientation::Column,
        LinearOrientation::ColumnReverse,
    ];
    let linear_layout_gravity_values = [
        LinearLayoutGravity::None,
        LinearLayoutGravity::Top,
        LinearLayoutGravity::Bottom,
        LinearLayoutGravity::Left,
        LinearLayoutGravity::Right,
        LinearLayoutGravity::CenterVertical,
        LinearLayoutGravity::CenterHorizontal,
        LinearLayoutGravity::FillVertical,
        LinearLayoutGravity::FillHorizontal,
        LinearLayoutGravity::Center,
        LinearLayoutGravity::Stretch,
        LinearLayoutGravity::Start,
        LinearLayoutGravity::End,
    ];

    let mut case_index = 0;
    for linear_orientation in orientations {
        for direction in LAYOUT_DIRECTIONS {
            for linear_layout_gravity in linear_layout_gravity_values {
                let (tree, root) = linear_layout_gravity_tree(
                    linear_orientation,
                    direction,
                    linear_layout_gravity,
                );
                assert_head_to_head_or_skip(
                    case_index,
                    tree,
                    root,
                    Constraints::definite(120.0, 90.0),
                );
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_linear_cross_gravity_orientation_direction_matrix_matches_cpp() {
    let orientations = [
        LinearOrientation::Horizontal,
        LinearOrientation::HorizontalReverse,
        LinearOrientation::Vertical,
        LinearOrientation::VerticalReverse,
        LinearOrientation::Row,
        LinearOrientation::RowReverse,
        LinearOrientation::Column,
        LinearOrientation::ColumnReverse,
    ];
    let linear_cross_gravity_values = [
        LinearCrossGravity::None,
        LinearCrossGravity::Start,
        LinearCrossGravity::End,
        LinearCrossGravity::Center,
        LinearCrossGravity::Stretch,
    ];

    let mut case_index = 0;
    for linear_orientation in orientations {
        for direction in LAYOUT_DIRECTIONS {
            for linear_cross_gravity in linear_cross_gravity_values {
                let (tree, root) =
                    linear_cross_gravity_tree(linear_orientation, direction, linear_cross_gravity);
                assert_head_to_head_or_skip(
                    case_index,
                    tree,
                    root,
                    Constraints::definite(120.0, 90.0),
                );
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_linear_css_alignment_matrix_matches_cpp() {
    let orientations = [
        LinearOrientation::Horizontal,
        LinearOrientation::HorizontalReverse,
        LinearOrientation::Vertical,
        LinearOrientation::VerticalReverse,
        LinearOrientation::Row,
        LinearOrientation::RowReverse,
        LinearOrientation::Column,
        LinearOrientation::ColumnReverse,
    ];
    let align_items_values = [
        AlignItems::Stretch,
        AlignItems::FlexStart,
        AlignItems::Start,
        AlignItems::Center,
        AlignItems::FlexEnd,
        AlignItems::End,
        AlignItems::Baseline,
    ];
    let align_self_values = [
        None,
        Some(AlignItems::Stretch),
        Some(AlignItems::FlexStart),
        Some(AlignItems::Center),
        Some(AlignItems::FlexEnd),
        Some(AlignItems::Baseline),
    ];

    let mut case_index = 0;
    for linear_orientation in orientations {
        for direction in LAYOUT_DIRECTIONS {
            for align_items in align_items_values {
                for align_self in align_self_values {
                    let (tree, root) = linear_css_alignment_tree(
                        linear_orientation,
                        direction,
                        align_items,
                        align_self,
                    );
                    assert_head_to_head_or_skip(
                        case_index,
                        tree,
                        root,
                        Constraints::definite(120.0, 90.0),
                    );
                    case_index += 1;
                }
            }
        }
    }
}

#[test]
fn generated_linear_start_end_alias_matrix_matches_cpp() {
    let orientations = [
        LinearOrientation::Horizontal,
        LinearOrientation::HorizontalReverse,
        LinearOrientation::Vertical,
        LinearOrientation::VerticalReverse,
        LinearOrientation::Row,
        LinearOrientation::RowReverse,
        LinearOrientation::Column,
        LinearOrientation::ColumnReverse,
    ];
    let justify_content_values = [JustifyContent::Start, JustifyContent::End];

    let mut case_index = 0;
    for linear_orientation in orientations {
        for direction in LAYOUT_DIRECTIONS {
            for justify_content in justify_content_values {
                let (tree, root) =
                    linear_orientation_tree(linear_orientation, direction, justify_content);
                assert_head_to_head_or_skip(
                    case_index,
                    tree,
                    root,
                    Constraints::definite(120.0, 90.0),
                );
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_random_axis_length_variants_cover_fr_units() {
    let mut rng = DeterministicRng::new(0xA715);
    let values = (0..RANDOM_AXIS_LENGTH_VARIANT_COUNT)
        .map(|variant| random_axis_length_for_variant(&mut rng, variant))
        .collect::<Vec<_>>();

    assert!(values.iter().any(|value| matches!(value, Length::Auto)));
    assert!(values
        .iter()
        .any(|value| matches!(value, Length::Points(_))));
    assert!(values
        .iter()
        .any(|value| matches!(value, Length::Percent(_))));
    assert!(values.iter().any(|value| matches!(value, Length::Fr(_))));
}

#[test]
fn generated_random_minmax_variants_cover_fr_units() {
    let mut rng = DeterministicRng::new(0xA11A);
    let pairs = (0..RANDOM_MINMAX_LENGTH_VARIANT_COUNT)
        .map(|variant| random_coherent_minmax_lengths_for_variant(&mut rng, variant))
        .collect::<Vec<_>>();

    assert!(pairs
        .iter()
        .any(|(min, max)| matches!(min, Length::Fr(_)) && matches!(max, Length::Auto)));
    assert!(pairs
        .iter()
        .any(|(min, max)| matches!(min, Length::Fr(_)) && matches!(max, Length::Fr(_))));
}

#[test]
fn generated_linear_weight_gravity_constraint_matrix_matches_cpp() {
    let orientations = [
        LinearOrientation::Horizontal,
        LinearOrientation::HorizontalReverse,
        LinearOrientation::Vertical,
        LinearOrientation::VerticalReverse,
    ];
    let directions = LAYOUT_DIRECTIONS;
    let constraint_modes = [
        LinearConstraintMode::DefiniteRoot,
        LinearConstraintMode::AtMostOwner,
        LinearConstraintMode::IndefiniteOwner,
    ];
    let edge_patterns = [
        LinearEdgePattern::WeightedMinMax,
        LinearEdgePattern::WeightSumMainGravity,
        LinearEdgePattern::LayoutGravityOverride,
        LinearEdgePattern::CrossAutoMarginBaseline,
    ];

    let mut case_index = 0;
    for orientation in orientations {
        for direction in directions {
            for constraint_mode in constraint_modes {
                for edge_pattern in edge_patterns {
                    let (tree, root, constraints) = linear_edge_case_tree(
                        orientation,
                        direction,
                        constraint_mode,
                        edge_pattern,
                    );
                    assert_head_to_head_or_skip(case_index, tree, root, constraints);
                    case_index += 1;
                }
            }
        }
    }
}

#[test]
fn generated_linear_composite_feature_matrix_matches_cpp() {
    let orientations = [
        LinearOrientation::Horizontal,
        LinearOrientation::HorizontalReverse,
        LinearOrientation::Vertical,
        LinearOrientation::VerticalReverse,
    ];
    let directions = LAYOUT_DIRECTIONS;
    let constraint_modes = [
        LinearConstraintMode::DefiniteRoot,
        LinearConstraintMode::AtMostOwner,
        LinearConstraintMode::IndefiniteOwner,
    ];

    let mut case_index = 0;
    for orientation in orientations {
        for direction in directions {
            for constraint_mode in constraint_modes {
                let (tree, root, constraints) =
                    linear_composite_feature_tree(orientation, direction, constraint_mode);
                assert_head_to_head_or_skip(case_index, tree, root, constraints);
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_display_none_origin_matrix_matches_cpp() {
    let containers = GENERATED_CONTAINERS;

    for (case_index, container) in containers.into_iter().enumerate() {
        let (tree, root) = display_none_origin_tree(container);
        assert_head_to_head_or_skip(case_index, tree, root, Constraints::definite(128.0, 88.0));
    }
}

#[test]
fn generated_out_of_flow_position_matrix_matches_cpp() {
    let containers = GENERATED_CONTAINERS;
    let positions = [PositionType::Absolute, PositionType::Fixed];
    let insets = [
        OutOfFlowInset::None,
        OutOfFlowInset::Start,
        OutOfFlowInset::End,
        OutOfFlowInset::Both,
    ];

    let mut case_index = 0;
    for container in containers {
        for position in positions {
            for horizontal_inset in insets {
                for vertical_inset in insets {
                    let (tree, root) = out_of_flow_position_tree(
                        container,
                        position,
                        horizontal_inset,
                        vertical_inset,
                    );
                    assert_head_to_head_or_skip(
                        case_index,
                        tree,
                        root,
                        Constraints::definite(160.0, 120.0),
                    );
                    case_index += 1;
                }
            }
        }
    }
}

#[test]
fn generated_out_of_flow_sizing_matrix_matches_cpp() {
    let containers = GENERATED_CONTAINERS;
    let positions = [PositionType::Absolute, PositionType::Fixed];
    let variants = [
        OutOfFlowSizingVariant::PercentCalc,
        OutOfFlowSizingVariant::FillAvailable,
        OutOfFlowSizingVariant::OversizedFillAvailableMeasured,
        OutOfFlowSizingVariant::MinMaxMeasuredClamp,
        OutOfFlowSizingVariant::FitContentMeasured,
        OutOfFlowSizingVariant::AspectBorderBoxMeasured,
    ];

    let mut case_index = 0;
    for container in containers {
        for position in positions {
            for variant in variants {
                let (tree, root) = out_of_flow_sizing_tree(container, position, variant);
                assert_head_to_head_or_skip(
                    case_index,
                    tree,
                    root,
                    Constraints::definite(160.0, 120.0),
                );
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_grid_out_of_flow_area_matrix_matches_cpp() {
    let positions = [PositionType::Absolute, PositionType::Fixed];
    let directions = LAYOUT_DIRECTIONS;
    let area_patterns = [
        GridOutOfFlowAreaPattern::SingleTrack,
        GridOutOfFlowAreaPattern::SpanningTracks,
        GridOutOfFlowAreaPattern::AutoPaddingEdges,
        GridOutOfFlowAreaPattern::LastLineToAutoEnd,
    ];
    let sizing_variants = [
        GridOutOfFlowSizingVariant::FillAvailableInsets,
        GridOutOfFlowSizingVariant::MeasuredAlignment,
        GridOutOfFlowSizingVariant::FitContentSubtree,
        GridOutOfFlowSizingVariant::MeasuredFitContent,
    ];

    let mut case_index = 0;
    for position in positions {
        for direction in directions {
            for area_pattern in area_patterns {
                for sizing_variant in sizing_variants {
                    let (tree, root) = grid_out_of_flow_area_tree(
                        position,
                        direction,
                        area_pattern,
                        sizing_variant,
                    );
                    assert_head_to_head_or_skip(
                        case_index,
                        tree,
                        root,
                        Constraints::definite(132.0, 88.0),
                    );
                    case_index += 1;
                }
            }
        }
    }
}

#[test]
fn generated_fixed_descendant_matrix_matches_cpp() {
    let root_containers = [
        GeneratedContainer::Block,
        GeneratedContainer::FlexRow,
        GeneratedContainer::FlexColumnRtl,
        GeneratedContainer::LinearColumnRtl,
        GeneratedContainer::LinearRow,
        GeneratedContainer::Relative,
        GeneratedContainer::Grid,
    ];
    let nested_containers = [
        GeneratedContainer::Block,
        GeneratedContainer::FlexColumnRtl,
        GeneratedContainer::FlexRow,
        GeneratedContainer::LinearRow,
        GeneratedContainer::LinearColumnRtl,
        GeneratedContainer::Relative,
        GeneratedContainer::Grid,
    ];
    let variants = [
        FixedDescendantVariant::PercentStart,
        FixedDescendantVariant::CalcEnd,
        FixedDescendantVariant::FillAvailable,
        FixedDescendantVariant::MeasuredAspect,
        FixedDescendantVariant::FitContentSubtree,
    ];

    let mut case_index = 0;
    for root_container in root_containers {
        for nested_container in nested_containers {
            for variant in variants {
                let (tree, root) = fixed_descendant_tree(root_container, nested_container, variant);
                assert_head_to_head_or_skip(
                    case_index,
                    tree,
                    root,
                    Constraints::definite(180.0, 130.0),
                );
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_sticky_position_matrix_matches_cpp() {
    let containers = GENERATED_CONTAINERS;
    let inset_lengths = [
        StickyInsetLength::Points,
        StickyInsetLength::Percent,
        StickyInsetLength::Calc,
    ];
    let insets = [
        OutOfFlowInset::None,
        OutOfFlowInset::Start,
        OutOfFlowInset::End,
        OutOfFlowInset::Both,
    ];

    let mut case_index = 0;
    for container in containers {
        for inset_length in inset_lengths {
            for horizontal_inset in insets {
                for vertical_inset in insets {
                    let (tree, root) = sticky_position_tree(
                        container,
                        inset_length,
                        horizontal_inset,
                        vertical_inset,
                    );
                    assert_head_to_head_or_skip(
                        case_index,
                        tree,
                        root,
                        Constraints::definite(160.0, 120.0),
                    );
                    case_index += 1;
                }
            }
        }
    }
}

#[test]
fn generated_sticky_sizing_matrix_matches_cpp() {
    let containers = GENERATED_CONTAINERS;
    let variants = [
        StickySizingVariant::PercentCalc,
        StickySizingVariant::AutoMeasured,
        StickySizingVariant::MinMaxMeasuredClamp,
        StickySizingVariant::FitContentMeasured,
        StickySizingVariant::AspectBorderBoxMeasured,
    ];

    let mut case_index = 0;
    for container in containers {
        for variant in variants {
            let (tree, root) = sticky_sizing_tree(container, variant);
            assert_head_to_head_or_skip(
                case_index,
                tree,
                root,
                Constraints::definite(160.0, 120.0),
            );
            case_index += 1;
        }
    }
}

#[test]
fn generated_relative_center_parent_edge_matrix_matches_cpp() {
    let centers = [
        RelativeCenter::None,
        RelativeCenter::Horizontal,
        RelativeCenter::Vertical,
        RelativeCenter::Both,
    ];
    let parent_edges = [
        RelativeParentEdge::None,
        RelativeParentEdge::Start,
        RelativeParentEdge::End,
        RelativeParentEdge::Both,
    ];

    let mut case_index = 0;
    for center in centers {
        for horizontal_edge in parent_edges {
            for vertical_edge in parent_edges {
                let (tree, root) =
                    relative_center_parent_edge_tree(center, horizontal_edge, vertical_edge);
                assert_head_to_head_or_skip(case_index, tree, root, Constraints::indefinite());
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_relative_sibling_dependency_matrix_matches_cpp() {
    let sibling_edges = [
        RelativeSiblingEdge::After,
        RelativeSiblingEdge::Before,
        RelativeSiblingEdge::AlignStart,
        RelativeSiblingEdge::AlignEnd,
    ];
    let layout_once_values = [false, true];

    let mut case_index = 0;
    for relative_layout_once in layout_once_values {
        for horizontal_edge in sibling_edges {
            for vertical_edge in sibling_edges {
                let (tree, root) = relative_sibling_dependency_tree(
                    relative_layout_once,
                    horizontal_edge,
                    vertical_edge,
                );
                assert_head_to_head_or_skip(case_index, tree, root, Constraints::indefinite());
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_relative_missing_reference_matrix_matches_cpp() {
    let sibling_edges = [
        RelativeSiblingEdge::After,
        RelativeSiblingEdge::Before,
        RelativeSiblingEdge::AlignStart,
        RelativeSiblingEdge::AlignEnd,
    ];
    let constraint_modes = [
        RelativeConstraintMode::DefiniteRoot,
        RelativeConstraintMode::AtMostOwner,
        RelativeConstraintMode::IndefiniteOwner,
    ];
    let layout_once_values = [false, true];

    let mut case_index = 0;
    for constraint_mode in constraint_modes {
        for relative_layout_once in layout_once_values {
            for horizontal_edge in sibling_edges {
                for vertical_edge in sibling_edges {
                    let (tree, root, constraints) = relative_missing_reference_tree(
                        constraint_mode,
                        relative_layout_once,
                        horizontal_edge,
                        vertical_edge,
                    );
                    assert_head_to_head_or_skip(case_index, tree, root, constraints);
                    case_index += 1;
                }
            }
        }
    }
}

#[test]
fn generated_relative_dependency_resolution_matrix_matches_cpp() {
    let constraint_modes = [
        RelativeConstraintMode::DefiniteRoot,
        RelativeConstraintMode::AtMostOwner,
        RelativeConstraintMode::IndefiniteOwner,
    ];
    let layout_once_values = [false, true];
    let dependency_patterns = [
        RelativeDependencyPattern::DuplicatePosition,
        RelativeDependencyPattern::DisplayNoneDuplicate,
        RelativeDependencyPattern::DuplicateEdgeAlignment,
        RelativeDependencyPattern::ParentEndRecompute,
        RelativeDependencyPattern::CombinedDependencyOrder,
    ];

    let mut case_index = 0;
    for constraint_mode in constraint_modes {
        for relative_layout_once in layout_once_values {
            for dependency_pattern in dependency_patterns {
                let (tree, root, constraints) = relative_dependency_resolution_tree(
                    constraint_mode,
                    relative_layout_once,
                    dependency_pattern,
                );
                assert_head_to_head_or_skip(case_index, tree, root, constraints);
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_relative_measured_constraint_matrix_matches_cpp() {
    let layout_once_values = [false, true];
    let constraint_patterns = [
        RelativeMeasuredConstraint::ParentEnd,
        RelativeMeasuredConstraint::ParentBoth,
        RelativeMeasuredConstraint::AfterAnchor,
        RelativeMeasuredConstraint::BeforeAnchor,
        RelativeMeasuredConstraint::BetweenAnchors,
    ];
    let measure_behaviors = [
        GeneratedMeasureBehavior::HeightFromWidth {
            intrinsic_width: 70.0,
            fallback_height: 8.0,
            height_ratio: 0.25,
        },
        GeneratedMeasureBehavior::WidthByHeightMode {
            at_most_width: 74.0,
            definite_width: 31.0,
            height: 9.0,
        },
    ];

    let mut case_index = 0;
    for relative_layout_once in layout_once_values {
        for constraint_pattern in constraint_patterns {
            for measure_behavior in measure_behaviors {
                let (tree, root) = relative_measured_constraint_tree(
                    relative_layout_once,
                    constraint_pattern,
                    measure_behavior,
                );
                assert_head_to_head_or_skip(case_index, tree, root, Constraints::indefinite());
                case_index += 1;
            }
        }
    }
}

#[test]
fn generated_relative_composite_feature_matrix_matches_cpp() {
    let constraint_modes = [
        RelativeConstraintMode::DefiniteRoot,
        RelativeConstraintMode::AtMostOwner,
        RelativeConstraintMode::IndefiniteOwner,
    ];
    let layout_once_values = [false, true];

    let mut case_index = 0;
    for constraint_mode in constraint_modes {
        for relative_layout_once in layout_once_values {
            let (tree, root, constraints) =
                relative_composite_feature_tree(constraint_mode, relative_layout_once);
            assert_head_to_head_or_skip(case_index, tree, root, constraints);
            case_index += 1;
        }
    }
}

#[test]
fn generated_deterministic_supported_tree_fuzz_matches_cpp() {
    let mut rng = DeterministicRng::new(0x5A17_1A64);
    let case_filter = generated_case_filter();
    let case_count = generated_case_count(case_filter);
    for case_index in 0..case_count {
        let (tree, root, constraints) = deterministic_supported_tree(&mut rng, case_index);
        if case_filter.is_some_and(|filter| filter != case_index) {
            continue;
        }
        match run_head_to_head(tree.clone(), root, constraints, LayoutTolerance::default()) {
            Ok(_) => {}
            Err(ParityError::CppBaseline(
                CppBaselineError::NativeFeatureDisabled | CppBaselineError::NativeLinkUnavailable,
            )) => {}
            Err(error) => panic!(
                "deterministic fuzz case {case_index} failed: {error}\n{}\nconstraints={constraints:?}\n{}",
                debug_layout_snapshots(&tree, root, constraints),
                debug_tree_summary(&tree)
            ),
        }
    }
}

fn run_deterministic_supported_tree_cases(case_indices: &[usize], label: &str) {
    assert!(!case_indices.is_empty());

    let mut sorted_cases = case_indices.to_vec();
    sorted_cases.sort_unstable();
    sorted_cases.dedup();

    let mut rng = DeterministicRng::new(0x5A17_1A64);
    let max_case_index = *sorted_cases.last().expect("case list is non-empty");
    let mut next_case = 0;

    for case_index in 0..=max_case_index {
        let (tree, root, constraints) = deterministic_supported_tree(&mut rng, case_index);
        if sorted_cases.get(next_case).copied() != Some(case_index) {
            continue;
        }
        next_case += 1;

        match run_head_to_head(tree.clone(), root, constraints, LayoutTolerance::default()) {
            Ok(_) => {}
            Err(ParityError::CppBaseline(
                CppBaselineError::NativeFeatureDisabled | CppBaselineError::NativeLinkUnavailable,
            )) => {}
            Err(error) => panic!(
                "{label} case {case_index} failed: {error}\n{}\nconstraints={constraints:?}\n{}",
                debug_layout_snapshots(&tree, root, constraints),
                debug_tree_summary(&tree)
            ),
        }
    }

    assert_eq!(next_case, sorted_cases.len());
}

#[test]
fn generated_deterministic_high_case_regressions_match_cpp() {
    run_deterministic_supported_tree_cases(
        &[
            25, 26, 95, 172, 175, 215, 481, 992, 1012, 1234, 2167, 2299, 2425, 2523, 2704, 2791,
            3109, 3814, 4187, 6723, 6754, 7009, 7662, 7834, 8638, 9259, 9591, 9907, 10035, 12823,
            13868, 15500, 16328, 19719, 19993, 22474, 23012, 23362, 25535, 27453, 27673, 29021,
            29221, 29902, 31230, 34113, 41175, 42544, 44450, 45883, 51367, 54850, 56293, 64120,
            64135, 68032, 68538, 68701, 71254, 76766, 86849, 86992, 87239, 88209, 89938, 96812,
            99274, 105004, 105770, 106204, 109786, 110407, 114658, 117329, 117836, 121948, 127981,
            134513, 139357, 139979, 14505, 47159, 69145, 79192, 83434, 85507, 91679, 146179,
            149574, 160141, 161737, 161817, 164190, 164482, 165472, 166953, 176185, 176542, 176761,
            178066, 178583, 179252, 184937, 186434, 190825, 18982, 191781, 197620, 197653, 202380,
            203219, 207391, 210793, 218134, 226104, 226687, 237668, 242282, 243040, 244918, 251182,
            259483, 269542, 278605, 282829, 283687, 283842, 285802, 289600, 291152, 292360, 299934,
            299965, 302041, 302185, 307159, 308572, 309457, 310564, 316984, 318982, 319761, 320341,
            320509, 324307, 328591, 331564, 331954, 333262, 337984, 339274, 340393, 349150, 351670,
            352168, 352507, 353716, 355459, 356476, 357577, 358597, 359128, 370004, 372628, 379001,
            379945, 380056, 383395, 385732, 389959, 392284, 390103, 394393, 396763, 406621, 407422,
            411883, 411918, 413818, 422704, 428455, 429025, 433231, 434269, 435562, 437389, 441040,
            441260, 441274, 443098, 453535, 455530, 456847, 458437, 466714, 467131, 467404, 467710,
            468928, 470710, 476011, 479230, 480139, 482731, 483016, 483940, 486302, 486361, 488938,
            495574, 496681, 497539, 500887, 501562, 504658, 516046, 517072, 536077, 536206, 539452,
            540076, 540469, 540769, 541387, 545509, 549595, 549826, 559621, 559681, 562162, 562909,
            563128, 566536, 566881, 569228, 570262, 573658, 573712, 575104, 577384, 579595, 583909,
            588640, 590308, 591160, 595567, 597256, 598741, 602614, 603613, 610390, 611671, 612157,
            621826, 622414, 622600, 627868, 630196, 631288, 631777, 633370, 634267, 637210, 637504,
            642361, 646950, 652294, 653143, 655681, 655924, 656965, 659557, 661243, 663121, 663199,
            667462, 668800, 673411, 674587, 675739, 679180, 679381, 680551, 687208, 690619, 691855,
            692140, 692560, 693904, 694987, 698668, 700933, 711010, 712609, 712636, 715732, 721072,
            724985, 726547, 726901, 734488, 738802, 739687, 742114, 742318, 746173, 747619, 748012,
            751531, 761683, 762991, 763882, 763942, 764425, 764680, 772306, 776896, 10733, 2740,
            5572, 8359, 14304, 27731, 42293, 55744, 520441, 523900, 524797, 532018, 537223,
        ],
        "deterministic high-case regression",
    );
}

#[test]
fn generated_deterministic_flex_basis_cache_regressions_match_cpp() {
    let mut rng = DeterministicRng::new(0x5A17_1A64);
    for case_index in 0..DEFAULT_DETERMINISTIC_SUPPORTED_TREE_CASES {
        let (tree, root, constraints) = deterministic_supported_tree(&mut rng, case_index);
        if !matches!(case_index, 19 | 46 | 544 | 787 | 1006) {
            continue;
        }
        match run_head_to_head(tree.clone(), root, constraints, LayoutTolerance::default()) {
            Ok(_) => {}
            Err(ParityError::CppBaseline(
                CppBaselineError::NativeFeatureDisabled | CppBaselineError::NativeLinkUnavailable,
            )) => {}
            Err(error) => panic!(
                "deterministic flex-basis cache regression case {case_index} failed: {error}\n{}\nconstraints={constraints:?}\n{}",
                debug_layout_snapshots(&tree, root, constraints),
                debug_tree_summary(&tree)
            ),
        }
    }
}

#[test]
fn generated_deterministic_percentage_rounding_regressions_match_cpp() {
    let mut rng = DeterministicRng::new(0x5A17_1A64);
    for case_index in 0..DEFAULT_DETERMINISTIC_SUPPORTED_TREE_CASES {
        let (tree, root, constraints) = deterministic_supported_tree(&mut rng, case_index);
        if !matches!(case_index, 102) {
            continue;
        }
        match run_head_to_head(tree.clone(), root, constraints, LayoutTolerance::default()) {
            Ok(_) => {}
            Err(ParityError::CppBaseline(
                CppBaselineError::NativeFeatureDisabled | CppBaselineError::NativeLinkUnavailable,
            )) => {}
            Err(error) => panic!(
                "deterministic percentage rounding regression case {case_index} failed: {error}\n{}\nconstraints={constraints:?}\n{}",
                debug_layout_snapshots(&tree, root, constraints),
                debug_tree_summary(&tree)
            ),
        }
    }
}

#[test]
fn generated_deterministic_reverse_flex_bound_rounding_regressions_match_cpp() {
    let mut rng = DeterministicRng::new(0x5A17_1A64);
    for case_index in 0..DEFAULT_DETERMINISTIC_SUPPORTED_TREE_CASES {
        let (tree, root, constraints) = deterministic_supported_tree(&mut rng, case_index);
        if !matches!(case_index, 2011) {
            continue;
        }
        match run_head_to_head(tree.clone(), root, constraints, LayoutTolerance::default()) {
            Ok(_) => {}
            Err(ParityError::CppBaseline(
                CppBaselineError::NativeFeatureDisabled | CppBaselineError::NativeLinkUnavailable,
            )) => {}
            Err(error) => panic!(
                "deterministic reverse flex bound rounding regression case {case_index} failed: {error}\n{}\nconstraints={constraints:?}\n{}",
                debug_layout_snapshots(&tree, root, constraints),
                debug_tree_summary(&tree)
            ),
        }
    }
}

#[test]
fn generated_deterministic_linear_final_cross_regressions_match_cpp() {
    let mut rng = DeterministicRng::new(0x5A17_1A64);
    for case_index in 0..DEFAULT_DETERMINISTIC_SUPPORTED_TREE_CASES {
        let (tree, root, constraints) = deterministic_supported_tree(&mut rng, case_index);
        if !matches!(case_index, 5 | 9 | 39 | 48 | 63 | 83 | 223 | 989 | 2661) {
            continue;
        }
        match run_head_to_head(tree.clone(), root, constraints, LayoutTolerance::default()) {
            Ok(_) => {}
            Err(ParityError::CppBaseline(
                CppBaselineError::NativeFeatureDisabled | CppBaselineError::NativeLinkUnavailable,
            )) => {}
            Err(error) => panic!(
                "deterministic linear final cross regression case {case_index} failed: {error}\n{}\nconstraints={constraints:?}\n{}",
                debug_layout_snapshots(&tree, root, constraints),
                debug_tree_summary(&tree)
            ),
        }
    }
}

#[test]
fn generated_deterministic_padding_border_clamp_regressions_match_cpp() {
    let mut rng = DeterministicRng::new(0x5A17_1A64);
    for case_index in 0..DEFAULT_DETERMINISTIC_SUPPORTED_TREE_CASES {
        let (tree, root, constraints) = deterministic_supported_tree(&mut rng, case_index);
        if !matches!(case_index, 308) {
            continue;
        }
        match run_head_to_head(tree.clone(), root, constraints, LayoutTolerance::default()) {
            Ok(_) => {}
            Err(ParityError::CppBaseline(
                CppBaselineError::NativeFeatureDisabled | CppBaselineError::NativeLinkUnavailable,
            )) => {}
            Err(error) => panic!(
                "deterministic padding/border clamp regression case {case_index} failed: {error}\n{}\nconstraints={constraints:?}\n{}",
                debug_layout_snapshots(&tree, root, constraints),
                debug_tree_summary(&tree)
            ),
        }
    }
}

fn generated_case_filter() -> Option<usize> {
    std::env::var(ENV_GENERATED_CASE)
        .ok()
        .and_then(|value| value.parse::<usize>().ok())
}

fn generated_case_count(case_filter: Option<usize>) -> usize {
    let requested_count = generated_case_count_from_env_value(
        std::env::var(ENV_GENERATED_CASE_COUNT).ok().as_deref(),
    );
    generated_case_count_from_parts(requested_count, case_filter)
}

fn generated_case_count_from_env_value(value: Option<&str>) -> usize {
    value
        .and_then(|value| value.parse::<usize>().ok())
        .filter(|count| *count > 0)
        .unwrap_or(DEFAULT_DETERMINISTIC_SUPPORTED_TREE_CASES)
}

fn generated_case_count_from_parts(requested_count: usize, case_filter: Option<usize>) -> usize {
    case_filter.map_or(requested_count, |filter| {
        requested_count.max(filter.saturating_add(1))
    })
}

#[test]
fn generated_case_count_defaults_and_accepts_positive_env_values() {
    assert_eq!(
        generated_case_count_from_env_value(None),
        DEFAULT_DETERMINISTIC_SUPPORTED_TREE_CASES
    );
    assert_eq!(generated_case_count_from_env_value(Some("4096")), 4096);
    assert_eq!(
        generated_case_count_from_env_value(Some("0")),
        DEFAULT_DETERMINISTIC_SUPPORTED_TREE_CASES
    );
    assert_eq!(
        generated_case_count_from_env_value(Some("not-a-number")),
        DEFAULT_DETERMINISTIC_SUPPORTED_TREE_CASES
    );
}

#[test]
fn generated_case_filter_extends_case_count_to_reach_filtered_case() {
    assert_eq!(generated_case_count_from_parts(16, None), 16);
    assert_eq!(generated_case_count_from_parts(16, Some(4)), 16);
    assert_eq!(generated_case_count_from_parts(16, Some(32)), 33);
}

fn flex_matrix_tree(
    flex_direction: FlexDirection,
    direction: Direction,
    justify_content: JustifyContent,
    align_items: AlignItems,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(flex_style(Style {
        direction,
        flex_direction,
        justify_content,
        align_items,
        width: Length::points(120.0),
        height: Length::points(80.0),
        ..Style::default()
    })));

    for (index, (basis, height)) in [(18.0, 8.0), (24.0, 12.0), (30.0, 16.0)]
        .into_iter()
        .enumerate()
    {
        let child = tree.push(SimpleNode::new(flex_style(Style {
            flex_basis: Length::points(basis),
            width: Length::points(basis),
            height: if index == 1 {
                Length::Auto
            } else {
                Length::points(height)
            },
            margin: Rect::new(
                Length::points(index as f32),
                Length::points((2 - index) as f32),
                Length::ZERO,
                Length::points((index % 2) as f32),
            ),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    (tree, root)
}

fn wrapped_flex_tree(
    justify_content: JustifyContent,
    align_content: AlignContent,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(flex_style(Style {
        flex_wrap: FlexWrap::Wrap,
        justify_content,
        align_content,
        align_items: AlignItems::FlexStart,
        width: Length::points(72.0),
        height: Length::points(60.0),
        row_gap: Length::points(2.0),
        column_gap: Length::points(1.0),
        ..Style::default()
    })));

    for width in [30.0, 24.0, 28.0, 20.0] {
        let child = tree.push(SimpleNode::new(flex_style(Style {
            flex_basis: Length::points(width),
            height: Length::points(10.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    (tree, root)
}

fn flex_wrap_direction_alignment_tree(
    flex_direction: FlexDirection,
    direction: Direction,
    flex_wrap: FlexWrap,
    align_content: AlignContent,
    align_items: AlignItems,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(flex_style(Style {
        direction,
        flex_direction,
        flex_wrap,
        justify_content: JustifyContent::FlexStart,
        align_content,
        align_items,
        width: Length::points(76.0),
        height: Length::points(64.0),
        row_gap: Length::points(3.0),
        column_gap: Length::points(2.0),
        padding: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(4.0),
            Length::points(5.0),
        ),
        ..Style::default()
    })));
    let is_row = flex_direction.is_row();

    for (index, (width, height)) in [(28.0, 16.0), (34.0, 12.0), (20.0, 18.0), (25.0, 14.0)]
        .into_iter()
        .enumerate()
    {
        let child = tree.push(SimpleNode::new(flex_style(Style {
            flex_basis: Length::points(if is_row { width } else { height }),
            width: Length::points(width),
            height: Length::points(height),
            margin: Rect::new(
                Length::points((index % 2) as f32),
                Length::points((index % 3) as f32),
                Length::points((index % 2) as f32),
                Length::points(((index + 1) % 2) as f32),
            ),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    (tree, root)
}

fn measured_callback_tree(
    container: GeneratedContainer,
    variant: MeasuredVariant,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(measured_container_style(
        container, variant,
    )));

    for index in 0..2 {
        let style = measured_child_style(variant, index);
        let measured_size = measured_child_size(variant, index);
        let child = if matches!(variant, MeasuredVariant::Baseline) {
            SimpleNode::with_measured_size_and_baseline(
                style,
                measured_size,
                if index == 0 { 9.0 } else { 14.0 },
            )
        } else {
            SimpleNode::with_measured_size(style, measured_size)
        };
        let child = tree.push(child);
        tree.append_child(root, child);
    }

    (tree, root)
}

fn measured_container_style(container: GeneratedContainer, variant: MeasuredVariant) -> Style {
    let base = Style {
        width: Length::points(132.0),
        height: Length::points(84.0),
        padding: Rect::new(
            Length::points(2.0),
            Length::points(4.0),
            Length::points(3.0),
            Length::points(5.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 3.0),
        justify_content: JustifyContent::Center,
        align_items: if matches!(variant, MeasuredVariant::Baseline) {
            AlignItems::Baseline
        } else {
            AlignItems::Center
        },
        align_content: AlignContent::Stretch,
        ..Style::default()
    };

    match container {
        GeneratedContainer::Block => block_style(base),
        GeneratedContainer::FlexRow => flex_style(base),
        GeneratedContainer::FlexColumnRtl => flex_style(Style {
            direction: Direction::Rtl,
            flex_direction: FlexDirection::Column,
            ..base
        }),
        GeneratedContainer::LinearRow => linear_style(Style {
            linear_orientation: LinearOrientation::Horizontal,
            ..base
        }),
        GeneratedContainer::LinearColumnRtl => linear_style(Style {
            direction: Direction::Rtl,
            linear_orientation: LinearOrientation::Vertical,
            ..base
        }),
        GeneratedContainer::Relative => relative_style(base),
        GeneratedContainer::Grid => grid_style(Style {
            grid_template_columns: vec![Length::points(38.0), Length::points(36.0)],
            grid_template_rows: vec![Length::points(26.0), Length::points(24.0)],
            column_gap: Length::points(4.0),
            row_gap: Length::points(3.0),
            ..base
        }),
    }
}

fn measured_child_style(variant: MeasuredVariant, index: usize) -> Style {
    let mut style = block_style(Style {
        margin: Rect::new(
            Length::points((index + 1) as f32),
            Length::points((2 - index) as f32),
            Length::points((index % 2) as f32 + 1.0),
            Length::points(((index + 1) % 2) as f32 + 1.0),
        ),
        padding: Rect::new(
            Length::points(1.0),
            Length::points(index as f32 + 1.0),
            Length::points(2.0),
            Length::points(1.0),
        ),
        border: Rect::new(1.0, index as f32, 1.0, 2.0),
        ..Style::default()
    });

    match variant {
        MeasuredVariant::Plain | MeasuredVariant::Baseline => {}
        MeasuredVariant::MinMax => {
            style.min_width = Length::points(if index == 0 { 24.0 } else { 10.0 });
            style.max_width = Length::points(if index == 0 { 40.0 } else { 22.0 });
            style.min_height = Length::points(if index == 0 { 8.0 } else { 18.0 });
            style.max_height = Length::points(if index == 0 { 16.0 } else { 30.0 });
        }
        MeasuredVariant::AspectBorderBox => {
            style.box_sizing = BoxSizing::BorderBox;
            style.width = Length::points(if index == 0 { 36.0 } else { 28.0 });
            style.aspect_ratio = Some(if index == 0 { 2.0 } else { 1.25 });
        }
    }

    style
}

fn measured_child_size(variant: MeasuredVariant, index: usize) -> Size {
    match variant {
        MeasuredVariant::Plain => {
            if index == 0 {
                Size::new(21.0, 13.0)
            } else {
                Size::new(17.0, 19.0)
            }
        }
        MeasuredVariant::Baseline => {
            if index == 0 {
                Size::new(20.0, 18.0)
            } else {
                Size::new(24.0, 16.0)
            }
        }
        MeasuredVariant::MinMax => {
            if index == 0 {
                Size::new(12.0, 12.0)
            } else {
                Size::new(34.0, 12.0)
            }
        }
        MeasuredVariant::AspectBorderBox => {
            if index == 0 {
                Size::new(10.0, 40.0)
            } else {
                Size::new(30.0, 12.0)
            }
        }
    }
}

fn flex_baseline_propagation_tree(
    constraint_mode: BaselineConstraintMode,
    trigger: BaselineTrigger,
    source: BaselineSource,
) -> (SimpleTree, usize, Constraints) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(flex_baseline_root_style(
        constraint_mode,
        trigger,
    )));
    let reference = tree.push(SimpleNode::with_measured_size_and_baseline(
        block_style(Style {
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::points(2.0),
                Length::points(1.0),
            ),
            ..baseline_trigger_style(trigger)
        }),
        Size::new(11.0, 38.0),
        31.0,
    ));
    let candidate = append_baseline_source(&mut tree, source, trigger);
    let trailing = tree.push(SimpleNode::with_measured_size(
        block_style(Style {
            width: Length::points(9.0),
            height: Length::points(13.0),
            margin: Rect::new(
                Length::ZERO,
                Length::points(1.0),
                Length::points(1.0),
                Length::points(3.0),
            ),
            ..Style::default()
        }),
        Size::new(9.0, 13.0),
    ));

    for child in [reference, candidate, trailing] {
        tree.append_child(root, child);
    }

    (tree, root, flex_baseline_constraints(constraint_mode))
}

fn flex_baseline_root_style(
    constraint_mode: BaselineConstraintMode,
    trigger: BaselineTrigger,
) -> Style {
    let mut style = Style {
        flex_direction: FlexDirection::Row,
        align_items: match trigger {
            BaselineTrigger::ContainerAlignItems => AlignItems::Baseline,
            BaselineTrigger::ChildAlignSelf => AlignItems::FlexStart,
        },
        justify_content: JustifyContent::FlexStart,
        padding: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(4.0),
            Length::points(5.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        ..Style::default()
    };
    if matches!(constraint_mode, BaselineConstraintMode::DefiniteRoot) {
        style.width = Length::points(118.0);
        style.height = Length::points(76.0);
    }
    flex_style(style)
}

fn flex_baseline_constraints(constraint_mode: BaselineConstraintMode) -> Constraints {
    match constraint_mode {
        BaselineConstraintMode::DefiniteRoot => Constraints::definite(118.0, 76.0),
        BaselineConstraintMode::AtMostOwner => Constraints::new(
            SideConstraint::at_most(118.0),
            SideConstraint::at_most(76.0),
        ),
        BaselineConstraintMode::IndefiniteOwner => Constraints::indefinite(),
    }
}

fn baseline_trigger_style(trigger: BaselineTrigger) -> Style {
    Style {
        align_self: match trigger {
            BaselineTrigger::ContainerAlignItems => None,
            BaselineTrigger::ChildAlignSelf => Some(AlignItems::Baseline),
        },
        ..Style::default()
    }
}

fn append_baseline_source(
    tree: &mut SimpleTree,
    source: BaselineSource,
    trigger: BaselineTrigger,
) -> usize {
    match source {
        BaselineSource::MeasuredLeaf => tree.push(SimpleNode::with_measured_size_and_baseline(
            block_style(Style {
                margin: Rect::new(
                    Length::points(2.0),
                    Length::points(1.0),
                    Length::points(1.0),
                    Length::points(2.0),
                ),
                ..baseline_trigger_style(trigger)
            }),
            Size::new(18.0, 24.0),
            17.0,
        )),
        BaselineSource::NestedFlex => {
            let nested = tree.push(SimpleNode::new(flex_style(Style {
                align_items: AlignItems::Baseline,
                margin: Rect::new(
                    Length::points(1.0),
                    Length::points(1.0),
                    Length::points(2.0),
                    Length::points(2.0),
                ),
                ..baseline_trigger_style(trigger)
            })));
            append_nested_baseline_children(tree, nested, 6.0, 19.0);
            nested
        }
        BaselineSource::NestedFlexColumn | BaselineSource::NestedFlexColumnReverse => {
            let nested = tree.push(SimpleNode::new(flex_style(Style {
                flex_direction: match source {
                    BaselineSource::NestedFlexColumn => FlexDirection::Column,
                    BaselineSource::NestedFlexColumnReverse => FlexDirection::ColumnReverse,
                    _ => unreachable!("matched nested flex column sources"),
                },
                justify_content: JustifyContent::Center,
                width: Length::points(26.0),
                height: Length::points(48.0),
                align_items: AlignItems::FlexStart,
                margin: Rect::new(
                    Length::points(2.0),
                    Length::points(1.0),
                    Length::points(2.0),
                    Length::points(1.0),
                ),
                ..baseline_trigger_style(trigger)
            })));
            append_nested_baseline_children(tree, nested, 7.0, 16.0);
            nested
        }
        BaselineSource::NestedLinear => {
            let nested = tree.push(SimpleNode::new(linear_style(Style {
                linear_orientation: LinearOrientation::Horizontal,
                margin: Rect::new(
                    Length::points(2.0),
                    Length::points(1.0),
                    Length::points(1.0),
                    Length::points(3.0),
                ),
                ..baseline_trigger_style(trigger)
            })));
            append_nested_baseline_children(tree, nested, 8.0, 21.0);
            nested
        }
        BaselineSource::NestedLinearVertical | BaselineSource::NestedLinearVerticalReverse => {
            let nested = tree.push(SimpleNode::new(linear_style(Style {
                linear_orientation: match source {
                    BaselineSource::NestedLinearVertical => LinearOrientation::Vertical,
                    BaselineSource::NestedLinearVerticalReverse => {
                        LinearOrientation::VerticalReverse
                    }
                    _ => unreachable!("matched nested linear vertical sources"),
                },
                linear_gravity: LinearGravity::CenterVertical,
                width: Length::points(25.0),
                height: Length::points(52.0),
                margin: Rect::new(
                    Length::points(1.0),
                    Length::points(3.0),
                    Length::points(2.0),
                    Length::points(1.0),
                ),
                ..baseline_trigger_style(trigger)
            })));
            append_nested_baseline_children(tree, nested, 9.0, 18.0);
            nested
        }
        BaselineSource::NestedGridFallback => {
            let nested = tree.push(SimpleNode::new(grid_style(Style {
                width: Length::points(24.0),
                height: Length::points(18.0),
                grid_template_columns: vec![Length::points(24.0)],
                grid_template_rows: vec![Length::points(18.0)],
                align_items: AlignItems::Baseline,
                margin: Rect::new(
                    Length::points(1.0),
                    Length::points(2.0),
                    Length::points(3.0),
                    Length::points(1.0),
                ),
                ..baseline_trigger_style(trigger)
            })));
            let child = tree.push(SimpleNode::with_measured_size_and_baseline(
                block_style(Style {
                    width: Length::points(10.0),
                    height: Length::points(8.0),
                    grid_column_start: Some(1),
                    grid_row_start: Some(1),
                    ..Style::default()
                }),
                Size::new(10.0, 8.0),
                5.0,
            ));
            tree.append_child(nested, child);
            nested
        }
        BaselineSource::NestedRelativeFallback => {
            let nested = tree.push(SimpleNode::new(relative_style(Style {
                width: Length::points(22.0),
                height: Length::points(16.0),
                margin: Rect::new(
                    Length::points(2.0),
                    Length::points(2.0),
                    Length::points(2.0),
                    Length::points(1.0),
                ),
                ..baseline_trigger_style(trigger)
            })));
            let child = tree.push(SimpleNode::with_measured_size(
                block_style(Style {
                    relative_align_left: RELATIVE_ALIGN_PARENT,
                    relative_align_top: RELATIVE_ALIGN_PARENT,
                    ..Style::default()
                }),
                Size::new(12.0, 9.0),
            ));
            tree.append_child(nested, child);
            nested
        }
    }
}

fn append_nested_baseline_children(
    tree: &mut SimpleTree,
    nested: usize,
    first_baseline: f32,
    second_baseline: f32,
) {
    let first = tree.push(SimpleNode::with_measured_size_and_baseline(
        block_style(Style {
            margin: Rect::new(
                Length::points(1.0),
                Length::ZERO,
                Length::points(1.0),
                Length::points(2.0),
            ),
            ..Style::default()
        }),
        Size::new(10.0, 18.0),
        first_baseline,
    ));
    let second = tree.push(SimpleNode::with_measured_size_and_baseline(
        block_style(Style {
            margin: Rect::new(
                Length::ZERO,
                Length::points(1.0),
                Length::points(2.0),
                Length::points(1.0),
            ),
            ..Style::default()
        }),
        Size::new(12.0, 24.0),
        second_baseline,
    ));
    tree.append_child(nested, first);
    tree.append_child(nested, second);
}

fn linear_orientation_tree(
    linear_orientation: LinearOrientation,
    direction: Direction,
    justify_content: JustifyContent,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_style(Style {
        direction,
        linear_orientation,
        justify_content,
        width: Length::points(120.0),
        height: Length::points(90.0),
        padding: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(5.0),
            Length::points(7.0),
        ),
        ..Style::default()
    })));

    for (index, (width, height)) in [(16.0, 10.0), (22.0, 12.0), (18.0, 14.0)]
        .into_iter()
        .enumerate()
    {
        let child = tree.push(SimpleNode::new(block_style(Style {
            width: Length::points(width),
            height: Length::points(height),
            margin: Rect::new(
                Length::points(index as f32),
                Length::points((2 - index) as f32),
                Length::points((index % 2) as f32),
                Length::ZERO,
            ),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    (tree, root)
}

fn linear_gravity_tree(
    linear_orientation: LinearOrientation,
    direction: Direction,
    linear_gravity: LinearGravity,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_style(Style {
        direction,
        linear_orientation,
        linear_gravity,
        justify_content: JustifyContent::FlexEnd,
        width: Length::points(120.0),
        height: Length::points(90.0),
        padding: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(5.0),
            Length::points(7.0),
        ),
        ..Style::default()
    })));

    for (index, (width, height)) in [(16.0, 10.0), (22.0, 12.0), (18.0, 14.0)]
        .into_iter()
        .enumerate()
    {
        let child = tree.push(SimpleNode::new(block_style(Style {
            width: Length::points(width),
            height: Length::points(height),
            margin: Rect::new(
                Length::points(index as f32),
                Length::points((2 - index) as f32),
                Length::points((index % 2) as f32),
                Length::ZERO,
            ),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    (tree, root)
}

fn linear_layout_gravity_tree(
    linear_orientation: LinearOrientation,
    direction: Direction,
    linear_layout_gravity: LinearLayoutGravity,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_style(Style {
        direction,
        linear_orientation,
        align_items: AlignItems::Stretch,
        linear_cross_gravity: LinearCrossGravity::Center,
        width: Length::points(120.0),
        height: Length::points(90.0),
        padding: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(5.0),
            Length::points(7.0),
        ),
        ..Style::default()
    })));

    for (index, (width, height)) in [(16.0, 10.0), (22.0, 12.0), (18.0, 14.0)]
        .into_iter()
        .enumerate()
    {
        let child = tree.push(SimpleNode::new(block_style(Style {
            width: Length::points(width),
            height: Length::points(height),
            linear_layout_gravity: if index == 1 {
                linear_layout_gravity
            } else {
                LinearLayoutGravity::None
            },
            margin: Rect::new(
                Length::points(index as f32),
                Length::points((2 - index) as f32),
                Length::points((index % 2) as f32),
                Length::ZERO,
            ),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    (tree, root)
}

fn linear_cross_gravity_tree(
    linear_orientation: LinearOrientation,
    direction: Direction,
    linear_cross_gravity: LinearCrossGravity,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_style(Style {
        direction,
        linear_orientation,
        align_items: AlignItems::FlexStart,
        linear_cross_gravity,
        width: Length::points(120.0),
        height: Length::points(90.0),
        padding: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(5.0),
            Length::points(7.0),
        ),
        ..Style::default()
    })));

    for (index, (width, height)) in [(16.0, 10.0), (22.0, 12.0), (18.0, 14.0)]
        .into_iter()
        .enumerate()
    {
        let child = tree.push(SimpleNode::new(block_style(Style {
            width: Length::points(width),
            height: Length::points(height),
            margin: Rect::new(
                Length::points(index as f32),
                Length::points((2 - index) as f32),
                Length::points((index % 2) as f32),
                Length::ZERO,
            ),
            ..Style::default()
        })));
        tree.append_child(root, child);
    }

    (tree, root)
}

fn linear_css_alignment_tree(
    linear_orientation: LinearOrientation,
    direction: Direction,
    align_items: AlignItems,
    align_self: Option<AlignItems>,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(linear_style(Style {
        direction,
        linear_orientation,
        align_items,
        linear_cross_gravity: LinearCrossGravity::None,
        width: Length::points(120.0),
        height: Length::points(90.0),
        padding: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(5.0),
            Length::points(7.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        ..Style::default()
    })));

    for (index, (main, cross)) in [(16.0, 10.0), (22.0, 0.0), (18.0, 14.0)]
        .into_iter()
        .enumerate()
    {
        let child_style = linear_axis_child_style(
            linear_orientation,
            Length::points(main),
            if cross == 0.0 {
                Length::Auto
            } else {
                Length::points(cross)
            },
            Style {
                align_self: (index == 1).then_some(align_self).flatten(),
                margin: linear_cross_margin(
                    linear_orientation,
                    Length::points(index as f32),
                    Length::points((2 - index) as f32),
                ),
                padding: Rect::all(Length::points(1.0)),
                border: Rect::all(1.0),
                ..Style::default()
            },
        );
        let child = tree.push(SimpleNode::new(child_style));
        tree.append_child(root, child);
    }

    (tree, root)
}

fn linear_edge_case_tree(
    linear_orientation: LinearOrientation,
    direction: Direction,
    constraint_mode: LinearConstraintMode,
    edge_pattern: LinearEdgePattern,
) -> (SimpleTree, usize, Constraints) {
    let mut tree = SimpleTree::default();
    let (mut root_style, constraints) =
        linear_edge_root_style(linear_orientation, direction, constraint_mode);

    match edge_pattern {
        LinearEdgePattern::WeightedMinMax => {
            root_style.linear_cross_gravity = LinearCrossGravity::Stretch;
        }
        LinearEdgePattern::WeightSumMainGravity => {
            root_style.linear_gravity = LinearGravity::End;
            root_style.linear_weight_sum = 4.0;
        }
        LinearEdgePattern::LayoutGravityOverride => {
            root_style.align_items = AlignItems::Stretch;
            root_style.linear_cross_gravity = LinearCrossGravity::End;
        }
        LinearEdgePattern::CrossAutoMarginBaseline => {
            root_style.align_items = AlignItems::FlexStart;
            root_style.linear_cross_gravity = LinearCrossGravity::End;
        }
    }

    let root = tree.push(SimpleNode::new(linear_style(root_style)));
    match edge_pattern {
        LinearEdgePattern::WeightedMinMax => {
            append_linear_weighted_minmax_children(&mut tree, root, linear_orientation)
        }
        LinearEdgePattern::WeightSumMainGravity => {
            append_linear_weight_sum_children(&mut tree, root, linear_orientation)
        }
        LinearEdgePattern::LayoutGravityOverride => {
            append_linear_layout_gravity_children(&mut tree, root, linear_orientation)
        }
        LinearEdgePattern::CrossAutoMarginBaseline => {
            append_linear_auto_margin_baseline_children(&mut tree, root, linear_orientation)
        }
    }

    (tree, root, constraints)
}

fn linear_edge_root_style(
    linear_orientation: LinearOrientation,
    direction: Direction,
    constraint_mode: LinearConstraintMode,
) -> (Style, Constraints) {
    let mut style = Style {
        direction,
        linear_orientation,
        width: Length::Auto,
        height: Length::Auto,
        padding: Rect::new(
            Length::points(3.0),
            Length::points(2.0),
            Length::points(4.0),
            Length::points(1.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        ..Style::default()
    };
    let root_width = 142.0;
    let root_height = 96.0;
    let constraints = match constraint_mode {
        LinearConstraintMode::DefiniteRoot => {
            style.width = Length::points(root_width);
            style.height = Length::points(root_height);
            Constraints::definite(root_width, root_height)
        }
        LinearConstraintMode::AtMostOwner => Constraints::new(
            SideConstraint::at_most(root_width),
            SideConstraint::at_most(root_height),
        ),
        LinearConstraintMode::IndefiniteOwner => Constraints::indefinite(),
    };

    (style, constraints)
}

fn append_linear_weighted_minmax_children(
    tree: &mut SimpleTree,
    root: usize,
    linear_orientation: LinearOrientation,
) {
    let fixed = tree.push(SimpleNode::new(linear_axis_child_style(
        linear_orientation,
        Length::points(18.0),
        Length::points(13.0),
        Style {
            margin: linear_main_margin(linear_orientation, 1.0, 2.0),
            ..Style::default()
        },
    )));
    let capped = tree.push(SimpleNode::new(linear_axis_child_style(
        linear_orientation,
        Length::Auto,
        Length::Auto,
        linear_axis_min_max_style(
            linear_orientation,
            Style {
                linear_weight: 1.0,
                margin: linear_main_margin(linear_orientation, 2.0, 1.0),
                ..Style::default()
            },
            Length::Auto,
            Length::percent(30.0),
        ),
    )));
    let floored = tree.push(SimpleNode::new(linear_axis_child_style(
        linear_orientation,
        Length::Auto,
        Length::points(9.0),
        linear_axis_min_max_style(
            linear_orientation,
            Style {
                linear_weight: 2.0,
                margin: linear_main_margin(linear_orientation, 0.0, 3.0),
                ..Style::default()
            },
            Length::points(34.0),
            Length::Auto,
        ),
    )));

    for child in [fixed, capped, floored] {
        tree.append_child(root, child);
    }
}

fn append_linear_weight_sum_children(
    tree: &mut SimpleTree,
    root: usize,
    linear_orientation: LinearOrientation,
) {
    let leading = tree.push(SimpleNode::new(linear_axis_child_style(
        linear_orientation,
        Length::points(12.0),
        Length::points(11.0),
        Style {
            order: 1,
            ..Style::default()
        },
    )));
    let weighted_a = tree.push(SimpleNode::new(linear_axis_child_style(
        linear_orientation,
        Length::Auto,
        Length::points(10.0),
        Style {
            linear_weight: 1.0,
            order: 0,
            ..Style::default()
        },
    )));
    let weighted_b = tree.push(SimpleNode::new(linear_axis_child_style(
        linear_orientation,
        Length::Auto,
        Length::points(14.0),
        Style {
            linear_weight: 1.0,
            order: 2,
            ..Style::default()
        },
    )));

    for child in [leading, weighted_a, weighted_b] {
        tree.append_child(root, child);
    }
}

fn append_linear_layout_gravity_children(
    tree: &mut SimpleTree,
    root: usize,
    linear_orientation: LinearOrientation,
) {
    let start = tree.push(SimpleNode::new(linear_axis_child_style(
        linear_orientation,
        Length::points(15.0),
        Length::points(10.0),
        Style {
            linear_layout_gravity: LinearLayoutGravity::Start,
            ..Style::default()
        },
    )));
    let center = tree.push(SimpleNode::new(linear_axis_child_style(
        linear_orientation,
        Length::points(17.0),
        Length::points(12.0),
        Style {
            linear_layout_gravity: LinearLayoutGravity::Center,
            ..Style::default()
        },
    )));
    let fill = tree.push(SimpleNode::new(linear_axis_child_style(
        linear_orientation,
        Length::points(11.0),
        Length::Auto,
        Style {
            linear_layout_gravity: if linear_orientation.is_row() {
                LinearLayoutGravity::FillVertical
            } else {
                LinearLayoutGravity::FillHorizontal
            },
            margin: linear_cross_margin(
                linear_orientation,
                Length::points(1.0),
                Length::points(2.0),
            ),
            ..Style::default()
        },
    )));

    for child in [start, center, fill] {
        tree.append_child(root, child);
    }
}

fn append_linear_auto_margin_baseline_children(
    tree: &mut SimpleTree,
    root: usize,
    linear_orientation: LinearOrientation,
) {
    let measured = tree.push(SimpleNode::with_measured_size_and_baseline(
        linear_axis_child_style(
            linear_orientation,
            Length::Auto,
            Length::Auto,
            Style {
                margin: linear_cross_margin(linear_orientation, Length::Auto, Length::Auto),
                ..Style::default()
            },
        ),
        if linear_orientation.is_row() {
            Size::new(22.0, 12.0)
        } else {
            Size::new(12.0, 22.0)
        },
        5.0,
    ));
    let fixed = tree.push(SimpleNode::new(linear_axis_child_style(
        linear_orientation,
        Length::points(13.0),
        Length::points(9.0),
        Style::default(),
    )));

    for child in [measured, fixed] {
        tree.append_child(root, child);
    }
}

fn linear_composite_feature_tree(
    linear_orientation: LinearOrientation,
    direction: Direction,
    constraint_mode: LinearConstraintMode,
) -> (SimpleTree, usize, Constraints) {
    let mut tree = SimpleTree::default();
    let (mut root_style, constraints) =
        linear_edge_root_style(linear_orientation, direction, constraint_mode);
    root_style.linear_gravity = LinearGravity::SpaceBetween;
    root_style.linear_cross_gravity = LinearCrossGravity::Center;
    root_style.align_items = AlignItems::FlexStart;
    root_style.justify_content = JustifyContent::Center;
    root_style.min_width = Length::points(42.0);
    root_style.min_height = Length::points(36.0);

    let root = tree.push(SimpleNode::new(linear_style(root_style)));

    let measured = tree.push(SimpleNode::with_measured_size_and_baseline(
        linear_axis_child_style(
            linear_orientation,
            Length::Auto,
            Length::Auto,
            Style {
                order: 2,
                margin: linear_main_margin(linear_orientation, 1.0, 2.0),
                padding: Rect::all(Length::points(1.0)),
                border: Rect::all(1.0),
                ..Style::default()
            },
        ),
        if linear_orientation.is_row() {
            Size::new(21.0, 13.0)
        } else {
            Size::new(13.0, 21.0)
        },
        7.0,
    ));
    let weighted = tree.push(SimpleNode::new(linear_axis_child_style(
        linear_orientation,
        Length::Auto,
        Length::percent(42.0),
        linear_axis_min_max_style(
            linear_orientation,
            Style {
                order: 0,
                linear_weight: 1.0,
                margin: linear_main_margin(linear_orientation, 2.0, 1.0),
                padding: Rect::new(
                    Length::points(1.0),
                    Length::ZERO,
                    Length::points(2.0),
                    Length::ZERO,
                ),
                border: Rect::all(1.0),
                ..Style::default()
            },
            Length::points(18.0),
            Length::calc(16.0, 45.0),
        ),
    )));
    let aspect = tree.push(SimpleNode::new(linear_axis_child_style(
        linear_orientation,
        Length::points(24.0),
        Length::Auto,
        Style {
            order: 1,
            box_sizing: BoxSizing::BorderBox,
            aspect_ratio: Some(1.5),
            linear_layout_gravity: if linear_orientation.is_row() {
                LinearLayoutGravity::FillVertical
            } else {
                LinearLayoutGravity::FillHorizontal
            },
            margin: linear_cross_margin(
                linear_orientation,
                Length::points(1.0),
                Length::points(2.0),
            ),
            padding: Rect::all(Length::points(1.0)),
            border: Rect::all(1.0),
            ..Style::default()
        },
    )));
    let hidden = tree.push(SimpleNode::new(Style::display_none()));
    let hidden_descendant = tree.push(SimpleNode::new(block_style(Style {
        width: Length::points(19.0),
        height: Length::points(7.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_style(Style {
        position: PositionType::Absolute,
        left: Length::percent(12.0),
        right: Length::calc(3.0, 10.0),
        top: Length::points(4.0),
        bottom: Length::Auto,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 40.0))),
        height: Length::points(14.0),
        margin: Rect::all(Length::points(1.0)),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(1.0),
        ..Style::default()
    })));

    for child in [measured, weighted, aspect, hidden, absolute] {
        tree.append_child(root, child);
    }
    tree.append_child(hidden, hidden_descendant);

    (tree, root, constraints)
}

fn linear_axis_child_style(
    linear_orientation: LinearOrientation,
    main_size: Length,
    cross_size: Length,
    style: Style,
) -> Style {
    let mut style = block_style(style);
    if linear_orientation.is_row() {
        style.width = main_size;
        style.height = cross_size;
    } else {
        style.width = cross_size;
        style.height = main_size;
    }
    style
}

fn linear_axis_min_max_style(
    linear_orientation: LinearOrientation,
    mut style: Style,
    min_main: Length,
    max_main: Length,
) -> Style {
    if linear_orientation.is_row() {
        style.min_width = min_main;
        style.max_width = max_main;
    } else {
        style.min_height = min_main;
        style.max_height = max_main;
    }
    style
}

fn linear_main_margin(linear_orientation: LinearOrientation, start: f32, end: f32) -> Rect<Length> {
    if linear_orientation.is_row() {
        Rect::new(
            Length::points(start),
            Length::points(end),
            Length::ZERO,
            Length::ZERO,
        )
    } else {
        Rect::new(
            Length::ZERO,
            Length::ZERO,
            Length::points(start),
            Length::points(end),
        )
    }
}

fn linear_cross_margin(
    linear_orientation: LinearOrientation,
    start: Length,
    end: Length,
) -> Rect<Length> {
    if linear_orientation.is_row() {
        Rect::new(Length::ZERO, Length::ZERO, start, end)
    } else {
        Rect::new(start, end, Length::ZERO, Length::ZERO)
    }
}

fn display_none_origin_tree(container: GeneratedContainer) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(display_none_origin_container_style(
        container,
    )));
    let first = tree.push(SimpleNode::new(block_style(Style {
        width: Length::points(18.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    })));
    let hidden = tree.push(SimpleNode::new(Style::display_none()));
    let hidden_descendant = tree.push(SimpleNode::new(block_style(Style {
        width: Length::points(40.0),
        height: Length::points(16.0),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(block_style(Style {
        width: Length::points(14.0),
        height: Length::points(12.0),
        margin: Rect::new(
            Length::points(2.0),
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
        ),
        ..Style::default()
    })));

    tree.append_child(root, first);
    tree.append_child(root, hidden);
    tree.append_child(hidden, hidden_descendant);
    tree.append_child(root, second);

    (tree, root)
}

fn display_none_origin_container_style(container: GeneratedContainer) -> Style {
    let base = Style {
        width: Length::points(118.0),
        height: Length::points(76.0),
        padding: Rect::new(
            Length::points(3.0),
            Length::points(4.0),
            Length::points(5.0),
            Length::points(6.0),
        ),
        border: Rect::new(2.0, 3.0, 4.0, 5.0),
        align_items: AlignItems::FlexStart,
        justify_content: JustifyContent::FlexStart,
        ..Style::default()
    };

    match container {
        GeneratedContainer::Block => block_style(base),
        GeneratedContainer::FlexRow => flex_style(base),
        GeneratedContainer::FlexColumnRtl => flex_style(Style {
            direction: Direction::Rtl,
            flex_direction: FlexDirection::Column,
            ..base
        }),
        GeneratedContainer::LinearRow => linear_style(Style {
            linear_orientation: LinearOrientation::Horizontal,
            ..base
        }),
        GeneratedContainer::LinearColumnRtl => linear_style(Style {
            direction: Direction::Rtl,
            linear_orientation: LinearOrientation::Vertical,
            ..base
        }),
        GeneratedContainer::Relative => relative_style(base),
        GeneratedContainer::Grid => grid_style(Style {
            grid_template_columns: vec![Length::points(26.0), Length::points(24.0)],
            grid_template_rows: vec![Length::points(18.0), Length::points(16.0)],
            column_gap: Length::points(3.0),
            row_gap: Length::points(2.0),
            ..base
        }),
    }
}

fn out_of_flow_position_tree(
    container: GeneratedContainer,
    position: PositionType,
    horizontal_inset: OutOfFlowInset,
    vertical_inset: OutOfFlowInset,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(generated_container_style(container)));
    let out_of_flow = tree.push(SimpleNode::new(out_of_flow_child_style(
        position,
        horizontal_inset,
        vertical_inset,
    )));
    tree.append_child(root, out_of_flow);

    (tree, root)
}

fn out_of_flow_sizing_tree(
    container: GeneratedContainer,
    position: PositionType,
    variant: OutOfFlowSizingVariant,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(generated_container_style(container)));
    let out_of_flow = tree.push(out_of_flow_sizing_child_node(position, variant));
    let in_flow_sibling = tree.push(SimpleNode::new(block_style(Style {
        width: Length::points(13.0),
        height: Length::points(9.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::ZERO,
            Length::points(1.0),
        ),
        ..Style::default()
    })));
    tree.append_child(root, out_of_flow);
    tree.append_child(root, in_flow_sibling);

    (tree, root)
}

fn grid_out_of_flow_area_tree(
    position: PositionType,
    direction: Direction,
    area_pattern: GridOutOfFlowAreaPattern,
    sizing_variant: GridOutOfFlowSizingVariant,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_out_of_flow_area_root_style(direction)));
    let out_of_flow = tree.push(grid_out_of_flow_area_node(
        position,
        area_pattern,
        sizing_variant,
    ));
    let in_flow = tree.push(SimpleNode::new(block_style(Style {
        width: Length::points(12.0),
        height: Length::points(9.0),
        grid_column_start: Some(1),
        grid_row_start: Some(1),
        ..Style::default()
    })));
    tree.append_child(root, in_flow);
    tree.append_child(root, out_of_flow);

    if matches!(
        sizing_variant,
        GridOutOfFlowSizingVariant::FitContentSubtree
    ) {
        let content = tree.push(SimpleNode::new(block_style(Style {
            width: Length::points(48.0),
            height: Length::points(22.0),
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::ZERO,
                Length::points(1.0),
            ),
            ..Style::default()
        })));
        tree.append_child(out_of_flow, content);
    }

    (tree, root)
}

fn grid_out_of_flow_area_root_style(direction: Direction) -> Style {
    grid_style(Style {
        direction,
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
        justify_items: JustifyItems::Center,
        align_items: AlignItems::Center,
        ..Style::default()
    })
}

fn grid_out_of_flow_area_node(
    position: PositionType,
    area_pattern: GridOutOfFlowAreaPattern,
    sizing_variant: GridOutOfFlowSizingVariant,
) -> SimpleNode {
    let style = grid_out_of_flow_area_child_style(position, area_pattern, sizing_variant);
    match sizing_variant {
        GridOutOfFlowSizingVariant::MeasuredAlignment => {
            SimpleNode::with_measured_size(style, Size::new(11.0, 13.0))
        }
        GridOutOfFlowSizingVariant::MeasuredFitContent => {
            SimpleNode::with_measured_size(style, Size::new(50.0, 21.0))
        }
        GridOutOfFlowSizingVariant::FillAvailableInsets
        | GridOutOfFlowSizingVariant::FitContentSubtree => SimpleNode::new(style),
    }
}

fn grid_out_of_flow_area_child_style(
    position: PositionType,
    area_pattern: GridOutOfFlowAreaPattern,
    sizing_variant: GridOutOfFlowSizingVariant,
) -> Style {
    let mut style = block_style(Style {
        position,
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(2.0),
            Length::points(1.0),
        ),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(1.0),
        ..Style::default()
    });

    apply_grid_out_of_flow_area_pattern(&mut style, area_pattern);
    apply_grid_out_of_flow_sizing_variant(&mut style, sizing_variant);
    style
}

fn apply_grid_out_of_flow_area_pattern(style: &mut Style, area_pattern: GridOutOfFlowAreaPattern) {
    match area_pattern {
        GridOutOfFlowAreaPattern::SingleTrack => {
            style.grid_column_start = Some(1);
            style.grid_column_end = Some(2);
            style.grid_row_start = Some(1);
            style.grid_row_end = Some(2);
        }
        GridOutOfFlowAreaPattern::SpanningTracks => {
            style.grid_column_start = Some(2);
            style.grid_column_end = Some(3);
            style.grid_row_start = Some(1);
            style.grid_row_end = Some(3);
        }
        GridOutOfFlowAreaPattern::AutoPaddingEdges => {}
        GridOutOfFlowAreaPattern::LastLineToAutoEnd => {
            style.grid_column_start = Some(3);
            style.grid_row_start = Some(2);
        }
    }
}

fn apply_grid_out_of_flow_sizing_variant(
    style: &mut Style,
    sizing_variant: GridOutOfFlowSizingVariant,
) {
    match sizing_variant {
        GridOutOfFlowSizingVariant::FillAvailableInsets => {
            style.width = Length::Auto;
            style.height = Length::Auto;
            style.left = Length::percent(10.0);
            style.right = Length::calc(2.0, 15.0);
            style.top = Length::calc(1.0, 20.0);
            style.bottom = Length::percent(12.0);
        }
        GridOutOfFlowSizingVariant::MeasuredAlignment => {
            style.width = Length::Auto;
            style.height = Length::Auto;
            style.justify_self = JustifyItems::End;
            style.align_self = Some(AlignItems::Center);
        }
        GridOutOfFlowSizingVariant::FitContentSubtree => {
            style.width = Length::fit_content(Some(BaseLength::fixed(36.0)));
            style.height = Length::fit_content(Some(BaseLength::fixed(16.0)));
            style.justify_self = JustifyItems::Center;
            style.align_self = Some(AlignItems::Center);
        }
        GridOutOfFlowSizingVariant::MeasuredFitContent => {
            style.width = Length::fit_content(Some(BaseLength::fixed_and_percent(3.0, 50.0)));
            style.height = Length::fit_content(Some(BaseLength::fixed_and_percent(2.0, 40.0)));
            style.right = Length::percent(8.0);
            style.bottom = Length::calc(1.0, 10.0);
        }
    }
}

fn fixed_descendant_tree(
    root_container: GeneratedContainer,
    nested_container: GeneratedContainer,
    variant: FixedDescendantVariant,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(generated_container_style(root_container)));
    let nested = tree.push(SimpleNode::new(fixed_descendant_nested_style(
        nested_container,
    )));
    let wrapper = tree.push(SimpleNode::new(block_style(Style {
        width: Length::points(27.0),
        height: Length::points(22.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(1.0),
            Length::points(2.0),
        ),
        padding: Rect::all(Length::points(1.0)),
        ..Style::default()
    })));
    let fixed = tree.push(fixed_descendant_node(variant));
    let fixed_child = tree.push(SimpleNode::new(block_style(Style {
        width: Length::points(54.0),
        height: Length::points(18.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    })));
    let nested_sibling = tree.push(SimpleNode::new(block_style(Style {
        width: Length::points(16.0),
        height: Length::points(12.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(3.0),
            Length::points(1.0),
        ),
        ..Style::default()
    })));
    let root_sibling = tree.push(SimpleNode::new(block_style(Style {
        width: Length::points(18.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::points(2.0),
            Length::points(1.0),
            Length::ZERO,
            Length::points(2.0),
        ),
        ..Style::default()
    })));

    tree.append_child(root, nested);
    tree.append_child(root, root_sibling);
    tree.append_child(nested, wrapper);
    tree.append_child(nested, nested_sibling);
    tree.append_child(wrapper, fixed);
    if !matches!(variant, FixedDescendantVariant::MeasuredAspect) {
        tree.append_child(fixed, fixed_child);
    }

    (tree, root)
}

fn fixed_descendant_nested_style(container: GeneratedContainer) -> Style {
    let base = Style {
        width: Length::points(58.0),
        height: Length::points(42.0),
        padding: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(4.0),
            Length::points(5.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        justify_content: JustifyContent::FlexStart,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    };

    match container {
        GeneratedContainer::Block => block_style(base),
        GeneratedContainer::FlexRow => flex_style(base),
        GeneratedContainer::FlexColumnRtl => flex_style(Style {
            direction: Direction::Rtl,
            flex_direction: FlexDirection::Column,
            ..base
        }),
        GeneratedContainer::LinearRow => linear_style(Style {
            linear_orientation: LinearOrientation::Horizontal,
            ..base
        }),
        GeneratedContainer::LinearColumnRtl => linear_style(Style {
            direction: Direction::Rtl,
            linear_orientation: LinearOrientation::Vertical,
            ..base
        }),
        GeneratedContainer::Relative => relative_style(base),
        GeneratedContainer::Grid => grid_style(Style {
            grid_template_columns: vec![Length::points(24.0), Length::points(20.0)],
            grid_template_rows: vec![Length::points(16.0), Length::points(18.0)],
            column_gap: Length::points(3.0),
            row_gap: Length::points(2.0),
            ..base
        }),
    }
}

fn sticky_position_tree(
    container: GeneratedContainer,
    inset_length: StickyInsetLength,
    horizontal_inset: OutOfFlowInset,
    vertical_inset: OutOfFlowInset,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(generated_container_style(container)));
    let sticky = tree.push(SimpleNode::new(sticky_child_style(
        inset_length,
        horizontal_inset,
        vertical_inset,
    )));
    let follower = tree.push(SimpleNode::new(block_style(Style {
        width: Length::points(11.0),
        height: Length::points(9.0),
        margin: Rect::new(
            Length::points(2.0),
            Length::points(1.0),
            Length::points(3.0),
            Length::points(4.0),
        ),
        ..Style::default()
    })));
    tree.append_child(root, sticky);
    tree.append_child(root, follower);

    (tree, root)
}

fn sticky_sizing_tree(
    container: GeneratedContainer,
    variant: StickySizingVariant,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(generated_container_style(container)));
    let sticky = tree.push(sticky_sizing_child_node(variant));
    let follower = tree.push(SimpleNode::new(block_style(Style {
        width: Length::points(13.0),
        height: Length::points(10.0),
        margin: Rect::new(
            Length::points(2.0),
            Length::points(1.0),
            Length::points(3.0),
            Length::points(2.0),
        ),
        ..Style::default()
    })));
    tree.append_child(root, sticky);
    tree.append_child(root, follower);

    (tree, root)
}

fn generated_container_style(container: GeneratedContainer) -> Style {
    let base = Style {
        width: Length::points(126.0),
        height: Length::points(92.0),
        padding: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(7.0),
            Length::points(11.0),
        ),
        border: Rect::new(1.0, 2.0, 3.0, 4.0),
        justify_content: JustifyContent::Center,
        align_items: AlignItems::FlexEnd,
        ..Style::default()
    };

    match container {
        GeneratedContainer::Block => block_style(base),
        GeneratedContainer::FlexRow => flex_style(base),
        GeneratedContainer::FlexColumnRtl => flex_style(Style {
            direction: Direction::Rtl,
            flex_direction: FlexDirection::Column,
            flex_wrap: FlexWrap::WrapReverse,
            ..base
        }),
        GeneratedContainer::LinearRow => linear_style(Style {
            linear_orientation: LinearOrientation::Horizontal,
            ..base
        }),
        GeneratedContainer::LinearColumnRtl => linear_style(Style {
            direction: Direction::Rtl,
            linear_orientation: LinearOrientation::Vertical,
            ..base
        }),
        GeneratedContainer::Relative => relative_style(base),
        GeneratedContainer::Grid => grid_style(Style {
            grid_template_columns: vec![Length::points(50.0), Length::points(40.0)],
            grid_template_rows: vec![Length::points(30.0), Length::points(28.0)],
            column_gap: Length::points(4.0),
            row_gap: Length::points(6.0),
            ..base
        }),
    }
}

fn out_of_flow_child_style(
    position: PositionType,
    horizontal_inset: OutOfFlowInset,
    vertical_inset: OutOfFlowInset,
) -> Style {
    let mut style = block_style(Style {
        position,
        width: if matches!(horizontal_inset, OutOfFlowInset::Both) {
            Length::Auto
        } else {
            Length::points(18.0)
        },
        height: if matches!(vertical_inset, OutOfFlowInset::Both) {
            Length::Auto
        } else {
            Length::points(12.0)
        },
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(3.0),
            Length::points(4.0),
        ),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(1.0),
        ..Style::default()
    });
    apply_horizontal_inset(&mut style, horizontal_inset);
    apply_vertical_inset(&mut style, vertical_inset);
    style
}

fn apply_horizontal_inset(style: &mut Style, inset: OutOfFlowInset) {
    match inset {
        OutOfFlowInset::None => {}
        OutOfFlowInset::Start => style.left = Length::points(9.0),
        OutOfFlowInset::End => style.right = Length::points(13.0),
        OutOfFlowInset::Both => {
            style.left = Length::points(9.0);
            style.right = Length::points(13.0);
        }
    }
}

fn apply_vertical_inset(style: &mut Style, inset: OutOfFlowInset) {
    match inset {
        OutOfFlowInset::None => {}
        OutOfFlowInset::Start => style.top = Length::points(7.0),
        OutOfFlowInset::End => style.bottom = Length::points(11.0),
        OutOfFlowInset::Both => {
            style.top = Length::points(7.0);
            style.bottom = Length::points(11.0);
        }
    }
}

fn out_of_flow_sizing_child_node(
    position: PositionType,
    variant: OutOfFlowSizingVariant,
) -> SimpleNode {
    let style = out_of_flow_sizing_child_style(position, variant);
    match variant {
        OutOfFlowSizingVariant::MinMaxMeasuredClamp => {
            SimpleNode::with_measured_size(style, Size::new(80.0, 10.0))
        }
        OutOfFlowSizingVariant::FitContentMeasured => {
            SimpleNode::with_measured_size(style, Size::new(92.0, 64.0))
        }
        OutOfFlowSizingVariant::AspectBorderBoxMeasured => {
            SimpleNode::with_measured_size(style, Size::new(50.0, 18.0))
        }
        OutOfFlowSizingVariant::OversizedFillAvailableMeasured => {
            SimpleNode::with_measure_func(style, generated_width_mode_sensitive_height_measure)
        }
        OutOfFlowSizingVariant::PercentCalc | OutOfFlowSizingVariant::FillAvailable => {
            SimpleNode::new(style)
        }
    }
}

fn generated_width_mode_sensitive_height_measure(constraints: Constraints) -> Size {
    let height = if constraints.width.is_definite() {
        17.0
    } else if constraints.width.is_at_most() {
        31.0
    } else {
        43.0
    };
    Size::new(11.0, height)
}

fn out_of_flow_sizing_child_style(
    position: PositionType,
    variant: OutOfFlowSizingVariant,
) -> Style {
    let mut style = block_style(Style {
        position,
        width: Length::points(18.0),
        height: Length::points(12.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(3.0),
            Length::points(4.0),
        ),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(1.0),
        ..Style::default()
    });

    match variant {
        OutOfFlowSizingVariant::PercentCalc => {
            style.width = Length::calc(8.0, 45.0);
            style.height = Length::percent(40.0);
            style.left = Length::percent(10.0);
            style.top = Length::calc(2.0, 15.0);
        }
        OutOfFlowSizingVariant::FillAvailable => {
            style.width = Length::Auto;
            style.height = Length::Auto;
            style.left = Length::percent(10.0);
            style.right = Length::calc(3.0, 20.0);
            style.top = Length::calc(2.0, 15.0);
            style.bottom = Length::percent(25.0);
        }
        OutOfFlowSizingVariant::OversizedFillAvailableMeasured => {
            style.width = Length::Auto;
            style.height = Length::Auto;
            style.left = Length::percent(90.0);
            style.right = Length::calc(30.0, 80.0);
            style.top = Length::points(5.0);
        }
        OutOfFlowSizingVariant::MinMaxMeasuredClamp => {
            style.width = Length::Auto;
            style.height = Length::Auto;
            style.min_width = Length::percent(30.0);
            style.max_width = Length::calc(10.0, 40.0);
            style.min_height = Length::calc(4.0, 20.0);
            style.max_height = Length::percent(65.0);
            style.left = Length::points(7.0);
            style.top = Length::points(5.0);
        }
        OutOfFlowSizingVariant::FitContentMeasured => {
            style.width = Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 50.0)));
            style.height = Length::fit_content(Some(BaseLength::fixed_and_percent(4.0, 45.0)));
            style.right = Length::percent(12.0);
            style.bottom = Length::calc(1.0, 18.0);
        }
        OutOfFlowSizingVariant::AspectBorderBoxMeasured => {
            style.box_sizing = BoxSizing::BorderBox;
            style.width = Length::percent(42.0);
            style.height = Length::Auto;
            style.aspect_ratio = Some(1.6);
            style.left = Length::calc(3.0, 8.0);
            style.top = Length::percent(10.0);
        }
    }

    style
}

fn fixed_descendant_node(variant: FixedDescendantVariant) -> SimpleNode {
    let style = fixed_descendant_style(variant);
    match variant {
        FixedDescendantVariant::MeasuredAspect => {
            SimpleNode::with_measured_size(style, Size::new(72.0, 31.0))
        }
        FixedDescendantVariant::PercentStart
        | FixedDescendantVariant::CalcEnd
        | FixedDescendantVariant::FillAvailable
        | FixedDescendantVariant::FitContentSubtree => SimpleNode::new(style),
    }
}

fn fixed_descendant_style(variant: FixedDescendantVariant) -> Style {
    let mut style = block_style(Style {
        position: PositionType::Fixed,
        width: Length::points(18.0),
        height: Length::points(12.0),
        margin: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(1.0),
            Length::points(4.0),
        ),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(1.0),
        ..Style::default()
    });

    match variant {
        FixedDescendantVariant::PercentStart => {
            style.width = Length::percent(32.0);
            style.height = Length::percent(28.0);
            style.left = Length::percent(10.0);
            style.top = Length::percent(15.0);
        }
        FixedDescendantVariant::CalcEnd => {
            style.right = Length::calc(4.0, 7.0);
            style.bottom = Length::calc(3.0, 11.0);
        }
        FixedDescendantVariant::FillAvailable => {
            style.width = Length::Auto;
            style.height = Length::Auto;
            style.left = Length::percent(8.0);
            style.right = Length::calc(3.0, 12.0);
            style.top = Length::calc(2.0, 10.0);
            style.bottom = Length::percent(18.0);
        }
        FixedDescendantVariant::MeasuredAspect => {
            style.box_sizing = BoxSizing::BorderBox;
            style.width = Length::percent(40.0);
            style.height = Length::Auto;
            style.aspect_ratio = Some(2.0);
            style.left = Length::points(9.0);
            style.top = Length::points(6.0);
        }
        FixedDescendantVariant::FitContentSubtree => {
            style.width = Length::fit_content(Some(BaseLength::fixed(60.0)));
            style.height = Length::fit_content(Some(BaseLength::fixed(20.0)));
            style.left = Length::points(7.0);
            style.top = Length::points(9.0);
        }
    }

    style
}

fn sticky_child_style(
    inset_length: StickyInsetLength,
    horizontal_inset: OutOfFlowInset,
    vertical_inset: OutOfFlowInset,
) -> Style {
    let mut style = block_style(Style {
        position: PositionType::Sticky,
        width: Length::points(18.0),
        height: Length::points(12.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(3.0),
            Length::points(4.0),
        ),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(1.0),
        ..Style::default()
    });
    apply_horizontal_sticky_inset(&mut style, inset_length, horizontal_inset);
    apply_vertical_sticky_inset(&mut style, inset_length, vertical_inset);
    style
}

fn sticky_sizing_child_node(variant: StickySizingVariant) -> SimpleNode {
    let style = sticky_sizing_child_style(variant);
    match variant {
        StickySizingVariant::AutoMeasured => {
            SimpleNode::with_measured_size(style, Size::new(24.0, 16.0))
        }
        StickySizingVariant::MinMaxMeasuredClamp => {
            SimpleNode::with_measured_size(style, Size::new(80.0, 9.0))
        }
        StickySizingVariant::FitContentMeasured => {
            SimpleNode::with_measured_size(style, Size::new(74.0, 36.0))
        }
        StickySizingVariant::AspectBorderBoxMeasured => {
            SimpleNode::with_measured_size(style, Size::new(44.0, 18.0))
        }
        StickySizingVariant::PercentCalc => SimpleNode::new(style),
    }
}

fn sticky_sizing_child_style(variant: StickySizingVariant) -> Style {
    let mut style = block_style(Style {
        position: PositionType::Sticky,
        width: Length::points(18.0),
        height: Length::points(12.0),
        left: Length::calc(3.0, 10.0),
        top: Length::percent(20.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(2.0),
            Length::points(1.0),
        ),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(1.0),
        ..Style::default()
    });

    match variant {
        StickySizingVariant::PercentCalc => {
            style.width = Length::calc(8.0, 35.0);
            style.height = Length::percent(32.0);
            style.right = Length::percent(12.0);
            style.bottom = Length::calc(1.0, 15.0);
        }
        StickySizingVariant::AutoMeasured => {
            style.width = Length::Auto;
            style.height = Length::Auto;
            style.right = Length::points(5.0);
            style.bottom = Length::points(4.0);
        }
        StickySizingVariant::MinMaxMeasuredClamp => {
            style.width = Length::Auto;
            style.height = Length::Auto;
            style.min_width = Length::percent(25.0);
            style.max_width = Length::calc(8.0, 35.0);
            style.min_height = Length::calc(3.0, 18.0);
            style.max_height = Length::percent(70.0);
        }
        StickySizingVariant::FitContentMeasured => {
            style.width = Length::fit_content(Some(BaseLength::fixed_and_percent(6.0, 45.0)));
            style.height = Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 35.0)));
            style.right = Length::percent(15.0);
            style.bottom = Length::calc(2.0, 12.0);
        }
        StickySizingVariant::AspectBorderBoxMeasured => {
            style.box_sizing = BoxSizing::BorderBox;
            style.width = Length::percent(38.0);
            style.height = Length::Auto;
            style.aspect_ratio = Some(1.5);
        }
    }

    style
}

fn apply_horizontal_sticky_inset(
    style: &mut Style,
    inset_length: StickyInsetLength,
    inset: OutOfFlowInset,
) {
    match inset {
        OutOfFlowInset::None => {}
        OutOfFlowInset::Start => {
            style.left = sticky_inset_length(inset_length, 6.0, 10.0, 3.0, 10.0)
        }
        OutOfFlowInset::End => style.right = sticky_inset_length(inset_length, 8.0, 20.0, 4.0, 5.0),
        OutOfFlowInset::Both => {
            style.left = sticky_inset_length(inset_length, 6.0, 10.0, 3.0, 10.0);
            style.right = sticky_inset_length(inset_length, 8.0, 20.0, 4.0, 5.0);
        }
    }
}

fn apply_vertical_sticky_inset(
    style: &mut Style,
    inset_length: StickyInsetLength,
    inset: OutOfFlowInset,
) {
    match inset {
        OutOfFlowInset::None => {}
        OutOfFlowInset::Start => {
            style.top = sticky_inset_length(inset_length, 7.0, 25.0, 2.0, 25.0)
        }
        OutOfFlowInset::End => {
            style.bottom = sticky_inset_length(inset_length, 11.0, 50.0, 1.0, 50.0)
        }
        OutOfFlowInset::Both => {
            style.top = sticky_inset_length(inset_length, 7.0, 25.0, 2.0, 25.0);
            style.bottom = sticky_inset_length(inset_length, 11.0, 50.0, 1.0, 50.0);
        }
    }
}

fn sticky_inset_length(
    inset_length: StickyInsetLength,
    points: f32,
    percent: f32,
    calc_fixed: f32,
    calc_percent: f32,
) -> Length {
    match inset_length {
        StickyInsetLength::Points => Length::points(points),
        StickyInsetLength::Percent => Length::percent(percent),
        StickyInsetLength::Calc => Length::calc(calc_fixed, calc_percent),
    }
}

fn relative_center_parent_edge_tree(
    relative_center: RelativeCenter,
    horizontal_edge: RelativeParentEdge,
    vertical_edge: RelativeParentEdge,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_style(Style {
        width: Length::points(110.0),
        height: Length::points(90.0),
        padding: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(7.0),
            Length::points(11.0),
        ),
        ..Style::default()
    })));
    let child = tree.push(SimpleNode::with_measured_size(
        block_style(Style {
            relative_center,
            margin: Rect::new(
                Length::points(2.0),
                Length::points(3.0),
                Length::points(4.0),
                Length::points(5.0),
            ),
            ..relative_parent_edge_style(horizontal_edge, vertical_edge)
        }),
        Size::new(20.0, 12.0),
    ));
    tree.append_child(root, child);

    (tree, root)
}

fn relative_parent_edge_style(
    horizontal_edge: RelativeParentEdge,
    vertical_edge: RelativeParentEdge,
) -> Style {
    let mut style = Style::default();
    match horizontal_edge {
        RelativeParentEdge::None => {}
        RelativeParentEdge::Start => style.relative_align_left = RELATIVE_ALIGN_PARENT,
        RelativeParentEdge::End => style.relative_align_right = RELATIVE_ALIGN_PARENT,
        RelativeParentEdge::Both => {
            style.relative_align_left = RELATIVE_ALIGN_PARENT;
            style.relative_align_right = RELATIVE_ALIGN_PARENT;
        }
    }
    match vertical_edge {
        RelativeParentEdge::None => {}
        RelativeParentEdge::Start => style.relative_align_top = RELATIVE_ALIGN_PARENT,
        RelativeParentEdge::End => style.relative_align_bottom = RELATIVE_ALIGN_PARENT,
        RelativeParentEdge::Both => {
            style.relative_align_top = RELATIVE_ALIGN_PARENT;
            style.relative_align_bottom = RELATIVE_ALIGN_PARENT;
        }
    }
    style
}

fn relative_sibling_dependency_tree(
    relative_layout_once: bool,
    horizontal_edge: RelativeSiblingEdge,
    vertical_edge: RelativeSiblingEdge,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_style(Style {
        width: Length::points(120.0),
        height: Length::points(85.0),
        relative_layout_once,
        ..Style::default()
    })));
    let follower = tree.push(SimpleNode::with_measured_size(
        block_style(relative_sibling_edge_style(horizontal_edge, vertical_edge)),
        Size::new(11.0, 9.0),
    ));
    let anchor = tree.push(SimpleNode::with_measured_size(
        block_style(Style {
            relative_id: 7,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            margin: Rect::new(
                Length::points(2.0),
                Length::points(4.0),
                Length::points(3.0),
                Length::points(1.0),
            ),
            ..Style::default()
        }),
        Size::new(24.0, 16.0),
    ));
    tree.append_child(root, follower);
    tree.append_child(root, anchor);

    (tree, root)
}

fn relative_sibling_edge_style(
    horizontal_edge: RelativeSiblingEdge,
    vertical_edge: RelativeSiblingEdge,
) -> Style {
    let mut style = Style {
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(3.0),
            Length::points(4.0),
        ),
        ..Style::default()
    };
    match horizontal_edge {
        RelativeSiblingEdge::After => style.relative_right_of = 7,
        RelativeSiblingEdge::Before => style.relative_left_of = 7,
        RelativeSiblingEdge::AlignStart => style.relative_align_left = 7,
        RelativeSiblingEdge::AlignEnd => style.relative_align_right = 7,
    }
    match vertical_edge {
        RelativeSiblingEdge::After => style.relative_bottom_of = 7,
        RelativeSiblingEdge::Before => style.relative_top_of = 7,
        RelativeSiblingEdge::AlignStart => style.relative_align_top = 7,
        RelativeSiblingEdge::AlignEnd => style.relative_align_bottom = 7,
    }
    style
}

fn relative_missing_reference_tree(
    constraint_mode: RelativeConstraintMode,
    relative_layout_once: bool,
    horizontal_edge: RelativeSiblingEdge,
    vertical_edge: RelativeSiblingEdge,
) -> (SimpleTree, usize, Constraints) {
    const MISSING_RELATIVE_ID: i32 = 404;

    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_dependency_root_style(
        constraint_mode,
        relative_layout_once,
    )));
    let anchor = tree.push(SimpleNode::with_measured_size(
        block_style(Style {
            relative_id: 7,
            relative_align_left: RELATIVE_ALIGN_PARENT,
            relative_align_top: RELATIVE_ALIGN_PARENT,
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::points(1.0),
                Length::points(2.0),
            ),
            ..Style::default()
        }),
        Size::new(18.0, 12.0),
    ));
    let mut missing_reference_style = relative_sibling_edge_style(horizontal_edge, vertical_edge);
    replace_relative_id(
        &mut missing_reference_style.relative_left_of,
        MISSING_RELATIVE_ID,
    );
    replace_relative_id(
        &mut missing_reference_style.relative_right_of,
        MISSING_RELATIVE_ID,
    );
    replace_relative_id(
        &mut missing_reference_style.relative_top_of,
        MISSING_RELATIVE_ID,
    );
    replace_relative_id(
        &mut missing_reference_style.relative_bottom_of,
        MISSING_RELATIVE_ID,
    );
    replace_relative_id(
        &mut missing_reference_style.relative_align_left,
        MISSING_RELATIVE_ID,
    );
    replace_relative_id(
        &mut missing_reference_style.relative_align_right,
        MISSING_RELATIVE_ID,
    );
    replace_relative_id(
        &mut missing_reference_style.relative_align_top,
        MISSING_RELATIVE_ID,
    );
    replace_relative_id(
        &mut missing_reference_style.relative_align_bottom,
        MISSING_RELATIVE_ID,
    );
    let follower = tree.push(SimpleNode::with_measured_size(
        block_style(Style {
            order: -1,
            padding: Rect::all(Length::points(1.0)),
            border: Rect::all(1.0),
            ..missing_reference_style
        }),
        Size::new(13.0, 9.0),
    ));
    let parent_aligned = tree.push(SimpleNode::with_measured_size(
        block_style(Style {
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            margin: Rect::all(Length::points(1.0)),
            ..Style::default()
        }),
        Size::new(11.0, 7.0),
    ));

    for child in [anchor, follower, parent_aligned] {
        tree.append_child(root, child);
    }

    (tree, root, relative_dependency_constraints(constraint_mode))
}

fn replace_relative_id(value: &mut i32, replacement: i32) {
    if *value == 7 {
        *value = replacement;
    }
}

fn relative_dependency_resolution_tree(
    constraint_mode: RelativeConstraintMode,
    relative_layout_once: bool,
    dependency_pattern: RelativeDependencyPattern,
) -> (SimpleTree, usize, Constraints) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(relative_dependency_root_style(
        constraint_mode,
        relative_layout_once,
    )));
    append_relative_dependency_children(&mut tree, root, dependency_pattern);
    (tree, root, relative_dependency_constraints(constraint_mode))
}

fn relative_dependency_constraints(constraint_mode: RelativeConstraintMode) -> Constraints {
    match constraint_mode {
        RelativeConstraintMode::DefiniteRoot => Constraints::definite(132.0, 92.0),
        RelativeConstraintMode::AtMostOwner => Constraints::new(
            SideConstraint::at_most(132.0),
            SideConstraint::at_most(92.0),
        ),
        RelativeConstraintMode::IndefiniteOwner => Constraints::indefinite(),
    }
}

fn relative_dependency_root_style(
    constraint_mode: RelativeConstraintMode,
    relative_layout_once: bool,
) -> Style {
    let mut style = Style {
        relative_layout_once,
        padding: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(4.0),
            Length::points(5.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        ..Style::default()
    };
    if matches!(constraint_mode, RelativeConstraintMode::DefiniteRoot) {
        style.width = Length::points(132.0);
        style.height = Length::points(92.0);
    }
    relative_style(style)
}

fn append_relative_dependency_children(
    tree: &mut SimpleTree,
    root: usize,
    dependency_pattern: RelativeDependencyPattern,
) {
    match dependency_pattern {
        RelativeDependencyPattern::DuplicatePosition => {
            let first_anchor = tree.push(SimpleNode::with_measured_size(
                block_style(relative_anchor_style(10, 1.0, 2.0)),
                Size::new(42.0, 24.0),
            ));
            let follower = tree.push(SimpleNode::with_measured_size(
                block_style(Style {
                    relative_right_of: 10,
                    relative_bottom_of: 10,
                    margin: Rect::new(
                        Length::points(1.0),
                        Length::points(2.0),
                        Length::points(1.0),
                        Length::ZERO,
                    ),
                    ..Style::default()
                }),
                Size::new(8.0, 6.0),
            ));
            let last_anchor = tree.push(SimpleNode::with_measured_size(
                block_style(relative_anchor_style(10, 0.0, 1.0)),
                Size::new(18.0, 11.0),
            ));
            for child in [first_anchor, follower, last_anchor] {
                tree.append_child(root, child);
            }
        }
        RelativeDependencyPattern::DisplayNoneDuplicate => {
            let visible_anchor = tree.push(SimpleNode::with_measured_size(
                block_style(relative_anchor_style(20, 1.0, 0.0)),
                Size::new(19.0, 13.0),
            ));
            let follower = tree.push(SimpleNode::with_measured_size(
                block_style(Style {
                    relative_right_of: 20,
                    relative_bottom_of: 20,
                    ..Style::default()
                }),
                Size::new(9.0, 7.0),
            ));
            let hidden_anchor = tree.push(SimpleNode::new(Style {
                display: Display::None,
                relative_id: 20,
                width: Length::points(80.0),
                height: Length::points(40.0),
                ..Style::default()
            }));
            for child in [visible_anchor, follower, hidden_anchor] {
                tree.append_child(root, child);
            }
        }
        RelativeDependencyPattern::DuplicateEdgeAlignment => {
            let first_anchor = tree.push(SimpleNode::with_measured_size(
                block_style(Style {
                    relative_id: 30,
                    relative_align_right: RELATIVE_ALIGN_PARENT,
                    relative_align_bottom: RELATIVE_ALIGN_PARENT,
                    margin: Rect::new(
                        Length::points(1.0),
                        Length::points(2.0),
                        Length::points(3.0),
                        Length::points(1.0),
                    ),
                    ..Style::default()
                }),
                Size::new(24.0, 18.0),
            ));
            let follower = tree.push(SimpleNode::with_measured_size(
                block_style(Style {
                    relative_align_left: 30,
                    relative_align_bottom: 30,
                    margin: Rect::new(
                        Length::points(2.0),
                        Length::points(1.0),
                        Length::points(1.0),
                        Length::points(2.0),
                    ),
                    ..Style::default()
                }),
                Size::new(10.0, 8.0),
            ));
            let last_anchor = tree.push(SimpleNode::with_measured_size(
                block_style(relative_anchor_style(30, 0.0, 0.0)),
                Size::new(16.0, 10.0),
            ));
            for child in [first_anchor, follower, last_anchor] {
                tree.append_child(root, child);
            }
        }
        RelativeDependencyPattern::ParentEndRecompute => {
            let trailing = tree.push(SimpleNode::with_measured_size(
                block_style(Style {
                    relative_align_right: RELATIVE_ALIGN_PARENT,
                    relative_align_bottom: RELATIVE_ALIGN_PARENT,
                    margin: Rect::new(
                        Length::points(2.0),
                        Length::points(1.0),
                        Length::points(3.0),
                        Length::points(2.0),
                    ),
                    ..Style::default()
                }),
                Size::new(21.0, 12.0),
            ));
            let centered = tree.push(SimpleNode::with_measured_size(
                block_style(Style {
                    relative_center: RelativeCenter::Both,
                    ..Style::default()
                }),
                Size::new(14.0, 9.0),
            ));
            for child in [trailing, centered] {
                tree.append_child(root, child);
            }
        }
        RelativeDependencyPattern::CombinedDependencyOrder => {
            let first = tree.push(SimpleNode::with_measured_size(
                block_style(Style {
                    relative_id: 41,
                    relative_bottom_of: 42,
                    margin: Rect::new(
                        Length::ZERO,
                        Length::points(1.0),
                        Length::points(2.0),
                        Length::ZERO,
                    ),
                    ..Style::default()
                }),
                Size::new(12.0, 10.0),
            ));
            let second = tree.push(SimpleNode::with_measured_size(
                block_style(Style {
                    relative_id: 42,
                    relative_right_of: 41,
                    margin: Rect::new(
                        Length::points(1.0),
                        Length::ZERO,
                        Length::ZERO,
                        Length::points(1.0),
                    ),
                    ..Style::default()
                }),
                Size::new(7.0, 8.0),
            ));
            let root_item = tree.push(SimpleNode::with_measured_size(
                block_style(Style {
                    relative_center: RelativeCenter::Horizontal,
                    order: -1,
                    ..Style::default()
                }),
                Size::new(9.0, 5.0),
            ));
            for child in [first, second, root_item] {
                tree.append_child(root, child);
            }
        }
    }
}

fn relative_anchor_style(relative_id: i32, horizontal_margin: f32, vertical_margin: f32) -> Style {
    Style {
        relative_id,
        margin: Rect::new(
            Length::points(horizontal_margin),
            Length::points(horizontal_margin + 1.0),
            Length::points(vertical_margin),
            Length::points(vertical_margin + 1.0),
        ),
        ..Style::default()
    }
}

fn relative_measured_constraint_tree(
    relative_layout_once: bool,
    constraint_pattern: RelativeMeasuredConstraint,
    measure_behavior: GeneratedMeasureBehavior,
) -> (GeneratedMeasuringTree, usize) {
    let mut tree = GeneratedMeasuringTree::default();
    let root = tree.push(GeneratedMeasuringNode::new(relative_style(Style {
        width: Length::points(124.0),
        height: Length::points(88.0),
        relative_layout_once,
        padding: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(7.0),
            Length::points(11.0),
        ),
        ..Style::default()
    })));
    let start_anchor = tree.push(GeneratedMeasuringNode::measured(
        block_style(Style {
            relative_id: 101,
            relative_align_left: RELATIVE_ALIGN_PARENT,
            relative_align_top: RELATIVE_ALIGN_PARENT,
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::points(1.0),
                Length::ZERO,
            ),
            ..Style::default()
        }),
        Size::new(22.0, 13.0),
    ));
    let measured = tree.push(GeneratedMeasuringNode::with_behavior(
        block_style(relative_measured_constraint_style(constraint_pattern)),
        measure_behavior,
    ));
    let end_anchor = tree.push(GeneratedMeasuringNode::measured(
        block_style(Style {
            relative_id: 202,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            margin: Rect::new(
                Length::points(2.0),
                Length::points(1.0),
                Length::ZERO,
                Length::points(1.0),
            ),
            ..Style::default()
        }),
        Size::new(18.0, 15.0),
    ));

    for child in [start_anchor, measured, end_anchor] {
        tree.append_child(root, child);
    }

    (tree, root)
}

fn relative_measured_constraint_style(constraint_pattern: RelativeMeasuredConstraint) -> Style {
    let mut style = Style {
        margin: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(1.0),
            Length::points(4.0),
        ),
        ..Style::default()
    };
    match constraint_pattern {
        RelativeMeasuredConstraint::ParentEnd => {
            style.relative_align_right = RELATIVE_ALIGN_PARENT;
            style.relative_align_bottom = RELATIVE_ALIGN_PARENT;
        }
        RelativeMeasuredConstraint::ParentBoth => {
            style.relative_align_left = RELATIVE_ALIGN_PARENT;
            style.relative_align_right = RELATIVE_ALIGN_PARENT;
            style.relative_align_top = RELATIVE_ALIGN_PARENT;
            style.relative_align_bottom = RELATIVE_ALIGN_PARENT;
        }
        RelativeMeasuredConstraint::AfterAnchor => {
            style.relative_right_of = 101;
            style.relative_bottom_of = 101;
        }
        RelativeMeasuredConstraint::BeforeAnchor => {
            style.relative_left_of = 202;
            style.relative_top_of = 202;
        }
        RelativeMeasuredConstraint::BetweenAnchors => {
            style.relative_right_of = 101;
            style.relative_left_of = 202;
            style.relative_bottom_of = 101;
            style.relative_top_of = 202;
        }
    }
    style
}

fn relative_composite_feature_tree(
    constraint_mode: RelativeConstraintMode,
    relative_layout_once: bool,
) -> (SimpleTree, usize, Constraints) {
    let mut tree = SimpleTree::default();
    let root_style = relative_dependency_root_style(constraint_mode, relative_layout_once);
    let constraints = relative_dependency_constraints(constraint_mode);
    let root = tree.push(SimpleNode::new(root_style));

    let start_anchor = tree.push(SimpleNode::with_measured_size(
        block_style(Style {
            relative_id: 101,
            relative_align_left: RELATIVE_ALIGN_PARENT,
            relative_align_top: RELATIVE_ALIGN_PARENT,
            margin: Rect::new(
                Length::points(2.0),
                Length::points(1.0),
                Length::points(3.0),
                Length::ZERO,
            ),
            padding: Rect::all(Length::points(1.0)),
            border: Rect::all(1.0),
            ..Style::default()
        }),
        Size::new(21.0, 12.0),
    ));
    let end_anchor = tree.push(SimpleNode::with_measured_size(
        block_style(Style {
            relative_id: 202,
            relative_align_right: RELATIVE_ALIGN_PARENT,
            relative_align_bottom: RELATIVE_ALIGN_PARENT,
            margin: Rect::new(
                Length::points(1.0),
                Length::points(2.0),
                Length::ZERO,
                Length::points(2.0),
            ),
            padding: Rect::all(Length::points(1.0)),
            border: Rect::all(1.0),
            ..Style::default()
        }),
        Size::new(18.0, 14.0),
    ));
    let between = tree.push(SimpleNode::with_measured_size(
        block_style(Style {
            relative_right_of: 101,
            relative_left_of: 202,
            relative_bottom_of: 101,
            relative_top_of: 202,
            min_width: Length::points(9.0),
            max_width: Length::calc(4.0, 50.0),
            min_height: Length::points(8.0),
            max_height: Length::percent(80.0),
            margin: Rect::all(Length::points(1.0)),
            padding: Rect::all(Length::points(1.0)),
            border: Rect::all(1.0),
            ..Style::default()
        }),
        Size::new(45.0, 22.0),
    ));
    let centered = tree.push(SimpleNode::new(block_style(Style {
        order: -1,
        relative_center: RelativeCenter::Both,
        width: Length::points(17.0),
        height: Length::points(9.0),
        box_sizing: BoxSizing::BorderBox,
        aspect_ratio: Some(1.75),
        margin: Rect::all(Length::points(1.0)),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(1.0),
        ..Style::default()
    })));
    let follower = tree.push(SimpleNode::new(block_style(Style {
        relative_right_of: 101,
        relative_align_bottom: 101,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(3.0, 35.0))),
        height: Length::calc(5.0, 20.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(2.0),
            Length::points(1.0),
        ),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(1.0),
        ..Style::default()
    })));
    let hidden = tree.push(SimpleNode::new(Style::display_none()));
    let hidden_descendant = tree.push(SimpleNode::new(block_style(Style {
        width: Length::points(20.0),
        height: Length::points(8.0),
        ..Style::default()
    })));
    let absolute = tree.push(SimpleNode::new(block_style(Style {
        position: PositionType::Absolute,
        left: Length::percent(8.0),
        right: Length::Auto,
        top: Length::calc(2.0, 12.0),
        bottom: Length::points(3.0),
        width: Length::points(16.0),
        height: Length::fit_content(Some(BaseLength::fixed(18.0))),
        margin: Rect::all(Length::points(1.0)),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(1.0),
        ..Style::default()
    })));

    for child in [
        start_anchor,
        centered,
        between,
        follower,
        hidden,
        end_anchor,
        absolute,
    ] {
        tree.append_child(root, child);
    }
    tree.append_child(hidden, hidden_descendant);

    (tree, root, constraints)
}

fn grid_item_alignment_tree(
    direction: Direction,
    justify_items: JustifyItems,
    align_items: AlignItems,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_style(Style {
        direction,
        justify_items,
        align_items,
        width: Length::points(80.0),
        height: Length::points(55.0),
        grid_template_columns: vec![Length::points(30.0), Length::points(40.0)],
        grid_template_rows: vec![Length::points(20.0), Length::points(25.0)],
        column_gap: Length::points(3.0),
        row_gap: Length::points(5.0),
        ..Style::default()
    })));

    for (column, row, width, height) in [(1, 1, 12.0, 8.0), (2, 1, 16.0, 10.0), (1, 2, 10.0, 14.0)]
    {
        let child = tree.push(SimpleNode::new(Style {
            width: Length::points(width),
            height: Length::points(height),
            grid_column_start: Some(column),
            grid_row_start: Some(row),
            ..Style::default()
        }));
        tree.append_child(root, child);
    }

    (tree, root)
}

fn grid_auto_margin_alignment_tree(
    direction: Direction,
    placement: GridAutoMarginPlacement,
    inline_margin: GridAutoMarginPattern,
    block_margin: GridAutoMarginPattern,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_style(Style {
        direction,
        justify_items: JustifyItems::End,
        align_items: AlignItems::FlexEnd,
        width: Length::points(112.0),
        height: Length::points(74.0),
        padding: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(4.0),
            Length::points(5.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        grid_template_columns: vec![Length::points(32.0), Length::points(42.0)],
        grid_template_rows: vec![Length::points(24.0), Length::points(28.0)],
        grid_auto_columns: vec![Length::points(30.0)],
        grid_auto_rows: vec![Length::points(22.0)],
        column_gap: Length::points(4.0),
        row_gap: Length::points(3.0),
        ..Style::default()
    })));

    let auto_margin_item = tree.push(SimpleNode::new(grid_auto_margin_child_style(
        placement,
        inline_margin,
        block_margin,
    )));
    let sibling = tree.push(SimpleNode::new(Style {
        width: Length::points(9.0),
        height: Length::points(7.0),
        grid_column_start: Some(1),
        grid_row_start: Some(2),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::ZERO,
            Length::points(1.0),
        ),
        ..Style::default()
    }));
    tree.append_child(root, auto_margin_item);
    tree.append_child(root, sibling);

    (tree, root)
}

fn grid_auto_margin_child_style(
    placement: GridAutoMarginPlacement,
    inline_margin: GridAutoMarginPattern,
    block_margin: GridAutoMarginPattern,
) -> Style {
    let mut style = Style {
        width: Length::points(13.0),
        height: Length::points(11.0),
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        margin: grid_auto_margin_rect(inline_margin, block_margin),
        ..Style::default()
    };

    match placement {
        GridAutoMarginPlacement::ExplicitFirstCell => {
            style.grid_column_start = Some(1);
            style.grid_row_start = Some(1);
        }
        GridAutoMarginPlacement::ExplicitSecondCell => {
            style.grid_column_start = Some(2);
            style.grid_row_start = Some(2);
        }
        GridAutoMarginPlacement::AutoPlacedSpan => {
            style.grid_column_span = 2;
        }
    }

    style
}

fn grid_auto_margin_rect(
    inline_margin: GridAutoMarginPattern,
    block_margin: GridAutoMarginPattern,
) -> Rect<Length> {
    Rect::new(
        auto_margin_start(inline_margin, Length::points(1.0)),
        auto_margin_end(inline_margin, Length::points(2.0)),
        auto_margin_start(block_margin, Length::points(3.0)),
        auto_margin_end(block_margin, Length::points(4.0)),
    )
}

fn auto_margin_start(pattern: GridAutoMarginPattern, fallback: Length) -> Length {
    match pattern {
        GridAutoMarginPattern::Start | GridAutoMarginPattern::Both => Length::Auto,
        GridAutoMarginPattern::None | GridAutoMarginPattern::End => fallback,
    }
}

fn auto_margin_end(pattern: GridAutoMarginPattern, fallback: Length) -> Length {
    match pattern {
        GridAutoMarginPattern::End | GridAutoMarginPattern::Both => Length::Auto,
        GridAutoMarginPattern::None | GridAutoMarginPattern::Start => fallback,
    }
}

fn grid_track_sizing_tree(
    constraint_mode: GridTrackConstraintMode,
    variant: GridTrackSizingVariant,
) -> (SimpleTree, usize, Constraints) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_track_sizing_root_style(
        constraint_mode,
        variant,
    )));
    append_grid_track_sizing_children(&mut tree, root, variant);
    (tree, root, grid_track_sizing_constraints(constraint_mode))
}

fn grid_track_sizing_constraints(constraint_mode: GridTrackConstraintMode) -> Constraints {
    match constraint_mode {
        GridTrackConstraintMode::DefiniteRoot => Constraints::definite(164.0, 102.0),
        GridTrackConstraintMode::IndefiniteOwner => Constraints::indefinite(),
        GridTrackConstraintMode::AtMostOwner => Constraints::new(
            SideConstraint::at_most(148.0),
            SideConstraint::at_most(94.0),
        ),
    }
}

fn grid_track_sizing_root_style(
    constraint_mode: GridTrackConstraintMode,
    variant: GridTrackSizingVariant,
) -> Style {
    let mut style = Style {
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
        ..Style::default()
    };

    if matches!(constraint_mode, GridTrackConstraintMode::DefiniteRoot) {
        style.width = Length::points(164.0);
        style.height = Length::points(102.0);
    }

    match variant {
        GridTrackSizingVariant::FlexibleMinMax => {
            style.grid_template_columns =
                vec![Length::points(18.0), Length::points(12.0), Length::Auto];
            style.grid_template_columns_max =
                vec![Length::fr(1.0), Length::fr(2.0), Length::MaxContent];
            style.grid_template_rows = vec![Length::Auto, Length::points(14.0)];
            style.grid_template_rows_max = vec![Length::MaxContent, Length::points(18.0)];
        }
        GridTrackSizingVariant::FixedMaxGrowthLimit => {
            style.grid_template_columns = vec![Length::Auto, Length::Auto, Length::Auto];
            style.grid_template_columns_max =
                vec![Length::points(52.0), Length::points(64.0), Length::Auto];
            style.grid_template_rows = vec![Length::Auto, Length::Auto];
            style.grid_template_rows_max = vec![Length::points(34.0), Length::points(42.0)];
        }
        GridTrackSizingVariant::MaxContentMinimum => {
            style.grid_template_columns =
                vec![Length::MaxContent, Length::points(10.0), Length::Auto];
            style.grid_template_columns_max = vec![
                Length::points(40.0),
                Length::points(10.0),
                Length::MaxContent,
            ];
            style.grid_template_rows = vec![Length::MaxContent, Length::points(8.0)];
            style.grid_template_rows_max = vec![Length::points(22.0), Length::points(8.0)];
        }
        GridTrackSizingVariant::FitContentCaps => {
            style.grid_template_columns =
                vec![Length::points(16.0), Length::Auto, Length::points(10.0)];
            style.grid_template_columns_max = vec![
                Length::fit_content(Some(BaseLength::fixed_and_percent(6.0, 32.0))),
                Length::fit_content(Some(BaseLength::fixed(46.0))),
                Length::points(22.0),
            ];
            style.grid_template_rows = vec![Length::Auto, Length::points(12.0)];
            style.grid_template_rows_max = vec![
                Length::fit_content(Some(BaseLength::fixed_and_percent(4.0, 40.0))),
                Length::MaxContent,
            ];
        }
        GridTrackSizingVariant::ImplicitAutoTracks => {
            style.grid_template_columns = vec![Length::points(18.0)];
            style.grid_template_columns_max = vec![Length::points(18.0)];
            style.grid_template_rows = vec![Length::points(12.0)];
            style.grid_template_rows_max = vec![Length::points(12.0)];
            style.grid_auto_columns = vec![
                Length::Auto,
                Length::fit_content(Some(BaseLength::fixed(30.0))),
                Length::points(14.0),
            ];
            style.grid_auto_columns_max = vec![
                Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 35.0))),
                Length::MaxContent,
                Length::fr(1.0),
            ];
            style.grid_auto_rows = vec![
                Length::Auto,
                Length::fit_content(Some(BaseLength::fixed_and_percent(3.0, 30.0))),
            ];
            style.grid_auto_rows_max = vec![Length::MaxContent, Length::fr(1.0)];
            style.grid_auto_flow = GridAutoFlow::ColumnDense;
        }
        GridTrackSizingVariant::ImplicitFitContentMaxCaps => {
            style.grid_auto_columns = vec![Length::points(20.0), Length::points(10.0)];
            style.grid_auto_columns_max = vec![
                Length::fit_content(Some(BaseLength::fixed(40.0))),
                Length::points(10.0),
            ];
            style.grid_auto_rows = vec![Length::points(20.0), Length::points(10.0)];
            style.grid_auto_rows_max = vec![
                Length::fit_content(Some(BaseLength::fixed(40.0))),
                Length::points(10.0),
            ];
        }
    }

    grid_style(style)
}

fn append_grid_track_sizing_children(
    tree: &mut SimpleTree,
    root: usize,
    variant: GridTrackSizingVariant,
) {
    match variant {
        GridTrackSizingVariant::FlexibleMinMax => {
            let first = tree.push(SimpleNode::new(grid_track_sizing_child_style(Style {
                width: Length::points(24.0),
                height: Length::points(10.0),
                grid_column_start: Some(1),
                grid_row_start: Some(1),
                ..Style::default()
            })));
            let spanning = tree.push(SimpleNode::with_measured_size(
                grid_track_sizing_child_style(Style {
                    grid_column_start: Some(2),
                    grid_column_span: 2,
                    grid_row_start: Some(1),
                    ..Style::default()
                }),
                Size::new(82.0, 26.0),
            ));
            let marker = tree.push(SimpleNode::new(grid_track_sizing_child_style(Style {
                width: Length::points(9.0),
                height: Length::points(7.0),
                grid_column_start: Some(3),
                grid_row_start: Some(2),
                ..Style::default()
            })));
            for child in [first, spanning, marker] {
                tree.append_child(root, child);
            }
        }
        GridTrackSizingVariant::FixedMaxGrowthLimit => {
            let first_span = tree.push(SimpleNode::with_measured_size(
                grid_track_sizing_child_style(Style {
                    grid_column_start: Some(1),
                    grid_column_span: 2,
                    grid_row_start: Some(1),
                    ..Style::default()
                }),
                Size::new(88.0, 31.0),
            ));
            let second_span = tree.push(SimpleNode::with_measured_size(
                grid_track_sizing_child_style(Style {
                    grid_column_start: Some(2),
                    grid_column_span: 2,
                    grid_row_start: Some(1),
                    ..Style::default()
                }),
                Size::new(78.0, 24.0),
            ));
            let marker = tree.push(SimpleNode::new(grid_track_sizing_child_style(Style {
                width: Length::points(12.0),
                height: Length::points(8.0),
                grid_column_start: Some(3),
                grid_row_start: Some(2),
                ..Style::default()
            })));
            for child in [first_span, second_span, marker] {
                tree.append_child(root, child);
            }
        }
        GridTrackSizingVariant::MaxContentMinimum => {
            let intrinsic = tree.push(SimpleNode::with_measured_size(
                grid_track_sizing_child_style(Style {
                    grid_column_start: Some(1),
                    grid_row_start: Some(1),
                    ..Style::default()
                }),
                Size::new(70.0, 30.0),
            ));
            let spanning = tree.push(SimpleNode::with_measured_size(
                grid_track_sizing_child_style(Style {
                    grid_column_start: Some(1),
                    grid_column_span: 2,
                    grid_row_start: Some(2),
                    ..Style::default()
                }),
                Size::new(98.0, 8.0),
            ));
            let marker = tree.push(SimpleNode::new(grid_track_sizing_child_style(Style {
                width: Length::points(8.0),
                height: Length::points(6.0),
                grid_column_start: Some(2),
                grid_row_start: Some(1),
                ..Style::default()
            })));
            for child in [intrinsic, spanning, marker] {
                tree.append_child(root, child);
            }
        }
        GridTrackSizingVariant::FitContentCaps => {
            let capped = tree.push(SimpleNode::with_measured_size(
                grid_track_sizing_child_style(Style {
                    grid_column_start: Some(1),
                    grid_row_start: Some(1),
                    ..Style::default()
                }),
                Size::new(92.0, 54.0),
            ));
            let spanning = tree.push(SimpleNode::with_measured_size(
                grid_track_sizing_child_style(Style {
                    grid_column_start: Some(1),
                    grid_column_span: 2,
                    grid_row_start: Some(1),
                    ..Style::default()
                }),
                Size::new(108.0, 28.0),
            ));
            let marker = tree.push(SimpleNode::new(grid_track_sizing_child_style(Style {
                width: Length::points(10.0),
                height: Length::points(9.0),
                grid_column_start: Some(3),
                grid_row_start: Some(2),
                ..Style::default()
            })));
            for child in [capped, spanning, marker] {
                tree.append_child(root, child);
            }
        }
        GridTrackSizingVariant::ImplicitAutoTracks => {
            let explicit = tree.push(SimpleNode::with_measured_size(
                grid_track_sizing_child_style(Style {
                    grid_column_start: Some(1),
                    grid_row_start: Some(1),
                    ..Style::default()
                }),
                Size::new(16.0, 10.0),
            ));
            let implicit_span = tree.push(SimpleNode::with_measured_size(
                grid_track_sizing_child_style(Style {
                    grid_column_start: Some(3),
                    grid_column_span: 2,
                    grid_row_start: Some(2),
                    ..Style::default()
                }),
                Size::new(74.0, 32.0),
            ));
            let far_implicit = tree.push(SimpleNode::new(grid_track_sizing_child_style(Style {
                width: Length::points(18.0),
                height: Length::points(11.0),
                grid_column_start: Some(5),
                grid_row_start: Some(3),
                ..Style::default()
            })));
            for child in [explicit, implicit_span, far_implicit] {
                tree.append_child(root, child);
            }
        }
        GridTrackSizingVariant::ImplicitFitContentMaxCaps => {
            let capped = tree.push(SimpleNode::with_measured_size(
                grid_track_sizing_child_style(Style {
                    grid_column_start: Some(1),
                    grid_row_start: Some(1),
                    ..Style::default()
                }),
                Size::new(70.0, 70.0),
            ));
            let following = tree.push(SimpleNode::new(grid_track_sizing_child_style(Style {
                width: Length::points(8.0),
                height: Length::points(6.0),
                grid_column_start: Some(2),
                grid_row_start: Some(2),
                ..Style::default()
            })));
            for child in [capped, following] {
                tree.append_child(root, child);
            }
        }
    }
}

fn grid_track_sizing_child_style(style: Style) -> Style {
    block_style(Style {
        justify_self: JustifyItems::Start,
        align_self: Some(AlignItems::FlexStart),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(1.0),
            Length::ZERO,
        ),
        ..style
    })
}

fn grid_content_alignment_tree(
    justify_content: JustifyContent,
    align_content: AlignContent,
    direction: Direction,
    size_mode: GridContentSizeMode,
) -> (SimpleTree, usize, Constraints) {
    let (width, height, column_gap, row_gap) = match size_mode {
        GridContentSizeMode::ExtraSpace => (90.0, 60.0, 2.0, 3.0),
        GridContentSizeMode::Overflow => (30.0, 24.0, 10.0, 10.0),
    };
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_style(Style {
        direction,
        justify_content,
        align_content,
        width: Length::points(width),
        height: Length::points(height),
        grid_template_columns: vec![Length::points(20.0), Length::points(18.0)],
        grid_template_rows: vec![Length::points(12.0), Length::points(14.0)],
        column_gap: Length::points(column_gap),
        row_gap: Length::points(row_gap),
        ..Style::default()
    })));

    for (column, row) in [(1, 1), (2, 1), (1, 2), (2, 2)] {
        let child = tree.push(SimpleNode::new(Style {
            grid_column_start: Some(column),
            grid_row_start: Some(row),
            ..Style::default()
        }));
        tree.append_child(root, child);
    }

    (tree, root, Constraints::definite(width, height))
}

fn grid_auto_flow_placement_tree(
    grid_auto_flow: GridAutoFlow,
    direction: Direction,
    pattern: GridPlacementPattern,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(grid_style(Style {
        direction,
        grid_auto_flow,
        width: Length::points(118.0),
        height: Length::points(86.0),
        grid_template_columns: vec![
            Length::points(20.0),
            Length::points(22.0),
            Length::points(24.0),
        ],
        grid_template_rows: vec![Length::points(14.0), Length::points(16.0)],
        grid_auto_columns: vec![Length::points(18.0), Length::points(26.0)],
        grid_auto_rows: vec![Length::points(12.0), Length::points(20.0)],
        column_gap: Length::points(3.0),
        row_gap: Length::points(2.0),
        padding: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(3.0),
            Length::points(4.0),
        ),
        ..Style::default()
    })));

    match pattern {
        GridPlacementPattern::AutoSpans => append_grid_auto_span_children(&mut tree, root),
        GridPlacementPattern::LockedLines => append_grid_locked_line_children(&mut tree, root),
        GridPlacementPattern::ImplicitLines => append_grid_implicit_line_children(&mut tree, root),
    }

    (tree, root)
}

fn append_grid_auto_span_children(tree: &mut SimpleTree, root: usize) {
    let first = tree.push(SimpleNode::new(grid_child_style(
        10.0,
        8.0,
        Style {
            grid_column_span: 2,
            ..Style::default()
        },
    )));
    let hidden = tree.push(SimpleNode::new(Style::display_none()));
    let second = tree.push(SimpleNode::new(grid_child_style(
        9.0,
        11.0,
        Style {
            grid_row_span: 2,
            ..Style::default()
        },
    )));
    let third = tree.push(SimpleNode::new(grid_child_style(
        13.0,
        7.0,
        Style::default(),
    )));
    let fourth = tree.push(SimpleNode::new(grid_child_style(
        8.0,
        9.0,
        Style {
            grid_column_span: 2,
            ..Style::default()
        },
    )));

    for child in [first, hidden, second, third, fourth] {
        tree.append_child(root, child);
    }
}

fn append_grid_locked_line_children(tree: &mut SimpleTree, root: usize) {
    let first = tree.push(SimpleNode::new(grid_child_style(
        10.0,
        8.0,
        Style {
            grid_row_start: Some(2),
            grid_column_span: 2,
            ..Style::default()
        },
    )));
    let second = tree.push(SimpleNode::new(grid_child_style(
        12.0,
        10.0,
        Style {
            grid_column_start: Some(2),
            grid_row_span: 2,
            ..Style::default()
        },
    )));
    let third = tree.push(SimpleNode::new(grid_child_style(
        9.0,
        7.0,
        Style::default(),
    )));
    let fourth = tree.push(SimpleNode::new(grid_child_style(
        11.0,
        9.0,
        Style {
            grid_row_start: Some(1),
            ..Style::default()
        },
    )));

    for child in [first, second, third, fourth] {
        tree.append_child(root, child);
    }
}

fn append_grid_implicit_line_children(tree: &mut SimpleTree, root: usize) {
    let first = tree.push(SimpleNode::new(grid_child_style(
        10.0,
        8.0,
        Style {
            grid_column_start: Some(-1),
            grid_row_start: Some(1),
            ..Style::default()
        },
    )));
    let second = tree.push(SimpleNode::new(grid_child_style(
        12.0,
        10.0,
        Style {
            grid_column_start: Some(5),
            grid_row_start: Some(2),
            ..Style::default()
        },
    )));
    let third = tree.push(SimpleNode::new(grid_child_style(
        9.0,
        7.0,
        Style {
            grid_row_start: Some(-1),
            grid_column_span: 2,
            ..Style::default()
        },
    )));
    let fourth = tree.push(SimpleNode::new(grid_child_style(
        11.0,
        9.0,
        Style::default(),
    )));

    for child in [first, second, third, fourth] {
        tree.append_child(root, child);
    }
}

fn grid_child_style(width: f32, height: f32, style: Style) -> Style {
    Style {
        width: Length::points(width),
        height: Length::points(height),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(1.0),
            Length::ZERO,
        ),
        ..style
    }
}

fn sizing_minmax_aspect_tree(
    container: GeneratedContainer,
    variant: SizingVariant,
) -> (SimpleTree, usize) {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(sizing_container_style(container, variant)));
    append_sizing_children(&mut tree, root, variant);
    (tree, root)
}

fn sizing_container_style(container: GeneratedContainer, variant: SizingVariant) -> Style {
    let mut base = Style {
        width: Length::points(112.0),
        height: Length::points(78.0),
        padding: Rect::new(
            Length::points(3.0),
            Length::points(4.0),
            Length::points(5.0),
            Length::points(2.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 3.0),
        justify_content: JustifyContent::Center,
        align_items: AlignItems::Center,
        ..Style::default()
    };

    match variant {
        SizingVariant::PercentCalcRoot => {
            base.width = Length::percent(72.0);
            base.height = Length::calc(8.0, 54.0);
        }
        SizingVariant::FitContentRoot => {
            base.width = Length::fit_content(Some(BaseLength::fixed_and_percent(6.0, 55.0)));
            base.height = Length::fit_content(Some(BaseLength::fixed_and_percent(4.0, 45.0)));
        }
        SizingVariant::FitContentSubtree => {}
        SizingVariant::PercentMinMaxRoot | SizingVariant::BorderBoxPercentMinMaxRoot => {
            if matches!(variant, SizingVariant::BorderBoxPercentMinMaxRoot) {
                base.box_sizing = BoxSizing::BorderBox;
            }
            base.width = Length::Auto;
            base.height = Length::Auto;
            base.min_width = Length::percent(45.0);
            base.max_width = Length::calc(16.0, 70.0);
            base.min_height = Length::percent(35.0);
            base.max_height = Length::calc(10.0, 80.0);
        }
        SizingVariant::ContentBoxAspectRoot | SizingVariant::BorderBoxAspectRoot => {
            if matches!(variant, SizingVariant::BorderBoxAspectRoot) {
                base.box_sizing = BoxSizing::BorderBox;
            }
            base.width = Length::points(92.0);
            base.height = Length::Auto;
            base.aspect_ratio = Some(1.6);
        }
        SizingVariant::IntrinsicMeasuredChild => {}
    }

    match container {
        GeneratedContainer::Block => block_style(base),
        GeneratedContainer::FlexRow => flex_style(base),
        GeneratedContainer::FlexColumnRtl => flex_style(Style {
            direction: Direction::Rtl,
            flex_direction: FlexDirection::Column,
            ..base
        }),
        GeneratedContainer::LinearRow => linear_style(Style {
            linear_orientation: LinearOrientation::Horizontal,
            ..base
        }),
        GeneratedContainer::LinearColumnRtl => linear_style(Style {
            direction: Direction::Rtl,
            linear_orientation: LinearOrientation::Vertical,
            ..base
        }),
        GeneratedContainer::Relative => relative_style(base),
        GeneratedContainer::Grid => grid_style(Style {
            grid_template_columns: vec![Length::points(34.0), Length::Auto],
            grid_template_rows: vec![Length::points(22.0), Length::Auto],
            column_gap: Length::points(3.0),
            row_gap: Length::points(4.0),
            ..base
        }),
    }
}

fn append_sizing_children(tree: &mut SimpleTree, root: usize, variant: SizingVariant) {
    if matches!(variant, SizingVariant::FitContentSubtree) {
        let child = tree.push(SimpleNode::new(block_style(Style {
            width: Length::fit_content(Some(BaseLength::fixed_and_percent(5.0, 50.0))),
            height: Length::fit_content(Some(BaseLength::fixed_and_percent(3.0, 40.0))),
            margin: Rect::new(
                Length::points(2.0),
                Length::points(1.0),
                Length::points(3.0),
                Length::points(2.0),
            ),
            ..Style::default()
        })));
        let grandchild = tree.push(SimpleNode::new(block_style(Style {
            width: Length::points(74.0),
            height: Length::points(26.0),
            padding: Rect::all(Length::points(1.0)),
            border: Rect::all(1.0),
            ..Style::default()
        })));
        let sibling = tree.push(SimpleNode::new(block_style(Style {
            width: Length::points(18.0),
            height: Length::points(12.0),
            ..Style::default()
        })));
        tree.append_child(root, child);
        tree.append_child(child, grandchild);
        tree.append_child(root, sibling);
        return;
    }

    if matches!(variant, SizingVariant::IntrinsicMeasuredChild) {
        let first = tree.push(SimpleNode::with_measured_size(
            block_style(Style {
                width: Length::max_content(),
                height: Length::fit_content(Some(BaseLength::fixed_and_percent(2.0, 40.0))),
                min_width: Length::points(18.0),
                max_width: Length::points(42.0),
                margin: Rect::new(
                    Length::points(1.0),
                    Length::points(2.0),
                    Length::points(1.0),
                    Length::points(2.0),
                ),
                ..Style::default()
            }),
            Size::new(35.0, 24.0),
        ));
        let second = tree.push(SimpleNode::with_measured_size(
            block_style(Style {
                width: Length::fit_content(Some(BaseLength::fixed_and_percent(3.0, 50.0))),
                height: Length::max_content(),
                min_height: Length::points(12.0),
                max_height: Length::points(30.0),
                ..Style::default()
            }),
            Size::new(26.0, 18.0),
        ));
        tree.append_child(root, first);
        tree.append_child(root, second);
        return;
    }

    let first = tree.push(SimpleNode::new(block_style(Style {
        width: Length::calc(4.0, 28.0),
        height: Length::percent(25.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    })));
    let second = tree.push(SimpleNode::new(block_style(Style {
        width: Length::points(32.0),
        height: Length::points(18.0),
        box_sizing: if matches!(
            variant,
            SizingVariant::BorderBoxPercentMinMaxRoot | SizingVariant::BorderBoxAspectRoot
        ) {
            BoxSizing::BorderBox
        } else {
            BoxSizing::ContentBox
        },
        padding: Rect::all(Length::points(2.0)),
        border: Rect::all(1.0),
        ..Style::default()
    })));
    tree.append_child(root, first);
    tree.append_child(root, second);
}

#[derive(Clone, Debug)]
struct GeneratedMeasuringNode {
    style: Style,
    layout: LayoutResult,
    children: Vec<usize>,
    measure: Option<GeneratedMeasureBehavior>,
}

impl GeneratedMeasuringNode {
    fn new(style: Style) -> Self {
        Self {
            style,
            layout: LayoutResult::default(),
            children: Vec::new(),
            measure: None,
        }
    }

    fn measured(style: Style, measured_size: Size) -> Self {
        Self::with_behavior(style, GeneratedMeasureBehavior::Fixed(measured_size))
    }

    fn with_behavior(style: Style, measure: GeneratedMeasureBehavior) -> Self {
        Self {
            measure: Some(measure),
            ..Self::new(style)
        }
    }
}

#[derive(Clone, Debug, Default)]
struct GeneratedMeasuringTree {
    nodes: Vec<GeneratedMeasuringNode>,
}

impl GeneratedMeasuringTree {
    fn push(&mut self, node: GeneratedMeasuringNode) -> usize {
        let id = self.nodes.len();
        self.nodes.push(node);
        id
    }

    fn append_child(&mut self, parent: usize, child: usize) {
        self.nodes[parent].children.push(child);
    }
}

impl LayoutTree for GeneratedMeasuringTree {
    type NodeId = usize;
    type Children<'a> = std::iter::Copied<std::slice::Iter<'a, usize>>;

    fn children(&self, node: Self::NodeId) -> Self::Children<'_> {
        self.nodes[node].children.iter().copied()
    }

    fn style(&self, node: Self::NodeId) -> &Style {
        &self.nodes[node].style
    }

    fn measure(&mut self, node: Self::NodeId, constraints: Constraints) -> Option<Size> {
        self.nodes[node].measure.map(|behavior| match behavior {
            GeneratedMeasureBehavior::Fixed(size) => Size::new(
                constraints.width.clamp(size.width),
                constraints.height.clamp(size.height),
            ),
            GeneratedMeasureBehavior::HeightFromWidth {
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
            GeneratedMeasureBehavior::WidthByHeightMode {
                at_most_width,
                definite_width,
                height,
            } => {
                let width = if constraints.height.is_definite() {
                    definite_width
                } else {
                    at_most_width
                };
                Size::new(
                    constraints.width.clamp(width),
                    constraints.height.clamp(height),
                )
            }
        })
    }

    fn has_measure(&self, node: Self::NodeId) -> bool {
        self.nodes[node].measure.is_some()
    }

    fn set_layout(&mut self, node: Self::NodeId, layout: LayoutResult) {
        self.nodes[node].layout = layout;
    }

    fn layout(&self, node: Self::NodeId) -> Option<LayoutResult> {
        Some(self.nodes[node].layout)
    }
}

impl BaselineLayoutTree for GeneratedMeasuringTree {
    fn layout_result(&self, node: Self::NodeId) -> LayoutResult {
        self.nodes[node].layout
    }
}

#[derive(Clone, Debug)]
struct DeterministicRng {
    state: u64,
}

impl DeterministicRng {
    const fn new(seed: u64) -> Self {
        Self { state: seed }
    }

    fn next_u32(&mut self) -> u32 {
        self.state = self
            .state
            .wrapping_mul(6_364_136_223_846_793_005)
            .wrapping_add(1_442_695_040_888_963_407);
        (self.state >> 32) as u32
    }

    fn range(&mut self, upper: usize) -> usize {
        debug_assert!(upper > 0);
        self.next_u32() as usize % upper
    }

    fn bool(&mut self) -> bool {
        self.range(2) == 0
    }

    fn points(&mut self, min: f32, step: f32, count: usize) -> f32 {
        min + step * self.range(count) as f32
    }
}

fn deterministic_supported_tree(
    rng: &mut DeterministicRng,
    case_index: usize,
) -> (SimpleTree, usize, Constraints) {
    let mut tree = SimpleTree::default();
    let root_display = match case_index % 3 {
        0 => Display::Block,
        1 => Display::Flex,
        _ => Display::Linear,
    };
    let root = tree.push(SimpleNode::new(random_container_style(
        rng,
        root_display,
        case_index,
    )));

    let child_count = 3 + rng.range(3);
    for child_index in 0..child_count {
        let child_display = random_child_display(rng, child_index);
        let child_style = random_child_style(rng, child_display, child_index);
        let child = SimpleNode::new(child_style);
        let child = tree.push(child);
        tree.append_child(root, child);

        if matches!(
            child_display,
            Display::Block | Display::Flex | Display::Linear
        ) && rng.range(4) == 0
        {
            append_random_grandchildren(rng, &mut tree, child, child_index);
        }
    }

    let constraints = match case_index % 3 {
        0 => Constraints::definite(160.0, 120.0),
        1 => Constraints {
            width: SideConstraint::at_most(180.0),
            height: SideConstraint::at_most(140.0),
        },
        _ => Constraints::indefinite(),
    };
    (tree, root, constraints)
}

fn debug_layout_snapshots(tree: &SimpleTree, root: usize, constraints: Constraints) -> String {
    let mut rust_tree = tree.clone();
    let rust_size =
        LayoutEngine::new().layout_with_owner_constraints(&mut rust_tree, root, constraints);
    let rust_snapshots = collect_layout_snapshots(&rust_tree, root);

    let mut cpp_tree = tree.clone();
    let cpp = CppStarlightEngine::new().layout(&mut cpp_tree, root, constraints);
    match cpp {
        Ok(cpp_size) => {
            let cpp_snapshots = collect_layout_snapshots(&cpp_tree, root);
            format!(
                "rust_size={rust_size:?}\ncpp_size={cpp_size:?}\nrust_snapshots={rust_snapshots:#?}\ncpp_snapshots={cpp_snapshots:#?}"
            )
        }
        Err(error) => format!(
            "rust_size={rust_size:?}\ncpp_error={error}\nrust_snapshots={rust_snapshots:#?}"
        ),
    }
}

fn debug_tree_summary(tree: &SimpleTree) -> String {
    let mut summary = String::from("tree_summary:\n");
    for (index, node) in tree.nodes.iter().enumerate() {
        let style = &node.style;
        summary.push_str(&format!(
            "  {index}: display={:?} position={:?} box={:?} dir={:?} width={:?} height={:?} min=({:?},{:?}) max=({:?},{:?}) margin={:?} padding={:?} border={:?} flex=({:?},{:?},{:?},{:?},{:?}) gap=({:?},{:?}) flex_basis={:?} grow={} shrink={} order={} align_self={:?} justify_self={:?} linear=({:?},{:?},{:?}) children={:?}\n",
            style.display,
            style.position,
            style.box_sizing,
            style.direction,
            style.width,
            style.height,
            style.min_width,
            style.min_height,
            style.max_width,
            style.max_height,
            style.margin,
            style.padding,
            style.border,
            style.flex_direction,
            style.flex_wrap,
            style.justify_content,
            style.align_items,
            style.align_content,
            style.row_gap,
            style.column_gap,
            style.flex_basis,
            style.flex_grow,
            style.flex_shrink,
            style.order,
            style.align_self,
            style.justify_self,
            style.linear_orientation,
            style.linear_gravity,
            style.linear_layout_gravity,
            node.children
        ));
    }
    summary
}

fn append_random_grandchildren(
    rng: &mut DeterministicRng,
    tree: &mut SimpleTree,
    parent: usize,
    child_index: usize,
) {
    for grandchild_index in 0..(1 + rng.range(2)) {
        let mut style = random_child_style(rng, Display::Block, grandchild_index);
        style.position = PositionType::Static;
        style.order = grandchild_index as i32;
        if child_index.is_multiple_of(2) {
            style.width = Length::points(rng.points(8.0, 3.0, 5));
        }
        let node = SimpleNode::new(style);
        let node = tree.push(node);
        tree.append_child(parent, node);
    }
}

fn random_container_style(
    rng: &mut DeterministicRng,
    display: Display,
    _case_index: usize,
) -> Style {
    let mut style = random_base_style(rng, display);
    style.width = random_axis_length(rng, true);
    style.height = random_axis_length(rng, true);
    style.min_width = Length::points(20.0);
    style.min_height = Length::points(16.0);
    style.padding = random_edge_lengths(rng, false);
    style.border = Rect::new(
        rng.range(2) as f32,
        rng.range(2) as f32,
        rng.range(2) as f32,
        rng.range(2) as f32,
    );

    match display {
        Display::Flex => {
            style.flex_direction = [
                FlexDirection::Row,
                FlexDirection::Column,
                FlexDirection::RowReverse,
                FlexDirection::ColumnReverse,
            ][rng.range(4)];
            style.flex_wrap =
                [FlexWrap::NoWrap, FlexWrap::Wrap, FlexWrap::WrapReverse][rng.range(3)];
            style.align_items = random_align_items(rng);
            style.align_content = random_align_content(rng);
            style.justify_content = random_justify_content(rng);
        }
        Display::Linear => {
            style.linear_orientation = [
                LinearOrientation::Horizontal,
                LinearOrientation::Vertical,
                LinearOrientation::HorizontalReverse,
                LinearOrientation::VerticalReverse,
            ][rng.range(4)];
            style.linear_gravity = [
                LinearGravity::None,
                LinearGravity::Center,
                LinearGravity::SpaceBetween,
                LinearGravity::Start,
                LinearGravity::End,
            ][rng.range(5)];
            style.linear_layout_gravity = [
                LinearLayoutGravity::None,
                LinearLayoutGravity::Center,
                LinearLayoutGravity::Stretch,
                LinearLayoutGravity::Start,
                LinearLayoutGravity::End,
            ][rng.range(5)];
        }
        Display::None | Display::Block | Display::Relative | Display::Grid => {}
    }

    style
}

fn random_child_style(rng: &mut DeterministicRng, display: Display, child_index: usize) -> Style {
    let mut style = random_base_style(rng, display);
    style.width = random_axis_length(rng, false);
    style.height = random_axis_length(rng, false);
    let (min_width, max_width) = random_coherent_minmax_lengths(rng);
    let (min_height, max_height) = random_coherent_minmax_lengths(rng);
    style.min_width = min_width;
    style.min_height = min_height;
    style.max_width = max_width;
    style.max_height = max_height;
    style.margin = random_edge_lengths(rng, true);
    style.padding = random_edge_lengths(rng, false);
    style.border = Rect::all(rng.range(2) as f32);
    style.order = child_index as i32 - 1;
    style.flex_basis = random_axis_length(rng, false);
    style.flex_grow = if rng.bool() { 1.0 } else { 0.0 };
    style.flex_shrink = if rng.bool() { 1.0 } else { 0.0 };
    style.linear_weight = 0.0;
    style.align_self = if rng.range(3) == 0 {
        Some(random_align_items(rng))
    } else {
        None
    };
    style.justify_self = random_justify_items(rng);

    style
}

fn random_base_style(rng: &mut DeterministicRng, display: Display) -> Style {
    Style {
        display,
        box_sizing: if rng.bool() {
            BoxSizing::ContentBox
        } else {
            BoxSizing::BorderBox
        },
        direction: LAYOUT_DIRECTIONS[rng.range(LAYOUT_DIRECTIONS.len())],
        row_gap: random_gap_length(rng),
        column_gap: random_gap_length(rng),
        ..Style::default()
    }
}

fn random_child_display(rng: &mut DeterministicRng, child_index: usize) -> Display {
    if child_index == 1 && rng.range(5) == 0 {
        return Display::None;
    }
    [Display::Block, Display::Flex, Display::Linear][rng.range(3)]
}

const RANDOM_AXIS_LENGTH_VARIANT_COUNT: usize = 4;

fn random_axis_length(rng: &mut DeterministicRng, _prefer_definite: bool) -> Length {
    let variant = rng.range(RANDOM_AXIS_LENGTH_VARIANT_COUNT);
    random_axis_length_for_variant(rng, variant)
}

fn random_axis_length_for_variant(rng: &mut DeterministicRng, variant: usize) -> Length {
    match variant {
        0 => Length::Auto,
        1 => Length::points(rng.points(18.0, 6.0, 10)),
        2 => Length::percent(rng.points(20.0, 10.0, 6)),
        3 => Length::fr(rng.points(1.0, 1.0, 4)),
        _ => unreachable!("random axis length variant is out of range"),
    }
}

const RANDOM_MINMAX_LENGTH_VARIANT_COUNT: usize = 6;

fn random_coherent_minmax_lengths(rng: &mut DeterministicRng) -> (Length, Length) {
    let variant = rng.range(RANDOM_MINMAX_LENGTH_VARIANT_COUNT);
    random_coherent_minmax_lengths_for_variant(rng, variant)
}

fn random_coherent_minmax_lengths_for_variant(
    rng: &mut DeterministicRng,
    variant: usize,
) -> (Length, Length) {
    match variant {
        0 => (Length::Auto, Length::Auto),
        1 => (Length::points(rng.points(8.0, 4.0, 4)), Length::Auto),
        2 => {
            let min = rng.points(8.0, 4.0, 4);
            (
                Length::points(min),
                Length::points(min + rng.points(16.0, 4.0, 4)),
            )
        }
        3 => (Length::Auto, Length::points(rng.points(32.0, 8.0, 4))),
        4 => (Length::fr(rng.points(4.0, 2.0, 4)), Length::Auto),
        5 => {
            let min = rng.points(4.0, 2.0, 4);
            (Length::fr(min), Length::fr(min + rng.points(12.0, 2.0, 4)))
        }
        _ => unreachable!("random min/max length variant is out of range"),
    }
}

fn random_edge_lengths(rng: &mut DeterministicRng, allow_auto: bool) -> Rect<Length> {
    fn edge(rng: &mut DeterministicRng, allow_auto: bool) -> Length {
        let _ = allow_auto;
        match rng.range(2) {
            0 => Length::ZERO,
            _ => Length::points(rng.points(1.0, 2.0, 4)),
        }
    }
    Rect::new(
        edge(rng, allow_auto),
        edge(rng, allow_auto),
        edge(rng, allow_auto),
        edge(rng, allow_auto),
    )
}

fn random_gap_length(rng: &mut DeterministicRng) -> Length {
    match rng.range(2) {
        0 => Length::ZERO,
        _ => Length::points(rng.points(1.0, 2.0, 4)),
    }
}

fn random_justify_content(rng: &mut DeterministicRng) -> JustifyContent {
    [
        JustifyContent::FlexStart,
        JustifyContent::Center,
        JustifyContent::FlexEnd,
        JustifyContent::SpaceBetween,
        JustifyContent::SpaceAround,
        JustifyContent::SpaceEvenly,
        JustifyContent::Start,
        JustifyContent::End,
    ][rng.range(8)]
}

fn random_align_items(rng: &mut DeterministicRng) -> AlignItems {
    [
        AlignItems::Stretch,
        AlignItems::FlexStart,
        AlignItems::Center,
        AlignItems::FlexEnd,
        AlignItems::Start,
        AlignItems::End,
    ][rng.range(6)]
}

fn random_align_content(rng: &mut DeterministicRng) -> AlignContent {
    [
        AlignContent::FlexStart,
        AlignContent::Center,
        AlignContent::FlexEnd,
        AlignContent::SpaceBetween,
        AlignContent::SpaceAround,
        AlignContent::Stretch,
    ][rng.range(6)]
}

fn random_justify_items(rng: &mut DeterministicRng) -> JustifyItems {
    [
        JustifyItems::Auto,
        JustifyItems::Stretch,
        JustifyItems::Start,
        JustifyItems::Center,
        JustifyItems::End,
    ][rng.range(5)]
}

fn flex_style(style: Style) -> Style {
    Style {
        display: Display::Flex,
        box_sizing: BoxSizing::ContentBox,
        ..style
    }
}

fn grid_style(style: Style) -> Style {
    Style {
        display: Display::Grid,
        box_sizing: BoxSizing::ContentBox,
        ..style
    }
}

fn linear_style(style: Style) -> Style {
    Style {
        display: Display::Linear,
        box_sizing: BoxSizing::ContentBox,
        ..style
    }
}

fn relative_style(style: Style) -> Style {
    Style {
        display: Display::Relative,
        box_sizing: BoxSizing::ContentBox,
        ..style
    }
}

fn block_style(style: Style) -> Style {
    Style {
        display: Display::Block,
        box_sizing: BoxSizing::ContentBox,
        ..style
    }
}

fn assert_head_to_head_or_skip<T>(
    case_index: usize,
    tree: T,
    root: T::NodeId,
    constraints: Constraints,
) where
    T: BaselineLayoutTree + Clone,
    T::NodeId: Debug,
{
    match run_head_to_head(tree, root, constraints, LayoutTolerance::default()) {
        Ok(_) => {}
        Err(ParityError::CppBaseline(
            CppBaselineError::NativeFeatureDisabled | CppBaselineError::NativeLinkUnavailable,
        )) => {}
        Err(error) => panic!("generated head-to-head case {case_index} failed: {error}"),
    }
}
