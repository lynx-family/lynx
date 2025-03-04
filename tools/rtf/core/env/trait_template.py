# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from core.env.context import RTFContext
from core.utils.log import Log


class Merge:
    def __init__(self):
        pass

    @staticmethod
    def merge_builder(old_builder, cur_builder):
        new_builder = {}
        for name in old_builder.keys():
            if name in cur_builder.keys():
                Log.fatal(f"Builder name duplicated ·{name}·")
            new_builder[name] = old_builder[name]
        new_builder.update(cur_builder)
        return new_builder

    @staticmethod
    def merge_coverage(old_coverage, cur_coverage):
        if old_coverage["type"] != cur_coverage["type"]:
            Log.fatal(
                f"Different types of coverage objects ({old_coverage['type']} -> {cur_coverage['type']}) "
                f"can't be merged!"
            )
        if "ignores" in old_coverage:
            cur_coverage["ignores"] += old_coverage["ignores"]
        return cur_coverage

    @staticmethod
    def merge_targets(old_targets, cur_targets):
        new_targets = {}
        for name in old_targets.keys():
            if name in cur_targets.keys():
                if "fake" in cur_targets[name] and cur_targets[name]["fake"]:
                    new_targets[name] = old_targets[name]
                    new_targets[name].update(cur_targets[name])
                    cur_targets.pop(name)
                else:
                    Log.fatal(f"Target name duplicated ·{name}·")
            else:
                new_targets[name] = old_targets[name]
        new_targets.update(cur_targets)
        return new_targets

    @staticmethod
    def merge_template(old_template, cur_template):
        new_template = cur_template
        new_template["builder"] = Merge.merge_builder(
            old_template["builder"], cur_template["builder"]
        )
        new_template["targets"] = Merge.merge_targets(
            old_template["targets"], cur_template["targets"]
        )
        new_template["coverage"] = Merge.merge_coverage(
            old_template["coverage"], cur_template["coverage"]
        )
        return new_template


class TraitTemplate:
    @staticmethod
    def trait_from_context(context: RTFContext):
        template = {
            "builder": context.find_variable("builder"),
            "coverage": context.find_variable("coverage"),
            "targets": context.find_variable("targets"),
        }
        if len(context.children) == 0:
            return template

        for child in context.children:
            child_template = TraitTemplate.trait_from_context(child)
            template = Merge.merge_template(child_template, template)

        return template
