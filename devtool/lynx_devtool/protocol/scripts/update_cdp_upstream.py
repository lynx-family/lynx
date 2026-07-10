#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Update or check the vendored Chrome DevTools Protocol schema."""

from __future__ import annotations

import argparse
import difflib
import json
import sys
import urllib.error
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Any


GITHUB_RAW_ROOT = "https://raw.githubusercontent.com/ChromeDevTools/devtools-protocol"
BROWSER_PROTOCOL_PATH = "json/browser_protocol.json"
JS_PROTOCOL_PATH = "json/js_protocol.json"


@dataclass(frozen=True)
class Paths:
    source_root: Path
    protocol_dir: Path
    upstream_schema: Path


class UpstreamError(Exception):
    pass


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Update or check the vendored Chrome DevTools Protocol schema."
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true", help="rewrite protocol.json")
    mode.add_argument("--check", action="store_true", help="check protocol.json")
    parser.add_argument(
        "--revision",
        help=(
            "devtools-protocol git revision to fetch from GitHub. "
            "Do not use this in normal CI."
        ),
    )
    parser.add_argument(
        "--browser-protocol",
        help="local path or URL for upstream json/browser_protocol.json",
    )
    parser.add_argument(
        "--js-protocol",
        help="local path or URL for upstream json/js_protocol.json",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=30.0,
        help="URL fetch timeout in seconds. Defaults to 30.",
    )
    args = parser.parse_args()

    try:
        validate_args(parser, args)
        paths = get_paths()
        sources = get_sources(args)
        if sources is None:
            expected_text = normalize_existing_protocol(paths.upstream_schema)
        else:
            browser_protocol = load_json_source(sources[0], args.timeout)
            js_protocol = load_json_source(sources[1], args.timeout)
            expected_text = build_protocol_json(browser_protocol, js_protocol)

        if args.write:
            paths.upstream_schema.write_text(expected_text, encoding="utf-8")
            print(f"Updated {repo_relative_path(paths, paths.upstream_schema)}")
            return 0

        return check_protocol(paths, expected_text)
    except UpstreamError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


def validate_args(parser: argparse.ArgumentParser, args: argparse.Namespace) -> None:
    has_file_sources = bool(args.browser_protocol or args.js_protocol)
    if args.revision and has_file_sources:
        parser.error("--revision cannot be combined with --browser-protocol or --js-protocol")
    if has_file_sources and not (args.browser_protocol and args.js_protocol):
        parser.error("--browser-protocol and --js-protocol must be provided together")
    if args.write and not args.revision and not has_file_sources:
        parser.error("--write requires --revision or both local upstream protocol files")
    if args.timeout <= 0:
        parser.error("--timeout must be positive")


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
    )


def get_sources(args: argparse.Namespace) -> tuple[str, str] | None:
    if args.revision:
        raw_root = f"{GITHUB_RAW_ROOT}/{args.revision}"
        return (
            f"{raw_root}/{BROWSER_PROTOCOL_PATH}",
            f"{raw_root}/{JS_PROTOCOL_PATH}",
        )
    if args.browser_protocol and args.js_protocol:
        return (args.browser_protocol, args.js_protocol)
    return None


def load_json_source(source: str, timeout: float) -> dict[str, Any]:
    if source.startswith(("http://", "https://")):
        try:
            with urllib.request.urlopen(source, timeout=timeout) as response:
                data = response.read()
        except urllib.error.URLError as exc:
            raise UpstreamError(f"failed to fetch {source}: {exc}") from exc
        try:
            return json.loads(data.decode("utf-8"))
        except UnicodeDecodeError as exc:
            raise UpstreamError(f"{source} is not valid UTF-8: {exc}") from exc
        except json.JSONDecodeError as exc:
            raise UpstreamError(f"{source} is not valid JSON: {exc}") from exc

    path = Path(source)
    if not path.exists():
        raise UpstreamError(f"missing upstream protocol file: {source}")
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise UpstreamError(f"{source} is not valid JSON: {exc}") from exc


