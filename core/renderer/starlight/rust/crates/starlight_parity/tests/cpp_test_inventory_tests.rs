// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::collections::BTreeSet;
use std::fs;
use std::path::{Path, PathBuf};

#[test]
fn every_cpp_starlight_unittest_source_is_in_starlight_testset() {
    let starlight_dir = starlight_source_dir();
    let build = fs::read_to_string(starlight_dir.join("BUILD.gn")).expect("read BUILD.gn");

    assert_eq!(
        cpp_unittest_sources_on_disk(&starlight_dir),
        starlight_testset_sources(&build),
        "every C++ Starlight unittest source under core/renderer/starlight must be part of starlight_testset so Rust parity inventory covers it"
    );
}

#[test]
fn current_cpp_starlight_tests_are_marked_translated_in_inventory() {
    let starlight_dir = starlight_source_dir();
    let build = fs::read_to_string(starlight_dir.join("BUILD.gn")).expect("read BUILD.gn");
    let inventory = fs::read_to_string(
        Path::new(env!("CARGO_MANIFEST_DIR")).join("CURRENT_CPP_TEST_INVENTORY.md"),
    )
    .expect("read Rust C++ test inventory");

    let testset_sources = inventory_scoped_cpp_test_sources(&starlight_dir, &build);
    assert_eq!(
        testset_sources,
        inventory_sources(&inventory),
        "inventory source sections must match renderer starlight_testset sources plus any standalone Starlight C++ tests"
    );

    for source in testset_sources {
        let source_text =
            fs::read_to_string(starlight_dir.join(&source)).expect("read C++ test source");
        let cpp_tests = cpp_gtest_cases(&source_text);
        assert_no_duplicate_translated_inventory_cases(&inventory, &source);
        let inventory_tests = inventory_translated_cases(&inventory, &source);
        assert_eq!(
            cpp_tests, inventory_tests,
            "translated inventory entries must match current C++ tests in {source}"
        );
    }
}

#[test]
fn grid_head_to_head_status_matches_grid_gap_inventory() {
    let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
    let cpp_inventory = fs::read_to_string(manifest_dir.join("CURRENT_CPP_TEST_INVENTORY.md"))
        .expect("read Rust C++ test inventory");
    let grid_inventory = fs::read_to_string(manifest_dir.join("GRID_ALGORITHM_COVERAGE.md"))
        .expect("read Rust grid algorithm inventory");

    let status = grid_head_to_head_status_section(&cpp_inventory);
    let normalized_status = collapse_ascii_whitespace(status);
    let (native_ignored, standalone_ignored) = ignored_grid_gap_counts(&grid_inventory);
    let expected_counts = format!(
        "{native_ignored} native ignored gap records and {standalone_ignored} standalone ignored gap records"
    );

    assert!(
        normalized_status.contains(&expected_counts),
        "grid head-to-head status must summarize exact ignored gap counts from GRID_ALGORITHM_COVERAGE.md"
    );
    assert!(
        status.contains("GRID_ALGORITHM_COVERAGE.md")
            && status.contains("ignored-head-to-head"),
        "grid head-to-head status must delegate exact known parity differences to the grid coverage inventory"
    );
    assert!(
        status.contains("source-built the standalone C++ library"),
        "grid head-to-head status must record source-built standalone C++ verification"
    );
    assert!(
        status.contains("starlight_cpp --features native-standalone")
            && status.contains("source-built the standalone C++ native library"),
        "grid head-to-head status must record an explicit source-built standalone C++ native library test"
    );
    assert!(
        !status.contains("1 failed") && !status.contains("standalone failure"),
        "grid head-to-head status must not keep stale standalone failure wording"
    );
}

#[test]
fn translated_inventory_entries_point_to_existing_rust_tests() {
    let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
    let inventory = fs::read_to_string(manifest_dir.join("CURRENT_CPP_TEST_INVENTORY.md"))
        .expect("read Rust C++ test inventory");

    let mut missing = Vec::new();
    for (cpp_case, rust_target) in inventory_translated_mappings(&inventory) {
        let Some((rust_file, rust_test)) = rust_target.split_once("::") else {
            missing.push(format!("{cpp_case}: malformed Rust target `{rust_target}`"));
            continue;
        };
        if !is_top_level_cargo_integration_test(rust_file) {
            missing.push(format!(
                "{cpp_case}: Rust target `{rust_target}` must point at a top-level Cargo integration test file under `tests/*.rs`"
            ));
            continue;
        }
        let rust_path = manifest_dir.join(rust_file);
        let Ok(source) = fs::read_to_string(&rust_path) else {
            missing.push(format!("{cpp_case}: missing Rust file `{rust_file}`"));
            continue;
        };
        if !rust_test_functions(&source).contains(rust_test) {
            missing.push(format!(
                "{cpp_case}: missing Rust test `{rust_file}::{rust_test}`"
            ));
            continue;
        }
        let Some(test_body) = rust_test_function_body(&source, rust_test) else {
            missing.push(format!(
                "{cpp_case}: could not read Rust test body `{rust_file}::{rust_test}`"
            ));
            continue;
        };
        if !rust_test_body_has_observable_check(test_body) {
            missing.push(format!(
                "{cpp_case}: Rust test `{rust_file}::{rust_test}` must contain an assertion or explicit panic check"
            ));
        }
    }

    assert!(
        missing.is_empty(),
        "translated C++ inventory entries must point at existing Rust tests:\n{}",
        missing.join("\n")
    );
}

