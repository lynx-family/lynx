// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::env;
use std::ffi::OsString;
use std::path::{Path, PathBuf};
use std::process::Command;

const FEATURE_ENV: &str = "CARGO_FEATURE_NATIVE_STANDALONE";
const CFG_NATIVE: &str = "starlight_cpp_native_standalone";
const CFG_NATIVE_CHECK: &str = "starlight_cpp_native_standalone_check";
const ENV_BUILD_FROM_SOURCE: &str = "STARLIGHT_CPP_NATIVE_STANDALONE_BUILD_FROM_SOURCE";
const ENV_CHECK_ONLY: &str = "STARLIGHT_CPP_NATIVE_STANDALONE_CHECK";
const ENV_LIB_DIR: &str = "STARLIGHT_CPP_NATIVE_STANDALONE_LIB_DIR";
const ENV_LINK_LIB: &str = "STARLIGHT_CPP_NATIVE_STANDALONE_LINK_LIB";
const ENV_CXX_STDLIB: &str = "STARLIGHT_CPP_NATIVE_STANDALONE_CXX_STDLIB";
const ENV_CXX: &str = "CXX";
const ENV_AR: &str = "AR";
const ENV_MACOSX_DEPLOYMENT_TARGET: &str = "MACOSX_DEPLOYMENT_TARGET";

const STARLIGHT_NATIVE_SOURCES: &[&str] = &[
    "core/services/starlight_standalone/core/src/starlight.cc",
    "core/services/starlight_standalone/core/src/starlight_config.cc",
    "core/renderer/starlight/layout/box_info.cc",
    "core/renderer/starlight/layout/cache_manager.cc",
    "core/renderer/starlight/layout/container_node.cc",
    "core/renderer/starlight/layout/elastic_layout_utils.cc",
    "core/renderer/starlight/layout/flex_info.cc",
    "core/renderer/starlight/layout/flex_layout_algorithm.cc",
    "core/renderer/starlight/layout/grid_item_info.cc",
    "core/renderer/starlight/layout/grid_layout_algorithm.cc",
    "core/renderer/starlight/layout/layout_algorithm.cc",
    "core/renderer/starlight/layout/layout_global.cc",
    "core/renderer/starlight/layout/layout_object.cc",
    "core/renderer/starlight/layout/linear_layout_algorithm.cc",
    "core/renderer/starlight/layout/logic_direction_utils.cc",
    "core/renderer/starlight/layout/position_layout_utils.cc",
    "core/renderer/starlight/layout/property_resolving_utils.cc",
    "core/renderer/starlight/layout/relative_layout_algorithm.cc",
    "core/renderer/starlight/layout/staggered_grid_layout_algorithm.cc",
    "core/renderer/starlight/style/borders_data.cc",
    "core/renderer/starlight/style/box_data.cc",
    "core/renderer/starlight/style/flex_data.cc",
    "core/renderer/starlight/style/grid_data.cc",
    "core/renderer/starlight/style/layout_computed_style.cc",
    "core/renderer/starlight/style/layout_style_utils.cc",
    "core/renderer/starlight/style/linear_data.cc",
    "core/renderer/starlight/style/relative_data.cc",
    "core/renderer/starlight/style/surround_data.cc",
    "core/renderer/starlight/types/nlength.cc",
    "base/src/log/alog_wrapper.cc",
    "base/src/log/log_stream.cc",
    "base/src/log/logging.cc",
    "base/src/value/base_string.cc",
];

const STARLIGHT_NATIVE_RERUN_DIRS: &[&str] = &[
    "base/include",
    "base/src/log",
    "base/src/value",
    "core/base",
    "core/include/starlight_standalone",
    "core/renderer/starlight/layout",
    "core/renderer/starlight/style",
    "core/renderer/starlight/types",
    "core/services/starlight_standalone/core/src",
    "core/style",
    "third_party/rapidjson",
];

fn main() {
    println!("cargo:rustc-check-cfg=cfg({CFG_NATIVE})");
    println!("cargo:rustc-check-cfg=cfg({CFG_NATIVE_CHECK})");
    println!("cargo:rerun-if-env-changed={ENV_BUILD_FROM_SOURCE}");
    println!("cargo:rerun-if-env-changed={ENV_CHECK_ONLY}");
    println!("cargo:rerun-if-env-changed={ENV_LIB_DIR}");
    println!("cargo:rerun-if-env-changed={ENV_LINK_LIB}");
    println!("cargo:rerun-if-env-changed={ENV_CXX_STDLIB}");
    println!("cargo:rerun-if-env-changed={ENV_CXX}");
    println!("cargo:rerun-if-env-changed={ENV_AR}");
    println!("cargo:rerun-if-env-changed={ENV_MACOSX_DEPLOYMENT_TARGET}");

    if env::var_os(FEATURE_ENV).is_none() {
        return;
    }

    if env::var_os(ENV_BUILD_FROM_SOURCE).is_some() {
        build_standalone_from_source();
        return;
    }

    let Some(link_lib) = env::var_os(ENV_LINK_LIB) else {
        if env::var_os(ENV_CHECK_ONLY).is_some() {
            println!("cargo:rustc-cfg={CFG_NATIVE_CHECK}");
        }
        println!(
            "cargo:warning={ENV_LINK_LIB} is not set; starlight_cpp native-standalone will report native link unavailable at runtime"
        );
        return;
    };

    println!("cargo:rustc-cfg={CFG_NATIVE}");

    if let Some(lib_dir) = env::var_os(ENV_LIB_DIR) {
        println!(
            "cargo:rustc-link-search=native={}",
            lib_dir.to_string_lossy()
        );
    }
    println!("cargo:rustc-link-lib={}", link_lib.to_string_lossy());

    if let Some(cxx_stdlib) = env::var_os(ENV_CXX_STDLIB) {
        println!("cargo:rustc-link-lib={}", cxx_stdlib.to_string_lossy());
    }
}

