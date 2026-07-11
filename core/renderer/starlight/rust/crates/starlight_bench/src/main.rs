// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#![forbid(unsafe_code)]

use std::env;
use std::hint::black_box;
use std::time::{Duration, Instant};

use starlight_cpp::{CppBaselineError, CppStarlightEngine};
use starlight_layout::{
    AlignContent, AlignItems, BaseLength, BoxSizing, Constraints, Direction, Display,
    FlexDirection, FlexWrap, GridAutoFlow, JustifyContent, JustifyItems, LayoutEngine, Length,
    LinearCrossGravity, LinearGravity, LinearLayoutGravity, LinearOrientation, ListComponentType,
    PositionType, Rect, RelativeCenter, SideConstraint, SimpleNode, SimpleTree, Size, Style,
    RELATIVE_ALIGN_PARENT,
};

const DEFAULT_MIN_SPEEDUP: f64 = 1.0;
const ENV_MIN_SPEEDUP: &str = "STARLIGHT_BENCH_MIN_SPEEDUP";
const ENV_REQUIRE_CPP_BASELINE: &str = "STARLIGHT_BENCH_REQUIRE_CPP_BASELINE";

fn main() {
    let args = BenchArgs::from_env();
    let mut failed = false;

    for scenario in BENCH_SCENARIOS {
        assert!(
            !scenario.features.is_empty(),
            "benchmark scenario {} must declare coverage features",
            scenario.name
        );
        let rust_duration = run_rust_benchmark(*scenario, args.nodes, args.iterations, args.warmup);
        print_result("rust_layout", scenario.name, args, rust_duration);

        match run_cpp_benchmark(*scenario, args.nodes, args.iterations, args.warmup) {
            Ok(cpp_duration) => {
                print_result("cpp_layout", scenario.name, args, cpp_duration);
                let speedup = duration_ratio(cpp_duration, rust_duration);
                if speedup <= args.min_speedup {
                    failed = true;
                    eprintln!(
                        "rust layout speedup for case={} is {:.3}, which does not exceed required {:.3}",
                        scenario.name, speedup, args.min_speedup
                    );
                } else {
                    println!(
                        "speedup case={} rust_over_cpp={:.3} min_required={:.3}",
                        scenario.name, speedup, args.min_speedup
                    );
                }
            }
            Err(error) => {
                println!(
                    "cpp_layout case={} unavailable=true reason={error}",
                    scenario.name
                );
                if args.require_cpp_baseline {
                    failed = true;
                    eprintln!(
                        "C++ baseline is required by {ENV_REQUIRE_CPP_BASELINE}, but case={} could not run",
                        scenario.name
                    );
                }
            }
        }
    }

    if failed {
        std::process::exit(1);
    }
}

#[derive(Clone, Copy)]
struct BenchScenario {
    name: &'static str,
    features: &'static [BenchFeature],
    build_tree: fn(usize) -> SimpleTree,
    constraints: fn(usize) -> Constraints,
}

macro_rules! bench_features {
    ($($feature:ident),+ $(,)?) => {
        #[derive(Clone, Copy, Debug, Eq, Ord, PartialEq, PartialOrd)]
        enum BenchFeature {
            $($feature),+
        }

        #[cfg(test)]
        impl BenchFeature {
            const ALL: &'static [Self] = &[$(Self::$feature),+];
        }
    };
}

bench_features! {
    Absolute,
    Alignment,
    AspectRatio,
    AtMostRoot,
    AutoMargin,
    Baseline,
    BaselinePropagation,
    Block,
    BoxSizing,
    Direction,
    DisplayNone,
    FitContent,
    Fixed,
    Flex,
    FlexAxisAlignment,
    FlexDistribution,
    FlexWrap,
    FlexWrapAlignment,
    Grid,
    GridAutoFlow,
    GridContentAlignment,
    GridItemAlignment,
    IntrinsicSizing,
    Linear,
    LinearCrossGravity,
    LinearGravity,
    LinearLayoutGravity,
    ListComponent,
    MeasuredCallbacks,
    MeasuredContent,
    MinMax,
    Order,
    OutOfFlow,
    OwnerConstraints,
    OwnerDirection,
    PercentCalc,
    PositionType,
    Relative,
    RelativeCenter,
    SpacingValues,
    StaggeredLinear,
    Sticky,
}

const BENCH_SCENARIOS: &[BenchScenario] = &[
    BenchScenario {
        name: "flex_grow_row",
        features: &[BenchFeature::Flex],
        build_tree: build_flex_grow_tree,
        constraints: wide_definite_constraints,
    },
    BenchScenario {
        name: "flex_wrap_gaps",
        features: &[BenchFeature::Flex, BenchFeature::FlexWrap],
        build_tree: build_flex_wrap_gap_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "flex_at_most_root",
        features: &[BenchFeature::AtMostRoot, BenchFeature::Flex],
        build_tree: build_flex_at_most_root_tree,
        constraints: at_most_constraints,
    },
    BenchScenario {
        name: "at_most_owner_matrix",
        features: &[
            BenchFeature::AtMostRoot,
            BenchFeature::Block,
            BenchFeature::FitContent,
            BenchFeature::Flex,
            BenchFeature::Grid,
            BenchFeature::Linear,
            BenchFeature::MeasuredContent,
            BenchFeature::MinMax,
            BenchFeature::OwnerConstraints,
            BenchFeature::PercentCalc,
            BenchFeature::Relative,
        ],
        build_tree: build_at_most_owner_matrix_tree,
        constraints: at_most_two_axis_constraints,
    },
    BenchScenario {
        name: "standalone_owner_direction_inheritance",
        features: &[
            BenchFeature::Flex,
            BenchFeature::OwnerConstraints,
            BenchFeature::OwnerDirection,
        ],
        build_tree: build_standalone_owner_direction_inheritance_tree,
        constraints: owner_direction_constraints,
    },
    BenchScenario {
        name: "flex_axis_alignment_matrix",
        features: &[
            BenchFeature::Alignment,
            BenchFeature::Direction,
            BenchFeature::Flex,
            BenchFeature::FlexAxisAlignment,
        ],
        build_tree: build_flex_axis_alignment_matrix_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "flex_distribution_matrix",
        features: &[
            BenchFeature::Direction,
            BenchFeature::Flex,
            BenchFeature::FlexDistribution,
            BenchFeature::MinMax,
        ],
        build_tree: build_flex_distribution_matrix_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "flex_wrap_alignment_matrix",
        features: &[
            BenchFeature::Alignment,
            BenchFeature::Direction,
            BenchFeature::Flex,
            BenchFeature::FlexWrap,
            BenchFeature::FlexWrapAlignment,
        ],
        build_tree: build_flex_wrap_alignment_matrix_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "flex_baseline_measured",
        features: &[
            BenchFeature::Alignment,
            BenchFeature::Baseline,
            BenchFeature::Flex,
            BenchFeature::MeasuredContent,
        ],
        build_tree: build_flex_baseline_measured_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "baseline_propagation_matrix",
        features: &[
            BenchFeature::Alignment,
            BenchFeature::Baseline,
            BenchFeature::BaselinePropagation,
            BenchFeature::Flex,
            BenchFeature::Grid,
            BenchFeature::Linear,
            BenchFeature::MeasuredContent,
            BenchFeature::Relative,
        ],
        build_tree: build_baseline_propagation_matrix_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "measured_callback_matrix",
        features: &[
            BenchFeature::Baseline,
            BenchFeature::Block,
            BenchFeature::FitContent,
            BenchFeature::Flex,
            BenchFeature::Grid,
            BenchFeature::Linear,
            BenchFeature::MeasuredCallbacks,
            BenchFeature::MeasuredContent,
            BenchFeature::MinMax,
            BenchFeature::Relative,
        ],
        build_tree: build_measured_callback_matrix_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "absolute_children",
        features: &[
            BenchFeature::Absolute,
            BenchFeature::Flex,
            BenchFeature::OutOfFlow,
        ],
        build_tree: build_absolute_children_tree,
        constraints: absolute_constraints,
    },
    BenchScenario {
        name: "nested_column_flex",
        features: &[BenchFeature::Flex],
        build_tree: build_nested_column_flex_tree,
        constraints: wide_definite_constraints,
    },
    BenchScenario {
        name: "in_flow_order_matrix",
        features: &[
            BenchFeature::Block,
            BenchFeature::Direction,
            BenchFeature::Flex,
            BenchFeature::Grid,
            BenchFeature::Linear,
            BenchFeature::Order,
        ],
        build_tree: build_in_flow_order_matrix_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "full_value_spacing_matrix",
        features: &[
            BenchFeature::Block,
            BenchFeature::Direction,
            BenchFeature::FitContent,
            BenchFeature::Flex,
            BenchFeature::Grid,
            BenchFeature::IntrinsicSizing,
            BenchFeature::Linear,
            BenchFeature::PercentCalc,
            BenchFeature::PositionType,
            BenchFeature::SpacingValues,
        ],
        build_tree: build_full_value_spacing_matrix_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "staggered_linear_list",
        features: &[
            BenchFeature::Linear,
            BenchFeature::ListComponent,
            BenchFeature::StaggeredLinear,
        ],
        build_tree: build_staggered_linear_list_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "staggered_linear_raw_list_gaps",
        features: &[
            BenchFeature::FitContent,
            BenchFeature::IntrinsicSizing,
            BenchFeature::Linear,
            BenchFeature::ListComponent,
            BenchFeature::StaggeredLinear,
        ],
        build_tree: build_staggered_linear_raw_list_gap_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "linear_gravity_matrix",
        features: &[
            BenchFeature::Direction,
            BenchFeature::Linear,
            BenchFeature::LinearGravity,
        ],
        build_tree: build_linear_gravity_matrix_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "linear_layout_gravity_matrix",
        features: &[
            BenchFeature::Direction,
            BenchFeature::Linear,
            BenchFeature::LinearLayoutGravity,
        ],
        build_tree: build_linear_layout_gravity_matrix_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "linear_cross_gravity_matrix",
        features: &[
            BenchFeature::Direction,
            BenchFeature::Linear,
            BenchFeature::LinearCrossGravity,
        ],
        build_tree: build_linear_cross_gravity_matrix_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "aspect_ratio_blocks",
        features: &[BenchFeature::AspectRatio, BenchFeature::Block],
        build_tree: build_aspect_ratio_block_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "box_sizing_matrix",
        features: &[
            BenchFeature::AspectRatio,
            BenchFeature::Block,
            BenchFeature::BoxSizing,
            BenchFeature::Flex,
            BenchFeature::Grid,
            BenchFeature::Linear,
            BenchFeature::MinMax,
            BenchFeature::Relative,
        ],
        build_tree: build_box_sizing_matrix_tree,
        constraints: wide_definite_constraints,
    },
    BenchScenario {
        name: "fit_content_subtrees",
        features: &[
            BenchFeature::Block,
            BenchFeature::FitContent,
            BenchFeature::Flex,
            BenchFeature::Grid,
            BenchFeature::IntrinsicSizing,
            BenchFeature::Linear,
            BenchFeature::Relative,
        ],
        build_tree: build_fit_content_subtree_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "mixed_position_offsets",
        features: &[
            BenchFeature::Absolute,
            BenchFeature::Fixed,
            BenchFeature::OutOfFlow,
            BenchFeature::PercentCalc,
        ],
        build_tree: build_mixed_position_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "position_type_matrix",
        features: &[
            BenchFeature::Absolute,
            BenchFeature::Fixed,
            BenchFeature::OutOfFlow,
            BenchFeature::PercentCalc,
            BenchFeature::PositionType,
            BenchFeature::Relative,
            BenchFeature::Sticky,
        ],
        build_tree: build_position_type_matrix_tree,
        constraints: absolute_constraints,
    },
    BenchScenario {
        name: "relative_dependency_graph",
        features: &[BenchFeature::Relative],
        build_tree: build_relative_dependency_tree,
        constraints: relative_constraints,
    },
    BenchScenario {
        name: "relative_center_matrix",
        features: &[
            BenchFeature::Alignment,
            BenchFeature::MeasuredContent,
            BenchFeature::Relative,
            BenchFeature::RelativeCenter,
        ],
        build_tree: build_relative_center_matrix_tree,
        constraints: indefinite_constraints,
    },
    BenchScenario {
        name: "sticky_percent_insets",
        features: &[
            BenchFeature::Grid,
            BenchFeature::Linear,
            BenchFeature::PercentCalc,
            BenchFeature::Relative,
            BenchFeature::Sticky,
        ],
        build_tree: build_sticky_percent_inset_tree,
        constraints: sticky_constraints,
    },
    BenchScenario {
        name: "mixed_display_none",
        features: &[
            BenchFeature::Block,
            BenchFeature::DisplayNone,
            BenchFeature::Flex,
            BenchFeature::Grid,
            BenchFeature::Linear,
            BenchFeature::Relative,
        ],
        build_tree: build_mixed_display_none_tree,
        constraints: wrap_constraints,
    },
    BenchScenario {
        name: "out_of_flow_intrinsic",
        features: &[
            BenchFeature::Absolute,
            BenchFeature::Block,
            BenchFeature::FitContent,
            BenchFeature::Fixed,
            BenchFeature::IntrinsicSizing,
            BenchFeature::OutOfFlow,
        ],
        build_tree: build_out_of_flow_intrinsic_tree,
        constraints: absolute_constraints,
    },
    BenchScenario {
        name: "out_of_flow_percent_calc_fill",
        features: &[
            BenchFeature::Absolute,
            BenchFeature::Block,
            BenchFeature::Fixed,
            BenchFeature::MeasuredContent,
            BenchFeature::OutOfFlow,
            BenchFeature::PercentCalc,
        ],
        build_tree: build_out_of_flow_percent_calc_fill_tree,
        constraints: out_of_flow_fill_constraints,
    },
    BenchScenario {
        name: "grid_out_of_flow_intrinsic",
        features: &[
            BenchFeature::Absolute,
            BenchFeature::FitContent,
            BenchFeature::Grid,
            BenchFeature::IntrinsicSizing,
            BenchFeature::OutOfFlow,
        ],
        build_tree: build_grid_out_of_flow_intrinsic_tree,
        constraints: grid_intrinsic_constraints,
    },
    BenchScenario {
        name: "grid_out_of_flow_areas",
        features: &[
            BenchFeature::Absolute,
            BenchFeature::Alignment,
            BenchFeature::Direction,
            BenchFeature::FitContent,
            BenchFeature::Grid,
            BenchFeature::MeasuredContent,
            BenchFeature::OutOfFlow,
            BenchFeature::PercentCalc,
        ],
        build_tree: build_grid_out_of_flow_area_tree,
        constraints: grid_area_constraints,
    },
    BenchScenario {
        name: "grid_item_alignment_matrix",
        features: &[
            BenchFeature::Alignment,
            BenchFeature::Direction,
            BenchFeature::Grid,
            BenchFeature::GridItemAlignment,
        ],
        build_tree: build_grid_item_alignment_matrix_tree,
        constraints: grid_area_constraints,
    },
    BenchScenario {
        name: "grid_content_alignment_matrix",
        features: &[
            BenchFeature::Alignment,
            BenchFeature::Direction,
            BenchFeature::Grid,
            BenchFeature::GridContentAlignment,
        ],
        build_tree: build_grid_content_alignment_matrix_tree,
        constraints: grid_area_constraints,
    },
    BenchScenario {
        name: "grid_auto_flow_matrix",
        features: &[
            BenchFeature::Direction,
            BenchFeature::DisplayNone,
            BenchFeature::Grid,
            BenchFeature::GridAutoFlow,
        ],
        build_tree: build_grid_auto_flow_matrix_tree,
        constraints: grid_auto_flow_constraints,
    },
    BenchScenario {
        name: "grid_auto_margin_alignment",
        features: &[
            BenchFeature::Alignment,
            BenchFeature::AutoMargin,
            BenchFeature::Direction,
            BenchFeature::Grid,
            BenchFeature::MeasuredContent,
        ],
        build_tree: build_grid_auto_margin_alignment_tree,
        constraints: grid_area_constraints,
    },
    BenchScenario {
        name: "grid_minmax_intrinsic_tracks",
        features: &[
            BenchFeature::FitContent,
            BenchFeature::Grid,
            BenchFeature::IntrinsicSizing,
            BenchFeature::MeasuredContent,
            BenchFeature::MinMax,
        ],
        build_tree: build_grid_minmax_intrinsic_tree,
        constraints: grid_intrinsic_constraints,
    },
    BenchScenario {
        name: "grid_auto_fit_content_max_tracks",
        features: &[
            BenchFeature::FitContent,
            BenchFeature::Grid,
            BenchFeature::IntrinsicSizing,
            BenchFeature::MeasuredContent,
            BenchFeature::MinMax,
        ],
        build_tree: build_grid_auto_fit_content_max_tree,
        constraints: grid_intrinsic_constraints,
    },
    BenchScenario {
        name: "grid_indefinite_auto_fit_content_max_tracks",
        features: &[
            BenchFeature::FitContent,
            BenchFeature::Grid,
            BenchFeature::IntrinsicSizing,
            BenchFeature::MeasuredContent,
            BenchFeature::MinMax,
        ],
        build_tree: build_grid_indefinite_auto_fit_content_max_tree,
        constraints: indefinite_constraints,
    },
];

fn print_result(label: &str, case: &str, args: BenchArgs, duration: Duration) {
    println!(
        "{label} case={case} nodes={} iterations={} warmup={} total_ns={} ns_per_iter={}",
        args.nodes,
        args.iterations,
        args.warmup,
        duration.as_nanos(),
        duration.as_nanos() / u128::from(args.iterations)
    );
}

fn duration_ratio(slower: Duration, faster: Duration) -> f64 {
    let faster_nanos = faster.as_nanos().max(1) as f64;
    slower.as_nanos() as f64 / faster_nanos
}

fn direction_for_bench_index(index: usize) -> Direction {
    match index % 2 {
        0 => Direction::Ltr,
        _ => Direction::Rtl,
    }
}

fn owner_direction_for_scenario(scenario: BenchScenario) -> Option<Direction> {
    scenario
        .features
        .contains(&BenchFeature::OwnerDirection)
        .then_some(Direction::Rtl)
}

fn run_rust_benchmark(
    scenario: BenchScenario,
    nodes: usize,
    iterations: u32,
    warmup: u32,
) -> Duration {
    if warmup > 0 {
        run_rust_iterations(scenario, nodes, warmup);
    }

    let start = Instant::now();
    run_rust_iterations(scenario, nodes, iterations);
    start.elapsed()
}

fn run_rust_iterations(scenario: BenchScenario, nodes: usize, iterations: u32) {
    let constraints = (scenario.constraints)(nodes);
    let mut trees = build_trees(scenario, nodes, iterations);
    let mut engine = LayoutEngine::new();
    for tree in &mut trees {
        black_box(run_rust_layout(&mut engine, tree, constraints, scenario));
    }
}

fn run_rust_layout(
    engine: &mut LayoutEngine,
    tree: &mut SimpleTree,
    constraints: Constraints,
    scenario: BenchScenario,
) -> Size {
    if let Some(owner_direction) = owner_direction_for_scenario(scenario) {
        let previous_directions = apply_owner_direction_to_unset_subtree(tree, 0, owner_direction);
        let size = engine.layout_with_owner_constraints(tree, 0, constraints);
        restore_directions(tree, previous_directions);
        size
    } else {
        engine.layout_with_owner_constraints(tree, 0, constraints)
    }
}

fn run_cpp_benchmark(
    scenario: BenchScenario,
    nodes: usize,
    iterations: u32,
    warmup: u32,
) -> Result<Duration, CppBaselineError> {
    if warmup > 0 {
        run_cpp_iterations(scenario, nodes, warmup)?;
    }

    let start = Instant::now();
    run_cpp_iterations(scenario, nodes, iterations)?;
    Ok(start.elapsed())
}

fn run_cpp_iterations(
    scenario: BenchScenario,
    nodes: usize,
    iterations: u32,
) -> Result<(), CppBaselineError> {
    let constraints = (scenario.constraints)(nodes);
    let mut engine = CppStarlightEngine::new();
    let mut trees = build_trees(scenario, nodes, iterations);
    for tree in &mut trees {
        let size = if let Some(owner_direction) = owner_direction_for_scenario(scenario) {
            engine.layout_with_owner_direction(tree, 0, constraints, owner_direction)?
        } else {
            engine.layout(tree, 0, constraints)?
        };
        black_box(size);
    }
    Ok(())
}

fn build_trees(scenario: BenchScenario, nodes: usize, iterations: u32) -> Vec<SimpleTree> {
    (0..iterations)
        .map(|_| (scenario.build_tree)(nodes))
        .collect()
}

fn apply_owner_direction_to_unset_subtree(
    tree: &mut SimpleTree,
    root: usize,
    owner_direction: Direction,
) -> Vec<(usize, Direction)> {
    let mut previous_directions = Vec::new();
    apply_owner_direction_to_unset_subtree_inner(
        tree,
        root,
        standalone_layout_direction(owner_direction),
        &mut previous_directions,
    );
    previous_directions
}

fn apply_owner_direction_to_unset_subtree_inner(
    tree: &mut SimpleTree,
    node: usize,
    layout_direction: Direction,
    previous_directions: &mut Vec<(usize, Direction)>,
) {
    if !tree.nodes[node].has_explicit_direction_style {
        previous_directions.push((node, tree.nodes[node].style.direction));
        tree.nodes[node].style.direction = layout_direction;
    }

    let children = tree.nodes[node].children.clone();
    for child in children {
        apply_owner_direction_to_unset_subtree_inner(
            tree,
            child,
            layout_direction,
            previous_directions,
        );
    }
}

fn restore_directions(tree: &mut SimpleTree, previous_directions: Vec<(usize, Direction)>) {
    for (node, direction) in previous_directions {
        tree.nodes[node].style.direction = direction;
    }
}

fn standalone_layout_direction(owner_direction: Direction) -> Direction {
    if owner_direction.is_any_rtl() {
        Direction::Rtl
    } else {
        Direction::Ltr
    }
}