def normalize_existing_protocol(schema_path: Path) -> str:
    if not schema_path.exists():
        raise UpstreamError(
            f"missing vendored CDP schema: {schema_path}\n"
            "Run update_cdp_upstream.py --write with --revision or local protocol files."
        )
    try:
        schema = json.loads(schema_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise UpstreamError(f"{schema_path} is not valid JSON: {exc}") from exc
    validate_protocol_schema(schema, str(schema_path))
    return canonical_json_text(
        {
            "version": schema["version"],
            "domains": schema["domains"],
        }
    )


def build_protocol_json(
    browser_protocol: dict[str, Any], js_protocol: dict[str, Any]
) -> str:
    validate_protocol_schema(browser_protocol, BROWSER_PROTOCOL_PATH)
    validate_protocol_schema(js_protocol, JS_PROTOCOL_PATH)

    browser_version = browser_protocol["version"]
    js_version = js_protocol["version"]
    if browser_version != js_version:
        raise UpstreamError(
            "browser and JS protocol versions differ: "
            f"{browser_version!r} != {js_version!r}"
        )

    domains = browser_protocol["domains"] + js_protocol["domains"]
    duplicate_domains = sorted(find_duplicate_domains(domains))
    if duplicate_domains:
        raise UpstreamError(
            "duplicate upstream CDP domains: " + ", ".join(duplicate_domains)
        )

    return canonical_json_text(
        {
            "version": browser_version,
            "domains": domains,
        }
    )


def validate_protocol_schema(schema: Any, label: str) -> None:
    if not isinstance(schema, dict):
        raise UpstreamError(f"{label} must contain a JSON object")
    version = schema.get("version")
    if not isinstance(version, dict):
        raise UpstreamError(f"{label} must contain a version object")
    domains = schema.get("domains")
    if not isinstance(domains, list):
        raise UpstreamError(f"{label} must contain a domains array")
    for index, domain_entry in enumerate(domains):
        if not isinstance(domain_entry, dict):
            raise UpstreamError(f"{label} domains[{index}] must be an object")
        domain = domain_entry.get("domain")
        if not isinstance(domain, str):
            raise UpstreamError(f"{label} domains[{index}] must contain domain")


def find_duplicate_domains(domains: list[dict[str, Any]]) -> set[str]:
    seen: set[str] = set()
    duplicates: set[str] = set()
    for domain_entry in domains:
        domain = domain_entry["domain"]
        if domain in seen:
            duplicates.add(domain)
        seen.add(domain)
    return duplicates


def canonical_json_text(value: dict[str, Any]) -> str:
    return json.dumps(value, ensure_ascii=False, indent=2) + "\n"


def check_protocol(paths: Paths, expected: str) -> int:
    if not paths.upstream_schema.exists():
        print("Vendored CDP schema is missing.", file=sys.stderr)
        print_update_command()
        return 1

    actual = paths.upstream_schema.read_text(encoding="utf-8")
    if actual == expected:
        return 0

    print("Vendored CDP schema is stale or not normalized.", file=sys.stderr)
    print_update_command()
    print(file=sys.stderr)
    for line in difflib.unified_diff(
        actual.splitlines(),
        expected.splitlines(),
        fromfile=repo_relative_path(paths, paths.upstream_schema),
        tofile=f"{repo_relative_path(paths, paths.upstream_schema)} (expected)",
        lineterm="",
    ):
        print(line, file=sys.stderr)
    return 1


def print_update_command() -> None:
    print("Run:", file=sys.stderr)
    print(
        "  tools/env.sh python3 "
        "devtool/lynx_devtool/protocol/scripts/update_cdp_upstream.py "
        "--write --revision <devtools-protocol-commit>",
        file=sys.stderr,
    )


def repo_relative_path(paths: Paths, path: Path) -> str:
    return path.resolve().relative_to(paths.source_root).as_posix()


if __name__ == "__main__":
    sys.exit(main())
