#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Build and publish the iOS XCFramework-backed Swift package.

The package intentionally uses one binary target per resolved root Pod. Every
publishable CocoaPods subspec is exposed as a SwiftPM product alias that points
at the complete binary for its root Pod and its dependency closure.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
from urllib.error import HTTPError
from urllib.parse import quote
from urllib.request import urlopen


COMPONENTS = (
    "LynxServiceAPI",
    "LynxBase",
    "Lynx",
    "BaseDevtool",
    "LynxDevtool",
    "LynxService",
    "XElement",
)
EXCLUDED_SUBSPEC_SEGMENTS = frozenset(("replay", "unittests", "unittestresource"))
MANIFEST_SCHEMA_VERSION = 1
PACKAGE_SUPPORT_TARGET = "LynxPackageSupport"
RESOLVED_SPECS_FILE = "ResolvedPodspecs.json"
VERSION_PATTERN = re.compile(
    r"^[0-9]+\.[0-9]+\.[0-9]+(?:-(?:alpha|rc)\.[0-9]+)?$"
)
SWIFT_TARGET_PATTERN = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
GITHUB_REPOSITORY_PATTERN = re.compile(
    r"^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$"
)
GIT_REVISION_PATTERN = re.compile(r"^[A-Fa-f0-9]{40}$")


def run(command, *, cwd=None, env=None, capture_output=False):
    print("+", " ".join(str(part) for part in command), flush=True)
    return subprocess.run(
        [str(part) for part in command],
        cwd=cwd,
        env=env,
        check=True,
        text=True,
        capture_output=capture_output,
    )


def read_json(path):
    with open(path, "r", encoding="utf-8") as source:
        return json.load(source)


