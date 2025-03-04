# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import copy
import os

from core.container.android_ut_container import AndroidUTContainer
from core.env.context import RTFContext
from core.env.env import RTFEnv
from core.env.trait_template import TraitTemplate
from core.options.options import Options
from plugins.plugin import Plugin


class AndroidUTListener(Options.OptionsListener):
    def __init__(self):
        super().__init__()

    def did_include(self, options, builders, targets, coverage):
        builder_map = {}
        label = self.generate_label(options.workspace)
        tmp_builder = copy.deepcopy(builders)
        for builder_name in tmp_builder.keys():
            builder_new_name = f"{builder_name}_{label}"
            builder_map[builder_name] = builder_new_name
            builders[builder_new_name] = builders[builder_name]
            del builders[builder_name]

        for builder in builders.values():
            builder["workspace"] = os.path.join(options.workspace, builder["workspace"])

        for target in targets.values():
            target["source_files"] = os.path.join(
                options.workspace, target["source_files"]
            )
            target["class_files"] = os.path.join(
                options.workspace, target["class_files"]
            )
            target["apk"] = os.path.join(options.workspace, target["apk"])
            if "builder" in target:
                target["builder"] = builder_map[target["builder"]]
            else:
                target["builder"] = builder_map["default"]


class AndroidUTPlugin(Plugin):
    def __init__(self):
        super().__init__("android-ut")

    def accept(self, args):
        template_names = args.names
        for template_name in template_names:
            template = os.path.join(
                RTFEnv.get_env("rtf_root_path"), f"android-ut-{template_name}.template"
            )
            with open(template, "r") as template_file:
                context = RTFContext(RTFEnv.get_project_root_path())
                context.options = Options(context, args.args)
                context.options.register_listener(AndroidUTListener())
                exec(template_file.read(), context.export())
                template = TraitTemplate.trait_from_context(context)
            if args.command == "run":
                self.__handle_run_command(template, args)

    def __handle_run_command(self, local_env, args):
        container = AndroidUTContainer(local_env["builder"], local_env["coverage"])
        container.use_real_device = args.rmd
        container.clean = not args.no_clean
        container.run(local_env["targets"], filter=args.target)

    def help(self):
        return "run targets of android-ut"

    def build_command_args(self, subparser):
        subparser_subparsers = subparser.add_subparsers(
            title="command",
            description="run|list",
            dest="command",
            required=True,
        )

        subparser_subparser = subparser_subparsers.add_parser("run", help="run targets")

        subparser_subparser.add_argument(
            "--names",
            dest="names",
            nargs="*",
            required=True,
            help="Specify the template name",
        )

        subparser_subparser.add_argument(
            "--target",
            dest="target",
            default="all",
            help="Specify the target name",
        )

        subparser_subparser.add_argument(
            "--args",
            dest="args",
            default="",
            help='User custom params, eg: --args="args_1=true, args_2=1"',
        )

        subparser_subparser.add_argument(
            "--rmd",
            action="store_true",
            default=False,
            help="run test in real mobile device",
        )

        subparser_subparser.add_argument(
            "--no-clean",
            action="store_true",
            default=False,
            help="Don't clean workspace before run test",
        )

        subparser_subparser = subparser_subparsers.add_parser(
            "list", help="list targets"
        )

        subparser_subparser.add_argument(
            "--names",
            dest="names",
            nargs="*",
            required=True,
            help="Specify the template name",
        )

        subparser_subparser.add_argument(
            "--args",
            dest="args",
            default="",
            help='User custom params, eg: --args="args_1=true, args_2=1"',
        )
