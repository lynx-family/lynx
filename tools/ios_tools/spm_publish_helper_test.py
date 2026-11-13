#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import hashlib
import json
from pathlib import Path
import plistlib
import struct
import sys
import tempfile
import unittest
from unittest import mock
from urllib.error import HTTPError

sys.path.insert(0, str(Path(__file__).resolve().parent))

import spm_publish_helper as helper


class SpmPublishHelperTest(unittest.TestCase):
    def test_component_manifest_rejects_invalid_or_missing_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            manifest = root / "Components.json"
            helper.write_json(root / "Example.podspec.json", {"name": "Example"})
            with self.assertRaises(FileNotFoundError):
                helper.load_component_specs(root, manifest)
            for content in (
                    [], {}, "Example", [None], [""], ["../Example"],
                    ["/Example"], ["Example", "Example"]):
                with self.subTest(content=content):
                    helper.write_json(manifest, content)
                    with self.assertRaises(ValueError):
                        helper.load_component_specs(root, manifest)
            helper.write_json(manifest, ["Example", "Missing"])
            with self.assertRaisesRegex(FileNotFoundError, "Missing.podspec.json"):
                helper.load_component_specs(root, manifest)
            helper.write_json(manifest, ["Example"])
            helper.write_json(root / "Example.podspec.json", {"name": "Other"})
            with self.assertRaisesRegex(ValueError, "does not match"):
                helper.load_component_specs(root, manifest)

    def test_build_cli_requires_and_passes_component_manifest(self):
        arguments = ["spm_publish_helper.py", "build", "--version", "1.2.3"]
        with mock.patch.object(sys, "argv", arguments), \
                mock.patch.object(sys, "stderr"), \
                self.assertRaises(SystemExit) as error:
            helper.main()
        self.assertEqual(error.exception.code, 2)
        with mock.patch.object(
                sys, "argv", arguments + ["--components-manifest", "Components.json"]), \
                mock.patch.object(helper, "create_xcframeworks") as build:
            helper.main()
        self.assertEqual(build.call_args.args[-1], "Components.json")

    @staticmethod
    def archive_fixture():
        # A minimal ar container with a Mach-O header as its member payload.
        payload = b"\xcf\xfa\xed\xfe" + b"\0" * 28
        member = f"{'fixture.o/':<16}{0:<12}{0:<6}{0:<6}{'100644':<8}{len(payload):<10}`\n"
        return b"!<arch>\n" + member.encode("ascii") + payload

    @classmethod
    def universal_fixture(cls):
        payload = cls.archive_fixture()
        return (
            struct.pack(">II", 0xcafebabe, 2)
            + struct.pack(">IIIII", 0x100000c, 0, 48, len(payload), 0)
            + struct.pack(">IIIII", 0x1000007, 3, 48 + len(payload), len(payload), 0)
            + payload + payload
        )

    @staticmethod
    def binary_spec(name, paths=(), should_build=False, dependencies=()):
        return {
            "name": name,
            "spm_build": {
                "module_name": name,
                "should_build": should_build,
                "dependencies": list(dependencies),
                "vendored_framework_patterns": list(map(str, paths)),
                "vendored_frameworks": list(map(str, paths)),
                "vendored_libraries": [],
            },
        }

    @classmethod
    def vendored_fixture(cls, root, name):
        container = root / f"{name}.xcframework"
        libraries = []
        for identifier, variant, architectures in (
                ("ios-arm64", "", ["arm64"]),
                ("ios-arm64_x86_64-simulator", "simulator", ["arm64", "x86_64"])):
            framework = container / identifier / f"{name}.framework"
            framework.mkdir(parents=True)
            (framework / name).write_bytes(
                cls.universal_fixture() if variant else cls.archive_fixture()
            )
            (framework / "Info.plist").write_bytes(
                plistlib.dumps({"CFBundleExecutable": name})
            )
            libraries.append({
                "LibraryIdentifier": identifier,
                "LibraryPath": framework.name,
                "SupportedPlatform": "ios",
                "SupportedPlatformVariant": variant,
                "SupportedArchitectures": architectures,
            })
        (container / "Info.plist").write_bytes(
            plistlib.dumps({"AvailableLibraries": libraries})
        )
        return container

    def test_skity_missing_executable_metadata_is_repaired_only_in_packaging_copy(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            container = self.vendored_fixture(root, "skity")
            originals = {}
            for plist in container.glob("*/skity.framework/Info.plist"):
                plist.write_bytes(plistlib.dumps({
                    "CFBundleName": "Skity",
                    "CFBundleIdentifier": "org.lynxsdk.lynx.skity",
                }))
                originals[plist] = plist.read_bytes()
            slices = helper.vendored_framework_slices(container)
            for index, framework in enumerate(slices):
                packaged = helper.prepare_framework_for_packaging(
                    framework, root / "staging" / str(index)
                )
                self.assertNotEqual(packaged, framework)
                info = plistlib.loads((packaged / "Info.plist").read_bytes())
                self.assertEqual(info["CFBundleExecutable"], "skity")
                self.assertEqual(info["CFBundleName"], "Skity")
                self.assertEqual(
                    (packaged / "skity").read_bytes(), (framework / "skity").read_bytes()
                )
            for plist, original in originals.items():
                self.assertEqual(plist.read_bytes(), original)

    def test_valid_explicit_executable_is_preserved(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            container = self.vendored_fixture(root, "skity")
            framework = container / "ios-arm64" / "skity.framework"
            (framework / "skity").rename(framework / "ActualBinary")
            (framework / "Info.plist").write_bytes(
                plistlib.dumps({"CFBundleExecutable": "ActualBinary"})
            )
            self.assertEqual(
                helper.prepare_framework_for_packaging(framework, root / "staging"),
                framework,
            )
            self.assertFalse((root / "staging").exists())

    def test_repair_dereferences_plist_symlink_without_modifying_original(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            container = self.vendored_fixture(root, "skity")
            framework = container / "ios-arm64" / "skity.framework"
            original = plistlib.dumps({"CFBundleName": "Skity"})
            shared_plist = root / "shared.plist"
            shared_plist.write_bytes(original)
            (framework / "Info.plist").unlink()
            (framework / "Info.plist").symlink_to(shared_plist)
            staged = helper.prepare_framework_for_packaging(framework, root / "staging")
            self.assertFalse((staged / "Info.plist").is_symlink())
            self.assertEqual(shared_plist.read_bytes(), original)
            self.assertTrue((framework / "Info.plist").is_symlink())
            self.assertEqual(
                plistlib.loads((staged / "Info.plist").read_bytes())["CFBundleExecutable"],
                "skity",
            )

    def test_invalid_explicit_executable_never_falls_back(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            container = self.vendored_fixture(root, "skity")
            framework = container / "ios-arm64" / "skity.framework"
            for value in ("", None, 42, "../skity", "/skity", ".", "..", "a\\b", "\0"):
                with self.subTest(value=value):
                    with self.assertRaisesRegex(ValueError, "Invalid CFBundleExecutable"):
                        helper.framework_executable(framework, {"CFBundleExecutable": value})
            with self.assertRaisesRegex(FileNotFoundError, "Missing vendored framework binary"):
                helper.framework_executable(framework, {"CFBundleExecutable": "missing"})

    def test_missing_field_still_requires_real_binary(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            container = self.vendored_fixture(root, "skity")
            framework = container / "ios-arm64" / "skity.framework"
            (framework / "skity").unlink()
            with self.assertRaisesRegex(FileNotFoundError, "Missing vendored framework binary"):
                helper.framework_executable(framework, {"CFBundleName": "Skity"})

    def test_invalid_binary_formats_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = Path(directory) / "skity"
            malformed_fat = (
                struct.pack(">II", 0xcafebabe, 1)
                + struct.pack(">IIIII", 0x100000c, 0, 28, 5000, 0)
                + self.archive_fixture()
            )
            for contents in (b"", b"not a binary", b"!<arch>\n", b"\xcf\xfa\xed\xfe",
                             b"\xca\xfe\xba\xbe" + b"\0" * 4, malformed_fat):
                with self.subTest(contents=contents[:8]):
                    binary.write_bytes(contents)
                    with self.assertRaisesRegex(ValueError, "Invalid framework binary format"):
                        helper.validate_framework_binary(binary)
            for contents in (self.archive_fixture(), self.universal_fixture(),
                             b"\xcf\xfa\xed\xfe" + b"\0" * 28):
                binary.write_bytes(contents)
                helper.validate_framework_binary(binary)

    def test_collect_vendored_and_source_frameworks_with_different_names(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            skity = self.vendored_fixture(root, "skity")
            textra = self.vendored_fixture(root, "LynxTextra")
            for plist in skity.glob("*/skity.framework/Info.plist"):
                plist.write_bytes(plistlib.dumps({"CFBundleName": "Skity"}))
            products = [root / "device", root / "simulator"]
            for product in products:
                (product / "MixedPod" / "MixedPod.framework").mkdir(parents=True)
                # A coincidental intermediate must not replace the declared binary.
                (product / "XCFrameworkIntermediates" / "skity.framework").mkdir(parents=True)
            spec = self.binary_spec("MixedPod", [skity, textra], should_build=True)
            frameworks, mapping = helper.collect_frameworks(
                {"MixedPod": spec}, *products
            )
            self.assertEqual(set(frameworks), {"MixedPod", "skity", "LynxTextra"})
            self.assertEqual(set(mapping["MixedPod"]), set(frameworks))
            self.assertEqual(
                frameworks["skity"][1], skity / "ios-arm64" / "skity.framework"
            )

    def test_missing_compiled_pod_is_not_header_only(self):
        with tempfile.TemporaryDirectory() as directory:
            products = Path(directory)
            with self.assertRaisesRegex(FileNotFoundError, "Missing device or simulator"):
                helper.collect_frameworks(
                    {"Required": self.binary_spec("Required", should_build=True)},
                    products, products,
                )

    def test_missing_vendored_binary_fails(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            container = self.vendored_fixture(root, "skity")
            (container / "ios-arm64" / "skity.framework" / "skity").unlink()
            with self.assertRaisesRegex(FileNotFoundError, "Missing vendored framework binary"):
                helper.collect_frameworks(
                    {"skity": self.binary_spec("skity", [container])}, root, root
                )

    def test_vendored_requires_complete_simulator_architectures(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            container = self.vendored_fixture(root, "skity")
            path = container / "Info.plist"
            metadata = plistlib.loads(path.read_bytes())
            metadata["AvailableLibraries"][1]["SupportedArchitectures"] = ["arm64"]
            path.write_bytes(plistlib.dumps(metadata))
            with self.assertRaisesRegex(ValueError, "simulator slice"):
                helper.vendored_framework_slices(container)

    def test_vendored_rejects_ambiguous_or_missing_platform_slices(self):
        for variant in ("missing", "duplicate"):
            with self.subTest(variant=variant), tempfile.TemporaryDirectory() as directory:
                container = self.vendored_fixture(Path(directory), "skity")
                path = container / "Info.plist"
                metadata = plistlib.loads(path.read_bytes())
                if variant == "missing":
                    metadata["AvailableLibraries"].pop()
                else:
                    metadata["AvailableLibraries"].append(metadata["AvailableLibraries"][1])
                path.write_bytes(plistlib.dumps(metadata))
                with self.assertRaisesRegex(ValueError, "simulator slice"):
                    helper.vendored_framework_slices(container)

    def test_single_platform_vendored_framework_is_rejected_explicitly(self):
        with self.assertRaisesRegex(ValueError, "requires an XCFramework"):
            helper.vendored_framework_slices(Path("Vendor.framework"))

    def test_unresolved_vendored_declarations_and_unsupported_libraries_fail(self):
        spec = self.binary_spec("Required")
        spec["spm_build"]["vendored_framework_patterns"] = ["*.xcframework"]
        with self.assertRaisesRegex(FileNotFoundError, "No declared vendored_frameworks"):
            helper.collect_frameworks({"Required": spec}, Path("."), Path("."))
        spec["spm_build"]["vendored_libraries"] = ["libRequired.a"]
        with self.assertRaisesRegex(ValueError, "Unsupported vendored_libraries"):
            helper.collect_frameworks({"Required": spec}, Path("."), Path("."))
        with self.assertRaisesRegex(ValueError, "Missing resolved build metadata"):
            helper.collect_frameworks({"Required": {"name": "Required"}}, Path("."), Path("."))

    def test_duplicate_binary_module_fails_even_for_different_pods(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            container = self.vendored_fixture(root, "Shared")
            with self.assertRaisesRegex(ValueError, "Duplicate SwiftPM binary target"):
                helper.collect_frameworks({
                    "A": self.binary_spec("A", [container]),
                    "B": self.binary_spec("B", [container]),
                }, root, root)

    def test_product_closure_requires_explicit_nonbinary_pods(self):
        graph = {"XElement": ["Headers"], "Headers": ["skity"]}
        mapping = {"XElement": ["XElement"], "skity": ["skity"]}
        with self.assertRaisesRegex(ValueError, "Unaccounted dependencies.*Headers"):
            helper.product_targets("XElement", graph, mapping)
        _, nonbinary = helper.collect_frameworks(
            {"Headers": self.binary_spec("Headers")}, Path("."), Path(".")
        )
        mapping.update(nonbinary)
        self.assertEqual(
            helper.product_targets("XElement", graph, mapping), ["XElement", "skity"]
        )

    def test_packaging_keeps_vendored_dependencies_in_generated_package(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            skity = self.vendored_fixture(root, "skity")
            textra = self.vendored_fixture(root, "LynxTextra")
            for plist in skity.glob("*/skity.framework/Info.plist"):
                plist.write_bytes(plistlib.dumps({"CFBundleName": "Skity"}))
            specs = {
                "XElement": self.binary_spec(
                    "XElement", should_build=True, dependencies=["AnimaX"]
                ),
                # Exercise several binaries from a single resolved Pod.
                "AnimaX": self.binary_spec("AnimaX", [skity, textra]),
            }
            device, simulator = root / "device", root / "simulator"
            for products in (device, simulator):
                framework = products / "XElement.framework"
                framework.mkdir(parents=True)
                (framework / "XElement").write_bytes(self.archive_fixture())
                (framework / "Info.plist").write_bytes(
                    plistlib.dumps({"CFBundleExecutable": "XElement"})
                )

            def run(command, **kwargs):
                if command[:2] == ["xcodebuild", "-create-xcframework"]:
                    for index, argument in enumerate(command):
                        if argument == "-framework":
                            path = Path(command[index + 1])
                            if path.stem == "skity":
                                self.assertIn("framework-staging", path.parts)
                                info = plistlib.loads((path / "Info.plist").read_bytes())
                                self.assertEqual(info["CFBundleExecutable"], "skity")
                if command[0] == "ditto":
                    Path(command[-1]).write_bytes(b"archive fixture")
                if command[:3] == ["swift", "package", "compute-checksum"]:
                    return mock.Mock(stdout=helper.file_sha256(command[-1]))
                return mock.Mock()

            components_manifest = root / "Components.json"
            helper.write_json(components_manifest, ["XElement"])
            helper.write_json(
                root / "XElement.podspec.json",
                {"name": "XElement", "subspecs": [{"name": "AnimaX"}]},
            )
            with mock.patch.object(helper, "create_host_project"), \
                    mock.patch.object(helper, "create_podfile"), \
                    mock.patch.object(helper, "copy_ssziparchive_zlib_headers"), \
                    mock.patch.object(helper, "installed_specs", return_value=specs), \
                    mock.patch.object(helper, "build_host", side_effect=[device, simulator]), \
                    mock.patch.object(helper, "run", side_effect=run):
                manifest_path = helper.create_xcframeworks(
                    root, root / "work", root / "dist", "4.4.0-alpha.13",
                    components_manifest,
                )
            helper.set_github_release_metadata(manifest_path, "example/lynx")
            manifest = helper.read_json(manifest_path)
            self.assertEqual(
                {a["target"] for a in manifest["artifacts"]},
                {"XElement", "skity", "LynxTextra"},
            )
            for product in manifest["products"]:
                self.assertEqual(product["targets"], ["LynxTextra", "XElement", "skity"])
            package = helper.render_package(manifest)
            self.assertIn('name: "skity"', package)
            self.assertIn('name: "LynxTextra"', package)

    def test_validate_version_rejects_nightly(self):
        helper.validate_version("4.2.0")
        helper.validate_version("4.2.0-alpha.1")
        helper.validate_version("4.2.0-rc.1")

        with self.assertRaisesRegex(ValueError, "Invalid SwiftPM release version"):
            helper.validate_version("4.2.0-nightly.202608280001.1.gabcdef12")

    def test_iter_subspec_names_excludes_non_shipping_subtrees(self):
        spec = {
            "name": "Lynx",
            "subspecs": [
                {"name": "Framework"},
                {
                    "name": "NapiBinding",
                    "subspecs": [{"name": "QuickJS"}],
                },
                {
                    "name": "UnitTests",
                    "subspecs": [{"name": "Fixtures"}],
                },
                {"name": "Replay"},
            ],
        }

        self.assertEqual(
            list(helper.iter_subspec_names(spec, "Lynx")),
            [
                "Lynx/Framework",
                "Lynx/NapiBinding",
                "Lynx/NapiBinding/QuickJS",
            ],
        )

    def test_product_name_replaces_subspec_separator(self):
        self.assertEqual(
            helper.product_name_for_spec("Lynx/NapiBinding/QuickJS"),
            "Lynx-NapiBinding-QuickJS",
        )

    def test_dependency_roots_collects_nested_dependencies(self):
        spec = {
            "name": "Lynx",
            "dependencies": {"LynxBase/Framework": ["= 1.2.3"]},
            "subspecs": [
                {"name": "Framework", "dependencies": {"PrimJS/quickjs": []}},
            ],
        }

        self.assertEqual(
            helper.dependency_roots(spec),
            {"LynxBase", "PrimJS"},
        )

    def test_dependency_closure_handles_cycles(self):
        graph = {
            "Lynx": ["LynxBase", "PrimJS"],
            "LynxBase": ["LynxServiceAPI"],
            "LynxServiceAPI": ["Lynx"],
        }

        self.assertEqual(
            helper.dependency_closure("Lynx", graph),
            {"Lynx", "LynxBase", "LynxServiceAPI", "PrimJS"},
        )

    def test_create_podfile_uses_all_publishable_subspecs(self):
        with tempfile.TemporaryDirectory() as directory:
            work_dir = Path(directory) / "work"
            work_dir.mkdir()
            helper.create_podfile(
                work_dir,
                Path("/repo/lynx"),
                {
                    "Lynx": {
                        "subspecs": [
                            {"name": "Framework"},
                            {"name": "UnitTests"},
                        ]
                    }
                },
            )

            podfile = (work_dir / "Podfile").read_text(encoding="utf-8")

        self.assertIn('pod "Lynx/Framework", :path => "/repo/lynx"', podfile)
        self.assertNotIn("UnitTests", podfile)
        self.assertIn("platform :ios, '12.0'", podfile)
        self.assertIn(
            "config.build_settings['IPHONEOS_DEPLOYMENT_TARGET'] = '12.0'",
            podfile,
        )
        self.assertNotIn("HEADER_SEARCH_PATHS", podfile)
        self.assertIn(
            "config.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] = "
            "'$(inherited) CHROMIUM_ZLIB_NO_CHROMECONF=1'",
            podfile,
        )
        self.assertIn("use_frameworks! :linkage => :static", podfile)
        self.assertIn("installer.pod_targets", podfile)
        self.assertIn(helper.RESOLVED_SPECS_FILE, podfile)

    def test_copy_ssziparchive_zlib_headers(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "repo" / "third_party" / "zlib"
            destination = (
                root
                / "work"
                / "Pods"
                / "SSZipArchive"
                / "SSZipArchive"
                / "minizip"
            )
            source.mkdir(parents=True)
            destination.mkdir(parents=True)
            (source / "zlib.h").write_text("zlib", encoding="utf-8")
            (source / "zconf.h").write_text("zconf", encoding="utf-8")

            helper.copy_ssziparchive_zlib_headers(
                root / "repo",
                root / "work",
            )

            self.assertEqual(
                (destination / "zlib.h").read_text(encoding="utf-8"),
                "zlib",
            )
            self.assertEqual(
                (destination / "zconf.h").read_text(encoding="utf-8"),
                "zconf",
            )

    def test_installed_specs_reads_cocoapods_resolved_export(self):
        with tempfile.TemporaryDirectory() as directory:
            work_dir = Path(directory)
            helper.write_json(
                work_dir / helper.RESOLVED_SPECS_FILE,
                {
                    "Lynx": {"name": "Lynx"},
                    "PrimJS": {"name": "PrimJS"},
                },
            )

            specs = helper.installed_specs(work_dir)

        self.assertEqual(set(specs), {"Lynx", "PrimJS"})

    def test_create_host_project_writes_valid_ruby_source(self):
        with tempfile.TemporaryDirectory() as directory, \
                mock.patch.object(helper, "run") as run:
            work_dir = Path(directory)
            helper.create_host_project(work_dir)
            script = (work_dir / "create_project.rb").read_text(encoding="utf-8")

        self.assertTrue(script.startswith("require 'xcodeproj'\n"))
        self.assertIn(":ios, '12.0'", script)
        self.assertIn("project.save\n", script)
        self.assertNotIn('\n"\n', script)
        run.assert_called_once_with(
            ["bundle", "exec", "ruby", "create_project.rb"],
            cwd=work_dir,
        )

    def test_render_package_creates_binary_and_alias_products(self):
        manifest = self.manifest()

        package = helper.render_package(manifest)

        self.assertIn("platforms: [.iOS(.v12)]", package)
        self.assertIn('name: "Lynx-Framework"', package)
        self.assertIn('name: "Lynx"', package)
        self.assertIn('url: "https://example.com/Lynx.zip"', package)
        self.assertIn('.copy("Resources/LynxResources.bundle")', package)
        self.assertIn('.linkedFramework("WebKit")', package)
        self.assertIn('.linkedLibrary("c++")', package)

    def test_render_package_rejects_artifact_without_url(self):
        manifest = self.manifest()
        manifest["artifacts"][0].pop("url")

        with self.assertRaisesRegex(ValueError, "URL is missing"):
            helper.render_package(manifest)

    def test_render_package_rejects_duplicate_product_names(self):
        manifest = self.manifest()
        manifest["products"].append({
            "name": "Lynx",
            "spec": "Lynx/Alias",
            "targets": ["Lynx"],
        })

        with self.assertRaisesRegex(ValueError, "Duplicate SwiftPM product"):
            helper.render_package(manifest)

    def test_render_package_rejects_invalid_binary_target_name(self):
        manifest = self.manifest()
        manifest["artifacts"][0]["target"] = "Lynx-Core"
        manifest["products"][0]["targets"] = ["Lynx-Core"]
        manifest["products"][1]["targets"] = ["Lynx-Core"]

        with self.assertRaisesRegex(ValueError, "Invalid SwiftPM binary target"):
            helper.render_package(manifest)

    def test_set_github_release_metadata_updates_urls(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest_path = Path(directory) / "PackageArtifacts.json"
            manifest = self.manifest()
            manifest["artifacts"][0].pop("url")
            helper.write_json(manifest_path, manifest)

            helper.set_github_release_metadata(
                manifest_path,
                "lynx-family/lynx",
                "lynx-family/lynx",
                "0123456789abcdef0123456789abcdef01234567",
            )

            release_manifest = helper.read_json(manifest_path)

        self.assertEqual(
            release_manifest["artifacts"][0]["url"],
            "https://github.com/lynx-family/lynx/releases/download/"
            "1.2.3/Lynx-1.2.3.xcframework.zip",
        )
        self.assertEqual(
            release_manifest["github_repository"], "lynx-family/lynx"
        )
        self.assertEqual(
            release_manifest["source_repository"], "lynx-family/lynx"
        )
        self.assertEqual(
            release_manifest["source_revision"],
            "0123456789abcdef0123456789abcdef01234567",
        )

    def test_set_github_release_metadata_requires_complete_source(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest_path = Path(directory) / "PackageArtifacts.json"
            manifest = self.manifest()
            manifest["artifacts"][0].pop("url")
            helper.write_json(manifest_path, manifest)

            with self.assertRaisesRegex(ValueError, "provided together"):
                helper.set_github_release_metadata(
                    manifest_path,
                    "lynx-family/lynx",
                    source_repository="lynx-family/lynx",
                )

    def test_verify_local_artifacts_checks_archive_checksum(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest_path = Path(directory) / "PackageArtifacts.json"
            archive = Path(directory) / "Lynx-1.2.3.xcframework.zip"
            archive.write_bytes(b"archive")
            manifest = self.manifest()
            digest = hashlib.sha256(b"archive").hexdigest()
            manifest["artifacts"][0]["sha256"] = digest
            manifest["artifacts"][0]["checksum"] = digest
            helper.write_json(manifest_path, manifest)

            helper.verify_local_artifacts(manifest_path, expected_version="1.2.3")

            archive.write_bytes(b"tampered")
            with self.assertRaisesRegex(
                    RuntimeError, "Local artifact checksum mismatch"):
                helper.verify_local_artifacts(manifest_path)

    def test_validate_manifest_file_requires_repository(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest_path = Path(directory) / "PackageArtifacts.json"
            helper.write_json(manifest_path, self.manifest())

            with self.assertRaisesRegex(
                    ValueError, "GitHub repository is missing"):
                helper.validate_manifest_file(manifest_path, "1.2.3")

            manifest = self.manifest()
            manifest["artifacts"][0].pop("url")
            helper.write_json(manifest_path, manifest)
            helper.set_github_release_metadata(
                manifest_path,
                "lynx-family/lynx",
                "lynx-family/lynx",
                "0123456789abcdef0123456789abcdef01234567",
            )
            helper.validate_manifest_file(
                manifest_path,
                "1.2.3",
                "lynx-family/lynx",
                "lynx-family/lynx",
                "0123456789abcdef0123456789abcdef01234567",
            )
            with self.assertRaisesRegex(ValueError, "does not match"):
                helper.validate_manifest_file(
                    manifest_path, "1.2.3", "example/lynx"
                )
            with self.assertRaisesRegex(ValueError, "source revision"):
                helper.validate_manifest_file(
                    manifest_path,
                    "1.2.3",
                    "lynx-family/lynx",
                    "lynx-family/lynx",
                    "fedcba9876543210fedcba9876543210fedcba98",
                )

    def test_copy_resource_bundles_keeps_nested_bundle_assets(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            products = root / "Products"
            resources = products / "LynxDebugResources.bundle"
            nested_file = (
                resources / "switchPage" / "devtoolSwitch.lynx.bundle"
            )
            nested_directory = resources / "nested.bundle"
            nested_file.parent.mkdir(parents=True)
            nested_file.write_text("template", encoding="utf-8")
            nested_directory.mkdir()
            (nested_directory / "asset").write_text("asset", encoding="utf-8")

            copied = helper.copy_resource_bundles(
                products,
                root / "Output",
            )

            self.assertEqual(copied, ["LynxDebugResources.bundle"])
            copied_resources = root / "Output" / resources.name
            self.assertEqual(
                (copied_resources / nested_file.relative_to(resources))
                .read_text(encoding="utf-8"),
                "template",
            )
            self.assertTrue(
                (copied_resources / nested_directory.relative_to(resources))
                .is_dir()
            )
            self.assertFalse((root / "Output" / nested_directory.name).exists())

    def test_generate_package_copies_resources_and_metadata(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "repo"
            dist = Path(directory) / "dist"
            resource = dist / "Resources" / "LynxResources.bundle"
            resource.mkdir(parents=True)
            (resource / "lynx_core.js").write_text("core", encoding="utf-8")
            manifest_path = dist / "PackageArtifacts.json"
            helper.write_json(manifest_path, self.manifest())

            helper.generate_package(root, manifest_path)

            self.assertTrue((root / "Package.swift").is_file())
            self.assertTrue(
                (root / "Sources" / helper.PACKAGE_SUPPORT_TARGET / "Resources"
                 / "LynxResources.bundle" / "lynx_core.js").is_file()
            )
            metadata = helper.read_json(root / "spm" / "PackageArtifacts.json")
            self.assertEqual(metadata["version"], "1.2.3")

    def test_verify_remote_checks_version_and_hash(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest_path = Path(directory) / "PackageArtifacts.json"
            helper.write_json(manifest_path, self.manifest())
            with mock.patch.object(
                    helper, "remote_sha256", return_value="a" * 64):
                helper.verify_remote_artifacts(manifest_path, "1.2.3")

            with self.assertRaisesRegex(ValueError, "does not match"):
                helper.verify_remote_artifacts(manifest_path, "1.2.4")

    def test_verify_remote_retries_new_release_asset(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest_path = Path(directory) / "PackageArtifacts.json"
            helper.write_json(manifest_path, self.manifest())
            not_found = HTTPError(
                "https://example.com/Lynx.zip", 404, "Not Found", {}, None
            )
            with mock.patch.object(
                    helper,
                    "remote_sha256",
                    side_effect=[not_found, "a" * 64]), mock.patch.object(
                        helper.time, "sleep") as sleep:
                helper.verify_remote_artifacts(manifest_path, "1.2.3")

        sleep.assert_called_once_with(1)

    @staticmethod
    def manifest():
        digest = "a" * 64
        return {
            "schema_version": 1,
            "version": "1.2.3",
            "deployment_target": helper.IOS_DEPLOYMENT_TARGET,
            "artifacts": [
                {
                    "pod": "Lynx",
                    "target": "Lynx",
                    "file": "Lynx-1.2.3.xcframework.zip",
                    "url": "https://example.com/Lynx.zip",
                    "sha256": digest,
                    "checksum": digest,
                }
            ],
            "products": [
                {"name": "Lynx", "spec": "Lynx", "targets": ["Lynx"]},
                {
                    "name": "Lynx-Framework",
                    "spec": "Lynx/Framework",
                    "targets": ["Lynx"],
                },
            ],
            "system_frameworks": ["WebKit"],
            "weak_frameworks": [],
            "system_libraries": ["stdc++"],
            "resources": ["LynxResources.bundle"],
        }


if __name__ == "__main__":
    unittest.main()
