# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from core.coverage.jacoco_coverage import JaCoCoCoverage
from core.coverage.llvm_coverage import LLVMCoverage
from core.base.constants import Constants
from core.base.result import Err, Ok


def CoverageFactory(coverage_params, coverage_file: str = None, coverage_format: str = "text"):
    coverage_type = coverage_params["type"]
    coverage = None
    if coverage_type == "llvm":
        coverage = LLVMCoverage(
            coverage_params["ignores"], coverage_params["output"], coverage_file, coverage_format
        )
    elif coverage_type == "jacoco":
        coverage = JaCoCoCoverage(
            coverage_params["output"],
            coverage_params["jacoco_cli"],
        )

    if coverage is None:
        return Err(
            Constants.COVERAGE_BUILD_ERR, f"Unsupported coverage_type {coverage_type}"
        )
    return Ok(coverage)
