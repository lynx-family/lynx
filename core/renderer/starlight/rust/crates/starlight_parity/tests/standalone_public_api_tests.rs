// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#![forbid(unsafe_code)]

#[cfg(feature = "native-standalone")]
use std::sync::atomic::{AtomicU32, AtomicUsize, Ordering};

#[cfg(feature = "native-standalone")]
use starlight_cpp::{
    CppStarlightEngine, StandalonePublicBoxAspectLayoutSnapshot, StandalonePublicConfigSnapshot,
    StandalonePublicDimensionLayoutSnapshot, StandalonePublicDimensionStyleSnapshot,
    StandalonePublicDirectionLayoutSnapshot, StandalonePublicDirectionSnapshot,
    StandalonePublicDirtySnapshot, StandalonePublicDisplayLayoutSnapshot,
    StandalonePublicEdgeLayoutSnapshot, StandalonePublicEdgeStyleSnapshot,
    StandalonePublicEdgeStyleVariantSnapshot, StandalonePublicFlexLayoutSnapshot,
    StandalonePublicGridAlignmentLayoutSnapshot, StandalonePublicGridTrackLayoutSnapshot,
    StandalonePublicLayoutEntrypointSnapshot, StandalonePublicLayoutEntrypointStageSnapshot,
    StandalonePublicLayoutGetterSnapshot, StandalonePublicLayoutNodeSnapshot,
    StandalonePublicLengthValue, StandalonePublicLinearLayoutSnapshot,
    StandalonePublicLinearListLayoutSnapshot, StandalonePublicMeasureDelegateSnapshot,
    StandalonePublicPositionLayoutSnapshot, StandalonePublicRelativeLayoutSnapshot,
    StandalonePublicScalarStyleSnapshot, StandalonePublicStyleStage, StandalonePublicTreeSnapshot,
    StandalonePublicTreeStage,
};
#[cfg(feature = "native-standalone")]
use starlight_layout::{
    AlignContent, AlignItems, BaseLength, BoxSizing, Direction, Display, FlexDirection, FlexWrap,
    GridAutoFlow, JustifyContent, JustifyItems, Length, LinearCrossGravity, LinearGravity,
    LinearLayoutGravity, LinearOrientation, ListComponentType, MeasureMode, PositionType,
    RelativeCenter, SideConstraint, Size, RELATIVE_ALIGN_PARENT,
};
#[cfg(feature = "native-standalone")]
use starlight_standalone::{
    NodeId, StandaloneConfig, StandaloneEdge, StandaloneGap, StandaloneTree,
};

#[cfg(feature = "native-standalone")]
static PUBLIC_MEASURE_CALL_COUNT: AtomicUsize = AtomicUsize::new(0);
#[cfg(feature = "native-standalone")]
static PUBLIC_BASELINE_CALL_COUNT: AtomicUsize = AtomicUsize::new(0);
#[cfg(feature = "native-standalone")]
static PUBLIC_MEASURE_WIDTH_BITS: AtomicU32 = AtomicU32::new(0);
#[cfg(feature = "native-standalone")]
static PUBLIC_MEASURE_WIDTH_MODE: AtomicU32 = AtomicU32::new(0);
#[cfg(feature = "native-standalone")]
static PUBLIC_MEASURE_HEIGHT_BITS: AtomicU32 = AtomicU32::new(0);
#[cfg(feature = "native-standalone")]
static PUBLIC_MEASURE_HEIGHT_MODE: AtomicU32 = AtomicU32::new(0);
#[cfg(feature = "native-standalone")]
static PUBLIC_BASELINE_WIDTH_BITS: AtomicU32 = AtomicU32::new(0);
#[cfg(feature = "native-standalone")]
static PUBLIC_BASELINE_HEIGHT_BITS: AtomicU32 = AtomicU32::new(0);
#[cfg(feature = "native-standalone")]
static PUBLIC_ENTRYPOINT_MEASURE_CALL_COUNT: AtomicUsize = AtomicUsize::new(0);
#[cfg(feature = "native-standalone")]
static PUBLIC_ENTRYPOINT_MEASURE_WIDTH_BITS: AtomicU32 = AtomicU32::new(0);
#[cfg(feature = "native-standalone")]
static PUBLIC_ENTRYPOINT_MEASURE_WIDTH_MODE: AtomicU32 = AtomicU32::new(0);
#[cfg(feature = "native-standalone")]
static PUBLIC_ENTRYPOINT_MEASURE_HEIGHT_BITS: AtomicU32 = AtomicU32::new(0);
#[cfg(feature = "native-standalone")]
static PUBLIC_ENTRYPOINT_MEASURE_HEIGHT_MODE: AtomicU32 = AtomicU32::new(0);

#[cfg(feature = "native-standalone")]
const PUBLIC_DISPLAY_VALUES: usize = 6;
#[cfg(feature = "native-standalone")]
const PUBLIC_EXPLICIT_DIRECTION_VALUES: usize = 2;
#[cfg(feature = "native-standalone")]
const PUBLIC_EDGE_VALUE_VARIANTS: usize = 7;
#[cfg(feature = "native-standalone")]
const PUBLIC_DIMENSION_VALUE_VARIANTS: usize = 8;
#[cfg(feature = "native-standalone")]
const PUBLIC_FLEX_DIRECTION_VALUES: usize = 4;
#[cfg(feature = "native-standalone")]
const PUBLIC_FLEX_WRAP_VALUES: usize = 3;
#[cfg(feature = "native-standalone")]
const PUBLIC_JUSTIFY_CONTENT_VALUES: usize = 9;
#[cfg(feature = "native-standalone")]
const PUBLIC_ALIGN_ITEMS_VALUES: usize = 7;
#[cfg(feature = "native-standalone")]
const PUBLIC_ALIGN_CONTENT_CPP_VALUES: usize = 7;
#[cfg(feature = "native-standalone")]
const PUBLIC_ALIGN_CONTENT_SAFE_VALUES: usize = 9;
#[cfg(feature = "native-standalone")]
const PUBLIC_ALIGN_SELF_VALUES: usize = 8;
#[cfg(feature = "native-standalone")]
const PUBLIC_LINEAR_ORIENTATION_VALUES: usize = 8;
#[cfg(feature = "native-standalone")]
const PUBLIC_LINEAR_GRAVITY_VALUES: usize = 11;
#[cfg(feature = "native-standalone")]
const PUBLIC_LINEAR_LAYOUT_GRAVITY_VALUES: usize = 13;
#[cfg(feature = "native-standalone")]
const PUBLIC_LINEAR_CROSS_GRAVITY_VALUES: usize = 5;
#[cfg(feature = "native-standalone")]
const PUBLIC_LIST_GAP_VALUE_VARIANTS: usize = 7;
#[cfg(feature = "native-standalone")]
const PUBLIC_GRID_AUTO_FLOW_VALUES: usize = 5;
#[cfg(feature = "native-standalone")]
const PUBLIC_JUSTIFY_ITEMS_VALUES: usize = 5;
#[cfg(feature = "native-standalone")]
const PUBLIC_LINEAR_LAYOUT_SNAPSHOT_COUNT: usize = 2
    + 4
    + PUBLIC_LINEAR_ORIENTATION_VALUES
    + PUBLIC_LINEAR_GRAVITY_VALUES * 2
    + PUBLIC_LINEAR_CROSS_GRAVITY_VALUES * 2
    + PUBLIC_LINEAR_LAYOUT_GRAVITY_VALUES * 2;
#[cfg(feature = "native-standalone")]
const PUBLIC_FLEX_LAYOUT_SNAPSHOT_COUNT: usize = 4
    + PUBLIC_FLEX_WRAP_VALUES
    + PUBLIC_ALIGN_CONTENT_SAFE_VALUES
    + PUBLIC_FLEX_DIRECTION_VALUES
    + PUBLIC_JUSTIFY_CONTENT_VALUES
    + PUBLIC_ALIGN_ITEMS_VALUES
    + 1
    + PUBLIC_ALIGN_SELF_VALUES
    + 2;
