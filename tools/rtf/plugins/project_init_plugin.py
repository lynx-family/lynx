# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os

from core.env.env import RTFEnv
from core.template.template import Template
from core.utils.log import Log
from plugins.plugin import Plugin


class InitPlugin(Plugin):
    def __init__(self):
        super().__init__("init")

    def accept(self, args):
        if args.init_type == "project":
            path = args.path
            rtf_file_path = os.path.join(path, ".rtf")
            if os.path.exists(rtf_file_path):
                Log.warning(f"Do not initialize workspace repeatedly!")
                return
            else:
                os.makedirs(rtf_file_path)
                rtf_config_file = os.path.join(rtf_file_path, "config")
                with open(rtf_config_file, "w"):
                    Log.success(f"workspace init success!")
        elif args.init_type == "template":
            template_type = args.template_type
            Template(template_type).save_to(
                RTFEnv.get_env("rtf_root_path"), args.template_name
            )
        else:
            Log.error(f"Unsupported init type {args.init_type}.")

    def help(self):
        return "init a new workspace"

    def build_command_args(self, subparser):
        subparser_subparsers = subparser.add_subparsers(
            title="init_type",
            description="valid init commands",
            dest="init_type",
            required=True,
        )
        subparser_subparser = subparser_subparsers.add_parser(
            "project", help="init project"
        )
        subparser_subparser.add_argument(
            "-p",
            "--path",
            dest="path",
            default=".",
            help="Specify the workspace path (default: '.')",
        )
        subparser_subparser = subparser_subparsers.add_parser(
            "template", help="init template"
        )

        subparser_subparser.add_argument(
            "-t",
            "--type",
            dest="template_type",
            required=True,
            help="Template type to initialize",
        )

        subparser_subparser.add_argument(
            "-n",
            "--name",
            dest="template_name",
            required=True,
            help="Template name to initialize",
        )
