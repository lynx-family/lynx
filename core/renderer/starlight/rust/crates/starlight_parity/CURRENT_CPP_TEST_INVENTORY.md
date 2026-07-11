# Current C++ Starlight Test Inventory

Renderer source targets are listed in `core/renderer/starlight/BUILD.gn` under
`starlight_testset`. `tests/cpp_test_inventory_tests.rs` verifies that every
C++ Starlight unittest source under `core/renderer/starlight` is part of that
target, and that every current C++ gtest case in that target is listed below as
`[translated]` and points at an existing Rust test function. The same inventory
scope also scans `core/services/starlight_standalone` and
`core/include/starlight_standalone`, so future standalone C++ tests must be
added here and ported to Rust instead of bypassing the Rust standalone path.

## Grid Head-to-Head Status (2026-07-08)

- `cargo test -q -p starlight_parity --test native_generated_head_to_head_tests grid`: 6 passed.
- `cargo test -q -p starlight_parity --test native_head_to_head_tests grid`: 169 passed, 10 ignored.
- `STARLIGHT_CPP_NATIVE_STANDALONE_BUILD_FROM_SOURCE=1 CARGO_TARGET_DIR=/private/tmp/starlight_cpp_native_grid_scrollable_20260708 cargo test -q -p starlight_cpp --features native-standalone`: source-built the standalone C++ native library, then 56 passed.
- `STARLIGHT_CPP_NATIVE_STANDALONE_BUILD_FROM_SOURCE=1 CARGO_TARGET_DIR=/private/tmp/starlight_cpp_native_grid_scrollable_20260708 cargo test -q -p starlight_parity --features native-standalone --test standalone_head_to_head_tests grid`: source-built the standalone C++ library, then 32 passed and 7 ignored.
- Source-built standalone C++ native archive:
  `/private/tmp/starlight_cpp_native_grid_scrollable_20260708/debug/build/starlight_cpp-db22e646577151ac/out/libstarlight_standalone.a`.

Known Rust/W3C vs current C++ standalone grid parity differences are recorded as
exact `ignored-head-to-head` entries in `GRID_ALGORITHM_COVERAGE.md`: 11 native
ignored gap records and 8 standalone ignored gap records. Two ignored grid gap
records do not appear in the `grid`-filtered Cargo counts because their function
names do not contain `grid`; the executable grid coverage inventory guard still
matches them by source file, test function, and ignore reason.

## `layout/container_node_unittest.cc`

- `[translated] ContainerNodeTests.ContainerNodeEmptyInit -> tests/container_node_tests.rs::container_node_empty_init`
- `[translated] ContainerNodeTests.ContainerNodeEmptyInsert -> tests/container_node_tests.rs::container_node_empty_insert`
- `[translated] ContainerNodeTests.ContainerNodeEmptyAppend -> tests/container_node_tests.rs::container_node_empty_append`
- `[translated] ContainerNodeTests.ContainerNodeAppend -> tests/container_node_tests.rs::container_node_append`
- `[translated] ContainerNodeTests.ContainerNodeInsertFront -> tests/container_node_tests.rs::container_node_insert_front`
- `[translated] ContainerNodeTests.ContainerNodeInsertMiddle -> tests/container_node_tests.rs::container_node_insert_middle`
- `[translated] ContainerNodeTests.ContainerNodeInsertEnd -> tests/container_node_tests.rs::container_node_insert_end`
- `[translated] ContainerNodeTests.ContainerNodeRemoveFirst -> tests/container_node_tests.rs::container_node_remove_first`
- `[translated] ContainerNodeTests.ContainerNodeRemoveMiddle -> tests/container_node_tests.rs::container_node_remove_middle`
- `[translated] ContainerNodeTests.ContainerNodeRemoveLast -> tests/container_node_tests.rs::container_node_remove_last`
- `[translated] ContainerNodeTests.ContainerNodeRemoveOnlyNode -> tests/container_node_tests.rs::container_node_remove_only_node`
- `[translated] ContainerNodeTests.ContainerNodeFindIndexAndDefensiveBranches -> tests/container_node_tests.rs::container_node_find_index_and_defensive_branches`
- `[translated] ContainerNodeTests.ContainerNodeFindPastEndAndInsertBeforeOwnedReference -> tests/container_node_tests.rs::container_node_find_past_end_and_insert_before_owned_reference`
- `[translated] ContainerNodeTests.ContainerNodeDestructorDetachesChildren -> tests/container_node_tests.rs::container_node_drop_detaches_children`
- `[translated] ContainerNodeTests.ContainerNodeInsertWithForeignReferenceDeath -> tests/container_node_tests.rs::container_node_insert_with_foreign_reference_panics`

## `style/data_ref_unittest.cc`

- `[translated] DataRefTest.CreateAndDestroy -> tests/data_ref_tests.rs::data_ref_create_and_destroy`
- `[translated] DataRefTest.CopyOnWrite -> tests/data_ref_tests.rs::data_ref_copy_on_write`
- `[translated] DataRefTest.RefCount -> tests/data_ref_tests.rs::data_ref_ref_count`
- `[translated] DataRefTest.AssignmentMoveAndEqualityBranches -> tests/data_ref_tests.rs::data_ref_assignment_move_and_equality_branches`
- `[translated] DataRefTest.SharedAccessAndInequalityFalseBranches -> tests/data_ref_tests.rs::data_ref_shared_access_and_inequality_false_branches`
- `[translated] StyleDataCoverageTest.BordersDataResetAndEquality -> tests/style_data_coverage_tests.rs::borders_data_reset_and_equality`
- `[translated] StyleDataCoverageTest.BordersDataFieldInequalityBranches -> tests/style_data_coverage_tests.rs::borders_data_field_inequality_branches`
- `[translated] StyleDataCoverageTest.LayoutStyleUtilsAndLayoutUnitBranches -> tests/style_data_coverage_tests.rs::layout_style_utils_and_layout_unit_branches`
- `[translated] StyleDataCoverageTest.LayoutConstraintBranches -> tests/style_data_coverage_tests.rs::layout_constraint_branches`
- `[translated] StyleDataCoverageTest.LayoutUnitComparisonAndMinMaxBranches -> tests/style_data_coverage_tests.rs::layout_unit_comparison_and_min_max_branches`
- `[translated] StyleDataCoverageTest.NLengthStringAndConversionBranches -> tests/style_data_coverage_tests.rs::nlength_string_and_conversion_branches`
- `[translated] StyleDataCoverageTest.LayoutComputedStyleCopyOnWriteDataRefs -> tests/style_data_coverage_tests.rs::layout_computed_style_copy_on_write_data_refs`
- `[translated] StyleDataCoverageTest.LayoutComputedStyleCopyFromNullDataRefs -> tests/style_data_coverage_tests.rs::layout_computed_style_copy_from_null_data_refs`