fn wide_definite_constraints(nodes: usize) -> Constraints {
    Constraints::new(
        SideConstraint::definite(nodes.max(1) as f32),
        SideConstraint::indefinite(),
    )
}

fn wrap_constraints(_nodes: usize) -> Constraints {
    Constraints::new(
        SideConstraint::definite(320.0),
        SideConstraint::indefinite(),
    )
}

fn owner_direction_constraints(nodes: usize) -> Constraints {
    Constraints::definite(30.0, nodes.max(1) as f32 * 20.0)
}

fn at_most_constraints(nodes: usize) -> Constraints {
    Constraints::new(
        SideConstraint::at_most(nodes.max(1) as f32 * 4.0),
        SideConstraint::indefinite(),
    )
}

fn at_most_two_axis_constraints(_nodes: usize) -> Constraints {
    Constraints::new(
        SideConstraint::at_most(320.0),
        SideConstraint::at_most(220.0),
    )
}

fn absolute_constraints(nodes: usize) -> Constraints {
    let rows = absolute_rows(nodes.max(1));
    Constraints::definite(320.0, rows as f32 * 4.0 + 4.0)
}

fn relative_constraints(_nodes: usize) -> Constraints {
    Constraints::definite(320.0, 160.0)
}

fn sticky_constraints(_nodes: usize) -> Constraints {
    Constraints::definite(320.0, 240.0)
}

fn out_of_flow_fill_constraints(_nodes: usize) -> Constraints {
    Constraints::definite(320.0, 240.0)
}

fn grid_intrinsic_constraints(_nodes: usize) -> Constraints {
    Constraints::definite(320.0, 160.0)
}

fn grid_area_constraints(_nodes: usize) -> Constraints {
    Constraints::definite(340.0, 320.0)
}

fn grid_auto_flow_constraints(_nodes: usize) -> Constraints {
    Constraints::definite(340.0, 320.0)
}

fn indefinite_constraints(_nodes: usize) -> Constraints {
    Constraints::indefinite()
}

fn build_standalone_owner_direction_inheritance_tree(nodes: usize) -> SimpleTree {
    let rows = nodes.max(1);
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::with_inherited_direction_style(Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Column,
        align_items: AlignItems::FlexStart,
        width: Length::points(30.0),
        height: Length::points(rows as f32 * 20.0),
        ..Style::default()
    }));

    for index in 0..rows {
        let inherited_container = tree.push(SimpleNode::with_inherited_direction_style(
            owner_direction_row_style(Direction::Ltr, index),
        ));
        let inherited_leaf = tree.push(SimpleNode::with_inherited_direction_style(
            owner_direction_leaf_style(index),
        ));
        tree.append_child(root, inherited_container);
        tree.append_child(inherited_container, inherited_leaf);

        let explicit_ltr_container = tree.push(SimpleNode::new(owner_direction_row_style(
            Direction::Ltr,
            index,
        )));
        let explicit_ltr_leaf = tree.push(SimpleNode::with_inherited_direction_style(
            owner_direction_leaf_style(index),
        ));
        tree.append_child(root, explicit_ltr_container);
        tree.append_child(explicit_ltr_container, explicit_ltr_leaf);
    }

    tree
}

fn owner_direction_row_style(direction: Direction, index: usize) -> Style {
    Style {
        display: Display::Flex,
        direction,
        flex_direction: FlexDirection::Row,
        justify_content: JustifyContent::FlexStart,
        align_items: AlignItems::FlexStart,
        width: Length::points(30.0),
        height: Length::points(10.0 + (index % 2) as f32),
        ..Style::default()
    }
}

fn owner_direction_leaf_style(index: usize) -> Style {
    Style {
        width: Length::points(10.0 + (index % 3) as f32),
        height: Length::points(5.0 + (index % 2) as f32),
        ..Style::default()
    }
}

fn build_flex_grow_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        height: Length::points(10.0),
        justify_content: JustifyContent::FlexStart,
        align_items: AlignItems::Stretch,
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let child = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            flex_basis: Length::points(1.0),
            flex_grow: 1.0 + (index % 3) as f32,
            height: Length::points(10.0),
            ..Style::default()
        }));
        tree.append_child(root, child);
    }

    tree
}

fn build_flex_at_most_root_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: JustifyContent::FlexStart,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let child = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            flex_basis: Length::points(1.0 + (index % 4) as f32),
            height: Length::points(4.0 + (index % 2) as f32),
            ..Style::default()
        }));
        tree.append_child(root, child);
    }

    tree
}

fn build_at_most_owner_matrix_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(12.0, 80.0))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(8.0, 70.0))),
        min_width: Length::percent(35.0),
        max_width: Length::calc(36.0, 80.0),
        min_height: Length::points(48.0),
        max_height: Length::calc(20.0, 85.0),
        padding: Rect::all(Length::points(1.0)),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let container = tree.push(SimpleNode::new(at_most_owner_container_style(index)));
        tree.append_child(root, container);

        for child_index in 0..3 {
            let child = tree.push(SimpleNode::with_measured_size(
                at_most_owner_child_style(index, child_index),
                Size::new(
                    20.0 + (index % 7) as f32 + child_index as f32 * 4.0,
                    10.0 + (index % 5) as f32 + child_index as f32 * 3.0,
                ),
            ));
            tree.append_child(container, child);
        }
    }

    tree
}

fn at_most_owner_container_style(index: usize) -> Style {
    let display = match index % 5 {
        0 => Display::Block,
        1 => Display::Flex,
        2 => Display::Linear,
        3 => Display::Grid,
        _ => Display::Relative,
    };
    let mut style = Style {
        display,
        width: if index.is_multiple_of(2) {
            Length::percent(42.0 + (index % 5) as f32)
        } else {
            Length::fit_content(Some(BaseLength::fixed_and_percent(
                18.0 + (index % 4) as f32,
                45.0,
            )))
        },
        height: if index.is_multiple_of(3) {
            Length::Auto
        } else {
            Length::fit_content(Some(BaseLength::fixed(34.0 + (index % 6) as f32)))
        },
        min_width: Length::points(36.0),
        max_width: Length::calc(24.0, 60.0),
        min_height: Length::points(18.0),
        max_height: Length::calc(12.0, 70.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        padding: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(1.0),
            Length::points(2.0),
        ),
        ..Style::default()
    };

    match display {
        Display::Flex => {
            style.flex_wrap = FlexWrap::Wrap;
            style.justify_content = JustifyContent::FlexStart;
            style.align_items = AlignItems::FlexStart;
            style.align_content = AlignContent::FlexStart;
            style.column_gap = Length::points(1.0);
            style.row_gap = Length::points(1.0);
        }
        Display::Linear => {
            style.linear_orientation = if index.is_multiple_of(2) {
                LinearOrientation::Horizontal
            } else {
                LinearOrientation::Vertical
            };
            style.linear_cross_gravity = LinearCrossGravity::Start;
        }
        Display::Grid => {
            style.grid_template_columns = vec![Length::points(30.0), Length::Auto];
            style.grid_template_rows = vec![Length::points(14.0), Length::Auto];
            style.grid_auto_flow = if (index / 5).is_multiple_of(2) {
                GridAutoFlow::Row
            } else {
                GridAutoFlow::Column
            };
            style.justify_items = JustifyItems::Start;
            style.align_items = AlignItems::FlexStart;
            style.column_gap = Length::points(1.0);
            style.row_gap = Length::points(1.0);
        }
        Display::Relative => {}
        Display::Block | Display::None => {}
    }

    style
}

fn at_most_owner_child_style(index: usize, child_index: usize) -> Style {
    let mut style = Style {
        display: Display::Block,
        width: match child_index {
            0 => Length::Auto,
            1 => Length::fit_content(Some(BaseLength::fixed_and_percent(6.0, 40.0))),
            _ => Length::percent(35.0),
        },
        height: match child_index {
            0 => Length::fit_content(Some(BaseLength::fixed(18.0))),
            1 => Length::Auto,
            _ => Length::percent(30.0),
        },
        min_width: Length::points(12.0 + child_index as f32 * 2.0),
        max_width: Length::calc(18.0 + child_index as f32 * 3.0, 45.0),
        min_height: Length::points(8.0 + child_index as f32),
        max_height: Length::calc(10.0 + child_index as f32 * 2.0, 55.0),
        flex_basis: Length::Auto,
        margin: Rect::new(
            Length::points((child_index % 2) as f32),
            Length::points((child_index % 3) as f32 * 0.5),
            Length::ZERO,
            Length::ZERO,
        ),
        ..Style::default()
    };

    if index % 5 == 4 {
        style.relative_center = match child_index {
            0 => RelativeCenter::Horizontal,
            1 => RelativeCenter::Vertical,
            _ => RelativeCenter::Both,
        };
        style.relative_align_left = RELATIVE_ALIGN_PARENT;
        style.relative_align_top = RELATIVE_ALIGN_PARENT;
    }

    style
}

fn build_flex_axis_alignment_matrix_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(320.0),
        height: Length::points(nodes.max(1) as f32 * 88.0 + 8.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let flex_direction = flex_direction_for_bench_index(index);
        let container = tree.push(SimpleNode::new(flex_axis_alignment_container_style(
            index,
            flex_direction,
        )));
        tree.append_child(root, container);

        for child_index in 0..3 {
            let child = tree.push(SimpleNode::new(flex_axis_alignment_child_style(
                flex_direction,
                child_index,
            )));
            tree.append_child(container, child);
        }
    }

    tree
}

fn flex_axis_alignment_container_style(index: usize, flex_direction: FlexDirection) -> Style {
    Style {
        display: Display::Flex,
        direction: direction_for_bench_index(index),
        flex_direction,
        justify_content: justify_content_for_bench_index(index),
        align_items: align_items_for_bench_index(index),
        width: Length::points(120.0),
        height: Length::points(80.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    }
}

fn flex_axis_alignment_child_style(flex_direction: FlexDirection, child_index: usize) -> Style {
    let basis = [18.0, 24.0, 30.0][child_index];
    let cross = [8.0, 12.0, 16.0][child_index];
    let auto_cross = child_index == 1;
    Style {
        display: Display::Flex,
        flex_basis: Length::points(basis),
        width: if flex_direction.is_row() {
            Length::points(basis)
        } else if auto_cross {
            Length::Auto
        } else {
            Length::points(cross)
        },
        height: if flex_direction.is_row() {
            if auto_cross {
                Length::Auto
            } else {
                Length::points(cross)
            }
        } else {
            Length::points(basis)
        },
        margin: Rect::new(
            Length::points(child_index as f32),
            Length::points((2 - child_index) as f32),
            Length::ZERO,
            Length::points((child_index % 2) as f32),
        ),
        ..Style::default()
    }
}

fn flex_direction_for_bench_index(index: usize) -> FlexDirection {
    match index % 4 {
        0 => FlexDirection::Row,
        1 => FlexDirection::RowReverse,
        2 => FlexDirection::Column,
        _ => FlexDirection::ColumnReverse,
    }
}

fn justify_content_for_bench_index(index: usize) -> JustifyContent {
    match index % 9 {
        0 => JustifyContent::Stretch,
        1 => JustifyContent::FlexStart,
        2 => JustifyContent::Start,
        3 => JustifyContent::Center,
        4 => JustifyContent::FlexEnd,
        5 => JustifyContent::End,
        6 => JustifyContent::SpaceBetween,
        7 => JustifyContent::SpaceAround,
        _ => JustifyContent::SpaceEvenly,
    }
}

fn align_items_for_bench_index(index: usize) -> AlignItems {
    match (index / 9) % 7 {
        0 => AlignItems::Stretch,
        1 => AlignItems::FlexStart,
        2 => AlignItems::Start,
        3 => AlignItems::Center,
        4 => AlignItems::FlexEnd,
        5 => AlignItems::End,
        _ => AlignItems::Baseline,
    }
}

fn build_flex_distribution_matrix_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(320.0),
        height: Length::points(nodes.max(1) as f32 * 86.0 + 8.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let flex_direction = flex_direction_for_bench_index(index);
        let container = tree.push(SimpleNode::new(flex_distribution_container_style(
            index,
            flex_direction,
        )));
        tree.append_child(root, container);

        for child_index in 0..5 {
            let child = tree.push(SimpleNode::new(flex_distribution_child_style(
                index,
                flex_direction,
                child_index,
            )));
            tree.append_child(container, child);
        }
    }

    tree
}

fn flex_distribution_container_style(index: usize, flex_direction: FlexDirection) -> Style {
    let grow_case = index.is_multiple_of(2);
    let main_size = if grow_case { 178.0 } else { 94.0 };
    let cross_size = 58.0;
    let (width, height) = if flex_direction.is_row() {
        (main_size, cross_size)
    } else {
        (cross_size, main_size)
    };

    Style {
        display: Display::Flex,
        direction: direction_for_bench_index(index),
        flex_direction,
        flex_wrap: FlexWrap::NoWrap,
        justify_content: JustifyContent::FlexStart,
        align_items: AlignItems::FlexStart,
        width: Length::points(width),
        height: Length::points(height),
        row_gap: Length::points(1.0),
        column_gap: Length::points(1.0),
        padding: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(1.0),
            Length::points(2.0),
        ),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    }
}

fn flex_distribution_child_style(
    index: usize,
    flex_direction: FlexDirection,
    child_index: usize,
) -> Style {
    let basis = [
        Length::points(18.0),
        Length::points(28.0),
        Length::points(36.0),
        Length::points(22.0),
        Length::percent(18.0),
    ][child_index];
    let grow = [0.0, 1.0, 2.0, 1.5, 0.5][child_index];
    let shrink = [1.0, 2.0, 0.5, 1.5, 0.0][child_index];
    let order = [-1, 2, 0, 3, -2][child_index] + (index % 2) as i32;
    let cross_size = 12.0 + child_index as f32 * 2.0;
    let (min_main, max_main) = match child_index {
        0 => (Length::points(24.0), Length::Auto),
        1 => (Length::Auto, Length::points(32.0)),
        2 => (Length::percent(22.0), Length::Auto),
        3 => (Length::Auto, Length::percent(24.0)),
        _ => (Length::Auto, Length::Auto),
    };

    let mut style = Style {
        display: Display::Block,
        flex_basis: basis,
        flex_grow: grow,
        flex_shrink: shrink,
        order,
        margin: Rect::new(
            Length::points((child_index % 2) as f32),
            Length::points((child_index % 3) as f32 * 0.5),
            Length::ZERO,
            Length::ZERO,
        ),
        ..Style::default()
    };

    if flex_direction.is_row() {
        style.height = Length::points(cross_size);
        style.min_width = min_main;
        style.max_width = max_main;
    } else {
        style.width = Length::points(cross_size);
        style.min_height = min_main;
        style.max_height = max_main;
    }

    style
}

fn build_flex_wrap_alignment_matrix_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(320.0),
        height: Length::points(nodes.max(1) as f32 * 72.0 + 8.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let flex_direction = flex_direction_for_bench_index(index);
        let container = tree.push(SimpleNode::new(flex_wrap_alignment_container_style(
            index,
            flex_direction,
        )));
        tree.append_child(root, container);

        for child_index in 0..4 {
            let child = tree.push(SimpleNode::new(flex_wrap_alignment_child_style(
                flex_direction,
                child_index,
            )));
            tree.append_child(container, child);
        }
    }

    tree
}

fn flex_wrap_alignment_container_style(index: usize, flex_direction: FlexDirection) -> Style {
    Style {
        display: Display::Flex,
        direction: direction_for_bench_index(index),
        flex_direction,
        flex_wrap: flex_wrap_for_bench_index(index),
        justify_content: JustifyContent::FlexStart,
        align_content: align_content_for_bench_index(index),
        align_items: align_items_for_bench_index(index),
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
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    }
}

fn flex_wrap_alignment_child_style(flex_direction: FlexDirection, child_index: usize) -> Style {
    let (width, height) = [(28.0, 16.0), (34.0, 12.0), (20.0, 18.0), (25.0, 14.0)][child_index];
    Style {
        display: Display::Flex,
        flex_basis: if flex_direction.is_row() {
            Length::points(width)
        } else {
            Length::points(height)
        },
        width: Length::points(width),
        height: Length::points(height),
        margin: Rect::new(
            Length::points(child_index as f32 % 2.0),
            Length::points((child_index % 3) as f32),
            Length::points((child_index % 2) as f32),
            Length::ZERO,
        ),
        ..Style::default()
    }
}

fn flex_wrap_for_bench_index(index: usize) -> FlexWrap {
    match index % 3 {
        0 => FlexWrap::NoWrap,
        1 => FlexWrap::Wrap,
        _ => FlexWrap::WrapReverse,
    }
}

fn align_content_for_bench_index(index: usize) -> AlignContent {
    match index % 9 {
        0 => AlignContent::FlexStart,
        1 => AlignContent::Start,
        2 => AlignContent::Center,
        3 => AlignContent::FlexEnd,
        4 => AlignContent::End,
        5 => AlignContent::SpaceBetween,
        6 => AlignContent::SpaceAround,
        7 => AlignContent::SpaceEvenly,
        _ => AlignContent::Stretch,
    }
}

fn build_flex_wrap_gap_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        flex_wrap: FlexWrap::Wrap,
        width: Length::points(320.0),
        row_gap: Length::points(1.0),
        column_gap: Length::points(1.0),
        justify_content: JustifyContent::FlexStart,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let child = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            flex_basis: Length::points(16.0 + (index % 5) as f32),
            height: Length::points(6.0 + (index % 3) as f32),
            ..Style::default()
        }));
        tree.append_child(root, child);
    }

    tree
}

fn build_flex_baseline_measured_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(320.0),
        align_items: AlignItems::Baseline,
        justify_content: JustifyContent::FlexStart,
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let width = 8.0 + (index % 7) as f32;
        let height = 6.0 + (index % 11) as f32;
        let baseline = 2.0 + (index % 5) as f32;
        let child = tree.push(SimpleNode::with_measured_size_and_baseline(
            Style {
                display: Display::Block,
                align_self: (index % 3 == 0).then_some(AlignItems::Baseline),
                margin: Rect::new(
                    Length::points((index % 2) as f32),
                    Length::points((index % 3) as f32),
                    Length::points((index % 4) as f32 * 0.5),
                    Length::points((index % 5) as f32 * 0.25),
                ),
                ..Style::default()
            },
            Size::new(width, height),
            baseline.min(height),
        ));
        tree.append_child(root, child);
    }

    tree
}

fn build_baseline_propagation_matrix_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(320.0),
        height: Length::points(nodes.max(1) as f32 * 54.0 + 8.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let container = tree.push(SimpleNode::new(baseline_propagation_container_style(index)));
        let reference = tree.push(SimpleNode::with_measured_size_and_baseline(
            Style {
                display: Display::Block,
                flex_basis: Length::points(12.0),
                width: Length::points(12.0),
                height: Length::points(32.0),
                margin: Rect::new(
                    Length::points(1.0),
                    Length::points(1.0),
                    Length::points(2.0),
                    Length::points(1.0),
                ),
                ..Style::default()
            },
            Size::new(12.0, 32.0),
            26.0,
        ));
        let candidate = append_baseline_propagation_source(&mut tree, index);
        let trailing = tree.push(SimpleNode::with_measured_size(
            Style {
                display: Display::Block,
                flex_basis: Length::points(10.0),
                width: Length::points(10.0),
                height: Length::points(12.0),
                margin: Rect::new(
                    Length::ZERO,
                    Length::points(1.0),
                    Length::ZERO,
                    Length::ZERO,
                ),
                ..Style::default()
            },
            Size::new(10.0, 12.0),
        ));

        tree.append_child(root, container);
        for child in [reference, candidate, trailing] {
            tree.append_child(container, child);
        }
    }

    tree
}

fn baseline_propagation_container_style(index: usize) -> Style {
    Style {
        display: Display::Flex,
        flex_direction: FlexDirection::Row,
        align_items: if index.is_multiple_of(2) {
            AlignItems::Baseline
        } else {
            AlignItems::FlexStart
        },
        justify_content: JustifyContent::FlexStart,
        width: Length::points(116.0),
        height: Length::points(48.0),
        padding: Rect::new(
            Length::points(1.0),
            Length::points(2.0),
            Length::points(1.0),
            Length::points(2.0),
        ),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    }
}

fn baseline_candidate_trigger_style(index: usize) -> Style {
    Style {
        align_self: (!index.is_multiple_of(2)).then_some(AlignItems::Baseline),
        ..Style::default()
    }
}

