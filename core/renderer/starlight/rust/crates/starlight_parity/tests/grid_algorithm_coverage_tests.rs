// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::collections::{BTreeMap, BTreeSet};
use std::fs;
use std::path::{Path, PathBuf};

const REQUIRED_SECTIONS: [&str; 13] = [
    "8.5 Grid Item Placement Algorithm",
    "9 Absolute Positioning",
    "10 Alignment and Spacing",
    "11.1 Grid Sizing Algorithm",
    "11.2 Track Sizing Terminology",
    "11.3 Track Sizing Algorithm",
    "11.4 Initialize Track Sizes",
    "11.5 Resolve Intrinsic Track Sizes",
    "11.5.1 Distributing Extra Space Across Spanned Tracks",
    "11.6 Maximize Tracks",
    "11.7 Expand Flexible Tracks",
    "11.7.1 Find the Size of an fr",
    "11.8 Stretch auto Tracks",
];

#[test]
fn grid_algorithm_inventory_tracks_w3c_placement_and_sizing_steps() {
    let inventory = read_inventory();
    let entries = inventory_entries(&inventory);
    let actual_sections = entries.keys().cloned().collect::<BTreeSet<_>>();
    let expected_sections = REQUIRED_SECTIONS
        .into_iter()
        .map(str::to_owned)
        .collect::<BTreeSet<_>>();

    assert_eq!(
        expected_sections, actual_sections,
        "W3C grid algorithm inventory must list placement, absolute positioning, alignment/spacing, and every section 11 sizing step"
    );

    for section in REQUIRED_SECTIONS {
        let entry = entries.get(section).expect("required section exists");
        assert!(
            matches!(entry.status.as_deref(), Some("partial" | "complete")),
            "{section} must have a `status` of partial or complete"
        );
        assert!(
            !entry.implementations.is_empty(),
            "{section} must list Rust implementation evidence"
        );
        assert!(
            !entry.tests.is_empty(),
            "{section} must list Rust test evidence"
        );
        assert!(
            !entry.gaps.is_empty(),
            "{section} must describe remaining W3C audit or implementation gaps"
        );
    }
}

#[test]
fn grid_algorithm_inventory_records_grid_module_coverage_snapshot() {
    let inventory = read_inventory();
    let normalized_inventory = inventory.split_whitespace().collect::<Vec<_>>().join(" ");
    assert!(
        inventory.contains("Grid Module Coverage Snapshot")
            && inventory.contains("cargo llvm-cov -q -p starlight_layout -p starlight_parity")
            && inventory.contains("--lib --test grid_layout_tests")
            && inventory.contains("crates/starlight_layout/src/engine/grid.rs")
            && inventory.contains("3571/3581 source-based lines, 99.72%")
            && inventory.contains("3505/3505")
            && inventory.contains("100.00%")
            && inventory.contains("show-missing-lines")
            && inventory.contains("does not")
            && inventory.contains("list `grid.rs`")
            && inventory.contains("synthetic region counters")
            && normalized_inventory.contains(
                "private helpers are counted only when they have W3C-grounded unit coverage"
            ),
        "grid inventory must keep the latest grid.rs coverage evidence and uncovered-line rationale"
    );
}

#[test]
fn grid_algorithm_inventory_records_visibility_layout_participation() {
    let inventory = read_inventory();
    let workspace = workspace_dir();
    let grid_tests =
        fs::read_to_string(workspace.join("crates/starlight_parity/tests/grid_layout_tests.rs"))
            .expect("read Rust grid layout tests");
    let native_tests = fs::read_to_string(
        workspace.join("crates/starlight_parity/tests/native_head_to_head_tests.rs"),
    )
    .expect("read native head-to-head tests");

    assert!(
        inventory.contains("hidden_and_collapse_grid_children_participate_in_auto_placement")
            && inventory.contains(
                "head_to_head_grid_visibility_hidden_and_collapse_participate_in_auto_placement"
            )
            && inventory.contains("CSS Display §4 visibility coverage")
            && inventory.contains("still participate in grid auto-placement and sizing"),
        "grid inventory must record visibility:hidden/collapse as grid layout participation, not resolver cleanup"
    );
    assert!(
        grid_tests.contains("fn hidden_and_collapse_grid_children_participate_in_auto_placement()")
            && grid_tests.contains("visibility: Visibility::Hidden")
            && grid_tests.contains("visibility: Visibility::Collapse"),
        "grid layout tests must cover hidden/collapse children participating in grid placement"
    );
    assert!(
        native_tests.contains(
            "fn head_to_head_grid_visibility_hidden_and_collapse_participate_in_auto_placement()"
        ),
        "native head-to-head tests must cover represented grid visibility placement parity"
    );
}