#[cfg(feature = "native-standalone")]
const PUBLIC_GRID_ALIGNMENT_VARIANT_SNAPSHOT_COUNT: usize = PUBLIC_JUSTIFY_CONTENT_VALUES
    + PUBLIC_ALIGN_CONTENT_CPP_VALUES
    + PUBLIC_JUSTIFY_ITEMS_VALUES
    + PUBLIC_ALIGN_ITEMS_VALUES
    + PUBLIC_JUSTIFY_ITEMS_VALUES
    + PUBLIC_ALIGN_SELF_VALUES;

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_layout_variant_matrices_cover_public_values() {
    let mut cpp = CppStarlightEngine::new();

    assert_eq!(
        cpp.standalone_public_display_layout_snapshots()
            .expect("C++ display layout snapshots")
            .len(),
        PUBLIC_DISPLAY_VALUES
    );
    assert_eq!(
        rust_public_display_layout_snapshots().len(),
        PUBLIC_DISPLAY_VALUES
    );
    assert_eq!(
        cpp.standalone_public_direction_layout_snapshots()
            .expect("C++ direction layout snapshots")
            .len(),
        PUBLIC_EXPLICIT_DIRECTION_VALUES
    );
    assert_eq!(
        rust_public_direction_layout_snapshots().len(),
        PUBLIC_EXPLICIT_DIRECTION_VALUES
    );
    assert_eq!(
        cpp.standalone_public_edge_style_variant_snapshots()
            .expect("C++ edge style variant snapshots")
            .len(),
        PUBLIC_EDGE_VALUE_VARIANTS
    );
    assert_eq!(
        rust_public_edge_style_variant_snapshots().len(),
        PUBLIC_EDGE_VALUE_VARIANTS
    );
    assert_eq!(
        cpp.standalone_public_edge_layout_snapshots()
            .expect("C++ edge layout snapshots")
            .len(),
        PUBLIC_EDGE_VALUE_VARIANTS
    );
    assert_eq!(
        rust_public_edge_layout_snapshots().len(),
        PUBLIC_EDGE_VALUE_VARIANTS
    );
    assert_eq!(
        cpp.standalone_public_dimension_style_variant_snapshots()
            .expect("C++ dimension style variant snapshots")
            .len(),
        PUBLIC_DIMENSION_VALUE_VARIANTS
    );
    assert_eq!(
        rust_public_dimension_style_variant_snapshots().len(),
        PUBLIC_DIMENSION_VALUE_VARIANTS
    );
    assert_eq!(
        cpp.standalone_public_dimension_layout_snapshots()
            .expect("C++ dimension layout snapshots")
            .len(),
        PUBLIC_DIMENSION_VALUE_VARIANTS
    );
    assert_eq!(
        rust_public_dimension_layout_snapshots().len(),
        PUBLIC_DIMENSION_VALUE_VARIANTS
    );

    assert_eq!(
        cpp.standalone_public_linear_layout_snapshots()
            .expect("C++ linear layout snapshots")
            .len(),
        PUBLIC_LINEAR_LAYOUT_SNAPSHOT_COUNT
    );
    assert_eq!(
        rust_public_linear_layout_snapshots().len(),
        PUBLIC_LINEAR_LAYOUT_SNAPSHOT_COUNT
    );
    assert_eq!(
        rust_public_linear_orientation_layout_snapshots().len(),
        PUBLIC_LINEAR_ORIENTATION_VALUES
    );
    assert_eq!(
        rust_public_linear_main_gravity_layout_snapshots().len(),
        PUBLIC_LINEAR_GRAVITY_VALUES * 2
    );
    assert_eq!(
        rust_public_linear_cross_gravity_layout_snapshots().len(),
        PUBLIC_LINEAR_CROSS_GRAVITY_VALUES * 2
    );
    assert_eq!(
        rust_public_linear_layout_gravity_variant_layout_snapshots().len(),
        PUBLIC_LINEAR_LAYOUT_GRAVITY_VALUES * 2
    );

    assert_eq!(
        cpp.standalone_public_flex_layout_snapshots()
            .expect("C++ flex layout snapshots")
            .len(),
        PUBLIC_FLEX_LAYOUT_SNAPSHOT_COUNT
    );
    assert_eq!(
        rust_public_flex_layout_snapshots().len(),
        PUBLIC_FLEX_LAYOUT_SNAPSHOT_COUNT
    );
    assert_eq!(
        rust_public_flex_wrap_layout_snapshots().len(),
        PUBLIC_FLEX_WRAP_VALUES
    );
    assert_eq!(
        rust_public_flex_align_content_variant_layout_snapshots().len(),
        PUBLIC_ALIGN_CONTENT_SAFE_VALUES
    );
    assert_eq!(
        rust_public_flex_direction_layout_snapshots().len(),
        PUBLIC_FLEX_DIRECTION_VALUES
    );
    assert_eq!(
        rust_public_flex_justify_content_layout_snapshots().len(),
        PUBLIC_JUSTIFY_CONTENT_VALUES
    );
    assert_eq!(
        rust_public_flex_align_items_layout_snapshots().len(),
        PUBLIC_ALIGN_ITEMS_VALUES
    );
    assert_eq!(
        rust_public_flex_align_self_variant_layout_snapshots().len(),
        PUBLIC_ALIGN_SELF_VALUES
    );

    assert_eq!(
        cpp.standalone_public_list_gap_layout_snapshots()
            .expect("C++ list-gap layout snapshots")
            .len(),
        PUBLIC_LIST_GAP_VALUE_VARIANTS
    );
    assert_eq!(
        rust_public_list_gap_layout_snapshots().len(),
        PUBLIC_LIST_GAP_VALUE_VARIANTS
    );
    assert_eq!(
        cpp.standalone_public_grid_auto_flow_layout_snapshots()
            .expect("C++ grid auto-flow layout snapshots")
            .len(),
        PUBLIC_GRID_AUTO_FLOW_VALUES
    );
    assert_eq!(
        rust_public_grid_auto_flow_layout_snapshots().len(),
        PUBLIC_GRID_AUTO_FLOW_VALUES
    );
    assert_eq!(
        cpp.standalone_public_grid_alignment_variant_layout_snapshots()
            .expect("C++ grid alignment variant layout snapshots")
            .len(),
        PUBLIC_GRID_ALIGNMENT_VARIANT_SNAPSHOT_COUNT
    );
    assert_eq!(
        rust_public_grid_alignment_variant_layout_snapshots().len(),
        PUBLIC_GRID_ALIGNMENT_VARIANT_SNAPSHOT_COUNT
    );
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_tree_mutation_api_matches_cpp() {
    let cpp_snapshots = CppStarlightEngine::new()
        .standalone_public_tree_mutation_snapshots()
        .expect("C++ public standalone tree mutation snapshots");
    let rust_snapshots = rust_public_tree_mutation_snapshots();

    assert_eq!(cpp_snapshots, rust_snapshots);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_dirty_api_matches_cpp() {
    let cpp_snapshot = CppStarlightEngine::new()
        .standalone_public_dirty_snapshot()
        .expect("C++ public standalone dirty snapshot");
    let rust_snapshot = rust_public_dirty_snapshot();

    assert_eq!(cpp_snapshot, rust_snapshot);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_edge_style_api_matches_cpp() {
    let cpp_snapshots = CppStarlightEngine::new()
        .standalone_public_edge_style_snapshots()
        .expect("C++ public standalone edge style snapshots");
    let rust_snapshots = rust_public_edge_style_snapshots();

    assert_eq!(cpp_snapshots, rust_snapshots);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_edge_style_variant_apis_match_cpp() {
    let cpp_snapshots = CppStarlightEngine::new()
        .standalone_public_edge_style_variant_snapshots()
        .expect("C++ public standalone edge style variant snapshots");
    let rust_snapshots = rust_public_edge_style_variant_snapshots();

    assert_eq!(cpp_snapshots, rust_snapshots);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_scalar_style_api_matches_cpp() {
    let cpp_snapshot = CppStarlightEngine::new()
        .standalone_public_scalar_style_snapshot()
        .expect("C++ public standalone scalar style snapshot");
    let rust_snapshot = rust_public_scalar_style_snapshot();

    assert_eq!(cpp_snapshot, rust_snapshot);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_dimension_style_api_matches_cpp() {
    let cpp_snapshot = CppStarlightEngine::new()
        .standalone_public_dimension_style_snapshot()
        .expect("C++ public standalone dimension style snapshot");
    let rust_snapshot = rust_public_dimension_style_snapshot();

    assert_eq!(cpp_snapshot, rust_snapshot);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_dimension_style_variant_apis_match_cpp() {
    let cpp_snapshots = CppStarlightEngine::new()
        .standalone_public_dimension_style_variant_snapshots()
        .expect("C++ public standalone dimension style variant snapshots");
    let rust_snapshots = rust_public_dimension_style_variant_snapshots();

    assert_eq!(cpp_snapshots, rust_snapshots);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_dimension_layout_apis_match_cpp() {
    let cpp_snapshots = CppStarlightEngine::new()
        .standalone_public_dimension_layout_snapshots()
        .expect("C++ public standalone dimension layout snapshots");
    let rust_snapshots = rust_public_dimension_layout_snapshots();

    assert_eq!(cpp_snapshots, rust_snapshots);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_direction_api_matches_cpp() {
    let cpp_snapshot = CppStarlightEngine::new()
        .standalone_public_direction_snapshot()
        .expect("C++ public standalone direction snapshot");
    let rust_snapshot = rust_public_direction_snapshot();

    assert_eq!(cpp_snapshot.default_is_rtl, rust_snapshot.default_is_rtl);
    assert_eq!(cpp_snapshot.rtl_is_rtl, rust_snapshot.rtl_is_rtl);
    assert_eq!(cpp_snapshot.ltr_is_rtl, rust_snapshot.ltr_is_rtl);
    assert_eq!(
        cpp_snapshot.dirty_after_direction_updates,
        rust_snapshot.dirty_after_direction_updates
    );
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_direction_layout_apis_match_cpp() {
    let cpp_snapshots = CppStarlightEngine::new()
        .standalone_public_direction_layout_snapshots()
        .expect("C++ public standalone direction layout snapshots");
    let rust_snapshots = rust_public_direction_layout_snapshots();

    assert_eq!(cpp_snapshots, rust_snapshots);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_layout_getter_api_matches_cpp() {
    let cpp_snapshot = CppStarlightEngine::new()
        .standalone_public_layout_getter_snapshot()
        .expect("C++ public standalone layout getter snapshot");
    let rust_snapshot = rust_public_layout_getter_snapshot();

    assert_eq!(cpp_snapshot, rust_snapshot);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_box_aspect_layout_api_matches_cpp() {
    let cpp_snapshot = CppStarlightEngine::new()
        .standalone_public_box_aspect_layout_snapshot()
        .expect("C++ public standalone box/aspect layout snapshot");
    let rust_snapshot = rust_public_box_aspect_layout_snapshot();

    assert_eq!(cpp_snapshot, rust_snapshot);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_display_layout_apis_match_cpp() {
    let cpp_snapshots = CppStarlightEngine::new()
        .standalone_public_display_layout_snapshots()
        .expect("C++ public standalone display layout snapshots");
    let rust_snapshots = rust_public_display_layout_snapshots();

    assert_eq!(cpp_snapshots, rust_snapshots);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_position_layout_api_matches_cpp() {
    let cpp_snapshot = CppStarlightEngine::new()
        .standalone_public_position_layout_snapshot()
        .expect("C++ public standalone position layout snapshot");
    let rust_snapshot = rust_public_position_layout_snapshot();

    assert_eq!(cpp_snapshot, rust_snapshot);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_relative_layout_apis_match_cpp() {
    let cpp_snapshots = CppStarlightEngine::new()
        .standalone_public_relative_layout_snapshots()
        .expect("C++ public standalone relative layout snapshots");
    let rust_snapshots = rust_public_relative_layout_snapshots();

    assert_eq!(cpp_snapshots, rust_snapshots);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_flex_layout_apis_match_cpp() {
    let cpp_snapshots = CppStarlightEngine::new()
        .standalone_public_flex_layout_snapshots()
        .expect("C++ public standalone flex layout snapshots");
    let rust_snapshots = rust_public_flex_layout_snapshots();

    assert_eq!(cpp_snapshots, rust_snapshots);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_edge_layout_apis_match_cpp() {
    let cpp_snapshots = CppStarlightEngine::new()
        .standalone_public_edge_layout_snapshots()
        .expect("C++ public standalone edge layout snapshots");
    let rust_snapshots = rust_public_edge_layout_snapshots();

    assert_eq!(cpp_snapshots, rust_snapshots);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_linear_layout_apis_match_cpp() {
    let cpp_snapshots = CppStarlightEngine::new()
        .standalone_public_linear_layout_snapshots()
        .expect("C++ public standalone linear layout snapshots");
    let rust_snapshots = rust_public_linear_layout_snapshots();

    assert_eq!(cpp_snapshots, rust_snapshots);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_linear_list_layout_api_matches_cpp() {
    let cpp_snapshot = CppStarlightEngine::new()
        .standalone_public_linear_list_layout_snapshot()
        .expect("C++ public standalone linear/list layout snapshot");
    let rust_snapshot = rust_public_linear_list_layout_snapshot();

    assert_eq!(cpp_snapshot, rust_snapshot);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_list_gap_layout_apis_match_cpp() {
    let cpp_snapshots = CppStarlightEngine::new()
        .standalone_public_list_gap_layout_snapshots()
        .expect("C++ public standalone list-gap layout snapshots");
    let rust_snapshots = rust_public_list_gap_layout_snapshots();

    assert_eq!(cpp_snapshots, rust_snapshots);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_grid_track_layout_api_matches_cpp() {
    let cpp_snapshot = CppStarlightEngine::new()
        .standalone_public_grid_track_layout_snapshot()
        .expect("C++ public standalone grid track layout snapshot");
    let rust_snapshot = rust_public_grid_track_layout_snapshot();

    assert_eq!(cpp_snapshot, rust_snapshot);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_grid_auto_flow_layout_apis_match_cpp() {
    let cpp_snapshots = CppStarlightEngine::new()
        .standalone_public_grid_auto_flow_layout_snapshots()
        .expect("C++ public standalone grid auto-flow layout snapshots");
    let rust_snapshots = rust_public_grid_auto_flow_layout_snapshots();

    assert_eq!(cpp_snapshots, rust_snapshots);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_grid_alignment_layout_api_matches_cpp() {
    let cpp_snapshot = CppStarlightEngine::new()
        .standalone_public_grid_alignment_layout_snapshot()
        .expect("C++ public standalone grid alignment layout snapshot");
    let rust_snapshot = rust_public_grid_alignment_layout_snapshot();

    assert_eq!(cpp_snapshot, rust_snapshot);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_grid_alignment_variant_layout_apis_match_cpp() {
    let cpp_snapshots = CppStarlightEngine::new()
        .standalone_public_grid_alignment_variant_layout_snapshots()
        .expect("C++ public standalone grid alignment variant layout snapshots");
    let rust_snapshots = rust_public_grid_alignment_variant_layout_snapshots();

    assert_eq!(cpp_snapshots, rust_snapshots);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_layout_entrypoint_api_matches_cpp() {
    let cpp_snapshot = CppStarlightEngine::new()
        .standalone_public_layout_entrypoint_snapshot()
        .expect("C++ public standalone layout entrypoint snapshot");
    let rust_snapshot = rust_public_layout_entrypoint_snapshot();

    assert_eq!(cpp_snapshot, rust_snapshot);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_config_api_matches_cpp() {
    let cpp_snapshot = CppStarlightEngine::new()
        .standalone_public_config_snapshot()
        .expect("C++ public standalone config snapshot");
    let rust_snapshot = rust_public_config_snapshot();

    assert_eq!(cpp_snapshot, rust_snapshot);
}

#[test]
#[cfg(feature = "native-standalone")]
fn rust_standalone_public_measure_delegate_api_matches_cpp() {
    let cpp_snapshot = CppStarlightEngine::new()
        .standalone_public_measure_delegate_snapshot()
        .expect("C++ public standalone measure delegate snapshot");
    let rust_snapshot = rust_public_measure_delegate_snapshot();

    assert_eq!(cpp_snapshot, rust_snapshot);
}

#[cfg(feature = "native-standalone")]
fn rust_public_tree_mutation_snapshots() -> Vec<StandalonePublicTreeSnapshot> {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let inserted = tree.create_default_node();
    let staging = tree.create_default_node();
    let nodes = [root, first, second, third, inserted, staging];
    let mut snapshots = Vec::new();

    tree.append_child(root, first).expect("append first child");
    tree.append_child(root, second)
        .expect("append second child");
    tree.append_child(root, third).expect("append third child");
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterAppend,
        &tree,
        &nodes,
        root,
        staging,
    ));

    calculate_probe_layout(&mut tree, root);
    calculate_probe_layout(&mut tree, staging);
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterCleanLayout,
        &tree,
        &nodes,
        root,
        staging,
    ));

    tree.remove_child(staging, second)
        .expect("no-op remove from wrong parent");
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterNoopRemove,
        &tree,
        &nodes,
        root,
        staging,
    ));

    tree.insert_child(root, inserted, 1)
        .expect("insert child at index");
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterIndexInsert,
        &tree,
        &nodes,
        root,
        staging,
    ));

    calculate_probe_layout(&mut tree, root);
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterCleanInsertedLayout,
        &tree,
        &nodes,
        root,
        staging,
    ));

    tree.append_child(staging, inserted)
        .expect("reparent inserted child to staging");
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterReparentToStaging,
        &tree,
        &nodes,
        root,
        staging,
    ));

    tree.insert_child_before(root, inserted, second)
        .expect("reparent inserted child before second");
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterInsertBefore,
        &tree,
        &nodes,
        root,
        staging,
    ));

    tree.remove_child(root, second)
        .expect("remove attached child");
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterRemoveChild,
        &tree,
        &nodes,
        root,
        staging,
    ));

    tree.remove_all_children(root)
        .expect("remove all root children");
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterRemoveAllChildren,
        &tree,
        &nodes,
        root,
        staging,
    ));

    tree.reset_node(staging).expect("reset staging node");
    snapshots.push(snapshot_public_tree_stage(
        StandalonePublicTreeStage::AfterResetStaging,
        &tree,
        &nodes,
        root,
        staging,
    ));

    snapshots
}

#[cfg(feature = "native-standalone")]
fn rust_public_dirty_snapshot() -> StandalonePublicDirtySnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let child = tree.create_default_node();
    let grandchild = tree.create_default_node();
    let nodes = [root, child, grandchild];

    tree.append_child(root, child).expect("append child");
    tree.append_child(child, grandchild)
        .expect("append grandchild");

    calculate_probe_layout(&mut tree, root);
    let after_clean_layout = snapshot_public_dirty_nodes(&tree, &nodes);

    tree.mark_dirty(grandchild).expect("mark grandchild dirty");
    let after_mark_grandchild = snapshot_public_dirty_nodes(&tree, &nodes);

    calculate_probe_layout(&mut tree, root);
    let after_reclean_layout = snapshot_public_dirty_nodes(&tree, &nodes);

    tree.mark_dirty(root).expect("mark root dirty");
    let after_mark_root = snapshot_public_dirty_nodes(&tree, &nodes);

    StandalonePublicDirtySnapshot {
        after_clean_layout,
        after_mark_grandchild,
        after_reclean_layout,
        after_mark_root,
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_edge_style_snapshots() -> Vec<StandalonePublicEdgeStyleSnapshot> {
    let mut tree = StandaloneTree::new();
    let ltr = tree.create_default_node();
    let rtl = tree.create_default_node();

    tree.set_direction(ltr, Direction::Ltr)
        .expect("set ltr direction");
    tree.set_position(ltr, StandaloneEdge::Start, Length::points(1.0))
        .expect("set ltr start position");
    tree.set_position(ltr, StandaloneEdge::End, Length::percent(20.0))
        .expect("set ltr end position");
    tree.set_position(ltr, StandaloneEdge::Vertical, Length::calc(3.0, 30.0))
        .expect("set ltr vertical position");
    tree.set_margin(ltr, StandaloneEdge::Horizontal, Length::Auto)
        .expect("set ltr horizontal margin");
    tree.set_margin(ltr, StandaloneEdge::Top, Length::percent(10.0))
        .expect("set ltr top margin");
    tree.set_padding(ltr, StandaloneEdge::All, Length::points(4.0))
        .expect("set ltr padding all");
    tree.set_padding(ltr, StandaloneEdge::Bottom, Length::calc(2.0, 5.0))
        .expect("set ltr bottom padding");
    tree.set_border(ltr, StandaloneEdge::All, 1.0)
        .expect("set ltr border all");
    tree.set_border(ltr, StandaloneEdge::Start, 2.0)
        .expect("set ltr start border");
    tree.set_border(ltr, StandaloneEdge::End, 3.0)
        .expect("set ltr end border");
    tree.set_gap(ltr, StandaloneGap::All, Length::points(6.0))
        .expect("set ltr all gap");
    tree.set_gap(ltr, StandaloneGap::Row, Length::percent(7.0))
        .expect("set ltr row gap");

    tree.set_direction(rtl, Direction::Rtl)
        .expect("set rtl direction");
    tree.set_position(rtl, StandaloneEdge::Start, Length::points(11.0))
        .expect("set rtl start position");
    tree.set_position(rtl, StandaloneEdge::End, Length::percent(12.0))
        .expect("set rtl end position");
    tree.set_position(rtl, StandaloneEdge::Vertical, Length::points(13.0))
        .expect("set rtl vertical position");
    tree.set_margin(rtl, StandaloneEdge::Start, Length::Auto)
        .expect("set rtl start margin");
    tree.set_margin(rtl, StandaloneEdge::End, Length::percent(14.0))
        .expect("set rtl end margin");
    tree.set_padding(rtl, StandaloneEdge::Horizontal, Length::points(15.0))
        .expect("set rtl horizontal padding");
    tree.set_padding(rtl, StandaloneEdge::Vertical, Length::percent(16.0))
        .expect("set rtl vertical padding");
    tree.set_border(rtl, StandaloneEdge::All, 17.0)
        .expect("set rtl border all");
    tree.set_border(rtl, StandaloneEdge::Start, 18.0)
        .expect("set rtl start border");
    tree.set_border(rtl, StandaloneEdge::End, 19.0)
        .expect("set rtl end border");
    tree.set_gap(rtl, StandaloneGap::All, Length::calc(4.0, 40.0))
        .expect("set rtl all gap");
    tree.set_gap(rtl, StandaloneGap::Column, Length::points(21.0))
        .expect("set rtl column gap");

    vec![
        snapshot_public_edge_style_stage(StandalonePublicStyleStage::Ltr, &tree, ltr),
        snapshot_public_edge_style_stage(StandalonePublicStyleStage::Rtl, &tree, rtl),
    ]
}

#[cfg(feature = "native-standalone")]
#[derive(Clone, Copy)]
enum PublicEdgeStyleVariant {
    Points,
    Percent,
    Calc,
    ValueFr,
    ValueMaxContent,
    ValueFitContent,
    Auto,
}

#[cfg(feature = "native-standalone")]
fn rust_public_edge_style_variant_snapshots() -> Vec<StandalonePublicEdgeStyleVariantSnapshot> {
    [
        PublicEdgeStyleVariant::Points,
        PublicEdgeStyleVariant::Percent,
        PublicEdgeStyleVariant::Calc,
        PublicEdgeStyleVariant::ValueFr,
        PublicEdgeStyleVariant::ValueMaxContent,
        PublicEdgeStyleVariant::ValueFitContent,
        PublicEdgeStyleVariant::Auto,
    ]
    .iter()
    .copied()
    .map(|variant| {
        let mut tree = StandaloneTree::new();
        let node = tree.create_default_node();
        apply_rust_public_edge_style_variant(&mut tree, node, variant);
        snapshot_public_edge_style_variant(&tree, node)
    })
    .collect()
}

#[cfg(feature = "native-standalone")]
fn snapshot_public_edge_style_variant(
    tree: &StandaloneTree,
    node: NodeId,
) -> StandalonePublicEdgeStyleVariantSnapshot {
    StandalonePublicEdgeStyleVariantSnapshot {
        position: length_value(
            tree.style_position(node, StandaloneEdge::Left)
                .expect("position left"),
        ),
        margin: length_value(
            tree.style_margin(node, StandaloneEdge::Right)
                .expect("margin right"),
        ),
        padding: length_value(
            tree.style_padding(node, StandaloneEdge::Top)
                .expect("padding top"),
        ),
        gap: length_value(
            tree.style_gap(node, StandaloneGap::Column)
                .expect("column gap"),
        ),
        dirty: tree.is_dirty(node).expect("dirty state"),
    }
}

#[cfg(feature = "native-standalone")]
fn apply_rust_public_edge_style_variant(
    tree: &mut StandaloneTree,
    node: NodeId,
    variant: PublicEdgeStyleVariant,
) {
    match variant {
        PublicEdgeStyleVariant::Points => {
            tree.set_position(node, StandaloneEdge::Left, Length::points(1.0))
                .expect("set position point");
            tree.set_margin(node, StandaloneEdge::Right, Length::points(2.0))
                .expect("set margin point");
            tree.set_padding(node, StandaloneEdge::Top, Length::points(3.0))
                .expect("set padding point");
            tree.set_gap(node, StandaloneGap::Column, Length::points(4.0))
                .expect("set gap point");
        }
        PublicEdgeStyleVariant::Percent => {
            tree.set_position(node, StandaloneEdge::Left, Length::percent(11.0))
                .expect("set position percent");
            tree.set_margin(node, StandaloneEdge::Right, Length::percent(12.0))
                .expect("set margin percent");
            tree.set_padding(node, StandaloneEdge::Top, Length::percent(13.0))
                .expect("set padding percent");
            tree.set_gap(node, StandaloneGap::Column, Length::percent(14.0))
                .expect("set gap percent");
        }
        PublicEdgeStyleVariant::Calc => {
            tree.set_position(node, StandaloneEdge::Left, Length::calc(21.0, 22.0))
                .expect("set position calc");
            tree.set_margin(node, StandaloneEdge::Right, Length::calc(23.0, 24.0))
                .expect("set margin calc");
            tree.set_padding(node, StandaloneEdge::Top, Length::calc(25.0, 26.0))
                .expect("set padding calc");
            tree.set_gap(node, StandaloneGap::Column, Length::calc(27.0, 28.0))
                .expect("set gap calc");
        }
        PublicEdgeStyleVariant::ValueFr => {
            tree.set_position(node, StandaloneEdge::Left, Length::fr(1.25))
                .expect("set position fr");
            tree.set_margin(node, StandaloneEdge::Right, Length::fr(2.25))
                .expect("set margin fr");
            tree.set_padding(node, StandaloneEdge::Top, Length::fr(3.25))
                .expect("set padding fr");
            tree.set_gap(node, StandaloneGap::Column, Length::fr(4.25))
                .expect("set gap fr");
        }
        PublicEdgeStyleVariant::ValueMaxContent => {
            tree.set_position(node, StandaloneEdge::Left, Length::max_content())
                .expect("set position max-content");
            tree.set_margin(node, StandaloneEdge::Right, Length::max_content())
                .expect("set margin max-content");
            tree.set_padding(node, StandaloneEdge::Top, Length::max_content())
                .expect("set padding max-content");
            tree.set_gap(node, StandaloneGap::Column, Length::max_content())
                .expect("set gap max-content");
        }
        PublicEdgeStyleVariant::ValueFitContent => {
            tree.set_position(
                node,
                StandaloneEdge::Left,
                Length::fit_content(Some(BaseLength::fixed_and_percent(31.0, 32.0))),
            )
            .expect("set position fit-content");
            tree.set_margin(
                node,
                StandaloneEdge::Right,
                Length::fit_content(Some(BaseLength::fixed_and_percent(33.0, 34.0))),
            )
            .expect("set margin fit-content");
            tree.set_padding(
                node,
                StandaloneEdge::Top,
                Length::fit_content(Some(BaseLength::fixed_and_percent(35.0, 36.0))),
            )
            .expect("set padding fit-content");
            tree.set_gap(
                node,
                StandaloneGap::Column,
                Length::fit_content(Some(BaseLength::fixed_and_percent(37.0, 38.0))),
            )
            .expect("set gap fit-content");
        }
        PublicEdgeStyleVariant::Auto => {
            tree.set_position(node, StandaloneEdge::Left, Length::Auto)
                .expect("set position auto");
            tree.set_margin(node, StandaloneEdge::Right, Length::Auto)
                .expect("set margin auto");
            tree.set_padding(node, StandaloneEdge::Top, Length::points(41.0))
                .expect("set padding point for auto case");
            tree.set_gap(node, StandaloneGap::Column, Length::points(42.0))
                .expect("set gap point for auto case");
        }
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_scalar_style_snapshot() -> StandalonePublicScalarStyleSnapshot {
    let mut tree = StandaloneTree::new();
    let node = tree.create_default_node();

    tree.set_flex_direction(node, FlexDirection::RowReverse)
        .expect("set flex direction");
    tree.set_justify_content(node, JustifyContent::SpaceBetween)
        .expect("set justify content");
    tree.set_align_content(node, AlignContent::SpaceEvenly)
        .expect("set align content");
    tree.set_align_items(node, AlignItems::Baseline)
        .expect("set align items");
    tree.set_align_self(node, Some(AlignItems::End))
        .expect("set align self");
    tree.set_position_type(node, PositionType::Sticky)
        .expect("set position type");
    tree.set_flex_wrap(node, FlexWrap::WrapReverse)
        .expect("set flex wrap");
    tree.set_linear_orientation(node, LinearOrientation::VerticalReverse)
        .expect("set linear orientation");
    tree.set_linear_gravity(node, LinearGravity::Center)
        .expect("set linear gravity");
    tree.set_linear_layout_gravity(node, LinearLayoutGravity::FillHorizontal)
        .expect("set linear layout gravity");
    tree.set_linear_cross_gravity(node, LinearCrossGravity::Stretch)
        .expect("set linear cross gravity");
    tree.set_relative_center(node, RelativeCenter::Both)
        .expect("set relative center");
    tree.set_grid_auto_flow(node, GridAutoFlow::ColumnDense)
        .expect("set grid auto flow");
    tree.set_justify_items(node, JustifyItems::Center)
        .expect("set justify items");
    tree.set_justify_self(node, JustifyItems::End)
        .expect("set justify self");
    tree.set_display(node, Display::Grid).expect("set display");
    tree.set_box_sizing(node, BoxSizing::ContentBox)
        .expect("set box sizing");
    tree.set_aspect_ratio(node, Some(1.5))
        .expect("set aspect ratio");
    tree.set_order(node, -2).expect("set order");
    tree.set_relative_id(node, 17).expect("set relative id");
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
    tree.set_relative_bottom_of(node, 7)
        .expect("set relative bottom of");
    tree.set_relative_left_of(node, 8)
        .expect("set relative left of");
    tree.set_relative_layout_once(node, true)
        .expect("set relative layout once");
    tree.set_grid_column_start(node, Some(2))
        .expect("set grid column start");
    tree.set_grid_column_end(node, Some(4))
        .expect("set grid column end");
    tree.set_grid_row_start(node, Some(3))
        .expect("set grid row start");
    tree.set_grid_row_end(node, Some(5))
        .expect("set grid row end");
    tree.set_grid_column_span(node, 6)
        .expect("set grid column span");
    tree.set_grid_row_span(node, 7).expect("set grid row span");
    tree.set_flex(node, 2.5).expect("set flex shorthand");
    tree.set_linear_weight(node, 3.0)
        .expect("set linear weight");
    tree.set_linear_weight_sum(node, 9.0)
        .expect("set linear weight sum");

    StandalonePublicScalarStyleSnapshot {
        flex_direction: flex_direction_value(tree.style_flex_direction(node).expect("flex")),
        justify_content: justify_content_value(
            tree.style_justify_content(node).expect("justify content"),
        ),
        align_content: align_content_value(tree.style_align_content(node).expect("align content")),
        align_items: align_items_value(tree.style_align_items(node).expect("align items")),
        align_self: tree
            .style_align_self(node)
            .expect("align self")
            .map_or(0, align_items_value),
        position_type: position_type_value(tree.style_position_type(node).expect("position type")),
        flex_wrap: flex_wrap_value(tree.style_flex_wrap(node).expect("flex wrap")),
        linear_orientation: linear_orientation_value(
            tree.style_linear_orientation(node)
                .expect("linear orientation"),
        ),
        linear_gravity: linear_gravity_value(
            tree.style_linear_gravity(node).expect("linear gravity"),
        ),
        linear_layout_gravity: linear_layout_gravity_value(
            tree.style_linear_layout_gravity(node)
                .expect("linear layout gravity"),
        ),
        linear_cross_gravity: linear_cross_gravity_value(
            tree.style_linear_cross_gravity(node)
                .expect("linear cross gravity"),
        ),
        relative_center: relative_center_value(
            tree.style_relative_center(node).expect("relative center"),
        ),
        grid_auto_flow: grid_auto_flow_value(
            tree.style_grid_auto_flow(node).expect("grid auto flow"),
        ),
        justify_items: justify_items_value(tree.style_justify_items(node).expect("justify items")),
        justify_self: justify_items_value(tree.style_justify_self(node).expect("justify self")),
        display: display_value(tree.style_display(node).expect("display")),
        box_sizing: box_sizing_value(tree.style_box_sizing(node).expect("box sizing")),
        aspect_ratio: tree
            .style_aspect_ratio(node)
            .expect("aspect ratio")
            .unwrap_or(0.0),
        order: tree.style_order(node).expect("order"),
        relative_id: tree.style_relative_id(node).expect("relative id"),
        relative_align_top: tree
            .style_relative_align_top(node)
            .expect("relative align top"),
        relative_align_right: tree
            .style_relative_align_right(node)
            .expect("relative align right"),
        relative_align_bottom: tree
            .style_relative_align_bottom(node)
            .expect("relative align bottom"),
        relative_align_left: tree
            .style_relative_align_left(node)
            .expect("relative align left"),
        relative_top_of: tree.style_relative_top_of(node).expect("relative top of"),
        relative_right_of: tree
            .style_relative_right_of(node)
            .expect("relative right of"),
        relative_bottom_of: tree
            .style_relative_bottom_of(node)
            .expect("relative bottom of"),
        relative_left_of: tree.style_relative_left_of(node).expect("relative left of"),
        relative_layout_once: tree
            .style_relative_layout_once(node)
            .expect("relative layout once"),
        grid_column_start: tree
            .style_grid_column_start(node)
            .expect("grid column start")
            .unwrap_or(0),
        grid_column_end: tree
            .style_grid_column_end(node)
            .expect("grid column end")
            .unwrap_or(0),
        grid_row_start: tree
            .style_grid_row_start(node)
            .expect("grid row start")
            .unwrap_or(0),
        grid_row_end: tree
            .style_grid_row_end(node)
            .expect("grid row end")
            .unwrap_or(0),
        grid_column_span: tree.style_grid_column_span(node).expect("grid column span") as i32,
        grid_row_span: tree.style_grid_row_span(node).expect("grid row span") as i32,
        flex_grow: tree.style_flex_grow(node).expect("flex grow"),
        flex_shrink: tree.style_flex_shrink(node).expect("flex shrink"),
        linear_weight: tree.style_linear_weight(node).expect("linear weight"),
        linear_weight_sum: tree
            .style_linear_weight_sum(node)
            .expect("linear weight sum"),
        dirty: tree.is_dirty(node).expect("dirty state"),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_measure_delegate_snapshot() -> StandalonePublicMeasureDelegateSnapshot {
    reset_public_measure_delegate_state();

    let mut tree = StandaloneTree::new();
    let node = tree.create_default_node();
    let initial_delegate_is_null = tree.measure_func(node).expect("initial measure").is_none()
        && tree
            .baseline_func(node)
            .expect("initial baseline")
            .is_none();
    let initial_has_measure_func = tree.has_measure_func(node).expect("initial measure flag");

    tree.set_measure_func(node, Some(rust_public_measure_delegate_measure))
        .expect("set measure func");
    tree.set_baseline_func(node, Some(rust_public_measure_delegate_baseline))
        .expect("set baseline func");
    let delegate_round_trips = tree.measure_func(node).expect("measure func").is_some()
        && tree.baseline_func(node).expect("baseline func").is_some();
    let after_set_has_measure_func = tree.has_measure_func(node).expect("measure flag");

    tree.calculate_layout_with_mode(
        node,
        starlight_layout::Constraints::at_most(50.0, 40.0),
        Direction::Ltr,
    )
    .expect("layout measured node");

    let layout_width = tree.layout_width(node).expect("layout width");
    let layout_height = tree.layout_height(node).expect("layout height");
    let layout_baseline = tree.layout_baseline(node).expect("layout baseline");

    tree.set_measure_func(node, None)
        .expect("clear measure func");
    tree.set_baseline_func(node, None)
        .expect("clear baseline func");
    let after_clear_delegate_is_null = tree.measure_func(node).expect("cleared measure").is_none()
        && tree
            .baseline_func(node)
            .expect("cleared baseline")
            .is_none();
    let after_clear_has_measure_func = tree.has_measure_func(node).expect("cleared measure flag");

    StandalonePublicMeasureDelegateSnapshot {
        initial_delegate_is_null,
        initial_has_measure_func,
        delegate_round_trips,
        after_set_has_measure_func,
        measure_call_count: PUBLIC_MEASURE_CALL_COUNT.load(Ordering::SeqCst),
        baseline_call_count: PUBLIC_BASELINE_CALL_COUNT.load(Ordering::SeqCst),
        measure_width: f32::from_bits(PUBLIC_MEASURE_WIDTH_BITS.load(Ordering::SeqCst)),
        measure_width_mode: PUBLIC_MEASURE_WIDTH_MODE.load(Ordering::SeqCst) as i32,
        measure_height: f32::from_bits(PUBLIC_MEASURE_HEIGHT_BITS.load(Ordering::SeqCst)),
        measure_height_mode: PUBLIC_MEASURE_HEIGHT_MODE.load(Ordering::SeqCst) as i32,
        baseline_width: f32::from_bits(PUBLIC_BASELINE_WIDTH_BITS.load(Ordering::SeqCst)),
        baseline_height: f32::from_bits(PUBLIC_BASELINE_HEIGHT_BITS.load(Ordering::SeqCst)),
        layout_width,
        layout_height,
        layout_baseline,
        after_clear_delegate_is_null,
        after_clear_has_measure_func,
    }
}

#[cfg(feature = "native-standalone")]
fn reset_public_measure_delegate_state() {
    PUBLIC_MEASURE_CALL_COUNT.store(0, Ordering::SeqCst);
    PUBLIC_BASELINE_CALL_COUNT.store(0, Ordering::SeqCst);
    PUBLIC_MEASURE_WIDTH_BITS.store(0, Ordering::SeqCst);
    PUBLIC_MEASURE_WIDTH_MODE.store(0, Ordering::SeqCst);
    PUBLIC_MEASURE_HEIGHT_BITS.store(0, Ordering::SeqCst);
    PUBLIC_MEASURE_HEIGHT_MODE.store(0, Ordering::SeqCst);
    PUBLIC_BASELINE_WIDTH_BITS.store(0, Ordering::SeqCst);
    PUBLIC_BASELINE_HEIGHT_BITS.store(0, Ordering::SeqCst);
}

#[cfg(feature = "native-standalone")]
fn rust_public_measure_delegate_measure(constraints: starlight_layout::Constraints) -> Size {
    PUBLIC_MEASURE_CALL_COUNT.fetch_add(1, Ordering::SeqCst);
    PUBLIC_MEASURE_WIDTH_BITS.store(constraints.width.size.to_bits(), Ordering::SeqCst);
    PUBLIC_MEASURE_WIDTH_MODE.store(
        measure_mode_value(constraints.width.mode) as u32,
        Ordering::SeqCst,
    );
    PUBLIC_MEASURE_HEIGHT_BITS.store(constraints.height.size.to_bits(), Ordering::SeqCst);
    PUBLIC_MEASURE_HEIGHT_MODE.store(
        measure_mode_value(constraints.height.mode) as u32,
        Ordering::SeqCst,
    );
    Size::new(constraints.width.size, constraints.height.size)
}

#[cfg(feature = "native-standalone")]
fn rust_public_measure_delegate_baseline(content_size: Size) -> f32 {
    PUBLIC_BASELINE_CALL_COUNT.fetch_add(1, Ordering::SeqCst);
    PUBLIC_BASELINE_WIDTH_BITS.store(content_size.width.to_bits(), Ordering::SeqCst);
    PUBLIC_BASELINE_HEIGHT_BITS.store(content_size.height.to_bits(), Ordering::SeqCst);
    content_size.width / 10.0 + content_size.height / 20.0
}

#[cfg(feature = "native-standalone")]
fn measure_mode_value(mode: MeasureMode) -> i32 {
    match mode {
        MeasureMode::Indefinite => 0,
        MeasureMode::Definite => 1,
        MeasureMode::AtMost => 2,
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_layout_entrypoint_snapshot() -> StandalonePublicLayoutEntrypointSnapshot {
    StandalonePublicLayoutEntrypointSnapshot {
        finite_owner: rust_public_layout_entrypoint_stage(|tree, node| {
            tree.calculate_layout(node, Size::new(50.0, 40.0), Direction::Ltr)
                .expect("finite owner layout");
        }),
        sentinel_undefined_owner: rust_public_layout_entrypoint_stage(|tree, node| {
            tree.calculate_layout_with_mode(
                node,
                starlight_layout::Constraints::new(
                    SideConstraint::indefinite(),
                    SideConstraint::indefinite(),
                ),
                Direction::Ltr,
            )
            .expect("sentinel undefined owner layout");
        }),
        at_most_owner: rust_public_layout_entrypoint_stage(|tree, node| {
            tree.calculate_layout_with_mode(
                node,
                starlight_layout::Constraints::at_most(50.0, 40.0),
                Direction::Ltr,
            )
            .expect("at-most owner layout");
        }),
        undefined_owner: rust_public_layout_entrypoint_stage(|tree, node| {
            tree.calculate_layout_with_mode(
                node,
                starlight_layout::Constraints::new(
                    SideConstraint::indefinite(),
                    SideConstraint::indefinite(),
                ),
                Direction::Ltr,
            )
            .expect("undefined owner layout");
        }),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_layout_entrypoint_stage(
    layout: impl FnOnce(&mut StandaloneTree, NodeId),
) -> StandalonePublicLayoutEntrypointStageSnapshot {
    reset_public_layout_entrypoint_state();

    let mut tree = StandaloneTree::new();
    let node = tree.create_default_node();
    tree.set_measure_func(node, Some(rust_public_layout_entrypoint_measure))
        .expect("set layout entrypoint measure func");
    layout(&mut tree, node);

    StandalonePublicLayoutEntrypointStageSnapshot {
        measure_call_count: PUBLIC_ENTRYPOINT_MEASURE_CALL_COUNT.load(Ordering::SeqCst),
        measure_width: f32::from_bits(PUBLIC_ENTRYPOINT_MEASURE_WIDTH_BITS.load(Ordering::SeqCst)),
        measure_width_mode: PUBLIC_ENTRYPOINT_MEASURE_WIDTH_MODE.load(Ordering::SeqCst) as i32,
        measure_height: f32::from_bits(
            PUBLIC_ENTRYPOINT_MEASURE_HEIGHT_BITS.load(Ordering::SeqCst),
        ),
        measure_height_mode: PUBLIC_ENTRYPOINT_MEASURE_HEIGHT_MODE.load(Ordering::SeqCst) as i32,
        layout_width: tree.layout_width(node).expect("layout width"),
        layout_height: tree.layout_height(node).expect("layout height"),
    }
}

#[cfg(feature = "native-standalone")]
fn reset_public_layout_entrypoint_state() {
    PUBLIC_ENTRYPOINT_MEASURE_CALL_COUNT.store(0, Ordering::SeqCst);
    PUBLIC_ENTRYPOINT_MEASURE_WIDTH_BITS.store(0, Ordering::SeqCst);
    PUBLIC_ENTRYPOINT_MEASURE_WIDTH_MODE.store(0, Ordering::SeqCst);
    PUBLIC_ENTRYPOINT_MEASURE_HEIGHT_BITS.store(0, Ordering::SeqCst);
    PUBLIC_ENTRYPOINT_MEASURE_HEIGHT_MODE.store(0, Ordering::SeqCst);
}

#[cfg(feature = "native-standalone")]
fn rust_public_layout_entrypoint_measure(constraints: starlight_layout::Constraints) -> Size {
    PUBLIC_ENTRYPOINT_MEASURE_CALL_COUNT.fetch_add(1, Ordering::SeqCst);
    PUBLIC_ENTRYPOINT_MEASURE_WIDTH_BITS.store(
        layout_entrypoint_observed_size(constraints.width).to_bits(),
        Ordering::SeqCst,
    );
    PUBLIC_ENTRYPOINT_MEASURE_WIDTH_MODE.store(
        measure_mode_value(constraints.width.mode) as u32,
        Ordering::SeqCst,
    );
    PUBLIC_ENTRYPOINT_MEASURE_HEIGHT_BITS.store(
        layout_entrypoint_observed_size(constraints.height).to_bits(),
        Ordering::SeqCst,
    );
    PUBLIC_ENTRYPOINT_MEASURE_HEIGHT_MODE.store(
        measure_mode_value(constraints.height.mode) as u32,
        Ordering::SeqCst,
    );
    Size::new(
        layout_entrypoint_measured_axis(constraints.width, 11.0, 3.0),
        layout_entrypoint_measured_axis(constraints.height, 13.0, 4.0),
    )
}

#[cfg(feature = "native-standalone")]
fn layout_entrypoint_observed_size(constraint: SideConstraint) -> f32 {
    if constraint.mode == MeasureMode::Indefinite {
        0.0
    } else {
        constraint.size
    }
}

#[cfg(feature = "native-standalone")]
fn layout_entrypoint_measured_axis(
    constraint: SideConstraint,
    undefined_size: f32,
    at_most_delta: f32,
) -> f32 {
    match constraint.mode {
        MeasureMode::Indefinite => undefined_size,
        MeasureMode::Definite => constraint.size,
        MeasureMode::AtMost => constraint.size - at_most_delta,
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_config_snapshot() -> StandalonePublicConfigSnapshot {
    let mut config = StandaloneConfig::new();
    let default_config_physical_pixels_per_layout_unit = config.physical_pixels_per_layout_unit();
    config.set_physical_pixels_per_layout_unit(2.0);
    let updated_config_physical_pixels_per_layout_unit = config.physical_pixels_per_layout_unit();

    let mut default_tree = StandaloneTree::new();
    let default_node = default_tree.create_default_node();
    default_tree
        .set_measured_size(default_node, Some(Size::new(10.2, 4.2)))
        .expect("set default measured size");
    default_tree
        .calculate_layout_with_mode(
            default_node,
            starlight_layout::Constraints::indefinite(),
            Direction::Ltr,
        )
        .expect("layout default config node");

    let mut configured_tree = StandaloneTree::new();
    let configured_node = configured_tree.create_default_node_with_config(
        StandaloneConfig::with_physical_pixels_per_layout_unit(2.0),
    );
    configured_tree
        .set_measured_size(configured_node, Some(Size::new(10.2, 4.2)))
        .expect("set configured measured size");
    configured_tree
        .calculate_layout_with_mode(
            configured_node,
            starlight_layout::Constraints::indefinite(),
            Direction::Ltr,
        )
        .expect("layout configured node");

    StandalonePublicConfigSnapshot {
        default_config_physical_pixels_per_layout_unit,
        updated_config_physical_pixels_per_layout_unit,
        default_node_width: default_tree
            .layout_width(default_node)
            .expect("default node width"),
        default_node_height: default_tree
            .layout_height(default_node)
            .expect("default node height"),
        configured_node_width: configured_tree
            .layout_width(configured_node)
            .expect("configured node width"),
        configured_node_height: configured_tree
            .layout_height(configured_node)
            .expect("configured node height"),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_layout_getter_snapshot() -> StandalonePublicLayoutGetterSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let child = tree.create_default_node();

    tree.set_display(root, Display::Block)
        .expect("set root display");
    tree.set_width(root, Length::points(100.0))
        .expect("set root width");
    tree.set_height(root, Length::points(80.0))
        .expect("set root height");
    tree.set_padding(root, StandaloneEdge::All, Length::points(4.0))
        .expect("set root padding all");
    tree.set_padding(root, StandaloneEdge::Left, Length::points(6.0))
        .expect("set root padding left");
    tree.set_border(root, StandaloneEdge::All, 2.0)
        .expect("set root border");
    tree.set_margin(root, StandaloneEdge::Left, Length::points(3.0))
        .expect("set root margin left");

    tree.set_display(child, Display::Block)
        .expect("set child display");
    tree.set_direction(child, Direction::Rtl)
        .expect("set child direction");
    tree.set_position_type(child, PositionType::Sticky)
        .expect("set child position type");
    tree.set_width(child, Length::points(40.0))
        .expect("set child width");
    tree.set_height(child, Length::points(20.0))
        .expect("set child height");
    tree.set_margin(child, StandaloneEdge::Left, Length::points(5.0))
        .expect("set child margin left");
    tree.set_margin(child, StandaloneEdge::Top, Length::points(7.0))
        .expect("set child margin top");
    tree.set_padding(child, StandaloneEdge::All, Length::points(1.0))
        .expect("set child padding");
    tree.set_border(child, StandaloneEdge::All, 2.0)
        .expect("set child border");
    tree.set_position(child, StandaloneEdge::Left, Length::points(11.0))
        .expect("set child left");
    tree.set_position(child, StandaloneEdge::Right, Length::points(12.0))
        .expect("set child right");
    tree.set_position(child, StandaloneEdge::Top, Length::points(13.0))
        .expect("set child top");
    tree.set_position(child, StandaloneEdge::Bottom, Length::points(14.0))
        .expect("set child bottom");

    tree.append_child(root, child).expect("append child");
    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::definite(120.0, 100.0),
        Direction::Ltr,
    )
    .expect("calculate layout");

    StandalonePublicLayoutGetterSnapshot {
        root: snapshot_public_layout_node(&tree, root),
        child: snapshot_public_layout_node(&tree, child),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_box_aspect_layout_snapshot() -> StandalonePublicBoxAspectLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let content_box = tree.create_default_node();
    let border_box = tree.create_default_node();
    let clamped = tree.create_default_node();
    let nodes = [root, content_box, border_box, clamped];

    tree.set_display(root, Display::Block)
        .expect("set root display");
    tree.set_width(root, Length::points(120.0))
        .expect("set root width");
    tree.set_height(root, Length::points(130.0))
        .expect("set root height");
    tree.set_padding(root, StandaloneEdge::Left, Length::points(3.0))
        .expect("set root left padding");
    tree.set_padding(root, StandaloneEdge::Top, Length::points(4.0))
        .expect("set root top padding");
    tree.set_border(root, StandaloneEdge::All, 1.0)
        .expect("set root border");

    tree.set_display(content_box, Display::Block)
        .expect("set content-box display");
    tree.set_box_sizing(content_box, BoxSizing::ContentBox)
        .expect("set content-box sizing");
    tree.set_aspect_ratio(content_box, Some(2.0))
        .expect("set content-box aspect ratio");
    tree.set_width(content_box, Length::points(48.0))
        .expect("set content-box width");
    tree.set_padding(content_box, StandaloneEdge::Horizontal, Length::points(4.0))
        .expect("set content-box horizontal padding");
    tree.set_padding(content_box, StandaloneEdge::Vertical, Length::points(3.0))
        .expect("set content-box vertical padding");
    tree.set_border(content_box, StandaloneEdge::All, 2.0)
        .expect("set content-box border");

    tree.set_display(border_box, Display::Block)
        .expect("set border-box display");
    tree.set_box_sizing(border_box, BoxSizing::BorderBox)
        .expect("set border-box sizing");
    tree.set_aspect_ratio(border_box, Some(2.0))
        .expect("set border-box aspect ratio");
    tree.set_width(border_box, Length::points(48.0))
        .expect("set border-box width");
    tree.set_padding(border_box, StandaloneEdge::Horizontal, Length::points(4.0))
        .expect("set border-box horizontal padding");
    tree.set_padding(border_box, StandaloneEdge::Vertical, Length::points(3.0))
        .expect("set border-box vertical padding");
    tree.set_border(border_box, StandaloneEdge::All, 2.0)
        .expect("set border-box border");

    tree.set_display(clamped, Display::Block)
        .expect("set clamped display");
    tree.set_box_sizing(clamped, BoxSizing::ContentBox)
        .expect("set clamped box sizing");
    tree.set_aspect_ratio(clamped, Some(3.0))
        .expect("set clamped aspect ratio");
    tree.set_width(clamped, Length::points(36.0))
        .expect("set clamped width");
    tree.set_min_height(clamped, Length::points(24.0))
        .expect("set clamped min height");
    tree.set_padding(clamped, StandaloneEdge::All, Length::points(2.0))
        .expect("set clamped padding");
    tree.set_border(clamped, StandaloneEdge::All, 1.0)
        .expect("set clamped border");

    tree.append_child(root, content_box)
        .expect("append content-box");
    tree.append_child(root, border_box)
        .expect("append border-box");
    tree.append_child(root, clamped).expect("append clamped");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::definite(120.0, 130.0),
        Direction::Ltr,
    )
    .expect("layout box/aspect tree");

    StandalonePublicBoxAspectLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
#[derive(Clone, Copy)]
enum PublicDisplayLayoutVariant {
    None,
    Block,
    Flex,
    Linear,
    Relative,
    Grid,
}

#[cfg(feature = "native-standalone")]
impl PublicDisplayLayoutVariant {
    fn display(self) -> Display {
        match self {
            PublicDisplayLayoutVariant::None => Display::None,
            PublicDisplayLayoutVariant::Block => Display::Block,
            PublicDisplayLayoutVariant::Flex => Display::Flex,
            PublicDisplayLayoutVariant::Linear => Display::Linear,
            PublicDisplayLayoutVariant::Relative => Display::Relative,
            PublicDisplayLayoutVariant::Grid => Display::Grid,
        }
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_display_layout_snapshots() -> Vec<StandalonePublicDisplayLayoutSnapshot> {
    [
        PublicDisplayLayoutVariant::None,
        PublicDisplayLayoutVariant::Block,
        PublicDisplayLayoutVariant::Flex,
        PublicDisplayLayoutVariant::Linear,
        PublicDisplayLayoutVariant::Relative,
        PublicDisplayLayoutVariant::Grid,
    ]
    .into_iter()
    .map(rust_public_display_layout_snapshot)
    .collect()
}

#[cfg(feature = "native-standalone")]
fn rust_public_display_layout_snapshot(
    variant: PublicDisplayLayoutVariant,
) -> StandalonePublicDisplayLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let container = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let nodes = [root, container, first, second, third];

    tree.set_display(root, Display::Block)
        .expect("set root display");
    tree.set_width(root, Length::points(180.0))
        .expect("set root width");
    tree.set_height(root, Length::points(130.0))
        .expect("set root height");
    tree.set_padding(root, StandaloneEdge::Left, Length::points(3.0))
        .expect("set root left padding");
    tree.set_padding(root, StandaloneEdge::Top, Length::points(5.0))
        .expect("set root top padding");
    tree.set_border(root, StandaloneEdge::All, 1.0)
        .expect("set root border");

    tree.set_display(container, variant.display())
        .expect("set container display");
    tree.set_width(container, Length::points(120.0))
        .expect("set container width");
    tree.set_height(container, Length::points(72.0))
        .expect("set container height");
    tree.set_margin(container, StandaloneEdge::Left, Length::points(7.0))
        .expect("set container left margin");
    tree.set_margin(container, StandaloneEdge::Top, Length::points(6.0))
        .expect("set container top margin");
    tree.set_padding(container, StandaloneEdge::Left, Length::points(2.0))
        .expect("set container left padding");
    tree.set_padding(container, StandaloneEdge::Top, Length::points(3.0))
        .expect("set container top padding");
    tree.set_border(container, StandaloneEdge::All, 1.0)
        .expect("set container border");
    tree.set_gap(container, StandaloneGap::Column, Length::points(4.0))
        .expect("set container column gap");
    tree.set_gap(container, StandaloneGap::Row, Length::points(3.0))
        .expect("set container row gap");
    tree.set_flex_direction(container, FlexDirection::Row)
        .expect("set container flex direction");
    tree.set_flex_wrap(container, FlexWrap::NoWrap)
        .expect("set container flex wrap");
    tree.set_justify_content(container, JustifyContent::FlexStart)
        .expect("set container justify content");
    tree.set_align_items(container, AlignItems::FlexStart)
        .expect("set container align items");
    tree.set_linear_orientation(container, LinearOrientation::Horizontal)
        .expect("set container linear orientation");
    tree.set_grid_auto_flow(container, GridAutoFlow::Row)
        .expect("set container grid auto flow");
    tree.set_justify_items(container, JustifyItems::Start)
        .expect("set container justify items");
    tree.set_grid_template_columns(container, [Length::points(36.0), Length::points(24.0)])
        .expect("set container grid columns");
    tree.set_grid_template_columns_max(container, [Length::points(36.0), Length::points(24.0)])
        .expect("set container grid column max tracks");
    tree.set_grid_template_rows(container, [Length::points(18.0), Length::points(16.0)])
        .expect("set container grid rows");
    tree.set_grid_template_rows_max(container, [Length::points(18.0), Length::points(16.0)])
        .expect("set container grid row max tracks");
    tree.set_grid_auto_columns(container, [Length::points(24.0)])
        .expect("set container auto columns");
    tree.set_grid_auto_columns_max(container, [Length::points(24.0)])
        .expect("set container auto column max tracks");
    tree.set_grid_auto_rows(container, [Length::points(16.0)])
        .expect("set container auto rows");
    tree.set_grid_auto_rows_max(container, [Length::points(16.0)])
        .expect("set container auto row max tracks");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_width(first, Length::points(30.0))
        .expect("set first width");
    tree.set_height(first, Length::points(10.0))
        .expect("set first height");
    tree.set_relative_id(first, 10)
        .expect("set first relative id");
    tree.set_grid_column_start(first, Some(1))
        .expect("set first grid column");
    tree.set_grid_row_start(first, Some(1))
        .expect("set first grid row");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_width(second, Length::points(20.0))
        .expect("set second width");
    tree.set_height(second, Length::points(12.0))
        .expect("set second height");
    tree.set_relative_right_of(second, 10)
        .expect("set second right-of");
    tree.set_grid_column_start(second, Some(2))
        .expect("set second grid column");
    tree.set_grid_row_start(second, Some(1))
        .expect("set second grid row");

    tree.set_display(third, Display::Block)
        .expect("set third display");
    tree.set_width(third, Length::points(16.0))
        .expect("set third width");
    tree.set_height(third, Length::points(8.0))
        .expect("set third height");
    tree.set_relative_bottom_of(third, 10)
        .expect("set third bottom-of");
    tree.set_grid_column_start(third, Some(1))
        .expect("set third grid column");
    tree.set_grid_row_start(third, Some(2))
        .expect("set third grid row");

    tree.append_child(container, first).expect("append first");
    tree.append_child(container, second).expect("append second");
    tree.append_child(container, third).expect("append third");
    tree.append_child(root, container)
        .expect("append container");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::definite(180.0, 130.0),
        Direction::Ltr,
    )
    .expect("layout display tree");

    let snapshot_nodes = if matches!(variant, PublicDisplayLayoutVariant::None) {
        &nodes[..2]
    } else {
        &nodes[..]
    };

    StandalonePublicDisplayLayoutSnapshot {
        nodes: snapshot_nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_position_layout_snapshot() -> StandalonePublicPositionLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let in_flow = tree.create_default_node();
    let relative = tree.create_default_node();
    let absolute = tree.create_default_node();
    let fixed = tree.create_default_node();
    let sticky = tree.create_default_node();
    let nodes = [root, in_flow, relative, absolute, fixed, sticky];

    tree.set_display(root, Display::Block)
        .expect("set root display");
    tree.set_width(root, Length::points(140.0))
        .expect("set root width");
    tree.set_height(root, Length::points(120.0))
        .expect("set root height");
    tree.set_padding(root, StandaloneEdge::Left, Length::points(6.0))
        .expect("set root left padding");
    tree.set_padding(root, StandaloneEdge::Top, Length::points(4.0))
        .expect("set root top padding");
    tree.set_border(root, StandaloneEdge::All, 1.0)
        .expect("set root border");

    tree.set_display(in_flow, Display::Block)
        .expect("set in-flow display");
    tree.set_width(in_flow, Length::points(24.0))
        .expect("set in-flow width");
    tree.set_height(in_flow, Length::points(10.0))
        .expect("set in-flow height");

    tree.set_display(relative, Display::Block)
        .expect("set relative display");
    tree.set_position_type(relative, PositionType::Relative)
        .expect("set relative position type");
    tree.set_position(relative, StandaloneEdge::Left, Length::points(5.0))
        .expect("set relative left");
    tree.set_position(relative, StandaloneEdge::Top, Length::points(7.0))
        .expect("set relative top");
    tree.set_width(relative, Length::points(26.0))
        .expect("set relative width");
    tree.set_height(relative, Length::points(12.0))
        .expect("set relative height");

    tree.set_display(absolute, Display::Block)
        .expect("set absolute display");
    tree.set_position_type(absolute, PositionType::Absolute)
        .expect("set absolute position type");
    tree.set_position(absolute, StandaloneEdge::Left, Length::points(10.0))
        .expect("set absolute left");
    tree.set_position(absolute, StandaloneEdge::Top, Length::points(15.0))
        .expect("set absolute top");
    tree.set_width(absolute, Length::points(30.0))
        .expect("set absolute width");
    tree.set_height(absolute, Length::points(14.0))
        .expect("set absolute height");

    tree.set_display(fixed, Display::Block)
        .expect("set fixed display");
    tree.set_position_type(fixed, PositionType::Fixed)
        .expect("set fixed position type");
    tree.set_position(fixed, StandaloneEdge::Right, Length::points(8.0))
        .expect("set fixed right");
    tree.set_position(fixed, StandaloneEdge::Bottom, Length::points(9.0))
        .expect("set fixed bottom");
    tree.set_width(fixed, Length::points(28.0))
        .expect("set fixed width");
    tree.set_height(fixed, Length::points(16.0))
        .expect("set fixed height");

    tree.set_display(sticky, Display::Block)
        .expect("set sticky display");
    tree.set_position_type(sticky, PositionType::Sticky)
        .expect("set sticky position type");
    tree.set_position(sticky, StandaloneEdge::Left, Length::points(3.0))
        .expect("set sticky left");
    tree.set_position(sticky, StandaloneEdge::Top, Length::points(4.0))
        .expect("set sticky top");
    tree.set_width(sticky, Length::points(22.0))
        .expect("set sticky width");
    tree.set_height(sticky, Length::points(11.0))
        .expect("set sticky height");

    tree.append_child(root, in_flow).expect("append in-flow");
    tree.append_child(root, relative).expect("append relative");
    tree.append_child(root, absolute).expect("append absolute");
    tree.append_child(root, fixed).expect("append fixed");
    tree.append_child(root, sticky).expect("append sticky");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::definite(140.0, 120.0),
        Direction::Ltr,
    )
    .expect("layout position tree");

    StandalonePublicPositionLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_relative_layout_snapshots() -> Vec<StandalonePublicRelativeLayoutSnapshot> {
    vec![
        rust_public_relative_definite_layout_snapshot(),
        rust_public_relative_layout_once_snapshot(),
    ]
}

#[cfg(feature = "native-standalone")]
fn rust_public_relative_definite_layout_snapshot() -> StandalonePublicRelativeLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let center_none = tree.create_default_node();
    let center_horizontal = tree.create_default_node();
    let center_vertical = tree.create_default_node();
    let center_both = tree.create_default_node();
    let parent_end = tree.create_default_node();
    let anchor = tree.create_default_node();
    let before = tree.create_default_node();
    let aligned = tree.create_default_node();
    let nodes = [
        root,
        center_none,
        center_horizontal,
        center_vertical,
        center_both,
        parent_end,
        anchor,
        before,
        aligned,
    ];

    tree.set_display(root, Display::Relative)
        .expect("set root display");
    tree.set_width(root, Length::points(100.0))
        .expect("set root width");
    tree.set_height(root, Length::points(80.0))
        .expect("set root height");

    tree.set_display(center_none, Display::Block)
        .expect("set none center display");
    tree.set_relative_center(center_none, RelativeCenter::None)
        .expect("set none center");
    tree.set_width(center_none, Length::points(20.0))
        .expect("set none center width");
    tree.set_height(center_none, Length::points(10.0))
        .expect("set none center height");

    tree.set_display(center_horizontal, Display::Block)
        .expect("set horizontal center display");
    tree.set_relative_center(center_horizontal, RelativeCenter::Horizontal)
        .expect("set horizontal center");
    tree.set_width(center_horizontal, Length::points(20.0))
        .expect("set horizontal center width");
    tree.set_height(center_horizontal, Length::points(10.0))
        .expect("set horizontal center height");

    tree.set_display(center_vertical, Display::Block)
        .expect("set vertical center display");
    tree.set_relative_center(center_vertical, RelativeCenter::Vertical)
        .expect("set vertical center");
    tree.set_width(center_vertical, Length::points(20.0))
        .expect("set vertical center width");
    tree.set_height(center_vertical, Length::points(10.0))
        .expect("set vertical center height");

    tree.set_display(center_both, Display::Block)
        .expect("set both center display");
    tree.set_relative_center(center_both, RelativeCenter::Both)
        .expect("set both center");
    tree.set_width(center_both, Length::points(20.0))
        .expect("set both center width");
    tree.set_height(center_both, Length::points(10.0))
        .expect("set both center height");

    tree.set_display(parent_end, Display::Block)
        .expect("set parent-end display");
    tree.set_relative_align_right(parent_end, RELATIVE_ALIGN_PARENT)
        .expect("set parent-end right");
    tree.set_relative_align_bottom(parent_end, RELATIVE_ALIGN_PARENT)
        .expect("set parent-end bottom");
    tree.set_width(parent_end, Length::points(18.0))
        .expect("set parent-end width");
    tree.set_height(parent_end, Length::points(8.0))
        .expect("set parent-end height");

    tree.set_display(anchor, Display::Block)
        .expect("set anchor display");
    tree.set_relative_id(anchor, 20).expect("set anchor id");
    tree.set_relative_align_right(anchor, RELATIVE_ALIGN_PARENT)
        .expect("set anchor right");
    tree.set_relative_align_bottom(anchor, RELATIVE_ALIGN_PARENT)
        .expect("set anchor bottom");
    tree.set_width(anchor, Length::points(20.0))
        .expect("set anchor width");
    tree.set_height(anchor, Length::points(20.0))
        .expect("set anchor height");

    tree.set_display(before, Display::Block)
        .expect("set before display");
    tree.set_relative_left_of(before, 20)
        .expect("set before left-of");
    tree.set_relative_top_of(before, 20)
        .expect("set before top-of");
    tree.set_width(before, Length::points(10.0))
        .expect("set before width");
    tree.set_height(before, Length::points(10.0))
        .expect("set before height");

    tree.set_display(aligned, Display::Block)
        .expect("set aligned display");
    tree.set_relative_align_left(aligned, 20)
        .expect("set aligned left");
    tree.set_relative_align_bottom(aligned, 20)
        .expect("set aligned bottom");
    tree.set_width(aligned, Length::points(5.0))
        .expect("set aligned width");
    tree.set_height(aligned, Length::points(7.0))
        .expect("set aligned height");

    tree.append_child(root, center_none)
        .expect("append none center");
    tree.append_child(root, center_horizontal)
        .expect("append horizontal center");
    tree.append_child(root, center_vertical)
        .expect("append vertical center");
    tree.append_child(root, center_both)
        .expect("append both center");
    tree.append_child(root, parent_end)
        .expect("append parent-end");
    tree.append_child(root, before).expect("append before");
    tree.append_child(root, aligned).expect("append aligned");
    tree.append_child(root, anchor).expect("append anchor");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::definite(100.0, 80.0),
        Direction::Ltr,
    )
    .expect("layout relative definite tree");

    StandalonePublicRelativeLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_relative_layout_once_snapshot() -> StandalonePublicRelativeLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let nodes = [root, first, second];

    tree.set_display(root, Display::Relative)
        .expect("set root display");
    tree.set_relative_layout_once(root, true)
        .expect("set root relative layout once");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_relative_id(first, 1).expect("set first id");
    tree.set_relative_bottom_of(first, 2)
        .expect("set first bottom-of");
    tree.set_width(first, Length::points(10.0))
        .expect("set first width");
    tree.set_height(first, Length::points(10.0))
        .expect("set first height");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_relative_id(second, 2).expect("set second id");
    tree.set_relative_right_of(second, 1)
        .expect("set second right-of");
    tree.set_width(second, Length::points(5.0))
        .expect("set second width");
    tree.set_height(second, Length::points(7.0))
        .expect("set second height");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::indefinite(),
        Direction::Ltr,
    )
    .expect("layout relative layout-once tree");

    StandalonePublicRelativeLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_linear_layout_snapshots() -> Vec<StandalonePublicLinearLayoutSnapshot> {
    let mut snapshots = vec![
        rust_public_linear_gravity_layout_snapshot(),
        rust_public_linear_weight_layout_snapshot(),
    ];
    snapshots.extend(rust_public_linear_weight_variant_layout_snapshots());
    snapshots.extend(rust_public_linear_orientation_layout_snapshots());
    snapshots.extend(rust_public_linear_main_gravity_layout_snapshots());
    snapshots.extend(rust_public_linear_cross_gravity_layout_snapshots());
    snapshots.extend(rust_public_linear_layout_gravity_variant_layout_snapshots());
    snapshots
}

#[cfg(feature = "native-standalone")]
fn rust_public_linear_gravity_layout_snapshot() -> StandalonePublicLinearLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let end_aligned = tree.create_default_node();
    let stretched = tree.create_default_node();
    let nodes = [root, first, end_aligned, stretched];

    tree.set_display(root, Display::Linear)
        .expect("set root display");
    tree.set_linear_orientation(root, LinearOrientation::Horizontal)
        .expect("set root linear orientation");
    tree.set_linear_gravity(root, LinearGravity::Center)
        .expect("set root linear gravity");
    tree.set_linear_cross_gravity(root, LinearCrossGravity::Center)
        .expect("set root linear cross gravity");
    tree.set_width(root, Length::points(100.0))
        .expect("set root width");
    tree.set_height(root, Length::points(40.0))
        .expect("set root height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_width(first, Length::points(10.0))
        .expect("set first width");
    tree.set_height(first, Length::points(8.0))
        .expect("set first height");

    tree.set_display(end_aligned, Display::Block)
        .expect("set end-aligned display");
    tree.set_width(end_aligned, Length::points(10.0))
        .expect("set end-aligned width");
    tree.set_height(end_aligned, Length::points(6.0))
        .expect("set end-aligned height");
    tree.set_linear_layout_gravity(end_aligned, LinearLayoutGravity::End)
        .expect("set end-aligned layout gravity");

    tree.set_display(stretched, Display::Block)
        .expect("set stretched display");
    tree.set_width(stretched, Length::points(10.0))
        .expect("set stretched width");
    tree.set_height(stretched, Length::points(5.0))
        .expect("set stretched height");
    tree.set_linear_layout_gravity(stretched, LinearLayoutGravity::Stretch)
        .expect("set stretched layout gravity");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, end_aligned)
        .expect("append end-aligned");
    tree.append_child(root, stretched)
        .expect("append stretched");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::definite(40.0),
        ),
        Direction::Ltr,
    )
    .expect("layout linear gravity tree");

    StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_linear_weight_layout_snapshot() -> StandalonePublicLinearLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let nodes = [root, first, second];

    tree.set_display(root, Display::Linear)
        .expect("set root display");
    tree.set_linear_orientation(root, LinearOrientation::Horizontal)
        .expect("set root linear orientation");
    tree.set_linear_gravity(root, LinearGravity::End)
        .expect("set root linear gravity");
    tree.set_linear_weight_sum(root, 4.0)
        .expect("set root linear weight sum");
    tree.set_width(root, Length::points(100.0))
        .expect("set root width");
    tree.set_height(root, Length::points(20.0))
        .expect("set root height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_linear_weight(first, 1.0)
        .expect("set first linear weight");
    tree.set_height(first, Length::points(10.0))
        .expect("set first height");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_linear_weight(second, 1.0)
        .expect("set second linear weight");
    tree.set_height(second, Length::points(10.0))
        .expect("set second height");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::definite(20.0),
        ),
        Direction::Ltr,
    )
    .expect("layout linear weight tree");

    StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_linear_weight_variant_layout_snapshots() -> Vec<StandalonePublicLinearLayoutSnapshot>
{
    vec![
        rust_public_horizontal_linear_weight_ratio_layout_snapshot(),
        rust_public_vertical_linear_weight_ratio_layout_snapshot(),
        rust_public_vertical_linear_weight_sum_layout_snapshot(),
        rust_public_linear_total_weight_below_one_layout_snapshot(),
    ]
}

#[cfg(feature = "native-standalone")]
fn rust_public_horizontal_linear_weight_ratio_layout_snapshot(
) -> StandalonePublicLinearLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let fixed = tree.create_default_node();
    let one_share = tree.create_default_node();
    let two_shares = tree.create_default_node();
    let nodes = [root, fixed, one_share, two_shares];

    tree.set_display(root, Display::Linear)
        .expect("set root display");
    tree.set_linear_orientation(root, LinearOrientation::Horizontal)
        .expect("set root linear orientation");
    tree.set_width(root, Length::points(120.0))
        .expect("set root width");
    tree.set_height(root, Length::points(30.0))
        .expect("set root height");

    tree.set_display(fixed, Display::Block)
        .expect("set fixed display");
    tree.set_width(fixed, Length::points(15.0))
        .expect("set fixed width");
    tree.set_height(fixed, Length::points(10.0))
        .expect("set fixed height");

    tree.set_display(one_share, Display::Block)
        .expect("set one-share display");
    tree.set_linear_weight(one_share, 1.0)
        .expect("set one-share linear weight");
    tree.set_height(one_share, Length::points(12.0))
        .expect("set one-share height");

    tree.set_display(two_shares, Display::Block)
        .expect("set two-shares display");
    tree.set_linear_weight(two_shares, 2.0)
        .expect("set two-shares linear weight");
    tree.set_height(two_shares, Length::points(14.0))
        .expect("set two-shares height");

    tree.append_child(root, fixed).expect("append fixed");
    tree.append_child(root, one_share)
        .expect("append one-share");
    tree.append_child(root, two_shares)
        .expect("append two-shares");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(120.0),
            SideConstraint::definite(30.0),
        ),
        Direction::Ltr,
    )
    .expect("layout horizontal linear weight ratio tree");

    StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_vertical_linear_weight_ratio_layout_snapshot() -> StandalonePublicLinearLayoutSnapshot
{
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let fixed = tree.create_default_node();
    let one_share = tree.create_default_node();
    let two_shares = tree.create_default_node();
    let nodes = [root, fixed, one_share, two_shares];

    tree.set_display(root, Display::Linear)
        .expect("set root display");
    tree.set_linear_orientation(root, LinearOrientation::Vertical)
        .expect("set root linear orientation");
    tree.set_width(root, Length::points(50.0))
        .expect("set root width");
    tree.set_height(root, Length::points(120.0))
        .expect("set root height");

    tree.set_display(fixed, Display::Block)
        .expect("set fixed display");
    tree.set_width(fixed, Length::points(10.0))
        .expect("set fixed width");
    tree.set_height(fixed, Length::points(15.0))
        .expect("set fixed height");

    tree.set_display(one_share, Display::Block)
        .expect("set one-share display");
    tree.set_linear_weight(one_share, 1.0)
        .expect("set one-share linear weight");
    tree.set_width(one_share, Length::points(12.0))
        .expect("set one-share width");

    tree.set_display(two_shares, Display::Block)
        .expect("set two-shares display");
    tree.set_linear_weight(two_shares, 2.0)
        .expect("set two-shares linear weight");
    tree.set_width(two_shares, Length::points(14.0))
        .expect("set two-shares width");

    tree.append_child(root, fixed).expect("append fixed");
    tree.append_child(root, one_share)
        .expect("append one-share");
    tree.append_child(root, two_shares)
        .expect("append two-shares");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(50.0),
            SideConstraint::definite(120.0),
        ),
        Direction::Ltr,
    )
    .expect("layout vertical linear weight ratio tree");

    StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_vertical_linear_weight_sum_layout_snapshot() -> StandalonePublicLinearLayoutSnapshot
{
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let fixed = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let nodes = [root, fixed, first, second];

    tree.set_display(root, Display::Linear)
        .expect("set root display");
    tree.set_linear_orientation(root, LinearOrientation::Vertical)
        .expect("set root linear orientation");
    tree.set_linear_weight_sum(root, 4.0)
        .expect("set root linear weight sum");
    tree.set_width(root, Length::points(40.0))
        .expect("set root width");
    tree.set_height(root, Length::points(100.0))
        .expect("set root height");

    tree.set_display(fixed, Display::Block)
        .expect("set fixed display");
    tree.set_width(fixed, Length::points(10.0))
        .expect("set fixed width");
    tree.set_height(fixed, Length::points(20.0))
        .expect("set fixed height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_linear_weight(first, 1.0)
        .expect("set first linear weight");
    tree.set_width(first, Length::points(10.0))
        .expect("set first width");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_linear_weight(second, 1.0)
        .expect("set second linear weight");
    tree.set_width(second, Length::points(12.0))
        .expect("set second width");

    tree.append_child(root, fixed).expect("append fixed");
    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(40.0),
            SideConstraint::definite(100.0),
        ),
        Direction::Ltr,
    )
    .expect("layout vertical linear weight sum tree");

    StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_linear_total_weight_below_one_layout_snapshot(
) -> StandalonePublicLinearLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let child = tree.create_default_node();
    let nodes = [root, child];

    tree.set_display(root, Display::Linear)
        .expect("set root display");
    tree.set_linear_orientation(root, LinearOrientation::Horizontal)
        .expect("set root linear orientation");
    tree.set_width(root, Length::points(100.0))
        .expect("set root width");
    tree.set_height(root, Length::points(20.0))
        .expect("set root height");

    tree.set_display(child, Display::Block)
        .expect("set child display");
    tree.set_linear_weight(child, 0.5)
        .expect("set child linear weight");
    tree.set_height(child, Length::points(10.0))
        .expect("set child height");

    tree.append_child(root, child).expect("append child");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::definite(20.0),
        ),
        Direction::Ltr,
    )
    .expect("layout total weight below one tree");

    StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_linear_orientation_layout_snapshots() -> Vec<StandalonePublicLinearLayoutSnapshot> {
    [
        LinearOrientation::Horizontal,
        LinearOrientation::HorizontalReverse,
        LinearOrientation::Vertical,
        LinearOrientation::VerticalReverse,
        LinearOrientation::Row,
        LinearOrientation::Column,
        LinearOrientation::RowReverse,
        LinearOrientation::ColumnReverse,
    ]
    .into_iter()
    .map(rust_public_linear_orientation_layout_snapshot)
    .collect()
}

#[cfg(feature = "native-standalone")]
fn rust_public_linear_orientation_layout_snapshot(
    orientation: LinearOrientation,
) -> StandalonePublicLinearLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let nodes = [root, first, second, third];

    tree.set_display(root, Display::Linear)
        .expect("set root display");
    tree.set_linear_orientation(root, orientation)
        .expect("set root linear orientation");
    tree.set_width(root, Length::points(90.0))
        .expect("set root width");
    tree.set_height(root, Length::points(70.0))
        .expect("set root height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_width(first, Length::points(10.0))
        .expect("set first width");
    tree.set_height(first, Length::points(12.0))
        .expect("set first height");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_width(second, Length::points(20.0))
        .expect("set second width");
    tree.set_height(second, Length::points(16.0))
        .expect("set second height");

    tree.set_display(third, Display::Block)
        .expect("set third display");
    tree.set_width(third, Length::points(15.0))
        .expect("set third width");
    tree.set_height(third, Length::points(10.0))
        .expect("set third height");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.append_child(root, third).expect("append third");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(90.0),
            SideConstraint::definite(70.0),
        ),
        Direction::Ltr,
    )
    .expect("layout linear orientation tree");

    StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_linear_main_gravity_layout_snapshots() -> Vec<StandalonePublicLinearLayoutSnapshot> {
    let mut snapshots = Vec::new();
    for gravity in [
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
    ] {
        snapshots.push(rust_public_linear_main_gravity_layout_snapshot(
            LinearOrientation::Vertical,
            gravity,
        ));
        snapshots.push(rust_public_linear_main_gravity_layout_snapshot(
            LinearOrientation::Horizontal,
            gravity,
        ));
    }
    snapshots
}

#[cfg(feature = "native-standalone")]
fn rust_public_linear_main_gravity_layout_snapshot(
    orientation: LinearOrientation,
    gravity: LinearGravity,
) -> StandalonePublicLinearLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let nodes = [root, first, second, third];

    tree.set_display(root, Display::Linear)
        .expect("set root display");
    tree.set_linear_orientation(root, orientation)
        .expect("set root linear orientation");
    tree.set_linear_gravity(root, gravity)
        .expect("set root linear gravity");
    tree.set_width(root, Length::points(100.0))
        .expect("set root width");
    tree.set_height(root, Length::points(90.0))
        .expect("set root height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_width(first, Length::points(10.0))
        .expect("set first width");
    tree.set_height(first, Length::points(12.0))
        .expect("set first height");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_width(second, Length::points(20.0))
        .expect("set second width");
    tree.set_height(second, Length::points(16.0))
        .expect("set second height");

    tree.set_display(third, Display::Block)
        .expect("set third display");
    tree.set_width(third, Length::points(15.0))
        .expect("set third width");
    tree.set_height(third, Length::points(10.0))
        .expect("set third height");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.append_child(root, third).expect("append third");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::definite(90.0),
        ),
        Direction::Ltr,
    )
    .expect("layout linear main-gravity tree");

    StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_linear_cross_gravity_layout_snapshots() -> Vec<StandalonePublicLinearLayoutSnapshot>
{
    let mut snapshots = Vec::new();
    for cross_gravity in [
        LinearCrossGravity::None,
        LinearCrossGravity::Start,
        LinearCrossGravity::End,
        LinearCrossGravity::Center,
        LinearCrossGravity::Stretch,
    ] {
        snapshots.push(rust_public_linear_cross_gravity_layout_snapshot(
            LinearOrientation::Vertical,
            cross_gravity,
        ));
        snapshots.push(rust_public_linear_cross_gravity_layout_snapshot(
            LinearOrientation::Horizontal,
            cross_gravity,
        ));
    }
    snapshots
}

#[cfg(feature = "native-standalone")]
fn rust_public_linear_cross_gravity_layout_snapshot(
    orientation: LinearOrientation,
    cross_gravity: LinearCrossGravity,
) -> StandalonePublicLinearLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let nodes = [root, first, second];

    tree.set_display(root, Display::Linear)
        .expect("set root display");
    tree.set_linear_orientation(root, orientation)
        .expect("set root linear orientation");
    tree.set_linear_cross_gravity(root, cross_gravity)
        .expect("set root linear cross gravity");
    tree.set_width(root, Length::points(100.0))
        .expect("set root width");
    tree.set_height(root, Length::points(90.0))
        .expect("set root height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_width(first, Length::points(20.0))
        .expect("set first width");
    tree.set_height(first, Length::points(10.0))
        .expect("set first height");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_width(second, Length::points(30.0))
        .expect("set second width");
    tree.set_height(second, Length::points(12.0))
        .expect("set second height");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::definite(90.0),
        ),
        Direction::Ltr,
    )
    .expect("layout linear cross-gravity tree");

    StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_linear_layout_gravity_variant_layout_snapshots(
) -> Vec<StandalonePublicLinearLayoutSnapshot> {
    let mut snapshots = Vec::new();
    for layout_gravity in [
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
    ] {
        snapshots.push(rust_public_linear_layout_gravity_variant_layout_snapshot(
            LinearOrientation::Vertical,
            layout_gravity,
        ));
        snapshots.push(rust_public_linear_layout_gravity_variant_layout_snapshot(
            LinearOrientation::Horizontal,
            layout_gravity,
        ));
    }
    snapshots
}

#[cfg(feature = "native-standalone")]
fn rust_public_linear_layout_gravity_variant_layout_snapshot(
    orientation: LinearOrientation,
    layout_gravity: LinearLayoutGravity,
) -> StandalonePublicLinearLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let child = tree.create_default_node();
    let nodes = [root, child];

    tree.set_display(root, Display::Linear)
        .expect("set root display");
    tree.set_linear_orientation(root, orientation)
        .expect("set root linear orientation");
    tree.set_linear_cross_gravity(root, LinearCrossGravity::Start)
        .expect("set root linear cross gravity");
    tree.set_width(root, Length::points(100.0))
        .expect("set root width");
    tree.set_height(root, Length::points(90.0))
        .expect("set root height");

    tree.set_display(child, Display::Block)
        .expect("set child display");
    tree.set_linear_layout_gravity(child, layout_gravity)
        .expect("set child layout gravity");
    tree.set_width(child, Length::points(20.0))
        .expect("set child width");
    tree.set_height(child, Length::points(10.0))
        .expect("set child height");

    tree.append_child(root, child).expect("append child");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::definite(90.0),
        ),
        Direction::Ltr,
    )
    .expect("layout linear layout-gravity tree");

    StandalonePublicLinearLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_layout_snapshots() -> Vec<StandalonePublicFlexLayoutSnapshot> {
    let mut snapshots = vec![
        rust_public_flex_alignment_order_layout_snapshot(),
        rust_public_flex_grow_layout_snapshot(),
        rust_public_flex_shrink_layout_snapshot(),
        rust_public_flex_align_content_layout_snapshot(),
    ];
    snapshots.extend(rust_public_flex_wrap_layout_snapshots());
    snapshots.extend(rust_public_flex_align_content_variant_layout_snapshots());
    snapshots.extend(rust_public_flex_direction_layout_snapshots());
    snapshots.extend(rust_public_flex_justify_content_layout_snapshots());
    snapshots.extend(rust_public_flex_align_items_layout_snapshots());
    snapshots.push(rust_public_flex_align_self_layout_snapshot());
    snapshots.extend(rust_public_flex_align_self_variant_layout_snapshots());
    snapshots.push(rust_public_flex_align_items_baseline_layout_snapshot());
    snapshots.push(rust_public_flex_align_self_baseline_layout_snapshot());
    snapshots
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_baseline_measure_first(_constraints: starlight_layout::Constraints) -> Size {
    Size::new(30.0, 20.0)
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_baseline_measure_second(_constraints: starlight_layout::Constraints) -> Size {
    Size::new(20.0, 10.0)
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_baseline_measure_third(_constraints: starlight_layout::Constraints) -> Size {
    Size::new(25.0, 16.0)
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_baseline_first(_content_size: Size) -> f32 {
    15.0
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_baseline_second(_content_size: Size) -> f32 {
    4.0
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_baseline_third(_content_size: Size) -> f32 {
    8.0
}

#[cfg(feature = "native-standalone")]
fn set_rust_public_flex_baseline_children(
    tree: &mut StandaloneTree,
    first: NodeId,
    second: NodeId,
    third: NodeId,
) {
    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_measure_func(first, Some(rust_public_flex_baseline_measure_first))
        .expect("set first measure");
    tree.set_baseline_func(first, Some(rust_public_flex_baseline_first))
        .expect("set first baseline");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_measure_func(second, Some(rust_public_flex_baseline_measure_second))
        .expect("set second measure");
    tree.set_baseline_func(second, Some(rust_public_flex_baseline_second))
        .expect("set second baseline");

    tree.set_display(third, Display::Block)
        .expect("set third display");
    tree.set_measure_func(third, Some(rust_public_flex_baseline_measure_third))
        .expect("set third measure");
    tree.set_baseline_func(third, Some(rust_public_flex_baseline_third))
        .expect("set third baseline");
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_align_items_baseline_layout_snapshot() -> StandalonePublicFlexLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let nodes = [root, first, second, third];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, FlexWrap::NoWrap)
        .expect("set root flex wrap");
    tree.set_justify_content(root, JustifyContent::FlexStart)
        .expect("set root justify content");
    tree.set_align_items(root, AlignItems::Baseline)
        .expect("set root align items");
    tree.set_width(root, Length::points(120.0))
        .expect("set root width");
    tree.set_height(root, Length::points(60.0))
        .expect("set root height");
    set_rust_public_flex_baseline_children(&mut tree, first, second, third);

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.append_child(root, third).expect("append third");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(120.0),
            SideConstraint::definite(60.0),
        ),
        Direction::Ltr,
    )
    .expect("layout flex align-items baseline tree");

    StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_align_self_baseline_layout_snapshot() -> StandalonePublicFlexLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let nodes = [root, first, second, third];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, FlexWrap::NoWrap)
        .expect("set root flex wrap");
    tree.set_justify_content(root, JustifyContent::FlexStart)
        .expect("set root justify content");
    tree.set_align_items(root, AlignItems::FlexStart)
        .expect("set root align items");
    tree.set_width(root, Length::points(120.0))
        .expect("set root width");
    tree.set_height(root, Length::points(60.0))
        .expect("set root height");
    set_rust_public_flex_baseline_children(&mut tree, first, second, third);
    tree.set_align_self(first, Some(AlignItems::Baseline))
        .expect("set first align self");
    tree.set_align_self(second, Some(AlignItems::Baseline))
        .expect("set second align self");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.append_child(root, third).expect("append third");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(120.0),
            SideConstraint::definite(60.0),
        ),
        Direction::Ltr,
    )
    .expect("layout flex align-self baseline tree");

    StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_align_content_variant_layout_snapshots(
) -> Vec<StandalonePublicFlexLayoutSnapshot> {
    [
        AlignContent::FlexStart,
        AlignContent::FlexEnd,
        AlignContent::Center,
        AlignContent::Stretch,
        AlignContent::SpaceBetween,
        AlignContent::SpaceAround,
        AlignContent::SpaceEvenly,
        AlignContent::Start,
        AlignContent::End,
    ]
    .into_iter()
    .map(rust_public_flex_align_content_variant_layout_snapshot)
    .collect()
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_align_content_variant_layout_snapshot(
    align_content: AlignContent,
) -> StandalonePublicFlexLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let nodes = [root, first, second, third];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, FlexWrap::Wrap)
        .expect("set root flex wrap");
    tree.set_justify_content(root, JustifyContent::FlexStart)
        .expect("set root justify content");
    tree.set_align_content(root, align_content)
        .expect("set root align content");
    tree.set_align_items(root, AlignItems::FlexStart)
        .expect("set root align items");
    tree.set_width(root, Length::points(55.0))
        .expect("set root width");
    tree.set_height(root, Length::points(105.0))
        .expect("set root height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_width(first, Length::points(30.0))
        .expect("set first width");
    tree.set_height(first, Length::points(10.0))
        .expect("set first height");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_width(second, Length::points(30.0))
        .expect("set second width");
    tree.set_height(second, Length::points(20.0))
        .expect("set second height");

    tree.set_display(third, Display::Block)
        .expect("set third display");
    tree.set_width(third, Length::points(30.0))
        .expect("set third width");
    tree.set_height(third, Length::points(15.0))
        .expect("set third height");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.append_child(root, third).expect("append third");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(55.0),
            SideConstraint::definite(105.0),
        ),
        Direction::Ltr,
    )
    .expect("layout flex align-content variant tree");

    StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_align_self_layout_snapshot() -> StandalonePublicFlexLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let inherited = tree.create_default_node();
    let self_start = tree.create_default_node();
    let self_end = tree.create_default_node();
    let self_stretch = tree.create_default_node();
    let nodes = [root, inherited, self_start, self_end, self_stretch];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, FlexWrap::NoWrap)
        .expect("set root flex wrap");
    tree.set_justify_content(root, JustifyContent::FlexStart)
        .expect("set root justify content");
    tree.set_align_items(root, AlignItems::Center)
        .expect("set root align items");
    tree.set_width(root, Length::points(120.0))
        .expect("set root width");
    tree.set_height(root, Length::points(60.0))
        .expect("set root height");

    tree.set_display(inherited, Display::Block)
        .expect("set inherited display");
    tree.set_width(inherited, Length::points(10.0))
        .expect("set inherited width");
    tree.set_height(inherited, Length::points(10.0))
        .expect("set inherited height");

    tree.set_display(self_start, Display::Block)
        .expect("set self-start display");
    tree.set_width(self_start, Length::points(10.0))
        .expect("set self-start width");
    tree.set_height(self_start, Length::points(20.0))
        .expect("set self-start height");
    tree.set_align_self(self_start, Some(AlignItems::Start))
        .expect("set self-start align self");

    tree.set_display(self_end, Display::Block)
        .expect("set self-end display");
    tree.set_width(self_end, Length::points(10.0))
        .expect("set self-end width");
    tree.set_height(self_end, Length::points(15.0))
        .expect("set self-end height");
    tree.set_align_self(self_end, Some(AlignItems::End))
        .expect("set self-end align self");

    tree.set_display(self_stretch, Display::Block)
        .expect("set self-stretch display");
    tree.set_width(self_stretch, Length::points(10.0))
        .expect("set self-stretch width");
    tree.set_align_self(self_stretch, Some(AlignItems::Stretch))
        .expect("set self-stretch align self");

    tree.append_child(root, inherited)
        .expect("append inherited");
    tree.append_child(root, self_start)
        .expect("append self-start");
    tree.append_child(root, self_end).expect("append self-end");
    tree.append_child(root, self_stretch)
        .expect("append self-stretch");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(120.0),
            SideConstraint::definite(60.0),
        ),
        Direction::Ltr,
    )
    .expect("layout flex align-self tree");

    StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_align_self_variant_layout_snapshots() -> Vec<StandalonePublicFlexLayoutSnapshot>
{
    [
        None,
        Some(AlignItems::Stretch),
        Some(AlignItems::FlexStart),
        Some(AlignItems::FlexEnd),
        Some(AlignItems::Center),
        Some(AlignItems::Baseline),
        Some(AlignItems::Start),
        Some(AlignItems::End),
    ]
    .into_iter()
    .map(rust_public_flex_align_self_variant_layout_snapshot)
    .collect()
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_align_self_variant_layout_snapshot(
    align_self: Option<AlignItems>,
) -> StandalonePublicFlexLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let inherited = tree.create_default_node();
    let nodes = [root, first, inherited];
    let first_has_height = align_self != Some(AlignItems::Stretch);

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, FlexWrap::NoWrap)
        .expect("set root flex wrap");
    tree.set_justify_content(root, JustifyContent::FlexStart)
        .expect("set root justify content");
    tree.set_align_items(root, AlignItems::FlexEnd)
        .expect("set root align items");
    tree.set_width(root, Length::points(100.0))
        .expect("set root width");
    tree.set_height(root, Length::points(50.0))
        .expect("set root height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_width(first, Length::points(12.0))
        .expect("set first width");
    if first_has_height {
        tree.set_height(first, Length::points(8.0))
            .expect("set first height");
    }
    tree.set_align_self(first, align_self)
        .expect("set first align self");

    tree.set_display(inherited, Display::Block)
        .expect("set inherited display");
    tree.set_width(inherited, Length::points(14.0))
        .expect("set inherited width");
    tree.set_height(inherited, Length::points(10.0))
        .expect("set inherited height");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, inherited)
        .expect("append inherited");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::definite(50.0),
        ),
        Direction::Ltr,
    )
    .expect("layout flex align-self variant tree");

    StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_align_items_layout_snapshots() -> Vec<StandalonePublicFlexLayoutSnapshot> {
    [
        AlignItems::Stretch,
        AlignItems::FlexStart,
        AlignItems::FlexEnd,
        AlignItems::Center,
        AlignItems::Baseline,
        AlignItems::Start,
        AlignItems::End,
    ]
    .into_iter()
    .map(rust_public_flex_align_items_layout_snapshot)
    .collect()
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_align_items_layout_snapshot(
    align_items: AlignItems,
) -> StandalonePublicFlexLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let nodes = [root, first, second, third];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, FlexWrap::NoWrap)
        .expect("set root flex wrap");
    tree.set_justify_content(root, JustifyContent::FlexStart)
        .expect("set root justify content");
    tree.set_align_items(root, align_items)
        .expect("set root align items");
    tree.set_width(root, Length::points(100.0))
        .expect("set root width");
    tree.set_height(root, Length::points(60.0))
        .expect("set root height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_width(first, Length::points(10.0))
        .expect("set first width");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_width(second, Length::points(20.0))
        .expect("set second width");

    tree.set_display(third, Display::Block)
        .expect("set third display");
    tree.set_width(third, Length::points(15.0))
        .expect("set third width");

    if align_items != AlignItems::Stretch {
        tree.set_height(first, Length::points(10.0))
            .expect("set first height");
        tree.set_height(second, Length::points(20.0))
            .expect("set second height");
        tree.set_height(third, Length::points(15.0))
            .expect("set third height");
    }

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.append_child(root, third).expect("append third");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::definite(60.0),
        ),
        Direction::Ltr,
    )
    .expect("layout flex align-items tree");

    StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_justify_content_layout_snapshots() -> Vec<StandalonePublicFlexLayoutSnapshot> {
    [
        JustifyContent::FlexStart,
        JustifyContent::Center,
        JustifyContent::FlexEnd,
        JustifyContent::SpaceBetween,
        JustifyContent::SpaceAround,
        JustifyContent::SpaceEvenly,
        JustifyContent::Stretch,
        JustifyContent::Start,
        JustifyContent::End,
    ]
    .into_iter()
    .map(rust_public_flex_justify_content_layout_snapshot)
    .collect()
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_justify_content_layout_snapshot(
    justify_content: JustifyContent,
) -> StandalonePublicFlexLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let nodes = [root, first, second, third];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, FlexWrap::NoWrap)
        .expect("set root flex wrap");
    tree.set_justify_content(root, justify_content)
        .expect("set root justify content");
    tree.set_align_items(root, AlignItems::FlexStart)
        .expect("set root align items");
    tree.set_width(root, Length::points(105.0))
        .expect("set root width");
    tree.set_height(root, Length::points(30.0))
        .expect("set root height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_width(first, Length::points(10.0))
        .expect("set first width");
    tree.set_height(first, Length::points(10.0))
        .expect("set first height");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_width(second, Length::points(20.0))
        .expect("set second width");
    tree.set_height(second, Length::points(10.0))
        .expect("set second height");

    tree.set_display(third, Display::Block)
        .expect("set third display");
    tree.set_width(third, Length::points(15.0))
        .expect("set third width");
    tree.set_height(third, Length::points(10.0))
        .expect("set third height");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.append_child(root, third).expect("append third");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(105.0),
            SideConstraint::definite(30.0),
        ),
        Direction::Ltr,
    )
    .expect("layout flex justify-content tree");

    StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_direction_layout_snapshots() -> Vec<StandalonePublicFlexLayoutSnapshot> {
    [
        FlexDirection::Column,
        FlexDirection::Row,
        FlexDirection::RowReverse,
        FlexDirection::ColumnReverse,
    ]
    .into_iter()
    .map(rust_public_flex_direction_layout_snapshot)
    .collect()
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_direction_layout_snapshot(
    flex_direction: FlexDirection,
) -> StandalonePublicFlexLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let nodes = [root, first, second, third];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, flex_direction)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, FlexWrap::NoWrap)
        .expect("set root flex wrap");
    tree.set_justify_content(root, JustifyContent::FlexStart)
        .expect("set root justify content");
    tree.set_align_items(root, AlignItems::FlexStart)
        .expect("set root align items");
    tree.set_width(root, Length::points(100.0))
        .expect("set root width");
    tree.set_height(root, Length::points(90.0))
        .expect("set root height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_width(first, Length::points(10.0))
        .expect("set first width");
    tree.set_height(first, Length::points(15.0))
        .expect("set first height");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_width(second, Length::points(20.0))
        .expect("set second width");
    tree.set_height(second, Length::points(25.0))
        .expect("set second height");

    tree.set_display(third, Display::Block)
        .expect("set third display");
    tree.set_width(third, Length::points(15.0))
        .expect("set third width");
    tree.set_height(third, Length::points(10.0))
        .expect("set third height");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.append_child(root, third).expect("append third");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::definite(90.0),
        ),
        Direction::Ltr,
    )
    .expect("layout flex direction tree");

    StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_alignment_order_layout_snapshot() -> StandalonePublicFlexLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let nodes = [root, first, second, third];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, FlexWrap::NoWrap)
        .expect("set root flex wrap");
    tree.set_justify_content(root, JustifyContent::SpaceBetween)
        .expect("set root justify content");
    tree.set_align_items(root, AlignItems::Center)
        .expect("set root align items");
    tree.set_width(root, Length::points(180.0))
        .expect("set root width");
    tree.set_height(root, Length::points(60.0))
        .expect("set root height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_flex_basis(first, Length::points(30.0))
        .expect("set first flex basis");
    tree.set_height(first, Length::points(10.0))
        .expect("set first height");
    tree.set_order(first, 2).expect("set first order");
    tree.set_align_self(first, Some(AlignItems::FlexEnd))
        .expect("set first align self");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_flex_basis(second, Length::percent(25.0))
        .expect("set second flex basis");
    tree.set_height(second, Length::points(20.0))
        .expect("set second height");
    tree.set_order(second, -1).expect("set second order");
    tree.set_align_self(second, Some(AlignItems::Center))
        .expect("set second align self");

    tree.set_display(third, Display::Block)
        .expect("set third display");
    tree.set_flex_basis(third, Length::calc(20.0, 10.0))
        .expect("set third flex basis");
    tree.set_height(third, Length::points(30.0))
        .expect("set third height");
    tree.set_order(third, 1).expect("set third order");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.append_child(root, third).expect("append third");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(180.0),
            SideConstraint::definite(60.0),
        ),
        Direction::Ltr,
    )
    .expect("layout flex alignment/order tree");

    StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_grow_layout_snapshot() -> StandalonePublicFlexLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let fixed = tree.create_default_node();
    let grow = tree.create_default_node();
    let shorthand = tree.create_default_node();
    let nodes = [root, fixed, grow, shorthand];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, FlexWrap::NoWrap)
        .expect("set root flex wrap");
    tree.set_align_items(root, AlignItems::FlexStart)
        .expect("set root align items");
    tree.set_width(root, Length::points(180.0))
        .expect("set root width");
    tree.set_height(root, Length::points(50.0))
        .expect("set root height");

    tree.set_display(fixed, Display::Block)
        .expect("set fixed display");
    tree.set_flex_basis(fixed, Length::points(30.0))
        .expect("set fixed flex basis");
    tree.set_height(fixed, Length::points(10.0))
        .expect("set fixed height");

    tree.set_display(grow, Display::Block)
        .expect("set grow display");
    tree.set_flex_basis(grow, Length::points(20.0))
        .expect("set grow flex basis");
    tree.set_flex_grow(grow, 1.0).expect("set grow factor");
    tree.set_height(grow, Length::points(20.0))
        .expect("set grow height");

    tree.set_display(shorthand, Display::Block)
        .expect("set shorthand display");
    tree.set_flex(shorthand, 2.0).expect("set shorthand flex");
    tree.set_height(shorthand, Length::points(30.0))
        .expect("set shorthand height");

    tree.append_child(root, fixed).expect("append fixed");
    tree.append_child(root, grow).expect("append grow");
    tree.append_child(root, shorthand)
        .expect("append shorthand");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(180.0),
            SideConstraint::definite(50.0),
        ),
        Direction::Ltr,
    )
    .expect("layout flex grow tree");

    StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_shrink_layout_snapshot() -> StandalonePublicFlexLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let inflexible = tree.create_default_node();
    let nodes = [root, first, second, inflexible];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, FlexWrap::NoWrap)
        .expect("set root flex wrap");
    tree.set_align_items(root, AlignItems::FlexStart)
        .expect("set root align items");
    tree.set_width(root, Length::points(90.0))
        .expect("set root width");
    tree.set_height(root, Length::points(50.0))
        .expect("set root height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_flex_basis(first, Length::points(60.0))
        .expect("set first flex basis");
    tree.set_flex_shrink(first, 1.0).expect("set first shrink");
    tree.set_height(first, Length::points(10.0))
        .expect("set first height");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_flex_basis(second, Length::calc(40.0, 20.0))
        .expect("set second flex basis");
    tree.set_flex_shrink(second, 2.0)
        .expect("set second shrink");
    tree.set_height(second, Length::points(20.0))
        .expect("set second height");

    tree.set_display(inflexible, Display::Block)
        .expect("set inflexible display");
    tree.set_flex_basis(inflexible, Length::percent(30.0))
        .expect("set inflexible flex basis");
    tree.set_flex_shrink(inflexible, 0.0)
        .expect("set inflexible shrink");
    tree.set_height(inflexible, Length::points(30.0))
        .expect("set inflexible height");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.append_child(root, inflexible)
        .expect("append inflexible");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(90.0),
            SideConstraint::definite(50.0),
        ),
        Direction::Ltr,
    )
    .expect("layout flex shrink tree");

    StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_wrap_layout_snapshots() -> Vec<StandalonePublicFlexLayoutSnapshot> {
    [FlexWrap::Wrap, FlexWrap::NoWrap, FlexWrap::WrapReverse]
        .into_iter()
        .map(rust_public_flex_wrap_layout_snapshot)
        .collect()
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_wrap_layout_snapshot(
    flex_wrap: FlexWrap,
) -> StandalonePublicFlexLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let nodes = [root, first, second, third];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, flex_wrap)
        .expect("set root flex wrap");
    tree.set_justify_content(root, JustifyContent::FlexStart)
        .expect("set root justify content");
    tree.set_align_content(root, AlignContent::FlexStart)
        .expect("set root align content");
    tree.set_align_items(root, AlignItems::FlexStart)
        .expect("set root align items");
    tree.set_width(root, Length::points(55.0))
        .expect("set root width");
    tree.set_height(root, Length::points(80.0))
        .expect("set root height");
    tree.set_gap(root, StandaloneGap::Row, Length::points(5.0))
        .expect("set root row gap");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_width(first, Length::points(30.0))
        .expect("set first width");
    tree.set_height(first, Length::points(10.0))
        .expect("set first height");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_width(second, Length::points(30.0))
        .expect("set second width");
    tree.set_height(second, Length::points(20.0))
        .expect("set second height");

    tree.set_display(third, Display::Block)
        .expect("set third display");
    tree.set_width(third, Length::points(30.0))
        .expect("set third width");
    tree.set_height(third, Length::points(15.0))
        .expect("set third height");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.append_child(root, third).expect("append third");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(55.0),
            SideConstraint::definite(80.0),
        ),
        Direction::Ltr,
    )
    .expect("layout flex wrap-reverse tree");

    StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_flex_align_content_layout_snapshot() -> StandalonePublicFlexLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let nodes = [root, first, second, third];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, FlexWrap::Wrap)
        .expect("set root flex wrap");
    tree.set_justify_content(root, JustifyContent::FlexStart)
        .expect("set root justify content");
    tree.set_align_content(root, AlignContent::SpaceBetween)
        .expect("set root align content");
    tree.set_align_items(root, AlignItems::FlexStart)
        .expect("set root align items");
    tree.set_width(root, Length::points(55.0))
        .expect("set root width");
    tree.set_height(root, Length::points(95.0))
        .expect("set root height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_width(first, Length::points(30.0))
        .expect("set first width");
    tree.set_height(first, Length::points(10.0))
        .expect("set first height");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_width(second, Length::points(30.0))
        .expect("set second width");
    tree.set_height(second, Length::points(20.0))
        .expect("set second height");

    tree.set_display(third, Display::Block)
        .expect("set third display");
    tree.set_width(third, Length::points(30.0))
        .expect("set third width");
    tree.set_height(third, Length::points(15.0))
        .expect("set third height");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.append_child(root, third).expect("append third");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(55.0),
            SideConstraint::definite(95.0),
        ),
        Direction::Ltr,
    )
    .expect("layout flex align-content tree");

    StandalonePublicFlexLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_edge_layout_snapshots() -> Vec<StandalonePublicEdgeLayoutSnapshot> {
    [
        PublicEdgeStyleVariant::Points,
        PublicEdgeStyleVariant::Percent,
        PublicEdgeStyleVariant::Calc,
        PublicEdgeStyleVariant::ValueFr,
        PublicEdgeStyleVariant::ValueMaxContent,
        PublicEdgeStyleVariant::ValueFitContent,
        PublicEdgeStyleVariant::Auto,
    ]
    .iter()
    .copied()
    .map(rust_public_edge_layout_snapshot)
    .collect()
}