fn append_baseline_propagation_source(tree: &mut SimpleTree, index: usize) -> usize {
    match index % 6 {
        0 => tree.push(SimpleNode::with_measured_size_and_baseline(
            Style {
                display: Display::Block,
                flex_basis: Length::points(18.0),
                width: Length::points(18.0),
                height: Length::points(22.0),
                margin: Rect::new(
                    Length::points(1.0),
                    Length::points(2.0),
                    Length::points(1.0),
                    Length::points(2.0),
                ),
                ..baseline_candidate_trigger_style(index)
            },
            Size::new(18.0, 22.0),
            16.0,
        )),
        1 => {
            let nested = tree.push(SimpleNode::new(Style {
                display: Display::Flex,
                align_items: AlignItems::Baseline,
                flex_basis: Length::points(28.0),
                width: Length::points(28.0),
                height: Length::points(26.0),
                margin: Rect::new(
                    Length::points(1.0),
                    Length::points(1.0),
                    Length::points(2.0),
                    Length::points(1.0),
                ),
                ..baseline_candidate_trigger_style(index)
            }));
            append_baseline_propagation_children(tree, nested, 6.0, 18.0);
            nested
        }
        2 => {
            let nested = tree.push(SimpleNode::new(Style {
                display: Display::Flex,
                flex_direction: FlexDirection::Column,
                justify_content: JustifyContent::Center,
                flex_basis: Length::points(26.0),
                width: Length::points(26.0),
                height: Length::points(40.0),
                align_items: AlignItems::FlexStart,
                margin: Rect::new(
                    Length::points(2.0),
                    Length::points(1.0),
                    Length::points(2.0),
                    Length::points(1.0),
                ),
                ..baseline_candidate_trigger_style(index)
            }));
            append_baseline_propagation_children(tree, nested, 7.0, 15.0);
            nested
        }
        3 => {
            let nested = tree.push(SimpleNode::new(Style {
                display: Display::Linear,
                linear_orientation: LinearOrientation::Horizontal,
                flex_basis: Length::points(30.0),
                width: Length::points(30.0),
                height: Length::points(26.0),
                margin: Rect::new(
                    Length::points(2.0),
                    Length::points(1.0),
                    Length::points(1.0),
                    Length::points(2.0),
                ),
                ..baseline_candidate_trigger_style(index)
            }));
            append_baseline_propagation_children(tree, nested, 8.0, 19.0);
            nested
        }
        4 => {
            let nested = tree.push(SimpleNode::new(Style {
                display: Display::Grid,
                flex_basis: Length::points(26.0),
                width: Length::points(26.0),
                height: Length::points(20.0),
                grid_template_columns: vec![Length::points(26.0)],
                grid_template_rows: vec![Length::points(20.0)],
                align_items: AlignItems::Baseline,
                margin: Rect::new(
                    Length::points(1.0),
                    Length::points(2.0),
                    Length::points(2.0),
                    Length::points(1.0),
                ),
                ..baseline_candidate_trigger_style(index)
            }));
            let child = tree.push(SimpleNode::with_measured_size_and_baseline(
                Style {
                    display: Display::Block,
                    width: Length::points(11.0),
                    height: Length::points(9.0),
                    grid_column_start: Some(1),
                    grid_row_start: Some(1),
                    ..Style::default()
                },
                Size::new(11.0, 9.0),
                6.0,
            ));
            tree.append_child(nested, child);
            nested
        }
        _ => {
            let nested = tree.push(SimpleNode::new(Style {
                display: Display::Relative,
                flex_basis: Length::points(24.0),
                width: Length::points(24.0),
                height: Length::points(18.0),
                margin: Rect::new(
                    Length::points(2.0),
                    Length::points(1.0),
                    Length::points(2.0),
                    Length::points(1.0),
                ),
                ..baseline_candidate_trigger_style(index)
            }));
            let child = tree.push(SimpleNode::with_measured_size(
                Style {
                    display: Display::Block,
                    relative_align_left: RELATIVE_ALIGN_PARENT,
                    relative_align_top: RELATIVE_ALIGN_PARENT,
                    ..Style::default()
                },
                Size::new(12.0, 9.0),
            ));
            tree.append_child(nested, child);
            nested
        }
    }
}

fn append_baseline_propagation_children(
    tree: &mut SimpleTree,
    parent: usize,
    first_baseline: f32,
    second_baseline: f32,
) {
    let first = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style {
            display: Display::Block,
            width: Length::points(10.0),
            height: Length::points(18.0),
            margin: Rect::new(
                Length::points(1.0),
                Length::ZERO,
                Length::points(1.0),
                Length::points(2.0),
            ),
            ..Style::default()
        },
        Size::new(10.0, 18.0),
        first_baseline,
    ));
    let second = tree.push(SimpleNode::with_measured_size_and_baseline(
        Style {
            display: Display::Block,
            width: Length::points(12.0),
            height: Length::points(24.0),
            margin: Rect::new(
                Length::ZERO,
                Length::points(1.0),
                Length::points(2.0),
                Length::points(1.0),
            ),
            ..Style::default()
        },
        Size::new(12.0, 24.0),
        second_baseline,
    ));
    tree.append_child(parent, first);
    tree.append_child(parent, second);
}

fn build_measured_callback_matrix_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(320.0),
        height: Length::points(nodes.max(1) as f32 * 70.0 + 8.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let container = tree.push(SimpleNode::new(measured_callback_container_style(index)));
        tree.append_child(root, container);

        for child_index in 0..4 {
            let style = measured_callback_child_style(index, child_index);
            let measured_size = Size::new(
                18.0 + (index % 7) as f32 + child_index as f32 * 3.0,
                9.0 + (index % 5) as f32 + child_index as f32 * 2.0,
            );
            let child = match child_index {
                0 => tree.push(SimpleNode::with_measure_func_and_baseline(
                    style,
                    bench_measure_callback,
                    bench_baseline_callback,
                )),
                1 => tree.push(SimpleNode::with_measure_func(style, bench_measure_callback)),
                2 => tree.push(SimpleNode::with_measured_size_and_baseline(
                    style,
                    measured_size,
                    (4.0 + child_index as f32 * 2.0).min(measured_size.height),
                )),
                _ => tree.push(SimpleNode::with_measured_size(style, measured_size)),
            };
            tree.append_child(container, child);
        }
    }

    tree
}

fn measured_callback_container_style(index: usize) -> Style {
    let display = match index % 5 {
        0 => Display::Block,
        1 => Display::Flex,
        2 => Display::Linear,
        3 => Display::Grid,
        _ => Display::Relative,
    };
    let mut style = Style {
        display,
        width: if index.is_multiple_of(3) {
            Length::fit_content(Some(BaseLength::fixed(126.0)))
        } else {
            Length::points(136.0)
        },
        height: if index.is_multiple_of(4) {
            Length::fit_content(Some(BaseLength::fixed(44.0)))
        } else {
            Length::points(58.0)
        },
        min_width: Length::points(72.0),
        max_width: Length::points(180.0),
        min_height: Length::points(28.0),
        max_height: Length::points(92.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all((index % 2) as f32 * 0.5),
        ..Style::default()
    };

    match display {
        Display::Flex => {
            style.flex_wrap = FlexWrap::Wrap;
            style.align_items = AlignItems::Baseline;
            style.justify_content = JustifyContent::FlexStart;
            style.align_content = AlignContent::FlexStart;
            style.column_gap = Length::points(1.0);
            style.row_gap = Length::points(1.0);
        }
        Display::Linear => {
            style.linear_orientation = if index.is_multiple_of(2) {
                LinearOrientation::Horizontal
            } else {
                LinearOrientation::Vertical
            };
            style.linear_cross_gravity = LinearCrossGravity::Start;
        }
        Display::Grid => {
            style.grid_template_columns = vec![Length::points(32.0), Length::Auto];
            style.grid_template_rows = vec![Length::points(16.0), Length::Auto];
            style.justify_items = JustifyItems::Start;
            style.align_items = AlignItems::FlexStart;
            style.column_gap = Length::points(1.0);
            style.row_gap = Length::points(1.0);
        }
        Display::Relative => {}
        Display::Block | Display::None => {}
    }

    style
}

fn measured_callback_child_style(index: usize, child_index: usize) -> Style {
    let mut style = Style {
        display: Display::Block,
        width: if child_index == 0 {
            Length::fit_content(Some(BaseLength::fixed(36.0)))
        } else {
            Length::Auto
        },
        height: if child_index == 1 {
            Length::fit_content(Some(BaseLength::fixed(18.0)))
        } else {
            Length::Auto
        },
        min_width: if child_index == 2 {
            Length::points(20.0)
        } else {
            Length::Auto
        },
        max_width: if child_index == 3 {
            Length::points(54.0)
        } else {
            Length::Auto
        },
        min_height: if child_index == 1 {
            Length::points(10.0)
        } else {
            Length::Auto
        },
        max_height: if child_index == 2 {
            Length::points(32.0)
        } else {
            Length::Auto
        },
        flex_basis: Length::Auto,
        align_self: child_index
            .is_multiple_of(2)
            .then_some(AlignItems::Baseline),
        margin: Rect::new(
            Length::points((child_index % 2) as f32),
            Length::points((child_index % 3) as f32 * 0.5),
            Length::points((index % 2) as f32 * 0.5),
            Length::ZERO,
        ),
        ..Style::default()
    };

    if index % 5 == 4 {
        style.relative_center = match child_index % 4 {
            0 => RelativeCenter::None,
            1 => RelativeCenter::Horizontal,
            2 => RelativeCenter::Vertical,
            _ => RelativeCenter::Both,
        };
        style.relative_align_left = RELATIVE_ALIGN_PARENT;
        style.relative_align_top = RELATIVE_ALIGN_PARENT;
    }

    style
}

fn bench_measure_callback(constraints: Constraints) -> Size {
    let width = constraints
        .width
        .bounded_size()
        .map_or(24.0, |size| (size - 3.0).max(1.0));
    let height = constraints
        .height
        .bounded_size()
        .map_or(12.0, |size| (size - 2.0).max(1.0));
    Size::new(width, height)
}

fn bench_baseline_callback(content_size: Size) -> f32 {
    (content_size.height - 3.0).max(0.0)
}

fn build_absolute_children_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(320.0),
        height: Length::points(absolute_rows(nodes.max(1)) as f32 * 4.0 + 4.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let child = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            position: PositionType::Absolute,
            left: Length::points((index % 64) as f32 * 5.0),
            top: Length::points((index / 64) as f32 * 4.0),
            width: Length::points(4.0),
            height: Length::points(3.0),
            ..Style::default()
        }));
        tree.append_child(root, child);
    }

    tree
}

fn absolute_rows(nodes: usize) -> usize {
    nodes.div_ceil(64)
}

fn build_nested_column_flex_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        justify_content: JustifyContent::FlexStart,
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));

    let branch_count = integer_sqrt_ceil(nodes.max(1)).max(1);
    let leaves_per_branch = nodes.max(1).div_ceil(branch_count);
    let mut emitted = 0;

    for branch_index in 0..branch_count {
        let branch = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            flex_direction: FlexDirection::Column,
            flex_basis: Length::points(8.0 + (branch_index % 4) as f32),
            row_gap: Length::points(0.5),
            align_items: AlignItems::FlexStart,
            ..Style::default()
        }));
        tree.append_child(root, branch);

        for leaf_index in 0..leaves_per_branch {
            if emitted >= nodes.max(1) {
                break;
            }
            emitted += 1;
            let leaf = tree.push(SimpleNode::new(Style {
                display: Display::Flex,
                width: Length::points(4.0 + (leaf_index % 3) as f32),
                height: Length::points(2.0),
                ..Style::default()
            }));
            tree.append_child(branch, leaf);
        }
    }

    tree
}

fn build_in_flow_order_matrix_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(320.0),
        height: Length::points(nodes.max(1) as f32 * 60.0 + 8.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let container = tree.push(SimpleNode::new(in_flow_order_container_style(index)));
        tree.append_child(root, container);

        for child_index in 0..5 {
            let child = tree.push(SimpleNode::new(in_flow_order_child_style(
                index,
                child_index,
            )));
            tree.append_child(container, child);
        }
    }

    tree
}

fn in_flow_order_container_style(index: usize) -> Style {
    let display = match index % 4 {
        0 => Display::Block,
        1 => Display::Flex,
        2 => Display::Linear,
        _ => Display::Grid,
    };
    let mut style = Style {
        display,
        direction: direction_for_bench_index(index),
        width: Length::points(122.0),
        height: Length::points(52.0),
        padding: Rect::all(Length::points(1.0)),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    };

    match display {
        Display::Flex => {
            style.flex_direction = flex_direction_for_bench_index(index);
            style.justify_content = JustifyContent::FlexStart;
            style.align_items = AlignItems::FlexStart;
            style.column_gap = Length::points(1.0);
        }
        Display::Linear => {
            style.linear_orientation = if index.is_multiple_of(2) {
                LinearOrientation::Horizontal
            } else {
                LinearOrientation::Vertical
            };
            style.linear_gravity = LinearGravity::None;
            style.linear_layout_gravity = LinearLayoutGravity::None;
        }
        Display::Grid => {
            style.grid_template_columns = vec![Length::points(24.0), Length::points(28.0)];
            style.grid_template_rows = vec![Length::points(12.0), Length::points(14.0)];
            style.grid_auto_flow = if (index / 4).is_multiple_of(2) {
                GridAutoFlow::Row
            } else {
                GridAutoFlow::Column
            };
            style.column_gap = Length::points(1.0);
            style.row_gap = Length::points(1.0);
        }
        Display::Block | Display::None | Display::Relative => {}
    }

    style
}

fn in_flow_order_child_style(index: usize, child_index: usize) -> Style {
    let order = [-2, 3, 0, 1, -1][child_index] + (index % 3) as i32 - 1;
    Style {
        display: Display::Block,
        order,
        flex_basis: Length::points(14.0 + child_index as f32 * 2.0),
        width: Length::points(14.0 + child_index as f32 * 2.0),
        height: Length::points(8.0 + (child_index % 3) as f32),
        margin: Rect::new(
            Length::points((child_index % 2) as f32 * 0.5),
            Length::ZERO,
            Length::ZERO,
            Length::points((child_index % 3) as f32 * 0.5),
        ),
        ..Style::default()
    }
}

fn build_full_value_spacing_matrix_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(320.0),
        height: Length::points(nodes.max(1) as f32 * 72.0 + 8.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let container = tree.push(SimpleNode::new(full_value_spacing_container_style(index)));
        tree.append_child(root, container);

        for child_index in 0..4 {
            let child = tree.push(SimpleNode::new(full_value_spacing_child_style(
                index,
                child_index,
            )));
            tree.append_child(container, child);
        }
    }

    tree
}

fn full_value_spacing_container_style(index: usize) -> Style {
    let display = match index % 4 {
        0 => Display::Block,
        1 => Display::Flex,
        2 => Display::Linear,
        _ => Display::Grid,
    };
    let mut style = Style {
        display,
        direction: direction_for_bench_index(index),
        width: Length::points(128.0),
        height: Length::points(64.0),
        padding: Rect::new(
            spacing_length_for_index(index),
            spacing_length_for_index(index + 1),
            spacing_length_for_index(index + 2),
            spacing_length_for_index(index + 3),
        ),
        border: Rect::new(
            1.0 + (index % 2) as f32,
            (index % 3) as f32 * 0.5,
            0.5 + (index % 2) as f32,
            (index % 4) as f32 * 0.25,
        ),
        row_gap: spacing_length_for_index(index + 4),
        column_gap: spacing_length_for_index(index + 5),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    };

    match display {
        Display::Flex => {
            style.flex_wrap = FlexWrap::Wrap;
            style.flex_direction = flex_direction_for_bench_index(index);
            style.justify_content = JustifyContent::FlexStart;
            style.align_items = AlignItems::FlexStart;
            style.align_content = AlignContent::FlexStart;
        }
        Display::Linear => {
            style.linear_orientation = if index.is_multiple_of(2) {
                LinearOrientation::Horizontal
            } else {
                LinearOrientation::Vertical
            };
            style.linear_column_count = Some(2 + index % 2);
            style.list_main_axis_gap = spacing_length_for_index(index + 6);
            style.list_cross_axis_gap = spacing_length_for_index(index + 7);
        }
        Display::Grid => {
            style.grid_template_columns = vec![Length::points(28.0), Length::points(34.0)];
            style.grid_template_rows = vec![Length::points(14.0), Length::points(16.0)];
            style.grid_auto_flow = if (index / 4).is_multiple_of(2) {
                GridAutoFlow::Row
            } else {
                GridAutoFlow::Column
            };
            style.justify_items = JustifyItems::Start;
            style.align_items = AlignItems::FlexStart;
        }
        Display::Block | Display::None | Display::Relative => {}
    }

    style
}

fn full_value_spacing_child_style(index: usize, child_index: usize) -> Style {
    let base = index + child_index * 3;
    Style {
        display: Display::Block,
        position: PositionType::Relative,
        left: spacing_length_for_index(base),
        top: spacing_length_for_index(base + 1),
        width: Length::points(18.0 + child_index as f32 * 3.0),
        height: Length::points(8.0 + child_index as f32 * 2.0),
        flex_basis: Length::points(18.0 + child_index as f32 * 3.0),
        margin: Rect::new(
            spacing_length_for_index(base + 2),
            spacing_length_for_index(base + 3),
            spacing_length_for_index(base + 4),
            spacing_length_for_index(base + 5),
        ),
        padding: Rect::new(
            spacing_length_for_index(base + 6),
            spacing_length_for_index(base + 7),
            spacing_length_for_index(base + 8),
            spacing_length_for_index(base + 9),
        ),
        border: Rect::new(
            child_index as f32 * 0.5,
            0.5 + (child_index % 2) as f32,
            (child_index % 3) as f32 * 0.25,
            1.0,
        ),
        ..Style::default()
    }
}

fn spacing_length_for_index(index: usize) -> Length {
    match index % 9 {
        0 => Length::points(2.0 + (index % 5) as f32),
        1 => Length::percent(4.0 + (index % 7) as f32),
        2 => Length::calc(1.0 + (index % 3) as f32, 3.0 + (index % 5) as f32),
        3 => Length::Auto,
        4 => Length::fr(1.0 + (index % 3) as f32),
        5 => Length::MaxContent,
        6 => Length::fit_content(None),
        7 => Length::fit_content(Some(BaseLength::fixed(3.0 + (index % 6) as f32))),
        _ => Length::fit_content(Some(BaseLength::fixed_and_percent(
            1.0 + (index % 4) as f32,
            8.0 + (index % 5) as f32,
        ))),
    }
}

fn build_staggered_linear_list_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Vertical,
        width: Length::points(320.0),
        linear_column_count: Some(4),
        list_cross_axis_gap: Length::points(2.0),
        align_items: AlignItems::Stretch,
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let list_component_type = match index % 31 {
            0 => Some(ListComponentType::Header),
            10 => Some(ListComponentType::Default),
            15 => Some(ListComponentType::ListRow),
            30 => Some(ListComponentType::Footer),
            _ => None,
        };
        let child = tree.push(SimpleNode::new(Style {
            display: Display::Flex,
            height: Length::points(4.0 + (index % 3) as f32),
            margin: starlight_layout::Rect::new(
                Length::points((index % 2) as f32),
                Length::points((index % 3) as f32),
                Length::ZERO,
                Length::ZERO,
            ),
            list_component_type,
            ..Style::default()
        }));
        tree.append_child(root, child);
    }

    tree
}

fn build_staggered_linear_raw_list_gap_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        linear_orientation: LinearOrientation::Vertical,
        width: Length::points(320.0),
        ..Style::default()
    }));

    let container_count = integer_sqrt_ceil(nodes.max(1)).max(1);
    let children_per_container = nodes.max(1).div_ceil(container_count);
    let gaps = [
        Length::Auto,
        Length::fr(4.0),
        Length::MaxContent,
        Length::fit_content(Some(BaseLength::fixed(14.0))),
    ];
    let mut emitted = 0;

    for container_index in 0..container_count {
        let container = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            linear_orientation: LinearOrientation::Vertical,
            width: Length::points(160.0 + (container_index % 3) as f32 * 8.0),
            linear_column_count: Some(2 + container_index % 3),
            list_cross_axis_gap: gaps[container_index % gaps.len()],
            align_items: AlignItems::Stretch,
            ..Style::default()
        }));
        tree.append_child(root, container);

        for child_index in 0..children_per_container {
            if emitted >= nodes.max(1) {
                break;
            }
            let list_component_type = match child_index % 17 {
                0 => Some(ListComponentType::Header),
                8 => Some(ListComponentType::ListRow),
                16 => Some(ListComponentType::Footer),
                _ => None,
            };
            let child = tree.push(SimpleNode::new(Style {
                display: Display::Flex,
                height: Length::points(4.0 + (emitted % 5) as f32),
                margin: Rect::new(
                    Length::points((emitted % 3) as f32),
                    Length::points((child_index % 2) as f32),
                    Length::ZERO,
                    Length::ZERO,
                ),
                list_component_type,
                ..Style::default()
            }));
            tree.append_child(container, child);
            emitted += 1;
        }
    }

    tree
}

fn build_linear_gravity_matrix_tree(nodes: usize) -> SimpleTree {
    const ORIENTATIONS: [LinearOrientation; 8] = [
        LinearOrientation::Horizontal,
        LinearOrientation::HorizontalReverse,
        LinearOrientation::Vertical,
        LinearOrientation::VerticalReverse,
        LinearOrientation::Row,
        LinearOrientation::RowReverse,
        LinearOrientation::Column,
        LinearOrientation::ColumnReverse,
    ];
    const GRAVITIES: [LinearGravity; 11] = [
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

    let mut tree = SimpleTree::default();
    let node_count = nodes.max(1);
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(340.0),
        ..Style::default()
    }));

    for index in 0..node_count {
        let orientation = ORIENTATIONS[index % ORIENTATIONS.len()];
        let is_row = orientation.is_row();
        let container = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            direction: direction_for_bench_index(index),
            linear_orientation: orientation,
            linear_gravity: GRAVITIES[index % GRAVITIES.len()],
            justify_content: JustifyContent::FlexEnd,
            align_items: AlignItems::FlexStart,
            width: Length::points(if is_row { 118.0 } else { 54.0 }),
            height: Length::points(if is_row { 42.0 } else { 96.0 }),
            padding: Rect::new(
                Length::points(2.0),
                Length::points(3.0),
                Length::points(4.0),
                Length::points(5.0),
            ),
            margin: Rect::new(
                Length::ZERO,
                Length::ZERO,
                Length::points(1.0),
                Length::ZERO,
            ),
            ..Style::default()
        }));
        tree.append_child(root, container);

        for child_index in 0..3 {
            let child = tree.push(SimpleNode::new(Style {
                display: Display::Block,
                width: Length::points(12.0 + ((index + child_index) % 5) as f32),
                height: Length::points(8.0 + ((index + child_index * 2) % 4) as f32),
                margin: Rect::new(
                    Length::points((child_index % 2) as f32),
                    Length::points(((index + child_index) % 3) as f32),
                    Length::points(((child_index + 1) % 2) as f32),
                    Length::ZERO,
                ),
                ..Style::default()
            }));
            tree.append_child(container, child);
        }
    }

    tree
}