#[test]
fn grid_algorithm_inventory_targets_existing_symbols() {
    let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
    let workspace_dir = workspace_dir();
    let entries = inventory_entries(&read_inventory());

    let mut missing = Vec::new();
    for entry in entries.values() {
        for target in &entry.implementations {
            let (file, symbol) = split_target(target);
            let path = workspace_dir.join(file);
            let Ok(source) = fs::read_to_string(&path) else {
                missing.push(format!(
                    "{}: missing implementation file `{file}`",
                    entry.section
                ));
                continue;
            };
            if !source.contains(symbol) {
                missing.push(format!(
                    "{}: implementation `{target}` does not contain symbol `{symbol}`",
                    entry.section
                ));
            }
        }

        for target in &entry.tests {
            let (file, test_name) = split_target(target);
            let path = manifest_dir.join(file);
            let Ok(source) = fs::read_to_string(&path) else {
                missing.push(format!("{}: missing test file `{file}`", entry.section));
                continue;
            };
            if !rust_test_functions(&source).contains(test_name) {
                missing.push(format!(
                    "{}: test `{target}` does not point to a #[test] function",
                    entry.section
                ));
            }
        }
    }

    assert!(
        missing.is_empty(),
        "grid algorithm inventory targets must resolve:\n{}",
        missing.join("\n")
    );
}

#[test]
fn grid_algorithm_inventory_records_ignored_grid_head_to_head_gaps() {
    let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
    let entries = inventory_entries(&read_inventory());
    let recorded = entries
        .values()
        .flat_map(|entry| entry.ignored_head_to_heads.iter())
        .map(IgnoredHeadToHead::key)
        .collect::<BTreeSet<_>>();

    let ignored = [
        "tests/native_head_to_head_tests.rs",
        "tests/standalone_head_to_head_tests.rs",
    ]
    .into_iter()
    .flat_map(|file| {
        let source = fs::read_to_string(manifest_dir.join(file))
            .unwrap_or_else(|error| panic!("read `{file}`: {error}"));
        ignored_grid_head_to_head_tests(file, &source)
    })
    .map(|record| record.key())
    .collect::<BTreeSet<_>>();

    assert!(
        !ignored.is_empty(),
        "head-to-head suites must keep explicit ignored grid gap records"
    );

    let missing = ignored.difference(&recorded).cloned().collect::<Vec<_>>();
    let stale = recorded.difference(&ignored).cloned().collect::<Vec<_>>();

    assert!(
        missing.is_empty() && stale.is_empty(),
        "GRID_ALGORITHM_COVERAGE.md must exactly record ignored grid head-to-head gaps\nmissing:\n{}\nstale:\n{}",
        missing.join("\n"),
        stale.join("\n")
    );
}

#[test]
fn grid_algorithm_inventory_records_starlight_cpp_grid_tests() {
    let inventory = read_inventory();
    assert!(
        inventory.contains("Starlight C++ Grid Test Inventory")
            && inventory.contains("StyleDataCoverageTest.LayoutComputedStyleCopyOnWriteDataRefs")
            && inventory.contains("StyleDataCoverageTest.LayoutComputedStyleCopyFromNullDataRefs")
            && inventory.contains("layout_computed_style_copy_on_write_data_refs")
            && inventory.contains("layout_computed_style_copy_from_null_data_refs")
            && inventory.contains("container_node_unittest.cc")
            && inventory.contains("has no grid-specific layout assertions"),
        "grid inventory must record the current Starlight C++ grid-related unittest surface"
    );

    let workspace = workspace_dir();
    let cxx_data_ref = fs::read_to_string(workspace.join("../style/data_ref_unittest.cc"))
        .expect("read C++ data ref tests");
    let cxx_container = fs::read_to_string(workspace.join("../layout/container_node_unittest.cc"))
        .expect("read C++ container node tests");
    let rust_style_data = fs::read_to_string(
        workspace.join("crates/starlight_parity/tests/style_data_coverage_tests.rs"),
    )
    .expect("read Rust style data coverage tests");
    assert!(
        cxx_data_ref.contains("LayoutComputedStyleCopyOnWriteDataRefs")
            && cxx_data_ref.contains("grid_data_.Access()->grid_column_gap_")
            && rust_style_data.contains("layout_computed_style_copy_on_write_data_refs")
            && rust_style_data.contains("shared.grid_data.access().unwrap().grid_column_gap"),
        "Rust style-data coverage must port the C++ grid copy-on-write assertion"
    );
    assert!(
        cxx_data_ref.contains("LayoutComputedStyleCopyFromNullDataRefs")
            && cxx_data_ref.contains("source.grid_data_ = nullptr")
            && rust_style_data.contains("layout_computed_style_copy_from_null_data_refs")
            && rust_style_data.contains("source.grid_data.clear()"),
        "Rust style-data coverage must port the C++ null grid_data fallback assertion"
    );
    assert!(
        !cxx_container.contains("Grid") && !cxx_container.contains("grid"),
        "update the grid inventory if container_node_unittest.cc grows grid-specific layout assertions"
    );
}

