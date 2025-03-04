# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
from plugins.plugin import Plugin


class ArgsParser:
    def __init__(self):
        self.parser = argparse.ArgumentParser(description="Reusable Testing Framework.")
        self.subparsers = self.parser.add_subparsers(
            dest="plugin", help="Available plugins"
        )
        self.parser.add_argument(
            "-v",
            "--version",
            action="version",
            version="RTF 0.1",
            help="print the RTF version number and exit (also --version)",
        )

    def init_subparsers(self, plugins: dict[str, Plugin]):
        for name in plugins:
            subparser = self.subparsers.add_parser(name, help=plugins[name].help())
            plugins[name].build_command_args(subparser)

    def parse_args(self):
        return self.parser.parse_args()

    def print_help(self):
        self.parser.print_help()