def write_json(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", encoding="utf-8") as output:
        json.dump(value, output, indent=2, sort_keys=True)
        output.write("\n")


def file_sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_version(version):
    if not VERSION_PATTERN.fullmatch(version):
        raise ValueError(f"Invalid SwiftPM release version: {version}")


def validate_manifest(manifest, *, require_urls):
    if manifest.get("schema_version") != MANIFEST_SCHEMA_VERSION:
        raise ValueError("Unsupported SwiftPM artifact manifest schema")
    validate_version(manifest["version"])

    artifacts = manifest.get("artifacts", [])
    products = manifest.get("products", [])
    if not artifacts:
        raise ValueError("SwiftPM artifact manifest contains no artifacts")
    if not products:
        raise ValueError("SwiftPM artifact manifest contains no products")

    artifact_fields = ("pod", "target", "file", "sha256", "checksum")
    seen_pods = set()
    seen_targets = set()
    seen_files = set()
    for artifact in artifacts:
        for field in artifact_fields:
            if not artifact.get(field):
                raise ValueError(f"Artifact is missing {field}")
        if artifact["pod"] in seen_pods:
            raise ValueError(f"Duplicate artifact Pod: {artifact['pod']}")
        if artifact["target"] in seen_targets:
            raise ValueError(
                f"Duplicate SwiftPM binary target: {artifact['target']}"
            )
        if not SWIFT_TARGET_PATTERN.fullmatch(artifact["target"]):
            raise ValueError(
                f"Invalid SwiftPM binary target name: {artifact['target']}"
            )
        if artifact["file"] in seen_files:
            raise ValueError(f"Duplicate artifact file: {artifact['file']}")
        if Path(artifact["file"]).name != artifact["file"]:
            raise ValueError(f"Artifact file must be a basename: {artifact['file']}")
        if not re.fullmatch(r"[A-Fa-f0-9]{64}", artifact["sha256"]):
            raise ValueError(f"Invalid SHA-256 for {artifact['target']}")
        if artifact["checksum"] != artifact["sha256"]:
            raise ValueError(
                f"SwiftPM checksum differs from SHA-256 for {artifact['target']}"
            )
        if require_urls and not artifact.get("url"):
            raise ValueError(f"Artifact URL is missing for {artifact['target']}")
        seen_pods.add(artifact["pod"])
        seen_targets.add(artifact["target"])
        seen_files.add(artifact["file"])

    seen_product_names = set()
    seen_specs = set()
    for product in products:
        name = product.get("name")
        spec = product.get("spec")
        targets = product.get("targets", [])
        if not name or not spec or not targets:
            raise ValueError("Product must contain name, spec, and targets")
        if name in seen_product_names:
            raise ValueError(f"Duplicate SwiftPM product: {name}")
        if spec in seen_specs:
            raise ValueError(f"Duplicate CocoaPods spec product: {spec}")
        unknown_targets = set(targets) - seen_targets
        if unknown_targets:
            raise ValueError(
                f"Product {name} references unknown targets: "
                f"{', '.join(sorted(unknown_targets))}"
            )
        if len(set(targets)) != len(targets):
            raise ValueError(f"Product {name} contains duplicate targets")
        seen_product_names.add(name)
        seen_specs.add(spec)

    for resource in manifest.get("resources", []):
        if Path(resource).name != resource:
            raise ValueError(f"Resource must be a basename: {resource}")

    github_repository = manifest.get("github_repository")
    if github_repository is not None:
        if not GITHUB_REPOSITORY_PATTERN.fullmatch(github_repository):
            raise ValueError(f"Invalid GitHub repository: {github_repository}")
        for artifact in artifacts:
            expected_url = github_release_artifact_url(
                github_repository, manifest["version"], artifact["file"]
            )
            if artifact.get("url") and artifact["url"] != expected_url:
                raise ValueError(
                    f"Unexpected GitHub Release URL for {artifact['target']}"
                )

    source_repository = manifest.get("source_repository")
    source_revision = manifest.get("source_revision")
    if bool(source_repository) != bool(source_revision):
        raise ValueError(
            "Source repository and source revision must be provided together"
        )
    if source_repository:
        if not GITHUB_REPOSITORY_PATTERN.fullmatch(source_repository):
            raise ValueError(f"Invalid source repository: {source_repository}")
        if not GIT_REVISION_PATTERN.fullmatch(source_revision):
            raise ValueError(f"Invalid source revision: {source_revision}")


def load_component_specs(podspec_dir):
    podspec_dir = Path(podspec_dir)
    specs = {}
    for component in COMPONENTS:
        path = podspec_dir / f"{component}.podspec.json"
        if not path.is_file():
            raise FileNotFoundError(f"Missing generated podspec: {path}")
        specs[component] = read_json(path)
    return specs


def is_publishable_subspec(full_name):
    segments = (segment.lower() for segment in full_name.split("/")[1:])
    return not any(segment in EXCLUDED_SUBSPEC_SEGMENTS for segment in segments)


def iter_subspec_names(spec, root_name, parent_name=None):
    for subspec in spec.get("subspecs", []):
        name = subspec["name"]
        if "/" in name:
            full_name = name if name.startswith(f"{root_name}/") else f"{root_name}/{name}"
        else:
            prefix = parent_name or root_name
            full_name = f"{prefix}/{name}"
        if is_publishable_subspec(full_name):
            yield full_name
            yield from iter_subspec_names(subspec, root_name, full_name)


def product_name_for_spec(spec_name):
    return spec_name.replace("/", "-")


def walk_spec_nodes(spec):
    yield spec
    for subspec in spec.get("subspecs", []):
        yield from walk_spec_nodes(subspec)


def dependency_roots(spec):
    result = set()
    for node in walk_spec_nodes(spec):
        dependencies = node.get("dependencies", {})
        names = dependencies.keys() if isinstance(dependencies, dict) else dependencies
        for name in names:
            result.add(str(name).split("/", 1)[0])
    return result


def collect_string_field(specs, field):
    values = set()
    for spec in specs.values():
        for node in walk_spec_nodes(spec):
            current = node.get(field, [])
            if isinstance(current, str):
                current = [current]
            values.update(str(value) for value in current)
    return sorted(values)


def dependency_closure(root, graph):
    result = set()
    pending = [root]
    while pending:
        current = pending.pop()
        if current in result:
            continue
        result.add(current)
        pending.extend(graph.get(current, ()))
    return result


def create_host_project(work_dir):
    script = work_dir / "create_project.rb"
    script.write_text(
        "require 'xcodeproj'\n"
        "project = Xcodeproj::Project.new('LynxSPMHost.xcodeproj')\n"
        "target = project.new_target(:application, 'LynxSPMHost', :ios, '10.0')\n"
        "group = project.main_group.new_group('Sources')\n"
        "source = group.new_file('Sources/main.m')\n"
        "target.add_file_references([source])\n"
        "target.build_configurations.each do |config|\n"
        "  config.build_settings['CODE_SIGNING_ALLOWED'] = 'NO'\n"
        "  config.build_settings['GENERATE_INFOPLIST_FILE'] = 'YES'\n"
        "end\n"
        "project.save\n",
        encoding="utf-8",
    )
    sources = work_dir / "Sources"
    sources.mkdir(parents=True, exist_ok=True)
    (sources / "main.m").write_text(
        "#import <UIKit/UIKit.h>\nint main(int argc, char **argv) { return 0; }\n",
        encoding="utf-8",
    )
    run(["bundle", "exec", "ruby", script.name], cwd=work_dir)


def create_podfile(work_dir, repo_root, component_specs):
    lines = [
        "require 'json'",
        "source 'https://cdn.cocoapods.org/'",
        "platform :ios, '10.0'",
        "use_frameworks! :linkage => :static",
        "project 'LynxSPMHost.xcodeproj'",
        "target 'LynxSPMHost' do",
    ]
    quoted_root = json.dumps(str(repo_root))
    for component, spec in component_specs.items():
        subspecs = sorted(set(iter_subspec_names(spec, component)))
        selected_specs = subspecs or [component]
        for selected_spec in selected_specs:
            lines.append(f"  pod {json.dumps(selected_spec)}, :path => {quoted_root}")
    lines.extend([
        "end",
        "",
        "post_install do |installer|",
        "  installer.pods_project.targets.each do |target|",
        "    target.build_configurations.each do |config|",
        "      config.build_settings['BUILD_LIBRARY_FOR_DISTRIBUTION'] = 'YES'",
        "      config.build_settings['CODE_SIGNING_ALLOWED'] = 'NO'",
        "      config.build_settings['GCC_TREAT_WARNINGS_AS_ERRORS'] = 'NO'",
        "    end",
        "  end",
        "  resolved_specs = installer.pod_targets.each_with_object({}) do |pod_target, specs|",
        "    specs[pod_target.root_spec.name] = pod_target.root_spec.to_hash",
        "  end",
        f"  File.write({json.dumps(str(work_dir / RESOLVED_SPECS_FILE))}, "
        "JSON.pretty_generate(resolved_specs))",
        "end",
        "",
    ])
    (work_dir / "Podfile").write_text("\n".join(lines), encoding="utf-8")


def build_host(work_dir, sdk, architectures):
    derived_data = work_dir / f"DerivedData-{sdk}"
    run([
        "xcodebuild",
        "-workspace", str(work_dir / "LynxSPMHost.xcworkspace"),
        "-scheme", "LynxSPMHost",
        "-configuration", "Release",
        "-sdk", sdk,
        "-derivedDataPath", str(derived_data),
        "CODE_SIGNING_ALLOWED=NO",
        "ONLY_ACTIVE_ARCH=NO",
        f"ARCHS={' '.join(architectures)}",
        "build",
    ])
    return derived_data / "Build" / "Products" / f"Release-{sdk}"


def installed_specs(work_dir):
    path = work_dir / RESOLVED_SPECS_FILE
    if not path.is_file():
        raise FileNotFoundError(f"CocoaPods did not export resolved specs: {path}")
    specs = read_json(path)
    if not specs:
        raise ValueError("CocoaPods resolved spec export is empty")
    return specs


def module_name_for_spec(spec):
    return spec.get("module_name") or re.sub(r"[^A-Za-z0-9_]", "_", spec["name"])


def find_framework(products_dir, module_name):
    candidates = [
        path for path in products_dir.rglob(f"{module_name}.framework")
        if "XCFrameworkIntermediates" not in path.parts
    ]
    if not candidates:
        return None
    return min(candidates, key=lambda path: (len(path.parts), str(path)))


def copy_resource_bundles(products_dir, destination):
    if destination.exists():
        shutil.rmtree(destination)
    destination.mkdir(parents=True)
    seen = set()
    for bundle in sorted(products_dir.rglob("*.bundle")):
        if "XCFrameworkIntermediates" in bundle.parts or bundle.name in seen:
            continue
        shutil.copytree(bundle, destination / bundle.name, symlinks=True)
        seen.add(bundle.name)
    return sorted(seen)


def create_xcframeworks(repo_root, work_dir, output_dir, version):
    component_specs = load_component_specs(repo_root)
    if work_dir.exists():
        shutil.rmtree(work_dir)
    work_dir.mkdir(parents=True)
    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True)

    create_host_project(work_dir)
    create_podfile(work_dir, repo_root, component_specs)
    environment = os.environ.copy()
    environment["PACKAGE_ENV"] = "prod"
    run(
        ["bundle", "exec", "pod", "install", f"--project-directory={work_dir}"],
        cwd=repo_root,
        env=environment,
    )

    device_products = build_host(work_dir, "iphoneos", ("arm64",))
    simulator_products = build_host(
        work_dir, "iphonesimulator", ("arm64", "x86_64")
    )
    resolved_specs = installed_specs(work_dir)
    artifacts = []
    root_to_target = {}

    for root_name, spec in sorted(resolved_specs.items()):
        target_name = module_name_for_spec(spec)
        if target_name in root_to_target.values():
            raise ValueError(f"Duplicate SwiftPM binary target name: {target_name}")
        device_framework = find_framework(device_products, target_name)
        simulator_framework = find_framework(simulator_products, target_name)
        if device_framework is None and simulator_framework is None:
            print(f"Skip header-only Pod without framework: {root_name}")
            continue
        if device_framework is None or simulator_framework is None:
            raise FileNotFoundError(
                f"Missing device or simulator framework for {root_name} ({target_name})"
            )

        xcframework = output_dir / f"{target_name}.xcframework"
        if xcframework.exists():
            shutil.rmtree(xcframework)
        run([
            "xcodebuild", "-create-xcframework",
            "-framework", str(device_framework),
            "-framework", str(simulator_framework),
            "-output", str(xcframework),
        ])
        archive = output_dir / f"{target_name}-{version}.xcframework.zip"
        if archive.exists():
            archive.unlink()
        run([
            "ditto", "-c", "-k", "--sequesterRsrc", "--keepParent",
            str(xcframework), str(archive),
        ])
        checksum = run(
            ["swift", "package", "compute-checksum", str(archive)],
            capture_output=True,
        ).stdout.strip()
        sha256 = file_sha256(archive)
        if checksum != sha256:
            raise RuntimeError(f"SwiftPM checksum differs from SHA-256 for {archive}")
        root_to_target[root_name] = target_name
        artifacts.append({
            "pod": root_name,
            "target": target_name,
            "file": archive.name,
            "sha256": sha256,
            "checksum": checksum,
        })

    graph = {
        name: sorted(dependency_roots(spec)) for name, spec in resolved_specs.items()
    }
    products = []
    for component, spec in component_specs.items():
        if component not in root_to_target:
            raise ValueError(f"No binary artifact was built for {component}")
        roots = dependency_closure(component, graph)
        targets = sorted({root_to_target[root] for root in roots if root in root_to_target})
        products.append({"name": component, "spec": component, "targets": targets})
        for subspec in sorted(set(iter_subspec_names(spec, component))):
            products.append({
                "name": product_name_for_spec(subspec),
                "spec": subspec,
                "targets": targets,
            })

    resources = copy_resource_bundles(
        device_products, output_dir / "Resources"
    )
    manifest = {
        "schema_version": MANIFEST_SCHEMA_VERSION,
        "version": version,
        "deployment_target": "10.0",
        "artifacts": artifacts,
        "products": products,
        "system_frameworks": collect_string_field(resolved_specs, "frameworks"),
        "weak_frameworks": collect_string_field(resolved_specs, "weak_frameworks"),
        "system_libraries": collect_string_field(resolved_specs, "libraries"),
        "resources": resources,
    }
    validate_manifest(manifest, require_urls=False)
    manifest_path = output_dir / "PackageArtifacts.json"
    write_json(manifest_path, manifest)
    return manifest_path


