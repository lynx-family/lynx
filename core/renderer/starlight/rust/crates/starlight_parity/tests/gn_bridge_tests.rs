// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#![forbid(unsafe_code)]

use std::fs;
use std::path::{Path, PathBuf};

#[test]
fn starlight_build_exposes_rust_test_group() {
    let starlight = starlight_source_dir();
    let build = fs::read_to_string(starlight.join("BUILD.gn")).expect("read Starlight BUILD.gn");

    assert!(
        build.contains("group(\"starlight_rust_tests\")"),
        "core/renderer/starlight/BUILD.gn must expose a Rust test target"
    );
    assert!(
        build.contains("deps = [ \"rust:starlight_rust_tests\" ]"),
        "the root Rust test target must delegate to the Rust workspace BUILD.gn"
    );
}

#[test]
fn starlight_build_exposes_rust_native_parity_and_benchmark_groups() {
    let starlight = starlight_source_dir();
    let build = fs::read_to_string(starlight.join("BUILD.gn")).expect("read Starlight BUILD.gn");

    for (target, delegate) in [
        (
            "group(\"starlight_rust_native_parity_tests\")",
            "deps = [ \"rust:starlight_rust_native_parity_tests\" ]",
        ),
        (
            "group(\"starlight_rust_cpp_import_tests\")",
            "deps = [ \"rust:starlight_rust_cpp_import_tests\" ]",
        ),
        (
            "group(\"starlight_rust_ffi_tests\")",
            "deps = [ \"rust:starlight_rust_ffi_tests\" ]",
        ),
        (
            "group(\"starlight_rust_benchmarks\")",
            "deps = [ \"rust:starlight_rust_benchmarks\" ]",
        ),
        (
            "group(\"starlight_rust_full_tests\")",
            "\"rust:starlight_rust_full_tests\"",
        ),
    ] {
        assert!(
            build.contains(target),
            "root BUILD.gn must contain {target}"
        );
        assert!(
            build.contains(delegate),
            "root BUILD.gn must delegate {target} to the Rust workspace"
        );
    }
}

#[test]
fn starlight_build_exposes_rust_ffi_staticlib_group() {
    let starlight = starlight_source_dir();
    let build = fs::read_to_string(starlight.join("BUILD.gn")).expect("read Starlight BUILD.gn");

    assert!(
        build.contains("group(\"starlight_rust_ffi\")"),
        "core/renderer/starlight/BUILD.gn must expose a Rust FFI build target"
    );
    assert!(
        build.contains("deps = [ \"rust:starlight_rust_ffi\" ]"),
        "the root Rust FFI target must delegate to the Rust workspace BUILD.gn"
    );
    assert!(
        build.contains("group(\"starlight_rust_ffi_link\")"),
        "core/renderer/starlight/BUILD.gn must expose a Rust FFI link target"
    );
    assert!(
        build.contains("public_deps = [ \"rust:starlight_rust_ffi_link\" ]"),
        "the root Rust FFI link target must publicly delegate to the Rust workspace link target"
    );
}

#[test]
fn starlight_build_keeps_rust_out_of_production_target() {
    let starlight = starlight_source_dir();
    let build = fs::read_to_string(starlight.join("BUILD.gn")).expect("read Starlight BUILD.gn");
    let starlight_target =
        braced_body_after(&build, "source_set(\"starlight\")").expect("starlight body exists");
    let full_group = braced_body_after(&build, "group(\"starlight_rust_full_tests\")")
        .expect("full body exists");

    for forbidden_fragment in [
        "enable_rust_starlight_layout",
        "LYNX_STARLIGHT_USE_RUST_LAYOUT",
        "starlight_rust_opt_in_compile_config",
        "starlight_rust_opt_in_compile_smoke",
        "starlight_rust_production_opt_in_tests",
        "rust_glue/layout_object_rust_adapter.h",
        "rust_glue/layout_object_rust_driver.h",
        "rust_glue/layout_object_style_converter.h",
    ] {
        assert!(
            !build.contains(forbidden_fragment),
            "root BUILD.gn must keep Rust standalone instead of exposing production opt-in: {forbidden_fragment}"
        );
    }
    assert!(
        !starlight_target.contains("rust:"),
        "production starlight target must not depend on Rust FFI"
    );
    assert!(
        !full_group.contains("starlight_rust_opt_in_compile_smoke"),
        "Rust full tests must not route through production opt-in smoke"
    );
}