#[test]
fn grid_algorithm_has_no_cxx_compatibility_debt_markers() {
    let inventory = read_inventory();
    let grid_engine = read_grid_engine();

    assert!(
        !inventory.contains("like_cxx"),
        "grid W3C algorithm inventory must not carry C++ compatibility debt markers"
    );
    assert!(
        !grid_engine.contains("like_cxx"),
        "safe Rust grid layout core must not add C++ compatibility-only branches"
    );
}

#[test]
fn grid_algorithm_stays_split_into_grid_module() {
    let workspace = workspace_dir();
    let engine = fs::read_to_string(workspace.join("crates/starlight_layout/src/engine.rs"))
        .expect("read Rust layout engine root");
    let grid_engine = read_grid_engine();

    assert!(
        engine.contains("mod grid;"),
        "engine.rs must keep grid layout in a dedicated engine/grid.rs module"
    );
    assert!(
        engine.contains("Display::Grid => self.layout_grid("),
        "engine.rs should only dispatch grid containers into the grid module"
    );

    for required_grid_symbol in [
        "pub(super) fn layout_grid",
        "fn resolve_grid_axis_placement",
        "fn resolve_grid_tracks_for_axis",
        "fn finalize_grid_tracks",
        "fn grid_absolute_area",
        "fn grid_out_of_flow_offset",
        "struct GridTrackSizing",
        "struct GridIntrinsicContribution",
    ] {
        assert!(
            grid_engine.contains(required_grid_symbol),
            "engine/grid.rs must own grid algorithm symbol `{required_grid_symbol}`"
        );
    }

    for legacy_marker in [
        "fn resolve_grid_axis_placement",
        "fn resolve_grid_tracks_for_axis",
        "fn finalize_grid_tracks",
        "fn grid_absolute_area",
        "fn grid_out_of_flow_offset",
        "struct GridTrackSizing",
        "struct GridIntrinsicContribution",
    ] {
        assert!(
            !engine.contains(legacy_marker),
            "engine.rs must not grow legacy grid algorithm code marker `{legacy_marker}`"
        );
    }
}