#[cfg(feature = "native-standalone")]
fn rust_public_edge_layout_snapshot(
    variant: PublicEdgeStyleVariant,
) -> StandalonePublicEdgeLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let primary = tree.create_default_node();
    let secondary = tree.create_default_node();
    let trailing = tree.create_default_node();
    let nodes = [root, primary, secondary, trailing];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, FlexWrap::Wrap)
        .expect("set root flex wrap");
    tree.set_align_items(root, AlignItems::FlexStart)
        .expect("set root align items");
    tree.set_width(root, Length::points(140.0))
        .expect("set root width");
    tree.set_height(root, Length::points(120.0))
        .expect("set root height");
    tree.set_padding(root, StandaloneEdge::Left, Length::points(5.0))
        .expect("set root left padding");
    tree.set_border(root, StandaloneEdge::All, 1.0)
        .expect("set root border");
    tree.set_gap(root, StandaloneGap::Row, Length::points(9.0))
        .expect("set root row gap");
    apply_rust_public_edge_layout_container_variant(&mut tree, root, variant);

    tree.set_display(primary, Display::Block)
        .expect("set primary display");
    tree.set_position_type(primary, PositionType::Relative)
        .expect("set primary position type");
    tree.set_width(primary, Length::points(50.0))
        .expect("set primary width");
    tree.set_height(primary, Length::points(20.0))
        .expect("set primary height");
    apply_rust_public_edge_style_variant(&mut tree, primary, variant);
    normalize_rust_public_edge_layout_child_variant(&mut tree, primary, variant);

    tree.set_display(secondary, Display::Block)
        .expect("set secondary display");
    tree.set_width(secondary, Length::points(50.0))
        .expect("set secondary width");
    tree.set_height(secondary, Length::points(25.0))
        .expect("set secondary height");

    tree.set_display(trailing, Display::Block)
        .expect("set trailing display");
    tree.set_width(trailing, Length::points(50.0))
        .expect("set trailing width");
    tree.set_height(trailing, Length::points(30.0))
        .expect("set trailing height");

    tree.append_child(root, primary).expect("append primary");
    tree.append_child(root, secondary)
        .expect("append secondary");
    tree.append_child(root, trailing).expect("append trailing");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(140.0),
            SideConstraint::definite(120.0),
        ),
        Direction::Ltr,
    )
    .expect("layout edge/gap tree");

    StandalonePublicEdgeLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn apply_rust_public_edge_layout_container_variant(
    tree: &mut StandaloneTree,
    node: NodeId,
    variant: PublicEdgeStyleVariant,
) {
    match variant {
        PublicEdgeStyleVariant::Points => {
            tree.set_padding(node, StandaloneEdge::Top, Length::points(3.0))
                .expect("set container padding point");
            tree.set_gap(node, StandaloneGap::Column, Length::points(4.0))
                .expect("set container gap point");
        }
        PublicEdgeStyleVariant::Percent => {
            tree.set_padding(node, StandaloneEdge::Top, Length::percent(13.0))
                .expect("set container padding percent");
            tree.set_gap(node, StandaloneGap::Column, Length::percent(14.0))
                .expect("set container gap percent");
        }
        PublicEdgeStyleVariant::Calc => {
            tree.set_padding(node, StandaloneEdge::Top, Length::calc(25.0, 26.0))
                .expect("set container padding calc");
            tree.set_gap(node, StandaloneGap::Column, Length::calc(27.0, 28.0))
                .expect("set container gap calc");
        }
        PublicEdgeStyleVariant::ValueFr => {
            tree.set_padding(node, StandaloneEdge::Top, Length::fr(3.25))
                .expect("set container padding fr");
            tree.set_gap(node, StandaloneGap::Column, Length::fr(4.25))
                .expect("set container gap fr");
        }
        PublicEdgeStyleVariant::ValueMaxContent => {
            tree.set_padding(node, StandaloneEdge::Top, Length::max_content())
                .expect("set container padding max-content");
            tree.set_gap(node, StandaloneGap::Column, Length::max_content())
                .expect("set container gap max-content");
        }
        PublicEdgeStyleVariant::ValueFitContent => {
            tree.set_padding(
                node,
                StandaloneEdge::Top,
                Length::fit_content(Some(BaseLength::fixed_and_percent(35.0, 0.0))),
            )
            .expect("set container padding fit-content");
            tree.set_gap(
                node,
                StandaloneGap::Column,
                Length::fit_content(Some(BaseLength::fixed_and_percent(37.0, 0.0))),
            )
            .expect("set container gap fit-content");
        }
        PublicEdgeStyleVariant::Auto => {
            tree.set_padding(node, StandaloneEdge::Top, Length::points(41.0))
                .expect("set container padding point for auto case");
            tree.set_gap(node, StandaloneGap::Column, Length::points(42.0))
                .expect("set container gap point for auto case");
        }
    }
}

