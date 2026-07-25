#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Update or check generated Lynx DevTool CDP metadata."""

from __future__ import annotations

import argparse
import difflib
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


UPSTREAM_ROOT = "https://chromedevtools.github.io/devtools-protocol/tot"
V8_CDP_ROOT = "https://chromedevtools.github.io/devtools-protocol/1-3"
FORWARDED_JS_ENGINE_DOMAINS = frozenset(
    {"Debugger", "Runtime", "HeapProfiler", "Profiler"}
)
METHOD_NAME_PATTERN = r"[A-Za-z][A-Za-z0-9]*\.[A-Za-z][A-Za-z0-9_]*"
METHOD_RE = re.compile(
    rf'functions_map_\["({METHOD_NAME_PATTERN})"\]'
)
PRIMJS_FUNCTION_RE = re.compile(
    r"const\s+debug_function_type\s*&\s*GetDebugFunctionMap\s*\(\)\s*\{",
    re.MULTILINE,
)
PRIMJS_METHOD_RE = re.compile(rf'\{{\s*"({METHOD_NAME_PATTERN})"\s*,')
REGISTER_FUNCTION_RE = re.compile(
    r"void\s+LynxDevToolNG::Register(Global|Instance)DomainAgents\s*\([^)]*\)\s*\{",
    re.MULTILINE,
)
REGISTER_AGENT_RE = re.compile(
    r'(?:\bglobal_dispatcher\.)?RegisterAgent\s*\(\s*"([A-Za-z][A-Za-z0-9]*)"'
)


@dataclass(frozen=True)
class Paths:
    source_root: Path
    protocol_dir: Path
    upstream_schema: Path
    manifest: Path
    custom_docs_dir: Path
    domain_agent_dir: Path
    registration_source: Path
    primjs_protocols_source: Path


@dataclass(frozen=True)
class LocalMethod:
    domain: str
    method: str
    source: str


class MetadataError(Exception):
    pass


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Update or check generated Lynx DevTool CDP metadata."
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true", help="rewrite generated metadata")
    mode.add_argument("--check", action="store_true", help="check generated metadata")
    args = parser.parse_args()

    paths = get_paths()
    try:
        upstream_methods = load_upstream_methods(paths.upstream_schema)
        local_methods = scan_local_methods(paths)
        scopes = scan_domain_scopes(paths.registration_source)
        manifest_text = build_manifest(paths, local_methods, scopes, upstream_methods)

        if args.write:
            paths.manifest.write_text(manifest_text, encoding="utf-8")
            missing_docs = find_missing_custom_docs(
                paths, local_methods, upstream_methods, check_existing_manifest=False
            )
            print(f"Updated {repo_relative_path(paths, paths.manifest)}")
            if missing_docs:
                print()
                print("Missing custom CDP docs:")
                for doc_path in missing_docs:
                    print(f"  {doc_path}")
            return 0

        return check_manifest(paths, manifest_text, local_methods, upstream_methods)
    except MetadataError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


def get_paths() -> Paths:
    script_path = Path(__file__).resolve()
    protocol_dir = script_path.parents[1]
    lynx_devtool_dir = protocol_dir.parent
    devtool_dir = lynx_devtool_dir.parent
    source_root = devtool_dir.parent
    return Paths(
        source_root=source_root,
        protocol_dir=protocol_dir,
        upstream_schema=protocol_dir / "cdp_upstream" / "protocol.json",
        manifest=protocol_dir / "cdp_manifest.generated.yaml",
        custom_docs_dir=protocol_dir / "custom_cdp_docs",
        domain_agent_dir=lynx_devtool_dir / "agent" / "domain_agent",
        registration_source=lynx_devtool_dir / "lynx_devtool_ng.cc",
        primjs_protocols_source=source_root
        / "third_party"
        / "quickjs"
        / "src"
        / "src"
        / "inspector"
        / "protocols.cc",
    )