#[test]
fn grid_inventory_surface_limits_are_grounded_in_starlight_sources() {
    let inventory = read_inventory();
    assert!(
        inventory.contains("not represented by the current Rust/C++ Starlight style surface"),
        "inventory must explicitly distinguish W3C grid features outside the current Starlight style surface"
    );
    assert!(
        inventory.contains("Syntax validation and shorthand expansion happen before layout")
            && inventory.contains("any semantic effect of syntactically valid values")
            && inventory.contains("fallback, or applicability is layout-engine logic")
            && inventory.contains("The style resolver only rejects or normalizes syntax/grammar invalid values")
            && inventory.contains("must not erase syntax-valid style data")
            && inventory.contains("numeric line/span placement fields"),
        "inventory must keep resolver work limited to syntax/shorthand handling and keep syntax-valid layout semantics in layout"
    );
    for unsupported_surface in [
        "named grid lines/areas",
        "repeat()/auto-fill/auto-fit",
        "grid-template-areas",
        "grid-area shorthand",
        "grid-template/grid shorthand",
    ] {
        assert!(
            inventory.contains(unsupported_surface),
            "inventory must record unsupported W3C grid surface `{unsupported_surface}`"
        );
    }
    for stale_layout_gap in [
        "Remaining §8.5 placement surface work includes named grid lines/areas",
        "Remaining work is limited to §9 variants not represented",
        "plus W3C track-list expansion",
        "Named grid lines/areas and grid-area shorthand remain parser/resolver surface work",
        "Track-list grammar for named grid lines/areas, repeat()/auto-fill/auto-fit, grid-template-areas, and grid-template/grid shorthand is parser/resolver work",
        "W3C track-list expansion for named grid lines/areas, repeat()/auto-fill/auto-fit, grid-template-areas, and grid-template/grid shorthand belongs to parser/resolver",
    ] {
        assert!(
            !inventory.contains(stale_layout_gap),
            "inventory must not move syntax-valid layout semantics into the resolver with stale marker `{stale_layout_gap}`"
        );
    }
    assert!(
        inventory.contains("Scrollable-overflow padding-edge lines are layout behavior")
            && inventory.contains("not resolver")
            && inventory.contains("syntax work")
            && inventory.contains("laid-out track extent")
            && inventory.contains("as the scrollable")
            && inventory.contains("content size for absolutely-positioned auto grid lines")
            && inventory
                .contains("absolute_grid_item_auto_lines_use_scrollable_overflow_padding_edges")
            && inventory.contains(
                "rtl_absolute_grid_item_auto_lines_use_scrollable_overflow_padding_edges"
            )
            && inventory.contains("no overflow/scroll-container input")
            && inventory.contains("no scrollable overflow rectangle output"),
        "inventory must classify scrollable-overflow grid auto lines as layout behavior while recording the missing public overflow surface"
    );

    let workspace = workspace_dir();
    let rust_style = fs::read_to_string(workspace.join("crates/starlight_layout/src/style.rs"))
        .expect("read Rust style surface");
    let rust_types = fs::read_to_string(workspace.join("crates/starlight_layout/src/types.rs"))
        .expect("read Rust layout types");
    let rust_ffi =
        fs::read_to_string(workspace.join("crates/starlight_ffi/src/lib.rs")).expect("read FFI");
    let cxx_grid_data =
        fs::read_to_string(workspace.join("../style/grid_data.h")).expect("read C++ GridData");
    let cxx_grid_algorithm =
        fs::read_to_string(workspace.join("../layout/grid_layout_algorithm.h"))
            .expect("read C++ grid algorithm header");
    let standalone_header =
        fs::read_to_string(workspace.join("../../../include/starlight_standalone/starlight.h"))
            .expect("read standalone public header");

    for supported in [
        "grid_template_columns",
        "grid_template_rows",
        "grid_auto_columns",
        "grid_auto_rows",
        "grid_column_start",
        "grid_column_end",
        "grid_row_start",
        "grid_row_end",
        "grid_column_span",
        "grid_row_span",
    ] {
        assert!(
            rust_style.contains(supported),
            "Rust Style must expose represented grid surface `{supported}`"
        );
        assert!(
            rust_ffi.contains(supported),
            "Rust FFI must expose represented grid surface `{supported}`"
        );
    }
    for unsupported in [
        "grid_template_areas",
        "grid_area_names",
        "grid_area_shorthand",
        "grid_column_start_name",
        "grid_column_end_name",
        "grid_row_start_name",
        "grid_row_end_name",
        "grid_auto_repeat",
        "grid_auto_fill",
        "grid_auto_fit",
        "grid_track_line_names",
        "grid_shorthand",
        "grid_template_shorthand",
        "writing_mode",
    ] {
        assert!(
            !rust_style.contains(unsupported),
            "Rust Style unexpectedly exposes unsupported grid surface `{unsupported}`"
        );
        assert!(
            !cxx_grid_data.contains(unsupported),
            "C++ GridData unexpectedly exposes unsupported grid surface `{unsupported}`"
        );
    }
    for unsupported_ffi_surface in [
        "grid_template_areas",
        "grid_area_names",
        "grid_area_shorthand",
        "grid_column_start_name",
        "grid_column_end_name",
        "grid_row_start_name",
        "grid_row_end_name",
        "grid_repeat",
        "grid_auto_repeat",
        "grid_auto_fill",
        "grid_auto_fit_repeat",
        "grid_auto_fit_track",
        "grid_template_shorthand",
        "grid_shorthand",
        "grid_track_line_names",
    ] {
        assert!(
            !rust_ffi.contains(unsupported_ffi_surface),
            "Rust FFI unexpectedly exposes unsupported grid parser surface `{unsupported_ffi_surface}`"
        );
    }
    for unsupported_api in [
        "SLNodeStyleSetGridTemplateAreas",
        "SLNodeStyleSetGridArea",
        "SLNodeStyleSetGridColumnStartName",
        "SLNodeStyleSetGridColumnEndName",
        "SLNodeStyleSetGridRowStartName",
        "SLNodeStyleSetGridRowEndName",
        "SLGridRepeat",
        "SLGridAutoFill",
        "SLGridAutoFit",
    ] {
        assert!(
            !standalone_header.contains(unsupported_api),
            "standalone public C API unexpectedly exposes unsupported grid surface `{unsupported_api}`"
        );
    }
    assert!(
        cxx_grid_algorithm.contains("Writing-mode is not yet supported"),
        "C++ grid algorithm must explicitly document that writing-mode is not represented"
    );
    assert!(
        !rust_style.contains("pub overflow")
            && !rust_style.contains("scroll_container")
            && !rust_types.contains("scrollable_overflow"),
        "public overflow/scroll-container inputs and scrollable overflow output should remain recorded as missing surface"
    );
}