#[test]
fn layout_object_relayout_remains_cpp_only() {
    let starlight = starlight_source_dir();
    let layout_object = fs::read_to_string(starlight.join("layout/layout_object.cc"))
        .expect("read layout_object.cc");
    let header =
        fs::read_to_string(starlight.join("layout/layout_object.h")).expect("read layout_object.h");
    let body = braced_body_after(&layout_object, "void LayoutObject::ReLayoutWithConstraints")
        .expect("ReLayoutWithConstraints body exists");
    let compact_body = without_ascii_whitespace(body);

    for forbidden_fragment in [
        "LYNX_STARLIGHT_USE_RUST_LAYOUT",
        "layout_object_rust_driver.h",
        "configs_.enable_rust_layout_",
        "CanRustLayoutWithFixedNodeSet",
        "CollectFixedDescendantsForRust",
        "TryLayoutWithNodeConstraints",
        "CacheExternalLayoutResultForConstraints",
        "SyncExternalLayoutPositionAndRunPlatformAlignmentRecursive",
        "SLRustLayoutRuntimeIsEnabled",
        "last_rust_layout_status_",
        "ApplyExternalLayoutResult",
    ] {
        assert!(
            !layout_object.contains(forbidden_fragment) && !header.contains(forbidden_fragment),
            "LayoutObject must not carry production Rust fast-path glue: {forbidden_fragment}"
        );
    }

    for required_fragment in [
        "SendLayoutEvent(LayoutEventType::UpdateMeasureBegin);",
        "UpdateMeasure(constraints,true,fixed_node_set);",
        "SendLayoutEvent(LayoutEventType::UpdateMeasureEnd);",
        "UpdateAlignment();",
        "RemoveAlgorithmRecursive();",
        "RoundToPixelGrid(offset_left_,offset_top_,0.f,0.f,false);",
    ] {
        assert!(
            compact_body.contains(&without_ascii_whitespace(required_fragment)),
            "ReLayoutWithConstraints must keep the C++ layout flow: {required_fragment}"
        );
    }
}

#[test]
fn template_config_does_not_decode_rust_layout_switch() {
    let root = repo_root();
    let layout_configs =
        fs::read_to_string(root.join("core/renderer/starlight/types/layout_configs.h"))
            .expect("read layout_configs.h");
    let config_yaml = fs::read_to_string(
        root.join("core/template_bundle/template_codec/binary_decoder/lynx_config.yml"),
    )
    .expect("read lynx_config.yml");
    let decoder = fs::read_to_string(
        root.join("core/template_bundle/template_codec/binary_decoder/lynx_config_decoder.h"),
    )
    .expect("read lynx_config_decoder.h");

    for forbidden_fragment in [
        "enable_rust_layout_",
        "enableRustStarlightLayout",
        "kEnableRustStarlightLayout",
    ] {
        assert!(
            !layout_configs.contains(forbidden_fragment)
                && !config_yaml.contains(forbidden_fragment)
                && !decoder.contains(forbidden_fragment),
            "template config must not expose a runtime Rust layout switch: {forbidden_fragment}"
        );
    }
}

#[test]
fn rust_build_bridge_runs_workspace_test_and_clippy_actions() {
    let starlight = starlight_source_dir();
    let rust_build =
        fs::read_to_string(starlight.join("rust/BUILD.gn")).expect("read Rust BUILD.gn");

    for target in [
        "group(\"starlight_rust_tests\")",
        "action(\"cargo_test_workspace\")",
        "action(\"cargo_clippy_workspace\")",
    ] {
        assert!(
            rust_build.contains(target),
            "rust/BUILD.gn must contain {target}"
        );
    }
    for required_fragment in [
        "scripts/cargo_workspace_action.py",
        "cargo_test_workspace.stamp",
        "cargo_clippy_workspace.stamp",
        "\"test\"",
        "\"clippy\"",
        "\"--workspace\"",
        "\"--all-targets\"",
        "\"-D\"",
        "\"warnings\"",
    ] {
        assert!(
            rust_build.contains(required_fragment),
            "rust/BUILD.gn must contain {required_fragment}"
        );
    }
}

