#!/usr/bin/python3
# -*- coding: utf-8 -*-
"""
Skity CMake Build Script

This script provides a convenient way to build Skity using CMake.
It supports various build configurations and options.
"""

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path


def run_command(cmd, cwd=None, capture_output=False):
    """Execute a command and return the result."""
    print(f"Running: {' '.join(cmd)}")
    try:
        if capture_output:
            result = subprocess.run(
                cmd, cwd=cwd, check=True, capture_output=True, text=True, encoding='utf-8'
            )
            if result.stdout:
                print(result.stdout)
        else:
            # Stream output in real-time
            result = subprocess.run(cmd, cwd=cwd, check=True, encoding='utf-8')
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error: Command failed with return code {e.returncode}")
        return False
    except FileNotFoundError as e:
        print(f"Error: Command not found - {e}")
        return False


def ensure_symlink(link_path, target_path):
    """Create or refresh a symlink at link_path pointing to target_path.

    The skity CMake build (and headers like "skity/include/skity/io/data.hpp")
    expect a co-located source layout. In this OSS checkout the deps live at
    sibling paths under lynx/third_party, so we materialize the expected layout
    via symlinks. Re-running is safe; broken or wrong-target links get rewired.
    """
    link = Path(link_path)
    target = Path(target_path)
    if not target.exists():
        return
    try:
        if link.is_symlink() or link.exists():
            if link.is_symlink() and Path(os.readlink(str(link))) == target:
                return
            if link.is_symlink() or link.is_file():
                link.unlink()
            else:
                # Empty directories left by hab sync — remove so we can symlink.
                try:
                    link.rmdir()
                except OSError:
                    return
        link.parent.mkdir(parents=True, exist_ok=True)
        os.symlink(str(target), str(link))
        print(f"Linked {link} -> {target}")
    except OSError as e:
        print(f"Warning: failed to create symlink {link} -> {target}: {e}")


def prepare_skity_layout(skity_root):
    """Ensure skity's expected sibling/third_party layout exists.

    `tools/hab sync` populates lynx/third_party/{libwebp,libjpeg-turbo,wuffs}
    but leaves lynx/third_party/skity/third_party/* as empty directories, and
    the in-tree skity headers are referenced via "skity/include/...". Wire
    those up here so CMake configure and source-set compilation succeed
    without manual setup after every sync.
    """
    skity_path = Path(skity_root).resolve()
    lynx_third_party = skity_path.parent

    # Self-loop so #include "skity/include/skity/io/data.hpp" resolves when
    # the include search path points at lynx/third_party.
    ensure_symlink(skity_path / "skity", skity_path)

    skity_third_party = skity_path / "third_party"
    skity_third_party.mkdir(parents=True, exist_ok=True)
    for dep in ("libwebp", "libjpeg-turbo", "wuffs"):
        ensure_symlink(skity_third_party / dep, lynx_third_party / dep)