#[cfg(feature = "native-standalone")]
fn normalize_rust_public_edge_layout_child_variant(
    tree: &mut StandaloneTree,
    node: NodeId,
    variant: PublicEdgeStyleVariant,
) {
    if !matches!(variant, PublicEdgeStyleVariant::ValueFitContent) {
        return;
    }

    tree.set_position(
        node,
        StandaloneEdge::Left,
        Length::fit_content(Some(BaseLength::fixed_and_percent(31.0, 0.0))),
    )
    .expect("normalize child position fit-content");
    tree.set_margin(
        node,
        StandaloneEdge::Right,
        Length::fit_content(Some(BaseLength::fixed_and_percent(33.0, 0.0))),
    )
    .expect("normalize child margin fit-content");
    tree.set_padding(
        node,
        StandaloneEdge::Top,
        Length::fit_content(Some(BaseLength::fixed_and_percent(35.0, 0.0))),
    )
    .expect("normalize child padding fit-content");
    tree.set_gap(
        node,
        StandaloneGap::Column,
        Length::fit_content(Some(BaseLength::fixed_and_percent(37.0, 0.0))),
    )
    .expect("normalize child gap fit-content");
}

#[cfg(feature = "native-standalone")]
fn rust_public_linear_list_layout_snapshot() -> StandalonePublicLinearListLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first_regular = tree.create_default_node();
    let second_regular = tree.create_default_node();
    let explicit_default = tree.create_default_node();
    let header = tree.create_default_node();
    let footer = tree.create_default_node();
    let list_row = tree.create_default_node();
    let nodes = [
        root,
        first_regular,
        second_regular,
        explicit_default,
        header,
        footer,
        list_row,
    ];

    tree.set_display(root, Display::Linear)
        .expect("set root display");
    tree.set_linear_orientation(root, LinearOrientation::Vertical)
        .expect("set root linear orientation");
    tree.set_width(root, Length::points(150.0))
        .expect("set root width");
    tree.set_linear_column_count(root, Some(3))
        .expect("set root column count");
    tree.set_list_cross_axis_gap(root, Length::points(12.0))
        .expect("set root cross-axis gap");
    tree.set_list_main_axis_gap(root, Length::points(4.0))
        .expect("set root main-axis gap");

    tree.set_display(first_regular, Display::Block)
        .expect("set first regular display");
    tree.set_height(first_regular, Length::points(10.0))
        .expect("set first regular height");
    tree.set_margin(first_regular, StandaloneEdge::Left, Length::points(3.0))
        .expect("set first regular left margin");
    tree.set_margin(first_regular, StandaloneEdge::Right, Length::points(5.0))
        .expect("set first regular right margin");

    tree.set_display(second_regular, Display::Block)
        .expect("set second regular display");
    tree.set_width(second_regular, Length::Auto)
        .expect("set second regular auto width");
    tree.set_height(second_regular, Length::points(11.0))
        .expect("set second regular height");

    tree.set_display(explicit_default, Display::Block)
        .expect("set explicit default display");
    tree.set_height(explicit_default, Length::points(12.0))
        .expect("set explicit default height");
    tree.set_list_component_type(explicit_default, Some(ListComponentType::Default))
        .expect("set explicit default component type");

    tree.set_display(header, Display::Block)
        .expect("set header display");
    tree.set_height(header, Length::points(7.0))
        .expect("set header height");
    tree.set_list_component_type(header, Some(ListComponentType::Header))
        .expect("set header component type");

    tree.set_display(footer, Display::Block)
        .expect("set footer display");
    tree.set_height(footer, Length::points(8.0))
        .expect("set footer height");
    tree.set_list_component_type(footer, Some(ListComponentType::Footer))
        .expect("set footer component type");

    tree.set_display(list_row, Display::Block)
        .expect("set list row display");
    tree.set_height(list_row, Length::points(9.0))
        .expect("set list row height");
    tree.set_list_component_type(list_row, Some(ListComponentType::ListRow))
        .expect("set list row component type");

    tree.append_child(root, first_regular)
        .expect("append first regular");
    tree.append_child(root, second_regular)
        .expect("append second regular");
    tree.append_child(root, explicit_default)
        .expect("append explicit default");
    tree.append_child(root, header).expect("append header");
    tree.append_child(root, footer).expect("append footer");
    tree.append_child(root, list_row).expect("append list row");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(150.0),
            SideConstraint::indefinite(),
        ),
        Direction::Ltr,
    )
    .expect("layout linear/list tree");

    StandalonePublicLinearListLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