fn build_linear_layout_gravity_matrix_tree(nodes: usize) -> SimpleTree {
    const ORIENTATIONS: [LinearOrientation; 8] = [
        LinearOrientation::Horizontal,
        LinearOrientation::HorizontalReverse,
        LinearOrientation::Vertical,
        LinearOrientation::VerticalReverse,
        LinearOrientation::Row,
        LinearOrientation::RowReverse,
        LinearOrientation::Column,
        LinearOrientation::ColumnReverse,
    ];
    const LAYOUT_GRAVITIES: [LinearLayoutGravity; 13] = [
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

    let mut tree = SimpleTree::default();
    let node_count = nodes.max(1);
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(340.0),
        ..Style::default()
    }));

    for index in 0..node_count {
        let orientation = ORIENTATIONS[index % ORIENTATIONS.len()];
        let is_row = orientation.is_row();
        let container = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            direction: direction_for_bench_index(index),
            linear_orientation: orientation,
            align_items: AlignItems::Stretch,
            width: Length::points(if is_row { 118.0 } else { 54.0 }),
            height: Length::points(if is_row { 42.0 } else { 96.0 }),
            padding: Rect::new(
                Length::points(2.0),
                Length::points(3.0),
                Length::points(4.0),
                Length::points(5.0),
            ),
            margin: Rect::new(
                Length::ZERO,
                Length::ZERO,
                Length::points(1.0),
                Length::ZERO,
            ),
            ..Style::default()
        }));
        tree.append_child(root, container);

        for child_index in 0..3 {
            let child = tree.push(SimpleNode::new(Style {
                display: Display::Block,
                width: Length::points(12.0 + ((index + child_index) % 5) as f32),
                height: Length::points(8.0 + ((index + child_index * 2) % 4) as f32),
                linear_layout_gravity: if child_index == 1 {
                    LAYOUT_GRAVITIES[index % LAYOUT_GRAVITIES.len()]
                } else {
                    LinearLayoutGravity::None
                },
                margin: Rect::new(
                    Length::points((child_index % 2) as f32),
                    Length::points(((index + child_index) % 3) as f32),
                    Length::points(((child_index + 1) % 2) as f32),
                    Length::ZERO,
                ),
                ..Style::default()
            }));
            tree.append_child(container, child);
        }
    }

    tree
}

fn build_linear_cross_gravity_matrix_tree(nodes: usize) -> SimpleTree {
    const ORIENTATIONS: [LinearOrientation; 8] = [
        LinearOrientation::Horizontal,
        LinearOrientation::HorizontalReverse,
        LinearOrientation::Vertical,
        LinearOrientation::VerticalReverse,
        LinearOrientation::Row,
        LinearOrientation::RowReverse,
        LinearOrientation::Column,
        LinearOrientation::ColumnReverse,
    ];
    const CROSS_GRAVITIES: [LinearCrossGravity; 5] = [
        LinearCrossGravity::None,
        LinearCrossGravity::Start,
        LinearCrossGravity::End,
        LinearCrossGravity::Center,
        LinearCrossGravity::Stretch,
    ];

    let mut tree = SimpleTree::default();
    let node_count = nodes.max(1);
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(340.0),
        ..Style::default()
    }));

    for index in 0..node_count {
        let orientation = ORIENTATIONS[index % ORIENTATIONS.len()];
        let is_row = orientation.is_row();
        let container = tree.push(SimpleNode::new(Style {
            display: Display::Linear,
            direction: direction_for_bench_index(index),
            linear_orientation: orientation,
            align_items: AlignItems::FlexStart,
            linear_cross_gravity: CROSS_GRAVITIES[index % CROSS_GRAVITIES.len()],
            width: Length::points(if is_row { 118.0 } else { 54.0 }),
            height: Length::points(if is_row { 42.0 } else { 96.0 }),
            padding: Rect::new(
                Length::points(2.0),
                Length::points(3.0),
                Length::points(4.0),
                Length::points(5.0),
            ),
            margin: Rect::new(
                Length::ZERO,
                Length::ZERO,
                Length::points(1.0),
                Length::ZERO,
            ),
            ..Style::default()
        }));
        tree.append_child(root, container);

        for child_index in 0..3 {
            let child = tree.push(SimpleNode::new(Style {
                display: Display::Block,
                width: Length::points(12.0 + ((index + child_index) % 5) as f32),
                height: Length::points(8.0 + ((index + child_index * 2) % 4) as f32),
                margin: Rect::new(
                    Length::points((child_index % 2) as f32),
                    Length::points(((index + child_index) % 3) as f32),
                    Length::points(((child_index + 1) % 2) as f32),
                    Length::ZERO,
                ),
                ..Style::default()
            }));
            tree.append_child(container, child);
        }
    }

    tree
}

fn build_aspect_ratio_block_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(320.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let child = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            width: Length::points(12.0 + (index % 17) as f32),
            aspect_ratio: Some(1.0 + (index % 4) as f32 * 0.25),
            ..Style::default()
        }));
        tree.append_child(root, child);
    }

    tree
}

fn build_box_sizing_matrix_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(360.0),
        padding: Rect::all(Length::points(2.0)),
        border: Rect::all(1.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let container = tree.push(SimpleNode::new(box_sizing_matrix_container_style(index)));
        let content = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            width: Length::points(18.0 + (index % 9) as f32),
            height: Length::points(8.0 + (index % 5) as f32),
            margin: Rect::new(
                Length::points((index % 2) as f32),
                Length::ZERO,
                Length::points((index % 3) as f32 * 0.5),
                Length::points((index % 2) as f32),
            ),
            padding: Rect::all(Length::points((index % 3) as f32 * 0.5)),
            border: Rect::all((index % 2) as f32),
            ..Style::default()
        }));
        tree.append_child(root, container);
        tree.append_child(container, content);
    }

    tree
}

fn box_sizing_matrix_container_style(index: usize) -> Style {
    let display = match index % 5 {
        0 => Display::Block,
        1 => Display::Flex,
        2 => Display::Linear,
        3 => Display::Relative,
        _ => Display::Grid,
    };
    let mut style = Style {
        display,
        box_sizing: if index.is_multiple_of(2) {
            BoxSizing::ContentBox
        } else {
            BoxSizing::BorderBox
        },
        width: match index % 3 {
            0 => Length::points(42.0 + (index % 11) as f32),
            1 => Length::percent(26.0 + (index % 7) as f32),
            _ => Length::calc(8.0 + (index % 5) as f32, 18.0 + (index % 4) as f32),
        },
        height: if index.is_multiple_of(4) {
            Length::Auto
        } else {
            Length::points(20.0 + (index % 9) as f32)
        },
        min_width: Length::points(24.0 + (index % 5) as f32),
        max_width: Length::calc(40.0 + (index % 9) as f32, 32.0),
        min_height: Length::points(12.0 + (index % 4) as f32),
        max_height: Length::calc(24.0 + (index % 6) as f32, 45.0),
        aspect_ratio: (index.is_multiple_of(4)).then_some(1.15 + (index % 5) as f32 * 0.12),
        margin: Rect::new(
            Length::points((index % 3) as f32),
            Length::points((index % 4) as f32 * 0.5),
            Length::points((index % 2) as f32),
            Length::ZERO,
        ),
        padding: Rect::new(
            Length::points(1.0 + (index % 2) as f32),
            Length::points(2.0 + (index % 3) as f32),
            Length::points(1.0 + (index % 4) as f32 * 0.5),
            Length::points(1.0),
        ),
        border: Rect::new(
            1.0 + (index % 2) as f32,
            0.5 + (index % 3) as f32 * 0.5,
            1.0,
            0.5 + (index % 2) as f32,
        ),
        align_items: AlignItems::Center,
        justify_content: JustifyContent::Center,
        ..Style::default()
    };

    match display {
        Display::Flex => {
            style.flex_direction = if index.is_multiple_of(2) {
                FlexDirection::Row
            } else {
                FlexDirection::Column
            };
        }
        Display::Linear => {
            style.linear_orientation = if index.is_multiple_of(2) {
                LinearOrientation::Horizontal
            } else {
                LinearOrientation::Vertical
            };
        }
        Display::Grid => {
            style.grid_template_columns = vec![Length::points(20.0), Length::Auto];
            style.grid_template_rows = vec![Length::points(12.0), Length::Auto];
            style.column_gap = Length::points(1.0);
            style.row_gap = Length::points(1.0);
        }
        _ => {}
    }

    style
}

fn build_fit_content_subtree_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(320.0),
        padding: Rect::all(Length::points(2.0)),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let container = tree.push(SimpleNode::new(fit_content_subtree_container_style(index)));
        let content = tree.push(SimpleNode::new(fit_content_subtree_content_style(index)));
        tree.append_child(root, container);
        tree.append_child(container, content);
    }

    tree
}

fn fit_content_subtree_container_style(index: usize) -> Style {
    let display = match index % 5 {
        0 => Display::Block,
        1 => Display::Flex,
        2 => Display::Linear,
        3 => Display::Relative,
        _ => Display::Grid,
    };
    let mut style = Style {
        display,
        width: Length::fit_content(Some(BaseLength::fixed_and_percent(
            4.0 + (index % 3) as f32,
            40.0 + (index % 5) as f32 * 3.0,
        ))),
        height: Length::fit_content(Some(BaseLength::fixed_and_percent(
            2.0 + (index % 2) as f32,
            25.0 + (index % 4) as f32 * 4.0,
        ))),
        margin: Rect::new(
            Length::points((index % 2) as f32),
            Length::points((index % 3) as f32),
            Length::points((index % 4) as f32 * 0.5),
            Length::ZERO,
        ),
        padding: Rect::all(Length::points((index % 3) as f32 * 0.5)),
        border: Rect::all((index % 2) as f32),
        align_items: AlignItems::FlexStart,
        justify_content: JustifyContent::FlexStart,
        ..Style::default()
    };

    match display {
        Display::Flex => {
            style.flex_direction = if index.is_multiple_of(2) {
                FlexDirection::Row
            } else {
                FlexDirection::Column
            };
        }
        Display::Linear => {
            style.linear_orientation = if index.is_multiple_of(2) {
                LinearOrientation::Horizontal
            } else {
                LinearOrientation::Vertical
            };
        }
        Display::Grid => {
            style.grid_template_columns = vec![Length::points(24.0), Length::Auto];
            style.grid_template_rows = vec![Length::points(12.0), Length::Auto];
            style.column_gap = Length::points(1.0);
            style.row_gap = Length::points(1.0);
        }
        _ => {}
    }

    style
}

fn fit_content_subtree_content_style(index: usize) -> Style {
    Style {
        display: Display::Block,
        width: Length::points(20.0 + (index % 17) as f32),
        height: Length::points(8.0 + (index % 7) as f32),
        padding: Rect::all(Length::points((index % 2) as f32)),
        border: Rect::all((index % 3) as f32 * 0.5),
        grid_column_start: (index % 5 == 4).then_some(1),
        grid_row_start: (index % 5 == 4).then_some(1),
        ..Style::default()
    }
}

fn build_mixed_position_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(320.0),
        padding: starlight_layout::Rect::all(Length::points(2.0)),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let (position, left, right, top, bottom) = match index % 4 {
            0 => (
                PositionType::Absolute,
                Length::Auto,
                Length::points((index % 11) as f32),
                Length::Auto,
                Length::points((index % 7) as f32),
            ),
            1 => (
                PositionType::Relative,
                Length::points((index % 5) as f32),
                Length::Auto,
                Length::points((index % 3) as f32),
                Length::Auto,
            ),
            _ => (
                PositionType::Static,
                Length::Auto,
                Length::Auto,
                Length::Auto,
                Length::Auto,
            ),
        };
        let child = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            position,
            left,
            right,
            top,
            bottom,
            width: Length::points(10.0 + (index % 9) as f32),
            height: Length::points(4.0 + (index % 5) as f32),
            ..Style::default()
        }));
        tree.append_child(root, child);
    }

    tree
}

fn build_position_type_matrix_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(360.0),
        height: Length::points(240.0 + nodes.max(1) as f32 * 0.5),
        padding: Rect::all(Length::points(4.0)),
        border: Rect::all(1.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let child = tree.push(SimpleNode::new(position_type_matrix_child_style(index)));
        tree.append_child(root, child);
    }

    tree
}

fn position_type_matrix_child_style(index: usize) -> Style {
    let mut style = Style {
        display: Display::Block,
        position: match index % 5 {
            0 => PositionType::Static,
            1 => PositionType::Relative,
            2 => PositionType::Absolute,
            3 => PositionType::Fixed,
            _ => PositionType::Sticky,
        },
        width: Length::points(16.0 + (index % 9) as f32),
        height: Length::points(8.0 + (index % 5) as f32),
        margin: Rect::new(
            Length::points((index % 3) as f32),
            Length::points((index % 2) as f32),
            Length::points((index % 4) as f32 * 0.5),
            Length::ZERO,
        ),
        padding: Rect::all(Length::points((index % 2) as f32)),
        border: Rect::all((index % 3) as f32 * 0.5),
        ..Style::default()
    };

    match style.position {
        PositionType::Static => {}
        PositionType::Relative => {
            style.left = Length::points((index % 5) as f32);
            style.top = Length::calc(1.0, (index % 4) as f32);
        }
        PositionType::Absolute => {
            style.right = Length::percent(4.0 + (index % 5) as f32);
            style.bottom = Length::calc(2.0, 5.0 + (index % 4) as f32);
        }
        PositionType::Fixed => {
            style.left = Length::calc(3.0, 6.0 + (index % 5) as f32);
            style.top = Length::percent(3.0 + (index % 4) as f32);
        }
        PositionType::Sticky => {
            style.left = Length::Auto;
            style.right = Length::Auto;
            style.top = Length::calc(1.0, 2.0 + (index % 4) as f32);
            style.bottom = Length::Auto;
        }
    }

    style
}

fn build_relative_dependency_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(320.0),
        height: Length::points(160.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let relative_id = (index / 4) as i32 + 1;
        let style = match index % 4 {
            0 => Style {
                width: Length::points(18.0 + (index % 7) as f32),
                height: Length::points(8.0 + (index % 5) as f32),
                relative_id,
                relative_align_right: RELATIVE_ALIGN_PARENT,
                relative_align_bottom: RELATIVE_ALIGN_PARENT,
                ..Style::default()
            },
            1 => Style {
                width: Length::points(5.0 + (index % 3) as f32),
                height: Length::points(4.0 + (index % 4) as f32),
                relative_right_of: relative_id,
                relative_bottom_of: relative_id,
                ..Style::default()
            },
            2 => Style {
                width: Length::points(12.0 + (index % 5) as f32),
                height: Length::points(6.0 + (index % 3) as f32),
                relative_id,
                ..Style::default()
            },
            _ => Style {
                width: Length::points(4.0 + (index % 4) as f32),
                height: Length::points(3.0 + (index % 5) as f32),
                relative_align_left: relative_id,
                relative_align_bottom: relative_id,
                ..Style::default()
            },
        };
        let child = tree.push(SimpleNode::new(style));
        tree.append_child(root, child);
    }

    tree
}

fn build_relative_center_matrix_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(320.0),
        height: Length::points(220.0),
        padding: Rect::new(
            Length::points(3.0),
            Length::points(5.0),
            Length::points(7.0),
            Length::points(11.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let child = tree.push(SimpleNode::with_measured_size(
            relative_center_matrix_child_style(index),
            Size::new(18.0 + (index % 7) as f32, 10.0 + (index % 5) as f32),
        ));
        tree.append_child(root, child);
    }

    tree
}

fn relative_center_matrix_child_style(index: usize) -> Style {
    let mut style = Style {
        display: Display::Block,
        relative_center: match index % 4 {
            0 => RelativeCenter::None,
            1 => RelativeCenter::Horizontal,
            2 => RelativeCenter::Vertical,
            _ => RelativeCenter::Both,
        },
        margin: Rect::new(
            Length::points((index % 3) as f32),
            Length::points((index % 2) as f32),
            Length::points((index % 4) as f32 * 0.5),
            Length::points((index % 5) as f32 * 0.5),
        ),
        padding: Rect::all(Length::points((index % 2) as f32)),
        border: Rect::all((index % 3) as f32 * 0.5),
        ..Style::default()
    };

    match index % 4 {
        0 => {
            style.relative_align_left = RELATIVE_ALIGN_PARENT;
            style.relative_align_top = RELATIVE_ALIGN_PARENT;
        }
        1 => {
            style.relative_align_right = RELATIVE_ALIGN_PARENT;
        }
        2 => {
            style.relative_align_bottom = RELATIVE_ALIGN_PARENT;
        }
        _ => {
            style.relative_align_left = RELATIVE_ALIGN_PARENT;
            style.relative_align_right = RELATIVE_ALIGN_PARENT;
            style.relative_align_top = RELATIVE_ALIGN_PARENT;
            style.relative_align_bottom = RELATIVE_ALIGN_PARENT;
        }
    }

    style
}

fn build_sticky_percent_inset_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let node_count = nodes.max(1);
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(320.0),
        height: Length::points(node_count as f32 * 44.0 + 8.0),
        ..Style::default()
    }));

    for index in 0..node_count {
        let display = match index % 4 {
            0 => Display::Flex,
            1 => Display::Linear,
            2 => Display::Grid,
            _ => Display::Relative,
        };
        let container = tree.push(SimpleNode::new(sticky_container_style(display)));
        let sticky = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            position: PositionType::Sticky,
            width: Length::points(20.0 + (index % 5) as f32),
            height: Length::points(10.0 + (index % 3) as f32),
            left: Length::percent(10.0),
            right: if index % 3 == 0 {
                Length::percent(5.0)
            } else {
                Length::Auto
            },
            top: Length::percent(25.0),
            bottom: if index % 5 == 0 {
                Length::percent(10.0)
            } else {
                Length::Auto
            },
            ..Style::default()
        }));
        let normal = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            width: Length::points(8.0 + (index % 7) as f32),
            height: Length::points(6.0 + (index % 5) as f32),
            ..Style::default()
        }));
        tree.append_child(root, container);
        tree.append_child(container, sticky);
        tree.append_child(container, normal);
    }

    tree
}

fn sticky_container_style(display: Display) -> Style {
    let mut style = Style {
        display,
        width: Length::points(320.0),
        height: Length::points(40.0),
        ..Style::default()
    };

    match display {
        Display::Flex => {
            style.align_items = AlignItems::FlexStart;
        }
        Display::Linear => {
            style.linear_orientation = LinearOrientation::Horizontal;
            style.align_items = AlignItems::FlexStart;
        }
        Display::Grid => {
            style.grid_template_columns = vec![Length::points(320.0)];
            style.grid_template_rows = vec![Length::points(40.0)];
            style.align_items = AlignItems::FlexStart;
        }
        Display::Relative => {}
        Display::Block | Display::None => {}
    }

    style
}

fn build_mixed_display_none_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let node_count = nodes.max(1);
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(320.0),
        ..Style::default()
    }));

    for index in 0..node_count {
        match index % 4 {
            0 => append_flex_display_none_case(&mut tree, root, index),
            1 => append_linear_display_none_case(&mut tree, root, index),
            2 => append_grid_display_none_case(&mut tree, root, index),
            _ => append_relative_display_none_case(&mut tree, root, index),
        }
    }

    tree
}

fn append_flex_display_none_case(tree: &mut SimpleTree, root: usize, index: usize) {
    let container = tree.push(SimpleNode::new(Style {
        display: Display::Flex,
        width: Length::points(320.0),
        height: Length::points(12.0),
        align_items: AlignItems::FlexStart,
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(10.0 + (index % 5) as f32),
        height: Length::points(10.0),
        ..Style::default()
    }));
    let hidden = tree.push(SimpleNode::new(hidden_display_none_style(80.0, 20.0)));
    let second = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(12.0 + (index % 3) as f32),
        height: Length::points(10.0),
        ..Style::default()
    }));

    tree.append_child(root, container);
    tree.append_child(container, first);
    tree.append_child(container, hidden);
    tree.append_child(container, second);
}

fn append_linear_display_none_case(tree: &mut SimpleTree, root: usize, index: usize) {
    let container = tree.push(SimpleNode::new(Style {
        display: Display::Linear,
        width: Length::points(320.0),
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        height: Length::points(5.0 + (index % 3) as f32),
        ..Style::default()
    }));
    let hidden = tree.push(SimpleNode::new(hidden_display_none_style(300.0, 50.0)));
    let second = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        height: Length::points(6.0 + (index % 4) as f32),
        ..Style::default()
    }));

    tree.append_child(root, container);
    tree.append_child(container, first);
    tree.append_child(container, hidden);
    tree.append_child(container, second);
}

fn append_grid_display_none_case(tree: &mut SimpleTree, root: usize, _index: usize) {
    let container = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(320.0),
        height: Length::points(24.0),
        grid_template_columns: vec![Length::points(160.0), Length::points(160.0)],
        grid_template_rows: vec![Length::points(24.0)],
        ..Style::default()
    }));
    let first = tree.push(SimpleNode::new(Style::default()));
    let hidden = tree.push(SimpleNode::new(hidden_display_none_style(160.0, 24.0)));
    let second = tree.push(SimpleNode::new(Style::default()));

    tree.append_child(root, container);
    tree.append_child(container, first);
    tree.append_child(container, hidden);
    tree.append_child(container, second);
}

fn append_relative_display_none_case(tree: &mut SimpleTree, root: usize, index: usize) {
    let relative_id = index as i32 + 1;
    let container = tree.push(SimpleNode::new(Style {
        display: Display::Relative,
        width: Length::points(320.0),
        height: Length::points(24.0),
        ..Style::default()
    }));
    let visible_anchor = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(20.0 + (index % 5) as f32),
        height: Length::points(8.0 + (index % 3) as f32),
        relative_id,
        ..Style::default()
    }));
    let follower = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(5.0 + (index % 4) as f32),
        height: Length::points(4.0 + (index % 2) as f32),
        relative_right_of: relative_id,
        relative_bottom_of: relative_id,
        ..Style::default()
    }));
    let hidden_anchor = tree.push(SimpleNode::new(Style {
        display: Display::None,
        width: Length::points(80.0),
        height: Length::points(30.0),
        relative_id,
        ..Style::default()
    }));

    tree.append_child(root, container);
    tree.append_child(container, visible_anchor);
    tree.append_child(container, follower);
    tree.append_child(container, hidden_anchor);
}