#[test]
fn rust_workspace_exposes_safe_standalone_crate() {
    let starlight = starlight_source_dir();
    let workspace = starlight.join("rust");
    let cargo = fs::read_to_string(workspace.join("Cargo.toml")).expect("read Rust Cargo.toml");
    let standalone_manifest =
        fs::read_to_string(workspace.join("crates/starlight_standalone/Cargo.toml"))
            .expect("read starlight_standalone Cargo.toml");
    let standalone_root =
        fs::read_to_string(workspace.join("crates/starlight_standalone/src/lib.rs"))
            .expect("read starlight_standalone lib.rs");
    let layout_engine = fs::read_to_string(workspace.join("crates/starlight_layout/src/engine.rs"))
        .expect("read starlight_layout engine.rs");
    let readme = fs::read_to_string(workspace.join("README.md")).expect("read Rust README.md");

    for required_fragment in [
        "\"crates/starlight_standalone\"",
        "starlight_standalone",
        "`crates/starlight_standalone`",
    ] {
        assert!(
            cargo.contains(required_fragment)
                || standalone_manifest.contains(required_fragment)
                || readme.contains(required_fragment),
            "Rust workspace must expose the standalone crate contract: {required_fragment}"
        );
    }

    for required_fragment in [
        "#![forbid(unsafe_code)]",
        "pub enum StandaloneEdge",
        "pub enum StandaloneGap",
        "Horizontal",
        "Vertical",
        "All",
        "pub struct StandaloneTree",
        "pub fn standalone_default_style() -> Style",
        "pub fn style_display(",
        "pub fn style_direction(",
        "pub fn style_position_type(",
        "pub fn style_box_sizing(",
        "pub fn style_flex_direction(",
        "pub fn style_flex_wrap(",
        "pub fn style_justify_content(",
        "pub fn style_align_content(",
        "pub fn style_align_items(",
        "pub fn style_align_self(",
        "pub fn style_justify_items(",
        "pub fn style_justify_self(",
        "pub fn style_aspect_ratio(",
        "pub fn style_order(",
        "pub fn style_flex_grow(",
        "pub fn style_flex_shrink(",
        "pub fn style_width(",
        "pub fn style_height(",
        "pub fn style_min_width(",
        "pub fn style_min_height(",
        "pub fn style_max_width(",
        "pub fn style_max_height(",
        "pub fn style_flex_basis(",
        "pub fn style_position(",
        "pub fn style_margin(",
        "pub fn style_padding(",
        "pub fn style_border(",
        "pub fn style_gap(",
        "pub fn style_row_gap(",
        "pub fn style_column_gap(",
        "pub fn style_linear_orientation(",
        "pub fn style_linear_gravity(",
        "pub fn style_linear_layout_gravity(",
        "pub fn style_linear_cross_gravity(",
        "pub fn style_linear_weight(",
        "pub fn style_linear_weight_sum(",
        "pub fn style_linear_column_count(",
        "pub fn style_list_main_axis_gap(",
        "pub fn style_list_cross_axis_gap(",
        "pub fn style_list_component_type(",
        "pub fn style_grid_template_columns(",
        "pub fn style_grid_template_rows(",
        "pub fn style_grid_template_columns_max(",
        "pub fn style_grid_template_rows_max(",
        "pub fn style_grid_auto_columns(",
        "pub fn style_grid_auto_rows(",
        "pub fn style_grid_auto_columns_max(",
        "pub fn style_grid_auto_rows_max(",
        "pub fn style_grid_auto_flow(",
        "pub fn style_grid_column_start(",
        "pub fn style_grid_column_end(",
        "pub fn style_grid_row_start(",
        "pub fn style_grid_row_end(",
        "pub fn style_grid_column_span(",
        "pub fn style_grid_row_span(",
        "pub fn style_relative_id(",
        "pub fn style_relative_align_top(",
        "pub fn style_relative_align_right(",
        "pub fn style_relative_align_bottom(",
        "pub fn style_relative_align_left(",
        "pub fn style_relative_top_of(",
        "pub fn style_relative_right_of(",
        "pub fn style_relative_bottom_of(",
        "pub fn style_relative_left_of(",
        "pub fn style_relative_layout_once(",
        "pub fn style_relative_center(",
        "pub fn set_display(",
        "pub fn set_direction(",
        "pub fn set_position_type(",
        "pub fn set_box_sizing(",
        "pub fn set_flex_direction(",
        "pub fn set_flex_wrap(",
        "pub fn set_justify_content(",
        "pub fn set_align_content(",
        "pub fn set_align_items(",
        "pub fn set_align_self(",
        "pub fn set_justify_items(",
        "pub fn set_justify_self(",
        "pub fn set_aspect_ratio(",
        "pub fn set_order(",
        "pub fn set_flex_grow(",
        "pub fn set_flex_shrink(",
        "pub fn set_gap(",
        "pub fn set_row_gap(",
        "pub fn set_column_gap(",
        "pub fn set_linear_orientation(",
        "pub fn set_linear_gravity(",
        "pub fn set_linear_layout_gravity(",
        "pub fn set_linear_cross_gravity(",
        "pub fn set_linear_weight(",
        "pub fn set_linear_weight_sum(",
        "pub fn set_linear_column_count(",
        "pub fn set_list_main_axis_gap(",
        "pub fn set_list_cross_axis_gap(",
        "pub fn set_list_component_type(",
        "pub fn set_grid_template_columns(",
        "pub fn set_grid_template_rows(",
        "pub fn set_grid_template_columns_max(",
        "pub fn set_grid_template_rows_max(",
        "pub fn set_grid_auto_columns(",
        "pub fn set_grid_auto_rows(",
        "pub fn set_grid_auto_columns_max(",
        "pub fn set_grid_auto_rows_max(",
        "pub fn set_grid_auto_flow(",
        "pub fn set_grid_column_start(",
        "pub fn set_grid_column_end(",
        "pub fn set_grid_row_start(",
        "pub fn set_grid_row_end(",
        "pub fn set_grid_column_span(",
        "pub fn set_grid_row_span(",
        "pub fn set_relative_id(",
        "pub fn set_relative_align_top(",
        "pub fn set_relative_align_right(",
        "pub fn set_relative_align_bottom(",
        "pub fn set_relative_align_left(",
        "pub fn set_relative_top_of(",
        "pub fn set_relative_right_of(",
        "pub fn set_relative_bottom_of(",
        "pub fn set_relative_left_of(",
        "pub fn set_relative_layout_once(",
        "pub fn set_relative_center(",
        "pub fn set_measured_size(",
        "pub fn measured_size(&self, node: NodeId) -> Result<Option<Size>, TreeError>",
        "pub fn has_measure_func(&self, node: NodeId) -> Result<bool, TreeError>",
        "pub fn set_baseline(&mut self, node: NodeId, baseline: Option<f32>) -> Result<(), TreeError>",
        "pub fn baseline(&self, node: NodeId) -> Result<Option<f32>, TreeError>",
        "pub struct StandaloneConfig",
        "pub fn create_default_node_with_config(",
        "pub fn node_config(&self, node: NodeId) -> Result<StandaloneConfig, TreeError>",
        "pub fn physical_pixels_per_layout_unit(&self, node: NodeId) -> Result<f32, TreeError>",
        "pub fn set_position(",
        "pub fn set_margin(",
        "pub fn set_padding(",
        "pub fn set_border(",
        "pub fn set_width(",
        "pub fn set_height(",
        "pub fn set_min_width(",
        "pub fn set_min_height(",
        "pub fn set_max_width(",
        "pub fn set_max_height(",
        "pub fn set_flex(&mut self, node: NodeId, value: f32) -> Result<(), TreeError>",
        "pub fn set_flex_basis(",
        "pub fn layout_left(&self, node: NodeId) -> Result<f32, TreeError>",
        "pub fn layout_top(&self, node: NodeId) -> Result<f32, TreeError>",
        "pub fn layout_width(&self, node: NodeId) -> Result<f32, TreeError>",
        "pub fn layout_height(&self, node: NodeId) -> Result<f32, TreeError>",
        "pub fn layout_baseline(&self, node: NodeId) -> Result<f32, TreeError>",
        "pub fn layout_margin(&self, node: NodeId, edge: StandaloneEdge) -> Result<f32, TreeError>",
        "pub fn layout_padding(&self, node: NodeId, edge: StandaloneEdge) -> Result<f32, TreeError>",
        "pub fn layout_border(&self, node: NodeId, edge: StandaloneEdge) -> Result<f32, TreeError>",
        "pub fn layout_sticky_position(",
        "fn resolve_edge(edges: Edges, edge: StandaloneEdge, is_rtl: bool) -> f32",
        "fn resolve_style_length_edge(edges: Rect<Length>, edge: StandaloneEdge, is_rtl: bool) -> Length",
        "fn resolve_style_edge(edges: Edges, edge: StandaloneEdge, is_rtl: bool) -> f32",
        "pub fn child_count(&self, node: NodeId) -> Result<usize, TreeError>",
        "pub fn child_at(&self, node: NodeId, index: usize) -> Result<Option<NodeId>, TreeError>",
        "pub fn child_at_standalone_index(",
        "pub fn is_rtl(&self, node: NodeId) -> Result<bool, TreeError>",
        "pub fn is_dirty(&self, node: NodeId) -> Result<bool, TreeError>",
        "pub fn mark_dirty(&mut self, node: NodeId) -> Result<(), TreeError>",
        "pub fn insert_child_or_append(",
        "pub fn insert_child_at_standalone_index(",
        "pub fn insert_child_before_or_append(",
        "pub fn reset_node(&mut self, node: NodeId) -> Result<(), TreeError>",
        "pub fn calculate_layout_with_mode(",
        "fn update_style(",
        "fn detach_from_parent(&mut self, child: NodeId)",
        "self.detach_from_parent(child);",
        "fn apply_position_edge(",
        "fn apply_length_edge(",
        "fn apply_f32_edge(",
        "impl LayoutTree for StandaloneTree",
        "LayoutEngine::new().layout_with_owner_constraints",
    ] {
        assert!(
            standalone_root.contains(required_fragment),
            "starlight_standalone must keep a safe owned-tree wrapper over the LayoutTree engine: {required_fragment}"
        );
    }

    for required_fragment in [
        "fn physical_pixels_per_layout_unit(&self, _node: Self::NodeId) -> f32",
        "fn round_to_pixel_grid(value: f32, physical_pixels_per_layout_unit: f32) -> f32",
        "fn ceil_to_pixel_grid(value: f32, physical_pixels_per_layout_unit: f32) -> f32",
    ] {
        assert!(
            layout_engine.contains(required_fragment),
            "starlight_layout must keep external-tree physical pixel rounding support: {required_fragment}"
        );
    }
}