#[derive(Clone, Copy)]
enum PublicListGapVariant {
    Points,
    Percent,
    Calc,
    ValueAuto,
    ValueFr,
    ValueMaxContent,
    ValueFitContent,
}

#[cfg(feature = "native-standalone")]
fn rust_public_list_gap_layout_snapshots() -> Vec<StandalonePublicLinearListLayoutSnapshot> {
    [
        PublicListGapVariant::Points,
        PublicListGapVariant::Percent,
        PublicListGapVariant::Calc,
        PublicListGapVariant::ValueAuto,
        PublicListGapVariant::ValueFr,
        PublicListGapVariant::ValueMaxContent,
        PublicListGapVariant::ValueFitContent,
    ]
    .iter()
    .copied()
    .map(rust_public_list_gap_layout_snapshot)
    .collect()
}

#[cfg(feature = "native-standalone")]
fn rust_public_list_gap_layout_snapshot(
    variant: PublicListGapVariant,
) -> StandalonePublicLinearListLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let fourth = tree.create_default_node();
    let nodes = [root, first, second, third, fourth];

    tree.set_display(root, Display::Linear)
        .expect("set root display");
    tree.set_linear_orientation(root, LinearOrientation::Vertical)
        .expect("set root linear orientation");
    tree.set_width(root, Length::points(200.0))
        .expect("set root width");
    tree.set_linear_column_count(root, Some(2))
        .expect("set root column count");
    apply_rust_public_list_gap_variant(&mut tree, root, variant);

    for (index, (node, height)) in [(first, 10.0), (second, 20.0), (third, 30.0), (fourth, 40.0)]
        .into_iter()
        .enumerate()
    {
        tree.set_display(node, Display::Block)
            .expect("set child display");
        tree.set_width(node, Length::Auto)
            .expect("set child auto width");
        tree.set_height(node, Length::points(height))
            .expect("set child height");
        tree.append_child(root, node)
            .unwrap_or_else(|_| panic!("append list-gap child {index}"));
    }

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(200.0),
            SideConstraint::indefinite(),
        ),
        Direction::Ltr,
    )
    .expect("layout list-gap tree");

    StandalonePublicLinearListLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn apply_rust_public_list_gap_variant(
    tree: &mut StandaloneTree,
    root: NodeId,
    variant: PublicListGapVariant,
) {
    let (main_gap, cross_gap) = match variant {
        PublicListGapVariant::Points => (Length::points(4.0), Length::points(12.0)),
        PublicListGapVariant::Percent => (Length::percent(5.0), Length::percent(10.0)),
        PublicListGapVariant::Calc => (Length::calc(6.0, 50.0), Length::calc(12.0, 50.0)),
        PublicListGapVariant::ValueAuto => (Length::Auto, Length::Auto),
        PublicListGapVariant::ValueFr => (Length::fr(3.0), Length::fr(12.0)),
        PublicListGapVariant::ValueMaxContent => (Length::max_content(), Length::max_content()),
        PublicListGapVariant::ValueFitContent => (
            Length::fit_content(Some(BaseLength::fixed_and_percent(7.0, 8.0))),
            Length::fit_content(Some(BaseLength::fixed_and_percent(14.0, 16.0))),
        ),
    };

    tree.set_list_main_axis_gap(root, main_gap)
        .expect("set list main-axis gap");
    tree.set_list_cross_axis_gap(root, cross_gap)
        .expect("set list cross-axis gap");
}

