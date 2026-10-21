#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
# cspell:ignore appex extensionless otool

"""Verify the ownership of Lynx and Sparkling in a built Explorer app.

The CocoaPods lockfile verifier proves where the build inputs came from.  This
script complements it at the artifact boundary: it inspects embedded
frameworks, Mach-O load commands, exported strong symbols, and the linker map.
"""

import argparse
import plistlib
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path


VALID_MODES = {"enable_sparkling", "disable_sparkling"}
OBJECT_LINE = re.compile(r"^\[\s*(\d+)\]\s+(.+?)\s*$")
SYMBOL_LINE = re.compile(
    r"^\S+\s+\S+\s+\[\s*(\d+)\]\s+(.+?)\s*$"
)
LYNX_CPP_SYMBOL = re.compile(r"(?:^|_)_?(?:Z|ZT|ZN).*4lynx(?:\d|[A-Z_])")
LYNX_OBJC_SYMBOL = re.compile(
    r"(?:OBJC_(?:CLASS|METACLASS)_\$_)?(?:Lynx|LYNX)[A-Za-z0-9_]+"
)
SPARKLING_SWIFT_SYMBOL = re.compile(r"\$s\d+Sparkling[A-Za-z0-9_]*")
SPARKLING_OBJC_SYMBOL = re.compile(
    r"(?:OBJC_(?:CLASS|METACLASS)_\$_)?SPK[A-Z][A-Za-z0-9_]+"
)
LYNX_RUNTIME_OBJC_SYMBOL = re.compile(
    r"^_+OBJC_(?:CLASS|METACLASS)_\$_(?:LynxEnv|LynxView)$"
)
ROOT_MANGLED_LYNX_NAMESPACE = re.compile(
    r"^_+ZN(?:[KVRrO]+)?4lynx(?P<length>\d+)"
    r"(?P<namespace>[A-Za-z_][A-Za-z0-9_]*)"
)
# Shared utility namespaces such as base, fml, and shell are intentionally
# absent: dependent pods instantiate their header templates and therefore own
# some such symbols without owning the Lynx runtime implementation.
LYNX_RUNTIME_CPP_NAMESPACES = {
    "animation",
    "css",
    "debug",
    "devtool",
    "error",
    "event",
    "gfx",
    "lepus",
    "list",
    "parser",
    "perfetto",
    "pub",
    "runtime",
    "service",
    "ssr",
    "starlight",
    "style",
    "tasm",
    "template_codec",
    "trace",
    "transforms",
    "value",
    "worklet",
}
# Podfile uses `use_frameworks! :linkage => :static`. These source-owned pod
# targets therefore contribute archives beside LynxExplorer.app in Xcode's
# configuration products directory; none of them belongs inside the app.
LOCAL_LYNX_STATIC_PRODUCTS = {
    "BaseDevtool",
    "Lynx",
    "LynxBase",
    "LynxDevtool",
    "LynxLibraryRegistry",
    "LynxService",
    "LynxServiceAPI",
    "XElement",
}
SPARKLING_GO_RESOURCE = Path("Resource/extensions/sparkling-go/main.lynx.bundle")


def _run_tool(arguments):
    return subprocess.run(
        ["xcrun", *arguments],
        check=False,
        capture_output=True,
        text=True,
    )


def _is_lynx_framework(path):
    return path.name.casefold().startswith("lynx") and path.suffix == ".framework"


def _frameworks(app_path):
    """Return framework directories, including aliases but not their contents."""
    found = []
    pending = [app_path]
    while pending:
        directory = pending.pop()
        try:
            children = list(directory.iterdir())
        except OSError:
            continue
        for child in children:
            if child.name.endswith(".framework") and child.is_dir():
                found.append(child)
            elif child.is_dir() and not child.is_symlink():
                pending.append(child)
    return sorted(found, key=lambda path: str(path))


def _bundle_executable(bundle):
    executable_name = None
    plist_path = bundle / "Info.plist"
    try:
        with plist_path.open("rb") as stream:
            executable_name = plistlib.load(stream).get("CFBundleExecutable")
    except (OSError, plistlib.InvalidFileException, AttributeError):
        pass
    if not isinstance(executable_name, str) or not executable_name:
        executable_name = bundle.stem
    return bundle / executable_name


def _macho_binaries(app_path, frameworks):
    candidates = [_bundle_executable(app_path)]
    candidates.extend(_bundle_executable(framework) for framework in frameworks)
    candidates.extend(
        _bundle_executable(bundle)
        for pattern in ("*.appex", "*.app")
        for bundle in app_path.rglob(pattern)
    )
    candidates.extend(app_path.rglob("*.dylib"))
    unique = {}
    for candidate in candidates:
        if candidate.is_file():
            unique[str(candidate.resolve(strict=False))] = candidate
    return sorted(unique.values(), key=lambda path: str(path))