#[test]
fn rust_build_bridge_exposes_native_parity_and_benchmark_gates() {
    let starlight = starlight_source_dir();
    let rust_build =
        fs::read_to_string(starlight.join("rust/BUILD.gn")).expect("read Rust BUILD.gn");

    for target in [
        "group(\"starlight_rust_native_parity_tests\")",
        "group(\"starlight_rust_cpp_import_tests\")",
        "group(\"starlight_rust_ffi_tests\")",
        "group(\"starlight_rust_benchmarks\")",
        "group(\"starlight_rust_full_tests\")",
        "action(\"cargo_test_native_head_to_head\")",
        "action(\"cargo_test_cpp_native_contracts\")",
        "action(\"cargo_test_ffi_link_smoke\")",
        "action(\"cargo_bench_cpp_baseline\")",
    ] {
        assert!(
            rust_build.contains(target),
            "rust/BUILD.gn must contain {target}"
        );
    }

    for required_fragment in [
        "cargo_test_native_head_to_head.stamp",
        "STARLIGHT_CPP_NATIVE_STANDALONE_BUILD_FROM_SOURCE=1",
        "STARLIGHT_GENERATED_CASE_COUNT=32768",
        "\"test\"",
        "\"-p\"",
        "\"starlight_parity\"",
        "\"--features\"",
        "\"native-standalone\"",
        "\"--test\"",
        "\"native_head_to_head_tests\"",
        "\"native_generated_head_to_head_tests\"",
        "\"standalone_head_to_head_tests\"",
    ] {
        assert!(
            rust_build.contains(required_fragment),
            "native parity GN action must contain {required_fragment}"
        );
    }

    for required_fragment in [
        "cargo_test_cpp_native_contracts.stamp",
        "STARLIGHT_CPP_NATIVE_STANDALONE_CHECK=1",
        "\"test\"",
        "\"-p\"",
        "\"starlight_cpp\"",
        "\"--features\"",
        "\"native-standalone\"",
    ] {
        assert!(
            rust_build.contains(required_fragment),
            "C++ import contract GN action must contain {required_fragment}"
        );
    }

    for required_fragment in [
        "cargo_test_ffi_link_smoke.stamp",
        "deps = [ \":cargo_build_starlight_ffi_staticlib\" ]",
        "STARLIGHT_RUST_FFI_LIBRARY=",
        "\"test\"",
        "\"-p\"",
        "\"starlight_ffi\"",
        "\"--test\"",
        "\"c_abi_smoke_tests\"",
    ] {
        assert!(
            rust_build.contains(required_fragment),
            "FFI link smoke GN action must contain {required_fragment}"
        );
    }

    for required_fragment in [
        ":cargo_test_ffi_link_smoke",
        ":cargo_test_cpp_native_contracts",
        ":cargo_test_native_head_to_head",
        ":cargo_bench_cpp_baseline",
    ] {
        assert!(
            rust_build.contains(required_fragment),
            "full Rust Starlight GN gate must contain {required_fragment}"
        );
    }

    for required_fragment in [
        "cargo_bench_cpp_baseline.stamp",
        "STARLIGHT_BENCH_MIN_SPEEDUP=1.0",
        "STARLIGHT_BENCH_REQUIRE_CPP_BASELINE=1",
        "STARLIGHT_CPP_NATIVE_STANDALONE_BUILD_FROM_SOURCE=1",
        "\"run\"",
        "\"--release\"",
        "\"-p\"",
        "\"starlight_bench\"",
        "\"--features\"",
        "\"native-standalone\"",
        "\"1000\"",
        "\"200\"",
        "\"10\"",
    ] {
        assert!(
            rust_build.contains(required_fragment),
            "benchmark GN action must contain {required_fragment}"
        );
    }
}

