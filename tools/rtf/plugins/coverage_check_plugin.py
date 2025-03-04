# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from core.checker.checker_factory import CheckerFactory
from core.utils.log import Log
from plugins.plugin import Plugin


class CoverageCheckerPlugin(Plugin):
    def __init__(self):
        super().__init__("coverage-check")

    def accept(self, args):
        checker = CheckerFactory(args.check_type)
        if checker is None:
            Log.fatal(f"check type `{args.check_type}` is not supported!")
        checker.check(args)

    def help(self):
        return "check coverage"

    def build_command_args(self, subparser):
        subparser.add_argument(
            "--type",
            dest="check_type",
            required=True,
            help="check type, android 、cpp、oc etc.",
        )

        subparser.add_argument(
            "--inputs",
            dest="inputs",
            nargs="*",
            required=True,
            help="Specify the input data",
        )
        subparser.add_argument(
            "--threshold", type=float, default=0.5, help="Input a threshold"
        )

        subparser.add_argument(
            "--count", type=int, default=1, help="check commit numbers"
        )

        subparser.add_argument("--job", default=None)