fn starlight_source_dir() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../..")
        .canonicalize()
        .expect("resolve core/renderer/starlight")
}

fn starlight_testset_sources(build: &str) -> BTreeSet<String> {
    let target_start = build
        .find("unittest_set(\"starlight_testset\")")
        .expect("starlight_testset target exists");
    let target = &build[target_start..];
    let sources_start = target
        .find("sources = [")
        .expect("starlight_testset declares sources");
    let sources = &target[sources_start..];
    let list_start = sources.find('[').expect("sources list starts");
    let list_end = sources.find(']').expect("sources list ends");

    sources[list_start + 1..list_end]
        .lines()
        .filter_map(quoted_value)
        .collect()
}

fn inventory_scoped_cpp_test_sources(starlight_dir: &Path, build: &str) -> BTreeSet<String> {
    let mut sources = starlight_testset_sources(build);
    sources.extend(standalone_cpp_test_sources(starlight_dir));
    sources
}

fn standalone_cpp_test_sources(starlight_dir: &Path) -> BTreeSet<String> {
    let mut sources = BTreeSet::new();
    for root in [
        "../../services/starlight_standalone",
        "../../include/starlight_standalone",
    ] {
        let root_path = starlight_dir.join(root);
        if !root_path.exists() {
            continue;
        }
        sources.extend(
            cpp_unittest_sources_on_disk(&root_path)
                .into_iter()
                .map(|source| format!("{root}/{source}")),
        );
    }
    sources
}

fn cpp_unittest_sources_on_disk(starlight_dir: &Path) -> BTreeSet<String> {
    let mut sources = BTreeSet::new();
    collect_cpp_unittest_sources(starlight_dir, starlight_dir, &mut sources);
    sources
}

fn collect_cpp_unittest_sources(root: &Path, dir: &Path, sources: &mut BTreeSet<String>) {
    for entry in fs::read_dir(dir).expect("read Starlight source dir") {
        let entry = entry.expect("read Starlight source entry");
        let path = entry.path();
        let file_type = entry.file_type().expect("read Starlight source file type");
        if file_type.is_dir() {
            collect_cpp_unittest_sources(root, &path, sources);
            continue;
        }
        if !file_type.is_file() {
            continue;
        }

        let Some(file_name) = path.file_name().and_then(|name| name.to_str()) else {
            continue;
        };
        if !(file_name.ends_with("_unittest.cc") || file_name.ends_with("_test.cc")) {
            continue;
        }

        let source = path
            .strip_prefix(root)
            .expect("unittest source is below Starlight root")
            .to_string_lossy()
            .replace('\\', "/");
        assert!(sources.insert(source.clone()), "duplicate source {source}");
    }
}

fn inventory_sources(inventory: &str) -> BTreeSet<String> {
    inventory
        .lines()
        .filter_map(|line| {
            let line = line.strip_prefix("## `")?;
            let (source, _) = line.split_once('`')?;
            Some(source.to_owned())
        })
        .collect()
}

fn grid_head_to_head_status_section(inventory: &str) -> &str {
    let header = "## Grid Head-to-Head Status";
    let start = inventory
        .find(header)
        .expect("grid head-to-head status section exists");
    let section = &inventory[start + header.len()..];
    let end = section.find("\n## `").unwrap_or(section.len());
    &section[..end]
}

fn ignored_grid_gap_counts(grid_inventory: &str) -> (usize, usize) {
    let mut native = 0;
    let mut standalone = 0;
    for line in grid_inventory.lines() {
        if !line.starts_with("- `ignored-head-to-head`: ") {
            continue;
        }
        if line.contains("`tests/native_head_to_head_tests.rs::") {
            native += 1;
        } else if line.contains("`tests/standalone_head_to_head_tests.rs::") {
            standalone += 1;
        }
    }

    (native, standalone)
}