#[test]
fn rust_benchmark_gn_action_requires_source_built_cpp_baseline_and_release_speedup_gate() {
    let starlight = starlight_source_dir();
    let rust_build =
        fs::read_to_string(starlight.join("rust/BUILD.gn")).expect("read Rust BUILD.gn");
    let benchmark_body = braced_body_after(&rust_build, "action(\"cargo_bench_cpp_baseline\")")
        .expect("cargo_bench_cpp_baseline action body exists");
    let compact_benchmark = without_ascii_whitespace(benchmark_body);

    for required_fragment in [
        "env=[\"STARLIGHT_BENCH_MIN_SPEEDUP=1.0\",\"STARLIGHT_BENCH_REQUIRE_CPP_BASELINE=1\",\"STARLIGHT_CPP_NATIVE_STANDALONE_BUILD_FROM_SOURCE=1\",]",
        "\"run\",\"--release\",\"-p\",\"starlight_bench\",\"--features\",\"native-standalone\",\"--\",\"1000\",\"200\",\"10\",",
        "cargo_bench_cpp_baseline.stamp",
    ] {
        assert!(
            compact_benchmark.contains(&without_ascii_whitespace(required_fragment)),
            "cargo_bench_cpp_baseline must keep the source-built C++ baseline, required speedup gate, release run, and stable workload: {required_fragment}"
        );
    }

    let benchmark_group = braced_body_after(&rust_build, "group(\"starlight_rust_benchmarks\")")
        .expect("starlight_rust_benchmarks group body exists");
    assert!(
        without_ascii_whitespace(benchmark_group).contains(&without_ascii_whitespace(
            "deps = [ \":cargo_bench_cpp_baseline\" ]"
        )),
        "starlight_rust_benchmarks must execute the release Rust-vs-C++ benchmark gate"
    );

    let full_group = braced_body_after(&rust_build, "group(\"starlight_rust_full_tests\")")
        .expect("starlight_rust_full_tests group body exists");
    assert!(
        without_ascii_whitespace(full_group)
            .contains(&without_ascii_whitespace("\":cargo_bench_cpp_baseline\"")),
        "starlight_rust_full_tests must include the benchmark gate"
    );
}