fn hidden_display_none_style(width: f32, height: f32) -> Style {
    Style {
        display: Display::None,
        width: Length::points(width),
        height: Length::points(height),
        ..Style::default()
    }
}

fn build_out_of_flow_intrinsic_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(320.0),
        height: Length::points(absolute_rows(nodes.max(1)) as f32 * 8.0 + 160.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let position = if index % 5 == 0 {
            PositionType::Fixed
        } else {
            PositionType::Absolute
        };
        let (width, height) = match index % 3 {
            0 => (Length::MaxContent, Length::MaxContent),
            1 => (
                Length::fit_content(Some(BaseLength::fixed(24.0 + (index % 7) as f32))),
                Length::fit_content(Some(BaseLength::fixed(8.0 + (index % 5) as f32))),
            ),
            _ => (Length::Auto, Length::Auto),
        };
        let child = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            position,
            width,
            height,
            left: Length::points((index % 64) as f32 * 5.0),
            top: Length::points((index / 64) as f32 * 8.0),
            ..Style::default()
        }));
        let content = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            width: Length::points(16.0 + (index % 11) as f32),
            height: Length::points(4.0 + (index % 7) as f32),
            ..Style::default()
        }));
        tree.append_child(root, child);
        tree.append_child(child, content);
    }

    tree
}

fn build_out_of_flow_percent_calc_fill_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(320.0),
        height: Length::points(240.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let position = if index % 4 == 0 {
            PositionType::Fixed
        } else {
            PositionType::Absolute
        };
        let child = tree.push(SimpleNode::with_measured_size(
            Style {
                display: Display::Block,
                position,
                left: Length::percent(5.0 + (index % 5) as f32),
                right: Length::calc(4.0 + (index % 3) as f32, 10.0 + (index % 4) as f32 * 2.0),
                top: Length::calc(2.0 + (index % 4) as f32, 5.0 + (index % 3) as f32 * 3.0),
                bottom: Length::percent(10.0 + (index % 5) as f32 * 2.0),
                margin: Rect::new(
                    Length::points((index % 3) as f32),
                    Length::points((index % 4) as f32),
                    Length::points((index % 2) as f32),
                    Length::points((index % 5) as f32),
                ),
                ..Style::default()
            },
            Size::new(480.0 + (index % 17) as f32, 260.0 + (index % 13) as f32),
        ));
        tree.append_child(root, child);
    }

    tree
}

fn build_grid_out_of_flow_intrinsic_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(320.0),
        height: Length::points(160.0),
        grid_template_columns: vec![
            Length::points(40.0),
            Length::points(48.0),
            Length::points(56.0),
            Length::points(64.0),
        ],
        grid_template_rows: vec![
            Length::points(24.0),
            Length::points(28.0),
            Length::points(32.0),
            Length::points(36.0),
        ],
        column_gap: Length::points(2.0),
        row_gap: Length::points(3.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let column = (index % 4) as i32 + 1;
        let row = ((index / 4) % 4) as i32 + 1;
        let position = if index % 5 == 0 {
            PositionType::Fixed
        } else {
            PositionType::Absolute
        };
        let (width, height) = match index % 3 {
            0 => (Length::MaxContent, Length::MaxContent),
            1 => (
                Length::fit_content(Some(BaseLength::fixed(24.0 + (index % 7) as f32))),
                Length::fit_content(Some(BaseLength::fixed(8.0 + (index % 5) as f32))),
            ),
            _ => (Length::Auto, Length::Auto),
        };
        let child = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            position,
            width,
            height,
            grid_column_start: Some(column),
            grid_column_end: Some(column + 1),
            grid_row_start: Some(row),
            grid_row_end: Some(row + 1),
            left: Length::points((index % 3) as f32),
            top: Length::points((index % 2) as f32),
            ..Style::default()
        }));
        let content = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            width: Length::points(16.0 + (index % 11) as f32),
            height: Length::points(4.0 + (index % 7) as f32),
            ..Style::default()
        }));
        tree.append_child(root, child);
        tree.append_child(child, content);
    }

    tree
}

fn build_grid_out_of_flow_area_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let node_count = nodes.max(1);
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(340.0),
        height: Length::points(node_count as f32 * 70.0 + 8.0),
        ..Style::default()
    }));

    for index in 0..node_count {
        let container = tree.push(SimpleNode::new(grid_area_bench_container_style(index)));
        let marker = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            width: Length::points(8.0 + (index % 5) as f32),
            height: Length::points(6.0 + (index % 3) as f32),
            grid_column_start: Some(1),
            grid_row_start: Some(1),
            ..Style::default()
        }));
        let out_of_flow = append_grid_area_bench_out_of_flow(&mut tree, index);

        tree.append_child(root, container);
        tree.append_child(container, marker);
        tree.append_child(container, out_of_flow);
    }

    tree
}

fn grid_area_bench_container_style(index: usize) -> Style {
    Style {
        display: Display::Grid,
        direction: direction_for_bench_index(index),
        width: Length::points(320.0),
        height: Length::points(64.0),
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
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    }
}

fn append_grid_area_bench_out_of_flow(tree: &mut SimpleTree, index: usize) -> usize {
    let mut style = Style {
        display: Display::Block,
        position: if index.is_multiple_of(5) {
            PositionType::Fixed
        } else {
            PositionType::Absolute
        },
        margin: Rect::new(
            Length::points(1.0 + (index % 2) as f32),
            Length::points((index % 3) as f32),
            Length::points((index % 2) as f32),
            Length::points(1.0 + (index % 4) as f32),
        ),
        padding: Rect::all(Length::points(1.0)),
        border: Rect::all(1.0),
        ..Style::default()
    };

    match index % 4 {
        0 => {
            style.grid_column_start = Some(1);
            style.grid_column_end = Some(2);
            style.grid_row_start = Some(1);
            style.grid_row_end = Some(2);
        }
        1 => {
            style.grid_column_start = Some(2);
            style.grid_column_end = Some(3);
            style.grid_row_start = Some(1);
            style.grid_row_end = Some(3);
        }
        2 => {}
        _ => {
            style.grid_column_start = Some(3);
            style.grid_row_start = Some(2);
        }
    }

    match index % 4 {
        0 => {
            style.width = Length::Auto;
            style.height = Length::Auto;
            style.left = Length::percent(10.0);
            style.right = Length::calc(2.0, 15.0);
            style.top = Length::calc(1.0, 20.0);
            style.bottom = Length::percent(12.0);
            tree.push(SimpleNode::new(style))
        }
        1 => {
            style.width = Length::Auto;
            style.height = Length::Auto;
            style.justify_self = JustifyItems::End;
            style.align_self = Some(AlignItems::Center);
            tree.push(SimpleNode::with_measured_size(
                style,
                Size::new(11.0 + (index % 7) as f32, 13.0 + (index % 5) as f32),
            ))
        }
        2 => {
            style.width = Length::fit_content(Some(BaseLength::fixed(36.0)));
            style.height = Length::fit_content(Some(BaseLength::fixed(16.0)));
            style.justify_self = JustifyItems::Center;
            style.align_self = Some(AlignItems::Center);
            let out_of_flow = tree.push(SimpleNode::new(style));
            let content = tree.push(SimpleNode::new(Style {
                display: Display::Block,
                width: Length::points(40.0 + (index % 9) as f32),
                height: Length::points(18.0 + (index % 5) as f32),
                ..Style::default()
            }));
            tree.append_child(out_of_flow, content);
            out_of_flow
        }
        _ => {
            style.width = Length::fit_content(Some(BaseLength::fixed_and_percent(3.0, 50.0)));
            style.height = Length::fit_content(Some(BaseLength::fixed_and_percent(2.0, 40.0)));
            style.right = Length::percent(8.0);
            style.bottom = Length::calc(1.0, 10.0);
            tree.push(SimpleNode::with_measured_size(
                style,
                Size::new(44.0 + (index % 11) as f32, 17.0 + (index % 7) as f32),
            ))
        }
    }
}

fn build_grid_item_alignment_matrix_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(360.0),
        height: Length::points(nodes.max(1) as f32 * 74.0 + 8.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let container = tree.push(SimpleNode::new(grid_item_alignment_container_style(index)));
        tree.append_child(root, container);
        for child_index in 0..3 {
            let child = tree.push(SimpleNode::new(grid_item_alignment_child_style(
                index,
                child_index,
            )));
            tree.append_child(container, child);
        }
    }

    tree
}

fn grid_item_alignment_container_style(index: usize) -> Style {
    Style {
        display: Display::Grid,
        direction: direction_for_bench_index(index),
        justify_items: match index % 5 {
            0 => JustifyItems::Auto,
            1 => JustifyItems::Stretch,
            2 => JustifyItems::Start,
            3 => JustifyItems::Center,
            _ => JustifyItems::End,
        },
        align_items: match (index / 5) % 7 {
            0 => AlignItems::Stretch,
            1 => AlignItems::FlexStart,
            2 => AlignItems::Start,
            3 => AlignItems::Center,
            4 => AlignItems::FlexEnd,
            5 => AlignItems::End,
            _ => AlignItems::Baseline,
        },
        width: Length::points(320.0),
        height: Length::points(64.0),
        padding: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(4.0),
            Length::points(5.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        grid_template_columns: vec![Length::points(30.0), Length::points(42.0), Length::Auto],
        grid_template_rows: vec![Length::points(18.0), Length::points(24.0)],
        column_gap: Length::points(3.0),
        row_gap: Length::points(4.0),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    }
}

fn grid_item_alignment_child_style(index: usize, child_index: usize) -> Style {
    let mut style = Style {
        display: Display::Block,
        width: Length::points(10.0 + ((index + child_index) % 7) as f32),
        height: Length::points(7.0 + ((index + child_index) % 5) as f32),
        grid_column_start: Some((child_index % 3) as i32 + 1),
        grid_row_start: Some((child_index % 2) as i32 + 1),
        margin: Rect::new(
            Length::points((index % 2) as f32),
            Length::points((child_index % 2) as f32),
            Length::ZERO,
            Length::points(((index + child_index) % 3) as f32 * 0.5),
        ),
        padding: Rect::all(Length::points((child_index % 2) as f32)),
        border: Rect::all((index % 2) as f32),
        ..Style::default()
    };

    if child_index == 1 {
        style.justify_self = match index % 5 {
            0 => JustifyItems::Auto,
            1 => JustifyItems::Stretch,
            2 => JustifyItems::Start,
            3 => JustifyItems::Center,
            _ => JustifyItems::End,
        };
    }
    if child_index == 2 {
        style.align_self = Some(match index % 7 {
            0 => AlignItems::Stretch,
            1 => AlignItems::FlexStart,
            2 => AlignItems::Start,
            3 => AlignItems::Center,
            4 => AlignItems::FlexEnd,
            5 => AlignItems::End,
            _ => AlignItems::Baseline,
        });
    }

    style
}

fn build_grid_content_alignment_matrix_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(360.0),
        height: Length::points(nodes.max(1) as f32 * 74.0 + 8.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let container = tree.push(SimpleNode::new(grid_content_alignment_container_style(
            index,
        )));
        tree.append_child(root, container);
        for (column, row) in [(1, 1), (2, 1), (1, 2), (2, 2)] {
            let child = tree.push(SimpleNode::new(Style {
                display: Display::Block,
                width: Length::points(8.0 + ((index + column as usize) % 5) as f32),
                height: Length::points(6.0 + ((index + row as usize) % 4) as f32),
                grid_column_start: Some(column),
                grid_row_start: Some(row),
                ..Style::default()
            }));
            tree.append_child(container, child);
        }
    }

    tree
}

fn grid_content_alignment_container_style(index: usize) -> Style {
    let extra_space = index.is_multiple_of(2);
    let (width, height, column_gap, row_gap) = if extra_space {
        (90.0, 60.0, 2.0, 3.0)
    } else {
        (30.0, 24.0, 10.0, 10.0)
    };

    Style {
        display: Display::Grid,
        direction: direction_for_bench_index(index),
        justify_content: match index % 9 {
            0 => JustifyContent::FlexStart,
            1 => JustifyContent::Start,
            2 => JustifyContent::Center,
            3 => JustifyContent::FlexEnd,
            4 => JustifyContent::End,
            5 => JustifyContent::SpaceBetween,
            6 => JustifyContent::SpaceAround,
            7 => JustifyContent::SpaceEvenly,
            _ => JustifyContent::Stretch,
        },
        align_content: match (index / 9) % 9 {
            0 => AlignContent::FlexStart,
            1 => AlignContent::Start,
            2 => AlignContent::Center,
            3 => AlignContent::FlexEnd,
            4 => AlignContent::End,
            5 => AlignContent::SpaceBetween,
            6 => AlignContent::SpaceAround,
            7 => AlignContent::SpaceEvenly,
            _ => AlignContent::Stretch,
        },
        width: Length::points(width),
        height: Length::points(height),
        grid_template_columns: vec![Length::points(20.0), Length::points(18.0)],
        grid_template_rows: vec![Length::points(12.0), Length::points(14.0)],
        column_gap: Length::points(column_gap),
        row_gap: Length::points(row_gap),
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    }
}

fn build_grid_auto_flow_matrix_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(340.0),
        height: Length::points(nodes.max(1) as f32 * 92.0 + 8.0),
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let container = tree.push(SimpleNode::new(grid_auto_flow_container_style(index)));
        tree.append_child(root, container);
        match index % 3 {
            0 => append_grid_auto_flow_span_children(&mut tree, container, index),
            1 => append_grid_auto_flow_locked_children(&mut tree, container, index),
            _ => append_grid_auto_flow_implicit_children(&mut tree, container, index),
        }
    }

    tree
}

fn grid_auto_flow_container_style(index: usize) -> Style {
    Style {
        display: Display::Grid,
        direction: direction_for_bench_index(index),
        grid_auto_flow: match index % 5 {
            0 => GridAutoFlow::Row,
            1 => GridAutoFlow::Column,
            2 => GridAutoFlow::Dense,
            3 => GridAutoFlow::RowDense,
            _ => GridAutoFlow::ColumnDense,
        },
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
        margin: Rect::new(
            Length::points(1.0),
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    }
}

fn append_grid_auto_flow_span_children(tree: &mut SimpleTree, root: usize, index: usize) {
    let children = [
        grid_auto_flow_child_style(
            10.0,
            8.0,
            Style {
                grid_column_span: 2,
                ..Style::default()
            },
        ),
        Style::display_none(),
        grid_auto_flow_child_style(
            9.0,
            11.0,
            Style {
                grid_row_span: 2,
                ..Style::default()
            },
        ),
        grid_auto_flow_child_style(13.0 + (index % 3) as f32, 7.0, Style::default()),
        grid_auto_flow_child_style(
            8.0,
            9.0,
            Style {
                grid_column_span: 2,
                ..Style::default()
            },
        ),
    ];

    for style in children {
        let child = tree.push(SimpleNode::new(style));
        tree.append_child(root, child);
    }
}

fn append_grid_auto_flow_locked_children(tree: &mut SimpleTree, root: usize, index: usize) {
    let children = [
        grid_auto_flow_child_style(
            10.0,
            8.0,
            Style {
                grid_row_start: Some(2),
                grid_column_span: 2,
                ..Style::default()
            },
        ),
        grid_auto_flow_child_style(
            12.0,
            10.0,
            Style {
                grid_column_start: Some(2),
                grid_row_span: 2,
                ..Style::default()
            },
        ),
        grid_auto_flow_child_style(9.0 + (index % 4) as f32, 7.0, Style::default()),
        grid_auto_flow_child_style(
            11.0,
            9.0,
            Style {
                grid_row_start: Some(1),
                ..Style::default()
            },
        ),
    ];

    for style in children {
        let child = tree.push(SimpleNode::new(style));
        tree.append_child(root, child);
    }
}

fn append_grid_auto_flow_implicit_children(tree: &mut SimpleTree, root: usize, index: usize) {
    let children = [
        grid_auto_flow_child_style(
            10.0,
            8.0,
            Style {
                grid_column_start: Some(-1),
                grid_row_start: Some(1),
                ..Style::default()
            },
        ),
        grid_auto_flow_child_style(
            12.0,
            10.0,
            Style {
                grid_column_start: Some(5),
                grid_row_start: Some(2),
                ..Style::default()
            },
        ),
        grid_auto_flow_child_style(
            9.0,
            7.0,
            Style {
                grid_row_start: Some(-1),
                grid_column_span: 2,
                ..Style::default()
            },
        ),
        grid_auto_flow_child_style(11.0 + (index % 5) as f32, 9.0, Style::default()),
    ];

    for style in children {
        let child = tree.push(SimpleNode::new(style));
        tree.append_child(root, child);
    }
}

fn grid_auto_flow_child_style(width: f32, height: f32, style: Style) -> Style {
    Style {
        display: Display::Block,
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

fn build_grid_auto_margin_alignment_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let node_count = nodes.max(1);
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Block,
        width: Length::points(340.0),
        height: Length::points(node_count as f32 * 48.0 + 8.0),
        ..Style::default()
    }));

    for index in 0..node_count {
        let container = tree.push(SimpleNode::new(grid_auto_margin_bench_container_style(
            index,
        )));
        let auto_margin_item = tree.push(SimpleNode::with_measured_size(
            grid_auto_margin_bench_item_style(index),
            Size::new(13.0 + (index % 5) as f32, 9.0 + (index % 4) as f32),
        ));
        let marker = tree.push(SimpleNode::new(Style {
            display: Display::Block,
            width: Length::points(6.0 + (index % 3) as f32),
            height: Length::points(5.0 + (index % 2) as f32),
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

        tree.append_child(root, container);
        tree.append_child(container, auto_margin_item);
        tree.append_child(container, marker);
    }

    tree
}

fn grid_auto_margin_bench_container_style(index: usize) -> Style {
    Style {
        display: Display::Grid,
        direction: direction_for_bench_index(index),
        width: Length::points(320.0),
        height: Length::points(42.0),
        padding: Rect::new(
            Length::points(2.0),
            Length::points(3.0),
            Length::points(4.0),
            Length::points(5.0),
        ),
        border: Rect::new(1.0, 2.0, 1.0, 2.0),
        grid_template_columns: vec![Length::points(72.0), Length::points(88.0)],
        grid_template_rows: vec![Length::points(18.0), Length::points(16.0)],
        grid_auto_columns: vec![Length::points(64.0)],
        grid_auto_rows: vec![Length::points(14.0)],
        column_gap: Length::points(3.0),
        row_gap: Length::points(2.0),
        justify_items: JustifyItems::End,
        align_items: AlignItems::FlexEnd,
        margin: Rect::new(
            Length::ZERO,
            Length::ZERO,
            Length::points(1.0),
            Length::ZERO,
        ),
        ..Style::default()
    }
}

fn grid_auto_margin_bench_item_style(index: usize) -> Style {
    let (grid_column_start, grid_row_start, grid_column_span) = match index % 3 {
        0 => (Some(1), Some(1), 1),
        1 => (Some(2), Some(2), 1),
        _ => (None, None, 2),
    };
    Style {
        display: Display::Block,
        width: Length::Auto,
        height: Length::Auto,
        grid_column_start,
        grid_row_start,
        grid_column_span,
        margin: match index % 4 {
            0 => Rect::new(
                Length::Auto,
                Length::ZERO,
                Length::points(1.0),
                Length::ZERO,
            ),
            1 => Rect::new(
                Length::ZERO,
                Length::Auto,
                Length::ZERO,
                Length::points(1.0),
            ),
            2 => Rect::new(Length::Auto, Length::Auto, Length::Auto, Length::Auto),
            _ => Rect::new(
                Length::percent(2.0),
                Length::Auto,
                Length::calc(1.0, 2.0),
                Length::Auto,
            ),
        },
        justify_self: match index % 3 {
            0 => JustifyItems::Start,
            1 => JustifyItems::End,
            _ => JustifyItems::Center,
        },
        align_self: Some(match index % 3 {
            0 => AlignItems::FlexStart,
            1 => AlignItems::FlexEnd,
            _ => AlignItems::Center,
        }),
        ..Style::default()
    }
}

fn build_grid_minmax_intrinsic_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(320.0),
        height: Length::points(160.0),
        grid_template_columns: vec![
            Length::points(16.0),
            Length::Auto,
            Length::fr(1.0),
            Length::points(24.0),
        ],
        grid_template_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed(48.0))),
            Length::MaxContent,
            Length::fr(2.0),
            Length::fit_content(Some(BaseLength::fixed_and_percent(8.0, 25.0))),
        ],
        grid_auto_rows: vec![Length::Auto, Length::points(12.0)],
        grid_auto_rows_max: vec![Length::MaxContent, Length::points(24.0)],
        column_gap: Length::points(2.0),
        row_gap: Length::points(1.0),
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Stretch,
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let span = if index % 5 == 0 { 2 } else { 1 };
        let column_start = (index % (5 - span)) as i32 + 1;
        let row_start = (index / 4) as i32 + 1;
        let child = tree.push(SimpleNode::with_measured_size(
            Style {
                display: Display::Block,
                grid_column_start: Some(column_start),
                grid_column_span: span,
                grid_row_start: Some(row_start),
                margin: starlight_layout::Rect::new(
                    Length::points((index % 3) as f32),
                    Length::points((index % 2) as f32),
                    Length::points((index % 4) as f32 * 0.5),
                    Length::ZERO,
                ),
                justify_self: if index % 7 == 0 {
                    JustifyItems::Start
                } else {
                    JustifyItems::Stretch
                },
                ..Style::default()
            },
            starlight_layout::Size::new(18.0 + (index % 17) as f32, 4.0 + (index % 5) as f32),
        ));
        tree.append_child(root, child);
    }

    tree
}