def configure_cmake(
    build_dir,
    build_type,
    enable_example,
    enable_test,
    enable_codec,
    enable_install,
    install_prefix,
    additional_args,
    skity_root,
):
    """Configure CMake build."""

    # Create build directory if it doesn't exist
    build_path = Path(build_dir)
    build_path.mkdir(parents=True, exist_ok=True)

    # Prepare CMake command
    cmake_cmd = [
        "cmake",
        "-B",
        str(build_path),
        "-S",
        str(skity_root),
        f"-DCMAKE_BUILD_TYPE={build_type}",
    ]

    # Add optional flags (explicitly set to ensure default behavior)
    cmake_cmd.append(f"-DSKITY_EXAMPLE={'ON' if enable_example else 'OFF'}")
    cmake_cmd.append(f"-DSKITY_TEST={'ON' if enable_test else 'OFF'}")
    cmake_cmd.append(f"-DSKITY_CODEC_MODULE={'ON' if enable_codec else 'OFF'}")
    cmake_cmd.append("-DSKITY_CMAKE_TO_GN=ON")

    if enable_install:
        cmake_cmd.append("-DSKITY_INSTALL=ON")

    if install_prefix:
        cmake_cmd.append(f"-DCMAKE_INSTALL_PREFIX={install_prefix}")

    cmake_cmd.append("-DCMAKE_POLICY_VERSION_MINIMUM=3.5")

    # Add platform-specific flags
    if platform.system() == "Linux":
        cmake_cmd.append("-DCMAKE_POSITION_INDEPENDENT_CODE=ON")
        cmake_cmd.append("-DCMAKE_CXX_FLAGS=-fPIC")
        cmake_cmd.append("-DCMAKE_C_FLAGS=-fPIC")
    elif platform.system() == "Darwin":
        # macOS: Set deployment target to 10.15 to match API requirements
        cmake_cmd.append("-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15")
        # Add compiler flags to suppress warnings that are treated as errors
        # and specifically avoid treating -Wmismatched-tags as an error.
        macos_warning_flags = (
            "-Wno-error=mismatched-tags "
            "-Wno-mismatched-tags "
            "-Wno-unused-private-field "
            "-Wno-unguarded-availability-new "
            "-Wno-unused-function "
            "-Wno-sign-compare"
        )
        cmake_cmd.append(f"-DCMAKE_CXX_FLAGS={macos_warning_flags}")
        cmake_cmd.append(f"-DCMAKE_C_FLAGS={macos_warning_flags}")
        cmake_cmd.append(f"-DCMAKE_OBJCXX_FLAGS={macos_warning_flags}")
        cmake_cmd.append("-DSKITY_CT_FONT=ON")
    elif platform.system() == "Windows":
        cmake_cmd.append("-DCMAKE_CXX_STANDARD=20")
        cmake_cmd.append("-DCMAKE_CXX_STANDARD_REQUIRED=ON")
        cmake_cmd.append("-DCMAKE_CXX_EXTENSIONS=OFF")
        # Enable exceptions for clang-cl
        cmake_cmd.append("-DCMAKE_CXX_FLAGS=/EHsc")
        cmake_cmd.append("-DCMAKE_C_FLAGS=/EHsc")

    # Append any user-provided extra CMake arguments at the end so they can
    # override defaults above if necessary (e.g. custom CMAKE_CXX_FLAGS).
    if additional_args:
        cmake_cmd.extend(additional_args)

    print(f"Configuring CMake in {build_path}...")
    print(f"Build type: {build_type}")
    print(f"Cmake cmd: {cmake_cmd}")
    return run_command(cmake_cmd, capture_output=False)


def generate_gni_files(build_dir, skity_root, enable_codec=False):
    """Generate skity.gni and skity-codec.gni files using CMake.

    This function will exit the program with minimal output if any error occurs.
    """
    build_path = Path(build_dir)

    # Generate GNI files
    targets = ["skity.gni"]
    if enable_codec:
        targets.append("skity-codec.gni")

    gni_cmd = ["cmake", "--build", str(build_path), "--target"] + targets

    try:
        success = run_command(gni_cmd, capture_output=True)
        return success
    except:
        return False


def dedupe_gni_list(gni_path, list_name):
    """Keep generated GN list entries ordered while removing duplicates."""
    path = Path(gni_path)
    if not path.exists():
        return

    lines = path.read_text(encoding="utf-8").splitlines(True)
    output = []
    seen = set()
    in_list = False

    for line in lines:
        stripped = line.strip()
        if stripped == f"{list_name} = [":
            in_list = True
            seen.clear()
            output.append(line)
            continue

        if in_list:
            if stripped == "]":
                in_list = False
                output.append(line)
                continue

            if stripped.startswith("\"") and stripped.endswith("\","):
                if stripped in seen:
                    continue
                seen.add(stripped)

        output.append(line)

    path.write_text("".join(output), encoding="utf-8")