#[cfg(feature = "native-standalone")]
fn rust_public_grid_track_layout_snapshot() -> StandalonePublicGridTrackLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let explicit_intrinsic = tree.create_default_node();
    let explicit_following = tree.create_default_node();
    let implicit_intrinsic = tree.create_default_node();
    let implicit_following = tree.create_default_node();
    let nodes = [
        root,
        explicit_intrinsic,
        explicit_following,
        implicit_intrinsic,
        implicit_following,
    ];

    tree.set_display(root, Display::Grid)
        .expect("set root display");
    tree.set_width(root, Length::points(120.0))
        .expect("set root width");
    tree.set_height(root, Length::points(120.0))
        .expect("set root height");
    tree.set_gap(root, StandaloneGap::Column, Length::points(3.0))
        .expect("set root column gap");
    tree.set_gap(root, StandaloneGap::Row, Length::points(2.0))
        .expect("set root row gap");
    tree.set_justify_items(root, JustifyItems::Start)
        .expect("set root justify items");
    tree.set_align_items(root, AlignItems::FlexStart)
        .expect("set root align items");
    tree.set_grid_auto_flow(root, GridAutoFlow::ColumnDense)
        .expect("set root grid auto flow");
    tree.set_grid_template_columns(root, [Length::points(20.0), Length::points(10.0)])
        .expect("set root grid columns");
    tree.set_grid_template_columns_max(
        root,
        [
            Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
            Length::points(10.0),
        ],
    )
    .expect("set root grid column max tracks");
    tree.set_grid_template_rows(root, [Length::points(20.0), Length::points(10.0)])
        .expect("set root grid rows");
    tree.set_grid_template_rows_max(
        root,
        [
            Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
            Length::points(10.0),
        ],
    )
    .expect("set root grid row max tracks");
    tree.set_grid_auto_columns(root, [Length::points(20.0), Length::points(10.0)])
        .expect("set root auto columns");
    tree.set_grid_auto_columns_max(
        root,
        [
            Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
            Length::points(50.0),
        ],
    )
    .expect("set root auto column max tracks");
    tree.set_grid_auto_rows(root, [Length::points(20.0), Length::points(10.0)])
        .expect("set root auto rows");
    tree.set_grid_auto_rows_max(
        root,
        [
            Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 50.0))),
            Length::points(50.0),
        ],
    )
    .expect("set root auto row max tracks");

    tree.set_display(explicit_intrinsic, Display::Block)
        .expect("set explicit intrinsic display");
    tree.set_width(explicit_intrinsic, Length::points(90.0))
        .expect("set explicit intrinsic width");
    tree.set_height(explicit_intrinsic, Length::points(90.0))
        .expect("set explicit intrinsic height");
    tree.set_grid_column_start(explicit_intrinsic, Some(1))
        .expect("set explicit intrinsic column");
    tree.set_grid_row_start(explicit_intrinsic, Some(1))
        .expect("set explicit intrinsic row");

    tree.set_display(explicit_following, Display::Block)
        .expect("set explicit following display");
    tree.set_width(explicit_following, Length::points(8.0))
        .expect("set explicit following width");
    tree.set_height(explicit_following, Length::points(8.0))
        .expect("set explicit following height");
    tree.set_grid_column_start(explicit_following, Some(2))
        .expect("set explicit following column");
    tree.set_grid_row_start(explicit_following, Some(2))
        .expect("set explicit following row");

    tree.set_display(implicit_intrinsic, Display::Block)
        .expect("set implicit intrinsic display");
    tree.set_width(implicit_intrinsic, Length::points(90.0))
        .expect("set implicit intrinsic width");
    tree.set_height(implicit_intrinsic, Length::points(90.0))
        .expect("set implicit intrinsic height");
    tree.set_grid_column_start(implicit_intrinsic, Some(3))
        .expect("set implicit intrinsic column");
    tree.set_grid_row_start(implicit_intrinsic, Some(3))
        .expect("set implicit intrinsic row");

    tree.set_display(implicit_following, Display::Block)
        .expect("set implicit following display");
    tree.set_width(implicit_following, Length::points(8.0))
        .expect("set implicit following width");
    tree.set_height(implicit_following, Length::points(8.0))
        .expect("set implicit following height");
    tree.set_grid_column_start(implicit_following, Some(4))
        .expect("set implicit following column");
    tree.set_grid_row_start(implicit_following, Some(4))
        .expect("set implicit following row");

    tree.append_child(root, explicit_intrinsic)
        .expect("append explicit intrinsic");
    tree.append_child(root, explicit_following)
        .expect("append explicit following");
    tree.append_child(root, implicit_intrinsic)
        .expect("append implicit intrinsic");
    tree.append_child(root, implicit_following)
        .expect("append implicit following");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::definite(120.0, 120.0),
        Direction::Ltr,
    )
    .expect("layout grid track tree");

    StandalonePublicGridTrackLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_grid_auto_flow_layout_snapshots() -> Vec<StandalonePublicGridTrackLayoutSnapshot> {
    [
        GridAutoFlow::Row,
        GridAutoFlow::Column,
        GridAutoFlow::Dense,
        GridAutoFlow::RowDense,
        GridAutoFlow::ColumnDense,
    ]
    .into_iter()
    .map(rust_public_grid_auto_flow_layout_snapshot)
    .collect()
}

