# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from typing import Callable

from core.builder.gn_builder import GnBuilder
from core.builder.gradle_builder import GradleBuilder
from core.target.target import Target
from core.utils.log import Log
from core.base.result import Err, Ok
from core.base.constants import Constants


def BuilderFactory(builder_meta_data: dict, silent: bool = False):
    builder_type = builder_meta_data["type"]
    builder = None
    if builder_type == "gn":
        args = builder_meta_data["args"]
        output = builder_meta_data["output"]
        builder = GnBuilder(args, output, silent)
    elif builder_type == "gradle":
        args = builder_meta_data["args"]
        workspace = builder_meta_data["workspace"]
        builder = GradleBuilder(args, workspace, silent)
    if builder is None:
        return Err(
            Constants.BUILDER_BUILD_ERR, f"build type {builder_type} is unsupported!"
        )
    return Ok(builder)


class BuilderManager:
    def __init__(self, builder_params: dict, silent: bool = False):
        self.builders = {}
        self.builder_params = builder_params
        self.silent = silent

    def pre_action(self, skip: Callable[[], bool] = None):
        for builder_name in self.builder_params.keys():
            builder = BuilderFactory(self.builder_params[builder_name], self.silent)
            if builder.is_err():
                return builder
            else:
                self.builders[builder_name] = builder.get_value()
        for builder in self.builders.values():
            result = builder.pre_action(skip)
            if result.is_err():
                return result
        return Ok(None)

    def build(self, target: Target):
        builder_name = (
            target.params["builder"] if "builder" in target.params else "default"
        )
        return self.builders[builder_name].build(target)