def github_release_artifact_url(repository, version, filename):
    if not GITHUB_REPOSITORY_PATTERN.fullmatch(repository):
        raise ValueError(f"Invalid GitHub repository: {repository}")
    return (
        f"https://github.com/{repository}/releases/download/"
        f"{quote(version, safe='')}/{quote(filename, safe='')}"
    )


def set_github_release_metadata(
        manifest_path, repository, source_repository=None,
        source_revision=None):
    manifest_path = Path(manifest_path)
    manifest = read_json(manifest_path)
    validate_manifest(manifest, require_urls=False)
    if bool(source_repository) != bool(source_revision):
        raise ValueError(
            "Source repository and source revision must be provided together"
        )

    for artifact in manifest["artifacts"]:
        artifact["url"] = github_release_artifact_url(
            repository, manifest["version"], artifact["file"]
        )
    manifest["github_repository"] = repository
    if source_repository:
        manifest["source_repository"] = source_repository
        manifest["source_revision"] = source_revision.lower()
    validate_manifest(manifest, require_urls=True)
    write_json(manifest_path, manifest)


def remote_sha256(url):
    digest = hashlib.sha256()
    with urlopen(url, timeout=120) as response:
        for chunk in iter(lambda: response.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def check_manifest_version(manifest, expected_version):
    if expected_version and manifest["version"] != expected_version:
        raise ValueError(
            f"SwiftPM artifact version {manifest['version']} does not match "
            f"{expected_version}"
        )


def validate_manifest_file(
        manifest_path, expected_version=None, expected_repository=None,
        expected_source_repository=None, expected_source_revision=None):
    manifest = read_json(manifest_path)
    validate_manifest(manifest, require_urls=True)
    check_manifest_version(manifest, expected_version)
    if not manifest.get("github_repository"):
        raise ValueError("GitHub repository is missing")
    if (
            expected_repository and
            manifest["github_repository"] != expected_repository):
        raise ValueError(
            f"SwiftPM artifact repository {manifest['github_repository']} "
            f"does not match {expected_repository}"
        )
    if (
            expected_source_repository and
            manifest.get("source_repository") != expected_source_repository):
        raise ValueError(
            f"SwiftPM source repository {manifest.get('source_repository')} "
            f"does not match {expected_source_repository}"
        )
    if (
            expected_source_revision and
            manifest.get("source_revision") != expected_source_revision.lower()):
        raise ValueError(
            f"SwiftPM source revision {manifest.get('source_revision')} "
            f"does not match {expected_source_revision.lower()}"
        )


def verify_local_artifacts(
        manifest_path, artifacts_dir=None, expected_version=None):
    manifest_path = Path(manifest_path)
    manifest = read_json(manifest_path)
    validate_manifest(manifest, require_urls=True)
    check_manifest_version(manifest, expected_version)
    artifacts_dir = (
        Path(artifacts_dir) if artifacts_dir else manifest_path.parent
    )
    for artifact in manifest["artifacts"]:
        path = artifacts_dir / artifact["file"]
        if not path.is_file():
            raise FileNotFoundError(f"Missing XCFramework archive: {path}")
        local_hash = file_sha256(path)
        if (
                local_hash != artifact["sha256"] or
                local_hash != artifact["checksum"]):
            raise RuntimeError(f"Local artifact checksum mismatch: {path}")


def swift_string(value):
    return json.dumps(value, ensure_ascii=False)


def render_package(manifest):
    validate_manifest(manifest, require_urls=True)
    binary_targets = []
    for artifact in sorted(manifest["artifacts"], key=lambda item: item["target"]):
        binary_targets.append(
            "        .binaryTarget(\n"
            f"            name: {swift_string(artifact['target'])},\n"
            f"            url: {swift_string(artifact['url'])},\n"
            f"            checksum: {swift_string(artifact['checksum'])}\n"
            "        )"
        )

    linker_settings = []
    frameworks = set(manifest.get("system_frameworks", []))
    frameworks.update(manifest.get("weak_frameworks", []))
    for framework in sorted(frameworks):
        linker_settings.append(f"            .linkedFramework({swift_string(framework)})")
    for library in sorted(set(manifest.get("system_libraries", []))):
        if library == "stdc++":
            library = "c++"
        linker_settings.append(f"            .linkedLibrary({swift_string(library)})")

    support_arguments = []
    if manifest.get("resources"):
        resources = ", ".join(
            f".copy({swift_string('Resources/' + resource)})"
            for resource in sorted(manifest["resources"])
        )
        support_arguments.append(
            f"            resources: [{resources}]"
        )
    if linker_settings:
        support_arguments.append(
            "            linkerSettings: [\n"
            + ",\n".join(linker_settings)
            + "\n            ]"
        )
    support_target = "        .target(\n            name: \"LynxPackageSupport\""
    if support_arguments:
        support_target += ",\n" + ",\n".join(support_arguments)
    support_target += "\n        )"

    products = []
    for product in sorted(manifest["products"], key=lambda item: item["name"]):
        targets = list(product["targets"]) + [PACKAGE_SUPPORT_TARGET]
        rendered_targets = ", ".join(swift_string(target) for target in targets)
        products.append(
            "        .library(\n"
            f"            name: {swift_string(product['name'])},\n"
            f"            targets: [{rendered_targets}]\n"
            "        )"
        )

    deployment_target = manifest["deployment_target"]
    if deployment_target.endswith(".0"):
        deployment_target = deployment_target[:-2]

    return (
        "// swift-tools-version: 5.9\n"
        "// Generated by tools/ios_tools/spm_publish_helper.py. Do not edit.\n\n"
        "import PackageDescription\n\n"
        "let package = Package(\n"
        "    name: \"Lynx\",\n"
        f"    platforms: [.iOS(.v{deployment_target.replace('.', '_')})],\n"
        "    products: [\n"
        + ",\n".join(products)
        + "\n    ],\n"
        "    targets: [\n"
        + ",\n".join(binary_targets + [support_target])
        + "\n    ]\n"
        ")\n"
    )


def generate_package(repo_root, manifest_path):
    repo_root = Path(repo_root)
    manifest_path = Path(manifest_path)
    manifest = read_json(manifest_path)
    validate_manifest(manifest, require_urls=True)

    support_root = repo_root / "Sources" / PACKAGE_SUPPORT_TARGET
    if support_root.exists():
        shutil.rmtree(support_root)
    support_root.mkdir(parents=True)
    resources_source = manifest_path.parent / "Resources"
    if manifest.get("resources"):
        shutil.copytree(resources_source, support_root / "Resources")
        source = (
            "import Foundation\n\n"
            "public enum LynxPackageResources {\n"
            "    public static let bundle = Bundle.module\n"
            "}\n"
        )
    else:
        source = "public enum LynxPackageSupport {}\n"
    (support_root / "LynxPackageSupport.swift").write_text(source, encoding="utf-8")
    (repo_root / "Package.swift").write_text(render_package(manifest), encoding="utf-8")
    write_json(repo_root / "spm" / "PackageArtifacts.json", manifest)


def verify_remote_artifacts(manifest_path, expected_version=None):
    manifest = read_json(manifest_path)
    validate_manifest(manifest, require_urls=True)
    check_manifest_version(manifest, expected_version)
    for artifact in manifest["artifacts"]:
        for attempt in range(3):
            try:
                actual = remote_sha256(artifact["url"])
                break
            except HTTPError as error:
                if error.code != 404 or attempt == 2:
                    raise
                time.sleep(2 ** attempt)
        if actual != artifact["sha256"] or actual != artifact["checksum"]:
            raise ValueError(f"Checksum mismatch for {artifact['url']}")


def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    build = subparsers.add_parser("build")
    build.add_argument("--version", required=True)
    build.add_argument("--repo-root", default=".")
    build.add_argument("--work-dir", default="out/ios-spm/work")
    build.add_argument("--output-dir", default="out/ios-spm/dist")

    release = subparsers.add_parser("set-github-release")
    release.add_argument("--manifest", required=True)
    release.add_argument("--repository", required=True)
    release.add_argument("--source-repository")
    release.add_argument("--source-revision")

    package = subparsers.add_parser("generate-package")
    package.add_argument("--manifest", required=True)
    package.add_argument("--repo-root", default=".")

    validate = subparsers.add_parser("validate-manifest")
    validate.add_argument("--manifest", default="spm/PackageArtifacts.json")
    validate.add_argument("--version")
    validate.add_argument("--repository")
    validate.add_argument("--source-repository")
    validate.add_argument("--source-revision")

    verify_local = subparsers.add_parser("verify-local")
    verify_local.add_argument("--manifest", required=True)
    verify_local.add_argument("--artifacts-dir")
    verify_local.add_argument("--version")

    verify = subparsers.add_parser("verify-remote")
    verify.add_argument("--manifest", default="spm/PackageArtifacts.json")
    verify.add_argument("--version")

    args = parser.parse_args()
    if args.command == "build":
        validate_version(args.version)
        create_xcframeworks(
            Path(args.repo_root).resolve(),
            Path(args.work_dir).resolve(),
            Path(args.output_dir).resolve(),
            args.version,
        )
    elif args.command == "set-github-release":
        set_github_release_metadata(
            args.manifest,
            args.repository,
            args.source_repository,
            args.source_revision,
        )
    elif args.command == "generate-package":
        generate_package(args.repo_root, args.manifest)
    elif args.command == "validate-manifest":
        validate_manifest_file(
            args.manifest,
            args.version,
            args.repository,
            args.source_repository,
            args.source_revision,
        )
    elif args.command == "verify-local":
        verify_local_artifacts(
            args.manifest, args.artifacts_dir, args.version
        )
    elif args.command == "verify-remote":
        verify_remote_artifacts(args.manifest, args.version)


if __name__ == "__main__":
    try:
        main()
    except (ValueError, FileNotFoundError, RuntimeError) as error:
        print(f"error: {error}", file=sys.stderr)
        sys.exit(1)