#[test]
fn rust_build_bridge_exports_ffi_staticlib_action() {
    let starlight = starlight_source_dir();
    let rust_build =
        fs::read_to_string(starlight.join("rust/BUILD.gn")).expect("read Rust BUILD.gn");

    for target in [
        "group(\"starlight_rust_ffi\")",
        "source_set(\"starlight_rust_ffi_link\")",
        "config(\"starlight_rust_ffi_link_config\")",
        "action(\"cargo_build_starlight_ffi_staticlib\")",
    ] {
        assert!(
            rust_build.contains(target),
            "rust/BUILD.gn must contain {target}"
        );
    }
    for required_fragment in [
        "scripts/cargo_staticlib_action.py",
        "libstarlight_ffi.a",
        "starlight_ffi.lib",
        "crates/starlight_ffi/include",
        "public_configs = [ \":starlight_rust_ffi_link_config\" ]",
        "libs = [ rebase_path(starlight_rust_ffi_staticlib, root_build_dir) ]",
        "\"--package\"",
        "\"starlight_ffi\"",
        "\"--output\"",
    ] {
        assert!(
            rust_build.contains(required_fragment),
            "rust/BUILD.gn must contain {required_fragment}"
        );
    }
}

#[test]
fn rust_build_bridge_publishes_c_and_cpp_ffi_headers() {
    let starlight = starlight_source_dir();
    let rust_build =
        fs::read_to_string(starlight.join("rust/BUILD.gn")).expect("read Rust BUILD.gn");

    for required_fragment in [
        "\"crates/starlight_ffi/include/starlight_rust_ffi.h\"",
        "\"crates/starlight_ffi/include/starlight_rust_ffi_cpp.h\"",
        "include_dirs = [ \"crates/starlight_ffi/include\" ]",
        "public_configs = [ \":starlight_rust_ffi_link_config\" ]",
    ] {
        assert!(
            rust_build.contains(required_fragment),
            "Rust FFI GN link target must publish public C/C++ headers through the link config: {required_fragment}"
        );
    }
}

