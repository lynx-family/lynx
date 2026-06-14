# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import json
import sys
import os
import re
from metadata_def import BaseObject, BaseMember, BaseParam
from collections import deque
from env_setup import (
    NODE_PATH,
    API_CONFIG,
    LYNX_ROOT_PATH,
    HARMONY_API_PATH,
)
import subprocess
from api_writer import write_api_metadata

PYJSON5_SRC_PATH = os.path.join(
    LYNX_ROOT_PATH,
    "lynx",
    "third_party",
    "binding",
    "idl-codegen",
    "third_party",
    "pyjson5",
    "src",
)
if PYJSON5_SRC_PATH not in sys.path:
    sys.path.insert(0, PYJSON5_SRC_PATH)

import json5


class Parser:
    def __init__(self):
        pass


class HarmonyParser(Parser):
    def __init__(self):
        super().__init__()
        if API_CONFIG.get("harmony") is None:
            self._need_to_parse = False
            return

        self.arkts_parser_path: str = os.path.join(os.path.dirname(__file__), "cli.ts")

        self._need_to_parse = True
        self.command: list[str] = ["npx", "ts-node", self.arkts_parser_path, "--file"]
        if "lynx_so_index_file" in API_CONFIG["harmony"]:
            self._lynx_so_index_file = os.path.join(
                LYNX_ROOT_PATH, API_CONFIG["harmony"]["lynx_so_index_file"]
            )
        else:
            self._lynx_so_index_file = None

        if "lynx_devtool_so_index_file" in API_CONFIG["harmony"]:
            self._lynx_devtool_so_index_file = os.path.join(
                LYNX_ROOT_PATH, API_CONFIG["harmony"]["lynx_devtool_so_index_file"]
            )
        else:
            self._lynx_devtool_so_index_file = None

        self.export_object_dict: dict = {}
        self.metadata_path: list[str] = [
            os.path.join(LYNX_ROOT_PATH, path)
            for path in API_CONFIG["harmony"]["metadata_path"]
        ]
        self.api_file: str = os.path.join(HARMONY_API_PATH, "lynx_harmony.api")

    def _ensure_parser(self) -> bool:
        esbuild_path = os.path.join(LYNX_ROOT_PATH, API_CONFIG["path"]["esbuild"])
        try:
            subprocess.run(
                [
                    esbuild_path,
                    "cli.ts",
                    "--bundle",
                    "--platform=node",
                    "--minify",
                    "--outfile=dist/arkts-parser.js",
                ],
                capture_output=True,
                text=True,
                check=True,
                cwd=os.path.dirname(__file__),
            )
        except subprocess.CalledProcessError as e:
            print(e.stdout)
            print(f"install arkts parser failed: {e.stderr}", file=sys.stderr)
            return False
        return True

    def parse(self):
        self.export_object_dict = self._parse_ets_exports_recursive()
        export_object_list = [
            {"path": path, "exports": exports}
            for path, exports in self.export_object_dict.items()
        ]
        object_in_json = json.dumps(export_object_list, indent=2)

        command = [*self.command, object_in_json]
        try:
            result = subprocess.run(
                command,
                capture_output=True,
                text=True,
                check=True,
                cwd=os.path.dirname(__file__),
            )
        except subprocess.CalledProcessError as e:
            print(e.stdout)
            print(f"generate api metadata failed: {e.stderr}", file=sys.stderr)
            return None

        try:
            data_list = json.loads(result.stdout)
        except json.JSONDecodeError as e:
            print(f"parse generated api metadata failed: {e}", file=sys.stderr)
            return None

        object_list = [BaseObject(**data) for data in data_list]
        for object in object_list:
            object.children = [BaseMember(**member) for member in object.children]
            for member in object.children:
                member.params = (
                    [BaseParam(**param) for param in member.params]
                    if member.params
                    else []
                )
                member.returns = BaseParam(**member.returns) if member.returns else None

        return object_list

    def dump(self) -> bool:
        if not self._need_to_parse:
            return True

        try:
            object_list = self.parse()
        except Exception as e:
            print(f"parse harmony api metadata failed: {e}", file=sys.stderr)
            print(f"Kept existing API metadata file: {self.api_file}", file=sys.stderr)
            return False
        if object_list is None:
            print(f"Kept existing API metadata file: {self.api_file}", file=sys.stderr)
            return False
        return write_api_metadata(self.api_file, object_list, "harmony")

    def _clean_exported_item(self, item_str: str) -> str:
        return item_str.split(" as ")[0].strip()

    def _get_oh_package_main_file(self, oh_package_path: str):
        with open(oh_package_path, "r", encoding="utf-8") as f:
            package_data = json5.loads(f.read())

        if not isinstance(package_data, dict):
            return None

        main_file = package_data.get("main")
        return main_file if isinstance(main_file, str) else None

    def _resolve_absolute_path(
        self, current_file_abs_dir: str, relative_import_path: str
    ):
        path_to_resolve = relative_import_path

        # Skip ohpm package references (e.g. @lynx/lynx)
        if path_to_resolve.startswith("@"):
            return None

        if "liblynx.so" in path_to_resolve:
            return self._lynx_so_index_file
        if "liblynxdevtool.so" in path_to_resolve:
            return self._lynx_devtool_so_index_file

        if (
            not path_to_resolve.endswith(".so")
            and not os.path.splitext(path_to_resolve)[1]
        ):
            path_to_resolve += ".ets"

        return os.path.normpath(os.path.join(current_file_abs_dir, path_to_resolve))

    def _parse_single_file_content_exports(
        self,
        file_content: str,
        current_file_abs_path: str,
        target_exports_map: dict,
        files_to_process_queue: deque,
        processed_abs_paths_set: set,
    ):
        export_pattern = re.compile(
            r"export\s+(\*|\{[\s\S]*?\})\s+from\s+['\"]([^'\"]+)['\"];?"
        )

        current_file_directory = os.path.dirname(current_file_abs_path)

        for match in export_pattern.finditer(file_content):
            exported_specifier = match.group(1).strip()
            path_in_from_clause = match.group(2).strip()
            abs_path_to_potentially_queue = self._resolve_absolute_path(
                current_file_directory, path_in_from_clause
            )

            if abs_path_to_potentially_queue is None:
                continue

            if exported_specifier == "*":
                if abs_path_to_potentially_queue.endswith(".ets"):
                    if os.path.isfile(abs_path_to_potentially_queue):
                        if (
                            abs_path_to_potentially_queue not in files_to_process_queue
                            and abs_path_to_potentially_queue
                            not in processed_abs_paths_set
                        ):
                            files_to_process_queue.append(abs_path_to_potentially_queue)
            else:
                items_str_inside_braces = exported_specifier[1:-1].strip()

                cleaned_exported_objects = []
                if items_str_inside_braces:
                    raw_object_strs = [
                        item.strip()
                        for item in items_str_inside_braces.split(",")
                        if item.strip()
                    ]
                    cleaned_exported_objects = [
                        self._clean_exported_item(obj_str)
                        for obj_str in raw_object_strs
                    ]

                if abs_path_to_potentially_queue not in target_exports_map:
                    target_exports_map[abs_path_to_potentially_queue] = []

                for obj_name in cleaned_exported_objects:
                    if (
                        obj_name
                        not in target_exports_map[abs_path_to_potentially_queue]
                    ):
                        target_exports_map[abs_path_to_potentially_queue].append(
                            obj_name
                        )

    def _parse_ets_exports_recursive(self) -> dict:
        files_to_process_queue = deque()
        for path in self.metadata_path:
            if os.path.isfile(path):
                files_to_process_queue.append(path)
            elif os.path.isdir(path):
                for root, dirs, files in os.walk(path):
                    dirs[:] = [d for d in dirs if d not in ("oh_modules", "build", ".hvigor")]
                    if "oh-package.json5" in files:
                        try:
                            main_file = self._get_oh_package_main_file(
                                os.path.join(root, "oh-package.json5")
                            )
                            if main_file:
                                idx_path = os.path.normpath(os.path.join(root, main_file))
                                if os.path.isfile(idx_path):
                                    files_to_process_queue.append(idx_path)
                        except Exception as e:
                            print(f"Failed to parse oh-package.json5 at {root}: {e}")
            else:
                print(f"'{path}' not found.")
                return {}

        processed_abs_paths = set()
        final_exports_map = {}
        files_to_process_queue = deque(sorted(files_to_process_queue))

        while files_to_process_queue:
            current_abs_path_to_process = files_to_process_queue.popleft()

            if current_abs_path_to_process in processed_abs_paths:
                continue

            processed_abs_paths.add(current_abs_path_to_process)

            try:
                with open(current_abs_path_to_process, "r", encoding="utf-8") as f:
                    file_content = f.read()
                self._parse_single_file_content_exports(
                    file_content,
                    current_abs_path_to_process,
                    final_exports_map,
                    files_to_process_queue,
                    processed_abs_paths,
                )
            except FileNotFoundError:
                print(f"'{current_abs_path_to_process}' not found. Skipping...")
            except Exception as e:
                print(f"'{current_abs_path_to_process}' processing error: {e}")

        return final_exports_map


if __name__ == "__main__":
    parser = HarmonyParser()
    parser.dump()