#[test]
fn grid_track_inventory_records_fit_content_resolver_invariant() {
    let inventory = read_inventory();
    assert!(
        inventory.contains("The grid layout algorithm consumes style resolver output")
            && inventory.contains("syntactically valid for their CSS property")
            && inventory.contains("Invalid CSS inputs belong to the")
            && inventory.contains("not to layout-algorithm repair logic"),
        "grid inventory must record the resolver-output contract for all grid algorithm inputs"
    );
    assert!(
        inventory.contains("only a CSS syntax/grammar boundary")
            && inventory.contains("syntactically valid but a specific layout mode")
            && inventory.contains("does not apply, does not participate, or falls back")
            && inventory.contains("that remains layout-engine logic")
            && inventory.contains("The style resolver only rejects or normalizes syntax/grammar invalid values")
            && inventory.contains("must not erase syntax-valid style data"),
        "grid inventory must not move syntactically valid layout applicability rules into the resolver"
    );
    assert!(
        inventory.contains("fit-content(<length-percentage>)")
            && inventory.contains("Layout consumes style resolver output")
            && inventory.contains("unsupported style-surface states for grid tracks")
            && inventory.contains("missing-argument invariant"),
        "grid inventory must record that grid track fit-content requires a W3C argument"
    );
    for unsupported_marker in [
        "grid_fit_content_empty_argument_does_not_cap_intrinsic_growth",
        "grid_fit_content_without_argument_does_not_cap_intrinsic_growth",
        "Starlight/C++ no-argument fit-content",
    ] {
        assert!(
            !inventory.contains(unsupported_marker),
            "grid inventory must not list unsupported argumentless fit-content marker `{unsupported_marker}` as coverage"
        );
    }

    let grid_engine = read_grid_engine();
    let grid_layout_tests = fs::read_to_string(
        Path::new(env!("CARGO_MANIFEST_DIR")).join("tests/grid_layout_tests.rs"),
    )
    .expect("read grid layout tests");
    let native_bridge = fs::read_to_string(
        Path::new(env!("CARGO_MANIFEST_DIR")).join("../starlight_cpp/src/native.rs"),
    )
    .expect("read native C++ bridge");
    let standalone_tree_tests = fs::read_to_string(
        Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("../starlight_standalone/tests/standalone_tree_tests.rs"),
    )
    .expect("read standalone tree tests");
    assert!(
        !grid_engine.contains("BaseLength::empty()"),
        "grid engine tests must not model non-W3C empty grid track fit-content input"
    );
    assert!(
        !grid_layout_tests.contains("BaseLength::empty()"),
        "grid layout tests must not model non-W3C empty grid track fit-content input"
    );
    for line in grid_layout_tests.lines() {
        assert!(
            !(line.contains("grid_template_")
                || line.contains("grid_auto_columns")
                || line.contains("grid_auto_rows"))
                || !line.contains("fit_content(None)"),
            "grid layout tests must not model non-W3C argumentless grid track fit-content input: `{line}`"
        );
    }
    assert!(
        native_bridge.contains("Length::FitContent(Some(base)) => base.has_value()")
            && native_bridge.contains("Length::FitContent(None) | Length::MinContent => false"),
        "native bridge grid track mapping must only encode argumented fit-content for grid tracks"
    );
    assert!(
        grid_engine.contains("grid track fit-content requires a <length-percentage> argument"),
        "grid engine must assert the W3C argumented fit-content track invariant instead of repairing argumentless track state"
    );
    assert!(
        !native_bridge.contains("starlight_value_from_grid_track_length(Length::FitContent(None))"),
        "native bridge must not encode unsupported argumentless fit-content as a grid track"
    );
    for line in standalone_tree_tests.lines() {
        assert!(
            !(line.contains("set_grid_") && line.contains("fit_content(None)")),
            "standalone tree public grid setter tests must not round-trip unsupported grid track `{line}`"
        );
    }
}

#[test]
fn grid_track_inventory_records_fr_resolver_invariant() {
    let inventory = read_inventory();
    assert!(
        inventory.contains("non-negative resolver output")
            && inventory.contains("does not clamp negative `fr` values")
            && inventory.contains("raw `NLength` flex-factor use")
            && inventory.contains("input belongs to the parser/style resolver"),
        "grid inventory must record that negative fr values are resolver errors, not layout algorithm repair cases"
    );

    let grid_engine = read_grid_engine();
    let workspace = workspace_dir();
    assert!(
        !grid_engine.contains("flex.max(0.0)"),
        "grid engine must not silently clamp invalid negative fr track factors"
    );
    assert!(
        grid_engine.contains("debug_assert!(flex >= 0.0);"),
        "grid engine should keep the resolver invariant visible in debug builds"
    );
    let cxx_grid_algorithm =
        fs::read_to_string(workspace.join("../layout/grid_layout_algorithm.cc"))
            .expect("read C++ grid algorithm");
    assert!(
        cxx_grid_algorithm
            .contains("flex_factor[idx] = max_track_sizing_function[idx].GetRawValue();"),
        "C++ grid algorithm should be recorded as using the raw fr flex factor"
    );
}