#[cfg(feature = "native-standalone")]
fn rust_public_grid_auto_flow_layout_snapshot(
    auto_flow: GridAutoFlow,
) -> StandalonePublicGridTrackLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let third = tree.create_default_node();
    let fourth = tree.create_default_node();
    let fifth = tree.create_default_node();
    let nodes = [root, first, second, third, fourth, fifth];
    let fixed_tracks = [
        Length::points(10.0),
        Length::points(10.0),
        Length::points(10.0),
    ];

    tree.set_display(root, Display::Grid)
        .expect("set root display");
    tree.set_grid_auto_flow(root, auto_flow)
        .expect("set root grid auto flow");
    tree.set_width(root, Length::points(42.0))
        .expect("set root width");
    tree.set_height(root, Length::points(40.0))
        .expect("set root height");
    tree.set_gap(root, StandaloneGap::Column, Length::points(2.0))
        .expect("set root column gap");
    tree.set_gap(root, StandaloneGap::Row, Length::points(1.0))
        .expect("set root row gap");
    tree.set_justify_items(root, JustifyItems::Start)
        .expect("set root justify items");
    tree.set_align_items(root, AlignItems::FlexStart)
        .expect("set root align items");
    tree.set_grid_template_columns(root, fixed_tracks)
        .expect("set root grid columns");
    tree.set_grid_template_columns_max(root, fixed_tracks)
        .expect("set root grid column max tracks");
    tree.set_grid_template_rows(root, fixed_tracks)
        .expect("set root grid rows");
    tree.set_grid_template_rows_max(root, fixed_tracks)
        .expect("set root grid row max tracks");
    tree.set_grid_auto_columns(root, fixed_tracks)
        .expect("set root auto columns");
    tree.set_grid_auto_columns_max(root, fixed_tracks)
        .expect("set root auto column max tracks");
    tree.set_grid_auto_rows(root, fixed_tracks)
        .expect("set root auto rows");
    tree.set_grid_auto_rows_max(root, fixed_tracks)
        .expect("set root auto row max tracks");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_width(first, Length::points(6.0))
        .expect("set first width");
    tree.set_height(first, Length::points(6.0))
        .expect("set first height");
    tree.set_grid_column_span(first, 2)
        .expect("set first column span");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_width(second, Length::points(7.0))
        .expect("set second width");
    tree.set_height(second, Length::points(7.0))
        .expect("set second height");
    tree.set_grid_row_span(second, 2)
        .expect("set second row span");

    tree.set_display(third, Display::Block)
        .expect("set third display");
    tree.set_width(third, Length::points(5.0))
        .expect("set third width");
    tree.set_height(third, Length::points(5.0))
        .expect("set third height");
    tree.set_grid_column_span(third, 2)
        .expect("set third column span");

    tree.set_display(fourth, Display::Block)
        .expect("set fourth display");
    tree.set_width(fourth, Length::points(4.0))
        .expect("set fourth width");
    tree.set_height(fourth, Length::points(8.0))
        .expect("set fourth height");
    tree.set_grid_row_span(fourth, 2)
        .expect("set fourth row span");

    tree.set_display(fifth, Display::Block)
        .expect("set fifth display");
    tree.set_width(fifth, Length::points(3.0))
        .expect("set fifth width");
    tree.set_height(fifth, Length::points(3.0))
        .expect("set fifth height");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");
    tree.append_child(root, third).expect("append third");
    tree.append_child(root, fourth).expect("append fourth");
    tree.append_child(root, fifth).expect("append fifth");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::definite(42.0, 40.0),
        Direction::Ltr,
    )
    .expect("layout grid auto-flow tree");

    StandalonePublicGridTrackLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_grid_alignment_layout_snapshot() -> StandalonePublicGridAlignmentLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let inherited = tree.create_default_node();
    let self_end = tree.create_default_node();
    let self_stretch = tree.create_default_node();
    let nodes = [root, inherited, self_end, self_stretch];

    tree.set_display(root, Display::Grid)
        .expect("set root display");
    tree.set_width(root, Length::points(120.0))
        .expect("set root width");
    tree.set_height(root, Length::points(100.0))
        .expect("set root height");
    tree.set_gap(root, StandaloneGap::Column, Length::points(2.0))
        .expect("set root column gap");
    tree.set_gap(root, StandaloneGap::Row, Length::points(4.0))
        .expect("set root row gap");
    tree.set_justify_content(root, JustifyContent::Center)
        .expect("set root justify content");
    tree.set_align_content(root, AlignContent::Center)
        .expect("set root align content");
    tree.set_justify_items(root, JustifyItems::Center)
        .expect("set root justify items");
    tree.set_align_items(root, AlignItems::Center)
        .expect("set root align items");
    tree.set_grid_template_columns(root, [Length::points(20.0), Length::points(20.0)])
        .expect("set root grid columns");
    tree.set_grid_template_columns_max(root, [Length::points(20.0), Length::points(20.0)])
        .expect("set root grid column max tracks");
    tree.set_grid_template_rows(root, [Length::points(20.0), Length::points(20.0)])
        .expect("set root grid rows");
    tree.set_grid_template_rows_max(root, [Length::points(20.0), Length::points(20.0)])
        .expect("set root grid row max tracks");

    tree.set_display(inherited, Display::Block)
        .expect("set inherited display");
    tree.set_width(inherited, Length::points(10.0))
        .expect("set inherited width");
    tree.set_height(inherited, Length::points(8.0))
        .expect("set inherited height");
    tree.set_grid_column_start(inherited, Some(1))
        .expect("set inherited column");
    tree.set_grid_row_start(inherited, Some(1))
        .expect("set inherited row");

    tree.set_display(self_end, Display::Block)
        .expect("set self-end display");
    tree.set_width(self_end, Length::points(8.0))
        .expect("set self-end width");
    tree.set_height(self_end, Length::points(6.0))
        .expect("set self-end height");
    tree.set_justify_self(self_end, JustifyItems::End)
        .expect("set self-end justify self");
    tree.set_align_self(self_end, Some(AlignItems::FlexEnd))
        .expect("set self-end align self");
    tree.set_grid_column_start(self_end, Some(2))
        .expect("set self-end column");
    tree.set_grid_row_start(self_end, Some(1))
        .expect("set self-end row");

    tree.set_display(self_stretch, Display::Block)
        .expect("set self-stretch display");
    tree.set_justify_self(self_stretch, JustifyItems::Stretch)
        .expect("set self-stretch justify self");
    tree.set_align_self(self_stretch, Some(AlignItems::Stretch))
        .expect("set self-stretch align self");
    tree.set_grid_column_start(self_stretch, Some(1))
        .expect("set self-stretch column");
    tree.set_grid_row_start(self_stretch, Some(2))
        .expect("set self-stretch row");

    tree.append_child(root, inherited)
        .expect("append inherited");
    tree.append_child(root, self_end).expect("append self-end");
    tree.append_child(root, self_stretch)
        .expect("append self-stretch");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::definite(120.0, 100.0),
        Direction::Ltr,
    )
    .expect("layout grid alignment tree");

    StandalonePublicGridAlignmentLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_grid_alignment_variant_layout_snapshots(
) -> Vec<StandalonePublicGridAlignmentLayoutSnapshot> {
    let mut snapshots = Vec::new();
    for value in [
        JustifyContent::FlexStart,
        JustifyContent::Center,
        JustifyContent::FlexEnd,
        JustifyContent::SpaceBetween,
        JustifyContent::SpaceAround,
        JustifyContent::SpaceEvenly,
        JustifyContent::Stretch,
        JustifyContent::Start,
        JustifyContent::End,
    ] {
        snapshots.push(rust_public_grid_alignment_variant_layout_snapshot(
            PublicGridAlignmentVariant::JustifyContent(value),
        ));
    }
    for value in [
        AlignContent::FlexStart,
        AlignContent::Center,
        AlignContent::FlexEnd,
        AlignContent::Stretch,
        AlignContent::SpaceBetween,
        AlignContent::SpaceAround,
        AlignContent::SpaceEvenly,
    ] {
        snapshots.push(rust_public_grid_alignment_variant_layout_snapshot(
            PublicGridAlignmentVariant::AlignContent(value),
        ));
    }
    for value in [
        JustifyItems::Auto,
        JustifyItems::Stretch,
        JustifyItems::Start,
        JustifyItems::End,
        JustifyItems::Center,
    ] {
        snapshots.push(rust_public_grid_alignment_variant_layout_snapshot(
            PublicGridAlignmentVariant::JustifyItems(value),
        ));
    }
    for value in [
        AlignItems::Stretch,
        AlignItems::FlexStart,
        AlignItems::FlexEnd,
        AlignItems::Center,
        AlignItems::Baseline,
        AlignItems::Start,
        AlignItems::End,
    ] {
        snapshots.push(rust_public_grid_alignment_variant_layout_snapshot(
            PublicGridAlignmentVariant::AlignItems(value),
        ));
    }
    for value in [
        JustifyItems::Auto,
        JustifyItems::Stretch,
        JustifyItems::Start,
        JustifyItems::End,
        JustifyItems::Center,
    ] {
        snapshots.push(rust_public_grid_alignment_variant_layout_snapshot(
            PublicGridAlignmentVariant::JustifySelf(value),
        ));
    }
    for value in [
        None,
        Some(AlignItems::Stretch),
        Some(AlignItems::FlexStart),
        Some(AlignItems::FlexEnd),
        Some(AlignItems::Center),
        Some(AlignItems::Baseline),
        Some(AlignItems::Start),
        Some(AlignItems::End),
    ] {
        snapshots.push(rust_public_grid_alignment_variant_layout_snapshot(
            PublicGridAlignmentVariant::AlignSelf(value),
        ));
    }
    snapshots
}

#[cfg(feature = "native-standalone")]
#[derive(Clone, Copy)]
enum PublicGridAlignmentVariant {
    JustifyContent(JustifyContent),
    AlignContent(AlignContent),
    JustifyItems(JustifyItems),
    AlignItems(AlignItems),
    JustifySelf(JustifyItems),
    AlignSelf(Option<AlignItems>),
}