fn build_grid_auto_fit_content_max_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        width: Length::points(320.0),
        height: Length::points(160.0),
        grid_auto_columns: vec![
            Length::points(20.0),
            Length::Auto,
            Length::points(12.0),
            Length::Auto,
        ],
        grid_auto_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 25.0))),
            Length::fit_content(Some(BaseLength::fixed_and_percent(8.0, 20.0))),
            Length::MaxContent,
            Length::points(36.0),
        ],
        grid_auto_rows: vec![
            Length::points(10.0),
            Length::Auto,
            Length::points(8.0),
            Length::Auto,
        ],
        grid_auto_rows_max: vec![
            Length::fit_content(Some(BaseLength::fixed_and_percent(0.0, 30.0))),
            Length::fit_content(Some(BaseLength::fixed_and_percent(4.0, 25.0))),
            Length::MaxContent,
            Length::points(28.0),
        ],
        column_gap: Length::points(1.0),
        row_gap: Length::points(1.0),
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Stretch,
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let column = (index % 12) as i32 + 1;
        let row = ((index / 12) % 12) as i32 + 1;
        let column_span = if index % 11 == 0 { 2 } else { 1 };
        let row_span = if index % 13 == 0 { 2 } else { 1 };
        let child = tree.push(SimpleNode::with_measured_size(
            Style {
                display: Display::Block,
                grid_column_start: Some(column),
                grid_column_span: column_span,
                grid_row_start: Some(row),
                grid_row_span: row_span,
                margin: starlight_layout::Rect::new(
                    Length::points((index % 2) as f32),
                    Length::points((index % 3) as f32 * 0.5),
                    Length::points((index % 4) as f32 * 0.25),
                    Length::ZERO,
                ),
                justify_self: if index % 5 == 0 {
                    JustifyItems::Start
                } else {
                    JustifyItems::Stretch
                },
                ..Style::default()
            },
            starlight_layout::Size::new(18.0 + (index % 23) as f32, 6.0 + (index % 11) as f32),
        ));
        tree.append_child(root, child);
    }

    tree
}

fn build_grid_indefinite_auto_fit_content_max_tree(nodes: usize) -> SimpleTree {
    let mut tree = SimpleTree::default();
    let root = tree.push(SimpleNode::new(Style {
        display: Display::Grid,
        grid_auto_columns: vec![
            Length::points(20.0),
            Length::points(10.0),
            Length::Auto,
            Length::points(8.0),
        ],
        grid_auto_columns_max: vec![
            Length::fit_content(Some(BaseLength::fixed(40.0))),
            Length::points(10.0),
            Length::fit_content(Some(BaseLength::fixed(32.0))),
            Length::points(16.0),
        ],
        grid_auto_rows: vec![
            Length::points(20.0),
            Length::points(10.0),
            Length::Auto,
            Length::points(8.0),
        ],
        grid_auto_rows_max: vec![
            Length::fit_content(Some(BaseLength::fixed(40.0))),
            Length::points(10.0),
            Length::fit_content(Some(BaseLength::fixed(32.0))),
            Length::points(16.0),
        ],
        column_gap: Length::points(1.0),
        row_gap: Length::points(1.0),
        align_items: AlignItems::FlexStart,
        justify_items: JustifyItems::Stretch,
        ..Style::default()
    }));

    for index in 0..nodes.max(1) {
        let column = (index % 16) as i32 + 1;
        let row = (index / 16) as i32 + 1;
        let child = tree.push(SimpleNode::with_measured_size(
            Style {
                display: Display::Block,
                grid_column_start: Some(column),
                grid_row_start: Some(row),
                margin: starlight_layout::Rect::new(
                    Length::points((index % 2) as f32),
                    Length::points((index % 3) as f32 * 0.5),
                    Length::points((index % 4) as f32 * 0.25),
                    Length::ZERO,
                ),
                justify_self: if index % 5 == 0 {
                    JustifyItems::Start
                } else {
                    JustifyItems::Stretch
                },
                align_self: Some(AlignItems::FlexStart),
                ..Style::default()
            },
            starlight_layout::Size::new(50.0 + (index % 19) as f32, 42.0 + (index % 13) as f32),
        ));
        tree.append_child(root, child);
    }

    tree
}

fn integer_sqrt_ceil(value: usize) -> usize {
    let mut candidate = 1;
    while candidate * candidate < value {
        candidate += 1;
    }
    candidate
}

#[derive(Clone, Copy, Debug)]
struct BenchArgs {
    nodes: usize,
    iterations: u32,
    warmup: u32,
    require_cpp_baseline: bool,
    min_speedup: f64,
}

impl BenchArgs {
    fn from_env() -> Self {
        let nodes = parse_arg(1).unwrap_or(1_000);
        let iterations = parse_arg(2).unwrap_or(200).max(1);
        let warmup = parse_arg(3).unwrap_or(10);
        Self {
            nodes,
            iterations,
            warmup,
            require_cpp_baseline: env_flag(ENV_REQUIRE_CPP_BASELINE),
            min_speedup: env_min_speedup(ENV_MIN_SPEEDUP),
        }
    }
}

fn parse_arg<T: std::str::FromStr>(index: usize) -> Option<T> {
    env::args().nth(index)?.parse().ok()
}

fn env_flag(name: &str) -> bool {
    env::var(name)
        .map(|value| env_flag_value(&value))
        .unwrap_or(false)
}

fn env_flag_value(value: &str) -> bool {
    let value = value.trim();
    !value.is_empty() && !value.eq_ignore_ascii_case("0") && !value.eq_ignore_ascii_case("false")
}

fn env_min_speedup(name: &str) -> f64 {
    env::var(name)
        .ok()
        .and_then(|value| parse_min_speedup_value(&value))
        .unwrap_or(DEFAULT_MIN_SPEEDUP)
}

fn parse_min_speedup_value(value: &str) -> Option<f64> {
    let speedup = value.trim().parse::<f64>().ok()?;
    (speedup.is_finite() && speedup > 0.0).then_some(speedup)
}

#[cfg(test)]
mod tests {
    use super::{
        absolute_constraints, at_most_constraints, at_most_two_axis_constraints, env_flag_value,
        grid_intrinsic_constraints, indefinite_constraints, integer_sqrt_ceil,
        out_of_flow_fill_constraints, parse_min_speedup_value, relative_constraints,
        sticky_constraints, wide_definite_constraints, BenchFeature, BENCH_SCENARIOS,
        RELATIVE_ALIGN_PARENT,
    };
    #[cfg(feature = "native-standalone")]
    use starlight_cpp::{CppBaselineError, CppStarlightEngine};
    use starlight_layout::{
        AlignContent, AlignItems, BoxSizing, Direction, Display, FlexDirection, FlexWrap,
        GridAutoFlow, JustifyContent, JustifyItems, LayoutEngine, Length, LinearCrossGravity,
        LinearGravity, LinearLayoutGravity, ListComponentType, MeasureMode, PositionType,
        RelativeCenter, SideConstraint,
    };
    #[cfg(feature = "native-standalone")]
    use starlight_layout::{LayoutResult, Rect, Size};
    use std::collections::BTreeSet;

    #[cfg(feature = "native-standalone")]
    const LAYOUT_EPSILON: f32 = 0.01;

    #[cfg(feature = "native-standalone")]
    fn assert_layout_near(scenario: &str, index: usize, rust: LayoutResult, cpp: LayoutResult) {
        let label = format!("{scenario} node {index}");
        assert_size_near(&format!("{label} size"), rust.size, cpp.size);
        assert_scalar_near(&format!("{label} offset.x"), rust.offset.x, cpp.offset.x);
        assert_scalar_near(&format!("{label} offset.y"), rust.offset.y, cpp.offset.y);
        assert_edges_near(&format!("{label} margin"), rust.margin, cpp.margin);
        assert_edges_near(&format!("{label} padding"), rust.padding, cpp.padding);
        assert_edges_near(&format!("{label} border"), rust.border, cpp.border);
        assert_edges_near(&format!("{label} sticky"), rust.sticky_pos, cpp.sticky_pos);
        assert_scalar_near(
            &format!("{label} baseline"),
            rust.baseline.unwrap_or(rust.size.height),
            cpp.baseline.unwrap_or(cpp.size.height),
        );
    }

    #[cfg(feature = "native-standalone")]
    fn assert_size_near(label: &str, rust: Size, cpp: Size) {
        assert_scalar_near(&format!("{label}.width"), rust.width, cpp.width);
        assert_scalar_near(&format!("{label}.height"), rust.height, cpp.height);
    }

    #[cfg(feature = "native-standalone")]
    fn assert_edges_near(label: &str, rust: Rect<f32>, cpp: Rect<f32>) {
        assert_scalar_near(&format!("{label}.left"), rust.left, cpp.left);
        assert_scalar_near(&format!("{label}.right"), rust.right, cpp.right);
        assert_scalar_near(&format!("{label}.top"), rust.top, cpp.top);
        assert_scalar_near(&format!("{label}.bottom"), rust.bottom, cpp.bottom);
    }

    #[cfg(feature = "native-standalone")]
    fn assert_scalar_near(label: &str, rust: f32, cpp: f32) {
        assert!(
            (rust - cpp).abs() <= LAYOUT_EPSILON,
            "{label} mismatch: rust={rust}, cpp={cpp}"
        );
    }

    #[cfg(feature = "native-standalone")]
    fn assert_benchmark_scenario_layout_matches_cpp(
        scenario: super::BenchScenario,
        nodes: usize,
    ) -> bool {
        let constraints = (scenario.constraints)(nodes);
        let mut rust_tree = (scenario.build_tree)(nodes);
        let mut cpp_tree = (scenario.build_tree)(nodes);

        let mut rust_engine = LayoutEngine::new();
        let rust_size =
            super::run_rust_layout(&mut rust_engine, &mut rust_tree, constraints, scenario);
        let mut cpp_engine = CppStarlightEngine::new();
        let cpp_result = if let Some(owner_direction) =
            super::owner_direction_for_scenario(scenario)
        {
            cpp_engine.layout_with_owner_direction(&mut cpp_tree, 0, constraints, owner_direction)
        } else {
            cpp_engine.layout(&mut cpp_tree, 0, constraints)
        };
        let cpp_size = match cpp_result {
            Ok(size) => size,
            Err(
                CppBaselineError::NativeFeatureDisabled | CppBaselineError::NativeLinkUnavailable,
            ) => return false,
            Err(error) => panic!(
                "benchmark scenario {} must run on C++ baseline: {error}",
                scenario.name
            ),
        };

        assert_size_near(&format!("{} root size", scenario.name), rust_size, cpp_size);
        assert_eq!(
            rust_tree.nodes.len(),
            cpp_tree.nodes.len(),
            "Rust and C++ benchmark scenario {} must have the same node count",
            scenario.name
        );
        for (index, (rust_node, cpp_node)) in rust_tree
            .nodes
            .iter()
            .zip(cpp_tree.nodes.iter())
            .enumerate()
        {
            assert_layout_near(scenario.name, index, rust_node.layout, cpp_node.layout);
        }
        true
    }

    #[test]
    fn benchmark_scenarios_run_with_rust_engine() {
        let mut engine = LayoutEngine::new();

        for scenario in BENCH_SCENARIOS {
            let mut tree = (scenario.build_tree)(8);
            let constraints = (scenario.constraints)(8);
            let size = super::run_rust_layout(&mut engine, &mut tree, constraints, *scenario);

            assert!(size.width >= 0.0);
            assert!(size.height >= 0.0);
            assert!(
                tree.nodes.len() >= minimum_nodes_for_scenario(scenario.name),
                "scenario {} built too few nodes",
                scenario.name
            );
        }
    }

    #[cfg(feature = "native-standalone")]
    #[test]
    fn benchmark_scenarios_are_supported_by_cpp_baseline_when_available() {
        for scenario in BENCH_SCENARIOS {
            match super::run_cpp_iterations(*scenario, 8, 1) {
                Ok(()) => {}
                Err(starlight_cpp::CppBaselineError::NativeFeatureDisabled)
                | Err(starlight_cpp::CppBaselineError::NativeLinkUnavailable) => return,
                Err(error) => panic!(
                    "benchmark scenario {} must be supported by the imported C++ baseline: {error}",
                    scenario.name
                ),
            }
        }
    }

    #[cfg(feature = "native-standalone")]
    #[test]
    fn benchmark_scenarios_with_full_layout_parity_match_cpp_baseline_when_available() {
        const FULL_LAYOUT_PARITY_SCENARIOS: &[&str] = &[
            "flex_grow_row",
            "flex_wrap_gaps",
            "flex_at_most_root",
            "at_most_owner_matrix",
            "standalone_owner_direction_inheritance",
            "flex_axis_alignment_matrix",
            "flex_distribution_matrix",
            "flex_wrap_alignment_matrix",
            "flex_baseline_measured",
            "baseline_propagation_matrix",
            "measured_callback_matrix",
            "absolute_children",
            "nested_column_flex",
            "in_flow_order_matrix",
            "full_value_spacing_matrix",
            "staggered_linear_list",
            "staggered_linear_raw_list_gaps",
            "linear_gravity_matrix",
            "linear_layout_gravity_matrix",
            "linear_cross_gravity_matrix",
            "aspect_ratio_blocks",
            "box_sizing_matrix",
            "fit_content_subtrees",
            "mixed_position_offsets",
            "position_type_matrix",
            "relative_dependency_graph",
            "relative_center_matrix",
            "sticky_percent_insets",
            "mixed_display_none",
            "out_of_flow_intrinsic",
            "out_of_flow_percent_calc_fill",
            "grid_out_of_flow_intrinsic",
            "grid_out_of_flow_areas",
            "grid_item_alignment_matrix",
            "grid_content_alignment_matrix",
            "grid_auto_flow_matrix",
            "grid_auto_margin_alignment",
            "grid_minmax_intrinsic_tracks",
            "grid_auto_fit_content_max_tracks",
            "grid_indefinite_auto_fit_content_max_tracks",
        ];

        assert_eq!(
            FULL_LAYOUT_PARITY_SCENARIOS.len(),
            BENCH_SCENARIOS.len(),
            "full-layout parity must include every benchmark scenario"
        );

        for scenario_name in FULL_LAYOUT_PARITY_SCENARIOS {
            let scenario = BENCH_SCENARIOS
                .iter()
                .find(|scenario| scenario.name == *scenario_name)
                .expect("full-layout parity benchmark scenario exists");
            if !assert_benchmark_scenario_layout_matches_cpp(*scenario, 8) {
                break;
            }
        }
    }

    #[test]
    fn benchmark_scenario_names_are_unique() {
        let mut names = BTreeSet::new();
        for scenario in BENCH_SCENARIOS {
            assert!(
                names.insert(scenario.name),
                "duplicate scenario {}",
                scenario.name
            );
        }
    }

    #[test]
    fn benchmark_scenarios_cover_required_layout_features() {
        let required_features = BenchFeature::ALL.iter().copied().collect::<BTreeSet<_>>();
        let covered_features = BENCH_SCENARIOS
            .iter()
            .flat_map(|scenario| scenario.features.iter().copied())
            .collect::<BTreeSet<_>>();

        let missing_features = required_features
            .difference(&covered_features)
            .copied()
            .collect::<Vec<_>>();
        assert!(
            missing_features.is_empty(),
            "benchmark scenarios must cover every required layout feature; missing: {missing_features:?}"
        );

        for scenario in BENCH_SCENARIOS {
            assert!(
                !scenario.features.is_empty(),
                "scenario {} must declare at least one coverage feature",
                scenario.name
            );
        }
    }