def main():
    # Add buildtools/cmake/bin to PATH
    script_dir = Path(__file__).parent.resolve()
    project_root = script_dir.parent.parent
    cmake_bin_dir = project_root / "buildtools" / "cmake" / "bin"
    if cmake_bin_dir.exists():
        os.environ["PATH"] = str(cmake_bin_dir) + os.pathsep + os.environ["PATH"]
        print(f"Added {cmake_bin_dir} to PATH")

    parser = argparse.ArgumentParser(
        description="Build Skity using CMake",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""Examples:
  %(prog)s --build-type Debug --example
  %(prog)s --build-type Release --test --codec
  %(prog)s --build-dir out/custom --build-type RelWithDebInfo --example --test -j 8
  %(prog)s --build-type Release --install -j 4
  %(prog)s --build-dir /custom/path --install --install-prefix /usr/local
  %(prog)s --skity-root /path/to/skity --build-dir out/release --build-type Release
        """,
    )

    parser.add_argument(
        "--skity-root",
        "-s",
        type=str,
        help="Path to Skity root directory (default: auto-detect from script location)",
    )

    parser.add_argument(
        "--build-dir",
        "-b",
        type=str,
        help="Build directory (default: auto-generate based on skity-root and build-type)",
    )

    parser.add_argument(
        "--build-type",
        "-t",
        choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"],
        default="Release",
        help="CMake build type (default: Release)",
    )

    parser.add_argument(
        "--example",
        "-e",
        action="store_true",
        default=False,
        help="Enable building examples (SKITY_EXAMPLE=ON)",
    )

    parser.add_argument(
        "--test",
        action="store_true",
        default=False,
        help="Enable building tests (SKITY_TEST=ON)",
    )

    parser.add_argument(
        "--codec",
        "-c",
        action="store_true",
        default=True,
        help="Enable codec module (SKITY_CODEC_MODULE=ON)",
    )

    parser.add_argument(
        "--install",
        "-i",
        action="store_true",
        help="Enable install target (SKITY_INSTALL=ON)",
    )

    parser.add_argument(
        "--install-prefix",
        type=str,
        help="Custom install prefix path (CMAKE_INSTALL_PREFIX)",
    )

    parser.add_argument("--jobs", "-j", type=int, help="Number of parallel build jobs")

    parser.add_argument(
        "--configure-only",
        action="store_true",
        default=True,
        help="Only configure CMake, don't build",
    )

    parser.add_argument(
        "--generate-gni",
        action="store_true",
        default=True,
        help="Generate skity.gni and skity-codec.gni files",
    )

    parser.add_argument("--cmake_args", nargs="*", help="Additional CMake arguments")

    args = parser.parse_args()

    # Determine skity root directory
    if args.skity_root:
        skity_root = Path(args.skity_root)
    else:
        # Auto-detect skity root from script location
        script_dir = Path(__file__).parent.parent.parent
        skity_root = script_dir / "third_party" / "skity"

    # Determine build directory
    if args.build_dir:
        build_dir = Path(args.build_dir)
    else:
        # Auto-generate build directory based on skity-root and build-type
        build_dir = script_dir / "out" / "skity_build"

    # Configure step
    prepare_skity_layout(str(skity_root))
    success = configure_cmake(
        str(build_dir),
        args.build_type,
        args.example,
        args.test,
        args.codec,
        args.install,
        args.install_prefix,
        args.cmake_args,
        str(skity_root),
    )

    # Generate GNI files if requested
    if success and args.generate_gni:
        print("\nGenerating GNI files...")
        # generate_gni_files will exit the program if it fails
        success = generate_gni_files(str(build_dir), str(skity_root), args.codec)
        if success:
            dedupe_gni_list(Path(skity_root) / "skity.gni", "skity_sources")

    if success:
        print("\n Build completed successfully!")
        if args.example and not args.configure_only:
            print(f"\nTo run examples, check the binaries in: {build_dir}")
        if args.generate_gni:
            print(f"\nGNI files generated in: {build_dir}")
    else:
        print("\n Build failed!")
        sys.exit(1)


if __name__ == "__main__":
    main()