#[test]
fn grid_track_inventory_records_span_resolver_invariant() {
    let inventory = read_inventory();
    let normalized_inventory = inventory.split_whitespace().collect::<Vec<_>>().join(" ");
    assert!(
        normalized_inventory.contains("positive resolver output")
            && normalized_inventory.contains("does not silently clamp zero spans to one")
            && normalized_inventory.contains("spans are rejected at the Rust FFI boundary"),
        "grid inventory must record that zero spans are resolver/FFI errors, not layout repair cases"
    );

    let grid_engine = read_grid_engine();
    let grow_intrinsic_tracks = source_after(&grid_engine, "fn grow_grid_intrinsic_tracks")
        .split("fn indefinite_grid_intrinsic_base_required_size")
        .next()
        .expect("grow_grid_intrinsic_tracks source slice");
    let update_growth_limits = source_after(&grid_engine, "fn update_grid_intrinsic_growth_limits")
        .split("fn distribute_grid_growth_limits_to_tracks")
        .next()
        .expect("update_grid_intrinsic_growth_limits source slice");
    let workspace = workspace_dir();
    let ffi = fs::read_to_string(workspace.join("crates/starlight_ffi/src/lib.rs"))
        .expect("read Rust FFI");
    assert!(
        !grid_engine.contains("span.max(1)")
            && !grid_engine.contains("input.span.max(1)")
            && !grid_engine.contains("grid_intrinsic_growth_returns_for_empty_or_out_of_bounds_span")
            && !grow_intrinsic_tracks.contains("if span.is_empty()")
            && !grow_intrinsic_tracks.contains("span.end.min")
            && !update_growth_limits.contains("span.end.min")
            && !update_growth_limits.contains("start >= end")
            && grid_engine.contains("debug_assert!(span > 0);")
            && grid_engine.contains("debug_assert!(input.span > 0);")
            && grow_intrinsic_tracks.contains("debug_assert!(start < end);")
            && grow_intrinsic_tracks.contains("debug_assert!(end <= tracks.len());")
            && update_growth_limits.contains("debug_assert!(start < end);")
            && update_growth_limits.contains("debug_assert!(end <= tracks.len());"),
        "grid engine must not silently clamp invalid zero spans or repair empty/out-of-range intrinsic spans"
    );
    assert!(
        ffi.contains("grid_column_span: grid_span_from_ffi(style.grid_column_span)?")
            && ffi.contains("grid_row_span: grid_span_from_ffi(style.grid_row_span)?")
            && ffi.contains("fn ffi_rejects_invalid_grid_spans()"),
        "Rust FFI must reject zero grid spans before constructing layout styles"
    );
}

#[test]
fn grid_external_intrinsic_measurement_surface_is_rust_only_until_cpp_bridge_exists() {
    let inventory = read_inventory();
    assert!(
        inventory.contains("LayoutTree::measure_min_content")
            && inventory.contains("LayoutTree::measure_max_content")
            && inventory.contains("distinct intrinsic measurement callbacks"),
        "inventory must record why external min/max-content grid contributions are Rust-only"
    );

    let workspace = workspace_dir();
    let layout_engine = fs::read_to_string(workspace.join("crates/starlight_layout/src/engine.rs"))
        .expect("read Rust layout engine trait");
    let native_bridge = fs::read_to_string(workspace.join("crates/starlight_cpp/src/native.rs"))
        .expect("read native C++ bridge");
    let rust_ffi = fs::read_to_string(workspace.join("crates/starlight_ffi/src/lib.rs"))
        .expect("read Rust FFI");
    let rust_ffi_header =
        fs::read_to_string(workspace.join("crates/starlight_ffi/include/starlight_rust_ffi.h"))
            .expect("read Rust FFI header");
    let standalone_header =
        fs::read_to_string(workspace.join("../../../include/starlight_standalone/starlight.h"))
            .expect("read standalone public header");

    assert!(
        layout_engine.contains("fn measure_min_content(")
            && layout_engine.contains("fn measure_max_content("),
        "Rust LayoutTree must expose distinct intrinsic measurement callbacks"
    );
    assert!(
        native_bridge.contains(".measure(context.node, constraints)")
            && !native_bridge.contains("measure_min_content(")
            && !native_bridge.contains("measure_max_content("),
        "native C++ bridge currently mirrors only the regular measure callback"
    );
    assert!(
        standalone_header.contains("StarlightMeasureFunc measure_func_")
            && standalone_header.contains("StarlightBaselineFunc baseline_func_"),
        "standalone public measure delegate must expose the current regular measure/baseline surface"
    );
    assert!(
        inventory.contains("Rust FFI can transport `Length::MinContent`")
            && rust_ffi.contains("7 => Ok(Length::MinContent)")
            && rust_ffi_header.contains("SLRustLengthMinContent = 7"),
        "Rust FFI must expose min-content while C++ standalone remains the parity gap"
    );
    for unsupported_callback in [
        "measure_min_content",
        "measure_max_content",
        "min_content_func",
        "max_content_func",
        "StarlightMinContent",
        "StarlightMaxContent",
    ] {
        assert!(
            !standalone_header.contains(unsupported_callback),
            "standalone public C API unexpectedly exposes intrinsic measurement callback `{unsupported_callback}`"
        );
    }
}