def _read_load_commands(binary):
    result = _run_tool(["otool", "-L", str(binary)])
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip())
    commands = []
    for line in result.stdout.splitlines()[1:]:
        command = line.strip().split(" (", 1)[0]
        if command:
            commands.append(command)
    return commands


def _read_strong_symbols(binary):
    # -g: external, -m: extended Mach-O type, -U: defined symbols only.  The
    # extended form lets us exclude weak definitions instead of confusing them
    # with duplicate strong implementations.
    result = _run_tool(["nm", "-gmU", str(binary)])
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip())
    symbols = set()
    for line in result.stdout.splitlines():
        line = line.strip()
        if not line or " weak " in f" {line.lower()} ":
            continue
        symbols.add(line.rsplit(None, 1)[-1])
    return symbols


def _parse_link_map(path):
    owners = {}
    symbols = []
    section = None
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.startswith("# Object files:"):
            section = "objects"
            continue
        if line.startswith("# Symbols:"):
            section = "symbols"
            continue
        if line.startswith("#"):
            if line.endswith(":"):
                section = None
            continue
        if section == "objects":
            match = OBJECT_LINE.match(line)
            if match:
                owners[int(match.group(1))] = match.group(2)
        elif section == "symbols":
            match = SYMBOL_LINE.match(line)
            if match:
                symbols.append((int(match.group(1)), match.group(2)))
    return owners, symbols


def _owner_archive(owner):
    """Return the archive path for .a files and extensionless static frameworks."""
    archive_member = re.match(r"^(.*)\([^()]+\)$", owner)
    return archive_member.group(1) if archive_member else owner


def _owner_identity(owner):
    # The archive/framework is the implementation owner, not each member object.
    return _owner_archive(owner)


def _local_lynx_static_owners(app_path):
    products_root = app_path.parent.resolve(strict=False)
    return {
        (
            products_root
            / product
            / f"{product}.framework"
            / product
        ).resolve(strict=False)
        for product in LOCAL_LYNX_STATIC_PRODUCTS
    }


def _lynx_cpp_namespace(symbol):
    match = ROOT_MANGLED_LYNX_NAMESPACE.match(symbol)
    if not match:
        return None
    length = int(match.group("length"))
    namespace = match.group("namespace")
    if len(namespace) < length:
        return None
    return namespace[:length]


def _is_lynx_runtime_symbol(symbol):
    return bool(
        LYNX_RUNTIME_OBJC_SYMBOL.fullmatch(symbol)
        or symbol.startswith(("_lynx_", "__lynx_"))
        or _lynx_cpp_namespace(symbol) in LYNX_RUNTIME_CPP_NAMESPACES
    )


def _is_linker_synthesized_owner(owner):
    return owner.strip().casefold() == "linker synthesized"


def _is_lynx_symbol(symbol):
    return bool(
        LYNX_CPP_SYMBOL.search(symbol)
        or LYNX_OBJC_SYMBOL.search(symbol)
        or symbol.startswith(("_lynx_", "__lynx_"))
    )


def _is_sparkling_symbol(symbol):
    return bool(
        SPARKLING_SWIFT_SYMBOL.search(symbol)
        or SPARKLING_OBJC_SYMBOL.search(symbol)
        or symbol.startswith(("_SPK", "__SPK"))
    )


def _is_lynx_install_name(command):
    return bool(
        re.search(
            r"(?:^|/)(?:Lynx[A-Za-z0-9_-]*)\.framework/(?:Lynx[A-Za-z0-9_-]*)$",
            command,
            re.IGNORECASE,
        )
    )


def _duplicate_symbol_errors(symbol_owners):
    errors = []
    for symbol, owners in sorted(symbol_owners.items()):
        identities = sorted({_owner_identity(str(owner)) for owner in owners})
        if _is_lynx_symbol(symbol) and len(identities) > 1:
            errors.append(
                "duplicate strong Lynx symbol "
                f"{symbol} across owners: {', '.join(identities)}"
            )
    return errors