#[cfg(feature = "native-standalone")]
fn rust_public_grid_alignment_variant_layout_snapshot(
    variant: PublicGridAlignmentVariant,
) -> StandalonePublicGridAlignmentLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let nodes = [root, first, second];

    tree.set_display(root, Display::Grid)
        .expect("set root display");
    tree.set_width(root, Length::points(120.0))
        .expect("set root width");
    tree.set_height(root, Length::points(100.0))
        .expect("set root height");
    tree.set_gap(root, StandaloneGap::Column, Length::points(2.0))
        .expect("set root column gap");
    tree.set_gap(root, StandaloneGap::Row, Length::points(4.0))
        .expect("set root row gap");
    tree.set_justify_content(root, JustifyContent::FlexStart)
        .expect("set root justify content");
    tree.set_align_content(root, AlignContent::FlexStart)
        .expect("set root align content");
    tree.set_justify_items(root, JustifyItems::Start)
        .expect("set root justify items");
    tree.set_align_items(root, AlignItems::FlexStart)
        .expect("set root align items");
    tree.set_grid_template_columns(root, [Length::points(30.0), Length::points(30.0)])
        .expect("set root grid columns");
    tree.set_grid_template_columns_max(root, [Length::points(30.0), Length::points(30.0)])
        .expect("set root grid column max tracks");
    tree.set_grid_template_rows(root, [Length::points(24.0), Length::points(24.0)])
        .expect("set root grid rows");
    tree.set_grid_template_rows_max(root, [Length::points(24.0), Length::points(24.0)])
        .expect("set root grid row max tracks");

    let mut first_has_width = true;
    let mut second_has_width = true;
    let mut first_has_height = true;
    let mut second_has_height = true;

    match variant {
        PublicGridAlignmentVariant::JustifyContent(value) => tree
            .set_justify_content(root, value)
            .expect("set variant justify content"),
        PublicGridAlignmentVariant::AlignContent(value) => tree
            .set_align_content(root, value)
            .expect("set variant align content"),
        PublicGridAlignmentVariant::JustifyItems(value) => {
            tree.set_justify_items(root, value)
                .expect("set variant justify items");
            first_has_width = value != JustifyItems::Auto && value != JustifyItems::Stretch;
            second_has_width = first_has_width;
        }
        PublicGridAlignmentVariant::AlignItems(value) => {
            tree.set_align_items(root, value)
                .expect("set variant align items");
            first_has_height = value != AlignItems::Stretch;
            second_has_height = first_has_height;
        }
        PublicGridAlignmentVariant::JustifySelf(value) => {
            tree.set_justify_items(root, JustifyItems::End)
                .expect("set inherited justify items");
            tree.set_justify_self(first, value)
                .expect("set variant justify self");
            first_has_width = value != JustifyItems::Stretch;
        }
        PublicGridAlignmentVariant::AlignSelf(value) => {
            tree.set_align_items(root, AlignItems::FlexEnd)
                .expect("set inherited align items");
            tree.set_align_self(first, value)
                .expect("set variant align self");
            first_has_height = value != Some(AlignItems::Stretch);
        }
    }

    tree.set_display(first, Display::Block)
        .expect("set first display");
    if first_has_width {
        tree.set_width(first, Length::points(10.0))
            .expect("set first width");
    }
    if first_has_height {
        tree.set_height(first, Length::points(8.0))
            .expect("set first height");
    }
    tree.set_grid_column_start(first, Some(1))
        .expect("set first column");
    tree.set_grid_row_start(first, Some(1))
        .expect("set first row");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    if second_has_width {
        tree.set_width(second, Length::points(12.0))
            .expect("set second width");
    }
    if second_has_height {
        tree.set_height(second, Length::points(9.0))
            .expect("set second height");
    }
    tree.set_grid_column_start(second, Some(2))
        .expect("set second column");
    tree.set_grid_row_start(second, Some(2))
        .expect("set second row");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::definite(120.0, 100.0),
        Direction::Ltr,
    )
    .expect("layout grid alignment variant tree");

    StandalonePublicGridAlignmentLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn snapshot_public_layout_node(
    tree: &StandaloneTree,
    node: NodeId,
) -> StandalonePublicLayoutNodeSnapshot {
    const EDGES: [StandaloneEdge; 6] = [
        StandaloneEdge::Left,
        StandaloneEdge::Right,
        StandaloneEdge::Top,
        StandaloneEdge::Bottom,
        StandaloneEdge::Start,
        StandaloneEdge::End,
    ];

    StandalonePublicLayoutNodeSnapshot {
        left: tree.layout_left(node).expect("layout left"),
        top: tree.layout_top(node).expect("layout top"),
        width: tree.layout_width(node).expect("layout width"),
        height: tree.layout_height(node).expect("layout height"),
        baseline: tree.layout_baseline(node).expect("layout baseline"),
        margin: EDGES
            .iter()
            .map(|edge| tree.layout_margin(node, *edge).expect("layout margin"))
            .collect(),
        padding: EDGES
            .iter()
            .map(|edge| tree.layout_padding(node, *edge).expect("layout padding"))
            .collect(),
        border: EDGES
            .iter()
            .map(|edge| tree.layout_border(node, *edge).expect("layout border"))
            .collect(),
        sticky_position: EDGES
            .iter()
            .map(|edge| {
                tree.layout_sticky_position(node, *edge)
                    .expect("layout sticky position")
            })
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_dimension_style_snapshot() -> StandalonePublicDimensionStyleSnapshot {
    let mut tree = StandaloneTree::new();
    let node = tree.create_default_node();

    tree.set_flex_basis(node, Length::percent(12.5))
        .expect("set flex basis");
    tree.set_width(node, Length::Auto).expect("set width");
    tree.set_height(node, Length::calc(30.0, 45.0))
        .expect("set height");
    tree.set_min_width(node, Length::max_content())
        .expect("set min width");
    tree.set_max_width(
        node,
        Length::fit_content(Some(BaseLength::fixed_and_percent(10.0, 25.0))),
    )
    .expect("set max width");
    tree.set_min_height(node, Length::points(7.0))
        .expect("set min height");
    tree.set_max_height(node, Length::fit_content(None))
        .expect("set max height");

    snapshot_public_dimension_style(&tree, node)
}

#[cfg(feature = "native-standalone")]
#[derive(Clone, Copy)]
enum PublicDimensionStyleVariant {
    Points,
    Percent,
    Calc,
    ValueFr,
    Auto,
    MaxContent,
    FitContent,
    FitContentValue,
}

#[cfg(feature = "native-standalone")]
fn rust_public_dimension_style_variant_snapshots() -> Vec<StandalonePublicDimensionStyleSnapshot> {
    [
        PublicDimensionStyleVariant::Points,
        PublicDimensionStyleVariant::Percent,
        PublicDimensionStyleVariant::Calc,
        PublicDimensionStyleVariant::ValueFr,
        PublicDimensionStyleVariant::Auto,
        PublicDimensionStyleVariant::MaxContent,
        PublicDimensionStyleVariant::FitContent,
        PublicDimensionStyleVariant::FitContentValue,
    ]
    .iter()
    .copied()
    .map(|variant| {
        let mut tree = StandaloneTree::new();
        let node = tree.create_default_node();
        apply_rust_public_dimension_style_variant(&mut tree, node, variant);
        snapshot_public_dimension_style(&tree, node)
    })
    .collect()
}

#[cfg(feature = "native-standalone")]
fn snapshot_public_dimension_style(
    tree: &StandaloneTree,
    node: NodeId,
) -> StandalonePublicDimensionStyleSnapshot {
    StandalonePublicDimensionStyleSnapshot {
        flex_basis: length_value(tree.style_flex_basis(node).expect("flex basis")),
        width: length_value(tree.style_width(node).expect("width")),
        height: length_value(tree.style_height(node).expect("height")),
        min_width: length_value(tree.style_min_width(node).expect("min width")),
        max_width: length_value(tree.style_max_width(node).expect("max width")),
        min_height: length_value(tree.style_min_height(node).expect("min height")),
        max_height: length_value(tree.style_max_height(node).expect("max height")),
        dirty: tree.is_dirty(node).expect("dirty state"),
    }
}

#[cfg(feature = "native-standalone")]
fn apply_rust_public_dimension_style_variant(
    tree: &mut StandaloneTree,
    node: NodeId,
    variant: PublicDimensionStyleVariant,
) {
    match variant {
        PublicDimensionStyleVariant::Points => {
            tree.set_flex_basis(node, Length::points(11.0))
                .expect("set flex basis point");
            tree.set_width(node, Length::points(21.0))
                .expect("set width point");
            tree.set_height(node, Length::points(31.0))
                .expect("set height point");
            tree.set_min_width(node, Length::points(4.0))
                .expect("set min width point");
            tree.set_max_width(node, Length::points(41.0))
                .expect("set max width point");
            tree.set_min_height(node, Length::points(5.0))
                .expect("set min height point");
            tree.set_max_height(node, Length::points(51.0))
                .expect("set max height point");
        }
        PublicDimensionStyleVariant::Percent => {
            tree.set_flex_basis(node, Length::percent(12.0))
                .expect("set flex basis percent");
            tree.set_width(node, Length::percent(22.0))
                .expect("set width percent");
            tree.set_height(node, Length::percent(32.0))
                .expect("set height percent");
            tree.set_min_width(node, Length::percent(6.0))
                .expect("set min width percent");
            tree.set_max_width(node, Length::percent(42.0))
                .expect("set max width percent");
            tree.set_min_height(node, Length::percent(7.0))
                .expect("set min height percent");
            tree.set_max_height(node, Length::percent(52.0))
                .expect("set max height percent");
        }
        PublicDimensionStyleVariant::Calc => {
            tree.set_flex_basis(node, Length::calc(13.0, 14.0))
                .expect("set flex basis calc");
            tree.set_width(node, Length::calc(23.0, 24.0))
                .expect("set width calc");
            tree.set_height(node, Length::calc(33.0, 34.0))
                .expect("set height calc");
            tree.set_min_width(node, Length::calc(8.0, 9.0))
                .expect("set min width calc");
            tree.set_max_width(node, Length::calc(43.0, 44.0))
                .expect("set max width calc");
            tree.set_min_height(node, Length::calc(10.0, 11.0))
                .expect("set min height calc");
            tree.set_max_height(node, Length::calc(53.0, 54.0))
                .expect("set max height calc");
        }
        PublicDimensionStyleVariant::ValueFr => {
            tree.set_flex_basis(node, Length::fr(1.25))
                .expect("set flex basis fr");
            tree.set_width(node, Length::fr(2.25))
                .expect("set width fr");
            tree.set_height(node, Length::fr(3.25))
                .expect("set height fr");
            tree.set_min_width(node, Length::fr(4.25))
                .expect("set min width fr");
            tree.set_max_width(node, Length::fr(5.25))
                .expect("set max width fr");
            tree.set_min_height(node, Length::fr(6.25))
                .expect("set min height fr");
            tree.set_max_height(node, Length::fr(7.25))
                .expect("set max height fr");
        }
        PublicDimensionStyleVariant::Auto => {
            tree.set_flex_basis(node, Length::Auto)
                .expect("set flex basis auto");
            tree.set_width(node, Length::Auto).expect("set width auto");
            tree.set_height(node, Length::Auto)
                .expect("set height auto");
            tree.set_min_width(node, Length::points(6.0))
                .expect("set min width point for auto case");
            tree.set_max_width(node, Length::points(46.0))
                .expect("set max width point for auto case");
            tree.set_min_height(node, Length::points(7.0))
                .expect("set min height point for auto case");
            tree.set_max_height(node, Length::points(57.0))
                .expect("set max height point for auto case");
        }
        PublicDimensionStyleVariant::MaxContent => {
            tree.set_flex_basis(node, Length::max_content())
                .expect("set flex basis max-content");
            tree.set_width(node, Length::max_content())
                .expect("set width max-content");
            tree.set_height(node, Length::max_content())
                .expect("set height max-content");
            tree.set_min_width(node, Length::max_content())
                .expect("set min width max-content");
            tree.set_max_width(node, Length::max_content())
                .expect("set max width max-content");
            tree.set_min_height(node, Length::max_content())
                .expect("set min height max-content");
            tree.set_max_height(node, Length::max_content())
                .expect("set max height max-content");
        }
        PublicDimensionStyleVariant::FitContent => {
            tree.set_flex_basis(node, Length::fit_content(None))
                .expect("set flex basis fit-content");
            tree.set_width(node, Length::fit_content(None))
                .expect("set width fit-content");
            tree.set_height(node, Length::fit_content(None))
                .expect("set height fit-content");
            tree.set_min_width(node, Length::fit_content(None))
                .expect("set min width fit-content");
            tree.set_max_width(node, Length::fit_content(None))
                .expect("set max width fit-content");
            tree.set_min_height(node, Length::fit_content(None))
                .expect("set min height fit-content");
            tree.set_max_height(node, Length::fit_content(None))
                .expect("set max height fit-content");
        }
        PublicDimensionStyleVariant::FitContentValue => {
            tree.set_flex_basis(
                node,
                Length::fit_content(Some(BaseLength::fixed_and_percent(15.0, 16.0))),
            )
            .expect("set flex basis fit-content value");
            tree.set_width(
                node,
                Length::fit_content(Some(BaseLength::fixed_and_percent(25.0, 26.0))),
            )
            .expect("set width fit-content value");
            tree.set_height(
                node,
                Length::fit_content(Some(BaseLength::fixed_and_percent(35.0, 36.0))),
            )
            .expect("set height fit-content value");
            tree.set_min_width(
                node,
                Length::fit_content(Some(BaseLength::fixed_and_percent(17.0, 18.0))),
            )
            .expect("set min width fit-content value");
            tree.set_max_width(
                node,
                Length::fit_content(Some(BaseLength::fixed_and_percent(45.0, 46.0))),
            )
            .expect("set max width fit-content value");
            tree.set_min_height(
                node,
                Length::fit_content(Some(BaseLength::fixed_and_percent(19.0, 20.0))),
            )
            .expect("set min height fit-content value");
            tree.set_max_height(
                node,
                Length::fit_content(Some(BaseLength::fixed_and_percent(55.0, 56.0))),
            )
            .expect("set max height fit-content value");
        }
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_dimension_layout_snapshots() -> Vec<StandalonePublicDimensionLayoutSnapshot> {
    [
        PublicDimensionStyleVariant::Points,
        PublicDimensionStyleVariant::Percent,
        PublicDimensionStyleVariant::Calc,
        PublicDimensionStyleVariant::ValueFr,
        PublicDimensionStyleVariant::Auto,
        PublicDimensionStyleVariant::MaxContent,
        PublicDimensionStyleVariant::FitContent,
        PublicDimensionStyleVariant::FitContentValue,
    ]
    .iter()
    .copied()
    .map(rust_public_dimension_layout_snapshot)
    .collect()
}

#[cfg(feature = "native-standalone")]
fn rust_public_dimension_layout_snapshot(
    variant: PublicDimensionStyleVariant,
) -> StandalonePublicDimensionLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let basis = tree.create_default_measured_node(Size::new(58.0, 17.0));
    let sized = tree.create_default_measured_node(Size::new(58.0, 17.0));
    let clamped = tree.create_default_measured_node(Size::new(58.0, 17.0));
    let trailing = tree.create_default_node();
    let nodes = [root, basis, sized, clamped, trailing];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_flex_wrap(root, FlexWrap::NoWrap)
        .expect("set root flex wrap");
    tree.set_align_items(root, AlignItems::FlexStart)
        .expect("set root align items");
    tree.set_width(root, Length::points(200.0))
        .expect("set root width");
    tree.set_height(root, Length::points(100.0))
        .expect("set root height");

    tree.set_display(basis, Display::Block)
        .expect("set basis display");
    tree.set_height(basis, Length::points(10.0))
        .expect("set basis height");
    apply_rust_public_dimension_layout_basis_variant(&mut tree, basis, variant);

    tree.set_display(sized, Display::Block)
        .expect("set sized display");
    apply_rust_public_dimension_layout_size_variant(&mut tree, sized, variant);

    tree.set_display(clamped, Display::Block)
        .expect("set clamped display");
    tree.set_width(clamped, Length::points(30.0))
        .expect("set clamped width");
    tree.set_height(clamped, Length::points(12.0))
        .expect("set clamped height");
    apply_rust_public_dimension_layout_clamp_variant(&mut tree, clamped, variant);

    tree.set_display(trailing, Display::Block)
        .expect("set trailing display");
    tree.set_width(trailing, Length::points(20.0))
        .expect("set trailing width");
    tree.set_height(trailing, Length::points(12.0))
        .expect("set trailing height");

    tree.append_child(root, basis).expect("append basis");
    tree.append_child(root, sized).expect("append sized");
    tree.append_child(root, clamped).expect("append clamped");
    tree.append_child(root, trailing).expect("append trailing");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(200.0),
            SideConstraint::definite(100.0),
        ),
        Direction::Ltr,
    )
    .expect("layout dimension tree");

    StandalonePublicDimensionLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn apply_rust_public_dimension_layout_basis_variant(
    tree: &mut StandaloneTree,
    node: NodeId,
    variant: PublicDimensionStyleVariant,
) {
    let value = match variant {
        PublicDimensionStyleVariant::Points => Length::points(11.0),
        PublicDimensionStyleVariant::Percent => Length::percent(25.0),
        PublicDimensionStyleVariant::Calc => Length::calc(13.0, 10.0),
        PublicDimensionStyleVariant::ValueFr => Length::fr(30.0),
        PublicDimensionStyleVariant::Auto => Length::Auto,
        PublicDimensionStyleVariant::MaxContent => Length::max_content(),
        PublicDimensionStyleVariant::FitContent => Length::fit_content(None),
        PublicDimensionStyleVariant::FitContentValue => {
            Length::fit_content(Some(BaseLength::fixed_and_percent(45.0, 0.0)))
        }
    };
    tree.set_flex_basis(node, value)
        .expect("set layout flex basis variant");
}

#[cfg(feature = "native-standalone")]
fn apply_rust_public_dimension_layout_size_variant(
    tree: &mut StandaloneTree,
    node: NodeId,
    variant: PublicDimensionStyleVariant,
) {
    let (width, height) = match variant {
        PublicDimensionStyleVariant::Points => (Length::points(21.0), Length::points(31.0)),
        PublicDimensionStyleVariant::Percent => (Length::percent(25.0), Length::percent(20.0)),
        PublicDimensionStyleVariant::Calc => (Length::calc(23.0, 10.0), Length::calc(13.0, 10.0)),
        PublicDimensionStyleVariant::ValueFr => (Length::fr(22.0), Length::fr(18.0)),
        PublicDimensionStyleVariant::Auto => (Length::Auto, Length::Auto),
        PublicDimensionStyleVariant::MaxContent => (Length::max_content(), Length::max_content()),
        PublicDimensionStyleVariant::FitContent => {
            (Length::fit_content(None), Length::fit_content(None))
        }
        PublicDimensionStyleVariant::FitContentValue => (
            Length::fit_content(Some(BaseLength::fixed_and_percent(45.0, 0.0))),
            Length::fit_content(Some(BaseLength::fixed_and_percent(32.0, 0.0))),
        ),
    };
    tree.set_width(node, width)
        .expect("set layout width variant");
    tree.set_height(node, height)
        .expect("set layout height variant");
}

#[cfg(feature = "native-standalone")]
fn apply_rust_public_dimension_layout_clamp_variant(
    tree: &mut StandaloneTree,
    node: NodeId,
    variant: PublicDimensionStyleVariant,
) {
    let (min_width, max_width, min_height, max_height) = match variant {
        PublicDimensionStyleVariant::Points => (
            Length::points(45.0),
            Length::points(60.0),
            Length::points(18.0),
            Length::points(24.0),
        ),
        PublicDimensionStyleVariant::Percent => (
            Length::percent(20.0),
            Length::percent(60.0),
            Length::percent(20.0),
            Length::percent(50.0),
        ),
        PublicDimensionStyleVariant::Calc => (
            Length::calc(35.0, 5.0),
            Length::calc(70.0, 0.0),
            Length::calc(15.0, 5.0),
            Length::calc(30.0, 0.0),
        ),
        PublicDimensionStyleVariant::ValueFr => (
            Length::fr(44.0),
            Length::fr(60.0),
            Length::fr(18.0),
            Length::fr(25.0),
        ),
        PublicDimensionStyleVariant::Auto => (
            Length::points(6.0),
            Length::points(46.0),
            Length::points(7.0),
            Length::points(57.0),
        ),
        PublicDimensionStyleVariant::MaxContent => (
            Length::max_content(),
            Length::max_content(),
            Length::max_content(),
            Length::max_content(),
        ),
        PublicDimensionStyleVariant::FitContent => (
            Length::fit_content(None),
            Length::fit_content(None),
            Length::fit_content(None),
            Length::fit_content(None),
        ),
        PublicDimensionStyleVariant::FitContentValue => (
            Length::fit_content(Some(BaseLength::fixed_and_percent(45.0, 0.0))),
            Length::fit_content(Some(BaseLength::fixed_and_percent(55.0, 0.0))),
            Length::fit_content(Some(BaseLength::fixed_and_percent(18.0, 0.0))),
            Length::fit_content(Some(BaseLength::fixed_and_percent(22.0, 0.0))),
        ),
    };

    tree.set_min_width(node, min_width)
        .expect("set layout min-width variant");
    tree.set_max_width(node, max_width)
        .expect("set layout max-width variant");
    tree.set_min_height(node, min_height)
        .expect("set layout min-height variant");
    tree.set_max_height(node, max_height)
        .expect("set layout max-height variant");
}

#[cfg(feature = "native-standalone")]
fn rust_public_direction_snapshot() -> StandalonePublicDirectionSnapshot {
    let mut tree = StandaloneTree::new();
    let node = tree.create_default_node();

    let default_is_rtl = tree.is_rtl(node).expect("default rtl query");
    tree.set_direction(node, Direction::Rtl)
        .expect("set rtl direction");
    let rtl_is_rtl = tree.is_rtl(node).expect("rtl query");
    tree.set_direction(node, Direction::Ltr)
        .expect("set ltr direction");
    let ltr_is_rtl = tree.is_rtl(node).expect("ltr query");

    StandalonePublicDirectionSnapshot {
        default_is_rtl,
        rtl_is_rtl,
        ltr_is_rtl,
        dirty_after_direction_updates: tree.is_dirty(node).expect("dirty state"),
    }
}

#[cfg(feature = "native-standalone")]
fn rust_public_direction_layout_snapshots() -> Vec<StandalonePublicDirectionLayoutSnapshot> {
    [Direction::Ltr, Direction::Rtl]
        .into_iter()
        .map(rust_public_direction_layout_snapshot)
        .collect()
}

#[cfg(feature = "native-standalone")]
fn rust_public_direction_layout_snapshot(
    direction: Direction,
) -> StandalonePublicDirectionLayoutSnapshot {
    let mut tree = StandaloneTree::new();
    let root = tree.create_default_node();
    let first = tree.create_default_node();
    let second = tree.create_default_node();
    let nodes = [root, first, second];

    tree.set_display(root, Display::Flex)
        .expect("set root display");
    tree.set_direction(root, direction)
        .expect("set root direction");
    tree.set_flex_direction(root, FlexDirection::Row)
        .expect("set root flex direction");
    tree.set_justify_content(root, JustifyContent::FlexStart)
        .expect("set root justify content");
    tree.set_align_items(root, AlignItems::FlexStart)
        .expect("set root align items");
    tree.set_width(root, Length::points(100.0))
        .expect("set root width");
    tree.set_height(root, Length::points(40.0))
        .expect("set root height");

    tree.set_display(first, Display::Block)
        .expect("set first display");
    tree.set_width(first, Length::points(10.0))
        .expect("set first width");
    tree.set_height(first, Length::points(10.0))
        .expect("set first height");
    tree.set_margin(first, StandaloneEdge::Start, Length::points(7.0))
        .expect("set first start margin");
    tree.set_margin(first, StandaloneEdge::End, Length::points(3.0))
        .expect("set first end margin");

    tree.set_display(second, Display::Block)
        .expect("set second display");
    tree.set_width(second, Length::points(20.0))
        .expect("set second width");
    tree.set_height(second, Length::points(10.0))
        .expect("set second height");

    tree.append_child(root, first).expect("append first");
    tree.append_child(root, second).expect("append second");

    tree.calculate_layout_with_mode(
        root,
        starlight_layout::Constraints::new(
            SideConstraint::definite(100.0),
            SideConstraint::definite(40.0),
        ),
        Direction::Ltr,
    )
    .expect("layout direction tree");

    StandalonePublicDirectionLayoutSnapshot {
        nodes: nodes
            .iter()
            .map(|node| snapshot_public_layout_node(&tree, *node))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn snapshot_public_edge_style_stage(
    stage: StandalonePublicStyleStage,
    tree: &StandaloneTree,
    node: NodeId,
) -> StandalonePublicEdgeStyleSnapshot {
    const EDGES: [StandaloneEdge; 9] = [
        StandaloneEdge::Left,
        StandaloneEdge::Right,
        StandaloneEdge::Top,
        StandaloneEdge::Bottom,
        StandaloneEdge::Start,
        StandaloneEdge::End,
        StandaloneEdge::Horizontal,
        StandaloneEdge::Vertical,
        StandaloneEdge::All,
    ];
    const GAPS: [StandaloneGap; 3] = [
        StandaloneGap::Column,
        StandaloneGap::Row,
        StandaloneGap::All,
    ];

    StandalonePublicEdgeStyleSnapshot {
        stage,
        position: EDGES
            .iter()
            .map(|edge| {
                style_edge_length_value(
                    *edge,
                    tree.style_position(node, *edge).expect("position style"),
                )
            })
            .collect(),
        margin: EDGES
            .iter()
            .map(|edge| {
                style_edge_length_value(
                    *edge,
                    tree.style_margin(node, *edge).expect("margin style"),
                )
            })
            .collect(),
        padding: EDGES
            .iter()
            .map(|edge| {
                style_edge_length_value(
                    *edge,
                    tree.style_padding(node, *edge).expect("padding style"),
                )
            })
            .collect(),
        border: EDGES
            .iter()
            .map(|edge| tree.style_border(node, *edge).expect("border style"))
            .collect(),
        gap: GAPS
            .iter()
            .map(|gap| length_value(tree.style_gap(node, *gap).expect("gap style")))
            .collect(),
        dirty: tree.is_dirty(node).expect("dirty state"),
    }
}

#[cfg(feature = "native-standalone")]
fn style_edge_length_value(edge: StandaloneEdge, length: Length) -> StandalonePublicLengthValue {
    match edge {
        StandaloneEdge::Horizontal | StandaloneEdge::Vertical | StandaloneEdge::All => {
            StandalonePublicLengthValue::default_point()
        }
        StandaloneEdge::Left
        | StandaloneEdge::Right
        | StandaloneEdge::Top
        | StandaloneEdge::Bottom
        | StandaloneEdge::Start
        | StandaloneEdge::End => length_value(length),
    }
}

#[cfg(feature = "native-standalone")]
fn length_value(length: Length) -> StandalonePublicLengthValue {
    match length {
        Length::Auto => StandalonePublicLengthValue::auto(),
        Length::Points(value) => StandalonePublicLengthValue::points(value),
        Length::Percent(value) => StandalonePublicLengthValue::percent(value),
        Length::Calc { fixed, percent } => StandalonePublicLengthValue::calc(fixed, percent),
        Length::Fr(value) => StandalonePublicLengthValue {
            value,
            unit: StandalonePublicLengthValue::UNIT_FR,
            percentage: 0.0,
            flags: StandalonePublicLengthValue::FLAG_HAS_VALUE,
        },
        Length::MinContent => unreachable!("min-content is rejected by standalone validation"),
        Length::MaxContent => StandalonePublicLengthValue {
            value: 0.0,
            unit: StandalonePublicLengthValue::UNIT_MAX_CONTENT,
            percentage: 0.0,
            flags: 0,
        },
        Length::FitContent(None) => StandalonePublicLengthValue {
            value: 0.0,
            unit: StandalonePublicLengthValue::UNIT_FIT_CONTENT,
            percentage: 0.0,
            flags: 0,
        },
        Length::FitContent(Some(base)) => {
            let mut flags = StandalonePublicLengthValue::FLAG_HAS_VALUE;
            if base.contains_percentage() {
                flags |= StandalonePublicLengthValue::FLAG_HAS_PERCENTAGE;
            }
            StandalonePublicLengthValue {
                value: base.fixed_part(),
                unit: StandalonePublicLengthValue::UNIT_FIT_CONTENT,
                percentage: base.percentage_part(),
                flags,
            }
        }
    }
}

#[cfg(feature = "native-standalone")]
fn display_value(value: Display) -> i32 {
    match value {
        Display::None => 0,
        Display::Flex => 1,
        Display::Grid => 2,
        Display::Linear => 3,
        Display::Relative => 4,
        Display::Block => 5,
    }
}

#[cfg(feature = "native-standalone")]
fn align_items_value(value: AlignItems) -> i32 {
    match value {
        AlignItems::Stretch => 1,
        AlignItems::FlexStart => 2,
        AlignItems::FlexEnd => 3,
        AlignItems::Center => 4,
        AlignItems::Baseline => 5,
        AlignItems::Start => 6,
        AlignItems::End => 7,
    }
}

#[cfg(feature = "native-standalone")]
fn align_content_value(value: AlignContent) -> i32 {
    match value {
        AlignContent::FlexStart | AlignContent::Start => 0,
        AlignContent::FlexEnd | AlignContent::End => 1,
        AlignContent::Center => 2,
        AlignContent::Stretch => 3,
        AlignContent::SpaceBetween => 4,
        AlignContent::SpaceAround => 5,
        AlignContent::SpaceEvenly => 6,
    }
}

#[cfg(feature = "native-standalone")]
fn justify_content_value(value: JustifyContent) -> i32 {
    match value {
        JustifyContent::FlexStart => 0,
        JustifyContent::Center => 1,
        JustifyContent::FlexEnd => 2,
        JustifyContent::SpaceBetween => 3,
        JustifyContent::SpaceAround => 4,
        JustifyContent::SpaceEvenly => 5,
        JustifyContent::Stretch => 6,
        JustifyContent::Start => 7,
        JustifyContent::End => 8,
    }
}

#[cfg(feature = "native-standalone")]
fn flex_direction_value(value: FlexDirection) -> i32 {
    match value {
        FlexDirection::Column => 0,
        FlexDirection::Row => 1,
        FlexDirection::RowReverse => 2,
        FlexDirection::ColumnReverse => 3,
    }
}

#[cfg(feature = "native-standalone")]
fn flex_wrap_value(value: FlexWrap) -> i32 {
    match value {
        FlexWrap::Wrap => 0,
        FlexWrap::NoWrap => 1,
        FlexWrap::WrapReverse => 2,
    }
}

#[cfg(feature = "native-standalone")]
fn linear_orientation_value(value: LinearOrientation) -> i32 {
    match value {
        LinearOrientation::Horizontal => 0,
        LinearOrientation::Vertical => 1,
        LinearOrientation::HorizontalReverse => 2,
        LinearOrientation::VerticalReverse => 3,
        LinearOrientation::Row => 4,
        LinearOrientation::Column => 5,
        LinearOrientation::RowReverse => 6,
        LinearOrientation::ColumnReverse => 7,
    }
}

#[cfg(feature = "native-standalone")]
fn linear_gravity_value(value: LinearGravity) -> i32 {
    match value {
        LinearGravity::None => 0,
        LinearGravity::Top => 1,
        LinearGravity::Bottom => 2,
        LinearGravity::Left => 3,
        LinearGravity::Right => 4,
        LinearGravity::CenterVertical => 5,
        LinearGravity::CenterHorizontal => 6,
        LinearGravity::SpaceBetween => 7,
        LinearGravity::Start => 8,
        LinearGravity::End => 9,
        LinearGravity::Center => 10,
    }
}

#[cfg(feature = "native-standalone")]
fn linear_layout_gravity_value(value: LinearLayoutGravity) -> i32 {
    match value {
        LinearLayoutGravity::None => 0,
        LinearLayoutGravity::Top => 1,
        LinearLayoutGravity::Bottom => 2,
        LinearLayoutGravity::Left => 3,
        LinearLayoutGravity::Right => 4,
        LinearLayoutGravity::CenterVertical => 5,
        LinearLayoutGravity::CenterHorizontal => 6,
        LinearLayoutGravity::FillVertical => 7,
        LinearLayoutGravity::FillHorizontal => 8,
        LinearLayoutGravity::Center => 9,
        LinearLayoutGravity::Stretch => 10,
        LinearLayoutGravity::Start => 11,
        LinearLayoutGravity::End => 12,
    }
}

#[cfg(feature = "native-standalone")]
fn linear_cross_gravity_value(value: LinearCrossGravity) -> i32 {
    match value {
        LinearCrossGravity::None => 0,
        LinearCrossGravity::Start => 1,
        LinearCrossGravity::End => 2,
        LinearCrossGravity::Center => 3,
        LinearCrossGravity::Stretch => 4,
    }
}

#[cfg(feature = "native-standalone")]
fn relative_center_value(value: RelativeCenter) -> i32 {
    match value {
        RelativeCenter::None => 0,
        RelativeCenter::Vertical => 1,
        RelativeCenter::Horizontal => 2,
        RelativeCenter::Both => 3,
    }
}

#[cfg(feature = "native-standalone")]
fn grid_auto_flow_value(value: GridAutoFlow) -> i32 {
    match value {
        GridAutoFlow::Row => 0,
        GridAutoFlow::Column => 1,
        GridAutoFlow::Dense => 2,
        GridAutoFlow::RowDense => 3,
        GridAutoFlow::ColumnDense => 4,
    }
}

#[cfg(feature = "native-standalone")]
fn justify_items_value(value: JustifyItems) -> i32 {
    match value {
        JustifyItems::Auto => 0,
        JustifyItems::Stretch => 1,
        JustifyItems::Start => 2,
        JustifyItems::End => 3,
        JustifyItems::Center => 4,
    }
}

#[cfg(feature = "native-standalone")]
fn position_type_value(value: PositionType) -> i32 {
    match value {
        PositionType::Absolute => 0,
        PositionType::Relative => 1,
        PositionType::Fixed => 2,
        PositionType::Sticky => 3,
        PositionType::Static => 1,
    }
}

#[cfg(feature = "native-standalone")]
fn box_sizing_value(value: BoxSizing) -> i32 {
    match value {
        BoxSizing::BorderBox => 0,
        BoxSizing::ContentBox => 1,
    }
}

#[cfg(feature = "native-standalone")]
fn calculate_probe_layout(tree: &mut StandaloneTree, root: NodeId) {
    tree.calculate_layout(root, Size::new(100.0, 50.0), Direction::Ltr)
        .expect("calculate probe layout");
}

#[cfg(feature = "native-standalone")]
fn snapshot_public_tree_stage(
    stage: StandalonePublicTreeStage,
    tree: &StandaloneTree,
    nodes: &[NodeId],
    root: NodeId,
    staging: NodeId,
) -> StandalonePublicTreeSnapshot {
    StandalonePublicTreeSnapshot {
        stage,
        root_children: child_ids(tree, nodes, root),
        staging_children: child_ids(tree, nodes, staging),
        parents: nodes
            .iter()
            .map(|node| {
                tree.parent(*node)
                    .expect("parent")
                    .and_then(|parent| node_id(nodes, parent))
            })
            .collect(),
        dirty: nodes
            .iter()
            .map(|node| tree.is_dirty(*node).expect("dirty state"))
            .collect(),
    }
}

#[cfg(feature = "native-standalone")]
fn snapshot_public_dirty_nodes(tree: &StandaloneTree, nodes: &[NodeId]) -> Vec<bool> {
    nodes
        .iter()
        .map(|node| tree.is_dirty(*node).expect("dirty state"))
        .collect()
}

#[cfg(feature = "native-standalone")]
fn child_ids(tree: &StandaloneTree, nodes: &[NodeId], parent: NodeId) -> Vec<usize> {
    tree.children(parent)
        .expect("children")
        .iter()
        .filter_map(|child| node_id(nodes, *child))
        .collect()
}

#[cfg(feature = "native-standalone")]
fn node_id(nodes: &[NodeId], node: NodeId) -> Option<usize> {
    nodes.iter().position(|candidate| *candidate == node)
}