#[test]
fn grid_inventory_fragmentation_surface_is_not_represented() {
    let inventory = read_inventory();
    assert!(
        inventory.contains("12 Fragmenting Grid Layout"),
        "inventory must record the W3C fragmentation section as a current surface boundary"
    );
    for required_gap_term in ["fragmentainer", "page-break", "break-before/after/inside"] {
        assert!(
            inventory.contains(required_gap_term),
            "inventory must name missing fragmentation input `{required_gap_term}`"
        );
    }

    let workspace = workspace_dir();
    let rust_style = fs::read_to_string(workspace.join("crates/starlight_layout/src/style.rs"))
        .expect("read Rust style surface");
    let rust_ffi =
        fs::read_to_string(workspace.join("crates/starlight_ffi/src/lib.rs")).expect("read FFI");
    let cxx_grid_data =
        fs::read_to_string(workspace.join("../style/grid_data.h")).expect("read C++ GridData");
    let standalone_header =
        fs::read_to_string(workspace.join("../../../include/starlight_standalone/starlight.h"))
            .expect("read standalone public header");

    for unsupported_fragmentation_surface in [
        "fragmentainer",
        "fragmentation_context",
        "break_before",
        "break_after",
        "break_inside",
        "page_break",
        "SLNodeStyleSetBreakBefore",
        "SLNodeStyleSetBreakAfter",
        "SLNodeStyleSetBreakInside",
    ] {
        assert!(
            !rust_style.contains(unsupported_fragmentation_surface),
            "Rust Style unexpectedly exposes fragmentation surface `{unsupported_fragmentation_surface}`"
        );
        assert!(
            !rust_ffi.contains(unsupported_fragmentation_surface),
            "Rust FFI unexpectedly exposes fragmentation surface `{unsupported_fragmentation_surface}`"
        );
        assert!(
            !cxx_grid_data.contains(unsupported_fragmentation_surface),
            "C++ GridData unexpectedly exposes fragmentation surface `{unsupported_fragmentation_surface}`"
        );
        assert!(
            !standalone_header.contains(unsupported_fragmentation_surface),
            "standalone public C API unexpectedly exposes fragmentation surface `{unsupported_fragmentation_surface}`"
        );
    }
}

#[test]
fn grid_track_sizing_pipeline_preserves_w3c_phase_order() {
    let grid_engine = read_grid_engine();
    let layout_grid = source_after(&grid_engine, "pub(super) fn layout_grid");
    assert_source_order(
        layout_grid,
        &[
            "let (mut columns, mut column_track_sizing) = self.resolve_grid_tracks_for_axis(",
            "self.stretch_grid_auto_tracks(\n            &mut columns",
            "self.update_grid_block_axis_contributions_after_column_sizing(",
            "let (mut rows, mut row_track_sizing) = self.resolve_grid_tracks_for_axis(",
            "self.stretch_grid_auto_tracks(\n            &mut rows",
            ".update_grid_inline_axis_contributions_after_row_sizing(",
            "if inline_contribution_changed {",
            "(columns, column_track_sizing) = self.resolve_grid_tracks_for_axis(",
            ".update_grid_block_axis_contributions_after_column_sizing(",
            "if block_contribution_changed {",
            "(rows, row_track_sizing) = self.resolve_grid_tracks_for_axis(",
        ],
    );

    let resolve_grid_tracks_for_axis =
        source_after(&grid_engine, "fn resolve_grid_tracks_for_axis");
    assert_source_order(
        resolve_grid_tracks_for_axis,
        &[
            "self.resolve_grid_tracks(definitions, count, content_limit, max_content_limit, gap)",
            "Self::grid_intrinsic_growth_groups(measured_items, &track_sizing, horizontal)",
            "self.finalize_grid_tracks(",
            "self.expand_grid_flexible_tracks_for_indefinite_content(",
        ],
    );

    let finalize_grid_tracks = source_after(&grid_engine, "fn finalize_grid_tracks");
    assert_source_order(
        finalize_grid_tracks,
        &[
            "self.maximize_grid_tracks(tracks, growth_limits, content_limit, gap);",
            "self.expand_grid_flexible_tracks(tracks, track_sizing, content_limit, gap);",
            "self.maximize_grid_tracks_for_indefinite_content(",
        ],
    );
}

#[derive(Debug, Default)]
struct InventoryEntry {
    section: String,
    status: Option<String>,
    implementations: Vec<String>,
    tests: Vec<String>,
    gaps: Vec<String>,
    ignored_head_to_heads: Vec<IgnoredHeadToHead>,
}

#[derive(Clone, Debug, Eq, PartialEq)]
struct IgnoredHeadToHead {
    target: String,
    reason: String,
}

impl IgnoredHeadToHead {
    fn key(&self) -> String {
        format!("{} -- {}", self.target, self.reason)
    }
}

fn read_inventory() -> String {
    fs::read_to_string(Path::new(env!("CARGO_MANIFEST_DIR")).join("GRID_ALGORITHM_COVERAGE.md"))
        .expect("read grid algorithm coverage inventory")
}

fn read_grid_engine() -> String {
    fs::read_to_string(workspace_dir().join("crates/starlight_layout/src/engine/grid.rs"))
        .expect("read Rust grid layout engine")
}