    #[test]
    fn benchmark_scenarios_cover_baseline_propagation_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "baseline_propagation_matrix")
            .expect("baseline propagation benchmark scenario exists");
        for feature in [
            BenchFeature::Alignment,
            BenchFeature::Baseline,
            BenchFeature::BaselinePropagation,
            BenchFeature::Flex,
            BenchFeature::Grid,
            BenchFeature::Linear,
            BenchFeature::MeasuredContent,
            BenchFeature::Relative,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "baseline propagation benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(60);
        let mut saw_measured_leaf_baseline = false;
        let mut saw_container_align_items_baseline = false;
        let mut saw_child_align_self_baseline = false;
        let mut saw_nested_flex_row = false;
        let mut saw_nested_flex_column = false;
        let mut saw_nested_linear = false;
        let mut saw_nested_grid = false;
        let mut saw_nested_relative = false;

        for node in &tree.nodes {
            saw_measured_leaf_baseline |= node.measured_size.is_some() && node.baseline.is_some();
            saw_container_align_items_baseline |= node.style.display == Display::Flex
                && node.children.len() == 3
                && node.style.align_items == AlignItems::Baseline;
            saw_child_align_self_baseline |= node.style.align_self == Some(AlignItems::Baseline);
            saw_nested_flex_row |= node.style.display == Display::Flex
                && node.children.len() == 2
                && node.style.flex_direction == FlexDirection::Row
                && node.style.align_items == AlignItems::Baseline;
            saw_nested_flex_column |= node.style.display == Display::Flex
                && node.children.len() == 2
                && node.style.flex_direction == FlexDirection::Column;
            saw_nested_linear |= node.style.display == Display::Linear && node.children.len() == 2;
            saw_nested_grid |= node.style.display == Display::Grid && node.children.len() == 1;
            saw_nested_relative |=
                node.style.display == Display::Relative && node.children.len() == 1;
        }

        assert!(saw_measured_leaf_baseline);
        assert!(saw_container_align_items_baseline);
        assert!(saw_child_align_self_baseline);
        assert!(saw_nested_flex_row);
        assert!(saw_nested_flex_column);
        assert!(saw_nested_linear);
        assert!(saw_nested_grid);
        assert!(saw_nested_relative);
    }

    #[test]
    fn benchmark_scenarios_cover_at_most_owner_constraint_matrix() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "at_most_owner_matrix")
            .expect("at-most owner constraint benchmark scenario exists");
        for feature in [
            BenchFeature::AtMostRoot,
            BenchFeature::Block,
            BenchFeature::FitContent,
            BenchFeature::Flex,
            BenchFeature::Grid,
            BenchFeature::Linear,
            BenchFeature::MeasuredContent,
            BenchFeature::MinMax,
            BenchFeature::OwnerConstraints,
            BenchFeature::PercentCalc,
            BenchFeature::Relative,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "at-most owner constraint benchmark must declare {feature:?} coverage"
            );
        }

        let constraints = (scenario.constraints)(64);
        assert_eq!(constraints.width.mode, MeasureMode::AtMost);
        assert_eq!(constraints.height.mode, MeasureMode::AtMost);

        let tree = (scenario.build_tree)(50);
        let mut saw_display = [false; 5];
        let mut saw_percent = false;
        let mut saw_calc = false;
        let mut saw_fit_content = false;
        let mut saw_minmax = false;
        let mut saw_measured = false;
        let mut saw_relative_parent_alignment = false;

        for node in &tree.nodes {
            match node.style.display {
                Display::Block => saw_display[0] = true,
                Display::Flex => saw_display[1] = true,
                Display::Linear => saw_display[2] = true,
                Display::Grid => saw_display[3] = true,
                Display::Relative => saw_display[4] = true,
                Display::None => {}
            }
            saw_percent |= matches!(
                node.style.width,
                Length::Percent(_) | Length::FitContent(Some(_))
            ) || matches!(
                node.style.height,
                Length::Percent(_) | Length::FitContent(Some(_))
            );
            saw_calc |= matches!(node.style.max_width, Length::Calc { .. })
                || matches!(node.style.max_height, Length::Calc { .. });
            saw_fit_content |= matches!(node.style.width, Length::FitContent(_))
                || matches!(node.style.height, Length::FitContent(_));
            saw_minmax |= !matches!(node.style.min_width, Length::Auto)
                || !matches!(node.style.max_width, Length::Auto)
                || !matches!(node.style.min_height, Length::Auto)
                || !matches!(node.style.max_height, Length::Auto);
            saw_measured |= node.measured_size.is_some();
            saw_relative_parent_alignment |= node.style.relative_align_left
                == RELATIVE_ALIGN_PARENT
                || node.style.relative_align_top == RELATIVE_ALIGN_PARENT;
        }

        assert!(
            saw_display.iter().all(|seen| *seen),
            "at-most owner constraint benchmark must cover block/flex/linear/grid/relative containers"
        );
        assert!(saw_percent);
        assert!(saw_calc);
        assert!(saw_fit_content);
        assert!(saw_minmax);
        assert!(saw_measured);
        assert!(saw_relative_parent_alignment);
    }

    #[test]
    fn benchmark_scenarios_cover_owner_direction_inheritance() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "standalone_owner_direction_inheritance")
            .expect("standalone owner-direction inheritance benchmark scenario exists");
        for feature in [
            BenchFeature::Flex,
            BenchFeature::OwnerConstraints,
            BenchFeature::OwnerDirection,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "standalone owner-direction benchmark must declare {feature:?} coverage"
            );
        }

        let constraints = (scenario.constraints)(8);
        assert_eq!(constraints.width, SideConstraint::definite(30.0));
        assert_eq!(constraints.height, SideConstraint::definite(160.0));

        let tree = (scenario.build_tree)(8);
        assert_eq!(
            super::owner_direction_for_scenario(*scenario),
            Some(Direction::Rtl)
        );

        let mut saw_inherited_row = false;
        let mut saw_explicit_ltr_row = false;
        for node in &tree.nodes {
            let is_owner_direction_row = node.style.display == Display::Flex
                && node.children.len() == 1
                && node.style.flex_direction == FlexDirection::Row
                && node.style.width == Length::points(30.0);
            saw_inherited_row |= is_owner_direction_row && !node.has_explicit_direction_style;
            saw_explicit_ltr_row |= is_owner_direction_row
                && node.has_explicit_direction_style
                && node.style.direction == Direction::Ltr;
        }

        assert!(saw_inherited_row);
        assert!(saw_explicit_ltr_row);
    }

    #[test]
    fn benchmark_scenarios_cover_measured_callback_matrix_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "measured_callback_matrix")
            .expect("measured callback benchmark scenario exists");
        for feature in [
            BenchFeature::Baseline,
            BenchFeature::Block,
            BenchFeature::FitContent,
            BenchFeature::Flex,
            BenchFeature::Grid,
            BenchFeature::Linear,
            BenchFeature::MeasuredCallbacks,
            BenchFeature::MeasuredContent,
            BenchFeature::MinMax,
            BenchFeature::Relative,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "measured callback benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(40);
        let mut saw_display = [false; 5];
        let mut saw_measured = false;
        let mut saw_measure_callback = false;
        let mut saw_baseline = false;
        let mut saw_baseline_callback = false;
        let mut saw_fit_content = false;
        let mut saw_minmax = false;
        let mut saw_relative_parent_alignment = false;
        let mut saw_align_self_baseline = false;

        for node in &tree.nodes {
            match node.style.display {
                Display::Block => saw_display[0] = true,
                Display::Flex => saw_display[1] = true,
                Display::Linear => saw_display[2] = true,
                Display::Grid => saw_display[3] = true,
                Display::Relative => saw_display[4] = true,
                Display::None => {}
            }
            saw_measured |= node.measured_size.is_some() || node.measure_func.is_some();
            saw_measure_callback |= node.measure_func.is_some();
            saw_baseline |= node.baseline.is_some() || node.baseline_func.is_some();
            saw_baseline_callback |= node.baseline_func.is_some();
            saw_fit_content |= matches!(node.style.width, Length::FitContent(_))
                || matches!(node.style.height, Length::FitContent(_));
            saw_minmax |= !matches!(node.style.min_width, Length::Auto)
                || !matches!(node.style.max_width, Length::Auto)
                || !matches!(node.style.min_height, Length::Auto)
                || !matches!(node.style.max_height, Length::Auto);
            saw_relative_parent_alignment |= node.style.relative_align_left
                == RELATIVE_ALIGN_PARENT
                || node.style.relative_align_top == RELATIVE_ALIGN_PARENT;
            saw_align_self_baseline |= node.style.align_self == Some(AlignItems::Baseline);
        }

        assert!(
            saw_display.iter().all(|seen| *seen),
            "measured callback benchmark must cover block/flex/linear/grid/relative containers"
        );
        assert!(saw_measured);
        assert!(saw_measure_callback);
        assert!(saw_baseline);
        assert!(saw_baseline_callback);
        assert!(saw_fit_content);
        assert!(saw_minmax);
        assert!(saw_relative_parent_alignment);
        assert!(saw_align_self_baseline);
    }

    #[test]
    #[cfg(feature = "native-standalone")]
    fn measured_callback_matrix_layout_matches_cpp_baseline_when_available() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "measured_callback_matrix")
            .expect("measured callback benchmark scenario exists");
        let _baseline_available = assert_benchmark_scenario_layout_matches_cpp(*scenario, 12);
    }

    #[test]
    fn benchmark_scenarios_cover_full_value_spacing_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "full_value_spacing_matrix")
            .expect("full-value spacing benchmark scenario exists");
        for feature in [
            BenchFeature::Block,
            BenchFeature::Direction,
            BenchFeature::FitContent,
            BenchFeature::Flex,
            BenchFeature::Grid,
            BenchFeature::IntrinsicSizing,
            BenchFeature::Linear,
            BenchFeature::PercentCalc,
            BenchFeature::PositionType,
            BenchFeature::SpacingValues,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "full-value spacing benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(72);
        let mut saw_display = [false; 4];
        let mut saw_ltr = false;
        let mut saw_rtl = false;
        let mut coverage = SpacingCoverage::default();

        for node in &tree.nodes {
            match node.style.display {
                Display::Block => saw_display[0] = true,
                Display::Flex => saw_display[1] = true,
                Display::Linear => saw_display[2] = true,
                Display::Grid => saw_display[3] = true,
                Display::None | Display::Relative => {}
            }
            match node.style.direction {
                Direction::Ltr => saw_ltr = true,
                Direction::Rtl => saw_rtl = true,
            }

            coverage.record_rect(node.style.margin);
            coverage.saw_margin = true;
            coverage.record_rect(node.style.padding);
            coverage.saw_padding = true;
            coverage.record(node.style.row_gap);
            coverage.saw_row_gap = true;
            coverage.record(node.style.column_gap);
            coverage.saw_column_gap = true;
            coverage.record(node.style.list_main_axis_gap);
            coverage.record(node.style.list_cross_axis_gap);
            if node.style.display == Display::Linear {
                coverage.saw_list_gap = true;
            }
            if node.style.position == PositionType::Relative {
                coverage.record(node.style.left);
                coverage.record(node.style.top);
                coverage.saw_position_edge = true;
            }
            coverage.saw_border |= node.style.border.left > 0.0
                || node.style.border.right > 0.0
                || node.style.border.top > 0.0
                || node.style.border.bottom > 0.0;
        }

        assert!(
            saw_display.iter().all(|seen| *seen),
            "full-value spacing benchmark must cover block/flex/linear/grid containers"
        );
        assert!(saw_ltr);
        assert!(saw_rtl);
        coverage.assert_complete();
    }

    #[derive(Default)]
    struct SpacingCoverage {
        saw_points: bool,
        saw_percent: bool,
        saw_calc: bool,
        saw_auto: bool,
        saw_fr: bool,
        saw_max_content: bool,
        saw_fit_content_no_arg: bool,
        saw_fit_content_fixed: bool,
        saw_fit_content_fixed_percent: bool,
        saw_margin: bool,
        saw_padding: bool,
        saw_row_gap: bool,
        saw_column_gap: bool,
        saw_list_gap: bool,
        saw_position_edge: bool,
        saw_border: bool,
    }

    impl SpacingCoverage {
        fn record_rect(&mut self, rect: starlight_layout::Rect<Length>) {
            self.record(rect.left);
            self.record(rect.right);
            self.record(rect.top);
            self.record(rect.bottom);
        }

        fn record(&mut self, length: Length) {
            match length {
                Length::Points(_) => self.saw_points = true,
                Length::Percent(_) => self.saw_percent = true,
                Length::Calc { .. } => self.saw_calc = true,
                Length::Auto => self.saw_auto = true,
                Length::Fr(_) => self.saw_fr = true,
                Length::MaxContent => self.saw_max_content = true,
                Length::FitContent(None) => self.saw_fit_content_no_arg = true,
                Length::FitContent(Some(base)) if base.contains_percentage() => {
                    self.saw_fit_content_fixed_percent = true;
                }
                Length::FitContent(Some(_)) => self.saw_fit_content_fixed = true,
            }
        }

        fn assert_complete(self) {
            assert!(self.saw_points);
            assert!(self.saw_percent);
            assert!(self.saw_calc);
            assert!(self.saw_auto);
            assert!(self.saw_fr);
            assert!(self.saw_max_content);
            assert!(self.saw_fit_content_no_arg);
            assert!(self.saw_fit_content_fixed);
            assert!(self.saw_fit_content_fixed_percent);
            assert!(self.saw_margin);
            assert!(self.saw_padding);
            assert!(self.saw_row_gap);
            assert!(self.saw_column_gap);
            assert!(self.saw_list_gap);
            assert!(self.saw_position_edge);
            assert!(self.saw_border);
        }
    }

    #[test]
    fn benchmark_scenarios_cover_in_flow_order_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "in_flow_order_matrix")
            .expect("in-flow order benchmark scenario exists");
        for feature in [
            BenchFeature::Block,
            BenchFeature::Direction,
            BenchFeature::Flex,
            BenchFeature::Grid,
            BenchFeature::Linear,
            BenchFeature::Order,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "in-flow order benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(36);
        let mut saw_display = [false; 4];
        let mut saw_ltr = false;
        let mut saw_rtl = false;
        let mut saw_negative_order = false;
        let mut saw_zero_order = false;
        let mut saw_positive_order = false;
        let mut saw_grid_row_flow = false;
        let mut saw_grid_column_flow = false;

        for node in &tree.nodes {
            match node.style.display {
                Display::Block => saw_display[0] = true,
                Display::Flex => saw_display[1] = true,
                Display::Linear => saw_display[2] = true,
                Display::Grid => {
                    saw_display[3] = true;
                    match node.style.grid_auto_flow {
                        GridAutoFlow::Row => saw_grid_row_flow = true,
                        GridAutoFlow::Column => saw_grid_column_flow = true,
                        GridAutoFlow::Dense
                        | GridAutoFlow::RowDense
                        | GridAutoFlow::ColumnDense => {}
                    }
                }
                Display::None | Display::Relative => {}
            }
            match node.style.direction {
                Direction::Ltr => saw_ltr = true,
                Direction::Rtl => saw_rtl = true,
            }
            saw_negative_order |= node.style.order < 0;
            saw_zero_order |= node.style.order == 0;
            saw_positive_order |= node.style.order > 0;
        }

        assert!(
            saw_display.iter().all(|seen| *seen),
            "in-flow order benchmark must cover block/flex/linear/grid containers"
        );
        assert!(saw_ltr);
        assert!(saw_rtl);
        assert!(saw_negative_order);
        assert!(saw_zero_order);
        assert!(saw_positive_order);
        assert!(saw_grid_row_flow);
        assert!(saw_grid_column_flow);
    }

    #[test]
    fn benchmark_scenarios_cover_flex_distribution_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "flex_distribution_matrix")
            .expect("flex distribution benchmark scenario exists");
        for feature in [
            BenchFeature::Direction,
            BenchFeature::Flex,
            BenchFeature::FlexDistribution,
            BenchFeature::MinMax,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "flex distribution benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(48);
        let mut saw_ltr = false;
        let mut saw_rtl = false;
        let mut saw_flex_direction = [false; 4];
        let mut saw_grow = false;
        let mut saw_shrink_override = false;
        let mut saw_point_basis = false;
        let mut saw_percent_basis = false;
        let mut saw_negative_order = false;
        let mut saw_positive_order = false;
        let mut saw_row_minmax = false;
        let mut saw_column_minmax = false;
        let mut saw_percent_minmax = false;

        for node in &tree.nodes {
            if node.style.display == Display::Flex
                && matches!(node.style.flex_wrap, FlexWrap::NoWrap)
                && (matches!(node.style.width, Length::Points(value) if (value - 178.0).abs() < 0.01 || (value - 94.0).abs() < 0.01)
                    || matches!(node.style.height, Length::Points(value) if (value - 178.0).abs() < 0.01 || (value - 94.0).abs() < 0.01))
            {
                match node.style.direction {
                    Direction::Ltr => saw_ltr = true,
                    Direction::Rtl => saw_rtl = true,
                }
                match node.style.flex_direction {
                    FlexDirection::Row => saw_flex_direction[0] = true,
                    FlexDirection::RowReverse => saw_flex_direction[1] = true,
                    FlexDirection::Column => saw_flex_direction[2] = true,
                    FlexDirection::ColumnReverse => saw_flex_direction[3] = true,
                }
            }

            saw_grow |= node.style.flex_grow > 0.0;
            saw_shrink_override |= (node.style.flex_shrink - 1.0).abs() > f32::EPSILON;
            saw_point_basis |= matches!(node.style.flex_basis, Length::Points(_));
            saw_percent_basis |= matches!(node.style.flex_basis, Length::Percent(_));
            saw_negative_order |= node.style.order < 0;
            saw_positive_order |= node.style.order > 0;
            saw_row_minmax |= !matches!(node.style.min_width, Length::Auto)
                || !matches!(node.style.max_width, Length::Auto);
            saw_column_minmax |= !matches!(node.style.min_height, Length::Auto)
                || !matches!(node.style.max_height, Length::Auto);
            saw_percent_minmax |= matches!(node.style.min_width, Length::Percent(_))
                || matches!(node.style.max_width, Length::Percent(_))
                || matches!(node.style.min_height, Length::Percent(_))
                || matches!(node.style.max_height, Length::Percent(_));
        }

        assert!(saw_ltr);
        assert!(saw_rtl);
        assert!(
            saw_flex_direction.iter().all(|seen| *seen),
            "flex distribution benchmark must cover every FlexDirection value"
        );
        assert!(saw_grow);
        assert!(saw_shrink_override);
        assert!(saw_point_basis);
        assert!(saw_percent_basis);
        assert!(saw_negative_order);
        assert!(saw_positive_order);
        assert!(saw_row_minmax);
        assert!(saw_column_minmax);
        assert!(saw_percent_minmax);
    }

    #[test]
    fn benchmark_scenarios_cover_flex_axis_alignment_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "flex_axis_alignment_matrix")
            .expect("flex axis alignment benchmark scenario exists");
        for feature in [
            BenchFeature::Alignment,
            BenchFeature::Direction,
            BenchFeature::Flex,
            BenchFeature::FlexAxisAlignment,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "flex axis alignment benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(252);
        let mut saw_ltr = false;
        let mut saw_rtl = false;
        let mut saw_flex_direction = [false; 4];
        let mut saw_justify_content = [false; 9];
        let mut saw_align_items = [false; 7];
        let mut saw_auto_cross_child = false;

        for node in &tree.nodes {
            if node.style.display != Display::Flex {
                continue;
            }
            match node.style.direction {
                Direction::Ltr => saw_ltr = true,
                Direction::Rtl => saw_rtl = true,
            }
            match node.style.flex_direction {
                FlexDirection::Row => saw_flex_direction[0] = true,
                FlexDirection::RowReverse => saw_flex_direction[1] = true,
                FlexDirection::Column => saw_flex_direction[2] = true,
                FlexDirection::ColumnReverse => saw_flex_direction[3] = true,
            }
            match node.style.justify_content {
                JustifyContent::Stretch => saw_justify_content[0] = true,
                JustifyContent::FlexStart => saw_justify_content[1] = true,
                JustifyContent::Start => saw_justify_content[2] = true,
                JustifyContent::Center => saw_justify_content[3] = true,
                JustifyContent::FlexEnd => saw_justify_content[4] = true,
                JustifyContent::End => saw_justify_content[5] = true,
                JustifyContent::SpaceBetween => saw_justify_content[6] = true,
                JustifyContent::SpaceAround => saw_justify_content[7] = true,
                JustifyContent::SpaceEvenly => saw_justify_content[8] = true,
            }
            match node.style.align_items {
                AlignItems::Stretch => saw_align_items[0] = true,
                AlignItems::FlexStart => saw_align_items[1] = true,
                AlignItems::Start => saw_align_items[2] = true,
                AlignItems::Center => saw_align_items[3] = true,
                AlignItems::FlexEnd => saw_align_items[4] = true,
                AlignItems::End => saw_align_items[5] = true,
                AlignItems::Baseline => saw_align_items[6] = true,
            }
            saw_auto_cross_child |= matches!(node.style.width, Length::Auto)
                || matches!(node.style.height, Length::Auto);
        }

        assert!(saw_ltr);
        assert!(saw_rtl);
        assert!(
            saw_flex_direction.iter().all(|seen| *seen),
            "flex axis alignment benchmark must cover every FlexDirection value"
        );
        assert!(
            saw_justify_content.iter().all(|seen| *seen),
            "flex axis alignment benchmark must cover every JustifyContent value"
        );
        assert!(
            saw_align_items.iter().all(|seen| *seen),
            "flex axis alignment benchmark must cover every AlignItems value"
        );
        assert!(saw_auto_cross_child);
    }

    #[test]
    fn benchmark_scenarios_cover_flex_wrap_alignment_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "flex_wrap_alignment_matrix")
            .expect("flex wrap alignment benchmark scenario exists");
        for feature in [
            BenchFeature::Alignment,
            BenchFeature::Direction,
            BenchFeature::Flex,
            BenchFeature::FlexWrap,
            BenchFeature::FlexWrapAlignment,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "flex wrap alignment benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(252);
        let mut saw_ltr = false;
        let mut saw_rtl = false;
        let mut saw_flex_direction = [false; 4];
        let mut saw_flex_wrap = [false; 3];
        let mut saw_align_content = [false; 9];
        let mut saw_align_items = [false; 7];
        let mut saw_gap = false;
        let mut saw_wrap_reverse_space_between = false;

        for node in &tree.nodes {
            let is_matrix_container = node.style.display == Display::Flex
                && matches!(node.style.width, Length::Points(value) if (value - 76.0).abs() < 0.01)
                && matches!(node.style.height, Length::Points(value) if (value - 64.0).abs() < 0.01);
            if !is_matrix_container {
                continue;
            }

            match node.style.direction {
                Direction::Ltr => saw_ltr = true,
                Direction::Rtl => saw_rtl = true,
            }
            match node.style.flex_direction {
                FlexDirection::Row => saw_flex_direction[0] = true,
                FlexDirection::RowReverse => saw_flex_direction[1] = true,
                FlexDirection::Column => saw_flex_direction[2] = true,
                FlexDirection::ColumnReverse => saw_flex_direction[3] = true,
            }
            match node.style.flex_wrap {
                FlexWrap::NoWrap => saw_flex_wrap[0] = true,
                FlexWrap::Wrap => saw_flex_wrap[1] = true,
                FlexWrap::WrapReverse => saw_flex_wrap[2] = true,
            }
            match node.style.align_content {
                AlignContent::FlexStart => saw_align_content[0] = true,
                AlignContent::Start => saw_align_content[1] = true,
                AlignContent::Center => saw_align_content[2] = true,
                AlignContent::FlexEnd => saw_align_content[3] = true,
                AlignContent::End => saw_align_content[4] = true,
                AlignContent::SpaceBetween => saw_align_content[5] = true,
                AlignContent::SpaceAround => saw_align_content[6] = true,
                AlignContent::SpaceEvenly => saw_align_content[7] = true,
                AlignContent::Stretch => saw_align_content[8] = true,
            }
            match node.style.align_items {
                AlignItems::Stretch => saw_align_items[0] = true,
                AlignItems::FlexStart => saw_align_items[1] = true,
                AlignItems::Start => saw_align_items[2] = true,
                AlignItems::Center => saw_align_items[3] = true,
                AlignItems::FlexEnd => saw_align_items[4] = true,
                AlignItems::End => saw_align_items[5] = true,
                AlignItems::Baseline => saw_align_items[6] = true,
            }
            saw_gap |= matches!(node.style.row_gap, Length::Points(value) if (value - 3.0).abs() < 0.01)
                && matches!(node.style.column_gap, Length::Points(value) if (value - 2.0).abs() < 0.01);
            saw_wrap_reverse_space_between |= node.style.flex_wrap == FlexWrap::WrapReverse
                && node.style.align_content == AlignContent::SpaceBetween;
        }

        assert!(saw_ltr);
        assert!(saw_rtl);
        assert!(
            saw_flex_direction.iter().all(|seen| *seen),
            "flex wrap alignment benchmark must cover every FlexDirection value"
        );
        assert!(
            saw_flex_wrap.iter().all(|seen| *seen),
            "flex wrap alignment benchmark must cover every FlexWrap value"
        );
        assert!(
            saw_align_content.iter().all(|seen| *seen),
            "flex wrap alignment benchmark must cover every AlignContent value"
        );
        assert!(
            saw_align_items.iter().all(|seen| *seen),
            "flex wrap alignment benchmark must cover every AlignItems value"
        );
        assert!(saw_gap);
        assert!(
            saw_wrap_reverse_space_between,
            "flex wrap alignment benchmark must cover wrap-reverse with space-between line distribution"
        );
    }

    #[test]
    fn benchmark_scenarios_cover_box_sizing_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "box_sizing_matrix")
            .expect("box-sizing benchmark scenario exists");
        for feature in [
            BenchFeature::AspectRatio,
            BenchFeature::Block,
            BenchFeature::BoxSizing,
            BenchFeature::Flex,
            BenchFeature::Grid,
            BenchFeature::Linear,
            BenchFeature::MinMax,
            BenchFeature::Relative,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "box-sizing benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(20);
        let mut saw_content_box = false;
        let mut saw_border_box = false;
        let mut saw_display = [false; 5];
        let mut saw_aspect_ratio = false;
        let mut saw_minmax = false;

        for node in &tree.nodes {
            match node.style.box_sizing {
                BoxSizing::ContentBox => saw_content_box = true,
                BoxSizing::BorderBox => saw_border_box = true,
            }
            match node.style.display {
                Display::Block => saw_display[0] = true,
                Display::Flex => saw_display[1] = true,
                Display::Linear => saw_display[2] = true,
                Display::Relative => saw_display[3] = true,
                Display::Grid => saw_display[4] = true,
                Display::None => {}
            }
            saw_aspect_ratio |= node.style.aspect_ratio.is_some();
            saw_minmax |= !matches!(node.style.min_width, Length::Auto)
                || !matches!(node.style.max_width, Length::Auto)
                || !matches!(node.style.min_height, Length::Auto)
                || !matches!(node.style.max_height, Length::Auto);
        }

        assert!(saw_content_box);
        assert!(saw_border_box);
        assert!(
            saw_display.iter().all(|seen| *seen),
            "box-sizing benchmark must cover block/flex/linear/relative/grid containers"
        );
        assert!(saw_aspect_ratio);
        assert!(saw_minmax);
    }

    #[test]
    fn benchmark_scenarios_cover_position_type_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "position_type_matrix")
            .expect("position-type benchmark scenario exists");
        for feature in [
            BenchFeature::Absolute,
            BenchFeature::Fixed,
            BenchFeature::OutOfFlow,
            BenchFeature::PercentCalc,
            BenchFeature::PositionType,
            BenchFeature::Relative,
            BenchFeature::Sticky,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "position-type benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(20);
        let mut saw_position = [false; 5];
        let mut saw_percent_or_calc_inset = false;
        for node in &tree.nodes {
            match node.style.position {
                PositionType::Static => saw_position[0] = true,
                PositionType::Relative => saw_position[1] = true,
                PositionType::Absolute => saw_position[2] = true,
                PositionType::Fixed => saw_position[3] = true,
                PositionType::Sticky => saw_position[4] = true,
            }
            saw_percent_or_calc_inset |=
                matches!(node.style.left, Length::Percent(_) | Length::Calc { .. })
                    || matches!(node.style.right, Length::Percent(_) | Length::Calc { .. })
                    || matches!(node.style.top, Length::Percent(_) | Length::Calc { .. })
                    || matches!(node.style.bottom, Length::Percent(_) | Length::Calc { .. });
        }

        assert!(
            saw_position.iter().all(|seen| *seen),
            "position-type benchmark must cover every PositionType value"
        );
        assert!(saw_percent_or_calc_inset);
    }

    #[test]
    fn benchmark_scenarios_cover_grid_item_alignment_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "grid_item_alignment_matrix")
            .expect("grid item alignment benchmark scenario exists");
        for feature in [
            BenchFeature::Alignment,
            BenchFeature::Direction,
            BenchFeature::Grid,
            BenchFeature::GridItemAlignment,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "grid item alignment benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(105);
        let mut saw_ltr = false;
        let mut saw_rtl = false;
        let mut saw_justify_items = [false; 5];
        let mut saw_align_items = [false; 7];
        let mut saw_justify_self_auto = false;
        let mut saw_align_self_baseline = false;

        for node in &tree.nodes {
            match node.style.direction {
                Direction::Ltr => saw_ltr = true,
                Direction::Rtl => saw_rtl = true,
            }
            if node.style.display == Display::Grid {
                match node.style.justify_items {
                    JustifyItems::Auto => saw_justify_items[0] = true,
                    JustifyItems::Stretch => saw_justify_items[1] = true,
                    JustifyItems::Start => saw_justify_items[2] = true,
                    JustifyItems::Center => saw_justify_items[3] = true,
                    JustifyItems::End => saw_justify_items[4] = true,
                }
                match node.style.align_items {
                    AlignItems::Stretch => saw_align_items[0] = true,
                    AlignItems::FlexStart => saw_align_items[1] = true,
                    AlignItems::Start => saw_align_items[2] = true,
                    AlignItems::Center => saw_align_items[3] = true,
                    AlignItems::FlexEnd => saw_align_items[4] = true,
                    AlignItems::End => saw_align_items[5] = true,
                    AlignItems::Baseline => saw_align_items[6] = true,
                }
            }
            saw_justify_self_auto |= node.style.justify_self == JustifyItems::Auto;
            saw_align_self_baseline |= node.style.align_self == Some(AlignItems::Baseline);
        }

        assert!(saw_ltr);
        assert!(saw_rtl);
        assert!(
            saw_justify_items.iter().all(|seen| *seen),
            "grid item alignment benchmark must cover every JustifyItems value"
        );
        assert!(
            saw_align_items.iter().all(|seen| *seen),
            "grid item alignment benchmark must cover every AlignItems value"
        );
        assert!(saw_justify_self_auto);
        assert!(saw_align_self_baseline);
    }

    #[test]
    fn benchmark_scenarios_cover_grid_content_alignment_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "grid_content_alignment_matrix")
            .expect("grid content alignment benchmark scenario exists");
        for feature in [
            BenchFeature::Alignment,
            BenchFeature::Direction,
            BenchFeature::Grid,
            BenchFeature::GridContentAlignment,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "grid content alignment benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(162);
        let mut saw_ltr = false;
        let mut saw_rtl = false;
        let mut saw_justify_content = [false; 9];
        let mut saw_align_content = [false; 9];
        let mut saw_extra_space = false;
        let mut saw_overflow = false;

        for node in &tree.nodes {
            if node.style.display != Display::Grid {
                continue;
            }
            match node.style.direction {
                Direction::Ltr => saw_ltr = true,
                Direction::Rtl => saw_rtl = true,
            }
            match node.style.justify_content {
                JustifyContent::FlexStart => saw_justify_content[0] = true,
                JustifyContent::Start => saw_justify_content[1] = true,
                JustifyContent::Center => saw_justify_content[2] = true,
                JustifyContent::FlexEnd => saw_justify_content[3] = true,
                JustifyContent::End => saw_justify_content[4] = true,
                JustifyContent::SpaceBetween => saw_justify_content[5] = true,
                JustifyContent::SpaceAround => saw_justify_content[6] = true,
                JustifyContent::SpaceEvenly => saw_justify_content[7] = true,
                JustifyContent::Stretch => saw_justify_content[8] = true,
            }
            match node.style.align_content {
                AlignContent::FlexStart => saw_align_content[0] = true,
                AlignContent::Start => saw_align_content[1] = true,
                AlignContent::Center => saw_align_content[2] = true,
                AlignContent::FlexEnd => saw_align_content[3] = true,
                AlignContent::End => saw_align_content[4] = true,
                AlignContent::SpaceBetween => saw_align_content[5] = true,
                AlignContent::SpaceAround => saw_align_content[6] = true,
                AlignContent::SpaceEvenly => saw_align_content[7] = true,
                AlignContent::Stretch => saw_align_content[8] = true,
            }

            let column_gap = match node.style.column_gap {
                Length::Points(value) => value,
                _ => 0.0,
            };
            let row_gap = match node.style.row_gap {
                Length::Points(value) => value,
                _ => 0.0,
            };
            let width = match node.style.width {
                Length::Points(value) => value,
                _ => 0.0,
            };
            let height = match node.style.height {
                Length::Points(value) => value,
                _ => 0.0,
            };
            saw_extra_space |= (width - 90.0).abs() < 0.01
                && (height - 60.0).abs() < 0.01
                && (column_gap - 2.0).abs() < 0.01
                && (row_gap - 3.0).abs() < 0.01;
            saw_overflow |= (width - 30.0).abs() < 0.01
                && (height - 24.0).abs() < 0.01
                && (column_gap - 10.0).abs() < 0.01
                && (row_gap - 10.0).abs() < 0.01;
        }

        assert!(saw_ltr);
        assert!(saw_rtl);
        assert!(
            saw_justify_content.iter().all(|seen| *seen),
            "grid content alignment benchmark must cover every JustifyContent value"
        );
        assert!(
            saw_align_content.iter().all(|seen| *seen),
            "grid content alignment benchmark must cover every AlignContent value"
        );
        assert!(saw_extra_space);
        assert!(saw_overflow);
    }

    #[test]
    fn benchmark_scenarios_cover_grid_auto_flow_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "grid_auto_flow_matrix")
            .expect("grid auto-flow benchmark scenario exists");
        for feature in [
            BenchFeature::Direction,
            BenchFeature::DisplayNone,
            BenchFeature::Grid,
            BenchFeature::GridAutoFlow,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "grid auto-flow benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(30);
        let mut saw_ltr = false;
        let mut saw_rtl = false;
        let mut saw_auto_flow = [false; 5];
        let mut saw_display_none = false;
        let mut saw_span = false;
        let mut saw_locked_line = false;
        let mut saw_implicit_line = false;

        for node in &tree.nodes {
            if node.style.display == Display::None {
                saw_display_none = true;
            }
            if node.style.display == Display::Grid {
                match node.style.direction {
                    Direction::Ltr => saw_ltr = true,
                    Direction::Rtl => saw_rtl = true,
                }
                match node.style.grid_auto_flow {
                    GridAutoFlow::Row => saw_auto_flow[0] = true,
                    GridAutoFlow::Column => saw_auto_flow[1] = true,
                    GridAutoFlow::Dense => saw_auto_flow[2] = true,
                    GridAutoFlow::RowDense => saw_auto_flow[3] = true,
                    GridAutoFlow::ColumnDense => saw_auto_flow[4] = true,
                }
            }
            saw_span |= node.style.grid_column_span > 1 || node.style.grid_row_span > 1;
            saw_locked_line |= matches!(
                (node.style.grid_column_start, node.style.grid_row_start),
                (Some(2), _) | (_, Some(1 | 2))
            );
            saw_implicit_line |= matches!(
                (node.style.grid_column_start, node.style.grid_row_start),
                (Some(-1 | 5), _) | (_, Some(-1))
            );
        }

        assert!(saw_ltr);
        assert!(saw_rtl);
        assert!(
            saw_auto_flow.iter().all(|seen| *seen),
            "grid auto-flow benchmark must cover every GridAutoFlow value"
        );
        assert!(saw_display_none);
        assert!(saw_span);
        assert!(saw_locked_line);
        assert!(saw_implicit_line);
    }

    #[test]
    fn benchmark_scenarios_cover_direction_performance_case() {
        let mut saw_direction_scenario = false;

        for scenario in BENCH_SCENARIOS {
            if !scenario.features.contains(&BenchFeature::Direction) {
                continue;
            }
            saw_direction_scenario = true;

            let tree = (scenario.build_tree)(9);
            let mut saw_ltr = false;
            let mut saw_rtl = false;
            for node in &tree.nodes {
                match node.style.direction {
                    Direction::Ltr => saw_ltr = true,
                    Direction::Rtl => saw_rtl = true,
                }
            }

            assert!(saw_ltr, "scenario {} must include LTR nodes", scenario.name);
            assert!(saw_rtl, "scenario {} must include RTL nodes", scenario.name);
        }

        assert!(saw_direction_scenario);
    }

    #[test]
    fn benchmark_scenarios_cover_linear_gravity_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "linear_gravity_matrix")
            .expect("linear gravity benchmark scenario exists");
        for feature in [
            BenchFeature::Direction,
            BenchFeature::Linear,
            BenchFeature::LinearGravity,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "linear gravity benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(88);
        let mut saw_ltr = false;
        let mut saw_rtl = false;
        let mut saw_linear_container = false;
        let mut saw_gravity = [false; 11];
        for node in &tree.nodes {
            if node.style.display != Display::Linear {
                continue;
            }
            saw_linear_container = true;
            match node.style.direction {
                Direction::Ltr => saw_ltr = true,
                Direction::Rtl => saw_rtl = true,
            }
            match node.style.linear_gravity {
                LinearGravity::None => saw_gravity[0] = true,
                LinearGravity::Top => saw_gravity[1] = true,
                LinearGravity::Bottom => saw_gravity[2] = true,
                LinearGravity::Left => saw_gravity[3] = true,
                LinearGravity::Right => saw_gravity[4] = true,
                LinearGravity::CenterVertical => saw_gravity[5] = true,
                LinearGravity::CenterHorizontal => saw_gravity[6] = true,
                LinearGravity::SpaceBetween => saw_gravity[7] = true,
                LinearGravity::Start => saw_gravity[8] = true,
                LinearGravity::End => saw_gravity[9] = true,
                LinearGravity::Center => saw_gravity[10] = true,
            }
        }

        assert!(saw_linear_container);
        assert!(saw_ltr);
        assert!(saw_rtl);
        assert!(
            saw_gravity.iter().all(|seen| *seen),
            "linear gravity benchmark must cover every LinearGravity value"
        );
    }

    #[test]
    fn benchmark_scenarios_cover_linear_layout_gravity_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "linear_layout_gravity_matrix")
            .expect("linear layout gravity benchmark scenario exists");
        for feature in [
            BenchFeature::Direction,
            BenchFeature::Linear,
            BenchFeature::LinearLayoutGravity,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "linear layout gravity benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(104);
        let mut saw_ltr = false;
        let mut saw_rtl = false;
        let mut saw_linear_container = false;
        let mut saw_layout_gravity = [false; 13];
        for node in &tree.nodes {
            if node.style.display == Display::Linear {
                saw_linear_container = true;
                match node.style.direction {
                    Direction::Ltr => saw_ltr = true,
                    Direction::Rtl => saw_rtl = true,
                }
            }
            match node.style.linear_layout_gravity {
                LinearLayoutGravity::None => saw_layout_gravity[0] = true,
                LinearLayoutGravity::Top => saw_layout_gravity[1] = true,
                LinearLayoutGravity::Bottom => saw_layout_gravity[2] = true,
                LinearLayoutGravity::Left => saw_layout_gravity[3] = true,
                LinearLayoutGravity::Right => saw_layout_gravity[4] = true,
                LinearLayoutGravity::CenterVertical => saw_layout_gravity[5] = true,
                LinearLayoutGravity::CenterHorizontal => saw_layout_gravity[6] = true,
                LinearLayoutGravity::FillVertical => saw_layout_gravity[7] = true,
                LinearLayoutGravity::FillHorizontal => saw_layout_gravity[8] = true,
                LinearLayoutGravity::Center => saw_layout_gravity[9] = true,
                LinearLayoutGravity::Stretch => saw_layout_gravity[10] = true,
                LinearLayoutGravity::Start => saw_layout_gravity[11] = true,
                LinearLayoutGravity::End => saw_layout_gravity[12] = true,
            }
        }

        assert!(saw_linear_container);
        assert!(saw_ltr);
        assert!(saw_rtl);
        assert!(
            saw_layout_gravity.iter().all(|seen| *seen),
            "linear layout gravity benchmark must cover every LinearLayoutGravity value"
        );
    }

    #[test]
    fn benchmark_scenarios_cover_linear_cross_gravity_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "linear_cross_gravity_matrix")
            .expect("linear cross gravity benchmark scenario exists");
        for feature in [
            BenchFeature::Direction,
            BenchFeature::Linear,
            BenchFeature::LinearCrossGravity,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "linear cross gravity benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(40);
        let mut saw_ltr = false;
        let mut saw_rtl = false;
        let mut saw_linear_container = false;
        let mut saw_cross_gravity = [false; 5];
        for node in &tree.nodes {
            if node.style.display != Display::Linear {
                continue;
            }
            saw_linear_container = true;
            match node.style.direction {
                Direction::Ltr => saw_ltr = true,
                Direction::Rtl => saw_rtl = true,
            }
            match node.style.linear_cross_gravity {
                LinearCrossGravity::None => saw_cross_gravity[0] = true,
                LinearCrossGravity::Start => saw_cross_gravity[1] = true,
                LinearCrossGravity::End => saw_cross_gravity[2] = true,
                LinearCrossGravity::Center => saw_cross_gravity[3] = true,
                LinearCrossGravity::Stretch => saw_cross_gravity[4] = true,
            }
        }

        assert!(saw_linear_container);
        assert!(saw_ltr);
        assert!(saw_rtl);
        assert!(
            saw_cross_gravity.iter().all(|seen| *seen),
            "linear cross gravity benchmark must cover every LinearCrossGravity value"
        );
    }

    #[test]
    fn benchmark_scenarios_cover_staggered_list_component_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "staggered_linear_list")
            .expect("staggered linear list benchmark scenario exists");
        for feature in [
            BenchFeature::Linear,
            BenchFeature::ListComponent,
            BenchFeature::StaggeredLinear,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "staggered list component benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(31);
        let mut saw_header = false;
        let mut saw_footer = false;
        let mut saw_list_row = false;
        let mut saw_default = false;
        let mut saw_regular = false;
        for node in &tree.nodes {
            match node.style.list_component_type {
                Some(ListComponentType::Header) => saw_header = true,
                Some(ListComponentType::Footer) => saw_footer = true,
                Some(ListComponentType::ListRow) => saw_list_row = true,
                Some(ListComponentType::Default) => saw_default = true,
                None if node.style.display != Display::Linear => saw_regular = true,
                None => {}
            }
        }

        assert!(saw_header);
        assert!(saw_footer);
        assert!(saw_list_row);
        assert!(saw_default);
        assert!(saw_regular);
    }

    #[test]
    fn benchmark_scenarios_cover_raw_intrinsic_list_gap_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "staggered_linear_raw_list_gaps")
            .expect("raw/intrinsic list-gap benchmark scenario exists");
        for feature in [
            BenchFeature::FitContent,
            BenchFeature::IntrinsicSizing,
            BenchFeature::Linear,
            BenchFeature::StaggeredLinear,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "raw list-gap benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(16);
        let mut saw_auto = false;
        let mut saw_fr = false;
        let mut saw_max_content = false;
        let mut saw_fit_content = false;
        for node in &tree.nodes {
            if node.style.linear_column_count.is_none() {
                continue;
            }
            match node.style.list_cross_axis_gap {
                Length::Auto => saw_auto = true,
                Length::Fr(_) => saw_fr = true,
                Length::MaxContent => saw_max_content = true,
                Length::FitContent(Some(_)) => saw_fit_content = true,
                _ => {}
            }
        }

        assert!(saw_auto);
        assert!(saw_fr);
        assert!(saw_max_content);
        assert!(saw_fit_content);
    }

    #[test]
    fn benchmark_scenarios_cover_relative_center_performance_case() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "relative_center_matrix")
            .expect("relative-center benchmark scenario exists");
        for feature in [
            BenchFeature::Alignment,
            BenchFeature::MeasuredContent,
            BenchFeature::Relative,
            BenchFeature::RelativeCenter,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "relative-center benchmark must declare {feature:?} coverage"
            );
        }

        let tree = (scenario.build_tree)(16);
        let mut saw_center = [false; 4];
        let mut saw_parent_edge = false;
        let mut saw_measured = false;
        for node in &tree.nodes {
            match node.style.relative_center {
                RelativeCenter::None => saw_center[0] = true,
                RelativeCenter::Horizontal => saw_center[1] = true,
                RelativeCenter::Vertical => saw_center[2] = true,
                RelativeCenter::Both => saw_center[3] = true,
            }
            saw_parent_edge |= node.style.relative_align_left == RELATIVE_ALIGN_PARENT
                || node.style.relative_align_right == RELATIVE_ALIGN_PARENT
                || node.style.relative_align_top == RELATIVE_ALIGN_PARENT
                || node.style.relative_align_bottom == RELATIVE_ALIGN_PARENT;
            saw_measured |= node.measured_size.is_some();
        }

        assert!(
            saw_center.iter().all(|seen| *seen),
            "relative-center benchmark must cover every RelativeCenter value"
        );
        assert!(saw_parent_edge);
        assert!(saw_measured);
    }

    #[test]
    fn benchmark_scenarios_cover_indefinite_grid_auto_fit_content_max_caps() {
        let scenario = BENCH_SCENARIOS
            .iter()
            .find(|scenario| scenario.name == "grid_indefinite_auto_fit_content_max_tracks")
            .expect("indefinite grid auto fit-content max benchmark scenario exists");
        for feature in [
            BenchFeature::FitContent,
            BenchFeature::Grid,
            BenchFeature::IntrinsicSizing,
            BenchFeature::MeasuredContent,
            BenchFeature::MinMax,
        ] {
            assert!(
                scenario.features.contains(&feature),
                "indefinite grid auto fit-content max benchmark must declare {feature:?} coverage"
            );
        }

        assert_eq!(
            (scenario.constraints)(16).width.mode,
            MeasureMode::Indefinite
        );
        assert_eq!(
            (scenario.constraints)(16).height.mode,
            MeasureMode::Indefinite
        );

        let tree = (scenario.build_tree)(16);
        let root_style = &tree.nodes[0].style;
        let saw_column_fixed_fit_content = root_style.grid_auto_columns_max.iter().any(|length| {
            matches!(
                length,
                Length::FitContent(Some(base))
                    if (base.fixed_part() - 40.0).abs() < 0.01
                        && !base.contains_percentage()
            )
        });
        let saw_row_fixed_fit_content = root_style.grid_auto_rows_max.iter().any(|length| {
            matches!(
                length,
                Length::FitContent(Some(base))
                    if (base.fixed_part() - 40.0).abs() < 0.01
                        && !base.contains_percentage()
            )
        });

        assert!(saw_column_fixed_fit_content);
        assert!(saw_row_fixed_fit_content);
    }

    #[test]
    fn env_flag_value_accepts_common_false_values() {
        assert!(!env_flag_value(""));
        assert!(!env_flag_value("0"));
        assert!(!env_flag_value("false"));
        assert!(!env_flag_value(" FALSE "));
        assert!(env_flag_value("1"));
        assert!(env_flag_value("true"));
        assert!(env_flag_value("required"));
    }

    #[test]
    fn min_speedup_value_accepts_only_positive_finite_values() {
        assert_eq!(parse_min_speedup_value("1"), Some(1.0));
        assert_eq!(parse_min_speedup_value(" 1.25 "), Some(1.25));
        assert_eq!(parse_min_speedup_value("0.01"), Some(0.01));
        assert_eq!(parse_min_speedup_value("0"), None);
        assert_eq!(parse_min_speedup_value("-1"), None);
        assert_eq!(parse_min_speedup_value("nan"), None);
        assert_eq!(parse_min_speedup_value("inf"), None);
        assert_eq!(parse_min_speedup_value("required"), None);
    }

    fn minimum_nodes_for_scenario(name: &str) -> usize {
        match name {
            "nested_column_flex" => 12,
            "sticky_percent_insets" => 25,
            "mixed_display_none" => 33,
            "out_of_flow_intrinsic" | "grid_out_of_flow_intrinsic" => 17,
            "grid_auto_margin_alignment" => 25,
            _ => 9,
        }
    }

    #[test]
    fn helper_constraints_match_scenario_shapes() {
        assert_eq!(integer_sqrt_ceil(1), 1);
        assert_eq!(integer_sqrt_ceil(15), 4);
        assert_eq!(integer_sqrt_ceil(16), 4);

        assert_eq!(
            wide_definite_constraints(0).width,
            SideConstraint::definite(1.0)
        );
        assert_eq!(at_most_constraints(4).width, SideConstraint::at_most(16.0));
        assert_eq!(
            at_most_two_axis_constraints(8).width,
            SideConstraint::at_most(320.0)
        );
        assert_eq!(
            at_most_two_axis_constraints(8).height,
            SideConstraint::at_most(220.0)
        );
        assert_eq!(absolute_constraints(128).height.mode, MeasureMode::Definite);
        assert_eq!(
            relative_constraints(8).height,
            SideConstraint::definite(160.0)
        );
        assert_eq!(
            sticky_constraints(8).height,
            SideConstraint::definite(240.0)
        );
        assert_eq!(
            out_of_flow_fill_constraints(8).height,
            SideConstraint::definite(240.0)
        );
        assert_eq!(
            grid_intrinsic_constraints(8).height,
            SideConstraint::definite(160.0)
        );
        assert_eq!(
            indefinite_constraints(8).width.mode,
            MeasureMode::Indefinite
        );
        assert_eq!(
            indefinite_constraints(8).height.mode,
            MeasureMode::Indefinite
        );
    }
}