def verify_artifact(
    mode,
    app_path,
    link_map_path,
    derived_data_path,
    lynx_root,
    load_commands=None,
    strong_symbols=None,
):
    """Return all artifact ownership violations without mutating the artifact."""
    errors = []
    app_path = Path(app_path)
    link_map_path = Path(link_map_path)
    if mode not in VALID_MODES:
        return ["mode must be 'enable_sparkling' or 'disable_sparkling'"]
    if not app_path.is_dir():
        return [f"app bundle is missing at {app_path}"]
    if not link_map_path.is_file():
        return [f"link map is missing at {link_map_path}"]

    frameworks = _frameworks(app_path)
    lynx_frameworks = [path for path in frameworks if _is_lynx_framework(path)]

    for framework in lynx_frameworks:
        errors.append(
            "embedded Lynx framework is forbidden with static CocoaPods linkage: "
            f"{framework}"
        )

    by_basename = defaultdict(list)
    for framework in lynx_frameworks:
        by_basename[framework.name.casefold()].append(framework)
    for paths in by_basename.values():
        if len(paths) > 1:
            errors.append(
                "duplicate Lynx framework basename "
                f"'{paths[0].name}': {', '.join(map(str, paths))}"
            )

    by_realpath = defaultdict(list)
    for framework in frameworks:
        by_realpath[str(framework.resolve(strict=False))].append(framework)
    for realpath, paths in by_realpath.items():
        if len(paths) > 1 and any(_is_lynx_framework(path) for path in paths):
            errors.append(
                "duplicate Lynx framework realpath "
                f"'{realpath}': {', '.join(map(str, paths))}"
            )

    binaries = _macho_binaries(app_path, frameworks)
    if load_commands is None:
        load_commands = {}
        for binary in binaries:
            try:
                load_commands[binary] = _read_load_commands(binary)
            except RuntimeError as error:
                errors.append(f"cannot inspect load commands for {binary}: {error}")
    for binary, commands in load_commands.items():
        for command in commands:
            if _is_lynx_install_name(command):
                errors.append(
                    f"Lynx install name is forbidden with static CocoaPods linkage "
                    f"in {binary}: {command}"
                )

    if strong_symbols is None:
        strong_symbols = {}
        for binary in binaries:
            try:
                strong_symbols[binary] = _read_strong_symbols(binary)
            except RuntimeError as error:
                errors.append(f"cannot inspect strong symbols for {binary}: {error}")

    binary_symbol_owners = defaultdict(set)
    all_symbols = set()
    for binary, symbols in strong_symbols.items():
        for symbol in symbols:
            binary_symbol_owners[symbol].add(str(binary))
            all_symbols.add(symbol)

    try:
        link_owners, link_symbols = _parse_link_map(link_map_path)
    except OSError as error:
        errors.append(f"cannot read link map {link_map_path}: {error}")
        link_owners, link_symbols = {}, []
    for _, symbol in link_symbols:
        all_symbols.add(symbol)
    runtime_symbols = []
    for identifier, symbol in link_symbols:
        if not _is_lynx_runtime_symbol(symbol):
            continue
        owner = link_owners.get(identifier, f"link-map object {identifier}")
        # Link maps attribute coalesced globals and stubs to the linker. They do
        # not identify an implementation input and therefore cannot establish
        # (or violate) CocoaPods provenance.
        if _is_linker_synthesized_owner(owner):
            continue
        runtime_symbols.append((symbol, owner))
    if not runtime_symbols:
        errors.append("link map has no Lynx runtime symbols")
    else:
        local_owners = _local_lynx_static_owners(app_path)
        for symbol, owner in runtime_symbols:
            archive = Path(_owner_archive(owner)).resolve(strict=False)
            if archive not in local_owners:
                errors.append(
                    "non-local Lynx runtime symbol owner "
                    f"for {symbol}: {owner}"
                )

    # The app link map describes the inputs that produced one Mach-O image;
    # exported-symbol scans describe the finished images.  Comparing the two
    # sets directly would count the same implementation twice.
    errors.extend(_duplicate_symbol_errors(binary_symbol_owners))

    sparkling_symbols = sorted(symbol for symbol in all_symbols if _is_sparkling_symbol(symbol))
    sparkling_go_resource = app_path / SPARKLING_GO_RESOURCE
    if mode == "enable_sparkling":
        if not sparkling_symbols:
            errors.append("enable_sparkling artifact has no Sparkling symbols")
        if not sparkling_go_resource.is_file():
            errors.append(
                "enable_sparkling artifact is missing Sparkling Go resource "
                f"{SPARKLING_GO_RESOURCE}"
            )
    if mode == "disable_sparkling":
        errors.extend(
            f"disable_sparkling artifact contains Sparkling symbol {symbol}"
            for symbol in sparkling_symbols
        )
        if sparkling_go_resource.exists():
            errors.append(
                "disable_sparkling artifact contains Sparkling Go resource "
                f"{SPARKLING_GO_RESOURCE}"
            )

    # A deterministic order keeps CI diagnostics and fixture assertions stable.
    return sorted(set(errors))


def _parser():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", required=True, choices=sorted(VALID_MODES))
    parser.add_argument("--app", required=True, type=Path)
    parser.add_argument("--link-map", required=True, type=Path)
    parser.add_argument("--derived-data", required=True, type=Path)
    parser.add_argument("--lynx-root", required=True, type=Path)
    return parser


def main(argv=None):
    arguments = _parser().parse_args(argv)
    errors = verify_artifact(
        arguments.mode,
        arguments.app,
        arguments.link_map,
        arguments.derived_data,
        arguments.lynx_root,
    )
    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1
    print(
        f"linked framework verification passed ({arguments.mode}): "
        f"{arguments.app}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