fn collapse_ascii_whitespace(text: &str) -> String {
    text.split_whitespace().collect::<Vec<_>>().join(" ")
}

fn cpp_gtest_cases(source: &str) -> BTreeSet<String> {
    let mut cases = BTreeSet::new();
    let mut cursor = 0;
    while let Some(relative_start) = source[cursor..].find("TEST") {
        let start = cursor + relative_start;
        if !is_gtest_macro_boundary(source, start) {
            cursor = start + "TEST".len();
            continue;
        }
        let Some(arguments_start) = gtest_macro_arguments_start(&source[start..]) else {
            cursor = start + "TEST".len();
            continue;
        };
        let arguments_start = start + arguments_start;
        let arguments_end = arguments_start
            + source[arguments_start..]
                .find(')')
                .expect("gtest macro argument list ends");
        let arguments = &source[arguments_start..arguments_end];
        let mut parts = arguments.splitn(3, ',');
        let suite = parts
            .next()
            .expect("gtest suite")
            .split_whitespace()
            .collect::<String>();
        let name = parts
            .next()
            .expect("gtest name")
            .split_whitespace()
            .collect::<String>();
        let case = format!("{suite}.{name}");
        assert!(
            cases.insert(case.clone()),
            "duplicate C++ gtest case {case}"
        );
        cursor = arguments_end + 1;
    }
    cases
}

fn gtest_macro_arguments_start(text: &str) -> Option<usize> {
    ["TEST(", "TEST_F(", "TEST_P("]
        .into_iter()
        .find_map(|macro_name| text.starts_with(macro_name).then_some(macro_name.len()))
}

fn is_gtest_macro_boundary(source: &str, start: usize) -> bool {
    source[..start]
        .chars()
        .next_back()
        .map(|previous| !previous.is_ascii_alphanumeric() && previous != '_')
        .unwrap_or(true)
}

fn inventory_translated_cases(inventory: &str, source: &str) -> BTreeSet<String> {
    inventory_translated_case_list(inventory, source)
        .into_iter()
        .map(|entry| entry.case)
        .collect()
}

fn assert_no_duplicate_translated_inventory_cases(inventory: &str, source: &str) {
    let mut seen = BTreeSet::new();
    for entry in inventory_translated_case_list(inventory, source) {
        assert!(
            seen.insert(entry.case.clone()),
            "duplicate translated inventory entry {} in {source}",
            entry.case
        );
    }
}

fn inventory_translated_mappings(inventory: &str) -> Vec<(String, String)> {
    inventory
        .lines()
        .filter_map(translated_inventory_entry)
        .map(|entry| {
            let rust_target = entry.rust_target.unwrap_or_else(|| {
                panic!(
                    "translated inventory entry {} must map to a Rust test",
                    entry.case
                )
            });
            (entry.case, rust_target)
        })
        .collect()
}

fn inventory_translated_case_list(inventory: &str, source: &str) -> Vec<TranslatedInventoryEntry> {
    let header = format!("## `{source}`");
    let section_start = inventory.find(&header).expect("inventory section exists");
    let section = &inventory[section_start + header.len()..];
    let section_end = section.find("\n## `").unwrap_or(section.len());

    section[..section_end]
        .lines()
        .filter_map(translated_inventory_entry)
        .collect()
}

#[derive(Debug, Eq, PartialEq)]
struct TranslatedInventoryEntry {
    case: String,
    rust_target: Option<String>,
}

fn translated_inventory_entry(line: &str) -> Option<TranslatedInventoryEntry> {
    let line = line.trim_start().strip_prefix("- `[translated] ")?;
    let line = line.trim_end_matches('`');
    let (case, rust_target) = match line.split_once(" -> ") {
        Some((case, rust_target)) => (case, Some(rust_target.to_owned())),
        None => (line, None),
    };
    Some(TranslatedInventoryEntry {
        case: case.to_owned(),
        rust_target,
    })
}

fn is_top_level_cargo_integration_test(rust_file: &str) -> bool {
    let path = Path::new(rust_file);
    let mut components = path.components();
    let Some(std::path::Component::Normal(root)) = components.next() else {
        return false;
    };
    if root != "tests" {
        return false;
    }
    let Some(std::path::Component::Normal(file_name)) = components.next() else {
        return false;
    };
    if components.next().is_some() {
        return false;
    }

    Path::new(file_name)
        .extension()
        .is_some_and(|ext| ext == "rs")
}