#[test]
fn rust_gn_bridge_runner_exists() {
    let runner = starlight_source_dir().join("rust/scripts/cargo_workspace_action.py");
    let source = fs::read_to_string(&runner).expect("read Cargo workspace action runner");
    assert!(
        source.contains("CARGO_TARGET_DIR"),
        "runner must redirect cargo artifacts into the GN output tree"
    );
    assert!(
        source.contains("absolutize_env_path(env, \"STARLIGHT_RUST_FFI_LIBRARY\")"),
        "runner must resolve the GN-provided FFI staticlib path before cargo changes cwd"
    );
    assert!(
        source.contains("stamp.write(\"ok\\n\")"),
        "runner must write the declared GN stamp output after success"
    );
}

#[test]
fn rust_staticlib_gn_bridge_runner_copies_cargo_staticlib() {
    let runner = starlight_source_dir().join("rust/scripts/cargo_staticlib_action.py");
    let source = fs::read_to_string(&runner).expect("read Cargo staticlib action runner");
    for required_fragment in [
        "\"cargo\", \"build\", \"-p\", args.package, \"--release\"",
        "CARGO_TARGET_DIR",
        "find_staticlib",
        "lib{package}.a",
        "{package}.lib",
        "shutil.copyfile",
    ] {
        assert!(
            source.contains(required_fragment),
            "staticlib runner must contain {required_fragment}"
        );
    }
}

fn starlight_source_dir() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../..")
        .canonicalize()
        .expect("resolve core/renderer/starlight")
}

fn repo_root() -> PathBuf {
    starlight_source_dir()
        .ancestors()
        .nth(3)
        .expect("resolve repository root")
        .to_path_buf()
}

fn braced_body_after<'a>(source: &'a str, marker: &str) -> Option<&'a str> {
    let marker_start = source.find(marker)?;
    let after_marker = &source[marker_start..];
    let body_start = after_marker.find('{')? + marker_start + 1;
    let mut depth = 1usize;
    for (offset, byte) in source[body_start..].bytes().enumerate() {
        match byte {
            b'{' => depth += 1,
            b'}' => {
                depth -= 1;
                if depth == 0 {
                    return Some(&source[body_start..body_start + offset]);
                }
            }
            _ => {}
        }
    }
    None
}

fn without_ascii_whitespace(source: &str) -> String {
    source
        .chars()
        .filter(|character| !character.is_ascii_whitespace())
        .collect()
}