def load_upstream_methods(schema_path: Path) -> dict[str, set[str]]:
    if not schema_path.exists():
        raise MetadataError(
            f"missing vendored CDP schema: {schema_path}\n"
            "Add devtool/lynx_devtool/protocol/cdp_upstream/protocol.json first."
        )
    try:
        schema = json.loads(schema_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise MetadataError(f"invalid JSON in {schema_path}: {exc}") from exc

    domains = schema.get("domains")
    if not isinstance(domains, list):
        raise MetadataError(f"{schema_path} must contain a top-level domains array")

    upstream: dict[str, set[str]] = {}
    for domain_entry in domains:
        if not isinstance(domain_entry, dict):
            continue
        domain = domain_entry.get("domain")
        if not isinstance(domain, str):
            continue
        commands = domain_entry.get("commands", [])
        if not isinstance(commands, list):
            commands = []
        methods = {
            command["name"]
            for command in commands
            if isinstance(command, dict) and isinstance(command.get("name"), str)
        }
        upstream[domain] = methods
    return upstream


def scan_local_methods(paths: Paths) -> list[LocalMethod]:
    if not paths.domain_agent_dir.exists():
        raise MetadataError(f"missing domain agent directory: {paths.domain_agent_dir}")

    methods: dict[tuple[str, str], LocalMethod] = {}
    for source_file in sorted(paths.domain_agent_dir.glob("*.cc")):
        if source_file.name.endswith("_unittest.cc"):
            continue
        text = source_file.read_text(encoding="utf-8")
        for full_method in METHOD_RE.findall(text):
            domain, method = full_method.split(".", 1)
            if domain in FORWARDED_JS_ENGINE_DOMAINS:
                continue
            key = (domain, method)
            source = repo_relative_path(paths, source_file)
            add_local_method(methods, domain, method, source, full_method)

    scan_primjs_methods(paths, methods)

    return [methods[key] for key in sorted(methods)]


def scan_primjs_methods(
    paths: Paths, methods: dict[tuple[str, str], LocalMethod]
) -> None:
    source_file = paths.primjs_protocols_source
    if not source_file.exists():
        raise MetadataError(f"missing PrimJS CDP method map: {source_file}")

    text = source_file.read_text(encoding="utf-8")
    match = PRIMJS_FUNCTION_RE.search(text)
    if not match:
        raise MetadataError(
            f"cannot find GetDebugFunctionMap() in {repo_relative_path(paths, source_file)}"
        )

    body = extract_braced_block(text, match.end() - 1)
    source = repo_relative_path(paths, source_file)
    found = False
    for full_method in PRIMJS_METHOD_RE.findall(body):
        domain, method = full_method.split(".", 1)
        add_local_method(methods, domain, method, source, full_method)
        found = True

    if not found:
        raise MetadataError(
            f"no CDP methods found in {repo_relative_path(paths, source_file)}"
        )


def add_local_method(
    methods: dict[tuple[str, str], LocalMethod],
    domain: str,
    method: str,
    source: str,
    full_method: str,
) -> None:
    key = (domain, method)
    existing = methods.get(key)
    if existing and existing.source != source:
        raise MetadataError(
            f"duplicate method declaration for {full_method}: "
            f"{existing.source} and {source}"
        )
    methods[key] = LocalMethod(domain=domain, method=method, source=source)


def scan_domain_scopes(source_file: Path) -> dict[str, str]:
    if not source_file.exists():
        raise MetadataError(f"missing domain registration source: {source_file}")
    text = source_file.read_text(encoding="utf-8")

    domain_scopes: dict[str, set[str]] = {}
    for match in REGISTER_FUNCTION_RE.finditer(text):
        scope = "global" if match.group(1) == "Global" else "instance"
        body = extract_braced_block(text, match.end() - 1)
        for domain in REGISTER_AGENT_RE.findall(body):
            domain_scopes.setdefault(domain, set()).add(scope)

    scopes: dict[str, str] = {}
    for domain, found_scopes in domain_scopes.items():
        if found_scopes == {"global"}:
            scopes[domain] = "global"
        elif found_scopes == {"instance"}:
            scopes[domain] = "instance"
        elif found_scopes == {"global", "instance"}:
            scopes[domain] = "global-and-instance"
        else:
            raise MetadataError(f"unexpected scope set for {domain}: {found_scopes}")
    return scopes


def extract_braced_block(text: str, open_brace_index: int) -> str:
    if text[open_brace_index] != "{":
        raise MetadataError("internal parser error: expected opening brace")
    depth = 0
    for index in range(open_brace_index, len(text)):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return text[open_brace_index + 1 : index]
    raise MetadataError("unterminated function body while scanning domain registration")


def build_manifest(
    paths: Paths,
    local_methods: list[LocalMethod],
    scopes: dict[str, str],
    upstream_methods: dict[str, set[str]],
) -> str:
    domains: dict[str, list[LocalMethod]] = {}
    for method in local_methods:
        domains.setdefault(method.domain, []).append(method)

    missing_scope = sorted(domain for domain in domains if domain not in scopes)
    if missing_scope:
        raise MetadataError(
            "methods found for unregistered domains: " + ", ".join(missing_scope)
        )

    lines = [
        "# Copyright 2026 The Lynx Authors. All rights reserved.",
        "# Licensed under the Apache License Version 2.0 that can be found in the",
        "# LICENSE file in the root directory of this source tree.",
        "#",
        "# Generated by devtool/lynx_devtool/protocol/scripts/update_cdp_metadata.py.",
        "# Do not edit by hand.",
        "",
        "version: 2",
        "generatedFrom:",
        "  chromeDevToolsProtocol: cdp_upstream/protocol.json",
        f"upstreamRoot: {yaml_scalar(UPSTREAM_ROOT)}",
        "externalReferences:",
        "  v8:",
        "    displayName: V8",
        f"    cdpRoot: {yaml_scalar(V8_CDP_ROOT)}",
        "domains:",
    ]

    for domain in sorted(domains):
        domain_origin = "standard" if domain in upstream_methods else "lynx-extension"
        lines.extend(
            [
                f"  - name: {yaml_scalar(domain)}",
                f"    origin: {domain_origin}",
                f"    scope: {scopes[domain]}",
                "    methods:",
            ]
        )
        for method in sorted(domains[domain], key=lambda item: item.method):
            method_origin = classify_method(method, upstream_methods)
            lines.extend(
                [
                    f"      - name: {yaml_scalar(method.method)}",
                    f"        origin: {method_origin}",
                    f"        source: {yaml_scalar(method.source)}",
                ]
            )
    lines.append("")
    return "\n".join(lines)


def classify_method(
    method: LocalMethod, upstream_methods: dict[str, set[str]]
) -> str:
    upstream_domain_methods = upstream_methods.get(method.domain)
    if upstream_domain_methods is None:
        return "lynx-extension"
    if method.method in upstream_domain_methods:
        return "standard"
    return "lynx-extension"


def yaml_scalar(value: str) -> str:
    if re.fullmatch(r"[A-Za-z0-9_./:#-]+", value):
        return value
    return json.dumps(value)


def check_manifest(
    paths: Paths,
    expected: str,
    local_methods: list[LocalMethod],
    upstream_methods: dict[str, set[str]],
) -> int:
    if not paths.manifest.exists():
        print("DevTool CDP metadata is missing.", file=sys.stderr)
        print_update_command()
        return 1

    actual = paths.manifest.read_text(encoding="utf-8")
    failed = False
    if actual != expected:
        print("DevTool CDP metadata is stale.", file=sys.stderr)
        print_update_command()
        print(file=sys.stderr)
        for line in difflib.unified_diff(
            actual.splitlines(),
            expected.splitlines(),
            fromfile=repo_relative_path(paths, paths.manifest),
            tofile=f"{repo_relative_path(paths, paths.manifest)} (expected)",
            lineterm="",
        ):
            print(line, file=sys.stderr)
        failed = True

    missing_docs = find_missing_custom_docs(
        paths, local_methods, upstream_methods, check_existing_manifest=True
    )
    if missing_docs:
        if failed:
            print(file=sys.stderr)
        print("DevTool CDP custom docs are missing.", file=sys.stderr)
        for doc_path in missing_docs:
            print(f"  {doc_path}", file=sys.stderr)
        failed = True

    return 1 if failed else 0


def find_missing_custom_docs(
    paths: Paths,
    local_methods: list[LocalMethod],
    upstream_methods: dict[str, set[str]],
    check_existing_manifest: bool,
) -> list[str]:
    missing_docs: list[str] = []
    for method in local_methods:
        if classify_method(method, upstream_methods) == "standard":
            continue
        doc_file = (
            paths.custom_docs_dir / method.domain / "methods" / f"{method.method}.yaml"
        )
        if not doc_file.exists():
            missing_docs.append(repo_relative_path(paths, doc_file))

    if not check_existing_manifest:
        return missing_docs

    stale_docs = find_docs_without_methods(paths, local_methods)
    if stale_docs:
        raise MetadataError(
            "custom CDP docs without matching metadata method:\n  "
            + "\n  ".join(stale_docs)
        )
    return missing_docs


def find_docs_without_methods(
    paths: Paths, local_methods: Iterable[LocalMethod]
) -> list[str]:
    if not paths.custom_docs_dir.exists():
        return []
    known_methods = {(method.domain, method.method) for method in local_methods}
    stale_docs: list[str] = []
    for doc_file in sorted(paths.custom_docs_dir.glob("*/methods/*.yaml")):
        domain = doc_file.parents[1].name
        method = doc_file.stem
        if (domain, method) not in known_methods:
            stale_docs.append(repo_relative_path(paths, doc_file))
    return stale_docs


def print_update_command() -> None:
    print("Run:", file=sys.stderr)
    print(
        "  tools/env.sh python3 "
        "devtool/lynx_devtool/protocol/scripts/update_cdp_metadata.py --write",
        file=sys.stderr,
    )


def repo_relative_path(paths: Paths, path: Path) -> str:
    return path.resolve().relative_to(paths.source_root).as_posix()


if __name__ == "__main__":
    sys.exit(main())
