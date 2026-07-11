// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#![forbid(unsafe_code)]

use std::fs;
use std::path::{Path, PathBuf};

#[test]
fn safe_rust_crates_forbid_unsafe_code_at_the_crate_root() {
    let workspace = workspace_dir();
    for member in workspace_members(&workspace) {
        if glue_workspace_member(&member) {
            continue;
        }

        let crate_roots = crate_roots_for_member(&workspace, &member);
        assert!(
            !crate_roots.is_empty(),
            "{member} must expose at least one Rust crate root"
        );
        for crate_root in crate_roots {
            let source =
                fs::read_to_string(workspace.join(&crate_root)).expect("crate root is readable");
            assert!(
                source.contains("#![forbid(unsafe_code)]"),
                "{crate_root} must keep safe Rust enforced by a crate-level lint"
            );
        }
    }
}

#[test]
fn ffi_glue_crates_make_the_unsafe_boundary_explicit() {
    let workspace = workspace_dir();
    let cpp_root = fs::read_to_string(workspace.join("crates/starlight_cpp/src/lib.rs"))
        .expect("starlight_cpp crate root is readable");
    assert!(
        cpp_root
            .contains("#![cfg_attr(not(feature = \"native-standalone\"), forbid(unsafe_code))]"),
        "starlight_cpp must forbid unsafe code unless the native FFI backend is enabled"
    );
    assert!(
        cpp_root.contains("#![deny(unsafe_op_in_unsafe_fn)]"),
        "starlight_cpp native glue must require explicit unsafe blocks inside unsafe functions"
    );

    let ffi_root = fs::read_to_string(workspace.join("crates/starlight_ffi/src/lib.rs"))
        .expect("starlight_ffi crate root is readable");
    assert!(
        ffi_root.contains("#![deny(unsafe_op_in_unsafe_fn)]"),
        "starlight_ffi must require explicit unsafe blocks inside unsafe functions"
    );
}

#[test]
fn unsafe_keyword_is_confined_to_glue_sources() {
    let workspace = workspace_dir();
    let mut violations = Vec::new();
    for file in rust_files_under(&workspace.join("crates")) {
        if unsafe_allowed_in(&workspace, &file) {
            continue;
        }
        let source = fs::read_to_string(&file).expect("Rust source is readable");
        if contains_unsafe_keyword(&source) {
            violations.push(relative_path(&workspace, &file));
        }
    }

    assert!(
        violations.is_empty(),
        "unsafe is only allowed in C/C++ glue sources, but found it in:\n{}",
        violations.join("\n")
    );
}

fn workspace_dir() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../..")
        .canonicalize()
        .expect("resolve Rust Starlight workspace")
}

fn workspace_members(workspace: &Path) -> Vec<String> {
    let manifest =
        fs::read_to_string(workspace.join("Cargo.toml")).expect("workspace Cargo.toml is readable");
    let mut members = Vec::new();
    let mut in_members = false;

    for line in manifest.lines() {
        let line = line.trim();
        if !in_members {
            if line.starts_with("members") && line.contains('[') {
                in_members = true;
            }
            continue;
        }
        if line.contains(']') {
            break;
        }
        let entry = line.trim_end_matches(',').trim();
        if let Some(member) = entry
            .strip_prefix('"')
            .and_then(|entry| entry.strip_suffix('"'))
        {
            members.push(member.to_owned());
        }
    }

    assert!(
        !members.is_empty(),
        "workspace Cargo.toml must declare Rust Starlight members"
    );
    members
}

fn glue_workspace_member(member: &str) -> bool {
    matches!(member, "crates/starlight_cpp" | "crates/starlight_ffi")
}

fn crate_roots_for_member(workspace: &Path, member: &str) -> Vec<String> {
    ["src/lib.rs", "src/main.rs"]
        .into_iter()
        .map(|root| format!("{member}/{root}"))
        .filter(|root| workspace.join(root).exists())
        .collect()
}

fn rust_files_under(root: &Path) -> Vec<PathBuf> {
    let mut files = Vec::new();
    collect_rust_files(root, &mut files);
    files
}

fn collect_rust_files(path: &Path, files: &mut Vec<PathBuf>) {
    for entry in fs::read_dir(path).expect("source directory is readable") {
        let entry = entry.expect("directory entry is readable");
        let path = entry.path();
        if path.file_name().is_some_and(|name| name == "target") {
            continue;
        }
        if path.is_dir() {
            collect_rust_files(&path, files);
        } else if path.extension().is_some_and(|extension| extension == "rs") {
            files.push(path);
        }
    }
}

fn unsafe_allowed_in(workspace: &Path, file: &Path) -> bool {
    let relative = relative_path(workspace, file);
    relative.starts_with("crates/starlight_ffi/")
        || relative == "crates/starlight_cpp/src/native.rs"
        || relative == "crates/starlight_parity/tests/unsafe_boundary_tests.rs"
}

fn relative_path(workspace: &Path, file: &Path) -> String {
    file.strip_prefix(workspace)
        .expect("file is under workspace")
        .components()
        .map(|component| component.as_os_str().to_string_lossy())
        .collect::<Vec<_>>()
        .join("/")
}

fn contains_unsafe_keyword(source: &str) -> bool {
    source
        .split(|character: char| character != '_' && !character.is_ascii_alphanumeric())
        .any(|token| token == "unsafe")
}