fn build_standalone_from_source() {
    let manifest_dir = PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").unwrap());
    let repo_root = manifest_dir
        .join("../../../../../..")
        .canonicalize()
        .expect("failed to resolve Lynx repository root for Starlight source build");
    let generated_css_type = repo_root.join("core/renderer/starlight/style/auto_gen_css_type.h");
    if !generated_css_type.exists() {
        panic!(
            "missing {}; run `python3 tools/css_generator/css_parser_generator.py` from the repository root before using {ENV_BUILD_FROM_SOURCE}=1",
            generated_css_type.display()
        );
    }

    for dir in STARLIGHT_NATIVE_RERUN_DIRS {
        println!("cargo:rerun-if-changed={}", repo_root.join(dir).display());
    }
    for source in STARLIGHT_NATIVE_SOURCES {
        println!(
            "cargo:rerun-if-changed={}",
            repo_root.join(source).display()
        );
    }
    println!("cargo:rerun-if-changed={}", generated_css_type.display());

    let out_dir = PathBuf::from(env::var_os("OUT_DIR").unwrap());
    let object_dir = out_dir.join("starlight_native_objects");
    std::fs::create_dir_all(&object_dir).expect("failed to create Starlight native object dir");

    let cxx = env::var_os(ENV_CXX).unwrap_or_else(|| OsString::from("clang++"));
    let ar = env::var_os(ENV_AR).unwrap_or_else(|| OsString::from("ar"));
    let mut objects = Vec::with_capacity(STARLIGHT_NATIVE_SOURCES.len());
    for source in STARLIGHT_NATIVE_SOURCES {
        let object = object_dir.join(format!("{}.o", object_name(source)));
        compile_source(&repo_root, &cxx, source, &object);
        objects.push(object);
    }

    let library = out_dir.join("libstarlight_standalone.a");
    archive_objects(&repo_root, &ar, &library, &objects);

    println!("cargo:rustc-cfg={CFG_NATIVE}");
    println!("cargo:rustc-link-search=native={}", out_dir.display());
    println!("cargo:rustc-link-lib=static=starlight_standalone");
    link_cxx_stdlib();
}

fn compile_source(repo_root: &Path, cxx: &OsString, source: &str, object: &Path) {
    let mut command = Command::new(cxx);
    command.current_dir(repo_root);
    command.args([
        "-std=c++17",
        "-I.",
        "-Ithird_party/rapidjson",
        "-c",
        source,
        "-o",
    ]);
    command.arg(object);
    if env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("macos") {
        let deployment_target =
            env::var(ENV_MACOSX_DEPLOYMENT_TARGET).unwrap_or_else(|_| "11.0".to_string());
        command.arg(format!("-mmacosx-version-min={deployment_target}"));
    }
    let status = command
        .status()
        .unwrap_or_else(|error| panic!("failed to run C++ compiler for {source}: {error}"));
    if !status.success() {
        panic!("C++ compiler failed for {source} with status {status}");
    }
}

fn archive_objects(repo_root: &Path, ar: &OsString, library: &Path, objects: &[PathBuf]) {
    let mut command = Command::new(ar);
    command.current_dir(repo_root);
    command.arg("rcs").arg(library);
    command.args(objects);
    let status = command.status().unwrap_or_else(|error| {
        panic!("failed to run archiver for Starlight native library: {error}")
    });
    if !status.success() {
        panic!("archiver failed for Starlight native library with status {status}");
    }
}

fn link_cxx_stdlib() {
    if let Some(cxx_stdlib) = env::var_os(ENV_CXX_STDLIB) {
        println!("cargo:rustc-link-lib={}", cxx_stdlib.to_string_lossy());
    } else if env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("macos") {
        println!("cargo:rustc-link-lib=c++");
    } else {
        println!("cargo:rustc-link-lib=stdc++");
    }
}

fn object_name(source: &str) -> String {
    source
        .chars()
        .map(|ch| if ch.is_ascii_alphanumeric() { ch } else { '_' })
        .collect()
}