fn rust_test_functions(source: &str) -> BTreeSet<&str> {
    let mut tests = BTreeSet::new();
    let mut pending_test_attr = false;
    for line in source.lines().map(str::trim) {
        if line == "#[test]" {
            pending_test_attr = true;
            continue;
        }
        if !pending_test_attr {
            continue;
        }
        if line.is_empty() || line.starts_with("#[") {
            continue;
        }
        if let Some(function) = line
            .strip_prefix("fn ")
            .and_then(|rest| rest.split_once('('))
            .map(|(name, _)| name)
        {
            tests.insert(function);
        }
        pending_test_attr = false;
    }
    tests
}

fn rust_test_function_body<'a>(source: &'a str, function_name: &str) -> Option<&'a str> {
    let needle = format!("fn {function_name}(");
    let function_start = source.find(&needle)?;
    let body_open = function_start + source[function_start..].find('{')?;
    let body_start = body_open + 1;
    let mut depth = 1usize;
    for (offset, character) in source[body_start..].char_indices() {
        match character {
            '{' => depth += 1,
            '}' => {
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

fn rust_test_body_has_observable_check(body: &str) -> bool {
    body.contains("assert") || body.contains("catch_unwind")
}

fn quoted_value(line: &str) -> Option<String> {
    let line = line.trim();
    let line = line.strip_prefix('"')?;
    let (value, _) = line.split_once('"')?;
    Some(value.to_owned())
}

#[test]
fn cpp_gtest_cases_parses_multiline_and_fixture_macros() {
    let cases = cpp_gtest_cases(
        r#"
        TEST(SimpleSuite, PlainCase) {}
        TEST_F(FixtureSuite, FixtureCase) {}
        TEST_P(ParamSuite,
               ParamCase) {}
        // Substrings such as MY_TEST(Foo, Bar) are not gtest cases.
        "#,
    );

    assert_eq!(
        cases,
        BTreeSet::from([
            "FixtureSuite.FixtureCase".to_owned(),
            "ParamSuite.ParamCase".to_owned(),
            "SimpleSuite.PlainCase".to_owned(),
        ])
    );
}

#[test]
fn translated_inventory_entry_parses_optional_rust_target() {
    assert_eq!(
        translated_inventory_entry(
            "- `[translated] Suite.Case -> tests/example_tests.rs::suite_case`"
        ),
        Some(TranslatedInventoryEntry {
            case: "Suite.Case".to_owned(),
            rust_target: Some("tests/example_tests.rs::suite_case".to_owned()),
        })
    );
    assert_eq!(
        translated_inventory_entry("- `[translated] Suite.Case`"),
        Some(TranslatedInventoryEntry {
            case: "Suite.Case".to_owned(),
            rust_target: None,
        })
    );
}

#[test]
fn translated_inventory_targets_must_be_top_level_integration_tests() {
    assert!(is_top_level_cargo_integration_test(
        "tests/example_tests.rs"
    ));
    assert!(!is_top_level_cargo_integration_test("src/lib.rs"));
    assert!(!is_top_level_cargo_integration_test("tests/helpers/mod.rs"));
    assert!(!is_top_level_cargo_integration_test(
        "tests/example_tests.txt"
    ));
    assert!(!is_top_level_cargo_integration_test(
        "../tests/example_tests.rs"
    ));
}

#[test]
fn rust_test_functions_only_collects_test_annotated_functions() {
    let source = r#"
        fn helper() {}
        #[test]
        fn actual_test() {}
        #[test]
        #[should_panic]
        fn panic_test() {}
    "#;
    assert_eq!(
        rust_test_functions(source),
        BTreeSet::from(["actual_test", "panic_test"])
    );
}

#[test]
fn rust_test_function_body_extracts_nested_blocks() {
    let source = r#"
        #[test]
        fn actual_test() {
            let value = {
                let nested = 1;
                nested + 1
            };
            assert_eq!(value, 2);
        }

        fn helper() {}
    "#;

    let body = rust_test_function_body(source, "actual_test").expect("test body");
    assert!(body.contains("let nested = 1;"));
    assert!(body.contains("assert_eq!(value, 2);"));
}

#[test]
fn rust_test_body_observable_check_requires_assert_or_panic_probe() {
    assert!(rust_test_body_has_observable_check(
        "let value = 1; assert_eq!(value, 1);"
    ));
    assert!(rust_test_body_has_observable_check(
        "let result = catch_unwind(|| panic!());"
    ));
    assert!(!rust_test_body_has_observable_check("let value = 1;"));
}

#[test]
#[should_panic(expected = "duplicate translated inventory entry Suite.Case in sample.cc")]
fn duplicate_inventory_entries_are_rejected() {
    assert_no_duplicate_translated_inventory_cases(
        r#"
        ## `sample.cc`

        - `[translated] Suite.Case`
        - `[translated] Suite.Case`
        "#,
        "sample.cc",
    );
}