fn workspace_dir() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../..")
        .canonicalize()
        .expect("resolve Rust workspace")
}

fn source_after<'a>(source: &'a str, marker: &str) -> &'a str {
    let start = source
        .find(marker)
        .unwrap_or_else(|| panic!("source must contain `{marker}`"));
    &source[start..]
}

fn assert_source_order(source: &str, markers: &[&str]) {
    let mut offset = 0;
    for marker in markers {
        let remaining = &source[offset..];
        let index = remaining
            .find(marker)
            .unwrap_or_else(|| panic!("source marker `{marker}` must appear after byte {offset}"));
        offset += index + marker.len();
    }
}

fn inventory_entries(inventory: &str) -> BTreeMap<String, InventoryEntry> {
    let mut entries = BTreeMap::new();
    let mut current: Option<InventoryEntry> = None;

    for line in inventory.lines() {
        if let Some(section) = section_header(line) {
            if let Some(entry) = current.take() {
                entries.insert(entry.section.clone(), entry);
            }
            current = Some(InventoryEntry {
                section,
                ..InventoryEntry::default()
            });
            continue;
        }

        let Some(entry) = current.as_mut() else {
            continue;
        };
        if let Some(status) = inventory_value(line, "status") {
            entry.status = Some(status);
        } else if let Some(implementation) = inventory_value(line, "implementation") {
            entry.implementations.push(implementation);
        } else if let Some(test) = inventory_value(line, "tests") {
            entry.tests.push(test);
        } else if let Some(gap) = inventory_value(line, "gap") {
            entry.gaps.push(gap);
        } else if let Some(ignored) = inventory_ignored_head_to_head(line) {
            entry.ignored_head_to_heads.push(ignored);
        }
    }

    if let Some(entry) = current {
        entries.insert(entry.section.clone(), entry);
    }

    entries
}

fn section_header(line: &str) -> Option<String> {
    let line = line.strip_prefix("## `")?;
    let (section, _) = line.split_once('`')?;
    Some(section.to_owned())
}

fn inventory_value(line: &str, key: &str) -> Option<String> {
    let prefix = format!("- `{key}`: ");
    line.strip_prefix(&prefix)
        .map(|value| value.trim().trim_matches('`').to_owned())
}

fn inventory_ignored_head_to_head(line: &str) -> Option<IgnoredHeadToHead> {
    let value = line.strip_prefix("- `ignored-head-to-head`: ")?.trim();
    let value = value.strip_prefix('`')?;
    let (target, reason) = value.split_once('`')?;
    let reason = reason.trim().strip_prefix("--")?.trim();
    Some(IgnoredHeadToHead {
        target: target.to_owned(),
        reason: reason.to_owned(),
    })
}

fn split_target(target: &str) -> (&str, &str) {
    target
        .split_once("::")
        .unwrap_or_else(|| panic!("inventory target `{target}` must contain `::`"))
}

fn ignored_grid_head_to_head_tests(file: &str, source: &str) -> Vec<IgnoredHeadToHead> {
    let mut ignored = Vec::new();
    let mut ignore_reason = None;

    for line in source.lines() {
        let trimmed = line.trim_start();
        if let Some(reason) = ignore_reason_attr(trimmed) {
            ignore_reason = Some(reason);
            continue;
        }

        if let Some(name) = trimmed.strip_prefix("fn ") {
            let Some((name, _)) = name.split_once('(') else {
                ignore_reason = None;
                continue;
            };
            if let Some(reason) = ignore_reason.take() {
                if name.contains("grid") || reason.contains("grid") {
                    ignored.push(IgnoredHeadToHead {
                        target: format!("{file}::{name}"),
                        reason,
                    });
                }
            }
            continue;
        }

        if ignore_reason.is_some() && !trimmed.starts_with("#[") && !trimmed.is_empty() {
            ignore_reason = None;
        }
    }

    ignored
}

fn ignore_reason_attr(line: &str) -> Option<String> {
    let reason = line.strip_prefix("#[ignore = \"")?;
    let reason = reason.strip_suffix("\"]")?;
    Some(reason.to_owned())
}

fn rust_test_functions(source: &str) -> BTreeSet<&str> {
    let mut tests = BTreeSet::new();
    let mut previous_line_was_test_attr = false;
    for line in source.lines() {
        let trimmed = line.trim_start();
        if trimmed == "#[test]" {
            previous_line_was_test_attr = true;
            continue;
        }
        if previous_line_was_test_attr && trimmed.starts_with("#[") {
            continue;
        }
        if previous_line_was_test_attr {
            if let Some(name) = trimmed.strip_prefix("fn ") {
                let Some((name, _)) = name.split_once('(') else {
                    previous_line_was_test_attr = false;
                    continue;
                };
                tests.insert(name);
            }
            previous_line_was_test_attr = false;
        }
    }
    tests
}
